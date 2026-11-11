// Copyright (c) 2014-2017 The The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "net/http_client.h"
#include "currency_protocol/blobdatatype.h"
#include "rpc/mining_protocol_defs.h"
#include "currency_core/difficulty.h"
#include <boost/atomic.hpp>
#include <atomic>


#define LOCAL_SCRATCHPAD_CACHE_EXPIRATION_INTERVAL 60*60*24*3   //3 days
#define LOCAL_SCRATCHPAD_CACHE_STORE_INTERVAL      60*60*12     //12 hours

/*
namespace mining
{
  class  simpleminer
  {
  public:
    static void init_options(boost::program_options::options_description& desc);
    bool init(const boost::program_options::variables_map& vm);
    bool run();
  private: 

    struct height_info_native
    {
      uint64_t height;
      crypto::hash id;
    };
    
    struct job_details_native
    {
      currency::blobdata blob;
      currency::difficulty_type difficulty;
      std::string job_id;
      height_info_native prev_hi;
    };


    static bool text_height_info_to_native_height_info(const height_info& job, height_info_native& hi_native);
    static bool native_height_info_to_text_height_info(height_info& job, const height_info_native& hi_native);

    template<class job_details_t>
    bool text_job_details_to_native_job_details(const job_details_t& job, simpleminer::job_details_native& native_details)
    {
      bool r = epee::string_tools::parse_hexstr_to_binbuff(job.blob, native_details.blob);
      CHECK_AND_ASSERT_MES(r, false, "wrong buffer sent from pool server");
      r = epee::string_tools::get_xtype_from_string(native_details.difficulty, job.difficulty);
      CHECK_AND_ASSERT_MES(r, false, "wrong buffer sent from pool server");
      native_details.job_id = job.job_id;

      return text_height_info_to_native_height_info(job.prev_hi, native_details.prev_hi);
    }



    bool get_job();
    bool reinit_scratchpad();
    bool apply_addendums(const std::list<addendum>& addms);
    bool pop_addendum(const addendum& add);
    bool push_addendum(const addendum& add);
    void worker_thread(uint64_t start_nonce, uint32_t nonce_offset, std::atomic<uint32_t> *result, std::atomic<bool> *do_reset, std::atomic<bool> *done);
    void update_fast_scratchpad();
    void free_fast_scratchpad();
    bool init_scratchpad();
    bool reset_scratchpad();
    bool load_scratchpad_from_file(const std::string& path);
    bool store_scratchpad_to_file(const std::string& path);

    std::vector<mining::addendum> m_blocks_addendums; //need to handle splits without re-downloading whole scratchpad
    height_info_native m_hi;
    std::vector<crypto::hash> m_scratchpad;
    crypto::hash *m_fast_scratchpad;
    uint32_t m_fast_scratchpad_pages;
    uint64_t m_last_job_ticks;
    uint64_t m_last_scratchpad_store_time;
    bool m_fast_mmapped;
    uint32_t m_threads_total;
    std::atomic<uint64_t> m_hashes_done;
    std::string m_pool_session_id;
    simpleminer::job_details_native m_job;
    std::condition_variable m_work_done_cond;
    std::mutex m_work_mutex;
    std::string m_scratchpad_url;
    std::string m_scratchpad_local_path;

    std::string m_pool_ip;
    std::string m_pool_port;
    std::string m_login;
    std::string m_pass;
    epee::net_utils::http::http_simple_client m_http_client;
  };
}
*/