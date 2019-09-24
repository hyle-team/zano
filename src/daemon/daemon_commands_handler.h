// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/lexical_cast.hpp>

#include "console_handler.h"
#include "p2p/net_node.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "common/util.h"
#include "crypto/hash.h"
#include "warnings.h"
#include "currency_core/bc_offers_service.h"
#include "serialization/binary_utils.h"

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4100)

class daemon_commands_handler
{
  typedef nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > srv_type;
  srv_type& m_srv;
  currency::core_rpc_server& m_rpc;
  typedef epee::srv_console_handlers_binder<nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > > cmd_binder_type;
  cmd_binder_type m_cmd_binder;
public:
  daemon_commands_handler(nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >& srv, currency::core_rpc_server& rpc) :m_srv(srv), m_rpc(rpc)
  {
    m_cmd_binder.set_handler("help", boost::bind(&console_handlers_binder::help, &m_cmd_binder, _1), "Show this help");
    m_cmd_binder.set_handler("print_pl", boost::bind(&daemon_commands_handler::print_pl, this, _1), "Print peer list");
    m_cmd_binder.set_handler("print_cn", boost::bind(&daemon_commands_handler::print_cn, this, _1), "Print connections");
    m_cmd_binder.set_handler("print_bc", boost::bind(&daemon_commands_handler::print_bc, this, _1), "Print blockchain info in a given blocks range, print_bc <begin_height> [<end_height>]");
    m_cmd_binder.set_handler("print_bc_tx", boost::bind(&daemon_commands_handler::print_bc_tx, this, _1), "Print blockchain info with trnsactions in a given blocks range, print_bc <begin_height> [<end_height>]");
    //m_cmd_binder.set_handler("print_bci", boost::bind(&daemon_commands_handler::print_bci, this, _1));
    m_cmd_binder.set_handler("print_bc_outs", boost::bind(&daemon_commands_handler::print_bc_outs, this, _1));
    m_cmd_binder.set_handler("print_market", boost::bind(&daemon_commands_handler::print_market, this, _1));
    m_cmd_binder.set_handler("print_bc_outs_stat", boost::bind(&daemon_commands_handler::print_bc_outs_stat, this, _1));
    m_cmd_binder.set_handler("print_block", boost::bind(&daemon_commands_handler::print_block, this, _1), "Print block, print_block <block_hash> | <block_height>");
    m_cmd_binder.set_handler("print_block_info", boost::bind(&daemon_commands_handler::print_block_info, this, _1), "Print block info, print_block <block_hash> | <block_height>");
    m_cmd_binder.set_handler("print_tx", boost::bind(&daemon_commands_handler::print_tx, this, _1), "Print transaction, print_tx <transaction_hash>");
    m_cmd_binder.set_handler("start_mining", boost::bind(&daemon_commands_handler::start_mining, this, _1), "Start mining for specified address, start_mining <addr> [threads=1]");
    m_cmd_binder.set_handler("stop_mining", boost::bind(&daemon_commands_handler::stop_mining, this, _1), "Stop mining");
    m_cmd_binder.set_handler("print_pool", boost::bind(&daemon_commands_handler::print_pool, this, _1), "Print transaction pool (long format)");
    m_cmd_binder.set_handler("print_pool_sh", boost::bind(&daemon_commands_handler::print_pool_sh, this, _1), "Print transaction pool (short format)");
    m_cmd_binder.set_handler("show_hr", boost::bind(&daemon_commands_handler::show_hr, this, _1), "Start showing hash rate");
    m_cmd_binder.set_handler("hide_hr", boost::bind(&daemon_commands_handler::hide_hr, this, _1), "Stop showing hash rate");
    m_cmd_binder.set_handler("save", boost::bind(&daemon_commands_handler::save, this, _1), "Save blockchain");
    m_cmd_binder.set_handler("print_daemon_stat", boost::bind(&daemon_commands_handler::print_daemon_stat, this, _1), "Print daemon stat");
    m_cmd_binder.set_handler("print_debug_stat", boost::bind(&daemon_commands_handler::print_debug_stat, this, _1), "Print debug stat info");
    m_cmd_binder.set_handler("get_transactions_statics", boost::bind(&daemon_commands_handler::get_transactions_statistics, this, _1), "Calculates transactions statistics");
    m_cmd_binder.set_handler("force_relay_tx_pool", boost::bind(&daemon_commands_handler::force_relay_tx_pool, this, _1), "re-relay all transactions from pool");
    m_cmd_binder.set_handler("enable_channel", boost::bind(&daemon_commands_handler::enable_channel, this, _1), "Enable specified log channel");
    m_cmd_binder.set_handler("disable_channel", boost::bind(&daemon_commands_handler::disable_channel, this, _1), "Enable specified log channel");
    m_cmd_binder.set_handler("clear_cache", boost::bind(&daemon_commands_handler::clear_cache, this, _1), "Clear blockchain storage cache");
    m_cmd_binder.set_handler("clear_altblocks", boost::bind(&daemon_commands_handler::clear_altblocks, this, _1), "Clear blockchain storage cache");
    m_cmd_binder.set_handler("truncate_bc", boost::bind(&daemon_commands_handler::truncate_bc, this, _1), "Truncate blockchain to specified height");
    m_cmd_binder.set_handler("inspect_block_index", boost::bind(&daemon_commands_handler::inspect_block_index, this, _1), "Inspects block index for internal errors");
    m_cmd_binder.set_handler("print_db_performance_data", boost::bind(&daemon_commands_handler::print_db_performance_data, this, _1), "Dumps all db containers performance counters");
    m_cmd_binder.set_handler("search_by_id", boost::bind(&daemon_commands_handler::search_by_id, this, _1), "Search all possible elemets by given id");
    m_cmd_binder.set_handler("find_key_image", boost::bind(&daemon_commands_handler::find_key_image, this, _1), "Try to find tx related to key_image");
    m_cmd_binder.set_handler("rescan_aliases", boost::bind(&daemon_commands_handler::rescan_aliases, this, _1), "Debug function");
    m_cmd_binder.set_handler("forecast_difficulty", boost::bind(&daemon_commands_handler::forecast_difficulty, this, _1), "Prints PoW and PoS difficulties for as many future blocks as possible based on current conditions");
    m_cmd_binder.set_handler("print_deadlock_guard", boost::bind(&daemon_commands_handler::print_deadlock_guard, this, _1), "Print all threads which is blocked or involved in mutex ownership");
    m_cmd_binder.set_handler("print_block_from_hex_blob", boost::bind(&daemon_commands_handler::print_block_from_hex_blob, this, _1), "Unserialize block from hex binary data to json-like representation");
    m_cmd_binder.set_handler("print_tx_from_hex_blob", boost::bind(&daemon_commands_handler::print_tx_from_hex_blob, this, _1), "Unserialize transaction from hex binary data to json-like representation");
    m_cmd_binder.set_handler("print_tx_outputs_usage", boost::bind(&daemon_commands_handler::print_tx_outputs_usage, this, _1), "Analyse if tx outputs for involved in subsequent transactions");
    m_cmd_binder.set_handler("print_difficulties_of_last_n_blocks", boost::bind(&daemon_commands_handler::print_difficulties_of_last_n_blocks, this, _1), "Print difficulties of last n blocks");

#ifdef _DEBUG
    m_cmd_binder.set_handler("debug_set_time_adj", boost::bind(&daemon_commands_handler::debug_set_time_adj, this, _1), "DEBUG: set core time adjustment");
#endif
  }

  bool start_handling()
  {
    m_cmd_binder.start_handling(&m_srv, "", "");
    return true;
  }

  void stop_handling()
  {
    m_cmd_binder.stop_handling();
  }

private:


  //   //--------------------------------------------------------------------------------
  //   std::string get_commands_str()
  //   {
  //     return m_cmd_binder.get_usage();
  //   }
  //   //--------------------------------------------------------------------------------
  //   bool help(const std::vector<std::string>& /*args*/)
  //   {
  //     std::cout << get_commands_str() << ENDL;
  //     return true;
  //   }
  //--------------------------------------------------------------------------------
  bool print_pl(const std::vector<std::string>& args)
  {
    m_srv.log_peerlist();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool save(const std::vector<std::string>& args)
  {
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_daemon_stat(const std::vector<std::string>& args)
  {
    currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
    req.flags = COMMAND_RPC_GET_INFO_FLAG_ALL_FLAGS;
    currency::COMMAND_RPC_GET_INFO::response resp = AUTO_VAL_INIT(resp);
    currency::core_rpc_server::connection_context cc = AUTO_VAL_INIT(cc);
    m_rpc.on_get_info(req, resp, cc);
    LOG_PRINT_L0("STAT INFO:" << ENDL << epee::serialization::store_t_to_json(resp).c_str());
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_debug_stat(const std::vector<std::string>& args)
  {
    currency::core_stat_info si = AUTO_VAL_INIT(si);
    currency::core_stat_info::params pr = AUTO_VAL_INIT(pr);
    pr.chain_len = 10;
    m_srv.get_payload_object().get_core().get_stat_info(pr, si);
    LOG_PRINT_GREEN("CORE_STAT_INFO: " << epee::serialization::store_t_to_json(si), LOG_LEVEL_0);

    return true;
  }
  //--------------------------------------------------------------------------------
  bool get_transactions_statistics(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_blockchain_storage().print_transactions_statistics();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool force_relay_tx_pool(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_tx_pool().force_relay_pool();
    return true;
  }
  bool enable_channel(const std::vector<std::string>& args)
  {
    if (args.size() != 1)
    {
      std::cout << "Missing channel name to enable." << ENDL;
      return true;
    }
    epee::log_space::log_singletone::enable_channel(args[0]);
    return true;
  }
  bool disable_channel(const std::vector<std::string>& args)
  {
    if (args.size() != 1)
    {
      std::cout << "Missing channel name to disable." << ENDL;
      return true;
    }
    epee::log_space::log_singletone::disable_channel(args[0]);
    return true;
  }

  bool clear_cache(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_blockchain_storage().reset_db_cache();
    return true;
  }
  bool clear_altblocks(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_blockchain_storage().clear_altblocks();
    return true;
  }

  //--------------------------------------------------------------------------------
  bool show_hr(const std::vector<std::string>& args)
  {
    if (!m_srv.get_payload_object().get_core().get_miner().is_mining())
    {
      std::cout << "Mining is not started. You need start mining before you can see hash rate." << ENDL;
    }
    else
    {
      m_srv.get_payload_object().get_core().get_miner().do_print_hashrate(true);
    }
    return true;
  }
  //--------------------------------------------------------------------------------
  bool hide_hr(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_miner().do_print_hashrate(false);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_bc_outs(const std::vector<std::string>& args)
  {
    if (args.size() != 1)
    {
      std::cout << "need file path as parameter" << ENDL;
      return true;
    }
    m_srv.get_payload_object().get_core().print_blockchain_outs(args[0]);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_market(const std::vector<std::string>& args)
  {
    bc_services::bc_offers_service* offers_service = dynamic_cast<bc_services::bc_offers_service*>(m_srv.get_payload_object().get_core().get_blockchain_storage().get_attachment_services_manager().get_service_by_id(BC_OFFERS_SERVICE_ID));
    CHECK_AND_ASSERT_MES(offers_service != nullptr, false, "Offers service was not registered in attachment service manager!");

    offers_service->print_market(log_space::log_singletone::get_default_log_folder() + "/market.txt");
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_bc_outs_stat(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_blockchain_storage().print_blockchain_outs_stat();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_cn(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().log_connections();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_bc(const std::vector<std::string>& args)
  {
    if (!args.size())
    {
      std::cout << "need block index parameter" << ENDL;
      return false;
    }
    uint64_t start_index = 0;
    uint64_t end_block_parametr = m_srv.get_payload_object().get_core().get_current_blockchain_size();
    if (!string_tools::get_xtype_from_string(start_index, args[0]))
    {
      std::cout << "wrong starter block index parameter" << ENDL;
      return false;
    }
    if (args.size() > 1 && !string_tools::get_xtype_from_string(end_block_parametr, args[1]))
    {
      std::cout << "wrong end block index parameter" << ENDL;
      return false;
    }

    m_srv.get_payload_object().get_core().print_blockchain(start_index, end_block_parametr);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool truncate_bc(const std::vector<std::string>& args)
  {
    if (!args.size())
    {
      std::cout << "truncate height needed" << ENDL;
      return false;
    }
    uint64_t tr_h = 0;
    if (!string_tools::get_xtype_from_string(tr_h, args[0]))
    {
      std::cout << "wrong truncate index" << ENDL;
      return false;
    }

    m_srv.get_payload_object().get_core().get_blockchain_storage().truncate_blockchain(tr_h);
    return true;
  }
  bool inspect_block_index(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_blockchain_storage().inspect_blocks_index();
    return true;
  }
  bool print_db_performance_data(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_blockchain_storage().print_db_cache_perfeormance_data();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool search_by_id(const std::vector<std::string>& args)
  {

    if (!args.size())
    {
      std::cout << "need ID parameter" << ENDL;
      return false;
    }
    crypto::hash id = currency::null_hash;
    if (!parse_hash256(args[0], id))
    {
      std::cout << "specified ID parameter '" << args[0] << "' is wrong" << ENDL;
      return false;
    }
    std::list<std::string> res_list;
    m_srv.get_payload_object().get_core().get_blockchain_storage().search_by_id(id, res_list);
    std::string joined = boost::algorithm::join(res_list, "|");
    LOG_PRINT_L0("Result of search: " << joined);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool find_key_image(const std::vector<std::string>& args)
  {

    if (!args.size())
    {
      std::cout << "need key_image parameter" << ENDL;
      return false;
    }
    crypto::key_image ki = currency::null_ki;
    if (!epee::string_tools::parse_tpod_from_hex_string(args[0], ki))
    {
      std::cout << "specified key_image parameter '" << args[0] << "' is wrong" << ENDL;
      return false;
    }
    std::list<std::string> res_list;
    crypto::hash tx_id = currency::null_hash;
    auto tx_chain_entry = m_srv.get_payload_object().get_core().get_blockchain_storage().find_key_image_and_related_tx(ki, tx_id);

    if (tx_chain_entry)
    {
      LOG_PRINT_L0("Found tx: " << ENDL << obj_to_json_str(tx_chain_entry->tx) << ENDL << "height: " << tx_chain_entry->m_keeper_block_height);
    }
    if (tx_id == currency::null_hash)
    {
      LOG_PRINT_L0("Not found any related tx.");
    }
    else
    {
      LOG_PRINT_L0("TxID: " << tx_id);
    }
    return true;
  }
  //--------------------------------------------------------------------------------
  bool rescan_aliases(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_blockchain_storage().validate_all_aliases_for_new_median_mode();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_bc_tx(const std::vector<std::string>& args)
  {
    if (!args.size())
    {
      std::cout << "need block index parameter" << ENDL;
      return false;
    }
    uint64_t start_index = 0;
    uint64_t end_block_parametr = m_srv.get_payload_object().get_core().get_current_blockchain_size();
    if (!string_tools::get_xtype_from_string(start_index, args[0]))
    {
      std::cout << "wrong starter block index parameter" << ENDL;
      return false;
    }
    if (args.size() > 1 && !string_tools::get_xtype_from_string(end_block_parametr, args[1]))
    {
      std::cout << "wrong end block index parameter" << ENDL;
      return false;
    }
    LOG_PRINT_GREEN("Storing text to blockchain_with_tx.txt....", LOG_LEVEL_0);
    m_srv.get_payload_object().get_core().get_blockchain_storage().print_blockchain_with_tx(start_index, end_block_parametr);
    LOG_PRINT_GREEN("Done", LOG_LEVEL_0);
    return true;
  }
  //--------------------------------------------------------------------------------
  template<class t_item>
  bool print_t_from_hex_blob(const std::string& item_hex_blob)
  {
    std::string bin_buff;
    bool res = epee::string_tools::parse_hexstr_to_binbuff(item_hex_blob, bin_buff);
    CHECK_AND_ASSERT_MES(res, false, "failed to parse hex");

    t_item item = AUTO_VAL_INIT(item);

    res = ::serialization::parse_binary(bin_buff, item);
    CHECK_AND_ASSERT_MES(res, false, "failed to parse binary");


    LOG_PRINT_L0("OBJECT " << typeid(item).name() << ": " << ENDL << obj_to_json_str(item));
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block_from_hex_blob(const std::vector<std::string>& args)
  {
    if (!args.size())
    {
      std::cout << "need block blob parameter" << ENDL;
      return false;
    }

    print_t_from_hex_blob<currency::block>(args[0]);

    LOG_PRINT_GREEN("Done", LOG_LEVEL_0);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_tx_from_hex_blob(const std::vector<std::string>& args)
  {
    if (!args.size())
    {
      std::cout << "need block blob parameter" << ENDL;
      return false;
    }

    print_t_from_hex_blob<currency::transaction>(args[0]);

    LOG_PRINT_GREEN("Done", LOG_LEVEL_0);
    return true;
  }
  //--------------------------------------------------------------------------------
  struct tx_pool_exported_blobs
  {
    std::list<currency::tx_rpc_extended_info> all_txs_details;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(all_txs_details)
    END_KV_SERIALIZE_MAP()
  };
  //--------------------------------------------------------------------------------
  bool export_tx_pool_to_json(const std::vector<std::string>& args)
  {
    //     if (!args.size())
    //     {
    //       std::cout << "need block blob parameter" << ENDL;
    //       return false;
    //     }
    //     tx_pool_exported_blobs tx_pool_json;
    //     m_srv.get_payload_object().get_core().get_tx_pool().get_all_transactions_details(tx_pool_json.all_txs_details);
    //     std::string pool_state = epee::serialization::store_t_to_json(tx_pool_json);
    //     CHECK_AND_ASSERT_THROW(pool_state.size(), false, "Unable to export pool");
    // 
    //     bool r = file_io_utils::save_string_to_file(args[0], pool_state);
    //     CHECK_AND_ASSERT_THROW(r, false, "Unable to export pool");
    //     LOG_PRINT_GREEN("Exported OK(" << tx_pool_json.all_txs_details.size() <<" transactions)");
    return true;
  }
  //--------------------------------------------------------------------------------
  bool import_tx_pool_to_json(const std::vector<std::string>& args)
  {
    //     if (!args.size())
    //     {
    //       std::cout << "need block blob parameter" << ENDL;
    //       return false;
    //     }
    // 
    //     std::string buff;
    //     bool r = file_io_utils::load_file_to_string(args[0], buff);
    //     
    //     tx_pool_exported_blobs tx_pool_json;
    // 
    // 
    //     m_srv.get_payload_object().get_core().get_tx_pool().get_all_transactions_details(tx_pool_json.all_txs_details);
    //     std::string pool_state = epee::serialization::store_t_to_json(tx_pool_json);
    //     CHECK_AND_ASSERT_THROW(pool_state.size(), false, "Unable to export pool");
    // 
    // 
    //     CHECK_AND_ASSERT_THROW(r, false, "Unable to export pool");
    //     LOG_PRINT_GREEN("Exported OK(" << tx_pool_json.all_txs_details.size() << " transactions)");
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_bci(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().print_blockchain_index();
    return true;
  }
  //--------------------------------------------------------------------------------
  template <typename T>
  static bool print_as_json(T& obj)
  {
    std::cout << obj_to_json_str(obj) << ENDL;
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block_by_height(uint64_t height)
  {
    std::list<currency::block> blocks;
    m_srv.get_payload_object().get_core().get_blocks(height, 1, blocks);

    if (1 == blocks.size())
    {
      currency::block& block = blocks.front();
      LOG_PRINT_GREEN("------------------ block_id: " << get_block_hash(block) << " ------------------" << ENDL << currency::obj_to_json_str(block), LOG_LEVEL_0);
    }
    else
    {
      uint64_t current_height;
      crypto::hash top_id;
      m_srv.get_payload_object().get_core().get_blockchain_top(current_height, top_id);
      LOG_PRINT_GREEN("block wasn't found. Current block chain height: " << current_height << ", requested: " << height, LOG_LEVEL_0);
      return false;
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block_info_by_height(uint64_t height)
  {
    std::list<currency::block_rpc_extended_info> blocks;
    bool r = m_srv.get_payload_object().get_core().get_blockchain_storage().get_main_blocks_rpc_details(height, 1, false, blocks);
    if (r && blocks.size())
    {
      currency::block b = AUTO_VAL_INIT(b);
      m_srv.get_payload_object().get_core().get_blockchain_storage().get_block_by_height(height, b);
      currency::blobdata blob = block_to_blob(b);
      std::string block_hex = epee::string_tools::buff_to_hex_nodelimer(blob);

      currency::block_rpc_extended_info& rbei = blocks.back();
      LOG_PRINT_GREEN("------------------ block_id: " << rbei.id << " ------------------" << 
        ENDL << epee::serialization::store_t_to_json(rbei) << ENDL
        << " ------------------ hex_blob: " << ENDL << block_hex, LOG_LEVEL_0);
    }
    else
    {
      LOG_PRINT_GREEN("block wasn't found: " << height, LOG_LEVEL_0);
      return false;
    }
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block_info_by_hash(const std::string& arg)
  {
    crypto::hash block_hash;
    if (!parse_hash256(arg, block_hash))
    {
      return false;
    }

    currency::block_rpc_extended_info bei = AUTO_VAL_INIT(bei);
    bool r = m_srv.get_payload_object().get_core().get_blockchain_storage().get_main_block_rpc_details(block_hash, bei);

    if (r)
    {
      currency::block b = AUTO_VAL_INIT(b);
      m_srv.get_payload_object().get_core().get_blockchain_storage().get_block_by_hash(block_hash, b);
      currency::blobdata blob = block_to_blob(b);
      std::string block_hex = epee::string_tools::buff_to_hex_nodelimer(blob);
      //      currency::block& block = bei.bl;
      LOG_PRINT_GREEN("------------------ block_id: " << bei.id << " ------------------" << ENDL 
        << epee::serialization::store_t_to_json(bei) << ENDL 
        << " ------------------ hex_blob: " << ENDL << block_hex, LOG_LEVEL_0);
    }
    else
    {
      LOG_PRINT_GREEN("block wasn't found: " << arg, LOG_LEVEL_0);
      return false;
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block_by_hash(const std::string& arg)
  {
    crypto::hash block_hash;
    if (!parse_hash256(arg, block_hash))
    {
      return false;
    }

    //std::list<crypto::hash> block_ids;
    //block_ids.push_back(block_hash);
    //std::list<currency::block> blocks;
    //std::list<crypto::hash> missed_ids;
    currency::block_extended_info bei = AUTO_VAL_INIT(bei);
    bool r = m_srv.get_payload_object().get_core().get_blockchain_storage().get_block_extended_info_by_hash(block_hash, bei);
    //m_srv.get_payload_object().get_core().get_blocks(block_ids, blocks, missed_ids);

    if (r)
    {
      //      currency::block& block = bei.bl;
      LOG_PRINT_GREEN("------------------ block_id: " << get_block_hash(bei.bl) << " ------------------" << ENDL << currency::obj_to_json_str(bei), LOG_LEVEL_0);
      m_srv.get_payload_object().get_core().get_blockchain_storage().calc_tx_cummulative_blob(bei.bl);
    }
    else
    {
      LOG_PRINT_GREEN("block wasn't found: " << arg, LOG_LEVEL_0);
      return false;
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block(const std::vector<std::string>& args)
  {
    if (args.empty())
    {
      std::cout << "expected: print_block (<block_hash> | <block_height>)" << std::endl;
      return true;
    }

    const std::string& arg = args.front();
    try
    {
      uint64_t height = boost::lexical_cast<uint64_t>(arg);
      print_block_by_height(height);
    }
    catch (boost::bad_lexical_cast&)
    {
      print_block_by_hash(arg);
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block_info(const std::vector<std::string>& args)
  {
    if (args.empty())
    {
      std::cout << "expected: print_block_info (<block_hash> | <block_height>)" << std::endl;
      return true;
    }

    const std::string& arg = args.front();
    try
    {
      uint64_t height = boost::lexical_cast<uint64_t>(arg);
      print_block_info_by_height(height);
    }
    catch (boost::bad_lexical_cast&)
    {
      print_block_info_by_hash(arg);
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_tx(const std::vector<std::string>& args)
  {
    if (args.empty())
    {
      std::cout << "expected: print_tx <transaction hash>" << std::endl;
      return true;
    }

    const std::string& str_hash = args.front();
    crypto::hash tx_hash;
    if (!parse_hash256(str_hash, tx_hash))
    {
      return true;
    }

    //     std::vector<crypto::hash> tx_ids;
    //     tx_ids.push_back(tx_hash);
    //     std::list<currency::transaction> txs;
    //     std::list<crypto::hash> missed_ids;
    //     m_srv.get_payload_object().get_core().get_transactions(tx_ids, txs, missed_ids);

    currency::transaction_chain_entry tx_entry = AUTO_VAL_INIT(tx_entry);

    if (!m_srv.get_payload_object().get_core().get_blockchain_storage().get_tx_chain_entry(tx_hash, tx_entry))
    {
      LOG_PRINT_RED("transaction wasn't found: " << tx_hash, LOG_LEVEL_0);
    }
    currency::block_extended_info bei = AUTO_VAL_INIT(bei);
    m_srv.get_payload_object().get_core().get_blockchain_storage().get_block_extended_info_by_height(tx_entry.m_keeper_block_height, bei);
    uint64_t timestamp = bei.bl.timestamp;

    const currency::transaction& tx = tx_entry.tx;
    std::stringstream ss;

    ss << "------------------------------------------------------"
      << ENDL << "tx_id: " << tx_hash
      << ENDL << "keeper_block: " << tx_entry.m_keeper_block_height << ",  timestamp (" << timestamp << ") " << epee::misc_utils::get_internet_time_str(timestamp)
      << ENDL << currency::obj_to_json_str(tx)
      << ENDL << "------------------------------------------------------"
      << ENDL << epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(tx))
      << ENDL << "------------------------------------------------------";


    ss << "ATTACHMENTS: " << ENDL;
    for (auto at : tx.attachment)
    {
      if (at.type() == typeid(currency::tx_service_attachment))
      {
        const currency::tx_service_attachment& sa = boost::get<currency::tx_service_attachment>(at);
        ss << "++++++++++++++++++++++++++++++++ " << ENDL;
        ss << "[SERVICE_ATTACHMENT]: ID = \'" << sa.service_id << "\', INSTRUCTION: \'" << sa.instruction << "\'" << ENDL;

        if (!(sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY))
        {
          std::string body = sa.body;
          if (sa.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY)
          {
            bool r = epee::zlib_helper::unpack(sa.body, body);
            CHECK_AND_ASSERT_MES(r, false, "Failed to unpack");
          }
          ss << "BODY: " << body << ENDL;
        }
      }
    }

    LOG_PRINT_GREEN(ss.str(), LOG_LEVEL_0);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_tx_outputs_usage(const std::vector<std::string>& args)
  {
    if (args.empty())
    {
      std::cout << "expected: print_tx <transaction hash>" << std::endl;
      return true;
    }

    const std::string& str_hash = args.front();
    crypto::hash tx_hash;
    if (!parse_hash256(str_hash, tx_hash))
    {
      return true;
    }

    m_srv.get_payload_object().get_core().get_blockchain_storage().print_tx_outputs_lookup(tx_hash);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_difficulties_of_last_n_blocks(const std::vector<std::string>& args)
  {
    if (args.empty())
    {
      std::cout << "expected: n - number of blocks to read" << std::endl;
      return true;
    }

    const std::string& amount = args.front();
    uint64_t n = 0;
    if (!epee::string_tools::get_xtype_from_string(n, amount))
    {
      std::cout << "unable to convert to number '" << amount << "'" << std::endl;
      return true;
    }

    m_srv.get_payload_object().get_core().get_blockchain_storage().print_last_n_difficulty_numbers(n);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_pool(const std::vector<std::string>& args)
  {
    LOG_PRINT_L0("Pool state: " << ENDL << m_srv.get_payload_object().get_core().print_pool(false));
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_pool_sh(const std::vector<std::string>& args)
  {
    LOG_PRINT_L0("Pool state: " << ENDL << m_srv.get_payload_object().get_core().print_pool(true));
    return true;
  }  //--------------------------------------------------------------------------------
  bool start_mining(const std::vector<std::string>& args)
  {
    if (!args.size())
    {
      std::cout << "Please, specify wallet address to mine for: start_mining <addr> [threads=1]" << std::endl;
      return true;
    }

    currency::account_public_address adr;
    if (!currency::get_account_address_from_str(adr, args.front()))
    {
      std::cout << "target account address has wrong format" << std::endl;
      return true;
    }
    size_t threads_count = 1;
    if (args.size() > 1)
    {
      bool ok = string_tools::get_xtype_from_string(threads_count, args[1]);
      threads_count = (ok && 0 < threads_count) ? threads_count : 1;
    }

    m_srv.get_payload_object().get_core().get_miner().start(adr, threads_count);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool stop_mining(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_miner().stop();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool forecast_difficulty(const std::vector<std::string>& args)
  {
    if (args.size() > 0)
    {
      LOG_ERROR("command requires no agruments");
      return false;
    }

    std::vector<std::pair<uint64_t, currency::wide_difficulty_type>> pow_diffs, pos_diffs;

    bool r = false;
    r = m_srv.get_payload_object().get_core().get_blockchain_storage().forecast_difficulty(pow_diffs, false);
    CHECK_AND_ASSERT_MES(r, false, "forecast_difficulty failed");

    r = m_srv.get_payload_object().get_core().get_blockchain_storage().forecast_difficulty(pos_diffs, true);
    CHECK_AND_ASSERT_MES(r, false, "forecast_difficulty failed");

    CHECK_AND_ASSERT_MES(pow_diffs.size() == pow_diffs.size(), false, "mismatch in sizes: " << pow_diffs.size() << ", " << pow_diffs.size());

    std::stringstream ss;
    ss << " The next " << pow_diffs.size() - 1 << " PoW and PoS blocks will have the following difficulties:" << ENDL;
    ss << " #  aprox. PoW height   PoW difficulty  aprox. PoS height   PoS difficulty" << ENDL;
    for (size_t i = 0; i < pow_diffs.size(); ++i)
    {
      ss << std::right << std::setw(2) << i;
      ss << "  " << std::setw(6) << std::left << pow_diffs[i].first;
      if (i == 0)
        ss << " (last block) ";
      else
        ss << "              ";
      ss << std::setw(10) << std::left << pow_diffs[i].second;

      ss << "      ";
      ss << std::setw(6) << std::left << pos_diffs[i].first;
      if (i == 0)
        ss << " (last block) ";
      else
        ss << "              ";
      ss << pos_diffs[i].second;
      ss << ENDL;
    }

    LOG_PRINT_L0(ENDL << ss.str());
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_deadlock_guard(const std::vector<std::string>& args)
  {
    LOG_PRINT_L0(ENDL << epee::deadlock_guard_singleton::get_dlg_state());
    return true;
  }
  //--------------------------------------------------------------------------------
#ifdef _DEBUG  
  static std::atomic<int64_t>& debug_core_time_shift_accessor()
  {
    static std::atomic<int64_t> time_shift(0);
    return time_shift;
  }

  static uint64_t debug_core_time_function()
  {
    return (int64_t)time(NULL) + debug_core_time_shift_accessor().load(std::memory_order_relaxed);
  }

  bool debug_set_time_adj(const std::vector<std::string>& args)
  {
    if (args.size() != 1)
    {
      LOG_PRINT_RED("one arg required: signed time shift in seconds", LOG_LEVEL_0);
      return false;
    }

    int64_t time_shift = 0;
    if (!epee::string_tools::string_to_num_fast(args[0], time_shift))
    {
      LOG_PRINT_RED("could not parse: " << args[0], LOG_LEVEL_0);
      return false;
    }

    uint64_t time_before = debug_core_time_function();
    debug_core_time_shift_accessor().store(time_shift);
    uint64_t time_after = debug_core_time_function();

    currency::blockchain_storage& bcs = m_srv.get_payload_object().get_core().get_blockchain_storage();

    currency::core_runtime_config crc = bcs.get_core_runtime_config();
    crc.get_core_time = &debug_core_time_function;
    bcs.set_core_runtime_config(crc);

    LOG_PRINT_L0("debug time shift set to " << time_shift << " : time before: " << time_before << ", time_after: " << time_after);

    return true;
  }
#endif



};
POP_WARNINGS