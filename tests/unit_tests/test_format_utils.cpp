// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "common/util.h"
#include "currency_core/currency_format_utils.h"
#include "../core_tests/chaingen.h"

TEST(parse_and_validate_tx_extra, is_correct_parse_and_validate_tx_extra)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::account_base acc;
  acc.generate();
  currency::blobdata b = "dsdsdfsdfsf";
  bool r = currency::construct_miner_tx(0, 0, 10000000000000, 1000, TESTS_DEFAULT_FEE, acc.get_keys().account_address, acc.get_keys().account_address, tx, b, 1);
  ASSERT_TRUE(r);
  crypto::public_key tx_pub_key;
  r = currency::parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_TRUE(r);
}
TEST(parse_and_validate_tx_extra, is_correct_extranonce_too_big)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::account_base acc;
  acc.generate();
  currency::blobdata b(260, 0);
  bool r = currency::construct_miner_tx(0, 0, 10000000000000, 1000, TESTS_DEFAULT_FEE, acc.get_keys().account_address, acc.get_keys().account_address, tx, b, 1);
  ASSERT_FALSE(r);
}


TEST(parse_and_validate_tx_extra, is_correct_wrong_extra_nonce_double_entry)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::blobdata v = "asasdasd";
  currency::add_tx_extra_userdata(tx, v);
  currency::add_tx_extra_userdata(tx, v);
  crypto::public_key tx_pub_key;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_FALSE(r);
}


TEST(parse_and_validate_tx_extra, test_payment_ids)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);

  currency::account_base acc;
  acc.generate();
  std::string h = acc.get_public_address_str();
  bool r = currency::set_payment_id_to_tx(tx.attachment, h);
  ASSERT_TRUE(r);
  
  std::string h2;
  r = currency::get_payment_id_from_tx(tx.attachment, h2);
  ASSERT_TRUE(r);
  ASSERT_EQ(h, h2);
  
}

template<int sz>
struct t_dummy
{
  char a[sz];
};
template <typename forced_to_pod_t>
void force_random(forced_to_pod_t& o)
{

  t_dummy<sizeof(forced_to_pod_t)> d = crypto::rand<t_dummy<sizeof(forced_to_pod_t)>>();
  o = *reinterpret_cast<forced_to_pod_t*>(&d);
}

// TEST(parse_and_validate_tx_extra, put_and_load_alias)
// {
// 
//   currency::transaction miner_tx = AUTO_VAL_INIT(miner_tx);
//   currency::account_public_address acc = AUTO_VAL_INIT(acc);
//   currency::extra_alias_entry alias = AUTO_VAL_INIT(alias);
//   force_random(alias.m_address);
//   force_random(alias.m_sign);
//   force_random(alias.m_view_key);
//   alias.m_alias = "sdsdsdsfsfd";
//   alias.m_text_comment = "werwrwerw";
// 
//   bool res = currency::construct_miner_tx(0, 0, 0, 0, 0, 0, acc, miner_tx, currency::blobdata(), 10, alias);
//   ASSERT_TRUE(res);
//   currency::tx_extra_info ei = AUTO_VAL_INIT(ei);
//   bool r = parse_and_validate_tx_extra(miner_tx, ei);
//   ASSERT_TRUE(r);
//   if(!(ei.m_alias.m_address.spend_public_key == alias.m_address.spend_public_key &&
//     ei.m_alias.m_address.view_public_key == alias.m_address.view_public_key &&
//     ei.m_alias.m_alias == alias.m_alias &&
//     ei.m_alias.m_sign == alias.m_sign &&
//     ei.m_alias.m_text_comment == alias.m_text_comment &&
//     ei.m_alias.m_view_key == alias.m_view_key))
//   {
//     ASSERT_TRUE(false);
//   }
// 
//   force_random(alias.m_address);
//   force_random(alias.m_sign);
//   force_random(alias.m_view_key);
//   alias.m_alias = "sdsdsdsfsfd";
//   alias.m_text_comment = std::string();
// 
//   res = currency::construct_miner_tx(0, 0, 0, 0, 0, 0, acc, miner_tx, currency::blobdata(), 10, alias);
//   ei = AUTO_VAL_INIT(ei);
//   r = parse_and_validate_tx_extra(miner_tx, ei);
//   ASSERT_TRUE(r);
//   if (!(ei.m_alias.m_address.spend_public_key == alias.m_address.spend_public_key &&
//     ei.m_alias.m_address.view_public_key == alias.m_address.view_public_key &&
//     ei.m_alias.m_alias == alias.m_alias &&
//     ei.m_alias.m_sign == alias.m_sign &&
//     ei.m_alias.m_text_comment == alias.m_text_comment &&
//     ei.m_alias.m_view_key == alias.m_view_key))
//   {
//     ASSERT_TRUE(false);
//   }
// 
// }


TEST(validate_parse_amount_case, validate_parse_amount)
{
  uint64_t res = 0;
  bool r = currency::parse_amount(res, "0.0001");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000000);

  r = currency::parse_amount(res, "100.0001");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000100000000);

  r = currency::parse_amount(res, "000.0000");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);

  r = currency::parse_amount(res, "0");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);


  r = currency::parse_amount(res, "   100.0001    ");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000100000000);

  r = currency::parse_amount(res, "   100.0000    ");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000000000000);

  r = currency::parse_amount(res, "   100. 0000    ");
  ASSERT_FALSE(r);

  r = currency::parse_amount(res, "100. 0000");
  ASSERT_FALSE(r);

  r = currency::parse_amount(res, "100 . 0000");
  ASSERT_FALSE(r);

  r = currency::parse_amount(res, "100.00 00");
  ASSERT_FALSE(r);

  r = currency::parse_amount(res, "1 00.00 00");
  ASSERT_FALSE(r);
}
