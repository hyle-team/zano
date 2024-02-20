// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



#include "include_base_utils.h"
#include "version.h"
#include "epee/include/gzip_encoding.h"

using namespace epee;
#include <boost/program_options.hpp>
//#include <boost/interprocess/ipc/message_queue.hpp>
#include "p2p/p2p_protocol_defs.h"
#include "common/command_line.h"
#include "currency_core/currency_core.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "net/levin_client.h"
#include "storages/levin_abstract_invoke2.h"
#include "currency_core/currency_core.h"
#include "storages/portable_storage_template_helper.h"
#include "crypto/crypto.h"
#include "storages/http_abstract_invoke.h"
#include "net/http_client.h"
#include "currency_core/genesis_acc.h"
#include <cstdlib>

namespace po = boost::program_options;
using namespace currency;
using namespace nodetool;






namespace
{
  const command_line::arg_descriptor<std::string> arg_ip                  ("ip", "set ip", "127.0.0.1");
  const command_line::arg_descriptor<size_t>      arg_port                ("port", "set port");
  const command_line::arg_descriptor<size_t>      arg_rpc_port            ("rpc-port", "set rpc port", RPC_DEFAULT_PORT);
  const command_line::arg_descriptor<uint32_t>    arg_timeout             ("timeout", "set timeout", 5000);
  const command_line::arg_descriptor<std::string> arg_priv_key            ("private-key", "private key to subscribe debug command");
  const command_line::arg_descriptor<uint64_t>    arg_peer_id             ("peer-id", "peerid if known(if not - will be requested)", 0);
  const command_line::arg_descriptor<bool>        arg_generate_keys       ("generate-keys-pair", "generate private and public keys pair");
  const command_line::arg_descriptor<bool>        arg_request_stat_info   ("request-stat-info", "request statistics information");
  const command_line::arg_descriptor<uint64_t>    arg_log_journal_len     ( "log-journal-len", "Specify length of list of last error messages in log");
  const command_line::arg_descriptor<bool>        arg_request_net_state   ("request-net-state", "request network state information (peer list, connections count)");
  const command_line::arg_descriptor<bool>        arg_get_daemon_info     ("rpc-get-daemon-info", "request daemon state info vie rpc (--rpc-port option should be set ).");
  const command_line::arg_descriptor<bool>        arg_get_aliases         ("rpc-get-aliases", "request daemon aliases all list");
  const command_line::arg_descriptor<std::string> arg_increment_build_no  ( "increment-build-no", "Increment build nimber");
  const command_line::arg_descriptor<std::string> arg_upate_maintainers_info  ("update-maintainers-info", "Push maintainers info into the network, update-maintainers-info=file-with-info.json");
  const command_line::arg_descriptor<std::string> arg_update_build_no     ("update-build-no", "Updated version number in version template file");
  const command_line::arg_descriptor<std::string> arg_generate_genesis    ("generate-genesis", "Generate genesis coinbase based on config file");
  const command_line::arg_descriptor<uint64_t>    arg_genesis_split_amount  ( "genesis-split-amount", "Set split amount for generating genesis block");
  const command_line::arg_descriptor<std::string> arg_get_info_flags      ( "getinfo-flags-hex", "Set of bits for rpc-get-daemon-info", "");
  const command_line::arg_descriptor<int64_t>    arg_set_peer_log_level  ( "set-peer-log-level", "Set log level for remote peer");
  const command_line::arg_descriptor<std::string> arg_download_peer_log   ( "download-peer-log", "Download log from remote peer <starting_offset>[,<count>]");
  const command_line::arg_descriptor<bool>        arg_do_consloe_log     ( "do-console-log", "Tool generates debug console output(debug purposes)");
  const command_line::arg_descriptor<std::string> arg_generate_integrated_address  ( "generate-integrated-address", "Tool create integrated address from simple address and payment_id");
  const command_line::arg_descriptor<std::string> arg_pack_file           ("pack-file", "perform gzip-packing and calculate hash for a given file");
  const command_line::arg_descriptor<std::string> arg_unpack_file         ("unpack-file", "Perform gzip-unpacking and calculate hash for a given file");
  const command_line::arg_descriptor<std::string> arg_target_file         ("target-file", "Specify target file for pack-file and unpack-file commands");
  //const command_line::arg_descriptor<std::string> arg_send_ipc            ("send-ipc", "Send IPC request to UI");
}

typedef COMMAND_REQUEST_STAT_INFO_T<t_currency_protocol_handler<core>::stat_info> COMMAND_REQUEST_STAT_INFO;

struct response_schema
{
  std::string status;
  std::string COMMAND_REQUEST_STAT_INFO_status;
  std::string COMMAND_REQUEST_NETWORK_STATE_status;
  enableable<COMMAND_REQUEST_STAT_INFO::response> si_rsp;
  enableable<COMMAND_REQUEST_NETWORK_STATE::response> ns_rsp;

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(status)
    KV_SERIALIZE(COMMAND_REQUEST_STAT_INFO_status)
    KV_SERIALIZE(COMMAND_REQUEST_NETWORK_STATE_status)
    KV_SERIALIZE(si_rsp)
    KV_SERIALIZE(ns_rsp)
  END_KV_SERIALIZE_MAP() 
};

  std::string get_response_schema_as_json(response_schema& rs)
  {
    std::stringstream ss;
    ss << "{" << ENDL 
       << "  \"status\": \"" << rs.status << "\"," << ENDL
       << "  \"COMMAND_REQUEST_NETWORK_STATE_status\": \"" << rs.COMMAND_REQUEST_NETWORK_STATE_status << "\"," << ENDL
       << "  \"COMMAND_REQUEST_STAT_INFO_status\": \"" << rs.COMMAND_REQUEST_STAT_INFO_status <<  "\"";
    if(rs.si_rsp.enabled)
    {
      ss << "," << ENDL << "  \"si_rsp\": " <<  epee::serialization::store_t_to_json(rs.si_rsp.v, 1);
    }
    if(rs.ns_rsp.enabled)
    {
      ss << "," << ENDL << "  \"ns_rsp\": {" << ENDL 
        << "    \"local_time\": " <<  rs.ns_rsp.v.local_time << "," << ENDL 
        << "    \"my_id\": \"" <<  rs.ns_rsp.v.my_id << "\"," << ENDL
        << "    \"up_time\": \"" << rs.ns_rsp.v.up_time << "\"," << ENDL
        << "    \"connections_list\": [" << ENDL;

      size_t i = 0;
      for(const connection_entry& ce : rs.ns_rsp.v.connections_list)
      {
        ss << "      {"
          "\"peer_id\": \""     << ce.id << "\", "
          "\"ip\": \""          << string_tools::get_ip_string_from_int32(ce.adr.ip) << "\", "
          "\"port\": "          << ce.adr.port << ", "
          "\"time_started\": "  << ce.time_started << ", "
          "\"last_recv\": "     << ce.last_recv << ", "
          "\"last_send\": "     << ce.last_send << ", "
          "\"version\": \""       << ce.version << "\", "
          "\"is_income\": "     << ce.is_income << "}";
        if(rs.ns_rsp.v.connections_list.size()-1 != i)
          ss << ",";
        ss << ENDL; 
        i++;
      }
      ss << "    ]," << ENDL;
      ss << "    \"local_peerlist_white\": [" << ENDL;      
      i = 0;
      for(const peerlist_entry& pe : rs.ns_rsp.v.local_peerlist_white)
      {
        ss <<  "      {\"peer_id\": \"" << pe.id << "\", \"ip\": \"" << string_tools::get_ip_string_from_int32(pe.adr.ip) << "\", \"port\": " << pe.adr.port << ", \"last_seen\": "<< rs.ns_rsp.v.local_time - pe.last_seen << "}";
        if(rs.ns_rsp.v.local_peerlist_white.size()-1 != i)
          ss << ",";
        ss << ENDL; 
        i++;
      }
      ss << "    ]," << ENDL;

      ss << "    \"local_peerlist_gray\": [" << ENDL;      
      i = 0;
      for(const peerlist_entry& pe : rs.ns_rsp.v.local_peerlist_gray)
      {
        ss <<  "      {\"peer_id\": \"" << pe.id << "\", \"ip\": \"" << string_tools::get_ip_string_from_int32(pe.adr.ip) << "\", \"port\": " << pe.adr.port << ", \"last_seen\": "<< rs.ns_rsp.v.local_time - pe.last_seen << "}";
        if(rs.ns_rsp.v.local_peerlist_gray.size()-1 != i)
          ss << ",";
        ss << ENDL; 
        i++;
      }
      ss << "    ]" << ENDL << "  }" << ENDL;
    }
    ss << "}" << ENDL;
    return ss.str();
  }
//---------------------------------------------------------------------------------------------------------------
bool print_COMMAND_REQUEST_STAT_INFO(const COMMAND_REQUEST_STAT_INFO::response& si)
{
  std::cout << " ------ COMMAND_REQUEST_STAT_INFO ------ " << ENDL;
  std::cout << "Version:             " << si.version << ENDL;
  std::cout << "Connections:          " << si.connections_count << ENDL;
  std::cout << "INC Connections:     " << si.incoming_connections_count << ENDL;


  std::cout << "Tx pool size:        " << si.payload_info.tx_pool_size << ENDL;
  std::cout << "BC height:           " << si.payload_info.blockchain_height << ENDL;
  std::cout << "Mining speed:          " << si.payload_info.mining_speed << ENDL;
  std::cout << "Alternative blocks:  " << si.payload_info.alternative_blocks << ENDL;
  std::cout << "Top block id:        " << si.payload_info.top_block_id_str << ENDL;
  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool print_COMMAND_REQUEST_NETWORK_STATE(const COMMAND_REQUEST_NETWORK_STATE::response& ns)
{
  std::cout << " ------ COMMAND_REQUEST_NETWORK_STATE ------ " << ENDL;
  std::cout << "Peer id: " << ns.my_id << ENDL;
  std::cout << "Active connections:"  << ENDL;
  BOOST_FOREACH(const connection_entry& ce, ns.connections_list)
  {
    std::cout <<  ce.id << "\t" << string_tools::get_ip_string_from_int32(ce.adr.ip) << ":" << ce.adr.port << (ce.is_income ? "(INC)":"(OUT)") << ENDL; 
  }
  
  std::cout << "Peer list white:" << ns.my_id << ENDL;
  BOOST_FOREACH(const peerlist_entry& pe, ns.local_peerlist_white)
  {
    std::cout <<  pe.id << "\t" << string_tools::get_ip_string_from_int32(pe.adr.ip) << ":" << pe.adr.port <<  "\t" << misc_utils::get_time_interval_string(ns.local_time - pe.last_seen) << ENDL; 
  }

  std::cout << "Peer list gray:" << ns.my_id << ENDL;
  BOOST_FOREACH(const peerlist_entry& pe, ns.local_peerlist_gray)
  {
    std::cout <<  pe.id << "\t" << string_tools::get_ip_string_from_int32(pe.adr.ip) << ":" << pe.adr.port <<  "\t" << misc_utils::get_time_interval_string(ns.local_time - pe.last_seen) << ENDL; 
  }


  return true;
}



std::string split_to_c_literal_lines(const std::string& str, size_t line_len)
{
  std::stringstream res;
  size_t current_offset = 0;
  while (current_offset < str.size())
  {
    if (current_offset + line_len > str.size())
      line_len = str.size() - current_offset;

    res << "\"" << str.substr(current_offset, line_len) << "\"" << ENDL ;
    current_offset += line_len;
  }
  return res.str();
}

template<class t_pod>
std::string pod_to_uint8_array(const t_pod& v)
{
  using namespace std;
  uint8_t* parray = (uint8_t*)&v;
  size_t count = sizeof(t_pod);
  std::stringstream ss;
  for (size_t i = 0; i != count; i++)
  {
    ss << (i == 0 ? "0x" : ",0x") << hex << setw(2) << setfill('0') << +parray[i];
  }
  return ss.str();
}


template<class CharT>
std::basic_string<CharT> buff_to_uint64_array(const std::basic_string<CharT>& s)
{
  using namespace std;


  uint64_t* parray = (uint64_t*)s.data();
  size_t count = s.size() / sizeof(uint64_t);
  size_t rest_bytes = s.size() - count * sizeof(uint64_t);


  basic_stringstream<CharT> hexStream;
  hexStream << "--------- genesis.h---------" << ENDL
    << "#pragma pack(push, 1)" << ENDL
    << "struct genesis_tx_raw_data" << ENDL
    << "{" << ENDL
    << "  uint64_t const v[" << count << "];" << ENDL
    << "  uint8_t const r[" << rest_bytes << "];" << ENDL
    << "};" << ENDL
    << "#pragma pack(pop)" << ENDL
    << "extern const genesis_tx_raw_data ggenesis_tx_raw;" << ENDL
    << ENDL
    << "--------- genesis.cpp---------" << ENDL
    << "const genesis_tx_raw_data ggenesis_tx_raw = {{" << ENDL;
    ;

  //hexStream << hex << ; //<< noshowbase;

  for (size_t i = 0; i != count; i++)
  {
    hexStream << (i == 0 ? "0x":",0x")<< hex << setw(16) << setfill('0') << parray[i];
  }
  hexStream << "}," << ENDL << "{";
  uint8_t* ptail_array = (uint8_t*)&parray[count];
  for (size_t i = 0; i != rest_bytes; i++)
  {
    hexStream << (i == 0 ? "0x":",0x") << hex << setw(2) << setfill('0') << +ptail_array[i];
  }
  hexStream << "}};" << ENDL;
  return hexStream.str();
}

struct payment_method_summary
{
  double amount_total;
  uint64_t amount_paid_this;
  double amount_usd_eq;
};

void process_payment_entrie(payment_method_summary& pms, const std::string& amount_in_coin, uint64_t amount_in_this, const std::string& to_usd_rate)
{
  double d_amount_in_coin = std::stod(amount_in_coin);
  // double d_to_usd_rate = std::stod(to_usd_rate);
  //CHECK_AND_ASSERT_THROW_MES(d_amount_in_coin, "unable to parse amount_in_coin: " << amount_in_coin);
  CHECK_AND_ASSERT_THROW_MES(amount_in_this, "unable to parse amount_this");
  //CHECK_AND_ASSERT_THROW_MES(d_to_usd_rate, "unable to parse to_usd_rate: " << to_usd_rate);
  pms.amount_total += d_amount_in_coin;
  pms.amount_paid_this += amount_in_this;
  pms.amount_usd_eq += d_amount_in_coin * d_amount_in_coin;
}

uint64_t get_total_from_payments_stat(const std::map<std::string, payment_method_summary>& payments_stat)
{
  uint64_t total_this = 0;
  for (const auto& se : payments_stat)
  {
    total_this += se.second.amount_paid_this;
  }
  return total_this;
}

bool generate_genesis(const std::string& path_config, uint64_t premine_split_amount)
{
  TIME_MEASURE_START_MS(load_json_time);
  currency::genesis_config_json_struct gcp = AUTO_VAL_INIT(gcp);
  if (!epee::serialization::load_t_from_json_file(gcp, path_config))
  {
    LOG_ERROR("Failed to open JSON file from " << path_config);
    return false;
  }
  TIME_MEASURE_FINISH_MS(load_json_time);

  TIME_MEASURE_START_MS(loading_to_map);
  std::map<std::string, uint64_t> amounts_map;
  for (auto& p : gcp.payments)
  {
    p.amount_this_coin_int = std::round(p.amount_this_coin_fl * COIN);
    amounts_map[p.address_this] = p.amount_this_coin_int;
  }
  TIME_MEASURE_FINISH_MS(loading_to_map);
  TIME_MEASURE_START_MS(loading_to_map2);
  std::map<std::string, uint64_t> amounts_map2;
  auto hint_it = amounts_map2.insert(*amounts_map.begin()).first;
  for (auto it = ++amounts_map.begin(); it != amounts_map.end(); it++)
  {
    hint_it = amounts_map2.insert(hint_it, *it);
  }
  TIME_MEASURE_FINISH_MS(loading_to_map2);

  std::cout << "path: " << path_config << ENDL <<  "items " << amounts_map.size()
    << ", load_json_time:" << load_json_time 
    << ", loading_to_map: " << loading_to_map 
    << ", loading_to_map2: " << loading_to_map2 << ENDL;


  currency::block bl = boost::value_initialized<block>();

  std::vector<tx_destination_entry> destinations;
  tx_destination_entry de = AUTO_VAL_INIT(de);
  de.addr.resize(1);


  std::map<std::string, payment_method_summary> payments_stat;
  
  //make sure it initialized with zeros 
  payments_stat["qtum"] = payments_stat["bch"]
    = payments_stat["rep"] = payments_stat["dash"]
    = payments_stat["ltc"] = payments_stat["eos"]
    = payments_stat["eth"] = payments_stat["btc"]
    = payments_stat["prm"]
    = AUTO_VAL_INIT_T(payment_method_summary);

  uint64_t summary_premine_coins = 0;
  for (auto& p: gcp.payments)
  {

    bool r = get_account_address_from_str(de.addr.back(), p.address_this);
    CHECK_AND_ASSERT_MES(r, false, "wrong address string: " << p.address_this);

    de.amount = p.amount_this_coin_int; //std::cout << de.amount << ENDL;
    summary_premine_coins += de.amount;
    destinations.push_back(de);


#define PROCESS_XXX_COIN_ENTRIE(ticker)  else if (!p.paid_ ## ticker.empty()) \
    { \
      process_payment_entrie(payments_stat[#ticker], p.paid_ ## ticker, p.amount_this_coin_int, p. ticker ## _usd_price); \
    }

    if (false) {}
    PROCESS_XXX_COIN_ENTRIE(qtum)
    PROCESS_XXX_COIN_ENTRIE(bch)
    PROCESS_XXX_COIN_ENTRIE(rep)
    PROCESS_XXX_COIN_ENTRIE(dash)
    PROCESS_XXX_COIN_ENTRIE(ltc)
    PROCESS_XXX_COIN_ENTRIE(eos)
    PROCESS_XXX_COIN_ENTRIE(eth)
    PROCESS_XXX_COIN_ENTRIE(btc)    
    PROCESS_XXX_COIN_ENTRIE(prm)
    PROCESS_XXX_COIN_ENTRIE(xmr)

    // self check
      if (summary_premine_coins != get_total_from_payments_stat(payments_stat))
      {
        LOG_ERROR("Internal error: missmatch of balances");
      }
  }

  uint64_t total_this = 0;
  double total_usd_eq = 0;
#define COLUMN_INTERVAL_LAYOUT 30
  std::stringstream ss;
  ss.precision(10);
  ss << std::setw(COLUMN_INTERVAL_LAYOUT) << std::left << "NAME" << std::setw(COLUMN_INTERVAL_LAYOUT) << std::left << "AMOUNT" << std::setw(COLUMN_INTERVAL_LAYOUT) << std::left << "AMOUNT THIS" << std::setw(COLUMN_INTERVAL_LAYOUT) << std::left << "USD EQ" << ENDL;
  for (auto& se : payments_stat)
  {
    ss << std::setw(COLUMN_INTERVAL_LAYOUT) << std::left  << se.first << std::setw(COLUMN_INTERVAL_LAYOUT) << std::fixed << std::left << se.second.amount_total << std::setw(COLUMN_INTERVAL_LAYOUT) << std::fixed << std::left << print_money(se.second.amount_paid_this)  << std::setw(COLUMN_INTERVAL_LAYOUT) << std::fixed << std::left << se.second.amount_usd_eq << ENDL;
    total_this += se.second.amount_paid_this;
    total_usd_eq += se.second.amount_usd_eq;
  }
  ss << ENDL << std::setw(COLUMN_INTERVAL_LAYOUT) << std::left << "TOTAL" << std::setw(COLUMN_INTERVAL_LAYOUT) << std::fixed << std::left << " " << std::setw(COLUMN_INTERVAL_LAYOUT) << std::fixed << std::left << print_money(total_this) << std::setw(COLUMN_INTERVAL_LAYOUT) << std::fixed << std::left << total_usd_eq << ENDL;
  
  ss << ENDL << std::setw(COLUMN_INTERVAL_LAYOUT) << std::left << "PREMINE_AMOUNT" << std::setw(COLUMN_INTERVAL_LAYOUT) << std::fixed << std::left << " " << std::setw(COLUMN_INTERVAL_LAYOUT) << std::fixed << std::left << total_this << "(" << print_money(total_this) <<"THIS)" << ENDL;

  epee::log_space::set_console_color(epee::log_space::console_colors::console_color_magenta, true);
  std::cout << "GENESIS STAT: " << ENDL << ss.str();
  epee::log_space::set_console_color(epee::log_space::console_colors::console_color_default, false);

  bool r = file_io_utils::save_string_to_file(path_config + ".premine_stat.txt", ss.str());


  ss.str("");
  ss.clear();
  const account_public_address dummy_address = AUTO_VAL_INIT(dummy_address);

  std::cout << ENDL << "PROOF PHRASE: " << gcp.proof_string << ENDL;
  uint64_t block_reward_without_fee = 0;
  uint64_t block_reward = 0;
  construct_miner_tx(0, 0, 0, 0, 0, dummy_address, dummy_address, bl.miner_tx, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4, gcp.proof_string, CURRENCY_MINER_TX_MAX_OUTS, false, pos_entry(), nullptr, nullptr, destinations);
  currency::blobdata txb = tx_to_blob(bl.miner_tx);

  //self validate block

  if (currency::get_outs_money_amount(bl.miner_tx) != total_this || summary_premine_coins != total_this)
  {
    LOG_ERROR("Internal error: total_this = " << total_this << " didn't match with miner_tx total = " << currency::get_outs_money_amount(bl.miner_tx));
  }



  std::string hex_tx_represent = string_tools::buff_to_hex_nodelimer(txb);
  std::string uint64_t_array_represent = /*string_tools::*/buff_to_uint64_array(txb);
  
  r = file_io_utils::save_string_to_file(path_config + ".genesis.txt", hex_tx_represent);
  CHECK_AND_ASSERT_MES_NO_RET(r, "failed to create " << path_config << ".genesis.txt");
  r = file_io_utils::save_string_to_file(path_config + ".genesis.uint64.array.txt", uint64_t_array_represent);
  CHECK_AND_ASSERT_MES_NO_RET(r, "failed to create " << path_config << ".genesis.uint64.array.txt");

  //generate address dictionary
  std::map<uint64_t, uint64_t> ordered_keys;
  uint64_t i = 0;
  for (auto& p : gcp.payments)
  {
    uint64_t key = currency::get_string_uint64_hash(p.address_this);
    auto it = ordered_keys.find(key);
    if (it != ordered_keys.end())
    {
      LOG_ERROR("Collision found on address " << p.address_this << " key: " << key << " offset_associated: " << it->second);
      return false;
    }
    ordered_keys[key] = i;
    i++;
  }
  //dump it to file
  
  ss << "-------------genesis_acc.cpp-------------" << ENDL
    << "const std::string ggenesis_tx_pub_key_str = \"" << string_tools::pod_to_hex(get_tx_pub_key_from_extra(bl.miner_tx)) << "\";" << ENDL;
  ss << "const crypto::public_key ggenesis_tx_pub_key = epee::string_tools::parse_tpod_from_hex_string<crypto::public_key>(ggenesis_tx_pub_key_str);" << ENDL
    << "extern const genesis_tx_dictionary_entry ggenesis_dict[" << ordered_keys.size() <<  "];" << ENDL
    << "const genesis_tx_dictionary_entry ggenesis_dict[" << ordered_keys.size() << "] = {";

  for (auto it = ordered_keys.begin(); it != ordered_keys.end(); it++)
  {
    ss << (it == ordered_keys.begin() ? "":",") << ENDL << "{" << it->first << "ULL," << it->second << "}";
  }
  ss << ENDL << "};" << ENDL;

  r = file_io_utils::save_string_to_file(path_config + ".genesis.dictionary.txt", ss.str());
  CHECK_AND_ASSERT_MES_NO_RET(r, "failed to create " << path_config << ".genesis.dictionary.txt");

  return true;
}
#undef COLUMN_INTERVAL_LAYOUT
//---------------------------------------------------------------------------------------------------------------
bool handle_get_aliases(po::variables_map& vm)
{
  if(!command_line::has_arg(vm, arg_rpc_port))
  {
    std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "RPC port not set \"" << ENDL << "}" << ENDL;
    return false;
  }

  epee::net_utils::http::http_simple_client http_client;

  currency::COMMAND_RPC_GET_ALL_ALIASES::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_ALL_ALIASES::response res = AUTO_VAL_INIT(res);
  std::string daemon_addr = command_line::get_arg(vm, arg_ip) + ":" + std::to_string(command_line::get_arg(vm, arg_rpc_port));
  bool r = epee::net_utils::invoke_http_json_rpc(daemon_addr + "/json_rpc", "get_all_alias_details", req, res, http_client, command_line::get_arg(vm, arg_timeout));
  if(!r)
  {
    std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "Failed to invoke request \"" << ENDL << "}" << ENDL;
    return false;
  }
  std::cout << epee::serialization::store_t_to_json(res);

  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool handle_get_daemon_info(po::variables_map& vm)
{
  if(!command_line::has_arg(vm, arg_rpc_port))
  {
    std::cout << "ERROR: rpc port not set" << ENDL;
    return false;
  }

  currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_INFO::response res = AUTO_VAL_INIT(res);

  if (command_line::has_arg(vm, arg_get_info_flags))
  {
    std::string flags_str = command_line::get_arg(vm, arg_get_info_flags);
    if (!epee::string_tools::get_xnum_from_hex_string(flags_str, req.flags))
    {
      LOG_ERROR("Wrong flags specified, reset to all flags");
      req.flags = COMMAND_RPC_GET_INFO_FLAG_ALL_FLAGS;
    }
  }
  else
  {
    //very slow case
    req.flags = COMMAND_RPC_GET_INFO_FLAG_ALL_FLAGS; 
  }

  epee::net_utils::http::http_simple_client http_client;

  std::string daemon_addr = command_line::get_arg(vm, arg_ip) + ":" + std::to_string(command_line::get_arg(vm, arg_rpc_port));
  std::string url = daemon_addr + "/getinfo";
  bool r = net_utils::invoke_http_json_remote_command2(url, req, res, http_client, command_line::get_arg(vm, arg_timeout));
  if(!r)
  {
    std::cout << "ERROR: failed to invoke request to " << url << ENDL;
    return false;
  }

#define PRINT_FIELD_NAME(owner_object, prefix, field_name) << prefix #field_name ": " << owner_object.field_name << ENDL

  std::cout << "OK" << ENDL
    << "height: " << res.height << ENDL
    << "pos_difficulty: " << res.pos_difficulty << ENDL
    << "pow_difficulty: " << res.pow_difficulty << ENDL
    << "tx_count: " << res.tx_count << ENDL
    << "tx_pool_size: " << res.tx_pool_size << ENDL
    << "alt_blocks_count: " << res.alt_blocks_count << ENDL
    << "outgoing_connections_count: " << res.outgoing_connections_count << ENDL
    << "incoming_connections_count: " << res.incoming_connections_count << ENDL
    << "white_peerlist_size: " << res.white_peerlist_size << ENDL
    << "grey_peerlist_size: " << res.grey_peerlist_size << ENDL
    << "current_network_hashrate_50: " << res.current_network_hashrate_50 << ENDL
    << "current_network_hashrate_350: " << res.current_network_hashrate_350 << ENDL
    << "seconds_between_10_blocks: " << res.seconds_for_10_blocks << ENDL
    << "seconds_between_30_blocks: " << res.seconds_for_30_blocks << ENDL
    << "alias_count: " << res.alias_count << ENDL
    << "transactions_cnt_per_day: " << res.transactions_cnt_per_day << ENDL
    << "transactions_volume_per_day: " << res.transactions_volume_per_day << ENDL
    << "pos_sequence_factor: " << res.pos_sequence_factor << ENDL
    << "pow_sequence_factor: " << res.pow_sequence_factor << ENDL
    << "last_pos_timestamp: " << res.last_pos_timestamp << ENDL
    << "last_pow_timestamp: " << res.last_pow_timestamp << ENDL
    << "total_coins: " << res.total_coins << ENDL
    << "pos_difficulty_in_coins: " << currency::wide_difficulty_type(res.pos_difficulty) / COIN << ENDL
    << "block_reward: " << res.block_reward << ENDL
    << "last_block_total_reward: " << res.last_block_total_reward << ENDL
    << "outs_stat_amount1_0_001: " << res.outs_stat.amount_0_001 << ENDL
    << "outs_stat_amount2_0_01: " << res.outs_stat.amount_0_01 << ENDL
    << "outs_stat_amount3_0_1: " << res.outs_stat.amount_0_1 << ENDL
    << "outs_stat_amount4_1: " << res.outs_stat.amount_10 << ENDL
    << "outs_stat_amount5_10: " << res.outs_stat.amount_10 << ENDL
    << "outs_stat_amount6_100: " << res.outs_stat.amount_100 << ENDL
    << "outs_stat_amount7_1000: " << res.outs_stat.amount_1000 << ENDL
    << "outs_stat_amount8_10000: " << res.outs_stat.amount_10000 << ENDL
    << "outs_stat_amount9_100000: " << res.outs_stat.amount_100000 << ENDL
    << "outs_stat_amount10_1000000: " << res.outs_stat.amount_1000000 << ENDL
    << "current_max_allowed_block_size: " << res.current_max_allowed_block_size << ENDL
    << "last_block_size: " << res.last_block_size << ENDL
    << "tx_count_in_last_block: " << res.tx_count_in_last_block << ENDL
    << "pos_diff_total_coins_rate: " << res.pos_diff_total_coins_rate << ENDL
    << "pos_block_ts_shift_vs_actual: " << res.pos_block_ts_shift_vs_actual << ENDL
    << "block_processing_time_0: " << res.performance_data.block_processing_time_0 << ENDL
    << "block_processing_time_1: " << res.performance_data.block_processing_time_1 << ENDL
    << "etc_stuff_6: " << res.performance_data.etc_stuff_6 << ENDL
    << "insert_time_4: " << res.performance_data.insert_time_4 << ENDL
    << "longhash_calculating_time_3: " << res.performance_data.longhash_calculating_time_3 << ENDL
    << "raise_block_core_event: " << res.performance_data.raise_block_core_event << ENDL
    << "target_calculating_time_2: " << res.performance_data.target_calculating_time_2 << ENDL
    << "target_calculating_enum_blocks: " << res.performance_data.target_calculating_enum_blocks << ENDL
    << "target_calculating_calc: " << res.performance_data.target_calculating_calc << ENDL
    << "all_txs_insert_time_5: " << res.performance_data.all_txs_insert_time_5 << ENDL
    << "tx_add_one_tx_time: " << res.performance_data.tx_add_one_tx_time << ENDL
    << "tx_check_inputs_time: " << res.performance_data.tx_check_inputs_time << ENDL
    << "tx_process_attachment: " << res.performance_data.tx_process_attachment << ENDL
    << "tx_process_extra: " << res.performance_data.tx_process_extra << ENDL
    << "tx_process_inputs: " << res.performance_data.tx_process_inputs << ENDL
    << "tx_push_global_index: " << res.performance_data.tx_push_global_index << ENDL
    << "tx_check_exist: " << res.performance_data.tx_check_exist << ENDL
    << "tx_store_db: " << res.performance_data.tx_store_db << ENDL
    << "db_tx_count: " << res.performance_data.tx_count << ENDL
    << "db_writer_tx_count: " << res.performance_data.writer_tx_count << ENDL
    << "db_map_size: " << res.performance_data.map_size << ENDL
    << "market_size: " << res.offers_count << ENDL
    PRINT_FIELD_NAME(res.performance_data, "", tx_print_log)
    PRINT_FIELD_NAME(res.performance_data, "", tx_prapare_append)
    PRINT_FIELD_NAME(res.performance_data, "", tx_append_time)
    PRINT_FIELD_NAME(res.performance_data, "", tx_append_rl_wait)
    PRINT_FIELD_NAME(res.performance_data, "", tx_append_is_expired)
    PRINT_FIELD_NAME(res.performance_data, "", tx_mixin_count)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_prefix_hash)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_attachment_check)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop_kimage_check)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop_ch_in_val_sig)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop_scan_outputkeys_get_item_size)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop_scan_outputkeys_relative_to_absolute)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop_scan_outputkeys_loop)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop_scan_outputkeys_loop_get_subitem)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop_scan_outputkeys_loop_find_tx)
    PRINT_FIELD_NAME(res.performance_data, "", tx_check_inputs_loop_scan_outputkeys_loop_handle_output)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", tx_processing_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", check_inputs_types_supported_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", expiration_validate_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", validate_amount_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", validate_alias_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", check_keyimages_ws_ms_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", check_inputs_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", begin_tx_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", update_db_time)
    PRINT_FIELD_NAME(res.tx_pool_performance_data, "pool_", db_commit_time);


  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool handle_request_stat(po::variables_map& vm, peerid_type peer_id)
{

  if(!command_line::has_arg(vm, arg_priv_key))
  {
    std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "secret key not set \"" << ENDL << "}" << ENDL;
    return false;
  }
  crypto::secret_key prvk = AUTO_VAL_INIT(prvk);
  if(!string_tools::hex_to_pod(command_line::get_arg(vm, arg_priv_key) , prvk))
  {
    std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "wrong secret key set \"" << ENDL << "}" << ENDL;
    return false;
  }


  response_schema rs = AUTO_VAL_INIT(rs);

  net_utils::levin_client2 transport;
  if(!transport.connect(command_line::get_arg(vm, arg_ip), static_cast<int>(command_line::get_arg(vm, arg_port)), static_cast<int>(command_line::get_arg(vm, arg_timeout))))
  {
    std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "Failed to connect to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port) << "\"" << ENDL << "}" << ENDL;
    return false;
  }else
    rs.status = "OK";

  if(!peer_id)
  {
    COMMAND_REQUEST_PEER_ID::request req = AUTO_VAL_INIT(req);
    COMMAND_REQUEST_PEER_ID::response rsp = AUTO_VAL_INIT(rsp);
    if(!net_utils::invoke_remote_command2(COMMAND_REQUEST_PEER_ID::ID, req, rsp, transport))
    {
      std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "Failed to connect to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port) << "\"" << ENDL << "}" << ENDL;
      return false;
    }else
    {
      peer_id = rsp.my_id;
    }
  }


  nodetool::proof_of_trust pot = AUTO_VAL_INIT(pot);
  pot.peer_id = peer_id;
  pot.time = time(NULL);
  crypto::public_key pubk = AUTO_VAL_INIT(pubk);
  string_tools::hex_to_pod(P2P_MAINTAINERS_PUB_KEY, pubk);
  crypto::hash h = tools::get_proof_of_trust_hash(pot);
  crypto::generate_signature(h, pubk, prvk, pot.sign);

  if(command_line::get_arg(vm, arg_request_stat_info))
  {
    uint64_t log_journal_len = 10;
    if (command_line::has_arg(vm, arg_log_journal_len))
      log_journal_len = command_line::get_arg(vm, arg_log_journal_len);

    COMMAND_REQUEST_STAT_INFO::request req = AUTO_VAL_INIT(req);
    req.pr.chain_len = 3;
    req.pr.logs_journal_len = log_journal_len;
    req.tr = pot;
    if(!net_utils::invoke_remote_command2(COMMAND_REQUEST_STAT_INFO::ID, req, rs.si_rsp.v, transport))
    {
      std::stringstream ss;
      ss << "ERROR: " << "Failed to invoke remote command COMMAND_REQUEST_STAT_INFO to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port);
      rs.COMMAND_REQUEST_STAT_INFO_status = ss.str();
    }else
    {
      rs.si_rsp.enabled = true;
      rs.COMMAND_REQUEST_STAT_INFO_status = "OK";
    }
  }


  if(command_line::get_arg(vm, arg_request_net_state))
  {
    ++pot.time;
    h = tools::get_proof_of_trust_hash(pot);
    crypto::generate_signature(h, pubk, prvk, pot.sign);
    COMMAND_REQUEST_NETWORK_STATE::request req = AUTO_VAL_INIT(req);    
    req.tr = pot;
    if(!net_utils::invoke_remote_command2(COMMAND_REQUEST_NETWORK_STATE::ID, req, rs.ns_rsp.v, transport))
    {
      std::stringstream ss;
      ss << "ERROR: " << "Failed to invoke remote command COMMAND_REQUEST_NETWORK_STATE to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port);
      rs.COMMAND_REQUEST_NETWORK_STATE_status = ss.str();
    }else
    {
      rs.ns_rsp.enabled = true;
      rs.COMMAND_REQUEST_NETWORK_STATE_status = "OK";
    }
  }
  std::cout << get_response_schema_as_json(rs);
  return true;
  }
//---------------------------------------------------------------------------------------------------------------
std::string get_password(const std::string& promt)
{
  std::string res;
#ifndef WIN32
  const char* ppass = getpass(promt.c_str());
  if (ppass)
    res = ppass;
#else
  std::cout << promt;
  std::getline(std::cin, res);
#endif
  return res;
}
//---------------------------------------------------------------------------------------------------------------
bool get_private_key(crypto::secret_key& pk, po::variables_map& vm)
{
  std::string key_str;

  if(command_line::has_arg(vm, arg_priv_key))
  {
    key_str = command_line::get_arg(vm, arg_priv_key);
  }
  else
  {
    key_str = get_password("Enter maintain private key:");
  }
  
  if(!string_tools::hex_to_pod(key_str, pk))
  {
    std::cout << "ERROR: wrong secret key set" << ENDL;
    return false;
  }
  crypto::public_key pubkey = AUTO_VAL_INIT(pubkey);
  if(!crypto::secret_key_to_public_key(pk, pubkey))
  {
    std::cout << "ERROR: wrong secret key set(secret_key_to_public_key failed)" << ENDL;
    return false;
  }
  if( pubkey != tools::get_public_key_from_string(P2P_MAINTAINERS_PUB_KEY))
  {
    std::cout << "ERROR: wrong secret key set(public keys not match)" << ENDL;
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool handle_increment_build_no(po::variables_map& vm)
{
  std::string path = command_line::get_arg(vm, arg_increment_build_no);
  if (!path.size())
  {
    std::cout << "no argument for increment_build_no" << ENDL;
    return false;
  }
  std::string target_buff;
  if (!file_io_utils::load_file_to_string(path, target_buff))
  {
    std::cout << "noFailed to read file " << path << ENDL;
    return false;
  }
  std::string pattern = "#define PROJECT_VERSION_BUILD_NO ";
  std::string::size_type templ_offet = target_buff.find(pattern);
  if (templ_offet == std::string::npos)
  {
    std::cout << "Filed to find pattern in " << path << ENDL;
    return false;
  }


  std::string::size_type p = target_buff.find('\n', templ_offet + pattern.size());
  if (p == std::string::npos)
  {
    std::cout << "Filed to find pattern in " << path << ENDL;
    return false;
  }
  //for windows-like line endings 
  if (target_buff[p - 1] == '\r')
    --p;

  std::string build_no_str = target_buff.substr(templ_offet + pattern.size(), p - (templ_offet + pattern.size()));
  build_no_str = string_tools::trim(build_no_str);
  uint64_t build_num = 0;
  if (!string_tools::get_xtype_from_string(build_num, build_no_str))
  {
    std::cout << "Filed to find pattern in " << path << ENDL;
    return false;
  }

  ++build_num;
  target_buff.erase(templ_offet + pattern.size(), p - (templ_offet + pattern.size()));
  target_buff.insert(templ_offet + pattern.size(), std::to_string(build_num));
  
  if (!file_io_utils::save_string_to_file(path, target_buff))
  {
    std::cout << "Filed to save file " << path << ENDL;
    return false;
  }
  std::cout << "SUCCESSFULLY INCREMENTED: " << build_num << ENDL;

  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool handle_update_maintainers_info(po::variables_map& vm)
{
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  size_t rpc_port = RPC_DEFAULT_PORT;
  if(!command_line::has_arg(vm, arg_rpc_port))
  {
    std::cout << "ERROR: rpc port not set" << ENDL;
    return false;
  }

   crypto::secret_key prvk = AUTO_VAL_INIT(prvk);
  if(!get_private_key(prvk, vm))
  {
    std::cout << "ERROR: secret key error" << ENDL;
    return false;
  }

  std::string path = command_line::get_arg(vm, arg_upate_maintainers_info);

  epee::net_utils::http::http_simple_client http_client;

  currency::COMMAND_RPC_SET_MAINTAINERS_INFO::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_SET_MAINTAINERS_INFO::response res = AUTO_VAL_INIT(res);

  maintainers_info mi = AUTO_VAL_INIT(mi);
  bool r = epee::serialization::load_t_from_json_file(mi, path);
  CHECK_AND_ASSERT_MES(r, false, "Failed to load maintainers_info from json file: " << path);
  mi.timestamp = time(NULL);  
  std::cout << "timestamp: " << mi.timestamp << ENDL;
  epee::serialization::store_t_to_binary(mi, req.maintainers_info_buff);
  crypto::generate_signature(currency::get_blob_hash(req.maintainers_info_buff), tools::get_public_key_from_string(P2P_MAINTAINERS_PUB_KEY), prvk, req.sign);

  std::string daemon_addr = command_line::get_arg(vm, arg_ip) + ":" + std::to_string(command_line::get_arg(vm, arg_rpc_port));
  r = net_utils::invoke_http_bin_remote_command2(daemon_addr + "/set_maintainers_info.bin", req, res, http_client, command_line::get_arg(vm, arg_timeout));
  if(!r)
  {
    std::cout << "ERROR: failed to invoke request" << ENDL;
    return false;
  }
  if(res.status != API_RETURN_CODE_OK)
  {
    std::cout << "ERROR: failed to update maintainers info: " << res.status << ENDL;
    return false;
  }

  std::cout << "OK" << ENDL;
  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool generate_and_print_keys()
{
  crypto::public_key pk = AUTO_VAL_INIT(pk);
  crypto::secret_key sk = AUTO_VAL_INIT(sk);
  generate_keys(pk, sk);
  std::cout << "PUBLIC KEY: " << epee::string_tools::pod_to_hex(pk) << ENDL 
    << "PRIVATE KEY: " << epee::string_tools::pod_to_hex(sk);
  return true;
}
//---------------------------------------------------------------------------------------------------------------
template<class command_t>
bool invoke_debug_command(po::variables_map& vm, const crypto::secret_key& sk, net_utils::levin_client2& transport, peerid_type& peer_id, typename command_t::request& req, typename command_t::response& rsp)
{
  if (!transport.is_connected())
  {
    if (!transport.connect(command_line::get_arg(vm, arg_ip), static_cast<int>(command_line::get_arg(vm, arg_port)), static_cast<int>(command_line::get_arg(vm, arg_timeout))))
    {
      std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "Failed to connect to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port) << "\"" << ENDL << "}" << ENDL;
      return false;
    }
  }

  if (!peer_id)
  {
    COMMAND_REQUEST_PEER_ID::request id_req = AUTO_VAL_INIT(id_req);
    COMMAND_REQUEST_PEER_ID::response id_rsp = AUTO_VAL_INIT(id_rsp);
    if (!net_utils::invoke_remote_command2(COMMAND_REQUEST_PEER_ID::ID, id_req, id_rsp, transport))
    {
      std::cout << "{" << ENDL << "  \"status\": \"ERROR: " << "Failed to connect to " << command_line::get_arg(vm, arg_ip) << ":" << command_line::get_arg(vm, arg_port) << "\"" << ENDL << "}" << ENDL;
      return false;
    }
    else
    {
      peer_id = id_rsp.my_id;
    }
  }

  nodetool::proof_of_trust pot = AUTO_VAL_INIT(pot);
  pot.peer_id = peer_id;
  pot.time = time(NULL);
  crypto::public_key pubk = AUTO_VAL_INIT(pubk);
  string_tools::hex_to_pod(P2P_MAINTAINERS_PUB_KEY, pubk);
  crypto::hash h = tools::get_proof_of_trust_hash(pot);
  crypto::generate_signature(h, pubk, sk, pot.sign);

  req.tr = pot;

  return net_utils::invoke_remote_command2(command_t::ID, req, rsp, transport);
}
//---------------------------------------------------------------------------------------------------------------
bool handle_set_peer_log_level(po::variables_map& vm)
{
  crypto::secret_key sk = AUTO_VAL_INIT(sk);
  if (!get_private_key(sk, vm))
  {
    std::cout << "ERROR: secret key error" << ENDL;
    return false;
  }

  int64_t log_level = command_line::get_arg(vm, arg_set_peer_log_level);
  if (log_level < LOG_LEVEL_0 || log_level > LOG_LEVEL_MAX)
  {
    std::cout << "Error: invalid log level value: " << log_level << ENDL;
    return false;
  }

  net_utils::levin_client2 transport;
  peerid_type peer_id = 0;

  COMMAND_SET_LOG_LEVEL::request req = AUTO_VAL_INIT(req);
  req.new_log_level = log_level;
  
  COMMAND_SET_LOG_LEVEL::response rsp = AUTO_VAL_INIT(rsp);
  if (!invoke_debug_command<COMMAND_SET_LOG_LEVEL>(vm, sk, transport, peer_id, req, rsp))
  {
    std::cout << "ERROR: invoking COMMAND_SET_LOG_LEVEL failed" << ENDL;
    return false;
  }

  std::cout << "OK! Log level changed: " << rsp.old_log_level << " -> " << rsp.current_log_level << ENDL;

  return true;
}
//---------------------------------------------------------------------------------------------------------------
bool handle_download_peer_log(po::variables_map& vm)
{
  crypto::secret_key sk = AUTO_VAL_INIT(sk);
  if (!get_private_key(sk, vm))
  {
    std::cout << "ERROR: secret key error" << ENDL;
    return false;
  }

  int64_t start_offset_signed = 0;
  int64_t count = -1;

  std::string arg_str = command_line::get_arg(vm, arg_download_peer_log);
  size_t comma_pos = arg_str.find(',');
  if (comma_pos != std::string::npos)
  {
    // count is specified
    if (!epee::string_tools::string_to_num_fast(arg_str.substr(comma_pos + 1), count) || count < 0)
    {
      std::cout << "ERROR: invalid argument: " << arg_str << ENDL;
      return false;
    }
    arg_str.erase(comma_pos);
  }
  if (!epee::string_tools::string_to_num_fast(arg_str, start_offset_signed) || start_offset_signed < 0)
  {
    std::cout << "ERROR: couldn't parse start_offset: " << arg_str << ENDL;
    return false;
  }
  uint64_t start_offset = static_cast<uint64_t>(start_offset_signed);

  net_utils::levin_client2 transport;
  peerid_type peer_id = 0;

  COMMAND_REQUEST_LOG::request req = AUTO_VAL_INIT(req);
  COMMAND_REQUEST_LOG::response rsp = AUTO_VAL_INIT(rsp);
  if (!invoke_debug_command<COMMAND_REQUEST_LOG>(vm, sk, transport, peer_id, req, rsp) || !rsp.error.empty())
  {
    std::cout << "ERROR: invoking COMMAND_REQUEST_LOG failed: " << rsp.error << ENDL;
    return false;
  }

  std::cout << "Current log level: " << rsp.current_log_level << ENDL;
  std::cout << "Current log size:  " << rsp.current_log_size << ENDL;

  if (start_offset == 0 && count == 0)
    return true; // a caller wanted to just get the info, end of story

  if (start_offset >= rsp.current_log_size)
  {
    std::cout << "ERROR: invalid start offset: " << start_offset << ", log size: " << rsp.current_log_size << ENDL;
    return false;
  }

  std::cout << "Downloading..." << ENDL;

  std::string local_filename = tools::get_default_data_dir() + "/log_" + epee::string_tools::num_to_string_fast(peer_id) + ".log";
  std::ofstream log{ local_filename, std::ifstream::binary };
  if (!log)
  {
    std::cout << "Couldn't open " << local_filename << " for writing." << ENDL;
    return false;
  }

  const uint64_t chunk_size = 1024 * 1024 * 5;
  uint64_t end_offset = start_offset;
  while (true)
  {
    req.log_chunk_offset = end_offset;
    req.log_chunk_size = std::min(chunk_size, rsp.current_log_size - req.log_chunk_offset);

    if (count > 0)
    {
      uint64_t bytes_left = count + start_offset - end_offset;
      req.log_chunk_size = std::min(req.log_chunk_size, bytes_left);
    }

    if (req.log_chunk_size == 0)
      break;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!invoke_debug_command<COMMAND_REQUEST_LOG>(vm, sk, transport, peer_id, req, rsp) || !rsp.error.empty())
    {
      std::cout << "ERROR: invoking COMMAND_REQUEST_LOG failed: " << rsp.error << ENDL;
      return false;
    }

    if (!epee::zlib_helper::unpack(rsp.log_chunk))
    {
      std::cout << "ERROR: zip unpack failed" << ENDL;
      return false;
    }

    if (rsp.log_chunk.size() != req.log_chunk_size)
    {
      std::cout << "ERROR: unpacked size: " << rsp.log_chunk.size() << ", requested: " << req.log_chunk_size << ENDL;
      return false;
    }

    log.write(rsp.log_chunk.c_str(), rsp.log_chunk.size());
    
    end_offset += req.log_chunk_size;

    std::cout << end_offset - start_offset << " bytes downloaded" << ENDL;
  }

  std::cout << "Remote log from offset " << start_offset << " to offset " << end_offset << " (" << end_offset - start_offset << " bytes) " <<
    "was successfully downloaded to " << local_filename << ENDL;

  return true;
}


bool handle_generate_integrated_address(po::variables_map& vm)
{
  std::string add_and_payment_id = command_line::get_arg(vm, arg_generate_integrated_address);

  std::string::size_type off = add_and_payment_id.find(':');
  if (off == std::string::npos)
  {
    std::cout << "ERROR: wrong syntax, delimiter symbol (':') not found " << ENDL;
    return false;
  }

  std::string address = add_and_payment_id.substr(0, off);
  std::string payment_id = add_and_payment_id.substr(off+1, add_and_payment_id.length());
  std::string payment_id_bin;
  if (!epee::string_tools::parse_hexstr_to_binbuff(payment_id, payment_id_bin))
  {
    payment_id_bin = payment_id;
  }

  if (address.empty() || payment_id_bin.empty())
  {
    std::cout << "ERROR: wrong syntax, address or paymentd_id not set" << ENDL;
    return false;
  }

  account_public_address acc_addr = AUTO_VAL_INIT(acc_addr);
  bool r = currency::get_account_address_from_str(acc_addr, address);
  if (!r)
  {
    std::cout << "ERROR: wrong syntax, address is wrong: " << address << ENDL;
    return false;
  }

  std::string integrated_addr = currency::get_account_address_and_payment_id_as_str(acc_addr, payment_id_bin);


  std::cout << "Integrated address: " << integrated_addr << ENDL;

  return true;
}
//---------------------------------------------------------------------------------------------------------------
template<class archive_processor_t>
bool process_archive(archive_processor_t& arch_processor, bool is_packing, std::ifstream& source, std::ofstream& target)
{
  source.seekg(0, std::ios::end);
  uint64_t sz = source.tellg();
  uint64_t remaining = sz;
  uint64_t written_bytes = 0;

  crypto::stream_cn_hash hash_stream;
  
  source.seekg(0, std::ios::beg);

#define PACK_READ_BLOCKS_SIZE  1048576 // 1MB blocks

  std::string buff;

  auto writer_cb = [&](const std::string& piece_of_transfer)
  {
    target.write(piece_of_transfer.data(), piece_of_transfer.size());
    written_bytes += piece_of_transfer.size();
    if (!is_packing)
      hash_stream.update(piece_of_transfer.data(), piece_of_transfer.size());
    return true;
  };

  while (remaining)
  {
    uint64_t read_sz = remaining >= PACK_READ_BLOCKS_SIZE ? PACK_READ_BLOCKS_SIZE : remaining;
    buff.resize(read_sz);
    source.read(const_cast<char*>(buff.data()), buff.size());
    if (!source)
    {
      std::cout << "Error on read from source" << ENDL;
      return true;
    }

    if (is_packing)
      hash_stream.update(buff.data(), buff.size());

    bool r = arch_processor.update_in(buff, writer_cb);
    CHECK_AND_ASSERT_MES(r, false, "arch_processor.update_in failed");


    remaining -= read_sz;
    std::cout << "Progress: " << ((sz - remaining) * 100) / sz << "%\r";
  }

  //flush gzip decoder
  arch_processor.stop(writer_cb);

  source.close();
  target.close();

  crypto::hash data_hash = hash_stream.calculate_hash();

  std::cout << "\r\nFile " << (is_packing ? "packed" : "unpacked") << " from size " << sz << " to " << written_bytes <<
                "\r\nhash of the data is " << epee::string_tools::pod_to_hex(data_hash) << "\r\n";

  return true;
}

/*
bool handle_send_ipc(const std::string& parms)
{
  try{
    boost::interprocess::message_queue mq
    (boost::interprocess::open_only   //only open
      , GUI_IPC_MESSAGE_CHANNEL_NAME  //name
    );
    
    mq.send(parms.data(), parms.size(), 0);
    
    return true;
  }
  catch (const std::exception& ex)
  {
    boost::interprocess::message_queue::remove(GUI_IPC_MESSAGE_CHANNEL_NAME);
    LOG_ERROR("Failed to receive IPC que: " << ex.what());
  }

  catch (...)
  {
    boost::interprocess::message_queue::remove(GUI_IPC_MESSAGE_CHANNEL_NAME);
    LOG_ERROR("Failed to receive IPC que: unknown exception");
  }
  return false;
}
*/

bool handle_pack_file(po::variables_map& vm)
{
  bool do_pack = false;
  std::string path_source;
  std::string path_target;
  if (command_line::has_arg(vm, arg_pack_file))
  {
    path_source = command_line::get_arg(vm, arg_pack_file);
    do_pack = true;
  }
  else if (command_line::has_arg(vm, arg_unpack_file))
  {
    path_source = command_line::get_arg(vm, arg_unpack_file);
    do_pack = false;
  }
  else
  {
    return false;
  }

  if (!command_line::has_arg(vm, arg_target_file))
    std::cout << "Error: Parameter target_file is not set." << ENDL;
  path_target = command_line::get_arg(vm, arg_target_file);

  std::ifstream source;
  source.open(path_source, std::ios::binary | std::ios::in );
  if (!source.is_open())
  {
    std::cout << "Error: Unable to open " << path_source << ENDL;
    return false;
  }
  
  std::ofstream target;
  target.open(path_target, std::ios::binary | std::ios::out | std::ios::trunc);
  if (!target.is_open())
  {
    std::cout << "Error: Unable to open " << path_target << ENDL;
    return false;
  }

  if (do_pack)
  {
    epee::net_utils::gzip_encoder_lyambda gzip_encoder(Z_BEST_COMPRESSION);
    return process_archive(gzip_encoder, true, source, target);
  }
  else
  {
    epee::net_utils::gzip_decoder_lambda gzip_decoder;
    return process_archive(gzip_decoder, false, source, target);
  }
}

//---------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
    {
      TRY_ENTRY();

  string_tools::set_module_name_and_folder(argv[0]);
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);

  tools::signal_handler::install_fatal([](int sig_number, void* address) {
    LOG_ERROR("\n\nFATAL ERROR\nsig: " << sig_number << ", address: " << address);
    std::fflush(nullptr); // all open output streams are flushed
  });

  // Declare the supported options.
  po::options_description desc_general("General options");
  command_line::add_arg(desc_general, command_line::arg_help);

  po::options_description desc_params("Connectivity options");
  command_line::add_arg(desc_params, arg_ip);
  command_line::add_arg(desc_params, arg_port);
  command_line::add_arg(desc_params, arg_rpc_port);
  command_line::add_arg(desc_params, arg_timeout);
  command_line::add_arg(desc_params, arg_request_stat_info);
  command_line::add_arg(desc_params, arg_request_net_state);
  command_line::add_arg(desc_params, arg_generate_keys);
  command_line::add_arg(desc_params, arg_peer_id);
  command_line::add_arg(desc_params, arg_priv_key);
  command_line::add_arg(desc_params, arg_get_daemon_info);
  command_line::add_arg(desc_params, arg_get_aliases);
  command_line::add_arg(desc_params, arg_upate_maintainers_info);
  command_line::add_arg(desc_params, arg_increment_build_no);
  command_line::add_arg(desc_params, command_line::arg_version);
  command_line::add_arg(desc_params, arg_generate_genesis);
  command_line::add_arg(desc_params, arg_genesis_split_amount);
  command_line::add_arg(desc_params, arg_get_info_flags);
  command_line::add_arg(desc_params, arg_log_journal_len);
  command_line::add_arg(desc_params, arg_set_peer_log_level);
  command_line::add_arg(desc_params, arg_download_peer_log);
  command_line::add_arg(desc_params, arg_do_consloe_log);
  command_line::add_arg(desc_params, arg_generate_integrated_address);
  command_line::add_arg(desc_params, arg_pack_file);
  command_line::add_arg(desc_params, arg_unpack_file);
  command_line::add_arg(desc_params, arg_target_file);
  //command_line::add_arg(desc_params, arg_send_ipc);
  

  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);
    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << desc_all << ENDL;
      return false;
    }

    po::store(command_line::parse_command_line(argc, argv, desc_params, false), vm);
    po::notify(vm);

    return true;
  });
  if (!r)
    return EXIT_FAILURE;
  if (command_line::has_arg(vm, arg_do_consloe_log) && command_line::get_arg(vm, arg_do_consloe_log))
  {
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  }


  if (command_line::get_arg(vm, command_line::arg_version))
  {
    std::cout << CURRENCY_NAME << " v" << PROJECT_VERSION_LONG << ENDL;
    return EXIT_SUCCESS;
  }
  if(command_line::has_arg(vm, arg_request_stat_info) || command_line::has_arg(vm, arg_request_net_state))
  {
    return handle_request_stat(vm, command_line::get_arg(vm, arg_peer_id)) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  if(command_line::has_arg(vm, arg_get_daemon_info))
  {
    return handle_get_daemon_info(vm) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if(command_line::has_arg(vm, arg_generate_keys))
  {
    return  generate_and_print_keys() ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if(command_line::has_arg(vm, arg_get_aliases))
  {
    return handle_get_aliases(vm) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if(command_line::has_arg(vm, arg_upate_maintainers_info))
  {
    return handle_update_maintainers_info(vm) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if (command_line::has_arg(vm, arg_increment_build_no))
  {
    return handle_increment_build_no(vm) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if (command_line::has_arg(vm, arg_generate_genesis))
  {
    return generate_genesis(command_line::get_arg(vm, arg_generate_genesis), 10000000000000000) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if (command_line::has_arg(vm, arg_set_peer_log_level))
  {
    return handle_set_peer_log_level(vm) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if (command_line::has_arg(vm, arg_download_peer_log))
  {
    return handle_download_peer_log(vm) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if (command_line::has_arg(vm, arg_generate_integrated_address))
  {
    return handle_generate_integrated_address(vm) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  else if (command_line::has_arg(vm, arg_pack_file) || command_line::has_arg(vm, arg_unpack_file))
  {
    return handle_pack_file(vm) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  /*else if (command_line::has_arg(vm, arg_send_ipc))
  {
    handle_send_ipc(command_line::get_arg(vm, arg_send_ipc)) ? EXIT_SUCCESS : EXIT_FAILURE;
  }*/
  else
  {
    std::cerr << "Not enough arguments." << ENDL;
    std::cerr << desc_all << ENDL;
  }

      CATCH_ENTRY_L0(__func__, EXIT_FAILURE);
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }

  return EXIT_FAILURE;
}

