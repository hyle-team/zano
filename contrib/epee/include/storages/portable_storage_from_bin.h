// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 



#pragma once 

#include <limits>
#include "misc_language.h"
#include "portable_storage_base.h"

#ifdef EPEE_PORTABLE_STORAGE_RECURSION_LIMIT
#define EPEE_PORTABLE_STORAGE_RECURSION_LIMIT_INTERNAL EPEE_PORTABLE_STORAGE_RECURSION_LIMIT
#else 
#define EPEE_PORTABLE_STORAGE_RECURSION_LIMIT_INTERNAL 100
#endif

namespace epee
{
  namespace serialization
  {
    struct portable_storage_limits
    {
      size_t max_storage_entries_count = 1000000;
      size_t max_array_elements_count = 1000000;
      size_t max_sections_entries_count = 1000000;
      // Opt-in strict mode; legacy callers leave it false and keep the historic
      // exclusive, post-load accounting. In strict mode the three maxima above
      // are INCLUSIVE aggregate totals charged before insertion/loading, and
      // nesting depth, string length, duplicate keys, trailing bytes and
      // cooperative cancellation are enforced as well.
      bool strict = false;
      size_t max_nesting_depth = 0;  // sections and arrays, root section = 1
      size_t max_string_length = 0;  // bytes, inclusive
      bool (*should_stop)(void*) = nullptr; // borrowed; polled periodically
      void* stop_context = nullptr;
    };

    inline static constexpr portable_storage_limits gdefault_portable_storage_limits{};



    struct throwable_buffer_reader
    {
      throwable_buffer_reader(const void* ptr, size_t sz, const portable_storage_limits& limits);
      void read(void* target, size_t count);
      void read_sec_name(std::string& sce_name);
      template<class t_pod_type>
      void read(t_pod_type& pod_val);
      template<class t_type>
      t_type read();
      template<class type_name>
      storage_entry read_ae();
      storage_entry load_storage_array_entry(uint8_t type);
      uint64_t read_varint();
      template<class t_type>
      storage_entry read_se();
      storage_entry load_storage_entry();
      void read(section& sec);
      void read(std::string& str);
      void read(array_entry &ae);
      size_t remaining() const { return m_count; }


      size_t m_storage_entries_count = 0;
      size_t m_arrays_count = 0;
      size_t m_array_elements_count = 0;
      size_t m_sections_entries_count = 0;

      portable_storage_limits m_limits = gdefault_portable_storage_limits;
    private:
      // Strict-only helpers. They load values in place because array_entry_t
      // (portable_storage_base.h) has no move constructor, so returning an
      // array by value would deep-copy every element again.
      void strict_read_section(section& sec);
      void strict_load_entry(storage_entry& target);
      template<class t_type>
      void strict_load_array(storage_entry& target);
      // Array elements; bool goes through strict_read_bool().
      template<class t_type>
      void strict_read_value(t_type& value) { read(value); }
      void strict_read_value(bool& value) { value = strict_read_bool(); }
      // Reads the byte as uint8_t and rejects anything but 0/1 before a bool
      // is formed, so no invalid bool representation is ever created.
      bool strict_read_bool();
      struct strict_depth_scope
      {
        throwable_buffer_reader& m_owner;
        explicit strict_depth_scope(throwable_buffer_reader& owner) : m_owner(owner)
        {
          CHECK_AND_ASSERT_THROW_MES(m_owner.m_depth < m_owner.m_limits.max_nesting_depth, "Wrong blob data in portable storage: nesting limit exceeded");
          ++m_owner.m_depth;
        }
        ~strict_depth_scope() { --m_owner.m_depth; }
      };
      size_t m_depth = 0;
      size_t m_strict_polls = 0;

      struct mem_limitation_guard
      {
        size_t& m_counter_ref;
        throwable_buffer_reader& m_owner;
        mem_limitation_guard(size_t& counter, throwable_buffer_reader& owner):m_counter_ref(counter), m_owner(owner)
        {
          ++m_counter_ref;
          CHECK_AND_ASSERT_THROW_MES(m_counter_ref < EPEE_PORTABLE_STORAGE_RECURSION_LIMIT_INTERNAL, "Wrong blob data in portable storage: recursion limitation (" << EPEE_PORTABLE_STORAGE_RECURSION_LIMIT_INTERNAL << ") exceeded");
          if (m_owner.m_limits.strict)
          {
            // Strict totals are inclusive and charged at their allocation
            // sites; here only poll cancellation, every 256 reader steps.
            const portable_storage_limits& l = m_owner.m_limits;
            CHECK_AND_ASSERT_THROW_MES((++m_owner.m_strict_polls & 0xff) != 0 || !l.should_stop || !l.should_stop(l.stop_context), "portable storage: decoding cancelled");
            return;
          }

          CHECK_AND_ASSERT_THROW_MES(m_owner.m_storage_entries_count < m_owner.m_limits.max_storage_entries_count, "Wrong blob data in portable storage: max_storage_entries_count (" << m_owner.m_limits.max_storage_entries_count << ") exceeded");
          CHECK_AND_ASSERT_THROW_MES(m_owner.m_array_elements_count < m_owner.m_limits.max_array_elements_count, "Wrong blob data in portable storage: max_array_elements_count (" << m_owner.m_limits.max_array_elements_count << ") exceeded");
          CHECK_AND_ASSERT_THROW_MES(m_owner.m_sections_entries_count < m_owner.m_limits.max_sections_entries_count, "Wrong blob data in portable storage: max_sections_entries_count (" << m_owner.m_limits.max_sections_entries_count << ") exceeded");
        }
        ~mem_limitation_guard() noexcept(false)
        {
          CHECK_AND_ASSERT_THROW_MES(m_counter_ref != 0, "Internal error: m_counter_ref == 0 while ~mem_limitation_guard()");
          --m_counter_ref;
        }
      };
#define CHECK_MEM_LIMITATIONS()  mem_limitation_guard rl(m_recursion_count, *this)

      const uint8_t* m_ptr;
      size_t m_count;
      size_t m_recursion_count;

    };

    inline throwable_buffer_reader::throwable_buffer_reader(const void* ptr, size_t sz, const portable_storage_limits& limits) : m_limits(limits)
    {
      if(!ptr) 
        throw std::runtime_error("throwable_buffer_reader: ptr==nullptr");
      if(!sz)
        throw std::runtime_error("throwable_buffer_reader: sz==0");
      m_ptr = (uint8_t*)ptr;
      m_count = sz;
      m_recursion_count = 0;
    }
    inline 
    void throwable_buffer_reader::read(void* target, size_t count)
    {
      CHECK_MEM_LIMITATIONS();
      CHECK_AND_ASSERT_THROW_MES(m_count >= count, " attempt to read " << count << " bytes from buffer with " << m_count << " bytes remained");
      memcpy(target, m_ptr, count);
      m_ptr += count;
      m_count -= count;
    }
    inline 
    void throwable_buffer_reader::read_sec_name(std::string& sce_name)
    {
      CHECK_MEM_LIMITATIONS();
      uint8_t name_len = 0;
      read(name_len);
      sce_name.resize(name_len);
      read((void*)sce_name.data(), name_len);
    }

    template<class t_pod_type>
    void throwable_buffer_reader::read(t_pod_type& pod_val)
    {
      CHECK_MEM_LIMITATIONS();
      static_assert(std::is_pod<t_pod_type>::value, "POD type expected");
      read(&pod_val, sizeof(pod_val));
    }
    
    template<class t_type>
    t_type throwable_buffer_reader::read()
    {
      CHECK_MEM_LIMITATIONS();
      t_type v;
      read(v);
      return v;
    }


    template<class type_name>
    storage_entry throwable_buffer_reader::read_ae()
    {
      CHECK_MEM_LIMITATIONS();
      //for pod types
      array_entry_t<type_name> sa;
      uint64_t size = read_varint();
      CHECK_AND_ASSERT_THROW_MES(size < this->m_limits.max_array_elements_count && size < this->m_limits.max_array_elements_count - this->m_array_elements_count,
                "Wrong blob data ...: max_array_elements_count exceeded (size=" << size << ")");

      size_t loaded_items_count = 0;
      //TODO: add some optimization here later
      while (size--)
      {
         sa.m_array.push_back(read<type_name>());  
         loaded_items_count++;
      }
        
      m_array_elements_count += loaded_items_count;
      m_arrays_count++;
      return storage_entry(array_entry(sa));
    }

    inline 
    storage_entry throwable_buffer_reader::load_storage_array_entry(uint8_t type)
    {
      CHECK_MEM_LIMITATIONS();
      type &= ~SERIALIZE_FLAG_ARRAY;
      switch(type)
      {
      case SERIALIZE_TYPE_INT64:  return read_ae<int64_t>();
      case SERIALIZE_TYPE_INT32:  return read_ae<int32_t>();
      case SERIALIZE_TYPE_INT16:  return read_ae<int16_t>();
      case SERIALIZE_TYPE_INT8:   return read_ae<int8_t>();
      case SERIALIZE_TYPE_UINT64: return read_ae<uint64_t>();
      case SERIALIZE_TYPE_UINT32: return read_ae<uint32_t>();
      case SERIALIZE_TYPE_UINT16: return read_ae<uint16_t>();
      case SERIALIZE_TYPE_UINT8:  return read_ae<uint8_t>();
      case SERIALIZE_TYPE_DUOBLE: return read_ae<double>();
      case SERIALIZE_TYPE_BOOL:   return read_ae<bool>();
      case SERIALIZE_TYPE_STRING: return read_ae<std::string>();
      case SERIALIZE_TYPE_OBJECT: return read_ae<section>();
      case SERIALIZE_TYPE_ARRAY:  return read_ae<array_entry>();
      default: 
        CHECK_AND_ASSERT_THROW_MES(false, "unknown entry_type code = " << type);
      }
    }

    inline 
    uint64_t throwable_buffer_reader::read_varint()
    {
      CHECK_MEM_LIMITATIONS();
      CHECK_AND_ASSERT_THROW_MES(m_count >= 1, "empty buff, expected place for varint");
      uint64_t v = 0;
      uint8_t size_mask = (*(uint8_t*)m_ptr) &PORTABLE_RAW_SIZE_MARK_MASK;
      switch (size_mask)
      {
      case PORTABLE_RAW_SIZE_MARK_BYTE: v = read<uint8_t>();break;
      case PORTABLE_RAW_SIZE_MARK_WORD: v = read<uint16_t>();break;
      case PORTABLE_RAW_SIZE_MARK_DWORD: v = read<uint32_t>();break;
      case PORTABLE_RAW_SIZE_MARK_INT64: v = read<uint64_t>();break;
      default:
        CHECK_AND_ASSERT_THROW_MES(false, "unknown varint size_mask = " << size_mask);
      }
      v >>= 2;
      return v;
    }

    template<class t_type>
    storage_entry throwable_buffer_reader::read_se()
    {
      CHECK_MEM_LIMITATIONS();
      t_type v;
      read(v);
      return storage_entry(v);
    }

    template<>
    inline storage_entry throwable_buffer_reader::read_se<std::string>()
    {
      CHECK_MEM_LIMITATIONS();
      return storage_entry(read<std::string>());
    }


    template<>
    inline storage_entry throwable_buffer_reader::read_se<section>()
    {
      CHECK_MEM_LIMITATIONS();
      section s;//use extra variable due to vs bug, line "storage_entry se(section()); " can't be compiled in visual studio
      storage_entry se(s);
      section& section_entry = boost::get<section>(se);
      read(section_entry);
      return se;
    }

    template<>
    inline storage_entry throwable_buffer_reader::read_se<array_entry>()
    {
      CHECK_MEM_LIMITATIONS();
      uint8_t ent_type = 0;
      read(ent_type);
      CHECK_AND_ASSERT_THROW_MES(ent_type&SERIALIZE_FLAG_ARRAY, "wrong type sequenses");
      return load_storage_array_entry(ent_type);
    }

    inline 
    storage_entry throwable_buffer_reader::load_storage_entry()
    {
      CHECK_MEM_LIMITATIONS();
      uint8_t ent_type = 0;
      read(ent_type);
      if(ent_type&SERIALIZE_FLAG_ARRAY)
        return load_storage_array_entry(ent_type);

      switch(ent_type)
      {
      case SERIALIZE_TYPE_INT64:  return read_se<int64_t>();
      case SERIALIZE_TYPE_INT32:  return read_se<int32_t>();
      case SERIALIZE_TYPE_INT16:  return read_se<int16_t>();
      case SERIALIZE_TYPE_INT8:   return read_se<int8_t>();
      case SERIALIZE_TYPE_UINT64: return read_se<uint64_t>();
      case SERIALIZE_TYPE_UINT32: return read_se<uint32_t>();
      case SERIALIZE_TYPE_UINT16: return read_se<uint16_t>();
      case SERIALIZE_TYPE_UINT8:  return read_se<uint8_t>();
      case SERIALIZE_TYPE_DUOBLE: return read_se<double>();
      case SERIALIZE_TYPE_BOOL:   return read_se<bool>();
      case SERIALIZE_TYPE_STRING: return read_se<std::string>();
      case SERIALIZE_TYPE_OBJECT: return read_se<section>();
      case SERIALIZE_TYPE_ARRAY:  return read_se<array_entry>();
      default: 
        CHECK_AND_ASSERT_THROW_MES(false, "unknown entry_type code = " << ent_type);
      }
    }
    inline
    void throwable_buffer_reader::strict_read_section(section& sec)
    {
      strict_depth_scope depth(*this);
      CHECK_AND_ASSERT_THROW_MES(m_sections_entries_count < m_limits.max_sections_entries_count, "Wrong blob data in portable storage: max_sections_entries_count exceeded");
      ++m_sections_entries_count;
      sec.m_entries.clear();
      uint64_t count = read_varint();
      // Every entry needs at least a name length and a type byte.
      CHECK_AND_ASSERT_THROW_MES(count <= m_count / 2, "Wrong blob data in portable storage: section entry count exceeds input");
      while (count--)
      {
        std::string name;
        read_sec_name(name);
        CHECK_AND_ASSERT_THROW_MES(m_storage_entries_count < m_limits.max_storage_entries_count, "Wrong blob data in portable storage: max_storage_entries_count exceeded");
        ++m_storage_entries_count;
        auto inserted = sec.m_entries.emplace(std::move(name), storage_entry());
        CHECK_AND_ASSERT_THROW_MES(inserted.second, "Wrong blob data in portable storage: duplicate entry name");
        strict_load_entry(inserted.first->second);
      }
    }

    template<class t_type>
    void throwable_buffer_reader::strict_load_array(storage_entry& target)
    {
      uint64_t size = read_varint();
      // Every element needs at least one input byte; charge before loading.
      CHECK_AND_ASSERT_THROW_MES(size <= m_count && m_array_elements_count <= m_limits.max_array_elements_count &&
        size <= m_limits.max_array_elements_count - m_array_elements_count, "Wrong blob data in portable storage: max_array_elements_count exceeded");
      m_array_elements_count += static_cast<size_t>(size);
      const array_entry_t<t_type> empty; // lvalue, as in read_ae(): array_entry lists some types twice
      target = array_entry(empty);
      std::list<t_type>& items = boost::get<array_entry_t<t_type>>(boost::get<array_entry>(target)).m_array;
      for (; size != 0; --size)
      {
        items.emplace_back();
        strict_read_value(items.back());
      }
      m_arrays_count++;
    }

    inline
    bool throwable_buffer_reader::strict_read_bool()
    {
      const uint8_t byte = read<uint8_t>();
      CHECK_AND_ASSERT_THROW_MES(byte <= 1, "Wrong blob data in portable storage: invalid bool value");
      return byte != 0;
    }

    inline
    void throwable_buffer_reader::strict_load_entry(storage_entry& target)
    {
      CHECK_MEM_LIMITATIONS();
      uint8_t ent_type = 0;
      read(ent_type);
      if (ent_type & SERIALIZE_FLAG_ARRAY)
      {
        strict_depth_scope depth(*this);
        switch (ent_type & ~SERIALIZE_FLAG_ARRAY)
        {
        case SERIALIZE_TYPE_INT64:  return strict_load_array<int64_t>(target);
        case SERIALIZE_TYPE_INT32:  return strict_load_array<int32_t>(target);
        case SERIALIZE_TYPE_INT16:  return strict_load_array<int16_t>(target);
        case SERIALIZE_TYPE_INT8:   return strict_load_array<int8_t>(target);
        case SERIALIZE_TYPE_UINT64: return strict_load_array<uint64_t>(target);
        case SERIALIZE_TYPE_UINT32: return strict_load_array<uint32_t>(target);
        case SERIALIZE_TYPE_UINT16: return strict_load_array<uint16_t>(target);
        case SERIALIZE_TYPE_UINT8:  return strict_load_array<uint8_t>(target);
        case SERIALIZE_TYPE_DUOBLE: return strict_load_array<double>(target);
        case SERIALIZE_TYPE_BOOL:   return strict_load_array<bool>(target);
        case SERIALIZE_TYPE_STRING: return strict_load_array<std::string>(target);
        case SERIALIZE_TYPE_OBJECT: return strict_load_array<section>(target);
        default:
          // Includes arrays of arrays, which the legacy reader cannot load either.
          CHECK_AND_ASSERT_THROW_MES(false, "Wrong blob data in portable storage: unsupported array type");
        }
      }
      switch (ent_type)
      {
      case SERIALIZE_TYPE_INT64:  target = read<int64_t>(); return;
      case SERIALIZE_TYPE_INT32:  target = read<int32_t>(); return;
      case SERIALIZE_TYPE_INT16:  target = read<int16_t>(); return;
      case SERIALIZE_TYPE_INT8:   target = read<int8_t>(); return;
      case SERIALIZE_TYPE_UINT64: target = read<uint64_t>(); return;
      case SERIALIZE_TYPE_UINT32: target = read<uint32_t>(); return;
      case SERIALIZE_TYPE_UINT16: target = read<uint16_t>(); return;
      case SERIALIZE_TYPE_UINT8:  target = read<uint8_t>(); return;
      case SERIALIZE_TYPE_DUOBLE: target = read<double>(); return;
      case SERIALIZE_TYPE_BOOL:   target = strict_read_bool(); return;
      case SERIALIZE_TYPE_STRING: target = std::string(); read(boost::get<std::string>(target)); return;
      case SERIALIZE_TYPE_OBJECT: target = section(); read(boost::get<section>(target)); return;
      default:
        CHECK_AND_ASSERT_THROW_MES(false, "Wrong blob data in portable storage: unsupported entry type");
      }
    }

    inline
    void throwable_buffer_reader::read(section& sec)
    {
      CHECK_MEM_LIMITATIONS();
      if (m_limits.strict)
        return strict_read_section(sec);
      sec.m_entries.clear();
      uint64_t count = read_varint();
      while(count--)
      {
        //read section name string
        std::string sec_name;
        read_sec_name(sec_name);
        sec.m_entries.insert(std::make_pair(sec_name, load_storage_entry()));
        m_storage_entries_count++;
      }

      m_sections_entries_count++;
    }
    inline 
    void throwable_buffer_reader::read(std::string& str)
    {
      CHECK_MEM_LIMITATIONS();
      if (m_limits.strict)
      {
        // Every check sees the full decoded length; narrowing only afterwards.
        const uint64_t wide_len = read_varint();
        CHECK_AND_ASSERT_THROW_MES(wide_len <= static_cast<uint64_t>((std::numeric_limits<size_t>::max)()), "Wrong blob data in portable storage: string length not representable");
        CHECK_AND_ASSERT_THROW_MES(wide_len < static_cast<uint64_t>(MAX_STRING_LEN_POSSIBLE), "to big string len value in storage: " << wide_len);
        CHECK_AND_ASSERT_THROW_MES(wide_len <= static_cast<uint64_t>(m_limits.max_string_length), "Wrong blob data in portable storage: string length limit exceeded");
        CHECK_AND_ASSERT_THROW_MES(wide_len <= static_cast<uint64_t>(m_count), "string len count value " << wide_len << " goes out of remain storage len " << m_count);
        const size_t len = static_cast<size_t>(wide_len);
        str.assign((const char*)m_ptr, len);
        m_ptr += len;
        m_count -= len;
        return;
      }
      size_t len = static_cast<size_t>(read_varint());
      CHECK_AND_ASSERT_THROW_MES(len < MAX_STRING_LEN_POSSIBLE, "to big string len value in storage: " << len);
      CHECK_AND_ASSERT_THROW_MES(m_count >= len, "string len count value " << len << " goes out of remain storage len " << m_count);
      //do this manually to avoid double memory write in huge strings (first time at resize, second at read)
      str.assign((const char*)m_ptr, len);
      m_ptr+=len;
      m_count -= len;
    }
    inline
    void throwable_buffer_reader::read(array_entry &ae)
    {
      CHECK_MEM_LIMITATIONS();
      CHECK_AND_ASSERT_THROW_MES(false, "Reading array entry is not supported");
    }
  }
}
