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

namespace tools
{
  //-----------------------------------------------------------------------------------
  const command_line::arg_descriptor<std::string> wallet_rpc_server::
    = {"rpc-bind-port", "Starts wallet as rpc server for wallet operations, sets bind port for server", "", true};
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_rpc_bind_ip = {"rpc-bind-ip", "Specify ip to bind rpc server", "127.0.0.1"};
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_miner_text_info = { "miner-text-info", "Wallet password", "", true };

  void wallet_rpc_server::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_miner_text_info);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::wallet_rpc_server(wallet2& w):m_wallet(w), m_do_mint(false)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::run(bool do_mint, bool offline_mode)
  {
    m_do_mint = do_mint;

    if (!offline_mode)
    {
      m_net_server.add_idle_handler([this]() {
        size_t blocks_fetched = 0;
        bool received_money = false;
        bool ok;
        std::atomic<bool> stop(false);
        m_wallet.refresh(blocks_fetched, received_money, ok, stop);
        if (stop)
          return true;

        if (m_do_mint)
          m_wallet.try_mint_pos();
        return true;
      }, 2000);
    }

    //DO NOT START THIS SERVER IN MORE THEN 1 THREADS WITHOUT REFACTORING
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::run(1, true);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_bind_ip = command_line::get_arg(vm, arg_rpc_bind_ip);
    m_port = command_line::get_arg(vm, arg_rpc_bind_port);
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
  bool wallet_rpc_server::on_getbalance(const wallet_rpc::COMMAND_RPC_GET_BALANCE::request& req, wallet_rpc::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, connection_context& cntx)
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
  bool wallet_rpc_server::on_getaddress(const wallet_rpc::COMMAND_RPC_GET_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
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
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_transfer(const wallet_rpc::COMMAND_RPC_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
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
      m_wallet.transfer(dsts, req.mixin, req.unlock_time, req.fee, extra, attachments, detail::ssi_digit, tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, CURRENCY_TO_KEY_OUT_RELAXED, true, 0, true, &signed_tx_blob_str);
      if (m_wallet.is_watch_only())
      {
        res.tx_unsigned_hex = epee::string_tools::buff_to_hex_nodelimer(signed_tx_blob_str); // watch-only wallets can't sign and relay transactions
        // leave res.tx_hash empty, because tx has will change after signing
      }
      else
      {
        res.tx_hash = epee::string_tools::pod_to_hex(currency::get_transaction_hash(tx));
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
  bool wallet_rpc_server::on_store(const wallet_rpc::COMMAND_RPC_STORE::request& req, wallet_rpc::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    try
    {
      m_wallet.store();
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
  bool wallet_rpc_server::on_get_payments(const wallet_rpc::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx)
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
      wallet_rpc::payment_details rpc_payment;
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
  bool wallet_rpc_server::on_get_bulk_payments(const wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx)
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
        wallet_rpc::payment_details rpc_payment;
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
  bool wallet_rpc_server::on_make_integrated_address(const wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
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
  bool wallet_rpc_server::on_split_integrated_address(const wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
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
  bool wallet_rpc_server::on_sign_transfer(const wallet_rpc::COMMAND_SIGN_TRANSFER::request& req, wallet_rpc::COMMAND_SIGN_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    try
    {
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
  bool wallet_rpc_server::on_submit_transfer(const wallet_rpc::COMMAND_SUBMIT_TRANSFER::request& req, wallet_rpc::COMMAND_SUBMIT_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    std::string tx_signed_blob;
    if (!string_tools::parse_hexstr_to_binbuff(req.tx_signed_hex, tx_signed_blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "tx_signed_hex is invalid";
      return false;
    }

    try
    {
      currency::transaction tx = AUTO_VAL_INIT(tx);
      m_wallet.submit_transfer(tx_signed_blob, tx);
      res.tx_hash = epee::string_tools::pod_to_hex(currency::get_transaction_hash(tx));
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

} // namespace tools
