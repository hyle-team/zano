// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <thread>

#include "include_base_utils.h"
using namespace epee;
#include "syncobj.h"
#include "readwrite_lock.h"





std::recursive_mutex l1, l2, l3, l_main;
std::recursive_mutex local1;
epee::shared_recursive_mutex sh_write_lock;
epee::shared_membership<shared_recursive_mutex> sh_read_lock(sh_write_lock);
void f1();
void f2();
void f3();
std::atomic<bool> do_dl(false);

void f1()
{
  log_space::log_singletone::set_thread_log_prefix("[F1]");
  {
    CRITICAL_REGION_LOCAL_VAR(local1, sss);
  }

  CRITICAL_REGION_LOCAL(l1);
  do_dl = true;
  //potential dead_lock
  CRITICAL_REGION_LOCAL1(l2);
}
std::thread f1_thread;
void f2()
{
  log_space::log_singletone::set_thread_log_prefix("[F2]");
  CRITICAL_REGION_LOCAL(l2);
  f1_thread = std::thread([&]() {f1(); });
  CRITICAL_REGION_LOCAL1(l3);

}
std::thread f2_thread;
void f3()
{
  log_space::log_singletone::set_thread_log_prefix("[F3]");
  CRITICAL_REGION_LOCAL(l3);
  f2_thread = std::thread([&]() {f2(); });
  CRITICAL_REGION_LOCAL1(l_main);
}

 /*

 do_deadlock_test -> (l_main) -------------------------------------------------------| l1
                                |                                               ^
                                |                                               | 
                                ---f3-->(l3)-------| l_main                     |
                                              |                                 |
                                              |                                 |
                                              ---f2-->(l2)-------| l3           |
                                                            |                   |
                                                            |                   |
                                                            ---f1-->(l1)------------| l2
                                                                          


*/
void do_deadlock_test_main()
{
  log_space::log_singletone::set_thread_log_prefix("[MAIN]");
  bool r = CRITICAL_SECTION_TRY_LOCK(l_main);
  if (!r)
    return;
  std::thread f3_thread = std::thread([&]() {f3(); });
  
  while(!do_dl)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

  //to be sure it will happened here
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  CRITICAL_REGION_LOCAL_VAR(sh_write_lock, eeee);
  CRITICAL_REGION_LOCAL_VAR(sh_read_lock, ffff);
  //TODO: lock/unlock for shared

  try
  {
    //potential dead_lock
    CRITICAL_REGION_LOCAL1(l1);

  }

  catch (const std::runtime_error& e)
  {
    std::string s = e.what();
    LOG_PRINT_L0(e.what());
    CRITICAL_SECTION_UNLOCK(l_main);
    f3_thread.join();
    f2_thread.join();
    f1_thread.join();
  }

}