// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include <boost/atomic.hpp>
#include <boost/program_options.hpp>
#include <atomic>
#include "currency_basic.h"
#include "difficulty.h"
#include "math_helper.h"
#include "blockchain_storage.h"

#define CURRENCY_MINER_BLOCK_BLOB_NONCE_OFFSET    1

namespace currency
{

  inline uint64_t& access_nonce_in_block_blob(blobdata& bd)
  {
    return *reinterpret_cast<uint64_t*>(&bd[CURRENCY_MINER_BLOCK_BLOB_NONCE_OFFSET]);
  }

  inline const uint64_t& access_nonce_in_block_blob(const blobdata& bd)
  {
    return *reinterpret_cast<const uint64_t*>(&bd[CURRENCY_MINER_BLOCK_BLOB_NONCE_OFFSET]);
  }

  struct i_miner_handler
  {
    virtual bool handle_block_found(const block& b, block_verification_context* p_verification_result = nullptr) = 0;
    virtual bool get_block_template(block& b, crypto::hash& seed, const account_public_address& adr, const account_public_address& stakeholder_address, wide_difficulty_type& diffic, uint64_t& height, const blobdata& ex_nonce, bool pos = false, const pos_entry& pe = pos_entry()) = 0;
  protected:
    ~i_miner_handler(){};
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class miner
  {
  public: 
    miner(i_miner_handler* phandler, blockchain_storage& bc);
    ~miner();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    static void init_options(boost::program_options::options_description& desc);
    bool on_block_chain_update();
    bool start(const account_public_address& adr, size_t threads_count);
    uint64_t get_speed();
    void send_stop_signal();
    bool stop();
    bool is_mining();
    bool on_idle();
    void on_synchronized();
    //synchronous analog (for fast calls)
    void pause();
    void resume();
    void do_print_hashrate(bool do_hr);

    inline
    static bool find_nonce_for_given_block(block& bl, const wide_difficulty_type& diffic, uint64_t height, const crypto::hash& seed, scratchpad_keeper& sk)
    {      
      blobdata bd = get_block_hashing_blob(bl);
      uint64_t& nonce_ref = access_nonce_in_block_blob(bd);
      nonce_ref = 0;

      for(; bl.nonce != std::numeric_limits<uint64_t>::max(); bl.nonce++)
      {
        nonce_ref = bl.nonce;

        crypto::hash h = sk.get_pow_hash_from_blob(bd, height, seed);
        if(check_hash(h, diffic))
        {
          LOG_PRINT_L0("Found nonce for block: " << get_block_hash(bl) << "[" << height << "]: PoW:" << h << "(diff:" << diffic << "), ts: " << bl.timestamp);
          return true;
        }
      }
      return false;
    }

  private:
    bool set_block_template(const block& bl, const wide_difficulty_type& diffic, uint64_t height, const crypto::hash& seed);
    bool worker_thread();
    bool request_block_template();
    void  merge_hr();
    
    struct miner_config
    {
      uint64_t current_extra_message_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(current_extra_message_index)
      END_KV_SERIALIZE_MAP()
    };


    volatile uint32_t m_stop;
    ::critical_section m_template_lock;
    block m_template;
    std::atomic<uint32_t> m_template_no;
    std::atomic<uint32_t> m_starter_nonce;
    wide_difficulty_type m_diffic;
    std::atomic<uint64_t> m_height;
    scratchpad_keeper m_scratchpad;
    crypto::hash m_seed;
    volatile uint32_t m_thread_index; 
    volatile uint32_t m_threads_total;
    std::atomic<int32_t> m_pausers_count;
    ::critical_section m_miners_count_lock;    

    std::list<boost::thread> m_threads;
    ::critical_section m_threads_lock;
    i_miner_handler* m_phandler;
    blockchain_storage& m_bc;
    account_public_address m_mine_address;
    math_helper::once_a_time_seconds<5> m_update_block_template_interval;
    math_helper::once_a_time_seconds<2> m_update_merge_hr_interval;
    std::vector<blobdata> m_extra_messages;
    miner_config m_config;
    std::string m_config_folder;  
    std::string m_template_extra_text;
    std::atomic<uint64_t> m_current_hash_rate;
    std::atomic<uint64_t> m_last_hr_merge_time;
    std::atomic<uint64_t> m_hashes;
    bool m_do_print_hashrate;
    bool m_do_mining;
    
  };
}



