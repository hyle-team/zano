// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <boost/filesystem.hpp>
#include <unordered_set>
#include <vector>

#include "common/db_backend_lmdb.h"
#include "tx_pool.h"
#include "currency_boost_serialization.h"
#include "currency_core/currency_config.h"
#include "blockchain_storage.h"
#include "common/boost_serialization_helper.h"
#include "common/int-util.h"
#include "misc_language.h"
#include "warnings.h"
#include "crypto/hash.h"
#include "offers_service_basics.h"
#include "profile_tools.h"

DISABLE_VS_WARNINGS(4244 4345 4503) //'boost::foreach_detail_::or_' : decorated name length exceeded, name was truncated

#define TRANSACTION_POOL_CONTAINER_TRANSACTIONS           "transactions"
#define TRANSACTION_POOL_CONTAINER_CANCEL_OFFER_HASH      "cancel_offer_hash"
#define TRANSACTION_POOL_CONTAINER_BLACK_TX_LIST          "black_tx_list"
#define TRANSACTION_POOL_CONTAINER_ALIAS_NAMES            "alias_names"
#define TRANSACTION_POOL_CONTAINER_ALIAS_ADDRESSES        "alias_addresses"
#define TRANSACTION_POOL_CONTAINER_KEY_IMAGES             "key_images"
#define TRANSACTION_POOL_CONTAINER_SOLO_OPTIONS           "solo"
#define TRANSACTION_POOL_MAJOR_COMPATIBILITY_VERSION      BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION + 1

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "tx_pool"
ENABLE_CHANNEL_BY_DEFAULT("tx_pool");

namespace currency
{
  //---------------------------------------------------------------------------------
  tx_memory_pool::tx_memory_pool(blockchain_storage& bchs, i_currency_protocol* pprotocol) :
    m_blockchain(bchs),
    m_pprotocol(pprotocol),
    m_db(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend), m_dummy_rw_lock),
    m_db_transactions(m_db),
    m_db_cancel_offer_hash(m_db),
    m_db_black_tx_list(m_db),
    m_db_solo_options(m_db), 
    m_db_key_images_set(m_db),
    m_db_alias_names(m_db),
    m_db_alias_addresses(m_db),
    m_db_storage_major_compatibility_version(TRANSACTION_POOL_MAJOR_COMPATIBILITY_VERSION, m_db_solo_options)
  {

  }

  bool tx_memory_pool::is_valid_contract_finalization_tx(const transaction &tx)const
  {
    if (tx.vin.size() != 1 || tx.vin[0].type() != typeid(txin_multisig))
    {
      return false;
    }

    const txin_multisig& ms_in = boost::get<const txin_multisig>(tx.vin[0]);
    crypto::hash related_tx_id = null_hash; uint64_t out_no = 0;    
    if (!m_blockchain.get_multisig_id_details(ms_in.multisig_out_id, related_tx_id, out_no))
    {
      LOG_ERROR("Related multisig tx not found, multisig_out_id=" << ms_in.multisig_out_id);
      return false;
    }
    
    auto related_tx_ptr = m_blockchain.get_tx_chain_entry(related_tx_id);
    if (!related_tx_ptr)
    {
      LOG_ERROR("Tx "  << related_tx_id << " related to multisig id " << ms_in.multisig_out_id
        << "(discovered by reviewing tx " << get_transaction_hash(tx) <<") tx not found in blockchain, multisig_out_id=" << ms_in.multisig_out_id);
      return false;
    }
    
    if (get_tx_fee(tx) < get_tx_fee(related_tx_ptr->tx))
    {
      LOG_ERROR("Tx " << get_transaction_hash(tx) << " fee=" << get_tx_fee(tx) << " less then parent multisig tx " << related_tx_id << " fee= " << get_tx_fee(related_tx_ptr->tx));
      return false;
    }

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::add_tx(const transaction &tx, const crypto::hash &id, uint64_t blob_size, tx_verification_context& tvc, bool kept_by_block, bool from_core)
  {    
    TIME_MEASURE_START_PD(tx_processing_time);
    TIME_MEASURE_START_PD(check_inputs_types_supported_time);
    if(!check_inputs_types_supported(tx))
    {
      tvc.m_verification_failed = true;
      return false;
    }
    TIME_MEASURE_FINISH_PD(check_inputs_types_supported_time);

    TIME_MEASURE_START_PD(expiration_validate_time);
    if (!from_core && !kept_by_block && m_blockchain.is_tx_expired(tx))
    {
      uint64_t tx_expiration_time = get_tx_expiration_time(tx);
      uint64_t ts_median = m_blockchain.get_tx_expiration_median();
      LOG_PRINT_L0("transaction " << id << " is expired, rejected by tx pool (tx timestamp: " << tx_expiration_time << " - " << TX_EXPIRATION_MEDIAN_SHIFT << " (median shift) = " <<
        tx_expiration_time - TX_EXPIRATION_MEDIAN_SHIFT << ", blockchain timestamp median: " << ts_median <<
        ", diff: " << epee::misc_utils::get_time_interval_string(ts_median + TX_EXPIRATION_MEDIAN_SHIFT - tx_expiration_time) << ")");
      tvc.m_verification_failed = true;
      return false;
    }      
    TIME_MEASURE_FINISH_PD(expiration_validate_time);

    TIME_MEASURE_START_PD(validate_amount_time);
    uint64_t inputs_amount = 0;
    if(!get_inputs_money_amount(tx, inputs_amount))
    {
      tvc.m_verification_failed = true;
      return false;
    } 

    CHECK_AND_ASSERT_MES_CUSTOM(tx.vout.size() <= CURRENCY_TX_MAX_ALLOWED_OUTS, false, tvc.m_verification_failed = true, "transaction has too many outs = " << tx.vout.size());

    uint64_t outputs_amount = get_outs_money_amount(tx);

    if(outputs_amount > inputs_amount)
    {
      LOG_PRINT_L0("transaction use more money then it has: use " << outputs_amount << ", have " << inputs_amount);
      tvc.m_verification_failed = true;
      return false;
    }
    TIME_MEASURE_FINISH_PD(validate_amount_time);

    TIME_MEASURE_START_PD(validate_alias_time);
    if (!from_core && !validate_alias_info(tx, kept_by_block))
    {
      LOG_PRINT_RED_L0("validate_alias_info failed");
      tvc.m_verification_failed = true;
      return false;
    }
    TIME_MEASURE_FINISH_PD(validate_alias_time);

    TIME_MEASURE_START_PD(check_keyimages_ws_ms_time);
    //check key images for transaction if it is not kept by block
    if(!from_core && !kept_by_block)
    {
      if(have_tx_keyimges_as_spent(tx))
      {
        LOG_ERROR("Transaction with id= "<< id << " used already spent key images");
        tvc.m_verification_failed = true;
        return false;
      }

      //transaction spam protection, soft rule
      uint64_t tx_fee = inputs_amount - outputs_amount;
      if (tx_fee < m_blockchain.get_core_runtime_config().tx_pool_min_fee)
      {
        //exception for cancel offer transactions
        if (process_cancel_offer_rules(tx))
        {
          // this tx has valid offer cansellation instructions and thus can go for free
          // check soft size constrain
          if (blob_size > CURRENCY_FREE_TX_MAX_BLOB_SIZE)
          {
            LOG_ERROR("Blob size (" << blob_size << ") << exceeds limit for transaction " << id << " that contains offer cancellation and has smaller fee (" << tx_fee << ") than expected");
            tvc.m_verification_failed = true;
            return false;
          }
        }
        else if (is_valid_contract_finalization_tx(tx))
        {
          // that means tx has less fee then allowed by current tx pull rules, but this transaction is actually 
          // a finalization of contract, and template of this contract finalization tx was prepared actually before 
          // fee rules had been changed, so it's ok, let it in.
        }
        else
        {
          // this tx has no fee OR invalid offer cancellations instructions -- so the exceptions of zero fee is not applicable
          LOG_ERROR("Transaction with id= " << id << " has too small fee: " << tx_fee << ", expected fee: " << m_blockchain.get_core_runtime_config().tx_pool_min_fee);
          tvc.m_verification_failed = false;
          tvc.m_should_be_relayed = false;
          tvc.m_added_to_pool = false;
          return true;
        }
      }

      // check tx multisig inputs/output against tx in the pool and in the blockchain
      if (!check_tx_multisig_ins_and_outs(tx, true))
      {
        tvc.m_verification_failed = true;
        return false;
      }
    }
    TIME_MEASURE_FINISH_PD(check_keyimages_ws_ms_time);

    TIME_MEASURE_START_PD(check_inputs_time);
    crypto::hash max_used_block_id = null_hash;
    uint64_t max_used_block_height = 0;
    bool ch_inp_res = m_blockchain.check_tx_inputs(tx, id, max_used_block_height, max_used_block_id);
    if (!ch_inp_res && !kept_by_block && !from_core)
    {
      LOG_PRINT_L0("tx used wrong inputs, rejected");
      tvc.m_verification_failed = true;
      return false;
    }
    TIME_MEASURE_FINISH_PD(check_inputs_time);

    do_insert_transaction(tx, id, blob_size, kept_by_block, inputs_amount - outputs_amount, ch_inp_res ? max_used_block_id : null_hash, ch_inp_res ? max_used_block_height : 0);
    
    TIME_MEASURE_FINISH_PD(tx_processing_time);
    tvc.m_added_to_pool = true;
    tvc.m_should_be_relayed = ch_inp_res; // relay tx only if it has valid inputs (i.e. do not relay kept_by_block tx with wrong inputs)
    tvc.m_verification_impossible = !ch_inp_res; // mark 'kept_by_block' tx with wrong inputs as impossible to be verified
    tvc.m_verification_failed = false;
    //succeed

    LOG_PRINT_L2("[TX_POOL ADD_TX] timing(micsec) : " << print_fixed_decimal_point(tx_processing_time, 3)
      << "("<< m_performance_data.check_inputs_types_supported_time.get_last_val()
      << "/" << m_performance_data.expiration_validate_time.get_last_val()
      << "/" << m_performance_data.validate_amount_time.get_last_val()
      << "/" << m_performance_data.validate_alias_time.get_last_val()
      << "/" << m_performance_data.check_keyimages_ws_ms_time.get_last_val()
      << "/" << m_performance_data.check_inputs_time.get_last_val()
      << "/" << m_performance_data.begin_tx_time.get_last_val()
      << "/" << m_performance_data.update_db_time.get_last_val()
      << "/" << m_performance_data.db_commit_time.get_last_val() << ")"    );

    return true;
  }
  //---------------------------------------------------------------------------------
#define LOCAL_READONLY_TRANSACTION() \
  m_db.begin_readonly_transaction(); \
  misc_utils::auto_scope_leave_caller db_tx_closer = misc_utils::create_scope_leave_handler([&]() \
  { \
    m_db.commit_transaction(); \
  });


  bool tx_memory_pool::do_insert_transaction(const transaction &tx, const crypto::hash &id, uint64_t blob_size, bool kept_by_block, uint64_t fee, const crypto::hash& max_used_block_id, uint64_t max_used_block_height)
  {
    TIME_MEASURE_START_PD(begin_tx_time);
    m_db.begin_transaction();
    TIME_MEASURE_FINISH_PD(begin_tx_time);

    TIME_MEASURE_START_PD(update_db_time);
    misc_utils::auto_scope_leave_caller seh = misc_utils::create_scope_leave_handler([&]()
    {
      TIME_MEASURE_START_PD(db_commit_time);
      m_db.commit_transaction();
      TIME_MEASURE_FINISH_PD(db_commit_time);

    });
    //CHECK_AND_ASSERT_MES_CUSTOM(!m_db_transactions.get(id), false, (tvc.m_added_to_pool = false, tvc.m_verification_failed = true), "internal error: failed to add transaction " << id << "     to the pool as it already exists");
    tx_details td = AUTO_VAL_INIT(td);
    td.blob_size = blob_size;
    td.tx = tx;
    td.kept_by_block = kept_by_block;
    td.fee = fee;
    td.max_used_block_id = max_used_block_id;
    td.max_used_block_height = max_used_block_height;
    td.last_failed_height = 0;
    td.last_failed_id = null_hash;
    td.receive_time = get_core_time();

    m_db_transactions.set(id, td);
    on_tx_add(tx, kept_by_block);

    TIME_MEASURE_FINISH_PD(update_db_time);
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::check_is_taken(const crypto::hash& id) const
  {
    CRITICAL_REGION_LOCAL(m_taken_txs_lock);
    return m_taken_txs.count(id) ? true : false;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::set_taken(const crypto::hash& id)
  {
    CRITICAL_REGION_LOCAL(m_taken_txs_lock);
    m_taken_txs.insert(id);
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::reset_all_taken()
  {
    CRITICAL_REGION_LOCAL(m_taken_txs_lock);
    m_taken_txs.clear();
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::process_cancel_offer_rules(const transaction& tx)
  {
    //TODO: this code doesn't take into account offer id in source tx
    //TODO: add scan on tx size for free transaction here
    m_db_transactions.begin_transaction();
    misc_utils::auto_scope_leave_caller seh = misc_utils::create_scope_leave_handler([&](){m_db_transactions.commit_transaction(); });


    size_t serv_att_count = 0;
    std::list<bc_services::cancel_offer> co_list;
    for (const auto& a: tx.attachment)
    {
      if (a.type() == typeid(tx_service_attachment))
      {
        const tx_service_attachment& srv_at = boost::get<tx_service_attachment>(a);
        if (srv_at.service_id == BC_OFFERS_SERVICE_ID && srv_at.instruction == BC_OFFERS_SERVICE_INSTRUCTION_DEL)
        {
          if (!m_blockchain.validate_tx_service_attachmens_in_services(srv_at, serv_att_count, tx))
          {
            LOG_ERROR("validate_tx_service_attachmens_in_services failed for an offer cancellation transaction");
            return false;
          }
          bc_services::extract_type_and_add<bc_services::cancel_offer>(srv_at.body, co_list);
          if (m_db_cancel_offer_hash.get(co_list.back().tx_id))
          {
            LOG_ERROR("cancellation of offer " << co_list.back().tx_id << " has already been processed earlier; zero fee is disallowed");
            return false;
          }
        }
        serv_att_count++;
      }
    }
    if (!co_list.size())
    {
      LOG_PRINT_L1("No cancel offers found");
      return false;
    }

    for (auto co : co_list)
    {
      m_db_cancel_offer_hash.set(co.tx_id, true);
    }

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::unprocess_cancel_offer_rules(const transaction& tx)
  {
    m_db_transactions.begin_transaction();
    misc_utils::auto_scope_leave_caller seh = misc_utils::create_scope_leave_handler([&](){m_db_transactions.commit_transaction(); });

    std::list<bc_services::cancel_offer> co_list;
    for (const auto& a : tx.attachment)
    {
      if (a.type() == typeid(tx_service_attachment))
      {
        const tx_service_attachment& srv_at = boost::get<tx_service_attachment>(a);
        if (srv_at.service_id == BC_OFFERS_SERVICE_ID && srv_at.instruction == BC_OFFERS_SERVICE_INSTRUCTION_DEL)
        {
          co_list.clear();
          bc_services::extract_type_and_add<bc_services::cancel_offer>(srv_at.body, co_list);
          if (!co_list.size())
            return false;
          auto vptr = m_db_cancel_offer_hash.find(co_list.back().tx_id);
          if (vptr == m_db_cancel_offer_hash.end())
            return false;
          m_db_cancel_offer_hash.erase(co_list.back().tx_id);
        }
      }
    }
    return true;
  }
//   //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_aliases_from_tx_pool(std::list<extra_alias_entry>& aliases)const
  {

    //TODO: OPTIMIZATION put cache here!!!!!

    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
    {
      tx_extra_info ei = AUTO_VAL_INIT(ei);
      bool r = parse_and_validate_tx_extra(tx_entry.tx, ei);
      CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra on unprocess_blockchain_tx_extra");
      if (ei.m_alias.m_alias.size())
        aliases.push_back(ei.m_alias);

      return true;
    });
    return true;

  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_aliases_from_tx_pool(std::map<std::string, size_t>& aliases)const
  {
    std::list<extra_alias_entry> aliases_local;
    get_aliases_from_tx_pool(aliases_local);

    for (auto& alias_info : aliases_local)
    {
      ++aliases[alias_info.m_alias];
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::validate_alias_info(const transaction& tx, bool is_in_block) const
  {
    LOCAL_READONLY_TRANSACTION();
    extra_alias_entry eai = AUTO_VAL_INIT(eai);
    if (get_type_in_variant_container(tx.extra, eai))
    {
      //check in blockchain
      extra_alias_entry eai2 = AUTO_VAL_INIT(eai2);
      bool already_have_alias_registered = m_blockchain.get_alias_info(eai.m_alias, eai2);
      //size_t alias_size = eai.m_alias.size();
      
      if (!is_in_block  && !eai.m_sign.size() && already_have_alias_registered)
      {
        LOG_PRINT_L0("Alias \"" << eai.m_alias  << "\" already registered in blockchain, transaction rejected");
        return false;
      }

      std::string prev_alias = m_blockchain.get_alias_by_address(eai.m_address);
      if (!is_in_block && !eai.m_sign.size() &&
        prev_alias.size())
      {
        LOG_PRINT_L0("Address \"" << get_account_address_as_str(eai.m_address)  
          << "\" already registered with \""<< prev_alias 
          << "\" aliass in blockchain (new alias: \"" << eai.m_alias  << "\"), transaction rejected");
        return false;
      }

      if (!is_in_block)
      {
        if (m_db_alias_names.get(eai.m_alias))
        {
          LOG_PRINT_L0("Alias \"" << eai.m_alias << "\" already in transaction pool, transaction rejected");
          return false;
        }
        if (m_db_alias_addresses.get(eai.m_address))
        {
          LOG_PRINT_L0("Alias \"" << eai.m_alias << "\" already in transaction pool by it's address(" << get_account_address_as_str(eai.m_address) << ") , transaction rejected");
          return false;
        }
        //validate alias reward
        if (!m_blockchain.prevalidate_alias_info(tx, eai))
        {
          LOG_PRINT_L0("Alias \"" << eai.m_alias << "\" reward validation failed, transaction rejected");
          return false;
        }
      }


    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::add_tx(const transaction &tx, tx_verification_context& tvc, bool kept_by_block, bool from_core)
  {
    crypto::hash h = null_hash;
    uint64_t blob_size = 0;

    get_transaction_hash(tx, h, blob_size);
    return add_tx(tx, h, blob_size, tvc, kept_by_block, from_core);
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::take_tx(const crypto::hash &id, transaction &tx, size_t& blob_size, uint64_t& fee)
  {
    
    m_db_transactions.begin_transaction();
    misc_utils::auto_scope_leave_caller seh = misc_utils::create_scope_leave_handler([&](){m_db_transactions.commit_transaction();});
    
    auto txe_tr = m_db_transactions.find(id);
    if (txe_tr == m_db_transactions.end())
      return false;

    tx = txe_tr->tx;
    blob_size = txe_tr->blob_size;
    fee = txe_tr->fee;
    unprocess_cancel_offer_rules(txe_tr->tx);
    m_db_transactions.erase(id);
    on_tx_remove(tx, txe_tr->kept_by_block);
    set_taken(id);
    return true;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::on_idle()
  {
    m_remove_stuck_tx_interval.do_call([this](){return remove_stuck_transactions();});
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::remove_stuck_transactions()
  {    
    if (!CRITICAL_SECTION_TRY_LOCK(m_remove_stuck_txs_lock))
      return true;

    CRITICAL_REGION_LOCAL(m_remove_stuck_txs_lock);
    CRITICAL_SECTION_UNLOCK(m_remove_stuck_txs_lock);// release try_lock iteration
    
    m_db_transactions.begin_transaction();
    misc_utils::auto_scope_leave_caller seh = misc_utils::create_scope_leave_handler([&](){m_db_transactions.commit_transaction(); });

    struct tx_to_delete_entry
    {
      tx_to_delete_entry(const crypto::hash &hash, const transaction &tx, bool kept_by_block)
        : hash(hash), tx(tx), kept_by_block(kept_by_block)
      {}
      crypto::hash hash;
      transaction tx;
      bool kept_by_block;
    };
    
    std::vector<tx_to_delete_entry> to_delete;

    const uint64_t tx_expiration_ts_median = m_blockchain.get_tx_expiration_median();
    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
    {
      //never remove transactions which related to alt blocks, 
      //or we can get network split as a worst case (impossible to switch to 
      //altchain if it won, and node stuck forever).
      if (m_blockchain.is_tx_related_to_altblock(h))
        return true;

      // maximum age check - remove too old
      uint64_t tx_age = get_core_time() - tx_entry.receive_time;
      if ((tx_age > CURRENCY_MEMPOOL_TX_LIVETIME ))
      {

        LOG_PRINT_L0("Tx " << h << " removed from tx pool, reason: outdated, age: " << tx_age);
        to_delete.push_back(tx_to_delete_entry(h, tx_entry.tx, tx_entry.kept_by_block));
      }

      // expiration time check - remove expired
      if (is_tx_expired(tx_entry.tx, tx_expiration_ts_median) )
      {
        LOG_PRINT_L0("Tx " << h << " removed from tx pool, reason: expired, expiration time: " << get_tx_expiration_time(tx_entry.tx) << ", blockchain median: " << tx_expiration_ts_median);
        to_delete.push_back(tx_to_delete_entry(h, tx_entry.tx, tx_entry.kept_by_block));
      }

      return true;
    });
    
    for (auto& e : to_delete)
    {
      m_db_transactions.erase(e.hash);
      on_tx_remove(e.tx, e.kept_by_block);
    }


    return true;
  }
  //---------------------------------------------------------------------------------
  size_t tx_memory_pool::get_transactions_count() const 
  {
    return m_db_transactions.size();
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transactions(std::list<transaction>& txs) const
  {
    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
    {
      txs.push_back(tx_entry.tx);
      return true;
    });

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_all_transactions_details(std::list<tx_rpc_extended_info>& txs) const
  {
    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
    {
      txs.push_back(tx_rpc_extended_info());
      tx_rpc_extended_info& trei = txs.back();
      trei.blob_size = tx_entry.blob_size;
      fill_tx_rpc_details(trei, tx_entry.tx, nullptr, h, tx_entry.receive_time, true);
      return true;
    });

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_all_transactions_brief_details(std::list<tx_rpc_brief_info>& txs) const
  {
    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
    {
      txs.push_back(tx_rpc_brief_info());
      tx_rpc_brief_info& trbi = txs.back();
      trbi.id = epee::string_tools::pod_to_hex(h);
      trbi.fee = tx_entry.fee;
      trbi.sz = tx_entry.blob_size;
      trbi.total_amount = get_outs_money_amount(tx_entry.tx);
      return true;
    });

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_all_transactions_list(std::list<std::string>& txs)const
  {
    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
    {
      txs.push_back(epee::string_tools::pod_to_hex(h));
      return true;
    });

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transactions_details(const std::list<std::string>& ids, std::list<tx_rpc_extended_info>& txs) const
  {
    for (auto& id_str:  ids)
    {
      crypto::hash id = null_hash;
      if (!epee::string_tools::hex_to_pod(id_str, id))
      {
        LOG_ERROR("Failed to parse id in list: " << id_str);
        return false;
      }

      auto ptei = m_db_transactions.get(id);
      if (!ptei)
        return false;

      txs.push_back(tx_rpc_extended_info());
      tx_rpc_extended_info& trei = txs.back();
      trei.blob_size = ptei->blob_size;
      fill_tx_rpc_details(trei, ptei->tx, nullptr, id, ptei->receive_time, false);
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transactions_brief_details(const std::list<std::string>& ids, std::list<tx_rpc_brief_info>& txs)const
  {
    LOCAL_READONLY_TRANSACTION();
    for (auto& id_str : ids)
    {
      crypto::hash id = null_hash;
      if (!epee::string_tools::hex_to_pod(id_str, id))
      {
        LOG_ERROR("Failed to parse id in list: " << id_str);
        return false;
      }

      auto ptei = m_db_transactions.get(id);
      if (!ptei)
        return false;

      txs.push_back(tx_rpc_brief_info());
      tx_rpc_brief_info& trbi = txs.back();
      trbi.sz = ptei->blob_size;
      trbi.id = id_str;
      trbi.fee = ptei->fee;
      trbi.total_amount = get_outs_money_amount(ptei->tx);
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transaction_details(const crypto::hash& id, tx_rpc_extended_info& trei) const
  {
    auto ptei = m_db_transactions.get(id);
    if (!ptei)
      return false;

    fill_tx_rpc_details(trei, ptei->tx, nullptr, id, ptei->receive_time, false);
    return true;
  }  
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transaction(const crypto::hash& id, transaction& tx) const
  {
    auto tx_ptr = m_db_transactions.find(id);
    if (tx_ptr == m_db_transactions.end())
      return false;
    tx = tx_ptr->tx;
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transaction(const crypto::hash& id, tx_details& txd) const
  {
    auto tx_ptr = m_db_transactions.find(id);
    if (tx_ptr == m_db_transactions.end())
      return false;
    txd = *tx_ptr;
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::on_blockchain_inc(uint64_t new_block_height, const crypto::hash& top_block_id)
  {
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::on_blockchain_dec(uint64_t new_block_height, const crypto::hash& top_block_id)
  {
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::on_finalize_db_transaction()
  {
    reset_all_taken();
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::set_protocol(i_currency_protocol* pprotocol)
  {
    m_pprotocol = pprotocol;
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::force_relay_pool() const
  {
    LOG_PRINT_GREEN("Preparing relay message...", LOG_LEVEL_0);
    NOTIFY_NEW_TRANSACTIONS::request r = AUTO_VAL_INIT(r);

    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& k, const tx_details& v)
    {
      r.txs.push_back(t_serializable_object_to_blob(v.tx));
      return true;
    });
    LOG_PRINT_GREEN("Sending....", LOG_LEVEL_0);
    CHECK_AND_ASSERT_MES(m_pprotocol, false, "m_pprotocol is not set");
    currency_connection_context fake_context = AUTO_VAL_INIT(fake_context);
    m_pprotocol->relay_transactions(r, fake_context);
    LOG_PRINT_GREEN("Sent.", LOG_LEVEL_0);
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::add_transaction_to_black_list(const transaction& tx)
  {
    // atm:
    // 1) the only side effect of a tx being blacklisted is the one is just ignored by fill_block_template(), but it still can be added to blockchain/pool
    // 2) it's permanent
    LOG_PRINT_YELLOW("TX ADDED TO POOL'S BLACKLIST: " << get_transaction_hash(tx), LOG_LEVEL_0);
    m_db.begin_transaction();
    m_db_black_tx_list.set(get_transaction_hash(tx), true);
    m_db.commit_transaction();
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx(const crypto::hash &id) const
  {    
    if(m_db_transactions.get(id))
      return true;

    if (check_is_taken(id))
      return true;
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx_keyimges_as_spent(const transaction& tx) const
  {
    for(const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, true);//should never fail
        if(have_tx_keyimg_as_spent(tokey_in.k_image))
           return true;

      }
    }
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::insert_key_images(const transaction& tx, bool kept_by_block)
  {
    for(const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, true);//should never fail
        uint64_t count = 0;
        auto ki_entry_ptr = m_db_key_images_set.get(tokey_in.k_image);
        if (ki_entry_ptr.get())
          count = *ki_entry_ptr;
        ++count;
        m_db_key_images_set.set(tokey_in.k_image, count);
      }
    }
    return false;
  }
  //--------------------------------------------------------------------------------- 
  bool tx_memory_pool::on_tx_add(const transaction& tx, bool kept_by_block)
  {
    if (!kept_by_block)
      insert_key_images(tx, kept_by_block); // take into account only key images from txs that are not 'kept_by_block'
    insert_alias_info(tx);
    return true;
  }
  //--------------------------------------------------------------------------------- 
  bool tx_memory_pool::on_tx_remove(const transaction& tx, bool kept_by_block)
  {
    if (!kept_by_block)
      remove_key_images(tx, kept_by_block); // take into account only key images from txs that are not 'kept_by_block'
    remove_alias_info(tx);
    return true;
  }
  //--------------------------------------------------------------------------------- 
  bool tx_memory_pool::insert_alias_info(const transaction& tx)
  {
    tx_extra_info ei = AUTO_VAL_INIT(ei);
    bool r = parse_and_validate_tx_extra(tx, ei);
    CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra on unprocess_blockchain_tx_extra");
    if (ei.m_alias.m_alias.size())
    {
      m_db_alias_names.set(ei.m_alias.m_alias, true);
      m_db_alias_addresses.set(ei.m_alias.m_address, true);
    }

    return true;
  }
  //--------------------------------------------------------------------------------- 
  bool tx_memory_pool::remove_alias_info(const transaction& tx)
  {
    
    tx_extra_info ei = AUTO_VAL_INIT(ei);
    bool r = parse_and_validate_tx_extra(tx, ei);
    CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra on unprocess_blockchain_tx_extra");
    if (ei.m_alias.m_alias.size())
    {
      m_db_alias_names.erase(ei.m_alias.m_alias);
      m_db_alias_addresses.erase(ei.m_alias.m_address);
    }

    return true;
  }
  //--------------------------------------------------------------------------------- 
  bool tx_memory_pool::remove_key_images(const transaction& tx, bool kept_by_block)
  {
    for(const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, true);//should never fail
        uint64_t count = 0;
        auto ki_entry_ptr = m_db_key_images_set.get(tokey_in.k_image);
        if (!ki_entry_ptr.get() || *ki_entry_ptr == 0)
        {
          LOG_ERROR("INTERNAL_ERROR: for tx " << get_transaction_hash(tx) << " key image " << tokey_in.k_image << " not found");
          continue;
        }
        count = *ki_entry_ptr;
        --count;
        if (count)
          m_db_key_images_set.set(tokey_in.k_image, count);
        else
          m_db_key_images_set.erase(tokey_in.k_image);
      }
    }
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_key_images_from_tx_pool(std::unordered_set<crypto::key_image>& key_images) const
  {
    
    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
    {
      for (auto& in : tx_entry.tx.vin)
      {
        if (in.type() == typeid(txin_to_key))
        {
          key_images.insert(boost::get<txin_to_key>(in).k_image);
        }
      }
      return true;
    });

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx_keyimg_as_spent(const crypto::key_image& key_im)const
  {
    
    auto ptr = m_db_key_images_set.find(key_im);
    if (ptr)
      return true;
    return false;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::lock()
  {
    CRITICAL_SECTION_LOCK(m_remove_stuck_txs_lock);
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::unlock()
  {
    CRITICAL_SECTION_UNLOCK(m_remove_stuck_txs_lock);
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::purge_transactions()
  {
    
    m_db.begin_transaction();
    m_db_transactions.clear();
    m_db_key_images_set.clear();
    m_db_cancel_offer_hash.clear();
    m_db.commit_transaction();
    // should m_db_black_tx_list be cleared here?
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::clear()
  {
    m_db.begin_transaction();
    m_db_transactions.clear();
    m_db_cancel_offer_hash.clear();
    m_db_black_tx_list.clear();
    m_db_key_images_set.clear();
    m_db.commit_transaction();
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::is_transaction_ready_to_go(tx_details& txd, const crypto::hash& id)const 
  {
    //not the best implementation at this time, sorry :(

    if (m_db_black_tx_list.get(get_transaction_hash(txd.tx)))
      return false;

    //check is ring_signature already checked ?
    if(txd.max_used_block_id == null_hash)
    {//not checked, lets try to check

      if(txd.last_failed_id != null_hash && m_blockchain.get_current_blockchain_size() > txd.last_failed_height && txd.last_failed_id == m_blockchain.get_block_id_by_height(txd.last_failed_height))
        return false;//we already sure that this tx is broken for this height

      if(!m_blockchain.check_tx_inputs(txd.tx, id, txd.max_used_block_height, txd.max_used_block_id))
      {
        txd.last_failed_height = m_blockchain.get_top_block_height();
        txd.last_failed_id = m_blockchain.get_block_id_by_height(txd.last_failed_height);
        return false;
      }
    }else
    {
      if(txd.max_used_block_height >= m_blockchain.get_current_blockchain_size())
        return false;
      if(m_blockchain.get_block_id_by_height(txd.max_used_block_height) != txd.max_used_block_id)
      {
        //if we already failed on this height and id, skip actual ring signature check
        if(txd.last_failed_id == m_blockchain.get_block_id_by_height(txd.last_failed_height))
          return false;
        //check ring signature again, it is possible (with very small chance) that this transaction become again valid
        if(!m_blockchain.check_tx_inputs(txd.tx, id, txd.max_used_block_height, txd.max_used_block_id))
        {
          txd.last_failed_height = m_blockchain.get_top_block_height();
          txd.last_failed_id = m_blockchain.get_block_id_by_height(txd.last_failed_height);
          return false;
        }
      }
    }
    //if we here, transaction seems valid, but, anyway, check for key_images collisions with blockchain, just to be sure
    if(m_blockchain.have_tx_keyimges_as_spent(txd.tx))
      return false;

    if (!check_tx_multisig_ins_and_outs(txd.tx, false))
      return false;

    //transaction is ok.
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_key_images(const std::unordered_set<crypto::key_image>& k_images, const transaction& tx)const 
  {
    LOCAL_READONLY_TRANSACTION();
    for(size_t i = 0; i!= tx.vin.size(); i++)
    {
      if (tx.vin[i].type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(tx.vin[i], const txin_to_key, itk, false);
        if (k_images.count(itk.k_image))
          return true;
      }
    }
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::append_key_images(std::unordered_set<crypto::key_image>& k_images, const transaction& tx)
  {
    for(size_t i = 0; i!= tx.vin.size(); i++)
    {
      if (tx.vin[i].type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(tx.vin[i], const txin_to_key, itk, false);
        auto i_res = k_images.insert(itk.k_image);
        CHECK_AND_ASSERT_MES(i_res.second, false, "internal error: key images pool cache - inserted duplicate image in set: " << itk.k_image);
      }
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  std::string tx_memory_pool::print_pool(bool short_format)const 
  {
    std::stringstream ss;
    if (short_format)
    {
      std::list<std::pair<crypto::hash, tx_details>> txs;
      {
        
        m_db_transactions.enumerate_items([&txs](uint64_t i, const crypto::hash& h, const tx_details &tx_entry) { txs.push_back(std::make_pair(h, tx_entry)); return true; });
      }
      if (txs.empty())
        return "(no transactions, the pool is empty)";
      // sort output by receive time
      txs.sort([](const std::pair<crypto::hash, tx_details>& lhs, const std::pair<crypto::hash, tx_details>& rhs) -> bool { return lhs.second.receive_time < rhs.second.receive_time; });
      ss << "#    | transaction id                                                   | size  | fee       | ins | outs | outs money      | live_time      | max used block   | last failed block | kept by a block?" << ENDL;
      //     1234  <f99fe6d4335fc0ddd69e6880a4d95e0f6ea398de0324a6837021a61c6a31cacd>  87157   0.10000111  2000  2000   112000.12345678   d0.h10.m16.s17   123456 <12345..>   123456 <12345..>    YES   
      size_t i = 0;
      for (auto& tx : txs)
      {
        auto& txd = tx.second;
        ss << std::left
          << std::setw(4) << i++ << "  "
          << tx.first << "  "
          << std::setw(5) << txd.blob_size << "   "
          << std::setw(10) << print_money_brief(txd.fee) << "  "
          << std::setw(4) << txd.tx.vin.size() << "  "
          << std::setw(4) << txd.tx.vout.size() << "   "
          << std::right << std::setw(15) << print_money(get_outs_money_amount(txd.tx)) << std::left << "   "
          << std::setw(14) << epee::misc_utils::get_time_interval_string(get_core_time() - txd.receive_time) << "   "
          << std::setw(6) << txd.max_used_block_height << " "
          << std::setw(9) << print16(txd.max_used_block_id) << "   "
          << std::setw(6) << txd.last_failed_height << " "
          << std::setw(9) << print16(txd.last_failed_id) << "    "
          << (txd.kept_by_block ? "YES" : "no ")
          << ENDL;
      }
      return ss.str();
    }
    
    // long format
    
    m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
    {
      auto& txd = tx_entry;
      ss << "id: " << h << ENDL
        << obj_to_json_str(txd.tx) << ENDL
        << "blob_size: " << txd.blob_size << ENDL
        << "fee: " << txd.fee << ENDL
        << "kept_by_block: " << (txd.kept_by_block ? "true" : "false") << ENDL
        << "max_used_block_height: " << txd.max_used_block_height << ENDL
        << "max_used_block_id: " << txd.max_used_block_id << ENDL
        << "last_failed_height: " << txd.last_failed_height << ENDL
        << "last_failed_id: " << txd.last_failed_id << ENDL
        << "live_time: " << epee::misc_utils::get_time_interval_string(get_core_time() - txd.receive_time) << ENDL;
      return true;
    });

    return ss.str();
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::fill_block_template(block &bl, 
    bool pos, 
    size_t median_size, 
    uint64_t already_generated_coins, 
    size_t &total_size, 
    uint64_t &fee, 
    uint64_t height)
  {
    LOCAL_READONLY_TRANSACTION();
    //typedef transactions_container::value_type txv;
    typedef std::pair<crypto::hash, std::shared_ptr<const tx_details> > txv;
    

    std::vector<txv> txs_v;
    txs_v.reserve(m_db_transactions.size());
    std::vector<txv*> txs;

    //std::transform(m_transactions.begin(), m_transactions.end(), txs.begin(), [](txv &a) -> txv * { return &a; });
    //keep getting it as a values cz db items cache will keep it as unserialied object stored by shared ptrs 
    m_db_transactions.enumerate_keys([&](uint64_t i, crypto::hash& k){txs_v.resize(i + 1); txs_v[i].first = k; return true;});
    txs.resize(txs_v.size(), nullptr);
    
    for (uint64_t i = 0; i != txs_v.size(); i++)
    {      
      auto& k = txs_v[i].first;
      txs_v[i].second = m_db_transactions.get(k);
      if (!txs_v[i].second)
      {
        LOG_ERROR("Internal tx pool db error: key " << k << " was enumerated as key but couldn't get value");
        continue;
      }
      txs[i] = &txs_v[i];
    }

    
    
    std::sort(txs.begin(), txs.end(), [](txv *a, txv *b) -> bool {
      boost::multiprecision::uint128_t a_, b_;
      a_ = boost::multiprecision::uint128_t(a->second->fee) * b->second->blob_size;
      b_ = boost::multiprecision::uint128_t(b->second->fee) * a->second->blob_size;
      return a_ > b_;
    });

    size_t current_size = 0;
    uint64_t current_fee = 0;
    uint64_t best_money;
    if (!get_block_reward(pos, median_size, CURRENCY_COINBASE_BLOB_RESERVED_SIZE, already_generated_coins, best_money, height)) {
      LOG_ERROR("Block with just a miner transaction is already too large!");
      return false;
    }
    size_t best_position = 0;
    total_size = 0;
    fee = 0;
    uint64_t alias_count = 0;

    // scan txs for alias reg requests - if there are such requests, don't process alias updates
    bool alias_regs_exist = false;
    for (auto txp : txs) 
    {
      tx_extra_info ei = AUTO_VAL_INIT(ei);
      bool r = parse_and_validate_tx_extra(txp->second->tx, ei);
      CHECK_AND_ASSERT_MES(r, false, "parse_and_validate_tx_extra failed while looking up the tx pool");
      if (!ei.m_alias.m_alias.empty() && !ei.m_alias.m_sign.size()) {
        alias_regs_exist = true;
        break;
      }
    }

    const uint64_t tx_expiration_ts_median = m_blockchain.get_tx_expiration_median();

    std::unordered_set<crypto::key_image> k_images;

    for (size_t i = 0; i < txs.size(); i++)
    {
      txv &tx(*txs[i]);

      // expiration time check -- skip expired transactions
      if (is_tx_expired(tx.second->tx, tx_expiration_ts_median))
      {
          txs[i] = nullptr;
          continue;
      } 
      
      // alias checks
      tx_extra_info ei = AUTO_VAL_INIT(ei);
      bool r = parse_and_validate_tx_extra(tx.second->tx, ei);
      CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra on unprocess_blockchain_tx_extra");
      if (!ei.m_alias.m_alias.empty())
      {
        bool update_an_alias = !ei.m_alias.m_sign.empty();
        if ((alias_count >= MAX_ALIAS_PER_BLOCK) ||                   // IF this tx registers/updates an alias AND alias per block threshold exceeded
          (update_an_alias && alias_regs_exist))                      // OR this tx updates an alias AND there are alias reg requests...
        {
          txs[i] = NULL;                                              // ...skip this tx
          continue;
        }
      }

      //is_transaction_ready_to_go can change tx_details in case of some errors, so we make local copy, 
      //do check if it's changed and reassign it to db if needed
      tx_details local_copy_txd = *(tx.second);
      bool is_tx_ready_to_go_result = is_transaction_ready_to_go(local_copy_txd, tx.first);
      if (!is_tx_ready_to_go_result && 
        (local_copy_txd.last_failed_height != tx.second->last_failed_height || local_copy_txd.last_failed_id != tx.second->last_failed_id))
      {
        m_db_transactions.begin_transaction();
        m_db_transactions.set(get_transaction_hash(local_copy_txd.tx), local_copy_txd);
        m_db_transactions.commit_transaction();
      }

      if (!is_tx_ready_to_go_result || have_key_images(k_images, tx.second->tx)) {
        txs[i] = NULL;
        continue;
      }
      append_key_images(k_images, tx.second->tx);

      current_size += tx.second->blob_size;
      current_fee += tx.second->fee;

      uint64_t current_reward;
      if (!get_block_reward(pos, median_size, current_size + CURRENCY_COINBASE_BLOB_RESERVED_SIZE, already_generated_coins, current_reward, height))
      {
        break; // current block size is too big
      }

      if (best_money < current_reward + current_fee) {
        best_money = current_reward + current_fee;
        best_position = i + 1;
        total_size = current_size;
        fee = current_fee;
      }

      if (!ei.m_alias.m_alias.empty())
        ++alias_count;
    }

    for (size_t i = 0; i != txs.size(); i++)
    {
      if (txs[i])
      {
        if (i < best_position)
        {
          bl.tx_hashes.push_back(txs[i]->first);
        }
        else if (have_attachment_service_in_container(txs[i]->second->tx.attachment, BC_OFFERS_SERVICE_ID, BC_OFFERS_SERVICE_INSTRUCTION_DEL))
        {
          // BC_OFFERS_SERVICE_INSTRUCTION_DEL transactions has zero fee, so include them here regardless of reward effectiveness
          bl.tx_hashes.push_back(txs[i]->first);
          total_size += txs[i]->second->blob_size;
        }
      }
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::initialize_db_solo_options_values()
  {
    m_db.begin_transaction();
    m_db_storage_major_compatibility_version = TRANSACTION_POOL_MAJOR_COMPATIBILITY_VERSION;
    m_db.commit_transaction();
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::init(const std::string& config_folder)
  {
    m_config_folder = config_folder;

    uint64_t cache_size = CACHE_SIZE;
    LOG_PRINT_GREEN("Using pool db file cache size(L1): " << cache_size, LOG_LEVEL_0);

    LOG_PRINT_L0("Loading blockchain...");
    const std::string folder_name = m_config_folder + "/" CURRENCY_POOLDATA_FOLDERNAME;
    tools::create_directories_if_necessary(folder_name);
    bool res = m_db.open(folder_name, cache_size);
    CHECK_AND_ASSERT_MES(res, false, "Failed to initialize pool database in folder: " << folder_name);

    res = m_db_transactions.init(TRANSACTION_POOL_CONTAINER_TRANSACTIONS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_key_images_set.init(TRANSACTION_POOL_CONTAINER_KEY_IMAGES);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_cancel_offer_hash.init(TRANSACTION_POOL_CONTAINER_CANCEL_OFFER_HASH);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_black_tx_list.init(TRANSACTION_POOL_CONTAINER_BLACK_TX_LIST);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_alias_names.init(TRANSACTION_POOL_CONTAINER_ALIAS_NAMES);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_alias_addresses.init(TRANSACTION_POOL_CONTAINER_ALIAS_ADDRESSES);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_solo_options.init(TRANSACTION_POOL_CONTAINER_SOLO_OPTIONS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");


    m_db_transactions.set_cache_size(1000);
    m_db_alias_names.set_cache_size(10000);
    m_db_alias_addresses.set_cache_size(10000);
    m_db_cancel_offer_hash.set_cache_size(1000);
    m_db_black_tx_list.set_cache_size(1000);

    bool need_reinit = false;
    if (m_db_storage_major_compatibility_version != TRANSACTION_POOL_MAJOR_COMPATIBILITY_VERSION)
      need_reinit = true;

    if (need_reinit)
    {
      clear();
      LOG_PRINT_MAGENTA("Tx Pool reinitialized.", LOG_LEVEL_0);
    }
    initialize_db_solo_options_values();

    LOG_PRINT_GREEN("TX_POOL Initialized ok. (" << m_db_transactions.size() << " transactions)", LOG_LEVEL_0);
    return true;
  }

  //---------------------------------------------------------------------------------
  bool tx_memory_pool::deinit()
  {
    m_db.close();
    return true;
  }
  //---------------------------------------------------------------------------------
  uint64_t tx_memory_pool::get_core_time() const
  {
    return m_blockchain.get_core_runtime_config().get_core_time();
  }
  //---------------------------------------------------------------------------------
  namespace
  {
    struct ms_out_info
    {
      crypto::hash multisig_id;
      size_t input_output_index;
      bool is_input;
    };
  }
  //---------------------------------------------------------------------------------
  void collect_multisig_ids_from_tx(const transaction& tx, std::vector<ms_out_info>& result)
  {
    result.reserve(tx.vin.size() + tx.vout.size());

    size_t idx = 0;
    for (const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_multisig))
        result.push_back(ms_out_info({ boost::get<txin_multisig>(in).multisig_out_id, idx, true }));
      ++idx;
    }

    idx = 0;
    for (const auto& out : tx.vout)
    {
      if (out.target.type() == typeid(txout_multisig))
        result.push_back(ms_out_info({ get_multisig_out_id(tx, idx), idx, false }));
      ++idx;
    }
  }
  //---------------------------------------------------------------------------------
  bool does_tx_have_given_multisig_id(const transaction& tx, const crypto::hash& multisig_id)
  {
    for (const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_multisig) && boost::get<txin_multisig>(in).multisig_out_id == multisig_id)
        return true;
    }

    size_t idx = 0;
    for (const auto& out : tx.vout)
    {
      if (out.target.type() == typeid(txout_multisig) && get_multisig_out_id(tx, idx) == multisig_id)
        return true;
      ++idx;
    }

    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::check_tx_multisig_ins_and_outs(const transaction& tx, bool check_against_pool_txs) const 
  {
    
    LOCAL_READONLY_TRANSACTION();
    std::vector<ms_out_info> ms_ids;
    collect_multisig_ids_from_tx(tx, ms_ids);

    for (auto& el : ms_ids)
    {
      // check given multisig output against all blockchain's multisig outputs
      if (!el.is_input && m_blockchain.has_multisig_output(el.multisig_id))
      {
        LOG_PRINT_L0("Transaction " << get_transaction_hash(tx) << " : output #" << el.input_output_index << " has multisig id " << el.multisig_id << " that is already in the blockchain");
        return false;
      }

      // check given multisig input/output against all transactions in the pool
      if (check_against_pool_txs)
      {
        bool has_found = false;
        m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)
        {
          if (does_tx_have_given_multisig_id(tx_entry.tx, el.multisig_id))
          {
            has_found = true;
            LOG_PRINT_L0("Transaction " << get_transaction_hash(tx) << (el.is_input ? " : input #" : " : output #") << el.input_output_index << " has multisig id " << el.multisig_id <<
              " that is already in the pool in tx " << get_transaction_hash(tx_entry.tx));
            return false;
          }  
          return true;
        });
        if (has_found)
          return false;
      }
    }

    return true;
  }
}

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL


