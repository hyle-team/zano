// Copyright (c) 2022 Zano Project
// Copyright (c) 2012-2014 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "currency_core/currency_format_utils.h"
#include "common/boost_serialization_helper.h"
#include "currency_core/currency_boost_serialization.h"
#include "currency_core/difficulty.h"
#include "common/difficulty_boost_serialization.h"

using namespace currency;

TEST(block_pack_unpack, basic_struct_packing)
{
  currency::block b = AUTO_VAL_INIT(b);
  currency::generate_genesis_block(b);
  currency::blobdata blob = ::t_serializable_object_to_blob(b);
  currency::block b_loaded = AUTO_VAL_INIT(b_loaded);
  currency::parse_and_validate_block_from_blob(blob, b_loaded);
  crypto::hash original_id = get_block_hash(b);
  crypto::hash loaded_id = get_block_hash(b_loaded);
  ASSERT_EQ(original_id, loaded_id);

  std::stringstream ss;
  boost::archive::binary_oarchive a(ss);
  a << b;
  currency::block b_loaded_from_boost = AUTO_VAL_INIT(b_loaded_from_boost);
  boost::archive::binary_iarchive a2(ss);
  a2 >> b_loaded_from_boost;
  crypto::hash loaded_boost_id = get_block_hash(b_loaded_from_boost);
  ASSERT_EQ(original_id, loaded_boost_id);

  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::tx_service_attachment att;
  att.service_id = "N";
  att.instruction = "ADD";
  att.body = "sdsdsdsd";
  tx.attachment.push_back(att);
}

TEST(boost_multiprecision_serizlization, basic_struct_packing)
{
  std::vector<currency::wide_difficulty_type> v_origial;
  for(int i = 0; i != 1; i++)
  {
    v_origial.push_back(currency::wide_difficulty_type("117868131154734361989189100"));
    if(v_origial.size() > 1)
      v_origial.back() *= v_origial[v_origial.size()-2];
  }
   

  std::stringstream ss;
  boost::archive::binary_oarchive a(ss);
  a << v_origial;

  std::vector<currency::wide_difficulty_type> v_unserialized;

  boost::archive::binary_iarchive a2(ss);
  a2 >> v_unserialized;

  if(v_origial != v_unserialized)
  {
    ASSERT_EQ(v_origial, v_unserialized);
  }
  ASSERT_EQ(v_origial, v_unserialized);
}

////////////////////////////////////////////////////////////////////////////////

TEST(tx_signatures_packing, 1)
{
  std::vector<signature_v> sigs;

  sigs.clear();
  ASSERT_EQ(1, get_object_blobsize(sigs));

  //  v(x)  =  tools::get_varint_packed_size(x)

  {
    // empty NLSAG
    // v(1) + (1 + v(0) + 0 * 2 * 32) = 3                                                    
    sigs.clear();
    sigs.emplace_back(std::move(NLSAG_sig()));
    ASSERT_EQ(3, t_serializable_object_to_blob(sigs).size());
    ASSERT_EQ(3, get_object_blobsize(sigs));
  }

  {
    // 128 empty NLSAGs
    // v(128) + 128 * (1 + v(0) + 0 * 2 * 32) = 258
    sigs.clear();
    for(size_t i = 0; i < 128; ++i)
      sigs.emplace_back(std::move(NLSAG_sig()));
    ASSERT_EQ(258, t_serializable_object_to_blob(sigs).size());
    ASSERT_EQ(258, get_object_blobsize(sigs));
  }

  {
    // 128 10-ring NLSAGs
    // v(128) + 128 * (1 + v(10) + 10 * 2 * 32) = 82178
    sigs.clear();
    NLSAG_sig nlsag = AUTO_VAL_INIT(nlsag);
    nlsag.s.resize(10);
    for(size_t i = 0; i < 128; ++i)
      sigs.push_back(nlsag);
    ASSERT_EQ(82178, t_serializable_object_to_blob(sigs).size());
    ASSERT_EQ(82178, get_object_blobsize(sigs));
  }

  // the following tests cases should be redone
  // TODO @#@#

  {
    // empty ZC_sig
    // v(1) + (1 + 32 + 32 + (1 + 10*32) + 32) = 99
    sigs.clear();
    sigs.emplace_back(std::move(ZC_sig()));
    ASSERT_EQ(99, t_serializable_object_to_blob(sigs).size());
    ASSERT_EQ(99, get_object_blobsize(sigs));
  }

  {
    // 128 empty ZC_sigs
    // v(128) + 128 * (1 + 32 + 32 + (v(0) + 0*32) + 32) = 12546
    sigs.clear();
    for(size_t i = 0; i < 128; ++i)
      sigs.emplace_back(std::move(ZC_sig()));
    ASSERT_EQ(12546, t_serializable_object_to_blob(sigs).size());
    ASSERT_EQ(12546, get_object_blobsize(sigs));
  }

  {
    // 128 10-ring ZC_sigs
    // v(128) + 128 * (1 + 32 + 32 + (v(10) + 10*32) + 32) = 53506   (97 + (v(10) + 10*32))
    ZC_sig zc = AUTO_VAL_INIT(zc);
    //zc.clsags_gg.r.resize(10);
    sigs.clear();
    for(size_t i = 0; i < 128; ++i)
      sigs.emplace_back(zc);
    ASSERT_EQ(53506, t_serializable_object_to_blob(sigs).size());
    ASSERT_EQ(53506, get_object_blobsize(sigs));
  }
}
