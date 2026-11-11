// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "chaingen.h"
#include "currency_core/bc_offers_service.h"


inline bool get_all_offers(currency::core& c, std::list<bc_services::offer_details_ex>& offers)
{
  bc_services::bc_offers_service* offers_service = dynamic_cast<bc_services::bc_offers_service*>(c.get_blockchain_storage().get_attachment_services_manager().get_service_by_id(BC_OFFERS_SERVICE_ID));
  CHECK_AND_ASSERT_MES(offers_service != nullptr, false, "Offers service was not registered in attachment service manager!");

  bc_services::core_offers_filter cof = AUTO_VAL_INIT(cof);
  cof.limit = 100000;
  cof.offset = 0;
  cof.order_by = ORDER_BY_TIMESTAMP;
  uint64_t total_count_stub;
    
  offers_service->get_offers_ex(cof, offers, total_count_stub, c.get_blockchain_storage().get_core_runtime_config().get_core_time());
  return true;
}