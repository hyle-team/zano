// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <unordered_map>


#include "transactions_flow_test.h"
#include "include_base_utils.h"
using namespace epee;
#include "console_handler.h"
#include "storages/http_abstract_invoke.h"
#include "wallet/wallet2.h"
#include "string_coding.h"
#include "math_helper.h"
#include "common/variant_helper.h"

using namespace currency;
namespace ph = boost::placeholders;

#define TESTS_DEFAULT_FEE                   TX_DEFAULT_FEE 

std::string get_random_hex_text(size_t len)
{
  std::string buff(len / 2, 0);
  crypto::generate_random_bytes(len / 2, (void*)buff.data());
  return string_tools::buff_to_hex_nodelimer(buff);
}


std::wstring generate_random_wallet_name()
{
  std::wstringstream ss;
  ss << boost::uuids::random_generator()();
  return ss.str();
}

inline uint64_t random(const uint64_t max_value) {
  return (uint64_t(rand()) ^
          (uint64_t(rand())<<16) ^
          (uint64_t(rand())<<32) ^
          (uint64_t(rand())<<48)) % max_value;
}

bool do_send_money(tools::wallet2& w1, tools::wallet2& w2, size_t mix_in_factor, uint64_t amount_to_transfer, transaction& tx, size_t parts=1)
{
  CHECK_AND_ASSERT_MES(parts > 0, false, "parts must be > 0");

  std::vector<currency::tx_destination_entry> dsts;
  dsts.reserve(parts);
  uint64_t amount_used = 0;
  uint64_t max_part = amount_to_transfer / parts;

  for (size_t i = 0; i < parts; ++i)
  {
    currency::tx_destination_entry de;
    de.addr.push_back(w2.get_account().get_keys().account_address);

    if (i < parts - 1)
      de.amount = random(max_part);
    else
      de.amount = amount_to_transfer - amount_used;
    amount_used += de.amount;

    //std::cout << "PARTS (" << amount_to_transfer << ") " << amount_used << " " << de.amount << std::endl;

    dsts.push_back(de);
  }

  try
  {  
    uint64_t ticks = epee::misc_utils::get_tick_count();
    w1.transfer(dsts, mix_in_factor, 0, w1.get_core_runtime_config().tx_default_fee, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(w1.get_core_runtime_config().tx_default_fee), tx);
    uint64_t ticks_for_tx = epee::misc_utils::get_tick_count() - ticks;
    LOG_PRINT_GREEN("Send tx took " << ticks_for_tx << "ms", LOG_LEVEL_0);
    return true;
  }
  catch (const std::exception& e)
  {
    LOG_PRINT_RED("Failed to send tx: " << e.what(), LOG_LEVEL_0);
    return false;
  }
}

bool do_register_alias(tools::wallet2& w1, const std::string& alias_name, const std::string& comment, const account_public_address& address, transaction& tx)
{
  try
  {

    uint64_t ticks = epee::misc_utils::get_tick_count();
    currency::extra_alias_entry eae = AUTO_VAL_INIT(eae);
    eae.m_address = address;
    eae.m_alias = alias_name;
    eae.m_text_comment = comment;
    w1.request_alias_registration(eae, tx, TX_DEFAULT_FEE, currency::get_alias_coast_from_fee(eae.m_alias, TX_DEFAULT_FEE*10));
    uint64_t ticks_for_tx = epee::misc_utils::get_tick_count() - ticks;
    LOG_PRINT_GREEN("Send alias_registration_tx took " << ticks_for_tx << "ms", LOG_LEVEL_0);
    return true;
  }
  catch (const std::exception& e)
  {
    LOG_PRINT_RED("Failed to send tx(alias): " << e.what(), LOG_LEVEL_0);
    return false;
  }
}

bool do_register_offer(tools::wallet2& w1, transaction& tx)
{
  const char* currencies[] = { "USD", "EUR", "CNY", "INR", "BRR", "GBP", "CHF", "JPY", "RUR", "BTC", "AUD", "CAD", "SGD", "MYR", "AZN", "ALL", "DZD", "AOA", "ARS", "AMD", "AWG", "AFA", "BSD", "BDT", "BBD", "BHD", "BZD", "BYB", "BGL", "BOB", "BWP", "BND", "BMD", "BIF", "VUV", "HUF", "VEB", "XCD", "VND", "HTG", "GMD", "GHC", "GTQ", "GNF", "GIP", "HML", "GEL", "ANG", "DKK", "RSD", "AED", "STD", "ZWD", "KYD", "SBD", "TTD", "FJD", "DOP", "EGP", "ZMK", "NIO", "ILS", "IDR", "JOD", "IQD", "IRR", "IEP", "ISK", "YER", "KHR", "QAR", "KES", "PGK", "CYP", "COP", "KMF", "BAM", "CRC", "CUP", "KWD", "HRK", "KGS", "MMK", "LAK", "LVL", "SLL", "LRD", "LBP", "LYD", "LTL", "LSL", "MUR", "MRO", "MKD", "MWK", "MGF", "MVR", "MTL", "MAD", "MXN", "MZM", "MDL", "MNT", "ERN", "BTN", "NPR", "NGN", "NZD", "PEN", "TWD", "NOK", "OMR", "TOP", "PKR", "PYG", "PLZ", "ROL", "SVC", "WST", "SAR", "SZL", "KPW", "SCR", "SYP", "SOS", "SDD", "SRG", "THB", "TZS", "KZT", "SIT", "TND", "TRY", "TMM", "UGS", "UZS", "UAH", "UYP", "PHP", "DJF", "XOF", "XAF", "XOP", "XPF", "RWF", "FKP", "CZK", "CLP", "SEK", "LKR", "ESC", "CVE", "EEK", "ETB", "ZAR", "KRW", "JMD" };
  const char* countries[] = { "ZD", "AF", "AX", "AL", "DZ", "AS", "AD", "AO", "AI", "AG", "AR", "AM", "AW", "AU", "AT", "AZ", "BS", "BH", "BD", "BB", "BY", "BE", "BZ", "BJ", "BM", "BT", "BO", "BQ", "BA", "BW", "BV", "BR", "IO", "UM", "VG", "BN", "BG", "BF", "BI", "KH", "CM", "CA", "CV", "KY", "CF", "TD", "CL", "CN", "CX", "CC", "CO", "KM", "CG", "CD", "CK", "CR", "HR", "CU", "CW", "CY", "CZ", "DK", "DJ", "DM", "DO", "EC", "EG", "SV", "GQ", "ER", "EE", "ET", "FK", "FO", "FJ", "FI", "FR", "GF", "PF", "TF", "GA", "GM", "GE", "DE", "GH", "GI", "GR", "GL", "GD", "GP", "GU", "GT", "GG", "GN", "GW", "GY", "HT", "HM", "HN", "HK", "HU", "IS", "IN", "ID", "CI", "IR", "IQ", "IE", "IM", "IL", "IT", "JM", "JP", "JE", "JO", "KZ", "KE", "KI", "KW", "KG", "LA", "LV", "LB", "LS", "LR", "ZF", "LY", "LI", "LT", "LU", "MO", "MK", "MG", "MW", "MY", "MV", "ML", "MT", "MH", "MQ", "MR", "MU", "YT", "MX", "FM", "MD", "MC", "MN", "ME", "MS", "MA", "MZ", "MM", "NA", "NR", "NP", "NL", "NC", "NZ", "NI", "NE", "NG", "NU", "NF", "KP", "MP", "NO", "OM", "PK", "PW", "PS", "PA", "PG", "PY", "PE", "PH", "PN", "PL", "PT", "PR", "QA", "XK", "RE", "RO", "RU", "RW", "BL", "SH", "KN", "LC", "MF", "PM", "VC", "WS", "SM", "ST", "SA", "SN", "RS", "SC", "SL", "SG", "SX", "SK", "SI", "SB", "SO", "ZA", "GS", "KR", "ZE", "SS", "ES", "LK", "SD", "SR", "SJ", "SZ", "SE", "CH", "SY", "TW", "TJ", "TZ", "TH", "TL", "TG", "TK", "TO", "TT", "TN", "TR", "ZC", "TM", "TC", "TV", "UG", "UA", "AE", "GB", "US", "UY", "UZ", "VU", "VE", "VN", "WF", "EH", "YE", "ZM", "ZW"};

  try
  {
    uint64_t ticks = epee::misc_utils::get_tick_count();
    bc_services::offer_details_ex od = AUTO_VAL_INIT(od);
    od.amount_primary = rand()%1000000000000;
    od.amount_target = rand() % 1000000000000;
    od.bonus = get_random_hex_text(7);
    od.target = get_random_hex_text(7); 
    od.primary = currencies[rand() % (sizeof(currencies) / sizeof(currencies[0]))];
    od.location_country = countries[rand() % (sizeof(countries) / sizeof(countries[0]))];
    od.location_city = get_random_hex_text(7);
    od.contacts = "[{\"EMAIL\":\""; od.contacts += get_random_hex_text(5) + "@" + get_random_hex_text(5) + ".com\"}]";           // [] (Skype, mail, ICQ, etc., website)
    od.comment = get_random_hex_text(7);
    od.payment_types = "[]";      // []money accept type(bank transaction, internet money, cash, etc)
    od.deal_option = "";        // []full amount, by parts
    od.category = "NAA,NAB";           // []
    od.expiration_time = 10;        // n-days
    od.fee = TX_DEFAULT_FEE;

    w1.push_offer(od, tx);
    uint64_t ticks_for_tx = epee::misc_utils::get_tick_count() - ticks;
    LOG_PRINT_GREEN("Send push_offer_tx took " << ticks_for_tx << "ms", LOG_LEVEL_0);
    return true;
  }
  catch (const std::exception& e)
  {
    LOG_PRINT_RED("Failed to send tx(offer): " << e.what(), LOG_LEVEL_0);
    return false;
  }
}



#define ESTIMATE_INPUTS_COUNT_LIMIT_FOR_TX_BLOWUP (CURRENCY_TX_MAX_ALLOWED_OUTS - 4)

bool do_send_money_by_fractions(tools::wallet2& w1, tools::wallet2& w2, size_t mix_in_factor, uint64_t amount_to_transfer, transaction& tx, uint64_t fraction_size)
{
  //send all of "amount_to_transfer" even with last big transfer to utilize all amount taken from transfers
  CHECK_AND_ASSERT_MES(fraction_size > 0, false, "parts must be > 0");

  std::vector<currency::tx_destination_entry> dsts;
  dsts.reserve(ESTIMATE_INPUTS_COUNT_LIMIT_FOR_TX_BLOWUP);
  uint64_t amount_used = 0;

  for (size_t i = 0; i < ESTIMATE_INPUTS_COUNT_LIMIT_FOR_TX_BLOWUP; ++i)
  {
    currency::tx_destination_entry de;
    de.addr.push_back(w2.get_account().get_keys().account_address);

    if (i == ESTIMATE_INPUTS_COUNT_LIMIT_FOR_TX_BLOWUP - 1)
    {
      de.amount = amount_to_transfer - amount_used;
    }
    else if (amount_used + fraction_size < amount_to_transfer)
    {
      de.amount = fraction_size;
    }
    else
    {
      de.amount = amount_to_transfer - amount_used;
    }
    if (!de.amount)
      break;
    amount_used += de.amount;
    dsts.push_back(de);
  }

  try
  {
    w1.transfer(dsts, mix_in_factor, 0, w1.get_core_runtime_config().tx_default_fee, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(w1.get_core_runtime_config().tx_default_fee), tx);
    LOG_PRINT_GREEN("Split transaction sent " << get_transaction_hash(tx) <<  ", destinations: " << dsts.size() << ", blob size: " << get_object_blobsize(tx), LOG_LEVEL_0);
    return true;
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("exception while sending transfer: " << e.what());
    return false;
  }
}

uint64_t got_money_in_first_transfers(const tools::transfer_container& incoming_transfers, size_t n_transfers)
{
  uint64_t summ = 0;
  size_t count = 0;
  BOOST_FOREACH(const tools::transfer_details& td, incoming_transfers)
  {
    summ += boost::get<tx_out_bare>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]).amount;
    if(++count >= n_transfers)
      return summ;
  }
  return summ;
}

#define FIRST_N_TRANSFERS 10*10
struct flow_test_context
{
  flow_test_context(tools::wallet2& w_) :w(w_), stop_end_exit(false), pause(false)
  {}
  tools::wallet2& w;
  std::atomic<bool> stop_end_exit;
  std::atomic<bool> pause;
};


void wait_unlock_money(tools::wallet2& w, flow_test_context& control)
{
  size_t blocks_fetched = 0;
  uint64_t wait_count = 0;
  while (!control.pause && !control.stop_end_exit)
  {
    LOG_PRINT_GREEN("Wait iteration " << wait_count++ << " ...", LOG_LEVEL_0);
    misc_utils::sleep_no_w(1000);
    bool received_money = false;
    w.refresh(blocks_fetched, received_money, control.stop_end_exit);
    if (blocks_fetched)
    {
      uint64_t unlocked_balance = 0;
      uint64_t balance = w.balance(unlocked_balance);
      LOG_PRINT_GREEN("Balance: " << balance << ", unlocked balance: " << unlocked_balance, LOG_LEVEL_0);
      if (unlocked_balance == balance)
        return;
    }
  }
}

std::string get_incoming_transfers_str(tools::wallet2& w)
{
  tools::transfer_container transfers;
  w.get_transfers(transfers);

  uint64_t spent_count = 0;
  uint64_t unspent_count = 0;
  for (const auto& td : transfers)
  {
    if (td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT)
    {
      ++spent_count;
    }
    else
    {
      ++unspent_count;
    }
  }
  std::stringstream ss;
  ss << "spent: " << spent_count << ", unspent: " << unspent_count;
  return ss.str();
}

void wait_receive_and_unlock_half_balance(tools::wallet2& w, flow_test_context& control)
{
  size_t blocks_fetched = 0;
  uint64_t wait_count = 0;

  while (w.unlocked_balance() < w.balance()/2 && !control.pause && !control.stop_end_exit)
  {
    LOG_PRINT_GREEN("Wait iteration " << wait_count++ << " ...", LOG_LEVEL_0);
    misc_utils::sleep_no_w(1000);
    bool received_money = false;
    w.refresh(blocks_fetched, received_money, control.stop_end_exit);
    if (blocks_fetched)
    {
      uint64_t unlocked_balance = 0;
      uint64_t balance = w.balance(unlocked_balance);
      LOG_PRINT_GREEN("Balance: " << balance << ", unlocked balance: " << unlocked_balance, LOG_LEVEL_0);
    }
  }
  LOG_PRINT_GREEN("Storing wallet... ", LOG_LEVEL_0);
  w.store();
  LOG_PRINT_GREEN("Wallet stored OK", LOG_LEVEL_0);
}

void wait_receive_and_unlock_money(tools::wallet2& w, uint64_t amount_to_wait)
{
  size_t blocks_fetched = 0;
  uint64_t wait_count = 0;
  
  while (w.unlocked_balance() < amount_to_wait )
  {
    LOG_PRINT_GREEN("Wait iteration " << wait_count++ << " ...", LOG_LEVEL_0);
    misc_utils::sleep_no_w(1000);
    bool received_money = false;
    std::atomic<bool> stop(false);
    w.refresh(blocks_fetched, received_money, stop);
    if (blocks_fetched)
    {
      uint64_t unlocked_balance = 0;
      uint64_t balance = w.balance(unlocked_balance);
      LOG_PRINT_GREEN("Balance: " << balance << ", unlocked balance: " << unlocked_balance, LOG_LEVEL_0);
      if (unlocked_balance == balance && unlocked_balance >= amount_to_wait)
        return;
    }
  }
}




class flow_test_console_cmmands_handler
{
  
  epee::console_handlers_binder m_cmd_binder;
  flow_test_context& m_context;
public:
  flow_test_console_cmmands_handler(flow_test_context& contxt):m_context(contxt)
  {
    m_cmd_binder.set_handler("help", boost::bind(&console_handlers_binder::help, &m_cmd_binder, ph::_1), "Show this help");
    m_cmd_binder.set_handler("exit", boost::bind(&flow_test_console_cmmands_handler::exit, this, ph::_1), "Exit");
    m_cmd_binder.set_handler("pause", boost::bind(&flow_test_console_cmmands_handler::pause, this, ph::_1), "Pause");
    m_cmd_binder.set_handler("continue", boost::bind(&flow_test_console_cmmands_handler::do_continue, this, ph::_1), "Continue");
    m_cmd_binder.set_handler("refresh", boost::bind(&flow_test_console_cmmands_handler::refresh, this, ph::_1), "Refresh");
  }

  bool start_handling()
  {
    m_cmd_binder.start_handling("#");
    return true;
  }

private:
  bool exit(const std::vector<std::string>& args)
  {
    m_context.stop_end_exit = true;
    return true;
  }

  bool pause(const std::vector<std::string>& args)
  {
    m_context.pause = true;
    return true;
  }

  bool do_continue(const std::vector<std::string>& args)
  {
    m_context.pause = false;
    return true;
  }
  bool refresh(const std::vector<std::string>& args)
  {
    LOG_PRINT_GREEN("Refreshing...  ", LOG_LEVEL_0);
    m_context.w.refresh();
    LOG_PRINT_GREEN("Storing wallet... ", LOG_LEVEL_0);
    m_context.w.store();
    LOG_PRINT_GREEN("Wallet stored OK", LOG_LEVEL_0);
    return true;
  }

};

uint64_t get_tx_pool_size(std::string& daemon_addr)
{
  epee::net_utils::http::http_simple_client http_client;

  currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  req.flags = 0;
  currency::COMMAND_RPC_GET_INFO::response res = AUTO_VAL_INIT(res);

  bool r = net_utils::invoke_http_json_remote_command2(daemon_addr + "/getinfo", req, res, http_client, 10000);
  if (!r)
  {
    std::cout << "ERROR: failed to invoke request to daemon" << ENDL;
    return false;
  }
  return res.tx_pool_size;
}

bool test_serialization()
{
  extra_alias_entry ee = {};
  ee.m_text_comment = "adadasdasd";
  std::string ddd = currency::obj_to_json_str(ee);
  return true;
}


struct test_serialization_2
{
  uint64_t var1;
  uint64_t var2;
  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(var1)
    KV_SERIALIZE(var2)
  END_KV_SERIALIZE_MAP()
};

bool test_serialization2()
{
  test_serialization_2 ee = {};
  std::string json_buf = "{\"var1\": 111, \"var2\": \"222\"}";
  epee::serialization::load_t_from_json(ee, json_buf);
  std::string json_buf2 = epee::serialization::store_t_to_json(ee);
  return true;
}

bool transactions_flow_test(
  std::wstring path_source_wallet, std::string source_wallet_pass,
  std::wstring path_terget_wallet, std::string target_wallet_pass,
  std::string& daemon_addr_a,
  std::string& daemon_addr_b,
  size_t mix_in_factor, size_t action, uint64_t max_tx_in_pool)
{
  LOG_PRINT_L0("-----------------------STARTING TRANSACTIONS FLOW TEST (action " << action << ")-----------------------");
  LOG_PRINT_L0("Max tx in pool: " << max_tx_in_pool);
  tools::wallet2 w1, w2;
  flow_test_context control(w1);
  flow_test_console_cmmands_handler cc(control);
  cc.start_handling();

  try
  {
    w1.load(path_source_wallet, source_wallet_pass);
    w2.load(path_terget_wallet, target_wallet_pass);
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("failed to generate wallet: " << e.what());
    return false;
  }

  w1.init(daemon_addr_a);
  w2.init(daemon_addr_b);

  size_t blocks_fetched = 0;
  bool received_money;
  bool ok;
  std::atomic<bool> stop;
  if(!w1.refresh(blocks_fetched, received_money, ok, stop))
  {
    LOG_ERROR( "failed to refresh source wallet from " << daemon_addr_a );
    return false;
  }
  LOG_PRINT_GREEN("Using wallets: " << ENDL
    << "Source:  " << w1.get_account().get_public_address_str() << ENDL << "Path: " << epee::string_encoding::wstring_to_utf8(path_source_wallet) << ENDL
    << "Target:  " << w2.get_account().get_public_address_str() << ENDL << "Path: " << epee::string_encoding::wstring_to_utf8(path_terget_wallet), LOG_LEVEL_1);

  //lets make a lot of small outs to ourselves
  //since it is not possible to start from transaction that bigger than 500Kb, we gonna make transactions 
  //with 500 outs (about 400kb), and we have to wait appropriate count blocks, mined for test wallet
  LOG_PRINT_GREEN("Transfers: " << get_incoming_transfers_str(w1), LOG_LEVEL_0);

  uint64_t transfer_size = TX_DEFAULT_FEE;//amount_to_transfer / transactions_count;
  tools::transfer_container incoming_transfers; 
  size_t prepared_transfers = 0;

   
  if (action == TRANSACTIONS_FLOW_TESTACTION_SPLIT_INITAL)
  {
    LOG_PRINT_GREEN("Waiting to unlock money...", LOG_LEVEL_0);
    wait_unlock_money(w1, control);
    while (!control.stop_end_exit)
    {
      LOG_PRINT_GREEN("Getting transfers...", LOG_LEVEL_0);
      w1.get_transfers(incoming_transfers);
      LOG_PRINT_GREEN("Got " << incoming_transfers.size() << " transfers, splitting to fractions...", LOG_LEVEL_0);
      //lets go!
      size_t count = 0;
      prepared_transfers = 0;
      BOOST_FOREACH(tools::transfer_details& td, incoming_transfers)
      {
        if (td.is_spent())
          continue;

        VARIANT_SWITCH_BEGIN(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]);
        VARIANT_CASE(currency::tx_out_bare, ob);
          if (ob.amount <= transfer_size + TX_DEFAULT_FEE)
          {
            ++prepared_transfers;
            continue;
          }
        VARIANT_CASE(currency::tx_out_zarcanum, oz);
          // @#@
        VARIANT_SWITCH_END();

        ++count;
        currency::transaction tx_s;
        if (w1.unlocked_balance() >= boost::get<tx_out_bare>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]).amount)
        {
          bool r = do_send_money_by_fractions(w1, w1, 0, boost::get<tx_out_bare>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]).amount - TX_DEFAULT_FEE, tx_s, transfer_size);
          CHECK_AND_ASSERT_MES(r, false, "Failed to send starter tx " << get_transaction_hash(tx_s));
        }
        else
        {
          break;
        }
      }
      if (!count)
        break;

      //waiting to unlock money
      LOG_PRINT_GREEN("Waiting to unlock money...", LOG_LEVEL_0);
      wait_unlock_money(w1, control);
    }
    LOG_PRINT_GREEN("Storing wallet...", LOG_LEVEL_0);
    w1.store();
    LOG_PRINT_GREEN("Wallet splitted, " << prepared_transfers << " prepared_transfers.", LOG_LEVEL_0);
    return true;
  }

      //do actual transfer
  uint64_t transfered_money = 0;
  size_t i = 0;
  struct tx_test_entry
  {
    transaction tx;
    size_t m_received_count;
    uint64_t amount_transfered;
  };

  size_t current_action = action;
  if (current_action == TRANSACTIONS_FLOW_TESTACTION_MIXED)
    current_action = TRANSACTIONS_FLOW_TESTACTION_DEFAULT;

  crypto::key_image lst_sent_ki = AUTO_VAL_INIT(lst_sent_ki);
  std::unordered_map<crypto::hash, tx_test_entry> txs;

  enum test_state
  {
    inflate_pool = 0, 
    deflate_pool = 1
  };
  test_state state = inflate_pool;

  epee::math_helper::once_a_time_seconds<10> state_check_interval;
  while (!control.stop_end_exit)
  {

    state_check_interval.do_call([&]()
    {
      uint64_t pool_size = get_tx_pool_size(daemon_addr_a);
      switch (state)
      {
      case inflate_pool:
        if (pool_size > max_tx_in_pool)
        {
          LOG_PRINT_GREEN("POOL SIZE " << pool_size << ", switched to deflate mode...", LOG_LEVEL_0);
          state = deflate_pool;
        }
        break;
      case deflate_pool:
        if (pool_size == 0)
        {
          LOG_PRINT_GREEN("POOL SIZE " << pool_size << ", switched to inflate mode...", LOG_LEVEL_0);
          state = inflate_pool;
        }
        else if (pool_size < 20)
        {
          LOG_PRINT_GREEN("Pool is about to be empty: " << pool_size, LOG_LEVEL_0);
        }
        break;
      default:
        break;
      }
      return true;
    });
    
    if (control.pause || state == deflate_pool)
    {
      --i;
      epee::misc_utils::sleep_no_w(100);
      continue;
    }
    

    //uint64_t amount_to_tx = (amount_to_transfer - transfered_money) > transfer_size ? transfer_size : (amount_to_transfer - transfered_money);
    uint64_t sent_amount = 0;

    transaction tx;
    bool res = false;
    if (current_action == TRANSACTIONS_FLOW_TESTACTION_DEFAULT)
    {
      res = do_send_money(w1, w2, mix_in_factor, TX_DEFAULT_FEE, tx);
    }
    else if (current_action == TRANSACTIONS_FLOW_TESTACTION_ALIAS)
    {
      sent_amount = currency::get_alias_coast_from_fee(get_random_hex_text(12), TX_DEFAULT_FEE);
      currency::account_base acc;
      acc.generate();
      res = do_register_alias(w1, get_random_hex_text(12), get_random_hex_text(15), acc.get_public_address(), tx);
    }
    else if (current_action == TRANSACTIONS_FLOW_TESTACTION_OFFERS)
    {
      sent_amount = TX_DEFAULT_FEE;
      currency::account_base acc;
      acc.generate();
      res = do_register_offer(w1, tx);
    }


    if(!res)
    {
      LOG_PRINT_GREEN("Sync wallet(transfers: " << get_incoming_transfers_str(w1) << ")...", LOG_LEVEL_0);
      wait_receive_and_unlock_half_balance(w1, control);
      LOG_PRINT_GREEN("Sync wallet finished(transfers: " << get_incoming_transfers_str(w1) << ")", LOG_LEVEL_0);
      continue;
    }
    lst_sent_ki = boost::get<txin_to_key>(tx.vin[0]).k_image;

    transfered_money += sent_amount;

    LOG_PRINT_L0("Transferred[type: "<< current_action << "]" << print_money(get_outs_money_amount(tx)) << "(+fee:" << print_money(get_tx_fee(tx)) << "), i=" << i );
    tx_test_entry& ent = txs[get_transaction_hash(tx)] = boost::value_initialized<tx_test_entry>();
    ent.amount_transfered = sent_amount;
    ent.tx = tx;
    if (action == TRANSACTIONS_FLOW_TESTACTION_MIXED)
    {
      current_action++;
      if (current_action >= TRANSACTIONS_FLOW_TESTACTION_OFFERS)
        current_action = TRANSACTIONS_FLOW_TESTACTION_DEFAULT;
    }
  }

  LOG_PRINT_GREEN("Exiting, storing wallet... ", LOG_LEVEL_0);
  w1.store();
  LOG_PRINT_GREEN("Wallet stored OK", LOG_LEVEL_0);

  return true;
}

