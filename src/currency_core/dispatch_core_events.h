// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_basic.h"
#include "offers_service_basics.h" //TODO: point to refactoring 
#include "rpc/core_rpc_server_commands_defs.h"
//#include "serialization/keyvalue_serialization_boost_variant.h"

#define CORE_EVENT_ADD_OFFER         "CORE_EVENT_ADD_OFFER"
#define CORE_EVENT_REMOVE_OFFER      "CORE_EVENT_REMOVE_OFFER"
#define CORE_EVENT_UPDATE_OFFER      "CORE_EVENT_UPDATE_OFFER"
#define CORE_EVENT_ADD_ALIAS         "CORE_EVENT_ADD_ALIAS"
#define CORE_EVENT_UPDATE_ALIAS      "CORE_EVENT_UPDATE_ALIAS"
#define CORE_EVENT_BLOCK_ADDED       "CORE_EVENT_BLOCK_ADDED"
#define CORE_EVENT_ADD_ASSET         "CORE_EVENT_ADD_ASSET"


namespace currency
{
  typedef boost::variant<bc_services::offer_details_ex, bc_services::update_offer_details, alias_rpc_details, update_alias_rpc_details, void_struct> core_event_v;

  struct core_event
  {
    std::string method;
    core_event_v details;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(method)
      //KV_SERIALIZE(details)
    END_KV_SERIALIZE_MAP()
  };



  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct i_core_event_handler
  {
    virtual void on_core_event(const std::string event_name, const core_event_v& e){};
    virtual void on_complete_events(){};
    virtual void on_clear_events(){};
  };
}