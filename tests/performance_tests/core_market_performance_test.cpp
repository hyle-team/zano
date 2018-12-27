// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <iostream>
#include <stdint.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <boost/program_options.hpp>

#include "include_base_utils.h"

#include "currency_core/currency_core.h"
#include "misc_language.h"
#include "currency_core/bc_offers_service.h"

#if defined(WIN32)
#include <crtdbg.h>
#endif


uint64_t get_memory_used()
{

#if defined(WIN32)
  
  _CrtMemState state = {0};
  _CrtMemCheckpoint(&state);
  return state.lTotalCount;
  /*
  HANDLE hProcess;
  PROCESS_MEMORY_COUNTERS pmc;
  hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
    PROCESS_VM_READ,
    FALSE, GetCurrentProcessId());

  if (NULL == hProcess)
  {
    std::cout << "Failed to get process handle";
    return 0;
  }
  if (!GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
  {
    std::cout << "Failed to GetProcessMemoryInfo";
    return 0;
  }
  
  return pmc.WorkingSetSize;
  */
#endif
  return 0;
}

std::string get_random_rext(size_t len)
{
  std::string buff(len/2, 0);
  crypto::generate_random_bytes(len/2, (void*)buff.data());
  return string_tools::buff_to_hex_nodelimer(buff);
}

bool run_core_market_performance_tests(uint64_t offers_amount)
{
#if defined(WIN32)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
#endif
  currency::core c(nullptr);

  bc_services::bc_offers_service* offers_service = dynamic_cast<bc_services::bc_offers_service*>(c.get_blockchain_storage().get_attachment_services_manager().get_service_by_id(BC_OFFERS_SERVICE_ID));
  CHECK_AND_ASSERT_MES(offers_service != nullptr, false, "Offers service was not registered in attachment service manager!");
  auto& m_offers = offers_service->get_offers_container();
  uint64_t mem_use_before = get_memory_used();

  std::cout << "Memory usage at start: " << mem_use_before << std::endl;

  std::string s;
  std::cin >> s;
  std::cout << "Filling market....";
  for (uint64_t i = 0; i != offers_amount; i++)
  {
    bc_services::offer_details od;
    od.offer_type = rand()%4;     
    od.amount_primary = rand();     
    od.amount_target = rand();     
    od.bonus = get_random_rext(10);
    od.target = get_random_rext(10);
    od.location_country = get_random_rext(6);
    od.location_city = get_random_rext(10);
    od.contacts = get_random_rext(20);
    od.comment = get_random_rext(30);
    od.payment_types = get_random_rext(10);
    od.deal_option = get_random_rext(10);
    od.category = get_random_rext(4);
    od.expiration_time = 3;

    bc_services::offer_details_ex_with_hash odl;
    static_cast<bc_services::offer_details&>(odl) = od;
    odl.h = crypto::rand<crypto::hash>();

    odl.timestamp = time(nullptr);
    odl.index_in_tx = 0;
    odl.tx_hash = currency::get_object_hash(od.comment);
    odl.stopped = false;
    odl.fee = 10000;

    try
    {
      auto& offers_index = m_offers.get<bc_services::by_id>();
      offers_index.insert(odl);
    }
    catch (...)
    {
      LOG_ERROR("Failed to put offer to index, offer[" << odl.h << "]: " << epee::serialization::store_t_to_json(odl));
      return false;
    }
  }
  std::cout << "Done" << std::endl;
  uint64_t mem_usage = get_memory_used();
  std::cout << "Memory usage: " << (mem_usage - mem_use_before) / (1024 * 1024) << "Mbytes" << std::endl;

  std::cin >> s;
  
  std::cout << "Finished" << std::endl;
  return true;
}
