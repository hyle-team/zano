// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <QObject>
#ifndef Q_MOC_RUN
#include "warnings.h"

PUSH_VS_WARNINGS
DISABLE_VS_WARNINGS(4100)
DISABLE_VS_WARNINGS(4503)
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet/wallet_public_structs_defs.h"
#include "currency_core/offers_services_helpers.h"
#include "currency_core/basic_api_response_codes.h"
POP_VS_WARNINGS

#endif

namespace view
{

  /*slots structures*/
  struct transfer_destination
  {
    std::string address;
    std::string amount;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address)
      KV_SERIALIZE(amount)
    END_KV_SERIALIZE_MAP()
  };

  struct transfer_params
  {
    uint64_t wallet_id;
    std::list<transfer_destination> destinations;
    uint64_t mixin_count;
    uint64_t lock_time;
    std::string payment_id;
    std::string comment;
    uint64_t fee;
    bool push_payer;
    bool hide_receiver;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(destinations)
      KV_SERIALIZE(mixin_count)
      KV_SERIALIZE(lock_time)
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(comment)
      KV_SERIALIZE(fee)
      KV_SERIALIZE(push_payer)
      KV_SERIALIZE(hide_receiver)
    END_KV_SERIALIZE_MAP()
  };

  struct transfer_response
  {
    bool success;
    std::string tx_hash;
    uint64_t    tx_blob_size;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(success)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(tx_blob_size)
    END_KV_SERIALIZE_MAP()
  };


  enum ui_last_build_displaymode
  {
    ui_lb_dm_actual = 0,
    ui_lb_dm_new = 1,
    ui_lb_dm_new_alert_calm = 2,
    ui_lb_dm_new_alert_urgent = 3,
    ui_lb_dm_new_alert_critical = 4
  };




  struct block_info
  {
    uint64_t date;
    uint64_t    h;
    std::string type;
    std::string diff;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(date)
      KV_SERIALIZE(h)
      KV_SERIALIZE(type)
      KV_SERIALIZE(diff)
    END_KV_SERIALIZE_MAP()
  };


  struct switch_view_info
  {
    enum ui_views
    {
      ui_view_dashboard = 1,
      ui_view_wallet = 2
    };

    int view;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(view)
    END_KV_SERIALIZE_MAP()
  };

  /*signal structures*/
  struct daemon_status_info
  {
public:

    uint64_t daemon_network_state;
    uint64_t synchronization_start_height;
    uint64_t max_net_seen_height;
    uint64_t height;
    uint64_t out_connections_count;
    uint64_t inc_connections_count;
    int64_t net_time_delta_median;
    int64_t synchronized_connections_count;
    std::string pos_difficulty;
    std::string pow_difficulty;
    uint64_t hashrate;
    uint64_t last_build_displaymode;
    uint64_t alias_count;
    std::string last_build_available;
    //std::list<block_info> last_blocks;
    bool is_pos_allowed;
    uint64_t expiration_median_timestamp;
    bool is_disconnected;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(daemon_network_state)
      KV_SERIALIZE(synchronization_start_height)
      KV_SERIALIZE(synchronized_connections_count)
      KV_SERIALIZE(max_net_seen_height)
      KV_SERIALIZE(height)
      KV_SERIALIZE(out_connections_count)
      KV_SERIALIZE(inc_connections_count)
      KV_SERIALIZE(net_time_delta_median)
      KV_SERIALIZE(pos_difficulty)
      KV_SERIALIZE(pow_difficulty)
      KV_SERIALIZE(hashrate)
      KV_SERIALIZE(last_build_displaymode)
      KV_SERIALIZE(last_build_available)
      //KV_SERIALIZE(last_blocks)
      KV_SERIALIZE(alias_count)
      KV_SERIALIZE(is_pos_allowed)
      KV_SERIALIZE(expiration_median_timestamp)
      KV_SERIALIZE(is_disconnected)
    END_KV_SERIALIZE_MAP()
  };




  struct wallet_status_info
  {
    enum state
    {
      wallet_state_synchronizing = 1,
      wallet_state_ready = 2, 
      wallet_state_error = 3
    };


    uint64_t wallet_id;
    uint64_t wallet_state;
    bool is_mining;
    bool is_alias_operations_available;
    uint64_t balance;
    uint64_t unlocked_balance;
    uint64_t awaiting_in;
    uint64_t awaiting_out;
    uint64_t minied_total;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(wallet_state)
      KV_SERIALIZE(is_mining)
      KV_SERIALIZE(is_alias_operations_available)
      KV_SERIALIZE(balance)
      KV_SERIALIZE(unlocked_balance)
      KV_SERIALIZE(awaiting_in)
      KV_SERIALIZE(awaiting_out)
      KV_SERIALIZE(minied_total)
    END_KV_SERIALIZE_MAP()
  };  
  
  struct wallet_info
  {
    uint64_t unlocked_balance;
    uint64_t balance;
		uint64_t mined_total;
    std::string address;
    std::string tracking_hey;
    std::string path;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(unlocked_balance)
      KV_SERIALIZE(balance)
			KV_SERIALIZE(mined_total)			
      KV_SERIALIZE(address)
      KV_SERIALIZE(tracking_hey)
      KV_SERIALIZE(path)
    END_KV_SERIALIZE_MAP()
  };



  struct wallet_entry_info
  {
    wallet_info wi;
    uint64_t    wallet_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wi)
      KV_SERIALIZE(wallet_id)
    END_KV_SERIALIZE_MAP()

  };



  struct wallet_id_obj
  {
    uint64_t wallet_id;
    
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
    END_KV_SERIALIZE_MAP()
  };

  struct request_mining_estimate
  {
    uint64_t amount_coins;
    uint64_t time;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount_coins)
      KV_SERIALIZE(time)
    END_KV_SERIALIZE_MAP()
  };

  struct response_mining_estimate
  {
    uint64_t final_amount;
    uint64_t all_coins_and_pos_diff_rate;
    std::vector<uint64_t> days_estimate;
    std::string error_code;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(final_amount)
      KV_SERIALIZE(all_coins_and_pos_diff_rate)
      KV_SERIALIZE(days_estimate)
      KV_SERIALIZE(error_code)
    END_KV_SERIALIZE_MAP()
  };

  struct backup_keys_request
  {

    uint64_t wallet_id;
    std::string path;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(path)
    END_KV_SERIALIZE_MAP()
  };

  struct system_filedialog_request
  {
    std::string default_dir;
    std::string caption;
    std::string filemask;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(caption)
      KV_SERIALIZE(filemask)
      KV_SERIALIZE(default_dir)
    END_KV_SERIALIZE_MAP()
  };

  struct system_filedialog_response
  {
    std::string error_code;
    std::string path;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(error_code)
      KV_SERIALIZE(path)
    END_KV_SERIALIZE_MAP()
  };




  struct push_offer_param
  {
    uint64_t wallet_id;
    bc_services::offer_details_ex od;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(od)
    END_KV_SERIALIZE_MAP()
  };


  struct cancel_offer_param
  {
    uint64_t wallet_id;
    crypto::hash tx_id;
    uint64_t no;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE_POD_AS_HEX_STRING_N(tx_id, "id")
      KV_SERIALIZE_N(no, "oi")
    END_KV_SERIALIZE_MAP()
  };



  struct transfer_event_info
  {
    tools::wallet_public::wallet_transfer_info ti;
    uint64_t unlocked_balance;
    uint64_t balance;
		uint64_t total_mined;
    uint64_t wallet_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(ti)
      KV_SERIALIZE(unlocked_balance)
			KV_SERIALIZE(balance)
			KV_SERIALIZE(total_mined)
      KV_SERIALIZE(wallet_id)
    END_KV_SERIALIZE_MAP()
  };

  struct transfers_array
  {
    std::vector<tools::wallet_public::wallet_transfer_info> unconfirmed;
    std::vector<tools::wallet_public::wallet_transfer_info> history;
    uint64_t total_history_items;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(unconfirmed)
      KV_SERIALIZE(history)
      KV_SERIALIZE(total_history_items)
    END_KV_SERIALIZE_MAP()

  };

  struct open_wallet_request
  {
    std::string pass;
    std::string path;
    uint64_t txs_to_return;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(pass)
      KV_SERIALIZE(path)
      KV_SERIALIZE(txs_to_return)
    END_KV_SERIALIZE_MAP()
  };

  struct get_recent_transfers_request
  {
    uint64_t wallet_id;
    uint64_t offset;
    uint64_t count;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(offset)
      KV_SERIALIZE(count)
    END_KV_SERIALIZE_MAP()
  };

  struct reset_pass_request
  {
    uint64_t wallet_id;
    std::string pass;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(pass)
    END_KV_SERIALIZE_MAP()
  };

  struct restore_wallet_request
  {
    std::string pass;
    std::string path;
    std::string restore_key;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(pass)
      KV_SERIALIZE(path)
      KV_SERIALIZE(restore_key)
    END_KV_SERIALIZE_MAP()
  };

  struct open_wallet_response
  {
    uint64_t wallet_id;
    transfers_array recent_history;
    wallet_info wi;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(recent_history)
      KV_SERIALIZE(wi)
    END_KV_SERIALIZE_MAP()
  };

  struct wallets_summary_info
  {
    std::list<wallet_entry_info> wallets;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallets)
    END_KV_SERIALIZE_MAP()
  };


  struct header_entry
  {
    std::string field;
    std::string val;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(field)
      KV_SERIALIZE(val)
    END_KV_SERIALIZE_MAP()
  };

  struct request_uri_params
  {
    std::string method;
    std::string data;
    std::vector<header_entry > headers;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(method)
      KV_SERIALIZE(data)
      KV_SERIALIZE(headers)
    END_KV_SERIALIZE_MAP()
  };

  struct password_data
  {
    std::string pass;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(pass)
    END_KV_SERIALIZE_MAP()
  };


  struct alias_set
  {
    std::list<currency::alias_rpc_details> aliases;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(aliases)
    END_KV_SERIALIZE_MAP()
  };

  struct request_alias_param
  {
    currency::alias_rpc_details alias;
    uint64_t wallet_id;
    uint64_t fee;
    uint64_t reward;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(alias)
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(fee)                             
      KV_SERIALIZE(reward)
    END_KV_SERIALIZE_MAP()
  };


  struct start_backend_params
  {

    BEGIN_KV_SERIALIZE_MAP()
    END_KV_SERIALIZE_MAP()
  };


  struct wallet_sync_progres_param
  {
    uint64_t wallet_id;
    uint64_t progress;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(progress)
    END_KV_SERIALIZE_MAP()
  };

  struct get_restore_info_response
  {
    std::string restore_key;
    std::string error_code;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(restore_key)
      KV_SERIALIZE(error_code)
    END_KV_SERIALIZE_MAP()
  };

  struct get_alias_coast_response
  {
    std::string error_code;
    uint64_t coast;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(coast)
      KV_SERIALIZE(error_code)
    END_KV_SERIALIZE_MAP()
  };


  struct print_text_param
  {
    std::string html_text;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(html_text)
    END_KV_SERIALIZE_MAP()
  };


  struct get_fav_offers_request
  {
    std::list<bc_services::offer_id> ids;
    bc_services::core_offers_filter filter;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(ids)
      KV_SERIALIZE(filter)
    END_KV_SERIALIZE_MAP()
  };

  struct set_localization_request
  {
    std::vector<std::string> strings;
    std::string language_title;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(strings)
      KV_SERIALIZE(language_title)
    END_KV_SERIALIZE_MAP()

  };



  struct wallet_and_contract_id_param
  {
    uint64_t wallet_id;
    crypto::hash contract_id;


    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE_POD_AS_HEX_STRING(contract_id)
    END_KV_SERIALIZE_MAP()
  };

  struct release_contract_param : public wallet_and_contract_id_param
  {
    std::string release_type;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(release_type)
      KV_CHAIN_BASE(wallet_and_contract_id_param)
    END_KV_SERIALIZE_MAP()

  };

  struct contract_and_fee_param : public wallet_and_contract_id_param
  {
    uint64_t fee;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(fee)
      KV_CHAIN_BASE(wallet_and_contract_id_param)
    END_KV_SERIALIZE_MAP()

  };

  struct crequest_cancel_contract_param : public contract_and_fee_param
  {
    uint64_t expiration_period;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(expiration_period)
      KV_CHAIN_BASE(contract_and_fee_param)
    END_KV_SERIALIZE_MAP()
  };

  struct create_proposal_param_gui : public tools::wallet_public::create_proposal_param
  {
    uint64_t wallet_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_CHAIN_BASE(tools::wallet_public::create_proposal_param)
    END_KV_SERIALIZE_MAP()
  };

  struct address_validation_response
  {
    std::string error_code;
    std::string payment_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(error_code)
      KV_SERIALIZE(payment_id)
    END_KV_SERIALIZE_MAP()
  };


  struct gui_options
  {
    bool use_debug_mode;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(use_debug_mode)
    END_KV_SERIALIZE_MAP()

  };

  struct print_log_params
  {
    std::string msg;
    int log_level;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(msg)
      KV_SERIALIZE(log_level)
    END_KV_SERIALIZE_MAP()
  };


  struct api_response
  {
//    std::string request_id;
    std::string error_code;

    BEGIN_KV_SERIALIZE_MAP()
//      KV_SERIALIZE(request_id)
      KV_SERIALIZE(error_code)
    END_KV_SERIALIZE_MAP()
  };

  template<typename t_type>
  struct api_response_t
  {
    std::string error_code;
    t_type response_data;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(error_code)
      KV_SERIALIZE(response_data)
    END_KV_SERIALIZE_MAP()
  };

  template<typename t_type>
  struct api_request_t
  {
    uint64_t wallet_id;
    t_type req_data;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(req_data)
    END_KV_SERIALIZE_MAP()
  };




  struct api_void
  {
    BEGIN_KV_SERIALIZE_MAP()
    END_KV_SERIALIZE_MAP()
  };
  
  template<typename t_type>
  struct struct_with_one_t_type
  {
    t_type v;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(v)
    END_KV_SERIALIZE_MAP()
  };




#define API_RETURN_CODE_OK                                      BASIC_RESPONSE_STATUS_OK
#define API_RETURN_CODE_FAIL                                    BASIC_RESPONSE_STATUS_FAILED
#define API_RETURN_CODE_NOT_FOUND                               BASIC_RESPONSE_STATUS_NOT_FOUND
#define API_RETURN_CODE_ACCESS_DENIED                           "ACCESS_DENIED"
#define API_RETURN_CODE_INTERNAL_ERROR                          "INTERNAL_ERROR"
#define API_RETURN_CODE_NOT_ENOUGH_MONEY                        "NOT_ENOUGH_MONEY"
#define API_RETURN_CODE_INTERNAL_ERROR_QUE_FULL                 "INTERNAL_ERROR_QUE_FULL"
#define API_RETURN_CODE_BAD_ARG                                 "BAD_ARG"
#define API_RETURN_CODE_BAD_ARG_EMPTY_DESTINATIONS              "BAD_ARG_EMPTY_DESTINATIONS"
#define API_RETURN_CODE_BAD_ARG_WRONG_FEE                       "BAD_ARG_WRONG_FEE"
#define API_RETURN_CODE_BAD_ARG_INVALID_ADDRESS                 "BAD_ARG_INVALID_ADDRESS"
#define API_RETURN_CODE_BAD_ARG_WRONG_AMOUNT                    "BAD_ARG_WRONG_AMOUNT"
#define API_RETURN_CODE_BAD_ARG_WRONG_PAYMENT_ID                "BAD_ARG_WRONG_PAYMENT_ID"
#define API_RETURN_CODE_WRONG_PASSWORD                          "WRONG_PASSWORD"
#define API_RETURN_CODE_WALLET_WRONG_ID                         "WALLET_WRONG_ID"
#define API_RETURN_CODE_FILE_NOT_FOUND                          "FILE_NOT_FOUND"
#define API_RETURN_CODE_ALREADY_EXISTS                          "ALREADY_EXISTS"
#define API_RETURN_CODE_CANCELED                                "CANCELED"
#define API_RETURN_CODE_FILE_RESTORED                           "FILE_RESTORED"
#define API_RETURN_CODE_TRUE                                    "TRUE"
#define API_RETURN_CODE_FALSE                                   "FALSE"
#define API_RETURN_CODE_CORE_BUSY                               "CORE_BUSY"
#define API_RETURN_CODE_OVERFLOW                                "OVERFLOW"

#define API_MAX_ALIASES_COUNT                                   10000

  struct i_view
  {
    virtual bool update_daemon_status(const daemon_status_info& info){ return true; }
    virtual bool on_backend_stopped(){ return true; }
    virtual bool show_msg_box(const std::string& message){ return true; }
    virtual bool update_wallet_status(const wallet_status_info& wsi){ return true; }
    virtual bool update_wallets_info(const wallets_summary_info& wsi){ return true; }
    virtual bool money_transfer(const transfer_event_info& wsi){ return true; }
    virtual bool wallet_sync_progress(const view::wallet_sync_progres_param& p){ return true; }
    virtual bool init(const std::string& path){ return true; }
    virtual bool pos_block_found(const currency::block& block_found){ return true; }
    virtual bool money_transfer_cancel(const transfer_event_info& wsi){ return true; }
    virtual bool set_options(const gui_options& opt){ return true; }
  };

}
