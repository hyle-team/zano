// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <boost/foreach.hpp>
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"
#include "serialization/serialization.h"
#include "serialization/binary_archive.h"
#include "serialization/json_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/variant.h"
#include "serialization/binary_utils.h"
#include "gtest/gtest.h"
using namespace std;

struct Struct
{
  int32_t a;
  int32_t b;
  char blob[8];
};

namespace currency
{
  ostream& operator<<(ostream& stream, [[maybe_unused]] const currency::signature_v& signature)
  {
    return stream;
  }
}

template <class Archive>
struct serializer<Archive, Struct>
{
  static bool serialize(Archive &ar, Struct &s) {
    ar.begin_object();
    ar.tag("a");
    ar.serialize_int(s.a);
    ar.tag("b");
    ar.serialize_int(s.b);
    ar.tag("blob");
    ar.serialize_blob(s.blob, sizeof(s.blob));
    ar.end_object();
    return true;
  }
};

struct Struct1
{
  vector<boost::variant<Struct, int32_t>> si;
  vector<int16_t> vi;

  BEGIN_SERIALIZE_OBJECT()
    FIELD(si)
    FIELD(vi)
  END_SERIALIZE()
  /*template <bool W, template <bool> class Archive>
  bool do_serialize(Archive<W> &ar)
  {
    ar.begin_object();
    ar.tag("si");
    ::do_serialize(ar, si);
    ar.tag("vi");
    ::do_serialize(ar, vi);
    ar.end_object();
  }*/
};

struct Blob
{
  uint64_t a;
  uint32_t b;

  bool operator==(const Blob& rhs) const
  {
    return a == rhs.a;
  }
};

VARIANT_TAG(binary_archive, Struct, 0xe0);
VARIANT_TAG(binary_archive, int, 0xe1);
VARIANT_TAG(json_archive, Struct, "struct");
VARIANT_TAG(json_archive, int, "int");
VARIANT_TAG(debug_archive, Struct1, "struct1");
VARIANT_TAG(debug_archive, Struct, "struct");
VARIANT_TAG(debug_archive, int, "int");

BLOB_SERIALIZER(Blob);

bool try_parse(const string &blob)
{
  Struct1 s1;
  return serialization::parse_binary(blob, s1);
}

TEST(Serialization, BinaryArchiveInts) {
  uint64_t x = 0xff00000000, x1;

  ostringstream oss;
  binary_archive<true> oar(oss);
  oar.serialize_int(x);
  ASSERT_TRUE(oss.good());
  ASSERT_EQ(8, oss.str().size());
  ASSERT_EQ(string("\0\0\0\0\xff\0\0\0", 8), oss.str());

  istringstream iss(oss.str());
  binary_archive<false> iar(iss);
  iar.serialize_int(x1);
  ASSERT_EQ(8, iss.tellg());
  ASSERT_TRUE(iss.good());

  ASSERT_EQ(x, x1);
}

TEST(Serialization, BinaryArchiveVarInts) {
  uint64_t x = 0xff00000000, x1;

  ostringstream oss;
  binary_archive<true> oar(oss);
  oar.serialize_varint(x);
  ASSERT_TRUE(oss.good());
  ASSERT_EQ(6, oss.str().size());
  ASSERT_EQ(string("\x80\x80\x80\x80\xF0\x1F", 6), oss.str());

  istringstream iss(oss.str());
  binary_archive<false> iar(iss);
  iar.serialize_varint(x1);
  ASSERT_TRUE(iss.good());
  ASSERT_EQ(x, x1);
}

TEST(Serialization, Test1) {
  ostringstream str;
  binary_archive<true> ar(str);

  Struct1 s1;
  s1.si.push_back(0);
  {
    Struct s;
    s.a = 5;
    s.b = 65539;
    std::memcpy(s.blob, "12345678", 8);
    s1.si.push_back(s);
  }
  s1.si.push_back(1);
  s1.vi.push_back(10);
  s1.vi.push_back(22);

  string blob;
  ASSERT_TRUE(serialization::dump_binary(s1, blob));
  ASSERT_TRUE(try_parse(blob));

  ASSERT_EQ('\xE0', blob[6]);
  blob[6] = '\xE1';
  ASSERT_FALSE(try_parse(blob));
  blob[6] = '\xE2';
  ASSERT_FALSE(try_parse(blob));
}

TEST(Serialization, Overflow) {
  Blob x = { 0xff00000000 };
  Blob x1;

  string blob;
  ASSERT_TRUE(serialization::dump_binary(x, blob));
  ASSERT_EQ(sizeof(Blob), blob.size());

  ASSERT_TRUE(serialization::parse_binary(blob, x1));
  ASSERT_EQ(x, x1);

  vector<Blob> bigvector;
  ASSERT_FALSE(serialization::parse_binary(blob, bigvector));
  ASSERT_EQ(0, bigvector.size());
}

TEST(Serialization, serializes_vector_uint64_as_varint)
{
  std::vector<uint64_t> v;
  string blob;

  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(1, blob.size());

  // +1 byte
  v.push_back(0);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(2, blob.size());

  // +1 byte
  v.push_back(1);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(3, blob.size());

  // +2 bytes
  v.push_back(0x80);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(5, blob.size());

  // +2 bytes
  v.push_back(0xFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(7, blob.size());

  // +2 bytes
  v.push_back(0x3FFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(9, blob.size());

  // +3 bytes
  v.push_back(0x40FF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(12, blob.size());

  // +10 bytes
  v.push_back(0xFFFFFFFFFFFFFFFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(22, blob.size());
}

TEST(Serialization, serializes_vector_int64_as_fixed_int)
{
  std::vector<int64_t> v;
  string blob;

  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(1, blob.size());

  // +8 bytes
  v.push_back(0);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(9, blob.size());

  // +8 bytes
  v.push_back(1);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(17, blob.size());

  // +8 bytes
  v.push_back(0x80);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(25, blob.size());

  // +8 bytes
  v.push_back(0xFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(33, blob.size());

  // +8 bytes
  v.push_back(0x3FFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(41, blob.size());

  // +8 bytes
  v.push_back(0x40FF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(49, blob.size());

  // +8 bytes
  v.push_back(0xFFFFFFFFFFFFFFFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(57, blob.size());
}
/*
namespace
{
  template<typename T>
  std::vector<T> linearize_vector2(const std::vector< std::vector<T> >& vec_vec)
  {
    std::vector<T> res;
    BOOST_FOREACH(const auto& vec, vec_vec)
    {
      res.insert(res.end(), vec.begin(), vec.end());
    }
    return res;
  }

  template<typename T>
  std::vector<T> linearize_vector2(const std::vector< currency::signature_v >& vec_vec)
  {
    std::vector<T> res;
    BOOST_FOREACH(const auto& vec, vec_vec)
    {
      res.insert(res.end(), vec.begin(), vec.end());
    }
    return res;
  }
}
*/

bool test_get_varint_packed_size_for_num(uint64_t n)
{
  std::stringstream ss;
  typedef std::ostreambuf_iterator<char> it;
  tools::write_varint(it(ss), n);
  uint64_t sz = ss.str().size();
  if(sz != tools::get_varint_packed_size(n))
    return false;
  else 
    return true;
}

TEST(Serialization, validate_get_varint_packed_size)
{
  ASSERT_TRUE(test_get_varint_packed_size_for_num(127));
  ASSERT_TRUE(test_get_varint_packed_size_for_num(128));

  ASSERT_TRUE(test_get_varint_packed_size_for_num(16383));
  ASSERT_TRUE(test_get_varint_packed_size_for_num(16383+1));

  ASSERT_TRUE(test_get_varint_packed_size_for_num(2097151));
  ASSERT_TRUE(test_get_varint_packed_size_for_num(2097151+1));

  ASSERT_TRUE(test_get_varint_packed_size_for_num(268435455));
  ASSERT_TRUE(test_get_varint_packed_size_for_num(268435455+1));

  ASSERT_TRUE(test_get_varint_packed_size_for_num(34359738367));
  ASSERT_TRUE(test_get_varint_packed_size_for_num(34359738367+1));

  ASSERT_TRUE(test_get_varint_packed_size_for_num(4398046511103));
  ASSERT_TRUE(test_get_varint_packed_size_for_num(4398046511103+1));

  ASSERT_TRUE(test_get_varint_packed_size_for_num(4398046511103));
  ASSERT_TRUE(test_get_varint_packed_size_for_num(4398046511103+1));

  ASSERT_TRUE(test_get_varint_packed_size_for_num(562949953421311));
  ASSERT_TRUE(test_get_varint_packed_size_for_num(562949953421311+1));

}

TEST(Serialization, serializes_transacion_signatures_correctly)
{
  using namespace currency;

  transaction tx;
  transaction tx1;
  string blob;

  // Empty tx
  tx = AUTO_VAL_INIT(tx);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(6, blob.size()); // 5 bytes + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  if (!(tx == tx1))
  {
    ASSERT_TRUE(false);
  }
  
  ASSERT_EQ(tx.signatures, tx1.signatures);
  //ASSERT_EQ(linearize_vector2(boost::get<currency::NLSAG_sig>(tx.signature).s), linearize_vector2(boost::get<currency::NLSAG_sig>(tx1.signature).s));

  // Miner tx without signatures
  txin_gen txin_gen1;
  txin_gen1.height = 0;
  tx = AUTO_VAL_INIT(tx);
  tx.vin.push_back(txin_gen1);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(8, blob.size()); // 5 bytes + 2 bytes vin[0] + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  if (!(tx == tx1))
  {
    ASSERT_TRUE(false);
  }
  ASSERT_EQ(tx.signatures, tx1.signatures);
  //ASSERT_EQ(linearize_vector2(boost::get<currency::NLSAG_sig>(tx.signature).s), linearize_vector2(boost::get<currency::NLSAG_sig>(tx1.signature).s));

  // Miner tx with empty signatures 2nd vector
  tx.signatures.resize(1);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(9, blob.size()); // 5 bytes + 2 bytes vin[0] + 0 bytes extra + 0 bytes signatures + counter
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  if (!(tx == tx1))
  {
    ASSERT_TRUE(false);
  }
  ASSERT_EQ(tx.signatures, tx1.signatures);
  //ASSERT_EQ(linearize_vector2(boost::get<currency::NLSAG_sig>(tx.signature).s), linearize_vector2(boost::get<currency::NLSAG_sig>(tx1.signature).s));

  //TX VALIDATION REMOVED FROM SERIALIZATION
  /*
  // Miner tx with one signature
  tx.signatures[0].resize(1);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Miner tx with 2 empty vectors
  tx.signatures.resize(2);
  tx.signatures[0].resize(0);
  tx.signatures[1].resize(0);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Miner tx with 2 signatures
  tx.signatures[0].resize(1);
  tx.signatures[1].resize(1);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));
  */

  // Two txin_gen, no signatures
  tx.vin.push_back(txin_gen1);
  tx.signatures.resize(0);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(10, blob.size()); // 5 bytes + 2 * 2 bytes vins + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  if (!(tx == tx1))
  {
    ASSERT_TRUE(false);
  }
  ASSERT_EQ(tx.signatures, tx1.signatures);
  //ASSERT_EQ(linearize_vector2(boost::get<currency::NLSAG_sig>(tx.signature).s), linearize_vector2(boost::get<currency::NLSAG_sig>(tx1.signature).s));

  // Two txin_gen, signatures vector contains only one empty element
  //signatures.resize(1);
  //ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Two txin_gen, signatures vector contains two empty elements
  tx.signatures.resize(2);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(12, blob.size()); // 5 bytes + 2 * 2 bytes vins + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  if (!(tx == tx1))
  {
    ASSERT_TRUE(false);
  }

  ASSERT_EQ(tx.signatures, tx1.signatures);
  //ASSERT_EQ(linearize_vector2(boost::get<currency::NLSAG_sig>(tx.signature).s), linearize_vector2(boost::get<currency::NLSAG_sig>(tx1.signature).s));

  // Two txin_gen, signatures vector contains three empty elements
  //signatures.resize(3);
  //ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Two txin_gen, signatures vector contains two non empty elements
  //signatures.resize(2);
  //signatures[0].resize(1);
  //signatures[1].resize(1);
  //ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // A few bytes instead of signature
  tx.vin.clear();
  tx.vin.push_back(txin_gen1);
  tx.signatures.clear();
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  blob.append(std::string(sizeof(crypto::signature) / 2, 'x'));
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));

  // blob contains one signature
  blob.append(std::string(sizeof(crypto::signature) / 2, 'y'));
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));

  // Not enough signature vectors for all inputs
  //txin_to_key txin_to_key1;
  //txin_to_key1.key_offsets.resize(2);
  //tx.vin.clear();
  //tx.vin.push_back(txin_to_key1);
  //tx.vin.push_back(txin_to_key1);
  //signatures.resize(1);
  //signatures[0].resize(2);
  //ASSERT_FALSE(serialization::dump_binary(tx, blob));
  
  
  
  /*
  // Too much signatures for two inputs
  tx.signatures.resize(3);
  tx.signatures[0].resize(2);
  tx.signatures[1].resize(2);
  tx.signatures[2].resize(2);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // First signatures vector contains too little elements
  tx.signatures.resize(2);
  tx.signatures[0].resize(1);
  tx.signatures[1].resize(2);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // First signatures vector contains too much elements
  tx.signatures.resize(2);
  tx.signatures[0].resize(3);
  tx.signatures[1].resize(2);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // There are signatures for each input
  tx.signatures.resize(2);
  tx.signatures[0].resize(2);
  tx.signatures[1].resize(2);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(boost::get<currency::NLSAG_sig>(tx.signature).s), linearize_vector2(boost::get<currency::NLSAG_sig>(tx1.signature).s));

  // Blob doesn't contain enough data
  blob.resize(blob.size() - sizeof(crypto::signature) / 2);
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));

  // Blob contains too much data
  blob.resize(blob.size() + sizeof(crypto::signature));
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));

  // Blob contains one excess signature
  blob.resize(blob.size() + sizeof(crypto::signature) / 2);
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));
  */
}

using namespace currency;

class transaction_prefix_old_tests
{
public:
  // tx version information
  uint64_t   version{};
  //extra
  std::vector<extra_v> extra;
  std::vector<txin_v> vin;
  std::vector<tx_out_bare> vout;//std::vector<tx_out> vout;

  BEGIN_SERIALIZE()
    VARINT_FIELD(version)
    if (TRANSACTION_VERSION_PRE_HF4 < version) return false;
    FIELD(vin)
    FIELD(vout)
    FIELD(extra)
  END_SERIALIZE()
};

class transaction_prefix_old_tests_chain
{
public:
  // tx version information
  uint64_t   version{};
  //extra
  std::vector<extra_v> extra;
  std::vector<txin_v> vin;
  std::vector<tx_out_bare> vout;//std::vector<tx_out> vout;

  BEGIN_SERIALIZE()
    //VARINT_FIELD(version)
    if (TRANSACTION_VERSION_PRE_HF4 < version) return false;
    FIELD(vin)
    FIELD(vout)
    FIELD(extra)
    END_SERIALIZE()
};

class transaction_old_tests : public transaction_prefix_old_tests
{
public:
  std::vector<std::vector<crypto::signature> > signatures; //count signatures  always the same as inputs count
  std::vector<attachment_v> attachment;


  BEGIN_SERIALIZE_OBJECT()
    FIELDS(*static_cast<transaction_prefix_old_tests *>(this))
    FIELD(signatures)
    FIELD(attachment)
  END_SERIALIZE()
};


template<typename transaction_prefix_current_t>
bool transition_convert(const transaction_prefix_current_t& from, transaction_prefix_old_tests_chain& to)
{
  to.version = from.version;
  to.extra = from.extra;
  to.vin = from.vin;
  for (const auto& v : from.vout)
  {
    if (v.type() == typeid(tx_out_bare))
    {
      to.vout.push_back(boost::get<tx_out_bare>(v));
    }
    else {
      throw std::runtime_error("Unexpected type in tx_out_v");
    }
  }
  return true;
}
template<typename transaction_prefix_current_t>
bool transition_convert(const transaction_prefix_old_tests_chain& from, transaction_prefix_current_t& to)
{
  to.version = from.version;
  to.extra = from.extra;
  to.vin = from.vin;
  for (const auto& v : from.vout)
  {
    to.vout.push_back(v);
  }
  return true;
}

class transaction_prefix_new_tests
{
public:
  // tx version information
  uint64_t   version{};
  //extra
  std::vector<extra_v> extra;
  std::vector<txin_v> vin;
  std::vector<tx_out_v> vout;//std::vector<tx_out> vout;

  BEGIN_SERIALIZE()
    VARINT_FIELD(version)
    CHAIN_TRANSITION_VER(TRANSACTION_VERSION_INITAL, transaction_prefix_old_tests_chain)
    CHAIN_TRANSITION_VER(TRANSACTION_VERSION_PRE_HF4, transaction_prefix_old_tests_chain)
    if (CURRENT_TRANSACTION_VERSION < version) return false;
    FIELD(vin)
    FIELD(vout)
    FIELD(extra)
  END_SERIALIZE()

protected:
  transaction_prefix_new_tests() {}
};


class transaction_new_tests : public transaction_prefix_new_tests
{
public:
  std::vector<std::vector<crypto::signature> > signatures; //count signatures  always the same as inputs count
  std::vector<attachment_v> attachment;

  transaction_new_tests();

  BEGIN_SERIALIZE_OBJECT()
    FIELDS(*static_cast<transaction_prefix_new_tests *>(this))
    FIELD(signatures)
    FIELD(attachment)
    END_SERIALIZE()
};

inline
transaction_new_tests::transaction_new_tests()
{
  version = 0;
  vin.clear();
  vout.clear();
  extra.clear();
  signatures.clear();
  attachment.clear();

}

bool operator ==(const transaction_new_tests& a, const transaction_new_tests& b) {
  return true; a.attachment.size() == b.attachment.size() &&
    a.extra.size() == b.extra.size() &&
    a.vin.size() == b.vin.size() &&
    a.vout.size() == b.vout.size() &&
    a.signatures.size() == b.signatures.size();
}

bool operator ==(const transaction_new_tests& a, const transaction_old_tests& b) {
  return true; a.attachment.size() == b.attachment.size() &&
    a.extra.size() == b.extra.size() &&
    a.vin.size() == b.vin.size() &&
    a.vout.size() == b.vout.size() &&
    a.signatures.size() == b.signatures.size();
}

void validate_tx_serialisation(transaction_new_tests& tx)
{
  transaction_new_tests tx1;
  string blob;
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  if (!(tx == tx1))
  {
    ASSERT_TRUE(false);
  }
  
  transaction_old_tests tx2;
  ASSERT_TRUE(serialization::parse_binary(blob, tx2));
  if (!(tx == tx2))
  {
    ASSERT_TRUE(false);
  }
}

TEST(Serialization, serializes_transacion_versions)
{

  using namespace currency;

  transaction_new_tests tx;


  // Empty tx
  /*
  tx = AUTO_VAL_INIT(tx);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(6, blob.size()); // 5 bytes + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  if (!(tx == tx1))
  {
    ASSERT_TRUE(false);
  }
  ASSERT_EQ(linearize_vector2(boost::get<currency::NLSAG_sig>(tx.signature).s), linearize_vector2(boost::get<currency::NLSAG_sig>(tx1.signature).s));
  */

  txin_gen txin_gen1;
  txin_gen1.height = 0;
  tx.vin.push_back(txin_gen1);
  txin_to_key txin_key = AUTO_VAL_INIT(txin_key);
  txin_key.amount = 11111;
  txin_key.k_image = epee::string_tools::parse_tpod_from_hex_string<crypto::key_image>("cc608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");
  signed_parts sp = { 1, 2 };
  txin_key.etc_details.push_back(sp);
  ref_by_id rid = AUTO_VAL_INIT(rid);
  rid.tx_id = epee::string_tools::parse_tpod_from_hex_string<crypto::hash>("11608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");
  rid.n = 99999999;
  txin_key.key_offsets.push_back(rid);
  rid.n = 999999;
  txin_key.key_offsets.push_back(rid);
  tx.vin.push_back(txin_key);
  tx_out_bare vout = AUTO_VAL_INIT(vout);
  vout.amount = 11111;
  txout_to_key totk = AUTO_VAL_INIT(totk);
  totk.key = epee::string_tools::parse_tpod_from_hex_string<crypto::public_key>("22608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");
  totk.mix_attr = 22;
  vout.target = totk;
  tx.vout.push_back(vout);
  tx.vout.push_back(vout);

  tx_comment c;
  c.comment = "sdcwdcwcewdcecevthbtg";
  tx.extra.push_back(c);
  extra_alias_entry eae = AUTO_VAL_INIT(eae);
  currency::get_account_address_from_str(eae.m_address, "ZxDcDWmA7Yj32srfjMHAY6WPzBB8uqpvzKxEsAjDZU6NRg1yZsRfmr87mLXCvMRHXd5n2kdRWhbqA3WWTbEW4jLd1XxL46tnv");
  eae.m_alias = "eokcmeockme";
  eae.m_text_comment = "sdssccsc";
  tx.extra.push_back(eae); 
  tx.signatures.resize(2);
  crypto::signature sig = epee::string_tools::parse_tpod_from_hex_string<crypto::signature>("22608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab9985222608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");
  tx.signatures[0].push_back(sig);
  tx.signatures[0].push_back(sig);
  tx.signatures[1].push_back(sig);
  tx.signatures[1].push_back(sig);
  
  tx.version = TRANSACTION_VERSION_INITAL;

  validate_tx_serialisation(tx);
}

struct A
{
  std::string one;
  std::string two;
  std::vector<std::string> vector_one;

  BEGIN_SERIALIZE()
    FIELD(one)
    FIELD(two)
    FIELD(vector_one)
    END_SERIALIZE()
};

struct A_v1 : public A
{
  std::vector<std::string> vector_two;
  uint8_t current_version = 1;

  BEGIN_SERIALIZE()
    FIELD(one)
    FIELD(two)
    FIELD(vector_one)
    VERSION_TO_MEMBER(1, current_version)
    FIELD(vector_two)
  END_SERIALIZE()
};


struct A_v2 : public A_v1
{
  std::vector<std::string> vector_3;
  std::vector<std::string> vector_4;
  uint8_t current_version = 2;

  BEGIN_SERIALIZE()
    //CURRENT_VERSION(2)
    FIELD(one)
    FIELD(two)
    FIELD(vector_one)
    VERSION_TO_MEMBER(2, current_version)
    if (s_version < 1) return true;
    FIELD(vector_two)
    if (s_version < 2) return true;
    FIELD(vector_3)
    FIELD(vector_4)
  END_SERIALIZE()
};

struct A_v3 : public A_v2
{
  std::vector<std::string> vector_5;
  uint8_t current_version = 3;

  BEGIN_SERIALIZE()
    //CURRENT_VERSION(3)
    FIELD(one)
    FIELD(two)
    FIELD(vector_one)
    VERSION_TO_MEMBER(3, current_version)
    if (s_version < 1) return true;
    FIELD(vector_two)
    if (s_version < 2) return true;
    FIELD(vector_3)
    FIELD(vector_4)
    if (s_version < 3) return true;
    FIELD(vector_5)
  END_SERIALIZE()
};

inline bool operator==(const A& lhs, const A& rhs) 
{
  if ((lhs.one != rhs.one || lhs.two != rhs.two || lhs.vector_one != rhs.vector_one))
  {
    return false;
  }
  return true;
}

inline bool operator==(const A_v1& lhs, const A_v1& rhs)
{
  if (!(static_cast<const A&>(lhs) == static_cast<const A&>(rhs)))
    return false;
  if ((lhs.vector_two != rhs.vector_two))
  {
    return false;
  }
  return true;
}

inline bool operator==(const A_v2& lhs, const A_v2& rhs)
{
  if (!(static_cast<const A_v1&>(lhs) == static_cast<const A_v1&>(rhs)))
    return false;
  if ((lhs.vector_3 != rhs.vector_3))
  {
    return false;
  }
  return true;
}

inline bool operator==(const A_v3& lhs, const A_v3& rhs)
{
  if (!(static_cast<const A_v2&>(lhs) == static_cast<const A_v2&>(rhs)))
    return false;
  if ((lhs.vector_5 != rhs.vector_5))
  {
    return false;
  }
  return true;
}

template<typename first, typename second>
bool perform_test_ser_vers(second& f)
{
  string blob;
  first s_s = AUTO_VAL_INIT(s_s);
  if (!serialization::dump_binary(f, blob))
    return false;
  if (!serialization::parse_binary(blob, s_s))
    return false;


  if (!(f == static_cast<second&>(s_s)))
  {
    return false;
  }
  return true;
}

TEST(Serialization, versioning2)
{
  A_v3 a_3;
  a_3.one = "one";
  a_3.two = "two";
  a_3.vector_one.push_back(a_3.one);
  a_3.vector_one.push_back(a_3.two);
  a_3.vector_two = a_3.vector_one;
  a_3.vector_5 = a_3.vector_4 = a_3.vector_3 = a_3.vector_two;

  A_v2 a_2(a_3);
  A_v1 a_1(a_3);
  A a(a_3);

  bool r = perform_test_ser_vers<A_v1>(a);
  ASSERT_TRUE(r);
  r = perform_test_ser_vers<A_v2>(a);
  ASSERT_TRUE(r);
  r = perform_test_ser_vers<A_v3>(a);
  ASSERT_TRUE(r);

  r = perform_test_ser_vers<A_v2>(a_1);
  ASSERT_TRUE(r);
  r = perform_test_ser_vers<A_v3>(a_1);
  ASSERT_TRUE(r);

  r = perform_test_ser_vers<A_v3>(a_2);
  ASSERT_TRUE(r);

  r = perform_test_ser_vers<A_v3>(a_3);
  ASSERT_TRUE(r);

}

template <typename T>
inline T parse_hex_pod(const char* hex)
{
  return epee::string_tools::parse_tpod_from_hex_string<T>(hex);
}

template <typename T>
inline T parse_hex_pod(std::string_view hex)
{
  return epee::string_tools::parse_tpod_from_hex_string<T>(std::string(hex));
}

using first_payload_t = typename boost::mpl::front<all_payload_types>::type;

static void validate_against_chain_blob(const std::string& chain_blob_str, const std::string& expected_hash_hex = "")
{
  using namespace currency;

  std::string chain_blob = epee::string_encoding::base64_decode(chain_blob_str);

  transaction tx;
  ASSERT_TRUE(serialization::parse_binary(chain_blob, tx)) << "parse_binary failed on chain blob";

  if(!expected_hash_hex.empty())
  {
    crypto::hash real_hash = get_transaction_hash(tx);
    crypto::hash expected_hash = parse_hex_pod<crypto::hash>(expected_hash_hex);
    ASSERT_EQ(real_hash, expected_hash) << "tx hash mismatch after deserialization";
  }

  std::string reserialized;
  ASSERT_TRUE(serialization::dump_binary(tx, reserialized)) << "dump_binary failed";

  ASSERT_EQ(chain_blob.size(), reserialized.size()) << "size mismatch vs chain blob";
  ASSERT_EQ(chain_blob, reserialized) << "canonical blob mismatch vs chain blob";
}

void fill_test_tx(currency::transaction& tx, uint64_t tx_version)
{
  using namespace currency;

  tx = transaction{};
  tx.attachment.clear();
  tx.signatures.clear();
  tx.proofs.clear();

  tx.version = tx_version;

  txin_to_key in1 = AUTO_VAL_INIT(in1);
  in1.amount  = 11111;
  in1.k_image = parse_hex_pod<crypto::key_image>("cc608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");

  ref_by_id rid1 = AUTO_VAL_INIT(rid1);
  rid1.tx_id = parse_hex_pod<crypto::hash>("11608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");
  rid1.n = 12345;  in1.key_offsets.push_back(rid1);
  rid1.n = 67890;  in1.key_offsets.push_back(rid1);

  signed_parts sp = {1, 2};
  in1.etc_details.push_back(sp);
  tx.vin.push_back(in1);

  txin_to_key in2 = AUTO_VAL_INIT(in2);
  in2.amount  = 22222;
  in2.k_image = parse_hex_pod<crypto::key_image>("22608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");
  ref_by_id rid2 = AUTO_VAL_INIT(rid2);
  rid2.tx_id = parse_hex_pod<crypto::hash>("33608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");
  rid2.n = 111; in2.key_offsets.push_back(rid2);
  rid2.n = 222; in2.key_offsets.push_back(rid2);
  tx.vin.push_back(in2);

  tx_comment c; c.comment = "dummy-comment";
  tx.extra.push_back(c);

  extra_alias_entry eae = AUTO_VAL_INIT(eae);
  get_account_address_from_str(eae.m_address, "ZxDcDWmA7Yj32srfjMHAY6WPzBB8uqpvzKxEsAjDZU6NRg1yZsRfmr87mLXCvMRHXd5n2kdRWhbqA3WWTbEW4jLd1XxL46tnv");
  eae.m_alias = "dummy-alias";
  eae.m_text_comment = "dummy-note";
  tx.extra.push_back(eae);

  tx_out_bare o = AUTO_VAL_INIT(o);
  o.amount = 33333;
  txout_to_key tk = AUTO_VAL_INIT(tk);
  tk.key = parse_hex_pod<crypto::public_key>("44f3a4e5b6c7d8e90112233445566778899aabbccddeeff0011223344556677");
  tk.mix_attr = 22;
  o.target = tk;
  tx.vout.push_back(o);
  tx.vout.push_back(o);

  if (tx_version >= TRANSACTION_VERSION_POST_HF5)
    tx.hardfork_id = 1;

  first_payload_t pl{};
  tx.attachment.push_back(pl);

  crypto::signature sig = parse_hex_pod<crypto::signature>(
    "22608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852"
    "22608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852");

  currency::NLSAG_sig nls = AUTO_VAL_INIT(nls);
  nls.s.push_back(sig);
  nls.s.push_back(sig);
  tx.signatures.clear();
  tx.signatures.push_back(nls);

  zc_balance_proof pr{};
  tx.proofs.push_back(pr);
}

inline void print_hex_from_blob(const std::string& blob)
{
  for (size_t i = 0; i < blob.size(); ++i)
    printf("%02X", (unsigned char)blob[i]);
  printf("\n");
}

currency::transaction tx_from_hex(const std::string& hex_blob)
{
  std::string blob;
  epee::string_tools::parse_hexstr_to_binbuff(hex_blob, blob);
  currency::transaction tx;
  serialization::parse_binary(blob, tx);
  return tx;
}

struct tx_data
{
  std::string tx_blob;
  std::string tx_hash;
};

TEST(Serialization, deserialize_srialize_compare)
{
  using namespace currency;
  /*
    How to get hex blob:
    std::string blob;
    transaction tx0;
    fill_test_tx(tx0, TRANSACTION_VERSION_INITAL);
    serialization::dump_binary(tx0, blob);
    print_hex_from_blob(blob);
  */

  std::vector<tx_data> real_txs =
  {
    // 190 397 height
    {R"(AQIAvc8LAYCA6Jipv40HARrLAgAAAAAAACCBsBgvtAcYEuJBveBuqtM2fCR7uJhJEI/2NdR5fY9HAAKAoJSljR0D3m1bMzzdsJn+Jk+c+0ZRK0FDIR2X9+fi9tStek7BQ+MAgIDomKm/jQcDw6RsiHjwMolGH91QKoQk74nb4QP0OCrxEOvntGAbN9UABRa5TITTyw4i8sN+zst4Lyoh1aQTJEYDLVHfmWBIcIRhIhUAF6x5DsfPCxvvs4rsBQEBknG/AgXDFxvGN8Hw/rVNYMYW94YLQiylCAwVU9I5RQzYIH4eMXhszw6dyJMPOrp/K9PQF31/w66Smgs/50qrAQA=)", "f5bb8598339aba05380d2cd1c013184ca6ecab07bc11d054fc25cbe582e1cb81"},
    // 190 398 height
    {R"(AQEAvs8LAYCglKWNHQPmyuMkkt7EzI/3ZIdQ+MSGlhQE3nOD3i+hwa/8ysCwgAAFFhHZVdVFYnMWF4DQfNsbm5Y5HrhVqlJjRqo/Q1aPMzalFQAXX9YOyM8LCQjGcKsvZ4jBGwAA)", "be6276f772049a61f5a099bb7a7f96da3124e618b9771986083b29827859fe37"},
    // 190 399 height
    {R"(AQIAv88LAYCAiPzNvMMjARoaEQAAAAAAAHTBCw0eIBrlAIaDYMO22eZtqOnwBWblwXTr/CK/dWljAAKAoJSljR0DI1MfgsujLMbT58xuaPmXihQSu8V+fh0KIum2pa6qoNkAgICI/M28wyMDyif0wz2ADvrnVPq/2yvALDfRbQBJyD+XX2BeNZSENcoABRbI9XeuopH3jOLXCymbgULZEpcU0Y29pVDKl7gT71l9ORUAFw1pDsnPCxuKtIrsBQEBHAG+oO56ZpF55tpZPbkleZ2Z68AiQs96pDcrq3mKfAgukNk8lWqLYh6qWPNoGsXw/m8HdA0ZJz1VlRHPxIoGAgA=)", "28bb02591461e2262727004433d67ad9e0d5d263cb9cb55e66e085dca101e94f"},
    // 190 400 height
    {R"(AQEAwM8LAYCglKWNHQN8d6jVar+zr3cXjpMLqzuWkkQ6evU4FUsw4dV1WT3RowAFFrgiUPmNR2T2XYRINamds/5BWHEuSRWGXLeIQOMFtBqQFQAX7J4Oys8LCQgEtUC0O8dLggAA)", "847076e94869f14a680f3480acc7dd413f4e75a91abd1cb14dbfe93c20c5bad3"},
    // 190 401 height
    {R"(AQEAwc8LAYCglKWNHQNdQBKPC2E7tgiGAUG6MwO0hlvD+P5RSQN7h3c+Ik/mPQAFFsmiyPSvDzyubjlkB2F7x2+OnM70uo6wBcSNXDAK7anqFQAXFhAOy88LCQioOF8ya1nlJwAA)", "96d87eb49087c57dc08f378c7503c85f5517f959ae776bf03a59528fc8e85ea6"},
    // 190 402 height
    {R"(AQIAws8LAYCA+MSRjfSUBAEaVyAAAAAAAACMD4+DMvn17q4XBentxgL1n2PhCweaH9mUE2qmD28trgADgNiO4W8DlsZ0M9/Z0nzHWVd3n/TZAOzKz6OcKmKUANeqvaR1z9YAgKCUpY0dAzHquJCj3+mnfDmDCSFq3Slg62h7rGKJcAxXjerdLLbeAICA+MSRjfSUBAOOxkt3jIaDOJObrKRLrLz015qouPnZzrERH9kYMGEbYAAGFjKjkIrgc8+WB5Hbo0Up1mdACJLkH39Cf7GK6smMmf47Ew8xLjAuNDFbYTZhYjJhZF0VABfhJA7MzwsbprSK7AUBAU1a3Rx4j7uYQbOAUTz6DrfAvzQox1X3rEV2/EEQywQC8NQEoZWHzcHoJV+iI85YqVe215UhBgOwR20u+COkdAMA)", "0976afb09b975e2f579853eb6b8d478d0c54abe32fb20e79345966301984666a"},
    // 190 402 height
    {R"(AQEBgKCUpY0dBBrSiQAAAAAAABpW1gAAAAAAABrGJAAAAAAAABqI7AEAAAAAAJh9g6uOAZyoRVTiketiZ67CN9d2nl4DPBDGJrNLGMlnAAWAoL6BlQEDmNlkNOMfob0wmJEiYyyBfTLEVXWSvIE4r4+bANrhFcAAgOjtoboBA8FOTMVBWldiWPFcv5h6b/bwyGj84pPwVr75hn2/irsiAIDwksvdCAMvsicv/mD0TaYSE2j3/oTwAJpGp5QpaSnlPnoZWtROAQCA8JLL3QgDEAp5f6bMekD7Kx9tpmWW3VlbylnajH//imm4gvz88swAgPCSy90IAwiL7Kgek6R3+8GEPGKk+4NpYKN80o+SyjxG1EQ2O2eBAAQWUsapGQTuBdqGrfD63bQuIwIL4TgEYtFVo+7cBIgID9EXCacXclQXM74AAA==)", "54890fd37a1d8d3cd2396275cd404a83916aff8c7f06127e06f7e6e432f56433"},
    // 190 402 height
    {R"(AQIBgKCUpY0dBBqdXgAAAAAAABr6JgAAAAAAABpDLwIAAAAAABqavAAAAAAAANTWgAF7tZT9YVvvNHTOe1zvBK8qpH2+pqRoxZfYxC/0AAGAkN/ASgQaRgwAAAAAAAAa+QQAAAAAAAAaTgUAAAAAAAAaBiEAAAAAAAC+DQlfXKq26yVnVEOoFa4UMkF4noVWQUPUm79pgGH3UwACgMivoCUDg1GqczmnoqiwTED3nFkbZKsKRMycU/PcRmT7z/2BG80AgKCUpY0dAzzXwLCk9++RVWtDCNZMfKXe4VgCTQg7IdxMTHjqs0npAAMW3N+3kqMfZsMjVwbG8JS2qO4tO8eX7Vzc88WEiok5bC4XXdMSECUwuqZhIMfXg9iChEJKIho+h9QTSii9GCmn4tociFedAQAA)", "062e19360e424dd469ba71faf1e0d2e6a345cc360c9eadd1d530ff26f5fc7cba"},
    // 190 402 height
    {R"(AQIBgKCUpY0dBBpFYQEAAAAAABqJkQEAAAAAABo1CwAAAAAAABp0cwAAAAAAAB9ge0d5TXjvEgUqToS0ciLE0c8piAXLBjbGWTBdQGkuAAGAoLeH6QUEGncFAAAAAAAAGkgBAAAAAAAAGlQJAAAAAAAAGioTAAAAAAAACKqVXNJoxyRfgtnG6OxHdsC/mx52QQyDicDjrYLNq60AA4CIrKPPAgNomMbmsRSi77mllk7iHPVpfxJR6FHBCx2zL3DgI5dV6ACA0NvD9AIDW/uDV5JzEO4/+ZbfWSCC+IxfyZSU9mSirxvakjk5VWgAgKCUpY0dA6x7Jka32iUkZscA8MCHhZslFTE5FmQ9eFSjPEBzg5XAAAQW6rc0AE+++rwnijUkcrmGGg2bHKziGYofjhBkXjyCnQAX9m0X5QkSEEaTPSfCLD62ZRkV3Ce3bzrgSEVcSk39Rsi5iXQfHekLAQAA)", "e52e82db644c2c9590f29d515f3a2a89ff371fddd3c4132067e50f31bc9d2ab9"},
    // 190 403 height
    {R"(AQIAw88LAYCA+MSRjfSUBAEaWSAAAAAAAACNo4mwXCcg2Myqr8X4jJAryJXE5r2pnBoadNnFG89PIgACgKCUpY0dA0pdDex7pZst6HMuXiP9cSWZUT5aUz0io3SucpnSdkDcAICA+MSRjfSUBAPJsi8v2wfGDefcGGOs44zRNmuEqSHLnVPOWUQt4Z7XVQAGFsb+/2GEZsMLZwlgK71bBxXSe0kH3Q5wJ0VJrhmlcgiXEw8xLjAuNDFbYTZhYjJhZF0VABddqw7NzwsbrLSK7AUBASz34BoFdO7MJg+KzJ2Zt968r7AJPku3vVXDuvPhr4oNmAlvg6cFv/CIbbvnd+eHtszE0BarOcqpuOTYM3e+ZAcA)", "fa623613b6a77834b411b5101cc0410295f2d65df7b3cbe304c1bc99c2bbe7a9"},
    // 256 000 height
    {R"(AQIAgNAPAYCAqOyFr9GxAQEaKTIAAAAAAACP+BrUFAzakZ0P2PKP0mcxAR+8xWUQqoI4c7JMR2NfcgADgNiO4W8D5oyv3xLSFRxaVTxobzoRJ5vuEpIYuCC2KiUgm4Q52BkAgKCUpY0dAwd7c2zqxrPzHBnx8UPCgSA/MSvqOP6wgZHsOdp6U+8iAICAqOyFr9GxAQMY/UjjcPj19l6l70ksT4HQQCavddPrF/6CPTST0RgX4wAFFoHe0X43/2qwrkTvUyVwCtT+TVTKuMUOfuQ2KKt+pFxVFQAXuwMOitAPG+fP/O0FAQHUz320RtuvlLNvevi901lC+Rd4NKli68AlZQBGT3XyAn+UY32VFKCFHHahHsEHc2fBkd96czMeXmEgRKBRri4PAA==)", "56b2b876b0346a010d5d8438dd6504c73d93cf35f4db52ab6c180efc1e5be2b5"},
    // 256 000 height
    {R"(AQEBgOCllrsRBBqPBwAAAAAAABplEQAAAAAAABqyAAAAAAAAABpHEQAAAAAAAMyK+wGj7rMKcranu56WH43Lxvz1XAacmXVbvm29AyIXAAOAiKyjzwIDkFomVhb9eIj2/0NqsOVx5lb3IcqW3dRi+G2KDwcvLZoAgKC3h+kFA1cyn3/oYyZmQhDYs1maPsQ2NOZO3wv+6suo6t8FLF9NAIDwksvdCAN00Ni9D9apboHGQJ9R1/l5zUMi4mkNnmI/8Zi1GVMAMAADFi/NgDf+J6VJ8ZSnpGfFXSdDiEK4jrTqlwvpTp+PqVdgF3ESFxEEAAA=)", "452a1f60d7eb9ec9c9eb86706d34880c8d36d844e7b7ae676f5da860d490805b"},
    // 256 000 height
    {R"(AQEBgPCSy90IBBrYWQAAAAAAABqTGAAAAAAAABqsOgAAAAAAABpuBwAAAAAAAI5s6Yv7KRg9qZuf8hrI1TA4k8Uv3o9ChFubyYRwuR9UAAKAiKyjzwIDEKKZaXqru0VvXbQkFWD989HsI4+5DW0ze+f+9yPEdIsAgKC3h+kFA5uHKzUDUsHY06ZQLv0/Y/1dAqCZ5tga8j0vXyPdatKMAAMWCpqM2COjNjeAOrda3HGsx71W4wFn5aripzI7MWqVdC0Xh7gSEIMXyvYdJ7Mp5q9V3P0w0SVd9+cJON/uK7u0or3dG3Q9AQAA)", "9b870809e33da00a62419eeb7739f8f2793f6e2542b23a3c0626f2ff21a4da63"},
    // 256 000 height
    {R"(AQMBgKCUpY0dBBqf1AAAAAAAABpyEgAAAAAAABqN6QIAAAAAABrMtwAAAAAAAPor21r7EmhPbx/hsSRW7BtERqxD1SYFAaEcp9lP7LgaAAGAoJSljR0EGo+mAQAAAAAAGv43AAAAAAAAGiUQAQAAAAAAGrmZAQAAAAAAwd1viHLaB8QYLYSM/mqAgpLiFUVPraaC51z+YYrRAiEAAYCQ38BKBBr5FgAAAAAAABq5EQAAAAAAABoBGwAAAAAAABpaAwAAAAAAAApP7mHgDfFMT7g3c5OFVusfaqvllXU0eXT/0888N53eAAKAyK+gJQNWZ5TlmYdR0motbHtRt4bd6WX5pJAfgxHNDBtcE+J2dQCAwKjKmjoD8O4aHb4drCaLckr6VTXh2+v72DkHC1358w3BBSWBto0AAxbiAFtvvGXNw2BoA/007OrTkOtpm1cVMwE79gdlY1UJ7he0hBIQao3dBH1Ir0qOI7TaMhVOodqOjYgOC2JgF/k78wsEHgEBAAA=)", "d8597bea2bf062e9e7699c149fb50b043454657e2dda08448379a7a60e87c025"},
    // 256 001 height
    {R"(AQIAgdAPAYCA+MSRjfSUBAEaLjsAAAAAAABEyGFIT5hVgV0FSm2v8BEm9/J+Gabn9uxDDBwrQ4oVWgACgKCUpY0dA6XULlOE6bF/UDuvreAzmyt/zckTtkrIqdzFvyrfmtJOAICA+MSRjfSUBAMHNpZ+qBLu8i5INgQmZUWt/0FtSaJfuBdmpNZ7fRXDigAGFsZBbjt1eE+QSyfv2cYtuKkM3XEhpj1Ab/7n2oHK2Lz7ExExLjEuMi42N1szMTZhZTlkXRUACwLAYQ6L0A8b78/87QUBAQbsJuxHBKnix5xFafWlGtUtvTARc2MfXpiNqysHdsAIX4k7qSiQO7KMKhndnoeUon3Q37T4Tuq5ZJ4fZsaL9Q8A)", "22ead43ea7475a33a2d6600ebbc6c5fa63fe0c9ac4be7af82f710277f5b13ea0"},
    // 256 002 height
    {R"(AQIAgtAPAYCAqs6TjAkBGlEDAAAAAAAAHJCWWVYLeK6HvD+q7vD1EQo40we6u8PQwFbCxT1qGEYAAoCglKWNHQN2QCYLdaT7jF4PVoLFde4mDtZlEiIoA5oQZUO9/O9SsgCAgKrOk4wJA18X7Z7tblrPbMcvqr4oR5sDGSla5TLZxGNZTVxR/PAuAAYWEqnJkPmSpW7o1MDc1/PCA8Ccmryor47C5KXRzCpeVCsTFzEuMS4zLjcwWzVlOGU3YmYtZGlydHldFQALAoIGDozQDxvSz/ztBQEBxWWeldCBVE/saqRLdOq4EHOMJpTmMWips6O/ZnLzGAK1F8CKBNdmf0YogseQfy2uLXYyjODbJeODbDz3qkRIBwA=)", "e8d308a22456ac5eeca7f81a5e029c1fb9fd44d9655bd25a5ce5946e6f6aa577"},
    // 256 003 height
    {R"(AQEAg9APAYCglKWNHQNmpcO+Osi4V+TdKWjs13LrEAaAguLkBt+8pdyzj97CcwAFFgaRR9A9lfcoh4VClMJojH/yelgWhPVyj6DyqK5/PnsRFQAX0gsOjdAPCQj78a2g78cRwwAA)", "b9a02ceb6763184b325a28095d4c9849aa55eed31ec505eaf53d943ea361aca2"},
    // 256 004 height
    {R"(AQIAhNAPAYCAhP6m3uERARqBEgAAAAAAAAhfqSoR0hcJra5ZKeh3iPzPIXOfVPWFmH3BFMN8NHMZAAKAoJSljR0DYcsYZwsdx7ZbVU9YNWvG+S4DY31X1/mbJAzquVjB1o0AgICE/qbe4REDpqrmHr5xcuJUIv/rLlFSkBW8blGG4YlprTVTd2BRa4MABRZlnYyMY//pWZmYdosdc+RYarJwvgMcmT0qTGRrnnoTkBUAF0k5Do7QDxvO0PztBQEBK/F+hgDowAZuSrdeaoXqLfuURdgNbeSWP9XBl0gNXglAl2+nY7SVSIQwnUBkbRbflvmcr+j1NYyjnGvx3WfNBAA=)", "86a55cc8572c5ad02dbdee4c4e565c3db065cad09064c28ee80d6a9226e84c65"},
    // 256 005 height
    {R"(AQIAhdAPAYCA0LHS/poOARoDBAAAAAAAALQoTWjvOZDT4yl/A6V2d0c18P9FWIZyuFNb7uz8F9mkAAKAoJSljR0DcKnyL4T6YCd43W5i7BVE1XN/33p+uph5GWyb9TYOnmYAgIDQsdL+mg4DWEBu5avJzh0n2a+nBboZI8QBmqcAc6FlTmINMJqMqWUABRYAvgKdP6JydZ1vWnZqI2urvtFDYlBx8/FQGkncSBigoRUACwKh6Q6P0A8b0tD87QUBAWO/KCkmup1H0D8COyab9Di3hFDyR8NBRyvO/gGHTpQBS1OgAYr4phI+jF768EjHA04at/8JV4MNcXfW31+xxAIA)", "a1ed7715913326b074949d27f2746eb3c26f9e1004925aa363c909b63bdc227e"},
    // 256 006 height
    {R"(AQEAhtAPAYCglKWNHQMiGIbHXGRm4fIREb2sYGfOUlSTZD+TIwPpm7Od6f6XCQAFFtWQ/FvODB/75CZyqmH7jKY7yVMXfcx35VCnZnXBvcFlFQAXVFkOkNAPCQhXH8YuD9dz9wAA)", "226195ce83d9f8a712c544ae610e8bc225edd8052f463fa2ec11a2e4aa38cdeb"},
    // 1 055 580 height
    {R"(AQIA3LZAAYCAgr+T7/AIARoOPQAAAAAAANt+/Gp7cmTdl4ZuEuDSfMAH+DXJkY2RieZdmCJoyjanAAOAyK+gJQNe3Jx7pGDiGZ6zaArX5i8uASssOG3NJcgl0L3IITbuVACAoJSljR0DwnRfCPsmnTWYR0f7HtpIg8he1tUjc0klJcKXI+G8oqwAgICCv5Pv8AgDUu9gjvKqiZdFlZ6z722UpqwWWwX6RNWVbYPOg+MSST0ABRZfAHVuUWDAyeBUK6DLJFokbP6HRwc7j1u+Qh8IV+juMhUACwJW2w7mtkAbt4v4hAYBAQs3UgHG2+6/X7c5jWs02M/hvAdtFRb/CYoxJKbQ9hELUvfuZbtOiq0aVCmNcdyVhnQM/qrDN6y/hBKB4llNDQYA)", "a1e6950cafb663e8df0f22a1827029aee93a730cd10a01d63c2519313860292c"},
    // 1 055 580 height
    {R"(AQIBgKCUpY0dAhpmsQQAAAAAABpPLA0AAAAAAH4x97Lpf2wjBC/FtqD+hA8nYvsEe4C41INuNXfm4T2fAAGAsIHarxQCGtowAAAAAAAAGiIKAAAAAAAAKbHXi4RyeiQrQmOyUg8E5PBqjSLklDOfZTamdCHTHUIAB4Co1rkHA2t/Wv9Rxmd3M0EqzXx3D90CUozspu1pBoocUKyUrr4MAICg2eYdA+mvGh5kMSfAQN4xSXYKBLJX12yFNXsRXlC8/BGk6P5kAICQ38BKAz+geg/AHdIKvEmVgeVZyx1jVtI5+aPyFmJeOfZQmRYvAICwncLfAQPKwLAR84lJ3I6nuHyL/7F5SL5csr/oGVhdiK7cXh8jqwCA0NvD9AID2+qy/sOW7fF6UHCIkRetLXdeymCOodNbqL1eB8MAjiQAgJDK0sYOA2g2GswHDDi+AETYXZLSQEG2Mjo0yvtenXlSmM7FIJLiAICglKWNHQNYP3CMKRutueaN40BNAQh4V01UXQytjPMdaSQizNu2SwAHICHzYOrGBKHrqj4DuDD0TX9RLIcSJvcNuqd1rD0nmEetdYr++ubqTCVwtagCWy888zk8sG+sdTdduR2eV4e9Wo3BFqh0cLgGz6vXfFEsaTtY5zCraTpve9saVArCLZK4ZG9fFwAACvxLKY1399WROWcRvaFyb5fRqwy+i5QKu3z/2SpUc+yUgNd5SwsCwpcLAvYKEhDVbSeBzgpa9nw0RX266sfllPKDOmokkGDbg+vhtSkZKgEAAA==)", "166620a19d63df30f11ad1bf3695f649f9a242398434602ad2af3d7e32729898"},
    // 1 055 581 height
    {R"(AQIA3bZAAYCAhP6m3uERARrEZQAAAAAAAOwQeZcCA4GJwmZjbM5TBu+AAPvNkAlPD6U68uENIIlQAAKAoJSljR0DEZuOrH6yvdMfmVC5vsAskmN0hF8la8jaSfBizzzaYzAAgICE/qbe4REDEpbVzsMe31aDOR+mpKSCfzNTHRCc/YCJmg2t9Dpt8RAABRYA28r3jtBTLmema9lLLHJ4qSzZ+hgxPbiOfblpyWxm6BUACwI17g7ntkAbyov4hAYBAZvlg8WawRbW1J0iR3+v4CbTd2aieE+FKpPMcFfWVo8HUFSEghnlnrBofVwZc2sWpmmOq6u1SD0nqQyg2mQiDgUA)", "2c8a9a44daa7de6267c8e02f72d13e55b7a5b41d535fd1fdb35f2c284922b136"},
    // 1 055 582 height
    {R"(AQEA3rZAAYCglKWNHQNeW1KKNzECK1UGg0/MsScCl/3r/N1g4HejeVutczIElQAFFmLFdN2EXRNtjVapFucWhL6ADArSSmzCiIv7cE8XY2X3FQALArBfDui2QAkIDFur0Jzc8OoAAA==)", "15ce3ddc4a1d90ab2e5b3e8eb1a94f09fe19d0ea7b97512cd90a6a1613e86c4e"},
    // 1 055 583 height
    {R"(AQIA37ZAAYCAoLGXvMXGBQEaoX0AAAAAAAC5sfyO9JpQQMxslLjTbah6qeza0u5SL7XGqqU8o00ymwACgKCUpY0dA6td9cSv2zY+4BZKC4KtSUPBZ3o+li7uekZR8UpGxpFNAICAoLGXvMXGBQMDQDJC0vTnK4TiY9joyood2FOvQhvKD8lC0gfHpwE9EgAFFrAkDnjwwWapuIsyS69NM9hvSQWI3ZhlTARik0WKoMHKFQALAt8dDum2QBvli/iEBgEBDyWQ7KZTjygSOvo1323+Vsqs+A+gVPr9qaQ/0vouzAwhOzVrmE6hbSswYTSQYR/ytBt/NhRWNiRHt3CQhve1DQA=)", "2b07fa7d8221b0050747de7256e93ebc05cfb041c4bb503c6f0bc06b92b9017a"},
    // 1 055 584 height
    {R"(AQIA4LZAAYCAoPC38o2OAQEaO0IAAAAAAAAostFLOXEyJ3lqnBT3pO3cDKJPQSKB/kXelksCfRxMxgACgKCUpY0dAxW8Nzku5CJSMveFmasNIUDKlzWd8fGSGNNyvaoBhfpnAICAoPC38o2OAQO7bE8UPALPuZPuC2UpYGBcdDesIHYcT4K5I7FE/hsJRAAFFl86HwKHvNCjdQixH5M/Yd7up/UGMuGoifU6xF0gCw9sFQALAoDeDuq2QBvui/iEBgEBy2RFHTYctH4Oi2XArLF+57RK49cnFA9LjJF088PNLAmmQBQOtMd4q5pBrHT5UZWInucEA/QX8vuEeB7e0XymBgA=)", "5373224489bba5c614db41fc967c7cb36a019614597fead5a1d84c713633019d"},
    // 1 055 585 height
    {R"(AQIA4bZAAYCAsaK50cwBARpTHgAAAAAAAP5QU7tf8eO37CXXPmX9AphIA10rKXY/Co33OVKztKlkAAKAoJSljR0DGWtgES53cTfaMZGem7Hpu+lYfcOTMiTMi8xcEsLFCPUAgICxornRzAEDc9EbyY1UMlaQtGh593puWlg1bverwg6ZRZHYAVMbUGMABRZi6IWQumpBuz3OTCOi+596uVEYHOvYp4ZfmU8C5oteOBUACwIQcg7rtkAbyYv4hAYBAe15Ji1uN6aK3IvLQrZRRIwXbKz/AwYLABY7p+3bYTkLL5VamM6cMIzzTBebjKMh5l+OU2b10AHyGlVDRUpjcgcA)", "ef09d1fa679e13be453291280db5f96145cc9ed9c84334a1a9ba5e7c86149d6e"},
    // 1 055 586 height
    {R"(AQIA4rZAAYCAgr+T7/AIARoaPQAAAAAAAAx2X3lv3DFzvqKOYmn1TQURT0f2rQ1sjRhgt1n4DdaCAAKAoJSljR0DfKzx/2YrE/lml/MTzG+KaEagQwQQk+z2vcGyYdIgFkEAgICCv5Pv8AgD9B8h99OE6YXlfHpIDKaYRK+mxirKzXJlOWMMxEogaIsABRbjkmP4i7YUyNWBqZcnuz4YvKSZaNMYI2mYGDG/MVVn+hUACwITjw7stkAb+ov4hAYBASDOfkjQDq/0LjKrDY7P7N/Lg0qv1qWKPH2LLar6HyoF0AriojBb2sEsHVPeRIr7UfhLvLVhM7dxAwHKmg9nqAcA)", "64bf8e34a52c73b4adbdf29f640644b10ef43028736dc63daceeb6b6793a07f1"},
    // 1 055 587 height
    {R"(AQIA47ZAAYCAkPib+YZHARrYRAAAAAAAAGRkh18/XkGsqmZlkuEA0F0/9Fz4CrkDWD726tn3i1mtAAKAoJSljR0DFCgnlmvU+TwlBp4XkZs9DrEG1dk18xdSFlfi2xpcFyUAgICQ+Jv5hkcDtz6NXm5qN+SqL3ztXeqBQzob518FEPGDFjqT8QP1OkEABRbRiW9oj2VG8jjcIuYSH0wF5rh9f9rCQffq50jS6dNGPRUACwKTiw7ttkAb/4v4hAYBAayqW/gaNkvlHbrq/BhTGVOyv6HiyGOW5oxSXhQVv2ICbRcG29Qq8YSkRi1Zn3lY8wC+ugcejCXN7UI/DrTEWwcA)", "5935f5520f13d9de407d7987dcb74fce2e236b9250bc024f42b3e3996b4da6d6"},
    // 1 055 588 height
    {R"(AQEA5LZAAYCglKWNHQOqNOnm/q6T+c1Rr+SPnGEeQalIDTSjXs5eQo16t/wXzAAFFmKYnH0FPpeO7Sf5HeQPDTXaHQbdXBoCxCPRTtbZZakdFQALAiKmDu62QAkIvhN7EuT30V4AAA==)", "eecd38de7ec07d2cbcad13cf229cd02666db57898b571a182ca6ecf47dc4d891"},
    // 2 111 111 height
    {R"(AQEAh+2AAQKAyK+gJQPduP2QaKZZz07M+N+WnT2yz3USaz/H0McewfaswO2obACAoJSljR0DSpHSRbipAOD/1bvW3m1hwxPDbpDfgdpsKalf9EuPeWoABRauXZiTZcY8smT8gzHRWVJn4tIlVFO1lRP31xsPN+rXvhUACwIGZA6R7YABCQgNS7FRiCENJQAA)", "14303a7c9fd36a03c92e893abc81d8ef35a449c220dd60c5de05ebbfb09393fb"},
    // 2 111 111 height
    {R"(AQEBgICxornRzAELGkgKAAAAAAAAGjkMAAAAAAAAGoQGAAAAAAAAGiYBAAAAAAAAGrAAAAAAAAAAGu0AAAAAAAAAGtEPAAAAAAAAGjEFAAAAAAAAGr8JAAAAAAAAGukCAAAAAAAAGi8GAAAAAAAAWC+JkY2H6VPJ7KLEa/VduZZkEJ/HYVlajqs29FB3sxYABoCAjZP113EDmP3Vl90mKAr0IMVyeUO+IBlOyBWa/BdRzD5U7k9pAu0AgIiso88CA1F+1cOEGDjgeGgizFuQEQpmDh/sP/npMgb6O2fdv1lcAIDQuOGYGgNo3Z7MvcduJvgPXvfZ8GSHCO7Ohlq4W2fLD6f3yGe0fgCAoLbO94UCAwSrsukeEoikraGdqo7DAD4iPwfWO0+O07rKSI4xCcAtAIDAnpCsuxQDQ3R2/9VfWWr6pOBS0T7jm/9dSE53Te3WNTfssIYTfBQAgIC7i5ObRAOYsV8s1OWYLWF4n8Cl2HAiiT2VDtbC5B1oowyUyHppzQAIH3pFnRRFEfrbVz7OsU3V5f0GfeoY29b0oD+71PzNjRbHqe5Dpbv8gsL6qUwsbkeaYaPYE1Lm+knigzwtkRYx1JixIKqkRc08LXMwpFGpY39RQ5U4ZC4g0tEYawa4/LduP4skzHfyg1ZeViLav6gWoBdif40vAcXeEUbQ7FplNnRrdvixFjzK2xE2AFRCqEjSCRZS0DmsB3InZ+BejJdGPqv3QuiLFwAACp7efn+slenJcYJoHx4snmcGZdv2V9oreFIwy5eXtCIMeHjg2gsCqXMLAlfIEgVZsi2FDasajBF9s0GNt2ZZzbEBcDdRNP50PfHigkFtfQEBC//aXD0O0EmpKpIPK2CADz0qdvhE0LFCK7Ilf666r7gNtS+r63/pJOGhMB5j8fUtXn/JQLSp3gpeuPXiURSw4wJZo2JCwj1HW4b1CLiPqdxfiQ/CnXUM2d0Ex6Wl/MUVBH2PACaNJ3t4c7aTpnaeR+lSwC3uiL0pJmIZDU7QkMgG3Px0k4UfkxxWWGuE/+2LXwQexwSa87D/NSaAuy6CkQC8lEZkHdVsCyco+L0BaHxLVPVdVtDnp5WnGgqlkjLPCGndi4WeNItr9fxa41BQyUXAy+rk3lzDyI6/Ej1JFjsOBJpwuxeky55kX+lgA6UwxBfoZbUCaX3uZeBw5U0C+QEPMvXNA3IY5g4nmpajEgBl9xs01shd+I5cNezJ6aznCwfof14hYBOp79fHdRx3q0B/fDeWRqoCex4nOOldwmgP/oQlzJFTcZmmFIOPK6yNxg7wAJceWkSvYJOqwleamAmTKrwwvJddlGu8i/b7/mFioE0NoHv1C4F12yLl6PYPCbQz4NADKfBTvr501Vg5qDmv1eRhmBAmMus2tVrNwysCT9uHTPZ7fuHpEjokvLDe6psEmV27yzwaKxE8VG6IKwsWXbADfs/Vo3kDmL4wCw0mbCLpYO3u0/bKjAdHvDLqANbW91kOmzOl3EEwWmUONaqfyigDF/wNjxyf4jRVADcH1JG8j1tZA08ssp/h/nhPSg5pnmnluW51Jj+c+YoaLwtjXE1m7F8RMhw3hi1Xod/08RMNpGzOsAVfxTxQMdXYAwV60jTrtS8/ehmwhPs35Ll8a/gcgDHgF1y/oBla34sCVSVKLBmO/prFS6sc+cOT96bJUHwPY0/yKgUefddPSgoi7Pd9sd3YMCAApQlJqq7Q22K9bvkWxdMTDpAvFO49DOrV/MlitfsEDz8/LFFfmY/9kq2za2KCKx0+rdPPPJ8LAQcCHeE=)", "5a8a82974c8fc90ed67c221d0427fa05b150b3857e3407d0b43a31784225effd"},
    // 2 111 112 height
    {R"(AQIAiO2AAQGAgI2T9ddxARqLHwAAAAAAAOnRQs56HVFq9ZlN8/6DgpT5lhgMqXbr5WQu3cJDF05FAAOA6O2hugEDc8XRuXSjXZ6kdFwcG+5RpE0ifSbW/+lbvXRoNHYwZAcAgKCUpY0dAxyuEosTxwNxQF5EOftQKnJMXBxNALwPeH5LV/KV/g6sAICAjZP113EDOD+6mlZxdP9DlIIQIMm7rWYTWglBUqXXsT0t8x2G/PMABRZ+joIzz/GiyL/KC+LRKMaxuApd8rfPLTIclt19cFlGahUACwKO9w6S7YABDAFkAAi2nGVkAAAAAAAAAQGUf9/2a4nqzr9/GOUMZZPjt3HbvBdNzqUUnVuFd/vZB91HFGnO1d+IGtRZVc7YTYj3yCN72eGV0mi7oBFUOEABAA==)", "00235f9a69b7a1c66f5f7b69c50c140914ed1d994ecd73442e4b01b902b430d0"},
    // 2 111 112 height
    {R"(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)", "ded8ee2b5a036bd5a7b59384ff3992abcc4bd581423b8fa7d7f91c40fcceb474"},
    // 2 111 112 height
    {R"(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)", "3e609d508dc931441a637e5ff80bce2e37b5157fd98ad610bbe4ec797772bb47"},
    // 2 111 112 height
    {R"(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)", "58830288e6726c865cd4b037a0922dd0dc7c18fb6114e6b7f70d9e0e6e75c4a5"},
    // 2 111 112 height
    {R"(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)", "165d2f03555ff432389b41a9782b369cb05242e35570d4031d905c290b20328d"},
    // 2 111 112 height
    {R"(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)", "38703a4a1c03bfae5b1ec3e252981686aa4a4ec57d9d11a11213404015f1396b"},
    // 2 111 113 height
    {R"(AQEAie2AAQGAoJSljR0DamzE1goiEP5jfcFonLPinRR2xd1zU05IaLQVU6UhNmEABRZtvr1bSCEjKmAZQWP7VO+rNuKVWLZesDH6STy7COMbThUACwLglQ6T7YABCQTvauUJAAA=)", "36c7952bb118f2ad373de9943316b598381df1e4ee82396b0bc12e112664db4a"},
    // 2 111 114 height
    {R"(AQIAiu2AAQGAgNDYi96i4wIBGn9cAAAAAAAAG+XagslXk+kTlRF3NY6gxYXj9n5c/BF6XTpF3AFzOX0AAoCglKWNHQPAMn3Lu5UA8LcP1h7ho7OpzShxIED9u9yz++hf/XejMQCAgNDYi96i4wID5+RcIGNHudZnl3m/SNm4B5u02wdYnmEUg/mZC9QECvsABRY2d/xmSVeBOAj1wl83RP03RRe70jpW5Fwk4OJYpb20IhUACwKRNQ6U7YABDAFkAAiBnWVkAAAAAAAAAQFuMMJosSIr44kqab+/zyrFbZo+NY9E+nqFCpgF0uolCwpGhExt61aSiyAXzxGZJz4ap2IQCT8Z9PknILd4zzIMAA==)", "1ded2f68fb29cdcac0e0f3a1b08742e064894a73ab3091397fe748eac6b87893"},
    // 3 000 000 height
    {R"(AgEAwI23AQYWOYYL5dIwW3Yr5bBR5jQdLzbj4SqYOKmlm6opa+7zNeITFVNUMTJfMWZmZWNhODZjNDc2OGI1NBUACwL4IgsCjYsOyo23AQImr7GnTJA/UHjT8hzwST4Qu7dreXorv2G3JjB5OaPpHFTyQHyEDWKXM0vOFSKZ7e11lJCzyYul6LtAF7wr7Vmi/qI/AqbwF1ZLGYGqBY9EtzsxN2THX4jm2GS7gSvZg5RRdMMtPqr6/GI79IPoWNQui/TsffBkraLjSTRGnP9rYmgvq2MP42l6AgAmnMd5bgd2zjQTcfo69QbMNwFwrv7vBy9Kxxm3lBY9ZVaoigF+cbgZ8uLo6KbmrwH1hR0CKJHFkRWQmisva4KUqpwNPOPvp6oW0aM1aA4ywq8g2I+h+tRiorjTDylJSBXwdMMtPqr6/GI79IPoWNQui/TsffBkraLjSTRGnP9rYmgnoSEuwMjrngAAAAMuAC8HIfylJ1j6C8s5hEQeX9BEkAj4BKZsqWhcZTUiQCisprN5nBzZzsJBxP/d5OtFYX7u3u4GozSxtqR76Doco6HLXd6Pj6I/Ni/UzoyHHXAWx2yStxmHsS9Ml6GUInQvbMYfZlHNDIksakg03F2GWdq1kZdII039tRg4q+JjM2AUpPXixJdFghfec9kNlmeXaj7z6jDnIbbacy359YtkMwWebyfNRn04KzzPnXhFUVHfx3nMOFp/4NwpmUq8PWatIrJSzITMtck1OaBhBKRSSQDH47ebYq9ftqoiUBsjxJU0BBUHE4ZerKvHeX5y2NnETi4sLyZZIKKWQ4c4YCegC+lIr0nCSsl93AfvB7pWbdhd4jQBBGzk+Y4gmhtFd+dg9BNk+NA8Pmz/xM8cu17NGLi+0T3cuEWudBTxmRPmMo5ohC1ydN5UFO3Ra4kPyCNoJ6WY5wAoy/xnQdA1mX0hyiieaT3L4YhUrOaUrveVyh01H4jB54BPkfHw9S4k9CRF7HcNNM7j8IUV3g3qeHK2BR3s9U34xR0r56wluQFCoO0VyLIyOzOfrjommzp8qRTabOZTstvgGOCG2+8Kx0RRtCDvS+TJ/U5j2Lyh0BkQ79AKXTB4KqJ0DKuG89xNkWzODtycfW940/uUePpPBlLBdme1BFIqPk7ss+dybh5KU4pPTrDKLWEWNhgU9y9sZQRzP+RxXWEVjDlaklayYzEsOEN0V8T/2V8Luk45dhEJMK+dxA5nRzcuBJj+brijIUHpn16DAdL3NQpn0kZIN9wrE9Vt2QBxV8SPla4urmEWOO+lHGgJCZzWuDkGOCu9cgdZyOfk2YPAdb4CAKkGMqrF7BG4kwgC90HrH8jwXgPk6MNx9VEOqR4teMzTNXJ+npuhQQtjgYHTOiNAiPXBXme/ZGJTZ4FxR6SLBY2W/5DWohtf68EcpAKV3jVpNKoxR2xwXW1FSIiUyLoD6hxShWjZ/NXtvGF/AVrkv5VUSNJlLFWeSNQ9kNah8m/Spn9w7dFe0CM70yoCAibupW/prtiUdabVT/y2MDDF5pjyXna/GtxK9gwkNH8L+IaM4leqMINAXnBKVe/TCsKh0oyhFz6q5+wpsUJF3g/zzpUVCdu4MDaewNyvvqWW5QhH/tzFT3XXz5ScJr0GBDDkFcl2bx5j6ygBYxibadSX6XgpOJIUIEvtLFsTuXBcA94fbRQle17zQIYyw/Qv9pWrTnUVwXObAkItIxSzz1kBbBq1C6UNVw2aw3D42ZQ4FrFK4i53vQhwIBttgaUFyww=)", "0b7edec6e55b60e01c65854c6d1524f5b8c9108e9eb3b316731d6976b8c749cf"},
    // 3 000 001 height
    {R"(AgEAwY23AQYWQOOJd9mpKQ25f8fvLe2DAZPKXgY+2Wr6SOcNJiFDDRwTHFdvb2x5UG9vbHktREUxLXphbm8tMS0zOFRNTzkVAAsCjh8LAsnlDsuNtwECJszmiX8K9Mk2vb+EEhZnQ9Y+vPAwkTmQytoBjdT8KddPurm7A9rFiNGVRhTzAsh92yHIseomoDO2dNc+dMwTPxAMxEJ/wlVgjhuZ9rejMRDAZcKqF85DgqSthMy1eeVnP3TDLT6q+vxiO/SD6FjULov07H3wZK2i40k0Rpz/a2JoGcW1uycfSCkAJs8vJ1cN1GQGPZs3BhXt+qD3mGr3wquu6SceB2UDHt1GMsMrly/FbVkv8UhNiPRPZXu9F6bDO0+y3Fsm7gqDrP4s3Ip0VjqxmLyzdKtyICjwhN40IjpIcWzJjtoHr+WtTXTDLT6q+vxiO/SD6FjULov07H3wZK2i40k0Rpz/a2Jo+2Oucqoa6pgAAAADLgAvBzsU+RzvPMgwx0mrc/UT7C0hfgvg9z1+8JmFwRFD3gC/bgWMWlJtZC6g8AREtvPV3ipEFlTjvRFsg3Rvy9MRnZfh1svW8BDyBp44GVadRtdN25MU7+YZH1d/eGQN+wZtPJqPkgtYSU/owoD+FphilAoxDWkfuQubcUVNCwtb03dUG6ObgjST3jjxPgq8dwHuV4PQ8/Ed4JjZSVDJ73F3UKTt8k3DFH0lOb/q0E+eKavyjjchpp+OAJNcLRnRKKe3ihszoK1KGyKUBpI2tTPxnDzlY03plzj8BROqXQEpzMNwB+fjX/fXgK6iPbIEG0fd4HHztCUdw5Oitw/NRvmWHVCs7ZszTYTraCxf0kGVCKePHf64nHtxwE4trmR6167BVTjRq4qszx3l8M+Gklu8oxqDlx1Yb8x3h8qDzXcKD9KAozzSksbzKDq1jWerqskCfgak0ao/X5In3/a91W6qGP3pGDjRl0Jc0gypyDUZT4Br1WPFxwctRiqwAc9ZJOa643qXknyBmV72S9NGIh15GuPy6Yes0vSNXkkmHHP6uaMeMv2wks0bhG4xoW8TAKB3ctrhiazDnJKUTzt/Z9dCgPnoZJ/caVyHKXIydxa63BU7N0WPYv79rqDKYT0nsMd+lSlm55BAK8BFGcHVuwLVEmi8UPCdG+YeyfOt8JMM0V83afHFgIoPZi1XcqiAbwlCjdVzJofNbYvODM02Se+iD6qwG4jZPnelz2hJK+3chYiWFeUj93f59lWJxOHgfaQhbALSeCq9p3MJcdw3Pmmm6HEMso27pXYHh8Kcx4O8UsNQCOOa8ta+/nDFFfEVCh+YvwnCUw9eCCYqVoYEa2WvaQABApa14Ne4TZDFMmiKM6uskoVgpgw/zLkuronCjGPGybSvEE5FD2iw6jouiujrPsmUzwRtb2C0w1PFLM81Z5Tvd28CrUu2pN7swRVUEc2w5z8yw50QZBHLhTtWgaHGBZiFCQJvzSIKk2eackp/2DW+AzaACrUtESPn6LL8Pf6FdjLvDQJR805ocrEBAaYAAQ+/O3q6OndYveiFXBAMwSotDzKBBQ4+WZfM4scUwqYZdQFWIS0cW/VenWmoMbNmna0CW78DUriKirC6wXQmTd1pTpkDHBnk/BMbFUJHMbI4+fO/BwIwJZDGlnf3WIwpabt8Kh1k233dFjqJMcaqmLsaPTfKcwqQSsVxjkbP2KQe9WPcD0fKSpM63BIT9EZj6Xry1vsbCGgGGR/QrQngaYwx5m0aNDmeYvk71yB51GNeDFbUxygL)", "b0d3b99e677d4b201745d1cd31e9ef330c04792c3c2d925d91174fa926b7d223"},
    // 3 000 002 height
    {R"(AgEAwo23AQYWijFf+skW+xmiWfASLLjzyXoChG6CR14FwAuIS7vriAoTFFNUMV82ZmVlZmM2NDZjYWVkMDIwFQALAg6mCwKizw7MjbcBAiYI6eX9TXwQ7jtGeU1DchZ2MEBSfH3x/iN1PzViMqRSt82CZxE6HjCs4W+r9sMM7VO5BQA+KH00d+RBfa4wX/dFvBOo5rqMNdmCIRIagj9UBzFzOlbHGXh+6I9/PIKeBgt0wy0+qvr8Yjv0g+hY1C6L9Ox98GStouNJNEac/2tiaGiL7GA1Xuf+ACYaWbhY0omjdHND6fy+yM7sypugLSwuRmalho90OYgApG47RYILUzqzL3BVlKYnz8LyH9OdY/DtvOmMTTWiZ9mDY6UPSsW0kDjT/QmhYiMKPf6bXRQl63rsIF6sG+Ddocl0wy0+qvr8Yjv0g+hY1C6L9Ox98GStouNJNEac/2tiaGWlJ1/kTI1lAAAAAy4ALwevaxzqU+ar5m1XmZSqIuhjZuzz7VscpXDZHzdVxOT/iT7B0LHFTlBIs/+T3jDLh30pkyPGdPUF+0/fwPV4cOULqffqSyLIF2BVWkTb8Mh0TiGjyeGgCn++QfhhPJRcCfOB54TWodOBWqsw4+gtXIk0Y7GuGuSqNr0YeT0uxkleiTSBHIpR/RCW8yKDvKEW7ztrTJeHYoPfATHSMbT44nEnLFZ0QLyJcwVEg64fRva/OexZjY6jlZ2hteUGNm3OK4P73ox5zAWwj1oKGV5e2rjH21jERema4qdFdde04rH0lges0ErhVRp7ZxKsu4/xRD9PteosNBpoQYkNgOz6NZN6HEtuRhczuJpXbQTwGnwhncvis5QeAb9irUWMSytm7NAs1pL8s/X+Ka5FqsxY9fEFe+grJs1s1tEVDFxO7i8D+YyFzqfs0kOQwJfVvFyWN5djjbdCG2js50jvQtXrb6qXIkW+DvpYmSbi2F0x/xrS02njy8NUz/ldIMTFk4A3u7SGri/PGwjqItrApmqE+zMN4Z9tGqR/tNiA44JFHgX22AB23XHnQt6dTkSGlC3DWpV7bj5xlRV5SOsTqAMJDqBBYwIZbFRe0YRm1m8i9kr1ynS0U4aQpUFurxR442QhCYA5l6sdQOyaEJY7X3DArvBHviILEgZwgoytUOOILUW3Yp99kjU702hK0lN1qQ8m/h+KxOmZyxjQ9DNOdJETfHjkvvaKwnzU7aJKOBKYCevJcZviIea77yrBGq5kRilzMgkNCnt39ZrSmRtErQP6gPB636Hr9yO+elgXyFVF1glPQgVeI7g3YtgFDRa8v4KFPLsqamVswmC/rAuPWmus0IOxCQJAHBMbblSzN2hR1weUAuOBSqSzqDtp4estsk2UmCjseAEA+WuzkwBoNFP/L7JtlyXoxHsUPwTj5sLlWMgd8e0OAsZ7m0OWANZS4H/L9L3r3sBTq40qYiXfa4rkw1V63oADO/wPJaEidiV9oY4uGfUTmtuoJAmsEEviPO+LFGmvIQkC+XgwszCrf/qaZu14s0RWGUwPkdeXozFjVtuA084YiwacR2SznRZHfbcCq75KxyifyBhdpMOzeYKN7ZfAGcLSAYgsJ7ZhSRFWZcitEt6c7cW16vu2DvNSJiltYwurpREBMKEVKkbjVEUncnwI5yGPvcb1YShvtdMXVCTS6+kniBsG0oMjoz/fnqnAQ/yaYKXAjPlp1DQXSMsxQYNXDdYcGQg9lp2Z+BxF65HbW8IBaQZjylaIW3vc4Zlif2V7pmmFCQ==)", "589a7ae5df1ab2a3b2ed6a080b2662898df156f75bcfc63eee10535189482ae1"},
    // 3 000 002 height
    {R"(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)", "d27c82e514975b702676689f57854b2a3553ebf43714db256fa4be88fe6c8844"},
    // 3 000 002 height
    {R"(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)", "eec2479876f9ced9498d8f57c9f688a598c03e197e40945ba179f58f66c82e2a"},
    // 3 000 002 height
    {R"(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)", "4aeadfc8dd2236b8a99437b2b0b0d82b8a1b8ad6957b9d35d1209584e43a131d"},
    // 3 000 002 height
    {R"(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)", "5babe91bb94b8e653959b3e3c2fbe0f3d75b3ec74126f00650c3fa6d244bc276"},
    // 3 000 002 height
    {R"(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)", "a92b68c21a29623f6b0b091d23e15fd3811659713635e44862fecfb67544fb80"},
    // 3 000 002 height
    {R"(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)", "b01f9583bed72581e25274779365925b74b2b83b334d657d6b7d04fa42d7c3b1"},
    // 3 000 002 height
    {R"(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)", "d2972e5ef2d2481562a31df8fa488b8504ef366926ba368df12952c7df320948"},
  };


  for (const auto& it : real_txs)
    validate_against_chain_blob(it.tx_blob, it.tx_hash);

  std::vector<std::string> test_hex_blobs = 
  {
    R"(000201E756021911608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852B9601911608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852B29204CC608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520111010201CEAD01021933608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998526F1933608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852DE0122608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520002B584020344F3A4E5B6C7D8E90112233445566778899AABBCCDDEEFF0011223344556670716B584020344F3A4E5B6C7D8E90112233445566778899AABBCCDDEEFF001122334455667071602070D64756D6D792D636F6D6D656E74210B64756D6D792D616C696173DCE222654FD00C269314225DF54FAFBFC0B35A310C77714F195ED0DE224393822A46F7C02C0128711CD0FC004F101C8E5E641DC629410EFFB676E7D02B423E28000A64756D6D792D6E6F74650000010222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852010C0000000000)",
    R"(010201E756021911608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852B9601911608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852B29204CC608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520111010201CEAD01021933608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998526F1933608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852DE0122608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520002B584020344F3A4E5B6C7D8E90112233445566778899AABBCCDDEEFF0011223344556670716B584020344F3A4E5B6C7D8E90112233445566778899AABBCCDDEEFF001122334455667071602070D64756D6D792D636F6D6D656E74210B64756D6D792D616C696173DCE222654FD00C269314225DF54FAFBFC0B35A310C77714F195ED0DE224393822A46F7C02C0128711CD0FC004F101C8E5E641DC629410EFFB676E7D02B423E28000A64756D6D792D6E6F74650000010222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852010C0000000000)",
    R"(020201E756021911608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852B9601911608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852B29204CC608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520111010201CEAD01021933608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998526F1933608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852DE0122608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520002070D64756D6D792D636F6D6D656E74210B64756D6D792D616C696173DCE222654FD00C269314225DF54FAFBFC0B35A310C77714F195ED0DE224393822A46F7C02C0128711CD0FC004F101C8E5E641DC629410EFFB676E7D02B423E28000A64756D6D792D6E6F746500000224B584020344F3A4E5B6C7D8E90112233445566778899AABBCCDDEEFF001122334455667071624B584020344F3A4E5B6C7D8E90112233445566778899AABBCCDDEEFF0011223344556670716010C0000000000012A0222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520130000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)",
    R"(030201E756021911608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852B9601911608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852B29204CC608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520111010201CEAD01021933608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998526F1933608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852DE0122608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520002070D64756D6D792D636F6D6D656E74210B64756D6D792D616C696173DCE222654FD00C269314225DF54FAFBFC0B35A310C77714F195ED0DE224393822A46F7C02C0128711CD0FC004F101C8E5E641DC629410EFFB676E7D02B423E28000A64756D6D792D6E6F746500000224B584020344F3A4E5B6C7D8E90112233445566778899AABBCCDDEEFF001122334455667071624B584020344F3A4E5B6C7D8E90112233445566778899AABBCCDDEEFF001122334455667071601010C0000000000012A0222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB9985222608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB998520130000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000)"
  };

  for(const auto& it : test_hex_blobs)
  {
    std::string bin;
    epee::string_tools::parse_hexstr_to_binbuff(it, bin);
    std::string b64 = epee::string_encoding::base64_encode(bin);
    validate_against_chain_blob(b64);
  }

  // transaction tx4;
  // fill_test_tx(tx4, TRANSACTION_VERSION_POST_HF6);
  // validate_blob_serialization(tx4);
}
