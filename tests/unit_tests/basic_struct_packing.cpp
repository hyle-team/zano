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


