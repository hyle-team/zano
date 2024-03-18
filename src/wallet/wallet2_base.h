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
#include "currency_core/core_runtime_config.h"
#include "currency_core/bc_offers_serialization.h"
#include "currency_core/bc_escrow_service.h"
#include "common/pod_array_file_container.h"
#include "wallet_chain_shortener.h"
#include "tor-connect/torlib/tor_lib_iface.h"
#include "currency_core/pos_mining.h"
#include "view_iface.h"


#define   WALLET_TRANSFER_DETAIL_FLAG_SPENT                            uint32_t(1 << 0)
#define   WALLET_TRANSFER_DETAIL_FLAG_BLOCKED                          uint32_t(1 << 1)       
#define   WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION      uint32_t(1 << 2)
#define   WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER                   uint32_t(1 << 3)
#define   WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION             uint32_t(1 << 4) // transfer is reserved for cold-signing (unsigned tx was created and passed for signing)
#define   WALLET_TRANSFER_DETAIL_FLAG_HTLC_REDEEM                      uint32_t(1 << 5) // for htlc keeps info if this htlc belong as redeem or as refund



namespace tools
{

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

      for (auto& de : dsts)
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
            [&](uint64_t chunk) { splitted_dsts.push_back(currency::tx_destination_entry(chunk, de.addr, de.asset_id)); },
            [&](uint64_t a_dust) { splitted_dsts.push_back(currency::tx_destination_entry(a_dust, de.addr, de.asset_id)); }, max_output_allowed);
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
      splitted_dsts.insert(splitted_dsts.end(), dsts.begin(), dsts.end());
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

  struct tx_dust_policy
  {
    uint64_t dust_threshold = 0;
    bool add_to_fee = false;
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


  struct construct_tx_param
  {
    // preparing data for tx
    std::vector<currency::tx_destination_entry> dsts;
    size_t fake_outputs_count = 0;
    uint64_t fee = 0;
    tx_dust_policy dust_policy;
    crypto::hash multisig_id = currency::null_hash;
    uint8_t flags = 0;
    uint8_t split_strategy_id = 0;
    bool mark_tx_as_complete = false;

    crypto::hash htlc_tx_id;
    std::string htlc_origin;

    // constructing tx
    uint64_t unlock_time = 0;
    std::vector<currency::extra_v> extra;
    std::vector<currency::attachment_v> attachments;
    currency::account_public_address crypt_address;
    uint8_t tx_outs_attr = 0;
    bool shuffle = false;
    bool create_utxo_defragmentation_tx = false;
    bool need_at_least_1_zc = false;
    //crypto::secret_key asset_deploy_control_key = currency::null_skey;
    currency::thirdparty_sign_handler* pthirdparty_sign_handler = nullptr;
    crypto::public_key ado_current_asset_owner = currency::null_pkey;
  };

  struct mode_separate_context
  {
    currency::transaction tx_for_mode_separate;
    view::ionic_swap_proposal_info proposal_info;
    bool escrow = false;
  };


  struct selection_for_amount
  {
    uint64_t needed_amount = 0;
    uint64_t found_amount = 0;
    //std::vector<uint64_t> selected_indicies;
  };
  typedef std::unordered_map<crypto::public_key, selection_for_amount> assets_selection_context;

  //general rollback mechanism
  struct asset_register_event
  {
    crypto::public_key asset_id = currency::null_pkey;
    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(asset_id)
    END_BOOST_SERIALIZATION()

  };

  struct wallet_own_asset_context: public currency::asset_descriptor_base
  {
    bool thirdparty_custody = false;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE_BASE_CLASS(currency::asset_descriptor_base)
      BOOST_SERIALIZE(thirdparty_custody)
    END_BOOST_SERIALIZATION()
  };

  struct asset_update_event
  {
    crypto::public_key asset_id = currency::null_pkey;
    wallet_own_asset_context own_context;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(asset_id)
      BOOST_SERIALIZE(own_context)
      END_BOOST_SERIALIZATION()
  };

  struct asset_unown_event
  {
    crypto::public_key asset_id = currency::null_pkey;
    wallet_own_asset_context own_context;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(asset_id)
      BOOST_SERIALIZE(own_context)
      END_BOOST_SERIALIZATION()
  };

  typedef boost::variant<asset_register_event, asset_update_event, asset_unown_event> wallet_event_t;

  struct transaction_wallet_info
  {
    uint64_t m_block_height = 0;
    uint64_t m_block_timestamp = 0;
    currency::transaction m_tx;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_block_height)
      KV_SERIALIZE(m_block_timestamp)
      KV_SERIALIZE_CUSTOM(m_tx, std::string, currency::transform_tx_to_str, currency::transform_str_to_tx)
    END_KV_SERIALIZE_MAP()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(m_block_height)
      BOOST_SERIALIZE(m_block_timestamp)
      BOOST_SERIALIZE(m_tx)
    END_BOOST_SERIALIZATION()

  };

 
  namespace detail
  {
    //----------------------------------------------------------------------------------------------------
    inline const transaction_wallet_info& transform_ptr_to_value(const std::shared_ptr<transaction_wallet_info>& a)
    {
      return *a;
    }
    //----------------------------------------------------------------------------------------------------
    inline std::shared_ptr<transaction_wallet_info> transform_value_to_ptr(const transaction_wallet_info& d)
    {
      THROW_IF_TRUE_WALLET_INT_ERR_EX_NO_HANDLER(false, "transform_value_to_ptr shoruld never be called");
      return std::shared_ptr<transaction_wallet_info>();
    }
    //----------------------------------------------------------------------------------------------------
  }


  struct transfer_details_base
  {
    struct ZC_out_info // TODO: @#@# consider using wallet_out_info instead
    {
      ZC_out_info() = default;
      ZC_out_info(const crypto::scalar_t& amount_blinding_mask, const crypto::scalar_t& asset_id_blinding_mask, const crypto::public_key& asset_id)
        : amount_blinding_mask(amount_blinding_mask), asset_id_blinding_mask(asset_id_blinding_mask), asset_id(asset_id)
      {}
      crypto::scalar_t amount_blinding_mask = 0;
      crypto::scalar_t asset_id_blinding_mask = 0;
      crypto::public_key asset_id = currency::null_pkey; // not blinded, not multiplied by 1/8 TODO: @#@# consider changing to point_t, also consider using wallet wallet_out_info
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount_blinding_mask)
        KV_SERIALIZE(asset_id_blinding_mask)
        KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)
      END_KV_SERIALIZE_MAP()

      BEGIN_BOOST_SERIALIZATION()
        BOOST_SERIALIZE(amount_blinding_mask)
        BOOST_SERIALIZE(asset_id_blinding_mask)
        BOOST_SERIALIZE(asset_id)
      END_BOOST_SERIALIZATION()
    };

    std::shared_ptr<transaction_wallet_info> m_ptx_wallet_info;
    uint64_t m_internal_output_index = 0;
    uint64_t m_spent_height = 0;
    uint32_t m_flags = 0;
    uint64_t m_amount = 0;
    boost::shared_ptr<ZC_out_info> m_zc_info_ptr;

    uint64_t amount() const { return m_amount; }
    uint64_t amount_for_global_output_index() const { return is_zc() ? 0 : m_amount; } // amount value for global outputs index, it's zero for outputs with hidden amounts

    // @#@ will throw if type is not tx_out_bare, TODO: change according to new model, 
    // need to replace all get_tx_out_bare_from_out_v() to proper code
    //const currency::tx_out_bare& output() const { return currency::get_tx_out_bare_from_out_v(m_ptx_wallet_info->m_tx.vout[m_internal_output_index]); }

    const currency::tx_out_v& output() const { return m_ptx_wallet_info->m_tx.vout[m_internal_output_index]; }
    uint8_t mix_attr() const { uint8_t result = UINT8_MAX; get_mix_attr_from_tx_out_v(output(), result); return result; }
    crypto::hash tx_hash() const { return get_transaction_hash(m_ptx_wallet_info->m_tx); }
    bool is_spent() const { return m_flags & WALLET_TRANSFER_DETAIL_FLAG_SPENT; }
    bool is_spendable() const { return (m_flags & (WALLET_TRANSFER_DETAIL_FLAG_SPENT | WALLET_TRANSFER_DETAIL_FLAG_BLOCKED | WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION | WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION)) == 0; }
    bool is_reserved_for_escrow() const { return ((m_flags & WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION) != 0); }
    bool is_zc() const { return m_zc_info_ptr.get(); }
    const crypto::public_key& get_asset_id() const { if (m_zc_info_ptr.get()) { return m_zc_info_ptr->asset_id; } else { return currency::native_coin_asset_id; } }
    bool is_native_coin() const { return m_zc_info_ptr.get() ? (m_zc_info_ptr->asset_id == currency::native_coin_asset_id) : true; }
    bool is_htlc() const {

      if (m_ptx_wallet_info->m_tx.vout[m_internal_output_index].type() == typeid(currency::tx_out_bare) &&
        boost::get<currency::tx_out_bare>(m_ptx_wallet_info->m_tx.vout[m_internal_output_index]).target.type() == typeid(currency::txout_htlc))
        return true;
      return false;
    }
    static inline uint64_t transfer_details_base_to_amount(const transfer_details_base& tdb)
    {
      return tdb.amount();
    }
    //----------------------------------------------------------------------------------------------------
    static inline std::string transfer_details_base_to_tx_hash(const transfer_details_base& tdb)
    {
      return epee::string_tools::pod_to_hex(currency::get_transaction_hash(tdb.m_ptx_wallet_info->m_tx));
    }



    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_CUSTOM(m_ptx_wallet_info, const transaction_wallet_info&, detail::transform_ptr_to_value, detail::transform_value_to_ptr)
      KV_SERIALIZE(m_internal_output_index)
      KV_SERIALIZE(m_spent_height)
      KV_SERIALIZE(m_flags)
      KV_SERIALIZE(m_amount)
      KV_SERIALIZE_N(m_zc_info_ptr, "zc_out_info")
      KV_SERIALIZE_EPHEMERAL_N(uint64_t, transfer_details_base_to_amount, "amount")
      KV_SERIALIZE_EPHEMERAL_N(std::string, transfer_details_base_to_tx_hash, "tx_id")
    END_KV_SERIALIZE_MAP()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(m_ptx_wallet_info)
      BOOST_SERIALIZE(m_internal_output_index)
      BOOST_SERIALIZE(m_flags)
      BOOST_SERIALIZE(m_spent_height)
      BOOST_SERIALIZE(m_amount)
      BOOST_SERIALIZE(m_zc_info_ptr)
    END_BOOST_SERIALIZATION()
  };




  struct transfer_details_extra_option_htlc_info
  {
    std::string origin;  //this field filled only if htlc had been redeemed
    crypto::hash redeem_tx_id = currency::null_hash;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(origin)
      BOOST_SERIALIZE(redeem_tx_id)
    END_BOOST_SERIALIZATION()
  };


  typedef boost::variant<transfer_details_extra_option_htlc_info, currency::tx_payer> transfer_details_extra_options_v;

  struct transfer_details : public transfer_details_base
  {
    uint64_t m_global_output_index = 0;
    crypto::key_image m_key_image; //TODO: key_image stored twice :(
    std::vector<transfer_details_extra_options_v> varian_options;

    //v2
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_global_output_index)
      KV_SERIALIZE_POD_AS_HEX_STRING(m_key_image)
      KV_CHAIN_BASE(transfer_details_base)
    END_KV_SERIALIZE_MAP()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(m_global_output_index)
      BOOST_SERIALIZE(m_key_image)
      BOOST_SERIALIZE_BASE_CLASS(transfer_details_base)
      BOOST_SERIALIZE(varian_options)
    END_BOOST_SERIALIZATION()
  };

  //used in wallet 
  struct htlc_expiration_trigger
  {
    bool is_wallet_owns_redeem = false; //specify if this HTLC belong to this wallet by pkey_redeem or by pkey_refund
    uint64_t transfer_index = 0;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(is_wallet_owns_redeem)
      BOOST_SERIALIZE(transfer_index)
    END_BOOST_SERIALIZATION()
  };


  struct payment_details_subtransfer
  {
    crypto::public_key asset_id = currency::null_pkey;
    uint64_t amount = 0;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(asset_id)
      BOOST_SERIALIZE(amount)
      END_BOOST_SERIALIZATION()
  };

  struct payment_details
  {
    crypto::hash m_tx_hash = currency::null_hash;
    uint64_t m_amount = 0;                                 // native coins amount
    uint64_t m_block_height = 0;
    uint64_t m_unlock_time = 0;
    std::vector<payment_details_subtransfer> subtransfers; //subtransfers added for confidential asset only, native amount should be stored in m_amount (for space saving)

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(m_tx_hash)
      BOOST_SERIALIZE(m_amount)
      BOOST_SERIALIZE(m_block_height)
      BOOST_SERIALIZE(m_unlock_time)
      BOOST_SERIALIZE(subtransfers)
      END_BOOST_SERIALIZATION()
  };

  struct expiration_entry_info
  {
    std::vector<uint64_t> selected_transfers;
    uint64_t expiration_time = 0;
    crypto::hash related_tx_id = currency::null_hash; // tx id which caused money lock, if any (ex: escrow proposal transport tx)
    std::vector<payment_details_subtransfer> receved;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(selected_transfers)
      BOOST_SERIALIZE(expiration_time)
      BOOST_SERIALIZE(related_tx_id)
      BOOST_SERIALIZE(receved)
      END_BOOST_SERIALIZATION()
  };

  typedef std::unordered_multimap<std::string, payment_details> payment_container;

  typedef std::deque<transfer_details> transfer_container;
  typedef std::unordered_map<crypto::hash, transfer_details_base> multisig_transfer_container;
  typedef std::unordered_map<crypto::hash, tools::wallet_public::escrow_contract_details_basic> escrow_contracts_container;
  typedef std::map<uint64_t, std::set<size_t> > free_amounts_cache_type;
  typedef std::unordered_map<crypto::public_key, free_amounts_cache_type> free_assets_amounts_cache_type;
  typedef std::unordered_map<std::pair<uint64_t, uint64_t>, uint64_t> amount_gindex_to_transfer_id_container; // maps [amount; gindex] -> tid

}// namespace tools

BOOST_CLASS_VERSION(tools::transfer_details, 3)
BOOST_CLASS_VERSION(tools::transfer_details_base, 2)
