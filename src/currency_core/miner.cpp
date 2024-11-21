// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sstream>
#include <numeric>
#include <boost/utility/value_init.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/limits.hpp>
#include <boost/foreach.hpp>
#include "misc_language.h"
#include "include_base_utils.h"

#include "currency_format_utils.h"
#include "file_io_utils.h"
#include "common/command_line.h"
#include "string_coding.h"
#include "version.h"
#include "storages/portable_storage_template_helper.h"
#include "basic_pow_helpers.h"

using namespace epee;

#include "miner.h"



namespace currency
{

  namespace
  {
    const command_line::arg_descriptor<std::string>   arg_extra_messages      ("extra-messages-file", "Specify file for extra messages to include into coinbase transactions");
    const command_line::arg_descriptor<std::string>   arg_start_mining        ("start-mining", "Specify wallet address to mining for");
    const command_line::arg_descriptor<uint32_t>      arg_mining_threads      ("mining-threads", "Specify mining threads count");
    const command_line::arg_descriptor<std::string>   arg_block_template_extra_text  ( "miner-text-info", "Set block extra text info");
  }


  miner::miner(i_miner_handler* phandler, blockchain_storage& bc)
    : m_stop(1)
    , m_template(boost::value_initialized<block>())
    , m_template_no(0)
    , m_diffic(0)
    , m_thread_index(0)
    , m_phandler(phandler)
    , m_height(0)
    , m_pausers_count(0)
    , m_block_template_ready(false)
    , m_threads_total(0)
    , m_starter_nonce(0)
    , m_do_print_hashrate(false)
    , m_do_mining(false)
    , m_current_hash_rate(0)
    , m_last_hr_merge_time(0)
    , m_hashes(0)
    , m_config(AUTO_VAL_INIT(m_config))
    , m_mine_address{}
  {
  }
  //-----------------------------------------------------------------------------------------------------
  miner::~miner()
  {
    TRY_ENTRY();
    stop();
    CATCH_ENTRY_NO_RETURN();
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::set_block_template(const block& bl, const wide_difficulty_type& di, uint64_t height)
  {
    CRITICAL_REGION_LOCAL(m_template_lock);
    m_template = bl;
    m_diffic = di;
    m_height = height;
    ++m_template_no;
    m_starter_nonce = crypto::rand<uint32_t>();
    m_block_template_ready = true;
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::on_block_chain_update()
  {
    if(!is_mining())
      return true;
    
    //here miner threads may work few rounds without 
    return request_block_template();
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::request_block_template()
  {
    block bl = AUTO_VAL_INIT(bl);
    wide_difficulty_type di = AUTO_VAL_INIT(di);
    uint64_t height = AUTO_VAL_INIT(height);
    currency::blobdata extra_nonce = m_template_extra_text;
    if(m_extra_messages.size() && m_config.current_extra_message_index < m_extra_messages.size())
    {
      extra_nonce += std::string("|") + m_extra_messages[m_config.current_extra_message_index];
    }
    if(!m_phandler->get_block_template(bl, m_mine_address, m_mine_address, di, height, extra_nonce))
    {
      // it's quite possible that block template cannot be created at particular time; need to wait
      m_block_template_ready = false;
      return false;
    }
    set_block_template(bl, di, height);
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::on_idle()
  {
    m_update_block_template_interval.do_call([&](){
      if(is_mining())
        request_block_template();
      return true;
    });

    m_update_merge_hr_interval.do_call([&](){
      merge_hr();
      return true;
    });
    
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::do_print_hashrate(bool do_hr)
  {
    m_do_print_hashrate = do_hr;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::merge_hr()
  {
    if(m_last_hr_merge_time && is_mining())
    {
      m_current_hash_rate = (m_hashes * 1000) / ((misc_utils::get_tick_count() - m_last_hr_merge_time + 1));
    }
    m_last_hr_merge_time = misc_utils::get_tick_count();
    m_hashes = 0;
    if(m_do_print_hashrate && is_mining())
      std::cout << "hr: " << m_current_hash_rate << ENDL;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_extra_messages);
    command_line::add_arg(desc, arg_start_mining);
    command_line::add_arg(desc, arg_mining_threads);
    command_line::add_arg(desc, arg_block_template_extra_text);    
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::init(const boost::program_options::variables_map& vm)
  {
    m_config_folder = command_line::get_arg(vm, command_line::arg_data_dir);
    epee::serialization::load_t_from_json_file(m_config, m_config_folder + "/" + MINER_CONFIG_FILENAME);

    if(command_line::has_arg(vm, arg_extra_messages))
    {
      std::string buff;
      bool r = file_io_utils::load_file_to_string(command_line::get_arg(vm, arg_extra_messages), buff);
      CHECK_AND_ASSERT_MES(r, false, "Failed to load file with extra messages: " << command_line::get_arg(vm, arg_extra_messages));
      std::vector<std::string> extra_vec;
      boost::split(extra_vec, buff, boost::is_any_of("\n"), boost::token_compress_on );
      m_extra_messages.resize(extra_vec.size());
      for(size_t i = 0; i != extra_vec.size(); i++)
      {
        string_tools::trim(extra_vec[i]);
        if(!extra_vec[i].size())
          continue;
        buff = string_encoding::base64_decode(extra_vec[i]);
        if(buff != "0")
          m_extra_messages[i] = buff;
      }
      LOG_PRINT_L0("Loaded " << m_extra_messages.size() << " extra messages, current index " << m_config.current_extra_message_index);
    }

    if(command_line::has_arg(vm, arg_start_mining))
    {
      if(!currency::get_account_address_from_str(m_mine_address, command_line::get_arg(vm, arg_start_mining)))
      {
        LOG_ERROR("Target account address " << command_line::get_arg(vm, arg_start_mining) << " has wrong format, starting daemon canceled");
        return false;
      }
      m_threads_total = 1;
      m_do_mining = true;
      if(command_line::has_arg(vm, arg_mining_threads))
      {
        m_threads_total = command_line::get_arg(vm, arg_mining_threads);
      }
    }

    if (command_line::has_arg(vm, arg_block_template_extra_text))
    {
      m_template_extra_text = command_line::get_arg(vm, arg_block_template_extra_text);
    }
    else
    {
      m_template_extra_text = PROJECT_VERSION_LONG;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::deinit()
  {
    if (!tools::create_directories_if_necessary(m_config_folder))
    {
      LOG_PRINT_L0("Failed to create data directory: " << m_config_folder);
      return false;
    }
    epee::serialization::store_t_to_json_file(m_config, m_config_folder + "/" + MINER_CONFIG_FILENAME);
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::is_mining()
  {
    return !m_stop;
  }
  //----------------------------------------------------------------------------------------------------- 
  bool miner::start(const account_public_address& adr, size_t threads_count)
  {

    m_mine_address = adr;
    m_threads_total = static_cast<uint32_t>(threads_count);
    m_starter_nonce = crypto::rand<uint32_t>();
    CRITICAL_REGION_LOCAL(m_threads_lock);
    if(is_mining())
    {
      LOG_PRINT_L0("Starting miner:  miner already running");
      return false;
    }

    if(!m_threads.empty())
    {
      LOG_ERROR("Starting miner canceled:  there are still active mining threads");
      return false;
    }

    while(!m_template_no)
    {
      if (request_block_template())
        break;
      std::this_thread::sleep_for(std::chrono::seconds(2));
      LOG_PRINT_L1("Trying to get block template....");
    }

    boost::interprocess::ipcdetail::atomic_write32(&m_stop, 0);
    boost::interprocess::ipcdetail::atomic_write32(&m_thread_index, 0);

    for(size_t i = 0; i != threads_count; i++)
      m_threads.push_back(boost::thread(boost::bind(&miner::worker_thread, this)));

    LOG_PRINT_L0("Mining has started with " << threads_count << " threads, good luck!" )
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  uint64_t miner::get_speed()
  {
    if(is_mining())
      return m_current_hash_rate;
    else 
      return 0;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::send_stop_signal()
  {
    boost::interprocess::ipcdetail::atomic_write32(&m_stop, 1);
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::stop()
  {
    send_stop_signal();
    CRITICAL_REGION_LOCAL(m_threads_lock);

    BOOST_FOREACH(boost::thread& th, m_threads)
      th.join();

    m_threads.clear();
    LOG_PRINT_L0("Mining has been stopped, " << m_threads.size() << " finished" );
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::on_synchronized()
  {
    if(m_do_mining)
    {
      start(m_mine_address, m_threads_total);
    }
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::pause()
  {
    CRITICAL_REGION_LOCAL(m_miners_count_lock);
    ++m_pausers_count;
    if(m_pausers_count == 1 && is_mining())
      LOG_PRINT_L2("MINING PAUSED");
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::resume()
  {
    CRITICAL_REGION_LOCAL(m_miners_count_lock);
    --m_pausers_count;
    if(m_pausers_count < 0)
    {
      m_pausers_count = 0;
      LOG_PRINT_RED_L0("Unexpected miner::resume() called");
    }
    if(!m_pausers_count && is_mining())
      LOG_PRINT_L2("MINING RESUMED");
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::worker_thread()
  {
    uint32_t th_local_index = boost::interprocess::ipcdetail::atomic_inc32(&m_thread_index);
    LOG_PRINT_L0("Miner thread was started ["<< th_local_index << "]");
    log_space::log_singletone::set_thread_log_prefix(std::string("[miner ") + std::to_string(th_local_index) + "]");
    uint64_t nonce = m_starter_nonce + th_local_index;
    wide_difficulty_type local_diff = 0;
    uint32_t local_template_ver = 0;
    blobdata local_blob_data;
    uint64_t local_height = 0;
    crypto::hash local_blob_data_hash = null_hash;

    block b{};

    while(!m_stop)
    {
      if(m_pausers_count != 0 || !m_block_template_ready)
      {
        misc_utils::sleep_no_w(100);
        continue;
      }

      if(local_template_ver != m_template_no)
      {        
        CRITICAL_REGION_BEGIN(m_template_lock);
        b = m_template;
        b.nonce = 0;
        local_diff = m_diffic;
        local_height = m_height;
        CRITICAL_REGION_END();
        //local_template_height = get_block_height(b);
        local_template_ver = m_template_no;
        nonce = m_starter_nonce + th_local_index;
        local_blob_data = get_block_hashing_blob(b);
        local_blob_data_hash = crypto::cn_fast_hash(local_blob_data.data(), local_blob_data.size());
      }

      if(!local_template_ver)//no any set_block_template call
      {
        LOG_PRINT_L2("Block template not set yet");
        epee::misc_utils::sleep_no_w(1000);
        continue;
      }
      //b.nonce = nonce;
      //access_nonce_in_block_blob(local_blob_data) = b.nonce;
      crypto::hash h = get_block_longhash(local_height, local_blob_data_hash, nonce);

      if(check_hash(h, local_diff))
      {
        b.nonce = nonce;
        ++m_config.current_extra_message_index;
        LOG_PRINT_GREEN("Found block for difficulty: " << local_diff << ", height: " << local_height << ", PoW hash: " << h << ", local_blob_data_hash: " << local_blob_data_hash << ", nonce: " << std::hex << nonce, LOG_LEVEL_0);
        if(!m_phandler->handle_block_found(b))
        {
          --m_config.current_extra_message_index;
        }
        else
        {
          //success, let's update config
          epee::serialization::store_t_to_json_file(m_config, m_config_folder + "/" + MINER_CONFIG_FILENAME);
        }
      }
      nonce+=m_threads_total;
      ++m_hashes;
    }
    LOG_PRINT_L0("Miner thread stopped ["<< th_local_index << "]");
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
}

