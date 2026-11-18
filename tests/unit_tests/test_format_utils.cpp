// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include <iostream>

#include "common/util.h"
#define UTF_CPP_CPLUSPLUS 201703L
#include "common/utf8.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/offers_services_helpers.h"
#include "net/http_server_handlers_map2.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "rpc/core_rpc_server.h"
#include "../core_tests/chaingen.h"

TEST(json_serialization, rpc_response_replaces_invalid_utf8)
{
  struct response_handler : epee::net_utils::http::i_chain_handler
  {
    struct response_payload
    {
      epee::serialization::portable_storage storage;
      bool store(epee::serialization::portable_storage& target) const
      {
        target = storage;
        return true;
      }
    } payload;
    std::string body;
    std::string mime = "application/json";
    bool handle_http_request_map(const epee::net_utils::http::http_request_info&,
      epee::net_utils::http::http_response_info& response, epee::net_utils::connection_context_base&,
      bool& call_found, documentation&) override
    {
      call_found = true;
      response.m_mime_tipe = mime;
      if (mime == "application/json")
        return epee::net_utils::http::store_json_response(payload, response.m_body);
      response.m_body = body;
      return true;
    }
  } handler;

  currency::core core(nullptr);
  currency::t_currency_protocol_handler<currency::core> protocol(core, nullptr);
  core.set_currency_protocol(&protocol);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core>> p2p(protocol);
  const auto folder = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("rpc_utf8_%%%%-%%%%");
  boost::filesystem::create_directory(folder);
  auto cleanup = epee::misc_utils::create_scope_leave_handler([&] {
    boost::system::error_code ec;
    boost::filesystem::remove_all(folder, ec);
  });
  bc_services::bc_offers_service offers(nullptr);
  boost::program_options::variables_map vm;
  vm.insert({arg_market_disable.name, boost::program_options::variable_value(false, false)});
  ASSERT_TRUE(static_cast<currency::i_bc_service&>(offers).init(folder.string(), vm));
  currency::core_rpc_server rpc(core, p2p, offers);
  rpc.set_rpc_chain_handler(&handler);
  currency::core_rpc_server::connection_context context;
  epee::net_utils::http::http_request_info request = {};
  request.m_URI = "/utf8_test";

  const std::string decoded_e8 = "\xC3\xA8";
  const struct { const char* name; std::string input; std::string expected; } cases[] = {
    { "historical", "<v\xE8" "3;&\xEE" "n", "<v\xC3\xA8" "3;&\xC3\xAE" "n" },
    { "invalid_lead", "\xFF", "\xC3\xBF" },
    { "continuation", "\x80", "\xC2\x80" },
    { "truncated", "\xE2\x82", "\xC3\xA2\xC2\x82" },
    { "overlong", "\xC0\xAF", "\xC3\x80\xC2\xAF" },
    { "surrogate", "\xED\xA0\x80", "\xC3\xAD\xC2\xA0\xC2\x80" },
    { "above_unicode", "\xF4\x90\x80\x80", "\xC3\xB4\xC2\x90\xC2\x80\xC2\x80" },
    { "before_json_escapes", "\xE8\"\\\n", decoded_e8 + "\"\\\n" },
    { "before_valid_utf8", "\xE8\xC3\xA8", decoded_e8 + "\xC3\xA8" },
    { "valid_utf8", "\xC3\xA8\xD0\xAF\xF0\x9F\x98\x80", "\xC3\xA8\xD0\xAF\xF0\x9F\x98\x80" },
    { "controls", std::string("\0\x01\"\\\n", 5), std::string("\0\x01\"\\\n", 5) }
  };
  for (const auto& test_case : cases)
  {
    SCOPED_TRACE(test_case.name);
    auto& source = handler.payload.storage;
    ASSERT_TRUE(source.set_value("text", test_case.input, nullptr));
    ASSERT_TRUE(source.dump_as_json(handler.body));
    const std::string original_json = handler.body;
    epee::net_utils::http::http_response_info response = {};
    ASSERT_TRUE(rpc.handle_http_request(request, response, context));
    EXPECT_EQ(200, response.m_response_code);
    ASSERT_TRUE(source.dump_as_json(handler.body));
    EXPECT_EQ(original_json, handler.body);
    EXPECT_EQ(std::string::npos, utf8::find_invalid(response.m_body));
    epee::serialization::portable_storage parsed;
    ASSERT_TRUE(parsed.load_from_json(response.m_body));
    std::string text;
    ASSERT_TRUE(parsed.get_value("text", text, nullptr));
    EXPECT_EQ(test_case.expected, text);
    if (test_case.input == test_case.expected)
      EXPECT_EQ(original_json, response.m_body);
    else
      EXPECT_NE(std::string::npos, response.m_body.find("\\u00"));
    EXPECT_EQ(std::string::npos, response.m_body.find("\\uFFFD"));
  }

  handler.payload.storage = epee::serialization::portable_storage();
  auto& source = handler.payload.storage;
  auto section = source.open_section("nested", nullptr, true);
  ASSERT_TRUE(section);
  ASSERT_TRUE(source.set_value("key\xE8", std::string("\xEE"), section));
  ASSERT_TRUE(source.dump_as_json(handler.body));
  epee::net_utils::http::http_response_info nested_response = {};
  ASSERT_TRUE(rpc.handle_http_request(request, nested_response, context));
  epee::serialization::portable_storage nested;
  ASSERT_TRUE(nested.load_from_json(nested_response.m_body));
  section = nested.open_section("nested", nullptr);
  ASSERT_TRUE(section);
  std::string value;
  ASSERT_TRUE(nested.get_value("key" + decoded_e8, value, section));
  EXPECT_EQ("\xC3\xAE", value);

  handler.mime = " application/octet-stream";
  handler.body = std::string("\0\xE8\xEE\xFF", 4);
  epee::net_utils::http::http_response_info binary_response = {};
  ASSERT_TRUE(rpc.handle_http_request(request, binary_response, context));
  EXPECT_EQ(handler.body, binary_response.m_body);

  // The unknown-method error echoes id and has no JSON MIME type set.
  rpc.set_rpc_chain_handler(nullptr);
  request.m_URI = "/json_rpc";
  request.m_body = "{\"jsonrpc\":\"2.0\",\"method\":\"utf8_test\",\"id\":\"\xE8\"}";
  epee::net_utils::http::http_response_info error_response = {};
  ASSERT_TRUE(rpc.handle_http_request(request, error_response, context));
  EXPECT_EQ(std::string::npos, utf8::find_invalid(error_response.m_body));
  EXPECT_NE(std::string::npos, error_response.m_body.find("\\u00e8"));
  epee::serialization::portable_storage parsed;
  ASSERT_TRUE(parsed.load_from_json(error_response.m_body));
  std::string id;
  ASSERT_TRUE(parsed.get_value("id", id, nullptr));
  EXPECT_EQ(decoded_e8, id);
}

TEST(json_serialization, raw_bytes_and_unicode_escapes_trace)
{
  const auto print_bytes = [](const char* stage, const std::string& bytes)
  {
    std::cout << stage << " hex=" << epee::string_tools::buff_to_hex_nodelimer(bytes)
      << " utf8_valid=" << (utf8::find_invalid(bytes) == std::string::npos) << '\n';
  };
  const struct
  {
    const char* name;
    const char* input;
    bool valid_utf8;
  } cases[] = {
    { "control_01", "\x01", true },
    { "historical_raw_bytes", "<v\xE8" "3;&\xEE" "n", false },
    { "valid_utf8", "<v\xC3\xA8" "3;&\xC3\xAE" "n", true }
  };
  for (const auto& test_case : cases)
  {
    SCOPED_TRACE(test_case.name);
    const std::string input = test_case.input;
    std::cout << "\nCASE " << test_case.name << '\n';
    print_bytes("before_serialization", input);
    epee::serialization::portable_storage source;
    ASSERT_TRUE(source.set_value("text", input, nullptr));
    std::string json;
    ASSERT_TRUE(source.dump_as_json(json));
    std::cout << "writer_json:\n" << json << '\n';
    print_bytes("writer_json", json);
    EXPECT_EQ(test_case.valid_utf8, utf8::find_invalid(json) == std::string::npos);
    if (input == "\x01")
      EXPECT_NE(std::string::npos, json.find(R"(\u0001)"));

    // epee accepting the JSON does not establish UTF-8 validity
    epee::serialization::portable_storage restored;
    const bool parsed = restored.load_from_json(json);
    std::cout << "epee_parse_ok=" << parsed << '\n';
    ASSERT_TRUE(parsed);
    std::string decoded;
    ASSERT_TRUE(restored.get_value("text", decoded, nullptr));
    print_bytes("after_deserialization", decoded);
    EXPECT_EQ(input, decoded);
    EXPECT_EQ(test_case.valid_utf8, utf8::find_invalid(decoded) == std::string::npos);
    std::string rewritten_json;
    ASSERT_TRUE(restored.dump_as_json(rewritten_json));
    std::cout << "after_deserialization_json:\n" << rewritten_json << '\n';
    EXPECT_EQ(json, rewritten_json);
  }

  // supplied unicode escapes, not JSON generated by the current writer
  const std::string escaped_json = R"({"text":"\u00e8\u00ee"})";
  std::cout << "\nCASE explicit_unicode_escapes\ninput_json:\n" << escaped_json << '\n';
  print_bytes("input_json", escaped_json);
  epee::serialization::portable_storage restored;
  ASSERT_TRUE(restored.load_from_json(escaped_json));
  std::string decoded;
  ASSERT_TRUE(restored.get_value("text", decoded, nullptr));
  print_bytes("after_deserialization", decoded);
  EXPECT_EQ("\xC3\xA8\xC3\xAE", decoded);
  EXPECT_NE("\xE8\xEE", decoded);
  EXPECT_EQ(std::string::npos, utf8::find_invalid(decoded));
  std::string rewritten_json;
  ASSERT_TRUE(restored.dump_as_json(rewritten_json));
  std::cout << "after_deserialization_json:\n" << rewritten_json << '\n';
  print_bytes("after_deserialization_json", rewritten_json);
  EXPECT_EQ(std::string::npos, utf8::find_invalid(rewritten_json));
}

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
  const char* utf8_text = "<v\xC3\xA8" "3;&\xC3\xAE" "n";
  const struct
  {
    const char* input;
    const char* expected_json_text;
    const char* expected_decoded_text;
  } cases[] = {
    { "<v\xE8" "3;&\xEE" "n", R"(<v\u00e83;&\u00een)", utf8_text },
    { utf8_text, utf8_text, utf8_text }
  };
  currency::account_base acc;
  acc.generate();
  for (const auto& test_case : cases)
  {
    currency::block_extended_info bei = {};
    bei.bl.major_version = 1;
    uint64_t block_reward_without_fee = 0, block_reward = 0;
    ASSERT_TRUE(currency::construct_miner_tx(0, 0, 10000000000000, 1000, TESTS_DEFAULT_FEE,
      acc.get_keys().account_address, acc.get_keys().account_address, bei.bl.miner_tx,
      block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4, 0, test_case.input, 1));
    const std::string original_blob = t_serializable_object_to_blob(bei.bl);
    ASSERT_FALSE(original_blob.empty());

    currency::block_rpc_extended_info details = {};
    details.total_fee = TESTS_DEFAULT_FEE;
    ASSERT_TRUE(currency::fill_block_rpc_details(details, bei, currency::null_hash));
    EXPECT_EQ(test_case.input, details.miner_text_info);

    currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response response = {};
    response.status = API_RETURN_CODE_OK;
    response.blocks.push_back(details);
    std::string json;
    ASSERT_TRUE(epee::net_utils::http::store_json_response(response, json));
    EXPECT_EQ(std::string::npos, utf8::find_invalid(json));
    EXPECT_NE(std::string::npos, json.find(std::string("\"miner_text_info\": \"") + test_case.expected_json_text + "\""));
    EXPECT_EQ(test_case.input, response.blocks.front().miner_text_info);
    EXPECT_EQ(original_blob, t_serializable_object_to_blob(bei.bl));
    currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response parsed = {};
    ASSERT_TRUE(epee::serialization::load_t_from_json(parsed, json));
    ASSERT_EQ(1, parsed.blocks.size());
    EXPECT_EQ(test_case.expected_decoded_text, parsed.blocks.front().miner_text_info);
  }
}

TEST(market_json, offer_attachment_roundtrip)
{
  // test the payload codecs and offer signature, without chain acceptance or DB I/O
  const struct
  {
    const char* name;
    const char* comment;
    const char* json_comment;
  } cases[] = {
    { "UTF-8", "<v\xC3\xA8" "3;&\xC3\xAE" "n", "<v\xC3\xA8" "3;&\xC3\xAE" "n" },
    { "JSON escapes", "quote=\" slash=\\ newline=\n control=\x01", R"(quote=\" slash=\\ newline=\n control=\u0001)" },
    { "literal backslash-u", R"(\u00e8)", R"(\\u00e8)" }
  };
  currency::account_base acc;
  acc.generate();
  const currency::keypair owner_keys = currency::keypair::generate();
  for (const auto& test_case : cases)
  {
    SCOPED_TRACE(test_case.name);
    bc_services::update_offer offer = {};
    offer.tx_id = crypto::cn_fast_hash("offer", 5);
    offer.of.primary = "ZANO";
    offer.of.target = "USD";
    offer.of.amount_primary = 1000000000000;
    offer.of.amount_target = 1000000000000;
    offer.of.comment = test_case.comment;
    const std::string sig_blob = bc_services::make_offer_sig_blob(offer);
    crypto::generate_signature(crypto::cn_fast_hash(sig_blob.data(), sig_blob.size()), owner_keys.pub, owner_keys.sec, offer.sig);

    currency::transaction tx = {};
    tx.version = TRANSACTION_VERSION_PRE_HF4;
    bc_services::put_offer_into_attachment(offer, tx.attachment);
    ASSERT_EQ(1u, tx.attachment.size());
    const auto& attachment = boost::get<currency::tx_service_attachment>(tx.attachment.front());
    ASSERT_EQ(BC_OFFERS_SERVICE_INSTRUCTION_UPD, attachment.instruction);
    const std::string json = attachment.body;
    ASSERT_NE(std::string::npos, json.find(std::string("\"com\": \"") + test_case.json_comment + "\""));
    ASSERT_EQ(std::string::npos, utf8::find_invalid(json));
    std::cout << "MARKET " << test_case.name << " writer JSON: " << json << '\n';

    // put_offer_into_attachment writes JSON, transaction preparation compresses it
    currency::encrypt_payload_items(tx, acc.get_keys(), acc.get_keys().account_address, currency::keypair::generate());
    std::string tx_blob;
    ASSERT_TRUE(t_serializable_object_to_blob(tx, tx_blob));
    currency::transaction restored = {};
    ASSERT_TRUE(currency::parse_and_validate_tx_from_blob(tx_blob, restored));
    currency::tx_service_attachment restored_attachment = {};
    ASSERT_TRUE(currency::get_type_in_variant_container(restored.attachment, restored_attachment));
    std::string restored_json;
    ASSERT_TRUE(epee::zlib_helper::unpack(restored_attachment.body, restored_json));
    ASSERT_EQ(json, restored_json);

    std::vector<bc_services::offers_attachment_t> instructions;
    bc_services::extract_market_instructions(instructions, restored.attachment);
    ASSERT_EQ(1u, instructions.size());
    const auto* decoded = boost::get<bc_services::update_offer>(&instructions.front());
    ASSERT_TRUE(decoded != nullptr);
    EXPECT_EQ(offer.of.comment, decoded->of.comment);
    const std::string restored_sig_blob = bc_services::make_offer_sig_blob(*decoded);
    EXPECT_EQ(sig_blob, restored_sig_blob);
    const bool signature_valid = crypto::check_signature(crypto::cn_fast_hash(restored_sig_blob.data(), restored_sig_blob.size()), owner_keys.pub, decoded->sig);
    EXPECT_TRUE(signature_valid);
    std::cout << "MARKET " << test_case.name
      << " comment before=" << epee::string_tools::buff_to_hex_nodelimer(offer.of.comment)
      << " after=" << epee::string_tools::buff_to_hex_nodelimer(decoded->of.comment)
      << " signature_valid=" << signature_valid << '\n';
  }
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
