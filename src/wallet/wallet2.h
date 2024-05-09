// Copyright (c) 2014-2023 Zano Project
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
#include <boost/serialization/optional.hpp>
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
#include "tor-connect/torlib/tor_lib_iface.h"
#include "currency_core/pos_mining.h"
#include "view_iface.h"
#include "wallet2_base.h"
#include "decoy_selection.h"

#define WALLET_DEFAULT_TX_SPENDABLE_AGE                               CURRENCY_HF4_MANDATORY_MIN_COINAGE
#define WALLET_POS_MINT_CHECK_HEIGHT_INTERVAL                         1


const uint64_t WALLET_MINIMUM_HEIGHT_UNSET_CONST = std::numeric_limits<uint64_t>::max();


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



  class i_wallet2_callback
  {
  public:
    enum message_severity { ms_red, ms_yellow, ms_normal };

    virtual ~i_wallet2_callback() = default;

    virtual void on_new_block(uint64_t /*height*/, const currency::block& /*block*/) {}
    virtual void on_transfer2(const wallet_public::wallet_transfer_info& wti, const std::list<wallet_public::asset_balance_entry>& balances, uint64_t total_mined) {}
    virtual void on_pos_block_found(const currency::block& /*block*/) {}
    virtual void on_sync_progress(const uint64_t& /*percents*/) {}
    virtual void on_transfer_canceled(const wallet_public::wallet_transfer_info& wti) {}
    virtual void on_message(message_severity /*severity*/, const std::string& /*m*/) {}
    virtual void on_tor_status_change(const std::string& state) {}

    //mw api
    virtual void on_mw_get_wallets(std::vector<wallet_public::wallet_entry_info>& wallets) {}
    virtual bool on_mw_select_wallet(uint64_t wallet_id) { return true; }
  };



  class test_generator;


  /*
  This structure aggregates all variables that hold current wallet synchronization state and could be reset 
  */
  struct wallet2_base_state
  {
    wallet_chain_shortener m_chain;
    uint64_t m_minimum_height = WALLET_MINIMUM_HEIGHT_UNSET_CONST;
    amount_gindex_to_transfer_id_container m_amount_gindex_to_transfer_id;
    transfer_container m_transfers;
    multisig_transfer_container m_multisig_transfers;
    payment_container m_payments;
    std::unordered_map<crypto::key_image, size_t> m_key_images;
    std::vector<wallet_public::wallet_transfer_info> m_transfer_history;
    std::unordered_map<crypto::hash, currency::transaction> m_unconfirmed_in_transfers;
    std::unordered_map<crypto::hash, tools::wallet_public::wallet_transfer_info> m_unconfirmed_txs;
    std::unordered_set<crypto::hash> m_unconfirmed_multisig_transfers;
    std::unordered_map<crypto::hash, crypto::secret_key> m_tx_keys;
    std::unordered_map<crypto::public_key, wallet_own_asset_context> m_own_asset_descriptors;
    std::unordered_map<crypto::public_key, currency::asset_descriptor_base> m_custom_assets; //assets that manually added by user
    mutable std::unordered_map<crypto::public_key, currency::asset_descriptor_base> m_whitelisted_assets; //assets that whitelisted
    escrow_contracts_container m_contracts;
    std::multimap<uint64_t, htlc_expiration_trigger> m_htlcs; //map [expired_if_more_then] -> height of expiration
    amount_gindex_to_transfer_id_container m_active_htlcs; // map [amount; gindex] -> transfer index
    std::unordered_map<crypto::hash, uint64_t> m_active_htlcs_txid; // map [txid] -> transfer index, limitation: 1 transactiom -> 1 htlc
    std::list<expiration_entry_info> m_money_expirations;
    std::unordered_map<crypto::public_key, crypto::key_image> m_pending_key_images; // (out_pk -> ki) pairs of change outputs to be added in watch-only wallet without spend sec key
    uint64_t m_last_pow_block_h = 0;
    std::list<std::pair<uint64_t, wallet_event_t>> m_rollback_events;
    std::list<std::pair<uint64_t, uint64_t> > m_last_zc_global_indexs; // <height, last_zc_global_indexs>, biggest height comes in front

    //variables that not being serialized
    std::atomic<uint64_t> m_last_bc_timestamp = 0;
    uint64_t m_height_of_start_sync = 0;
    std::atomic<uint64_t> m_last_sync_percent = 0;
    mutable uint64_t m_current_wallet_file_size = 0;
    bool m_use_assets_whitelisting = true;
    mutable std::optional<bool> m_has_bare_unspent_outputs; // recalculated each time the balance() is called

    // variables that should be part of state data object but should not be stored during serialization
    mutable std::atomic<bool> m_whitelist_updated = false;
    //===============================================================
    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      if (t_archive::is_saving::value)
      {
        LOG_PRINT_MAGENTA("Serializing file with ver: " << ver, LOG_LEVEL_0);
      }


      // do not load wallet if data version is greather than the code version 
      if (ver > WALLET_FILE_SERIALIZATION_VERSION)
      {
        LOG_PRINT_MAGENTA("Wallet file truncated due to WALLET_FILE_SERIALIZATION_VERSION is more then curren build", LOG_LEVEL_0);
        return;
      }
      if (ver < WALLET_FILE_LAST_SUPPORTED_VERSION)
      {
        LOG_PRINT_MAGENTA("Wallet file truncated due to ver(" << ver << ") is less then WALLET_FILE_LAST_SUPPORTED_VERSION", LOG_LEVEL_0);
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
          LOG_PRINT_MAGENTA("Wallet file truncated due to mismatch CURRENCY_FORMATION_VERSION", LOG_LEVEL_0);
          return;
        }
      }
      //convert from old version
      a & m_chain;
      a & m_minimum_height;
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
      a & m_htlcs;
      a & m_active_htlcs;
      a & m_active_htlcs_txid;
      a & m_own_asset_descriptors;
      a & m_custom_assets;
      a & m_rollback_events;
      a & m_whitelisted_assets;
      a & m_use_assets_whitelisting;
      if (ver <= 165)
      {
        uint64_t last_zc_global_index = 0;
        a& last_zc_global_index;
        m_last_zc_global_indexs.push_back(std::make_pair(uint64_t(0), last_zc_global_index));
        return;
      }
      a& m_last_zc_global_indexs;
      if (ver == 166 && m_last_zc_global_indexs.size())
      {
        //workaround for m_last_zc_global_indexs holding invalid index for last item
        m_last_zc_global_indexs.pop_front();
      }      
    }
  };
  

  class wallet2: public tools::tor::t_transport_state_notifier, public boost::static_visitor<void>, public wallet2_base_state
  {
    wallet2(const wallet2&) = delete;
  public:
    wallet2();
    virtual ~wallet2() {}

    static std::string transfer_flags_to_str(uint32_t flags);

       

    struct mining_context : public currency::pos_mining_context
    {
      std::string   status;

      bool          is_pos_allowed = false;
      bool          is_pos_sequence_factor_good = false;

      uint64_t      index = 0; // index in m_transfers 
      uint64_t      stake_unlock_time = 0;
      uint64_t      height = 0;
      uint64_t      starter_timestamp = 0;
      crypto::hash  last_block_hash = currency::null_hash;

      uint64_t      iterations_processed = 0;
      uint64_t      total_items_checked = 0;
      uint64_t      total_amount_checked = 0;
    };


    /*
      This might be not the best solution so far, but after discussion with @sowle we came up to conclusion 
      that passing great variety of arguments through the long call stack of different member functions of the wallet will
      only complicate codebase and make it harder to understand. 
      current_operation_context will keep pointers to some useful data, and every function that use it, should 
      make sure(!!!) that pointer got nulled before pointed object got destroyed, likely by using SET_CONTEXT_OBJ_FOR_SCOPE macro

    */
    struct current_operation_context
    {
      construct_tx_param* pconstruct_tx_param = nullptr;
      currency::finalize_tx_param* pfinalize_tx_param = nullptr;
      const mode_separate_context* pmode_separate_context = nullptr;
    };
    

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

    typedef std::unordered_map<crypto::hash, std::pair<currency::transaction, wallet_public::employed_tx_entries>> multisig_entries_map;

    struct process_transaction_context
    {
      process_transaction_context(const currency::transaction& t) : tx(t) {}
      const currency::transaction& tx;
      bool spent_own_native_inputs = false; 
      // check all outputs for spending (compare key images)
      wallet_public::employed_tx_entries employed_entries;
      bool is_pos_coinbase = false;
      bool coin_base_tx = false;
      //PoW block don't have change, so all outs supposed to be marked as "mined"
      bool is_derived_from_coinbase = false;
      size_t i = 0;
      size_t sub_i = 0;
      uint64_t height = 0;
      uint64_t timestamp = 0;
      std::unordered_map<crypto::public_key, boost::multiprecision::int128_t> total_balance_change;
      std::vector<std::string> recipients;
      std::vector<std::string> remote_aliases;
      multisig_entries_map* pmultisig_entries = nullptr;
      crypto::public_key tx_pub_key = currency::null_pkey;
      uint64_t tx_expiration_ts_median = 0;
      uint64_t max_out_unlock_time = 0;

      const crypto::hash& tx_hash() const
      {
        if (tx_hash_ == currency::null_hash)
        {
          tx_hash_ = get_transaction_hash(tx);
        } 
        return tx_hash_;
      }
    private: 
      mutable crypto::hash tx_hash_ = currency::null_hash;
    };

    struct batch_of_bare_unspent_outs
    {
      std::vector<uint64_t> tids;
      uint64_t total_amount = 0;
      bool additional_tid = false; // additional zc transfer if total_amount < min fee
      uint64_t additional_tid_amount = 0;
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

    void get_recent_transfers_history(std::vector<wallet_public::wallet_transfer_info>& trs, size_t offset, size_t count, uint64_t& total, uint64_t& last_item_index, bool exclude_mining_txs = false, bool start_from_end = true);
    bool is_defragmentation_transaction(const wallet_public::wallet_transfer_info& wti);
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
    i_wallet2_callback* get_callback() { return m_wcallback.get(); }
    void set_do_rise_transfer(bool do_rise) { m_do_rise_transfer = do_rise; }

    bool has_related_alias_entry_unconfirmed(const currency::transaction& tx);
    bool has_bare_unspent_outputs() const;
    bool get_bare_unspent_outputs_stats(std::vector<batch_of_bare_unspent_outs>& buo_txs) const;
    bool sweep_bare_unspent_outputs(const currency::account_public_address& target_address, const std::vector<batch_of_bare_unspent_outs>& tids_grouped_by_txs,
      std::function<void(size_t batch_index, const currency::transaction& tx, uint64_t amount, uint64_t fee, bool sent_ok, const std::string& err)> on_tx_sent);
    bool sweep_bare_unspent_outputs(const currency::account_public_address& target_address, const std::vector<batch_of_bare_unspent_outs>& tids_grouped_by_txs,
      size_t& total_txs_sent, uint64_t& total_amount_sent, uint64_t& total_fee, uint64_t& total_bare_outs_sent);
    void handle_unconfirmed_tx(process_transaction_context& ptc);
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
    void request_alias_registration(currency::extra_alias_entry& ai, currency::transaction& res_tx, uint64_t fee, uint64_t reward = 0, const crypto::secret_key& authority_key = currency::null_skey); // if the given reward is 0, then the actual reward value will be requested via RPC
    void request_alias_update(currency::extra_alias_entry& ai, currency::transaction& res_tx, uint64_t fee, uint64_t reward);
    bool check_available_sources(std::list<uint64_t>& amounts);

    void deploy_new_asset(const currency::asset_descriptor_base& asset_info, const std::vector<currency::tx_destination_entry>& destinations, currency::transaction& result_tx, crypto::public_key& new_asset_id);
    void emit_asset(const crypto::public_key asset_id, std::vector<currency::tx_destination_entry>& destinations, currency::transaction& result_tx);
    void update_asset(const crypto::public_key asset_id, const currency::asset_descriptor_base new_descriptor, currency::transaction& result_tx);
    void burn_asset(const crypto::public_key asset_id, uint64_t amount_to_burn, currency::transaction& result_tx);
    void transfer_asset_ownership(const crypto::public_key asset_id, const crypto::public_key& new_owner, currency::transaction& result_tx);

    bool daemon_get_asset_info(const crypto::public_key& asset_id, currency::asset_descriptor_base& adb);
    bool set_core_proxy(const std::shared_ptr<i_core_proxy>& proxy);
    void set_defragmentation_tx_settings(bool enabled, uint64_t min_outs, uint64_t max_outs, uint64_t max_allowed_amount = CURRENCY_BLOCK_REWARD, size_t decoys_count = SIZE_MAX);
    void set_pos_required_decoys_count(size_t v) { m_required_decoys_count = v; }
    void set_minimum_height(uint64_t h);
    std::shared_ptr<i_core_proxy> get_core_proxy();
    uint64_t balance() const;
    uint64_t balance(uint64_t& unloked, uint64_t& awaiting_in, uint64_t& awaiting_out, uint64_t& mined, const crypto::public_key& asset_id = currency::native_coin_asset_id) const;
    bool balance(std::unordered_map<crypto::public_key, wallet_public::asset_balance_entry_base>& balances, uint64_t& mined) const;
    bool balance(std::list<wallet_public::asset_balance_entry>& balances, uint64_t& mined) const;
    uint64_t balance(const crypto::public_key& asset_id, uint64_t& unlocked) const;
    uint64_t balance(const crypto::public_key& asset_id) const;

    uint64_t balance(uint64_t& unloked) const;

    uint64_t unlocked_balance() const;
    bool get_asset_id_info(const crypto::public_key& asset_id, currency::asset_descriptor_base& asset_info, bool& whitelist_) const;
    void transfer(uint64_t amount, const currency::account_public_address& acc, const crypto::public_key& asset_id = currency::native_coin_asset_id);
    void transfer(uint64_t amount, size_t fake_outs_count, const currency::account_public_address& acc, uint64_t fee = TX_DEFAULT_FEE, const crypto::public_key& asset_id = currency::native_coin_asset_id);
    void transfer(uint64_t amount, const currency::account_public_address& acc, currency::transaction& result_tx, const crypto::public_key& asset_id = currency::native_coin_asset_id);

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

    // PoS mining
    void do_pos_mining_prepare_entry(mining_context& cxt, size_t transfer_index);
    bool do_pos_mining_iteration(mining_context& cxt, size_t transfer_index, uint64_t ts);
    template<typename idle_condition_cb_t> //do refresh as external callback
    bool scan_pos(mining_context& cxt, std::atomic<bool>& stop, idle_condition_cb_t idle_condition_cb, const currency::core_runtime_config &runtime_config);
    bool fill_mining_context(mining_context& ctx);
    
    void get_transfers(transfer_container& incoming_transfers) const;
    std::string get_transfers_str(bool include_spent = true, bool include_unspent = true, bool show_only_unknown = false, const std::string& filter_asset_ticker = std::string{}) const;
    std::string get_balance_str() const;
    std::string get_balance_str_raw() const;

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
      uint64_t fee, size_t& outs_total, uint64_t& amount_total, size_t& outs_swept, uint64_t& amount_swept, currency::transaction* p_result_tx = nullptr, std::string* p_filename_or_unsigned_tx_blob_str = nullptr);

    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id);
    inline uint64_t get_blockchain_current_size() const {
      return m_chain.get_blockchain_current_size();
    }
    
    uint64_t get_top_block_height() const { return m_chain.get_top_block_height(); }


    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      wallet2_base_state::serialize(a, ver);
    }

    bool is_transfer_ready_to_go(const transfer_details& td, uint64_t fake_outputs_count) const;
    bool is_transfer_able_to_go(const transfer_details& td, uint64_t fake_outputs_count) const;
    uint64_t select_indices_for_transfer(std::vector<uint64_t>& ind, free_amounts_cache_type& found_free_amounts, uint64_t needed_money, uint64_t fake_outputs_count);
    bool select_indices_for_transfer(assets_selection_context& needed_money_map, uint64_t fake_outputs_count, std::vector<uint64_t>& selected_indexes);

    //PoS
    //synchronous version of function 
    bool try_mint_pos();
    bool try_mint_pos(const currency::account_public_address& miner_address); // block reward will be sent to miner_address, stake will be returned back to the wallet
    //for unit tests
    friend class ::test_generator;
    
    //next functions in public area only becausce of test_generator
    //TODO: Need refactoring - remove it back to private zone 
    void set_genesis(const crypto::hash& genesis_hash);
    bool prepare_and_sign_pos_block(const mining_context& cxt, uint64_t full_block_reward, const currency::pos_entry& pe, currency::tx_generation_context& miner_tx_tgc, currency::block& b) const;
    void process_new_blockchain_entry(const currency::block& b, 
      const currency::block_direct_data_entry& bche, 
      const crypto::hash& bl_id,
      uint64_t height);
    void process_htlc_triggers_on_block_added(uint64_t height);
    void unprocess_htlc_triggers_on_block_removed(uint64_t height);
    
    bool get_pos_entries(std::vector<currency::pos_entry>& entries); // TODO: make it const
    size_t get_pos_entries_count();

    bool build_minted_block(const mining_context& cxt);
    bool build_minted_block(const mining_context& cxt, const currency::account_public_address& miner_address);
    std::string get_extra_text_for_block(uint64_t new_block_expected_height);
    bool reset_history();
    bool is_transfer_unlocked(const transfer_details& td) const;
    bool is_transfer_unlocked(const transfer_details& td, bool for_pos_mining, uint64_t& stake_lock_time) const;
    void get_mining_history(wallet_public::mining_history& hist, uint64_t timestamp_from = 0);
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

    bool prepare_transaction(construct_tx_param& ctp, currency::finalize_tx_param& ftp, const mode_separate_context& emode_separate = mode_separate_context());

    void finalize_transaction(currency::finalize_tx_param& ftp, currency::transaction& tx, crypto::secret_key& tx_key, bool broadcast_tx, bool store_tx_secret_key = true);
    void finalize_transaction(currency::finalize_tx_param& ftp, currency::finalized_tx& result, bool broadcast_tx, bool store_tx_secret_key = true );

    std::string get_log_prefix() const { return m_log_prefix; }
    static uint64_t get_max_unlock_time_from_receive_indices(const currency::transaction& tx, const wallet_public::employed_tx_entries& td);
    bool get_utxo_distribution(std::map<uint64_t, uint64_t>& distribution);
    uint64_t get_sync_progress();
    uint64_t get_wallet_file_size()const;
    void set_use_deffered_global_outputs(bool use);
    void set_use_assets_whitelisting(bool use);
    construct_tx_param get_default_construct_tx_param_inital();
    void set_disable_tor_relay(bool disable);
    uint64_t get_default_fee() {return TX_DEFAULT_FEE;}
    uint64_t get_current_minimum_network_fee() { return TX_DEFAULT_FEE; }
    void export_transaction_history(std::ostream& ss, const std::string& format, bool include_pos_transactions = true);

    bool add_custom_asset_id(const crypto::public_key& asset_id, currency::asset_descriptor_base& asset_descriptor);
    bool delete_custom_asset_id(const crypto::public_key& asset_id);
    const std::unordered_map<crypto::public_key, currency::asset_descriptor_base>& get_local_whitelist() const;
    const std::unordered_map<crypto::public_key, currency::asset_descriptor_base>& get_global_whitelist() const;
    const std::unordered_map<crypto::public_key, tools::wallet_own_asset_context>& get_own_assets() const;

    bool load_whitelisted_tokens_if_not_loaded() const;
    bool load_whitelisted_tokens() const;

    void set_connectivity_options(unsigned int timeout);
    
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

    void set_votes_config_path(const std::string& path_to_config_file);
    const tools::wallet_public::wallet_vote_config& get_current_votes() { return m_votes_config; }

    // ionic swaps:
    bool create_ionic_swap_proposal(const wallet_public::ionic_swap_proposal_info& proposal_details, const currency::account_public_address& destination_addr, wallet_public::ionic_swap_proposal& proposal);
    bool build_ionic_swap_template(const wallet_public::ionic_swap_proposal_info& proposal_detais, const currency::account_public_address& destination_addr,
      wallet_public::ionic_swap_proposal& proposal,
      std::vector<uint64_t>& selected_transfers_for_template);
    bool get_ionic_swap_proposal_info(const std::string&raw_proposal, wallet_public::ionic_swap_proposal_info& proposal_info) const;
    bool get_ionic_swap_proposal_info(const wallet_public::ionic_swap_proposal& proposal, wallet_public::ionic_swap_proposal_info& proposal_info) const;
    bool get_ionic_swap_proposal_info(const wallet_public::ionic_swap_proposal& proposal, wallet_public::ionic_swap_proposal_info& proposal_info, wallet_public::ionic_swap_proposal_context& ionic_context) const;
    bool accept_ionic_swap_proposal(const std::string& raw_proposal, currency::transaction& result_tx);
    bool accept_ionic_swap_proposal(const wallet_public::ionic_swap_proposal& proposal, currency::transaction& result_tx);

    // Signing and auth
    bool sign_buffer(const std::string& buff, crypto::signature& sig);
    bool validate_sign(const std::string& buff, const crypto::signature& sig, const crypto::public_key& pkey);
    bool encrypt_buffer(const std::string& buff, std::string& res_buff);
    bool decrypt_buffer(const std::string& buff, std::string& res_buff);
    bool is_in_hardfork_zone(uint64_t hardfork_index) const;
    //performance inefficient call, suitable only for rare ocasions or super lazy developers
    bool proxy_to_daemon(const std::string& uri, const std::string& body, int& response_code, std::string& response_body);

    construct_tx_param get_default_construct_tx_param();

    //---------- m_rollback_events visitor ------------------------------------------------
    void operator()(const asset_register_event& e);
    void operator()(const asset_update_event& e);
    void operator()(const asset_unown_event& e);

protected: 
    epee::misc_utils::events_dispatcher m_debug_events_dispatcher;

private:

    // -------- t_transport_state_notifier ------------------------------------------------
    virtual void notify_state_change(const std::string& state_code, const std::string& details = std::string());

    void add_rollback_event(uint64_t h, const wallet_event_t& ev);
    void handle_rollback_events(uint64_t including_height);
    // ------------------------------------------------------------------------------------
    void add_transfers_to_expiration_list(const std::vector<uint64_t>& selected_transfers, const std::vector<payment_details_subtransfer>& received, uint64_t expiration, const crypto::hash& related_tx_id);
    void remove_transfer_from_expiration_list(uint64_t transfer_index);
    void load_keys(const std::string& keys_file_name, const std::string& password, uint64_t file_signature, keys_file_data& kf_data);
    void process_ado_in_new_transaction(const currency::asset_descriptor_operation& ado, process_transaction_context& ptc);
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
    bool select_transfers(assets_selection_context& needed_money_map, size_t fake_outputs_count, uint64_t dust, std::vector<uint64_t>& selected_indicies);
    void add_transfers_to_transfers_cache(const std::vector<uint64_t>& indexs);
    void add_transfer_to_transfers_cache(uint64_t amount, uint64_t index, const crypto::public_key& asset_id = currency::native_coin_asset_id);
    bool prepare_file_names(const std::wstring& file_path);
    void process_unconfirmed(const currency::transaction& tx, std::vector<std::string>& recipients, std::vector<std::string>& recipients_aliases);
    void add_sent_unconfirmed_tx(const currency::transaction& tx,
                                 const std::vector<std::string>& recipients, 
                                 const std::vector<uint64_t>& selected_indicies, 
                                 const std::vector<currency::tx_destination_entry>& splitted_dsts);

    void update_current_tx_limit();
    void prepare_wti(wallet_public::wallet_transfer_info& wti, const process_transaction_context& tx_process_context);
    void prepare_wti_decrypted_attachments(wallet_public::wallet_transfer_info& wti, const std::vector<currency::payload_items_v>& decrypted_att);    
    void handle_money(const currency::block& b, const process_transaction_context& tx_process_context);
    void load_wti_from_process_transaction_context(wallet_public::wallet_transfer_info& wti, const process_transaction_context& tx_process_context);
    bool process_payment_id_for_wti(wallet_public::wallet_transfer_info& wti, const process_transaction_context& tx_process_context);
    void add_to_last_zc_global_indexs(uint64_t h, uint64_t last_zc_output_index);
    uint64_t get_actual_zc_global_index();
    void handle_pulled_blocks(size_t& blocks_added, std::atomic<bool>& stop,
      currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& blocks);
    std::string get_alias_for_address(const std::string& addr);
    std::vector<std::string> get_aliases_for_address(const std::string& addr);
    bool is_connected_to_net();
    bool is_transfer_okay_for_pos(const transfer_details& tr, bool is_zarcanum_hf, uint64_t& stake_unlock_time) const;
    bool scan_not_compliant_unconfirmed_txs();
    const currency::transaction& get_transaction_by_id(const crypto::hash& tx_hash);
    void rise_on_transfer2(const wallet_public::wallet_transfer_info& wti);
    void process_genesis_if_needed(const currency::block& genesis, const std::vector<uint64_t>* pglobal_indexes);
    bool build_escrow_proposal(bc_services::contract_private_details& ecrow_details, uint64_t fee, uint64_t unlock_time, currency::tx_service_attachment& att, std::vector<uint64_t>& selected_indicies);
    bool prepare_tx_sources(assets_selection_context& needed_money_map, size_t fake_outputs_count, uint64_t dust_threshold, std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies);
    bool prepare_tx_sources(size_t fake_outputs_count, std::vector<currency::tx_source_entry>& sources, const std::vector<uint64_t>& selected_indicies);
    bool prepare_tx_sources(size_t fake_outputs_count, bool use_all_decoys_if_found_less_than_required, std::vector<currency::tx_source_entry>& sources, const std::vector<uint64_t>& selected_indicies);
    bool prepare_tx_sources(crypto::hash multisig_id, std::vector<currency::tx_source_entry>& sources, uint64_t& found_money);
    bool prepare_tx_sources_htlc(crypto::hash htlc_tx_id, const std::string& origin, std::vector<currency::tx_source_entry>& sources, uint64_t& found_money);
    bool prepare_tx_sources_for_defragmentation_tx(std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies, uint64_t& found_money);
    void prefetch_global_indicies_if_needed(const std::vector<uint64_t>& selected_indicies);
    assets_selection_context get_needed_money(uint64_t fee, const std::vector<currency::tx_destination_entry>& dsts);
    void prepare_tx_destinations(const assets_selection_context& needed_money_map,
      detail::split_strategy_id_t destination_split_strategy_id,
      const tx_dust_policy& dust_policy,
      const std::vector<currency::tx_destination_entry>& dsts,
      uint8_t tx_flags,
      std::vector<currency::tx_destination_entry>& final_destinations);
    void prepare_tx_destinations(uint64_t needed_money,
      uint64_t found_money,
      detail::split_strategy_id_t destination_split_strategy_id,
      const tx_dust_policy& dust_policy,
      const std::vector<currency::tx_destination_entry>& dsts,
      const crypto::public_key& asset_id,
      std::vector<currency::tx_destination_entry>& final_detinations);
    bool handle_contract(wallet_public::wallet_transfer_info& wti, const bc_services::contract_private_details& cntr, const std::vector<currency::payload_items_v>& decrypted_attach);
    bool handle_release_contract(wallet_public::wallet_transfer_info& wti, const std::string& release_instruction);
    bool handle_cancel_proposal(wallet_public::wallet_transfer_info& wti, const bc_services::escrow_cancel_templates_body& ectb, const std::vector<currency::payload_items_v>& decrypted_attach);
    bool handle_expiration_list(uint64_t tx_expiration_ts_median);
    void handle_contract_expirations(uint64_t tx_expiration_ts_median);
    uint64_t get_current_tx_version();
    void change_contract_state(wallet_public::escrow_contract_details_basic& contract, uint32_t new_state, const crypto::hash& contract_id, const wallet_public::wallet_transfer_info& wti) const;
    void change_contract_state(wallet_public::escrow_contract_details_basic& contract, uint32_t new_state, const crypto::hash& contract_id, const std::string& reason = "internal intention") const;
    void load_votes_config();
    bool is_need_to_split_outputs();
    template<typename input_t>
    bool process_input_t(const input_t& in_t, wallet2::process_transaction_context& ptc, const currency::transaction& tx);



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

    void print_source_entry(std::stringstream& output, const currency::tx_source_entry& src) const;


    void init_log_prefix();
    void load_keys2ki(bool create_if_not_exist, bool& need_to_resync);

    void send_transaction_to_network(const currency::transaction& tx);
    void add_sent_tx_detailed_info(const currency::transaction& tx, 
      const std::vector<currency::attachment_v>& decrypted_att,
      const std::vector<currency::tx_destination_entry>& destinations,
      const std::vector<uint64_t>& selected_indicies);
    void mark_transfers_as_spent(const std::vector<uint64_t>& selected_transfers, const std::string& reason = std::string());
    void mark_transfers_with_flag(const std::vector<uint64_t>& selected_transfers, uint32_t flag, const std::string& reason = std::string(), bool throw_if_flag_already_set = false);
    void clear_transfers_from_flag(const std::vector<uint64_t>& selected_transfers, uint32_t flag, const std::string& reason = std::string()) noexcept;
    void exception_handler();
    void exception_handler() const;
    uint64_t get_minimum_allowed_fee_for_contract(const crypto::hash& ms_id);
    bool generate_utxo_defragmentation_transaction_if_needed(currency::transaction& tx);
    bool store_unsigned_tx_to_file_and_reserve_transfers(const currency::finalize_tx_param& ftp, const std::string& filename, std::string* p_unsigned_tx_blob_str = nullptr);
    void check_and_throw_if_self_directed_tx_with_payment_id_requested(const construct_tx_param& ctp);
    void push_new_block_id(const crypto::hash& id, uint64_t height);
    bool lookup_item_around(uint64_t i, std::pair<uint64_t, crypto::hash>& result);
    //void get_short_chain_history(std::list<crypto::hash>& ids);
    //void check_if_block_matched(uint64_t i, const crypto::hash& id, bool& block_found, bool& block_matched, bool& full_reset_needed);
    uint64_t detach_from_block_ids(uint64_t height);
    uint64_t get_wallet_minimum_height();
    uint64_t get_directly_spent_transfer_index_by_input_in_tracking_wallet(uint64_t amount, const std::vector<currency::txout_ref_v> & key_offsets);
    uint64_t get_directly_spent_transfer_index_by_input_in_tracking_wallet(const currency::txin_to_key& intk);
    uint64_t get_directly_spent_transfer_index_by_input_in_tracking_wallet(const currency::txin_zc_input& inzc);
    uint8_t out_get_mixin_attr(const currency::tx_out_v& out_t);
    const crypto::public_key& out_get_pub_key(const currency::tx_out_v& out_t, std::list<currency::htlc_info>& htlc_info_list);
    bool expand_selection_with_zc_input(assets_selection_context& needed_money_map, uint64_t fake_outputs_count, std::vector<uint64_t>& selected_indexes);

    void push_alias_info_to_extra_according_to_hf_status(const currency::extra_alias_entry& ai, std::vector<currency::extra_v>& extra);
    void remove_transfer_from_amount_gindex_map(uint64_t tid);
    uint64_t get_alias_cost(const std::string& alias);
    detail::split_strategy_id_t get_current_split_strategy();
    void build_distribution_for_input(std::vector<uint64_t>& offsets, uint64_t own_index);
    void select_decoys(currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount & amount_entry, uint64_t own_g_index);

    static void wti_to_csv_entry(std::ostream& ss, const wallet_public::wallet_transfer_info& wti, size_t index);
    static void wti_to_txt_line(std::ostream& ss, const wallet_public::wallet_transfer_info& wti, size_t index);
    static void wti_to_json_line(std::ostream& ss, const wallet_public::wallet_transfer_info& wti, size_t index);



    /*     
     
     !!!!! IMPORTAN !!!!! 

     All variables that supposed to hold wallet state of synchronization(i.e. transfers, assets, htlc, swaps, contracts) - should 
     be placed in wallet2_base_state base class to avoid typical bugs when it's forgotten to be included in reset/resync/serialize functions     
     
     */



    currency::account_base m_account;
    bool m_watch_only;
    std::string m_log_prefix; // part of pub address, prefix for logging functions
    std::wstring m_wallet_file;
    std::wstring m_pending_ki_file;
    std::string m_password;
   

    bool m_do_rise_transfer;
    
    bool m_defragmentation_tx_enabled;
    uint64_t m_max_allowed_output_amount_for_defragmentation_tx;
    uint64_t m_min_utxo_count_for_defragmentation_tx;
    uint64_t m_max_utxo_count_for_defragmentation_tx;
    size_t m_decoys_count_for_defragmentation_tx;
    
    size_t m_required_decoys_count;
    pending_ki_file_container_t m_pending_key_images_file_container;
    uint64_t m_upper_transaction_size_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value

    std::atomic<bool> m_stop;
    std::shared_ptr<i_core_proxy> m_core_proxy;
    std::shared_ptr<i_wallet2_callback> m_wcallback;


    currency::core_runtime_config m_core_runtime_config;    
    //optimization for big wallets and batch tx
    free_assets_amounts_cache_type m_found_free_amounts;
    uint64_t m_fake_outputs_count;
    std::string m_miner_text_info;

    bool m_use_deffered_global_outputs;
    bool m_disable_tor_relay;
    mutable current_operation_context m_current_context;

    std::string m_votes_config_path;
    tools::wallet_public::wallet_vote_config m_votes_config;

    uint64_t m_last_known_daemon_height = 0;

    //this needed to access wallets state in coretests, for creating abnormal blocks and tranmsactions
    friend class test_generator;
  }; // class wallet2

} // namespace tools

BOOST_CLASS_VERSION(tools::wallet2, WALLET_FILE_SERIALIZATION_VERSION)

BOOST_CLASS_VERSION(tools::wallet_public::wallet_transfer_info, 12)

namespace boost
{
  namespace serialization
  {

//     template <class Archive>
//     inline void serialize(Archive &a, tools::transfer_details &x, const boost::serialization::version_type ver)
//     {
//       a & x.m_global_output_index;
//       a & x.m_key_image;
//       a & static_cast<tools::transfer_details_base&>(x);
//       if (ver < 3)
//         return;
//       a & x.varian_options;
//     }

    
    /*template <class Archive>
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
      a & x.remote_aliases;
      a & x.comment;
      a & x.contract;
      a & x.selected_indicies;
      a & x.marketplace_entries;
      a & x.unlock_time;
      if (ver < 10)
        return;
      a & x.service_entries;
    }*/

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

  }
}

namespace tools
{
  template<typename input_t>
  bool wallet2::process_input_t(const input_t& in_t, wallet2::process_transaction_context& ptc, const currency::transaction& tx)
  {
    // check if this input spends our output
    uint64_t tr_index = UINT64_MAX;

    if (this->is_auditable() && this->is_watch_only())
    {
      // tracking wallet, assuming all outputs are spent directly because of mix_attr = 1
      tr_index = this->get_directly_spent_transfer_index_by_input_in_tracking_wallet(in_t);
    }
    else
    {
      // wallet with spend secret key -- we can calculate own key images and then search by them
      auto it = m_key_images.find(in_t.k_image);
      if (it != m_key_images.end())
      {
        tr_index = it->second;
      }
    }

    if (tr_index != UINT64_MAX)
    {
      transfer_details& td = m_transfers[tr_index];
      ptc.total_balance_change[td.get_asset_id()] -= td.amount();
      if (td.is_native_coin())
      {
        ptc.spent_own_native_inputs = true;
      }
      uint32_t flags_before = td.m_flags;
      td.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;
      td.m_spent_height = ptc.height;
      if (ptc.coin_base_tx && td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER)
        ptc.is_derived_from_coinbase = true;
      else
        ptc.is_derived_from_coinbase = false;

      if (td.is_native_coin())
      {
        WLT_LOG_L0("Spent native coins, transfer #" << tr_index << ", amount: " << currency::print_money_brief(td.amount()) << (td.is_zc() ? " (hidden), with tx: " : ", with tx: ") << get_transaction_hash(tx) << ", at height " << ptc.height <<
          "; flags: " << flags_before << " -> " << td.m_flags);
      }
      else
      {
        WLT_LOG_L0("Spent asset " << print16(td.get_asset_id()) << " , transfer #" << tr_index << ", amount: " << currency::print_money_brief(td.amount()) << ", with tx: " << get_transaction_hash(tx) << ", at height " << ptc.height <<
          "; flags: " << flags_before << " -> " << td.m_flags);
      }
      
      ptc.employed_entries.spent.push_back(wallet_public::employed_tx_entry{ ptc.i, td.amount(), td.get_asset_id()});
      remove_transfer_from_expiration_list(tr_index);
    }
    return true;
  }

  template<typename idle_condition_cb_t> //do refresh as external callback
  bool wallet2::scan_pos(mining_context& cxt,
    std::atomic<bool>& stop,
    idle_condition_cb_t idle_condition_cb,
    const currency::core_runtime_config &runtime_config)
  {
    cxt.status = API_RETURN_CODE_NOT_FOUND;
    uint64_t timstamp_last_idle_call = runtime_config.get_core_time();
    cxt.iterations_processed = 0;

    uint64_t ts_from = cxt.starter_timestamp; // median ts of last BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW blocks
    ts_from = ts_from - (ts_from % POS_SCAN_STEP) + POS_SCAN_STEP;
    uint64_t ts_to = runtime_config.get_core_time() + CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT - 5;
    ts_to = ts_to - (ts_to % POS_SCAN_STEP);
    CHECK_AND_ASSERT_MES(ts_to > ts_from, false, "scan_pos: ts_to <= ts_from: " << ts_to << ", " << ts_from);
    uint64_t ts_middle = (ts_to + ts_from) / 2;
    ts_middle -= ts_middle % POS_SCAN_STEP;
    uint64_t ts_window = std::min(ts_middle - ts_from, ts_to - ts_middle);

    for (size_t transfer_index = 0; transfer_index != m_transfers.size(); transfer_index++)
    {
      auto& tr = m_transfers[transfer_index];

      uint64_t stake_unlock_time = 0;
      if (!is_transfer_okay_for_pos(tr, cxt.zarcanum, stake_unlock_time))
        continue;

      
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

      do_pos_mining_prepare_entry(cxt, transfer_index);
      cxt.total_items_checked++;
      cxt.total_amount_checked += tr.amount();
      while(step <= ts_window)
      {
        //check every WALLET_POS_MINT_CHECK_HEIGHT_INTERVAL seconds wheither top block changed, if so - break the loop 
        if (runtime_config.get_core_time() - timstamp_last_idle_call > WALLET_POS_MINT_CHECK_HEIGHT_INTERVAL)
        {
          if (!idle_condition_cb())
          {
            LOG_PRINT_L0("Detected new block, minting interrupted");
            cxt.status = API_RETURN_CODE_NOT_FOUND;
            return false;
          }
          timstamp_last_idle_call = runtime_config.get_core_time();
        }


        uint64_t ts = go_past ? ts_middle - step : ts_middle + step;
        if (ts < ts_from || ts > ts_to)
        {
          next_turn();
          continue;
        }
        PROFILE_FUNC("general_mining_iteration");
        if (stop)
          return false;

        cxt.iterations_processed++;
        if (do_pos_mining_iteration(cxt, transfer_index, ts))
        {
          cxt.index = transfer_index;
          cxt.stake_unlock_time = stake_unlock_time;
          cxt.status = API_RETURN_CODE_OK;
          return true;
        }
        
        next_turn();
      }
    }
    cxt.status = API_RETURN_CODE_NOT_FOUND;
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
