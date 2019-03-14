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

//#define TRANSACTION_POOL_MAJOR_COMPATIBILITY_VERSION      BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION + 1

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "tx_pool"
ENABLE_CHANNEL_BY_DEFAULT("tx_pool");

namespace currency
{
  //---------------------------------------------------------------------------------
  tx_memory_pool::tx_memory_pool(blockchain_storage& bchs, i_currency_protocol* pprotocol)
    : m_blockchain(bchs)
    , m_pprotocol(pprotocol)
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
      crypto::key_image spent_ki = AUTO_VAL_INIT(spent_ki);
      if(have_tx_keyimges_as_spent(tx, &spent_ki))
      {
        LOG_ERROR("Transaction " << id << " uses already spent key image " << spent_ki);
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
  bool tx_memory_pool::do_insert_transaction(const transaction &tx, const crypto::hash &id, uint64_t blob_size, bool kept_by_block, uint64_t fee, const crypto::hash& max_used_block_id, uint64_t max_used_block_height)
  {
    TIME_MEASURE_START_PD(update_db_time);
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    
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

    m_transactions.insert(std::make_pair(id, td));
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

    CRITICAL_REGION_LOCAL(m_cancel_offer_hashes_lock);

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
          if (m_cancel_offer_hashes.count(co_list.back().tx_id))
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
      m_cancel_offer_hashes.insert(co.tx_id);
    }

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::unprocess_cancel_offer_rules(const transaction& tx)
  {
    CRITICAL_REGION_LOCAL(m_cancel_offer_hashes_lock);

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
          auto it = m_cancel_offer_hashes.find(co_list.back().tx_id);
          if (it == m_cancel_offer_hashes.end())
            return false;
          m_cancel_offer_hashes.erase(co_list.back().tx_id);
        }
      }
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_aliases_from_tx_pool(std::list<extra_alias_entry>& aliases)const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    //TODO: OPTIMIZATION put cache here!!!!!
    for (auto& td : m_transactions)
    {
      tx_extra_info ei = AUTO_VAL_INIT(ei);
      bool r = parse_and_validate_tx_extra(td.second.tx, ei);
      CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra for tx " << get_transaction_hash(td.second.tx));
      if (ei.m_alias.m_alias.size())
        aliases.push_back(ei.m_alias);
    }
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
    CRITICAL_REGION_LOCAL(m_aliases_lock);

    extra_alias_entry eai = AUTO_VAL_INIT(eai);
    if (get_type_in_variant_container(tx.extra, eai))
    {
      //check in blockchain
      extra_alias_entry eai2 = AUTO_VAL_INIT(eai2);
      bool already_have_alias_registered = m_blockchain.get_alias_info(eai.m_alias, eai2);
      
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
        if (m_alias_names_set.count(eai.m_alias))
        {
          LOG_PRINT_L0("Alias \"" << eai.m_alias << "\" already in transaction pool, transaction rejected");
          return false;
        }
        if (m_alias_addresses_set.count(eai.m_address))
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
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    auto it = m_transactions.find(id);
    if (it == m_transactions.end())
      return false;

    tx = it->second.tx;
    blob_size = it->second.blob_size;
    fee = it->second.fee;
    unprocess_cancel_offer_rules(it->second.tx);
    m_transactions.erase(id);
    on_tx_remove(tx, it->second.kept_by_block);
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

    CRITICAL_REGION_LOCAL1(m_transactions_lock); // !!TODO!! review lock scheme

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

    for(auto& td : m_transactions)
    {
      auto& tx_entry = td.second;
      const crypto::hash& h = td.first;
      //m_db_transactions.enumerate_items([&](uint64_t i, const crypto::hash& h, const tx_details &tx_entry)

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
    }
    
    for (auto& e : to_delete)
    {
      m_transactions.erase(e.hash);
      on_tx_remove(e.tx, e.kept_by_block);
    }

    return true;
  }
  //---------------------------------------------------------------------------------
  size_t tx_memory_pool::get_transactions_count() const 
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    return m_transactions.size();
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transactions(std::list<transaction>& txs) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    for(auto& td : m_transactions)
      txs.push_back(td.second.tx);

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_all_transactions_details(std::list<tx_rpc_extended_info>& txs) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    for(auto& td : m_transactions)
    {
      txs.push_back(tx_rpc_extended_info());
      tx_rpc_extended_info& trei = txs.back();
      trei.blob_size = td.second.blob_size;
      fill_tx_rpc_details(trei, td.second.tx, nullptr, td.first, td.second.receive_time, true);
    }

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_all_transactions_brief_details(std::list<tx_rpc_brief_info>& txs) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    for(auto& td : m_transactions)
    {
      txs.push_back(tx_rpc_brief_info());
      tx_rpc_brief_info& trbi = txs.back();
      trbi.id = epee::string_tools::pod_to_hex(td.first);
      trbi.fee = td.second.fee;
      trbi.sz = td.second.blob_size;
      trbi.total_amount = get_outs_money_amount(td.second.tx);
    }

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_all_transactions_list(std::list<std::string>& txs)const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    for(auto& td : m_transactions)
      txs.push_back(epee::string_tools::pod_to_hex(td.first));

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transactions_details(const std::list<std::string>& ids, std::list<tx_rpc_extended_info>& txs) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    for (auto& id_str : ids)
    {
      crypto::hash id = null_hash;
      if (!epee::string_tools::hex_to_pod(id_str, id))
      {
        LOG_ERROR("Failed to parse id in list: " << id_str);
        return false;
      }

      auto it = m_transactions.find(id);
      if (it == m_transactions.end())
        return false;

      txs.push_back(tx_rpc_extended_info());
      tx_rpc_extended_info& trei = txs.back();
      trei.blob_size = it->second.blob_size;
      fill_tx_rpc_details(trei, it->second.tx, nullptr, id, it->second.receive_time, false);
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transactions_brief_details(const std::list<std::string>& ids, std::list<tx_rpc_brief_info>& txs)const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    for (auto& id_str : ids)
    {
      crypto::hash id = null_hash;
      if (!epee::string_tools::hex_to_pod(id_str, id))
      {
        LOG_ERROR("Failed to parse id in list: " << id_str);
        return false;
      }

      auto it = m_transactions.find(id);
      if (it == m_transactions.end())
        return false;

      txs.push_back(tx_rpc_brief_info());
      tx_rpc_brief_info& trbi = txs.back();
      trbi.sz = it->second.blob_size;
      trbi.id = id_str;
      trbi.fee = it->second.fee;
      trbi.total_amount = get_outs_money_amount(it->second.tx);
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transaction_details(const crypto::hash& id, tx_rpc_extended_info& trei) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    auto it = m_transactions.find(id);
    if (it == m_transactions.end())
      return false;

    fill_tx_rpc_details(trei, it->second.tx, nullptr, id, it->second.receive_time, false);
    return true;
  }  
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transaction(const crypto::hash& id, transaction& tx) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    auto it = m_transactions.find(id);
    if (it == m_transactions.end())
      return false;

    tx = it->second.tx;
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transaction(const crypto::hash& id, tx_details& txd) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    auto it = m_transactions.find(id);
    if (it == m_transactions.end())
      return false;

    txd = it->second;
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

    {
      CRITICAL_REGION_LOCAL(m_transactions_lock);
      for(auto& td : m_transactions)
        r.txs.push_back(t_serializable_object_to_blob(td.second.tx));
    }

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
    CRITICAL_REGION_LOCAL(m_black_tx_list_lock);
    LOG_PRINT_YELLOW("TX ADDED TO POOL'S BLACKLIST: " << get_transaction_hash(tx), LOG_LEVEL_0);
    m_black_tx_list.insert(get_transaction_hash(tx));
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx(const crypto::hash &id) const
  {    
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    if(m_transactions.count(id))
      return true;

    if (check_is_taken(id))
      return true;

    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx_keyimges_as_spent(const transaction& tx, crypto::key_image* p_spent_ki /* = nullptr */) const
  {
    for(const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, true);//should never fail
        if (have_tx_keyimg_as_spent(tokey_in.k_image))
        {
          if (p_spent_ki)
            *p_spent_ki = tokey_in.k_image;
          return true;
        }

      }
    }
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::insert_key_images(const transaction& tx, bool kept_by_block)
  {
    CRITICAL_REGION_LOCAL(m_key_images_set_lock);

    for(const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, true);//should never fail
        uint64_t count = 0;
        auto it = m_key_images_set.find(tokey_in.k_image);
        if (it != m_key_images_set.end())
          count = it->second;
        uint64_t count_before = count;
        ++count;
        m_key_images_set[tokey_in.k_image] = count;
        LOG_PRINT_L2("tx pool: key image added: " << tokey_in.k_image << ", from tx " << get_transaction_hash(tx) << ", counter: " << count_before << " -> " << count);
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
    CRITICAL_REGION_LOCAL(m_aliases_lock);
    
    tx_extra_info ei = AUTO_VAL_INIT(ei);
    bool r = parse_and_validate_tx_extra(tx, ei);
    CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra on unprocess_blockchain_tx_extra");
    if (ei.m_alias.m_alias.size())
    {
      m_alias_names_set.insert(ei.m_alias.m_alias);
      m_alias_addresses_set.insert(ei.m_alias.m_address);
    }

    return true;
  }
  //--------------------------------------------------------------------------------- 
  bool tx_memory_pool::remove_alias_info(const transaction& tx)
  {
    CRITICAL_REGION_LOCAL(m_aliases_lock);
    
    tx_extra_info ei = AUTO_VAL_INIT(ei);
    bool r = parse_and_validate_tx_extra(tx, ei);
    CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra on unprocess_blockchain_tx_extra");
    if (ei.m_alias.m_alias.size())
    {
      m_alias_names_set.erase(ei.m_alias.m_alias);
      m_alias_addresses_set.erase(ei.m_alias.m_address);
    }

    return true;
  }
  //--------------------------------------------------------------------------------- 
  bool tx_memory_pool::remove_key_images(const transaction& tx, bool kept_by_block)
  {
    CRITICAL_REGION_LOCAL(m_key_images_set_lock);

    for(const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, true);//should never fail
        uint64_t count = 0;
        auto it = m_key_images_set.find(tokey_in.k_image);
        if (it == m_key_images_set.end())
        {
          LOG_ERROR("INTERNAL_ERROR: for tx " << get_transaction_hash(tx) << " key image " << tokey_in.k_image << " not found");
          continue;
        }
        count = it->second;
        uint64_t count_before = count;
        --count;
        if (count)
          m_key_images_set[tokey_in.k_image] = count;
        else
          m_key_images_set.erase(tokey_in.k_image);
        LOG_PRINT_L2("tx pool: key image removed: " << tokey_in.k_image << ", from tx " << get_transaction_hash(tx) << ", counter: " << count_before << " -> " << count);
      }
    }
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_key_images_from_tx_pool(std::unordered_set<crypto::key_image>& key_images) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    
    for(auto td : m_transactions)
    {
      for (auto& in : td.second.tx.vin)
      {
        if (in.type() == typeid(txin_to_key))
        {
          key_images.insert(boost::get<txin_to_key>(in).k_image);
        }
      }
    }

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx_keyimg_as_spent(const crypto::key_image& key_im)const
  {
    CRITICAL_REGION_LOCAL(m_key_images_set_lock);

    auto it = m_key_images_set.find(key_im);
    if (it != m_key_images_set.end())
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
    m_transactions.clear();
    m_cancel_offer_hashes.clear();
    // should m_db_black_tx_list be cleared here?
    m_key_images_set.clear();
    // TODO : m_alias_names_set ?
    // TODO : m_alias_addresses_set ?
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::clear()
  {
    m_transactions.clear();
    m_cancel_offer_hashes.clear();
    m_black_tx_list.clear();
    m_key_images_set.clear();
    // TODO : m_alias_names_set ?
    // TODO : m_alias_addresses_set ?
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::is_transaction_ready_to_go(tx_details& txd, const crypto::hash& id)const 
  {
    //not the best implementation at this time, sorry :(

    {
      CRITICAL_REGION_LOCAL(m_black_tx_list_lock);
      if (m_black_tx_list.count(get_transaction_hash(txd.tx)))
        return false;
    }

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
  bool tx_memory_pool::have_key_images(const std::unordered_set<crypto::key_image>& k_images, const transaction& tx)
  {
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
        CRITICAL_REGION_LOCAL(m_transactions_lock);
        txs.assign(m_transactions.begin(), m_transactions.end());
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
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    for(auto& td : m_transactions)
    {
      auto& txd = td.second;
      ss << "id: " << td.first << ENDL
        << obj_to_json_str(txd.tx) << ENDL
        << "blob_size: " << txd.blob_size << ENDL
        << "fee: " << txd.fee << ENDL
        << "kept_by_block: " << (txd.kept_by_block ? "true" : "false") << ENDL
        << "max_used_block_height: " << txd.max_used_block_height << ENDL
        << "max_used_block_id: " << txd.max_used_block_id << ENDL
        << "last_failed_height: " << txd.last_failed_height << ENDL
        << "last_failed_id: " << txd.last_failed_id << ENDL
        << "live_time: " << epee::misc_utils::get_time_interval_string(get_core_time() - txd.receive_time) << ENDL;
    }

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
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    typedef std::pair<crypto::hash, tx_details> txv;

    std::vector<txv> txs_v;
    txs_v.reserve(m_transactions.size());
    std::vector<txv*> txs;
    txs.reserve(m_transactions.size());

    //std::transform(m_transactions.begin(), m_transactions.end(), txs.begin(), [](txv &a) -> txv * { return &a; });
    //keep getting it as a values cz db items cache will keep it as unserialied object stored by shared ptrs 
    size_t i = 0;
    for(auto& t : m_transactions)
    {
      txs_v.resize(i + 1);
      txs_v[i] = t;
      
      txs.resize(i + 1);
      txs[i] = &txs_v[i];
    }
    
    std::sort(txs.begin(), txs.end(), [](txv *a, txv *b) -> bool {
      boost::multiprecision::uint128_t a_, b_;
      a_ = boost::multiprecision::uint128_t(a->second.fee) * b->second.blob_size;
      b_ = boost::multiprecision::uint128_t(b->second.fee) * a->second.blob_size;
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
      bool r = parse_and_validate_tx_extra(txp->second.tx, ei);
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
      if (is_tx_expired(tx.second.tx, tx_expiration_ts_median))
      {
          txs[i] = nullptr;
          continue;
      } 
      
      // alias checks
      tx_extra_info ei = AUTO_VAL_INIT(ei);
      bool r = parse_and_validate_tx_extra(tx.second.tx, ei);
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
      tx_details local_copy_txd = tx.second;
      bool is_tx_ready_to_go_result = is_transaction_ready_to_go(local_copy_txd, tx.first);
      if (!is_tx_ready_to_go_result && 
        (local_copy_txd.last_failed_height != tx.second.last_failed_height || local_copy_txd.last_failed_id != tx.second.last_failed_id))
      {
        m_transactions[get_transaction_hash(local_copy_txd.tx)] = local_copy_txd;
      }

      if (!is_tx_ready_to_go_result || have_key_images(k_images, tx.second.tx)) {
        txs[i] = NULL;
        continue;
      }
      append_key_images(k_images, tx.second.tx);

      current_size += tx.second.blob_size;
      current_fee += tx.second.fee;

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
        else if (have_attachment_service_in_container(txs[i]->second.tx.attachment, BC_OFFERS_SERVICE_ID, BC_OFFERS_SERVICE_INSTRUCTION_DEL))
        {
          // BC_OFFERS_SERVICE_INSTRUCTION_DEL transactions has zero fee, so include them here regardless of reward effectiveness
          bl.tx_hashes.push_back(txs[i]->first);
          total_size += txs[i]->second.blob_size;
        }
      }
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::initialize_db_solo_options_values()
  {
    //m_db.begin_transaction();
    //m_db_storage_major_compatibility_version = TRANSACTION_POOL_MAJOR_COMPATIBILITY_VERSION;
    //m_db.commit_transaction();
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::init(const std::string& config_folder)
  {
    m_config_folder = config_folder;

    const std::string filename = m_config_folder + "/" CURRENCY_POOLDATA_FILENAME;
    LOG_PRINT_L0("Loading tx pool from " << filename);
    boost::system::error_code ec;
    if(!boost::filesystem::exists(filename, ec))
      return true;
    bool res = tools::unserialize_obj_from_file(*this, filename);
    if(!res)
    {
      LOG_PRINT_L0("Failed to load tx pool from " << filename);
    }


    // TODO

    bool need_reinit = false;
    //if (m_db_storage_major_compatibility_version != TRANSACTION_POOL_MAJOR_COMPATIBILITY_VERSION)
    //  need_reinit = true;

    if (need_reinit)
    {
      clear();
      LOG_PRINT_MAGENTA("Tx Pool reinitialized.", LOG_LEVEL_0);
    }
    initialize_db_solo_options_values();

    LOG_PRINT_GREEN("TX_POOL Initialized ok. (" << m_transactions.size() << " transactions)", LOG_LEVEL_0);
    return true;
  }

  //---------------------------------------------------------------------------------
  bool tx_memory_pool::deinit()
  {
    if (!tools::create_directories_if_necessary(m_config_folder))
    {
      LOG_PRINT_L0("Failed to create data directory: " << m_config_folder);
      return false;
    }

    std::string filename = m_config_folder + "/" CURRENCY_POOLDATA_FILENAME;
    bool res = tools::serialize_obj_to_file(*this, filename);
    if(!res)
    {
      LOG_PRINT_L0("Failed to serialize tx pool to file " << filename);
    }
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
        result.push_back(ms_out_info{ boost::get<txin_multisig>(in).multisig_out_id, idx, true });
      ++idx;
    }

    idx = 0;
    for (const auto& out : tx.vout)
    {
      if (out.target.type() == typeid(txout_multisig))
        result.push_back(ms_out_info{ get_multisig_out_id(tx, idx), idx, false });
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
    CRITICAL_REGION_LOCAL(m_transactions_lock);

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
        for(auto& t : m_transactions)
        {
          if (does_tx_have_given_multisig_id(t.second.tx, el.multisig_id))
          {
            has_found = true;
            LOG_PRINT_L0("Transaction " << get_transaction_hash(tx) << (el.is_input ? " : input #" : " : output #") << el.input_output_index << " has multisig id " << el.multisig_id <<
              " that is already in the pool in tx " << get_transaction_hash(t.second.tx));
            return false;
          }  
        }

        if (has_found)
          return false;
      }
    }

    return true;
  }
}
