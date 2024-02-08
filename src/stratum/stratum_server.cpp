// Copyright (c) 2018-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stratum_server.h"
#include "stratum_helpers.h"
#include "net/abstract_tcp_server2.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_core.h"
#include "common/command_line.h"
#include "common/int-util.h"
#include "version.h"
#include "currency_protocol/currency_protocol_handler.h"

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "stratum"
ENABLE_CHANNEL_BY_DEFAULT("stratum");

using namespace stratum;

namespace currency
{

namespace
{
// This username should be used by stratum clients to log in IN CASE stratum server was started with "stratum-miner-address" option.
// Alternatevly, valid and the very same wallet address should be user among all the workers.
#define WORKER_ALLOWED_USERNAME "miner"

#define STRATUM_BIND_IP_DEFAULT "0.0.0.0"
#define STRATUM_THREADS_COUNT_DEFAULT 2
#define STRATUM_BLOCK_TEMPLATE_UPD_PERIOD_DEFAULT 30 // sec
#define STRATUM_TOTAL_HR_PRINT_INTERVAL_S_DEFAULT 60 // sec
#define VDIFF_TARGET_MIN_DEFAULT 100000000ull // = 100 Mh
#define VDIFF_TARGET_MAX_DEFAULT 100000000000ull // = 100 Gh
#define VDIFF_TARGET_TIME_DEFAULT 30 // sec
#define VDIFF_RETARGET_TIME_DEFAULT 240 // sec
#define VDIFF_RETARGET_SHARES_COUNT 12 // enforce retargeting if this many shares are received (high performace in terms of current difficulty)
#define VDIFF_VARIANCE_PERCENT_DEFAULT 25 // %

  const command_line::arg_descriptor<bool>        arg_stratum                ("stratum",                   "Stratum server: enable" );
  const command_line::arg_descriptor<std::string> arg_stratum_bind_ip        ("stratum-bind-ip",           "Stratum server: IP to bind",                STRATUM_BIND_IP_DEFAULT );
  const command_line::arg_descriptor<std::string> arg_stratum_bind_port      ("stratum-bind-port",         "Stratum server: port to listen at",         std::to_string(STRATUM_DEFAULT_PORT) );
  const command_line::arg_descriptor<size_t>      arg_stratum_threads        ("stratum-threads-count",     "Stratum server: number of server threads",  STRATUM_THREADS_COUNT_DEFAULT );
  const command_line::arg_descriptor<std::string> arg_stratum_miner_address = {"stratum-miner-address",     "Stratum server: miner address. All workers"
    " will mine to this address. If not set here, ALL workers should use the very same wallet address as username."
    " If set here - they're allowed to log in with username '" WORKER_ALLOWED_USERNAME "' instead of address."};
  
  const command_line::arg_descriptor<size_t>      arg_stratum_block_template_update_period = {"stratum-template-update-period",
    "Stratum server: if there are no new blocks, update block template this often (sec.)",  STRATUM_BLOCK_TEMPLATE_UPD_PERIOD_DEFAULT };
  const command_line::arg_descriptor<uint64_t>    arg_stratum_hr_print_interval  ("stratum-hr-print-interval", "Stratum server: how often to print hashrate stats (sec.)", STRATUM_TOTAL_HR_PRINT_INTERVAL_S_DEFAULT );
  
  const command_line::arg_descriptor<uint64_t>    arg_stratum_vdiff_target_min  ("stratum-vdiff-target-min",  "Stratum server: minimum worker difficulty",  VDIFF_TARGET_MIN_DEFAULT );
  const command_line::arg_descriptor<uint64_t>    arg_stratum_vdiff_target_max  ("stratum-vdiff-target-max",  "Stratum server: maximum worker difficulty",  VDIFF_TARGET_MAX_DEFAULT );
  const command_line::arg_descriptor<uint64_t>    arg_stratum_vdiff_target_time  ("stratum-vdiff-target-time",  "Stratum server: target time per share (i.e. try to get one share per this many seconds)",  VDIFF_TARGET_TIME_DEFAULT );
  const command_line::arg_descriptor<uint64_t>    arg_stratum_vdiff_retarget_time  ("stratum-vdiff-retarget-time",  "Stratum server: check to see if we should retarget this often (sec.)",  VDIFF_RETARGET_TIME_DEFAULT );
  const command_line::arg_descriptor<uint64_t>    arg_stratum_vdiff_retarget_shares  ("stratum-vdiff-retarget-shares",  "Stratum server: enforce retargeting if got this many shares",  VDIFF_RETARGET_SHARES_COUNT );
  const command_line::arg_descriptor<uint64_t>    arg_stratum_vdiff_variance_percent   ("stratum-vdiff-variance-percent",  "Stratum server: allow average time to very this % from target without retarget",  VDIFF_VARIANCE_PERCENT_DEFAULT );
  const command_line::arg_descriptor<bool>        arg_stratum_always_online  ( "stratum-always-online",                   "Stratum server consider the core being synchronized regardless of online status, useful for debugging with --offline-mode" );

//==============================================================================================================================

  static jsonrpc_id_null_t jsonrpc_id_null; // object of jsonrpc_id_null_t for convenience

// JSON-RPC error codes
#define JSONRPC_ERROR_CODE_DEFAULT          -32000 // -32000 to -32099 : Reserved for implementation-defined server-errors.
#define JSONRPC_ERROR_CODE_PARSE            -32700
#define JSONRPC_ERROR_CODE_METHOD_NOT_FOUND -32601

#define LP_CC_WORKER(         ct, message, log_level)  LOG_PRINT_CC(          ct, "WORKER " << ct.m_worker_name << ": " << message, log_level)
#define LP_CC_WORKER_GREEN(   ct, message, log_level)  LOG_PRINT_CC_GREEN(    ct, "WORKER " << ct.m_worker_name << ": " << message, log_level)
#define LP_CC_WORKER_RED(     ct, message, log_level)  LOG_PRINT_CC_RED(      ct, "WORKER " << ct.m_worker_name << ": " << message, log_level)
#define LP_CC_WORKER_BLUE(    ct, message, log_level)  LOG_PRINT_CC_BLUE(     ct, "WORKER " << ct.m_worker_name << ": " << message, log_level)
#define LP_CC_WORKER_YELLOW(  ct, message, log_level)  LOG_PRINT_CC_YELLOW(   ct, "WORKER " << ct.m_worker_name << ": " << message, log_level)
#define LP_CC_WORKER_CYAN(    ct, message, log_level)  LOG_PRINT_CC_CYAN(     ct, "WORKER " << ct.m_worker_name << ": " << message, log_level)
#define LP_CC_WORKER_MAGENTA( ct, message, log_level)  LOG_PRINT_CC_MAGENTA(  ct, "WORKER " << ct.m_worker_name << ": " << message, log_level)

#define HR_TO_STREAM_IN_MHS_1P(hr) std::fixed << std::setprecision(1) << hr / 1000000.0
#define HR_TO_STREAM_IN_MHS_3P(hr) std::fixed << std::setprecision(3) << hr / 1000000.0

// debug stuff
#define DBG_NETWORK_DIFFICULTY 0 // if non-zero: use this value as net difficulty when checking shares (useful for debugging on testnet, recommended value is 3000000000ull)
#define DBG_CORE_ALWAYS_SYNCRONIZED 0 // if set to 1: allows the server to start even if the core is not syncronized, useful for debugging with --offline-mode
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define DP(x) LOG_PRINT_L0("LINE " STRINGIZE(__LINE__) ":             " #x " = " << x)

//==============================================================================================================================
  struct vdiff_params_t
  {
    explicit vdiff_params_t()
      : target_min(0)
      , target_max(0)
      , target_time_ms(0)
      , retarget_time_ms(0)
      , retarget_shares_count(0)
      , variance_percent(0)
    {}

    vdiff_params_t(uint64_t target_min, uint64_t target_max, uint64_t target_time_s, uint64_t retarget_time_s, uint64_t retarget_shares_count, uint64_t variance_percent) 
      : target_min(target_min)
      , target_max(target_max)
      , target_time_ms(target_time_s * 1000)
      , retarget_time_ms(retarget_time_s * 1000)
      , retarget_shares_count(retarget_shares_count)
      , variance_percent(variance_percent)
    {}

    uint64_t target_min;
    uint64_t target_max;
    uint64_t target_time_ms;
    uint64_t retarget_time_ms;
    uint64_t retarget_shares_count;
    uint64_t variance_percent;
  };

  struct stratum_connection_context : public epee::net_utils::connection_context_base
  {
    explicit stratum_connection_context()
      : m_worker_name("?")
      , m_worker_difficulty(1)
      , m_ts_started(0)
      , m_ts_share_period_timer(0)
      , m_ts_difficulty_updated(0)
      , m_valid_shares_count(0)
      , m_wrong_shares_count(0)
      , m_hashes_calculated(0)
      , m_blocks_count(0)
    {}

    void set_worker_name(const std::string& worker_name)
    {
      CRITICAL_REGION_LOCAL(m_lock);
      m_worker_name = worker_name;
    }

    uint64_t get_hr_estimate_duration()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      return (epee::misc_utils::get_tick_count() - m_ts_started) / 1000;
    }
    
    uint64_t estimate_worker_hashrate()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      if (m_ts_started == 0)
        return 0;
      uint64_t duration = (epee::misc_utils::get_tick_count() - m_ts_started) / 1000;
      if (duration == 0)
        return 0;
      return (m_hashes_calculated / duration).convert_to<uint64_t>();
    }

    uint64_t get_average_share_period_ms()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      if (m_ts_share_period_timer == 0)
        return 0;
      uint64_t duration_ms = (epee::misc_utils::get_tick_count() - m_ts_share_period_timer);
      if (m_valid_shares_count == 0)
        return 0;
      return duration_ms / m_valid_shares_count;
    }

    void reset_average_share_period_ms()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      m_ts_share_period_timer = epee::misc_utils::get_tick_count();
      m_valid_shares_count = 0;
    }

    void set_worker_difficulty(const wide_difficulty_type& d)
    {
      CRITICAL_REGION_LOCAL(m_lock);
      m_worker_difficulty = d;
      m_ts_difficulty_updated = epee::misc_utils::get_tick_count();
    }

    void init_and_start_timers(const vdiff_params_t& vd_params)
    {
      CRITICAL_REGION_LOCAL(m_lock);
      m_ts_started = epee::misc_utils::get_tick_count();
      m_vd_params = vd_params;
      set_worker_difficulty(m_vd_params.target_min);
      reset_average_share_period_ms();
    }

    wide_difficulty_type get_worker_difficulty()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      return m_worker_difficulty;
    }

    // returns true if worker difficulty has just been changed
    bool adjust_worker_difficulty_if_needed()
    {
      CRITICAL_REGION_LOCAL(m_lock);

      // check retarget condition if 1) there're too many shares received; 2) difficulty was updated long enough time ago
      if (m_valid_shares_count >= m_vd_params.retarget_shares_count ||
          epee::misc_utils::get_tick_count() - m_ts_difficulty_updated > m_vd_params.retarget_time_ms)
      {
        int64_t average_share_period_ms = 0;
        wide_difficulty_type new_d = 0;
        if (m_valid_shares_count != 0)
        {
          // there were shares, calculate using average share period
          average_share_period_ms = static_cast<int64_t>(get_average_share_period_ms());
          if (average_share_period_ms != 0 &&
              uint64_t(llabs(average_share_period_ms - int64_t(m_vd_params.target_time_ms))) > m_vd_params.target_time_ms * m_vd_params.variance_percent / 100)
          {
            new_d = m_worker_difficulty * m_vd_params.target_time_ms / average_share_period_ms;
          }
        }
        else
        {
          // no shares are found during retarget_time_ms, perhaps the difficulty is too high, lower it significantly
          new_d = (m_vd_params.target_min + m_worker_difficulty) / 4;
        }

        if (new_d != 0)
        {
          if (new_d < m_vd_params.target_min)
            new_d = m_vd_params.target_min;
          if (new_d > m_vd_params.target_max)
            new_d = m_vd_params.target_max;
          
          if (new_d != m_worker_difficulty)
          {
            LP_CC_WORKER_YELLOW((*this), "difficulty update: " << m_worker_difficulty << " -> " << new_d <<
              " (av. share pediod was: " << average_share_period_ms << ", target: " << m_vd_params.target_time_ms <<
              ", shares: " << m_valid_shares_count << ", variance: " << int64_t(100 * average_share_period_ms / m_vd_params.target_time_ms - 100) << "%)", LOG_LEVEL_2);
            set_worker_difficulty(new_d);
            reset_average_share_period_ms();
            return true;
          }
        }
      }
      return false;
    }

    void increment_normal_shares_count()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      ++m_valid_shares_count;
      m_hashes_calculated += m_worker_difficulty;
    }

    void increment_stale_shares_count()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      ++m_valid_shares_count;
      m_hashes_calculated += m_worker_difficulty;
    }

    void increment_wrong_shares_count()
    {
      //++m_wrong_shares_count; not implemented yet
    }

    void increment_blocks_count()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      ++m_blocks_count;
    }

    size_t get_blocks_count()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      return m_blocks_count;
    }

    uint64_t get_current_valid_shares_count()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      return m_valid_shares_count;
    }

    mutable epee::critical_section m_lock;
    std::string m_worker_name;
    wide_difficulty_type m_worker_difficulty;
    uint64_t m_ts_started;
    uint64_t m_ts_share_period_timer;
    uint64_t m_ts_difficulty_updated;
    size_t m_valid_shares_count; // number of shares that satisfy worker's difficulty (valid + stale)
    size_t m_wrong_shares_count;
    size_t m_blocks_count;
    wide_difficulty_type m_hashes_calculated;
    vdiff_params_t m_vd_params;
  }; // struct stratum_connection_context

//==============================================================================================================================
  template<typename connection_context_t>
  class stratum_protocol_handler;

  template<typename connection_context_t>
  struct stratum_protocol_handler_config : public i_blockchain_update_listener
  {
    typedef stratum_protocol_handler_config<connection_context_t> this_t;
    typedef stratum_protocol_handler<connection_context_t> protocol_handler_t;

    stratum_protocol_handler_config()
      : m_max_packet_size(10240)
      , m_p_core(nullptr)
      , m_network_difficulty(0)
      , m_miner_addr(null_pub_addr)
      , m_block_template_ethash(null_hash)
      , m_blockchain_last_block_id(null_hash)
      , m_block_template_height(0)
      , m_block_template_update_ts(0)
      , m_last_ts_total_hr_was_printed(epee::misc_utils::get_tick_count())
      , m_total_hr_print_interval_ms(STRATUM_TOTAL_HR_PRINT_INTERVAL_S_DEFAULT * 1000)
      , m_block_template_update_pediod_ms(STRATUM_BLOCK_TEMPLATE_UPD_PERIOD_DEFAULT * 1000)
      , m_nameless_worker_id(0)
      , m_total_blocks_found(0)
      , m_stop_flag(false)
      , m_blocktemplate_update_thread(&this_t::block_template_update_thread, this)
      , m_is_core_always_online(false)
    {
      LOG_PRINT_L4("stratum_protocol_handler_config::ctor()");
    }

    ~stratum_protocol_handler_config()
    {
      NESTED_TRY_ENTRY();

      LOG_PRINT_L4("stratum_protocol_handler_config::dtor()");

      m_stop_flag = true;
      m_blocktemplate_update_thread.join();

      if (m_p_core)
        m_p_core->remove_blockchain_update_listener(this);

      NESTED_CATCH_ENTRY(__func__);
    }

    void add_protocol_handler(protocol_handler_t* p_ph)
    {
      CRITICAL_REGION_BEGIN(m_ph_map_lock);
      m_protocol_handlers[p_ph->get_context().m_connection_id] = p_ph;
      CRITICAL_REGION_END();
      LOG_PRINT_CC(p_ph->get_context(), "stratum_protocol_handler_config: protocol handler added", LOG_LEVEL_4);
      //m_pcommands_handler->on_connection_new(pconn->m_connection_context);
    }

    void remove_protocol_handler(protocol_handler_t* p_ph)
    {
      CRITICAL_REGION_BEGIN(m_ph_map_lock);
      m_protocol_handlers.erase(p_ph->get_context().m_connection_id);
      CRITICAL_REGION_END();
      LOG_PRINT_CC(p_ph->get_context(), "stratum_protocol_handler_config: protocol handler removed", LOG_LEVEL_4);
      //m_pcommands_handler->on_connection_close(pconn->m_connection_context);
    }

    void test()
    {
      //protocol_handler_t* p_ph = m_protocol_handlers.begin()->second;
      //std::string test = " \t  { \n \"{ \\\\  \":\"}\", \"id\":1,\"jsonrpc\":\"2.0\",\"result\":[\"0x63de85850f7e02baba2dc0765ebfb01040e56354729be70358ff341b48c608ad\",\"0xa92afa474bb50595e467a1722ed7a74fc00b3307de5022d5b76cf979490f2222\",\"0x00000000dbe6fecebdedd5beb573440e5a884d1b2fbf06fcce912adcb8d8422e\"]}";
      //std::string test = "{\"error\": [0], \"id\" : 555, \"result\" : \"AAA!\"}";
      //std::string test = "{\"id\":10,\"method\":\"eth_submitWork\",\"params\":[\"0x899624c0078e824f\",\"0xb98617aec14a2872fb45ab755f6273a1ae719d0ff0d441a25bb8235d82cb4123\",\"0x30f599f39276df17656727f16c3230c072dd8f2dd780161625479d352e8b2a97\"]}";
      //std::string test = "{\"id\":10,\"method\":\"eth_submitWork\",\"parasms\":[\"0x899624c0078e824f\"]}";
      //std::string test = R"({"worker": "", "jsonrpc": "2.0", "params": [], "id": 3, "method": "eth_getWork"})";
      //std::string test = R"({"worker": "eth1.0", "jsonrpc": "2.0", "params": ["HeyPnNRKwWzPF3JQMi1dcTJBRr3anA3iEMyY5vokAusYj73grFg8VhiYgWXJ4S31oT5ZV7rCjXn3QgZxQ9xr6DQJ1LThkj3", "x"], "id": 2, "method": "eth_submitLogin"})";
      //p_ph->handle_recv(test.c_str(), test.size());
    }

    void block_template_update_thread()
    {
      epee::log_space::log_singletone::set_thread_log_prefix("[ST]");
      while (!m_stop_flag)
      {
        if (is_core_syncronized() && epee::misc_utils::get_tick_count() - m_block_template_update_ts >= m_block_template_update_pediod_ms)
        {
          update_block_template();
        }
        print_total_hashrate_if_needed();
        epee::misc_utils::sleep_no_w(200);
      }
    }

    bool update_block_template(bool enforce_update = false)
    {
      CRITICAL_REGION_LOCAL(m_work_change_lock);
      uint64_t stub;
      crypto::hash top_block_id = null_hash;
      m_p_core->get_blockchain_top(stub, top_block_id);
      if (!enforce_update && top_block_id == m_blockchain_last_block_id && epee::misc_utils::get_tick_count() - m_block_template_update_ts < m_block_template_update_pediod_ms)
        return false;// no new blocks since last update, keep the same work
      
      LOG_PRINT("stratum_protocol_handler_config::update_block_template(" << (enforce_update ? "true" : "false") << ")", LOG_LEVEL_4);
      m_block_template = AUTO_VAL_INIT(m_block_template);
      wide_difficulty_type block_template_difficulty;
      blobdata extra = AUTO_VAL_INIT(extra);
      bool r = m_p_core->get_block_template(m_block_template, m_miner_addr, m_miner_addr, block_template_difficulty, m_block_template_height, extra);
      CHECK_AND_ASSERT_MES(r, false, "get_block_template failed");
#if DBG_NETWORK_DIFFICULTY == 0
      m_network_difficulty = block_template_difficulty;
#else
      m_network_difficulty = DBG_NETWORK_DIFFICULTY; // for debug purpose only
#endif
      m_blockchain_last_block_id = top_block_id;

      m_block_template_hash_blob = get_block_hashing_blob(m_block_template);
      if (access_nonce_in_block_blob(m_block_template_hash_blob) != 0)
      {
        LOG_PRINT_RED("non-zero nonce in generated block template", LOG_LEVEL_0);
        access_nonce_in_block_blob(m_block_template_hash_blob) = 0;
      }
      m_prev_block_template_ethash = m_block_template_ethash;
      m_block_template_ethash = crypto::cn_fast_hash(m_block_template_hash_blob.data(), m_block_template_hash_blob.size());
      m_block_template_update_ts = epee::misc_utils::get_tick_count();

      set_work_for_all_workers(); // notify all workers of updated work

      return true;
    }

    void set_work_for_all_workers(protocol_handler_t* ph_to_skip = nullptr)
    {
      LOG_PRINT("stratum_protocol_handler_config::set_work_for_all_workers()", LOG_LEVEL_4);
      CRITICAL_REGION_LOCAL(m_ph_map_lock);
      for (auto& ph : m_protocol_handlers)
      {
        if (ph.second == ph_to_skip)
          continue;
        ph.second->get_context().adjust_worker_difficulty_if_needed(); // some miners seem to not give a f*ck about updated job taget if the block hash wasn't changed, so change difficulty only on work update
        std::string new_work_json = get_work_json(ph.second->get_context().get_worker_difficulty());
        ph.second->set_work(new_work_json);
        ph.second->send_notification(new_work_json);
      }
    }

    std::string get_work_json(const wide_difficulty_type& worker_difficulty)
    {
      CRITICAL_REGION_LOCAL(m_work_change_lock);
      if (!is_core_syncronized())
        return R"("result":[])";

      crypto::hash target_boundary = null_hash;
      difficulty_to_boundary_long(worker_difficulty, target_boundary);

      ethash_hash256 seed_hash = ethash_calculate_epoch_seed(ethash_height_to_epoch(m_block_template_height));
      return R"("result":[")" + pod_to_net_format(m_block_template_ethash) + R"(",")" + pod_to_net_format(seed_hash) + R"(",")" + pod_to_net_format_reverse(target_boundary) + R"(",")" + pod_to_net_format_reverse(m_block_template_height) + R"("])";
    }

    void update_work(protocol_handler_t* p_ph)
    {
      LOG_PRINT_CC(p_ph->get_context(), "stratum_protocol_handler_config::update_work()", LOG_LEVEL_4);
      bool updated = false;
      if (is_core_syncronized())
      {
        updated = update_block_template();
      }
      else
      {
        LP_CC_WORKER_YELLOW(p_ph->get_context(), "core is NOT synchronized, respond with empty job package", LOG_LEVEL_2);
      }
      
      if (!updated)
        p_ph->set_work(get_work_json(p_ph->get_context().get_worker_difficulty()));
    }

    bool handle_work(protocol_handler_t* p_ph, const jsonrpc_id_t& id, const std::string& worker, uint64_t nonce, const crypto::hash& block_ethash)
    {
      CRITICAL_REGION_LOCAL(m_work_change_lock);
      bool r = false;

      if (!is_core_syncronized())
      {
        // TODO: make an option for more aggressive mining in case there's a little difference in blockchain size (1..2)
        LP_CC_WORKER_BLUE(p_ph->get_context(), "Work received, but the core is NOT syncronized. Skip...", LOG_LEVEL_1);
        p_ph->send_response_default(id);
        return true;
      }
  
      const uint64_t height = get_block_height(m_block_template);

      // make sure worker sent work with correct block ethash
      if (block_ethash != m_block_template_ethash)
      {
        if (block_ethash == m_prev_block_template_ethash)
        {
          // Got stale share, do nothing. In future it can be used for more aggressive mining strategies
          LP_CC_WORKER_BLUE(p_ph->get_context(), "got stale share, skip it", LOG_LEVEL_1);
          p_ph->send_response_default(id);
          p_ph->get_context().increment_stale_shares_count();
          return true;
        }

        LP_CC_WORKER_RED(p_ph->get_context(), "wrong work submitted, ethhash " << block_ethash << ", expected: " << m_block_template_ethash, LOG_LEVEL_0);
        p_ph->send_response_error(id, JSONRPC_ERROR_CODE_DEFAULT, "wrong work");
        p_ph->get_context().increment_wrong_shares_count();
        return false;
      }

      crypto::hash block_pow_hash = get_block_longhash(height, m_block_template_ethash, nonce);
      wide_difficulty_type worker_difficulty = p_ph->get_context().get_worker_difficulty();

      if (!check_hash(block_pow_hash, worker_difficulty))
      {
        LP_CC_WORKER_RED(p_ph->get_context(), "block pow hash " << block_pow_hash << " doesn't meet worker difficulty: " << worker_difficulty << ENDL <<
          "nonce: " << nonce << " (0x" << epee::string_tools::pod_to_hex(nonce) << ")", LOG_LEVEL_0);
        p_ph->send_response_error(id, JSONRPC_ERROR_CODE_DEFAULT, "not enough work was done");
        p_ph->get_context().increment_wrong_shares_count();
        return false;
      }

      p_ph->send_response_default(id);
      p_ph->get_context().increment_normal_shares_count();
      m_shares_per_minute.chick();

      if (!check_hash(block_pow_hash, m_network_difficulty))
      {
        // work is enough for worker difficulty, but not enough for network difficulty -- it's okay, move on!
        LP_CC_WORKER_GREEN(p_ph->get_context(), "share found for difficulty " << worker_difficulty << ", nonce: 0x" << epee::string_tools::pod_to_hex(nonce), LOG_LEVEL_1);
        LP_CC_WORKER_GREEN(p_ph->get_context(), "shares: " << p_ph->get_context().get_current_valid_shares_count() << ", share period av: " << p_ph->get_context().get_average_share_period_ms() << ", target: " << p_ph->get_context().m_vd_params.target_time_ms
          << ", variance: " << int64_t(100 * p_ph->get_context().get_average_share_period_ms() / p_ph->get_context().m_vd_params.target_time_ms - 100) << " %", LOG_LEVEL_3);
        return true;
      }

      // seems we've just found a block!
      // create a block template and push it to the core
      m_block_template.nonce = nonce;
      crypto::hash block_hash = get_block_hash(m_block_template);

      LP_CC_WORKER_GREEN(p_ph->get_context(), "block found " << block_hash << " at height " << height << " for difficulty " << m_network_difficulty << " pow: " << block_pow_hash << ENDL <<
        "nonce: " << nonce << " (0x" << epee::string_tools::pod_to_hex(nonce) << ")", LOG_LEVEL_1);

      block_verification_context bvc = AUTO_VAL_INIT(bvc);
      r = m_p_core->handle_block_found(m_block_template, &bvc, false);
      if (r)
      {
        if (!bvc.m_verification_failed && !bvc.m_added_to_altchain && bvc.m_added_to_main_chain && !bvc.m_already_exists && !bvc.m_marked_as_orphaned)
        {
          LP_CC_WORKER_GREEN(p_ph->get_context(), "found block " << block_hash << " at height " << height << " was successfully added to the blockchain, difficulty " << m_network_difficulty, LOG_LEVEL_0);
          r = update_block_template();
          CHECK_AND_ASSERT_MES_NO_RET(r, "Stratum: internal error. Block template wasn't updated as expected after handling found block.");
          p_ph->get_context().increment_blocks_count();
          ++m_total_blocks_found;
        }
        else
        {
          LP_CC_WORKER_RED(p_ph->get_context(), "block " << block_hash << " at height " << height << " was NOT added to the blockchain:" << ENDL <<
            "    verification_failed: " << bvc.m_verification_failed  << ENDL <<
            "    added_to_altchain:   " << bvc.m_added_to_altchain      << ENDL <<
            "    added_to_main_chain: " << bvc.m_added_to_main_chain  << ENDL <<
            "    already_exists:      " << bvc.m_already_exists       << ENDL <<
            "    marked_as_orphaned:  " << bvc.m_marked_as_orphaned, LOG_LEVEL_0);
        }
      }
      else
      {
        LP_CC_WORKER_RED(p_ph->get_context(), "block " << block_hash << " was rejected by the core", LOG_LEVEL_0);
      }

      return true;
    }

    bool handle_login(protocol_handler_t* p_ph, const jsonrpc_id_t& id, const std::string& user_str, const std::string& pass_str, const std::string& worker_str, uint64_t start_difficulty)
    {
      CRITICAL_REGION_LOCAL(m_work_change_lock);
      bool r = false, error = false;
      std::stringstream error_str;

      if (user_str == WORKER_ALLOWED_USERNAME)
      {
        // it's a valid login only in case miner address was already set
        if (m_miner_addr == null_pub_addr)
        {
          error = true;
          error_str << "trying to log in with '" WORKER_ALLOWED_USERNAME "' username, while mining address WAS NOT previously set in daemon. Set mining address in daemon OR use it as a username.";
        }
      }
      else
      {
        // mining address is used as username, make sure it's correct and match with previously set
        account_public_address address = null_pub_addr;
        r = get_account_address_from_str(address, user_str);
        if (!r)
        {
          error = true;
          error_str << "can't parse wallet address from username: " << user_str << ".";
        }

        if (m_miner_addr != null_pub_addr && address != m_miner_addr)
        {
          error = true;
          error_str << "wallet address " << user_str << " doesn't match the address previously set in daemon and/or other workers.";
        }

        set_miner_address(address);
      }

      if (error)
      {
        LP_CC_WORKER_RED(p_ph->get_context(), error_str.str() << " Connection will be dropped.", LOG_LEVEL_0);
        p_ph->send_response_error(id, JSONRPC_ERROR_CODE_DEFAULT, error_str.str());
        return false;
      }

      p_ph->get_context().set_worker_name(worker_str);
      if (start_difficulty != 0)
        p_ph->get_context().set_worker_difficulty(start_difficulty);

      LP_CC_WORKER_GREEN(p_ph->get_context(), "logged in with username " << user_str << ", start difficulty: " << (start_difficulty != 0 ? std::to_string(start_difficulty) : "default"), LOG_LEVEL_0);
      p_ph->send_response_default(id);
      
      // send initial work
      update_work(p_ph);

      return true;
    }

    bool handle_submit_hashrate(protocol_handler_t* p_ph, uint64_t rate, const crypto::hash& rate_submit_id)
    {
      LP_CC_WORKER_CYAN(p_ph->get_context(), "reported hashrate: " << HR_TO_STREAM_IN_MHS_3P(rate) << " Mh/s" << 
        ", estimated hashrate: " << HR_TO_STREAM_IN_MHS_3P(p_ph->get_context().estimate_worker_hashrate()) << " Mh/s, run time: " <<
        epee::misc_utils::get_time_interval_string(p_ph->get_context().get_hr_estimate_duration()), LOG_LEVEL_3);
      return true;
    }

    // required member for epee::net_utils::boosted_tcp_server concept
    void on_send_stop_signal()
    {
      LOG_PRINT_L4("stratum_protocol_handler_config::on_send_stop_signal()");
      CRITICAL_REGION_LOCAL(m_ph_map_lock);
      m_protocol_handlers.clear();
    }

    // i_blockchain_update_listener member
    virtual void on_blockchain_update() override
    {
      LOG_PRINT_L3("stratum_protocol_handler_config::on_blockchain_update()");

      if (!is_core_syncronized())
        return; // don't notify workers when blockchain is synchronizing

      update_block_template();
    }

    void set_core(currency::core* c)
    {
      m_p_core = c;
      m_p_core->add_blockchain_update_listener(this);
    }

    void set_is_core_always_online(bool is_core_always_online)
    {
      m_is_core_always_online = is_core_always_online;
    }

    void set_miner_address(const account_public_address& miner_addr)
    {
      m_miner_addr = miner_addr;
    }

    bool is_core_syncronized()
    {
      if (!m_p_core)
        return false;

#if DBG_CORE_ALWAYS_SYNCRONIZED == 1
      return true; // standalone mode, usefull for debugging WITH --offline-mode option
#endif
      if (m_is_core_always_online)
        return true;

      // TODO!!! Bad design, need more correct way of getting this information
      currency::i_currency_protocol* proto = m_p_core->get_protocol();
      currency::t_currency_protocol_handler<currency::core>* protocol = dynamic_cast<currency::t_currency_protocol_handler<currency::core>*>(proto);
      if (!protocol)
        return false;
      return protocol->is_synchronized();
    }

    void set_vdiff_params(const vdiff_params_t& p)
    {
      m_vdiff_params = p;
    }

    const vdiff_params_t& get_vdiff_params() const
    {
      return m_vdiff_params;
    }

    void print_total_hashrate_if_needed()
    {
      if (epee::misc_utils::get_tick_count() - m_last_ts_total_hr_was_printed < m_total_hr_print_interval_ms)
        return;

      if (!is_core_syncronized())
      {
        LOG_PRINT_L0("Blockchain is synchronizing...");
      }

      CRITICAL_REGION_LOCAL(m_ph_map_lock);
      if (m_protocol_handlers.empty())
      {
        LOG_PRINT_CYAN("Blocks found: [" << m_total_blocks_found << "], no miners connected", LOG_LEVEL_0);
      }
      else
      {
        std::stringstream ss;
        uint64_t total_reported_hr = 0, total_estimated_hr = 0;
        for (auto& ph : m_protocol_handlers)
        {
          uint64_t reported_hr = 0, estimated_hr = 0;
          ph.second->get_hashrate(reported_hr, estimated_hr);
          total_reported_hr += reported_hr;
          total_estimated_hr += estimated_hr;
          ss << ph.second->get_context().m_worker_name << ": [" << ph.second->get_context().get_blocks_count() << "] " << HR_TO_STREAM_IN_MHS_1P(reported_hr) << " (" << HR_TO_STREAM_IN_MHS_1P(estimated_hr) << "), ";
        }
        auto s = ss.str();
        LOG_PRINT_CYAN("Blocks found: [" << m_total_blocks_found << "], total speed: " << HR_TO_STREAM_IN_MHS_3P(total_reported_hr) << " Mh/s as reported by miners (" << HR_TO_STREAM_IN_MHS_3P(total_estimated_hr) << " Mh/s estimated by the server), current shares/min: " << m_shares_per_minute.get_speed() << ENDL <<
          m_protocol_handlers.size() << " worker(s): " << s.substr(0, s.length() > 2 ? s.length() - 2 : 0), LOG_LEVEL_0);
      }

      m_last_ts_total_hr_was_printed = epee::misc_utils::get_tick_count();
    }

    void set_total_hr_print_interval_s(uint64_t s)
    {
      m_total_hr_print_interval_ms = s * 1000;
    }

    void set_block_template_update_period(uint64_t s)
    {
      m_block_template_update_pediod_ms = s * 1000;
    }

    size_t get_number_id_for_nameless_worker()
    {
      CRITICAL_REGION_LOCAL(m_generic_lock);
      return m_nameless_worker_id++;
    }


    size_t m_max_packet_size;

  private:
    typedef std::unordered_map<boost::uuids::uuid, protocol_handler_t* , boost::hash<boost::uuids::uuid> > protocol_handlers_map;
    typedef epee::math_helper::speed<60 * 1000 /* ms */> shares_per_minute_rate_t;

    protocol_handlers_map m_protocol_handlers;
    mutable epee::critical_section m_ph_map_lock;
    mutable epee::critical_section m_work_change_lock;
    mutable epee::critical_section m_generic_lock;

    // job data
    block m_block_template;
    std::string m_block_template_hash_blob;
    crypto::hash m_block_template_ethash;
    crypto::hash m_blockchain_last_block_id;
    uint64_t m_block_template_height;
    std::atomic<uint64_t> m_block_template_update_ts;

    // previous job (for handling stale shares)
    crypto::hash m_prev_block_template_ethash;

    vdiff_params_t m_vdiff_params;
    wide_difficulty_type m_network_difficulty;

    core* m_p_core;
    account_public_address m_miner_addr; // all workers will share the same miner address
    std::atomic<uint64_t> m_last_ts_total_hr_was_printed;
    uint64_t m_total_hr_print_interval_ms;
    uint64_t m_block_template_update_pediod_ms;
    size_t m_nameless_worker_id;
    size_t m_total_blocks_found;
    shares_per_minute_rate_t m_shares_per_minute;
    bool m_is_core_always_online;

    std::atomic<bool> m_stop_flag;
    std::thread m_blocktemplate_update_thread;
  }; // struct stratum_protocol_handler_config

  //==============================================================================================================================

  template<typename connection_context_t>
  class stratum_protocol_handler
  {
  public:
    typedef stratum_protocol_handler<connection_context_t> this_t;
    typedef epee::net_utils::connection<this_t> connection_t;

    typedef connection_context_t connection_context; // required type for epee::net_utils::boosted_tcp_server concept
    typedef stratum_protocol_handler_config<connection_context_t> config_type; // required type for epee::net_utils::boosted_tcp_server concept

    // required member for epee::net_utils::boosted_tcp_server concept
    stratum_protocol_handler(connection_t* p_connection, config_type& config, connection_context_t& context)
      : m_p_connection(p_connection)
      , m_config(config)
      , m_context(context)
      , m_connection_initialized(false)
      , m_last_reported_hashrate(0)
    {
      LOG_PRINT_CC(m_context, "stratum_protocol_handler::ctor()", LOG_LEVEL_4);
    }

    ~stratum_protocol_handler()
    {
      NESTED_TRY_ENTRY();

      if (m_connection_initialized)
      {
        m_config.remove_protocol_handler(this);
        m_connection_initialized = false;
      }

      LOG_PRINT_CC(m_context, "stratum_protocol_handler::dtor()", LOG_LEVEL_4);

      NESTED_CATCH_ENTRY(__func__);
    }

    // required member for epee::net_utils::boosted_tcp_server concept
    bool handle_recv(const void* p_data, size_t data_size)
    {
      const char* str = static_cast<const char*>(p_data);

      if (!m_connection_initialized)
        return false;

      if (data_size > m_config.m_max_packet_size)
      {
        LP_CC_WORKER_RED(m_context, "Maximum packet size exceeded! maximum: " << m_config.m_max_packet_size << ", received: " << data_size << ". Connection will be closed.", LOG_LEVEL_0);
        return false;
      }

      m_json_helper.feed(str, data_size);
      LP_CC_WORKER(m_context, "DATA received <<<<<<<<<<<<< " << data_size << " bytes:" << ENDL << std::string(str, data_size), LOG_LEVEL_4);

      if (m_json_helper.has_objects())
      {
        std::string json;
        while (m_json_helper.pop_object(json))
        {
          epee::serialization::portable_storage ps;
          if (ps.load_from_json(json))
          {
            if (!handle_json_request(ps, json))
            {
              LP_CC_WORKER_RED(m_context, "JSON request handling failed. JSON:" << ENDL << json << ENDL, LOG_LEVEL_1);
            }
          }
          else
          {
            LP_CC_WORKER_RED(m_context, "JSON object detected, but can't be parsed. JSON:" << ENDL << json << ENDL, LOG_LEVEL_1);
          }
        }
      }

      return true;
    }

    bool handle_json_request(epee::serialization::portable_storage& ps, const std::string& json)
    {
      std::stringstream error_stream;
      jsonrpc_id_t id = jsonrpc_id_null;
      read_jsonrpc_id(ps, id);

      std::string method;
      if (ps_get_value_noexcept(ps, "method", method, nullptr))
      {
        epee::serialization::portable_storage::hsection params_section = ps.open_section("params", nullptr);
        auto handler_it = m_methods_handlers.find(method);
        if (handler_it == m_methods_handlers.end())
        {
          error_stream << "unknown method is requested: " << method << ENDL << "JSON request: " << json;
          LP_CC_WORKER_RED(m_context, error_stream.str(), LOG_LEVEL_1);
          send_response_error(id, JSONRPC_ERROR_CODE_METHOD_NOT_FOUND, error_stream.str());
          return false;
        }

        return (this->*(handler_it->second))(id, ps, params_section);
      }

      // if it's no a method call -- it should be result
      std::string result;
      if (ps_get_value_noexcept(ps, "result", result, nullptr))
      {
        std::string error;
        ps_get_value_noexcept(ps, "error", error, nullptr);
        epee::serialization::portable_storage::hsection error_section = ps.open_section("error", nullptr);
        if (error_section != nullptr || !error.empty())
        {
          LP_CC_WORKER_RED(m_context, "received an error: " << json, LOG_LEVEL_1);
          return true; // means it is handled ok
        }
      }

      LP_CC_WORKER_YELLOW(m_context, "Unrecongized request received: " << json, LOG_LEVEL_2);
      return true; // means it handled ok
    }

    static void init()
    {
      if (m_methods_handlers.empty())
      {
        m_methods_handlers.insert(std::make_pair("eth_submitLogin",             &this_t::handle_method_eth_submitLogin));
        m_methods_handlers.insert(std::make_pair("eth_getWork",                 &this_t::handle_method_eth_getWork));
        m_methods_handlers.insert(std::make_pair("eth_submitHashrate",          &this_t::handle_method_eth_submitHashrate));
        m_methods_handlers.insert(std::make_pair("eth_submitWork",              &this_t::handle_method_eth_submitWork));
      }
    }

    bool handle_method_eth_submitLogin(const jsonrpc_id_t& id, epee::serialization::portable_storage& ps, epee::serialization::portable_storage::hsection params_section)
    {
      std::string user_str, pass_str;
      epee::serialization::harray params_array = ps.get_first_value("params", user_str, nullptr);
      if (params_array != nullptr)
        ps.get_next_value(params_array, pass_str);

      std::string worker_str;
      ps_get_value_noexcept(ps, "worker", worker_str, nullptr);
      if (worker_str.empty())
        worker_str = std::to_string(m_config.get_number_id_for_nameless_worker());

      uint64_t start_difficulty = 0; // default
      size_t start_diff_delim_pos = user_str.find('-');
      if (start_diff_delim_pos != std::string::npos)
      {
        // extract start difficulty from username presuming it complies format "username.difficulty"
        std::string start_difficulty_str = user_str.substr(start_diff_delim_pos + 1);
        TRY_ENTRY()
          start_difficulty = std::stoull(start_difficulty_str);
        CATCH_ENTRY_CUSTOM("submitLogin", { LOG_PRINT_L0(worker_str << ": Can't parse start difficulty from " << start_difficulty_str); }, false);
        user_str = user_str.substr(0, start_diff_delim_pos);
      }

      LOG_PRINT_CC(m_context, "Stratum [submitLogin] USER: " << user_str << ", pass: " << pass_str << ", worker: " << worker_str << ", start diff.: " << (start_difficulty == 0 ? std::string("default") : std::to_string(start_difficulty)), LOG_LEVEL_3);
      return m_config.handle_login(this, id, user_str, pass_str, worker_str, start_difficulty);
    }

    bool handle_method_eth_getWork(const jsonrpc_id_t& id, epee::serialization::portable_storage& ps, epee::serialization::portable_storage::hsection params_section)
    {
      m_config.update_work(this);

      CRITICAL_REGION_LOCAL(m_work_change_lock);
      if (!m_cached_work_json.empty())
        send_response(id, m_cached_work_json);

      return true;
    }

    bool handle_method_eth_submitHashrate(const jsonrpc_id_t& id, epee::serialization::portable_storage& ps, epee::serialization::portable_storage::hsection params_section)
    {
      std::string rate_str, rate_submit_id_str;
      epee::serialization::harray params_array = ps.get_first_value("params", rate_str, nullptr);
      bool r = params_array != nullptr && ps.get_next_value(params_array, rate_submit_id_str);
      CHECK_AND_ASSERT_MES(r, false, "Incorrect parameters");

      struct { uint64_t low, high, higher, highest; } rate_256 = { 0 }; // this will allow any length of data from 64 to 256 bits (helpful for odd miners)
      CHECK_AND_ASSERT_MES(pod_from_net_format_reverse(rate_str, rate_256, true), false, "Can't parse rate from " << rate_str);
      CHECK_AND_ASSERT_MES(rate_256.high == 0 && rate_256.higher == 0 && rate_256.highest == 0, false, "rate overflow, rate str: " << rate_str);
      crypto::hash rate_submit_id = null_hash;
      CHECK_AND_ASSERT_MES(pod_from_net_format(rate_submit_id_str, rate_submit_id), false, "Can't parse rate_submit_id from " << rate_submit_id_str);

      m_last_reported_hashrate = rate_256.low;
      return m_config.handle_submit_hashrate(this, rate_256.low, rate_submit_id);
    }

    bool handle_method_eth_submitWork(const jsonrpc_id_t& id, epee::serialization::portable_storage& ps, epee::serialization::portable_storage::hsection params_section)
    {
      bool r = true;
      std::string nonce_str, header_str, mixhash_str;
      epee::serialization::harray params_array = ps.get_first_value("params", nonce_str, nullptr);
      r = params_array != nullptr && ps.get_next_value(params_array, header_str);
      r = params_array != nullptr && ps.get_next_value(params_array, mixhash_str);
      CHECK_AND_ASSERT_MES(r, false, "Incorrect parameters");

      std::string worker;
      ps_get_value_noexcept(ps, "worker", worker, nullptr);

      uint64_t nonce = 0;
      CHECK_AND_ASSERT_MES(pod_from_net_format_reverse(nonce_str, nonce, true), false, "Can't parse nonce from " << nonce_str);
      crypto::hash header_hash = null_hash;
      CHECK_AND_ASSERT_MES(pod_from_net_format(header_str, header_hash), false, "Can't parse header hash from " << header_str);

      return m_config.handle_work(this, id, worker, nonce, header_hash);
    }

    void send(const std::string& data)
    {
      static_cast<epee::net_utils::i_service_endpoint*>(m_p_connection)->do_send(data.c_str(), data.size());
      LOG_PRINT_CC(m_context, "DATA sent >>>>>>>>>>>>> " << ENDL << data, LOG_LEVEL_4);
    }

    void send_notification(const std::string& json)
    {
      // JSON-RPC 2.0 spec: "A Notification is a Request object without an "id" member."
      send(R"({"jsonrpc":"2.0",)" + json + "}" "\n"); // LF character is not specified by JSON-RPC standard, but it is REQUIRED by ethminer 0.12 to work
    }
    
    void send_response_method(const jsonrpc_id_t& id, const std::string& method, const std::string& response)
    {
      send_response(id, std::string(R"("method":")") + method + R"(",)" + response);
    }

    void send_response(const jsonrpc_id_t& id, const std::string& response)
    {
      send(R"({"jsonrpc":"2.0","id":)" + jsonrpc_id_to_value_str(id) + "," + response + "}" "\n"); // LF character is not specified by JSON-RPC standard, but it is REQUIRED by ethminer 0.12 to work
    }

    void send_response_default(const jsonrpc_id_t& id)
    {
      send_response(id, R"("result":true)");
    }

    void send_response_error(const jsonrpc_id_t& id, int64_t error_code, const std::string& error_message)
    {
      send_response(id, R"("error":{"code":)" + std::to_string(error_code) + R"(,"message":")" + error_message + R"("})");
    }

    void set_work(const std::string& json)
    {
      CRITICAL_REGION_LOCAL(m_work_change_lock);
      if (m_cached_work_json != json)
      {
        LP_CC_WORKER(m_context, "work updated: " << json, LOG_LEVEL_2);
      }
      m_cached_work_json = json;
    }

    // required member for epee::net_utils::boosted_tcp_server concept
    void handle_qued_callback()
    {
    }

    // required member for epee::net_utils::boosted_tcp_server concept
    bool after_init_connection()
    {
      LOG_PRINT_CC(m_context, "stratum_protocol_handler::after_init_connection()", LOG_LEVEL_4);
      if (!m_connection_initialized)
      {
        m_connection_initialized = true;
        m_config.add_protocol_handler(this);
        m_context.init_and_start_timers(m_config.get_vdiff_params());
      }
      LP_CC_WORKER_CYAN(m_context, "connected", LOG_LEVEL_1);
      return true;
    }

    // required member for epee::net_utils::boosted_tcp_server concept
    bool release_protocol()
    {
      LOG_PRINT_CC(m_context, "stratum_protocol_handler::release_protocol()", LOG_LEVEL_4);
      LP_CC_WORKER(m_context, "disconnected", LOG_LEVEL_0);
      return true;
    }

    connection_context_t& get_context() { return m_context; }
    const connection_context_t& get_context() const { return m_context; }

    void get_hashrate(uint64_t& last_reported_hr, uint64_t& estimated_hr)
    {
      last_reported_hr = m_last_reported_hashrate;
      estimated_hr = m_context.estimate_worker_hashrate();
    }


  private:
    connection_t* m_p_connection; // m_p_connection owns this protocol handler as data member, so back link is a simple pointer
    config_type& m_config;
    connection_context_t& m_context;

    json_helper m_json_helper;
    std::string m_cached_work_json;

    epee::critical_section m_work_change_lock;
    uint64_t m_last_reported_hashrate;

    typedef bool (this_t::*method_handler_func_t)(const jsonrpc_id_t& id, epee::serialization::portable_storage& ps, epee::serialization::portable_storage::hsection params_section);
    static std::unordered_map<std::string, method_handler_func_t> m_methods_handlers;
    
    std::atomic<bool> m_connection_initialized;
  }; // class stratum_protocol_handler
//==============================================================================================================================
  typedef stratum_protocol_handler<stratum_connection_context> protocol_handler_t;
  typedef epee::net_utils::boosted_tcp_server<protocol_handler_t> tcp_server_t;

  // static memeber definition
  template<typename connection_context_t>
  std::unordered_map<std::string, typename stratum_protocol_handler<connection_context_t>::method_handler_func_t> stratum_protocol_handler<connection_context_t>::m_methods_handlers;
} // anonumous namespace

//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
struct stratum_server_impl
{
  tcp_server_t server;
};
//------------------------------------------------------------------------------------------------------------------------------
stratum_server::stratum_server(core* c)
  : m_p_core(c)
  , m_threads_count(0)
{
  m_impl = new stratum_server_impl();
}
//------------------------------------------------------------------------------------------------------------------------------
stratum_server::~stratum_server()
{
  delete m_impl;
  m_impl = nullptr;
}
//------------------------------------------------------------------------------------------------------------------------------
void stratum_server::init_options(boost::program_options::options_description& desc)
{
  stratum_protocol_handler<stratum_connection_context>::init();

  command_line::add_arg(desc, arg_stratum);
  command_line::add_arg(desc, arg_stratum_bind_ip);
  command_line::add_arg(desc, arg_stratum_bind_port);
  command_line::add_arg(desc, arg_stratum_threads);
  command_line::add_arg(desc, arg_stratum_miner_address);
  command_line::add_arg(desc, arg_stratum_vdiff_target_min);
  command_line::add_arg(desc, arg_stratum_vdiff_target_max);
  command_line::add_arg(desc, arg_stratum_vdiff_target_time);
  command_line::add_arg(desc, arg_stratum_vdiff_retarget_time);
  command_line::add_arg(desc, arg_stratum_vdiff_retarget_shares);
  command_line::add_arg(desc, arg_stratum_vdiff_variance_percent);
  command_line::add_arg(desc, arg_stratum_block_template_update_period);
  command_line::add_arg(desc, arg_stratum_hr_print_interval);
  command_line::add_arg(desc, arg_stratum_always_online);

}
//------------------------------------------------------------------------------------------------------------------------------
bool stratum_server::should_start(const boost::program_options::variables_map& vm)
{
  return command_line::get_arg(vm, arg_stratum);
}
//------------------------------------------------------------------------------------------------------------------------------
bool stratum_server::init(const boost::program_options::variables_map& vm)
{
  bool r = false;
  m_impl->server.set_threads_prefix("ST");

  std::string bind_ip_str = command_line::get_arg(vm, arg_stratum_bind_ip);
  std::string bind_port_str = command_line::get_arg(vm, arg_stratum_bind_port);
  m_threads_count = command_line::get_arg(vm, arg_stratum_threads);

  auto& config = m_impl->server.get_config_object();
  config.set_core(m_p_core);
  
  if (command_line::has_arg(vm, arg_stratum_always_online))
  {
     config.set_is_core_always_online(command_line::get_arg(vm, arg_stratum_always_online));
  }


  if (command_line::has_arg(vm, arg_stratum_miner_address))
  {
    std::string miner_address_str = command_line::get_arg(vm, arg_stratum_miner_address);
    account_public_address miner_address = null_pub_addr;
    r = get_account_address_from_str(miner_address, miner_address_str);
    CHECK_AND_ASSERT_MES(r, false, "Stratum server: invalid miner address given: " << miner_address_str);
    config.set_miner_address(miner_address);
  }

  config.set_vdiff_params(
    vdiff_params_t(
      command_line::get_arg(vm, arg_stratum_vdiff_target_min),
      command_line::get_arg(vm, arg_stratum_vdiff_target_max),
      command_line::get_arg(vm, arg_stratum_vdiff_target_time),
      command_line::get_arg(vm, arg_stratum_vdiff_retarget_time),
      command_line::get_arg(vm, arg_stratum_vdiff_retarget_shares),
      command_line::get_arg(vm, arg_stratum_vdiff_variance_percent)
    )
  );

  config.set_block_template_update_period(command_line::get_arg(vm, arg_stratum_block_template_update_period));

  config.set_total_hr_print_interval_s(command_line::get_arg(vm, arg_stratum_hr_print_interval));

  LOG_PRINT_L0("Stratum server: start listening at " << bind_ip_str << ":" << bind_port_str << "...");
  r = m_impl->server.init_server(bind_port_str, bind_ip_str);
  CHECK_AND_ASSERT_MES(r, false, "Stratum server: initialization failure");

  return true;
}
//------------------------------------------------------------------------------------------------------------------------------
bool stratum_server::run(bool wait /* = true */)
{
  //m_impl->server.get_config_object().test();

  LOG_PRINT("Stratum server: start net server with " << m_threads_count << " threads...", LOG_LEVEL_0);
  if (!m_impl->server.run_server(m_threads_count, wait))
  {
    LOG_ERROR("Stratum server: net server failure");
    return false;
  }

  if (wait)
    LOG_PRINT("Stratum server: net server stopped", LOG_LEVEL_0);
  return true;
}
//------------------------------------------------------------------------------------------------------------------------------
bool stratum_server::deinit()
{
  return m_impl->server.deinit_server();
}
//------------------------------------------------------------------------------------------------------------------------------
bool stratum_server::timed_wait_server_stop(uint64_t ms)
{
  return m_impl->server.timed_wait_server_stop(ms);
}
//------------------------------------------------------------------------------------------------------------------------------
bool stratum_server::send_stop_signal()
{
  m_impl->server.send_stop_signal();
  return true;
}
//------------------------------------------------------------------------------------------------------------------------------

} // namespace currency
