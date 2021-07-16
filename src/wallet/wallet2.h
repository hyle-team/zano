// Copyright (c) 2014-2020 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <memory>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/serialization/extended_type_info.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <atomic>


#include "include_base_utils.h"
#include "profile_tools.h"
#include "sync_locked_object.h"


#include "currency_core/currency_boost_serialization.h"
#include "currency_core/account_boost_serialization.h"
#include "currency_core/currency_format_utils.h"

#include "common/make_hashable.h"
#include "wallet_public_structs_defs.h"
#include "currency_core/currency_format_utils.h"
#include "common/unordered_containers_boost_serialization.h"
#include "common/atomics_boost_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "crypto/chacha8.h"
#include "crypto/hash.h"
#include "core_rpc_proxy.h"
#include "core_default_rpc_proxy.h"
#include "wallet_errors.h"
#include "eos/portable_archive.hpp"
#include "currency_core/core_runtime_config.h"
#include "currency_core/bc_offers_serialization.h"
#include "currency_core/bc_escrow_service.h"
#include "common/pod_array_file_container.h"
#include "wallet_chain_shortener.h"


#define WALLET_DEFAULT_TX_SPENDABLE_AGE                               10
#define WALLET_POS_MINT_CHECK_HEIGHT_INTERVAL                         1

#define WALLET_DEFAULT_POS_MINT_PACKING_SIZE                          100

#define   WALLET_TRANSFER_DETAIL_FLAG_SPENT                            uint32_t(1 << 0)
#define   WALLET_TRANSFER_DETAIL_FLAG_BLOCKED                          uint32_t(1 << 1)       
#define   WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION      uint32_t(1 << 2)
#define   WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER                   uint32_t(1 << 3)
#define   WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION             uint32_t(1 << 4) // transfer is reserved for cold-signing (unsigned tx was created and passed for signing)
#define   WALLET_TRANSFER_DETAIL_FLAG_HTLC_REDEEM                      uint32_t(1 << 5) // for htlc keeps info if this htlc belong as redeem or as refund


const uint64_t WALLET_MINIMUM_HEIGHT_UNSET_CONST = std::numeric_limits<uint64_t>::max();
const uint64_t WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED = std::numeric_limits<uint64_t>::max();

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "wallet"

// wallet-specific logging functions
#define WLT_LOG(msg, level) LOG_PRINT("[W:" << m_log_prefix << "] " << msg, level)
#define WLT_LOG_L0(msg) LOG_PRINT_L0("[W:" << m_log_prefix << "] " << msg)
#define WLT_LOG_L1(msg) LOG_PRINT_L1("[W:" << m_log_prefix << "] " << msg)
#define WLT_LOG_L2(msg) LOG_PRINT_L2("[W:" << m_log_prefix << "] " << msg)
#define WLT_LOG_L3(msg) LOG_PRINT_L3("[W:" << m_log_prefix << "] " << msg)
#define WLT_LOG_L4(msg) LOG_PRINT_L4("[W:" << m_log_prefix << "] " << msg)
#define WLT_LOG_ERROR(msg) LOG_ERROR("[W:" << m_log_prefix << "] " << msg)
#define WLT_LOG_BLUE(msg, log_level)    LOG_PRINT_BLUE("[W:" << m_log_prefix << "] " << msg, log_level)
#define WLT_LOG_CYAN(msg, log_level)    LOG_PRINT_CYAN("[W:" << m_log_prefix << "] " << msg, log_level)
#define WLT_LOG_GREEN(msg, log_level)   LOG_PRINT_GREEN("[W:" << m_log_prefix << "] " << msg, log_level)
#define WLT_LOG_MAGENTA(msg, log_level) LOG_PRINT_MAGENTA("[W:" << m_log_prefix << "] " << msg, log_level)
#define WLT_LOG_RED(msg, log_level)     LOG_PRINT_RED("[W:" << m_log_prefix << "] " << msg, log_level)
#define WLT_LOG_YELLOW(msg, log_level)  LOG_PRINT_YELLOW("[W:" << m_log_prefix << "] " << msg, log_level)
#define WLT_CHECK_AND_ASSERT_MES(expr, ret, msg) CHECK_AND_ASSERT_MES(expr, ret, "[W:" << m_log_prefix << "] " << msg)
#define WLT_CHECK_AND_ASSERT_MES_NO_RET(expr, msg) CHECK_AND_ASSERT_MES_NO_RET(expr, "[W:" << m_log_prefix << "] " << msg)
#define WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(cond, msg) THROW_IF_FALSE_WALLET_INT_ERR_EX(cond, "[W:" << m_log_prefix << "] " << msg)
#define WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(cond, msg) THROW_IF_FALSE_WALLET_CMN_ERR_EX(cond, "[W:" << m_log_prefix << "] " << msg)
#define WLT_THROW_IF_FALSE_WALLET_EX_MES(cond, exception_t, msg, ...) THROW_IF_FALSE_WALLET_EX_MES(cond, exception_t, "[W:" << m_log_prefix << "] " << msg, ## __VA_ARGS__)




class test_generator;

namespace tools
{
#pragma pack(push, 1)
  struct wallet_file_binary_header
  {
    uint64_t m_signature;
    uint16_t m_cb_keys;
    //uint64_t m_cb_body; <-- this field never used, soo replace it with two other variables "m_ver" + and "m_reserved"
    uint32_t m_ver;
    uint32_t m_reserved; //for future use
  };
#pragma pack (pop)


  struct money_transfer2_details
  {
    std::vector<size_t> receive_indices;
    std::vector<size_t> spent_indices;
  };


  class i_wallet2_callback
  {
  public:
    enum message_severity { ms_red, ms_yellow, ms_normal };

    virtual ~i_wallet2_callback() = default;

    virtual void on_new_block(uint64_t /*height*/, const currency::block& /*block*/) {}
    virtual void on_transfer2(const wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined) {}
    virtual void on_pos_block_found(const currency::block& /*block*/) {}
    virtual void on_sync_progress(const uint64_t& /*percents*/) {}
    virtual void on_transfer_canceled(const wallet_public::wallet_transfer_info& wti) {}
    virtual void on_message(message_severity /*severity*/, const std::string& /*m*/) {}
  };

  struct tx_dust_policy
  {
    uint64_t dust_threshold;
    bool add_to_fee;
    currency::account_public_address addr_for_dust;

    tx_dust_policy(uint64_t a_dust_threshold = DEFAULT_DUST_THRESHOLD, bool an_add_to_fee = true, currency::account_public_address an_addr_for_dust = currency::account_public_address())
      : dust_threshold(a_dust_threshold)
      , add_to_fee(an_add_to_fee)
      , addr_for_dust(an_addr_for_dust)
    {
    }

    BEGIN_SERIALIZE_OBJECT()
      FIELD(dust_threshold)
      FIELD(add_to_fee)
      FIELD(addr_for_dust)
    END_SERIALIZE()
  };

  class test_generator;

#pragma pack(push, 1)
  struct out_key_to_ki
  {
    crypto::public_key out_key;
    crypto::key_image  key_image;
  };
#pragma pack(pop)

  typedef tools::pod_array_file_container<out_key_to_ki> pending_ki_file_container_t;

  namespace detail
  {
    //----------------------------------------------------------------------------------------------------
    inline void digit_split_strategy(const std::vector<currency::tx_destination_entry>& dsts,
      const currency::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<currency::tx_destination_entry>& splitted_dsts, uint64_t& dust, uint64_t max_output_allowed)
    {
      splitted_dsts.clear();
      dust = 0;

      for(auto& de : dsts)
      {
        if (de.addr.size() > 1)
        {
          //for multisig we don't split
          splitted_dsts.push_back(de);
        }
        else if (de.htlc_options.expiration != 0)
        {
          //for htlc we don't do split
          splitted_dsts.push_back(de);
        }
        else
        {
          currency::decompose_amount_into_digits(de.amount, dust_threshold,
            [&](uint64_t chunk) { splitted_dsts.push_back(currency::tx_destination_entry(chunk, de.addr)); },
            [&](uint64_t a_dust) { splitted_dsts.push_back(currency::tx_destination_entry(a_dust, de.addr)); }, max_output_allowed);
        }
      }

      if (change_dst.amount > 0)
      {
        if (change_dst.addr.size() > 1)
        {
          //for multisig we don't split
          splitted_dsts.push_back(change_dst);
        }
        else
        {
          currency::decompose_amount_into_digits(change_dst.amount, dust_threshold,
            [&](uint64_t chunk) { splitted_dsts.push_back(currency::tx_destination_entry(chunk, change_dst.addr)); },
            [&](uint64_t a_dust) { dust = a_dust; }, max_output_allowed);
        }
      }
    }
    //----------------------------------------------------------------------------------------------------
    inline void null_split_strategy(const std::vector<currency::tx_destination_entry>& dsts,
      const currency::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<currency::tx_destination_entry>& splitted_dsts, uint64_t& dust, uint64_t max_output_allowed)
    {
      splitted_dsts = dsts;

      dust = 0;
      uint64_t change = change_dst.amount;
      if (0 < dust_threshold)
      {
        for (uint64_t order = 10; order <= 10 * dust_threshold; order *= 10)
        {
          uint64_t dust_candidate = change_dst.amount % order;
          uint64_t change_candidate = (change_dst.amount / order) * order;
          if (dust_candidate <= dust_threshold)
          {
            dust = dust_candidate;
            change = change_candidate;
          }
          else
          {
            break;
          }
        }
      }

      if (0 != change)
      {
        splitted_dsts.push_back(currency::tx_destination_entry(change, change_dst.addr));
      }
    }
    //----------------------------------------------------------------------------------------------------
    inline void void_split_strategy(const std::vector<currency::tx_destination_entry>& dsts,
      const currency::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<currency::tx_destination_entry>& splitted_dsts, uint64_t& dust, uint64_t max_output_allowed)
    {
      splitted_dsts = dsts;
      if (change_dst.amount > 0)
        splitted_dsts.push_back(change_dst);
    }
    //----------------------------------------------------------------------------------------------------
    enum split_strategy_id_t { ssi_none = 0, ssi_digit = 1, ssi_null = 2, ssi_void = 3 };
    //----------------------------------------------------------------------------------------------------
    inline bool apply_split_strategy_by_id(split_strategy_id_t id, const std::vector<currency::tx_destination_entry>& dsts,
      const currency::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<currency::tx_destination_entry>& splitted_dsts, uint64_t& dust, uint64_t max_output_allowed)
    {
      switch (id)
      {
      case ssi_digit:
        digit_split_strategy(dsts, change_dst, dust_threshold, splitted_dsts, dust, max_output_allowed);
        return true;
      case ssi_null:
        null_split_strategy(dsts, change_dst, dust_threshold, splitted_dsts, dust, max_output_allowed);
        return true;
      case ssi_void:
        void_split_strategy(dsts, change_dst, dust_threshold, splitted_dsts, dust, max_output_allowed);
        return true;
      default:
        return false;
      }
    }

  } // namespace detail

  struct construct_tx_param
  {
    // preparing data for tx
    std::vector<currency::tx_destination_entry> dsts;
    size_t fake_outputs_count;
    uint64_t fee;
    tx_dust_policy dust_policy;
    crypto::hash multisig_id;
    uint8_t flags;
    uint8_t split_strategy_id;
    bool mark_tx_as_complete;

    crypto::hash htlc_tx_id;
    std::string htlc_origin;

    // constructing tx
    uint64_t unlock_time;
    std::vector<currency::extra_v> extra;
    std::vector<currency::attachment_v> attachments;
    currency::account_public_address crypt_address;
    uint8_t tx_outs_attr;
    bool shuffle;
    bool perform_packing;
  };

//   struct currency::finalize_tx_param
//   {
//     uint64_t unlock_time;
//     std::vector<currency::extra_v> extra;
//     std::vector<currency::attachment_v> attachments;
//     currency::account_public_address crypt_address;
//     uint8_t tx_outs_attr;
//     bool shuffle;
//     uint8_t flags;
//     crypto::hash multisig_id;
//     std::vector<currency::tx_source_entry> sources;
//     std::vector<uint64_t> selected_transfers;
//     std::vector<currency::tx_destination_entry> prepared_destinations;
// 
//     crypto::public_key spend_pub_key;  // only for validations
// 
//     BEGIN_SERIALIZE_OBJECT()
//       FIELD(unlock_time)
//       FIELD(extra)
//       FIELD(attachments)
//       FIELD(crypt_address)
//       FIELD(tx_outs_attr)
//       FIELD(shuffle)
//       FIELD(flags)
//       FIELD(multisig_id)
//       FIELD(sources)
//       FIELD(selected_transfers)
//       FIELD(prepared_destinations)
//       FIELD(spend_pub_key)
//     END_SERIALIZE()
//   };
// 
//   struct currency::finalized_tx
//   {
//     currency::transaction tx;
//     crypto::secret_key    one_time_key;
//     currency::finalize_tx_param     ftp;
//     std::vector<serializable_pair<uint64_t, crypto::key_image>> outs_key_images; // pairs (out_index, key_image) for each change output
// 
//     BEGIN_SERIALIZE_OBJECT()
//       FIELD(tx)
//       FIELD(one_time_key)
//       FIELD(ftp)
//       FIELD(outs_key_images)
//     END_SERIALIZE()
//   };

  class wallet2
  {
    wallet2(const wallet2&) = delete;
  public:
    wallet2();

    static std::string transfer_flags_to_str(uint32_t flags);
    static std::string transform_tx_to_str(const currency::transaction& tx);
    static currency::transaction transform_str_to_tx(const std::string& tx_str);



    struct transaction_wallet_info
    {
      uint64_t m_block_height;
      uint64_t m_block_timestamp;
      currency::transaction m_tx;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(m_block_height)
        KV_SERIALIZE(m_block_timestamp)
        KV_SERIALIZE_CUSTOM(m_tx, std::string, tools::wallet2::transform_tx_to_str, tools::wallet2::transform_str_to_tx)
      END_KV_SERIALIZE_MAP()
    };

    static const transaction_wallet_info& transform_ptr_to_value(const std::shared_ptr<transaction_wallet_info>& a);
    static std::shared_ptr<transaction_wallet_info> transform_value_to_ptr(const transaction_wallet_info& d);
    
    struct transfer_details_base;
    static uint64_t transfer_details_base_to_amount(const transfer_details_base& tdb);
    static std::string transfer_details_base_to_tx_hash(const transfer_details_base& tdb);

    struct transfer_details_base
    {
      std::shared_ptr<transaction_wallet_info> m_ptx_wallet_info;
      uint64_t m_internal_output_index;
      uint64_t m_spent_height;
      uint32_t m_flags;

      uint64_t amount() const { return m_ptx_wallet_info->m_tx.vout[m_internal_output_index].amount; }
      const currency::tx_out& output() const { return m_ptx_wallet_info->m_tx.vout[m_internal_output_index]; }
      uint8_t mix_attr() const { return output().target.type() == typeid(currency::txout_to_key) ? boost::get<const currency::txout_to_key&>(output().target).mix_attr : UINT8_MAX; }
      crypto::hash tx_hash() const { return get_transaction_hash(m_ptx_wallet_info->m_tx); }
      bool is_spent() const { return m_flags & WALLET_TRANSFER_DETAIL_FLAG_SPENT; }
      bool is_spendable() const { return (m_flags & (WALLET_TRANSFER_DETAIL_FLAG_SPENT | WALLET_TRANSFER_DETAIL_FLAG_BLOCKED | WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION | WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION)) == 0; }
      bool is_reserved_for_escrow() const { return ( (m_flags & WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION) != 0 );  }

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CUSTOM(m_ptx_wallet_info, const transaction_wallet_info&, tools::wallet2::transform_ptr_to_value, tools::wallet2::transform_value_to_ptr)
        KV_SERIALIZE(m_internal_output_index)
        KV_SERIALIZE(m_spent_height)
        KV_SERIALIZE(m_flags)
        KV_SERIALIZE_EPHEMERAL_N(uint64_t, tools::wallet2::transfer_details_base_to_amount, "amount")
        KV_SERIALIZE_EPHEMERAL_N(std::string, tools::wallet2::transfer_details_base_to_tx_hash, "tx_id")
      END_KV_SERIALIZE_MAP()

    };


    struct transfer_details_extra_option_htlc_info
    {
      std::string origin;  //this field filled only if htlc had been redeemed
      crypto::hash redeem_tx_id;
    };


    typedef boost::variant<transfer_details_extra_option_htlc_info, currency::tx_payer> transfer_details_extra_options_v;

    struct transfer_details : public transfer_details_base
    {
      uint64_t m_global_output_index;
      crypto::key_image m_key_image; //TODO: key_image stored twice :(
      std::vector<transfer_details_extra_options_v> varian_options;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(m_global_output_index)
        KV_SERIALIZE_POD_AS_HEX_STRING(m_key_image)
        KV_CHAIN_BASE(transfer_details_base)
      END_KV_SERIALIZE_MAP()
    };

    //used in wallet 
    struct htlc_expiration_trigger
    {
      bool is_wallet_owns_redeem; //specify if this HTLC belong to this wallet by pkey_redeem or by pkey_refund
      uint64_t transfer_index;
    };


    struct payment_details
    {
      crypto::hash m_tx_hash;
      uint64_t m_amount;
      uint64_t m_block_height;
      uint64_t m_unlock_time;
    };

    struct mining_context
    {
      currency::COMMAND_RPC_SCAN_POS::request sp;
      currency::COMMAND_RPC_SCAN_POS::response rsp;
      currency::wide_difficulty_type basic_diff;
      currency::stake_modifier_type sm;
    };

    struct expiration_entry_info
    {
      std::vector<uint64_t> selected_transfers;
      uint64_t change_amount;
      uint64_t expiration_time;
      crypto::hash related_tx_id; // tx id which caused money lock, if any (ex: escrow proposal transport tx)
    };



    typedef std::unordered_multimap<std::string, payment_details> payment_container;

    typedef std::deque<transfer_details> transfer_container;
    typedef std::unordered_map<crypto::hash, transfer_details_base> multisig_transfer_container;
    typedef std::unordered_map<crypto::hash, tools::wallet_public::escrow_contract_details_basic> escrow_contracts_container;
    typedef std::map<uint64_t, std::set<size_t> > free_amounts_cache_type;
    typedef std::unordered_map<std::pair<uint64_t, uint64_t>, uint64_t> amount_gindex_to_transfer_id_container; // maps [amount; gindex] -> tid


    struct keys_file_data_old
    {
      crypto::chacha8_iv iv;
      std::string account_data;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(account_data)
      END_SERIALIZE()
    };

    struct keys_file_data
    {
      uint8_t             version;
      crypto::chacha8_iv  iv;
      std::string         account_data;

      static keys_file_data from_old(const keys_file_data_old& v)
      {
        keys_file_data result = AUTO_VAL_INIT(result);
        result.iv = v.iv;
        result.account_data = v.account_data;
        return result;
      }

      DEFINE_SERIALIZATION_VERSION(1)
      BEGIN_SERIALIZE_OBJECT()
        VERSION_ENTRY(version)
        FIELD(iv)
        FIELD(account_data)
      END_SERIALIZE()
    };

    void assign_account(const currency::account_base& acc);
    void generate(const std::wstring& path, const std::string& password, bool auditable_wallet);
    void restore(const std::wstring& path, const std::string& pass, const std::string& seed_or_tracking_seed, bool tracking_wallet, const std::string& seed_password);
    void load(const std::wstring& path, const std::string& password);
    void store();
    void store(const std::wstring& path);
    void store(const std::wstring& path, const std::string& password);
    void store_watch_only(const std::wstring& path, const std::string& password) const;
    bool store_keys(std::string& buff, const std::string& password, wallet2::keys_file_data& keys_file_data, bool store_as_watch_only = false);
    std::wstring get_wallet_path()const { return m_wallet_file; }
    std::string get_wallet_password()const { return m_password; }
    currency::account_base& get_account() { return m_account; }
    const currency::account_base& get_account() const { return m_account; }

    void get_recent_transfers_history(std::vector<wallet_public::wallet_transfer_info>& trs, size_t offset, size_t count, uint64_t& total, uint64_t& last_item_index, bool exclude_mining_txs = false);
    uint64_t get_recent_transfers_total_count();
    uint64_t get_transfer_entries_count();
    void get_unconfirmed_transfers(std::vector<wallet_public::wallet_transfer_info>& trs, bool exclude_mining_txs = false);
    void init(const std::string& daemon_address = "http://localhost:8080");
    bool deinit();

    void stop() { m_stop.store(true, std::memory_order_relaxed); }
    void reset_creation_time(uint64_t timestamp);

    //i_wallet2_callback* callback() const { return m_wcallback; }
    //void callback(i_wallet2_callback* callback) { m_callback = callback; }
    void callback(std::shared_ptr<i_wallet2_callback> callback) { m_wcallback = callback; m_do_rise_transfer = (callback != nullptr); }
    void set_do_rise_transfer(bool do_rise) { m_do_rise_transfer = do_rise; }

    bool has_related_alias_entry_unconfirmed(const currency::transaction& tx);
    void scan_tx_pool(bool& has_related_alias_in_unconfirmed);
    void refresh();
    void refresh(size_t & blocks_fetched);
    void refresh(size_t & blocks_fetched, bool& received_money, std::atomic<bool>& stop);
    bool refresh(size_t & blocks_fetched, bool& received_money, bool& ok, std::atomic<bool>& stop);
    void refresh(std::atomic<bool>& stop);
    
    void resend_unconfirmed();
    void push_offer(const bc_services::offer_details_ex& od, currency::transaction& res_tx);
    void cancel_offer_by_id(const crypto::hash& tx_id, uint64_t of_ind, uint64_t fee, currency::transaction& tx);
    void update_offer_by_id(const crypto::hash& tx_id, uint64_t of_ind, const bc_services::offer_details_ex& od, currency::transaction& res_tx);
    void request_alias_registration(const currency::extra_alias_entry& ai, currency::transaction& res_tx, uint64_t fee, uint64_t reward);
    void request_alias_update(currency::extra_alias_entry& ai, currency::transaction& res_tx, uint64_t fee, uint64_t reward);
    bool check_available_sources(std::list<uint64_t>& amounts);
    

    bool set_core_proxy(const std::shared_ptr<i_core_proxy>& proxy);
    void set_pos_mint_packing_size(uint64_t new_size);
    void set_minimum_height(uint64_t h);
    std::shared_ptr<i_core_proxy> get_core_proxy();
    uint64_t balance() const;
    uint64_t balance(uint64_t& unloked, uint64_t& awaiting_in, uint64_t& awaiting_out, uint64_t& mined) const;
    uint64_t balance(uint64_t& unloked) const;
    uint64_t unlocked_balance() const;

    void transfer(uint64_t amount, const currency::account_public_address& acc);
    void transfer(uint64_t amount, const currency::account_public_address& acc, currency::transaction& result_tx);

    void transfer(const std::vector<currency::tx_destination_entry>& dsts,
                  size_t fake_outputs_count, 
                  uint64_t unlock_time, 
                  uint64_t fee, 
                  const std::vector<currency::extra_v>& extra, 
                  const std::vector<currency::attachment_v>& attachments, 
                  detail::split_strategy_id_t destination_split_strategy_id,
                  const tx_dust_policy& dust_policy);

    void transfer(const std::vector<currency::tx_destination_entry>& dsts,
      size_t fake_outputs_count,
      uint64_t unlock_time,
      uint64_t fee,
      const std::vector<currency::extra_v>& extra,
      const std::vector<currency::attachment_v>& attachments,
      detail::split_strategy_id_t destination_split_strategy_id,
      const tx_dust_policy& dust_policy,
      currency::transaction &tx,
      uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED,
      bool shuffle = true,
      uint8_t flags = 0,
      bool send_to_network = true,
      std::string* p_unsigned_filename_or_tx_blob_str = nullptr);

    void transfer(const std::vector<currency::tx_destination_entry>& dsts, 
                  size_t fake_outputs_count, 
                  uint64_t unlock_time, 
                  uint64_t fee, 
                  const std::vector<currency::extra_v>& extra, 
                  const std::vector<currency::attachment_v>& attachments);
    
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, 
                  size_t fake_outputs_count, 
                  uint64_t unlock_time, 
                  uint64_t fee, 
                  const std::vector<currency::extra_v>& extra, 
                  const std::vector<currency::attachment_v>& attachments, 
                  currency::transaction& tx);

    void transfer(construct_tx_param& ctp,
                  currency::transaction &tx,
                  bool send_to_network,
                  std::string* p_unsigned_filename_or_tx_blob_str = nullptr);
    
    void transfer(construct_tx_param& ctp,
                  currency::finalized_tx& result,
                  bool send_to_network,
                  std::string* p_unsigned_filename_or_tx_blob_str = nullptr);


    template<typename destination_split_strategy_t>
    void transfer_from_contract(
      const std::list<currency::account_keys>& owner_keys,
      crypto::hash multisig_id,
      const std::vector<currency::tx_destination_entry>& dsts,
      size_t fake_outputs_count,
      uint64_t unlock_time,
      uint64_t fee,
      const std::vector<currency::extra_v>& extra,
      const std::vector<currency::attachment_v>& attachments,
      destination_split_strategy_t destination_split_strategy,
      const tx_dust_policy& dust_policy,
      currency::transaction &tx,
      uint8_t tx_outs_attr,
      bool shuffle,
      uint8_t flags,
      bool send_to_network);


    void build_escrow_release_templates(crypto::hash multisig_id,
      uint64_t fee,
      currency::transaction& tx_release_template,
      currency::transaction& tx_burn_template,
      const bc_services::contract_private_details& ecrow_details);

    void build_escrow_cancel_template(crypto::hash multisig_id,
      uint64_t expiration_period,
      currency::transaction& tx_cancel_template,
      const bc_services::contract_private_details& ecrow_details);



    void build_escrow_template(const bc_services::contract_private_details& ecrow_detaild,
      size_t fake_outputs_count,
      uint64_t unlock_time,
      uint64_t expiration_time,
      uint64_t b_release_fee,
      const std::string& payment_id,
      currency::transaction& tx, 
      std::vector<uint64_t>& selected_transfers, 
      crypto::secret_key& one_time_key);


    void send_escrow_proposal(const wallet_public::create_proposal_param& wp,
      currency::transaction &proposal_tx,
      currency::transaction &escrow_template_tx);

    void send_escrow_proposal(const bc_services::contract_private_details& ecrow_detaild,
      size_t fake_outputs_count,
      uint64_t unlock_time,
      uint64_t expiration_period,
      uint64_t fee,
      uint64_t b_release_fee,
      const std::string& payment_id,
      currency::transaction &proposal_tx,
      currency::transaction &escrow_template_tx);

    bool check_connection();
    template<typename idle_condition_cb_t> //do refresh as external callback
    static bool scan_pos(mining_context& cxt, std::atomic<bool>& stop, idle_condition_cb_t idle_condition_cb, const currency::core_runtime_config &runtime_config);
    bool fill_mining_context(mining_context& ctx);
    void get_transfers(wallet2::transfer_container& incoming_transfers) const;
    std::string get_transfers_str(bool include_spent /*= true*/, bool include_unspent /*= true*/) const;

    // Returns all payments by given id in unspecified order
    void get_payments(const std::string& payment_id, std::list<payment_details>& payments, uint64_t min_height = 0) const;

    // callback: (const wallet_public::wallet_transfer_info& wti) -> bool, true -- continue, false -- stop
    template<typename callback_t>
    void enumerate_transfers_history(callback_t cb, bool enumerate_forward) const;

    // callback: (const wallet_public::wallet_transfer_info& wti) -> bool, true -- continue, false -- stop
    template<typename callback_t>
    void enumerate_unconfirmed_transfers(callback_t cb) const;

    bool is_watch_only() const { return m_watch_only; }
    bool is_auditable() const { return m_account.get_public_address().is_auditable(); }
    void sign_transfer(const std::string& tx_sources_blob, std::string& signed_tx_blob, currency::transaction& tx);
    void sign_transfer_files(const std::string& tx_sources_file, const std::string& signed_tx_file, currency::transaction& tx);
    void submit_transfer(const std::string& signed_tx_blob, currency::transaction& tx);
    void submit_transfer_files(const std::string& signed_tx_file, currency::transaction& tx);

    void sweep_below(size_t fake_outs_count, const currency::account_public_address& destination_addr, uint64_t threshold_amount, const currency::payment_id_t& payment_id,
      uint64_t fee, size_t& outs_total, uint64_t& amount_total, size_t& outs_swept, currency::transaction* p_result_tx = nullptr, std::string* p_filename_or_unsigned_tx_blob_str = nullptr);

    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id);
    inline uint64_t get_blockchain_current_size() const {
      return m_chain.get_blockchain_current_size();
    }
    
    uint64_t get_top_block_height() const { return m_chain.get_top_block_height(); }

    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      // do not load wallet if data version is greather than the code version 
      if (ver > WALLET_FILE_SERIALIZATION_VERSION)
      {
        WLT_LOG_MAGENTA("Wallet file truncated due to WALLET_FILE_SERIALIZATION_VERSION is more then curren build", LOG_LEVEL_0);
        return;
      }

      if (ver < 149)
      {
        WLT_LOG_MAGENTA("Wallet file truncated due to old version: " << ver, LOG_LEVEL_0);
        return;
      }

      if (t_archive::is_saving::value)
      {
        uint64_t formation_ver = CURRENCY_FORMATION_VERSION;
        a & formation_ver;
      }
      else
      {
        uint64_t formation_ver = 0;
        a & formation_ver;
        if (formation_ver != CURRENCY_FORMATION_VERSION)
        {
          WLT_LOG_MAGENTA("Wallet file truncated due to mismatch CURRENCY_FORMATION_VERSION", LOG_LEVEL_0);
          return;
        }
      }
      //convert from old version
      if (ver < 150)
      {
        WLT_LOG_MAGENTA("Converting blockchain into a short form...", LOG_LEVEL_0);
        std::vector<crypto::hash> old_blockchain;
        a & old_blockchain;
        uint64_t count = 0;
        for (auto& h : old_blockchain)
        {
          m_chain.push_new_block_id(h, count);
          count++;
        }
        WLT_LOG_MAGENTA("Converting done", LOG_LEVEL_0);
      }
      else
      {
        a & m_chain;
        a & m_minimum_height;
      }

      // v151: m_amount_gindex_to_transfer_id added
      if (ver >= 151)
        a & m_amount_gindex_to_transfer_id;

      a & m_transfers;
      a & m_multisig_transfers;
      a & m_key_images;      
      a & m_unconfirmed_txs;
      a & m_unconfirmed_multisig_transfers;
      a & m_payments;
      a & m_transfer_history;
      a & m_unconfirmed_in_transfers;
      a & m_contracts;
      a & m_money_expirations;
      a & m_pending_key_images;
      a & m_tx_keys;
      a & m_last_pow_block_h;

      //after processing
      if (ver < 152)
      {
        wipeout_extra_if_needed(m_transfer_history);
      }
      
      if (ver < 153)
        return;
      
      a & m_htlcs;
      a & m_active_htlcs;
      a & m_active_htlcs_txid;

    }

    void wipeout_extra_if_needed(std::vector<wallet_public::wallet_transfer_info>& transfer_history);
    bool is_transfer_ready_to_go(const transfer_details& td, uint64_t fake_outputs_count);
    bool is_transfer_able_to_go(const transfer_details& td, uint64_t fake_outputs_count);
    uint64_t select_indices_for_transfer(std::vector<uint64_t>& ind, free_amounts_cache_type& found_free_amounts, uint64_t needed_money, uint64_t fake_outputs_count);

    //PoS
    //synchronous version of function 
    bool try_mint_pos();
    bool try_mint_pos(const currency::account_public_address& miner_address); // block reward will be sent to miner_address, stake will be returned back to the wallet
    //for unit tests
    friend class ::test_generator;
    
    //next functions in public area only becausce of test_generator
    //TODO: Need refactoring - remove it back to private zone 
    void set_genesis(const crypto::hash& genesis_hash);
    bool prepare_and_sign_pos_block(currency::block& b,
      const currency::pos_entry& pos_info,
      const crypto::public_key& source_tx_pub_key,
      uint64_t in_tx_output_index,
      const std::vector<const crypto::public_key*>& keys_ptrs);
    void process_new_blockchain_entry(const currency::block& b, 
      const currency::block_direct_data_entry& bche, 
      const crypto::hash& bl_id,
      uint64_t height);
    void process_htlc_triggers_on_block_added(uint64_t height);
    void unprocess_htlc_triggers_on_block_removed(uint64_t height);
    bool get_pos_entries(currency::COMMAND_RPC_SCAN_POS::request& req);
    bool build_minted_block(const currency::COMMAND_RPC_SCAN_POS::request& req, const currency::COMMAND_RPC_SCAN_POS::response& rsp, uint64_t new_block_expected_height = UINT64_MAX);
    bool build_minted_block(const currency::COMMAND_RPC_SCAN_POS::request& req, const currency::COMMAND_RPC_SCAN_POS::response& rsp, const currency::account_public_address& miner_address, uint64_t new_block_expected_height = UINT64_MAX);
    bool reset_history();
    bool is_transfer_unlocked(const transfer_details& td) const;
    bool is_transfer_unlocked(const transfer_details& td, bool for_pos_mining, uint64_t& stake_lock_time) const;
    void get_mining_history(wallet_public::mining_history& hist);
    void set_core_runtime_config(const currency::core_runtime_config& pc);  
    currency::core_runtime_config& get_core_runtime_config();
    bool backup_keys(const std::string& path);
    bool reset_password(const std::string& pass);
    bool is_password_valid(const std::string& pass);
    bool get_actual_offers(std::list<bc_services::offer_details_ex>& offers);
    bool process_contract_info(wallet_public::wallet_transfer_info& wti, const std::vector<currency::payload_items_v>& decrypted_attach);
    bool handle_proposal(wallet_public::wallet_transfer_info& wti, const bc_services::proposal_body& prop);
    void accept_proposal(const crypto::hash& contract_id, uint64_t b_acceptance_fee, currency::transaction* p_acceptance_tx = nullptr);
    void finish_contract(const crypto::hash& contract_id, const std::string& release_type, currency::transaction* p_release_tx = nullptr);
    void request_cancel_contract(const crypto::hash& contract_id, uint64_t fee, uint64_t expiration_period, currency::transaction* p_cancellation_proposal_tx = nullptr);
    void accept_cancel_contract(const crypto::hash& contract_id, currency::transaction* p_cancellation_acceptance_tx = nullptr);

    void scan_tx_to_key_inputs(std::vector<uint64_t>& found_transfers, const currency::transaction& tx);
    void dump_trunsfers(std::stringstream& ss, bool verbose = true) const;
    std::string dump_trunsfers(bool verbose = false) const;
    void dump_key_images(std::stringstream& ss);
    void get_multisig_transfers(multisig_transfer_container& ms_transfers);
    const multisig_transfer_container& get_multisig_transfers() const { return m_multisig_transfers; }
    void set_miner_text_info(const std::string& mti) { m_miner_text_info = mti; }
    
    bool get_transfer_info_by_key_image(const crypto::key_image& ki, transfer_details& td, size_t& i);
    bool get_transfer_info_by_index(size_t i, transfer_details& td);
    size_t scan_for_collisions(std::unordered_map<crypto::key_image, std::list<size_t> >& key_images);
    size_t fix_collisions();
    size_t scan_for_transaction_entries(const crypto::hash& tx_id, const crypto::key_image& ki, std::list<transfer_details>& details);

    bool get_contracts(escrow_contracts_container& contracts);
    const std::list<expiration_entry_info>& get_expiration_entries() const { return m_money_expirations; };
    bool get_tx_key(const crypto::hash &txid, crypto::secret_key &tx_key) const;

    void prepare_transaction(construct_tx_param& ctp, currency::finalize_tx_param& ftp, const currency::transaction& tx_for_mode_separate = currency::transaction());

    void finalize_transaction(const currency::finalize_tx_param& ftp, currency::transaction& tx, crypto::secret_key& tx_key, bool broadcast_tx, bool store_tx_secret_key = true);
    void finalize_transaction(const currency::finalize_tx_param& ftp, currency::finalized_tx& result, bool broadcast_tx, bool store_tx_secret_key = true );

    std::string get_log_prefix() const { return m_log_prefix; }
    static uint64_t get_max_unlock_time_from_receive_indices(const currency::transaction& tx, const money_transfer2_details& td);
    bool get_utxo_distribution(std::map<uint64_t, uint64_t>& distribution);
    uint64_t get_sync_progress();
    uint64_t get_wallet_file_size()const;
    void set_use_deffered_global_outputs(bool use);
    construct_tx_param get_default_construct_tx_param_inital();
    
    /*
    create_htlc_proposal: if htlc_hash == null_hash, then this wallet is originator of the atomic process, and 
    we use deterministic origin, if given some particular htlc_hash, then we use this hash, and this means that 
    opener-hash will be given by other side
    */
    void create_htlc_proposal(uint64_t amount, const currency::account_public_address& addr, uint64_t lock_blocks_count, currency::transaction &tx, const crypto::hash& htlc_hash, std::string &origin);
    void get_list_of_active_htlc(std::list<wallet_public::htlc_entry_info>& htlcs, bool only_redeem_txs);
    void redeem_htlc(const crypto::hash& htlc_tx_id, const std::string& origin, currency::transaction& result_tx);
    void redeem_htlc(const crypto::hash& htlc_tx_id, const std::string& origin);
    bool check_htlc_redeemed(const crypto::hash& htlc_tx_id, std::string& origin, crypto::hash& redeem_tx_id);

private:

    void add_transfers_to_expiration_list(const std::vector<uint64_t>& selected_transfers, uint64_t expiration, uint64_t change_amount, const crypto::hash& related_tx_id);
    void remove_transfer_from_expiration_list(uint64_t transfer_index);
    void load_keys(const std::string& keys_file_name, const std::string& password, uint64_t file_signature, keys_file_data& kf_data);
    void process_new_transaction(const currency::transaction& tx, uint64_t height, const currency::block& b, const std::vector<uint64_t>* pglobal_indexes);
    void fetch_tx_global_indixes(const currency::transaction& tx, std::vector<uint64_t>& goutputs_indexes);
    void fetch_tx_global_indixes(const std::list<std::reference_wrapper<const currency::transaction>>& txs, std::vector<std::vector<uint64_t>>& goutputs_indexes);
    void detach_blockchain(uint64_t including_height);
    bool extract_offers_from_transfer_entry(size_t i, std::unordered_map<crypto::hash, bc_services::offer_details_ex>& offers_local);
    bool select_my_offers(std::list<bc_services::offer_details_ex>& offers);
    bool clear();
    bool reset_all();
    bool on_idle();
    void unserialize_block_complete_entry(const currency::COMMAND_RPC_GET_BLOCKS_FAST::response& serialized,
      currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& unserialized);
    void pull_blocks(size_t& blocks_added, std::atomic<bool>& stop);
    bool prepare_free_transfers_cache(uint64_t fake_outputs_count);
    uint64_t select_transfers(uint64_t needed_money, size_t fake_outputs_count, uint64_t dust, std::vector<uint64_t>& selected_indicies);
    void add_transfers_to_transfers_cache(const std::vector<uint64_t>& indexs);
    void add_transfer_to_transfers_cache(uint64_t amount, uint64_t index);
    bool prepare_file_names(const std::wstring& file_path);
    void process_unconfirmed(const currency::transaction& tx, std::vector<std::string>& recipients, std::vector<std::string>& recipients_aliases);
    void add_sent_unconfirmed_tx(const currency::transaction& tx,
                                 const std::vector<std::string>& recipients, 
                                 const std::vector<uint64_t>& selected_indicies, 
                                 const std::vector<currency::tx_destination_entry>& splitted_dsts);
    bool read_money_transfer2_details_from_tx(const currency::transaction& tx,
                                                        const std::vector<currency::tx_destination_entry>& splitted_dsts, 
                                                        wallet_public::wallet_transfer_info_details& wtd);

    void update_current_tx_limit();
    void prepare_wti(wallet_public::wallet_transfer_info& wti, uint64_t height, uint64_t timestamp, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td);
    void prepare_wti_decrypted_attachments(wallet_public::wallet_transfer_info& wti, const std::vector<currency::payload_items_v>& decrypted_att);
    void handle_money_received2(const currency::block& b,
                                const currency::transaction& tx, 
                                uint64_t amount, 
                                const money_transfer2_details& td);
    void handle_money_spent2(const currency::block& b,  
                             const currency::transaction& in_tx, 
                             uint64_t amount, 
                             const money_transfer2_details& td, 
                             const std::vector<std::string>& recipients,
                             const std::vector<std::string>& recipients_aliases);
    void handle_pulled_blocks(size_t& blocks_added, std::atomic<bool>& stop,
      currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& blocks);
    std::string get_alias_for_address(const std::string& addr);
    static bool build_kernel(const currency::pos_entry& pe, const currency::stake_modifier_type& stake_modifier, currency::stake_kernel& kernel, uint64_t& coindays_weight, uint64_t timestamp);
    bool is_connected_to_net();
    bool is_transfer_okay_for_pos(const transfer_details& tr, uint64_t& stake_unlock_time);
    bool scan_unconfirmed_outdate_tx();
    const currency::transaction& get_transaction_by_id(const crypto::hash& tx_hash);
    void rise_on_transfer2(const wallet_public::wallet_transfer_info& wti);
    void process_genesis_if_needed(const currency::block& genesis);
    bool build_escrow_proposal(bc_services::contract_private_details& ecrow_details, uint64_t fee, uint64_t unlock_time, currency::tx_service_attachment& att, std::vector<uint64_t>& selected_indicies);
    bool prepare_tx_sources(uint64_t needed_money, size_t fake_outputs_count, uint64_t dust_threshold, std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies, uint64_t& found_money);
    bool prepare_tx_sources(size_t fake_outputs_count, std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies, uint64_t& found_money);
    bool prepare_tx_sources(crypto::hash multisig_id, std::vector<currency::tx_source_entry>& sources, uint64_t& found_money);
    bool prepare_tx_sources_htlc(crypto::hash htlc_tx_id, const std::string& origin, std::vector<currency::tx_source_entry>& sources, uint64_t& found_money);
    bool prepare_tx_sources_for_packing(uint64_t items_to_pack, size_t fake_outputs_count, std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies, uint64_t& found_money);
    void prefetch_global_indicies_if_needed(std::vector<uint64_t>& selected_indicies);
    uint64_t get_needed_money(uint64_t fee, const std::vector<currency::tx_destination_entry>& dsts);
    void prepare_tx_destinations(uint64_t needed_money,
      uint64_t found_money,
      detail::split_strategy_id_t destination_split_strategy_id,
      const tx_dust_policy& dust_policy,
      const std::vector<currency::tx_destination_entry>& dsts,
      std::vector<currency::tx_destination_entry>& final_detinations);
    bool handle_contract(wallet_public::wallet_transfer_info& wti, const bc_services::contract_private_details& cntr, const std::vector<currency::payload_items_v>& decrypted_attach);
    bool handle_release_contract(wallet_public::wallet_transfer_info& wti, const std::string& release_instruction);
    bool handle_cancel_proposal(wallet_public::wallet_transfer_info& wti, const bc_services::escrow_cancel_templates_body& ectb, const std::vector<currency::payload_items_v>& decrypted_attach);
    bool handle_expiration_list(uint64_t tx_expiration_ts_median);
    void handle_contract_expirations(uint64_t tx_expiration_ts_median);

    void change_contract_state(wallet_public::escrow_contract_details_basic& contract, uint32_t new_state, const crypto::hash& contract_id, const wallet_public::wallet_transfer_info& wti) const;
    void change_contract_state(wallet_public::escrow_contract_details_basic& contract, uint32_t new_state, const crypto::hash& contract_id, const std::string& reason = "internal intention") const;
    

    const construct_tx_param& get_default_construct_tx_param();

    uint64_t get_tx_expiration_median() const;

    void print_tx_sent_message(const currency::transaction& tx, const std::string& description, uint64_t fee = UINT64_MAX);

    // Validates escrow template tx in assumption it's related to wallet's account (wallet's account is either A or B party in escrow process)
    bool validate_escrow_proposal(const wallet_public::wallet_transfer_info& wti, const bc_services::proposal_body& prop,
      std::vector<currency::payload_items_v>& decrypted_items, crypto::hash& ms_id, bc_services::contract_private_details& cpd);

    bool validate_escrow_release(const currency::transaction& tx, bool release_type_normal, const bc_services::contract_private_details& cpd,
      const currency::txout_multisig& source_ms_out, const crypto::hash& ms_id, size_t source_ms_out_index, const currency::transaction& source_tx, const currency::account_keys& a_keys) const;

    bool validate_escrow_contract(const wallet_public::wallet_transfer_info& wti, const bc_services::contract_private_details& cpd, bool is_a,
      const std::vector<currency::payload_items_v>& decrypted_items, crypto::hash& ms_id, bc_services::escrow_relese_templates_body& rtb);

    bool validate_escrow_cancel_release(const currency::transaction& tx, const wallet_public::wallet_transfer_info& wti, const bc_services::escrow_cancel_templates_body& ectb,
      const std::vector<currency::payload_items_v>& decrypted_items, crypto::hash& ms_id, bc_services::contract_private_details& cpd, const currency::transaction& source_tx,
      size_t source_ms_out_index, const currency::account_keys& b_keys, uint64_t minimum_release_fee) const;
      
    bool validate_escrow_cancel_proposal(const wallet_public::wallet_transfer_info& wti, const bc_services::escrow_cancel_templates_body& ectb,
      const std::vector<currency::payload_items_v>& decrypted_items, crypto::hash& ms_id, bc_services::contract_private_details& cpd,
      const currency::transaction& proposal_template_tx);

    
    void fill_transfer_details(const currency::transaction& tx, const tools::money_transfer2_details& td, tools::wallet_public::wallet_transfer_info_details& res_td) const;
    void print_source_entry(const currency::tx_source_entry& src) const;

    void init_log_prefix();
    void load_keys2ki(bool create_if_not_exist, bool& need_to_resync);

    void send_transaction_to_network(const currency::transaction& tx);
    void add_sent_tx_detailed_info(const currency::transaction& tx,
      const std::vector<currency::tx_destination_entry>& destinations,
      const std::vector<uint64_t>& selected_indicies);
    void mark_transfers_as_spent(const std::vector<uint64_t>& selected_transfers, const std::string& reason = std::string());
    void mark_transfers_with_flag(const std::vector<uint64_t>& selected_transfers, uint32_t flag, const std::string& reason = std::string(), bool throw_if_flag_already_set = false);
    void clear_transfers_from_flag(const std::vector<uint64_t>& selected_transfers, uint32_t flag, const std::string& reason = std::string()) noexcept;
    void exception_handler();
    void exception_handler() const;
    uint64_t get_minimum_allowed_fee_for_contract(const crypto::hash& ms_id);
    bool generate_packing_transaction_if_needed(currency::transaction& tx, uint64_t fake_outputs_number);
    bool store_unsigned_tx_to_file_and_reserve_transfers(const currency::finalize_tx_param& ftp, const std::string& filename, std::string* p_unsigned_tx_blob_str = nullptr);
    void check_and_throw_if_self_directed_tx_with_payment_id_requested(const construct_tx_param& ctp);
    void push_new_block_id(const crypto::hash& id, uint64_t height);
    bool lookup_item_around(uint64_t i, std::pair<uint64_t, crypto::hash>& result);
    //void get_short_chain_history(std::list<crypto::hash>& ids);
    //void check_if_block_matched(uint64_t i, const crypto::hash& id, bool& block_found, bool& block_matched, bool& full_reset_needed);
    uint64_t detach_from_block_ids(uint64_t height);
    uint64_t get_wallet_minimum_height();
    uint64_t get_directly_spent_transfer_id_by_input_in_tracking_wallet(const currency::txin_to_key& intk);

    void push_alias_info_to_extra_according_to_hf_status(const currency::extra_alias_entry& ai, std::vector<currency::extra_v>& extra);
    void remove_transfer_from_amount_gindex_map(uint64_t tid);

    currency::account_base m_account;
    bool m_watch_only;
    std::string m_log_prefix; // part of pub address, prefix for logging functions
    std::wstring m_wallet_file;
    std::wstring m_pending_ki_file;
    std::string m_password;
    uint64_t m_minimum_height;

    std::atomic<uint64_t> m_last_bc_timestamp; 
    bool m_do_rise_transfer;
    uint64_t m_pos_mint_packing_size;

    transfer_container m_transfers;
    multisig_transfer_container m_multisig_transfers;
    amount_gindex_to_transfer_id_container m_amount_gindex_to_transfer_id;
    payment_container m_payments;
    std::unordered_map<crypto::key_image, size_t> m_key_images;
    std::unordered_map<crypto::public_key, crypto::key_image> m_pending_key_images; // (out_pk -> ki) pairs of change outputs to be added in watch-only wallet without spend sec key
    pending_ki_file_container_t m_pending_key_images_file_container;
    uint64_t m_upper_transaction_size_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value

    std::atomic<bool> m_stop;
    std::vector<wallet_public::wallet_transfer_info> m_transfer_history;
    std::unordered_map<crypto::hash, currency::transaction> m_unconfirmed_in_transfers;
    std::unordered_map<crypto::hash, tools::wallet_public::wallet_transfer_info> m_unconfirmed_txs;
    std::unordered_set<crypto::hash> m_unconfirmed_multisig_transfers;
    std::unordered_map<crypto::hash, crypto::secret_key> m_tx_keys;

    std::multimap<uint64_t, htlc_expiration_trigger> m_htlcs; //map [expired_if_more_then] -> height of expiration
    amount_gindex_to_transfer_id_container m_active_htlcs; // map [amount; gindex] -> transfer index
    std::unordered_map<crypto::hash, uint64_t> m_active_htlcs_txid; // map [txid] -> transfer index, limitation: 1 transactiom -> 1 htlc

    std::shared_ptr<i_core_proxy> m_core_proxy;
    std::shared_ptr<i_wallet2_callback> m_wcallback;
    uint64_t m_height_of_start_sync;
    std::atomic<uint64_t> m_last_sync_percent;
    uint64_t m_last_pow_block_h;
    currency::core_runtime_config m_core_runtime_config;
    escrow_contracts_container m_contracts;
    wallet_chain_shortener m_chain;
    std::list<expiration_entry_info> m_money_expirations;
    //optimization for big wallets and batch tx

    free_amounts_cache_type m_found_free_amounts;
    uint64_t m_fake_outputs_count;
    std::string m_miner_text_info;

    mutable uint64_t m_current_wallet_file_size;
    bool m_use_deffered_global_outputs;
    //this needed to access wallets state in coretests, for creating abnormal blocks and tranmsactions
    friend class test_generator;
 
  }; // class wallet2

} // namespace tools

BOOST_CLASS_VERSION(tools::wallet2, WALLET_FILE_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(tools::wallet_public::wallet_transfer_info, 10)
BOOST_CLASS_VERSION(tools::wallet2::transfer_details, 3)

namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transaction_wallet_info &x, const boost::serialization::version_type ver)
    {
      a & x.m_block_height;
      a & x.m_block_timestamp;
      a & x.m_tx;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transfer_details_base &x, const boost::serialization::version_type ver)
    {
      a & x.m_ptx_wallet_info;
      a & x.m_internal_output_index;
      a & x.m_flags;
      a & x.m_spent_height;
    }

   
    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transfer_details_extra_option_htlc_info &x, const boost::serialization::version_type ver)
    {
      a & x.origin;
    }


    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_global_output_index;
      a & x.m_key_image;
      a & static_cast<tools::wallet2::transfer_details_base&>(x);
      if (ver < 3)
        return;
      a & x.varian_options;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::htlc_expiration_trigger &x, const boost::serialization::version_type ver)
    {
      a & x.is_wallet_owns_redeem;
      a & x.transfer_index;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::payment_details& x, const boost::serialization::version_type ver)
    {
      a & x.m_tx_hash;
      a & x.m_amount;
      a & x.m_block_height;
      a & x.m_unlock_time;
    }
    
    template <class Archive>
    inline void serialize(Archive& a, tools::wallet_public::wallet_transfer_info_details& x, const boost::serialization::version_type ver)
    {
      a & x.rcv;
      a & x.spn;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet_public::wallet_transfer_info& x, const boost::serialization::version_type ver)
    {      

      a & x.amount;
      a & x.timestamp;
      a & x.tx_hash;
      a & x.height;
      a & x.tx_blob_size;
      a & x.payment_id;
      a & x.remote_addresses; 
      a & x.is_income;
      a & x.td;
      a & x.tx;
      a & x.recipients_aliases;
      a & x.comment;
      a & x.contract;
      a & x.selected_indicies;
      a & x.marketplace_entries;
      a & x.unlock_time;
      if (ver < 10)
        return;
      a & x.service_entries;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet_public::escrow_contract_details_basic& x, const boost::serialization::version_type ver)
    {
      a & x.state;
      a & x.is_a;
      a & x.private_detailes;
      a & x.expiration_time;
      a & x.cancel_expiration_time;
      a & x.timestamp;
      a & x.height;
      a & x.payment_id;
      //is not kv_serialization map
      a & x.proposal;
      a & x.release_body;
      a & x.cancel_body;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet_public::escrow_contract_details& x, const boost::serialization::version_type ver)
    {
      a & static_cast<tools::wallet_public::escrow_contract_details_basic&>(x);
      a & x.contract_id;
    }


    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::expiration_entry_info& x, const boost::serialization::version_type ver)
    {
      a & x.expiration_time;
      a & x.change_amount;
      a & x.selected_transfers;
      a & x.related_tx_id;
    }

    


  }
}

namespace tools
{
  template<typename idle_condition_cb_t> //do refresh as external callback
  bool wallet2::scan_pos(mining_context& cxt,
    std::atomic<bool>& stop,
    idle_condition_cb_t idle_condition_cb,
    const currency::core_runtime_config &runtime_config)
  {
    cxt.rsp.status = API_RETURN_CODE_NOT_FOUND;
    uint64_t timstamp_start = runtime_config.get_core_time();
    uint64_t timstamp_last_idle_call = runtime_config.get_core_time();
    cxt.rsp.iterations_processed = 0;

    for (size_t i = 0; i != cxt.sp.pos_entries.size(); i++)
    {
      //set timestamp starting from timestamp%POS_SCAN_STEP = 0
      uint64_t adjusted_starter_timestamp = timstamp_start - POS_SCAN_STEP;
      adjusted_starter_timestamp = POS_SCAN_STEP * 2 - (adjusted_starter_timestamp%POS_SCAN_STEP) + adjusted_starter_timestamp;
      
      bool go_past = true;
      uint64_t step = 0;
      
      auto next_turn = [&](){
        if (!step)
        {
          step += POS_SCAN_STEP;
        }
        else if (go_past)
        {
          go_past = false;
        }
        else
        {
          go_past = true;
          step += POS_SCAN_STEP;
        }
      };

      while(step <= POS_SCAN_WINDOW)
      {

        //check every WALLET_POS_MINT_CHECK_HEIGHT_INTERVAL seconds if top block changes, in case - break loop 
        if (runtime_config.get_core_time() - timstamp_last_idle_call > WALLET_POS_MINT_CHECK_HEIGHT_INTERVAL)
        {
          if (!idle_condition_cb())
          {
            LOG_PRINT_L0("Detected new block, minting interrupted");
            cxt.rsp.status = API_RETURN_CODE_NOT_FOUND;
            return false;
          }
          timstamp_last_idle_call = runtime_config.get_core_time();
        }


        uint64_t ts = go_past ? adjusted_starter_timestamp - step : adjusted_starter_timestamp + step;
        if (ts < cxt.rsp.starter_timestamp)
        {
          next_turn();
          continue;
        }
        PROFILE_FUNC("general_mining_iteration");
        if (stop)
          return false;
        currency::stake_kernel sk = AUTO_VAL_INIT(sk);
        uint64_t coindays_weight = 0;
        build_kernel(cxt.sp.pos_entries[i], cxt.sm, sk, coindays_weight, ts);
        crypto::hash kernel_hash;
        {
          PROFILE_FUNC("calc_hash");
          kernel_hash = crypto::cn_fast_hash(&sk, sizeof(sk));
        }

        currency::wide_difficulty_type this_coin_diff = cxt.basic_diff / coindays_weight;
        bool check_hash_res;
        {
          PROFILE_FUNC("check_hash");
          check_hash_res = currency::check_hash(kernel_hash, this_coin_diff);
          ++cxt.rsp.iterations_processed;
        }
        if (check_hash_res)
        {
          //found kernel
          LOG_PRINT_GREEN("Found kernel: amount=" << currency::print_money(cxt.sp.pos_entries[i].amount) << ENDL
            << "difficulty_basic=" << cxt.basic_diff << ", diff for this coin: " << this_coin_diff << ENDL
            << "index=" << cxt.sp.pos_entries[i].index << ENDL
            << "kernel info: " << ENDL
            << print_stake_kernel_info(sk) << ENDL 
            << "kernel_hash(proof): " << kernel_hash,
            LOG_LEVEL_0);
          cxt.rsp.index = i;
          cxt.rsp.block_timestamp = ts;
          cxt.rsp.status = API_RETURN_CODE_OK;
          return true;
        }
        
        next_turn();
        
        
      }
    }
    cxt.rsp.status = API_RETURN_CODE_NOT_FOUND;
    return false;
  }

  template<typename callback_t>
  void wallet2::enumerate_transfers_history(callback_t cb, bool enumerate_forward) const
  {
    if (enumerate_forward)
    {
      for(auto it = m_transfer_history.begin(); it != m_transfer_history.end(); ++it)
        if (!cb(*it))
          break;
    }
    else
    {
      for(auto it = m_transfer_history.rbegin(); it != m_transfer_history.rend(); ++it)
        if (!cb(*it))
          break;
    }
  }

  template<typename callback_t>
  void wallet2::enumerate_unconfirmed_transfers(callback_t cb) const
  {
    for (auto& el : m_unconfirmed_txs)
      if (!cb(el.second))
        break;
  }

} // namespace tools

#if !defined(KEEP_WALLET_LOG_MACROS)
#undef WLT_LOG
#undef WLT_LOG_L0
#undef WLT_LOG_L1
#undef WLT_LOG_L2
#undef WLT_LOG_L3
#undef WLT_LOG_L4
#undef WLT_LOG_ERROR
#undef WLT_LOG_BLUE
#undef WLT_LOG_CYAN
#undef WLT_LOG_GREEN
#undef WLT_LOG_MAGENTA
#undef WLT_LOG_RED
#undef WLT_LOG_YELLOW
#undef WLT_CHECK_AND_ASSERT_MES
#undef WLT_CHECK_AND_ASSERT_MES_NO_RET
#undef WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX
#undef WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX
#undef WLT_THROW_IF_FALSE_WALLET_EX_MES
#endif


#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "wallet"
ENABLE_CHANNEL_BY_DEFAULT("wallet");
