// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "include_base_utils.h"
using namespace epee;


#include <set>
#include <unordered_map>
#include <unordered_set>
#include <boost/serialization/version.hpp>
#include <boost/utility.hpp>

#include "string_tools.h"
#include "syncobj.h"
#include "math_helper.h"

#include "common/db_abstract_accessor.h"

#include "currency_format_utils.h"
#include "verification_context.h"
#include "crypto/hash.h"
#include "common/boost_serialization_helper.h"
#include "currency_protocol/currency_protocol_handler_common.h"

namespace currency
{
  class blockchain_storage;
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  class tx_memory_pool: boost::noncopyable
  {
  public:

    struct tx_details
    {
      transaction tx;
      size_t blob_size;
      uint64_t fee;
      crypto::hash max_used_block_id;
      uint64_t max_used_block_height;
      bool kept_by_block;
      //
      uint64_t last_failed_height;
      crypto::hash last_failed_id;
      time_t receive_time;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(tx)
        FIELD(blob_size)
        FIELD(fee)
        FIELD(max_used_block_id)
        FIELD(max_used_block_height)
        FIELD(kept_by_block)
        FIELD(last_failed_height)
        FIELD(last_failed_id)
        FIELD(receive_time)
        END_SERIALIZE()

    };

    struct performnce_data
    {
      //tx zone
      epee::math_helper::average<uint64_t, 5> tx_processing_time;
      epee::math_helper::average<uint64_t, 5> check_inputs_types_supported_time;
      epee::math_helper::average<uint64_t, 5> expiration_validate_time;
      epee::math_helper::average<uint64_t, 5> validate_amount_time;
      epee::math_helper::average<uint64_t, 5> validate_alias_time;
      epee::math_helper::average<uint64_t, 5> check_keyimages_ws_ms_time;
      epee::math_helper::average<uint64_t, 5> check_inputs_time;
      epee::math_helper::average<uint64_t, 5> begin_tx_time;
      epee::math_helper::average<uint64_t, 5> update_db_time;
      epee::math_helper::average<uint64_t, 5> db_commit_time;      
    };


    tx_memory_pool(blockchain_storage& bchs, i_currency_protocol* pprotocol);
    bool add_tx(const transaction &tx, const crypto::hash &id, uint64_t blob_size, tx_verification_context& tvc, bool kept_by_block, bool from_core = false);
    bool add_tx(const transaction &tx, tx_verification_context& tvc, bool kept_by_block, bool from_core = false);

    bool do_insert_transaction(const transaction &tx, const crypto::hash &id, uint64_t blob_size, bool kept_by_block, uint64_t fee, const crypto::hash& max_used_block_id, uint64_t max_used_block_height);
    //gets tx and remove it from pool
    bool take_tx(const crypto::hash &id, transaction &tx, size_t& blob_size, uint64_t& fee);


    bool have_tx(const crypto::hash &id)const;
    bool have_tx_keyimg_as_spent(const crypto::key_image& key_im)const;
    bool have_tx_keyimges_as_spent(const transaction& tx, crypto::key_image* p_spent_ki = nullptr) const;
    const performnce_data& get_performnce_data() const { return m_performance_data; }


    bool check_tx_multisig_ins_and_outs(const transaction& tx, bool check_against_pool_txs)const;

    bool on_blockchain_inc(uint64_t new_block_height, const crypto::hash& top_block_id);
    bool on_blockchain_dec(uint64_t new_block_height, const crypto::hash& top_block_id);
    bool on_finalize_db_transaction();
    bool add_transaction_to_black_list(const transaction& tx);
    bool force_relay_pool() const;
    bool set_protocol(i_currency_protocol* pprotocol);


    void on_idle();

    void lock();
    void unlock();
    void purge_transactions();
    void clear();

    // load/store operations
    bool init(const std::string& config_folder);
    bool deinit();
    bool fill_block_template(block &bl, bool pos, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins, size_t &total_size, uint64_t &fee, uint64_t height);
    bool get_transactions(std::list<transaction>& txs) const;
    bool get_all_transactions_details(std::list<tx_rpc_extended_info>& txs)const;
    bool get_all_transactions_brief_details(std::list<tx_rpc_brief_info>& txs)const;
    bool get_all_transactions_list(std::list<std::string>& txs)const;
    bool get_transactions_details(const std::list<std::string>& ids, std::list<tx_rpc_extended_info>& txs)const;
    bool get_transactions_brief_details(const std::list<std::string>& ids, std::list<tx_rpc_brief_info>& txs)const;

    bool get_transaction_details(const crypto::hash& id, tx_rpc_extended_info& trei)const;
    bool get_transaction(const crypto::hash& h, transaction& tx)const;
    bool get_transaction(const crypto::hash& h, tx_details& txd)const;
    size_t get_transactions_count() const;
    bool have_key_images(const std::unordered_set<crypto::key_image>& kic, const transaction& tx)const;
    bool append_key_images(std::unordered_set<crypto::key_image>& kic, const transaction& tx);
    std::string print_pool(bool short_format)const;
    uint64_t get_core_time() const;
    bool get_aliases_from_tx_pool(std::list<extra_alias_entry>& aliases)const;
    bool get_aliases_from_tx_pool(std::map<std::string, size_t>& aliases)const;
    //crypto::hash get_last_core_hash() {return m_last_core_top_hash;}
    //void set_last_core_hash(const crypto::hash& h) { m_last_core_top_hash = h; }


  private:
    bool on_tx_add(const transaction& tx, bool kept_by_block);
    bool on_tx_remove(const transaction& tx, bool kept_by_block);
    bool insert_key_images(const transaction& tx, bool kept_by_block);
    bool remove_key_images(const transaction& tx, bool kept_by_block);
    bool insert_alias_info(const transaction& tx);
    bool remove_alias_info(const transaction& tx);

    bool is_valid_contract_finalization_tx(const transaction &tx)const;
    void initialize_db_solo_options_values();
    bool remove_stuck_transactions();
    bool is_transaction_ready_to_go(tx_details& txd, const crypto::hash& id)const;
    bool validate_alias_info(const transaction& tx, bool is_in_block)const;
    bool get_key_images_from_tx_pool(std::unordered_set<crypto::key_image>& key_images)const;
    //bool push_alias_info(const transaction& tx);
    //bool pop_alias_info(const transaction& tx);
    bool process_cancel_offer_rules(const transaction& tx);
    bool unprocess_cancel_offer_rules(const transaction& tx);
    bool check_is_taken(const crypto::hash& id) const;
    void set_taken(const crypto::hash& id);
    void reset_all_taken();
    
    typedef tools::db::cached_key_value_accessor<crypto::hash, tx_details, true, false> transactions_container;
    typedef tools::db::cached_key_value_accessor<crypto::hash, bool, false, false> hash_container; 
    typedef tools::db::cached_key_value_accessor<crypto::key_image, uint64_t, false, false> key_images_container;
    typedef tools::db::cached_key_value_accessor<uint64_t, uint64_t, false, true> solo_options_container;
    typedef tools::db::cached_key_value_accessor<std::string, bool, false, false> aliases_container; 
    typedef tools::db::cached_key_value_accessor<account_public_address, bool, false, false> address_to_aliases_container;


    //main accessor
    epee::shared_recursive_mutex m_dummy_rw_lock;
    tools::db::basic_db_accessor m_db;
    //containers

    transactions_container m_db_transactions;
    hash_container m_db_cancel_offer_hash;
    hash_container  m_db_black_tx_list;
    key_images_container m_db_key_images_set;
    aliases_container m_db_alias_names;
    address_to_aliases_container m_db_alias_addresses;
    solo_options_container m_db_solo_options;
    tools::db::solo_db_value<uint64_t, uint64_t, solo_options_container> m_db_storage_major_compatibility_version;
    //crypto::hash m_last_core_top_hash;


    epee::math_helper::once_a_time_seconds<30> m_remove_stuck_tx_interval;

    performnce_data m_performance_data;
    std::string m_config_folder;
    blockchain_storage& m_blockchain;
    i_currency_protocol* m_pprotocol;

    //in memory containers
    mutable epee::critical_section m_taken_txs_lock;
    std::unordered_set<crypto::hash> m_taken_txs;
    mutable epee::critical_section m_remove_stuck_txs_lock;

    
    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    class amount_visitor: public boost::static_visitor<uint64_t>
    {
    public:
      uint64_t operator()(const txin_to_key& tx) const
      {
        return tx.amount;
      }
      uint64_t operator()(const txin_gen& /*tx*/) const
      {
        CHECK_AND_ASSERT_MES(false, 0, "coinbase transaction in memory pool");
        return 0;
      }
      uint64_t operator()(const txin_multisig& in) const { return in.amount; }
    };

  };
}

namespace boost
{
  namespace serialization
  {
    template<class archive_t>
    void serialize(archive_t & ar, currency::tx_memory_pool::tx_details& td, const unsigned int version)
    {
      ar & td.blob_size;
      ar & td.fee;
      ar & td.tx;
      ar & td.max_used_block_height;
      ar & td.max_used_block_id;
      ar & td.last_failed_height;
      ar & td.last_failed_id;
      ar & td.receive_time;
      ar & td.kept_by_block;
    }
  }
}

BOOST_CLASS_VERSION(currency::tx_memory_pool, CURRENT_MEMPOOL_ARCHIVE_VER)



