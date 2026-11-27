// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "common/util.h"
#define UTF_CPP_CPLUSPLUS 201703L
#include "common/utf8.h"
#include "currency_core/currency_format_utils.h"
#include "net/http_server_handlers_map2.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "../core_tests/chaingen.h"

TEST(parse_and_validate_tx_extra, is_correct_parse_and_validate_tx_extra)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::account_base acc;
  acc.generate();
  currency::blobdata b = "dsdsdfsdfsf";
  uint64_t block_reward_without_fee = 0, block_reward = 0;
  bool r = currency::construct_miner_tx(0, 0, 10000000000000, 1000, TESTS_DEFAULT_FEE, acc.get_keys().account_address, acc.get_keys().account_address, tx,
    block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4, 0, b, 1);
  ASSERT_TRUE(r);
  crypto::public_key tx_pub_key;
  r = currency::parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_TRUE(r);
}
TEST(block_rpc_details, miner_userdata_does_not_break_json_utf8)
{
  const std::string historical = "miner\xE8";
  currency::block_extended_info block = {};
  block.bl.major_version = 1;
  currency::account_base miner;
  miner.generate();
  uint64_t reward_without_fee = 0, reward = 0;
  ASSERT_TRUE(currency::construct_miner_tx(0, 0, 10000000000000, 1000, TESTS_DEFAULT_FEE,
    miner.get_keys().account_address, miner.get_keys().account_address, block.bl.miner_tx,
    reward_without_fee, reward, TRANSACTION_VERSION_PRE_HF4, 0, historical, 1));
  currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response rpc = {}, decoded = {};
  auto& details = rpc.blocks.emplace_back();
  details.total_fee = TESTS_DEFAULT_FEE;
  ASSERT_TRUE(currency::fill_block_rpc_details(details, block, currency::null_hash));
  EXPECT_EQ(historical, details.miner_text_info);
  const std::string json = epee::serialization::store_t_to_json(rpc);
  ASSERT_TRUE(utf8::is_valid(json.begin(), json.end()));
  EXPECT_NE(std::string::npos, json.find(R"(miner\\xE8)"));
  ASSERT_TRUE(epee::serialization::load_t_from_json(decoded, json));
  ASSERT_EQ(1u, decoded.blocks.size());
  EXPECT_EQ(historical, decoded.blocks.front().miner_text_info);
}

TEST(block_rpc_details, getblocktemplate_pos_keyimage_json_roundtrip)
{
  using request = epee::json_rpc::request<currency::COMMAND_RPC_GETBLOCKTEMPLATE::request>;
  const std::string keyimage_hex = "a637203ec41eab772532d30420eac80612fce8e44f1758bc7e2cb1bdda815887";
  request sent{};
  sent.jsonrpc = "2.0";
  sent.id = std::string("0");
  sent.method = "getblocktemplate";
  sent.params.pos_block = true;
  ASSERT_TRUE(epee::string_tools::hex_to_pod(keyimage_hex, sent.params.pe.keyimage));
  std::string current_request;
  ASSERT_TRUE(epee::serialization::store_t_to_json(sent, current_request));
  ASSERT_TRUE(utf8::is_valid(current_request.begin(), current_request.end()));

  // 2.2.3.609 writer escaped control bytes but copied the high bytes of keyimage verbatim
  const std::string legacy_request =
    R"({"jsonrpc":"2.0","id":"0","method":"getblocktemplate","params":{"pos_block":true,"pe":{"keyimage":")"
    "\xA6" "7 >" "\xC4\\u001e\xAB" "w%2" "\xD3\\u0004 \xEA\xC8\\u0006\\u0012\xFC\xE8\xE4"
    "O\\u0017X\xBC~,\xB1\xBD\xDA\x81X\x87"
    R"("}}})";

  const auto check_keyimage = [&](const std::string& body, const char* direction)
  {
    SCOPED_TRACE(direction);
    request received{};
    ASSERT_TRUE(epee::serialization::load_t_from_json(received, body));
    EXPECT_EQ(sent.method, received.method);
    EXPECT_TRUE(received.params.pos_block);
    EXPECT_EQ(keyimage_hex, epee::string_tools::pod_to_hex(received.params.pe.keyimage));
  };
  check_keyimage(legacy_request, "2.2.3.609 -> current");
  check_keyimage(current_request, "current -> current");
}

TEST(parse_and_validate_tx_extra, is_correct_extranonce_too_big)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::account_base acc;
  acc.generate();
  currency::blobdata b(260, 0);
  uint64_t block_reward_without_fee = 0, block_reward = 0;
  bool r = currency::construct_miner_tx(0, 0, 10000000000000, 1000, TESTS_DEFAULT_FEE, acc.get_keys().account_address, acc.get_keys().account_address, tx,
    block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4, 0, b, 1);
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
  r = currency::get_tx_wide_payment_id_from_decrypted_container(tx.attachment, h2);
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
