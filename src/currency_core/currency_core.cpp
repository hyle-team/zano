// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
using namespace epee;

#include <boost/foreach.hpp>
#include <unordered_set>
#include "currency_core.h"
#include "common/command_line.h"
#include "common/util.h"
#include "warnings.h"
#include "crypto/crypto.h"
#include "currency_core/currency_config.h"
#include "currency_format_utils.h"
#include "misc_language.h"
#include "string_coding.h"
#include "tx_semantic_validation.h"

#define MINIMUM_REQUIRED_FREE_SPACE_BYTES (1024 * 1024 * 100)

DISABLE_VS_WARNINGS(4355)
#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "core"
ENABLE_CHANNEL_BY_DEFAULT("core");
namespace currency
{

  //-----------------------------------------------------------------------------------------------
  core::core(i_currency_protocol* pprotocol)
    : m_mempool(m_blockchain_storage, pprotocol)
    , m_blockchain_storage(m_mempool)
    , m_miner(this, m_blockchain_storage)
    , m_miner_address(boost::value_initialized<account_public_address>())
    , m_starter_message_showed(false)
    , m_critical_error_handler(nullptr)
    , m_stop_after_height(0)
  {
    set_currency_protocol(pprotocol);
  }
  //-----------------------------------------------------------------------------------
  void core::set_currency_protocol(i_currency_protocol* pprotocol)
  {
    if(pprotocol)
      m_pprotocol = pprotocol;
    else
      m_pprotocol = &m_protocol_stub;

    m_mempool.set_protocol(m_pprotocol);
  }
  //-----------------------------------------------------------------------------------
  void core::set_critical_error_handler(i_critical_error_handler* handler)
  {
    m_critical_error_handler = handler;
  }
  //-----------------------------------------------------------------------------------
  bool core::set_checkpoints(checkpoints&& chk_pts)
  {
    return m_blockchain_storage.set_checkpoints(std::move(chk_pts));
  }
  //-----------------------------------------------------------------------------------
  void core::init_options(boost::program_options::options_description& desc)
  {
    blockchain_storage::init_options(desc);
  }
  //-----------------------------------------------------------------------------------------------
  std::string core::get_config_folder()
  {
    return m_config_folder;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_config_folder = command_line::get_arg(vm, command_line::arg_data_dir);
    m_stop_after_height = 0;
    if(command_line::has_arg(vm, command_line::arg_stop_after_height))
      m_stop_after_height = static_cast<uint64_t>(command_line::get_arg(vm, command_line::arg_stop_after_height));

    if (m_stop_after_height != 0)
    {
      LOG_PRINT_YELLOW("Daemon will STOP after block " << m_stop_after_height, LOG_LEVEL_0);
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t core::get_current_blockchain_size() const
  {
    return m_blockchain_storage.get_current_blockchain_size();
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t core::get_current_tx_version() const
  {
    return get_tx_version(m_blockchain_storage.get_current_blockchain_size(), m_blockchain_storage.get_core_runtime_config().hard_forks);
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t core::get_top_block_height() const
  {
    return m_blockchain_storage.get_top_block_height();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blockchain_top(uint64_t& height, crypto::hash& top_id) const
  {
    top_id = m_blockchain_storage.get_top_block_id(height);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs)
  {
    return m_blockchain_storage.get_blocks(start_offset, count, blocks, txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks)
  {
    return m_blockchain_storage.get_blocks(start_offset, count, blocks);
  }  //-----------------------------------------------------------------------------------------------
  bool core::get_transactions(const std::vector<crypto::hash>& txs_ids, std::list<transaction>& txs, std::list<crypto::hash>& missed_txs)
  {
    return m_blockchain_storage.get_transactions(txs_ids, txs, missed_txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_transaction(const crypto::hash &h, transaction &tx)
  {
    std::vector<crypto::hash> ids;
    ids.push_back(h);
    std::list<transaction> ltx;
    std::list<crypto::hash> missing;
    if (m_blockchain_storage.get_transactions(ids, ltx, missing))
    {
      if (ltx.size() > 0)
      {
        tx = *ltx.begin();
        return true;
      }
    }

    return false;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_alternative_blocks(std::list<block>& blocks)
  {
    return m_blockchain_storage.get_alternative_blocks(blocks);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_alternative_blocks_count()
  {
    return m_blockchain_storage.get_alternative_blocks_count();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::init(const boost::program_options::variables_map& vm)
  {
    bool r = handle_command_line(vm);

    uint64_t available_space = 0;
    CHECK_AND_ASSERT_MES(!check_if_free_space_critically_low(&available_space), false, "free space in data folder is critically low: " << std::fixed << available_space / (1024 * 1024) << " MB");

    r = m_mempool.init(m_config_folder, vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize memory pool");

    r = m_blockchain_storage.init(m_config_folder, vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize blockchain storage");

    m_mempool.remove_incompatible_txs();

    r = m_miner.init(vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize miner");

    //check if tx_pool module synchronized with blockchaine storage
//     if (m_blockchain_storage.get_top_block_id() != m_mempool.get_last_core_hash())
//     {
//       m_mempool.clear();
//       LOG_PRINT_MAGENTA("Tx pool had been reset due to missmatch top block id", LOG_LEVEL_0);
//     }

    return load_state_data();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::set_genesis_block(const block& b)
  {
    return m_blockchain_storage.reset_and_set_genesis_block(b);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::load_state_data()
  {
    // may be some code later
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::deinit()
  {
    //m_mempool.set_last_core_hash(m_blockchain_storage.get_top_block_id());

    m_miner.stop();
    m_miner.deinit();
    m_mempool.deinit();
    m_blockchain_storage.deinit();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx(const transaction& tx, tx_verification_context& tvc, bool kept_by_block, const crypto::hash& tx_hash_ /* = null_hash */)
  {
    TIME_MEASURE_START_MS(wait_lock_time);
    CRITICAL_REGION_LOCAL(m_incoming_tx_lock);
    TIME_MEASURE_FINISH_MS(wait_lock_time);

    crypto::hash tx_hash = tx_hash_;
    if (tx_hash == null_hash)
      tx_hash = get_transaction_hash(tx);

    TIME_MEASURE_START_MS(add_new_tx_time);
    bool r = add_new_tx(tx, tx_hash, get_object_blobsize(tx), tvc, kept_by_block);
    TIME_MEASURE_FINISH_MS(add_new_tx_time);

    if(tvc.m_verification_failed)
    {LOG_PRINT_RED_L0("Transaction verification failed: " << tx_hash);}
    else if(tvc.m_verification_impossible)
    {LOG_PRINT_RED_L0("Transaction verification impossible: " << tx_hash);}

    if (tvc.m_added_to_pool)
    {
      LOG_PRINT_L2("incoming tx " << tx_hash << " was added to the pool");
    }
    LOG_PRINT_L2("[CORE HANDLE_INCOMING_TX1]: timing " << wait_lock_time
      << "/" << add_new_tx_time);
    return r;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx(const blobdata& tx_blob, tx_verification_context& tvc, bool kept_by_block)
  {
    CHECK_AND_ASSERT_MES(!kept_by_block, false, "Transaction associated with block came throw handle_incoming_tx!(not allowed anymore)");

    tvc = boost::value_initialized<tx_verification_context>();
    //want to process all transactions sequentially
    TIME_MEASURE_START_MS(wait_lock_time);
    CRITICAL_REGION_LOCAL(m_incoming_tx_lock);
    TIME_MEASURE_FINISH_MS(wait_lock_time);

    if(tx_blob.size() > CURRENCY_MAX_TRANSACTION_BLOB_SIZE)
    {
      LOG_PRINT_L0("WRONG TRANSACTION BLOB, too big size " << tx_blob.size() << ", rejected");
      tvc.m_verification_failed = true;
      return false;
    }

    crypto::hash tx_hash = null_hash;
    transaction tx;
    TIME_MEASURE_START_MS(parse_tx_time);
    if(!parse_tx_from_blob(tx, tx_hash, tx_blob))
    {
      LOG_PRINT_L0("WRONG TRANSACTION BLOB, Failed to parse, rejected");
      tvc.m_verification_failed = true;
      return false;
    }
    TIME_MEASURE_FINISH_MS(parse_tx_time);
    
    TIME_MEASURE_START_MS(check_tx_semantic_time);
    if(!validate_tx_semantic(tx, tx_blob.size()))
    {
      LOG_PRINT_L0("WRONG TRANSACTION SEMANTICS, Failed to check tx " << tx_hash << " semantic, rejected");
      tvc.m_verification_failed = true;
      return false;
    }
    TIME_MEASURE_FINISH_MS(check_tx_semantic_time);

    bool r = handle_incoming_tx(tx, tvc, kept_by_block, tx_hash);
    LOG_PRINT_L2("[CORE HANDLE_INCOMING_TX2]: timing " << wait_lock_time
      << "/" << parse_tx_time
      << "/" << check_tx_semantic_time);
    return r;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_stat_info(const core_stat_info::params& pr, core_stat_info& st_inf)
  {
    st_inf.mining_speed = m_miner.get_speed();
    st_inf.alternative_blocks = m_blockchain_storage.get_alternative_blocks_count();
    st_inf.blockchain_height = m_blockchain_storage.get_current_blockchain_size();
    st_inf.tx_pool_size = m_mempool.get_transactions_count();
    st_inf.top_block_id_str = epee::string_tools::pod_to_hex(m_blockchain_storage.get_top_block_id());
    
    std::list<block> blocks;
    m_blockchain_storage.get_blocks(m_blockchain_storage.get_current_blockchain_size() - pr.chain_len, static_cast<size_t>(pr.chain_len), blocks);
    for (auto& b : blocks)
    {
      st_inf.main_chain_blocks.push_back(chain_entry());
      st_inf.main_chain_blocks.back().h = get_block_height(b);
      st_inf.main_chain_blocks.back().id = epee::string_tools::pod_to_hex(get_block_hash(b));
    }
    blocks.clear();
    m_blockchain_storage.get_alternative_blocks(blocks);
    for (auto& b : blocks)
    {
      st_inf.alt_chain_blocks.push_back(chain_entry());
      st_inf.alt_chain_blocks.back().h = get_block_height(b);
      st_inf.alt_chain_blocks.back().id = epee::string_tools::pod_to_hex(get_block_hash(b));
    }
    blocks.clear();

    auto ch_locked_proxy_container = epee::log_space::get_channels_errors_stat_container();
    epee::log_space::channels_err_stat_container_type& ch_container_std = *ch_locked_proxy_container;
    for (const auto& entry : ch_container_std)
    {
      st_inf.errors_stat.push_back(error_stat_entry());
      st_inf.errors_stat.back().channel = entry.first;
      st_inf.errors_stat.back().err_count = entry.second;
    }
    st_inf.epic_failure_happend = m_blockchain_storage.get_performnce_data().epic_failure_happend;
    epee::log_space::log_singletone::get_journal_items(st_inf.errors_journal, pr.logs_journal_len);
    //check deadlocks
    auto dl_list = epee::deadlock_guard_singleton::get_deadlock_journal();
    if (dl_list.size())
    {
      st_inf.epic_failure_happend = true;
      st_inf.errors_journal.insert(st_inf.errors_journal.end(), dl_list.begin(), dl_list.end());
    }

    return true;
  }




  //-----------------------------------------------------------------------------------------------
  bool core::add_new_tx(const transaction& tx, tx_verification_context& tvc, bool kept_by_block)
  {
    crypto::hash tx_hash = null_hash;
    uint64_t blob_size = 0;
    get_transaction_hash(tx, tx_hash, blob_size);
    return add_new_tx(tx, tx_hash, blob_size, tvc, kept_by_block);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_blockchain_total_transactions()
  {
    return m_blockchain_storage.get_total_transactions();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys)
  {
    return m_blockchain_storage.get_outs(amount, pkeys);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_tx(const transaction& tx, const crypto::hash& tx_hash, size_t blob_size, tx_verification_context& tvc, bool kept_by_block)
  {
    if(m_mempool.have_tx(tx_hash))
    {
      LOG_PRINT_L3("add_new_tx: already have tx " << tx_hash << " in the pool");
      return true;
    }

    if(m_blockchain_storage.have_tx(tx_hash))
    {
      LOG_PRINT_L3("add_new_tx: already have tx " << tx_hash << " in the blockchain");
      return true;
    }

    return m_mempool.add_tx(tx, tx_hash, blob_size, tvc, kept_by_block);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_template(block& b, const account_public_address& adr, const account_public_address& stakeholder_address, wide_difficulty_type& diffic, uint64_t& height, const blobdata& ex_nonce, bool pos, const pos_entry& pe)
  {
    return m_blockchain_storage.create_block_template(adr, stakeholder_address, ex_nonce, pos, pe, nullptr, b, diffic, height);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_template(const create_block_template_params& params, create_block_template_response& resp)
  {
    return m_blockchain_storage.create_block_template(params, resp);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp) const 
  {
    return m_blockchain_storage.find_blockchain_supplement(qblock_ids, resp);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count) const 
  {
    return m_blockchain_storage.find_blockchain_supplement(qblock_ids, blocks, total_height, start_height, max_count);
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain(uint64_t start_index, uint64_t end_index)
  {
    m_blockchain_storage.print_blockchain(start_index, end_index);
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain_index()
  {
    m_blockchain_storage.print_blockchain_index();
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain_outs(const std::string& file)
  {
    m_blockchain_storage.print_blockchain_outs(file);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)
  {
    return m_blockchain_storage.get_random_outs_for_amounts(req, res);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs)
  {
    return m_blockchain_storage.get_tx_outputs_gindexs(tx_id, indexs);
  }
  //-----------------------------------------------------------------------------------------------
  void core::pause_mine()
  {
    m_miner.pause();
  }
  //-----------------------------------------------------------------------------------------------
  void core::resume_mine()
  {
    m_miner.resume();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_block_found(const block& b, block_verification_context* p_verification_result, bool need_update_miner_block_template)
  {
    try
    {
      TIME_MEASURE_START_MS(time_total_ms);
      block_verification_context bvc = boost::value_initialized<block_verification_context>();
      if (!p_verification_result)
        p_verification_result = &bvc;

      m_miner.pause();
      misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler([this]()
      {
        m_miner.resume();
      });

      TIME_MEASURE_START_MS(time_add_new_block_ms);
      m_blockchain_storage.add_new_block(b, *p_verification_result);
      TIME_MEASURE_FINISH_MS(time_add_new_block_ms);


      //anyway - update miner template
      TIME_MEASURE_START_MS(time_update_block_template_ms);
      if (need_update_miner_block_template)
        update_miner_block_template();
      TIME_MEASURE_FINISH_MS(time_update_block_template_ms);

      uint64_t time_pack_txs_ms = 0, time_relay_ms = 0;

      if (p_verification_result->m_verification_failed || !p_verification_result->m_added_to_main_chain)
      {
        LOG_PRINT2("failed_mined_blocks.log", "verification_failed: " << p_verification_result->m_verification_failed << ", added_to_main_chain: " << p_verification_result->m_added_to_main_chain << ENDL <<
          currency::obj_to_json_str(b), LOG_LEVEL_0);
      }
      CHECK_AND_ASSERT_MES(!p_verification_result->m_verification_failed, false, "mined block has failed to pass verification: id " << get_block_hash(b) << " @ height " << get_block_height(b) << " prev_id: " << b.prev_id);

      if(p_verification_result->m_added_to_main_chain)
      {
        time_pack_txs_ms = epee::misc_utils::get_tick_count();
        currency_connection_context exclude_context = boost::value_initialized<currency_connection_context>();
        NOTIFY_NEW_BLOCK::request arg = AUTO_VAL_INIT(arg);
        arg.hop = 0;
        arg.current_blockchain_height = m_blockchain_storage.get_current_blockchain_size();
        std::list<crypto::hash> missed_txs;
        std::list<transaction> txs;
        m_blockchain_storage.get_transactions(b.tx_hashes, txs, missed_txs);
        if(missed_txs.size() && m_blockchain_storage.get_block_id_by_height(get_block_height(b)) != get_block_hash(b))
        {
          LOG_PRINT_L0("Block found (id " << get_block_hash(b) << " @ height " << get_block_height(b) << ") but it seems that reorganize just happened after that, do not relay this block");
          return true;
        }
        if (txs.size() != b.tx_hashes.size() || missed_txs.size() != 0)
        {
          std::stringstream ss;
          ss << "txs:" << ENDL;
          for (auto& t : txs)
            ss << get_transaction_hash(t) << ENDL;
          ss << "missed txs:" << ENDL;
          for (auto& tx_id : missed_txs)
            ss << tx_id << ENDL;
          LOG_ERROR("can't find some transactions in found block: " << get_block_hash(b) << ", txs.size()=" << txs.size()
            << ", b.tx_hashes.size()=" << b.tx_hashes.size() << ", missed_txs.size()=" << missed_txs.size() << ENDL << ss.str());
          return false;
        }

        block_to_blob(b, arg.b.block);
        //pack transactions
        for(auto& tx : txs)
          arg.b.txs.push_back(t_serializable_object_to_blob(tx));

        TIME_MEASURE_FINISH_MS(time_pack_txs_ms);

        time_relay_ms = epee::misc_utils::get_tick_count();
        m_pprotocol->relay_block(arg, exclude_context);
        TIME_MEASURE_FINISH_MS(time_relay_ms);
      }

      TIME_MEASURE_FINISH_MS(time_total_ms);
      LOG_PRINT_L2("handle_block_found timings (ms): total: " << time_total_ms << ", add new block: " << time_add_new_block_ms << ", update template: " << time_update_block_template_ms << ", pack txs: " << time_pack_txs_ms << ", relay: " << time_relay_ms);

      return p_verification_result->m_added_to_main_chain;
    }
    catch(std::bad_alloc&)
    {
      LOG_ERROR("bad_alloc in core::handle_block_found(), requesting immediate stop...");
      if (m_critical_error_handler)
        m_critical_error_handler->on_immediate_stop_requested();
      return false;
    }
    catch(std::exception& e)
    {
      LOG_ERROR("caught exception in core::handle_block_found(): " << e.what());
      return false;
    }
    catch(...)
    {
      LOG_ERROR("caught unknown exception in core::handle_block_found()");
      return false;
    }
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_block_found(const block& b, block_verification_context* p_verification_result /* = nullptr */)
  {
    return handle_block_found(b, p_verification_result, true);
  }
  //-----------------------------------------------------------------------------------------------
  void core::on_synchronized()
  {
    m_miner.on_synchronized();
  }
  bool core::get_backward_blocks_sizes(uint64_t from_height, std::vector<size_t>& sizes, size_t count)
  {
    return m_blockchain_storage.get_backward_blocks_sizes(from_height, sizes, count);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_block(const block& b, block_verification_context& bvc)
  {
    try
    {
      uint64_t h = m_blockchain_storage.get_top_block_height();
      if (m_stop_after_height != 0 && h >= m_stop_after_height)
      {
        LOG_PRINT_YELLOW("Blockchain top block height is " << h << ", the daemon will now stop as requested", LOG_LEVEL_0);
        if (m_critical_error_handler)
          return m_critical_error_handler->on_immediate_stop_requested();
        return false;
      }

      bool r = m_blockchain_storage.add_new_block(b, bvc);
      if (r && bvc.m_added_to_main_chain)
      {
        uint64_t h = get_block_height(b);
        if (h > 0)
        {
          auto& crc = m_blockchain_storage.get_core_runtime_config();
          size_t hardfork_id_for_prev_block = crc.hard_forks.get_the_most_recent_hardfork_id_for_height(h);
          size_t hardfork_id_for_curr_block = crc.hard_forks.get_the_most_recent_hardfork_id_for_height(h + 1);
          if (hardfork_id_for_prev_block != hardfork_id_for_curr_block)
          {
            LOG_PRINT_GREEN("Hardfork " << hardfork_id_for_curr_block << " has been activated after the block at height " << h, LOG_LEVEL_0);
          }
        }

        if (h == m_stop_after_height)
        {
          LOG_PRINT_YELLOW("Reached block " << h << ", the daemon will now stop as requested", LOG_LEVEL_0);
          if (m_critical_error_handler)
            return m_critical_error_handler->on_immediate_stop_requested();
          return false;
        }
      }
      return r;
    }
    catch(std::bad_alloc&)
    {
      LOG_ERROR("bad_alloc in core::add_new_block(), requesting immediate stop...");
      if (m_critical_error_handler)
        m_critical_error_handler->on_immediate_stop_requested();
      return false;
    }
    catch(std::exception& e)
    {
      LOG_ERROR("caught exception in core::add_new_block(): " << e.what());
      return false;
    }
    catch(...)
    {
      LOG_ERROR("caught unknown exception in core::add_new_block()");
      return false;
    }
  }
  //-----------------------------------------------------------------------------------------------
  bool core::parse_block(const blobdata& block_blob, block& b, block_verification_context& bvc)
  {
    if (block_blob.size() > get_max_block_size())
    {
      LOG_PRINT_L0("WRONG BLOCK BLOB, too big size " << block_blob.size() << ", rejected");
      bvc.m_verification_failed = true;
      return false;
    }

    b = AUTO_VAL_INIT_T(block);
    if (!parse_and_validate_block_from_blob(block_blob, b))
    {
      LOG_PRINT_L0("Failed to parse and validate new block");
      bvc.m_verification_failed = true;
      return false;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::pre_validate_block(block& b, block_verification_context& bvc, const crypto::hash& id)
  {
    return m_blockchain_storage.pre_validate_relayed_block(b, bvc, id);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_block(const blobdata& block_blob, block_verification_context& bvc, bool update_miner_blocktemplate)
  {
    block  b = AUTO_VAL_INIT(b);
    if (!parse_block(block_blob, b, bvc))
    {
      LOG_PRINT_L0("Failed to parse and validate new block");
      bvc.m_verification_failed = true;
      return false;
    }
    return handle_incoming_block(b, bvc, update_miner_blocktemplate);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_block(const block& b, block_verification_context& bvc, bool update_miner_blocktemplate)
  {
    bool r = add_new_block(b, bvc);
    if (update_miner_blocktemplate && bvc.m_added_to_main_chain)
      update_miner_block_template();
    return r;
  }
  //-----------------------------------------------------------------------------------------------
  crypto::hash core::get_tail_id()
  {
    return m_blockchain_storage.get_top_block_id();
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_pool_transactions_count()
  {
    return m_mempool.get_transactions_count();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::have_block(const crypto::hash& id)
  {
    return m_blockchain_storage.have_block(id);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::parse_tx_from_blob(transaction& tx, crypto::hash& tx_hash, const blobdata& blob)
  {
    return parse_and_validate_tx_from_blob(blob, tx, tx_hash);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transactions(std::list<transaction>& txs)
  {
    return m_mempool.get_transactions(txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_short_chain_history(std::list<crypto::hash>& ids)
  {
    return m_blockchain_storage.get_short_chain_history(ids);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, currency_connection_context& context)const 
  {
    return m_blockchain_storage.handle_get_objects(arg, rsp);
  }
  //-----------------------------------------------------------------------------------------------
  crypto::hash core::get_block_id_by_height(uint64_t height)const 
  {
    return m_blockchain_storage.get_block_id_by_height(height);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_by_hash(const crypto::hash &h, block &blk) {
    return m_blockchain_storage.get_block_by_hash(h, blk);
  }
  //-----------------------------------------------------------------------------------------------
//   void core::get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid) {
//     m_blockchain_storage.get_all_known_block_ids(main, alt, invalid);
//   }
  //-----------------------------------------------------------------------------------------------
  std::string core::print_pool(bool short_format)
  {
    return m_mempool.print_pool(short_format);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::update_miner_block_template()
  {
    notify_blockchain_update_listeners();
    m_miner.on_block_chain_update();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::on_idle()
  {
    if(!m_starter_message_showed)
    {
      LOG_PRINT_L0(ENDL
        << "**********************************************************************" << ENDL 
        << "The daemon will start synchronizing with the network. It may take up to several hours." << ENDL 
        << ENDL
        << "You can adjust verbosity by using command: \"set_log <level>\", where 0 is the least verbose and 3 is the most." << ENDL
        << ENDL
        << "Use \"help\" command to list all available commands." << ENDL
        << ENDL
        << "Note: in case you need to interrupt the process, use \"exit\" command. Otherwise, the current progress might not be saved." << ENDL 
        << "**********************************************************************");
      m_starter_message_showed = true;
    }

    m_prune_alt_blocks_interval.do_call([this](){return m_blockchain_storage.prune_aged_alt_blocks();});
    m_check_free_space_interval.do_call([this](){ check_free_space(); return true; });
    m_miner.on_idle();
    m_mempool.on_idle();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  void core::add_blockchain_update_listener(i_blockchain_update_listener *l)
  {
    CRITICAL_REGION_LOCAL(m_blockchain_update_listeners_lock);
    m_blockchain_update_listeners.push_back(l);
  }
  //-----------------------------------------------------------------------------------------------
  void core::remove_blockchain_update_listener(i_blockchain_update_listener *l)
  {
    CRITICAL_REGION_LOCAL(m_blockchain_update_listeners_lock);
    m_blockchain_update_listeners.erase(std::remove(m_blockchain_update_listeners.begin(), m_blockchain_update_listeners.end(), l), m_blockchain_update_listeners.end());
  }
  //-----------------------------------------------------------------------------------------------
  void core::notify_blockchain_update_listeners()
  {
    CRITICAL_REGION_LOCAL(m_blockchain_update_listeners_lock);
    for(auto l : m_blockchain_update_listeners)
      l->on_blockchain_update();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_if_free_space_critically_low(uint64_t* p_available_space /* = nullptr */)
  {
    namespace fs = boost::filesystem;

    try
    {
      CHECK_AND_ASSERT_MES(tools::create_directories_if_necessary(m_config_folder), false, "create_directories_if_necessary failed: " << m_config_folder);
      std::wstring config_folder_w = epee::string_encoding::utf8_to_wstring(m_config_folder);
      fs::space_info si = fs::space(config_folder_w);
      if (p_available_space != nullptr)
        *p_available_space = si.available;
      return si.available < MINIMUM_REQUIRED_FREE_SPACE_BYTES;
    }
    catch (std::exception& e)
    {
      LOG_ERROR("failed to determine free space: " << e.what());
      return false;
    }
    catch (...)
    {
      LOG_ERROR("failed to determine free space: unknown exception");
      return false;
    }
  }

  void core::check_free_space()
  {
    if (!m_critical_error_handler)
      return;

    uint64_t available_space = 0;
    if (check_if_free_space_critically_low(&available_space))
      m_critical_error_handler->on_critical_low_free_space(available_space, MINIMUM_REQUIRED_FREE_SPACE_BYTES);
  }
  //-----------------------------------------------------------------------------------------------

}
