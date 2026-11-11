// Copyright (c) 2014-2017 The The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/* binary_archive.h
 *
 * Portable (low-endian) binary archive */
#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <boost/type_traits/make_unsigned.hpp>

#include "common/varint.h"
#include "warnings.h"

PUSH_VS_WARNINGS
DISABLE_VS_WARNINGS(4244)
DISABLE_VS_WARNINGS(4100)

//TODO: fix size_t warning in x32 platform

// Opt-in budget for decoding untrusted binary blobs. An archive without a
// budget keeps the legacy behavior exactly. Charges are cumulative REQUESTED
// storage/work, taken before the corresponding allocation; freed storage is
// not refunded. This is an approximate allocation budget, not an RSS bound.
// Maxima are inclusive: reaching a maximum exactly is accepted.
// Passive source-fixed first refusal only; never decoded values or requested counts.
enum class binary_decode_refusal { Unobserved, Arithmetic, Allocation, Collection };
struct binary_decode_budget
{
  uint64_t max_bytes = 0;
  uint64_t max_elements = 0;
  uint64_t bytes = 0;
  uint64_t elements = 0;
  // Cooperative cancellation, polled on every charge. Borrowed; never owned.
  bool (*should_stop)(void*) = nullptr;
  void* stop_context = nullptr;
  bool exhausted = false; // A limit was refused (not cancellation).
  bool cancelled = false;
  binary_decode_refusal first_refusal = binary_decode_refusal::Unobserved;

  bool charge(uint64_t count, uint64_t unit, uint64_t element_count) noexcept
  {
    if (exhausted || cancelled)
      return false;
    if (should_stop && should_stop(stop_context))
    {
      cancelled = true;
      return false;
    }
    if (unit != 0 && count > std::numeric_limits<uint64_t>::max() / unit)
      return refuse(binary_decode_refusal::Arithmetic);
    const uint64_t total = count * unit;
    if (bytes > max_bytes || total > max_bytes - bytes)
      return refuse(binary_decode_refusal::Allocation);
    if (elements > max_elements || element_count > max_elements - elements)
      return refuse(binary_decode_refusal::Collection);
    bytes += total;
    elements += element_count;
    return true;
  }
private:
  bool refuse(binary_decode_refusal context) noexcept {
    exhausted = true;
    if (first_refusal == binary_decode_refusal::Unobserved) first_refusal = context;
    return false;
  }
};

template <class Stream, bool IsSaving>
struct binary_archive_base
{
  typedef Stream stream_type;
  typedef binary_archive_base<Stream, IsSaving> base_type;
  typedef boost::mpl::bool_<IsSaving> is_saving;

  typedef uint8_t variant_tag_type;

  explicit binary_archive_base(stream_type &s) : stream_(s) { }

  bool is_saving_arch(){ return IsSaving; }
  void tag(const char *) { }
  void begin_object() { }
  void end_object() { }
  void begin_variant() { }
  void end_variant() { }
  stream_type &stream() { return stream_; }
protected:
  stream_type &stream_;
};

template <bool W>
struct binary_archive;

template <>
struct binary_archive<false> : public binary_archive_base<std::istream, false>
{
  explicit binary_archive(stream_type &s) : base_type(s) {
    stream_type::pos_type pos = stream_.tellg();
    stream_.seekg(0, std::ios_base::end);
    eof_pos_ = stream_.tellg();
    stream_.seekg(pos);
  }

  template <class T>
  void serialize_int(T &v)
  {
    serialize_uint(*(typename boost::make_unsigned<T>::type *)&v);
  }


  template <class T>
  void serialize_uint(T &v, size_t width = sizeof(T))
  {
    T ret = 0;
    unsigned shift = 0;
    for (size_t i = 0; i < width; i++) {
      //std::cerr << "tell: " << stream_.tellg() << " value: " << ret << std::endl;
      char c;
      stream_.get(c);
      T b = (unsigned char)c;
      ret += (b << shift);
      shift += 8;
    }
    v = ret;
  }
  void serialize_blob(void *buf, size_t len, const char *delimiter="") { 
    stream_.read((char *)buf, len);
  }

  template <class T>
  void serialize_varint(T &v)
  {
    serialize_uvarint(*(typename boost::make_unsigned<T>::type *)(&v));
  }

  template <class T>
  void serialize_uvarint(T &v)
  {
    if (budget_)
    {
      strict_read_uvarint(v);
      return;
    }
    typedef std::istreambuf_iterator<char> it;
    tools::read_varint(it(stream_), it(), v); // XXX handle failure
  }
  void begin_array(size_t &s)
  {
    serialize_varint(s);
  }
  void begin_array() { }

  void delimit_array() { }
  void end_array() { }

  void begin_string(const char *delimiter="\"") { }
  void end_string(const char *delimiter="\"") { }

  void read_variant_tag(variant_tag_type &t) {
    serialize_int(t);
  }

  size_t remaining_bytes() {
    if (!stream_.good())
      return 0;
    //std::cerr << "tell: " << stream_.tellg() << std::endl;
    assert(stream_.tellg() <= eof_pos_);
    return eof_pos_ - stream_.tellg();
  }

  // Borrowed for this archive's lifetime. Setting a budget selects strict mode.
  void set_decode_budget(binary_decode_budget* budget) { budget_ = budget; }
  binary_decode_budget* decode_budget() const { return budget_; }
protected:
  // Same encoding as tools::read_varint, but truncation, overflow and
  // non-canonical forms fail the stream instead of yielding a partial value.
  template <class T>
  void strict_read_uvarint(T &v)
  {
    char bytes[(std::numeric_limits<T>::digits + 6) / 7];
    size_t count = 0;
    for (;;)
    {
      const int c = stream_.get();
      if (c == std::char_traits<char>::eof() || count == sizeof(bytes))
      {
        stream_.setstate(std::ios::failbit);
        return;
      }
      bytes[count++] = static_cast<char>(c);
      if ((c & 0x80) == 0)
        break;
    }
    const unsigned char* first = reinterpret_cast<const unsigned char*>(bytes);
    if (tools::read_varint(static_cast<const unsigned char*>(first),
          static_cast<const unsigned char*>(first + count), v) != static_cast<int>(count))
      stream_.setstate(std::ios::failbit);
  }

  std::streamoff eof_pos_;
  binary_decode_budget* budget_ = nullptr;
};

// Budget hooks used by shared container/string/variant/conversion readers.
// Archives other than a budgeted binary_archive<false> are never charged.
template <class Archive>
inline bool serialization_budget_strict(Archive&) { return false; }
inline bool serialization_budget_strict(binary_archive<false>& ar) { return ar.decode_budget() != nullptr; }

template <class Archive>
inline bool serialization_budget_charge(Archive&, uint64_t, uint64_t, uint64_t) { return true; }
inline bool serialization_budget_charge(binary_archive<false>& ar, uint64_t count, uint64_t unit, uint64_t element_count)
{
  binary_decode_budget* budget = ar.decode_budget();
  if (!budget || budget->charge(count, unit, element_count))
    return true;
  ar.stream().setstate(std::ios::failbit);
  return false;
}

template <>
struct binary_archive<true> : public binary_archive_base<std::ostream, true>
{
  explicit binary_archive(stream_type &s) : base_type(s) { }

  template <class T>
  void serialize_int(T v)
  {
    serialize_uint(static_cast<typename boost::make_unsigned<T>::type>(v));
  }
  template <class T>
  void serialize_uint(T v)
  {
    for (size_t i = 0; i < sizeof(T); i++) {
      stream_.put((char)(v & 0xff));
      if (1 < sizeof(T)) {
        v >>= 8;
      }
    }
  }
  void serialize_blob(void *buf, size_t len, const char *delimiter="") { stream_.write((char *)buf, len); }

  template <class T>
  void serialize_varint(T &v)
  {
    serialize_uvarint(*(typename boost::make_unsigned<T>::type *)(&v));
  }

  template <class T>
  void serialize_uvarint(T &v)
  {
    typedef std::ostreambuf_iterator<char> it;
    tools::write_varint(it(stream_), v);
  }
  void begin_array(size_t s)
  {
    serialize_varint(s);
  }
  void begin_array() { }
  void delimit_array() { }
  void end_array() { }

  void begin_string(const char *delimiter="\"") { }
  void end_string(const char *delimiter="\"") { }

  void write_variant_tag(variant_tag_type t) {
    serialize_int(t);
  }

  // Strict opt-in for bounded hashing only: saving transitions may reserve
  // their exact destination size instead of growing it. Bytes are identical;
  // an archive that never sets this keeps the legacy behavior exactly.
  void set_strict_save(bool strict) { strict_save_ = strict; }
  bool strict_save() const { return strict_save_; }
private:
  bool strict_save_ = false;
};

template <class Archive>
inline bool serialization_save_strict(Archive&) { return false; }
inline bool serialization_save_strict(binary_archive<true>& ar) { return ar.strict_save(); }

POP_VS_WARNINGS
