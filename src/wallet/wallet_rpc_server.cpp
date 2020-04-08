// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "include_base_utils.h"
using namespace epee;

#include "wallet_rpc_server.h"
#include "common/command_line.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/account.h"
#include "misc_language.h"
#include "crypto/hash.h"
#include "wallet_rpc_server_error_codes.h"

#define WALLET_RPC_BEGIN_TRY_ENTRY()     try {
#define WALLET_RPC_CATCH_TRY_ENTRY()     } \
        catch (const std::exception& e) \
        { \
          er.code = WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR; \
          er.message = e.what(); \
          return false; \
        } \
        catch (...) \
        { \
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR; \
          er.message = "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR"; \
          return false; \
        } 


namespace tools
{
  //-----------------------------------------------------------------------------------
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_rpc_bind_port = {"rpc-bind-port", "Starts wallet as rpc server for wallet operations, sets bind port for server", "", true};
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_rpc_bind_ip = {"rpc-bind-ip", "Specify ip to bind rpc server", "127.0.0.1"};
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_miner_text_info = { "miner-text-info", "Wallet password", "", true };
  const command_line::arg_descriptor<bool>        wallet_rpc_server::arg_deaf_mode = { "deaf", "Put wallet into 'deaf' mode make it ignore any rpc commands(usable for safe PoS mining)", false, true };

  void wallet_rpc_server::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_miner_text_info);
    command_line::add_arg(desc, arg_deaf_mode);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::wallet_rpc_server(wallet2& w):m_wallet(w), m_do_mint(false), m_deaf(false)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::run(bool do_mint, bool offline_mode, const currency::account_public_address& miner_address)
  {
    static const uint64_t wallet_rpc_idle_work_period_ms = 2000;

    m_do_mint = do_mint;

    if (!offline_mode)
    {
      m_net_server.add_idle_handler([this, &miner_address]() -> bool
      {
        try
        {
          size_t blocks_fetched = 0;
          bool received_money = false, ok = false;
          std::atomic<bool> stop(false);
          LOG_PRINT_L2("wallet RPC idle: refreshing...");
          m_wallet.refresh(blocks_fetched, received_money, ok, stop);
          if (stop)
          {
            LOG_PRINT_L1("wallet RPC idle: refresh failed");
            return true;
          }

          bool has_related_alias_in_unconfirmed = false;
          LOG_PRINT_L2("wallet RPC idle: scanning tx pool...");
          m_wallet.scan_tx_pool(has_related_alias_in_unconfirmed);

          if (m_do_mint)
          {
            LOG_PRINT_L2("wallet RPC idle: trying to do PoS iteration...");
            m_wallet.try_mint_pos(miner_address);
          }
        }
        catch (error::no_connection_to_daemon&)
        {
          LOG_PRINT_RED("no connection to the daemon", LOG_LEVEL_0);
        }
        catch(std::exception& e)
        {
          LOG_ERROR("exeption caught in wallet_rpc_server::idle_handler: " << e.what());
        }
        catch(...)
        {
          LOG_ERROR("unknown exeption caught in wallet_rpc_server::idle_handler");
        }

        return true;
      }, wallet_rpc_idle_work_period_ms);
    }

    //DO NOT START THIS SERVER IN MORE THEN 1 THREADS WITHOUT REFACTORING
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::run(1, true);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_bind_ip = command_line::get_arg(vm, arg_rpc_bind_ip);
    m_port = command_line::get_arg(vm, arg_rpc_bind_port);
    m_deaf = command_line::get_arg(vm, arg_deaf_mode);
    if (m_deaf)
    {
      LOG_PRINT_MAGENTA("Wallet launched in 'deaf' mode", LOG_LEVEL_0);
    }

    if (command_line::has_arg(vm, arg_miner_text_info))
    {
      m_wallet.set_miner_text_info(command_line::get_arg(vm, arg_miner_text_info));
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::init(const boost::program_options::variables_map& vm)
  {
    m_net_server.set_threads_prefix("RPC");
    bool r = handle_command_line(vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to process command line in core_rpc_server");
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::init(m_port, m_bind_ip);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response, connection_context& m_conn_context)
  {
    response.m_response_code = 200; 
    response.m_response_comment = "Ok"; 
    std::string reference_stub; 
    bool call_found = false; 
    if (m_deaf)
    {
      response.m_response_code = 500; 
      response.m_response_comment = "Internal Server Error"; 
      return true;
    }
    if (!handle_http_request_map(query_info, response, m_conn_context, call_found, reference_stub) && response.m_response_code == 200)
    {
      response.m_response_code = 500; 
      response.m_response_comment = "Internal Server Error"; 
      return true;
    }
    if (!call_found)
    {
      response.m_response_code = 404; 
      response.m_response_comment = "Not Found"; 
      return true;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getbalance(const wallet_public::COMMAND_RPC_GET_BALANCE::request& req, wallet_public::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    try
    {
      res.balance = m_wallet.balance();
      res.unlocked_balance = m_wallet.unlocked_balance();
    }
    catch (std::exception& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getaddress(const wallet_public::COMMAND_RPC_GET_ADDRESS::request& req, wallet_public::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    try
    {
      res.address = m_wallet.get_account().get_public_address_str();
    }
    catch (std::exception& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  bool wallet_rpc_server::on_getwallet_info(const wallet_public::COMMAND_RPC_GET_WALLET_INFO::request& req, wallet_public::COMMAND_RPC_GET_WALLET_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    try
    {
      res.address = m_wallet.get_account().get_public_address_str();
      res.is_whatch_only = m_wallet.is_watch_only();
      res.path = epee::string_encoding::convert_to_ansii(m_wallet.get_wallet_path());
      res.transfers_count = m_wallet.get_recent_transfers_total_count();
      res.transfer_entries_count = m_wallet.get_transfer_entries_count();
      res.seed = m_wallet.get_account().get_restore_braindata();
      std::map<uint64_t, uint64_t> distribution;
      m_wallet.get_utxo_distribution(distribution);
      for (const auto& ent : distribution)
        res.utxo_distribution.push_back(currency::print_money_brief(ent.first) + ":" + std::to_string(ent.second));
      
      return true;
    }
    catch (std::exception& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
  }
  bool wallet_rpc_server::on_get_recent_txs_and_info(const wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::request& req, wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    try
    {
      if (req.update_provision_info)
      {
        res.pi.balance = m_wallet.balance(res.pi.unlocked_balance);
        res.pi.transfer_entries_count = m_wallet.get_transfer_entries_count();
        res.pi.transfers_count = m_wallet.get_recent_transfers_total_count();
      }

      if (req.offset == 0)
        m_wallet.get_unconfirmed_transfers(res.transfers);
      
      m_wallet.get_recent_transfers_history(res.transfers, req.offset, req.count, res.total_transfers);

      return true;
    }
    catch (std::exception& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_transfer(const wallet_public::COMMAND_RPC_TRANSFER::request& req, wallet_public::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    std::string payment_id;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.payment_id, payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("invalid encoded payment id: ") + req.payment_id + ", hex-encoded string was expected";
      return false;
    }

    std::vector<currency::tx_destination_entry> dsts;
    for (auto it = req.destinations.begin(); it != req.destinations.end(); it++) 
    {
      currency::tx_destination_entry de;
      de.addr.resize(1);
      std::string embedded_payment_id;
      if(!m_wallet.get_transfer_address(it->address, de.addr.back(), embedded_payment_id))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
        er.message = std::string("WALLET_RPC_ERROR_CODE_WRONG_ADDRESS: ") + it->address;
        return false;
      }
      if (embedded_payment_id.size() != 0)
      {
        if (payment_id.size() != 0)
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = std::string("embedded payment id: ") + embedded_payment_id + " conflicts with previously set payment id: " + payment_id;
          return false;
        }
        payment_id = embedded_payment_id;
      }
      de.amount = it->amount;
      dsts.push_back(de);
    }
    try
    {
      std::vector<currency::attachment_v> attachments; 
      if (!payment_id.empty() && !currency::set_payment_id_to_tx(attachments, payment_id))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = std::string("payment id ") + payment_id + " is invalid and can't be set";
        return false;
      }

      if (!req.comment.empty())
      {
        currency::tx_comment comment = AUTO_VAL_INIT(comment);
        comment.comment = req.comment;
        attachments.push_back(comment);
      }

      currency::transaction tx;
      std::vector<currency::extra_v> extra;
      std::string signed_tx_blob_str;
      m_wallet.transfer(dsts, req.mixin, 0/*req.unlock_time*/, req.fee, extra, attachments, detail::ssi_digit, tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, CURRENCY_TO_KEY_OUT_RELAXED, true, 0, true, &signed_tx_blob_str);
      if (m_wallet.is_watch_only())
      {
        res.tx_unsigned_hex = epee::string_tools::buff_to_hex_nodelimer(signed_tx_blob_str); // watch-only wallets can't sign and relay transactions
        // leave res.tx_hash empty, because tx has will change after signing
      }
      else
      {
        res.tx_hash = epee::string_tools::pod_to_hex(currency::get_transaction_hash(tx));
        res.tx_size = get_object_blobsize(tx);
      }
      return true;
    }
    catch (const tools::error::daemon_busy& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_DAEMON_IS_BUSY;
      er.message = e.what();
      return false; 
    }
    catch (const std::exception& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR;
      er.message = e.what();
      return false; 
    }
    catch (...)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR";
      return false; 
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_store(const wallet_public::COMMAND_RPC_STORE::request& req, wallet_public::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    m_wallet.store();
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_payments(const wallet_public::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    std::string payment_id;
    if (!currency::parse_payment_id_from_hex_str(req.payment_id, payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("invalid payment id given: \'") + req.payment_id + "\', hex-encoded string was expected";
      return false;
    }

    res.payments.clear();
    std::list<wallet2::payment_details> payment_list;
    m_wallet.get_payments(payment_id, payment_list);
    for (auto payment : payment_list)
    {
      if (payment.m_unlock_time && !req.allow_locked_transactions)
      {
        //check that transaction don't have locking for time longer then 10 blocks ahead
        //TODO: add code for "unlock_time" set as timestamp, now it's all being filtered
        if (payment.m_unlock_time > payment.m_block_height + WALLET_DEFAULT_TX_SPENDABLE_AGE)
          continue;
      }
      wallet_public::payment_details rpc_payment;
      rpc_payment.payment_id   = req.payment_id;
      rpc_payment.tx_hash      = epee::string_tools::pod_to_hex(payment.m_tx_hash);
      rpc_payment.amount       = payment.m_amount;
      rpc_payment.block_height = payment.m_block_height;
      rpc_payment.unlock_time  = payment.m_unlock_time;
      res.payments.push_back(rpc_payment);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_bulk_payments(const wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    res.payments.clear();

    for (auto & payment_id_str : req.payment_ids)
    {
      currency::payment_id_t payment_id;
      if (!currency::parse_payment_id_from_hex_str(payment_id_str, payment_id))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = std::string("invalid payment id given: \'") + payment_id_str + "\', hex-encoded string was expected";
        return false;
      }

      std::list<wallet2::payment_details> payment_list;
      m_wallet.get_payments(payment_id, payment_list, req.min_block_height);

      for (auto & payment : payment_list)
      {
        if (payment.m_unlock_time && !req.allow_locked_transactions)
        {
          //check that transaction don't have locking for time longer then 10 blocks ahead
          //TODO: add code for "unlock_time" set as timestamp, now it's all being filtered
          if (payment.m_unlock_time > payment.m_block_height + WALLET_DEFAULT_TX_SPENDABLE_AGE)
            continue;
        }

        wallet_public::payment_details rpc_payment;
        rpc_payment.payment_id = payment_id_str;
        rpc_payment.tx_hash = epee::string_tools::pod_to_hex(payment.m_tx_hash);
        rpc_payment.amount = payment.m_amount;
        rpc_payment.block_height = payment.m_block_height;
        rpc_payment.unlock_time = payment.m_unlock_time;
        res.payments.push_back(std::move(rpc_payment));
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_make_integrated_address(const wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    std::string payment_id;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.payment_id, payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("invalid payment id given: \'") + req.payment_id + "\', hex-encoded string was expected";
      return false;
    }

    if (!currency::is_payment_id_size_ok(payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("given payment id is too long: \'") + req.payment_id + "\'";
      return false;
    }

    if (payment_id.empty())
    {
      payment_id = std::string(8, ' ');
      crypto::generate_random_bytes(payment_id.size(), &payment_id.front());
    }

    res.integrated_address = currency::get_account_address_and_payment_id_as_str(m_wallet.get_account().get_public_address(), payment_id);
    res.payment_id = epee::string_tools::buff_to_hex_nodelimer(payment_id);

    return !res.integrated_address.empty();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_split_integrated_address(const wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    currency::account_public_address addr;
    std::string payment_id;
    if (!currency::get_account_address_and_payment_id_from_str(addr, payment_id, req.integrated_address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = std::string("invalid integrated address given: \'") + req.integrated_address + "\'";
      return false;
    }

    res.standard_address = currency::get_account_address_as_str(addr);
    res.payment_id = epee::string_tools::buff_to_hex_nodelimer(payment_id);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sweep_below(const wallet_public::COMMAND_SWEEP_BELOW::request& req, wallet_public::COMMAND_SWEEP_BELOW::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    currency::payment_id_t payment_id;
    if (!req.payment_id_hex.empty() && !currency::parse_payment_id_from_hex_str(req.payment_id_hex, payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("Invalid payment id: ") + req.payment_id_hex;
      return false;
    }

    currency::account_public_address addr;
    currency::payment_id_t integrated_payment_id;
    if (!m_wallet.get_transfer_address(req.address, addr, integrated_payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = std::string("Invalid address: ") + req.address;
      return false;
    }

    if (!integrated_payment_id.empty())
    {
      if (!payment_id.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = std::string("address ") + req.address + " has integrated payment id " + epee::string_tools::buff_to_hex_nodelimer(integrated_payment_id) +
          " which is incompatible with payment id " + epee::string_tools::buff_to_hex_nodelimer(payment_id) + " that was already assigned to this transfer";
        return false;
      }
      payment_id = integrated_payment_id;
    }

    if (req.fee < m_wallet.get_core_runtime_config().tx_pool_min_fee)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = std::string("Given fee is too low: ") + epee::string_tools::num_to_string_fast(req.fee) + ", minimum is: " + epee::string_tools::num_to_string_fast(m_wallet.get_core_runtime_config().tx_pool_min_fee);
      return false;
    }

    try
    {
      currency::transaction tx = AUTO_VAL_INIT(tx);
      size_t outs_total = 0, outs_swept = 0;
      uint64_t amount_total = 0, amount_swept = 0;

      std::string unsigned_tx_blob_str;
      m_wallet.sweep_below(req.mixin, addr, req.amount, payment_id, req.fee, outs_total, amount_total, outs_swept, &tx, &unsigned_tx_blob_str);

      get_inputs_money_amount(tx, amount_swept);
      res.amount_swept = amount_swept;
      res.amount_total = amount_total;
      res.outs_swept = outs_swept;
      res.outs_total = outs_total;

      if (m_wallet.is_watch_only())
      {
        res.tx_unsigned_hex = epee::string_tools::buff_to_hex_nodelimer(unsigned_tx_blob_str); // watch-only wallets can't sign and relay transactions
        // leave res.tx_hash empty, because tx has will change after signing
      }
      else
      {
        res.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(tx));
      }
      
      
    }
    catch (const tools::error::daemon_busy& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_DAEMON_IS_BUSY;
      er.message = e.what();
      return false;
    }
    catch (const std::exception& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR;
      er.message = e.what();
      return false;
    }
    catch (...)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR";
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sign_transfer(const wallet_public::COMMAND_SIGN_TRANSFER::request& req, wallet_public::COMMAND_SIGN_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::transaction tx = AUTO_VAL_INIT(tx);
    std::string tx_unsigned_blob;
    if (!string_tools::parse_hexstr_to_binbuff(req.tx_unsigned_hex, tx_unsigned_blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "tx_unsigned_hex is invalid";
      return false;
    }
    std::string tx_signed_blob;
    m_wallet.sign_transfer(tx_unsigned_blob, tx_signed_blob, tx);

    res.tx_signed_hex = epee::string_tools::buff_to_hex_nodelimer(tx_signed_blob);
    res.tx_hash = epee::string_tools::pod_to_hex(currency::get_transaction_hash(tx));
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_submit_transfer(const wallet_public::COMMAND_SUBMIT_TRANSFER::request& req, wallet_public::COMMAND_SUBMIT_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    std::string tx_signed_blob;
    if (!string_tools::parse_hexstr_to_binbuff(req.tx_signed_hex, tx_signed_blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "tx_signed_hex is invalid";
      return false;
    }

    WALLET_RPC_BEGIN_TRY_ENTRY();    
    currency::transaction tx = AUTO_VAL_INIT(tx);
    m_wallet.submit_transfer(tx_signed_blob, tx);
    res.tx_hash = epee::string_tools::pod_to_hex(currency::get_transaction_hash(tx));
    WALLET_RPC_CATCH_TRY_ENTRY();

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_search_for_transactions(const wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::request& req, wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    bool tx_id_specified = req.tx_id != currency::null_hash;

    // process confirmed txs
    m_wallet.enumerate_transfers_history([&](const wallet_public::wallet_transfer_info& wti) -> bool {

      if (tx_id_specified)
      {
        if (wti.tx_hash != req.tx_id)
          return true; // continue
      }

      if (req.filter_by_height)
      {
        if (!wti.height) // unconfirmed
          return true; // continue

        if (wti.height < req.min_height)
        {
          // no need to scan more
          return false; // stop
        }
        if (wti.height > req.max_height)
        {
          return true; // continue
        }
      }
    
      if (wti.is_income && req.in)
        res.in.push_back(wti);

      if (!wti.is_income && req.out)
        res.out.push_back(wti);   

      return true; // continue
    }, false /* enumerate_forward */);

    // process unconfirmed txs
    if (req.pool)
    {
      m_wallet.enumerate_unconfirmed_transfers([&](const wallet_public::wallet_transfer_info& wti) -> bool {
        if ((wti.is_income && req.in) || (!wti.is_income && req.out))
          res.pool.push_back(wti);
        return true; // continue
      });
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_send_proposal(const wallet_public::COMMAND_CONTRACTS_SEND_PROPOSAL::request& req, wallet_public::COMMAND_CONTRACTS_SEND_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();       
    currency::transaction tx = AUTO_VAL_INIT(tx);
    currency::transaction template_tx = AUTO_VAL_INIT(template_tx);
    m_wallet.send_escrow_proposal(req, tx, template_tx);  
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_accept_proposal(const wallet_public::COMMAND_CONTRACTS_ACCEPT_PROPOSAL::request& req, wallet_public::COMMAND_CONTRACTS_ACCEPT_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY(); 
    m_wallet.accept_proposal(req.contract_id, req.acceptance_fee);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_get_all(const wallet_public::COMMAND_CONTRACTS_GET_ALL::request& req, wallet_public::COMMAND_CONTRACTS_GET_ALL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();    
    tools::wallet2::escrow_contracts_container ecc;
    m_wallet.get_contracts(ecc);
    res.contracts.resize(ecc.size());
    size_t i = 0;
    for (auto& c : ecc)
    {
      static_cast<tools::wallet_public::escrow_contract_details_basic&>(res.contracts[i]) = c.second;
      res.contracts[i].contract_id = c.first;
      i++;
    }
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_release(const wallet_public::COMMAND_CONTRACTS_RELEASE::request& req, wallet_public::COMMAND_CONTRACTS_RELEASE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();    
    m_wallet.finish_contract(req.contract_id, req.release_type);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_request_cancel(const wallet_public::COMMAND_CONTRACTS_REQUEST_CANCEL::request& req, wallet_public::COMMAND_CONTRACTS_REQUEST_CANCEL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    m_wallet.request_cancel_contract(req.contract_id, req.fee, req.expiration_period);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_accept_cancel(const wallet_public::COMMAND_CONTRACTS_ACCEPT_CANCEL::request& req, wallet_public::COMMAND_CONTRACTS_ACCEPT_CANCEL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();    
    m_wallet.accept_cancel_contract(req.contract_id);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_marketplace_get_my_offers(const wallet_public::COMMAND_MARKETPLACE_GET_MY_OFFERS::request& req, wallet_public::COMMAND_MARKETPLACE_GET_MY_OFFERS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  { 
    WALLET_RPC_BEGIN_TRY_ENTRY();
    m_wallet.get_actual_offers(res.offers);
    size_t offers_count_before_filtering = res.offers.size();
    bc_services::filter_offers_list(res.offers, req.filter, m_wallet.get_core_runtime_config().get_core_time());
    LOG_PRINT("get_my_offers(): " << res.offers.size() << " offers returned (" << offers_count_before_filtering << " was before filter)", LOG_LEVEL_1);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_marketplace_push_offer(const wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
    m_wallet.push_offer(req.od, res_tx);

    res.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    res.tx_blob_size = currency::get_object_blobsize(res_tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_marketplace_push_update_offer(const wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {

    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
    m_wallet.update_offer_by_id(req.tx_id, req.no, req.od, res_tx);

    res.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    res.tx_blob_size = currency::get_object_blobsize(res_tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_marketplace_cancel_offer(const wallet_public::COMMAND_MARKETPLACE_CANCEL_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_CANCEL_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
    m_wallet.cancel_offer_by_id(req.tx_id, req.no, req.fee, res_tx);

    res.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    res.tx_blob_size = currency::get_object_blobsize(res_tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }



} // namespace tools
