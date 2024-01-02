// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "currency_core/currency_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "include_base_utils.h"

// TODO: get rid of all these types, leave only wallet_error with a string message and string/enum error designator


#define WALLET_LIB_STATE_SENDING                       "STATE_SENDING"
#define WALLET_LIB_SENT_SUCCESS                        "STATE_SENT_SUCCESS"
#define WALLET_LIB_SEND_FAILED                         "STATE_SEND_FAILED"


namespace tools
{
  namespace error
  {
    // std::exception
    //   std::runtime_error
    //     wallet_runtime_error *
    //       wallet_internal_error
    //         unexpected_txin_type
    //   std::logic_error
    //     wallet_logic_error *
    //       file_exists
    //       file_not_found
    //       file_read_error
    //       file_save_error
    //       invalid_password
    //       refresh_error *
    //         acc_outs_lookup_error
    //         block_parse_error
    //         get_blocks_error
    //         get_out_indexes_error
    //         tx_extra_parse_error
    //         tx_parse_error
    //       transfer_error *
    //         get_random_outs_general_error
    //         not_enough_money
    //         not_enough_outs_to_mix
    //         tx_not_constructed
    //         tx_rejected
    //         tx_sum_overflow
    //         tx_too_big
    //         zero_destination
    //       wallet_rpc_error *
    //         daemon_busy
    //         no_connection_to_daemon
    //       wallet_files_doesnt_correspond
    //
    // * - class with protected ctor

    //----------------------------------------------------------------------------------------------------
    template<typename Base>
    struct wallet_error_base : public Base
    {
      const std::string& location() const { return m_loc; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << m_loc << '[' << boost::replace_all_copy(std::string(typeid(*this).name()), "struct ", "");
        if (!m_error_code.empty())
          ss << "[" << m_error_code << "]";
        ss << "] " << Base::what();
        return ss.str();
      }

      virtual const char* what() const noexcept
      {
        m_what = to_string();
        return m_what.c_str();
      }

      virtual const std::string error_code() const noexcept
      {
        return m_error_code;
      }

      wallet_error_base(std::string&& loc, const std::string& message, const std::string& error_code)
        : Base(message)
        , m_loc(loc)
        , m_error_code(error_code)
      {
      }

    protected:
      wallet_error_base(std::string&& loc, const std::string& message)
        : Base(message)
        , m_loc(loc)
      {
      }


    private:
      std::string m_loc;
      std::string m_error_code;
      mutable std::string m_what;
    };
    //----------------------------------------------------------------------------------------------------
    const char* const failed_rpc_request_messages[] = {
      "failed to get blocks",
      "failed to get out indices",
      "failed to get random outs"
    };
    enum failed_rpc_request_message_indices
    {
      get_blocks_error_message_index,
      get_out_indices_error_message_index,
      get_random_outs_error_message_index
    };

    template<typename Base, int msg_index>
    struct failed_rpc_request : public Base
    {
      explicit failed_rpc_request(std::string&& loc, const std::string& status)
        : Base(std::move(loc), failed_rpc_request_messages[msg_index])
        , m_status(status)
      {
      }

      const std::string& status() const { return m_status; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << Base::to_string() << ", status = " << status();
        return ss.str();
      }

    private:
      std::string m_status;
    };
    //----------------------------------------------------------------------------------------------------
    typedef wallet_error_base<std::runtime_error> wallet_error;
    typedef wallet_error wallet_logic_error;
    typedef wallet_error wallet_runtime_error;
    //----------------------------------------------------------------------------------------------------
    struct wallet_internal_error : public wallet_runtime_error
    {
      explicit wallet_internal_error(std::string&& loc, const std::string& message)
        : wallet_runtime_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_wrong_seed_error : public wallet_runtime_error
    {
      explicit wallet_wrong_seed_error(std::string&& loc, const std::string& message)
        : wallet_runtime_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_common_error : public wallet_runtime_error
    {
      explicit wallet_common_error(std::string&& loc, const std::string& message)
        : wallet_runtime_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct unexpected_txin_type : public wallet_internal_error
    {
      explicit unexpected_txin_type(std::string&& loc, const currency::transaction& tx)
        : wallet_internal_error(std::move(loc), "one of tx inputs has unexpected type")
        , m_tx(tx)
      {
      }

      const currency::transaction& tx() const { return m_tx; }

      std::string to_string() const
      {
        std::ostringstream ss;
        currency::transaction tx = m_tx;
        ss << wallet_internal_error::to_string() << ", tx:\n" << currency::obj_to_json_str(tx);
        return ss.str();
      }

    private:
      currency::transaction m_tx;
    };
    //----------------------------------------------------------------------------------------------------
    const char* const file_error_messages[] = {
      "file already exists",
      "file not found",
      "failed to read file",
      "failed to save file"
    };
    enum file_error_message_indices
    {
      file_exists_message_index,
      file_not_found_message_index,
      file_read_error_message_index,
      file_save_error_message_index
    };

    template<int msg_index>
    struct file_error_base : public wallet_logic_error
    {
      explicit file_error_base(std::string&& loc, const std::string& file)
        : wallet_logic_error(std::move(loc), std::string(file_error_messages[msg_index]) +  " \"" + file + '\"')
        , m_file(file)
      {
      }

      const std::string& file() const { return m_file; }

      std::string to_string() const { return wallet_logic_error::to_string(); }

    private:
      std::string m_file;
    };
    //----------------------------------------------------------------------------------------------------
    typedef file_error_base<file_exists_message_index> file_exists;
    typedef file_error_base<file_not_found_message_index>  file_not_found;
    typedef file_error_base<file_not_found_message_index>  file_not_found;
    typedef file_error_base<file_read_error_message_index> file_read_error;
    typedef file_error_base<file_save_error_message_index> file_save_error;
    //----------------------------------------------------------------------------------------------------
    struct invalid_password : public wallet_logic_error
    {
      explicit invalid_password(std::string&& loc)
        : wallet_logic_error(std::move(loc), "invalid password")
      {
      }

      std::string to_string() const { return wallet_logic_error::to_string(); }
    };
    //----------------------------------------------------------------------------------------------------
    struct refresh_error : public wallet_logic_error
    {
    protected:
      refresh_error(std::string&& loc, const std::string& message)
        : wallet_logic_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct acc_outs_lookup_error : public refresh_error
    {
      explicit acc_outs_lookup_error(std::string&& loc, const currency::transaction& tx,
        const crypto::public_key& tx_pub_key, const currency::account_keys& acc_keys)
        : refresh_error(std::move(loc), "account outs lookup error")
        , m_tx(tx)
        , m_tx_pub_key(tx_pub_key)
        , m_acc_keys(acc_keys)
      {
      }

      const currency::transaction& tx() const { return m_tx; }
      const crypto::public_key& tx_pub_key() const { return m_tx_pub_key; }
      const currency::account_keys& acc_keys() const { return m_acc_keys; }

      std::string to_string() const
      {
        std::ostringstream ss;
        currency::transaction tx = m_tx;
        ss << refresh_error::to_string() << ", tx: " << currency::obj_to_json_str(tx);
        return ss.str();
      }

    private:
      const currency::transaction m_tx;
      const crypto::public_key m_tx_pub_key;
      const currency::account_keys m_acc_keys;
    };
    //----------------------------------------------------------------------------------------------------
    struct block_parse_error : public refresh_error
    {
      explicit block_parse_error(std::string&& loc, const currency::blobdata& block_data)
        : refresh_error(std::move(loc), "block parse error")
        , m_block_blob(block_data)
      {
      }

      const currency::blobdata& block_blob() const { return m_block_blob; }

      std::string to_string() const { return refresh_error::to_string(); }

    private:
      currency::blobdata m_block_blob;
    };
    //----------------------------------------------------------------------------------------------------
    typedef failed_rpc_request<refresh_error, get_blocks_error_message_index> get_blocks_error;
    //----------------------------------------------------------------------------------------------------
    typedef failed_rpc_request<refresh_error, get_out_indices_error_message_index> get_out_indices_error;
    //----------------------------------------------------------------------------------------------------
    struct tx_extra_parse_error : public refresh_error
    {
      explicit tx_extra_parse_error(std::string&& loc, const currency::transaction& tx)
        : refresh_error(std::move(loc), "transaction extra parse error")
        , m_tx(tx)
      {
      }

      const currency::transaction& tx() const { return m_tx; }

      std::string to_string() const
      {
        std::ostringstream ss;
        currency::transaction tx = m_tx;
        ss << refresh_error::to_string() << ", tx: " << currency::obj_to_json_str(tx);
        return ss.str();
      }

    private:
      const currency::transaction m_tx;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_parse_error : public refresh_error
    {
      explicit tx_parse_error(std::string&& loc, const currency::blobdata& tx_blob)
        : refresh_error(std::move(loc), "transaction parse error")
        , m_tx_blob(tx_blob)
      {
      }

      const currency::blobdata& tx_blob() const { return m_tx_blob; }

      std::string to_string() const { return refresh_error::to_string(); }

    private:
      currency::blobdata m_tx_blob;
    };
    //----------------------------------------------------------------------------------------------------
    struct transfer_error : public wallet_logic_error
    {
    protected:
      transfer_error(std::string&& loc, const std::string& message)
        : wallet_logic_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    typedef failed_rpc_request<transfer_error, get_random_outs_error_message_index> get_random_outs_error;
    //----------------------------------------------------------------------------------------------------
    struct not_enough_money : public transfer_error
    {
      not_enough_money(std::string&& loc, uint64_t availbable, uint64_t tx_amount, uint64_t fee, const crypto::public_key& asset_id)
        : transfer_error(std::move(loc), "")
        , m_available(availbable)
        , m_tx_amount(tx_amount)
        , m_fee(fee)
        , m_asset_id(asset_id)
      {
      }

      uint64_t available() const { return m_available; }
      uint64_t tx_amount() const { return m_tx_amount; }
      uint64_t fee() const { return m_fee; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string() <<
          "available: " << currency::print_money_brief(m_available) <<
          ", required: " << currency::print_money_brief(m_tx_amount + m_fee) <<
          " = " << currency::print_money_brief(m_tx_amount) << " + " << currency::print_money_brief(m_fee) << " (fee)";
        if (m_asset_id != currency::native_coin_asset_id)
          ss << ", asset_id: " << m_asset_id;
        return ss.str();
      }

    private:
      uint64_t m_available;
      uint64_t m_tx_amount;
      uint64_t m_fee;
      crypto::public_key m_asset_id;
    };

    struct no_zc_inputs : public transfer_error
    {
      no_zc_inputs(const std::string& /*v*/): transfer_error(std::string(""), API_RETURN_CODE_MISSING_ZC_INPUTS)
      {}      

      virtual const char* what() const noexcept
      {
        return API_RETURN_CODE_MISSING_ZC_INPUTS;
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct not_enough_outs_to_mix : public transfer_error
    {
      typedef std::vector<currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> scanty_outs_t;

      explicit not_enough_outs_to_mix(std::string&& loc, const scanty_outs_t& scanty_outs, size_t mixin_count)
        : transfer_error(std::move(loc), "not enough outputs to mix")
        , m_scanty_outs(scanty_outs)
        , m_mixin_count(mixin_count)
      {
      }

      const scanty_outs_t& scanty_outs() const { return m_scanty_outs; }
      size_t mixin_count() const { return m_mixin_count; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << API_RETURN_CODE_NOT_ENOUGH_OUTPUTS_FOR_MIXING;
        //ss << transfer_error::to_string() << ", mixin_count = " << m_mixin_count << ", scanty_outs:";
        //for (const auto& outs_for_amount : m_scanty_outs)
        //{
        //  ss << '\n' << currency::print_money(outs_for_amount.amount) << " - " << outs_for_amount.outs.size();
        //}
        return ss.str();
      }

      virtual const char* what() const noexcept
      {
        return API_RETURN_CODE_NOT_ENOUGH_OUTPUTS_FOR_MIXING;
      }

    private:
      scanty_outs_t m_scanty_outs;
      size_t m_mixin_count;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_not_constructed : public transfer_error
    {
      typedef std::vector<currency::tx_source_entry> sources_t;
      typedef std::vector<currency::tx_destination_entry> destinations_t;

      explicit tx_not_constructed(std::string&& loc, const sources_t& sources, const destinations_t& destinations, uint64_t unlock_time)
        : transfer_error(std::move(loc), "transaction was not constructed")
        , m_sources(sources)
        , m_destinations(destinations)
        , m_unlock_time(unlock_time)
      {
      }

      const sources_t& sources() const { return m_sources; }
      const destinations_t& destinations() const { return m_destinations; }
      uint64_t unlock_time() const { return m_unlock_time; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string();
        ss << "\nSources:";
        for (size_t i = 0; i < m_sources.size(); ++i)
        {
          const currency::tx_source_entry& src = m_sources[i];
          ss << "\n  " << i << ": ";
          ss << " amount: " << std::setw(21) << currency::print_money(src.amount);
          ss << (src.is_zc() ? " ZC  " : " old ");
          ss << " asset_id: " << src.asset_id;
           for (size_t j = 0; j < src.outputs.size(); ++j)
          {
            const currency::tx_source_entry::output_entry& out = src.outputs[j];
            if (out.out_reference.type() == typeid(uint64_t))
              ss << "\n      ref #" << j << ": " << boost::get<uint64_t>(out.out_reference);
            else
              ss << "\n      ref #" << j << ": ref by id";
           }

          // It's not good, if logs will contain such much data
          //ss << "\n    real_output: " << src.real_output;
          //ss << "\n    real_output_in_tx_index: " << src.real_output_in_tx_index;
          //ss << "\n    real_out_tx_key: " << epee::string_tools::pod_to_hex(src.real_out_tx_key);
          //ss << "\n    outputs:";
          //for (size_t j = 0; j < src.outputs.size(); ++j)
          //{
          //  const currency::tx_source_entry::output_entry& out = src.outputs[j];
          //  ss << "\n      " << j << ": " << out.first << ", " << epee::string_tools::pod_to_hex(out.second);
          //}
        }

        ss << "\nDestinations:";
        for (size_t i = 0; i < m_destinations.size(); ++i)
        {
          const currency::tx_destination_entry& dst = m_destinations[i];
          
          ss << "\n  " << i << ": ";
          for (auto & a : dst.addr)
          {
            ss << currency::get_account_address_as_str(a) << ";";
          } 
          ss << " amount: " << std::setw(21) << currency::print_money(dst.amount);
          ss << " asset_id: " << dst.asset_id;
        }

        ss << "\nunlock_time: " << m_unlock_time;

        return ss.str();
      }

    private:
      sources_t m_sources;
      destinations_t m_destinations;
      uint64_t m_unlock_time;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_rejected : public transfer_error
    {
      explicit tx_rejected(std::string&& loc, const currency::transaction& tx, const std::string& status)
        : transfer_error(std::move(loc), "transaction was rejected by daemon")
        , m_tx(tx)
        , m_status(status)
      {
      }

      const currency::transaction& tx() const { return m_tx; }
      const std::string& status() const { return m_status; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string() << ", status = " << m_status << ", tx: ";// << std::to_string(currency::get_transaction_prefix_hash(tx)) << "\n";
        currency::transaction tx = m_tx;
        ss << currency::obj_to_json_str(tx);
        return ss.str();
      }

    private:
      currency::transaction m_tx;
      std::string m_status;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_sum_overflow : public transfer_error
    {
      tx_sum_overflow(std::string&& loc, const std::vector<currency::tx_destination_entry>& destinations, uint64_t fee)
        : transfer_error(std::move(loc), "transaction sum + fee exceeds " + currency::print_money(std::numeric_limits<uint64_t>::max()))
        , m_destinations(destinations)
        , m_fee(fee)
      {
      }

      const std::vector<currency::tx_destination_entry>& destinations() const { return m_destinations; }
      uint64_t fee() const { return m_fee; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << transfer_error::to_string() <<
          ", fee = " << currency::print_money(m_fee) <<
          ", destinations:";
        for (const auto& dst : m_destinations)
        {
          ss << '\n' << currency::print_money(dst.amount) << " -> ";
          for (const auto& a : dst.addr)
            ss << currency::get_account_address_as_str(a) << " ";
        }
        return ss.str();
      }

    private:
      std::vector<currency::tx_destination_entry> m_destinations;
      uint64_t m_fee;
    };
    //----------------------------------------------------------------------------------------------------
    struct tx_too_big : public transfer_error
    {
      explicit tx_too_big(std::string&& loc, const currency::transaction& tx, uint64_t tx_size_limit)
        : transfer_error(std::move(loc), "transaction is too big")
        , m_tx(tx)
        , m_tx_size_limit(tx_size_limit)
      {
      }

      const currency::transaction& tx() const { return m_tx; }
      uint64_t tx_size_limit() const { return m_tx_size_limit; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << API_RETURN_CODE_TX_IS_TOO_BIG;
        //currency::transaction tx = m_tx;
        //ss << transfer_error::to_string() <<
        //  ", tx_size_limit = " << m_tx_size_limit <<
        //  ", tx size = " << get_object_blobsize(m_tx) <<
        //  ", tx:\n" << currency::obj_to_json_str(tx);
        return ss.str();
      }
      virtual const char* what() const noexcept
      {
        return API_RETURN_CODE_TX_IS_TOO_BIG;
      }
    private:
      currency::transaction m_tx;
      uint64_t m_tx_size_limit;
    };
    //----------------------------------------------------------------------------------------------------
    struct zero_destination : public transfer_error
    {
      explicit zero_destination(std::string&& loc)
        : transfer_error(std::move(loc), "destination amount is zero")
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_load_notice_wallet_restored : public wallet_runtime_error
    {
      explicit wallet_load_notice_wallet_restored(std::string&& loc, const std::string& message)
        : wallet_runtime_error(std::move(loc), message)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_rpc_error : public wallet_logic_error
    {
      const std::string& request() const { return m_request; }

      std::string to_string() const
      {
        std::ostringstream ss;
        ss << wallet_logic_error::to_string() << ", request = " << m_request;
        return ss.str();
      }

    protected:
      wallet_rpc_error(std::string&& loc, const std::string& message, const std::string& request)
        : wallet_logic_error(std::move(loc), message)
        , m_request(request)
      {
      }

    private:
      std::string m_request;
    };
    //----------------------------------------------------------------------------------------------------
    struct daemon_busy : public wallet_rpc_error
    {
      explicit daemon_busy(std::string&& loc, const std::string& request)
        : wallet_rpc_error(std::move(loc), "daemon is busy", request)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct no_connection_to_daemon : public wallet_rpc_error
    {
      explicit no_connection_to_daemon(std::string&& loc, const std::string& request)
        : wallet_rpc_error(std::move(loc), "no connection to daemon", request)
      {
      }
    };
    //----------------------------------------------------------------------------------------------------
    struct wallet_files_doesnt_correspond : public wallet_logic_error
    {
      explicit wallet_files_doesnt_correspond(std::string&& loc, const std::string& keys_file, const std::string& wallet_file)
        : wallet_logic_error(std::move(loc), "file " + wallet_file + " does not correspond to " + keys_file)
      {
      }

      const std::string& keys_file() const { return m_keys_file; }
      const std::string& wallet_file() const { return m_wallet_file; }

      std::string to_string() const { return wallet_logic_error::to_string(); }

    private:
      std::string m_keys_file;
      std::string m_wallet_file;
    };
    //----------------------------------------------------------------------------------------------------

#if !defined(_MSC_VER)

    template<typename TException, typename... TArgs>
    void throw_wallet_ex(std::string&& loc, const TArgs&... args)
    {
      TException e(std::move(loc), args...);
      LOG_PRINT_L0(e.to_string());
      throw e;
    }

#else
    #include <boost/preprocessor/repetition/enum_binary_params.hpp>
    #include <boost/preprocessor/repetition/enum_params.hpp>
    #include <boost/preprocessor/repetition/repeat_from_to.hpp>

    template<typename TException>
    void throw_wallet_ex(std::string&& loc)
    {
      TException e(std::move(loc));
      LOG_PRINT_L0(e.to_string());
      throw e;
    }

#define GEN_throw_wallet_ex(z, n, data)                                                       \
    template<typename TException, BOOST_PP_ENUM_PARAMS(n, typename TArg)>                     \
    void throw_wallet_ex(std::string&& loc, BOOST_PP_ENUM_BINARY_PARAMS(n, const TArg, &arg)) \
    {                                                                                         \
      TException e(std::move(loc), BOOST_PP_ENUM_PARAMS(n, arg));                             \
      LOG_PRINT_L0(e.to_string());                                                            \
      throw e;                                                                                \
    }

    BOOST_PP_REPEAT_FROM_TO(1, 6, GEN_throw_wallet_ex, ~)
#endif
  }
}

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
           

#define THROW_IF_TRUE_WALLET_EX(cond, err_type, ...)                                                       \
if (cond)                                                                                                  \
{                                                                                                          \
  exception_handler();                                                                                     \
  LOG_ERROR(#cond << ". THROW EXCEPTION: " << #err_type);                                                  \
  tools::error::throw_wallet_ex<err_type>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), ## __VA_ARGS__);  \
}

#define THROW_IF_FALSE_WALLET_EX(cond, err_type, ...)                                                      \
if (!(cond))                                                                                               \
{                                                                                                          \
  exception_handler();                                                                                     \
  LOG_ERROR(#cond << ". THROW EXCEPTION: " << #err_type);                                                  \
  tools::error::throw_wallet_ex<err_type>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), ## __VA_ARGS__);  \
}



//#define THROW_IF_FALSE_WALLET_EX_MES(cond, err_type, mess, ...)                                            
#define WLT_THROW_IF_FALSE_WITH_CODE(cond, mess, error_code)                                                \
if (!(cond))                                                                                               \
{                                                                                                          \
  exception_handler();                                                                                     \
  std::stringstream ss;                                                                                    \
  ss << std::endl << "[" << error_code << "]" << mess ;                                                                                 \
  LOG_ERROR(#cond << ". THROW EXCEPTION: " << error_code << ss.str());                                                  \
  tools::error::throw_wallet_ex<tools::error::wallet_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), ss.str(), error_code); \
}


#define THROW_IF_FALSE_WALLET_EX_MES(cond, err_type, mess, ...)                                            \
if (!(cond))                                                                                               \
{                                                                                                          \
  exception_handler();                                                                                     \
  std::stringstream ss;                                                                                    \
  ss << std::endl << mess;                                                                                 \
  LOG_ERROR(#cond << ". THROW EXCEPTION: " << #err_type);                                                  \
  tools::error::throw_wallet_ex<err_type>(std::string(__FILE__ ":" STRINGIZE(__LINE__)) + ss.str(), ## __VA_ARGS__); \
}


#define THROW_IF_TRUE_WALLET_INT_ERR_EX(cond, mess)                                                                               \
  if (cond)                                                                                                                       \
  {                                                                                                                               \
    exception_handler();                                                                                                          \
    std::stringstream ss;                                                                                                         \
    ss << mess;                                                                                                                   \
    LOG_ERROR(#cond << ". THROW EXCEPTION: wallet_internal_error");                                                               \
    tools::error::throw_wallet_ex<tools::error::wallet_internal_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), ss.str());  \
  }

#define THROW_IF_TRUE_WALLET_INT_ERR_EX_NO_HANDLER(cond, mess)                                                                      \
if (cond)                                                                                                                       \
  {                                                                                                                              \
  std::stringstream ss;                                                                                                         \
  ss << mess;                                                                                                                   \
  LOG_ERROR(#cond << ". THROW EXCEPTION: wallet_internal_error");                                                               \
  tools::error::throw_wallet_ex<tools::error::wallet_internal_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), ss.str());  \
}

#define THROW_IF_FALSE_WALLET_INT_ERR_EX_NO_HANDLER(cond, mess) THROW_IF_TRUE_WALLET_INT_ERR_EX_NO_HANDLER((!(cond)), mess)

#define THROW_IF_FALSE_WALLET_INT_ERR_EX(cond, mess)      THROW_IF_TRUE_WALLET_INT_ERR_EX((!(cond)), mess)

#define THROW_IF_FALSE_WALLET_CMN_ERR_EX(cond, mess)                                                                              \
  if (!(cond))                                                                                                                    \
  {                                                                                                                               \
    std::stringstream ss;                                                                                                         \
    ss << mess;                                                                                                                   \
    LOG_ERROR(" (" << #cond << ") is FALSE. THROW EXCEPTION: wallet_common_error");                                               \
    tools::error::throw_wallet_ex<tools::error::wallet_common_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), ss.str());    \
  }
#define THROW_WALLET_CMN_ERR_EX(mess)                                                                              \
  {                                                                                                                               \
    std::stringstream ss;                                                                                                         \
    ss << mess;                                                                                                                   \
    LOG_ERROR("THROW EXCEPTION: wallet_common_error");                                               \
    tools::error::throw_wallet_ex<tools::error::wallet_common_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), ss.str());    \
  }

