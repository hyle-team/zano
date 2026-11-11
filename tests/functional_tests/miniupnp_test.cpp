// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chrono>
#include <boost/thread.hpp>
#include "common/miniupnp_helper.h"



bool test_casual()
{
  LOG_PRINT_L0("Starting miniupnp tests...");
  tools::miniupnp_helper hlpr;

  hlpr.start_regular_mapping(10101, 10101, 10000);

  boost::this_thread::sleep_for(boost::chrono::milliseconds(50000));

  return true;
}



bool miniupnp_test()
{
  if(!test_casual())
  {
    LOG_ERROR("Failed test_casual()");
    return false;
  }




  return true;
}