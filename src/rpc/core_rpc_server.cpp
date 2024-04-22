// Copyright (c) 2014-2022 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
using namespace epee;

#include "core_rpc_server.h"
#include "common/command_line.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/account.h"

#include "misc_language.h"
#include "crypto/hash.h"
#include "core_rpc_server_error_codes.h"



namespace currency
{
  namespace
  {
    const command_line::arg_descriptor<std::string> arg_rpc_bind_ip    ("rpc-bind-ip", "", "127.0.0.1");
    const command_line::arg_descriptor<std::string> arg_rpc_bind_port  ("rpc-bind-port", "", std::to_string(RPC_DEFAULT_PORT));
    const command_line::arg_descriptor<bool> arg_rpc_ignore_status     ("rpc-ignore-offline", "Let rpc calls despite online/offline status");
  }
  //-----------------------------------------------------------------------------------
  void core_rpc_server::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_rpc_ignore_status);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  core_rpc_server::core_rpc_server(core& cr, nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >& p2p,
    bc_services::bc_offers_service& of)
    : m_core(cr)
    , m_p2p(p2p)
    , m_of(of)
    , m_ignore_status(false)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_bind_ip = command_line::get_arg(vm, arg_rpc_bind_ip);
    m_port = command_line::get_arg(vm, arg_rpc_bind_port);
    if (command_line::has_arg(vm, arg_rpc_ignore_status))
    {
      m_ignore_status = command_line::get_arg(vm, arg_rpc_ignore_status);
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::init(const boost::program_options::variables_map& vm)
  {
    m_net_server.set_threads_prefix("RPC");
    bool r = handle_command_line(vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to process command line in core_rpc_server");
    return epee::http_server_impl_base<core_rpc_server, connection_context>::init(m_port, m_bind_ip);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::check_core_ready_(const std::string& calling_method)
  {
#ifndef TESTNET
    if (m_ignore_status)
      return true;
    if(!m_p2p.get_payload_object().is_synchronized())
    {
      LOG_PRINT_L0("[" << calling_method << "]Core busy cz is_synchronized");
      return false;
    }
 #endif
    return true;
  }
#define check_core_ready() check_core_ready_(LOCAL_FUNCTION_DEF__)
#define CHECK_CORE_READY() if(!check_core_ready()){res.status =  API_RETURN_CODE_BUSY;return true;}
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.height = m_core.get_current_blockchain_size();
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, connection_context& cntx)
  {
    //unconditional values 
    res.height = m_core.get_current_blockchain_size();
    res.tx_count = m_core.get_blockchain_storage().get_total_transactions() - res.height; //without coinbase
    res.tx_pool_size = m_core.get_pool_transactions_count();
    res.alt_blocks_count = m_core.get_blockchain_storage().get_alternative_blocks_count();
    uint64_t total_conn = m_p2p.get_connections_count();
    res.outgoing_connections_count = m_p2p.get_outgoing_connections_count();
    res.incoming_connections_count = total_conn - res.outgoing_connections_count;
    res.synchronized_connections_count = m_p2p.get_payload_object().get_synchronized_connections_count();
    res.white_peerlist_size = m_p2p.get_peerlist_manager().get_white_peers_count();
    res.grey_peerlist_size = m_p2p.get_peerlist_manager().get_gray_peers_count();
    res.current_blocks_median = m_core.get_blockchain_storage().get_current_comulative_blocksize_limit() / 2;
    res.alias_count = m_core.get_blockchain_storage().get_aliases_count();
    res.current_max_allowed_block_size = m_core.get_blockchain_storage().get_current_comulative_blocksize_limit();
    if (!res.outgoing_connections_count)
      res.daemon_network_state = COMMAND_RPC_GET_INFO::daemon_network_state_connecting;
    else if (m_p2p.get_payload_object().is_synchronized())
      res.daemon_network_state = COMMAND_RPC_GET_INFO::daemon_network_state_online;
    else
      res.daemon_network_state = COMMAND_RPC_GET_INFO::daemon_network_state_synchronizing;
    res.synchronization_start_height = m_p2p.get_payload_object().get_core_inital_height();
    res.max_net_seen_height = m_p2p.get_payload_object().get_max_seen_height();
    m_p2p.get_maintainers_info(res.mi);
    res.pos_allowed = m_core.get_blockchain_storage().is_pos_allowed();
    wide_difficulty_type pos_diff = m_core.get_blockchain_storage().get_cached_next_difficulty(true);
    res.pos_difficulty = pos_diff.convert_to<std::string>();
    res.pow_difficulty = m_core.get_blockchain_storage().get_cached_next_difficulty(false).convert_to<uint64_t>();
    res.default_fee = m_core.get_blockchain_storage().get_core_runtime_config().tx_default_fee;
    res.minimum_fee = m_core.get_blockchain_storage().get_core_runtime_config().tx_pool_min_fee;

    auto & hf = m_core.get_blockchain_storage().get_core_runtime_config().hard_forks.m_height_the_hardfork_n_active_after;
    for (size_t i = 0; i != hf.size(); i++)
    {
      res.is_hardfok_active.push_back(m_core.get_blockchain_storage().is_hardfork_active(i));
    }

    //conditional values 
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_NET_TIME_DELTA_MEDIAN)
    {
      int64_t last_median2local_time_diff, last_ntp2local_time_diff;
      if (!m_p2p.get_payload_object().get_last_time_sync_difference(last_median2local_time_diff, last_ntp2local_time_diff))
        res.net_time_delta_median = 1;
    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_CURRENT_NETWORK_HASHRATE_50)
      res.current_network_hashrate_50 = m_core.get_blockchain_storage().get_current_hashrate(50);
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_CURRENT_NETWORK_HASHRATE_350)
      res.current_network_hashrate_350 = m_core.get_blockchain_storage().get_current_hashrate(350);
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_SECONDS_FOR_10_BLOCKS)
      res.seconds_for_10_blocks = m_core.get_blockchain_storage().get_seconds_between_last_n_block(10);
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_SECONDS_FOR_30_BLOCKS)
      res.seconds_for_30_blocks = m_core.get_blockchain_storage().get_seconds_between_last_n_block(30);
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_TRANSACTIONS_DAILY_STAT)
      m_core.get_blockchain_storage().get_transactions_daily_stat(res.transactions_cnt_per_day, res.transactions_volume_per_day);
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_LAST_POS_TIMESTAMP)
    {
      auto pos_bl_ptr = m_core.get_blockchain_storage().get_last_block_of_type(true);
      if (pos_bl_ptr)
        res.last_pos_timestamp = pos_bl_ptr->bl.timestamp;
    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_LAST_POW_TIMESTAMP)
    {
      auto pow_bl_ptr = m_core.get_blockchain_storage().get_last_block_of_type(false);
      if (pow_bl_ptr)
        res.last_pow_timestamp = pow_bl_ptr->bl.timestamp;
    }
    boost::multiprecision::uint128_t total_coins = 0;
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS)
    {
      total_coins = m_core.get_blockchain_storage().total_coins();
      res.total_coins = boost::lexical_cast<std::string>(total_coins);
    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_LAST_BLOCK_SIZE)
    {
      std::vector<size_t> sz;
      m_core.get_blockchain_storage().get_last_n_blocks_sizes(sz, 1);
      res.last_block_size = sz.size() ? sz.back() : 0;
    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_TX_COUNT_IN_LAST_BLOCK)
    {
      currency::block b = AUTO_VAL_INIT(b);
      m_core.get_blockchain_storage().get_top_block(b);
      res.tx_count_in_last_block = b.tx_hashes.size();
    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_POS_SEQUENCE_FACTOR)
      res.pos_sequence_factor = m_core.get_blockchain_storage().get_current_sequence_factor(true);
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_POW_SEQUENCE_FACTOR)
      res.pow_sequence_factor = m_core.get_blockchain_storage().get_current_sequence_factor(false);
    if (req.flags&(COMMAND_RPC_GET_INFO_FLAG_POS_DIFFICULTY | COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS))
    {
      res.block_reward = currency::get_base_block_reward(res.height);
      currency::block b = AUTO_VAL_INIT(b);
      m_core.get_blockchain_storage().get_top_block(b);
      res.last_block_total_reward = currency::get_reward_from_miner_tx(b.miner_tx);
      res.pos_diff_total_coins_rate = (pos_diff / (total_coins - PREMINE_AMOUNT + 1)).convert_to<uint64_t>();
      res.last_block_timestamp = b.timestamp;
      res.last_block_hash = string_tools::pod_to_hex(get_block_hash(b));
    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_POS_BLOCK_TS_SHIFT_VS_ACTUAL)
    {
      res.pos_block_ts_shift_vs_actual = 0;
      auto last_pos_block_ptr = m_core.get_blockchain_storage().get_last_block_of_type(true);
      if (last_pos_block_ptr)
        res.pos_block_ts_shift_vs_actual = last_pos_block_ptr->bl.timestamp - get_block_datetime(last_pos_block_ptr->bl);
    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_OUTS_STAT)
      m_core.get_blockchain_storage().get_outs_index_stat(res.outs_stat);

    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_PERFORMANCE)
    {
      currency::blockchain_storage::performnce_data& pd = m_core.get_blockchain_storage().get_performnce_data();
      //block processing zone
      res.performance_data.block_processing_time_0 = pd.block_processing_time_0_ms.get_avg()*1000;
      res.performance_data.block_processing_time_1 = pd.block_processing_time_1.get_avg();
      res.performance_data.target_calculating_time_2 = pd.target_calculating_time_2.get_avg();
      res.performance_data.longhash_calculating_time_3 = pd.longhash_calculating_time_3.get_avg();
      res.performance_data.all_txs_insert_time_5 = pd.all_txs_insert_time_5.get_avg();
      res.performance_data.etc_stuff_6 = pd.etc_stuff_6.get_avg();
      res.performance_data.insert_time_4 = pd.insert_time_4.get_avg();
      res.performance_data.raise_block_core_event = pd.raise_block_core_event.get_avg();
      res.performance_data.target_calculating_enum_blocks = pd.target_calculating_enum_blocks.get_avg();
      res.performance_data.target_calculating_calc = pd.target_calculating_calc.get_avg();
      //tx processing zone
      res.performance_data.tx_check_inputs_time = pd.tx_check_inputs_time.get_avg();
      res.performance_data.tx_add_one_tx_time = pd.tx_add_one_tx_time.get_avg();
      res.performance_data.tx_process_extra = pd.tx_process_extra.get_avg();
      res.performance_data.tx_process_attachment = pd.tx_process_attachment.get_avg();
      res.performance_data.tx_process_inputs = pd.tx_process_inputs.get_avg();
      res.performance_data.tx_push_global_index = pd.tx_push_global_index.get_avg();
      res.performance_data.tx_check_exist = pd.tx_check_exist.get_avg();
      res.performance_data.tx_append_time = pd.tx_append_time.get_avg();
      res.performance_data.tx_append_rl_wait = pd.tx_append_rl_wait.get_avg();
      res.performance_data.tx_append_is_expired = pd.tx_append_is_expired.get_avg();
      res.performance_data.tx_mixin_count = pd.tx_mixin_count.get_avg();

      
      res.performance_data.tx_store_db = pd.tx_store_db.get_avg();
      //db performance count
      res.performance_data.map_size = pd.si.map_size;
      res.performance_data.tx_count = pd.si.tx_count;
      res.performance_data.writer_tx_count = pd.si.write_tx_count;
#define COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(field_name)   res.performance_data.field_name = pd.field_name.get_avg(); 
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_prefix_hash);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_attachment_check);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop_kimage_check);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop_ch_in_val_sig);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop_scan_outputkeys_get_item_size);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop_scan_outputkeys_relative_to_absolute);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop_scan_outputkeys_loop);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop_scan_outputkeys_loop_get_subitem);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop_scan_outputkeys_loop_find_tx);
      COPY_AVG_TO_BLOCK_CHAIN_PERF_DATA(tx_check_inputs_loop_scan_outputkeys_loop_handle_output);

      //tx pool perfromance stat
      const currency::tx_memory_pool::performnce_data& pool_pd = m_core.get_tx_pool().get_performnce_data();
#define COPY_AVG_TO_POOL_PERF_DATA(field_name)   res.tx_pool_performance_data.field_name = pool_pd.field_name.get_avg();
      COPY_AVG_TO_POOL_PERF_DATA(tx_processing_time);
      COPY_AVG_TO_POOL_PERF_DATA(check_inputs_types_supported_time);
      COPY_AVG_TO_POOL_PERF_DATA(expiration_validate_time);
      COPY_AVG_TO_POOL_PERF_DATA(validate_amount_time);
      COPY_AVG_TO_POOL_PERF_DATA(validate_alias_time);
      COPY_AVG_TO_POOL_PERF_DATA(check_keyimages_ws_ms_time);
      COPY_AVG_TO_POOL_PERF_DATA(check_inputs_time);
      COPY_AVG_TO_POOL_PERF_DATA(begin_tx_time);
      COPY_AVG_TO_POOL_PERF_DATA(update_db_time);
      COPY_AVG_TO_POOL_PERF_DATA(db_commit_time);

    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_PERFORMANCE)
    {
      res.offers_count = m_of.get_offers_container().size();
    }
    if (req.flags&COMMAND_RPC_GET_INFO_FLAG_EXPIRATIONS_MEDIAN)
    {
      res.expiration_median_timestamp = m_core.get_blockchain_storage().get_tx_expiration_median();
    }
      


    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_blocks_direct(const COMMAND_RPC_GET_BLOCKS_DIRECT::request& req, COMMAND_RPC_GET_BLOCKS_DIRECT::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();

    if (req.block_ids.back() != m_core.get_blockchain_storage().get_block_id_by_height(0))
    {
      //genesis mismatch, return specific
      res.status = API_RETURN_CODE_GENESIS_MISMATCH;
      return true;
    }

    if (req.minimum_height >= m_core.get_blockchain_storage().get_current_blockchain_size())
    {
      //wrong minimum_height
      res.status = API_RETURN_CODE_BAD_ARG;
      return true;
    }

    blockchain_storage::blocks_direct_container bs;
    if(!m_core.get_blockchain_storage().find_blockchain_supplement(req.block_ids, bs, res.current_height, res.start_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT, req.minimum_height))
    {
      res.status = API_RETURN_CODE_FAIL;
      return false;
    }

    for(auto& b: bs)
    {
      res.blocks.resize(res.blocks.size()+1);
      res.blocks.back().block_ptr = b.first;
      res.blocks.back().txs_ptr = std::move(b.second);
      res.blocks.back().coinbase_ptr = b.third;
    }

    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_blocks(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();

    if (req.block_ids.back() != m_core.get_blockchain_storage().get_block_id_by_height(0))
    {
      //genesis mismatch, return specific
      res.status = API_RETURN_CODE_GENESIS_MISMATCH;
      return true;
    }

    if (req.minimum_height >= m_core.get_blockchain_storage().get_current_blockchain_size())
    {
      //wrong minimum_height
      res.status = API_RETURN_CODE_BAD_ARG;
      return true;
    }

    blockchain_storage::blocks_direct_container bs;
    if (!m_core.get_blockchain_storage().find_blockchain_supplement(req.block_ids, bs, res.current_height, res.start_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT, req.minimum_height))
    {
      res.status = API_RETURN_CODE_FAIL;
      return false;
    }

    for (auto& b : bs)
    {
      res.blocks.resize(res.blocks.size()+1);
      res.blocks.back().block = block_to_blob(b.first->bl);
      CHECK_AND_ASSERT_MES(b.third.get(), false, "Internal error on handling COMMAND_RPC_GET_BLOCKS_FAST: b.third is empty, ie coinbase info is not prepared");
      res.blocks.back().coinbase_global_outs = b.third->m_global_output_indexes;
      res.blocks.back().tx_global_outs.resize(b.second.size());
      size_t i = 0;
      
      BOOST_FOREACH(auto& t, b.second)
      {
        res.blocks.back().txs.push_back(tx_to_blob(t->tx));
        res.blocks.back().tx_global_outs[i].v = t->m_global_output_indexes;
        i++;
      }
    }

    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_random_outs(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.status = API_RETURN_CODE_FAIL;

    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req_native = AUTO_VAL_INIT(req_native);
    req_native.amounts = req.amounts;
    req_native.decoys_count = req.outs_count;
    req_native.use_forced_mix_outs = req.use_forced_mix_outs;
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res_native = AUTO_VAL_INIT(res_native);

    if(!m_core.get_random_outs_for_amounts(req_native, res_native))
    {
      return true;
    }

    for (const auto& item : res_native.outs)
    {      
      res.outs.push_back(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY::outs_for_amount());
      res.outs.back().amount = item.amount;
      for (const auto& subitem : item.outs)
      {
        res.outs.back().outs.push_back(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY::out_entry { subitem.global_amount_index, subitem.stealth_address });
      }
    }
    

    res.status = API_RETURN_CODE_OK;

    /*
    std::stringstream ss;
    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount outs_for_amount;
    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
    std::for_each(res.outs.begin(), res.outs.end(), [&](outs_for_amount& ofa)
    {
      ss << "[" << ofa.amount << "]:";
      CHECK_AND_ASSERT_MES(ofa.outs.size(), ;, "internal error: ofa.outs.size() is empty for amount " << ofa.amount);
      std::for_each(ofa.outs.begin(), ofa.outs.end(), [&](out_entry& oe)
          {
            ss << oe.global_amount_index << " ";
          });
      ss << ENDL;
    });
    std::string s = ss.str();
    LOG_PRINT_L2("COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS: " << ENDL << s);
    */

    return true;
  }

    //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_random_outs1(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.status = API_RETURN_CODE_FAIL;

    if(!m_core.get_random_outs_for_amounts(req, res))
    {
      return true;
    }

    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_random_outs3(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.status = API_RETURN_CODE_FAIL;
    if (!m_core.get_blockchain_storage().get_random_outs_for_amounts3(req, res))
    {
      return true;
    }

    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.tx_global_outs.resize(req.txids.size());
    size_t i = 0;
    for (auto& txid : req.txids)
    {
      bool r = m_core.get_tx_outputs_gindexs(txid, res.tx_global_outs[i].v);
      if (!r)
      {
        res.status = API_RETURN_CODE_FAIL;
        return true;
      }
      i++;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_maintainers_info(const COMMAND_RPC_SET_MAINTAINERS_INFO::request& req, COMMAND_RPC_SET_MAINTAINERS_INFO::response& res, connection_context& cntx)
  {
    if(!m_p2p.handle_maintainers_entry(req))
    {
      res.status = "Failed to get call handle_maintainers_entry()";
      return true;
    }

    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_tx_pool(const COMMAND_RPC_GET_TX_POOL::request& req, COMMAND_RPC_GET_TX_POOL::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    std::list<transaction> txs;
    if (!m_core.get_pool_transactions(txs))
    {
      res.status = "Failed to call get_pool_transactions()";
      return true;
    }

    res.tx_expiration_ts_median = m_core.get_blockchain_storage().get_tx_expiration_median();


    for(auto& tx: txs)
    {
      res.txs.push_back(t_serializable_object_to_blob(tx));
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_check_keyimages(const COMMAND_RPC_CHECK_KEYIMAGES::request& req, COMMAND_RPC_CHECK_KEYIMAGES::response& res, connection_context& cntx)
  {
    m_core.get_blockchain_storage().check_keyimages(req.images, res.images_stat);
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    std::vector<crypto::hash> vh;
    for(const auto& tx_hex_str : req.txs_hashes)
    {
      blobdata b;
      if(!string_tools::parse_hexstr_to_binbuff(tx_hex_str, b))
      {
        res.status = "Failed to parse hex representation of transaction hash";
        return true;
      }
      if(b.size() != sizeof(crypto::hash))
      {
        res.status = "Failed, size of data mismatch";
        return true;
      }
      vh.push_back(*reinterpret_cast<const crypto::hash*>(b.data()));
    }
    std::list<crypto::hash> missed_txs;
    std::list<transaction> txs;
    bool r = m_core.get_transactions(vh, txs, missed_txs);
    if(!r)
    {
      res.status = "Failed";
      return true;
    }

    for(auto& tx : txs)
    {
      blobdata blob = t_serializable_object_to_blob(tx);
      res.txs_as_hex.push_back(string_tools::buff_to_hex_nodelimer(blob));
    }

    for(const auto& miss_tx : missed_txs)
    {
      res.missed_tx.push_back(string_tools::pod_to_hex(miss_tx));
    }

    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_offers_ex(const COMMAND_RPC_GET_OFFERS_EX::request& req, COMMAND_RPC_GET_OFFERS_EX::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    m_of.get_offers_ex(req.filter, res.offers, res.total_offers, m_core.get_blockchain_storage().get_core_runtime_config().get_core_time());
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_validate_signature(const COMMAND_VALIDATE_SIGNATURE::request& req, COMMAND_VALIDATE_SIGNATURE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    if (!m_p2p.get_connections_count())
    {
      res.status = API_RETURN_CODE_DISCONNECTED;
      return true;
    }
    std::string buff = epee::string_encoding::base64_decode(req.buff);    
    crypto::public_key pkey = req.pkey;

    if(pkey == currency::null_pkey)
    {
      //need to load pkey from alias
      extra_alias_entry_base eaeb = AUTO_VAL_INIT(eaeb);
      if (!m_core.get_blockchain_storage().get_alias_info(req.alias, eaeb))
      {
        res.status = API_RETURN_CODE_NOT_FOUND;
        return true;
      }
      pkey = eaeb.m_address.spend_public_key;
    }

    crypto::hash h = crypto::cn_fast_hash(buff.data(), buff.size());
    bool sig_check_res = crypto::check_signature(h, pkey, req.sig);
    if (!sig_check_res)
    {
      res.status = API_RETURN_CODE_FAIL;
      return true;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_pos_mining_details(const COMMAND_RPC_GET_POS_MINING_DETAILS::request& req, COMMAND_RPC_GET_POS_MINING_DETAILS::response& res, connection_context& cntx)
  {
    if (!m_p2p.get_connections_count())
    {
      res.status = API_RETURN_CODE_DISCONNECTED;
      return true;
    }
    res.pos_mining_allowed = m_core.get_blockchain_storage().is_pos_allowed();
    if (!res.pos_mining_allowed)
    {
      res.status = API_RETURN_CODE_NOT_FOUND;
      return true;
    }

    res.pos_sequence_factor_is_good = true;
    uint64_t new_block_expected_height = m_core.get_blockchain_storage().get_top_block_height() + 1;
    size_t new_block_expected_sequence_factor = m_core.get_blockchain_storage().get_current_sequence_factor(true);
    if (new_block_expected_height > BLOCKCHAIN_HEIGHT_FOR_POS_STRICT_SEQUENCE_LIMITATION && new_block_expected_sequence_factor > BLOCK_POS_STRICT_SEQUENCE_LIMIT)
      res.pos_sequence_factor_is_good = false;

    res.pos_basic_difficulty = m_core.get_blockchain_storage().get_next_diff_conditional(true).convert_to<std::string>();
    m_core.get_blockchain_storage().build_stake_modifier(res.sm, blockchain_storage::alt_chain_type(), 0, &res.last_block_hash);// , &res.height);
    
    //TODO: need atomic operation with  build_stake_modifier()
    res.starter_timestamp = m_core.get_blockchain_storage().get_last_timestamps_check_window_median();
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_current_core_tx_expiration_median(const COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request& req, COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response& res, connection_context& cntx)
  {
    res.expiration_median = m_core.get_blockchain_storage().get_tx_expiration_median();
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_rpc_get_blocks_details(const COMMAND_RPC_GET_BLOCKS_DETAILS::request& req, COMMAND_RPC_GET_BLOCKS_DETAILS::response& res, connection_context& cntx)
  {
    m_core.get_blockchain_storage().get_main_blocks_rpc_details(req.height_start, req.count, req.ignore_transactions, res.blocks);
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------  
  bool core_rpc_server::on_get_tx_details(const COMMAND_RPC_GET_TX_DETAILS::request& req, COMMAND_RPC_GET_TX_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    crypto::hash h = null_hash;
    if(!epee::string_tools::hex_to_pod(req.tx_hash, h))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Invalid tx hash given";
      return false;
    }
    if (!m_core.get_blockchain_storage().get_tx_rpc_details(h, res.tx_info, 0, false))
    {
      if (!m_core.get_tx_pool().get_transaction_details(h, res.tx_info))
      {
      error_resp.code = CORE_RPC_ERROR_CODE_NOT_FOUND;
      error_resp.message = "tx is not found";
      return false;
      }

    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_search_by_id(const COMMAND_RPC_SERARCH_BY_ID::request& req, COMMAND_RPC_SERARCH_BY_ID::response& res, connection_context& cntx)
  {
    crypto::hash id = null_hash;
    if (!epee::string_tools::hex_to_pod(req.id, id))
      return false;

    m_core.get_blockchain_storage().search_by_id(id, res.types_found);
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_out_info(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::response& res, connection_context& cntx)
  {
    if (!m_core.get_blockchain_storage().get_global_index_details(req, res))
      res.status = API_RETURN_CODE_NOT_FOUND;
    else
      res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_multisig_info(const COMMAND_RPC_GET_MULTISIG_INFO::request& req, COMMAND_RPC_GET_MULTISIG_INFO::response& res, connection_context& cntx)
  {
    if (!m_core.get_blockchain_storage().get_multisig_id_details(req, res))
      res.status = API_RETURN_CODE_NOT_FOUND;
    else
      res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_pool_txs_details(const COMMAND_RPC_GET_POOL_TXS_DETAILS::request& req, COMMAND_RPC_GET_POOL_TXS_DETAILS::response& res, connection_context& cntx)
  {
    if (!req.ids.size())
    {
      m_core.get_tx_pool().get_all_transactions_details(res.txs);
    }
    else 
    {
      m_core.get_tx_pool().get_transactions_details(req.ids, res.txs);
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_pool_txs_brief_details(const COMMAND_RPC_GET_POOL_TXS_BRIEF_DETAILS::request& req, COMMAND_RPC_GET_POOL_TXS_BRIEF_DETAILS::response& res, connection_context& cntx)
  {
    if (!req.ids.size())
    {
      m_core.get_tx_pool().get_all_transactions_brief_details(res.txs);
    }
    else
    {
      m_core.get_tx_pool().get_transactions_brief_details(req.ids, res.txs);
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_all_pool_tx_list(const COMMAND_RPC_GET_ALL_POOL_TX_LIST::request& req, COMMAND_RPC_GET_ALL_POOL_TX_LIST::response& res, connection_context& cntx)
  {
    m_core.get_tx_pool().get_all_transactions_list(res.ids);
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_pool_info(const COMMAND_RPC_GET_POOL_INFO::request& req, COMMAND_RPC_GET_POOL_INFO::response& res, connection_context& cntx)
  {
    std::list<currency::extra_alias_entry> al_list;
    m_core.get_tx_pool().get_aliases_from_tx_pool(al_list);
    for (const auto& a : al_list)
    {
      res.aliases_que.push_back(alias_info_to_rpc_alias_info(a));
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_votes(const COMMAND_RPC_GET_VOTES::request& req, COMMAND_RPC_GET_VOTES::response& res, connection_context& cntx)
  {
    if (!m_core.get_blockchain_storage().get_pos_votes(req.h_start, req.h_end, res.votes))
    {
      res.status = API_RETURN_CODE_INTERNAL_ERROR;
      res.error_code = "Internal error";
      return true;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_asset_info(const COMMAND_RPC_GET_ASSET_INFO::request& req, COMMAND_RPC_GET_ASSET_INFO::response& res, connection_context& cntx)
  {
    if (!m_core.get_blockchain_storage().get_asset_info(req.asset_id, res.asset_descriptor))
    {
      res.status = API_RETURN_CODE_NOT_FOUND;
      return true;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_assets_list(const COMMAND_RPC_GET_ASSETS_LIST::request& req, COMMAND_RPC_GET_ASSETS_LIST::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    if (!m_core.get_blockchain_storage().get_assets(req.offset, req.count, res.assets))
    {
      res.status = API_RETURN_CODE_NOT_FOUND;
      return true;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_main_block_details(const COMMAND_RPC_GET_BLOCK_DETAILS::request& req, COMMAND_RPC_GET_BLOCK_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if (!m_core.get_blockchain_storage().get_main_block_rpc_details(req.id, res.block_details))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_NOT_FOUND;
      error_resp.message = "the requested block has not been found";
      return false;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_alt_block_details(const COMMAND_RPC_GET_BLOCK_DETAILS::request& req, COMMAND_RPC_GET_BLOCK_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if (!m_core.get_blockchain_storage().get_alt_block_rpc_details(req.id, res.block_details))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_NOT_FOUND;
      error_resp.message = "the requested block has not been found";
      return false;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_alt_blocks_details(const COMMAND_RPC_GET_ALT_BLOCKS_DETAILS::request& req, COMMAND_RPC_GET_ALT_BLOCKS_DETAILS::response& res, connection_context& cntx)
  {
    m_core.get_blockchain_storage().get_alt_blocks_rpc_details(req.offset, req.count, res.blocks);
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();

    std::string tx_blob;
    if(!string_tools::parse_hexstr_to_binbuff(req.tx_as_hex, tx_blob))
    {
      LOG_PRINT_L0("[on_send_raw_tx]: Failed to parse tx from hexbuff: " << req.tx_as_hex);
      res.status = "Failed";
      return true;
    }

    if (!m_p2p.get_payload_object().get_synchronized_connections_count())
    {
      LOG_PRINT_L0("[on_send_raw_tx]: Failed to send, daemon not connected to net");
      res.status = API_RETURN_CODE_DISCONNECTED;
      return true;
    }

    currency_connection_context fake_context = AUTO_VAL_INIT(fake_context);
    tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    if(!m_core.handle_incoming_tx(tx_blob, tvc, false))
    {
      LOG_PRINT_L0("[on_send_raw_tx]: Failed to process tx");
      res.status = "Failed";
      return true;
    }

    if(tvc.m_verification_failed)
    {
      LOG_PRINT_L0("[on_send_raw_tx]: tx verification failed");
      res.status = "Failed";
      return true;
    }

    if(!tvc.m_should_be_relayed)
    {
      LOG_PRINT_L0("[on_send_raw_tx]: tx accepted, but not relayed");
      res.status = "Not relayed";
      return true;
    }


    NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request r;
    r.txs.push_back(tx_blob);
    m_core.get_protocol()->relay_transactions(r, fake_context);
    //TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
    res.status = API_RETURN_CODE_OK;
    return true;
  }
	//------------------------------------------------------------------------------------------------------------------------------
	bool core_rpc_server::on_force_relaey_raw_txs(const COMMAND_RPC_FORCE_RELAY_RAW_TXS::request& req, COMMAND_RPC_FORCE_RELAY_RAW_TXS::response& res, connection_context& cntx)
	{
		CHECK_CORE_READY();
		NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request r = AUTO_VAL_INIT(r);

		for (const auto& t : req.txs_as_hex)
		{
			std::string tx_blob;
			if (!string_tools::parse_hexstr_to_binbuff(t, tx_blob))
			{
				LOG_PRINT_L0("[on_send_raw_tx]: Failed to parse tx from hexbuff: " << t);
				res.status = "Failed";
				return true;
			}
			r.txs.push_back(tx_blob);
		}

		currency_connection_context fake_context = AUTO_VAL_INIT(fake_context);
		bool call_res = m_core.get_protocol()->relay_transactions(r, fake_context);
		if (call_res)
			res.status = API_RETURN_CODE_OK;
		return call_res;
	}
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    account_public_address adr;
    if(!get_account_address_from_str(adr, req.miner_address))
    {
      res.status = "Failed, wrong address";
      return true;
    }

    if(!m_core.get_miner().start(adr, static_cast<size_t>(req.threads_count)))
    {
      res.status = "Failed, mining not started";
      return true;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    if(!m_core.get_miner().stop())
    {
      res.status = "Failed, mining not stopped";
      return true;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.count = m_core.get_current_blockchain_size();
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy";
      return false;
    }
    if(req.size() != 1)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Wrong parameters, expected height";
      return false;
    }
    uint64_t h = req[0];
    if(m_core.get_current_blockchain_size() <= h)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = std::string("To big height: ") + std::to_string(h) + ", current blockchain size = " +  std::to_string(m_core.get_current_blockchain_size());
    }
    res = string_tools::pod_to_hex(m_core.get_block_id_by_height(h));
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy";
      return false;
    }

    if(req.extra_text.size() > 255)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_RESERVE_SIZE;
      error_resp.message = "Extra text size is to big, maximum 255";
      return false;
    }

    currency::account_public_address miner_address = AUTO_VAL_INIT(miner_address);
    if(!req.wallet_address.size() || !currency::get_account_address_from_str(miner_address, req.wallet_address))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_WALLET_ADDRESS;
      error_resp.message = "Failed to parse wallet address";
      return false;
    }
    currency::account_public_address stakeholder_address = AUTO_VAL_INIT(stakeholder_address);
    if(req.pos_block && (!req.stakeholder_address.size() || !currency::get_account_address_from_str(stakeholder_address, req.stakeholder_address)))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_WALLET_ADDRESS;
      error_resp.message = "Failed to parse stakeholder address";
      return false;
    }


    create_block_template_params params = AUTO_VAL_INIT(params);
    params.miner_address = miner_address;
    params.stakeholder_address = stakeholder_address;
    params.ex_nonce = req.extra_text;
    params.pos = req.pos_block;
    params.pe = req.pe;
    //params.pe.keyimage key image will be set in the wallet
    //params.pe.wallet_index is not included in serialization map, TODO: refactoring here
    params.pcustom_fill_block_template_func = nullptr;
    if (req.explicit_transaction.size())
    {
      transaction tx = AUTO_VAL_INIT(tx);
      if (!parse_and_validate_tx_from_blob(req.explicit_transaction, tx))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
        error_resp.message = "Wrong parameters: explicit_transaction is invalid";
        LOG_ERROR("Failed to parse explicit_transaction blob");
        return false;
      }
      params.explicit_txs.push_back(tx);
    }

    create_block_template_response resp = AUTO_VAL_INIT(resp);
    if (!m_core.get_block_template(params, resp))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to create block template");
      return false;
    }

    res.difficulty = resp.diffic.convert_to<std::string>();
    blobdata block_blob = t_serializable_object_to_blob(resp.b);
    res.blocktemplate_blob = string_tools::buff_to_hex_nodelimer(block_blob);
    res.prev_hash = string_tools::pod_to_hex(resp.b.prev_id);
    res.miner_tx_tgc = resp.miner_tx_tgc;
    res.height = resp.height;
    res.block_reward_without_fee = resp.block_reward_without_fee;
    res.block_reward = resp.block_reward;
    res.txs_fee = resp.txs_fee;
    //calculate epoch seed
    res.seed = currency::ethash_epoch_to_seed(currency::ethash_height_to_epoch(res.height));

    res.status = API_RETURN_CODE_OK;
    LOG_PRINT_L1("COMMAND_RPC_GETBLOCKTEMPLATE OK, response block: " << ENDL << currency::obj_to_json_str(resp.b));
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    CHECK_CORE_READY();
    if(req.size()!=1)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Wrong param";
      return false;
    }
    blobdata blockblob;
    if(!string_tools::parse_hexstr_to_binbuff(req[0], blockblob))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong block blob";
      return false;
    }
    
    block b = AUTO_VAL_INIT(b);
    if(!parse_and_validate_block_from_blob(blockblob, b))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong block blob";
      return false;
    }

    block_verification_context bvc = AUTO_VAL_INIT(bvc);
    if(!m_core.handle_block_found(b, &bvc))
    {
      if (bvc.m_added_to_altchain)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_BLOCK_ADDED_AS_ALTERNATIVE;
        error_resp.message = "Block added as alternative";
        return false;
      }
      error_resp.code = CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED;
      error_resp.message = "Block not accepted";
      return false;
    }

    res.status = "OK";
    return true;
  }  
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_submitblock2(const COMMAND_RPC_SUBMITBLOCK2::request& req, COMMAND_RPC_SUBMITBLOCK2::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    CHECK_CORE_READY();


    block b = AUTO_VAL_INIT(b);
    if (!parse_and_validate_block_from_blob(req.b, b))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong block blob";
      return false;
    }

    block_verification_context bvc = AUTO_VAL_INIT(bvc);
    for (const auto& txblob : req.explicit_txs)
    {

      crypto::hash tx_hash = AUTO_VAL_INIT(tx_hash);
      transaction tx = AUTO_VAL_INIT(tx);
      if (!parse_and_validate_tx_from_blob(txblob.blob, tx, tx_hash))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
        error_resp.message = "Wrong explicit tx blob";
        return false;
      }
      bvc.m_onboard_transactions[tx_hash] = tx;
    }


    if (!m_core.handle_block_found(b, &bvc))
    {
      if (bvc.m_added_to_altchain)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_BLOCK_ADDED_AS_ALTERNATIVE;
        error_resp.message = "Block added as alternative";
        return false;
      }
      error_resp.code = CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED;
      error_resp.message = "Block not accepted";
      return false;
    }

    res.status = "OK";
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  uint64_t core_rpc_server::get_block_reward(const block& blk)
  {
    uint64_t reward = 0;
    BOOST_FOREACH(const auto& out, blk.miner_tx.vout)
    {
      VARIANT_SWITCH_BEGIN(out);
      VARIANT_CASE_CONST(tx_out_bare, out)
        reward += out.amount;
      VARIANT_CASE_CONST(tx_out_zarcanum, out)
        //@#@      
      VARIANT_SWITCH_END();
    }
    return reward;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::fill_block_header_response(const block& blk, bool orphan_status, block_header_response& response)
  {
    response.major_version = blk.major_version;
    response.minor_version = blk.minor_version;
    response.timestamp = blk.timestamp;
    response.prev_hash = string_tools::pod_to_hex(blk.prev_id);
    response.nonce = blk.nonce;
    response.orphan_status = orphan_status;
    response.height = get_block_height(blk);
    response.depth = m_core.get_current_blockchain_size() - response.height - 1;
    response.hash = string_tools::pod_to_hex(get_block_hash(blk));
    response.difficulty = m_core.get_blockchain_storage().block_difficulty(response.height).convert_to<std::string>();
    response.reward = get_block_reward(blk);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }
    block last_block = AUTO_VAL_INIT(last_block);
    bool have_last_block_hash = m_core.get_blockchain_storage().get_top_block(last_block);
    if (!have_last_block_hash)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get last block hash.";
      return false;
    }
    bool response_filled = fill_block_header_response(last_block, false, res.block_header);
    if (!response_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res, epee::json_rpc::error& error_resp, connection_context& cntx){
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }
    crypto::hash block_hash;
    bool hash_parsed = parse_hash256(req.hash, block_hash);
    if(!hash_parsed)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Failed to parse hex representation of block hash. Hex = " + req.hash + '.';
      return false;
    }
    block blk;
    bool have_block = m_core.get_block_by_hash(block_hash, blk);
    if (!have_block)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get block by hash. Hash = " + req.hash + '.';
      return false;
    }
    if (blk.miner_tx.vin.front().type() != typeid(txin_gen))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: coinbase transaction in the block has the wrong type";
      return false;
    }
    //uint64_t block_height = boost::get<txin_gen>(blk.miner_tx.vin.front()).height;
    bool response_filled = fill_block_header_response(blk, false, res.block_header);
    if (!response_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res, epee::json_rpc::error& error_resp, connection_context& cntx){
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }
    if(m_core.get_current_blockchain_size() <= req.height)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = std::string("To big height: ") + std::to_string(req.height) + ", current blockchain size = " +  std::to_string(m_core.get_current_blockchain_size());
      return false;
    }
    block blk = AUTO_VAL_INIT(blk);
    m_core.get_blockchain_storage().get_block_by_height(req.height, blk);
    
    bool response_filled = fill_block_header_response(blk, false, res.block_header);
    if (!response_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_alias_details(const COMMAND_RPC_GET_ALIAS_DETAILS::request& req, COMMAND_RPC_GET_ALIAS_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return true;
    }
    extra_alias_entry_base aib = AUTO_VAL_INIT(aib);
    if(!validate_alias_name(req.alias))
    {      
      res.status = "Alias have wrong name";
      return true;
    }
    if(!m_core.get_blockchain_storage().get_alias_info(req.alias, aib))
    {
      res.status = API_RETURN_CODE_NOT_FOUND;
      return true;
    }
    res.alias_details.address = currency::get_account_address_as_str(aib.m_address);
    res.alias_details.comment = aib.m_text_comment;
    if (aib.m_view_key.size())
      res.alias_details.tracking_key = string_tools::pod_to_hex(aib.m_view_key.back());

    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------

  bool core_rpc_server::on_get_all_aliases(const COMMAND_RPC_GET_ALL_ALIASES::request& req, COMMAND_RPC_GET_ALL_ALIASES::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }

    m_core.get_blockchain_storage().enumerate_aliases([&](uint64_t i, const std::string& alias, const extra_alias_entry_base& alias_entry_base) -> bool
    {
      res.aliases.push_back(alias_rpc_details());
      alias_info_to_rpc_alias_info(alias, alias_entry_base, res.aliases.back());
      return true;
    });

    res.status = API_RETURN_CODE_OK;
    return true;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_aliases(const COMMAND_RPC_GET_ALIASES::request& req, COMMAND_RPC_GET_ALIASES::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }

    m_core.get_blockchain_storage().get_aliases([&res](const std::string& alias, const currency::extra_alias_entry_base& ai){
      res.aliases.push_back(alias_rpc_details());
      alias_info_to_rpc_alias_info(alias, ai, res.aliases.back());
    }, req.offset, req.count);

    res.status = API_RETURN_CODE_OK;
    return true;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_alias_reward(const COMMAND_RPC_GET_ALIAS_REWARD::request& req, COMMAND_RPC_GET_ALIAS_REWARD::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    res.reward = m_core.get_blockchain_storage().get_alias_coast(req.alias);
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_est_height_from_date(const COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request& req, COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response& res, connection_context& cntx)
  {
    bool r = m_core.get_blockchain_storage().get_est_height_from_date(req.timestamp, res.h);

    if (r)
      res.status = API_RETURN_CODE_OK;
    else
      res.status = API_RETURN_CODE_NOT_FOUND;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_aliases_by_address(const COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& req, COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    account_public_address addr = AUTO_VAL_INIT(addr);
    if (!get_account_address_from_str(addr, req))
    {
      res.status = API_RETURN_CODE_FAIL;
      return true;
    }
    //res.alias = m_core.get_blockchain_storage().get_alias_by_address(addr);
    COMMAND_RPC_GET_ALIAS_DETAILS::request req2 = AUTO_VAL_INIT(req2);
    COMMAND_RPC_GET_ALIAS_DETAILS::response res2 = AUTO_VAL_INIT(res2);

    std::set<std::string> aliases = m_core.get_blockchain_storage().get_aliases_by_address(addr);

    if (!aliases.size())
    {
      res.status = API_RETURN_CODE_NOT_FOUND;
      return true;
    }

    for (auto it = aliases.begin(); it != aliases.end(); it++)
    {
      req2.alias = *it;
      bool r = this->on_get_alias_details(req2, res2, error_resp, cntx);
      if (!r || res2.status != API_RETURN_CODE_OK)
      {
        res.status = API_RETURN_CODE_FAIL;
        return true;
      }
      res.alias_info_list.push_back(alias_rpc_details());
      res.alias_info_list.back().details = res2.alias_details;
      res.alias_info_list.back().alias = req2.alias;

    }
    res.status = API_RETURN_CODE_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_reset_transaction_pool(const COMMAND_RPC_RESET_TX_POOL::request& req, COMMAND_RPC_RESET_TX_POOL::response& res, connection_context& cntx)
  {
    m_core.get_tx_pool().purge_transactions();
    res.status = API_RETURN_CODE_OK;
    return true;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_remove_tx_from_pool(const COMMAND_RPC_REMOVE_TX_FROM_POOL::request& req, COMMAND_RPC_REMOVE_TX_FROM_POOL::response& res, connection_context& cntx)
  {
    for (const auto& tx_id_str : req.tx_to_remove)
    {
      crypto::hash tx_id = epee::transform_str_to_t_pod<crypto::hash>(tx_id_str);
      currency::transaction tx; size_t dummy1 = 0; uint64_t dummy2 = 0;
      m_core.get_tx_pool().take_tx(tx_id, tx, dummy1, dummy2);
    }

    res.status = API_RETURN_CODE_OK;
    return true;
  }
}


#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL
