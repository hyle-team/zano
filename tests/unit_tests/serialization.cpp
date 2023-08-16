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


std::ostream& operator <<(std::ostream& o, const currency::signature_v& v)
{
  return o;
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


  BEGIN_SERIALIZE()
    FIELD(one)
    FIELD(two)
    FIELD(vector_one)
    VERSION(1)
    if (s_version < 1) return true;
    FIELD(vector_two)
  END_SERIALIZE()
};

struct A_v2 : public A_v1
{
  std::vector<std::string> vector_3;
  std::vector<std::string> vector_4;


  BEGIN_SERIALIZE()
    //CURRENT_VERSION(2)
    FIELD(one)
    FIELD(two)
    FIELD(vector_one)
    VERSION(2)
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

  BEGIN_SERIALIZE()
    //CURRENT_VERSION(3)
    FIELD(one)
    FIELD(two)
    FIELD(vector_one)
    VERSION(3)
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