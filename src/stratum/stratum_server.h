// Copyright (c) 2018-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "stratum"

namespace currency
{
  class core;
  struct stratum_server_impl;

  class stratum_server
  {
  public:
    static void init_options(boost::program_options::options_description& desc);
    static bool should_start(const boost::program_options::variables_map& vm);

    stratum_server(core* c);
    ~stratum_server();
    bool init(const boost::program_options::variables_map& vm);
    bool run(bool wait = true);
    bool deinit();
    bool timed_wait_server_stop(uint64_t ms);
    bool send_stop_signal();

  private:
    size_t m_threads_count;

    stratum_server_impl* m_impl;
    core* m_p_core;
  };
}

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL
