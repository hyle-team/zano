// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"

#include "offers_test.h"
#include "offers_tests_common.h"
#include "offers_helper.h"
#include "currency_core/offers_services_helpers.h"
#include "currency_core/bc_offers_service.h"

using namespace epee;
using namespace currency;

bool sign_an_offer(const transaction &offer_source_tx, size_t offer_tx_index, const account_keys& offer_owner, const blobdata &blob_for_sig, crypto::signature &sig)
{
  crypto::public_key related_tx_pub_key = get_tx_pub_key_from_extra(offer_source_tx);
  crypto::public_key related_tx_offer_pub_key = bc_services::get_offer_secure_key_by_index_from_tx(offer_source_tx, offer_tx_index);
  currency::keypair ephemeral = AUTO_VAL_INIT(ephemeral);
  bool r = currency::derive_ephemeral_key_helper(offer_owner, related_tx_pub_key, offer_tx_index, ephemeral);
  CHECK_AND_ASSERT_MES(r, false, "derive_ephemeral_key_helper failed");
  CHECK_AND_ASSERT_MES(ephemeral.pub == related_tx_offer_pub_key, false, "ephemeral.pub == related_tx_offer_pub_key");
  
  crypto::generate_signature(crypto::cn_fast_hash(blob_for_sig.data(), blob_for_sig.size()), ephemeral.pub, ephemeral.sec, sig);

  return true;
}

//------------------------------------------------------------------------------

bool fill_default_offer(bc_services::offer_details& od)
{
  od.offer_type = OFFER_TYPE_PRIMARY_TO_TARGET;
  od.amount_primary = 1000000000;
  od.amount_target = 22222222;
  od.target = "USD";
  od.location_country = "US";
  od.location_city = "New York City";
  od.contacts = "skype: zina; icq: 12313212; email: zina@zina.com; mobile: +621234567834";
  od.comment = "The best ever rate in NYC!!!";
  od.payment_types = "BTC;BANK;CASH";
  od.expiration_time = 10;
  return true;
}

offers_tests::offers_tests()
{
  REGISTER_CALLBACK_METHOD(offers_tests, configure_core);
  REGISTER_CALLBACK_METHOD(offers_tests, check_offers_1);
  REGISTER_CALLBACK_METHOD(offers_tests, check_offers_updated_1);
  REGISTER_CALLBACK_METHOD(offers_tests, check_offers_updated_2);
}
#define FIRST_UPDATED_LOCATION "Washington"
#define SECOND_UPDATED_LOCATION "LA"

bool offers_tests::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_ACCOUNT(events, third_acc);
  DO_CALLBACK(events, "configure_core");
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_11, blk_1, miner_account, 10);


  //push first offers
  std::vector<currency::attachment_v> attachments;
  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_default_offer(od);
  bc_services::put_offer_into_attachment(od, attachments);
  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk, miner_account, miner_account, MK_TEST_COINS(1), blk_11, attachments);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_12, blk_11, miner_account, txs_blk);

  //push second offer
  std::vector<currency::attachment_v> attachments2;
  bc_services::offer_details od2 = AUTO_VAL_INIT(od2);
  fill_default_offer(od2);
  bc_services::put_offer_into_attachment(od2, attachments2);
  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk2, miner_account, miner_account, MK_TEST_COINS(1), blk_12, attachments2);
  //crypto::secret_key offer_secret_key = generator.last_tx_generated_secret_key;
  currency::transaction tx = txs_blk2.back();
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_13, blk_12, miner_account, txs_blk2);
  
  // Both offers has timestamp of correspoging blocks which is greather then blk_11.
  // However, "now" is close to blk_11.timestamp because there was no commands after it in event queue to adjust the time.
  // This condition helps to check rear case, when offers marked with future timestamp should not be filtered out.
  DO_CALLBACK(events, "check_offers_1");

  //now update offer
  std::vector<currency::attachment_v> attachments3;
  bc_services::update_offer uo = AUTO_VAL_INIT(uo);
  fill_default_offer(uo.of);
  uo.of.location_city = "Washington";
  uo.offer_index = 0;
  uo.tx_id = currency::get_transaction_hash(tx);

  crypto::public_key related_tx_pub_key_1 = get_tx_pub_key_from_extra(tx);
  crypto::public_key related_tx_offer_pub_key_1 = bc_services::get_offer_secure_key_by_index_from_tx(tx, 0);
  currency::keypair ephemeral_1;
  bool r = currency::derive_ephemeral_key_helper(miner_account.get_keys(), related_tx_pub_key_1, 0, ephemeral_1);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to derive_ephemeral_key_helper");
  CHECK_AND_ASSERT_THROW_MES(ephemeral_1.pub == related_tx_offer_pub_key_1, "Failed to derive_ephemeral_key_helper(pub keys missmatch)");

  blobdata bl_for_sig = bc_services::make_offer_sig_blob(uo);
  crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()), ephemeral_1.pub, ephemeral_1.sec, uo.sig);
  bc_services::put_offer_into_attachment(uo, attachments3);



  //blobdata bl_for_sig = currency::make_offer_sig_blob(uo);
  //crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()), currency::get_tx_pub_key_from_extra(tx), offer_secret_key, uo.sig);
  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk3, miner_account, miner_account, MK_TEST_COINS(1), blk_13, attachments3);
  //offer_secret_key = generator.last_tx_generated_secret_key;
  tx = txs_blk3.back();
  tx_id_1 = currency::get_transaction_hash(tx);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_14, blk_13, miner_account, txs_blk3);
  DO_CALLBACK(events, "check_offers_updated_1");

  //and update this offer once again
  std::vector<currency::attachment_v> attachments4;
  bc_services::update_offer uo2 = AUTO_VAL_INIT(uo2);
  fill_default_offer(uo2.of);
  uo2.of.location_city = "LA";
  uo2.offer_index = 0;
  uo2.tx_id = currency::get_transaction_hash(tx);
  //---------------------------
  crypto::public_key related_tx_pub_key = get_tx_pub_key_from_extra(tx);
  crypto::public_key related_tx_offer_pub_key = bc_services::get_offer_secure_key_by_index_from_tx(tx, 0);
  currency::keypair ephemeral;
  r = currency::derive_ephemeral_key_helper(miner_account.get_keys(), related_tx_pub_key, 0, ephemeral);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to derive_ephemeral_key_helper");
  CHECK_AND_ASSERT_THROW_MES(ephemeral.pub == related_tx_offer_pub_key, "Failed to derive_ephemeral_key_helper(pub keys missmatch)");

  bl_for_sig = bc_services::make_offer_sig_blob(uo2);
  crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()), ephemeral.pub, ephemeral.sec, uo2.sig);
  bc_services::put_offer_into_attachment(uo2, attachments4);
  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk4, miner_account, miner_account, MK_TEST_COINS(1), blk_14, attachments4);
  //offer_secret_key = generator.last_tx_generated_secret_key;
  tx_id_2 = currency::get_transaction_hash(txs_blk4.back());
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_15, blk_14, miner_account, txs_blk4);
  DO_CALLBACK(events, "check_offers_updated_2");


  MAKE_NEXT_BLOCK(events, blk_16, blk_15, miner_account);
  return true;
}

bool offers_tests::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool get_of_by_hash(crypto::hash& id, const std::list<bc_services::offer_details_ex>& of_list, bc_services::offer_details_ex& od)
{
  for (auto& of : of_list)
  {
    if (of.tx_hash == id)
    {
      od = of;
      return true;
    }
  }

  return false;
}



bool offers_tests::check_offers_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::list<bc_services::offer_details_ex> offers;

  get_all_offers(c, offers);
  CHECK_EQ(2, offers.size());
  return true;
}
bool offers_tests::check_offers_updated_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::list<bc_services::offer_details_ex> offers;
 
  get_all_offers(c, offers);
  CHECK_EQ(2, offers.size());
  
  bc_services::offer_details_ex target_offer;
  bool r = get_of_by_hash(tx_id_1, offers, target_offer);
  CHECK_TEST_CONDITION(r);

  CHECK_EQ(target_offer.location_city, FIRST_UPDATED_LOCATION);

  return true;
}

bool offers_tests::check_offers_updated_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::list<bc_services::offer_details_ex> offers;

  get_all_offers(c, offers);
  CHECK_EQ(2, offers.size());

  bc_services::offer_details_ex target_offer;
  bool r = get_of_by_hash(tx_id_1, offers, target_offer);
  CHECK_TEST_CONDITION(!r);
  r = get_of_by_hash(tx_id_2, offers, target_offer);
  CHECK_TEST_CONDITION(r);

  CHECK_EQ(target_offer.location_city, SECOND_UPDATED_LOCATION);

  return true;
}

//------------------------------------------------------------------------------

bool construct_block_gentime_with_timestamp(test_generator& generator, const currency::block& prev_block, const currency::account_base& acc, uint64_t timestamp, currency::block& b)
{
  bool r = false;
  crypto::hash prev_id = get_block_hash(prev_block);
  uint64_t already_generated_coins = generator.get_already_generated_coins(prev_block);
  std::vector<size_t> block_sizes;
  generator.get_last_n_block_sizes(block_sizes, prev_id, CURRENCY_REWARD_BLOCKS_WINDOW);

  size_t height = get_block_height(prev_block) + 1;

  r = generator.construct_block(b, height, prev_id, acc, timestamp, already_generated_coins, block_sizes, std::list<transaction>());
  CHECK_AND_ASSERT_MES(r, false, "construct_block_manually failed");

  return true;
}

offers_expiration_test::offers_expiration_test()
  : m_offers_ts(0)
{
  REGISTER_CALLBACK_METHOD(offers_expiration_test, c1);
}

bool offers_expiration_test::generate(std::vector<test_event_entry>& events) const
{
  // Makes sure that offers are correctly removed from blockchain storage when they're expired by specified time or by general time limit (two weeks for now).
  // Test outline:
  // 1. Create four txs with expiration time set to 0 (max), 1, 2 and 3 days.
  // 2. Rewind time by 2 days, make sure only 2 offers are expired.
  // 3. Rewind time by OFFER_MAXIMUM_LIFE_TIME - 2 days more (to be OFFER_MAXIMUM_LIFE_TIME in total), make sure all offers are expired.

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  std::vector<currency::attachment_v> attachment;
  m_offer = AUTO_VAL_INIT(m_offer);
  fill_test_offer(m_offer);
  
  bc_services::offer_details od = m_offer;

  od.expiration_time = 0;
  attachment.clear();
  bc_services::put_offer_into_attachment(od, attachment);
  MAKE_TX_ATTACH(events, tx_0, miner_acc, alice_acc, TESTS_DEFAULT_FEE, blk_0r, attachment);

  od.expiration_time = 1;
  attachment.clear();
  bc_services::put_offer_into_attachment(od, attachment);
  MAKE_TX_ATTACH(events, tx_1, miner_acc, alice_acc, TESTS_DEFAULT_FEE, blk_0r, attachment);

  od.expiration_time = 2;
  attachment.clear();
  bc_services::put_offer_into_attachment(od, attachment);
  MAKE_TX_ATTACH(events, tx_2, miner_acc, alice_acc, TESTS_DEFAULT_FEE, blk_0r, attachment);

  od.expiration_time = 3;
  attachment.clear();
  bc_services::put_offer_into_attachment(od, attachment);
  MAKE_TX_ATTACH(events, tx_3, miner_acc, alice_acc, TESTS_DEFAULT_FEE, blk_0r, attachment);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1, tx_2, tx_3 }));
  m_offers_ts = blk_1.timestamp;
  events.push_back(event_core_time(m_offers_ts));

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  // rewind time for 2 days
  uint64_t offers_ts_plus_two_days = m_offers_ts + 60 * 60 * 24 * 2 + 1;
  events.push_back(event_core_time(offers_ts_plus_two_days));

  DO_CALLBACK(events, "c1");

  return true;
}

bool offers_expiration_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bc_services::bc_offers_service* offers_service = dynamic_cast<bc_services::bc_offers_service*>(c.get_blockchain_storage().get_attachment_services_manager().get_service_by_id(BC_OFFERS_SERVICE_ID));
  CHECK_AND_ASSERT_MES(offers_service != nullptr, false, "Offers service was not registered in attachment service manager!");

  // don't need to trim_offers() here because get_all_offers() returns offers filtered by expiration_time
  // offers_service->trim_offers();

  std::list<bc_services::offer_details_ex> offers;
  bool r = get_all_offers(c, offers);
  CHECK_AND_ASSERT_MES(r, false, "get_all_offers failed");
  CHECK_V_EQ_EXPECTED_AND_ASSERT(offers.size(), 2);

  bc_services::offer_details_ex offer_0 = offers.front(), offer_3 = offers.back();
  if (offers.front().expiration_time != 0)
  {
    offer_0 = offers.back();
    offer_3 = offers.front();
  }
  CHECK_AND_ASSERT_MES(offer_0.expiration_time == 0 && offer_3.expiration_time == 3, false, "Incorrect offers expiration time left");
  r = compare_offers(m_offer, offer_0, default_offer_skip_field_mask | offers_fields::offer_field_expiration_time);
  CHECK_AND_ASSERT_MES(r, false, "offer_0 obtained via bc_service differs from original");
  r = compare_offers(m_offer, offer_3, default_offer_skip_field_mask | offers_fields::offer_field_expiration_time);
  CHECK_AND_ASSERT_MES(r, false, "offer_3 obtained via bc_service differs from original");

  // rewind time very close to the offers' lifetime limit
  test_core_time::adjust(m_offers_ts + OFFER_MAXIMUM_LIFE_TIME - 1);

  offers_service->trim_offers();
  // trim_offers should still has no effect as all offers' are within their allowed lifetime (OFFER_MAXIMUM_LIFE_TIME)
  CHECK_V_EQ_EXPECTED_AND_ASSERT(offers_service->get_offers_container().size(), 4);

  offers.clear();
  r = get_all_offers(c, offers);
  CHECK_AND_ASSERT_MES(r, false, "get_all_offers failed");
  // get_all_offers should returns only 1 offer -- the one with 0 (max) expiration time 
  CHECK_V_EQ_EXPECTED_AND_ASSERT(offers.size(), 1);
  CHECK_V_EQ_EXPECTED_AND_ASSERT(offers.front().expiration_time, 0);


  // rewind time beyond the offers' lifetime limit
  test_core_time::adjust(m_offers_ts + OFFER_MAXIMUM_LIFE_TIME + 1);

  offers_service->trim_offers();
  // no offers should left in the backend service
  CHECK_V_EQ_EXPECTED_AND_ASSERT(offers_service->get_offers_container().size(), 0);

  offers.clear();
  r = get_all_offers(c, offers);
  CHECK_AND_ASSERT_MES(r, false, "get_all_offers failed");
  // no offers should be visible via get_all_offers()
  CHECK_V_EQ_EXPECTED_AND_ASSERT(offers.size(), 0);

  return true;
}

//------------------------------------------------------------------------------

offers_filtering_1::offers_filtering_1()
{
  REGISTER_CALLBACK_METHOD(offers_filtering_1, c1);

  test_gentime_settings s = test_generator::get_test_gentime_settings();
  s.tx_max_out_amount = PREMINE_AMOUNT; // use very high max allowed output amount to avoid splitting premine to countless outputs and slowing down the test to the death
  s.miner_tx_max_outs = 11;             // limit genesis coinbase outs count for the same reason
  test_generator::set_test_gentime_settings(s);
}

bool offers_filtering_1::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);

  std::list<transaction> txs;
  size_t count = 0;
  for (; count < offers_count / 2; ++count)
  {
    bc_services::offer_details od = AUTO_VAL_INIT(od);
    fill_test_offer(od);
    std::vector<currency::attachment_v> attachment;
    bc_services::put_offer_into_attachment(od, attachment);
    MAKE_TX_LIST_ATTACH(events, txs, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_0r, attachment);
  }
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, txs);
  
  txs.clear();
  for (; count < offers_count; ++count)
  {
    bc_services::offer_details od = AUTO_VAL_INIT(od);
    fill_test_offer(od);
    std::vector<currency::attachment_v> attachment;
    bc_services::put_offer_into_attachment(od, attachment);
    MAKE_TX_LIST_ATTACH(events, txs, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_1, attachment);
  }
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, txs);


  DO_CALLBACK(events, "c1");

  return true;
}

void _dbg_print_offer_list(const std::list<bc_services::offer_details_ex>& ol)
{
  int i = 0;
  for (auto o : ol)
  {
    LOG_PRINT_L0(
      i++ << ENDL <<
      "  amount_primary: " << o.amount_primary << ENDL <<
      "  amount_target:  " << o.amount_target << ENDL <<
      "  rate:           " << static_cast<double>(o.amount_primary) / o.amount_target << ENDL <<
      "  timestamp:      " << o.timestamp);
  }
}

bool check_offers_sorting(bool reverse, uint64_t order_by, const char* field_name, const std::list<bc_services::offer_details_ex>& original_list, //field_t (*field_selector) (const bc_services::offer_details_ex&),
  bool (*field_cmp) (const bc_services::offer_details_ex&, const bc_services::offer_details_ex&), currency::core& c)
{
  std::list<bc_services::offer_details_ex> offers_sorted(original_list);
  bc_services::core_offers_filter filter = AUTO_VAL_INIT(filter);
  filter.limit = UINT64_MAX;
  filter.order_by = order_by;
  filter.reverse = reverse;
  bool r = bc_services::filter_offers_list(offers_sorted, filter, c.get_blockchain_storage().get_core_runtime_config().get_core_time());
  CHECK_AND_FORCE_ASSERT_MES(r, false, "filter_offers_list failed");
  CHECK_AND_FORCE_ASSERT_MES(offers_sorted.size() == offers_filtering_1::offers_count, false, "filter_offers_list returned wrong offer list");

  //_dbg_print_offer_list(original_list);
  //_dbg_print_offer_list(offers_sorted);

  const bc_services::offer_details_ex* prev_offer = nullptr;
  for (const bc_services::offer_details_ex& o : offers_sorted)
  {
    CHECK_AND_ASSERT_MES(prev_offer == nullptr || field_cmp(*prev_offer, o), false, "Offers list was not sorted by " << field_name);
    prev_offer = &o;
  }
  return true;
}

bool check_offers_limits(const std::list<bc_services::offer_details_ex>& original_list, uint64_t offset, uint64_t limit, uint64_t expect, currency::core& c)
{
  std::list<bc_services::offer_details_ex> offers_sorted(original_list);
  bc_services::core_offers_filter filter = AUTO_VAL_INIT(filter);
  filter.offset = offset;
  filter.limit = limit;
  filter.order_by = ORDER_BY_TIMESTAMP;
  filter.reverse = false;
  bool r = bc_services::filter_offers_list(offers_sorted, filter, c.get_blockchain_storage().get_core_runtime_config().get_core_time());
  CHECK_AND_FORCE_ASSERT_MES(r, false, "filter_offers_list failed");
  CHECK_AND_FORCE_ASSERT_MES(offers_sorted.size() == expect, false, "filter_offers_list returned wrong size of offer list: " << offers_sorted.size() << " for offset: " << offset << ", limit: " << limit << ", expect: " << expect);

  return true;
}

bool offers_filtering_1::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bc_services::bc_offers_service* offers_service = dynamic_cast<bc_services::bc_offers_service*>(c.get_blockchain_storage().get_attachment_services_manager().get_service_by_id(BC_OFFERS_SERVICE_ID));
  CHECK_AND_ASSERT_MES(offers_service != nullptr, false, "Offers service was not registered in attachment service manager!");

  std::list<bc_services::offer_details_ex> offers;
  bool r = get_all_offers(c, offers);
  CHECK_AND_FORCE_ASSERT_MES(r, false, "get_all_offers failed");
  CHECK_AND_FORCE_ASSERT_MES(offers.size() == offers_count, false, "Incorrect offers count");

#define CHECK_SORTING(field, enum_val) \
  r &= check_offers_sorting(false, enum_val, #field, offers, [](const bc_services::offer_details_ex& a, const bc_services::offer_details_ex& b)->bool { return a.field <= b.field; }, c); \
  r &= check_offers_sorting(true,  enum_val, #field, offers, [](const bc_services::offer_details_ex& a, const bc_services::offer_details_ex& b)->bool { return a.field >= b.field; }, c)

  CHECK_SORTING(timestamp,      ORDER_BY_TIMESTAMP);
  CHECK_SORTING(amount_primary, ORDER_BY_AMOUNT_PRIMARY);
  CHECK_SORTING(amount_target,  ORDER_BY_AMOUNT_TARGET);

  // amount_rate requires custom comparator
  r &= check_offers_sorting(false, ORDER_BY_AMOUNT_RATE, "amount_rate", offers,
    [](const bc_services::offer_details_ex& a, const bc_services::offer_details_ex& b)->bool { return bc_services::calculate_offer_rate(a) <= bc_services::calculate_offer_rate(b); },
    c);
  r &= check_offers_sorting(true, ORDER_BY_AMOUNT_RATE, "amount_rate", offers,
    [](const bc_services::offer_details_ex& a, const bc_services::offer_details_ex& b)->bool { return bc_services::calculate_offer_rate(a) >= bc_services::calculate_offer_rate(b); },
    c);

  CHECK_SORTING(payment_types,  ORDER_BY_PAYMENT_TYPES);
  CHECK_SORTING(contacts,       ORDER_BY_CONTACTS);
  CHECK_SORTING(location_city,  ORDER_BY_LOCATION);

  r &= check_offers_sorting(false, ORDER_BY_NAME, "target", offers,
    [](const bc_services::offer_details_ex& a, const bc_services::offer_details_ex& b)->bool { return utf8_to_lower(a.target) <= utf8_to_lower(b.target); },
    c);

  uint64_t oles[][3] =
//  offset                              limit                               expect
  {{0,                                  offers_filtering_1::offers_count,   offers_filtering_1::offers_count},
   {1,                                  offers_filtering_1::offers_count,   offers_filtering_1::offers_count-1},
   {offers_filtering_1::offers_count-1, offers_filtering_1::offers_count,   1},
   {offers_filtering_1::offers_count,   offers_filtering_1::offers_count,   0},
   {offers_filtering_1::offers_count+1, offers_filtering_1::offers_count,   0},
   {0,                                  offers_filtering_1::offers_count+1, offers_filtering_1::offers_count},
   {0,                                  offers_filtering_1::offers_count-1, offers_filtering_1::offers_count-1},
   {1,                                  offers_filtering_1::offers_count-1, offers_filtering_1::offers_count-1},
   {1,                                  offers_filtering_1::offers_count-2, offers_filtering_1::offers_count-2},
   {0,                                  0,                                  0}
  };

  for (auto& el : oles)
  {
    r &= check_offers_limits(offers, el[0], el[1], el[2], c);
  }

  return r;
}

//------------------------------------------------------------------------------

offers_handling_on_chain_switching::offers_handling_on_chain_switching()
{
  REGISTER_CALLBACK_METHOD(offers_handling_on_chain_switching, c1);
}

bool offers_handling_on_chain_switching::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  auto& miner_acc = m_accounts[MINER_ACC_IDX];
  miner_acc.generate();
  auto& alice_acc = m_accounts[ALICE_ACC_IDX];
  alice_acc.generate();
                                                                                                                          // event index
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());                                               // 0
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);                        // 2N (N == CURRENCY_MINED_MONEY_UNLOCK_WINDOW)

  MAKE_TX(events, tx_0, miner_acc, alice_acc, TESTS_DEFAULT_FEE * 70, blk_0r);                                          // 2N+1
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);                                                            // 2N+2

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);                        // 4N+2

  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer(od);
  std::vector<currency::attachment_v> attachment;
  bc_services::put_offer_into_attachment(od, attachment);
  MAKE_TX_ATTACH(events, tx_1, alice_acc, alice_acc, TESTS_DEFAULT_FEE, blk_1r, attachment);                            // 4N+3

  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);                                                                      // 4N+4
  //MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);                                                            // 4N+4

  DO_CALLBACK(events, "c1");                                                                                              // 4N+5

  return true;
}

bool offers_handling_on_chain_switching::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false, bool_stub = false;

  // there's one tx with an offer in the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(c.get_alternative_blocks_count() == 0, false, "Incorrect alt blocks count: " << c.get_alternative_blocks_count());

  // check Alice's wallet against offers, there should not be a one
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh();
  alice_wlt->scan_tx_pool(bool_stub);
  std::list<bc_services::offer_details_ex> offers;
  r = alice_wlt->get_actual_offers(offers);
  CHECK_AND_ASSERT_MES(r, false, "get_actual_offers failed");
  CHECK_AND_ASSERT_MES(offers.size() == 0, false, "incorrect offers.size() = " << offers.size());


  // mine first alt block (no txs)
  uint64_t blk_1r_height = c.get_current_blockchain_size() - 2;
  crypto::hash blk_1r_id = c.get_block_id_by_height(blk_1r_height);
  block blk_2a = AUTO_VAL_INIT(blk_2a);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>(), blk_1r_id, blk_1r_height + 1, &blk_2a);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(c.get_alternative_blocks_count() == 1, false, "Incorrect alt blocks count: " << c.get_alternative_blocks_count());

  const transaction& tx_1 = boost::get<transaction>(events[4 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3]);

  // mine second alt block (include offer tx) -- this should trigger chain switching
  block blk_3a = AUTO_VAL_INIT(blk_3a);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>({tx_1}), get_block_hash(blk_2a), blk_1r_height + 2, &blk_3a);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(c.get_alternative_blocks_count() == 1, false, "Incorrect alt blocks count: " << c.get_alternative_blocks_count());

  // check the wallet for offers, we should have an offer now
  alice_wlt->refresh();
  alice_wlt->scan_tx_pool(bool_stub);
  offers.clear();
  r = alice_wlt->get_actual_offers(offers);
  CHECK_AND_ASSERT_MES(r, false, "get_actual_offers failed");
  CHECK_AND_ASSERT_MES(offers.size() == 1, false, "incorrect offers.size() = " << offers.size());


  return true;
}

//------------------------------------------------------------------------------

offer_removing_and_selected_output::offer_removing_and_selected_output()
{
  REGISTER_CALLBACK_METHOD(offer_removing_and_selected_output, check_offers);
}

bool offer_removing_and_selected_output::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: remove two offers and make sure in both cases a transfer with minimal amount is used for that "free" operation

  bool r = false;

  m_accounts.resize(TOTAL_ACCS_COUNT);
  auto& miner_acc = m_accounts[MINER_ACC_IDX];
  miner_acc.generate();
  auto& alice_acc = m_accounts[ALICE_ACC_IDX];
  alice_acc.generate();
                                                                                                 
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());                      
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // prepare different outputs for Alice
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(4), alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(8), alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(1) * 1.2, alice_acc.get_public_address()));   // <- min #2
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(1) * 1.1, alice_acc.get_public_address()));  // <- min #1
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(3), alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(2), alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(5), alice_acc.get_public_address()));
  uint64_t destinations_amount = 0;
  for(auto& d : destinations)
    destinations_amount += d.amount;
  std::vector<tx_source_entry> sources;
  r = fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), destinations_amount + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_0, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_0);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);                                   

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  uint64_t offer_cancel_tx_selected_amount = MK_TEST_COINS(1) * 11 / 10; // min #1
  DO_CALLBACK_PARAMS(events, "check_offers", offer_cancel_tx_selected_amount);

  offer_cancel_tx_selected_amount = MK_TEST_COINS(1) * 12 / 10; // min #2
  DO_CALLBACK_PARAMS(events, "check_offers", offer_cancel_tx_selected_amount);

  return true;
}

bool offer_removing_and_selected_output::check_offers(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  uint64_t offer_cancel_tx_selected_amount = 0;
  r = epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, offer_cancel_tx_selected_amount);
  CHECK_AND_ASSERT_MES(r && offer_cancel_tx_selected_amount > 0, false, "hex_to_pod failed");

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh();
  LOG_PRINT_CYAN("Alice's transfers:" << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_0);
  uint64_t alice_start_balance = alice_wlt->balance();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bc_services::offer_details_ex od = AUTO_VAL_INIT(od);
  fill_test_offer(od);
  od.fee = TESTS_DEFAULT_FEE;

  transaction create_offer_tx = AUTO_VAL_INIT(create_offer_tx);
  LOG_PRINT_CYAN("%%%%% alice_wlt->push_offer()", LOG_LEVEL_0);
  alice_wlt->push_offer(od, create_offer_tx);
  crypto::hash create_offer_tx_id = get_transaction_hash(create_offer_tx);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("offer's put into the blockchain", "Alice", alice_wlt, alice_start_balance - od.fee, true, 1), false, "");

  // check offer, retured by get_actual_offers()
  std::list<bc_services::offer_details_ex> offers;
  r = alice_wlt->get_actual_offers(offers);
  CHECK_AND_ASSERT_MES(r && offers.size() == 1, false, "alice_wlt->get_actual_offers failed, offers.size() == " << offers.size());
  CHECK_AND_ASSERT_MES(compare_offers(od, offers.front()), false, "");
  
  // cancel offer
  transaction cancel_offer_tx = AUTO_VAL_INIT(cancel_offer_tx);
  LOG_PRINT_CYAN("%%%%% alice_wlt->cancel_offer_by_id()", LOG_LEVEL_0);
  uint64_t cancellation_fee = TESTS_DEFAULT_FEE * 11 / 10;
  alice_wlt->cancel_offer_by_id(create_offer_tx_id, 0, cancellation_fee, cancel_offer_tx);

  // make sure used input is the expected one
  CHECK_AND_ASSERT_MES(cancel_offer_tx.vin.size() == 1, false, "cancel_offer_tx.vin.size() == " << cancel_offer_tx.vin.size());
  CHECKED_GET_SPECIFIC_VARIANT(cancel_offer_tx.vin[0], txin_to_key, in, false);
  CHECK_AND_ASSERT_MES(in.amount == offer_cancel_tx_selected_amount, false, "Offer's cancellation tx uses input with amount " << print_money_brief(in.amount) << ", while expected amount to be used is: " << print_money_brief(offer_cancel_tx_selected_amount));

  // make sure offer's cancellation tx has correct fee
  CHECK_AND_ASSERT_MES(get_tx_fee(cancel_offer_tx) == cancellation_fee, false, "get_tx_fee(cancel_offer_tx) = " << get_tx_fee(cancel_offer_tx));

  // add it to the blockchain
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // Alice's banace should not change
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("offer has been cancelled, ", "Alice", alice_wlt, alice_start_balance - od.fee - cancellation_fee, true, 1), false, "");

  // get_actual_offers should return empty list
  offers.clear();
  r = alice_wlt->get_actual_offers(offers);
  CHECK_AND_ASSERT_MES(r && offers.size() == 0, false, "alice_wlt->get_actual_offers failed, offers.size() = " << offers.size());

  return true;
}
//------------------------------------------------------------------------------

offers_and_stuck_txs::offers_and_stuck_txs()
{
  REGISTER_CALLBACK_METHOD(offers_and_stuck_txs, c1);
  REGISTER_CALLBACK_METHOD(offers_and_stuck_txs, check_offers);
}

bool offers_and_stuck_txs::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // add an offer -- tx_1
  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer(od);
  od.comment = "new";
  std::vector<attachment_v> attachments;
  bc_services::put_offer_into_attachment(od, attachments);
  MAKE_TX_ATTACH(events, tx_1, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_0r, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  
  // make tx deleting the offer -- tx_2, don't add it to the blockchain
  bc_services::cancel_offer co = AUTO_VAL_INIT(co);
  co.tx_id = get_transaction_hash(tx_1);
  co.offer_index = 0;
  r = sign_an_offer(tx_1, co.offer_index, miner_acc.get_keys(), bc_services::make_offer_sig_blob(co), co.sig);
  CHECK_AND_ASSERT_MES(r, false, "sign_an_offer failed");
  attachments.clear();
  bc_services::put_offer_into_attachment(co, attachments);
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  uint64_t fee = 7 * TESTS_DEFAULT_FEE;
  r = construct_tx_to_key(m_hardforks, events, tx_2, blk_1, miner_acc, miner_acc, TESTS_DEFAULT_FEE, fee, 0, 0, empty_extra, attachments);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  events.push_back(tx_2);

  CHECK_AND_ASSERT_MES(get_tx_fee(tx_2) == fee, false, "Incorrect fee: " << print_money_brief(get_tx_fee(tx_2)) << ", expected: " << print_money_brief(fee));

  // don't include tx_2 with offer deletion yet
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  size_t expected_tx_count = 1;
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", expected_tx_count);
  DO_CALLBACK(events, "print_tx_pool");

  // make tx updating the offer
  bc_services::update_offer uo = AUTO_VAL_INIT(uo);
  uo.tx_id = get_transaction_hash(tx_1);
  uo.offer_index = 0;
  uo.of = od;
  uo.of.comment = "update";
  r = sign_an_offer(tx_1, co.offer_index, miner_acc.get_keys(), bc_services::make_offer_sig_blob(uo), uo.sig);
  CHECK_AND_ASSERT_MES(r, false, "sign_an_offer failed");
  attachments.clear();
  bc_services::put_offer_into_attachment(uo, attachments);
  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  fee = 90 * TESTS_DEFAULT_FEE;
  r = construct_tx_to_key(m_hardforks, events, tx_3, blk_2, miner_acc, miner_acc, TESTS_DEFAULT_FEE, fee, 0, 0, empty_extra, attachments);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  events.push_back(tx_3);

  CHECK_AND_ASSERT_MES(get_tx_fee(tx_3) == fee, false, "Incorrect fee: " << print_money_brief(get_tx_fee(tx_3)) << ", expected: " << print_money_brief(fee));

  expected_tx_count = 2;
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", expected_tx_count);
  DO_CALLBACK(events, "print_tx_pool");

  DO_CALLBACK(events, "c1");

  return true;
}

bool offers_and_stuck_txs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return true;
}

bool offers_and_stuck_txs::check_offers(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return true;
}

//------------------------------------------------------------------------------

offers_updating_hack::offers_updating_hack()
{
  REGISTER_CALLBACK_METHOD(offers_updating_hack, update_foreign_offer);
  REGISTER_CALLBACK_METHOD(offers_updating_hack, delete_foreign_offer);
}

bool offers_updating_hack::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);
  
  transaction tx_a = AUTO_VAL_INIT(tx_a);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), TESTS_DEFAULT_FEE * 1000, 10, TESTS_DEFAULT_FEE, tx_a);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_a);
  transaction tx_b = AUTO_VAL_INIT(tx_b);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), TESTS_DEFAULT_FEE * 1000, 10, TESTS_DEFAULT_FEE, tx_b);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_b);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_a, tx_b }));
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // Alice adds and offer
  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer(od);
  od.comment = "new";
  std::vector<attachment_v> attachments;
  bc_services::put_offer_into_attachment(od, attachments);
  MAKE_TX_ATTACH(events, tx_1, alice_acc, alice_acc, TESTS_DEFAULT_FEE, blk_1r, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  DO_CALLBACK(events, "check_tx_pool_empty");

  // Alice updates the offer
  bc_services::update_offer uo = AUTO_VAL_INIT(uo);
  uo.tx_id = get_transaction_hash(tx_1);
  uo.offer_index = 0;
  uo.of = od;
  uo.of.comment = "update by Alice";
  r = sign_an_offer(tx_1, uo.offer_index, alice_acc.get_keys(), bc_services::make_offer_sig_blob(uo), uo.sig);
  CHECK_AND_ASSERT_MES(r, false, "sign_an_offer failed");
  attachments.clear();
  bc_services::put_offer_into_attachment(uo, attachments);
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  uint64_t fee = 90 * TESTS_DEFAULT_FEE;
  r = construct_tx_to_key(m_hardforks, events, tx_2, blk_2, alice_acc, alice_acc, TESTS_DEFAULT_FEE, fee, 0, 0, empty_extra, attachments);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  events.push_back(tx_2);
  CHECK_AND_ASSERT_MES(get_tx_fee(tx_2) == fee, false, "Incorrect fee: " << print_money_brief(get_tx_fee(tx_2)) << ", expected: " << print_money_brief(fee));
  // don't add tx_2 to the blockchain yet


  // ======= Case 1
  DO_CALLBACK(events, "update_foreign_offer");

  // ======= Prepare for case 2
  // these block should cause chain switching
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5a, blk_4a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_6a, blk_5a, miner_acc);
  DO_CALLBACK(events, "clear_tx_pool"); // remove alt txs
  events.push_back(tx_2); // put tx_2 into the pool back
  size_t expected_tx_count = 1;
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", expected_tx_count);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_6a), get_block_hash(blk_6a)));

  // ======= Case 2
  DO_CALLBACK(events, "delete_foreign_offer");

  return true;
}

bool offers_updating_hack::update_foreign_offer(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  bc_services::bc_offers_service* offers_service = dynamic_cast<bc_services::bc_offers_service*>(c.get_blockchain_storage().get_attachment_services_manager().get_service_by_id(BC_OFFERS_SERVICE_ID));
  CHECK_AND_ASSERT_MES(offers_service != nullptr, false, "Offers service was not registered in attachment service manager!");

  //
  // Attacker's part -- modify Alice's offers without Alice's private keys
  //

  // get offer update tx from the pool (it's not added yet to the blockchain)
  std::list<transaction> txs;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions(txs), false, "");
  CHECK_AND_ASSERT_MES(txs.size() == 1, false, "Incorrect tx pool count: " << txs.size());
  LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));

  const transaction& offer_update_tx = txs.back();
  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  r = get_type_in_variant_container(offer_update_tx.attachment, tsa);
  CHECK_AND_ASSERT_MES(r, false, "get_type_in_variant_container failed");
  CHECK_AND_ASSERT_MES(tsa.service_id == BC_OFFERS_SERVICE_ID && tsa.instruction == BC_OFFERS_SERVICE_INSTRUCTION_UPD, false, "invalid tsa: " << tsa.service_id << ", " << tsa.instruction);

  // unpack tsa with update offer
  bc_services::update_offer uo = AUTO_VAL_INIT(uo);
  std::string json_buff;
  CHECK_AND_ASSERT_MES(epee::zlib_helper::unpack(tsa.body, json_buff), false, "");
  CHECK_AND_ASSERT_MES(epee::serialization::load_t_from_json(uo, json_buff), false, "");
  
  // Case 1: Bob modifies Alice's offer
  
  // modify an offer and don't resign
  uo.of.comment = "update by Bob";

  // repack
  std::vector<attachment_v> attachments;
  bc_services::put_offer_into_attachment(uo, attachments);

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh();

  // make a tx with this attachment (note: tsa.security is signed using Bob's keys, can be resign later using Alice's public address)
  transaction tx = AUTO_VAL_INIT(tx);
  bob_wlt->transfer(std::vector<tx_destination_entry>({ tx_destination_entry(TESTS_DEFAULT_FEE * 3, m_accounts[BOB_ACC_IDX].get_public_address()) }), 0, 0, TESTS_DEFAULT_FEE * 81, empty_extra, attachments,
    tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, 0, true, 0, false);

  // don't resign yet

  // send to network
  tx_verification_context tvc = AUTO_VAL_INIT(tvc);
  r = c.handle_incoming_tx(tx_to_blob(tx), tvc, false);
  CHECK_AND_ASSERT_MES(r && tvc.m_added_to_pool, false, "handle_incoming_tx failed: r = " << r << ", tvc.m_added_to_pool = " << tvc.m_added_to_pool);
  LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));

  // put in a block 'tx', leave Alice's tx in the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>({ tx })), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));

  // check offers

  std::list<bc_services::offer_details_ex> offers;
  r = get_all_offers(c, offers);
  CHECK_AND_ASSERT_MES(r, false, "get_all_offers failed");

  CHECK_AND_ASSERT_MES(offers.size() == 1, false, "Incorrect offers count: " << offers.size());
  bc_services::offer_details_ex offer_0 = offers.front();

  LOG_PRINT_YELLOW("\n\n!!!!!!!!!!!! offer_0.comment: " << offer_0.comment << "\n\n", LOG_LEVEL_0);

  return true;
}


bool offers_updating_hack::delete_foreign_offer(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  bc_services::bc_offers_service* offers_service = dynamic_cast<bc_services::bc_offers_service*>(c.get_blockchain_storage().get_attachment_services_manager().get_service_by_id(BC_OFFERS_SERVICE_ID));
  CHECK_AND_ASSERT_MES(offers_service != nullptr, false, "Offers service was not registered in attachment service manager!");

  //
  // Attacker's part -- modify Alice's offers without Alice's private keys
  //

  // get offer update tx from the pool (it's not added yet to the blockchain)
  std::list<transaction> txs;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions(txs), false, "");
  CHECK_AND_ASSERT_MES(txs.size() == 1, false, "Incorrect tx pool count: " << txs.size());
  LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));

  const transaction& offer_update_tx = txs.back();
  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  r = get_type_in_variant_container(offer_update_tx.attachment, tsa);
  CHECK_AND_ASSERT_MES(r, false, "get_type_in_variant_container failed");
  CHECK_AND_ASSERT_MES(tsa.service_id == BC_OFFERS_SERVICE_ID && tsa.instruction == BC_OFFERS_SERVICE_INSTRUCTION_UPD, false, "invalid tsa: " << tsa.service_id << ", " << tsa.instruction);

  // unpack tsa with update offer
  bc_services::update_offer uo = AUTO_VAL_INIT(uo);
  std::string json_buff;
  CHECK_AND_ASSERT_MES(epee::zlib_helper::unpack(tsa.body, json_buff), false, "");
  CHECK_AND_ASSERT_MES(epee::serialization::load_t_from_json(uo, json_buff), false, "");
  
  // Case 2: Bob deletes Alice's offer
  bc_services::cancel_offer co = AUTO_VAL_INIT(co);
  co.tx_id = uo.tx_id;
  co.offer_index = uo.offer_index;
  co.sig = uo.sig;

  std::vector<attachment_v> attachments;
  bc_services::put_offer_into_attachment(co, attachments);

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh();

  // make a tx with this attachment (note: tsa.security is signed using Bob's keys, can be resign later using Alice's public address)
  transaction tx_c = AUTO_VAL_INIT(tx_c);
  bob_wlt->transfer(std::vector<tx_destination_entry>({ tx_destination_entry(TESTS_DEFAULT_FEE * 3, m_accounts[BOB_ACC_IDX].get_public_address()) }), 0, 0, TESTS_DEFAULT_FEE * 19, empty_extra, attachments,
    tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx_c, 0, true, 0, true);

  LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));

  // put in a block 'tx', leave Alice's tx in the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>({ tx_c })), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));

  std::list<bc_services::offer_details_ex> offers;
  r = get_all_offers(c, offers);
  CHECK_AND_ASSERT_MES(r, false, "get_all_offers failed");

  LOG_PRINT_YELLOW("\n\n!!!!!!!!!!!! offers.size(): " << offers.size() << "\n\n", LOG_LEVEL_0);

  return true;
}

//------------------------------------------------------------------------------

bool offers_multiple_update::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make an offer then update it twice (two very similar update requests). Make sure raw/normal offers' count is correct.

  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  
  // Miner adds and offer
  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer(od);
  od.comment = "new";
  std::vector<attachment_v> attachments;
  bc_services::put_offer_into_attachment(od, attachments);
  MAKE_TX_ATTACH(events, tx_1, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_1, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_1);

  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(1, 1));

  // Miner updates the offer
  bc_services::update_offer uo = AUTO_VAL_INIT(uo);
  uo.tx_id = get_transaction_hash(tx_1);
  uo.offer_index = 0;
  uo.of = od;
  uo.of.comment = "update";
  r = sign_an_offer(tx_1, uo.offer_index, miner_acc.get_keys(), bc_services::make_offer_sig_blob(uo), uo.sig);
  CHECK_AND_ASSERT_MES(r, false, "sign_an_offer failed");
  attachments.clear();
  bc_services::put_offer_into_attachment(uo, attachments);
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  uint64_t fee = 14 * TESTS_DEFAULT_FEE;
  r = construct_tx_to_key(m_hardforks, events, tx_2, blk_2, miner_acc, miner_acc, TESTS_DEFAULT_FEE, fee, 0, 0, empty_extra, attachments);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  events.push_back(tx_2);

  // Miner updates the same the offer once more
  uo.of.comment = "update 2";
  r = sign_an_offer(tx_1, uo.offer_index, miner_acc.get_keys(), bc_services::make_offer_sig_blob(uo), uo.sig);
  CHECK_AND_ASSERT_MES(r, false, "sign_an_offer failed");
  attachments.clear();
  bc_services::put_offer_into_attachment(uo, attachments);
  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  fee = 19 * TESTS_DEFAULT_FEE;
  r = construct_tx_to_key(m_hardforks, events, tx_3, blk_2, miner_acc, miner_acc, TESTS_DEFAULT_FEE, fee, 0, 0, empty_extra, attachments);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  events.push_back(tx_3);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, std::list<transaction>({ tx_2, tx_3 }));

  // raw count should increase by 1 (=2), normal cound should remain the same (=1)
  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(1, 2));

  return true;
}

//------------------------------------------------------------------------------

bool offer_sig_validity_in_update_and_cancel::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check that changing any field of signed update_offer or cancel_offer made signature invalid

  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  
  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);

  // del offer: valid sig, change  tx_id, change offer_index
  // upd offer: valid sig, change  tx_id, change offer_index, change of

  // Miner adds two offers
  bc_services::offer_details od_1 = AUTO_VAL_INIT(od_1);
  fill_test_offer(od_1);
  od_1.comment = "1";
  std::vector<attachment_v> attachments;
  bc_services::put_offer_into_attachment(od_1, attachments);
  MAKE_TX_ATTACH(events, tx_offer_1, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_1, attachments);

  bc_services::offer_details od_2 = AUTO_VAL_INIT(od_2);
  fill_test_offer(od_2);
  od_2.comment = "2";
  attachments.clear();
  bc_services::put_offer_into_attachment(od_2, attachments);
  MAKE_TX_ATTACH(events, tx_offer_2, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_1, attachments);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, std::list<transaction>({ tx_offer_1, tx_offer_2 }));

  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(2, 2));


  // Miner updates the offer
  bc_services::update_offer uo_good = AUTO_VAL_INIT(uo_good);
  uo_good.tx_id = get_transaction_hash(tx_offer_1);
  uo_good.offer_index = 0;
  uo_good.of = od_1;
  uo_good.of.comment = "update";
  r = sign_an_offer(tx_offer_1, uo_good.offer_index, miner_acc.get_keys(), bc_services::make_offer_sig_blob(uo_good), uo_good.sig);
  CHECK_AND_ASSERT_MES(r, false, "sign_an_offer failed");
  
  // 1. update offer with wrong tx_id (same sig)
  bc_services::update_offer uo_bad = uo_good;
  uo_bad.tx_id = get_transaction_hash(tx_offer_2); // tx_id of valid but another offer
  attachments.clear();
  bc_services::put_offer_into_attachment(uo_bad, attachments);
  MAKE_TX_ATTACH_FEE(events, tx_wrong_uo_1, miner_acc, miner_acc, TESTS_DEFAULT_FEE, 3 * TESTS_DEFAULT_FEE, blk_2, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_wrong_uo_1);

  // offers count shoudn't change
  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(2, 2));

  // 2. update offer with wrong offer_index (same sig)
  uo_bad = uo_good;
  uo_bad.offer_index = 1;
  attachments.clear();
  bc_services::put_offer_into_attachment(uo_bad, attachments);
  MAKE_TX_ATTACH_FEE(events, tx_wrong_uo_2, miner_acc, miner_acc, TESTS_DEFAULT_FEE, 4 * TESTS_DEFAULT_FEE, blk_3, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_wrong_uo_2);

  // offers count shoudn't change
  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(2, 2));

  // 3. update offer with wrong 'of' (same sig)
  uo_bad = uo_good;
  uo_bad.of.comment = "update bad";
  attachments.clear();
  bc_services::put_offer_into_attachment(uo_bad, attachments);
  MAKE_TX_ATTACH_FEE(events, tx_wrong_uo_3, miner_acc, miner_acc, TESTS_DEFAULT_FEE, 5 * TESTS_DEFAULT_FEE, blk_4, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4, miner_acc, tx_wrong_uo_3);
  //MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);

  // offers count shoudn't change
  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(2, 2));

  // 4. update offer with valid sig (sanity check)
  attachments.clear();
  bc_services::put_offer_into_attachment(uo_good, attachments);
  MAKE_TX_ATTACH_FEE(events, tx_good_uo, miner_acc, miner_acc, TESTS_DEFAULT_FEE, 6 * TESTS_DEFAULT_FEE, blk_5, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_6, blk_5, miner_acc, tx_good_uo);

  // offers raw count should change, normal count - not
  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(2, 3));



  // miner removes offer
  bc_services::cancel_offer co_good = AUTO_VAL_INIT(co_good);
  co_good.tx_id = get_transaction_hash(tx_good_uo);
  co_good.offer_index = 0;
  r = sign_an_offer(tx_good_uo, co_good.offer_index, miner_acc.get_keys(), bc_services::make_offer_sig_blob(co_good), co_good.sig);
  CHECK_AND_ASSERT_MES(r, false, "sign_an_offer failed");

  // 5. remove offer with invalid tx_id
  bc_services::cancel_offer co_bad = co_good;
  co_bad.tx_id = get_transaction_hash(tx_offer_2); // another tx
  attachments.clear();
  bc_services::put_offer_into_attachment(co_bad, attachments);
  MAKE_TX_ATTACH_FEE(events, tx_wrong_co_1, miner_acc, miner_acc, TESTS_DEFAULT_FEE, 7 * TESTS_DEFAULT_FEE, blk_6, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_7, blk_6, miner_acc, tx_wrong_co_1);

  // offers count shoudn't change
  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(2, 3));

  // 6. remove offer with invalid 
  co_bad = co_good;
  co_bad.offer_index = 1;
  attachments.clear();
  bc_services::put_offer_into_attachment(co_bad, attachments);
  MAKE_TX_ATTACH_FEE(events, tx_wrong_co_2, miner_acc, miner_acc, TESTS_DEFAULT_FEE, 8 * TESTS_DEFAULT_FEE, blk_7, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_8, blk_7, miner_acc, tx_wrong_co_2);

  // offers count shoudn't change
  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(2, 3));

  // 7. remove offer normally (sanity check)
  attachments.clear();
  bc_services::put_offer_into_attachment(co_good, attachments);
  MAKE_TX_ATTACH_FEE(events, tx_good_co, miner_acc, miner_acc, TESTS_DEFAULT_FEE, 9 * TESTS_DEFAULT_FEE, blk_8, attachments);
  MAKE_NEXT_BLOCK_TX1(events, blk_9, blk_8, miner_acc, tx_good_co);

  // offers raw count shoudn't change, normal count should decreased
  DO_CALLBACK_PARAMS(events, "check_offers_count", offers_count_param(1, 3));


  return true;
}

//------------------------------------------------------------------------------

offer_lifecycle_via_tx_pool::offer_lifecycle_via_tx_pool()
{
  REGISTER_CALLBACK_METHOD(offer_lifecycle_via_tx_pool, c1);
}

bool offer_lifecycle_via_tx_pool::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: go though typical offer life cycle using wallet member functions. Each operation will put a tx into the pool, then a block will be mined to commit it into the blockchain.

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool offer_lifecycle_via_tx_pool::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  miner_wlt->refresh();

  account_base someone;
  someone.generate();
  account_public_address some_addr = someone.get_public_address(); // just random public address

  uint64_t miner_start_balance = miner_wlt->balance();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));

  bc_services::offer_details_ex od = AUTO_VAL_INIT(od);
  fill_test_offer(od);
  od.comment = "1";
  od.fee = TESTS_DEFAULT_FEE * 3;

  // create an offer
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  TRY_ENTRY()
  miner_wlt->push_offer(od, tx_1);
  CATCH_ENTRY2(false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));
  r = mine_next_pow_block_in_playtime(some_addr, c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("After putting offer push tx into the blockchain", "miner", miner_wlt, miner_start_balance - od.fee), false, "");

  CHECK_AND_ASSERT_MES(check_offers_count(c, 0, std::vector<test_event_entry>({ callback_entry("", epee::string_tools::pod_to_hex(offers_count_param(1, 1))) })), false, "");

  // update the offer
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  od.comment = "UPDATE";
  TRY_ENTRY()
  miner_wlt->update_offer_by_id(get_transaction_hash(tx_1), 0, od, tx_2);
  CATCH_ENTRY2(false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));
  r = mine_next_pow_block_in_playtime(some_addr, c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("After updating offer push tx into the blockchain", "miner", miner_wlt, miner_start_balance - od.fee * 2), false, "");

  CHECK_AND_ASSERT_MES(check_offers_count(c, 0, std::vector<test_event_entry>({ callback_entry("", epee::string_tools::pod_to_hex(offers_count_param(1, 2))) })), false, "");


  // update the offer once again
  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  od.comment = "UPDATE2";
  TRY_ENTRY()
  miner_wlt->update_offer_by_id(get_transaction_hash(tx_2), 0, od, tx_3);
  CATCH_ENTRY2(false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));
  r = mine_next_pow_block_in_playtime(some_addr, c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("After second update offer push tx into the blockchain", "miner", miner_wlt, miner_start_balance - od.fee * 3), false, "");

  CHECK_AND_ASSERT_MES(check_offers_count(c, 0, std::vector<test_event_entry>({ callback_entry("", epee::string_tools::pod_to_hex(offers_count_param(1, 3))) })), false, "");


  // cancel the offer
  transaction tx_4 = AUTO_VAL_INIT(tx_4);
  TRY_ENTRY()
  miner_wlt->cancel_offer_by_id(get_transaction_hash(tx_3), 0, TESTS_DEFAULT_FEE, tx_4);
  CATCH_ENTRY2(false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));
  r = mine_next_pow_block_in_playtime(some_addr, c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Invalid tx pool count: " << c.get_pool_transactions_count() << ENDL << "tx pool:" << ENDL << c.get_tx_pool().print_pool(true));

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("After offer's cancellation push tx into the blockchain", "miner", miner_wlt, miner_start_balance - od.fee * 3 - TESTS_DEFAULT_FEE), false, "");

  CHECK_AND_ASSERT_MES(check_offers_count(c, 0, std::vector<test_event_entry>({ callback_entry("", epee::string_tools::pod_to_hex(offers_count_param(0, 3))) })), false, "");

  return true;
}

//------------------------------------------------------------------------------

offer_cancellation_with_zero_fee::offer_cancellation_with_zero_fee()
{
  REGISTER_CALLBACK_METHOD(offer_cancellation_with_zero_fee, c1);
}

bool offer_cancellation_with_zero_fee::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that an offer cannot be cancelled using zero fee

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool offer_cancellation_with_zero_fee::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  miner_wlt->refresh();
  LOG_PRINT_CYAN("Miners's transfers:" << ENDL << miner_wlt->dump_trunsfers(), LOG_LEVEL_0);
  uint64_t miner_start_balance = miner_wlt->balance();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bc_services::offer_details_ex od = AUTO_VAL_INIT(od);
  fill_test_offer(od);
  od.fee = TESTS_DEFAULT_FEE * 2;

  transaction create_offer_tx = AUTO_VAL_INIT(create_offer_tx);
  LOG_PRINT_CYAN("%%%%% miner_wlt->push_offer()", LOG_LEVEL_0);
  miner_wlt->push_offer(od, create_offer_tx);
  crypto::hash create_offer_tx_id = get_transaction_hash(create_offer_tx);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[ALICE_ACC_IDX].get_public_address(), c), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("offer's put into the blockchain", "Miner", miner_wlt, miner_start_balance - od.fee, true, 1), false, "");

  // check offer, retured by get_actual_offers()
  std::list<bc_services::offer_details_ex> offers;
  r = miner_wlt->get_actual_offers(offers);
  CHECK_AND_ASSERT_MES(r && offers.size() == 1, false, "miner_wlt->get_actual_offers failed, offers.size() == " << offers.size());
  CHECK_AND_ASSERT_MES(compare_offers(od, offers.front()), false, "");
  
  // cancel offer
  transaction cancel_offer_tx = AUTO_VAL_INIT(cancel_offer_tx);
  bool exception_caught = false;
  try
  {
    LOG_PRINT_CYAN("%%%%% miner_wlt->cancel_offer_by_id()", LOG_LEVEL_0);
    miner_wlt->cancel_offer_by_id(create_offer_tx_id, 0, 0 /* <-- zero fee */, cancel_offer_tx);
  }
  catch (std::exception& e)
  {
    LOG_PRINT_L0("Caught an expected exception: " << e.what());
    exception_caught = true;
  }

  CHECK_AND_ASSERT_MES(exception_caught, false, "An exception on calcel_offer_by_id() was not caught as expected");

  // make sure no new txs in the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // Make sure balance was no chaged by incorrect offer cancellation
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("offer has been cancelled, ", "Miner", miner_wlt, miner_start_balance - od.fee, true, 0), false, "");

  return true;
}
