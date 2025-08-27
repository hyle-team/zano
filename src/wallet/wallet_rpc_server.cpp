// Copyright (c) 2014-2024 Zano Project
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
#include "wallet_helpers.h"
#include "wrap_service.h"
PUSH_VS_WARNINGS
DISABLE_VS_WARNINGS(4244)
#include "jwt-cpp/jwt.h"
POP_VS_WARNINGS
#include "crypto/bitcoin/sha256_helper.h"

#define JWT_TOKEN_EXPIRATION_MAXIMUM          (60 * 60 * 1000)
#define JWT_TOKEN_CLAIM_NAME_BODY_HASH        "body_hash"
#define JWT_TOKEN_CLAIM_NAME_SALT             "salt"
#define JWT_TOKEN_CLAIM_NAME_EXPIRATION       "exp"     
#define JWT_TOKEN_OVERWHELM_LIMIT             100000   // if there are more records in m_jwt_used_salts then we consider it as an attack     



#define GET_WALLET()   wallet_rpc_locker w(m_pwallet_provider);

#define WALLET_RPC_BEGIN_TRY_ENTRY()     try { GET_WALLET();
#define WALLET_RPC_CATCH_TRY_ENTRY()     } \
        catch (const tools::error::daemon_busy& e) \
        { \
          er.code = WALLET_RPC_ERROR_CODE_DAEMON_IS_BUSY; \
          er.message = std::string("WALLET_RPC_ERROR_CODE_DAEMON_IS_BUSY: ") + e.what(); \
          return false; \
        } \
        catch (const tools::error::not_enough_money& e) \
        { \
          er.code = WALLET_RPC_ERROR_CODE_NOT_ENOUGH_MONEY; \
          er.message = std::string("WALLET_RPC_ERROR_CODE_NOT_ENOUGH_MONEY: ") + e.what(); \
          return false; \
        } \
        catch (const tools::error::wallet_error& e) \
        { \
          er.code = WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR; \
          er.message = std::string("WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR: ") + e.what(); \
          return false; \
        } \
        catch (const std::exception& e) \
        { \
          er.code = WALLET_RPC_ERROR_CODE_GENERIC_ERROR; \
          er.message = std::string("WALLET_RPC_ERROR_CODE_GENERIC_ERROR: ") + e.what(); \
          return false; \
        } \
        catch (...) \
        { \
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR; \
          er.message = "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR"; \
          return false; \
        } 


void exception_handler()
{}

namespace tools
{
  //-----------------------------------------------------------------------------------
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_rpc_bind_port    ("rpc-bind-port",   "Starts wallet as rpc server for wallet operations, sets bind port for server");
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_rpc_bind_ip      ("rpc-bind-ip",     "Specify ip to bind rpc server", "127.0.0.1");
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_miner_text_info  ("miner-text-info", "Wallet password");
  const command_line::arg_descriptor<bool>        wallet_rpc_server::arg_deaf_mode        ("deaf",            "Put wallet into 'deaf' mode make it ignore any rpc commands(usable for safe PoS mining)");
  const command_line::arg_descriptor<std::string> wallet_rpc_server::arg_jwt_secret       ("jwt-secret",      "Enables JWT auth over secret string provided");

  void wallet_rpc_server::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_miner_text_info);
    command_line::add_arg(desc, arg_deaf_mode);
    command_line::add_arg(desc, arg_jwt_secret);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::wallet_rpc_server(std::shared_ptr<wallet2> wptr):
    m_pwallet_provider_sh_ptr(new wallet_provider_simple(wptr))
    , m_pwallet_provider(m_pwallet_provider_sh_ptr.get())
    , m_do_mint(false)
    , m_deaf(false)
    , m_last_wallet_store_height(0)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::wallet_rpc_server(i_wallet_provider* provider_ptr):
    m_pwallet_provider(provider_ptr)
    , m_do_mint(false)
    , m_deaf(false)
    , m_last_wallet_store_height(0)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
//   std::shared_ptr<wallet2> wallet_rpc_server::get_wallet()
//   {
//     return std::shared_ptr<wallet2>(m_pwallet);
//   }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::run(bool do_mint, bool offline_mode, const currency::account_public_address& miner_address)
  {
    static const uint64_t wallet_rpc_idle_work_period_ms = 2000;

    m_do_mint = do_mint;
    if (m_do_mint)
      LOG_PRINT_CYAN("PoS mining is ON", LOG_LEVEL_0);

    if (!offline_mode)
    {
      m_net_server.add_idle_handler([this, &miner_address]() -> bool
      {
        try
        {
          GET_WALLET();          
          size_t blocks_fetched = 0;
          bool received_money = false, ok = false;
          std::atomic<bool> stop(false);
          LOG_PRINT_L2("wallet RPC idle: refreshing...");
          w.get_wallet()->refresh(blocks_fetched, received_money, ok, stop);
          if (stop)
          {
            LOG_PRINT_L1("wallet RPC idle: refresh failed");
            return true;
          }

          bool has_related_alias_in_unconfirmed = false;
          LOG_PRINT_L2("wallet RPC idle: scanning tx pool...");
          w.get_wallet()->scan_tx_pool(has_related_alias_in_unconfirmed);

          if (m_do_mint)
          {
            LOG_PRINT_L2("wallet RPC idle: trying to do PoS iteration...");
            w.get_wallet()->try_mint_pos(miner_address);
          }

          //auto-store wallet in server mode, let's do it every 24-hour
          if (w.get_wallet()->get_top_block_height() < m_last_wallet_store_height)
          {
            LOG_ERROR("Unexpected m_last_wallet_store_height = " << m_last_wallet_store_height << " or " << w.get_wallet()->get_top_block_height());
          }
          else if (w.get_wallet()->get_top_block_height() - m_last_wallet_store_height > CURRENCY_BLOCKS_PER_DAY)
          {
            //store wallet
            w.get_wallet()->store();
            m_last_wallet_store_height = w.get_wallet()->get_top_block_height();
          }
        }
        catch (error::no_connection_to_daemon&)
        {
          LOG_PRINT_RED("no connection to the daemon", LOG_LEVEL_0);
        }
        catch (std::exception& e)
        {
          LOG_ERROR("exeption caught in wallet_rpc_server::idle_handler: " << e.what());
        }
        catch (...)
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
    GET_WALLET();
    m_bind_ip = command_line::get_arg(vm, arg_rpc_bind_ip);
    m_port = command_line::get_arg(vm, arg_rpc_bind_port);
    m_deaf = command_line::get_arg(vm, arg_deaf_mode);
    if (m_deaf)
    {
      LOG_PRINT_MAGENTA("Wallet launched in 'deaf' mode", LOG_LEVEL_0);
    }

    if (command_line::has_arg(vm, arg_miner_text_info))
    {
      w.get_wallet()->set_miner_text_info(command_line::get_arg(vm, arg_miner_text_info));
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::init(const boost::program_options::variables_map& vm)
  {
    GET_WALLET();
    m_last_wallet_store_height = w.get_wallet()->get_top_block_height();
    m_net_server.set_threads_prefix("RPC");
    bool r = handle_command_line(vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to process command line in core_rpc_server");


    if(command_line::has_arg(vm, arg_jwt_secret))
    {
      m_jwt_secret = command_line::get_arg(vm, arg_jwt_secret);
    }
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::init(m_port, m_bind_ip);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::auth_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response, connection_context& conn_context)
  {

    auto it = std::find_if(query_info.m_header_info.m_etc_fields.begin(), query_info.m_header_info.m_etc_fields.end(), [](const auto& element)
                           { return element.first == ZANO_ACCESS_TOKEN; });
    if(it == query_info.m_header_info.m_etc_fields.end())
      return false;
    
    try
    {
      if(m_jwt_used_salts.get_set().size() > JWT_TOKEN_OVERWHELM_LIMIT)
      {
        throw std::runtime_error("Salt is overwhelmed");
      }

      auto decoded = jwt::decode(it->second, [](const std::string& str)
                                 { return jwt::base::decode<jwt::alphabet::base64>(jwt::base::pad<jwt::alphabet::base64>(str)); });


      auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256 { m_jwt_secret });

      verifier.verify(decoded);
      std::string body_hash     = decoded.get_payload_claim(JWT_TOKEN_CLAIM_NAME_BODY_HASH).as_string();
      std::string salt      = decoded.get_payload_claim(JWT_TOKEN_CLAIM_NAME_SALT).as_string();
      crypto::hash jwt_claim_sha256 = currency::null_hash;
      epee::string_tools::hex_to_pod(body_hash, jwt_claim_sha256);
      crypto::hash sha256 = crypto::sha256_hash(query_info.m_body.data(), query_info.m_body.size());
      if (sha256 != jwt_claim_sha256)
      {
        throw std::runtime_error("Body hash missmatch");
      }
      if(m_jwt_used_salts.get_set().find(salt) != m_jwt_used_salts.get_set().end())
      {
        throw std::runtime_error("Salt reused");
      }

      uint64_t ticks_now = epee::misc_utils::get_tick_count();
      m_jwt_used_salts.add(salt, ticks_now + JWT_TOKEN_EXPIRATION_MAXIMUM);
      m_jwt_used_salts.remove_if_expiration_less_than(ticks_now);

      LOG_PRINT_L3("JWT token OK");
      return true;
    }
    catch(const std::exception& e)
    {
      LOG_ERROR("Invalid JWT token: " << e.what());
      return false;
    }


    return false;

  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response, epee::net_utils::connection_context_base& conn_context, bool& call_found, documentation& docs)
  {
    
    //#ifndef _DEBUG
    if (m_jwt_secret.size() && conn_context.m_connection_id != RPC_INTERNAL_UI_CONTEXT)
    {
      if (!auth_http_request(query_info, response, conn_context))
      {
        call_found = true;
        response.m_response_code    = 401;
        response.m_response_comment = "Unauthorized";
        return true;
      }
    }
    //#endif

    response.m_response_code = 200;
    response.m_response_comment = "Ok";
    std::string reference_stub;
    if (m_deaf)
    {
      response.m_response_code = 500;
      response.m_response_comment = "Internal Server Error";
      return true;
    }
    if (!handle_http_request_map(query_info, response, conn_context, call_found, docs) && response.m_response_code == 200)
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
  void wallet_rpc_server::set_jwt_secret(const std::string& jwt)
  {
    m_jwt_secret = jwt;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  const std::string& wallet_rpc_server::get_jwt_secret()
  {
    return m_jwt_secret;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getbalance(const wallet_public::COMMAND_RPC_GET_BALANCE::request& req, wallet_public::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    uint64_t stub_mined = 0; // unused
    bool r = w.get_wallet()->balance(res.balances, stub_mined);
    CHECK_AND_ASSERT_THROW_MES(r, "m_wallet.balance failed");
    for (auto it = res.balances.begin(); it != res.balances.end(); ++it)
    {
      if (it->asset_info.asset_id == currency::native_coin_asset_id)
      {
        res.balance = it->total;
        res.unlocked_balance = it->unlocked;
        break;
      }
    }
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getaddress(const wallet_public::COMMAND_RPC_GET_ADDRESS::request& req, wallet_public::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    res.address = w.get_wallet()->get_account().get_public_address_str();
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getwallet_info(const wallet_public::COMMAND_RPC_GET_WALLET_INFO::request& req, wallet_public::COMMAND_RPC_GET_WALLET_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();

    res.address = w.get_wallet()->get_account().get_public_address_str();
    res.is_whatch_only = w.get_wallet()->is_watch_only();
    res.path = epee::string_encoding::convert_to_ansii(w.get_wallet()->get_wallet_path());
    res.transfers_count = w.get_wallet()->get_recent_transfers_total_count();
    res.transfer_entries_count = w.get_wallet()->get_transfer_entries_count();
    std::map<uint64_t, uint64_t> distribution;
    w.get_wallet()->get_utxo_distribution(distribution);
    for (const auto& ent : distribution)
      res.utxo_distribution.push_back(currency::print_money_brief(ent.first) + ":" + std::to_string(ent.second));

    res.current_height = w.get_wallet()->get_top_block_height();
    res.has_bare_unspent_outputs = w.get_wallet()->has_bare_unspent_outputs();
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getwallet_restore_info(const wallet_public::COMMAND_RPC_GET_WALLET_RESTORE_INFO::request& req, wallet_public::COMMAND_RPC_GET_WALLET_RESTORE_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    res.seed_phrase = w.get_wallet()->get_account().get_seed_phrase(req.seed_password);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_seed_phrase_info(const wallet_public::COMMAND_RPC_GET_SEED_PHRASE_INFO::request& req, wallet_public::COMMAND_RPC_GET_SEED_PHRASE_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    tools::get_seed_phrase_info(req.seed_phrase, req.seed_password, res);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  template<typename t_from, typename t_to>
  void copy_wallet_transfer_info_old_container(const t_from& from_c, t_to& to_c)
  {
    for (const auto& item : from_c)
    {
      to_c.push_back(wallet_public::wallet_transfer_info_old());
      *static_cast<wallet_public::wallet_transfer_info*>(&to_c.back()) = item;
    }
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_recent_txs_and_info(const wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::request& req, wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    //this is legacy api, should be removed after successful transition to HF4 
    wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO2::response rsp2 = AUTO_VAL_INIT(rsp2);
    WALLET_RPC_BEGIN_TRY_ENTRY();
    
    on_get_recent_txs_and_info2(req, rsp2, er, cntx);
    res.pi = rsp2.pi;
    res.total_transfers = rsp2.total_transfers;
    res.last_item_index = rsp2.last_item_index;
    copy_wallet_transfer_info_old_container(rsp2.transfers, res.transfers);   
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_recent_txs_and_info2(const wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO2::request& req, wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO2::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    if (req.update_provision_info)
    {
      res.pi.balance = w.get_wallet()->balance(res.pi.unlocked_balance);
      res.pi.transfer_entries_count = w.get_wallet()->get_transfer_entries_count();
      res.pi.transfers_count = w.get_wallet()->get_recent_transfers_total_count();
      res.pi.curent_height = w.get_wallet()->get_top_block_height();
    }

    if (req.offset == 0 && !req.exclude_unconfirmed)
      w.get_wallet()->get_unconfirmed_transfers(res.transfers, req.exclude_mining_txs);

    bool start_from_end = true;
    if (req.order == ORDER_FROM_BEGIN_TO_END)
    {
      start_from_end = false;
    }
    w.get_wallet()->get_recent_transfers_history(res.transfers, req.offset, req.count, res.total_transfers, res.last_item_index, req.exclude_mining_txs, start_from_end);

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_transfer(const wallet_public::COMMAND_RPC_TRANSFER::request& req, wallet_public::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    if (req.fee < w.get_wallet()->get_core_runtime_config().tx_pool_min_fee)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = std::string("Given fee is too low: ") + epee::string_tools::num_to_string_fast(req.fee) + ", minimum is: " + epee::string_tools::num_to_string_fast(w.get_wallet()->get_core_runtime_config().tx_pool_min_fee);
      return false;
    }

    std::string payment_id;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.payment_id, payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("invalid encoded payment id: ") + req.payment_id + ", hex-encoded string was expected";
      return false;
    }

    construct_tx_param ctp = w.get_wallet()->get_default_construct_tx_param_inital();
    if (req.service_entries_permanent)
    {
      //put it to extra
      ctp.extra.insert(ctp.extra.end(), req.service_entries.begin(), req.service_entries.end());
    }
    else
    {
      //put it to attachments
      ctp.attachments.insert(ctp.attachments.end(), req.service_entries.begin(), req.service_entries.end());
    }
    bool wrap = false;
    std::vector<currency::tx_destination_entry>& dsts = ctp.dsts;
    
    for (auto it = req.destinations.begin(); it != req.destinations.end(); it++)
    {
      currency::tx_destination_entry de;
      de.addr.resize(1);
      std::string embedded_payment_id;
      //check if address looks like wrapped address
      if (currency::is_address_like_wrapped(it->address))
      {
        if (wrap) {
          LOG_ERROR("More then one entries in transactions");
          er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
          er.message = "Second wrap entry not supported in transactions";
          return false;

        }
        LOG_PRINT_L0("Address " << it->address << " recognized as wrapped address, creating wrapping transaction...");
        //put into service attachment specially encrypted entry which will contain wrap address and network
        currency::tx_service_attachment sa = AUTO_VAL_INIT(sa);
        sa.service_id = BC_WRAP_SERVICE_ID;
        sa.instruction = BC_WRAP_SERVICE_INSTRUCTION_ERC20;
        sa.flags = TX_SERVICE_ATTACHMENT_ENCRYPT_BODY | TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE;
        sa.body = it->address;
        ctp.extra.push_back(sa);

        currency::account_public_address acc = AUTO_VAL_INIT(acc);
        currency::get_account_address_from_str(acc, BC_WRAP_SERVICE_CUSTODY_WALLET);
        de.addr.front() = acc;
        wrap = true;
        //encrypt body with a special way
      }
      else if (!w.get_wallet()->get_transfer_address(it->address, de.addr.back(), embedded_payment_id))
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
        if (it != req.destinations.begin())
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = std::string("payment id: ") + embedded_payment_id + " currently can only be set for the first destination (an so you can use integrated address only for the fist destination)";
          return false;
        }
        payment_id = embedded_payment_id;
      }
      de.amount = it->amount;
      de.asset_id = (it->asset_id == currency::null_pkey ? currency::native_coin_asset_id : it->asset_id);
      dsts.push_back(de);
    }

    std::vector<currency::attachment_v>& attachments = ctp.attachments;
    std::vector<currency::extra_v>& extra = ctp.extra;
    if (!payment_id.empty() && !currency::set_payment_id_to_tx(attachments, payment_id, w.get_wallet()->is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM)))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("payment id ") + payment_id + " is invalid and can't be set";
      return false;
    }

    if (!req.comment.empty() && payment_id.empty())
    {
      currency::tx_comment comment{};
      comment.comment = req.comment;
      extra.push_back(comment);
    }

    if (req.push_payer && !wrap)
    {
      currency::create_and_add_tx_payer_to_container_from_address(extra, w.get_wallet()->get_account().get_keys().account_address, w.get_wallet()->get_top_block_height(), w.get_wallet()->get_core_runtime_config());
    }

    if (!req.hide_receiver)
    {
      for (auto& d : dsts)
      {
        for (auto& a : d.addr)
          currency::create_and_add_tx_receiver_to_container_from_address(extra, a, w.get_wallet()->get_top_block_height(), w.get_wallet()->get_core_runtime_config());
      }
    }

    currency::finalized_tx result = AUTO_VAL_INIT(result);
    std::string unsigned_tx_blob_str;
    ctp.fee = req.fee;
    ctp.fake_outputs_count = req.mixin;
    w.get_wallet()->transfer(ctp, result, true, &unsigned_tx_blob_str);
    if (w.get_wallet()->is_watch_only())
    {
      res.tx_unsigned_hex = epee::string_tools::buff_to_hex_nodelimer(unsigned_tx_blob_str); // watch-only wallets could not sign and relay transactions
      // leave res.tx_hash empty, because tx hash will change after signing
    }
    else
    {
      crypto::hash tx_id = currency::get_transaction_hash(result.tx);
      res.tx_hash = epee::string_tools::pod_to_hex(tx_id);
      res.tx_size = get_object_blobsize(result.tx);
      //try to get wallet_transfer_info
      wallet_public::wallet_transfer_info wti = AUTO_VAL_INIT(wti);
      if (w.get_wallet()->find_unconfirmed_tx(tx_id, wti))
      {
        res.tx_details = wti;
      }
    }
    return true;

    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_store(const wallet_public::COMMAND_RPC_STORE::request& req, wallet_public::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    w.get_wallet()->store();
    boost::system::error_code ec = AUTO_VAL_INIT(ec);
    res.wallet_file_size = w.get_wallet()->get_wallet_file_size();
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::force_rescan_tx_pool(const wallet_public::COMMAND_RPC_FORCE_RESCAN_TX_POOL::request& req, wallet_public::COMMAND_RPC_FORCE_RESCAN_TX_POOL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    bool dummy = false;
    w.get_wallet()->scan_tx_pool(dummy);
    res.status = API_RETURN_CODE_OK;
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_payments(const wallet_public::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::string payment_id;
    if (!currency::parse_payment_id_from_hex_str(req.payment_id, payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("invalid payment id given: \'") + req.payment_id + "\', hex-encoded string was expected";
      return false;
    }

    res.payments.clear();
    std::list<payment_details> payment_list;
    w.get_wallet()->get_payments(payment_id, payment_list);
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
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_bulk_payments(const wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
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

      std::list<payment_details> payment_list;
      w.get_wallet()->get_payments(payment_id, payment_list, req.min_block_height);

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
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_make_integrated_address(const wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
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

    res.integrated_address = currency::get_account_address_and_payment_id_as_str(w.get_wallet()->get_account().get_public_address(), payment_id);
    res.payment_id = epee::string_tools::buff_to_hex_nodelimer(payment_id);
    return !res.integrated_address.empty();
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_split_integrated_address(const wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
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
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sweep_below(const wallet_public::COMMAND_SWEEP_BELOW::request& req, wallet_public::COMMAND_SWEEP_BELOW::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::payment_id_t payment_id;
    if (!req.payment_id_hex.empty() && !currency::parse_payment_id_from_hex_str(req.payment_id_hex, payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = std::string("Invalid payment id: ") + req.payment_id_hex;
      return false;
    }

    currency::account_public_address addr;
    currency::payment_id_t integrated_payment_id;
    if (!w.get_wallet()->get_transfer_address(req.address, addr, integrated_payment_id))
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

    if (req.fee < w.get_wallet()->get_core_runtime_config().tx_pool_min_fee)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = std::string("Given fee is too low: ") + epee::string_tools::num_to_string_fast(req.fee) + ", minimum is: " + epee::string_tools::num_to_string_fast(w.get_wallet()->get_core_runtime_config().tx_pool_min_fee);
      return false;
    }

    currency::transaction tx = AUTO_VAL_INIT(tx);
    size_t outs_total = 0, outs_swept = 0;
    uint64_t amount_total = 0, amount_swept = 0;

    std::string unsigned_tx_blob_str;
    w.get_wallet()->sweep_below(req.mixin, addr, req.amount, payment_id, req.fee, outs_total, amount_total, outs_swept, amount_swept, &tx, &unsigned_tx_blob_str);

    res.amount_swept = amount_swept;
    res.amount_total = amount_total;
    res.outs_swept = outs_swept;
    res.outs_total = outs_total;

    if (w.get_wallet()->is_watch_only())
    {
      res.tx_unsigned_hex = epee::string_tools::buff_to_hex_nodelimer(unsigned_tx_blob_str); // watch-only wallets can't sign and relay transactions
      // leave res.tx_hash empty, because tx has will change after signing
    }
    else
    {
      res.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(tx));
    }

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_bare_outs_stats(const wallet_public::COMMAND_RPC_GET_BARE_OUTS_STATS ::request& req, wallet_public::COMMAND_RPC_GET_BARE_OUTS_STATS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();

    if (w.get_wallet()->is_watch_only())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "operation cannot be performed in watch-only wallet";
      return false;
    }

    std::vector<tools::wallet2::batch_of_bare_unspent_outs> groups;
    if (!w.get_wallet()->get_bare_unspent_outputs_stats(groups))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "get_bare_unspent_outputs_stats failed";
      return false;
    }

    res.total_amount = 0;
    res.total_bare_outs = 0;
    res.expected_total_fee = 0;
    res.txs_count = 0;
    
    for(auto &g : groups)
    {
      for (auto& tid: g.tids)
      {
        tools::transfer_details td{};
        CHECK_AND_ASSERT_THROW_MES(w.get_wallet()->get_transfer_info_by_index(tid, td), "get_transfer_info_by_index failed with index " << tid);
        res.total_amount += td.m_amount;
      }
      ++res.txs_count;
      res.total_bare_outs += g.tids.size();
      res.expected_total_fee += TX_DEFAULT_FEE;
    }

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sweep_bare_outs(const wallet_public::COMMAND_RPC_SWEEP_BARE_OUTS::request& req, wallet_public::COMMAND_RPC_SWEEP_BARE_OUTS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();

    if (w.get_wallet()->is_watch_only())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "operation cannot be performed in watch-only wallet";
      return false;
    }

    std::vector<tools::wallet2::batch_of_bare_unspent_outs> groups;
    if (!w.get_wallet()->get_bare_unspent_outputs_stats(groups))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "get_bare_unspent_outputs_stats failed";
      return false;
    }

    res.amount_swept = 0;
    res.bare_outs_swept = 0;
    res.fee_spent = 0;

    size_t txs_sent = 0;
    w.get_wallet()->sweep_bare_unspent_outputs(w.get_wallet()->get_account().get_public_address(), groups, txs_sent, res.amount_swept, res.fee_spent, res.bare_outs_swept);
    res.txs_sent = txs_sent;

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
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
    w.get_wallet()->sign_transfer(tx_unsigned_blob, tx_signed_blob, tx);

    res.tx_signed_hex = epee::string_tools::buff_to_hex_nodelimer(tx_signed_blob);
    res.tx_hash = epee::string_tools::pod_to_hex(currency::get_transaction_hash(tx));
    WALLET_RPC_CATCH_TRY_ENTRY();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_submit_transfer(const wallet_public::COMMAND_SUBMIT_TRANSFER::request& req, wallet_public::COMMAND_SUBMIT_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::string tx_signed_blob;
    if (!string_tools::parse_hexstr_to_binbuff(req.tx_signed_hex, tx_signed_blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "tx_signed_hex is invalid";
      return false;
    }

 
    currency::transaction tx = AUTO_VAL_INIT(tx);
    w.get_wallet()->submit_transfer(tx_signed_blob, tx);
    res.tx_hash = epee::string_tools::pod_to_hex(currency::get_transaction_hash(tx));
    WALLET_RPC_CATCH_TRY_ENTRY();

    return true;
  }

  bool wallet_rpc_server::on_search_for_transactions(const wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS_LEGACY::request& req, wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS_LEGACY::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::response res_origin;
    bool r = this->on_search_for_transactions2(req, res_origin, er, cntx);
    copy_wallet_transfer_info_old_container(res_origin.in, res.in);
    copy_wallet_transfer_info_old_container(res_origin.out, res.out);
    copy_wallet_transfer_info_old_container(res_origin.pool, res.pool);

    return r;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_search_for_transactions2(const wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::request& req, wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    bool tx_id_specified = req.tx_id != currency::null_hash;

    // process confirmed txs
    w.get_wallet()->enumerate_transfers_history([&](const wallet_public::wallet_transfer_info& wti) -> bool {

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
    
      bool has_outgoing = wti.has_outgoing_entries();
      if (!has_outgoing && req.in)
        res.in.push_back(wti);

      if (has_outgoing && req.out)
        res.out.push_back(wti);   

      return true; // continue
    }, false /* enumerate_forward */);

    // process unconfirmed txs
    if (req.pool)
    {
      w.get_wallet()->enumerate_unconfirmed_transfers([&](const wallet_public::wallet_transfer_info& wti) -> bool {
        if ((!wti.has_outgoing_entries() && req.in) || (wti.has_outgoing_entries() && req.out))
          res.pool.push_back(wti);
        return true; // continue
      });
    }

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_mining_history(const wallet_public::COMMAND_RPC_GET_MINING_HISTORY::request& req, wallet_public::COMMAND_RPC_GET_MINING_HISTORY::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    w.get_wallet()->get_mining_history(res, req.v);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_register_alias(const wallet_public::COMMAND_RPC_REGISTER_ALIAS::request& req, wallet_public::COMMAND_RPC_REGISTER_ALIAS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
    if (!alias_rpc_details_to_alias_info(req.al, ai))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS";
      return false;
    }
    
    if (!currency::validate_alias_name(ai.m_alias))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS - Wrong alias name";
      return false;
    }

    currency::transaction tx = AUTO_VAL_INIT(tx);
    w.get_wallet()->request_alias_registration(ai, tx, w.get_wallet()->get_default_fee(), 0, req.authority_key);
    res.tx_id = get_transaction_hash(tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_update_alias(const wallet_public::COMMAND_RPC_UPDATE_ALIAS::request& req, wallet_public::COMMAND_RPC_UPDATE_ALIAS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
    if (!alias_rpc_details_to_alias_info(req.al, ai))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS";
      return false;
    }

    if (!currency::validate_alias_name(ai.m_alias))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS - Wrong alias name";
      return false;
    }    

    currency::transaction tx = AUTO_VAL_INIT(tx);
    w.get_wallet()->request_alias_update(ai, tx, w.get_wallet()->get_default_fee());
    res.tx_id = get_transaction_hash(tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_send_proposal(const wallet_public::COMMAND_CONTRACTS_SEND_PROPOSAL::request& req, wallet_public::COMMAND_CONTRACTS_SEND_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();       
    currency::transaction tx = AUTO_VAL_INIT(tx);
    currency::transaction template_tx = AUTO_VAL_INIT(template_tx);
    w.get_wallet()->send_escrow_proposal(req, tx, template_tx);  
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_accept_proposal(const wallet_public::COMMAND_CONTRACTS_ACCEPT_PROPOSAL::request& req, wallet_public::COMMAND_CONTRACTS_ACCEPT_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY(); 
    w.get_wallet()->accept_proposal(req.contract_id, req.acceptance_fee);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_get_all(const wallet_public::COMMAND_CONTRACTS_GET_ALL::request& req, wallet_public::COMMAND_CONTRACTS_GET_ALL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();    
    tools::escrow_contracts_container ecc;
    w.get_wallet()->get_contracts(ecc);
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
    w.get_wallet()->finish_contract(req.contract_id, req.release_type);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_request_cancel(const wallet_public::COMMAND_CONTRACTS_REQUEST_CANCEL::request& req, wallet_public::COMMAND_CONTRACTS_REQUEST_CANCEL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    w.get_wallet()->request_cancel_contract(req.contract_id, req.fee, req.expiration_period);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_contracts_accept_cancel(const wallet_public::COMMAND_CONTRACTS_ACCEPT_CANCEL::request& req, wallet_public::COMMAND_CONTRACTS_ACCEPT_CANCEL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();    
    w.get_wallet()->accept_cancel_contract(req.contract_id);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_marketplace_get_my_offers(const wallet_public::COMMAND_MARKETPLACE_GET_MY_OFFERS::request& req, wallet_public::COMMAND_MARKETPLACE_GET_MY_OFFERS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  { 
    WALLET_RPC_BEGIN_TRY_ENTRY();
    w.get_wallet()->get_actual_offers(res.offers);
    size_t offers_count_before_filtering = res.offers.size();
    bc_services::filter_offers_list(res.offers, req.filter, w.get_wallet()->get_core_runtime_config().get_core_time());
    LOG_PRINT("get_my_offers(): " << res.offers.size() << " offers returned (" << offers_count_before_filtering << " was before filter)", LOG_LEVEL_1);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_marketplace_push_offer(const wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    
    if (req.od.fee < w.get_wallet()->get_current_minimum_network_fee())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "fee is too low";
      return false;
    }

    currency::transaction res_tx{};
    w.get_wallet()->push_offer(req.od, res_tx);

    res.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    res.tx_blob_size = currency::get_object_blobsize(res_tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_marketplace_push_update_offer(const wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    if (req.od.fee < w.get_wallet()->get_current_minimum_network_fee())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "fee is too low";
      return false;
    }

    currency::transaction res_tx{};
    w.get_wallet()->update_offer_by_id(req.tx_id, req.no, req.od, res_tx);

    res.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    res.tx_blob_size = currency::get_object_blobsize(res_tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_marketplace_cancel_offer(const wallet_public::COMMAND_MARKETPLACE_CANCEL_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_CANCEL_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();

    if (req.fee < w.get_wallet()->get_current_minimum_network_fee())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "fee is too low";
      return false;
    }

    currency::transaction res_tx{};
    w.get_wallet()->cancel_offer_by_id(req.tx_id, req.no, req.fee, res_tx);

    res.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
    res.tx_blob_size = currency::get_object_blobsize(res_tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_ionic_swap_generate_proposal(const wallet_public::COMMAND_IONIC_SWAP_GENERATE_PROPOSAL::request& req, wallet_public::COMMAND_IONIC_SWAP_GENERATE_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::account_public_address destination_addr = AUTO_VAL_INIT(destination_addr);
    currency::payment_id_t integrated_payment_id;
    if (!w.get_wallet()->get_transfer_address(req.destination_address, destination_addr, integrated_payment_id))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS";
      return false;
    }
    if (integrated_payment_id.size())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS - integrated address is noit supported yet";
      return false;
    }

    wallet_public::ionic_swap_proposal proposal = AUTO_VAL_INIT(proposal);
    bool r = w.get_wallet()->create_ionic_swap_proposal(req.proposal, destination_addr, proposal);
    if (!r)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT - Error creating proposal";
      return false;
    }
    res.hex_raw_proposal = epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(proposal));
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_ionic_swap_get_proposal_info(const wallet_public::COMMAND_IONIC_SWAP_GET_PROPOSAL_INFO::request& req, wallet_public::COMMAND_IONIC_SWAP_GET_PROPOSAL_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::string raw_tx_template;
    bool r = epee::string_tools::parse_hexstr_to_binbuff(req.hex_raw_proposal, raw_tx_template);
    if (!r)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT - failed to parse template from hex";
      return false;
    }

    if (!w.get_wallet()->get_ionic_swap_proposal_info(raw_tx_template, res.proposal))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT - get_ionic_swap_proposal_info";
      return false;
    }
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_ionic_swap_accept_proposal(const wallet_public::COMMAND_IONIC_SWAP_ACCEPT_PROPOSAL::request& req, wallet_public::COMMAND_IONIC_SWAP_ACCEPT_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::string raw_tx_template;
    bool r = epee::string_tools::parse_hexstr_to_binbuff(req.hex_raw_proposal, raw_tx_template);
    if (!r)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT - failed to parse template from hex";
      return false;
    }

    currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
    if (!w.get_wallet()->accept_ionic_swap_proposal(raw_tx_template, result_tx))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT - failed to accept_ionic_swap_proposal()";
      return false;
    }

    res.result_tx_id = currency::get_transaction_hash(result_tx);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_assets_whitelist_get(const wallet_public::COMMAND_ASSETS_WHITELIST_GET::request& req, wallet_public::COMMAND_ASSETS_WHITELIST_GET::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();

    currency::assets_map_to_assets_list(res.local_whitelist, w.get_wallet()->get_local_whitelist());
    currency::assets_map_to_assets_list(res.global_whitelist, w.get_wallet()->get_global_whitelist());
    currency::assets_map_to_assets_list(res.own_assets, w.get_wallet()->get_own_assets());

    //const auto global_whitelist = w.get_wallet()->get_global_whitelist();

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_assets_whitelist_add(const wallet_public::COMMAND_ASSETS_WHITELIST_ADD::request& req, wallet_public::COMMAND_ASSETS_WHITELIST_ADD::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    if(!w.get_wallet()->add_custom_asset_id(req.asset_id, res.asset_descriptor))
    {
      res.status = API_RETURN_CODE_NOT_FOUND;
    }
    else
    {
      res.status = API_RETURN_CODE_OK;
    }
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  } 
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_assets_whitelist_remove(const wallet_public::COMMAND_ASSETS_WHITELIST_REMOVE::request& req, wallet_public::COMMAND_ASSETS_WHITELIST_REMOVE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    if(!w.get_wallet()->delete_custom_asset_id(req.asset_id))
    {
      res.status = API_RETURN_CODE_NOT_FOUND;
    }
    else
    {
      res.status = API_RETURN_CODE_OK;
    }
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  #define DESTINATIONS_COUNT_FOR_DEPLOY_OR_EMIT 10
  static_assert(DESTINATIONS_COUNT_FOR_DEPLOY_OR_EMIT >= CURRENCY_TX_MIN_ALLOWED_OUTS, "DESTINATIONS_COUNT_FOR_DEPLOY_OR_EMIT must be >= min allowed tx outs");
  void wallet_rpc_server::rpc_destinations_to_currency_destinations(const std::list<wallet_public::transfer_destination>& rpc_destinations, bool nullify_asset_id, bool try_to_split, std::vector<currency::tx_destination_entry>& result_destinations)
  {
    GET_WALLET();

    WLT_THROW_IF_FALSE_WITH_CODE(!rpc_destinations.empty(), "WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT", "WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT");

    std::list<wallet_public::transfer_destination> local_destinations;
    if (nullify_asset_id && try_to_split)
    {
      bool do_split = true;
      uint64_t total_amount     = rpc_destinations.front().amount;
      std::string first_address = rpc_destinations.front().address;
      for(auto it = std::next(rpc_destinations.begin()); it != rpc_destinations.end(); ++it)
      {
        total_amount += it->amount;
        if (first_address != it->address)
        {
          do_split = false;
          break;
        }
      }
      if (do_split)
      {
        const uint64_t el_amount = total_amount / DESTINATIONS_COUNT_FOR_DEPLOY_OR_EMIT; // approximation, see below
        wallet_public::transfer_destination td{};
        td.address = first_address;
        td.amount = el_amount;
        for(size_t i = 0; i < DESTINATIONS_COUNT_FOR_DEPLOY_OR_EMIT - 1; ++i)
          local_destinations.push_back(td);
        td.amount = total_amount - (DESTINATIONS_COUNT_FOR_DEPLOY_OR_EMIT - 1) * el_amount; // the last element must account for division error
        local_destinations.push_back(td);
      }
    }
    const std::list<wallet_public::transfer_destination>& destinations = local_destinations.size() != 0 ? local_destinations : rpc_destinations;

    for (auto it = destinations.begin(); it != destinations.end(); ++it)
    {
      currency::tx_destination_entry de{};
      de.addr.resize(1);
      std::string embedded_payment_id;
      //check if address looks like wrapped address
      WLT_THROW_IF_FALSE_WITH_CODE(!currency::is_address_like_wrapped(it->address), "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS", "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS");
      WLT_THROW_IF_FALSE_WITH_CODE(w.get_wallet()->get_transfer_address(it->address, de.addr.back(), embedded_payment_id), "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS", "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS");
      WLT_THROW_IF_FALSE_WITH_CODE(embedded_payment_id.size() == 0, "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS", "WALLET_RPC_ERROR_CODE_WRONG_ADDRESS");
      de.amount = it->amount;
      de.asset_id = nullify_asset_id ? currency::null_pkey : it->asset_id;
      result_destinations.push_back(de);
    }
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_asset_deploy(const wallet_public::COMMAND_ASSETS_DEPLOY::request& req_, wallet_public::COMMAND_ASSETS_DEPLOY::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    //make local req so we can modify it (if needed)
    wallet_public::COMMAND_ASSETS_DEPLOY::request req = req_;

    if (!currency::validate_asset_ticker_and_full_name(req.asset_descriptor))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "asset ticker or full_name is invalid";
      return false;
    }

    if (req.destinations.empty() && req.asset_descriptor.current_supply != 0)
    {
      req.destinations.push_back(tools::wallet_public::transfer_destination{ req.asset_descriptor.current_supply , w.get_wallet()->get_account().get_public_address_str(), currency::null_pkey });
    }

    std::vector<currency::tx_destination_entry> currency_destinations;
    rpc_destinations_to_currency_destinations(req.destinations, true, !req.do_not_split_destinations, currency_destinations);

    currency::finalized_tx ft{};
    w.get_wallet()->deploy_new_asset(req.asset_descriptor, currency_destinations, ft, res.new_asset_id);
    res.tx_id = ft.tx_id;

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void finalized_tx_to_external_signing_data(const currency::finalized_tx& ft, wallet_public::data_for_external_asset_signing_tx& data)
  {
    // include additonal info into response, if it's an external signing asset operation
    data.unsigned_tx = t_serializable_object_to_blob(ft.tx);
    data.tx_secret_key = ft.one_time_key;
    std::vector<std::string>& outs_addr = data.outputs_addresses;
    for (auto d : ft.ftp.prepared_destinations)
      outs_addr.push_back(currency::get_account_address_as_str(d.addr.back()));
    data.finalized_tx = t_serializable_object_to_blob(ft);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_asset_emit(const wallet_public::COMMAND_ASSETS_EMIT::request& req, wallet_public::COMMAND_ASSETS_EMIT::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::asset_descriptor_base last_adb{};
    CHECK_AND_ASSERT_THROW_MES(w.get_wallet()->daemon_get_asset_info(req.asset_id, last_adb), "unknown asset_id"); // TODO: bad design, consider refactoring -- sowle

    std::vector<currency::tx_destination_entry> currency_destinations;
    rpc_destinations_to_currency_destinations(req.destinations, true, !req.do_not_split_destinations, currency_destinations);

    currency::finalized_tx ft{};
    w.get_wallet()->emit_asset(req.asset_id, currency_destinations, ft);
    res.tx_id = ft.tx_id;

    if (ft.ftp.ado_sign_thirdparty)
    {
      // include additional info into response, if it's an external signing asset operation
      wallet_public::data_for_external_asset_signing_tx data{};
      finalized_tx_to_external_signing_data(ft, data);
      res.data_for_external_signing = data;
    }

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_asset_update(const wallet_public::COMMAND_ASSETS_UPDATE::request& req, wallet_public::COMMAND_ASSETS_UPDATE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::finalized_tx ft{};
    w.get_wallet()->update_asset(req.asset_id, req.asset_descriptor, ft);
    res.tx_id = ft.tx_id;

    if (ft.ftp.ado_sign_thirdparty)
    {
      // include additional info into response, if it's an external signing asset operation
      wallet_public::data_for_external_asset_signing_tx data{};
      finalized_tx_to_external_signing_data(ft, data);
      res.data_for_external_signing = data;
    }

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }  
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_asset_burn(const wallet_public::COMMAND_ASSETS_BURN::request& req, wallet_public::COMMAND_ASSETS_BURN::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();

    currency::finalized_tx ft{};
    w.get_wallet()->burn_asset(req.asset_id, req.burn_amount, ft, req.service_entries, req.point_tx_to_address, req.native_amount);
    res.tx_id = ft.tx_id;

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_asset_send_ext_signed_tx(const wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::request& req, wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();

    currency::finalized_tx ft{};
    if (!t_unserializable_object_from_blob(ft, req.finalized_tx))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "finalized_tx couldn't be deserialized";
      return false;
    }

    if (t_serializable_object_to_blob(ft.tx) != req.unsigned_tx)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = "unsigned_tx doesn't match finalized_tx";
      return false;
    }

    crypto::hash tx_id = currency::get_transaction_hash(ft.tx);
    if (req.expected_tx_id != tx_id)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
      er.message = std::string("expected_tx_id mismatch, real tx id is ") + epee::string_tools::pod_to_hex(tx_id);
      return false;
    }

    try
    {
      currency::transaction result_tx{};
      if (!req.regular_sig.is_zero())
      {
        w.get_wallet()->submit_externally_signed_asset_tx(ft, req.regular_sig, req.unlock_transfers_on_fail, result_tx, res.transfers_were_unlocked);
      }
      else
      {
        w.get_wallet()->submit_externally_signed_asset_tx(ft, req.eth_sig, req.unlock_transfers_on_fail, result_tx, res.transfers_were_unlocked);
      }
    }
    catch(std::exception& e)
    {
      // doing this to be able to return 'transfers_were_unlocked' to the caller even in the case of exception
      res.status = e.what();
      return true;
    }
    res.status = API_RETURN_CODE_OK;  
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_attach_asset_descriptor(const wallet_public::COMMAND_ATTACH_ASSET_DESCRIPTOR::request& req, wallet_public::COMMAND_ATTACH_ASSET_DESCRIPTOR::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    w.get_wallet()->attach_asset_descriptor(req, res);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_transfer_asset_ownership(const wallet_public::COMMAND_TRANSFER_ASSET_OWNERSHIP::request& req, wallet_public::COMMAND_TRANSFER_ASSET_OWNERSHIP::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    currency::asset_owner_pub_key_v new_owner_v;
    if (req.new_owner != currency::null_pkey && req.new_owner_eth_pub_key == currency::null_eth_public_key)
    {
      new_owner_v = req.new_owner;
    }
    else if(req.new_owner_eth_pub_key != currency::null_eth_public_key && req.new_owner == currency::null_pkey)
    {
      new_owner_v = req.new_owner_eth_pub_key;
    }
    else
    {
      res.status = API_RETURN_CODE_BAD_ARG_INVALID_ADDRESS;
      return true;
    }

    try
    {
      currency::finalized_tx ft{};
      w.get_wallet()->transfer_asset_ownership(req.asset_id, new_owner_v, ft);
      if (ft.ftp.ado_sign_thirdparty)
      {
        // include additional info into response, if it's an external signing asset operation
        wallet_public::data_for_external_asset_signing_tx data{};
        finalized_tx_to_external_signing_data(ft, data);
        res.data_for_external_signing = data;
      }
      res.tx_id = ft.tx_id;
      res.status = API_RETURN_CODE_OK;
      return true;
    }
    catch (std::exception& e)
    {
      // doing this to be able to return 'transfers_were_unlocked' to the caller even in the case of exception
      res.status = e.what();
      return true;
    }

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mw_get_wallets(const wallet_public::COMMAND_MW_GET_WALLETS::request& req, wallet_public::COMMAND_MW_GET_WALLETS::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::shared_ptr<i_wallet2_callback> pcallback = w.get_wallet()->get_callback();
    if (!pcallback)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR";
      return false;
    }
    pcallback->on_mw_get_wallets(res.wallets);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mw_select_wallet(const wallet_public::COMMAND_MW_SELECT_WALLET::request& req, wallet_public::COMMAND_MW_SELECT_WALLET::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::shared_ptr<i_wallet2_callback> pcallback = w.get_wallet()->get_callback();
    if (!pcallback)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR";
      return false;
    }
    pcallback->on_mw_select_wallet(req.wallet_id);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sign_message(const wallet_public::COMMAND_SIGN_MESSAGE::request& req, wallet_public::COMMAND_SIGN_MESSAGE::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();    
    std::string buff = epee::string_encoding::base64_decode(req.buff);
    w.get_wallet()->sign_buffer(buff, res.sig);
    res.pkey = w.get_wallet()->get_account().get_public_address().spend_public_key;
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_encrypt_data(const wallet_public::COMMAND_ENCRYPT_DATA::request& req, wallet_public::COMMAND_ENCRYPT_DATA::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::string buff = epee::string_encoding::base64_decode(req.buff);
    w.get_wallet()->encrypt_buffer(buff, res.res_buff);
    res.res_buff = epee::string_encoding::base64_encode(res.res_buff);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_proxy_to_daemon(const wallet_public::COMMAND_PROXY_TO_DAEMON::request& req, wallet_public::COMMAND_PROXY_TO_DAEMON::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::string buff = epee::string_encoding::base64_decode(req.base64_body);
    
    w.get_wallet()->proxy_to_daemon(req.uri, buff, res.response_code, res.base64_body);
        
    res.base64_body = epee::string_encoding::base64_encode(res.base64_body);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_clear_utxo_cold_sig_reservation(const wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION::request& req, wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();

    std::vector<uint64_t> affected_transfer_ids;
    w.get_wallet()->clear_utxo_cold_sig_reservation(affected_transfer_ids);

    for (auto tid : affected_transfer_ids)
    {
      transfer_details td{};
      if (!w.get_wallet()->get_transfer_info_by_index(tid, td))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "internal error: get_transfer_info_by_index failed for tid " + epee::string_tools::num_to_string_fast(tid);
        return false;
      }

      res.affected_outputs.emplace_back();
      currency::get_out_pub_key_from_tx_out_v(td.output(), res.affected_outputs.back().pk);
    }

    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_decrypt_data(const wallet_public::COMMAND_DECRYPT_DATA::request& req, wallet_public::COMMAND_DECRYPT_DATA::response& res, epee::json_rpc::error& er, connection_context& cntx)
  {
    WALLET_RPC_BEGIN_TRY_ENTRY();
    std::string buff = epee::string_encoding::base64_decode(req.buff);
    w.get_wallet()->encrypt_buffer(buff, res.res_buff);
    res.res_buff = epee::string_encoding::base64_encode(res.res_buff);
    return true;
    WALLET_RPC_CATCH_TRY_ENTRY();
  }
  //------------------------------------------------------------------------------------------------------------------------------
//   bool wallet_rpc_server::reset_active_wallet(std::shared_ptr<wallet2> w)
//   {
//     m_pwallet = w;
//     return true;
//   }
  //------------------------------------------------------------------------------------------------------------------------------
} // namespace tools
