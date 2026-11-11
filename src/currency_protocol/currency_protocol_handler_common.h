// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "p2p/net_node_common.h"
#include "currency_protocol/currency_protocol_defs.h"
#include "currency_core/connection_context.h"
namespace currency
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct i_currency_protocol
  {
    virtual bool relay_block(NOTIFY_NEW_BLOCK::request& arg, currency_connection_context& exclude_context)=0;
    virtual bool relay_transactions(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& arg, currency_connection_context& exclude_context)=0;
    virtual void on_hardfork_activated(size_t hardfork_id) {}
    //virtual bool request_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, currency_connection_context& context)=0;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct currency_protocol_stub: public i_currency_protocol
  {
    virtual bool relay_block(NOTIFY_NEW_BLOCK::request& /*arg*/, currency_connection_context& /*exclude_context*/)
    {
      return false;
    }
    virtual bool relay_transactions(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& /*arg*/, currency_connection_context& /*exclude_context*/)
    {
      return false;
    }

  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct i_critical_error_handler
  {
    // called by currency protocol when the time is critically out of sync
    // return true if the error is not ignored and the called should not proceed
    virtual bool on_critical_time_sync_error() = 0;
    
    virtual bool on_critical_low_free_space(uint64_t available, uint64_t required) = 0;

    virtual bool on_immediate_stop_requested() = 0;
  };


}
