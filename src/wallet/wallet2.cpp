// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <numeric>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include <boost/utility/value_init.hpp>
#include "include_base_utils.h"
using namespace epee;

#include "string_coding.h"
#define KEEP_WALLET_LOG_MACROS
#include "wallet2.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/bc_offers_service_basic.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "misc_language.h"

#include "common/boost_serialization_helper.h"
#include "crypto/crypto.h"
#include "serialization/binary_utils.h"
#include "currency_core/bc_payments_id_service.h"
#include "version.h"
using namespace currency;

#define MINIMUM_REQUIRED_WALLET_FREE_SPACE_BYTES (100*1024*1024) // 100 MB

#undef LOG_DEFAULT_CHANNEL
#define LOG_DEFAULT_CHANNEL "wallet"
ENABLE_CHANNEL_BY_DEFAULT("wallet")
namespace tools
{

  //---------------------------------------------------------------
  uint64_t wallet2::get_max_unlock_time_from_receive_indices(const currency::transaction& tx, const money_transfer2_details& td)
  {
    uint64_t max_unlock_time = 0;
    // etc_tx_details_expiration_time have priority over etc_tx_details_expiration_time2
    uint64_t major_unlock_time = get_tx_x_detail<etc_tx_details_unlock_time>(tx);
    if (major_unlock_time)
      return major_unlock_time;

    etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
    get_type_in_variant_container(tx.extra, ut2);
    if (!ut2.unlock_time_array.size())
      return 0;

    CHECK_AND_ASSERT_THROW_MES(ut2.unlock_time_array.size() == tx.vout.size(), "Internal error: wrong tx transfer details: ut2.unlock_time_array.size()" << ut2.unlock_time_array.size() << " is not equal transaction outputs vector size=" << tx.vout.size());

    for (auto ri : td.receive_indices)
    {
      CHECK_AND_ASSERT_THROW_MES(ri < tx.vout.size(), "Internal error: wrong tx transfer details: reciev index=" << ri << " is greater than transaction outputs vector " << tx.vout.size());
      if (tx.vout[ri].target.type() == typeid(currency::txout_to_key))
      {
        //update unlock_time if needed
        if (ut2.unlock_time_array[ri] > max_unlock_time)
          max_unlock_time = ut2.unlock_time_array[ri];
      }
    }
    return max_unlock_time;
  }
//----------------------------------------------------------------------------------------------------
void wallet2::fill_transfer_details(const currency::transaction& tx, const tools::money_transfer2_details& td, tools::wallet_public::wallet_transfer_info_details& res_td) const
{
  PROFILE_FUNC("wallet2::fill_transfer_details");
  for (auto si : td.spent_indices)
  {
    WLT_CHECK_AND_ASSERT_MES(si < tx.vin.size(), void(), "Internal error: wrong tx transfer details: spend index=" << si << " is greater than transaction inputs vector " << tx.vin.size());
    if (tx.vin[si].type() == typeid(currency::txin_to_key))
      res_td.spn.push_back(boost::get<currency::txin_to_key>(tx.vin[si]).amount);
  }

  for (auto ri : td.receive_indices)
  {
    WLT_CHECK_AND_ASSERT_MES(ri < tx.vout.size(), void(), "Internal error: wrong tx transfer details: reciev index=" << ri << " is greater than transaction outputs vector " << tx.vout.size());
    if (tx.vout[ri].target.type() == typeid(currency::txout_to_key))
    {
      res_td.rcv.push_back(tx.vout[ri].amount);
    }
  }
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::transfer_flags_to_str(uint32_t flags)
{
  std::string result(5, ' ');
  if (flags & WALLET_TRANSFER_DETAIL_FLAG_SPENT)
    result[0] = 's';
  if (flags & WALLET_TRANSFER_DETAIL_FLAG_BLOCKED)
    result[1] = 'b';
  if (flags & WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION)
    result[2] = 'e';
  if (flags & WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER)
    result[3] = 'm';
  if (flags & WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION)
    result[4] = 'c';
  return result;
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::transform_tx_to_str(const currency::transaction& tx)
{
  return currency::obj_to_json_str(tx);
}
//----------------------------------------------------------------------------------------------------
currency::transaction wallet2::transform_str_to_tx(const std::string& tx_str)
{
  THROW_IF_TRUE_WALLET_INT_ERR_EX_NO_HANDLER(false, "transform_str_to_tx shoruld never be called");
  return currency::transaction();
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::transfer_details_base_to_amount(const transfer_details_base& tdb)
{
  return tdb.amount();
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::transfer_details_base_to_tx_hash(const transfer_details_base& tdb)
{
  return epee::string_tools::pod_to_hex(currency::get_transaction_hash(tdb.m_ptx_wallet_info->m_tx));
}
//----------------------------------------------------------------------------------------------------
const wallet2::transaction_wallet_info& wallet2::transform_ptr_to_value(const std::shared_ptr<wallet2::transaction_wallet_info>& a)
{
  return *a;
}
//----------------------------------------------------------------------------------------------------
std::shared_ptr<wallet2::transaction_wallet_info> wallet2::transform_value_to_ptr(const wallet2::transaction_wallet_info& d)
{
  THROW_IF_TRUE_WALLET_INT_ERR_EX_NO_HANDLER(false, "transform_value_to_ptr shoruld never be called");
  return std::shared_ptr<wallet2::transaction_wallet_info>();
}

//----------------------------------------------------------------------------------------------------
void wallet2::init(const std::string& daemon_address)
{
  m_miner_text_info = PROJECT_VERSION_LONG;
  m_core_proxy->set_connection_addr(daemon_address);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::set_core_proxy(const std::shared_ptr<i_core_proxy>& proxy)
{
  THROW_IF_TRUE_WALLET_EX(!proxy, error::wallet_internal_error, "Trying to set null core proxy.");
  m_core_proxy = proxy;
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_pos_mint_packing_size(uint64_t new_size)
{
  m_pos_mint_packing_size = new_size;
}
//----------------------------------------------------------------------------------------------------
std::shared_ptr<i_core_proxy> wallet2::get_core_proxy()
{
  return m_core_proxy;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_transfer_info_by_key_image(const crypto::key_image& ki, transfer_details& td, size_t& i)
{
  auto it = m_key_images.find(ki);
  if (it == m_key_images.end())
  {
    return false;
  }
  THROW_IF_FALSE_WALLET_EX(it->second < m_transfers.size(), error::wallet_internal_error, "wrong out in transaction: internal index");
  td = m_transfers[it->second];
  i = it->second;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_transfer_info_by_index(size_t i, transfer_details& td)
{
  WLT_CHECK_AND_ASSERT_MES(i < m_transfers.size(), false, "wrong out in transaction: internal index, m_transfers.size()=" << m_transfers.size());
  td = m_transfers[i];
  return true;
}
//----------------------------------------------------------------------------------------------------
size_t wallet2::scan_for_collisions(std::unordered_map<crypto::key_image, std::list<size_t> >& key_images)
{
  size_t count = 0;  
  for (size_t i = 0; i != m_transfers.size(); i++)
  {
    key_images[m_transfers[i].m_key_image].push_back(i);
    if (key_images[m_transfers[i].m_key_image].size() > 1)
      count++;
  }
  return count;
}
//----------------------------------------------------------------------------------------------------
size_t wallet2::fix_collisions()
{
  std::unordered_map<crypto::key_image, std::list<size_t> > key_images;
  scan_for_collisions(key_images);
  size_t count = 0;
  for (auto &coll_entry : key_images)
  {
    if(coll_entry.second.size()<2)
      continue;

    currency::COMMAND_RPC_CHECK_KEYIMAGES::request req_ki = AUTO_VAL_INIT(req_ki);
    req_ki.images.push_back(coll_entry.first);
    currency::COMMAND_RPC_CHECK_KEYIMAGES::response rsp_ki = AUTO_VAL_INIT(rsp_ki);
    bool r = m_core_proxy->call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(req_ki, rsp_ki);
    THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "unable to get spent key image info for keyimage: " << coll_entry.first);
    THROW_IF_FALSE_WALLET_INT_ERR_EX(rsp_ki.images_stat.size() == 1, "unable to get spent key image info for keyimage: " << coll_entry.first << "keyimages size()=" << rsp_ki.images_stat.size());
    THROW_IF_FALSE_WALLET_INT_ERR_EX(*rsp_ki.images_stat.begin() != 0, "unable to get spent key image info for keyimage: " << coll_entry.first << "keyimages [0]=0");


    for (auto it = coll_entry.second.begin(); it!= coll_entry.second.end(); it++)
    {
      m_transfers[*it].m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;
      m_transfers[*it].m_spent_height = *rsp_ki.images_stat.begin();
      WLT_LOG_L0("Fixed collision for key image " << coll_entry.first << " transfer " << count);
      count++;
    }
  }

  return count;
}
//----------------------------------------------------------------------------------------------------
size_t wallet2::scan_for_transaction_entries(const crypto::hash& tx_id, const crypto::key_image& ki, std::list<transfer_details>& details)
{
  bool check_ki = ki != currency::null_ki;
  bool check_tx_id = tx_id != currency::null_hash;

  for (auto it = m_transfers.begin(); it != m_transfers.end(); it++)
  {
    if (check_ki && it->m_key_image == ki)
    {
      details.push_back(*it);
    }
    if (check_tx_id && get_transaction_hash(it->m_ptx_wallet_info->m_tx) == tx_id)
    {
      details.push_back(*it);
    }
  }
  return details.size();
}
//----------------------------------------------------------------------------------------------------
void wallet2::fetch_tx_global_indixes(const currency::transaction& tx, std::vector<uint64_t>& goutputs_indexes)
{
  std::list<std::reference_wrapper<const currency::transaction>> txs;
  txs.push_back(tx);
  std::vector<std::vector<uint64_t> > res;
  fetch_tx_global_indixes(txs, res);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(res.size() == 1, "fetch_tx_global_indixes for single entry returned wrong result: res.size()=" << res.size());
  goutputs_indexes = res[0];
}
//----------------------------------------------------------------------------------------------------
void wallet2::fetch_tx_global_indixes(const std::list<std::reference_wrapper<const currency::transaction>>& txs, std::vector<std::vector<uint64_t>>& goutputs_indexes)
{
  currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response res = AUTO_VAL_INIT(res);
  for (auto& tx : txs)
  {
    req.txids.push_back(get_transaction_hash(tx));
  }

  bool r = m_core_proxy->call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(req, res);
  THROW_IF_TRUE_WALLET_EX(!r, error::no_connection_to_daemon, "get_o_indexes.bin");
  THROW_IF_TRUE_WALLET_EX(res.status == API_RETURN_CODE_BUSY, error::daemon_busy, "get_o_indexes.bin");
  THROW_IF_TRUE_WALLET_EX(res.status != API_RETURN_CODE_OK, error::get_out_indices_error, res.status);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(res.tx_global_outs.size() == txs.size(), "res.tx_global_outs.size()(" << res.tx_global_outs.size()
    << ") == txs.size()(" << txs.size() << ")");
  goutputs_indexes.resize(txs.size());
  auto it_resp = res.tx_global_outs.begin();
  auto it_txs = txs.begin();
  size_t i = 0;
  for (; it_resp != res.tx_global_outs.end();)
  {
    THROW_IF_FALSE_WALLET_INT_ERR_EX(it_resp->v.size() == it_txs->get().vout.size(),
      "transactions outputs size=" << it_txs->get().vout.size() <<
      " not match with COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES response[i] size=" << it_resp->v.size());

    goutputs_indexes[i] = it_resp->v;
    it_resp++; it_txs++; i++;
  }
}

//----------------------------------------------------------------------------------------------------
void wallet2::process_new_transaction(const currency::transaction& tx, uint64_t height, const currency::block& b, const std::vector<uint64_t>* pglobal_indexes)
{
  std::vector<std::string> recipients, recipients_aliases;
  process_unconfirmed(tx, recipients, recipients_aliases);
  std::vector<size_t> outs;
  uint64_t tx_money_got_in_outs = 0;
  crypto::public_key tx_pub_key = null_pkey;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  THROW_IF_TRUE_WALLET_EX(!r, error::tx_extra_parse_error, tx);

  //check for transaction spends
  uint64_t tx_money_spent_in_ins = 0;
  // check all outputs for spending (compare key images)
  money_transfer2_details mtd;
  size_t i = 0;
  bool is_pos_coinbase = false;
  bool coin_base_tx = is_coinbase(tx, is_pos_coinbase);
  //PoW block don't have change, so all outs supposed to be marked as "mined"
  bool is_derived_from_coinbase = !is_pos_coinbase;

  for(auto& in : tx.vin)
  {
    if (in.type() == typeid(currency::txin_to_key))
    {
      const currency::txin_to_key& intk = boost::get<currency::txin_to_key>(in);

      // check if this input spends our output
      //transfer_details* p_td = nullptr;
      uint64_t tid = UINT64_MAX;
      
      if (is_auditable() && is_watch_only())
      {
        // auditable wallet
        // try to find a reference among own UTXOs
        std::vector<txout_v> abs_key_offsets = relative_output_offsets_to_absolute(intk.key_offsets); // potential speed-up: don't convert to abs offsets as we interested only in direct spends for auditable wallets. Now it's kind a bit paranoid.
        for(auto v : abs_key_offsets)
        {
          if (v.type() != typeid(uint64_t))
            continue;
          uint64_t gindex = boost::get<uint64_t>(v);
          auto it = m_amount_gindex_to_transfer_id.find(std::make_pair(intk.amount, gindex));
          if (it != m_amount_gindex_to_transfer_id.end())
          {
            tid = it->second;
            WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(tid < m_transfers.size(), "invalid tid: " << tid << ", ref from input with amount: " << intk.amount << ", gindex: " << gindex);
            auto& td = m_transfers[it->second];
            if (intk.key_offsets.size() != 1)
            {
              // own output was used in non-direct transaction
              // the core should not allow this to happen, the only way it may happen - mixing in own output that was sent without mix_attr == 1
              // log strange situation
              std::stringstream ss;
              ss << "own transfer tid=" << tid << " tx=" << td.tx_hash() << " mix_attr=" << td.mix_attr() << ", is referenced by a transaction with mixins, ref from input with amount: " << intk.amount << ", gindex: " << gindex;
              WLT_LOG_YELLOW(ss.str(), LOG_LEVEL_0);
              if (m_wcallback)
                m_wcallback->on_message(i_wallet2_callback::ms_yellow, ss.str());
              continue;
            }
            WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(!td.is_spent(), "transfer is spent, tid: " << tid << ", ref from input with amount: " << intk.amount << ", gindex: " << gindex);
            // own output is spent, handle it
            break;
          }
        }
      }
      else
      {
        // wallet with spend secret key -- we can calculate own key images and then search by them
        auto it = m_key_images.find(intk.k_image);
        if (it != m_key_images.end())
        {
          tid = it->second;
        }
      }

      if (tid != UINT64_MAX)
      {
        tx_money_spent_in_ins += intk.amount;
        transfer_details& td = m_transfers[tid];
        uint32_t flags_before = td.m_flags;
        td.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;
        td.m_spent_height = height;
        if (coin_base_tx && td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER)
          is_derived_from_coinbase = true;
        else
          is_derived_from_coinbase = false;
        WLT_LOG_L0("Spent key out, transfer #" << tid << ", amount: " << print_money(td.amount()) << ", with tx: " << get_transaction_hash(tx) << ", at height " << height <<
          "; flags: " << flags_before << " -> " << td.m_flags);
        mtd.spent_indices.push_back(i);
        remove_transfer_from_expiration_list(tid);
      }
    }
    else if (in.type() == typeid(currency::txin_multisig))
    {
      crypto::hash multisig_id = boost::get<currency::txin_multisig>(in).multisig_out_id;
      auto it = m_multisig_transfers.find(multisig_id);
      if (it != m_multisig_transfers.end())
      {
        it->second.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;
        it->second.m_spent_height = height;
        WLT_LOG_L0("Spent multisig out: " << multisig_id << ", amount: " << print_money(currency::get_amount_from_variant(in)) << ", with tx: " << get_transaction_hash(tx) << ", at height " << height);
        mtd.spent_indices.push_back(i);
      }
    }
    i++;
  }

  /*
  collect unlock_time from every output that transfered coins to this account and use maximum of
  all values m_payments entry, use this strict policy is required to protect exchanges from being feeded with
  useless outputs
  */
  uint64_t max_out_unlock_time = 0;

  //check for transaction income
  crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
  r = lookup_acc_outs(m_account.get_keys(), tx, tx_pub_key, outs, tx_money_got_in_outs, derivation);
  THROW_IF_TRUE_WALLET_EX(!r, error::acc_outs_lookup_error, tx, tx_pub_key, m_account.get_keys());

  if(!outs.empty() /*&& tx_money_got_in_outs*/)
  {
    //good news - got money! take care about it
    //usually we have only one transfer for user in transaction

    //create once instance of tx for all entries
    std::shared_ptr<transaction_wallet_info> pwallet_info(new transaction_wallet_info());
    pwallet_info->m_tx = tx;
    pwallet_info->m_block_height = height;
    pwallet_info->m_block_timestamp = b.timestamp;

    if (is_auditable())
    {      
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(pglobal_indexes && pglobal_indexes->size() == tx.vout.size(), "wrong pglobal_indexes = " << pglobal_indexes << "");
    }
    std::vector<uint64_t> outputs_index_local;

    if (!pglobal_indexes)
    {
      if (!m_use_deffered_global_outputs)
      {
        fetch_tx_global_indixes(tx, outputs_index_local);
        pglobal_indexes = &outputs_index_local;
      }
    }
   
    for (size_t i_in_outs = 0; i_in_outs != outs.size(); i_in_outs++)
    {
      size_t o = outs[i_in_outs];
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(o < tx.vout.size(), "wrong out in transaction: internal index=" << o << ", total_outs=" << tx.vout.size());
      if (tx.vout[o].target.type() == typeid(txout_to_key))
      {
        const currency::txout_to_key& otk = boost::get<currency::txout_to_key>(tx.vout[o].target);

        // obtain key image for this output
        crypto::key_image ki = currency::null_ki;
        if (m_watch_only)
        {
          if (!is_auditable())
          {
            // don't have spend secret key, so we unable to calculate key image for an output
            // look it up in special container instead
            auto it = m_pending_key_images.find(otk.key);
            if (it != m_pending_key_images.end())
            {
              ki = it->second;
              WLT_LOG_L1("pending key image " << ki << " was found by out pub key " << otk.key);
            }
            else
            {
              ki = currency::null_ki;
              WLT_LOG_L1("can't find pending key image by out pub key: " << otk.key << ", key image temporarily set to null");
            }
          }
        }
        else
        {
          // normal wallet, calculate and store key images for own outs
          currency::keypair in_ephemeral;
          currency::generate_key_image_helper(m_account.get_keys(), tx_pub_key, o, in_ephemeral, ki);
          WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(in_ephemeral.pub == otk.key, "key_image generated ephemeral public key that does not match with output_key");
        }
        
        if (ki != currency::null_ki)
        {
          // make sure calculated key image for this own output has not been seen before
          auto it = m_key_images.find(ki);
          if (it != m_key_images.end())
          {
            WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it->second < m_transfers.size(), "m_key_images entry has wrong m_transfers index, it->second: " << it->second << ", m_transfers.size(): " << m_transfers.size());
            const transfer_details& local_td = m_transfers[it->second];
            std::stringstream ss;
            ss << "tx " << get_transaction_hash(tx) << " @ block " << height << " has output #" << o << " with key image " << ki << " that has already been seen in output #" <<
              local_td.m_internal_output_index << " in tx " << get_transaction_hash(local_td.m_ptx_wallet_info->m_tx) << " @ block " << local_td.m_spent_height <<
              ". This output can't ever be spent and will be skipped.";
            WLT_LOG_YELLOW(ss.str(), LOG_LEVEL_0);
            if (m_wcallback)
              m_wcallback->on_message(i_wallet2_callback::ms_yellow, ss.str());
            WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(tx_money_got_in_outs >= tx.vout[o].amount, "tx_money_got_in_outs: " << tx_money_got_in_outs << ", tx.vout[o].amount:" << tx.vout[o].amount);
            tx_money_got_in_outs -= tx.vout[o].amount;
            continue; // skip the output
          }
        }

        if (is_auditable() && otk.mix_attr != CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
        {
          std::stringstream ss;
          ss << "output #" << o << " from tx " << get_transaction_hash(tx) << " with amount " << print_money_brief(tx.vout[o].amount)
            << " is targeted to this auditable wallet and has INCORRECT mix_attr = " << (uint64_t)otk.mix_attr << ". Output IGNORED.";
          WLT_LOG_RED(ss.str(), LOG_LEVEL_0);
          if (m_wcallback)
            m_wcallback->on_message(i_wallet2_callback::ms_red, ss.str());
          tx_money_got_in_outs -= tx.vout[o].amount;
          continue; // skip the output
        }

        mtd.receive_indices.push_back(o);

        m_transfers.push_back(boost::value_initialized<transfer_details>());
        transfer_details& td = m_transfers.back();
        td.m_ptx_wallet_info = pwallet_info;
        td.m_internal_output_index = o;
        td.m_key_image = ki;
        if (m_use_deffered_global_outputs)
        {
          if (pglobal_indexes && pglobal_indexes->size() > o)
            td.m_global_output_index = (*pglobal_indexes)[o];
          else
            td.m_global_output_index = WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED;
        }
        else
        {
          WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(pglobal_indexes, "pglobal_indexes IS NULL in non mobile wallet");
          td.m_global_output_index = (*pglobal_indexes)[o];
        }
        if (coin_base_tx)
        {
          //last out in coinbase tx supposed to be change from coinstake
          if (!(o == tx.vout.size() - 1 && !is_derived_from_coinbase))
          {
            td.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER;
          }
        }
        size_t transfer_index = m_transfers.size()-1;
        if (td.m_key_image != currency::null_ki)
          m_key_images[td.m_key_image] = transfer_index;
        add_transfer_to_transfers_cache(tx.vout[o].amount, transfer_index);
        uint64_t amount = tx.vout[o].amount;

        if (is_watch_only() && is_auditable())
        {
          WLT_CHECK_AND_ASSERT_MES_NO_RET(td.m_global_output_index != WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED, "td.m_global_output_index != WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED validation failed");
          auto amount_gindex_pair = std::make_pair(amount, td.m_global_output_index);
          WLT_CHECK_AND_ASSERT_MES_NO_RET(m_amount_gindex_to_transfer_id.count(amount_gindex_pair) == 0, "update m_amount_gindex_to_transfer_id: amount " << amount << ", gindex " << td.m_global_output_index << " already exists");
          m_amount_gindex_to_transfer_id[amount_gindex_pair] = transfer_index;
        }

        if (max_out_unlock_time < get_tx_unlock_time(tx, o))
          max_out_unlock_time = get_tx_unlock_time(tx, o);

        WLT_LOG_L0("Received money, transfer #" << transfer_index << ", amount: " << print_money(td.amount()) << ", with tx: " << get_transaction_hash(tx) << ", at height " << height);
      }
      else if (tx.vout[o].target.type() == typeid(txout_multisig))
      {
        crypto::hash multisig_id = currency::get_multisig_out_id(tx, o);
        WLT_CHECK_AND_ASSERT_MES_NO_RET(m_multisig_transfers.count(multisig_id) == 0, "multisig_id = " << multisig_id << " already in multisig container");
        transfer_details_base& tdb = m_multisig_transfers[multisig_id];
        tdb.m_ptx_wallet_info = pwallet_info;
        tdb.m_internal_output_index = o;
        WLT_LOG_L0("Received multisig, multisig out id: " << multisig_id << ", amount: " << tdb.amount() << ", with tx: " << get_transaction_hash(tx));
      }
    }
  }
  
  std::string payment_id;
  if (tx_money_got_in_outs && get_payment_id_from_tx(tx.attachment, payment_id))
  {
    uint64_t received = (tx_money_spent_in_ins < tx_money_got_in_outs) ? tx_money_got_in_outs - tx_money_spent_in_ins : 0;
    if (0 < received && payment_id.size())
    {
      payment_details payment;
      payment.m_tx_hash      = currency::get_transaction_hash(tx);
      payment.m_amount       = received;
      payment.m_block_height = height;
      payment.m_unlock_time = max_out_unlock_time;
      m_payments.emplace(payment_id, payment);
      WLT_LOG_L2("Payment found, id (hex): " << epee::string_tools::buff_to_hex_nodelimer(payment_id) << ", tx: " << payment.m_tx_hash << ", amount: " << print_money_brief(payment.m_amount));
    }
  }
   

  if (tx_money_spent_in_ins)
  {//this actually is transfer transaction, notify about spend
    if (tx_money_spent_in_ins > tx_money_got_in_outs)
    {//usual transfer 
      handle_money_spent2(b, tx, tx_money_spent_in_ins - (tx_money_got_in_outs+get_tx_fee(tx)), mtd, recipients, recipients_aliases);
    }
    else
    {//strange transfer, seems that in one transaction have transfers from different wallets.
      if (!is_coinbase(tx))
      {
        WLT_LOG_RED("Unusual transaction " << currency::get_transaction_hash(tx) << ", tx_money_spent_in_ins: " << tx_money_spent_in_ins << ", tx_money_got_in_outs: " << tx_money_got_in_outs, LOG_LEVEL_0);
      }
      handle_money_received2(b, tx, (tx_money_got_in_outs - (tx_money_spent_in_ins - get_tx_fee(tx))), mtd);
    }
  }
  else
  {
    if(tx_money_got_in_outs)
      handle_money_received2(b, tx, tx_money_got_in_outs, mtd);
    else if (currency::is_derivation_used_to_encrypt(tx, derivation))
    {
      //transaction doesn't transfer actually money, bud bring some information
      handle_money_received2(b, tx, 0, mtd);
    }
    else if (mtd.spent_indices.size())
    {
      // multisig spend detected
      handle_money_spent2(b, tx, 0, mtd, recipients, recipients_aliases);
    }
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::prepare_wti_decrypted_attachments(wallet_public::wallet_transfer_info& wti, const std::vector<currency::payload_items_v>& decrypted_att)
{
  PROFILE_FUNC("wallet2::prepare_wti_decrypted_attachments");

  if (wti.is_income)
  {
    account_public_address sender_address = AUTO_VAL_INIT(sender_address);
    wti.show_sender = handle_2_alternative_types_in_variant_container<tx_payer, tx_payer_old>(decrypted_att, [&](const tx_payer& p) { sender_address = p.acc_addr; return false; /* <- continue? */ } );
    if (wti.show_sender)
      wti.remote_addresses.push_back(currency::get_account_address_as_str(sender_address));
  }
  else
  {
    if (wti.remote_addresses.empty())
    {
      handle_2_alternative_types_in_variant_container<tx_receiver, tx_receiver_old>(decrypted_att, [&](const tx_receiver& p) {
        std::string addr_str = currency::get_account_address_as_str(p.acc_addr);
        wti.remote_addresses.push_back(addr_str);
        LOG_PRINT_YELLOW("prepare_wti_decrypted_attachments, income=false, wti.amount = " << print_money_brief(wti.amount) << ", rem. addr = " << addr_str, LOG_LEVEL_0);
        return true; // continue iterating through the container
      });
    }
  }

  currency::tx_comment cm;
  if (currency::get_type_in_variant_container(decrypted_att, cm))
    wti.comment = cm.comment;
  currency::get_payment_id_from_tx(decrypted_att, wti.payment_id);
}
//----------------------------------------------------------------------------------------------------
void wallet2::resend_unconfirmed()
{
  COMMAND_RPC_FORCE_RELAY_RAW_TXS::request req = AUTO_VAL_INIT(req);
  COMMAND_RPC_FORCE_RELAY_RAW_TXS::response res = AUTO_VAL_INIT(res);

  for (auto& ut : m_unconfirmed_txs)
  {
    req.txs_as_hex.push_back(epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(ut.second.tx)));
    WLT_LOG_GREEN("Relaying tx: " << ut.second.tx_hash, LOG_LEVEL_0);
  }

  if (!req.txs_as_hex.size())
    return;
  
  bool r = m_core_proxy->call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(req, res);
  WLT_CHECK_AND_ASSERT_MES(r, void(), "wrong result at call_COMMAND_RPC_FORCE_RELAY_RAW_TXS");
  WLT_CHECK_AND_ASSERT_MES(res.status == API_RETURN_CODE_OK, void(), "wrong result at call_COMMAND_RPC_FORCE_RELAY_RAW_TXS: status != OK, status=" << res.status);		

  WLT_LOG_GREEN("Relayed " << req.txs_as_hex.size() << " txs", LOG_LEVEL_0);
}
//-----------------------------------------------------------------------------------------------------
void wallet2::accept_proposal(const crypto::hash& contract_id, uint64_t b_acceptance_fee, currency::transaction* p_acceptance_tx /* = nullptr */)
{
  auto contr_it = m_contracts.find(contract_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it != m_contracts.end(), "Unknow contract id: " << contract_id);

  THROW_IF_FALSE_WALLET_INT_ERR_EX(!contr_it->second.is_a, "contr_it->second.is_a supposed to be false, but it is " << contr_it->second.is_a);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it->second.state == tools::wallet_public::escrow_contract_details_basic::proposal_sent, "contr_it->second.state supposed to be proposal_sent(" << tools::wallet_public::escrow_contract_details_basic::proposal_sent << ") but it is: " << tools::wallet_public::get_escrow_contract_state_name(contr_it->second.state));

  construct_tx_param construct_param = AUTO_VAL_INIT(construct_param);
  construct_param.fee = b_acceptance_fee;
  currency::transaction tx = contr_it->second.proposal.tx_template;
  crypto::secret_key one_time_key = contr_it->second.proposal.tx_onetime_secret_key;
  construct_param.crypt_address = m_account.get_public_address();
  construct_param.flags = TX_FLAG_SIGNATURE_MODE_SEPARATE;
  construct_param.mark_tx_as_complete = true;
  construct_param.split_strategy_id = detail::ssi_digit;

  //little hack for now, we add multisig_entry before transaction actually get to blockchain
  //to let prepare_transaction (which is called from build_escrow_release_templates) work correct 
  //this code definitely need to be rewritten later (very bad design)
  size_t n = get_multisig_out_index(tx.vout);
  THROW_IF_FALSE_WALLET_EX(n != tx.vout.size(), error::wallet_internal_error, "Multisig out not found in tx template in proposal");

  transfer_details_base& tdb = m_multisig_transfers[contract_id];
  //create once instance of tx for all entries
  std::shared_ptr<transaction_wallet_info> pwallet_info(new transaction_wallet_info());
  pwallet_info->m_tx = tx;;
  pwallet_info->m_block_height = 0;
  pwallet_info->m_block_timestamp = 0;
  tdb.m_ptx_wallet_info = pwallet_info;
  tdb.m_internal_output_index = n;
  tdb.m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_SPENT);
  //---------------------------------
  //figure out fee that was left for release contract 
  THROW_IF_FALSE_WALLET_INT_ERR_EX(tx.vout[n].amount > (contr_it->second.private_detailes.amount_to_pay +
    contr_it->second.private_detailes.amount_b_pledge +
    contr_it->second.private_detailes.amount_a_pledge), "THere is no left money for fee, contract_id: " << contract_id);
  uint64_t left_for_fee_in_multisig = tx.vout[n].amount - (contr_it->second.private_detailes.amount_to_pay +
    contr_it->second.private_detailes.amount_b_pledge +
    contr_it->second.private_detailes.amount_a_pledge);

  //prepare templates to let buyer release or burn escrow
  bc_services::escrow_relese_templates_body artb = AUTO_VAL_INIT(artb);
  build_escrow_release_templates(contract_id, 
    left_for_fee_in_multisig,
    artb.tx_normal_template,
    artb.tx_burn_template, 
    contr_it->second.private_detailes);


  //---second part of bad design ---
  auto it = m_multisig_transfers.find(contract_id);
  THROW_IF_FALSE_WALLET_EX(it != m_multisig_transfers.end(), error::wallet_internal_error, "Internal error");
  m_multisig_transfers.erase(it);
  //---------------------------------

  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  bc_services::pack_attachment_as_gzipped_bin(artb, tsa);
  tsa.service_id = BC_ESCROW_SERVICE_ID;
  tsa.instruction = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_TEMPLATES;
  tsa.flags |= TX_SERVICE_ATTACHMENT_ENCRYPT_BODY;
  construct_param.extra.push_back(tsa);

  //build transaction
  finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  prepare_transaction(construct_param, ftp, tx);
  mark_transfers_as_spent(ftp.selected_transfers, std::string("contract <") + epee::string_tools::pod_to_hex(contract_id) + "> has been accepted with tx <" + epee::string_tools::pod_to_hex(get_transaction_hash(tx)) + ">");

  try
  {
    finalize_transaction(ftp, tx, one_time_key, true);
  }
  catch (...)
  {
    clear_transfers_from_flag(ftp.selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_SPENT, std::string("exception in finalize_transaction, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(tx)));
    throw;
  }

  print_tx_sent_message(tx, "(contract <" + epee::string_tools::pod_to_hex(contract_id) + ">)", construct_param.fee);

  if (p_acceptance_tx != nullptr)
    *p_acceptance_tx = tx;
}
//---------------------------
void wallet2::finish_contract(const crypto::hash& contract_id, const std::string& release_type, currency::transaction* p_release_tx /* = nullptr */)
{
  auto contr_it = m_contracts.find(contract_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it != m_contracts.end(), "Unknow contract id: " << contract_id);

  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it->second.is_a, "contr_it->second.is_a is supposed to be true, but it is " << contr_it->second.is_a);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it->second.state == tools::wallet_public::escrow_contract_details_basic::contract_accepted
    || contr_it->second.state == tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent,
    "incorrect contract state at finish_contract(): (" << contr_it->second.state << "), expected states: contract_accepted (" << tools::wallet_public::escrow_contract_details_basic::contract_accepted << "), " <<
    "contract_cancel_proposal_sent (" << tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent << ")");

  auto multisig_it = m_multisig_transfers.find(contract_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(multisig_it != m_multisig_transfers.end(), "Unknow multisig id: " << contract_id);

  transaction tx = AUTO_VAL_INIT(tx);
  if (release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL)
  {
    tx = contr_it->second.release_body.tx_normal_template;
  }
  else if (release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN)
  {
    tx = contr_it->second.release_body.tx_burn_template;
  }
  else
  {
    THROW_IF_FALSE_WALLET_INT_ERR_EX(false,  "Unknow release_type = " << release_type);
  }

  bool is_input_fully_signed = false;
  bool r = sign_multisig_input_in_tx(tx, 0, m_account.get_keys(), multisig_it->second.m_ptx_wallet_info->m_tx, &is_input_fully_signed);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "sign_multisig_input_in_tx failed");
  // There's a two-party multisig input in tx and B-party had to already sign it upon template creation, so adding A-party sign here should make the tx fully signed. Make sure it does:
  THROW_IF_FALSE_WALLET_INT_ERR_EX(is_input_fully_signed, "sign_multisig_input_in_tx returned is_input_fully_signed == false");

  send_transaction_to_network(tx);

  if (p_release_tx != nullptr)
    *p_release_tx = tx;
}
//-----------------------------------------------------------------------------------------------------
void wallet2::accept_cancel_contract(const crypto::hash& contract_id, currency::transaction* p_cancellation_acceptance_tx /* = nullptr */)
{
  TIME_MEASURE_START_MS(timing1);
  auto contr_it = m_contracts.find(contract_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it != m_contracts.end(), "Unknow contract id");
  TIME_MEASURE_FINISH_MS(timing1);

  THROW_IF_FALSE_WALLET_INT_ERR_EX(!contr_it->second.is_a, "contr_it->second.is_a is supposed to be false, but it is " << contr_it->second.is_a);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it->second.state == tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent,
    "incorrect contract state: (" << contr_it->second.state << "), expected state: contract_cancel_proposal_sent (" << tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent << ")");

  TIME_MEASURE_START_MS(timing2);
  auto multisig_it = m_multisig_transfers.find(contract_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(multisig_it != m_multisig_transfers.end(), "Can't find multisig transfer by id: " << contract_id);
  TIME_MEASURE_FINISH_MS(timing2);

  transaction tx = contr_it->second.cancel_body.tx_cancel_template;

  TIME_MEASURE_START_MS(timing3);
  bool is_input_fully_signed = false;
  bool r = sign_multisig_input_in_tx(tx, 0, m_account.get_keys(), multisig_it->second.m_ptx_wallet_info->m_tx, &is_input_fully_signed);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "sign_multisig_input_in_tx failed");
  // There's a two-party multisig input in tx and A-party had to already sign it upon template creation, so adding B-party sign here should make the tx fully signed. Make sure it does:
  THROW_IF_FALSE_WALLET_INT_ERR_EX(is_input_fully_signed, "sign_multisig_input_in_tx returned is_input_fully_signed == false");

  send_transaction_to_network(tx);
  TIME_MEASURE_FINISH_MS(timing3);
  if (timing1 + timing2 + timing1 > 500)
    WLT_LOG_RED("[wallet2::accept_cancel_contract] LOW PERFORMANCE: " << timing1 << "," <<  timing2 << "," << timing1, LOG_LEVEL_0);

  if (p_cancellation_acceptance_tx != nullptr)
    *p_cancellation_acceptance_tx = tx;
}
//-----------------------------------------------------------------------------------------------------
void wallet2::request_cancel_contract(const crypto::hash& contract_id, uint64_t fee, uint64_t expiration_period, currency::transaction* p_cancellation_proposal_tx /* = nullptr */)
{
  auto contr_it = m_contracts.find(contract_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it != m_contracts.end(), "Unknow contract id: " << contract_id);

  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it->second.is_a, "contr_it->second.is_a supposed to be true at request_cancel_contract");
  THROW_IF_FALSE_WALLET_INT_ERR_EX(contr_it->second.state == tools::wallet_public::escrow_contract_details_basic::contract_accepted 
    || contr_it->second.state == tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent,
    "incorrect contract state at request_cancel_contract(): " << tools::wallet_public::get_escrow_contract_state_name(contr_it->second.state) << ", expected states: contract_accepted (" << tools::wallet_public::escrow_contract_details_basic::contract_accepted << "), " <<
    "contract_cancel_proposal_sent (" << tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent << ")");

  auto multisig_it = m_multisig_transfers.find(contract_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(multisig_it != m_multisig_transfers.end(), "Unknow multisig id: " << contract_id);


  //////////////////////////////////////////////////////////////////////////
  construct_tx_param construct_param = AUTO_VAL_INIT(construct_param);
  construct_param.fee = fee;

  //-------
  //prepare templates to let seller cancel escrow
  bc_services::escrow_cancel_templates_body ectb = AUTO_VAL_INIT(ectb);
  build_escrow_cancel_template(contract_id,
    expiration_period,
    ectb.tx_cancel_template,
    contr_it->second.private_detailes);

  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  bc_services::pack_attachment_as_gzipped_bin(ectb, tsa);
  tsa.service_id = BC_ESCROW_SERVICE_ID;
  tsa.instruction = BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL;
  tsa.flags |= TX_SERVICE_ATTACHMENT_ENCRYPT_BODY;
  construct_param.extra.push_back(tsa);
  construct_param.crypt_address = contr_it->second.private_detailes.b_addr;
  construct_param.split_strategy_id = detail::ssi_digit;

  finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  prepare_transaction(construct_param, ftp);
  currency::transaction tx = AUTO_VAL_INIT(tx);
  crypto::secret_key sk = AUTO_VAL_INIT(sk);
  mark_transfers_as_spent(ftp.selected_transfers, std::string("contract <") + epee::string_tools::pod_to_hex(contract_id) + "> has been requested for cancellaton with tx <" + epee::string_tools::pod_to_hex(get_transaction_hash(tx)) + ">");

  try
  {
    finalize_transaction(ftp, tx, sk, true);
  }
  catch (...)
  {
    clear_transfers_from_flag(ftp.selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_SPENT, std::string("exception in finalize_transaction, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(tx)));
    throw;
  }

  print_tx_sent_message(tx, "(transport for cancel proposal)", fee);

  if (p_cancellation_proposal_tx != nullptr)
    *p_cancellation_proposal_tx = tx;
}
//-----------------------------------------------------------------------------------------------------
void wallet2::scan_tx_to_key_inputs(std::vector<uint64_t>& found_transfers, const currency::transaction& tx)
{
  for (auto& in : tx.vin)
  {
    if (in.type() == typeid(currency::txin_to_key))
    {
      
      auto it = m_key_images.find(boost::get<currency::txin_to_key>(in).k_image);
      if (it != m_key_images.end())
        found_transfers.push_back(it->second);
    } 
  }
}
//-----------------------------------------------------------------------------------------------------
void wallet2::change_contract_state(wallet_public::escrow_contract_details_basic& contract, uint32_t new_state, const crypto::hash& contract_id, const wallet_public::wallet_transfer_info& wti) const
{
  WLT_LOG_YELLOW("escrow contract STATE CHANGE (" << (contract.is_a ? "A," : "B,") << contract_id << " via tx " << get_transaction_hash(wti.tx) << ", height: " << wti.height << ") : "
    << wallet_public::get_escrow_contract_state_name(contract.state) << " -> " << wallet_public::get_escrow_contract_state_name(new_state), LOG_LEVEL_1);
  
  contract.state = new_state;
  contract.height = wti.height; // update height of last state change
}
//-----------------------------------------------------------------------------------------------------
void wallet2::change_contract_state(wallet_public::escrow_contract_details_basic& contract, uint32_t new_state, const crypto::hash& contract_id, const std::string& reason /*= "internal intention"*/) const
{
  WLT_LOG_YELLOW("escrow contract STATE CHANGE (" << (contract.is_a ? "A," : "B,") << contract_id << " " << reason << ") : "
    << wallet_public::get_escrow_contract_state_name(contract.state) << " -> " << wallet_public::get_escrow_contract_state_name(new_state), LOG_LEVEL_1);
  
  contract.state = new_state;
}
//-----------------------------------------------------------------------------------------------------
bool wallet2::handle_proposal(wallet_public::wallet_transfer_info& wti, const bc_services::proposal_body& prop)
{
  PROFILE_FUNC("wallet2::handle_proposal");
  crypto::hash ms_id = AUTO_VAL_INIT(ms_id);
  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  std::vector<payload_items_v> decrypted_items;
  if (!validate_escrow_proposal(wti, prop, decrypted_items, ms_id, cpd))
    return false;

  wallet_public::escrow_contract_details_basic& ed = epee::misc_utils::get_or_insert_value_initialized(m_contracts, ms_id);
  ed.expiration_time = currency::get_tx_expiration_time(prop.tx_template);
  ed.timestamp = wti.timestamp;
  ed.is_a = cpd.a_addr.spend_public_key == m_account.get_keys().account_address.spend_public_key;
  change_contract_state(ed, wallet_public::escrow_contract_details_basic::proposal_sent, ms_id, wti);
  ed.private_detailes = cpd;
  currency::get_payment_id_from_tx(decrypted_items, ed.payment_id);
  ed.proposal = prop;
  ed.height = wti.height;
  wti.contract.resize(1);
  static_cast<wallet_public::escrow_contract_details_basic&>(wti.contract.back()) = ed;
  wti.contract.back().contract_id = ms_id;
  
  //correct fee in case if it "B", cz fee is paid by "A"
  if (!ed.is_a)
    wti.fee = 0;
  else
  {
    //if it's A' then mark proposal template's inputs as spent and add to expiration list
    std::vector<uint64_t> found_transfers;
    scan_tx_to_key_inputs(found_transfers, prop.tx_template);
    //scan outputs to figure out amount of change in escrow
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    std::vector<size_t> outs;
    uint64_t tx_money_got_in_outs = 0;
    bool r = lookup_acc_outs(m_account.get_keys(), prop.tx_template, outs, tx_money_got_in_outs, derivation);
    THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "Failed to lookup_acc_outs for tx: " << get_transaction_hash(prop.tx_template));

    add_transfers_to_expiration_list(found_transfers, ed.expiration_time, tx_money_got_in_outs, wti.tx_hash);
    WLT_LOG_GREEN("Locked " << found_transfers.size() << " transfers due to proposal " << ms_id, LOG_LEVEL_0);
  }


  return true;
}
//-----------------------------------------------------------------------------------------------------
bool wallet2::handle_release_contract(wallet_public::wallet_transfer_info& wti, const std::string& release_instruction)
{
  PROFILE_FUNC("wallet2::handle_release_contract");
  size_t n = get_multisig_in_index(wti.tx.vin);
  WLT_CHECK_AND_ASSERT_MES(n != wti.tx.vin.size(), false, "Multisig out not found in tx template in proposal");
  crypto::hash ms_id = boost::get<txin_multisig>(wti.tx.vin[n]).multisig_out_id;
  WLT_CHECK_AND_ASSERT_MES(ms_id != null_hash, false, "Multisig out not found in tx template in proposal");
  auto it = m_contracts.find(ms_id);
  WLT_CHECK_AND_ASSERT_MES(it != m_contracts.end(), false, "Multisig out not found in tx template in proposal");

  if (release_instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL)
    change_contract_state(it->second, wallet_public::escrow_contract_details_basic::contract_released_normal, ms_id, wti);
  else if (release_instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL)
    change_contract_state(it->second, wallet_public::escrow_contract_details_basic::contract_released_cancelled, ms_id, wti);
  else if (release_instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN)
  {
    change_contract_state(it->second, wallet_public::escrow_contract_details_basic::contract_released_burned, ms_id, wti);
    wti.amount = it->second.private_detailes.amount_to_pay + it->second.private_detailes.amount_a_pledge + it->second.private_detailes.amount_b_pledge;
    if (!it->second.is_a)
    {
      wti.fee = currency::get_tx_fee(wti.tx);
    }
    else
    {
      wti.fee = 0;
    }
  }
  else
  {
    WLT_LOG_ERROR("wrong release_instruction: " << release_instruction);
    return false;
  }

  wti.contract.resize(1);
  static_cast<wallet_public::escrow_contract_details_basic&>(wti.contract.back()) = it->second;
  wti.contract.back().contract_id = ms_id;

  //if it's A(buyer) then fee paid by B(seller)
  if (it->second.is_a)
  {
    wti.fee = 0;
  }
  return true;
}

//-----------------------------------------------------------------------------------------------------
bool wallet2::handle_contract(wallet_public::wallet_transfer_info& wti, const bc_services::contract_private_details& cntr, const std::vector<currency::payload_items_v>& decrypted_attach)
{
  PROFILE_FUNC("wallet2::handle_contract");
  bool is_a = cntr.a_addr == m_account.get_public_address();
  crypto::hash ms_id = AUTO_VAL_INIT(ms_id);
  bc_services::escrow_relese_templates_body rel = AUTO_VAL_INIT(rel);
  if (!validate_escrow_contract(wti, cntr, is_a, decrypted_attach, ms_id, rel))
    return false;

  wallet_public::escrow_contract_details_basic& ed = epee::misc_utils::get_or_insert_value_initialized(m_contracts, ms_id);
  ed.is_a = is_a;
  ed.expiration_time = currency::get_tx_expiration_time(wti.tx);
  if (wti.timestamp)
    ed.timestamp = wti.timestamp;
  ed.height = wti.height;
  ed.payment_id = wti.payment_id;
  change_contract_state(ed, wallet_public::escrow_contract_details_basic::contract_accepted, ms_id, wti);
  ed.private_detailes = cntr;
  ed.release_body = rel;

  wti.contract.resize(1);
  static_cast<wallet_public::escrow_contract_details_basic&>(wti.contract.back()) = ed;
  wti.contract.back().contract_id = ms_id;

  //fee workaround: in consolidating transactions impossible no figure out which part of participants paid fee for tx, so we correct it 
  //in code which know escrow protocol, and we know that fee paid by B(seller)
  if (ed.is_a)
  {
    wti.amount += wti.fee;
    wti.fee = 0;
  }


  return true;
}
//-----------------------------------------------------------------------------------------------------
bool wallet2::handle_cancel_proposal(wallet_public::wallet_transfer_info& wti, const bc_services::escrow_cancel_templates_body& ectb, const std::vector<currency::payload_items_v>& decrypted_attach)
{
  PROFILE_FUNC("wallet2::handle_cancel_proposal");
  //validate cancel proposal 
  WLT_CHECK_AND_ASSERT_MES(ectb.tx_cancel_template.vin.size() && ectb.tx_cancel_template.vin[0].type() == typeid(currency::txin_multisig), false, "Wrong cancel ecrow proposal");
  crypto::hash contract_id = boost::get<currency::txin_multisig>(ectb.tx_cancel_template.vin[0]).multisig_out_id;
  auto it = m_contracts.find(contract_id);
  WLT_CHECK_AND_ASSERT_MES(it != m_contracts.end(), false, "Multisig out not found in tx template in proposal");

  bool r = validate_escrow_cancel_proposal(wti, ectb, decrypted_attach, contract_id, it->second.private_detailes, it->second.proposal.tx_template);
  WLT_CHECK_AND_ASSERT_MES(r, false, "failed to validate escrow cancel request, contract id: " << contract_id);

  uint32_t contract_state = it->second.state;
  switch (contract_state)
  {
  case wallet_public::escrow_contract_details::contract_accepted:
    change_contract_state(it->second, wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent, contract_id, wti); BOOST_FALLTHROUGH;
    // pass through
  case wallet_public::escrow_contract_details::contract_cancel_proposal_sent: // update contract info even if already in that state
    it->second.cancel_body.tx_cancel_template = ectb.tx_cancel_template;
    it->second.cancel_expiration_time = currency::get_tx_expiration_time(ectb.tx_cancel_template);
    //update wti info to let GUI know
    wti.contract.resize(1);
    static_cast<wallet_public::escrow_contract_details_basic&>(wti.contract.back()) = it->second;
    wti.contract.back().contract_id = contract_id;
    return true;
  default:
    WLT_LOG_RED("handle_cancel_proposal for contract (" << (it->second.is_a ? "A," : "B,") << contract_id << " via tx " << get_transaction_hash(wti.tx) << ", height: " << wti.height << ") : " << ENDL <<
      "incorrect state " << wallet_public::get_escrow_contract_state_name(it->second.state) << ", while 'contract_accepted' or 'contract_cancel_proposal_sent' was expected -- decline cancel proposal", LOG_LEVEL_1);
  }
  
  return false;
}
//-----------------------------------------------------------------------------------------------------
bool wallet2::process_contract_info(wallet_public::wallet_transfer_info& wti, const std::vector<currency::payload_items_v>& decrypted_attach)
{
  PROFILE_FUNC("wallet2::process_contract_info");
  for (const auto& v : decrypted_attach)
  {
    if (v.type() == typeid(tx_service_attachment))
    {
      const tx_service_attachment& sa = boost::get<tx_service_attachment>(v);
      if (sa.service_id == BC_ESCROW_SERVICE_ID)
      {
        if (sa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_PROPOSAL)
        {
          bc_services::proposal_body prop = AUTO_VAL_INIT(prop);
          if (!t_unserializable_object_from_blob(prop, sa.body))
          {
            WLT_LOG_ERROR("Failed to unpack attachment for tx: " << wti.tx_hash);
            return false;
          }
          handle_proposal(wti, prop);
        }
        else if (sa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_PRIVATE_DETAILS)
        {
          //means some related contract appeared in blockchain
          bc_services::contract_private_details cntr = AUTO_VAL_INIT(cntr);
          if (!epee::serialization::load_t_from_json(cntr, sa.body))
          {
            WLT_LOG_ERROR("Failed to unpack attachment for tx: " << wti.tx_hash);
            return false;
          }
          handle_contract(wti, cntr, decrypted_attach);

        }
        else if (
          sa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL || 
          sa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL || 
          sa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN 
          )
        {
          handle_release_contract(wti, sa.instruction);
        }
        else if (sa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL)
        {
          //means some related contract appeared in blockchain
          bc_services::escrow_cancel_templates_body cpb = AUTO_VAL_INIT(cpb);
          if (!t_unserializable_object_from_blob(cpb, sa.body))
          {
            WLT_LOG_ERROR("Failed to unpack attachment for tx: " << wti.tx_hash);
            return false;
          }
          handle_cancel_proposal(wti, cpb, decrypted_attach);

        }
      }
    }
  }

  return true;
}
//-----------------------------------------------------------------------------------------------------
void wallet2::prepare_wti(wallet_public::wallet_transfer_info& wti, uint64_t height, uint64_t timestamp, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td)
{
  PROFILE_FUNC("wallet2::prepare_wti");
  wti.tx = tx;
  wti.amount = amount;
  wti.height = height;
  fill_transfer_details(tx, td, wti.td);
  wti.unlock_time = get_max_unlock_time_from_receive_indices(tx, td);
  wti.timestamp = timestamp;
  wti.fee = currency::is_coinbase(tx) ? 0:currency::get_tx_fee(tx);
  wti.tx_blob_size = static_cast<uint32_t>(currency::get_object_blobsize(wti.tx));
  wti.tx_hash = currency::get_transaction_hash(tx);
  wti.is_service = currency::is_service_tx(tx);
  wti.is_mixing = currency::is_mixin_tx(tx);
  wti.is_mining = currency::is_coinbase(tx);
  wti.tx_type = get_tx_type(tx);
  bc_services::extract_market_instructions(wti.srv_attachments, tx.attachment);

  // escrow transactions, which are built with TX_FLAG_SIGNATURE_MODE_SEPARATE flag actually encrypt attachments 
  // with buyer as a sender, and seller as receiver, despite the fact that for both sides transaction seen as outgoing
  // so here to decrypt tx properly we need to figure out, if this transaction is actually escrow acceptance. 
  //we check if spent_indices have zero then input do not belong to this account, which means that we are seller for this 
  //escrow, and decryption should be processed as income flag

  bool decrypt_attachment_as_income = wti.is_income;
  std::vector<currency::payload_items_v> decrypted_att;
  if (wti.tx_type == GUI_TX_TYPE_ESCROW_TRANSFER &&  std::find(td.spent_indices.begin(), td.spent_indices.end(), 0) == td.spent_indices.end())
    decrypt_attachment_as_income = true;


  decrypt_payload_items(decrypt_attachment_as_income, tx, m_account.get_keys(), decrypted_att);
  prepare_wti_decrypted_attachments(wti, decrypted_att);
  process_contract_info(wti, decrypted_att);
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_money_received2(const currency::block& b, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td)
{
  //decrypt attachments
  m_transfer_history.push_back(AUTO_VAL_INIT(wallet_public::wallet_transfer_info()));
  wallet_public::wallet_transfer_info& wti = m_transfer_history.back();
  wti.is_income = true;
  prepare_wti(wti, get_block_height(b), get_actual_timestamp(b), tx, amount, td);

  rise_on_transfer2(wti);
}
//----------------------------------------------------------------------------------------------------
void wallet2::rise_on_transfer2(const wallet_public::wallet_transfer_info& wti)
{
  PROFILE_FUNC("wallet2::rise_on_transfer2");
  if (!m_do_rise_transfer)
    return;
  uint64_t fake = 0;
  uint64_t unlocked_balance = 0;
  uint64_t mined = 0;
  uint64_t balance = this->balance(unlocked_balance, fake, fake, mined);
  m_wcallback->on_transfer2(wti, balance, unlocked_balance, mined);
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_money_spent2(const currency::block& b,
                                  const currency::transaction& in_tx, 
                                  uint64_t amount, 
                                  const money_transfer2_details& td, 
                                  const std::vector<std::string>& recipients, 
                                  const std::vector<std::string>& recipients_aliases)
{
  m_transfer_history.push_back(AUTO_VAL_INIT(wallet_public::wallet_transfer_info()));
  wallet_public::wallet_transfer_info& wti = m_transfer_history.back();
  wti.is_income = false;

  wti.remote_addresses = recipients;
  wti.recipients_aliases = recipients_aliases;
  prepare_wti(wti, get_block_height(b), get_actual_timestamp(b), in_tx, amount, td);
  rise_on_transfer2(wti);
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_unconfirmed(const currency::transaction& tx, std::vector<std::string>& recipients, std::vector<std::string>& recipients_aliases)
{
  auto unconf_it = m_unconfirmed_txs.find(get_transaction_hash(tx));
  if (unconf_it != m_unconfirmed_txs.end())
  {
    wallet_public::wallet_transfer_info& wti = unconf_it->second;
    recipients = wti.remote_addresses;
    recipients_aliases = wti.recipients_aliases;

    m_unconfirmed_txs.erase(unconf_it);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_new_blockchain_entry(const currency::block& b, const currency::block_direct_data_entry& bche, const crypto::hash& bl_id, uint64_t height)
{
  //handle transactions from new block
  THROW_IF_TRUE_WALLET_EX(height != get_blockchain_current_size() &&
    !(height == m_minimum_height || get_blockchain_current_size() <= 1), error::wallet_internal_error,
    "current_index=" + std::to_string(height) + ", get_blockchain_current_height()=" + std::to_string(get_blockchain_current_size()));

  //optimization: seeking only for blocks that are not older then the wallet creation time plus 1 day. 1 day is for possible user incorrect time setup
  const std::vector<uint64_t>* pglobal_index = nullptr;
  if (b.timestamp + 60 * 60 * 24 > m_account.get_createtime())
  {
    pglobal_index = nullptr;
    if (bche.coinbase_ptr.get())
    {
      pglobal_index = &(bche.coinbase_ptr->m_global_output_indexes);
    }
    TIME_MEASURE_START(miner_tx_handle_time);
    process_new_transaction(b.miner_tx, height, b, pglobal_index);
    TIME_MEASURE_FINISH(miner_tx_handle_time);

    TIME_MEASURE_START(txs_handle_time);
    for(const auto& tx_entry: bche.txs_ptr)
    {
      process_new_transaction(tx_entry->tx, height, b, &(tx_entry->m_global_output_indexes));
    }
    TIME_MEASURE_FINISH(txs_handle_time);
    WLT_LOG_L3("Processed block: " << bl_id << ", height " << height << ", " <<  miner_tx_handle_time + txs_handle_time << "(" << miner_tx_handle_time << "/" << txs_handle_time <<")ms");
  }else
  {
    WLT_LOG_L3( "Skipped block by timestamp, height: " << height << ", block time " << b.timestamp << ", account time " << m_account.get_createtime());
  }
  m_chain.push_new_block_id(bl_id, height); //m_blockchain.push_back(bl_id);
  m_last_bc_timestamp = b.timestamp;
  if (!is_pos_block(b))
    m_last_pow_block_h = height;

  m_wcallback->on_new_block(height, b);
}
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// void wallet2::get_short_chain_history(std::list<crypto::hash>& ids)
// {
//   ids.clear();
//   size_t i = 0;
//   size_t current_multiplier = 1;
//   size_t sz = get_blockchain_current_height();
//   if(!sz)
//     return;
//   size_t current_back_offset = 1;
//   bool genesis_included = false;
//   while(current_back_offset < sz)
//   {
//     ids.push_back(m_blockchain[sz-current_back_offset]);
//     if(sz-current_back_offset == 0)
//       genesis_included = true;
//     if(i < 10)
//     {
//       ++current_back_offset;
//     }else
//     {
//       current_back_offset += current_multiplier *= 2;
//     }
//     ++i;
//   }
//   if(!genesis_included)
//     ids.push_back(m_blockchain[0]);
// }

//----------------------------------------------------------------------------------------------------
void wallet2::set_minimum_height(uint64_t h)
{
  m_minimum_height = h;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_wallet_minimum_height()
{
 
  if (m_minimum_height != WALLET_MINIMUM_HEIGHT_UNSET_CONST)
    return m_minimum_height;

  currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response res = AUTO_VAL_INIT(res);
  req.timestamp = m_account.get_createtime();
  bool r = m_core_proxy->call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE(req, res);
  THROW_IF_FALSE_WALLET_EX(r, error::no_connection_to_daemon, "call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE");
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(res.status == API_RETURN_CODE_OK, "FAILED TO CALL COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE");
  m_minimum_height = res.h;
  return res.h;
}
//----------------------------------------------------------------------------------------------------
void wallet2::pull_blocks(size_t& blocks_added, std::atomic<bool>& stop)
{
  blocks_added = 0;
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response res = AUTO_VAL_INIT(res);

  req.minimum_height = get_wallet_minimum_height();
  if (is_auditable())
    req.need_global_indexes = true;

  m_chain.get_short_chain_history(req.block_ids);
  bool r = m_core_proxy->call_COMMAND_RPC_GET_BLOCKS_DIRECT(req, res);
  if (!r)
    throw error::no_connection_to_daemon(LOCATION_STR, "getblocks.bin");
  if (res.status == API_RETURN_CODE_GENESIS_MISMATCH)
  {
    WLT_LOG_MAGENTA("Reseting genesis block...", LOG_LEVEL_0);
    COMMAND_RPC_GET_BLOCKS_DETAILS::request gbd_req = AUTO_VAL_INIT(gbd_req);
    COMMAND_RPC_GET_BLOCKS_DETAILS::response gbd_res = AUTO_VAL_INIT(gbd_res);
    gbd_req.height_start = 0;
    gbd_req.count = 1;
    gbd_req.ignore_transactions = true;
    r = m_core_proxy->call_COMMAND_RPC_GET_BLOCKS_DETAILS(gbd_req, gbd_res);
    THROW_IF_TRUE_WALLET_EX(!r, error::no_connection_to_daemon, "get_blocks_details");
    THROW_IF_TRUE_WALLET_EX(gbd_res.status != API_RETURN_CODE_OK, error::get_blocks_error, gbd_res.status);
    THROW_IF_TRUE_WALLET_EX(gbd_res.blocks.size() == 0, error::get_blocks_error, gbd_res.status);
    crypto::hash new_genesis_id = null_hash;
    r = string_tools::parse_tpod_from_hex_string(gbd_res.blocks.back().id, new_genesis_id);
    THROW_IF_TRUE_WALLET_EX(!r, error::no_connection_to_daemon, "get_blocks_details");
    reset_all();
    m_chain.set_genesis(new_genesis_id);
    WLT_LOG_MAGENTA("New genesis set for wallet: " << new_genesis_id, LOG_LEVEL_0);
    m_chain.get_short_chain_history(req.block_ids);
    //req.block_ids.push_back(new_genesis_id);
    bool r = m_core_proxy->call_COMMAND_RPC_GET_BLOCKS_DIRECT(req, res);
    THROW_IF_TRUE_WALLET_EX(!r, error::no_connection_to_daemon, "getblocks.bin");
  }
  if (res.status == API_RETURN_CODE_BUSY)
  {
    WLT_LOG_L1("Core is busy, pull cancelled");
    stop = true;
    return;
  }
  THROW_IF_TRUE_WALLET_EX(res.status != API_RETURN_CODE_OK, error::get_blocks_error, res.status);
  THROW_IF_TRUE_WALLET_EX(get_blockchain_current_size() && get_blockchain_current_size() <= res.start_height && res.start_height != m_minimum_height, error::wallet_internal_error,
    "wrong daemon response: m_start_height=" + std::to_string(res.start_height) +
    " not less than local blockchain size=" + std::to_string(get_blockchain_current_size()));

  handle_pulled_blocks(blocks_added, stop, res);
}

//----------------------------------------------------------------------------------------------------
void wallet2::handle_pulled_blocks(size_t& blocks_added, std::atomic<bool>& stop, 
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& res)
{
  size_t current_index = res.start_height;
  bool been_matched_block = false;
  if (res.start_height == 0 && get_blockchain_current_size() == 1 && !res.blocks.empty())
  {
    const currency::block& genesis = res.blocks.front().block_ptr->bl;
    THROW_IF_TRUE_WALLET_EX(get_block_height(genesis) != 0, error::wallet_internal_error, "first block expected to be genesis");
    process_genesis_if_needed(genesis);
    res.blocks.pop_front();
    ++current_index;
    been_matched_block = true;
  }

  uint64_t last_matched_index = 0;
  for(const auto& bl_entry: res.blocks)
  {
    if (stop)
      break;

    const currency::block& bl = bl_entry.block_ptr->bl;
    uint64_t height = get_block_height(bl);
    uint64_t processed_blocks_count = get_blockchain_current_size();

    //TODO: get_block_hash is slow
    crypto::hash bl_id = get_block_hash(bl);

    if (processed_blocks_count != 1 && height > processed_blocks_count)
    {
      if (height != m_minimum_height)
      {
        //internal error: 
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(false,
          "height{" << height << "} > processed_blocks_count{" << processed_blocks_count << "}");
      }
      else
      {
        //possible case, wallet rewound to m_minimum_height
        m_chain.clear();
      }
    }
    else if (height == processed_blocks_count)
    {
      //regular block handling
      //self check
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(been_matched_block,
        "internal error: been_matched_block == false on process_new_blockchain_entry, bl_id" << bl_id << "h=" << height 
         << " (start_height=" + std::to_string(res.start_height) + ")");

      process_new_blockchain_entry(bl, bl_entry, bl_id, current_index);
      ++blocks_added;
    }
    else
    { 
      //checking if we need reorganize (might be just first matched block)
      bool block_found = false;
      bool block_matched = false;
      bool full_reset_needed = false;
      m_chain.check_if_block_matched(height, bl_id, block_found, block_matched, full_reset_needed);
      if (block_found && block_matched)
      {
        //block matched in that number
        last_matched_index = height;
        been_matched_block = true;
        WLT_LOG_L2("Block " << bl_id << " @ " << height << " is already in wallet's blockchain");
      }
      else
      {
        //this should happen ONLY after block been matched, if not then is internal error
        if (full_reset_needed)
        {
          last_matched_index = 0;
          been_matched_block = true;
        }
        else
        {
          WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(been_matched_block,
            "unmatched block while never been mathced block");
        }
        //TODO: take into account date of wallet creation
        //reorganize
        detach_blockchain(last_matched_index+1);
        process_new_blockchain_entry(bl, bl_entry, bl_id, height);
        ++blocks_added;
      }
    }


    ++current_index;
    if (res.current_height > m_height_of_start_sync)
    {
      uint64_t next_percent = (100 * (current_index - m_height_of_start_sync)) / (res.current_height - m_height_of_start_sync);
      if (next_percent != m_last_sync_percent)
      {
        m_wcallback->on_sync_progress(next_percent);
        m_last_sync_percent = next_percent;
      }
    }
  }

  WLT_LOG_L1("[PULL BLOCKS] " << res.start_height << " --> " << get_blockchain_current_size());
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_sync_progress()
{
  return m_last_sync_percent;
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh()
{
  size_t blocks_fetched = 0;
  refresh(blocks_fetched);
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh(size_t & blocks_fetched)
{
  bool received_money = false;
  refresh(blocks_fetched, received_money, m_stop);
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh(std::atomic<bool>& stop)
{
  bool f;
  size_t n;
  refresh(n, f, stop);
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(uint64_t amount, const currency::account_public_address& acc)
{
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  std::vector<tx_destination_entry> dst;
  dst.resize(1);
  dst.back().addr.push_back(acc);
  dst.back().amount = amount;
  transaction result_tx = AUTO_VAL_INIT(result_tx);
  this->transfer(dst, 0, 0, TX_DEFAULT_FEE, extra, attachments, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), result_tx);

}
//----------------------------------------------------------------------------------------------------
void wallet2::reset_creation_time(uint64_t timestamp)
{
  m_account.set_createtime(timestamp);
}
//----------------------------------------------------------------------------------------------------
void wallet2::update_current_tx_limit()
{
  currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_INFO::response res = AUTO_VAL_INIT(res);
  bool r = m_core_proxy->call_COMMAND_RPC_GET_INFO(req, res);
  THROW_IF_TRUE_WALLET_EX(!r, error::no_connection_to_daemon, "getinfo");
  THROW_IF_TRUE_WALLET_EX(res.status == API_RETURN_CODE_BUSY, error::daemon_busy, "getinfo");
  THROW_IF_TRUE_WALLET_EX(res.status != API_RETURN_CODE_OK, error::get_blocks_error, res.status);
  THROW_IF_TRUE_WALLET_EX(res.current_blocks_median < CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE, error::get_blocks_error, "bad median size");
  m_upper_transaction_size_limit = res.current_blocks_median - CURRENCY_COINBASE_BLOB_RESERVED_SIZE;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::has_related_alias_entry_unconfirmed(const currency::transaction& tx)
{
  std::string local_adr = m_account.get_public_address_str();
  tx_extra_info tei = AUTO_VAL_INIT(tei);
  parse_and_validate_tx_extra(tx, tei);
  if (tei.m_alias.m_alias.size())
  {
    //have some check address involved
    if (tei.m_alias.m_address.spend_public_key == m_account.get_keys().account_address.spend_public_key && 
      tei.m_alias.m_address.view_public_key == m_account.get_keys().account_address.view_public_key)
      return true;

    //check if it's update and address before was our address
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req = AUTO_VAL_INIT(req);
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::response res = AUTO_VAL_INIT(res);
    req.alias = tei.m_alias.m_alias;
    m_core_proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req, res);
    if (res.status != API_RETURN_CODE_OK)
      return false;
    if (local_adr == res.alias_details.address)
      return true;

  }
  return false;
}
//----------------------------------------------------------------------------------------------------
void wallet2::scan_tx_pool(bool& has_related_alias_in_unconfirmed)
{
  //get transaction pool content 
  currency::COMMAND_RPC_GET_TX_POOL::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_TX_POOL::response res = AUTO_VAL_INIT(res);
  bool r = m_core_proxy->call_COMMAND_RPC_GET_TX_POOL(req, res);
  if (res.status == API_RETURN_CODE_BUSY)
    throw error::daemon_busy(LOCATION_STR, "get_tx_pool");
  if (!r)
    throw error::no_connection_to_daemon(LOCATION_STR, "get_tx_pool");
  THROW_IF_TRUE_WALLET_EX(res.status != API_RETURN_CODE_OK, error::get_blocks_error, res.status);
  

  //- @#@ ----- debug 
#ifdef _DEBUG
  std::stringstream ss;
  ss << "TXS FROM POOL: " << ENDL;
  for (const auto &tx_blob : res.txs)
  {
    currency::transaction tx;
    bool r = parse_and_validate_tx_from_blob(tx_blob, tx);
    THROW_IF_TRUE_WALLET_EX(!r, error::tx_parse_error, tx_blob);
    crypto::hash tx_hash = currency::get_transaction_hash(tx);

    ss << tx_hash << ENDL;
  }
  ss << "UNCONFIRMED TXS: " << ENDL;
  for (const auto &tx_it : m_unconfirmed_in_transfers)
  {
    ss << tx_it.first << ENDL;
  }
  std::string config_tx = ss.str();
#endif
  //- @#@ ----- debug 

  std::unordered_map<crypto::hash, currency::transaction> unconfirmed_in_transfers_local(std::move(m_unconfirmed_in_transfers));
  std::unordered_map<crypto::hash, std::pair<currency::transaction, money_transfer2_details>> unconfirmed_multisig_transfers_from_tx_pool;

  has_related_alias_in_unconfirmed = false;
  uint64_t tx_expiration_ts_median = res.tx_expiration_ts_median; //get_tx_expiration_median();
  for (const auto &tx_blob : res.txs)
  {
    currency::transaction tx;
    money_transfer2_details td;
    bool r = parse_and_validate_tx_from_blob(tx_blob, tx);
    THROW_IF_TRUE_WALLET_EX(!r, error::tx_parse_error, tx_blob);
    has_related_alias_in_unconfirmed |= has_related_alias_entry_unconfirmed(tx);

    crypto::hash tx_hash = currency::get_transaction_hash(tx);
    auto it = unconfirmed_in_transfers_local.find(tx_hash);
    if (it != unconfirmed_in_transfers_local.end())
    {
      m_unconfirmed_in_transfers.insert(*it);
      continue;
    }

    // read extra
    std::vector<size_t> outs;
    uint64_t tx_money_got_in_outs = 0;
    crypto::public_key tx_pub_key = null_pkey;
    r = parse_and_validate_tx_extra(tx, tx_pub_key);
    THROW_IF_TRUE_WALLET_EX(!r, error::tx_extra_parse_error, tx);
    //check if we have money
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    r = lookup_acc_outs(m_account.get_keys(), tx, tx_pub_key, outs, tx_money_got_in_outs, derivation);
    THROW_IF_TRUE_WALLET_EX(!r, error::acc_outs_lookup_error, tx, tx_pub_key, m_account.get_keys());

    //collect incomes
    td.receive_indices = outs;
    bool new_multisig_spend_detected = false;
    //check if we have spendings
    uint64_t tx_money_spent_in_ins = 0;
    std::list<size_t> spend_transfers;
    // check all outputs for spending (compare key images)
    for (size_t i = 0; i != tx.vin.size(); i++)
    {
      auto& in = tx.vin[i];
      if (in.type() == typeid(currency::txin_to_key))
      {
        auto it = m_key_images.find(boost::get<currency::txin_to_key>(in).k_image);
        if (it != m_key_images.end())
        {
          tx_money_spent_in_ins += boost::get<currency::txin_to_key>(in).amount;
          td.spent_indices.push_back(i);
          spend_transfers.push_back(it->second);
        }
      }
      else if (in.type() == typeid(currency::txin_multisig))
      {
        crypto::hash multisig_id = boost::get<currency::txin_multisig>(in).multisig_out_id;
        auto it = m_multisig_transfers.find(multisig_id);
        if (it != m_multisig_transfers.end())
        {
          td.spent_indices.push_back(i);
          r = unconfirmed_multisig_transfers_from_tx_pool.insert(std::make_pair(multisig_id, std::make_pair(tx, td))).second;
          if (!r)
          {
            WLT_LOG_RED("Warning: Receiving the same multisig out id from tx pool more then once: " << multisig_id, LOG_LEVEL_0);
          }
          if (m_unconfirmed_multisig_transfers.count(multisig_id) == 0)
          {
            // new unconfirmed multisig (see also comments on multisig tranafers handling below)
            uint32_t flags_before = it->second.m_flags;
            it->second.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;      // mark as spent
            it->second.m_spent_height = 0;  // height 0 means unconfirmed
            WLT_LOG_L0("From tx pool got new unconfirmed multisig out with id: " << multisig_id << " tx: " << get_transaction_hash(tx) << " Marked as SPENT, flags: " << flags_before << " -> " << it->second.m_flags);
            new_multisig_spend_detected = true;
          }

        }
      }
    }
    

    if (((!tx_money_spent_in_ins && tx_money_got_in_outs) || (currency::is_derivation_used_to_encrypt(tx, derivation) && !tx_money_spent_in_ins)) && !is_tx_expired(tx, tx_expiration_ts_median))
    {
      m_unconfirmed_in_transfers[tx_hash] = tx;
      if (m_unconfirmed_txs.count(tx_hash))
        continue;

      //prepare notification about pending transaction
      wallet_public::wallet_transfer_info& unconfirmed_wti = misc_utils::get_or_insert_value_initialized(m_unconfirmed_txs, tx_hash);

      unconfirmed_wti.is_income = true;
      prepare_wti(unconfirmed_wti, 0, m_core_runtime_config.get_core_time(), tx, tx_money_got_in_outs, td);
      rise_on_transfer2(unconfirmed_wti);
    }
    else if (tx_money_spent_in_ins || new_multisig_spend_detected)
    {
      m_unconfirmed_in_transfers[tx_hash] = tx;
      if (m_unconfirmed_txs.count(tx_hash))
        continue;
      //outgoing tx that was sent somehow not from this application instance
      uint64_t amount = 0;
      /*if (!new_multisig_spend_detected && tx_money_spent_in_ins < tx_money_got_in_outs+get_tx_fee(tx))
      {
        WLT_LOG_ERROR("Transaction that get more then send: tx_money_spent_in_ins=" << tx_money_spent_in_ins
          << ", tx_money_got_in_outs=" << tx_money_got_in_outs << ", tx_id=" << tx_hash);
      }
      else*/ if (new_multisig_spend_detected)
      {
        amount = 0;
      }
      else
      {
        amount = tx_money_spent_in_ins - (tx_money_got_in_outs + get_tx_fee(tx));
      }

      //prepare notification about pending transaction
      wallet_public::wallet_transfer_info& unconfirmed_wti = misc_utils::get_or_insert_value_initialized(m_unconfirmed_txs, tx_hash);

      unconfirmed_wti.is_income = false;
      prepare_wti(unconfirmed_wti, 0, m_core_runtime_config.get_core_time(), tx, amount, td);
      //mark transfers as spend to get correct balance
      for (auto tr_index : spend_transfers)
      {
        if (tr_index > m_transfers.size())
        {
          WLT_LOG_ERROR("INTERNAL ERROR: tr_index " << tr_index << " more then m_transfers.size()=" << m_transfers.size());
          continue;
        }
        uint32_t flags_before = m_transfers[tr_index].m_flags;
        m_transfers[tr_index].m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;
        WLT_LOG_L1("wallet transfer #" << tr_index << " is marked as spent, flags: " << flags_before << " -> " << m_transfers[tr_index].m_flags << ", reason: UNCONFIRMED tx: " << tx_hash);
        unconfirmed_wti.selected_indicies.push_back(tr_index);
      }
      rise_on_transfer2(unconfirmed_wti);
    }
  }

  // Compare unconfirmed multisigs containers
  // IF     EXISTS IN unconfirmed_multisig_transfers_in_tx_pool AND     EXISTS IN m_unconfirmed_multisig_transfers  =>  already processed, do nothing
  // IF     EXISTS IN unconfirmed_multisig_transfers_in_tx_pool AND NOT EXISTS IN m_unconfirmed_multisig_transfers  =>  new unconfirmed, add to m_, mark as spent and nofity (see code above)
  // IF NOT EXISTS IN unconfirmed_multisig_transfers_in_tx_pool AND     EXISTS IN m_unconfirmed_multisig_transfers  =>  EITHER became confirmed (added to the blockchain) OR removed from the pool for some other reason (clear spent flag if there's spent height == 0, means wasn't added to the blockchain)

  std::unordered_set<crypto::hash> unconfirmed_in_multisig_transfers;
  for(auto& el : m_unconfirmed_in_transfers)
    for(auto &in : el.second.vin)
      if (in.type() == typeid(txin_multisig))
        unconfirmed_in_multisig_transfers.insert(boost::get<txin_multisig>(in).multisig_out_id);

  for (auto& multisig_id : m_unconfirmed_multisig_transfers)
  {
    if (unconfirmed_multisig_transfers_from_tx_pool.count(multisig_id) != 0)
      continue;

    if (unconfirmed_in_multisig_transfers.count(multisig_id) != 0)
      continue;

    // Process unconfirmed tx dissapeared from the pool
    auto it = m_multisig_transfers.find(multisig_id);
    if (it != m_multisig_transfers.end() && it->second.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT)
    {
      if (it->second.m_spent_height == 0)
      {
        // Looks like this tx didn't get into the blockchain, just removed from the pool for some reason.
        // So, clear spent flag.
        uint32_t flags_before = it->second.m_flags;
        it->second.m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_SPENT);
        WLT_LOG_L0("Unconfirmed multisig out with id: " << multisig_id << " was presiously marked as spent and now seems to be removed from the pool, while still not added to the blockchain. Marked as NOT SPENT" << ENDL
          << "ms source tx: " << (it->second.m_ptx_wallet_info != nullptr ? get_transaction_hash(it->second.m_ptx_wallet_info->m_tx) : null_hash) << " flags: " << flags_before << " -> " << it->second.m_flags);
      }
    }
  }

  // Populate updated unconfirmed list of multisign transfers
  m_unconfirmed_multisig_transfers.clear();
  for (auto& p : unconfirmed_multisig_transfers_from_tx_pool)
    m_unconfirmed_multisig_transfers.insert(p.first);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::on_idle()
{
  scan_unconfirmed_outdate_tx();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::scan_unconfirmed_outdate_tx()
{
  uint64_t tx_expiration_ts_median = get_tx_expiration_median();
  uint64_t time_limit = m_core_runtime_config.get_core_time() - CURRENCY_MEMPOOL_TX_LIVETIME;
  for (auto it = m_unconfirmed_txs.begin(); it != m_unconfirmed_txs.end(); )
  {
    bool tx_outdated = it->second.timestamp < time_limit;
    if (tx_outdated || is_tx_expired(it->second.tx, tx_expiration_ts_median))
    {
      WLT_LOG_BLUE("removing unconfirmed tx " << it->second.tx_hash << ", reason: " << (tx_outdated ? "outdated" : "expired") << ", tx_expiration_ts_median=" << tx_expiration_ts_median, LOG_LEVEL_0);
      //lookup all used transfer and update flags
      for (auto i : it->second.selected_indicies)
      {
        if (i >= m_transfers.size())
        {
          WLT_LOG_ERROR("Wrong index '" << i << "' in 'selected_indicies', while m_transfers.size() = " << m_transfers.size());
          continue;
        }
        if (!m_transfers[i].m_spent_height)
        {
          uint32_t flags_before = m_transfers[i].m_flags;
          m_transfers[i].m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_SPENT);
          WLT_LOG_BLUE("mark transfer #" << i << " as unspent, flags: " << flags_before << " -> " << m_transfers[i].m_flags << ", reason: removing unconfirmed tx " << it->second.tx_hash, LOG_LEVEL_0);
        }
      }
      //fire some event
      m_wcallback->on_transfer_canceled(it->second);
      m_unconfirmed_txs.erase(it++);
    }else
      it++;
  }

  //scan marked as spent but don't have height (unconfirmed, and actually not unconfirmed)
  std::unordered_set<crypto::key_image> ki_in_unconfirmed;
  for (auto it = m_unconfirmed_txs.begin(); it != m_unconfirmed_txs.end(); it++)
  {
    if (it->second.is_income)
      continue;

    for (auto& in : it->second.tx.vin)
    {
      if (in.type() == typeid(txin_to_key))
      {
        ki_in_unconfirmed.insert(boost::get<txin_to_key>(in).k_image);
      }
    }
  }

  size_t sz = m_transfers.size();
  for (size_t i = 0; i != sz; i++)
  {
    auto& t = m_transfers[i];

    if (t.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT && !t.m_spent_height && !static_cast<bool>(t.m_flags&WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION))
    {
      //check if there is unconfirmed for this transfer is no longer exist?
      if (!ki_in_unconfirmed.count((t.m_key_image)))
      {
        uint32_t flags_before = t.m_flags;
        t.m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_SPENT);
        WLT_LOG_BLUE("Transfer [" << i << "] marked as unspent, flags: " << flags_before << " -> " << t.m_flags << ", reason: there is no unconfirmed tx relataed to this key image", LOG_LEVEL_0);
      }
    }

  }
  
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh(size_t & blocks_fetched, bool& received_money, std::atomic<bool>& stop)
{
  received_money = false;
  blocks_fetched = 0;
  size_t added_blocks = 0;
  size_t try_count = 0;
  crypto::hash last_tx_hash_id = m_transfers.size() ? get_transaction_hash(m_transfers.back().m_ptx_wallet_info->m_tx) : null_hash;
  m_height_of_start_sync = get_blockchain_current_size();
  m_last_sync_percent = 0;
  while (!stop.load(std::memory_order_relaxed))
  {
    try
    {
      pull_blocks(added_blocks, stop);
      blocks_fetched += added_blocks;
      if(!added_blocks)
        break;
    }
    catch (error::no_connection_to_daemon&)
    {
      blocks_fetched += added_blocks;
      if (++try_count > 3)
        return;
      WLT_LOG_L2("no connection to the daemon, wait and try pull_blocks again (try_count: " << try_count << ", blocks_fetched: " << blocks_fetched << ")");
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    catch (const std::exception& e)
    {
      blocks_fetched += added_blocks;
      WLT_LOG_ERROR("refresh->pull_blocks failed, try_count: " << try_count << ", blocks_fetched: " << blocks_fetched << ", exception: " << e.what());
      return;
    }
  }

  if(last_tx_hash_id != (m_transfers.size() ? get_transaction_hash(m_transfers.back().m_ptx_wallet_info->m_tx) : null_hash))
    received_money = true;

  if (blocks_fetched)
  {
    on_idle();

    uint64_t tx_expiration_ts_median = get_tx_expiration_median();
    handle_expiration_list(tx_expiration_ts_median);
    handle_contract_expirations(tx_expiration_ts_median);

    m_found_free_amounts.clear();
  }
  

  WLT_LOG_L1("Refresh done, blocks received: " << blocks_fetched << ", balance: " << print_money(balance()) << ", unlocked: " << print_money(unlocked_balance()));
}
//----------------------------------------------------------------------------------------------------
bool wallet2::handle_expiration_list(uint64_t tx_expiration_ts_median)
{
  for (auto it = m_money_expirations.begin(); it != m_money_expirations.end(); )
  {
    if (tx_expiration_ts_median > it->expiration_time - TX_EXPIRATION_MEDIAN_SHIFT)
    {
      for (auto tr_ind : it->selected_transfers)
      {
        auto &transfer = m_transfers[tr_ind];
        if (!transfer.m_spent_height)
        {
          // Clear WALLET_TRANSFER_DETAIL_FLAG_BLOCKED and WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION flags only.
          // Note: transfer may still be marked as spent
          uint32_t flags_before = transfer.m_flags;
          transfer.m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_BLOCKED);
          transfer.m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION);
          WLT_LOG_GREEN("Unlocked money from expiration_list: transfer #" << tr_ind << ", flags: " << flags_before << " -> " << transfer.m_flags << ", amount: " << print_money(transfer.amount()) << ", tx: " << 
            (transfer.m_ptx_wallet_info != nullptr ? get_transaction_hash(transfer.m_ptx_wallet_info->m_tx) : null_hash), LOG_LEVEL_0);
        }

      }
      WLT_LOG_GREEN("expiration_list entry removed by median: " << tx_expiration_ts_median << ",  expiration time: " << it->expiration_time << ", related tx: " << it->related_tx_id, LOG_LEVEL_0);
      it = m_money_expirations.erase(it);
    }
    else
    {
      it++;
    }
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_contract_expirations(uint64_t tx_expiration_ts_median)
{
  for (auto& contract : m_contracts)
  {
    switch (contract.second.state)
    {
      case tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent:
        if (is_tx_expired(contract.second.cancel_body.tx_cancel_template, tx_expiration_ts_median))
          change_contract_state(contract.second, tools::wallet_public::escrow_contract_details_basic::contract_accepted, contract.first, "cancel proposal expiration");
        break;
      case tools::wallet_public::escrow_contract_details_basic::contract_released_cancelled:
        if (contract.second.height == 0 && is_tx_expired(contract.second.cancel_body.tx_cancel_template, tx_expiration_ts_median))
          change_contract_state(contract.second, tools::wallet_public::escrow_contract_details_basic::contract_accepted, contract.first, "cancel acceptance expiration");
        break;
    }
  }
}
//----------------------------------------------------------------------------------------------------
bool wallet2::refresh(size_t & blocks_fetched, bool& received_money, bool& ok, std::atomic<bool>& stop)
{
  try
  {
    refresh(blocks_fetched, received_money, stop);
    ok = true;
  }
  catch (...)
  {
    ok = false;
  }
  return ok;
}

//----------------------------------------------------------------------------------------------------
uint64_t wallet2::detach_from_block_ids(uint64_t including_height)
{
  //calculate number of erased blocks
  uint64_t blocks_detached = get_blockchain_current_size() - including_height;
  //id at height should be kept, the rest - erased
  m_chain.detach(including_height);
  return blocks_detached;
}
//----------------------------------------------------------------------------------------------------
void wallet2::remove_transfer_from_amount_gindex_map(uint64_t tid)
{
  for (auto it = m_amount_gindex_to_transfer_id.begin(); it != m_amount_gindex_to_transfer_id.end(); )
  {
    if (it->second == tid)
      it = m_amount_gindex_to_transfer_id.erase(it);
    else
      ++it;
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::detach_blockchain(uint64_t including_height)
{
  WLT_LOG_L0("Detaching blockchain on height " << including_height);
  size_t transfers_detached = 0;

  // rollback incoming transfers from detaching subchain
  {
    auto it = std::find_if(m_transfers.begin(), m_transfers.end(), [&](const transfer_details& td){return td.m_ptx_wallet_info->m_block_height >= including_height; });
    if (it != m_transfers.end())
    {
      size_t i_start = it - m_transfers.begin();

      for (size_t i = i_start; i != m_transfers.size(); i++)
      {
        auto it_ki = m_key_images.find(m_transfers[i].m_key_image);
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it_ki != m_key_images.end(), "key image " << m_transfers[i].m_key_image << " not found");
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(m_transfers[i].m_ptx_wallet_info->m_block_height >= including_height, "transfer #" << i << " block height is less than " << including_height);
        m_key_images.erase(it_ki);
        remove_transfer_from_amount_gindex_map(i);
        ++transfers_detached;
      }
      m_transfers.erase(it, m_transfers.end());
    }
  }
 
  size_t blocks_detached = detach_from_block_ids(including_height);

  //rollback spends
  // do not clear spent flag in spent transfers as corresponding txs are most likely in the pool
  // they will be moved into m_unconfirmed_txs for clearing in future (if tx will expire of removed from pool)
  for (size_t i = 0, sz = m_transfers.size(); i < sz; ++i)
  {
    auto& tr = m_transfers[i];
    if (tr.m_spent_height >= including_height)
    {
      WLT_LOG_BLUE("Transfer [" << i << "] spent height: " << tr.m_spent_height << " -> 0, reason: detaching blockchain", LOG_LEVEL_1);
      tr.m_spent_height = 0;
    }
  }

  //rollback tranfers history
  auto tr_hist_it = m_transfer_history.rend();
  for (auto it = m_transfer_history.rbegin(); it != m_transfer_history.rend(); it++)
  {
    if (it->height < including_height)
      break;
    tr_hist_it = it; // note that tr_hist_it->height >= height
  }
  
  if (tr_hist_it != m_transfer_history.rend())
  {
    auto it_from = --tr_hist_it.base();
    // before removing wti from m_transfer_history put it into m_unconfirmed_txs as txs from detached blocks are most likely be moved into the pool
    for (auto it = it_from; it != m_transfer_history.end(); ++it)
    {
      // skip coinbase txs as they are not expected to go into the pool
      if (is_coinbase(it->tx))
      {
        if (!it->is_mining)
          WLT_LOG_ERROR("is_mining flag is not consistent for tx " << it ->tx_hash);
        continue;
      }

      if (!m_unconfirmed_txs.insert(std::make_pair(it->tx_hash, *it)).second)
      {
        WLT_LOG_ERROR("can't move wti from transfer history to unronfirmed txs because such it is already here, tx hash: " << it->tx_hash);
      }
    }
    m_transfer_history.erase(it_from, m_transfer_history.end());
  }
 
  //rollback payments
  for (auto it = m_payments.begin(); it != m_payments.end(); )
  {
    if(including_height <= it->second.m_block_height)
      it = m_payments.erase(it);
    else
      ++it;
  }

  WLT_LOG_L0("Detached blockchain on height " << including_height << ", transfers detached " << transfers_detached << ", blocks detached " << blocks_detached);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::deinit()
{
  m_wcallback.reset();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::clear()
{
  reset_all();
  //currency::block b;
  //currency::generate_genesis_block(b);
  m_chain.clear();
  //m_blockchain.push_back(get_block_hash(b));
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::reset_all()
{
  //m_blockchain.clear();
  m_chain.clear();
  m_transfers.clear();
  m_amount_gindex_to_transfer_id.clear();
  m_key_images.clear();
  // m_pending_key_images is not cleared intentionally
  m_unconfirmed_in_transfers.clear();
  m_unconfirmed_txs.clear();
  m_unconfirmed_multisig_transfers.clear();
  // m_tx_keys is not cleared intentionally, considered to be safe
  m_multisig_transfers.clear();
  m_payments.clear();
  m_transfer_history.clear();
  //m_account = AUTO_VAL_INIT(m_account);

  //m_local_bc_size = 1; //including genesis
  m_last_bc_timestamp = 0;
  m_height_of_start_sync = 0;
  m_last_sync_percent = 0;
  m_last_pow_block_h = 0;
  m_current_wallet_file_size = 0;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::store_keys(std::string& buff, const std::string& password, bool store_as_watch_only /* = false */)
{
  currency::account_base acc = m_account;
  if (store_as_watch_only)
    acc.make_account_watch_only();

  std::string account_data;
  bool r = epee::serialization::store_t_to_binary(acc, account_data);
  WLT_CHECK_AND_ASSERT_MES(r, false, "failed to serialize wallet keys");

  wallet2::keys_file_data keys_file_data = boost::value_initialized<wallet2::keys_file_data>();

  crypto::chacha8_key key;
  crypto::generate_chacha8_key(password, key);
  std::string cipher;
  cipher.resize(account_data.size());
  keys_file_data.iv = crypto::rand<crypto::chacha8_iv>();
  crypto::chacha8(account_data.data(), account_data.size(), key, keys_file_data.iv, &cipher[0]);
  keys_file_data.account_data = cipher;

  r = ::serialization::dump_binary(keys_file_data, buff);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::backup_keys(const std::string& path)
{
  std::string buff;
  bool r = store_keys(buff, m_password);
  WLT_CHECK_AND_ASSERT_MES(r, false, "Failed to store keys");

  r = file_io_utils::save_string_to_file(path, buff);
  WLT_CHECK_AND_ASSERT_MES(r, false, "Failed to save_string_to_file at store keys");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::reset_password(const std::string& pass)
{
  m_password = pass;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_password_valid(const std::string& pass)
{
  return pass == m_password;
}
//----------------------------------------------------------------------------------------------------
namespace
{
  bool verify_keys(const crypto::secret_key& sec, const crypto::public_key& expected_pub)
  {
    crypto::public_key pub;
    bool r = crypto::secret_key_to_public_key(sec, pub);
    return r && expected_pub == pub;
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::init_log_prefix()
{
  m_log_prefix = m_account.get_public_address_str().substr(0, 6);
}
//----------------------------------------------------------------------------------------------------
void wallet2::load_keys2ki(bool create_if_not_exist, bool& need_to_resync)
{
  m_pending_key_images_file_container.close(); // just in case it was opened
  bool pki_corrupted = false;
  std::string reason;
  bool ok = m_pending_key_images_file_container.open(m_pending_ki_file, create_if_not_exist, &pki_corrupted, &reason);
  THROW_IF_FALSE_WALLET_EX(ok, error::file_not_found, m_log_prefix + ": error opening file " + string_encoding::convert_to_ansii(m_pending_ki_file));
  if (pki_corrupted)
  {
    WLT_LOG_ERROR("file " << string_encoding::convert_to_ansii(m_pending_ki_file) << " is corrupted! " << reason);
  }

  if (m_pending_key_images.size() < m_pending_key_images_file_container.size())
  {
    WLT_LOG_RED("m_pending_key_images size: " << m_pending_key_images.size() << " is LESS than m_pending_key_images_file_container size: " << m_pending_key_images_file_container.size(), LOG_LEVEL_0);
    WLT_LOG_L0("Restoring m_pending_key_images from file container...");
    m_pending_key_images.clear();
    for (size_t i = 0, size = m_pending_key_images_file_container.size(); i < size; ++i)
    {
      out_key_to_ki item = AUTO_VAL_INIT_T(out_key_to_ki);
      ok = m_pending_key_images_file_container.get_item(i, item);
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(ok, "m_pending_key_images_file_container.get_item() failed for index " << i << ", size: " << m_pending_key_images_file_container.size());
      ok = m_pending_key_images.insert(std::make_pair(item.out_key, item.key_image)).second;
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(ok, "m_pending_key_images.insert failed for index " << i << ", size: " << m_pending_key_images_file_container.size());
      WLT_LOG_L2("pending key image restored: (" << item.out_key << ", " << item.key_image << ")");
    }
    WLT_LOG_L0(m_pending_key_images.size() << " elements restored, requesting full wallet resync");
    WLT_LOG_L0("m_pending_key_images size: " << m_pending_key_images.size() << ", m_pending_key_images_file_container size: " << m_pending_key_images_file_container.size());
    need_to_resync = true;
  }
  else if (m_pending_key_images.size() > m_pending_key_images_file_container.size())
  {
    WLT_LOG_RED("m_pending_key_images size: " << m_pending_key_images.size() << " is GREATER than m_pending_key_images_file_container size: " << m_pending_key_images_file_container.size(), LOG_LEVEL_0);
    WLT_LOG_RED("UNRECOVERABLE ERROR, wallet stops", LOG_LEVEL_0);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(false, "UNRECOVERABLE ERROR, wallet stops: m_pending_key_images > m_pending_key_images_file_container" << ENDL << "Missing/wrong " << string_encoding::convert_to_ansii(m_pending_ki_file) << " file?");
  }
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_file_names(const std::wstring& file_path)
{
  m_wallet_file = file_path;

  m_pending_ki_file = string_tools::cut_off_extension(m_wallet_file) + L".outkey2ki";

  // make sure file path is accessible and exists
  boost::filesystem::path pp = boost::filesystem::path(file_path).parent_path();
  if (!pp.empty())
    boost::filesystem::create_directories(pp);

  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::load_keys(const std::string& buff, const std::string& password, uint64_t file_signature)
{
  bool r = false;
  wallet2::keys_file_data kf_data = AUTO_VAL_INIT(kf_data);
  if (file_signature == WALLET_FILE_SIGNATURE_OLD)
  {
    wallet2::keys_file_data_old kf_data_old;
    r = ::serialization::parse_binary(buff, kf_data_old);
    kf_data = wallet2::keys_file_data::from_old(kf_data_old);
  }
  else if (file_signature == WALLET_FILE_SIGNATURE_V2)
  {
    r = ::serialization::parse_binary(buff, kf_data);
  }
  THROW_IF_TRUE_WALLET_EX(!r, error::wallet_internal_error, "internal error: failed to deserialize");

  crypto::chacha8_key key;
  crypto::generate_chacha8_key(password, key);
  std::string account_data;
  account_data.resize(kf_data.account_data.size());
  crypto::chacha8(kf_data.account_data.data(), kf_data.account_data.size(), key, kf_data.iv, &account_data[0]);

  const currency::account_keys& keys = m_account.get_keys();
  r = epee::serialization::load_t_from_binary(m_account, account_data);
  r = r && verify_keys(keys.view_secret_key,  keys.account_address.view_public_key);
  if (keys.spend_secret_key == currency::null_skey)
    m_watch_only = true;
  else
    r = r && verify_keys(keys.spend_secret_key, keys.account_address.spend_public_key);
  if (!r)
  {
    WLT_LOG_L0("Wrong password for wallet " << string_encoding::convert_to_ansii(m_wallet_file));
    tools::error::throw_wallet_ex<error::invalid_password>(std::string(__FILE__ ":" STRINGIZE(__LINE__)));
  }
  init_log_prefix();
}
//----------------------------------------------------------------------------------------------------
void wallet2::assign_account(const currency::account_base& acc)
{
  clear();
  m_account = acc;
  init_log_prefix();
}
//----------------------------------------------------------------------------------------------------
void wallet2::generate(const std::wstring& path, const std::string& pass, bool auditable_wallet)
{
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(validate_password(pass), "new wallet generation failed: password contains forbidden characters")
  clear();
  prepare_file_names(path);

  check_for_free_space_and_throw_if_it_lacks(m_wallet_file);

  m_password = pass;
  m_account.generate(auditable_wallet);
  init_log_prefix();
  boost::system::error_code ignored_ec;
  THROW_IF_TRUE_WALLET_EX(boost::filesystem::exists(m_wallet_file, ignored_ec), error::file_exists, epee::string_encoding::convert_to_ansii(m_wallet_file));
  if (m_watch_only && !auditable_wallet)
  {
    bool stub;
    load_keys2ki(true, stub);
  }
  store();
}
//----------------------------------------------------------------------------------------------------
void wallet2::restore(const std::wstring& path, const std::string& pass, const std::string& seed_phrase_or_awo_blob, bool auditable_watch_only)
{
  bool r = false;
  clear();
  prepare_file_names(path);
  m_password = pass;
  
  if (auditable_watch_only)
  {
    r = m_account.restore_from_awo_blob(seed_phrase_or_awo_blob);
    init_log_prefix();
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "Could not load auditable watch-only wallet from a given blob: invalid awo blob");
    m_watch_only = true;
  }
  else
  {
    r = m_account.restore_from_braindata(seed_phrase_or_awo_blob);
    init_log_prefix();
    THROW_IF_FALSE_WALLET_EX(r, error::wallet_wrong_seed_error, epee::string_encoding::convert_to_ansii(m_wallet_file));
  }

  boost::system::error_code ignored_ec;
  THROW_IF_TRUE_WALLET_EX(boost::filesystem::exists(m_wallet_file, ignored_ec), error::file_exists, epee::string_encoding::convert_to_ansii(m_wallet_file));
  store();
}
//----------------------------------------------------------------------------------------------------
bool wallet2::check_connection()
{
  return m_core_proxy->check_connection();
}
//----------------------------------------------------------------------------------------------------
void wallet2::load(const std::wstring& wallet_, const std::string& password)
{
  clear();
  prepare_file_names(wallet_);

  check_for_free_space_and_throw_if_it_lacks(m_wallet_file);

  m_password = password;

  std::string keys_buff;
  std::string body_buff;


  boost::system::error_code e;
  bool exists = boost::filesystem::exists(m_wallet_file, e);
  THROW_IF_TRUE_WALLET_EX(e || !exists, error::file_not_found, epee::string_encoding::convert_to_ansii(m_wallet_file));
  boost::filesystem::ifstream data_file;
  data_file.open(m_wallet_file, std::ios_base::binary | std::ios_base::in);
  THROW_IF_TRUE_WALLET_EX(data_file.fail(), error::file_read_error, epee::string_encoding::convert_to_ansii(m_wallet_file));

  wallet_file_binary_header wbh = AUTO_VAL_INIT(wbh);

  data_file.read((char*)&wbh, sizeof(wbh));
  THROW_IF_TRUE_WALLET_EX(data_file.fail(), error::file_read_error, epee::string_encoding::convert_to_ansii(m_wallet_file));

  THROW_IF_TRUE_WALLET_EX(wbh.m_signature != WALLET_FILE_SIGNATURE_OLD && wbh.m_signature != WALLET_FILE_SIGNATURE_V2, error::file_read_error, epee::string_encoding::convert_to_ansii(m_wallet_file));
  THROW_IF_TRUE_WALLET_EX(wbh.m_cb_body > WALLET_FILE_MAX_BODY_SIZE || 
    wbh.m_cb_keys > WALLET_FILE_MAX_KEYS_SIZE, error::file_read_error, epee::string_encoding::convert_to_ansii(m_wallet_file));


  keys_buff.resize(wbh.m_cb_keys);
  data_file.read((char*)keys_buff.data(), wbh.m_cb_keys);

  load_keys(keys_buff, password, wbh.m_signature);

  bool need_to_resync = !tools::portable_unserialize_obj_from_stream(*this, data_file);

  if (m_watch_only && !is_auditable())
    load_keys2ki(true, need_to_resync);

  WLT_LOG_L0("Loaded wallet file" << (m_watch_only ? " (WATCH ONLY) " : " ") << string_encoding::convert_to_ansii(m_wallet_file) << " with public address: " << m_account.get_public_address_str());
  WLT_LOG_L0("(after loading: pending_key_images: " << m_pending_key_images.size() << ", pki file elements: " << m_pending_key_images_file_container.size() << ", tx_keys: " << m_tx_keys.size() << ")");

  if (need_to_resync)
  {
    reset_history();
    WLT_LOG_L0("Unable to load history data from wallet file, wallet will be resynced!");
  } 

  boost::system::error_code ec = AUTO_VAL_INIT(ec);
  m_current_wallet_file_size = boost::filesystem::file_size(wallet_, ec);

  THROW_IF_TRUE_WALLET_EX(need_to_resync, error::wallet_load_notice_wallet_restored, epee::string_encoding::convert_to_ansii(m_wallet_file));
}
//----------------------------------------------------------------------------------------------------
void wallet2::store()
{
  store(m_wallet_file, m_password);
}
//----------------------------------------------------------------------------------------------------
void wallet2::store(const std::wstring& path)
{
  store(path, m_password);
}
//----------------------------------------------------------------------------------------------------
void wallet2::store(const std::wstring& path_to_save, const std::string& password)
{
  LOG_PRINT_L0("(before storing: pending_key_images: " << m_pending_key_images.size() << ", pki file elements: " << m_pending_key_images_file_container.size() << ", tx_keys: " << m_tx_keys.size() << ")");

  // check_for_free_space_and_throw_if_it_lacks(path_to_save); temporary disabled, wallet saving implemented in two-stage scheme to avoid data loss due to lack of space

  std::string ascii_path_to_save = epee::string_encoding::convert_to_ansii(path_to_save);

  //prepare data
  std::string keys_buff;
  bool r = store_keys(keys_buff, password, m_watch_only);
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "failed to store_keys for wallet " << ascii_path_to_save);

  //store data
  wallet_file_binary_header wbh = AUTO_VAL_INIT(wbh);
  wbh.m_signature = WALLET_FILE_SIGNATURE_V2;
  wbh.m_cb_keys = keys_buff.size();
  //@#@ change it to proper
  wbh.m_cb_body = 1000;
  std::string header_buff((const char*)&wbh, sizeof(wbh));

  uint64_t ts = m_core_runtime_config.get_core_time();

  // save to tmp file, then rename
  boost::filesystem::path tmp_file_path = boost::filesystem::path(path_to_save);
  tmp_file_path += L".newtmp_" + std::to_wstring(ts);

  boost::filesystem::ofstream data_file;
  data_file.open(tmp_file_path, std::ios_base::binary | std::ios_base::out | std::ios::trunc);
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(!data_file.fail(), "failed to open binary wallet file for saving: " << tmp_file_path.string());
  data_file << header_buff << keys_buff;

  WLT_LOG_L0("Storing to temporary file " << tmp_file_path.string() << " ...");

  r = tools::portble_serialize_obj_to_stream(*this, data_file);
  if (!r)
  {
    data_file.close();
    boost::filesystem::remove(tmp_file_path); // remove tmp file if smth went wrong
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(false, "IO error while storing wallet to " << tmp_file_path.string() << " (portble_serialize_obj_to_stream failed)");
  }

  data_file.flush();
  data_file.close();

  WLT_LOG_L1("Stored successfully to temporary file " << tmp_file_path.string());

  // for the sake of safety perform a double-renaming: wallet file -> old tmp, new tmp -> wallet file, remove old tmp
  
  boost::filesystem::path tmp_old_file_path = boost::filesystem::path(path_to_save);
  tmp_old_file_path += L".oldtmp_" + std::to_wstring(ts);

  if (boost::filesystem::is_regular_file(path_to_save))
  {
    boost::filesystem::rename(path_to_save, tmp_old_file_path);
    WLT_LOG_L1("Renamed: " << ascii_path_to_save << " -> " << tmp_old_file_path.string());
  }
  
  boost::filesystem::rename(tmp_file_path, path_to_save);
  WLT_LOG_L1("Renamed: " << tmp_file_path.string() << " -> " << ascii_path_to_save);

  if (boost::filesystem::remove(tmp_old_file_path))
  {
    WLT_LOG_L1("Removed temporary file: " << tmp_old_file_path.string());
  }

  bool path_to_save_exists       = boost::filesystem::is_regular_file(path_to_save);
  bool tmp_file_path_exists      = boost::filesystem::is_regular_file(tmp_file_path);
  bool tmp_old_file_path_exists  = boost::filesystem::is_regular_file(tmp_old_file_path);
  
  boost::system::error_code ec = AUTO_VAL_INIT(ec);
  m_current_wallet_file_size = boost::filesystem::file_size(path_to_save, ec);
  if (path_to_save_exists && !tmp_file_path_exists && !tmp_old_file_path_exists)
  {
    WLT_LOG_L0("Wallet was successfully stored to " << ascii_path_to_save);
  }
  else
  {
    WLT_LOG_ERROR("Wallet stroing to " << ascii_path_to_save << " might not be successfull: path_to_save_exists=" << path_to_save_exists << ", tmp_file_path_exists=" << tmp_file_path_exists << ", tmp_old_file_path_exists=" << tmp_old_file_path_exists);
    throw tools::error::wallet_common_error(LOCATION_STR, "Wallet file storing might not be successfull. Please make sure you have backed up your seed phrase and check log for details.");
  }
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_wallet_file_size()const
{
  return m_current_wallet_file_size;
}
//----------------------------------------------------------------------------------------------------
void wallet2::store_watch_only(const std::wstring& path_to_save, const std::string& password) const
{
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(path_to_save != m_wallet_file, "trying to save watch-only wallet to the same wallet file!");
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(!m_watch_only, "saving watch-only wallet into a watch-only wallet is not allowed");

  // prepare data for watch-only wallet
  wallet2 wo;
  // wallet2 wo(*this); copy-constructor is not working, so do a this serialization workaround
  std::stringstream stream_buffer;
  tools::portble_serialize_obj_to_stream(*this, stream_buffer);
  tools::portable_unserialize_obj_from_stream(wo, stream_buffer);

  wo.m_watch_only = true;
  wo.m_account = m_account;
  wo.m_account.make_account_watch_only();
  wo.prepare_file_names(path_to_save);

  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(!boost::filesystem::exists(wo.m_wallet_file), "file " << epee::string_encoding::convert_to_ansii(wo.m_wallet_file) << " already exists");
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(!boost::filesystem::exists(wo.m_pending_ki_file), "file " << epee::string_encoding::convert_to_ansii(wo.m_pending_ki_file) << " already exists");

  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(wo.m_pending_key_images.empty(), "pending key images is expected to be empty");
  if (!is_auditable())
  {
    bool stub = false;
    wo.load_keys2ki(true, stub); // to create outkey2ki file
  }
  
  // populate pending key images for spent outputs (this will help to resync watch-only wallet)
  for (size_t ti = 0; ti < wo.m_transfers.size(); ++ti)
  {
    const auto& td = wo.m_transfers[ti];
    if (!td.is_spent())
      continue; // only spent transfers really need to be stored, because watch-only wallet will not be able to figure out they were spent otherwise
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(td.m_internal_output_index < td.m_ptx_wallet_info->m_tx.vout.size(), "invalid transfer #" << ti);
    const currency::txout_target_v& out_t = td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].target;
    if (out_t.type() != typeid(currency::txout_to_key))
      continue;
    const crypto::public_key& out_key = boost::get<txout_to_key>(out_t).key;
    wo.m_pending_key_images.insert(std::make_pair(out_key, td.m_key_image));
    wo.m_pending_key_images_file_container.push_back(tools::out_key_to_ki{ out_key, td.m_key_image });
    WLT_LOG_L1("preparing watch-only wallet: added pending ki (" << out_key << ", " << td.m_key_image << ")");
  }

  // TODO additional clearing for watch-only wallet's data

  wo.store(path_to_save, password);
}
//----------------------------------------------------------------------------------------------------
void wallet2::check_for_free_space_and_throw_if_it_lacks(const std::wstring& wallet_filename, uint64_t exact_size_needed_if_known /* = UINT64_MAX */)
{
  namespace fs = boost::filesystem;

  try
  {
    fs::path wallet_file_path(wallet_filename);
    fs::path base_path = wallet_file_path.parent_path();
    if (base_path.empty())
      base_path = fs::path(".");
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(fs::is_directory(base_path), "directory does not exist: " << base_path.string());

    uint64_t min_free_size = exact_size_needed_if_known;
    if (min_free_size == UINT64_MAX)
    {
      // if exact size needed is unknown -- determine it as
      // twice the original wallet file size or MINIMUM_REQUIRED_WALLET_FREE_SPACE_BYTES, which one is bigger
      min_free_size = MINIMUM_REQUIRED_WALLET_FREE_SPACE_BYTES;
      if (fs::is_regular_file(wallet_file_path))
        min_free_size = std::max(min_free_size, 2 * static_cast<uint64_t>(fs::file_size(wallet_file_path)));
    }
    else
    {
      min_free_size += 1024 * 1024 * 10; // add a little for FS overhead and so
    }

    fs::space_info si = fs::space(base_path);
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(si.available > min_free_size, "free space at " << base_path.string() << " is too low: " << si.available << ", required minimum is: " << min_free_size);
  }
  catch (tools::error::wallet_common_error&)
  {
    throw;
  }
  catch (std::exception& e)
  {
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(false, "failed to determine free space: " << e.what());
  }
  catch (...)
  {
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(false, "failed to determine free space: unknown exception");
  }
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::unlocked_balance() const
{
  uint64_t stub = 0;
  uint64_t unlocked_balance = 0;
  balance(unlocked_balance, stub, stub, stub);
  return unlocked_balance;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance(uint64_t& unloked) const
{
  uint64_t fake = 0;
  return balance(unloked, fake, fake, fake);
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance(uint64_t& unlocked, uint64_t& awaiting_in, uint64_t& awaiting_out, uint64_t& mined) const
{
  unlocked = 0;
  uint64_t balance_total = 0;
  awaiting_in = 0;
  awaiting_out = 0;
  mined = 0;
  
  for(auto& td : m_transfers)
  {
    if (td.is_spendable() || (td.is_reserved_for_escrow() && !td.is_spent()))
    {
      balance_total += td.amount();
      if (is_transfer_unlocked(td))
        unlocked += td.amount();
      if (td.m_flags & WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER)
        mined += td.amount();
    }
  }

  for(auto& utx : m_unconfirmed_txs)
  {
    if (utx.second.is_income)
    {
      balance_total += utx.second.amount;
      awaiting_in += utx.second.amount;
    }
    else
    {
      //collect change in unconfirmed outgoing transactions
      for (auto r : utx.second.td.rcv)
        balance_total += r;

      awaiting_out += utx.second.amount;
    }
  }

  return balance_total;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance() const
{
  uint64_t stub = 0;
  return balance(stub, stub, stub, stub);
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_transfers(wallet2::transfer_container& incoming_transfers) const
{
  incoming_transfers = m_transfers;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::generate_packing_transaction_if_needed(currency::transaction& tx, uint64_t fake_outputs_number)
{
  prepare_free_transfers_cache(0);
  auto it = m_found_free_amounts.find(CURRENCY_BLOCK_REWARD);
  if (it == m_found_free_amounts.end() || it->second.size() <= m_pos_mint_packing_size)
    return false;
  
  //let's check if we have at least WALLET_POS_MINT_PACKING_SIZE transactions which is ready to go
  size_t count = 0;
  for (auto it_ind = it->second.begin(); it_ind != it->second.end() && count <= m_pos_mint_packing_size; it_ind++)
  {
    if (is_transfer_ready_to_go(m_transfers[*it_ind], fake_outputs_number))
      ++count;
  }
  if (count <= m_pos_mint_packing_size)
    return false;
  construct_tx_param ctp = get_default_construct_tx_param();
  currency::tx_destination_entry de = AUTO_VAL_INIT(de);
  de.addr.push_back(m_account.get_public_address());
  de.amount = m_pos_mint_packing_size*CURRENCY_BLOCK_REWARD;
  ctp.dsts.push_back(de);
  ctp.perform_packing = true;
  
  TRY_ENTRY();
  transfer(ctp, tx, false, nullptr);
  CATCH_ENTRY2(false);

  return true;
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::get_transfers_str(bool include_spent /*= true*/, bool include_unspent /*= true*/) const
{
  static const char* header = "index                 amount  g_index  flags       block  tx                                                                  out#  key image";
  std::stringstream ss;
  ss << header << ENDL;
  size_t count = 0;
  for (size_t i = 0; i != m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];

    if ((td.is_spent() && !include_spent) || (!td.is_spent() && !include_unspent))
      continue;

    ss << std::right <<
      std::setw(5) << i << "  " <<
      std::setw(21) << print_money(td.amount()) << "  " <<
      std::setw(7) << td.m_global_output_index << "  " <<
      std::setw(2) << std::setfill('0') << td.m_flags << std::setfill(' ') << ":" <<
      std::setw(5) << transfer_flags_to_str(td.m_flags) << "  " <<
      std::setw(7) << td.m_ptx_wallet_info->m_block_height << "  " <<
      get_transaction_hash(td.m_ptx_wallet_info->m_tx) << "  " <<
      std::setw(4) << td.m_internal_output_index << "  " <<
      td.m_key_image << ENDL;
    
    ++count;
  }

  ss << "printed " << count << " outputs of " << m_transfers.size() << " total" << ENDL;
  return ss.str();
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_payments(const std::string& payment_id, std::list<wallet2::payment_details>& payments, uint64_t min_height) const
{
  auto range = m_payments.equal_range(payment_id);
  std::for_each(range.first, range.second, [&payments, &min_height](const payment_container::value_type& x)
  {
    if (min_height <= x.second.m_block_height)
    {
      payments.push_back(x.second);
    }
  });
}
//----------------------------------------------------------------------------------------------------
void wallet2::sign_transfer(const std::string& tx_sources_blob, std::string& signed_tx_blob, currency::transaction& tx)
{
  // assumed to be called from normal, non-watch-only wallet
  THROW_IF_FALSE_WALLET_EX(!m_watch_only, error::wallet_common_error, "watch-only wallet is unable to sign transfers, you need to use normal wallet for that");

  // decrypt the blob
  std::string decrypted_src_blob = crypto::chacha_crypt(tx_sources_blob, m_account.get_keys().view_secret_key);

  // deserialize args
  finalized_tx ft = AUTO_VAL_INIT(ft);
  bool r = t_unserializable_object_from_blob(ft.ftp, decrypted_src_blob);
  THROW_IF_FALSE_WALLET_EX(r, error::wallet_common_error, "Failed to decrypt tx sources blob");

  // make sure unsigned tx was created with the same keys
  THROW_IF_FALSE_WALLET_EX(ft.ftp.spend_pub_key == m_account.get_keys().account_address.spend_public_key, error::wallet_common_error, "The was created in a different wallet, keys missmatch");

  finalize_transaction(ft.ftp, ft.tx, ft.one_time_key, false);

  // calculate key images for each change output
  crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(
      crypto::generate_key_derivation(
          m_account.get_keys().account_address.view_public_key,
          ft.one_time_key,
          derivation),
      "internal error: sign_transfer: failed to generate key derivation("
          << m_account.get_keys().account_address.view_public_key
          << ", view secret key: " << ft.one_time_key << ")");

  for (size_t i = 0; i < ft.tx.vout.size(); ++i)
  {
    const auto& out = ft.tx.vout[i];
    if (out.target.type() != typeid(txout_to_key))
      continue;
    const txout_to_key& otk = boost::get<txout_to_key>(out.target);

    crypto::public_key ephemeral_pub = AUTO_VAL_INIT(ephemeral_pub);
    if (!crypto::derive_public_key(derivation, i, m_account.get_keys().account_address.spend_public_key, ephemeral_pub))
    {
      WLT_LOG_ERROR("derive_public_key failed for tx " << get_transaction_hash(ft.tx) << ", out # " << i);
    }

    if (otk.key == ephemeral_pub)
    {
      // this is the output to the given keys
      // derive secret key and calculate key image
      crypto::secret_key ephemeral_sec = AUTO_VAL_INIT(ephemeral_sec);
      crypto::derive_secret_key(derivation, i, m_account.get_keys().spend_secret_key, ephemeral_sec);
      crypto::key_image ki = AUTO_VAL_INIT(ki);
      crypto::generate_key_image(ephemeral_pub, ephemeral_sec, ki);

      ft.outs_key_images.push_back(make_serializable_pair(static_cast<uint64_t>(i), ki));
    }
  }

  // serialize and encrypt the result
  signed_tx_blob = t_serializable_object_to_blob(ft);
  crypto::chacha_crypt(signed_tx_blob, m_account.get_keys().view_secret_key);

  tx = ft.tx;
}
//----------------------------------------------------------------------------------------------------
void wallet2::sign_transfer_files(const std::string& tx_sources_file, const std::string& signed_tx_file, currency::transaction& tx)
{
  std::string sources_blob;
  bool r = epee::file_io_utils::load_file_to_string(tx_sources_file, sources_blob);
  THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "failed to open file " << tx_sources_file);

  std::string signed_tx_blob;
  sign_transfer(sources_blob, signed_tx_blob, tx);

  r = epee::file_io_utils::save_string_to_file(signed_tx_file, signed_tx_blob);
  THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "failed to store signed tx to file " << signed_tx_file);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_utxo_distribution(std::map<uint64_t, uint64_t>& distribution)
{
  prepare_free_transfers_cache(0);
  for (auto ent : m_found_free_amounts)
  {
    distribution[ent.first] = ent.second.size();
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::submit_transfer(const std::string& signed_tx_blob, currency::transaction& tx)
{
  // decrypt sources
  std::string decrypted_src_blob = crypto::chacha_crypt(signed_tx_blob, m_account.get_keys().view_secret_key);

  // deserialize tx data
  finalized_tx ft = AUTO_VAL_INIT(ft);
  bool r = t_unserializable_object_from_blob(ft, decrypted_src_blob);
  THROW_IF_FALSE_WALLET_EX(r, error::wallet_common_error, "Failed to decrypt signed tx data");
  tx = ft.tx;
  crypto::hash tx_hash = get_transaction_hash(tx);

  // foolproof
  THROW_IF_FALSE_WALLET_CMN_ERR_EX(ft.ftp.spend_pub_key == m_account.get_keys().account_address.spend_public_key, "The given tx was created in a different wallet, keys missmatch, tx hash: " << tx_hash);

  try
  {
    send_transaction_to_network(tx);
  }
  catch (...)
  {
    // clear transfers flags if smth went wrong
    uint32_t flag = WALLET_TRANSFER_DETAIL_FLAG_SPENT | WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION;
    clear_transfers_from_flag(ft.ftp.selected_transfers, flag, "broadcasting tx " + epee::string_tools::pod_to_hex(tx_hash) + " was unsuccessful");
    throw;
  }

  add_sent_tx_detailed_info(tx, ft.ftp.prepared_destinations, ft.ftp.selected_transfers);
  m_tx_keys.insert(std::make_pair(tx_hash, ft.one_time_key));

  if (m_watch_only)
  {
    std::vector<std::pair<crypto::public_key, crypto::key_image>> pk_ki_to_be_added;
    std::vector<std::pair<uint64_t, crypto::key_image>> tri_ki_to_be_added;

    for (auto& p : ft.outs_key_images)
    {
      THROW_IF_FALSE_WALLET_INT_ERR_EX(p.first < tx.vout.size(), "outs_key_images has invalid out index: " << p.first << ", tx.vout.size() = " << tx.vout.size());
      auto& out = tx.vout[p.first];
      THROW_IF_FALSE_WALLET_INT_ERR_EX(out.target.type() == typeid(txout_to_key), "outs_key_images has invalid out type, index: " << p.first);
      const txout_to_key& otk = boost::get<txout_to_key>(out.target);
      pk_ki_to_be_added.push_back(std::make_pair(otk.key, p.second));
    }

    THROW_IF_FALSE_WALLET_INT_ERR_EX(tx.vin.size() == ft.ftp.sources.size(), "tx.vin and ft.ftp.sources sizes missmatch");
    for (size_t i = 0; i < tx.vin.size(); ++i)
    {
      const txin_v& in = tx.vin[i];
      THROW_IF_FALSE_WALLET_CMN_ERR_EX(in.type() == typeid(txin_to_key), "tx " << tx_hash << " has a non txin_to_key input");
      const crypto::key_image& ki = boost::get<txin_to_key>(in).k_image;

      const auto& src = ft.ftp.sources[i];
      THROW_IF_FALSE_WALLET_INT_ERR_EX(src.real_output < src.outputs.size(), "src.real_output is out of bounds: " << src.real_output);
      const crypto::public_key& out_key = src.outputs[src.real_output].second;

      tri_ki_to_be_added.push_back(std::make_pair(src.transfer_index, ki));
      pk_ki_to_be_added.push_back(std::make_pair(out_key, ki));
    }

    for (auto& p : pk_ki_to_be_added)
    {
      auto it = m_pending_key_images.find(p.first);
      if (it != m_pending_key_images.end())
      {
        LOG_PRINT_YELLOW("warning: for tx " << tx_hash << " out pub key " << p.first << " already exist in m_pending_key_images, ki: " << it->second << ", proposed new ki: " << p.second, LOG_LEVEL_0);
      }
      else
      {
        m_pending_key_images[p.first] = p.second;
        m_pending_key_images_file_container.push_back(tools::out_key_to_ki{ p.first, p.second });
        LOG_PRINT_L2("for tx " << tx_hash << " pending key image added (" << p.first << ", " << p.second << ")");
      }
    }

    for (auto& p : tri_ki_to_be_added)
    {
      THROW_IF_FALSE_WALLET_INT_ERR_EX(p.first < m_transfers.size(), "incorrect transfer index: " << p.first);
      auto& tr = m_transfers[p.first];
      if (tr.m_key_image != currency::null_ki && tr.m_key_image != p.second)
      {
        LOG_PRINT_YELLOW("transfer #" << p.first << " already has not null key image " << tr.m_key_image << " and it will be replaced with ki " << p.second, LOG_LEVEL_0);
      }
      tr.m_key_image = p.second;
      m_key_images[p.second] = p.first;
      LOG_PRINT_L2("for tx " << tx_hash << " key image " << p.second << " was associated with transfer # " << p.first);
    }
  }

  // TODO: print inputs' key images
  print_tx_sent_message(tx, "(from submit_transfer)");
}
//----------------------------------------------------------------------------------------------------
void wallet2::submit_transfer_files(const std::string& signed_tx_file, currency::transaction& tx)
{
  std::string signed_tx_blob;
  bool r = epee::file_io_utils::load_file_to_string(signed_tx_file, signed_tx_blob);
  THROW_IF_FALSE_WALLET_EX(r, error::wallet_common_error, std::string("failed to open file ") + signed_tx_file);

  submit_transfer(signed_tx_blob, tx);
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_recent_transfers_total_count()
{
  return m_transfer_history.size();
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_transfer_entries_count()
{
  return m_transfers.size();
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_recent_transfers_history(std::vector<wallet_public::wallet_transfer_info>& trs, size_t offset, size_t count, uint64_t& total)
{
  if (offset >= m_transfer_history.size())
    return;

  auto start = m_transfer_history.rbegin() + offset;
  auto stop = m_transfer_history.size() - offset >= count ? start + count : m_transfer_history.rend();
  if (!count) 
    stop = m_transfer_history.rend();

  trs.insert(trs.end(), start, stop);
  total = m_transfer_history.size();
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id)
{
  return m_core_proxy->get_transfer_address(adr_str, addr, payment_id);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_okay_for_pos(const transfer_details& tr, uint64_t& stake_unlock_time)
{
  if (!tr.is_spendable())
    return false;

  //blockchain conditions
  if (!is_transfer_unlocked(tr, true, stake_unlock_time))
    return false;

  //prevent staking of after-last-pow-coins
  if (get_blockchain_current_size() - tr.m_ptx_wallet_info->m_block_height <= m_core_runtime_config.min_coinstake_age)
    return false;
  
  if (tr.m_ptx_wallet_info->m_block_height > m_last_pow_block_h)
    return false;

  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_mining_history(wallet_public::mining_history& hist)
{
  for (auto& tr : m_transfer_history)
  {
    if (currency::is_coinbase(tr.tx) && tr.tx.vin.size() == 2)
    {
      tools::wallet_public::mining_history_entry mhe = AUTO_VAL_INIT(mhe);
      mhe.a = tr.amount;
      mhe.t = tr.timestamp;
      mhe.h = tr.height;
      hist.mined_entries.push_back(mhe);
    }
  }
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_pos_entries(currency::COMMAND_RPC_SCAN_POS::request& req)
{
  for (size_t i = 0; i != m_transfers.size(); i++)
  {
    auto& tr = m_transfers[i];
    WLT_CHECK_AND_ASSERT_MES(tr.m_global_output_index != WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED, false, "Wrong output input in transaction");
    uint64_t stake_unlock_time = 0;
    if (!is_transfer_okay_for_pos(tr, stake_unlock_time))
      continue;
    currency::pos_entry pe = AUTO_VAL_INIT(pe);
    pe.amount = tr.amount();
    pe.index = tr.m_global_output_index;
    pe.keyimage = tr.m_key_image;
    pe.wallet_index = i;
    pe.stake_unlock_time = stake_unlock_time;
    pe.block_timestamp = tr.m_ptx_wallet_info->m_block_timestamp;
    req.pos_entries.push_back(pe);
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_and_sign_pos_block(currency::block& b, 
                                         const currency::pos_entry& pos_info, 
                                         const crypto::public_key& source_tx_pub_key, 
                                         uint64_t in_tx_output_index, 
                                         const std::vector<const crypto::public_key*>& keys_ptrs)
{
  //generate coinbase transaction
  WLT_CHECK_AND_ASSERT_MES(b.miner_tx.vin[0].type() == typeid(currency::txin_gen), false, "Wrong output input in transaction");
  WLT_CHECK_AND_ASSERT_MES(b.miner_tx.vin[1].type() == typeid(currency::txin_to_key), false, "Wrong output input in transaction");
  auto& txin = boost::get<currency::txin_to_key>(b.miner_tx.vin[1]);
  txin.k_image = pos_info.keyimage;
  WLT_CHECK_AND_ASSERT_MES(b.miner_tx.signatures.size() == 1 && b.miner_tx.signatures[0].size() == txin.key_offsets.size(),
    false, "Wrong signatures amount in coinbase transacton");



  //derive secret key
  crypto::key_derivation pos_coin_derivation = AUTO_VAL_INIT(pos_coin_derivation);
  bool r = crypto::generate_key_derivation(source_tx_pub_key,
    m_account.get_keys().view_secret_key,
    pos_coin_derivation);

  WLT_CHECK_AND_ASSERT_MES(r, false, "internal error: pos coin base generator: failed to generate_key_derivation("
    <<  source_tx_pub_key
    << ", view secret key: " << m_account.get_keys().view_secret_key << ")");

  crypto::secret_key derived_secret_ephemeral_key = AUTO_VAL_INIT(derived_secret_ephemeral_key);
  crypto::derive_secret_key(pos_coin_derivation,
    in_tx_output_index,
    m_account.get_keys().spend_secret_key,
    derived_secret_ephemeral_key);

  // sign block actually in coinbase transaction
  crypto::hash block_hash = currency::get_block_hash(b);

  crypto::generate_ring_signature(block_hash,
    txin.k_image,
    keys_ptrs,
    derived_secret_ephemeral_key,
    0,
    &b.miner_tx.signatures[0][0]);

  WLT_LOG_L4("GENERATED RING SIGNATURE: block_id " << block_hash
    << "txin.k_image" << txin.k_image
    << "key_ptr:" << *keys_ptrs[0]
    << "signature:" << b.miner_tx.signatures[0][0]);

  return true;
}
//------------------------------------------------------------------
bool wallet2::build_kernel(const pos_entry& pe, const stake_modifier_type& stake_modifier, stake_kernel& kernel, uint64_t& coindays_weight, uint64_t timestamp)
{
  PROFILE_FUNC("build_kernel");
  coindays_weight = 0;
  kernel = stake_kernel();
  kernel.kimage = pe.keyimage;
  kernel.stake_modifier = stake_modifier;
  kernel.block_timestamp = timestamp;

  coindays_weight = get_coinday_weight(pe.amount);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::fill_mining_context(mining_context& ctx)
{
// #ifdef _DEBUG
// 	currency::COMMAND_RPC_GETBLOCKTEMPLATE::request tmpl_req = AUTO_VAL_INIT(tmpl_req);
// 	currency::COMMAND_RPC_GETBLOCKTEMPLATE::response tmpl_rsp = AUTO_VAL_INIT(tmpl_rsp);
// 	tmpl_req.wallet_address = m_account.get_public_address_str();
// 	tmpl_req.pos_block = true;
// 	m_core_proxy->call_COMMAND_RPC_GETBLOCKTEMPLATE(tmpl_req, tmpl_rsp);
// #endif



  bool r = get_pos_entries(ctx.sp);
  WLT_CHECK_AND_ASSERT_MES(r, false, "Failed to get_pos_entries()");

  currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request pos_details_req = AUTO_VAL_INIT(pos_details_req);
  currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response pos_details_resp = AUTO_VAL_INIT(pos_details_resp);
  ctx.rsp.status = API_RETURN_CODE_NOT_FOUND;
  m_core_proxy->call_COMMAND_RPC_GET_POS_MINING_DETAILS(pos_details_req, pos_details_resp);
  if (pos_details_resp.status != API_RETURN_CODE_OK)
    return false;
  ctx.basic_diff.assign(pos_details_resp.pos_basic_difficulty);
  ctx.sm = pos_details_resp.sm;
  ctx.rsp.last_block_hash = pos_details_resp.last_block_hash;
  ctx.rsp.status = API_RETURN_CODE_OK;
  ctx.rsp.is_pos_allowed = pos_details_resp.pos_mining_allowed;
  ctx.rsp.starter_timestamp = pos_details_resp.starter_timestamp;
  return true;
}
//------------------------------------------------------------------
bool wallet2::try_mint_pos()
{
  return try_mint_pos(m_account.get_public_address());
}
//------------------------------------------------------------------
bool wallet2::try_mint_pos(const currency::account_public_address& miner_address)
{
  mining_context ctx = AUTO_VAL_INIT(ctx);
  WLT_LOG_L1("Starting PoS mining iteration");
  fill_mining_context(ctx);
  if (!ctx.rsp.is_pos_allowed)
  {
    WLT_LOG_YELLOW("POS MINING NOT ALLOWED YET", LOG_LEVEL_0);
    return true;
  }

  uint64_t pos_entries_amount = 0;
  for (auto& ent : ctx.sp.pos_entries)
    pos_entries_amount += ent.amount;

  std::atomic<bool> stop(false);
  scan_pos(ctx, stop, [this](){
    size_t blocks_fetched;
    refresh(blocks_fetched);
    if (blocks_fetched)
    {
      WLT_LOG_L0("Detected new block, minting interrupted");
      return false;
    }
    return true;
  }, m_core_runtime_config);
  
  if (ctx.rsp.status == API_RETURN_CODE_OK)
  {
    build_minted_block(ctx.sp, ctx.rsp, miner_address);
  }

  WLT_LOG_L0("PoS mining iteration finished, status: " << ctx.rsp.status << ", used " << ctx.sp.pos_entries.size() << " entries with total amount: " << print_money_brief(pos_entries_amount));

  return true;
}
//-------------------------------
bool wallet2::reset_history()
{
  std::string pass = m_password;
  std::wstring file_path = m_wallet_file;
  account_base acc_tmp = m_account;
  clear();
  m_account = acc_tmp;
  m_password = pass;
  prepare_file_names(file_path);
  return true;
}
//-------------------------------
bool wallet2::build_minted_block(const currency::COMMAND_RPC_SCAN_POS::request& req,
                                 const currency::COMMAND_RPC_SCAN_POS::response& rsp,
                                 uint64_t new_block_expected_height /* = UINT64_MAX */)
{
  return build_minted_block(req, rsp, m_account.get_public_address(), new_block_expected_height);
}

bool wallet2::build_minted_block(const currency::COMMAND_RPC_SCAN_POS::request& req, 
                                 const currency::COMMAND_RPC_SCAN_POS::response& rsp,
                                 const currency::account_public_address& miner_address,
                                 uint64_t new_block_expected_height /* UINT64_MAX */)
{
    //found a block, construct it, sign and push to daemon
    WLT_LOG_GREEN("Found kernel, constructing block", LOG_LEVEL_0);

    CHECK_AND_NO_ASSERT_MES(rsp.index < req.pos_entries.size(), false, "call_COMMAND_RPC_SCAN_POS returned wrong index: " << rsp.index << ", expected less then " << req.pos_entries.size());

    currency::COMMAND_RPC_GETBLOCKTEMPLATE::request tmpl_req = AUTO_VAL_INIT(tmpl_req);
    currency::COMMAND_RPC_GETBLOCKTEMPLATE::response tmpl_rsp = AUTO_VAL_INIT(tmpl_rsp);
    tmpl_req.wallet_address = get_account_address_as_str(miner_address);
    tmpl_req.stakeholder_address = get_account_address_as_str(m_account.get_public_address());
    tmpl_req.pos_block = true;
    tmpl_req.pos_amount = req.pos_entries[rsp.index].amount;
    tmpl_req.pos_index = req.pos_entries[rsp.index].index;
    tmpl_req.extra_text = m_miner_text_info;
    tmpl_req.stake_unlock_time = req.pos_entries[rsp.index].stake_unlock_time;
    //generate packing tx
    transaction pack_tx = AUTO_VAL_INIT(pack_tx);
    if (generate_packing_transaction_if_needed(pack_tx, 0))
    {
      tx_to_blob(pack_tx, tmpl_req.explicit_transaction);
      WLT_LOG_GREEN("Packing inputs: " << pack_tx.vin.size() << " inputs consolidated in tx " << get_transaction_hash(pack_tx), LOG_LEVEL_0);
    }
    m_core_proxy->call_COMMAND_RPC_GETBLOCKTEMPLATE(tmpl_req, tmpl_rsp);
    WLT_CHECK_AND_ASSERT_MES(tmpl_rsp.status == API_RETURN_CODE_OK, false, "Failed to create block template after kernel hash found!");

    currency::block b = AUTO_VAL_INIT(b);
    currency::blobdata block_blob;
    bool res = epee::string_tools::parse_hexstr_to_binbuff(tmpl_rsp.blocktemplate_blob, block_blob);
    WLT_CHECK_AND_ASSERT_MES(res, false, "Failed to create block template after kernel hash found!");
    res = parse_and_validate_block_from_blob(block_blob, b);
    WLT_CHECK_AND_ASSERT_MES(res, false, "Failed to create block template after kernel hash found!");

    if (rsp.last_block_hash != b.prev_id)
    {
      WLT_LOG_YELLOW("Kernel was found but block is behindhand, b.prev_id=" << b.prev_id << ", last_block_hash=" << rsp.last_block_hash, LOG_LEVEL_0);
      return false;
    }

    std::vector<const crypto::public_key*> keys_ptrs;
    WLT_CHECK_AND_ASSERT_MES(req.pos_entries[rsp.index].wallet_index < m_transfers.size(),
        false, "Wrong wallet_index at generating coinbase transacton");

    const auto& target = m_transfers[req.pos_entries[rsp.index].wallet_index].m_ptx_wallet_info->m_tx.vout[m_transfers[req.pos_entries[rsp.index].wallet_index].m_internal_output_index].target;
    WLT_CHECK_AND_ASSERT_MES(target.type() == typeid(currency::txout_to_key), false, "wrong type_id in source transaction in coinbase tx");

    const currency::txout_to_key& txtokey = boost::get<currency::txout_to_key>(target);
    keys_ptrs.push_back(&txtokey.key);

    //put actual time for tx block
    b.timestamp = rsp.block_timestamp;
    currency::etc_tx_time tt = AUTO_VAL_INIT(tt);
    tt.v = m_core_runtime_config.get_core_time();
    b.miner_tx.extra.push_back(tt);
    WLT_LOG_MAGENTA("Applying actual timestamp: " << epee::misc_utils::get_time_str(tt.v), LOG_LEVEL_0);

    //sign block
    res = prepare_and_sign_pos_block(b,
      req.pos_entries[rsp.index],
      get_tx_pub_key_from_extra(m_transfers[req.pos_entries[rsp.index].wallet_index].m_ptx_wallet_info->m_tx),
      m_transfers[req.pos_entries[rsp.index].wallet_index].m_internal_output_index,
      keys_ptrs);
    WLT_CHECK_AND_ASSERT_MES(res, false, "Failed to prepare_and_sign_pos_block");
    
    WLT_LOG_GREEN("Block constructed <" << get_block_hash(b) << ">, sending to core...", LOG_LEVEL_0);

    currency::COMMAND_RPC_SUBMITBLOCK2::request subm_req = AUTO_VAL_INIT(subm_req);
    currency::COMMAND_RPC_SUBMITBLOCK2::response subm_rsp = AUTO_VAL_INIT(subm_rsp);
    subm_req.b = t_serializable_object_to_blob(b);
    if (tmpl_req.explicit_transaction.size())
      subm_req.explicit_txs.push_back(hexemizer{ tmpl_req.explicit_transaction });
    
    m_core_proxy->call_COMMAND_RPC_SUBMITBLOCK2(subm_req, subm_rsp);
    if (subm_rsp.status != API_RETURN_CODE_OK)
    {
      WLT_LOG_ERROR("Constructed block is not accepted by core, status: " << subm_rsp.status);
      return false;
    }    
    WLT_LOG_GREEN("POS block generated and accepted, congrats!", LOG_LEVEL_0);
    m_wcallback->on_pos_block_found(b);
    //@#@
    //double check timestamp
    if (time(NULL) - static_cast<int64_t>(get_actual_timestamp(b)) > 5)
    {
      WLT_LOG_RED("Found block (" << get_block_hash(b) << ") timestamp ("  << get_actual_timestamp(b)
        << ") is suspiciously less (" << time(NULL) - static_cast<int64_t>(get_actual_timestamp(b)) << ") than current time ( " << time(NULL) << ")", LOG_LEVEL_0);
    }
    //
    return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_unconfirmed_transfers(std::vector<wallet_public::wallet_transfer_info>& trs)
{
  for (auto& u : m_unconfirmed_txs)
    trs.push_back(u.second);
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_core_runtime_config(const currency::core_runtime_config& pc)
{
  m_core_runtime_config = pc;
}
//----------------------------------------------------------------------------------------------------
currency::core_runtime_config& wallet2::get_core_runtime_config()
{
  return m_core_runtime_config;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_unlocked(const transfer_details& td) const
{
  uint64_t stub = 0;
  return is_transfer_unlocked(td, false, stub);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_unlocked(const transfer_details& td, bool for_pos_mining, uint64_t& stake_lock_time) const
{
  if (td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_BLOCKED)
    return false; 

  if (td.m_ptx_wallet_info->m_block_height + WALLET_DEFAULT_TX_SPENDABLE_AGE > get_blockchain_current_size())
    return false;

  

  uint64_t unlock_time = get_tx_unlock_time(td.m_ptx_wallet_info->m_tx, td.m_internal_output_index);
  if (for_pos_mining && get_blockchain_current_size() > m_core_runtime_config.hard_fork_01_starts_after_height)
  {
    //allowed of staking locked coins with 
    stake_lock_time = unlock_time;
  }
  else
  {
    if (!currency::is_tx_spendtime_unlocked(unlock_time, get_blockchain_current_size(), m_core_runtime_config.get_core_time()))
      return false;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::push_offer(const bc_services::offer_details_ex& od, currency::transaction& res_tx)
{
  currency::tx_destination_entry tx_dest;
  tx_dest.addr.push_back(m_account.get_keys().account_address);
  tx_dest.amount = m_core_runtime_config.tx_default_fee;
  std::vector<currency::tx_destination_entry> destinations;
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  bc_services::put_offer_into_attachment(static_cast<bc_services::offer_details>(od), attachments);

  destinations.push_back(tx_dest);
  transfer(destinations, 0, 0, od.fee, extra, attachments, detail::ssi_digit, tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx);
}
//----------------------------------------------------------------------------------------------------
const transaction& wallet2::get_transaction_by_id(const crypto::hash& tx_hash)
{
  for (auto it = m_transfer_history.rbegin(); it != m_transfer_history.rend(); it++)
  {
    if (it->tx_hash == tx_hash)
      return it->tx;
  }
  ASSERT_MES_AND_THROW("Tx " << tx_hash << " not found in wallet");
}
//----------------------------------------------------------------------------------------------------
void wallet2::cancel_offer_by_id(const crypto::hash& tx_id, uint64_t of_ind, uint64_t fee, currency::transaction& res_tx)
{
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;
  bc_services::cancel_offer co = AUTO_VAL_INIT(co);
  co.offer_index = of_ind;
  co.tx_id = tx_id;
  const transaction& related_tx = get_transaction_by_id(tx_id);
  crypto::public_key related_tx_pub_key = get_tx_pub_key_from_extra(related_tx);
  currency::keypair ephemeral = AUTO_VAL_INIT(ephemeral);
  bool r = currency::derive_ephemeral_key_helper(m_account.get_keys(), related_tx_pub_key, of_ind, ephemeral);
  CHECK_AND_ASSERT_THROW_MES(r, "derive_ephemeral_key_helper failed, tx_id: " << tx_id << ", of_ind:" << of_ind);

  blobdata sig_blob = bc_services::make_offer_sig_blob(co);
  crypto::generate_signature(crypto::cn_fast_hash(sig_blob.data(), sig_blob.size()), ephemeral.pub, ephemeral.sec, co.sig);
  bc_services::put_offer_into_attachment(co, attachments);

  transfer(std::vector<currency::tx_destination_entry>(), 0, 0, fee, extra, attachments, detail::ssi_digit, tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::update_offer_by_id(const crypto::hash& tx_id, uint64_t of_ind, const bc_services::offer_details_ex& od, currency::transaction& res_tx)
{
  currency::tx_destination_entry tx_dest;
  tx_dest.addr.push_back(m_account.get_keys().account_address);
  tx_dest.amount = m_core_runtime_config.tx_default_fee;
  std::vector<currency::tx_destination_entry> destinations;
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;
  bc_services::update_offer uo = AUTO_VAL_INIT(uo);
  uo.offer_index = of_ind;
  uo.tx_id = tx_id;
  uo.of = od;
  const transaction& related_tx = get_transaction_by_id(tx_id);
  crypto::public_key related_tx_pub_key = get_tx_pub_key_from_extra(related_tx);
  currency::keypair ephemeral;
  bool r = currency::derive_ephemeral_key_helper(m_account.get_keys(), related_tx_pub_key, of_ind, ephemeral);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to derive_ephemeral_key_helper" << tx_id);

  blobdata sig_blob = bc_services::make_offer_sig_blob(uo);
  crypto::generate_signature(crypto::cn_fast_hash(sig_blob.data(), sig_blob.size()), ephemeral.pub, ephemeral.sec, uo.sig);
  bc_services::put_offer_into_attachment(uo, attachments);

  destinations.push_back(tx_dest);
  transfer(destinations, 0, 0, od.fee, extra, attachments, detail::ssi_digit, tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::push_alias_info_to_extra_according_to_hf_status(const currency::extra_alias_entry& ai, std::vector<currency::extra_v>& extra)
{
  if (get_top_block_height() > m_core_runtime_config.hard_fork_02_starts_after_height)
  {
    // after HF2
    extra.push_back(ai);
  }
  else
  {
    // before HF2
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(!ai.m_address.is_auditable(), "auditable addresses are not supported in aliases prior to HF2");
    extra.push_back(ai.to_old());
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::request_alias_registration(const currency::extra_alias_entry& ai, currency::transaction& res_tx, uint64_t fee, uint64_t reward)
{
  if (!validate_alias_name(ai.m_alias))
  {
    throw std::runtime_error(std::string("wrong alias characters: ") + ai.m_alias);
  }

  std::vector<currency::tx_destination_entry> destinations;
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  push_alias_info_to_extra_according_to_hf_status(ai, extra);

  currency::tx_destination_entry tx_dest_alias_reward;
  tx_dest_alias_reward.addr.resize(1);
  get_aliases_reward_account(tx_dest_alias_reward.addr.back());
  tx_dest_alias_reward.amount = reward;
  destinations.push_back(tx_dest_alias_reward);

  transfer(destinations, 0, 0, fee, extra, attachments, detail::ssi_digit, tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx, CURRENCY_TO_KEY_OUT_RELAXED, false);
}
//----------------------------------------------------------------------------------------------------
void wallet2::request_alias_update(currency::extra_alias_entry& ai, currency::transaction& res_tx, uint64_t fee, uint64_t reward)
{
  if (!validate_alias_name(ai.m_alias))
  {
    throw std::runtime_error(std::string("wrong alias characters: ") + ai.m_alias);
  }
  bool r = currency::sign_extra_alias_entry(ai, m_account.get_keys().account_address.spend_public_key, m_account.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to sign alias update");
  WLT_LOG_L2("Generated upodate alias info: " << ENDL
    << "alias: " << ai.m_alias << ENDL
    << "signature: " << currency::print_t_array(ai.m_sign) << ENDL
    << "signed(owner) pub key: " << m_account.get_keys().account_address.spend_public_key << ENDL
    << "transfered to address: " << get_account_address_as_str(ai.m_address) << ENDL
    << "signed_hash: " << currency::get_sign_buff_hash_for_alias_update(ai)
    );

  std::vector<currency::tx_destination_entry> destinations;
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  push_alias_info_to_extra_according_to_hf_status(ai, extra);

  transfer(destinations, 0, 0, fee, extra, attachments, detail::ssi_digit, tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx, CURRENCY_TO_KEY_OUT_RELAXED, false);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::check_available_sources(std::list<uint64_t>& amounts)
{  
  std::list<std::vector<uint64_t> > holds;
  amounts.sort();
  bool res = true;
  for (uint64_t am : amounts)
  {
    holds.push_back(std::vector<uint64_t>());
    std::vector<uint64_t>& current_holds = holds.back();
    uint64_t found = select_transfers(am, 0, DEFAULT_DUST_THRESHOLD, current_holds);
    if (found < am)
    {
      res = false;
      break;
    }
    mark_transfers_with_flag(current_holds, WALLET_TRANSFER_DETAIL_FLAG_BLOCKED, "check_available_sources");
  }

  for (auto& h: holds)
  {
    clear_transfers_from_flag(h, WALLET_TRANSFER_DETAIL_FLAG_BLOCKED, "check_available_sources");
    add_transfers_to_transfers_cache(h);
  }
 

  WLT_LOG_MAGENTA("[CHECK_AVAILABLE_SOURCES]: " << amounts << " res: " << res << ENDL <<" holds: " << holds, LOG_LEVEL_0);
  return res;
}
//----------------------------------------------------------------------------------------------------
std::string get_random_rext(size_t len)
{
  std::string buff(len / 2, 0);
  crypto::generate_random_bytes(len / 2, (void*)buff.data());
  return string_tools::buff_to_hex_nodelimer(buff);
}
//----------------------------------------------------------------------------------------------------

// local_transfers_struct - structure to avoid copying the whole m_transfers 
struct local_transfers_struct
{
  local_transfers_struct(wallet2::transfer_container& tf) :l_transfers_ref(tf)
  {}

  wallet2::transfer_container& l_transfers_ref;
  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(l_transfers_ref)
  END_KV_SERIALIZE_MAP()
};

void wallet2::dump_trunsfers(std::stringstream& ss, bool verbose) const
{
  if (verbose)
  {
    std::string res_buff;
    local_transfers_struct lt(const_cast<transfer_container&>(m_transfers));
    epee::serialization::store_t_to_json(lt, res_buff);
    ss << res_buff;
  }
  else
  {
    boost::io::ios_flags_saver ifs(ss);
    ss << "index                 amount  spent_h  g_index   block  block_ts     flg tx                                                                   out#  key image" << ENDL;
    for (size_t i = 0; i != m_transfers.size(); i++)
    {
      const transfer_details& td = m_transfers[i];
      ss << std::right <<
        std::setw(5)   << i                                          << "  " <<
        std::setw(21)  << print_money(td.amount())                   << "  " <<
        std::setw(7)   << td.m_spent_height                          << "  " <<
        std::setw(7)   << td.m_global_output_index                   << "  " <<
        std::setw(6)   << td.m_ptx_wallet_info->m_block_height       << "  " <<
        std::setw(10)  << td.m_ptx_wallet_info->m_block_timestamp    << "  " <<
        std::setw(4)   << td.m_flags                                 << "  " <<
        get_transaction_hash(td.m_ptx_wallet_info->m_tx)             << "  " <<
        std::setw(4)   << td.m_internal_output_index                 << "  " <<
        td.m_key_image << ENDL;
    }
  }
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::dump_trunsfers(bool verbose) const
{
  std::stringstream ss;
  dump_trunsfers(ss, verbose);
  return ss.str();
}
//----------------------------------------------------------------------------------------------------
void wallet2::dump_key_images(std::stringstream& ss)
{
  for (auto& ki: m_key_images)
  {
    ss << "[" << ki.first << "]: " << ki.second << ENDL;
  }
}
void wallet2::get_multisig_transfers(multisig_transfer_container& ms_transfers)
{
  ms_transfers = m_multisig_transfers;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_contracts(escrow_contracts_container& contracts)
{
  contracts = m_contracts;
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::build_escrow_release_templates(crypto::hash multisig_id,
  uint64_t fee,
  currency::transaction& tx_release_template,
  currency::transaction& tx_burn_template,
  const bc_services::contract_private_details& ecrow_details)
{
  construct_tx_param construct_params = AUTO_VAL_INIT(construct_params);
  construct_params.fee = fee;
  construct_params.multisig_id = multisig_id;
  construct_params.split_strategy_id = detail::ssi_digit;
  construct_params.dsts.resize(2);
  //0 - addr_a
  //1 - addr_b
  construct_params.dsts[0].addr.push_back(ecrow_details.a_addr);
  construct_params.dsts[1].addr.push_back(ecrow_details.b_addr);


  //generate normal escrow release
  construct_params.dsts[0].amount = ecrow_details.amount_a_pledge;
  construct_params.dsts[1].amount = ecrow_details.amount_b_pledge + ecrow_details.amount_to_pay;
  
  //in case of ecrow_details.amount_a_pledge == 0 then exclude a
  if (construct_params.dsts[0].amount == 0)
    construct_params.dsts.erase(construct_params.dsts.begin());


  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  tsa.service_id = BC_ESCROW_SERVICE_ID;
  tsa.instruction = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL;
  construct_params.extra.push_back(tsa);
  {
    finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
    prepare_transaction(construct_params, ftp);
    crypto::secret_key sk = AUTO_VAL_INIT(sk);
    finalize_transaction(ftp, tx_release_template, sk, false);
  }

  //generate burn escrow 
  construct_params.dsts.resize(1);
  construct_params.dsts[0].addr.clear();
  construct_params.dsts[0].addr.push_back(null_pub_addr);
  construct_params.dsts[0].amount = ecrow_details.amount_a_pledge + ecrow_details.amount_b_pledge + ecrow_details.amount_to_pay;

  construct_params.extra.clear();
  tsa.instruction = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN;
  construct_params.extra.push_back(tsa);
  {
    finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
    prepare_transaction(construct_params, ftp);
    crypto::secret_key sk = AUTO_VAL_INIT(sk);
    finalize_transaction(ftp, tx_burn_template, sk, false);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::build_escrow_cancel_template(crypto::hash multisig_id,
  uint64_t expiration_period,
  currency::transaction& tx_cancel_template,
  const bc_services::contract_private_details& ecrow_details)
{
  auto it = m_multisig_transfers.find(multisig_id);
  THROW_IF_FALSE_WALLET_EX(it != m_multisig_transfers.end(), error::wallet_internal_error,
    "unable to find multisig id");

  THROW_IF_FALSE_WALLET_EX(it->second.amount() > (ecrow_details.amount_a_pledge + ecrow_details.amount_to_pay + ecrow_details.amount_b_pledge), error::wallet_internal_error,
    "multisig id out amount no more than escrow total amount");

  construct_tx_param construct_params = AUTO_VAL_INIT(construct_params);
  finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  construct_params.fee = it->second.amount() - (ecrow_details.amount_a_pledge + ecrow_details.amount_to_pay + ecrow_details.amount_b_pledge);
  construct_params.multisig_id = multisig_id;
  construct_params.split_strategy_id = detail::ssi_digit;
  construct_params.dsts.resize(2);
  //0 - addr_a
  //1 - addr_b
  construct_params.dsts[0].addr.push_back(ecrow_details.a_addr);
  construct_params.dsts[1].addr.push_back(ecrow_details.b_addr);


  //generate cancel escrow proposal
  construct_params.dsts[0].amount = ecrow_details.amount_a_pledge + ecrow_details.amount_to_pay;
  construct_params.dsts[1].amount = ecrow_details.amount_b_pledge;

  if (construct_params.dsts[0].amount == 0)
    construct_params.dsts.erase(construct_params.dsts.begin());
  else if (construct_params.dsts[1].amount == 0)
    construct_params.dsts.erase(construct_params.dsts.begin() + 1);

  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  tsa.service_id = BC_ESCROW_SERVICE_ID;
  tsa.instruction = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL;
  construct_params.extra.push_back(tsa);
  currency::etc_tx_details_expiration_time expir = AUTO_VAL_INIT(expir);
  expir.v = m_core_runtime_config.get_core_time() + expiration_period;
  construct_params.extra.push_back(expir);

  prepare_transaction(construct_params, ftp);
  crypto::secret_key sk = AUTO_VAL_INIT(sk);
  finalize_transaction(ftp, tx_cancel_template, sk, false);
}

//----------------------------------------------------------------------------------------------------
void wallet2::build_escrow_template(const bc_services::contract_private_details& ecrow_details,
  size_t fake_outputs_count,
  uint64_t unlock_time,
  uint64_t expiration_time,
  uint64_t b_release_fee,
  const std::string& payment_id,
  currency::transaction& tx,
  std::vector<uint64_t>& selected_transfers, 
  crypto::secret_key& one_time_key)
{
  construct_tx_param ctp = AUTO_VAL_INIT(ctp);
  ctp.crypt_address = ecrow_details.b_addr;
  ctp.dust_policy = tx_dust_policy(DEFAULT_DUST_THRESHOLD);
  ctp.fake_outputs_count = fake_outputs_count;
  ctp.fee = 0;
  ctp.flags = TX_FLAG_SIGNATURE_MODE_SEPARATE;
  ctp.mark_tx_as_complete = false;
  ctp.multisig_id = currency::null_hash;
  ctp.shuffle = true;
  ctp.split_strategy_id = detail::ssi_digit;
  ctp.tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED;
  ctp.unlock_time = unlock_time;

  etc_tx_details_expiration_time t = AUTO_VAL_INIT(t);
  t.v = expiration_time; // TODO: move it to construct_tx_param
  ctp.extra.push_back(t);
  currency::tx_service_attachment att = AUTO_VAL_INIT(att);
  bc_services::pack_attachment_as_gzipped_json(ecrow_details, att);
  att.flags |= TX_SERVICE_ATTACHMENT_ENCRYPT_BODY;
  att.service_id = BC_ESCROW_SERVICE_ID;
  att.instruction = BC_ESCROW_SERVICE_INSTRUCTION_PRIVATE_DETAILS;
  ctp.extra.push_back(att);

  ctp.dsts.resize(1);
  ctp.dsts.back().amount = ecrow_details.amount_a_pledge + ecrow_details.amount_b_pledge + ecrow_details.amount_to_pay + b_release_fee;
  ctp.dsts.back().amount_to_provide = ecrow_details.amount_a_pledge + ecrow_details.amount_to_pay;
  ctp.dsts.back().addr.push_back(ecrow_details.a_addr);
  ctp.dsts.back().addr.push_back(ecrow_details.b_addr);
  ctp.dsts.back().minimum_sigs = 2;
  if (payment_id.size())
  {
    currency::tx_service_attachment att = AUTO_VAL_INIT(att);
    att.body = payment_id;
    att.service_id = BC_PAYMENT_ID_SERVICE_ID;
    att.flags = TX_SERVICE_ATTACHMENT_DEFLATE_BODY;
    ctp.attachments.push_back(att);
  }

  finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  prepare_transaction(ctp, ftp, tx);

  selected_transfers = ftp.selected_transfers;

  finalize_transaction(ftp, tx, one_time_key, false);
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_transfers_to_expiration_list(const std::vector<uint64_t>& selected_transfers, uint64_t expiration, uint64_t change_amount, const crypto::hash& related_tx_id)
{
  // check all elements in selected_transfers for being already mentioned in m_money_expirations
  std::vector<uint64_t> selected_transfers_local;
  for(auto transfer_index : selected_transfers)
  {
    bool found = false;
    for(auto it = m_money_expirations.begin(); !found && it != m_money_expirations.end(); ++it)
    {
      auto& st = it->selected_transfers;
      found = std::find(st.begin(), st.end(), transfer_index) != st.end();
    }
    if (!found)
      selected_transfers_local.push_back(transfer_index); // populate selected_transfers_local only with indices, not containing in m_money_expirations
  }

  if (selected_transfers_local.empty())
    return;

  m_money_expirations.push_back(AUTO_VAL_INIT(expiration_entry_info()));
  m_money_expirations.back().expiration_time = expiration;
  m_money_expirations.back().selected_transfers = selected_transfers_local;
  m_money_expirations.back().change_amount = change_amount;
  m_money_expirations.back().related_tx_id = related_tx_id;

  std::stringstream ss;
  for (auto tr_ind : m_money_expirations.back().selected_transfers)
  {
    THROW_IF_FALSE_WALLET_INT_ERR_EX(tr_ind < m_transfers.size(), "invalid transfer index");
    uint32_t flags_before = m_transfers[tr_ind].m_flags;
    m_transfers[tr_ind].m_flags |= WALLET_TRANSFER_DETAIL_FLAG_BLOCKED;
    m_transfers[tr_ind].m_flags |= WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION;
    ss << " " << std::right << std::setw(4) << tr_ind << "  " << std::setw(21) << print_money(m_transfers[tr_ind].amount()) << "  "
      << std::setw(2) << std::left << flags_before << " -> " << std::setw(2) << std::left << m_transfers[tr_ind].m_flags << "  "
      << get_transaction_hash(m_transfers[tr_ind].m_ptx_wallet_info->m_tx) << std::endl;
  }
  WLT_LOG_GREEN(m_money_expirations.back().selected_transfers.size() << " transfer(s) added to expiration list:" << ENDL <<
    "index                 amount  flags     tx hash" << ENDL <<
    ss.str() <<
    "change_amount: " << print_money_brief(change_amount) << ", expire(s) at: " << expiration, LOG_LEVEL_0);
}
//----------------------------------------------------------------------------------------------------
void wallet2::remove_transfer_from_expiration_list(uint64_t transfer_index)
{
  THROW_IF_FALSE_WALLET_INT_ERR_EX(transfer_index < m_transfers.size(), "invalid transfer index");
  for(auto it = m_money_expirations.begin(); it != m_money_expirations.end(); /* nothing */)
  {
    auto& st = it->selected_transfers;
    auto jt = std::find(st.begin(), st.end(), transfer_index);
    if (jt != st.end())
    {
      WLT_LOG_GREEN("Transfer [" << transfer_index << "], amount: " << print_money(m_transfers[transfer_index].amount()) << ", tx: " << get_transaction_hash(m_transfers[transfer_index].m_ptx_wallet_info->m_tx) <<
        " was removed from the expiration list", LOG_LEVEL_0);
      st.erase(jt);
      if (st.empty())
      {
        it = m_money_expirations.erase(it);
        continue;
      }
    }
    ++it;
  }
  // clear proposal reservation flag and blocked flag
  uint32_t flags_before = m_transfers[transfer_index].m_flags;
  m_transfers[transfer_index].m_flags &= ~WALLET_TRANSFER_DETAIL_FLAG_BLOCKED;
  m_transfers[transfer_index].m_flags &= ~WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION;
  WLT_LOG_BLUE("Transfer [" << transfer_index << "] was cleared from escrow proposal reservation, flags: " << flags_before << " -> " << m_transfers[transfer_index].m_flags << ", reason: intentional removing from expiration list", LOG_LEVEL_0);
  
  // (don't change m_spent flag, because transfer status is unclear - the caller should take care of it)
}
//----------------------------------------------------------------------------------------------------
void wallet2::send_escrow_proposal(const wallet_public::create_proposal_param& wp,
  currency::transaction &proposal_tx,
  currency::transaction &escrow_template_tx)
{
  return send_escrow_proposal(wp.details, wp.fake_outputs_count, wp.unlock_time, wp.expiration_period, wp.fee, wp.b_fee, wp.payment_id, proposal_tx, escrow_template_tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::send_escrow_proposal(const bc_services::contract_private_details& ecrow_details,
  size_t fake_outputs_count,
  uint64_t unlock_time,
  uint64_t expiration_period,
  uint64_t fee,
  uint64_t b_release_fee,
  const std::string& payment_id,
  currency::transaction &tx, 
  currency::transaction &template_tx)
{
  if (!is_connected_to_net())
  {
    THROW_IF_TRUE_WALLET_EX(true, error::wallet_internal_error,
      "Transfer attempt while daemon offline");
  }

  crypto::secret_key one_time_key = AUTO_VAL_INIT(one_time_key);
  uint64_t expiration_time = m_core_runtime_config.get_core_time() + expiration_period;
  std::vector<uint64_t> selected_transfers_for_template;
  build_escrow_template(ecrow_details, fake_outputs_count, unlock_time, expiration_time, b_release_fee, payment_id, template_tx, selected_transfers_for_template, one_time_key);
  crypto::hash ms_id = get_multisig_out_id(template_tx, get_multisig_out_index(template_tx.vout));

  const uint32_t mask_to_mark_escrow_template_locked_transfers = WALLET_TRANSFER_DETAIL_FLAG_BLOCKED | WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION;
  mark_transfers_with_flag(selected_transfers_for_template, mask_to_mark_escrow_template_locked_transfers, "preparing escrow template tx, contract: " + epee::string_tools::pod_to_hex(ms_id));

  construct_tx_param ctp = AUTO_VAL_INIT(ctp);

  bc_services::proposal_body pb = AUTO_VAL_INIT(pb);
  pb.tx_onetime_secret_key = one_time_key;
  pb.tx_template = template_tx;
  currency::tx_service_attachment att = AUTO_VAL_INIT(att);
  att.body = t_serializable_object_to_blob(pb);
  att.service_id = BC_ESCROW_SERVICE_ID;
  att.instruction = BC_ESCROW_SERVICE_INSTRUCTION_PROPOSAL;
  att.flags = TX_SERVICE_ATTACHMENT_ENCRYPT_BODY | TX_SERVICE_ATTACHMENT_DEFLATE_BODY;
  ctp.attachments.push_back(att);

  ctp.crypt_address = ecrow_details.b_addr;
  ctp.dust_policy = tx_dust_policy(DEFAULT_DUST_THRESHOLD);
  ctp.fake_outputs_count = fake_outputs_count;
  ctp.fee = fee;
  ctp.shuffle = true;
  ctp.split_strategy_id = detail::ssi_digit;
  ctp.tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED;
  ctp.unlock_time = unlock_time;

  finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  try
  {
      prepare_transaction(ctp, ftp);
      crypto::secret_key sk = AUTO_VAL_INIT(sk);
      finalize_transaction(ftp, tx, sk, false);
  }
  catch (...)
  {
    clear_transfers_from_flag(selected_transfers_for_template, mask_to_mark_escrow_template_locked_transfers, "undo prepared escrow template tx"); // don't forget to unlock template transfers if smth went wrong
    add_transfers_to_transfers_cache(selected_transfers_for_template);
    throw;
  }

  send_transaction_to_network(tx);
  
  mark_transfers_as_spent(ftp.selected_transfers, std::string("escrow proposal sent, tx <") + epee::string_tools::pod_to_hex(get_transaction_hash(tx)) + ">, contract: " + epee::string_tools::pod_to_hex(ms_id));
  add_sent_tx_detailed_info(tx, ftp.prepared_destinations, ftp.selected_transfers);

  print_tx_sent_message(tx, "(from multisig)", fee);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources_for_packing(uint64_t items_to_pack, size_t fake_outputs_count, std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies, uint64_t& found_money)
{
  prepare_free_transfers_cache(fake_outputs_count);
  auto it = m_found_free_amounts.find(CURRENCY_BLOCK_REWARD);
  if (it == m_found_free_amounts.end() || it->second.size() < m_pos_mint_packing_size)
    return false;

  for (auto set_it = it->second.begin(); set_it != it->second.end() && selected_indicies.size() <= m_pos_mint_packing_size; )
  {
    if (is_transfer_ready_to_go(m_transfers[*set_it], fake_outputs_count))
    {
      found_money += it->first;
      selected_indicies.push_back(*set_it);
      WLT_LOG_L2("Selected index: " << *set_it << ", transfer_details: " << ENDL << epee::serialization::store_t_to_json(m_transfers[*set_it]));
      
      it->second.erase(set_it++);
    }
    else
      set_it++;
  }
  if (!it->second.size())
    m_found_free_amounts.erase(it);

  return prepare_tx_sources(fake_outputs_count, sources, selected_indicies, found_money);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources(uint64_t needed_money, size_t fake_outputs_count, uint64_t dust_threshold, std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies, uint64_t& found_money)
{
  found_money = select_transfers(needed_money, fake_outputs_count, dust_threshold, selected_indicies);
  THROW_IF_FALSE_WALLET_EX_MES(found_money >= needed_money, error::not_enough_money, "wallet_dump: " << ENDL << dump_trunsfers(false), found_money, needed_money, 0);
  return prepare_tx_sources(fake_outputs_count, sources, selected_indicies, found_money);
}
//----------------------------------------------------------------------------------------------------
void wallet2::prefetch_global_indicies_if_needed(std::vector<uint64_t>& selected_indicies)
{
  std::list<std::reference_wrapper<const currency::transaction>> txs;
  std::list<uint64_t> indices_that_requested_global_indicies;
  for (uint64_t i : selected_indicies)
  {
    if (m_transfers[i].m_global_output_index == WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED)
    {
      indices_that_requested_global_indicies.push_back(i);
      txs.push_back(m_transfers[i].m_ptx_wallet_info->m_tx);
    }
  }

  std::vector<std::vector<uint64_t> > outputs_for_all_txs;
  fetch_tx_global_indixes(txs, outputs_for_all_txs);
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(txs.size() == outputs_for_all_txs.size(), "missmatch sizes txs.size() == outputs_for_all_txs.size()");
  auto it_indices = indices_that_requested_global_indicies.begin();
  auto it_ooutputs = outputs_for_all_txs.begin();
  for (; it_ooutputs != outputs_for_all_txs.end();)
  {
    transfer_details& td = m_transfers[*it_indices];
    td.m_global_output_index = (*it_ooutputs)[td.m_internal_output_index];
    it_ooutputs++; it_indices++;
  }
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources(size_t fake_outputs_count, std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies, uint64_t& found_money)
{
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
  typedef currency::tx_source_entry::output_entry tx_output_entry;

  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response daemon_resp = AUTO_VAL_INIT(daemon_resp);
  if (fake_outputs_count)
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
    req.use_forced_mix_outs = false; //add this feature to UI later
    req.outs_count = fake_outputs_count + 1;// add one to make possible (if need) to skip real output key
    for (uint64_t i: selected_indicies)
    {
      auto it = m_transfers.begin() + i;
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it->m_ptx_wallet_info->m_tx.vout.size() > it->m_internal_output_index,
        "m_internal_output_index = " << it->m_internal_output_index <<
        " is greater or equal to outputs count = " << it->m_ptx_wallet_info->m_tx.vout.size());
      req.amounts.push_back(it->amount());
    }

    bool r = m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(req, daemon_resp);
    THROW_IF_FALSE_WALLET_EX(r, error::no_connection_to_daemon, "getrandom_outs.bin");
    THROW_IF_FALSE_WALLET_EX(daemon_resp.status != API_RETURN_CODE_BUSY, error::daemon_busy, "getrandom_outs.bin");
    THROW_IF_FALSE_WALLET_EX(daemon_resp.status == API_RETURN_CODE_OK, error::get_random_outs_error, daemon_resp.status);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(daemon_resp.outs.size() == selected_indicies.size(), 
      "daemon returned wrong response for getrandom_outs.bin, wrong amounts count = " << daemon_resp.outs.size() << ", expected: " << selected_indicies.size());

    std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> scanty_outs;
    for(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& amount_outs : daemon_resp.outs)
    {
      if (amount_outs.outs.size() < req.outs_count)
      {
        scanty_outs.push_back(amount_outs);
      }
    }
    THROW_IF_FALSE_WALLET_EX(scanty_outs.empty(), error::not_enough_outs_to_mix, scanty_outs, fake_outputs_count);
  }

  //lets prefetch m_global_output_index for selected_indicies
  prefetch_global_indicies_if_needed(selected_indicies);

  //prepare inputs
  size_t i = 0;
  for (uint64_t J : selected_indicies)
  {
    auto it = m_transfers.begin() + J;

    sources.push_back(AUTO_VAL_INIT(currency::tx_source_entry()));
    currency::tx_source_entry& src = sources.back();
    transfer_details& td = *it;
    src.transfer_index = it - m_transfers.begin();
    src.amount = td.amount();
    //paste mixin transaction
    if (daemon_resp.outs.size())
    {
      daemon_resp.outs[i].outs.sort([](const out_entry& a, const out_entry& b){return a.global_amount_index < b.global_amount_index; });
      for(out_entry& daemon_oe : daemon_resp.outs[i].outs)
      {
        if (td.m_global_output_index == daemon_oe.global_amount_index)
          continue;
        tx_output_entry oe;
        oe.first = daemon_oe.global_amount_index;
        oe.second = daemon_oe.out_key;
        src.outputs.push_back(oe);
        if (src.outputs.size() >= fake_outputs_count)
          break;
      }
    }

    //paste real transaction to the random index
    auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
    {
      if (a.first.type().hash_code() == typeid(uint64_t).hash_code())
        return boost::get<uint64_t>(a.first) >= td.m_global_output_index;
      return false; // TODO: implement deterministics real output placement in case there're ref_by_id outs
    });
    //size_t real_index = src.outputs.size() ? (rand() % src.outputs.size() ):0;
    tx_output_entry real_oe;
    real_oe.first = td.m_global_output_index; // TODO: use ref_by_id when neccessary
    real_oe.second = boost::get<txout_to_key>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].target).key;
    auto interted_it = src.outputs.insert(it_to_insert, real_oe);
    src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_ptx_wallet_info->m_tx);
    src.real_output = interted_it - src.outputs.begin();
    src.real_output_in_tx_index = td.m_internal_output_index;
    print_source_entry(src);
    ++i;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources(crypto::hash multisig_id, std::vector<currency::tx_source_entry>& sources, uint64_t& found_money)
{
  auto it = m_multisig_transfers.find(multisig_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(it != m_multisig_transfers.end(), "can't find multisig_id: " + epee::string_tools::pod_to_hex(multisig_id));
  THROW_IF_FALSE_WALLET_INT_ERR_EX(!it->second.is_spent(), "output with multisig_id: " + epee::string_tools::pod_to_hex(multisig_id) + " has already been spent by other party at height " + epee::string_tools::num_to_string_fast(it->second.m_spent_height));

  THROW_IF_FALSE_WALLET_INT_ERR_EX(it->second.m_internal_output_index < it->second.m_ptx_wallet_info->m_tx.vout.size(), "it->second.m_internal_output_index < it->second.m_tx.vout.size()");
  const tx_out& out = it->second.m_ptx_wallet_info->m_tx.vout[it->second.m_internal_output_index];
  THROW_IF_FALSE_WALLET_INT_ERR_EX(out.target.type() == typeid(txout_multisig), "ms out target type is " << out.target.type().name() << ", expected: txout_multisig");
  const txout_multisig& ms_out = boost::get<txout_multisig>(out.target);

  sources.push_back(AUTO_VAL_INIT(currency::tx_source_entry()));
  currency::tx_source_entry& src = sources.back();
  src.amount = found_money = out.amount;
  src.real_output_in_tx_index = it->second.m_internal_output_index;
  src.real_out_tx_key = get_tx_pub_key_from_extra(it->second.m_ptx_wallet_info->m_tx);
  src.multisig_id = multisig_id;
  src.ms_sigs_count = ms_out.minimum_sigs;
  src.ms_keys_count = ms_out.keys.size();
  return true;
}
//----------------------------------------------------------------------------------------------------------------
uint64_t wallet2::get_needed_money(uint64_t fee, const std::vector<currency::tx_destination_entry>& dsts)
{
  uint64_t needed_money = fee;
  BOOST_FOREACH(auto& dt, dsts)
  {
    THROW_IF_TRUE_WALLET_EX(0 == dt.amount, error::zero_destination);
    uint64_t money_to_add = dt.amount;
    if (dt.amount_to_provide)
      money_to_add = dt.amount_to_provide;
  
    needed_money += money_to_add;
    THROW_IF_TRUE_WALLET_EX(needed_money < money_to_add, error::tx_sum_overflow, dsts, fee);
  }
  return needed_money;
}
//----------------------------------------------------------------------------------------------------------------
void wallet2::send_transaction_to_network(const transaction& tx)
{
  COMMAND_RPC_SEND_RAW_TX::request req;
  req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(tx));
  COMMAND_RPC_SEND_RAW_TX::response daemon_send_resp;
  bool r = m_core_proxy->call_COMMAND_RPC_SEND_RAW_TX(req, daemon_send_resp);
  THROW_IF_TRUE_WALLET_EX(!r, error::no_connection_to_daemon, "sendrawtransaction");
  THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status == API_RETURN_CODE_BUSY, error::daemon_busy, "sendrawtransaction");
  THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status == API_RETURN_CODE_DISCONNECTED, error::wallet_internal_error, "Transfer attempt while daemon offline");
  THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status != API_RETURN_CODE_OK, error::tx_rejected, tx, daemon_send_resp.status);

  WLT_LOG_L2("transaction " << get_transaction_hash(tx) << " generated ok and sent to daemon:" << ENDL << currency::obj_to_json_str(tx));
}

void wallet2::add_sent_tx_detailed_info(const transaction& tx,
  const std::vector<currency::tx_destination_entry>& destinations,
  const std::vector<uint64_t>& selected_transfers)
{
  std::vector<std::string> recipients;
  std::unordered_set<account_public_address> used_addresses;
  for (const auto& d : destinations)
  {
    for (const auto& addr : d.addr)
    {
      if (used_addresses.insert(addr).second && addr != m_account.get_public_address())
        recipients.push_back(get_account_address_as_str(addr));
    }
  }
  if (!recipients.size())
  {
	  //transaction send to ourself
	  recipients.push_back(get_account_address_as_str(m_account.get_public_address()));
  }

  add_sent_unconfirmed_tx(tx, recipients, selected_transfers, destinations);
}
//----------------------------------------------------------------------------------------------------
void wallet2::mark_transfers_with_flag(const std::vector<uint64_t>& selected_transfers, uint32_t flag, const std::string& reason /* = empty_string */, bool throw_if_flag_already_set /* = false */)
{
  // check all selected transfers prior to flag change
  for (uint64_t i : selected_transfers)
  {
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(i < m_transfers.size(), "invalid transfer index given: " << i << ", m_transfers.size() == " << m_transfers.size());
    if (throw_if_flag_already_set)
    {
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX((m_transfers[i].m_flags & flag) == 0, "transfer #" << i << " already has flag " << flag << ": " << m_transfers[i].m_flags << ", transfer info:" << ENDL << epee::serialization::store_t_to_json(m_transfers[i]));
    }
  }

  for (uint64_t i : selected_transfers)
  {
    uint32_t flags_before = m_transfers[i].m_flags;
    m_transfers[i].m_flags |= flag;
    WLT_LOG_L1("marking transfer  #" << std::setfill('0') << std::right << std::setw(3) << i << " with flag " << flag << " : " << flags_before << " -> " << m_transfers[i].m_flags <<
      (reason.empty() ? "" : ", reason: ") << reason);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::clear_transfers_from_flag(const std::vector<uint64_t>& selected_transfers, uint32_t flag, const std::string& reason /* = empty_string */) noexcept
{
  TRY_ENTRY();
  for (uint64_t i : selected_transfers)
  {
    if (i >= m_transfers.size())
    {
      WLT_LOG_ERROR("INTERNAL ERROR: i: " << i << " >= m_transfers.size() : " << m_transfers.size());
      continue;
    }
    uint32_t flags_before = m_transfers[i].m_flags;
    m_transfers[i].m_flags &= ~flag;
    WLT_LOG_L1("clearing transfer #" << std::setfill('0') << std::right << std::setw(3) << i << " from flag " << flag << " : " << flags_before << " -> " << m_transfers[i].m_flags <<
      (reason.empty() ? "" : ", reason: ") << reason);
  }
  CATCH_ENTRY_NO_RETURN();
}
//----------------------------------------------------------------------------------------------------
void wallet2::exception_handler()
{
  m_found_free_amounts.clear();
}
//----------------------------------------------------------------------------------------------------
void wallet2::exception_handler() const
{
  // do nothing
  // TODO: is it correct?
}
//----------------------------------------------------------------------------------------------------
void wallet2::mark_transfers_as_spent(const std::vector<uint64_t>& selected_transfers, const std::string& reason /* = empty_string */)
{
  // TODO: design a safe undo for this operation
  mark_transfers_with_flag(selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_SPENT, reason);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::extract_offers_from_transfer_entry(size_t i, std::unordered_map<crypto::hash, bc_services::offer_details_ex>& offers_local)
{
  //TODO: this code supports only one market(offer) instruction per transaction
  switch (m_transfer_history[i].tx_type)
  {
    case GUI_TX_TYPE_PUSH_OFFER:
    {
      bc_services::offer_details od;
      if (!get_type_in_variant_container(m_transfer_history[i].srv_attachments, od))
      {
        WLT_LOG_ERROR("Transaction history entry " << i << " market as type " << m_transfer_history[i].tx_type << " but get_type_in_variant_container returned false for bc_services::offer_details");
        break;
      }
      crypto::hash h = null_hash;
      h = m_transfer_history[i].tx_hash;
      bc_services::offer_details_ex& ode = offers_local[h];
      ode = AUTO_VAL_INIT(bc_services::offer_details_ex());
      static_cast<bc_services::offer_details&>(ode) = od;
      //fill extra fields
      ode.tx_hash = m_transfer_history[i].tx_hash;
      ode.index_in_tx = 0; // TODO: handle multiple offers in tx, now only one per tx is supported
      ode.timestamp = m_transfer_history[i].timestamp; 
      ode.fee = m_transfer_history[i].fee;
      ode.stopped = false;
      break;
    }
    case GUI_TX_TYPE_UPDATE_OFFER:
    {
      bc_services::update_offer uo;
      if (!get_type_in_variant_container(m_transfer_history[i].srv_attachments, uo))
      {
        WLT_LOG_ERROR("Transaction history entry " << i << " market as type " << m_transfer_history[i].tx_type << " but get_type_in_variant_container returned false for update_offer");
        break;
      }
      crypto::hash h = null_hash;
      h = m_transfer_history[i].tx_hash;
      bc_services::offer_details_ex& ode = offers_local[h];
      ode = AUTO_VAL_INIT(bc_services::offer_details_ex());
      static_cast<bc_services::offer_details&>(ode) = uo.of;
      //fill extra fields
      ode.tx_hash = m_transfer_history[i].tx_hash;
      ode.index_in_tx = 0;
      ode.fee = m_transfer_history[i].fee;
      ode.stopped = false;
      ode.tx_original_hash = uo.tx_id;
      //remove old transaction
      crypto::hash h_old = uo.tx_id;
      auto it = offers_local.find(h_old);
      if (it == offers_local.end())
      {
        WLT_LOG_L3("Unable to find original tx record " << h_old << " in update offer " << h);
        break;
      }
      //keep original timestamp
      ode.timestamp = it->second.timestamp;
      offers_local.erase(it);
      break;
    }
    case GUI_TX_TYPE_CANCEL_OFFER:
    {
      bc_services::cancel_offer co;
      if (!get_type_in_variant_container(m_transfer_history[i].srv_attachments, co))
      {
        WLT_LOG_ERROR("Transaction history entry " << i << " market as type " << m_transfer_history[i].tx_type << " but get_type_in_variant_container returned false for cancel_offer");
        break;
      }
      crypto::hash h = co.tx_id;
      auto it = offers_local.find(h);
      if (it == offers_local.end())
      {
        WLT_LOG_L3("Unable to find original tx record " << h << " in cancel offer " << h);
        break;
      }
      offers_local.erase(it);

    }
    default:
      ;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::select_my_offers(std::list<bc_services::offer_details_ex>& offers)
{
  std::unordered_map<crypto::hash, bc_services::offer_details_ex> offers_local;

  if (!m_transfer_history.size())
    return true;

  uint64_t stop_timestamp = m_core_runtime_config.get_core_time() - OFFER_MAXIMUM_LIFE_TIME;

  size_t i = m_transfer_history.size() - 1;
  for (; i != 0; i--)
  {
    if (m_transfer_history[i].timestamp < stop_timestamp)
    {
      i++;
      break;
    }
  }
  if (i == 0 && m_transfer_history[0].timestamp < stop_timestamp)
    i++;
  if (i >= m_transfer_history.size())
    return true;

  for (; i != m_transfer_history.size(); i++)
  {
    extract_offers_from_transfer_entry(i, offers_local);
  }
  for (const auto& o : offers_local)
    offers.push_back(o.second);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_actual_offers(std::list<bc_services::offer_details_ex>& offers)
{
  select_my_offers(offers);
  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::select_indices_for_transfer(std::vector<uint64_t>& selected_indexes, free_amounts_cache_type& found_free_amounts, uint64_t needed_money, uint64_t fake_outputs_count)
{
  WLT_LOG_GREEN("Selecting indices for transfer found_free_amounts.size()=" << found_free_amounts.size() << "...", LOG_LEVEL_0);
  uint64_t found_money = 0;
  while(found_money < needed_money && found_free_amounts.size())
  {
    auto it = found_free_amounts.lower_bound(needed_money - found_money);
    if (!(it != found_free_amounts.end() && it->second.size()))
    {
      it = --found_free_amounts.end();
      WLT_CHECK_AND_ASSERT_MES(it->second.size(), 0, "internal error: empty found_free_amounts map");
    }
    if (is_transfer_ready_to_go(m_transfers[*it->second.begin()], fake_outputs_count))
    {
      found_money += it->first;
      selected_indexes.push_back(*it->second.begin());
      WLT_LOG_L2("Selected index: " << *it->second.begin() << ", transfer_details: " << ENDL << epee::serialization::store_t_to_json(m_transfers[*it->second.begin()]));
    }
    it->second.erase(it->second.begin());
    if (!it->second.size())
      found_free_amounts.erase(it);

  }
  WLT_LOG_GREEN("Selected " << print_money(found_money) << " coins, found_free_amounts.size()=" << found_free_amounts.size(), LOG_LEVEL_0);
  return found_money;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_ready_to_go(const transfer_details& td, uint64_t fake_outputs_count)
{
  if (is_transfer_able_to_go(td, fake_outputs_count) && is_transfer_unlocked(td))
  {
    return true;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_able_to_go(const transfer_details& td, uint64_t fake_outputs_count)
{
  if (!td.is_spendable())
    return false;

  if (!currency::is_mixattr_applicable_for_fake_outs_counter(boost::get<currency::txout_to_key>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].target).mix_attr, fake_outputs_count))
    return false;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_free_transfers_cache(uint64_t fake_outputs_count)
{
  WLT_LOG_L2("Preparing transfers_cache...");
  uint64_t count = 0;
  if (!m_found_free_amounts.size() || fake_outputs_count != m_fake_outputs_count)
  {
    m_found_free_amounts.clear();
    for (size_t i = 0; i < m_transfers.size(); ++i)
    {
      const transfer_details& td = m_transfers[i];
      if (is_transfer_able_to_go(td, fake_outputs_count))
      {
        m_found_free_amounts[td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].amount].insert(i);
        count++;
      }
    }
    m_fake_outputs_count = fake_outputs_count;
  }

  WLT_LOG_L2("Transfers_cache prepared. " << count << " items cached for " << m_found_free_amounts.size() << " amounts");
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_transfers_to_transfers_cache(const std::vector<uint64_t>& indexs)
{
  for (auto i : indexs)
    add_transfer_to_transfers_cache(m_transfers[i].m_ptx_wallet_info->m_tx.vout[m_transfers[i].m_internal_output_index].amount , i);
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_transfer_to_transfers_cache(uint64_t amount, uint64_t index)
{
  m_found_free_amounts[amount].insert(index);
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::select_transfers(uint64_t needed_money, size_t fake_outputs_count, uint64_t dust, std::vector<uint64_t>& selected_indicies)
{
  prepare_free_transfers_cache(fake_outputs_count);
  return select_indices_for_transfer(selected_indicies, m_found_free_amounts, needed_money, fake_outputs_count);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::read_money_transfer2_details_from_tx(const transaction& tx, const std::vector<currency::tx_destination_entry>& splitted_dsts,
                                                             wallet_public::wallet_transfer_info_details& wtd)
{
  PROFILE_FUNC("wallet2::read_money_transfer2_details_from_tx");
  for (auto& d : splitted_dsts)
  {
    if (d.addr.size() && d.addr.back().spend_public_key == m_account.get_keys().account_address.spend_public_key &&
      d.addr.back().view_public_key == m_account.get_keys().account_address.view_public_key)
      wtd.rcv.push_back(d.amount);
  }

  //scan key images
  for (auto& i : tx.vin)
  {
    if (i.type() == typeid(currency::txin_to_key))
    {
      const currency::txin_to_key& in_to_key = boost::get<currency::txin_to_key>(i);
      if (m_key_images.count(in_to_key.k_image))
      {
        wtd.spn.push_back(in_to_key.amount);
      }
    }
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_sent_unconfirmed_tx(const currency::transaction& tx,  
                                      const std::vector<std::string>& recipients, 
                                      const std::vector<uint64_t>& selected_indicies, 
                                      const std::vector<currency::tx_destination_entry>& splitted_dsts)
{
  PROFILE_FUNC("wallet2::add_sent_unconfirmed_tx");
  wallet_public::wallet_transfer_info& unconfirmed_wti = misc_utils::get_or_insert_value_initialized(m_unconfirmed_txs, currency::get_transaction_hash(tx));

  //unconfirmed_wti.tx = tx;
  unconfirmed_wti.remote_addresses = recipients;
  for (auto addr : recipients)
    unconfirmed_wti.recipients_aliases.push_back(get_alias_for_address(addr));
  unconfirmed_wti.is_income = false;
  unconfirmed_wti.selected_indicies = selected_indicies;
  /*TODO: add selected_indicies to read_money_transfer2_details_from_tx in case of performance problems*/
  read_money_transfer2_details_from_tx(tx, splitted_dsts,  unconfirmed_wti.td);
  
  uint64_t change_amount = 0;
  uint64_t inputs_amount = 0;
  for (auto i : unconfirmed_wti.td.rcv)
    change_amount += i;
  for (auto i : unconfirmed_wti.td.spn)
    inputs_amount += i;

  prepare_wti(unconfirmed_wti, 0, m_core_runtime_config.get_core_time(), tx, inputs_amount - (change_amount + get_tx_fee(tx)), money_transfer2_details());
  rise_on_transfer2(unconfirmed_wti);
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::get_alias_for_address(const std::string& addr)
{
  PROFILE_FUNC("wallet2::get_alias_for_address");
  currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request req = addr;
  currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response res = AUTO_VAL_INIT(res);
  if (!m_core_proxy->call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(req, res))
  {
    WLT_LOG_L0("Failed to COMMAND_RPC_GET_ALIASES_BY_ADDRESS");
    return "";
  }
  return res.alias_info.alias;
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, 
  const std::vector<currency::attachment_v>& attachments, 
  currency::transaction& tx)
{
  transfer(dsts, fake_outputs_count, unlock_time, fee, extra, attachments, detail::ssi_digit, tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, 
  const std::vector<currency::attachment_v>& attachments)
{
  currency::transaction tx;
  transfer(dsts, fake_outputs_count, unlock_time, fee, extra, attachments, tx);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_connected_to_net()
{
  currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_INFO::response res = AUTO_VAL_INIT(res);
  if (!m_core_proxy->call_COMMAND_RPC_GET_INFO(req, res))
  {
    WLT_LOG_L0("Failed to COMMAND_RPC_GET_INFO");
    return false;
  }
  return (res.synchronized_connections_count) ? true : false;
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_genesis_if_needed(const currency::block& genesis)
{
  if (!m_transfers.empty() || !m_key_images.empty())
    return;

  THROW_IF_TRUE_WALLET_EX(get_blockchain_current_size() > 1, error::wallet_internal_error, "Can't change wallet genesis block once the blockchain has been populated");

  crypto::hash genesis_hash = get_block_hash(genesis);
  if (get_blockchain_current_size() == 1 && m_chain.get_genesis() != genesis_hash)
      WLT_LOG_L0("Changing genesis block for wallet " << m_account.get_public_address_str() << ":" << ENDL << "    " << m_chain.get_genesis() << " -> " << genesis_hash);

  //m_blockchain.clear();

  //m_blockchain.push_back(genesis_hash);
  m_chain.set_genesis(genesis_hash);
  m_last_bc_timestamp = genesis.timestamp;

  WLT_LOG_L2("Processing genesis block: " << genesis_hash);
  process_new_transaction(genesis.miner_tx, 0, genesis, nullptr);
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_genesis(const crypto::hash& genesis_hash)
{
  THROW_IF_TRUE_WALLET_EX(get_blockchain_current_size() != 1, error::wallet_internal_error, "Can't change wallet genesis hash once the blockchain has been populated");
  WLT_LOG_L0("Changing genesis hash for wallet " << m_account.get_public_address_str() << ":" << ENDL << "    " << m_chain.get_genesis() << " -> " << genesis_hash);
  m_chain.set_genesis(genesis_hash);
}
//----------------------------------------------------------------------------------------------------
void wallet2::print_tx_sent_message(const currency::transaction& tx, const std::string& description, uint64_t fee /* = UINT64_MAX */)
{
  //uint64_t balance_unlocked = 0;
  //uint64_t balance_total = balance(balance_unlocked);

  std::stringstream ss;
  if (fee != UINT64_MAX)
    ss << "Commission: " << std::setw(21) << std::right << print_money(fee) << ENDL;

  WLT_LOG_CYAN("Transaction " << get_transaction_hash(tx) << " was successfully sent " << description << ENDL
    << ss.str()
//    << "Balance:    " << std::setw(21) << print_money(balance_total) << ENDL
//    << "Unlocked:   " << std::setw(21) << print_money(balance_unlocked) << ENDL
    << "Please, wait for confirmation for your balance to be unlocked.",
    LOG_LEVEL_0);
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_tx_expiration_median() const
{
  currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response res = AUTO_VAL_INIT(res);
  m_core_proxy->call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(req, res);

  if (res.status != API_RETURN_CODE_OK)
  {
    WLT_LOG_ERROR("COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN failed, status: " << res.status);
    return 0;
  }
  
  return res.expiration_median;
}
//----------------------------------------------------------------------------------------------------
void wallet2::print_source_entry(const currency::tx_source_entry& src) const
{
  std::ostringstream indexes;
  std::for_each(src.outputs.begin(), src.outputs.end(), [&](const currency::tx_source_entry::output_entry& s_e) { indexes << s_e.first << " "; });
  WLT_LOG_L0("amount=" << currency::print_money(src.amount) << ", real_output=" << src.real_output << ", real_output_in_tx_index=" << src.real_output_in_tx_index << ", indexes: " << indexes.str());
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_tx_key(const crypto::hash &txid, crypto::secret_key &tx_key) const
{
  const std::unordered_map<crypto::hash, crypto::secret_key>::const_iterator i = m_tx_keys.find(txid);
  if (i == m_tx_keys.end())
    return false;
  tx_key = i->second;
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::prepare_tx_destinations(uint64_t needed_money,
  uint64_t found_money,
  detail::split_strategy_id_t destination_split_strategy_id,
  const tx_dust_policy& dust_policy,
  const std::vector<currency::tx_destination_entry>& dsts,
  std::vector<currency::tx_destination_entry>& final_detinations)
{
  currency::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
  if (needed_money < found_money)
  {
    change_dts.addr.push_back(m_account.get_keys().account_address);
    change_dts.amount = found_money - needed_money;
  }
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(found_money >= needed_money, "needed_money==" << needed_money << "  <  found_money==" << found_money);

  uint64_t dust = 0;
  bool r = detail::apply_split_strategy_by_id(destination_split_strategy_id, dsts, change_dts, dust_policy.dust_threshold, final_detinations, dust, WALLET_MAX_ALLOWED_OUTPUT_AMOUNT);
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "invalid split strategy id: " << destination_split_strategy_id);
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(dust_policy.dust_threshold >= dust, "invalid dust value: dust = " << dust << ", dust_threshold = " << dust_policy.dust_threshold);

  //@#@
#ifdef _DEBUG
  if (final_detinations.size() > 10)
  {
    WLT_LOG_L0("final_detinations.size()=" << final_detinations.size());
  }
#endif
  //@#@

  if (0 != dust && !dust_policy.add_to_fee)
  {
    final_detinations.push_back(currency::tx_destination_entry(dust, dust_policy.addr_for_dust));
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::prepare_transaction(const construct_tx_param& ctp, finalize_tx_param& ftp, const currency::transaction& tx_for_mode_separate /* = currency::transaction() */)
{
  TIME_MEASURE_START_MS(get_needed_money_time);
  uint64_t needed_money = get_needed_money(ctp.fee, ctp.dsts);
  if (ctp.flags & TX_FLAG_SIGNATURE_MODE_SEPARATE && tx_for_mode_separate.vout.size())
  {
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(get_tx_flags(tx_for_mode_separate) & TX_FLAG_SIGNATURE_MODE_SEPARATE, "tx_param.flags differs from tx.flags");
    needed_money += (currency::get_outs_money_amount(tx_for_mode_separate) - get_inputs_money_amount(tx_for_mode_separate));
  }
  TIME_MEASURE_FINISH_MS(get_needed_money_time);

  uint64_t found_money = 0;

  TIME_MEASURE_START_MS(prepare_tx_sources_time);
  if (ctp.perform_packing)
    prepare_tx_sources_for_packing(WALLET_DEFAULT_POS_MINT_PACKING_SIZE, 0, ftp.sources, ftp.selected_transfers, found_money);
  else if (ctp.multisig_id == currency::null_hash)
    prepare_tx_sources(needed_money, ctp.fake_outputs_count, ctp.dust_policy.dust_threshold, ftp.sources, ftp.selected_transfers, found_money);
  else
    prepare_tx_sources(ctp.multisig_id, ftp.sources, found_money);
  TIME_MEASURE_FINISH_MS(prepare_tx_sources_time);

  TIME_MEASURE_START_MS(prepare_tx_destinations_time);
  prepare_tx_destinations(needed_money, found_money, static_cast<detail::split_strategy_id_t>(ctp.split_strategy_id), ctp.dust_policy, ctp.dsts, ftp.prepared_destinations);
  TIME_MEASURE_FINISH_MS(prepare_tx_destinations_time);

  if (ctp.mark_tx_as_complete && !ftp.sources.empty())
    ftp.sources.back().separately_signed_tx_complete = true;


  ftp.unlock_time = ctp.unlock_time;
  ftp.extra = ctp.extra; // TODO consider move semantic
  ftp.attachments = ctp.attachments; // TODO consider move semantic
  ftp.crypt_address = ctp.crypt_address;
  ftp.tx_outs_attr = ctp.tx_outs_attr;
  ftp.shuffle = ctp.shuffle;
  ftp.flags = ctp.flags;
  ftp.multisig_id = ctp.multisig_id;
  ftp.spend_pub_key = m_account.get_public_address().spend_public_key;

  /* TODO
  WLT_LOG_GREEN("[prepare_transaction]: get_needed_money_time: " << get_needed_money_time << " ms"
    << ", prepare_tx_sources_time: " << prepare_tx_sources_time << " ms"
    << ", prepare_tx_destinations_time: " << prepare_tx_destinations_time << " ms"
    << ", construct_tx_time: " << construct_tx_time << " ms"
    << ", sign_ms_input_time: " << sign_ms_input_time << " ms",
    LOG_LEVEL_0);*/
}
//----------------------------------------------------------------------------------------------------
void wallet2::finalize_transaction(const finalize_tx_param& ftp, currency::transaction& tx, crypto::secret_key& tx_key, bool broadcast_tx, bool store_tx_secret_key /* = true */)
{
  // NOTE: if broadcast_tx == true callback rise_on_transfer2() may be called at the end of this function.
  // That callback may call balance(), so it's important to have all used/spending transfers
  // to be correctly marked with corresponding flags PRIOR to calling finalize_transaction()

  // broadcasting tx without secret key storing is forbidden to avoid lost key issues
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(!broadcast_tx || store_tx_secret_key, "finalize_tx is requested to broadcast a tx without storing the key");

  //TIME_MEASURE_START_MS(construct_tx_time);
  bool r = currency::construct_tx(m_account.get_keys(),
    ftp.sources,
    ftp.prepared_destinations,
    ftp.extra,
    ftp.attachments,
    tx,
    tx_key,
    ftp.unlock_time, 
    ftp.crypt_address,
    0, // expiration time
    ftp.tx_outs_attr,
    ftp.shuffle,
    ftp.flags);
  //TIME_MEASURE_FINISH_MS(construct_tx_time);
  THROW_IF_FALSE_WALLET_EX(r, error::tx_not_constructed, ftp.sources, ftp.prepared_destinations, ftp.unlock_time);

  //TIME_MEASURE_START_MS(sign_ms_input_time);
  if (ftp.multisig_id != currency::null_hash)
  {
    // In case there's multisig input is used -- sign it partially with this wallet's keys (we don't have any others here).
    // NOTE: this tx will not be ready to send until all other necessary signs for ms input would made.
    auto it = m_multisig_transfers.find(ftp.multisig_id);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it != m_multisig_transfers.end(), "can't find multisig_id: " << ftp.multisig_id);
    const currency::transaction& ms_source_tx = it->second.m_ptx_wallet_info->m_tx;
    bool is_tx_input_fully_signed = false;
    r = sign_multisig_input_in_tx(tx, 0, m_account.get_keys(), ms_source_tx, &is_tx_input_fully_signed); // it's assumed that ms input is the first one (index 0)
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(r && !is_tx_input_fully_signed, "sign_multisig_input_in_tx failed: r = " << r << ", is_tx_input_fully_signed = " << is_tx_input_fully_signed);
  }
  //TIME_MEASURE_FINISH_MS(sign_ms_input_time);

  THROW_IF_FALSE_WALLET_EX(get_object_blobsize(tx) < CURRENCY_MAX_TRANSACTION_BLOB_SIZE, error::tx_too_big, tx, m_upper_transaction_size_limit);

  if (store_tx_secret_key)
    m_tx_keys.insert(std::make_pair(get_transaction_hash(tx), tx_key));

  //TIME_MEASURE_START(send_transaction_to_network_time);
  if (broadcast_tx)
    send_transaction_to_network(tx);
  //TIME_MEASURE_FINISH(send_transaction_to_network_time);

  //TIME_MEASURE_START(add_sent_tx_detailed_info_time);
  if (broadcast_tx)
    add_sent_tx_detailed_info(tx, ftp.prepared_destinations, ftp.selected_transfers);
  //TIME_MEASURE_FINISH(add_sent_tx_detailed_info_time);

  /* TODO
  WLT_LOG_GREEN("[prepare_transaction]: get_needed_money_time: " << get_needed_money_time << " ms"
  << ", prepare_tx_sources_time: " << prepare_tx_sources_time << " ms"
  << ", prepare_tx_destinations_time: " << prepare_tx_destinations_time << " ms"
  << ", construct_tx_time: " << construct_tx_time << " ms"
  << ", sign_ms_input_time: " << sign_ms_input_time << " ms",
  LOG_LEVEL_0);*/
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, const std::vector<currency::attachment_v>& attachments, detail::split_strategy_id_t destination_split_strategy_id, const tx_dust_policy& dust_policy)
{
  currency::transaction tx;
  transfer(dsts, fake_outputs_count, unlock_time, fee, extra, attachments, destination_split_strategy_id, dust_policy, tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts,
  size_t fake_outputs_count,
  uint64_t unlock_time,
  uint64_t fee,
  const std::vector<currency::extra_v>& extra,
  const std::vector<currency::attachment_v>& attachments,
  detail::split_strategy_id_t destination_split_strategy_id,
  const tx_dust_policy& dust_policy,
  currency::transaction &tx,
  uint8_t tx_outs_attr,
  bool shuffle,
  uint8_t flags,
  bool send_to_network,
  std::string* p_signed_tx_blob_str)
{
  //TIME_MEASURE_START(precalculation_time);
  construct_tx_param ctp = AUTO_VAL_INIT(ctp);
  ctp.attachments = attachments;
  ctp.crypt_address = currency::get_crypt_address_from_destinations(m_account.get_keys(), dsts);
  ctp.dsts = dsts;
  ctp.dust_policy = dust_policy;
  ctp.extra = extra;
  ctp.fake_outputs_count = fake_outputs_count;
  ctp.fee = fee;
  ctp.flags = flags;
  // ctp.mark_tx_as_complete
  // ctp.multisig_id
  ctp.shuffle = shuffle;
  ctp.split_strategy_id = destination_split_strategy_id;
  ctp.tx_outs_attr = tx_outs_attr;
  ctp.unlock_time = unlock_time;
  //TIME_MEASURE_FINISH(precalculation_time);
  transfer(ctp, tx, send_to_network, p_signed_tx_blob_str);
}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
construct_tx_param wallet2::get_default_construct_tx_param_inital()
{
  construct_tx_param ctp = AUTO_VAL_INIT(ctp);

  ctp.fee = m_core_runtime_config.tx_default_fee;
  ctp.dust_policy = tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD);
  ctp.split_strategy_id = tools::detail::ssi_digit;
  ctp.tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED;
  ctp.shuffle = 0;
  return ctp;
}
const construct_tx_param& wallet2::get_default_construct_tx_param()
{
  static construct_tx_param ctp = get_default_construct_tx_param_inital();
  return ctp;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::store_unsigned_tx_to_file_and_reserve_transfers(const finalize_tx_param& ftp, const std::string& filename, std::string* p_unsigned_tx_blob_str /* = nullptr */)
{
  TIME_MEASURE_START(store_unsigned_tx_time);
  blobdata bl = t_serializable_object_to_blob(ftp);
  crypto::chacha_crypt(bl, m_account.get_keys().view_secret_key);

  if (!filename.empty())
  {
    bool r = epee::file_io_utils::save_string_to_file(filename, bl);
    CHECK_AND_ASSERT_MES(r, false, "failed to store unsigned tx to " << filename);
    LOG_PRINT_L0("Transaction stored to " << filename << ". You need to sign this tx using a full-access wallet.");
  }
  else
  {
    CHECK_AND_ASSERT_MES(p_unsigned_tx_blob_str != nullptr, false, "empty filename and p_unsigned_tx_blob_str == null");
    *p_unsigned_tx_blob_str = bl;
  }

  TIME_MEASURE_FINISH(store_unsigned_tx_time);

  // reserve transfers at the very end
  TIME_MEASURE_START(mark_transfers_as_spent_time);
  mark_transfers_with_flag(ftp.selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION, std::string("cold sig reservation for money transfer"), true);
  TIME_MEASURE_FINISH(mark_transfers_as_spent_time);

  WLT_LOG_GREEN("[wallet::store_unsigned_tx_to_file_and_reserve_transfers]"
    << " store_unsigned_tx_time: " << print_fixed_decimal_point(store_unsigned_tx_time, 3)
    << ", mark_transfers_as_spent_time: " << print_fixed_decimal_point(mark_transfers_as_spent_time, 3)
    , LOG_LEVEL_1);
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::check_and_throw_if_self_directed_tx_with_payment_id_requested(const construct_tx_param& ctp)
{
  // If someone sends coins to his own address, all tx outputs will be detected as own outputs.
  // It's totally okay unless payment id is used, because it would be impossible to distinguish
  // between change outs and transfer outs. Thus, such tx with a payment id can't be correctly
  // obtained via RPC by the given payment id. It could be a problem for an exchange or other
  // service when a user, identifyied by payment id sends coins to another user on the same
  // exchange/service. Coins will be received but RPCs like get_payments won't give the transfer.
  // To avoid such issues we prohibit such txs with a soft rule on sender side.

  for (auto& d : ctp.dsts)
  {
    for (auto& addr : d.addr)
    {
      if (addr != m_account.get_public_address())
        return; // at least one destination address is not our address -- it's not self-directed tx
    }
  }

  // it's self-directed tx
  payment_id_t pid;
  bool has_payment_id = get_payment_id_from_tx(ctp.attachments, pid) && !pid.empty();
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(!has_payment_id, "sending funds to yourself with payment id is not allowed");
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const construct_tx_param& ctp,
  currency::transaction &tx,
  bool send_to_network,
  std::string* p_signed_tx_blob_str)
{
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(!is_auditable() || !is_watch_only(), "You can't initiate coins transfer using an auditable watch-only wallet."); // btw, watch-only wallets can call transfer() within cold-signing process

  check_and_throw_if_self_directed_tx_with_payment_id_requested(ctp);

  TIME_MEASURE_START(prepare_transaction_time);
  finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  prepare_transaction(ctp, ftp);
  TIME_MEASURE_FINISH(prepare_transaction_time);

  if (m_watch_only)
  {
    bool r = store_unsigned_tx_to_file_and_reserve_transfers(ftp, "zano_tx_unsigned", p_signed_tx_blob_str);
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "failed to store unsigned tx");
    WLT_LOG_GREEN("[wallet::transfer]" << " prepare_transaction_time: " << print_fixed_decimal_point(prepare_transaction_time, 3), LOG_LEVEL_0);
    return;
  }

  TIME_MEASURE_START(mark_transfers_as_spent_time);
  mark_transfers_as_spent(ftp.selected_transfers, std::string("money transfer, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(tx)));
  TIME_MEASURE_FINISH(mark_transfers_as_spent_time);

  TIME_MEASURE_START(finalize_transaction_time);
  try
  {
    crypto::secret_key sk = AUTO_VAL_INIT(sk);
    finalize_transaction(ftp, tx, sk, send_to_network);
  }
  catch (...)
  {
    clear_transfers_from_flag(ftp.selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_SPENT, std::string("exception on money transfer, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(tx)));
    throw;
  }
  TIME_MEASURE_FINISH(finalize_transaction_time);


  WLT_LOG_GREEN("[wallet::transfer]"
    //<< "  precalculation_time: " << print_fixed_decimal_point(precalculation_time, 3)
    << ", prepare_transaction_time: " << print_fixed_decimal_point(prepare_transaction_time, 3)
    << ", finalize_transaction_time: " << print_fixed_decimal_point(finalize_transaction_time, 3)
    << ", mark_transfers_as_spent_time: " << print_fixed_decimal_point(mark_transfers_as_spent_time, 3)
    , LOG_LEVEL_0);

  print_tx_sent_message(tx, std::string() + "(transfer)", ctp.fee);
}
//----------------------------------------------------------------------------------------------------
void wallet2::sweep_below(size_t fake_outs_count, const currency::account_public_address& destination_addr, uint64_t threshold_amount, const currency::payment_id_t& payment_id,
  uint64_t fee, size_t& outs_total, uint64_t& amount_total, size_t& outs_swept, currency::transaction* p_result_tx /* = nullptr */, std::string* p_filename_or_unsigned_tx_blob_str /* = nullptr */)
{
  static const size_t estimated_bytes_per_input = 78;
  const size_t estimated_max_inputs = static_cast<size_t>(CURRENCY_MAX_TRANSACTION_BLOB_SIZE / (estimated_bytes_per_input * (fake_outs_count + 1.5))); // estimated number of maximum tx inputs under the tx size limit
  const size_t tx_sources_for_querying_random_outs_max = estimated_max_inputs * 2;

  bool r = false;
  outs_total = 0;
  amount_total = 0;
  outs_swept = 0;

  std::vector<size_t> selected_transfers;
  selected_transfers.reserve(m_transfers.size());
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    uint64_t amount = td.amount();
    if (amount < threshold_amount &&
      is_transfer_ready_to_go(td, fake_outs_count))
    {
      selected_transfers.push_back(i);
      outs_total += 1;
      amount_total += amount;
    }
  }

  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(!selected_transfers.empty(), "No spendable outputs meet the criterion");

  // sort by amount descending in order to spend bigger outputs first
  std::sort(selected_transfers.begin(), selected_transfers.end(), [this](size_t a, size_t b) { return m_transfers[b].amount() < m_transfers[a].amount(); });

  // limit RPC request with reasonable number of sources
  if (selected_transfers.size() > tx_sources_for_querying_random_outs_max)
    selected_transfers.erase(selected_transfers.begin() + tx_sources_for_querying_random_outs_max, selected_transfers.end());

  //
  // TODO: prefetch gindexes here for each element of selected_transfers
  //

  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
  typedef currency::tx_source_entry::output_entry tx_output_entry;

  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response rpc_get_random_outs_resp = AUTO_VAL_INIT(rpc_get_random_outs_resp);
  if (fake_outs_count > 0)
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
    req.use_forced_mix_outs = false;
    req.outs_count = fake_outs_count + 1;
    for (size_t i : selected_transfers)
      req.amounts.push_back(m_transfers[i].amount());

    r = m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(req, rpc_get_random_outs_resp);
    
    THROW_IF_FALSE_WALLET_EX(r, error::no_connection_to_daemon, "getrandom_outs.bin");
    THROW_IF_FALSE_WALLET_EX(rpc_get_random_outs_resp.status != API_RETURN_CODE_BUSY, error::daemon_busy, "getrandom_outs.bin");
    THROW_IF_FALSE_WALLET_EX(rpc_get_random_outs_resp.status == API_RETURN_CODE_OK, error::get_random_outs_error, rpc_get_random_outs_resp.status);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(rpc_get_random_outs_resp.outs.size() == selected_transfers.size(),
      "daemon returned wrong number of amounts for getrandom_outs.bin: " << rpc_get_random_outs_resp.outs.size() << ", requested: " << selected_transfers.size());

    std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> scanty_outs;
    for (COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& amount_outs : rpc_get_random_outs_resp.outs)
    {
      if (amount_outs.outs.size() < fake_outs_count)
        scanty_outs.push_back(amount_outs);
    }
    THROW_IF_FALSE_WALLET_EX(scanty_outs.empty(), error::not_enough_outs_to_mix, scanty_outs, fake_outs_count);
  }

  finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  if (!payment_id.empty())
    set_payment_id_to_tx(ftp.attachments, payment_id);
  // put encrypted payer info into the extra
  ftp.crypt_address = destination_addr;
  
  currency::create_and_add_tx_payer_to_container_from_address(ftp.extra, m_account.get_public_address(), get_top_block_height(), m_core_runtime_config);
  
  ftp.flags = 0;
  // ftp.multisig_id -- not required
  // ftp.prepared_destinations -- will be filled by prepare_tx_destinations
  // ftp.selected_transfers -- needed only at stage of broadcasting or storing unsigned tx
  ftp.shuffle = false;
  // ftp.sources -- will be filled in try_construct_tx
  ftp.spend_pub_key = m_account.get_public_address().spend_public_key; // needed for offline signing
  ftp.tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED;
  ftp.unlock_time = 0;
  
  enum try_construct_result_t {rc_ok = 0, rc_too_few_outputs = 1, rc_too_many_outputs = 2, rc_create_tx_failed = 3 };
  auto get_result_t_str = [](try_construct_result_t t) -> const char*
  { return t == rc_ok ? "rc_ok" : t == rc_too_few_outputs ? "rc_too_few_outputs" : t == rc_too_many_outputs ? "rc_too_many_outputs" : t == rc_create_tx_failed ? "rc_create_tx_failed" : "unknown"; };

  auto try_construct_tx = [this, &selected_transfers, &rpc_get_random_outs_resp, &fake_outs_count, &fee, &destination_addr]
    (size_t st_index_upper_boundary, finalize_tx_param& ftp, uint64_t& amount_swept) -> try_construct_result_t
  {
    // prepare inputs
    amount_swept = 0;
    ftp.sources.clear();
    ftp.sources.resize(st_index_upper_boundary);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(st_index_upper_boundary <= selected_transfers.size(), "index_upper_boundary = " << st_index_upper_boundary << ", selected_transfers.size() = " << selected_transfers.size());
    for (size_t st_index = 0; st_index < st_index_upper_boundary; ++st_index)
    {
      currency::tx_source_entry& src = ftp.sources[st_index];
      size_t tr_index = selected_transfers[st_index];
      transfer_details& td = m_transfers[tr_index];
      src.transfer_index = tr_index;
      src.amount = td.amount();
      amount_swept += src.amount;
      
      // populate src.outputs with mix-ins
      if (rpc_get_random_outs_resp.outs.size())
      {
        rpc_get_random_outs_resp.outs[st_index].outs.sort([](const out_entry& a, const out_entry& b) { return a.global_amount_index < b.global_amount_index; });
        for (out_entry& daemon_oe : rpc_get_random_outs_resp.outs[st_index].outs)
        {
          if (td.m_global_output_index == daemon_oe.global_amount_index)
            continue;
          tx_output_entry oe;
          oe.first = daemon_oe.global_amount_index;
          oe.second = daemon_oe.out_key;
          src.outputs.push_back(oe);
          if (src.outputs.size() >= fake_outs_count)
            break;
        }
      }

      // insert real output into src.outputs
      auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
      {
        if (a.first.type().hash_code() == typeid(uint64_t).hash_code())
          return boost::get<uint64_t>(a.first) >= td.m_global_output_index;
        return false; // TODO: implement deterministics real output placement in case there're ref_by_id outs
      });
      tx_output_entry real_oe;
      real_oe.first = td.m_global_output_index;
      real_oe.second = boost::get<txout_to_key>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].target).key;
      auto inserted_it = src.outputs.insert(it_to_insert, real_oe);
      src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_ptx_wallet_info->m_tx);
      src.real_output = inserted_it - src.outputs.begin();
      src.real_output_in_tx_index = td.m_internal_output_index;
      //detail::print_source_entry(src);
    }

    if (amount_swept <= fee)
      return rc_too_few_outputs;

    // try to construct a transaction
    std::vector<currency::tx_destination_entry> dsts({ tx_destination_entry(amount_swept - fee, destination_addr) });
    prepare_tx_destinations(0, 0, detail::ssi_digit, tools::tx_dust_policy(), dsts, ftp.prepared_destinations);

    currency::transaction tx = AUTO_VAL_INIT(tx);
    crypto::secret_key tx_key = AUTO_VAL_INIT(tx_key);
    try
    {
      finalize_transaction(ftp, tx, tx_key, false, false);
    }
    catch (error::tx_too_big&)
    {
      return rc_too_many_outputs;
    }
    catch (...)
    {
      return rc_create_tx_failed;
    }

    return rc_ok;
  };

  size_t st_index_upper_boundary = std::min(selected_transfers.size(), estimated_max_inputs);
  uint64_t amount_swept = 0;
  try_construct_result_t res = try_construct_tx(st_index_upper_boundary, ftp, amount_swept);
  
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(res != rc_too_few_outputs, st_index_upper_boundary << " biggest unspent outputs have total amount of " << print_money_brief(amount_swept)
    << " which is less than required fee: " << print_money_brief(fee) << ", transaction cannot be constructed");
  
  if (res == rc_too_many_outputs)
  {
    WLT_LOG_L1("sweep_below: first try of try_construct_tx(" << st_index_upper_boundary << ") returned " << get_result_t_str(res));
    size_t low_bound = 0;
    size_t high_bound = st_index_upper_boundary;
    finalize_tx_param ftp_ok = ftp;
    for (;;)
    {
      if (low_bound + 1 >= high_bound)
      {
        st_index_upper_boundary = low_bound;
        res = rc_ok;
        ftp = ftp_ok;
        break;
      }
      st_index_upper_boundary = (low_bound + high_bound) / 2;
      try_construct_result_t res = try_construct_tx(st_index_upper_boundary, ftp, amount_swept);
      WLT_LOG_L1("sweep_below: try_construct_tx(" << st_index_upper_boundary << ") returned " << get_result_t_str(res));
      if (res == rc_ok)
      {
        low_bound = st_index_upper_boundary;
        ftp_ok = ftp;
      }
      else if (res == rc_too_many_outputs)
      {
        high_bound = st_index_upper_boundary;
      }
      else
        break;
    }
  }

  if (res != rc_ok)
  {
    uint64_t amount_min = UINT64_MAX, amount_max = 0, amount_sum = 0;
    for (auto& i : selected_transfers)
    {
      uint64_t amount = m_transfers[i].amount();
      amount_min = std::min(amount_min, amount);
      amount_max = std::max(amount_max, amount);
      amount_sum += amount;
    }
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(false, "try_construct_tx failed with result: " << get_result_t_str(res) << " (" << res << ")" <<
      ", selected_transfers stats:\n" <<
      "  outs:       " << selected_transfers.size() << ENDL <<
      "  amount min: " << print_money(amount_min) << ENDL <<
      "  amount max: " << print_money(amount_max) << ENDL <<
      "  amount avg: " << (selected_transfers.empty() ? std::string("n/a") : print_money(amount_sum / selected_transfers.size())));
  }

  // populate ftp.selected_transfers from ftp.sources
  ftp.selected_transfers.clear();
  for (size_t i = 0; i < ftp.sources.size(); ++i)
    ftp.selected_transfers.push_back(ftp.sources[i].transfer_index);

  outs_swept = ftp.sources.size();
  

  if (m_watch_only)
  {
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(p_filename_or_unsigned_tx_blob_str != nullptr, "p_filename_or_unsigned_tx_blob_str is null");
    bool r = store_unsigned_tx_to_file_and_reserve_transfers(ftp, *p_filename_or_unsigned_tx_blob_str, p_filename_or_unsigned_tx_blob_str);
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "failed to store unsigned tx");
    return;
  }

  mark_transfers_as_spent(ftp.selected_transfers, "sweep_below");

  transaction local_tx;
  transaction* p_tx = p_result_tx != nullptr ? p_result_tx : &local_tx;
  *p_tx = AUTO_VAL_INIT_T(transaction);
  try
  {
    crypto::secret_key sk = AUTO_VAL_INIT(sk);
    finalize_transaction(ftp, *p_tx, sk, true);
  }
  catch (...)
  {
    clear_transfers_from_flag(ftp.selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_SPENT, std::string("exception on sweep_below, tx id (might be wrong): ") + epee::string_tools::pod_to_hex(get_transaction_hash(*p_tx)));
    throw;
  }
  

}

} // namespace tools
