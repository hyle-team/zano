// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <numeric>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include <boost/utility/value_init.hpp>
#include "include_base_utils.h"
#include "net/levin_client.h"
using namespace epee;

#include "string_coding.h"
#define KEEP_WALLET_LOG_MACROS
#include "wallet2.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/bc_offers_service_basic.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "misc_language.h"
#include "common/util.h"

#include "common/boost_serialization_helper.h"
#include "crypto/crypto.h"
#include "serialization/binary_utils.h"
#include "currency_core/bc_payments_id_service.h"
#include "version.h"
#include "common/encryption_filter.h"
#include "crypto/bitcoin/sha256_helper.h"

#ifndef DISABLE_TOR
#  define DISABLE_TOR
#endif

#ifndef DISABLE_TOR
  #include "common/tor_helper.h"
#endif

#include "storages/levin_abstract_invoke2.h"
#include "common/variant_helper.h"
#include "currency_core/crypto_config.h"
#include "crypto/zarcanum.h"
#include "wallet_debug_events_definitions.h"
#include "decoy_selection.h"

using namespace currency;

#define SET_CONTEXT_OBJ_FOR_SCOPE(name, obj)  m_current_context.name = &obj; \
    auto COMBINE(auto_scope_var_, __LINE__) = epee::misc_utils::create_scope_leave_handler([&]() { m_current_context.name = nullptr; });


#define MINIMUM_REQUIRED_WALLET_FREE_SPACE_BYTES (100*1024*1024) // 100 MB

#define WALLET_TX_MAX_ALLOWED_FEE                                     (COIN * 100)

#define WALLET_FETCH_RANDOM_OUTS_SIZE                                 200  



#undef LOG_DEFAULT_CHANNEL
#define LOG_DEFAULT_CHANNEL "wallet"
ENABLE_CHANNEL_BY_DEFAULT("wallet")
namespace tools
{
  wallet2::wallet2()
    : m_stop(false)
    , m_wcallback(new i_wallet2_callback()) //stub
    , m_core_proxy(new default_http_core_proxy())
    , m_upper_transaction_size_limit(0)
    , m_fake_outputs_count(0)
    , m_do_rise_transfer(false)
    , m_log_prefix("???")
    , m_watch_only(false)
    , m_required_decoys_count(CURRENCY_DEFAULT_DECOY_SET_SIZE)
    , m_defragmentation_tx_enabled(false)
    , m_max_allowed_output_amount_for_defragmentation_tx(CURRENCY_BLOCK_REWARD)
    , m_min_utxo_count_for_defragmentation_tx(0)
    , m_max_utxo_count_for_defragmentation_tx(0)
    , m_decoys_count_for_defragmentation_tx(SIZE_MAX)
    , m_use_deffered_global_outputs(false)
#ifdef DISABLE_TOR
    , m_disable_tor_relay(true)
#else
    , m_disable_tor_relay(false)
#endif
    , m_votes_config_path(tools::get_default_data_dir() + "/" + CURRENCY_VOTING_CONFIG_DEFAULT_FILENAME)
  {
    m_core_runtime_config = currency::get_default_core_runtime_config();
  }
  //---------------------------------------------------------------
  uint64_t wallet2::get_max_unlock_time_from_receive_indices(const currency::transaction& tx, const wallet_public::employed_tx_entries& td)
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

    for (auto r : td.receive)
    {
      uint64_t ri = r.index;
      CHECK_AND_ASSERT_THROW_MES(ri < tx.vout.size(), "Internal error: wrong tx transfer details: reciev index=" << ri << " is greater than transaction outputs vector " << tx.vout.size());
      VARIANT_SWITCH_BEGIN(tx.vout[ri]);
      VARIANT_CASE_CONST(tx_out_bare, o)
        if (o.target.type() == typeid(currency::txout_to_key))
        {
          //update unlock_time if needed
          if (ut2.unlock_time_array[ri] > max_unlock_time)
            max_unlock_time = ut2.unlock_time_array[ri];
        }
      VARIANT_CASE_CONST(tx_out_zarcanum, o);
      VARIANT_SWITCH_END();

    }
    return max_unlock_time;
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
void wallet2::init(const std::string& daemon_address)
{
  //m_miner_text_info = PROJECT_VERSION_LONG;
  m_core_proxy->set_connection_addr(daemon_address);
  m_core_proxy->check_connection();

  std::stringstream ss;
  const tools::wallet_public::wallet_vote_config& votes = this->get_current_votes();
  if (votes.entries.size())
  {
    ss << "VOTING SET LOADED:";
    for (const auto& e : votes.entries)
    {
      ss << "\t\t" << e.proposal_id << "\t\t" << (e.vote ? "1" : "0") << "\t\t(" << e.h_start << " - " << e.h_end << ")";
    }
  }
  WLT_LOG_L0(ss.str());

}
//----------------------------------------------------------------------------------------------------
bool wallet2::set_core_proxy(const std::shared_ptr<i_core_proxy>& proxy)
{
  THROW_IF_TRUE_WALLET_EX(!proxy, error::wallet_internal_error, "Trying to set null core proxy.");
  m_core_proxy = proxy;
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_defragmentation_tx_settings(bool enabled, uint64_t min_outs, uint64_t max_outs, uint64_t max_allowed_amount, size_t decoys_count)
{
  m_defragmentation_tx_enabled                        = enabled;
  m_min_utxo_count_for_defragmentation_tx             = min_outs;
  m_max_utxo_count_for_defragmentation_tx             = max_outs;
  m_max_allowed_output_amount_for_defragmentation_tx  = max_allowed_amount;
  m_decoys_count_for_defragmentation_tx               = decoys_count;
  if (enabled)
  {
    WLT_LOG_L0("Defragmentation tx creation is enabled, settings: min outs: " << min_outs << ", max outs: " << max_outs << ", max amount: " << print_money_brief(max_allowed_amount) <<
      ", decoys: " << (decoys_count != SIZE_MAX ? epee::string_tools::num_to_string_fast(decoys_count) : std::string("default")));
  }
  else
  {
    WLT_LOG_L0("Defragmentation tx creation is disabled");
  }
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

bool out_is_to_key(const currency::tx_out_v& out_t)
{
  if (out_t.type() == typeid(currency::tx_out_bare))
  {
    return boost::get<currency::tx_out_bare>(out_t).target.type() == typeid(currency::txout_to_key);
  }
  return false;
}

bool out_is_multisig(const currency::tx_out_v& out_t)
{
  if (out_t.type() == typeid(currency::tx_out_bare))
  {
    return boost::get<currency::tx_out_bare>(out_t).target.type() == typeid(currency::txout_multisig);
  }
  return false;
}

bool out_is_to_htlc(const currency::tx_out_v& out_t)
{
  if (out_t.type() == typeid(currency::tx_out_bare))
  {
    return boost::get<currency::tx_out_bare>(out_t).target.type() == typeid(currency::txout_htlc);
  }
  return false;
}

bool out_is_zc(const currency::tx_out_v& out_t)
{
  return out_t.type() == typeid(currency::tx_out_zarcanum);
}

const currency::txout_htlc& out_get_htlc(const currency::tx_out_v& out_t)
{
  return boost::get<currency::txout_htlc>(boost::get<currency::tx_out_bare>(out_t).target);
}

const crypto::public_key& wallet2::out_get_pub_key(const currency::tx_out_v& out_t, std::list<currency::htlc_info>& htlc_info_list)
{
  if (out_t.type() == typeid(tx_out_bare))
  {
    const currency::tx_out_bare& out = boost::get<currency::tx_out_bare>(out_t);
    if (out.target.type() == typeid(currency::txout_to_key))
    {
      return boost::get<currency::txout_to_key>(out.target).key;
    }
    else
    {
      THROW_IF_FALSE_WALLET_INT_ERR_EX(out.target.type() == typeid(currency::txout_htlc), "Unexpected out type in target wallet: " << out.target.type().name());
      THROW_IF_FALSE_WALLET_INT_ERR_EX(htlc_info_list.size() > 0, "Found txout_htlc out but htlc_info_list is empty");
      bool hltc_our_out_is_before_expiration = htlc_info_list.front().hltc_our_out_is_before_expiration;
      htlc_info_list.pop_front();
      if (hltc_our_out_is_before_expiration)
      {
        return boost::get<currency::txout_htlc>(out.target).pkey_redeem;
      }
      else
      {
        return boost::get<currency::txout_htlc>(out.target).pkey_refund;
      }
    }
  }
  else
  {
    THROW_IF_FALSE_WALLET_INT_ERR_EX(out_t.type() == typeid(currency::tx_out_zarcanum), "Unexpected out type im wallet: " << out_t.type().name());
    return boost::get<currency::tx_out_zarcanum>(out_t).stealth_address;
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_ado_in_new_transaction(const currency::asset_descriptor_operation& ado, process_transaction_context& ptc)
{
  do
  {
    crypto::public_key asset_id{};
    if (ado.operation_type != ASSET_DESCRIPTOR_OPERATION_UNDEFINED)
      WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(get_or_calculate_asset_id(ado, nullptr, &asset_id), "get_or_calculate_asset_id failed");

    if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_REGISTER)
    {
      if (ado.descriptor.owner != m_account.get_public_address().spend_public_key)
        break;

      WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(m_own_asset_descriptors.count(asset_id) == 0, "asset with asset_id " << asset_id << " has already been registered in the wallet as own asset");
      wallet_own_asset_context& asset_context = m_own_asset_descriptors[asset_id];
      epee::misc_utils::cast_assign_a_to_b(asset_context, ado.descriptor);
      //*static_cast<asset_descriptor_base*>(&asset_context) = ado.descriptor;

      std::stringstream ss;
      ss << "New Asset Registered:"
        << ENDL << "asset id:         " << asset_id
        << ENDL << "Name:             " << asset_context.full_name
        << ENDL << "Ticker:           " << asset_context.ticker
        << ENDL << "Total Max Supply: " << print_asset_money(asset_context.total_max_supply, asset_context.decimal_point)
        << ENDL << "Current Supply:   " << print_asset_money(asset_context.current_supply, asset_context.decimal_point)
        << ENDL << "Decimal Point:    " << (int)asset_context.decimal_point;

      
      add_rollback_event(ptc.height, asset_register_event{ asset_id });
      WLT_LOG_MAGENTA(ss.str(), LOG_LEVEL_0);
      if (m_wcallback)
        m_wcallback->on_message(i_wallet2_callback::ms_yellow, ss.str());
    }
    else if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_EMIT || ado.operation_type == ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN)
    {
      auto it = m_own_asset_descriptors.find(asset_id);
      if (it == m_own_asset_descriptors.end())
        break;
      //asset had been updated
      add_rollback_event(ptc.height, asset_update_event{ it->first, it->second });
      epee::misc_utils::cast_assign_a_to_b(it->second, ado.descriptor);
      
    }
    else if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_UPDATE )
    {
      auto  it = m_own_asset_descriptors.find(asset_id);
      if (it == m_own_asset_descriptors.end())
      {
        if (ado.descriptor.owner == m_account.get_public_address().spend_public_key)
        {
          // ownership of the asset acquired

          wallet_own_asset_context& asset_context = m_own_asset_descriptors[asset_id];
          epee::misc_utils::cast_assign_a_to_b(asset_context, ado.descriptor);

          std::stringstream ss;
          ss << "Asset ownership acquired:"
            << ENDL << "asset id:         " << asset_id
            << ENDL << "Name:             " << ado.descriptor.full_name
            << ENDL << "Ticker:           " << ado.descriptor.ticker
            << ENDL << "Total Max Supply: " << print_asset_money(ado.descriptor.total_max_supply, ado.descriptor.decimal_point)
            << ENDL << "Current Supply:   " << print_asset_money(ado.descriptor.current_supply, ado.descriptor.decimal_point)
            << ENDL << "Decimal Point:    " << (int)ado.descriptor.decimal_point;


          add_rollback_event(ptc.height, asset_register_event{ asset_id });
          WLT_LOG_MAGENTA(ss.str(), LOG_LEVEL_0);
          if (m_wcallback)
            m_wcallback->on_message(i_wallet2_callback::ms_yellow, ss.str());
        }
        else
        {
          // update event of the asset that we not control, skip
          break;
        }
      }
      else
      {
        //update event for asset that we control, check if ownership is still ours
        if (ado.descriptor.owner != m_account.get_public_address().spend_public_key && !it->second.thirdparty_custody)
        {
          //ownership of the asset had been transfered
          add_rollback_event(ptc.height, asset_unown_event{ it->first, it->second });
          m_own_asset_descriptors.erase(it);

          std::stringstream ss;
          ss << "Asset ownership lost:"
            << ENDL << "asset id:         " << asset_id
            << ENDL << "New owner:        " << ado.descriptor.owner
            << ENDL << "Name:             " << ado.descriptor.full_name
            << ENDL << "Ticker:           " << ado.descriptor.ticker
            << ENDL << "Total Max Supply: " << print_asset_money(ado.descriptor.total_max_supply, ado.descriptor.decimal_point)
            << ENDL << "Current Supply:   " << print_asset_money(ado.descriptor.current_supply, ado.descriptor.decimal_point)
            << ENDL << "Decimal Point:    " << (int)ado.descriptor.decimal_point;

          add_rollback_event(ptc.height, asset_register_event{ asset_id });
          WLT_LOG_MAGENTA(ss.str(), LOG_LEVEL_0);
          if (m_wcallback)
            m_wcallback->on_message(i_wallet2_callback::ms_yellow, ss.str());
        }
        else
        {
          //just an update of the asset
          add_rollback_event(ptc.height, asset_update_event{ it->first, it->second });
          epee::misc_utils::cast_assign_a_to_b(it->second, ado.descriptor);
          
        }
      }
    }
  } while (false);
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_rollback_event(uint64_t h, const wallet_event_t& ev)
{
  m_rollback_events.emplace_back(h, ev);
}
//----------------------------------------------------------------------------------------------------
#define M_LAST_ZC_GLOBAL_INDEXS_MAX_SIZE                    30
void wallet2::add_to_last_zc_global_indexs(uint64_t h, uint64_t last_zc_output_index)
{
  if (m_last_zc_global_indexs.size())
  {
    if (h > m_last_zc_global_indexs.begin()->first)
    {
      //new block added on top of last one, simply add new record
      m_last_zc_global_indexs.push_front(std::make_pair(h, last_zc_output_index));
    }
    else if (h < m_last_zc_global_indexs.begin()->first)
    {
      //looks like reorganize, pop all records before 
      while (m_last_zc_global_indexs.size() && m_last_zc_global_indexs.begin()->first >= h)
      {
        m_last_zc_global_indexs.erase(m_last_zc_global_indexs.begin());
      }
      m_last_zc_global_indexs.push_front(std::make_pair(h, last_zc_output_index));
    }
    else
    {
      //@#@
#ifdef _DEBUG
      if (m_last_zc_global_indexs.begin()->second > last_zc_output_index)
      {
        LOG_ERROR("!!!!!!!!!!!!!!!!!");
      }
#endif
      //equals, same h but new last_zc_output_index, just update it, should be always bigger then prev
      WLT_THROW_IF_FALSE_WITH_CODE(m_last_zc_global_indexs.begin()->second <= last_zc_output_index,
        "condition m_last_zc_global_indexs.begin()->second " << m_last_zc_global_indexs.begin()->second << " <= last_zc_output_index " << last_zc_output_index << " failed", API_RETURN_CODE_INTERNAL_ERROR);
      m_last_zc_global_indexs.begin()->second = last_zc_output_index;
    }
  }
  else
  {
    //new block added on top of last one, simply add new record
    m_last_zc_global_indexs.push_front(std::make_pair(h, last_zc_output_index));
  }

  if (m_last_zc_global_indexs.size() > M_LAST_ZC_GLOBAL_INDEXS_MAX_SIZE)
    m_last_zc_global_indexs.pop_back();
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_actual_zc_global_index()
{
  WLT_THROW_IF_FALSE_WITH_CODE(m_last_zc_global_indexs.size(), "m_last_zc_global_indexs is empty", API_RETURN_CODE_INTERNAL_ERROR);
  for (auto it = m_last_zc_global_indexs.begin(); it != m_last_zc_global_indexs.end(); it++)
  {
    if (it->first <= m_last_known_daemon_height - WALLET_DEFAULT_TX_SPENDABLE_AGE)
    {
      return it->second;
    }
  }
  WLT_THROW_IF_FALSE_WITH_CODE(false, "doesn't have anything that match expected height = " << m_last_known_daemon_height - WALLET_DEFAULT_TX_SPENDABLE_AGE, API_RETURN_CODE_INTERNAL_ERROR);
  throw std::runtime_error(""); //mostly to suppress compiler warning 
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_new_transaction(const currency::transaction& tx, uint64_t height, const currency::block& b, const std::vector<uint64_t>* pglobal_indexes)
{
  //check for transaction spends
  process_transaction_context ptc(tx);

  process_unconfirmed(tx, ptc.recipients, ptc.remote_aliases);

  // check all outputs for spending (compare key images)
  ptc.coin_base_tx = is_coinbase(tx, ptc.is_pos_coinbase);
  //PoW block don't have change, so all outs supposed to be marked as "mined"
  ptc.is_derived_from_coinbase = !ptc.is_pos_coinbase;
  ptc.height = height;
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(pglobal_indexes, "pglobal_indexes not set");
  if (this->is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM))
  {
    if (pglobal_indexes->size())
    {
      //@#@
      WLT_LOG_L2("add_to_last_zc_global_indexs: h: " << height << ", b: " << currency::get_block_hash(b) << " , tx: " << currency::get_transaction_hash(tx) << ", last_zc_output_index: " << pglobal_indexes->back());
      add_to_last_zc_global_indexs(ptc.height, pglobal_indexes->back());
    }
  }

  for(auto& in : tx.vin)
  {
    ptc.sub_i = 0;
    VARIANT_SWITCH_BEGIN(in);
    VARIANT_CASE_CONST(currency::txin_to_key, intk)
    {
      process_input_t(intk, ptc, tx);
    }
    VARIANT_CASE_CONST(currency::txin_zc_input, in_zc)
    {
      process_input_t(in_zc, ptc, tx);
      //ptc.sub_i++;
    }
    VARIANT_CASE_CONST(currency::txin_multisig, inms)
    {
      crypto::hash multisig_id = inms.multisig_out_id;
      auto it = m_multisig_transfers.find(multisig_id);
      if (it != m_multisig_transfers.end())
      {
        it->second.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;
        it->second.m_spent_height = height;
        WLT_LOG_L0("Spent multisig out: " << multisig_id << ", amount: " << print_money(currency::get_amount_from_variant(in)) << ", with tx: " << ptc.tx_hash() << ", at height " << height);
        ptc.employed_entries.spent.push_back(wallet_public::employed_tx_entry{ ptc.i });
      }
    }
    VARIANT_CASE_CONST(currency::txin_htlc, in_htlc)
    {
      if (in_htlc.key_offsets.size() != 1)
      {
        LOG_ERROR("in_htlc.key_offsets.size() != 1, skip inout");
        continue;
      }

      if (in_htlc.key_offsets[0].type() != typeid(uint64_t))
      {
        LOG_ERROR("HTLC with ref_by_id is not supported by wallet yet");
        continue;
      }

      auto it = m_active_htlcs.find(std::make_pair(in_htlc.amount, boost::get<uint64_t>(in_htlc.key_offsets[0])));
      if (it != m_active_htlcs.end())
      {
        transfer_details& td = m_transfers[it->second];
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(td.m_ptx_wallet_info->m_tx.vout.size() > td.m_internal_output_index, "Internal error: wrong td.m_internal_output_index: " << td.m_internal_output_index);
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].type() == typeid(tx_out_bare), "Internal error: wrong output type: " << td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].type().name());
        const boost::typeindex::type_info& ti = boost::get<tx_out_bare>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]).target.type();
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(ti == typeid(txout_htlc), "Internal error: wrong type of output's target: " << ti.name());
        //input spend active htlc
        m_transfers[it->second].m_spent_height = height;
        transfer_details_extra_option_htlc_info& tdeohi = get_or_add_field_to_variant_vector<transfer_details_extra_option_htlc_info>(td.varian_options);
        tdeohi.origin = in_htlc.hltc_origin;
        tdeohi.redeem_tx_id = ptc.tx_hash();
      }
    }
    VARIANT_SWITCH_END();
    ptc.i++;
  }

  /*
  collect unlock_time from every output that transfered coins to this account and use maximum of
  all values m_payments entry, use this strict policy is required to protect exchanges from being feeded with
  useless outputs
  */
  ptc.max_out_unlock_time = 0;

  std::vector<wallet_out_info> outs;
  //uint64_t sum_of_native_outs = 0;   // TODO:  @#@# correctly calculate tx_money_got_in_outs for post-HF4
  ptc.tx_pub_key = null_pkey;
  bool r = parse_and_validate_tx_extra(tx, ptc.tx_pub_key);
  THROW_IF_TRUE_WALLET_EX(!r, error::tx_extra_parse_error, tx);

  //check for transaction income
  crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
  std::list<htlc_info> htlc_info_list;
  r = lookup_acc_outs(m_account.get_keys(), tx, ptc.tx_pub_key, outs, derivation, htlc_info_list);
  THROW_IF_TRUE_WALLET_EX(!r, error::acc_outs_lookup_error, tx, ptc.tx_pub_key, m_account.get_keys());

  if (!outs.empty())
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

    if (!pglobal_indexes || (pglobal_indexes->size() == 0 && tx.vout.size() != 0))
    {
      //if tx contain htlc_out, then we would need global_indexes anyway, to be able later detect redeem of htlc 
      if (m_use_deffered_global_outputs && htlc_info_list.size() == 0)
      {
        pglobal_indexes = nullptr;
      }
      else
      {
        fetch_tx_global_indixes(tx, outputs_index_local);
        pglobal_indexes = &outputs_index_local;
      }
    }
   
    for (size_t i_in_outs = 0; i_in_outs != outs.size(); i_in_outs++)
    {
      const wallet_out_info& out = outs[i_in_outs];
      size_t o = out.index;
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(o < tx.vout.size(), "wrong out in transaction: internal index: " << o << ", tx.vout.size(): " << tx.vout.size());
      {
        const currency::tx_out_v& out_v = tx.vout[o];
        bool out_type_zc        = out_is_zc(out_v);
        bool out_type_to_key    = out_is_to_key(out_v);
        bool out_type_htlc      = out_is_to_htlc(out_v);
        bool out_type_multisig  = out_is_multisig(out_v);

        if (out_type_zc || out_type_to_key || out_type_htlc)
        {
          crypto::public_key out_key = out_get_pub_key(out_v, htlc_info_list); // htlc_info_list contains information about which one, redeem or refund key is ours for an htlc output

          // obtain key image for this output
          crypto::key_image ki = currency::null_ki;
          if (m_watch_only)
          {
            if (!is_auditable())
            {
              // don't have spend secret key, so we unable to calculate key image for an output
              // look it up in special container instead
              auto it = m_pending_key_images.find(out_key);
              if (it != m_pending_key_images.end())
              {
                ki = it->second;
                WLT_LOG_L1("pending key image " << ki << " was found by out pub key " << out_key);
              }
              else
              {
                ki = currency::null_ki;
                WLT_LOG_L1("can't find pending key image by out pub key: " << out_key << ", key image temporarily set to null");
              }
            }
          }
          else
          {
            // normal wallet, calculate and store key images for own outs
            currency::keypair in_ephemeral = AUTO_VAL_INIT(in_ephemeral);
            currency::generate_key_image_helper(m_account.get_keys(), ptc.tx_pub_key, o, in_ephemeral, ki);
            WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(in_ephemeral.pub == out_key, "key_image generated ephemeral public key that does not match with output_key");
          }

          if (ki != currency::null_ki)
          {
            // make sure calculated key image for this own output has not been seen before
            auto it = m_key_images.find(ki);
            if (it != m_key_images.end())
            {
              // We encountered an output with a key image already seen. This implies only one can be spent in the future (assuming the first isn't spent yet).
              // To address this, we disregard such outputs and log a warning.
              // 
              // It was later revealed that auditable wallets could still be vulnerable: an attacker might quickly broadcast a transaction
              // using the same output's ephemeral keys + the same tx pub key. If the malicious transaction (potentially for a lesser amount)
              // arrives first, the recipient would be unable to spend the funds from the second, real transaction.
              // This attack vector was highlighted by Luke Parker (twitter: @kayabaNerve), who suggested selecting the output with the largest amount.
              // Sadly, this fix only applies to classic RingCT transactions and is incompatible with our use of Confidential Assets.
              // Consequently, we adopted a solution suggested by @crypto_zoidberg: verifying in zero knowledge that the sender possesses the transaction's
              // secret key. This verification is integrated with the balance proof (double Schnorr proof).
              // 
              // However, we continue to omit outputs with duplicate key images since they could originate from the same source (albeit impractically).
              // -- sowle

              WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it->second < m_transfers.size(), "m_key_images entry has wrong m_transfers index, it->second: " << it->second << ", m_transfers.size(): " << m_transfers.size());
              const transfer_details& local_td = m_transfers[it->second];

              std::stringstream ss;
              ss << "tx " << ptc.tx_hash() << " @ block " << height << " has output #" << o << " with amount " << out.amount;
              if (!out.is_native_coin())
                ss << "(asset_id: " << out.asset_id << ") ";
              ss << "and key image " << ki << " that has already been seen in output #" << local_td.m_internal_output_index << " in tx " << get_transaction_hash(local_td.m_ptx_wallet_info->m_tx)
                << " @ block " << local_td.m_spent_height << ". This output can't ever be spent and will be skipped.";
              WLT_LOG_YELLOW(ss.str(), LOG_LEVEL_0);
              if (m_wcallback)
                m_wcallback->on_message(i_wallet2_callback::ms_yellow, ss.str());
              //if (out.is_native_coin())
              //{
                //WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(sum_of_native_outs >= out.amount, "sum_of_native_outs: " << sum_of_native_outs << ", out.amount:" << out.amount);
                //sum_of_native_outs -= out.amount;
              //}
              continue; // skip the output
            }
          }

          uint8_t mix_attr = CURRENCY_TO_KEY_OUT_RELAXED;
          [[maybe_unused]] bool mix_attr_r = get_mix_attr_from_tx_out_v(out_v, mix_attr);
          if (is_auditable() && (out_type_to_key || out_type_zc) && mix_attr != CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
          {
            std::stringstream ss;
            ss << "output #" << o << " from tx " << ptc.tx_hash();
            if (!out.is_native_coin())
              ss << " asset_id: " << out.asset_id;
            ss << " with amount " << print_money_brief(out.amount)
              << " is targeted to this auditable wallet and has INCORRECT mix_attr = " << (uint64_t)mix_attr << ". Output is IGNORED.";
            WLT_LOG_YELLOW(ss.str(), LOG_LEVEL_0);
            if (m_wcallback)
              m_wcallback->on_message(i_wallet2_callback::ms_red, ss.str());
            //if (out.is_native_coin())
            //{
              //WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(sum_of_native_outs >= out.amount, "sum_of_native_outs: " << sum_of_native_outs << ", out.amount:" << out.amount);
              //sum_of_native_outs -= out.amount;
            //}
            continue; // skip the output
          }

          ptc.employed_entries.receive.push_back(wallet_public::employed_tx_entry{ o , out.amount , out.asset_id});

          m_transfers.push_back(boost::value_initialized<transfer_details>());
          transfer_details& td = m_transfers.back();
          td.m_ptx_wallet_info = pwallet_info;
          td.m_internal_output_index = o;
          td.m_key_image = ki;
          td.m_amount = out.amount;
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
            WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(pglobal_indexes->size() > o, "pglobal_indexes size()(" << pglobal_indexes->size() << ") <= o " << o);
            td.m_global_output_index = (*pglobal_indexes)[o];
          }
          if (ptc.coin_base_tx)
          {
            //last out in coinbase tx supposed to be change from coinstake
            //for genesis block we'll count every input as WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER
            if (td.m_ptx_wallet_info->m_block_height == 0 || !(o == tx.vout.size() - 1 && !ptc.is_derived_from_coinbase)) // TODO: @#@# reconsider this condition
            {
              td.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER;
            }
          }
          
          if (out_type_zc)
          {
            td.m_zc_info_ptr.reset(new transfer_details_base::ZC_out_info(out.amount_blinding_mask, out.asset_id_blinding_mask, out.asset_id));
          }

          size_t transfer_index = m_transfers.size() - 1;
          if (out_type_htlc)
          {
            const currency::txout_htlc& hltc = out_get_htlc(out_v);
            //mark this as spent
            td.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;
            //create entry for htlc input
            htlc_expiration_trigger het = AUTO_VAL_INIT(het);
            het.is_wallet_owns_redeem = (out_key == hltc.pkey_redeem) ? true : false;
            het.transfer_index = transfer_index;
            uint64_t expired_if_more_then = td.m_ptx_wallet_info->m_block_height + hltc.expiration;
            m_htlcs.insert(std::make_pair(expired_if_more_then, het));

            if (het.is_wallet_owns_redeem)
            {
              td.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_HTLC_REDEEM;
            }

            //active htlc
            auto amount_gindex_pair = std::make_pair(td.m_amount, td.m_global_output_index);
            m_active_htlcs[amount_gindex_pair] = transfer_index;
            m_active_htlcs_txid[ptc.tx_hash()] = transfer_index;
            //add payer to extra options 
            currency::tx_payer payer = AUTO_VAL_INIT(payer);
            if (het.is_wallet_owns_redeem)
            {
              if (currency::get_type_in_variant_container(tx.extra, payer))
              {
                crypto::chacha_crypt(payer.acc_addr, derivation);
                td.varian_options.push_back(payer);
              }
            }
            else
            {
              //since this is refund-mode htlc out, then sender is this wallet itself
              payer.acc_addr = m_account.get_public_address();
              td.varian_options.push_back(payer);
            }

          }
          else
          {
            ptc.total_balance_change[td.get_asset_id()] += td.amount();
            add_transfer_to_transfers_cache(td.m_amount, transfer_index, td.get_asset_id());
          }

          if (td.m_key_image != currency::null_ki)
            m_key_images[td.m_key_image] = transfer_index;

          if (is_watch_only() && is_auditable())
          {
            WLT_CHECK_AND_ASSERT_MES_NO_RET(td.m_global_output_index != WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED, "td.m_global_output_index != WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED validation failed");
            auto amount_gindex_pair = std::make_pair(td.amount_for_global_output_index(), td.m_global_output_index);
            WLT_CHECK_AND_ASSERT_MES_NO_RET(m_amount_gindex_to_transfer_id.count(amount_gindex_pair) == 0, "update m_amount_gindex_to_transfer_id: amount_for_global_output_index: " << td.amount_for_global_output_index() << ", gindex: " << td.m_global_output_index << " already exists");
            m_amount_gindex_to_transfer_id[amount_gindex_pair] = transfer_index;
          }

          if (ptc.max_out_unlock_time < get_tx_unlock_time(tx, o))
            ptc.max_out_unlock_time = get_tx_unlock_time(tx, o);

          if (out_type_to_key || out_type_zc)
          {
            if (td.is_native_coin())
            {
              WLT_LOG_L0("Received native coins, transfer #" << transfer_index << ", amount: " << print_money_brief(td.amount()) << (out_type_zc ? " (hidden)" : "") << ", with tx: " << ptc.tx_hash() << ", at height " << height);
            }
            else
            {
              // TODO @#@# output asset's ticker/name
              WLT_LOG_L0("Received asset " << print16(td.get_asset_id()) << ", transfer #" << transfer_index << ", amount: " << print_money_brief(td.amount()) << (out_type_zc ? " (hidden)" : "") << ", with tx: " << ptc.tx_hash() << ", at height " << height);
            }
          }
          else if (out_type_htlc)
          {
            WLT_LOG_L0("Detected HTLC[" << (td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_HTLC_REDEEM ? "REDEEM" : "REFUND") << "], transfer #" << transfer_index << ", amount: " << print_money(td.amount()) << ", with tx: " << ptc.tx_hash() << ", at height " << height);
          }
        }
        else if (out_type_multisig)
        {
          crypto::hash multisig_id = currency::get_multisig_out_id(tx, o);
          WLT_CHECK_AND_ASSERT_MES_NO_RET(m_multisig_transfers.count(multisig_id) == 0, "multisig_id = " << multisig_id << " already in multisig container");
          transfer_details_base& tdb = m_multisig_transfers[multisig_id];
          tdb.m_ptx_wallet_info = pwallet_info;
          tdb.m_internal_output_index = o;
          tdb.m_amount = outs[i_in_outs].amount;
          WLT_LOG_L0("Received multisig, multisig out id: " << multisig_id << ", amount: " << tdb.amount() << ", with tx: " << ptc.tx_hash());
        }
        else
        {
          WLT_LOG_YELLOW("Unexpected output type: " << out_v.type().name() << ", out index: " << o << " in tx " << ptc.tx_hash(), LOG_LEVEL_0);
        }
      }

    }
  }
  

  //do final calculations
  bool has_in_transfers = false;
  bool has_out_transfers = false;
  for (const auto& bce : ptc.total_balance_change)
  {
    if (bce.second > 0)
    {
      has_in_transfers = true;
    }
    else if (bce.second < 0)
    {
      has_out_transfers = true;
    }
  }

  //check if there are asset_registration that belong to this wallet
  const asset_descriptor_operation* pado = get_type_in_variant_container<const asset_descriptor_operation>(tx.extra);
  if (pado && (ptc.employed_entries.receive.size() || ptc.employed_entries.spent.size() || pado->descriptor.owner == m_account.get_public_address().spend_public_key))
  {
    //check if there are asset_registration that belong to this wallet
    process_ado_in_new_transaction(*pado, ptc);
  }

  if (has_in_transfers || has_out_transfers || is_derivation_used_to_encrypt(tx, derivation) || ptc.employed_entries.spent.size())
  {
    ptc.timestamp = get_block_datetime(b);
    handle_money(b, ptc);
  }

  /*
  if (ptc.sum_of_own_native_inputs)
  {//this actually is transfer transaction, notify about spend
    if (ptc.sum_of_own_native_inputs > sum_of_native_outs)
    {//usual transfer 
      handle_money_spent2(b, tx, ptc.sum_of_own_native_inputs - (sum_of_native_outs + get_tx_fee(tx)), ptc.mtd, recipients, remote_aliases);
    }
    else
    {//strange transfer, seems that in one transaction have transfers from different wallets.
      if (!is_coinbase(tx))
      {
        WLT_LOG_RED("Unusual transaction " << ptc.tx_hash() << ", sum_of_native_inputs: " << ptc.sum_of_own_native_inputs << ", sum_of_native_outs: " << sum_of_native_outs, LOG_LEVEL_0);
      }
      handle_money_received2(b, tx, (sum_of_native_outs - (ptc.sum_of_own_native_inputs - get_tx_fee(tx))), ptc.mtd);
    }
  }
  else
  {
    if (sum_of_native_outs != 0)
    {
      handle_money_received2(b, tx, sum_of_native_outs, ptc.mtd);
    }
    else if (currency::is_derivation_used_to_encrypt(tx, derivation))
    {
      //transaction doesn't transfer actually money, but bring some information
      handle_money_received2(b, tx, 0, ptc.mtd);
    }
    else if (ptc.mtd.spent_indices.size())
    {
      // multisig spend detected
      handle_money_spent2(b, tx, 0, ptc.mtd, recipients, remote_aliases);
    }
  }*/
}
//----------------------------------------------------------------------------------------------------
void wallet2::prepare_wti_decrypted_attachments(wallet_public::wallet_transfer_info& wti, const std::vector<currency::payload_items_v>& decrypted_att)
{
  PROFILE_FUNC("wallet2::prepare_wti_decrypted_attachments");

  if (!wti.payment_id.empty())
  {
    LOG_ERROR("wti.payment_id is expected to be empty. Go ahead.");
  }
  get_payment_id_from_decrypted_container(decrypted_att, wti.payment_id);

  for (const auto& item : decrypted_att)
  {
    if (item.type() == typeid(currency::tx_service_attachment))
    {
      wti.service_entries.push_back(boost::get<currency::tx_service_attachment>(item));
    }
  }

  if (wti.is_income_mode_encryption())
  {
    account_public_address sender_address = AUTO_VAL_INIT(sender_address);
    wti.show_sender = handle_2_alternative_types_in_variant_container<tx_payer, tx_payer_old>(decrypted_att, [&](const tx_payer& p) { sender_address = p.acc_addr; return false; /* <- continue? */ });
    if (wti.show_sender)
      if(!wti.remote_addresses.size()) 
        wti.remote_addresses.push_back(currency::get_account_address_as_str(sender_address));
  }
  else
  {
    if (wti.remote_addresses.empty())
    {
      handle_2_alternative_types_in_variant_container<tx_receiver, tx_receiver_old>(decrypted_att, [&](const tx_receiver& p) {
        std::string addr_str;
        if (wti.payment_id.empty())
          addr_str = currency::get_account_address_as_str(p.acc_addr);
        else
          addr_str = currency::get_account_address_and_payment_id_as_str(p.acc_addr, wti.payment_id); // show integrated address if there's a payment id provided
        wti.remote_addresses.push_back(addr_str);
        LOG_PRINT_YELLOW("prepare_wti_decrypted_attachments, income=false, rem. addr = " << addr_str, LOG_LEVEL_0);
        return true; // continue iterating through the container
      });
    }
  }

  currency::tx_comment cm;
  if (currency::get_type_in_variant_container(decrypted_att, cm))
    wti.comment = cm.comment;
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
  mode_separate_context msc = AUTO_VAL_INIT(msc);
  msc.escrow = true;
  msc.tx_for_mode_separate = contr_it->second.proposal.tx_template;
  currency::transaction& tx = msc.tx_for_mode_separate;
  crypto::secret_key one_time_key = contr_it->second.proposal.tx_onetime_secret_key;
  construct_param.crypt_address = m_account.get_public_address();
  construct_param.flags = TX_FLAG_SIGNATURE_MODE_SEPARATE;
  construct_param.mark_tx_as_complete = true;
  construct_param.split_strategy_id = get_current_split_strategy();

  //little hack for now, we add multisig_entry before transaction actually get to blockchain
  //to let prepare_transaction (which is called from build_escrow_release_templates) work correct 
  //this code definitely need to be rewritten later (very bad design)
  size_t n = get_multisig_out_index(tx.vout);
  THROW_IF_FALSE_WALLET_EX(n != tx.vout.size(), error::wallet_internal_error, "Multisig out not found in tx template in proposal");

  transfer_details_base& tdb = m_multisig_transfers[contract_id];
  //create once instance of tx for all entries
  std::shared_ptr<transaction_wallet_info> pwallet_info(new transaction_wallet_info());
  pwallet_info->m_tx = tx;
  pwallet_info->m_block_height = 0;
  pwallet_info->m_block_timestamp = 0;
  tdb.m_ptx_wallet_info = pwallet_info;
  tdb.m_internal_output_index = n;
  tdb.m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_SPENT);
  //---------------------------------
  //@#@ todo: proper handling with zarcanum_based stuff
  //figure out fee that was left for release contract 
  THROW_IF_FALSE_WALLET_INT_ERR_EX(tx.vout[n].type() == typeid(tx_out_bare), "Unexpected output type in accept proposal");
  THROW_IF_FALSE_WALLET_INT_ERR_EX(boost::get<tx_out_bare>(tx.vout[n]).amount > (contr_it->second.private_detailes.amount_to_pay +
    contr_it->second.private_detailes.amount_b_pledge +
    contr_it->second.private_detailes.amount_a_pledge), "THere is no left money for fee, contract_id: " << contract_id);
  uint64_t left_for_fee_in_multisig = boost::get<tx_out_bare>(tx.vout[n]).amount - (contr_it->second.private_detailes.amount_to_pay +
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
  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.tx_version = this->get_current_tx_version();
  prepare_transaction(construct_param, ftp, msc);
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
//---------------------------------------------------------------------------------
uint64_t wallet2::get_current_tx_version()
{
  uint64_t tx_expected_block_height = get_top_block_height() + 1;
  return currency::get_tx_version(tx_expected_block_height, this->m_core_runtime_config.hard_forks);
}
//---------------------------------------------------------------------------------
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
  construct_param.split_strategy_id = get_current_split_strategy();

  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.tx_version = this->get_current_tx_version();
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
void from_outs_to_received_items(const std::vector<currency::wallet_out_info>& outs, std::vector<tools::payment_details_subtransfer>& received, const currency::transaction& tx)
{
  for (const auto& item : outs)
  {
    if(!out_is_multisig(tx.vout[item.index]))
      received.push_back(tools::payment_details_subtransfer{ item.asset_id, item.amount});
  }
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
  currency::get_payment_id_from_decrypted_container(decrypted_items, ed.payment_id);
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
    std::vector<wallet_out_info> outs;
    bool r = lookup_acc_outs(m_account.get_keys(), prop.tx_template, outs, derivation);
    THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "Failed to lookup_acc_outs for tx: " << get_transaction_hash(prop.tx_template));
    std::vector<payment_details_subtransfer> received;
    from_outs_to_received_items(outs, received, prop.tx_template);
    add_transfers_to_expiration_list(found_transfers, received, ed.expiration_time, wti.tx_hash);
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
    wallet_public::wallet_sub_transfer_info* subptr = nullptr;
    for (auto& s: wti.subtransfers)
    {
      if (s.asset_id == currency::native_coin_asset_id)
        subptr = &s;
    }
    if (subptr == nullptr)
    {
      wti.subtransfers.push_back(wallet_public::wallet_sub_transfer_info());
      subptr = &wti.subtransfers.back();
    }

    subptr->amount = it->second.private_detailes.amount_to_pay + it->second.private_detailes.amount_a_pledge + it->second.private_detailes.amount_b_pledge;
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
    WLT_CHECK_AND_ASSERT_MES(wti.subtransfers.size(), false, "Unexpected subtransfers size"); //TODO: subject for refactoring
    wti.subtransfers.back().amount += wti.fee;
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
void wallet2::prepare_wti(wallet_public::wallet_transfer_info& wti, const process_transaction_context& tx_process_context)
{
  PROFILE_FUNC("wallet2::prepare_wti");
  wti.tx = tx_process_context.tx;

  load_wti_from_process_transaction_context(wti, tx_process_context);

  wti.height = tx_process_context.height;
  wti.employed_entries = tx_process_context.employed_entries;
  wti.unlock_time = get_max_unlock_time_from_receive_indices(tx_process_context.tx, tx_process_context.employed_entries);
  wti.timestamp = tx_process_context.timestamp;
  wti.tx_blob_size = static_cast<uint32_t>(currency::get_object_blobsize(wti.tx));
  wti.tx_hash = tx_process_context.tx_hash();
  load_wallet_transfer_info_flags(wti);
  bc_services::extract_market_instructions(wti.marketplace_entries, wti.tx.attachment);

  // escrow transactions, which are built with TX_FLAG_SIGNATURE_MODE_SEPARATE flag actually encrypt attachments 
  // with buyer as a sender, and seller as receiver, despite the fact that for both sides transaction seen as outgoing.
  // so here to decrypt tx properly we need to figure out, if this transaction is actually escrow acceptance. 
  //we check if spent_indices have zero then input do not belong to this account, which means that we are seller for this 
  //escrow, and decryption should be processed as income flag

  //let's assume that the one who pays for tx fee is sender of tx
  bool decrypt_attachment_as_income = !(tx_process_context.total_balance_change.count(currency::native_coin_asset_id) && tx_process_context.total_balance_change.at(currency::native_coin_asset_id) < 0 );
  std::vector<currency::payload_items_v> decrypted_att;
  bool has_zero_input_as_spent = false;
  for (const auto& item : tx_process_context.employed_entries.spent)
  {
    if (item.index == 0)
    {
      has_zero_input_as_spent = true;
      break;
    }
  }
   
  if (wti.tx_type == GUI_TX_TYPE_ESCROW_TRANSFER && !has_zero_input_as_spent)
    decrypt_attachment_as_income = true;


  decrypt_payload_items(decrypt_attachment_as_income, wti.tx, m_account.get_keys(), decrypted_att);
  if ((is_watch_only() && !decrypt_attachment_as_income)|| (wti.height > 638000 && !have_type_in_variant_container<etc_tx_flags16_t>(decrypted_att)))
  {
    remove_field_of_type_from_extra<tx_receiver_old>(decrypted_att);
    remove_field_of_type_from_extra<tx_payer_old>(decrypted_att);
  }
  if (is_watch_only() && !decrypt_attachment_as_income)
  {
    remove_field_of_type_from_extra<tx_comment>(decrypted_att);
  }
  prepare_wti_decrypted_attachments(wti, decrypted_att);
  process_contract_info(wti, decrypted_att);
  process_payment_id_for_wti(wti, tx_process_context);

}
//----------------------------------------------------------------------------------------------------
bool wallet2::process_payment_id_for_wti(wallet_public::wallet_transfer_info& wti, const process_transaction_context& ptc)
{
  //if(this->is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM))
  {
    if (wti.get_native_is_income() && wti.payment_id.size())
    {
      payment_details payment;
      payment.m_tx_hash = wti.tx_hash;
      payment.m_amount = 0;
      payment.m_block_height = wti.height;
      payment.m_unlock_time = ptc.max_out_unlock_time;

      for (const auto& bce : ptc.total_balance_change)
      {
        if (bce.second > 0)
        {
          if (bce.first == currency::native_coin_asset_id)
          {
            payment.m_amount = static_cast<uint64_t>(bce.second);
          }
          else
          {
            payment.subtransfers.push_back(payment_details_subtransfer{ bce.first, static_cast<uint64_t>(bce.second) });
          }
        }
      }
      m_payments.emplace(wti.payment_id, payment);
      WLT_LOG_L2("Payment found, id (hex): " << epee::string_tools::buff_to_hex_nodelimer(wti.payment_id) << ", tx: " << payment.m_tx_hash << ", amount: " << print_money_brief(payment.m_amount) << "subtransfers = " << payment.subtransfers.size());
    }
  }
  return true;
}
void wallet2::rise_on_transfer2(const wallet_public::wallet_transfer_info& wti)
{
  PROFILE_FUNC("wallet2::rise_on_transfer2");
  if (!m_do_rise_transfer)
    return;
  std::list<wallet_public::asset_balance_entry> balances;
  uint64_t mined_balance = 0;
  this->balance(balances, mined_balance);
  m_wcallback->on_transfer2(wti, balances, mined_balance);
  // second call for legacy callback handlers
  //m_wcallback->on_transfer2(wti, balances, mined_balance);
}
//----------------------------------------------------------------------------------------------------
/*
void wallet2::handle_money_spent2(const currency::block& b,
                                  const currency::transaction& in_tx, 
                                  uint64_t amount, 
                                  const money_transfer2_details& td, 
                                  const std::vector<std::string>& recipients, 
                                  const std::vector<std::string>& remote_aliases)
{
  m_transfer_history.push_back(AUTO_VAL_INIT(wallet_public::wallet_transfer_info()));
  wallet_public::wallet_transfer_info& wti = m_transfer_history.back();
  wti.is_income = false;

  wti.remote_addresses = recipients;
  wti.remote_aliases = remote_aliases;
  prepare_wti(wti, get_block_height(b), get_block_datetime(b), in_tx, amount, td);
  WLT_LOG_L1("[MONEY SPENT]: " << epee::serialization::store_t_to_json(wti));
  rise_on_transfer2(wti);
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_money_received2(const currency::block& b, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td)
{
  //decrypt attachments
  m_transfer_history.push_back(AUTO_VAL_INIT(wallet_public::wallet_transfer_info()));
  wallet_public::wallet_transfer_info& wti = m_transfer_history.back();
  wti.is_income = true;
  // TODO @#@# this function is only able to handle native coins atm, consider changing -- sowle
  wti.asset_id = native_coin_asset_id;
  prepare_wti(wti, get_block_height(b), get_block_datetime(b), tx, amount, td);
  WLT_LOG_L1("[MONEY RECEIVED]: " << epee::serialization::store_t_to_json(wti));
  rise_on_transfer2(wti);
}
*/
 //----------------------------------------------------------------------------------------------------
void wallet2::load_wti_from_process_transaction_context(wallet_public::wallet_transfer_info& wti, const process_transaction_context& tx_process_context)
{
  wti.remote_addresses = tx_process_context.recipients;
  wti.remote_aliases = tx_process_context.remote_aliases;
  for (const auto& bce : tx_process_context.total_balance_change)
  {
    wallet_public::wallet_sub_transfer_info wsti = AUTO_VAL_INIT(wsti);
    wsti.asset_id = bce.first;
    if (bce.second == 0)
    {
      continue;
    }
    else if (bce.second > 0)
    {
      //in transfer
      wsti.is_income = true;
      wsti.amount = static_cast<uint64_t>(bce.second);
    }
    else
    {
      //out transfer
      wsti.is_income = false;
      wsti.amount = static_cast<uint64_t>(bce.second * (-1));
    }
    wti.subtransfers.push_back(wsti);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_money(const currency::block& b, const process_transaction_context& tx_process_context)
{
  m_transfer_history.push_back(AUTO_VAL_INIT(wallet_public::wallet_transfer_info()));
  wallet_public::wallet_transfer_info& wti = m_transfer_history.back();
  prepare_wti(wti, tx_process_context);
  WLT_LOG_L1("[MONEY SPENT]: " << epee::serialization::store_t_to_json(wti));
  rise_on_transfer2(wti);
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_unconfirmed(const currency::transaction& tx, std::vector<std::string>& recipients, std::vector<std::string>& remote_aliases)
{
  auto unconf_it = m_unconfirmed_txs.find(get_transaction_hash(tx));
  if (unconf_it != m_unconfirmed_txs.end())
  {
    wallet_public::wallet_transfer_info& wti = unconf_it->second;
    recipients = wti.remote_addresses;
    remote_aliases = wti.remote_aliases;

    m_unconfirmed_txs.erase(unconf_it);
  }
}
//----------------------------------------------------------------------------------------------------

void wallet2::unprocess_htlc_triggers_on_block_removed(uint64_t height)
{
  if (!m_htlcs.size())
    return;

  if (height > m_htlcs.rbegin()->first)
  {
    //there is no active htlc that at this height
    CHECK_AND_ASSERT_MES(m_active_htlcs.size() == 0, void(), "Self check failed: m_active_htlcs.size() = " << m_active_htlcs.size());
    return;
  }
  //we have to check if there is a htlc that has to become deactivated
  auto pair_of_it = m_htlcs.equal_range(height);
  for (auto it = pair_of_it.first; it != pair_of_it.second; it++)
  {
    auto& tr = m_transfers[it->second.transfer_index];
    //found contract that supposed to be re-activated and set to active
    if (it->second.is_wallet_owns_redeem)
    {
      // this means that wallet received atomic as proposal but never activated it, and now we back to phase where out can be activated
      //but we keep spend flag anyway
      tr.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT; //re assure that it has spent flag
      tr.m_spent_height = 0;
    }
    else
    {
      // this means that wallet created atomic by itself, and second part didn't redeem it, 
      // so refund money became available, and now we back again to unavailable state
      tr.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT; //reset spent flag
      m_found_free_amounts.clear(); //reset free amounts cache 
      tr.m_spent_height = 0;
    }
    //re-add to active contracts
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(tr.m_ptx_wallet_info->m_tx.vout[tr.m_internal_output_index].type() == typeid(tx_out_bare), std::string("Unexprected type of out in unprocess_htlc_triggers_on_block_removed : ") + tr.m_ptx_wallet_info->m_tx.vout[tr.m_internal_output_index].type().name());
    auto pair_key = std::make_pair(tr.m_amount, tr.m_global_output_index);
    auto it_active_htlc = m_active_htlcs.find(pair_key);
    if (it_active_htlc != m_active_htlcs.end())
    {
      LOG_ERROR("Error at putting back htlc: already exist?");
      it_active_htlc->second = it->second.transfer_index;
      
    }
    else
    {
      m_active_htlcs[pair_key] = it->second.transfer_index;
    }

    const crypto::hash tx_id = tr.tx_hash();
    auto tx_id_it = m_active_htlcs_txid.find(tx_id);
    if (tx_id_it != m_active_htlcs_txid.end())
    {
      LOG_ERROR("Error at putting back htlc_txid: already exist?");
      tx_id_it->second = it->second.transfer_index;

    }
    else
    {
      m_active_htlcs_txid[tx_id] = it->second.transfer_index;
    }
  }
}
void wallet2::process_htlc_triggers_on_block_added(uint64_t height)
{
  if (!m_htlcs.size())
    return;

  if (height > m_htlcs.rbegin()->first)
  {
    //there is no active htlc that at this height
    CHECK_AND_ASSERT_MES(m_active_htlcs.size() == 0, void(), "Self check failed: m_active_htlcs.size() = " << m_active_htlcs.size());
    return;
  }
  //we have to check if there is a htlc that has to become deactivated
  auto pair_of_it = m_htlcs.equal_range(height);
  for (auto it = pair_of_it.first; it != pair_of_it.second; it++)
  {
    auto& tr = m_transfers[it->second.transfer_index];
    //found contract that supposed to be deactivated and set to innactive
    if (it->second.is_wallet_owns_redeem)
    {
      // this means that wallet received atomic as proposal but never activated it, money returned to initiator
      tr.m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT; //re assure that it has spent flag
      tr.m_spent_height = height;
    }
    else
    {
      // this means that wallet created atomic by itself, and second part didn't redeem it, so refund money should become available
      tr.m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_SPENT); //reset spent flag
      m_found_free_amounts.clear(); //reset free amounts cache 
      tr.m_spent_height = 0;
    }    
    
    //reset cache
    m_found_free_amounts.clear();

    //remove it from active contracts
    CHECK_AND_ASSERT_MES(tr.m_ptx_wallet_info->m_tx.vout[tr.m_internal_output_index].type() == typeid(tx_out_bare), void(), "Unexpected type out in process_htlc_triggers_on_block_added: " << tr.m_ptx_wallet_info->m_tx.vout[tr.m_internal_output_index].type().name());
    uint64_t amount = tr.m_amount;

    auto it_active_htlc = m_active_htlcs.find(std::make_pair(amount, tr.m_global_output_index));
    if (it_active_htlc == m_active_htlcs.end())
    {
      LOG_ERROR("Erasing active htlc(m_active_htlcs), but it seems to be already erased");
    }
    else
    {
      m_active_htlcs.erase(it_active_htlc);
      auto it_tx = m_active_htlcs_txid.find(tr.tx_hash());
      if (it_tx == m_active_htlcs_txid.end())
      {
        LOG_ERROR("Erasing active htlc(;), but it seems to be already erased");
      }
      else
      {
        m_active_htlcs_txid.erase(it_tx);
      }
    }
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
  if (get_block_height(b) > get_wallet_minimum_height()) // b.timestamp + 60 * 60 * 24 > m_account.get_createtime())
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
    size_t count = 0;
    for(const auto& tx_entry: bche.txs_ptr)
    {
      if (b.tx_hashes.size() < count || currency::get_transaction_hash(tx_entry->tx) != b.tx_hashes[count])
      {
        LOG_ERROR("Found tx order fail in process_new_blockchain_entry: count=" << count 
          << ", b.tx_hashes.size() = " << b.tx_hashes.size() << ", tx real id: " << currency::get_transaction_hash(tx_entry->tx) << ", bl_id: " << bl_id);
      }
      process_new_transaction(tx_entry->tx, height, b, &(tx_entry->m_global_output_indexes));
      count++;
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


  process_htlc_triggers_on_block_added(height);
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
  if (req.minimum_height > m_height_of_start_sync)
    m_height_of_start_sync = req.minimum_height;

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
    m_minimum_height = req.minimum_height;
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
    m_core_proxy->get_editable_proxy_diagnostic_info()->is_busy = true;
    stop = true;
    return;
  }
  m_core_proxy->get_editable_proxy_diagnostic_info()->is_busy = false;
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
  m_last_known_daemon_height = res.current_height;
  bool been_matched_block = false;
  if (res.start_height == 0 && get_blockchain_current_size() == 1 && !res.blocks.empty())
  {
    const currency::block& genesis = res.blocks.front().block_ptr->bl;
    THROW_IF_TRUE_WALLET_EX(get_block_height(genesis) != 0, error::wallet_internal_error, "first block expected to be genesis");
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(res.blocks.front().coinbase_ptr, "Unexpected empty coinbase");    
    process_genesis_if_needed(genesis, &(res.blocks.front().coinbase_ptr->m_global_output_indexes));
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
    else if (height == processed_blocks_count && been_matched_block)
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
        WLT_LOG_L4("Block " << bl_id << " @ " << height << " is already in wallet's blockchain");
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

  WLT_LOG_L2("[PULL BLOCKS] " << res.start_height << " --> " << get_blockchain_current_size() - 1);
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
  m_stop = false;
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
detail::split_strategy_id_t wallet2::get_current_split_strategy()
{
  if (is_need_to_split_outputs())
    return tools::detail::ssi_digit;
  else 
    return tools::detail::ssi_void;
}
//
void wallet2::transfer(uint64_t amount, const currency::account_public_address& acc, currency::transaction& result_tx, const crypto::public_key& asset_id /* = currency::native_coin_asset_id */)
{
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  std::vector<tx_destination_entry> dst;
  dst.resize(1);
  dst.back().addr.push_back(acc);
  dst.back().amount = amount;
  dst.back().asset_id = asset_id;
  this->transfer(dst, 0, 0, TX_DEFAULT_FEE, extra, attachments, get_current_split_strategy(), tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), result_tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(uint64_t amount, size_t fake_outs_count, const currency::account_public_address& acc, uint64_t fee /* = TX_DEFAULT_FEE*/,
  const crypto::public_key& asset_id /* = currency::native_coin_asset_id */)
{
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;
  transaction result_tx = AUTO_VAL_INIT(result_tx);

  std::vector<tx_destination_entry> dst;
  dst.resize(1);
  dst.back().addr.push_back(acc);
  dst.back().amount = amount;
  dst.back().asset_id = asset_id;
  this->transfer(dst, fake_outs_count, 0, fee, extra, attachments, get_current_split_strategy(), tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), result_tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(uint64_t amount, const currency::account_public_address& acc, const crypto::public_key& asset_id /* = currency::native_coin_asset_id */)
{
  transaction result_tx = AUTO_VAL_INIT(result_tx);
  this->transfer(amount, acc, result_tx, asset_id);
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
bool wallet2::has_bare_unspent_outputs() const
{
  if (m_account.get_createtime() > ZANO_HARDFORK_04_TIMESTAMP_ACTUAL)
    return false;

  [[maybe_unused]] uint64_t bal = 0;
  if (!m_has_bare_unspent_outputs.has_value())
    bal = balance();

  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(m_has_bare_unspent_outputs.has_value(), "m_has_bare_unspent_outputs has no value after balance()");

  return m_has_bare_unspent_outputs.value();
}
//----------------------------------------------------------------------------------------------------
#define MAX_INPUTS_FOR_SIMPLE_TX_EURISTIC 20
bool wallet2::get_bare_unspent_outputs_stats(std::vector<batch_of_bare_unspent_outs>& tids_grouped_by_txs) const
{
  tids_grouped_by_txs.clear();

  // 1/3. Populate a list of bare unspent outputs
  std::unordered_map<crypto::hash, std::vector<size_t>> buo_ids; // tx hash -> Bare Unspent Outs list
  for(size_t tid = 0; tid != m_transfers.size(); ++tid)
  {
    const auto& td = m_transfers[tid];
    if (!td.is_zc() && td.is_spendable())
    {
      buo_ids[td.tx_hash()].push_back(tid);
    }
  }

  if (buo_ids.empty())
    return true;

  // 2/3. Split them into groups
  tids_grouped_by_txs.emplace_back();
  for(auto& buo_el : buo_ids)
  {
    if (tids_grouped_by_txs.back().tids.size() + buo_el.second.size() > MAX_INPUTS_FOR_SIMPLE_TX_EURISTIC)
      tids_grouped_by_txs.emplace_back();

    for(auto& tid : buo_el.second)
    {
      if (tids_grouped_by_txs.back().tids.size() >= MAX_INPUTS_FOR_SIMPLE_TX_EURISTIC)
        tids_grouped_by_txs.emplace_back();
      tids_grouped_by_txs.back().tids.push_back((uint64_t)tid);
      tids_grouped_by_txs.back().total_amount += m_transfers[tid].m_amount;
    }
  }


  // 3/3. Iterate through groups and check whether total amount is big enough to cover min fee.
  // Add additional zc output if not.
  std::multimap<uint64_t, size_t> usable_zc_outs_tids; // grouped by amount
  bool usable_zc_outs_tids_precalculated = false;
  auto precalculate_usable_zc_outs_if_needed = [&](){
      if (usable_zc_outs_tids_precalculated)
        return;
      size_t decoys = is_auditable() ? 0 : m_core_runtime_config.hf4_minimum_mixins;
      for(size_t tid = 0; tid != m_transfers.size(); ++tid)
      {
        auto& td = m_transfers[tid];
        if (td.is_zc() && td.is_native_coin() && is_transfer_ready_to_go(td, decoys))
          usable_zc_outs_tids.insert(std::make_pair(td.m_amount, tid));
      }
      usable_zc_outs_tids_precalculated = true;
    };

  std::unordered_set<size_t> used_zc_outs;
  for(auto it = tids_grouped_by_txs.begin(); it != tids_grouped_by_txs.end(); )
  {
    auto& group = *it;
    if (group.total_amount < TX_MINIMUM_FEE)
    {
      precalculate_usable_zc_outs_if_needed();

      uint64_t min_required_amount = TX_MINIMUM_FEE - group.total_amount;
      auto jt = usable_zc_outs_tids.lower_bound(min_required_amount);
      bool found = false;
      while(jt != usable_zc_outs_tids.end())
      {
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(jt->first >= min_required_amount, "jt->first=" << jt->first << ", min_required_amount=" << min_required_amount);
        if (used_zc_outs.count(jt->second) == 0)
        {
          group.tids.push_back((uint64_t)jt->second);
          used_zc_outs.insert(jt->second);
          group.additional_tid = true;
          group.additional_tid_amount = jt->first;
          found = true;
          break;
        }
        ++jt;
      }

      if (!found)
      {
        // no usable outs for required amount, remove this group and go to the next
        it = tids_grouped_by_txs.erase(it);
        continue;
      }
    }

    ++it;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::sweep_bare_unspent_outputs(const currency::account_public_address& target_address, const std::vector<batch_of_bare_unspent_outs>& tids_grouped_by_txs,
  std::function<void(size_t batch_index, const currency::transaction& tx, uint64_t amount, uint64_t fee, bool sent_ok, const std::string& err)> on_tx_sent)
{
  if (m_watch_only)
    return false;

  size_t decoys_count = is_auditable() ? 0 : CURRENCY_DEFAULT_DECOY_SET_SIZE;

  bool send_to_network = true;

  size_t batch_index = 0;
  for(const batch_of_bare_unspent_outs& group : tids_grouped_by_txs)
  {
    currency::finalized_tx ftx{};
    currency::finalize_tx_param ftp{};
    ftp.pevents_dispatcher = &m_debug_events_dispatcher;
    ftp.tx_version = this->get_current_tx_version();

    if (!prepare_tx_sources(decoys_count, /*use_all_decoys_if_found_less_than_required*/ true, ftp.sources, group.tids))
    {
      on_tx_sent(batch_index, transaction{}, 0, 0, false, "sources for tx couldn't be prepared");
      LOG_PRINT_L0("prepare_tx_sources failed, batch_index = " << batch_index);
      return false;
    }
    uint64_t fee = TX_DEFAULT_FEE;
    std::vector<tx_destination_entry> destinations{tx_destination_entry(group.total_amount + group.additional_tid_amount - fee, target_address)};
    assets_selection_context needed_money_map{std::make_pair(native_coin_asset_id, selection_for_amount{group.total_amount + group.additional_tid_amount, group.total_amount + group.additional_tid_amount})};
    try
    {
      prepare_tx_destinations(needed_money_map, get_current_split_strategy(), tx_dust_policy{}, destinations, 0 /* tx_flags */, ftp.prepared_destinations);
    }
    catch(...)
    {
      on_tx_sent(batch_index, transaction{}, 0, 0, false, "destinations for tx couldn't be prepared");
      LOG_PRINT_L0("prepare_tx_destinations failed, batch_index = " << batch_index);
      return false;
    }

    mark_transfers_as_spent(ftp.selected_transfers, std::string("sweep bare UTXO, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(ftx.tx)));
    try
    {
      finalize_transaction(ftp, ftx, send_to_network);
      on_tx_sent(batch_index, ftx.tx, group.total_amount + group.additional_tid_amount, fee, true, std::string());
    }
    catch(std::exception& e)
    {
      clear_transfers_from_flag(ftp.selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_SPENT, std::string("exception on sweep bare UTXO, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(ftx.tx)));
      on_tx_sent(batch_index, transaction{}, 0, 0, false, e.what());
      return false;
    }
    
    ++batch_index;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::sweep_bare_unspent_outputs(const currency::account_public_address& target_address, const std::vector<batch_of_bare_unspent_outs>& tids_grouped_by_txs,
  size_t& total_txs_sent, uint64_t& total_amount_sent, uint64_t& total_fee_spent, uint64_t& total_bare_outs_sent)
{
  total_txs_sent = 0;
  total_amount_sent = 0;
  total_fee_spent = 0;
  total_bare_outs_sent = 0;
  auto on_tx_sent_callback = [&](size_t batch_index, const currency::transaction& tx, uint64_t amount, uint64_t fee, bool sent_ok, const std::string& err) {
      if (sent_ok)
      {
        total_bare_outs_sent += count_type_in_variant_container<txin_to_key>(tx.vin);
        ++total_txs_sent;
        total_fee_spent += fee;
        total_amount_sent += amount;
      }
    };

  return sweep_bare_unspent_outputs(target_address, tids_grouped_by_txs, on_tx_sent_callback);
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_directly_spent_transfer_index_by_input_in_tracking_wallet(const currency::txin_to_key& intk)
{
  return get_directly_spent_transfer_index_by_input_in_tracking_wallet(intk.amount, intk.key_offsets);
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_directly_spent_transfer_index_by_input_in_tracking_wallet(const currency::txin_zc_input& inzc)
{
  return get_directly_spent_transfer_index_by_input_in_tracking_wallet(0, inzc.key_offsets);
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_directly_spent_transfer_index_by_input_in_tracking_wallet(uint64_t amount,  const std::vector<currency::txout_ref_v> & key_offsets)
{
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(is_auditable() && is_watch_only(), "this is not an auditable-watch-only (tracking) wallet");
  
  uint64_t tid = UINT64_MAX;

  // try to find a reference among own UTXOs
  std::vector<txout_ref_v> abs_key_offsets = relative_output_offsets_to_absolute(key_offsets); // potential speed-up: don't convert to abs offsets as we interested only in direct spends for auditable wallets. Now it's kind a bit paranoid.
  for (auto v : abs_key_offsets)
  {
    if (v.type() != typeid(uint64_t))
      continue;
    uint64_t gindex = boost::get<uint64_t>(v);
    auto it = m_amount_gindex_to_transfer_id.find(std::make_pair(amount, gindex));
    if (it != m_amount_gindex_to_transfer_id.end())
    {
      tid = it->second;
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(tid < m_transfers.size(), "invalid tid: " << tid << ", ref from input with amount: " << amount << ", gindex: " << gindex);
      auto& td = m_transfers[it->second];
      if (key_offsets.size() != 1)
      {
        // own output was used in non-direct transaction
        // the core should not allow this to happen, the only way it may happen - mixing in own output that was sent without mix_attr == 1
        // log strange situation
        std::stringstream ss;
        ss << "own transfer tid=" << tid << " tx=" << td.tx_hash() << " mix_attr=" << td.mix_attr() << ", is referenced by a transaction with mixins, ref from input with amount: " << amount << ", gindex: " << gindex;
        WLT_LOG_YELLOW(ss.str(), LOG_LEVEL_0);
        if (m_wcallback)
          m_wcallback->on_message(i_wallet2_callback::ms_yellow, ss.str());
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(td.mix_attr() != CURRENCY_TO_KEY_OUT_FORCED_NO_MIX, ss.str()); // if mix_attr == 1 this should never happen (mixing in an output with mix_attr = 1) as the core must reject such txs
        // our own output has mix_attr != 1 for some reason (a sender did not set correct mix_attr e.g.)
        // but mixin count > 1 so we can't say it is spent for sure
        tid = UINT64_MAX;
        continue;
      }
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(td.m_spent_height == 0, "transfer is spent in blockchain, tid: " << tid << ", ref from input with amount: " << amount << ", gindex: " << gindex);
      // okay, own output is being spent, return  it
      break;
    }
  }
  return tid;
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_unconfirmed_tx(process_transaction_context& ptc)
{
  const transaction& tx = ptc.tx;
  ptc.timestamp = m_core_runtime_config.get_core_time();
  // read extra
  std::vector<wallet_out_info> outs;
  //uint64_t sum_of_received_native_outs = 0;
  crypto::public_key tx_pub_key = null_pkey;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  THROW_IF_TRUE_WALLET_EX(!r, error::tx_extra_parse_error, tx);
  //check if we have money
  crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
  r = lookup_acc_outs(m_account.get_keys(), tx, tx_pub_key, outs, derivation);
  THROW_IF_TRUE_WALLET_EX(!r, error::acc_outs_lookup_error, tx, tx_pub_key, m_account.get_keys());

  //collect incomes
  for (auto& o : outs)
  {
    if(out_is_multisig(tx.vout[o.index]))
      continue;

    ptc.total_balance_change[o.asset_id] += o.amount;
    ptc.employed_entries.receive.push_back(wallet_public::employed_tx_entry{ o.index, o.amount, o.asset_id });
  }


  bool new_multisig_spend_detected = false;
  //check if we have spendings
  //uint64_t sum_of_spent_native_coin = 0;
  std::list<size_t> spend_transfers;
  // check all outputs for spending (compare key images)
  for (size_t i = 0; i != tx.vin.size(); i++)
  {
    auto& in = tx.vin[i];
    if (in.type() == typeid(currency::txin_to_key))
    {
      const currency::txin_to_key& intk = boost::get<currency::txin_to_key>(in);
      uint64_t tid = UINT64_MAX;
      if (is_auditable() && is_watch_only())
      {
        // tracking wallet, assuming all outputs are spent directly because of mix_attr = 1
        tid = get_directly_spent_transfer_index_by_input_in_tracking_wallet(intk.amount, intk.key_offsets);
      }
      else
      {
        // wallet with spend secret key -- we can calculate own key images and then search among them
        auto it = m_key_images.find(intk.k_image);
        if (it != m_key_images.end())
        {
          tid = it->second;
        }
      }

      if (tid != UINT64_MAX)
      {
        // own output is being spent by this input
        //sum_of_spent_native_coin += intk.amount;
        ptc.employed_entries.spent.push_back(wallet_public::employed_tx_entry{ i, m_transfers[tid].amount(), m_transfers[tid].get_asset_id() });
        spend_transfers.push_back(tid);
        ptc.total_balance_change[currency::native_coin_asset_id] -= m_transfers[tid].amount();
        CHECK_AND_ASSERT_THROW_MES(m_transfers[tid].get_asset_id() == currency::native_coin_asset_id, "Unexpected asset id for native txin_to_key");
      }
    }
    else if (in.type() == typeid(currency::txin_zc_input))
    {
      // bad design -- remove redundancy like using wallet2::process_input_t()
      const currency::txin_zc_input& zc = boost::get<currency::txin_zc_input>(in);
      uint64_t tid = UINT64_MAX;
      if (is_auditable() && is_watch_only())
      {
        // tracking wallet, assuming all outputs are spent directly because of mix_attr = 1
        tid = get_directly_spent_transfer_index_by_input_in_tracking_wallet(zc);
      }
      else
      {
        // wallet with spend secret key -- we can calculate own key images and then search among them
        auto it = m_key_images.find(zc.k_image);
        if (it != m_key_images.end())
        {
          tid = it->second;
        }
      }

      if (tid != UINT64_MAX)
      {
        ptc.employed_entries.spent.push_back(wallet_public::employed_tx_entry{ i, m_transfers[tid].amount(), m_transfers[tid].get_asset_id() });
        spend_transfers.push_back(tid);
        ptc.total_balance_change[m_transfers[tid].get_asset_id()] -= m_transfers[tid].amount();
      }
    }
    else if (in.type() == typeid(currency::txin_multisig))
    {
      crypto::hash multisig_id = boost::get<currency::txin_multisig>(in).multisig_out_id;
      auto it = m_multisig_transfers.find(multisig_id);
      if (it != m_multisig_transfers.end())
      {
        //ptc.employed_entries.spent_indices.push_back(i);
        ptc.employed_entries.spent.push_back(wallet_public::employed_tx_entry{ i });
        if (ptc.pmultisig_entries)
        {
          r = ptc.pmultisig_entries->insert(std::make_pair(multisig_id, std::make_pair(tx, ptc.employed_entries))).second;
          if (!r)
          {
            WLT_LOG_RED("Warning: Receiving the same multisig out id from tx pool more then once: " << multisig_id, LOG_LEVEL_0);
          }
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

  //do final calculations
  bool has_in_transfers = false;
  bool has_out_transfers = false;
  for (const auto& bce : ptc.total_balance_change)
  {
    if (bce.second > 0)
    {
      has_in_transfers = true;
    }
    else if (bce.second < 0)
    {
      has_out_transfers = true;
    }
  }
  if (!is_tx_expired(tx, ptc.tx_expiration_ts_median) && (new_multisig_spend_detected || has_in_transfers || has_out_transfers || (currency::is_derivation_used_to_encrypt(tx, derivation))))
  {
    m_unconfirmed_in_transfers[ptc.tx_hash()] = tx;
    if (m_unconfirmed_txs.count(ptc.tx_hash()))
      return;

    //prepare notification about pending transaction
    wallet_public::wallet_transfer_info& unconfirmed_wti = misc_utils::get_or_insert_value_initialized(m_unconfirmed_txs, ptc.tx_hash());
    prepare_wti(unconfirmed_wti, ptc);
    for (auto tr_index : spend_transfers)
    {
      if (tr_index > m_transfers.size())
      {
        WLT_LOG_ERROR("INTERNAL ERROR: tr_index " << tr_index << " more then m_transfers.size()=" << m_transfers.size());
        continue;
      }
      uint32_t flags_before = m_transfers[tr_index].m_flags;
      m_transfers[tr_index].m_flags |= WALLET_TRANSFER_DETAIL_FLAG_SPENT;
      WLT_LOG_L1("wallet transfer #" << tr_index << " is marked as spent, flags: " << flags_before << " -> " << m_transfers[tr_index].m_flags << ", reason: UNCONFIRMED tx: " << ptc.tx_hash());
      unconfirmed_wti.selected_indicies.push_back(tr_index);
    }
    rise_on_transfer2(unconfirmed_wti);
  }
}


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
  multisig_entries_map unconfirmed_multisig_transfers_from_tx_pool;

  has_related_alias_in_unconfirmed = false;
  uint64_t tx_expiration_ts_median = res.tx_expiration_ts_median; //get_tx_expiration_median();
  for (const auto &tx_blob : res.txs)
  {
    currency::transaction tx;
    //money_transfer2_details td;
    process_transaction_context ptc(tx);
    ptc.tx_expiration_ts_median = tx_expiration_ts_median;
    ptc.pmultisig_entries = &unconfirmed_multisig_transfers_from_tx_pool;
    bool r = parse_and_validate_tx_from_blob(tx_blob, tx);
    THROW_IF_TRUE_WALLET_EX(!r, error::tx_parse_error, tx_blob);
    has_related_alias_in_unconfirmed |= has_related_alias_entry_unconfirmed(tx);

    //crypto::hash tx_hash = currency::get_transaction_hash(tx);
    auto it = unconfirmed_in_transfers_local.find(ptc.tx_hash());
    if (it != unconfirmed_in_transfers_local.end())
    {
      m_unconfirmed_in_transfers.insert(*it);
      continue;
    }

    handle_unconfirmed_tx(ptc);
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
  scan_not_compliant_unconfirmed_txs();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::scan_not_compliant_unconfirmed_txs()
{
  uint64_t tx_expiration_ts_median = get_tx_expiration_median();
  uint64_t time_limit = m_core_runtime_config.get_core_time() - CURRENCY_MEMPOOL_TX_LIVETIME;
  for (auto it = m_unconfirmed_txs.begin(); it != m_unconfirmed_txs.end(); )
  {
    bool remove_this_tx = false;
    std::stringstream reason_ss;
    if (it->second.timestamp < time_limit)
    {
      remove_this_tx = true;
      reason_ss << "outdated, ";
    }
    if (is_tx_expired(it->second.tx, tx_expiration_ts_median))
    {
      remove_this_tx = true;
      reason_ss << "expired, ";
    }
    if (is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM) && it->second.tx.version < TRANSACTION_VERSION_POST_HF4)
    {
      remove_this_tx = true;
      reason_ss << "not compliant with HF4, ";
    }

    if (remove_this_tx)
    {
      WLT_LOG_BLUE("removing unconfirmed tx " << it->second.tx_hash << ", reason: " << reason_ss.str() << "tx_expiration_ts_median=" << tx_expiration_ts_median, LOG_LEVEL_0);
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
          m_transfers[i].m_flags &= ~(WALLET_TRANSFER_DETAIL_FLAG_SPENT);  // TODO: consider removing other blocking flags (e.g. for escrow tx) -- sowle
          WLT_LOG_BLUE("mark transfer #" << i << " as unspent, flags: " << flags_before << " -> " << m_transfers[i].m_flags << ", reason: removing unconfirmed tx " << it->second.tx_hash, LOG_LEVEL_0);
        }
      }
      //fire some event
      m_wcallback->on_transfer_canceled(it->second);
      m_unconfirmed_txs.erase(it++);
    }
    else
    {
      it++;
    }
  }

  //scan marked as spent but don't have height (unconfirmed, and actually not unconfirmed)
  std::unordered_set<crypto::key_image> ki_in_unconfirmed;
  for (auto it = m_unconfirmed_txs.begin(); it != m_unconfirmed_txs.end(); it++)
  {
    if (!it->second.has_outgoing_entries())
      continue;

    for (auto& in_v : it->second.tx.vin)
    {
      crypto::key_image ki{};
      if (get_key_image_from_txin_v(in_v, ki))
        ki_in_unconfirmed.insert(ki);
    }
  }

  size_t sz = m_transfers.size();
  for (size_t i = 0; i != sz; i++)
  {
    auto& t = m_transfers[i];
  
    if (t.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT  && !t.m_spent_height && !static_cast<bool>(t.m_flags&WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION)
          && !t.is_htlc())
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
  load_whitelisted_tokens_if_not_loaded();

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
      if (m_wcallback)
        m_wcallback->on_message(tools::i_wallet2_callback::ms_red, "no connection to daemon");
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    catch (const std::exception& e)
    {
      blocks_fetched += added_blocks;
      WLT_LOG_ERROR("refresh->pull_blocks failed, try_count: " << try_count << ", blocks_fetched: " << blocks_fetched << ", exception: " << e.what());
      if (m_wcallback)
        m_wcallback->on_message(tools::i_wallet2_callback::ms_red, std::string("error on pulling blocks: ") + e.what());
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
  

  WLT_LOG("Refresh done, blocks received: " << blocks_fetched, blocks_fetched > 0 ? LOG_LEVEL_1 : LOG_LEVEL_2);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::handle_expiration_list(uint64_t tx_expiration_ts_median)
{
  for (auto it = m_money_expirations.begin(); it != m_money_expirations.end(); )
  {
    if (it->expiration_time < TX_EXPIRATION_MEDIAN_SHIFT || tx_expiration_ts_median > it->expiration_time - TX_EXPIRATION_MEDIAN_SHIFT)
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
        //check for htlc
        if (m_transfers[i].m_ptx_wallet_info->m_tx.vout[m_transfers[i].m_internal_output_index].type() == typeid(tx_out_bare) &&
            boost::get<tx_out_bare>(m_transfers[i].m_ptx_wallet_info->m_tx.vout[m_transfers[i].m_internal_output_index]).target.type() == typeid(txout_htlc))
        {
          //need to find an entry in m_htlc and remove it
          const txout_htlc& hltc = boost::get<txout_htlc>(boost::get<tx_out_bare>(m_transfers[i].m_ptx_wallet_info->m_tx.vout[m_transfers[i].m_internal_output_index]).target);
          uint64_t expiration_height = m_transfers[i].m_ptx_wallet_info->m_block_height + hltc.expiration;
          auto pair_of_it = m_htlcs.equal_range(expiration_height);
          bool found = false;
          for (auto it = pair_of_it.first; it != pair_of_it.second; it++)
          {
            if (it->second.transfer_index == i)
            {
              found = true;
              m_htlcs.erase(it);
              break;
            }
          }
          WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(found, "Internal error: not found record in m_htlcs during rollback");
        }


        if (!(m_transfers[i].m_key_image == null_ki && is_watch_only()))
        {
          auto it_ki = m_key_images.find(m_transfers[i].m_key_image);
          WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it_ki != m_key_images.end(), "key image " << m_transfers[i].m_key_image << " not found");
          WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(m_transfers[i].m_ptx_wallet_info->m_block_height >= including_height, "transfer #" << i << " block height is less than " << including_height);
          m_key_images.erase(it_ki);
        }
        remove_transfer_from_amount_gindex_map(i);
        ++transfers_detached;
      }
      m_transfers.erase(it, m_transfers.end());
    }
  }
 
  for (uint64_t i = get_top_block_height(); i != including_height - 1 && i != 0; i--)
  {
    unprocess_htlc_triggers_on_block_removed(i);
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
      //check if it's hltc contract
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

  //detach in m_last_zc_global_indexs
  while (m_last_zc_global_indexs.size() && including_height <= m_last_zc_global_indexs.begin()->first )
  {
    m_last_zc_global_indexs.erase(m_last_zc_global_indexs.begin());
  }

  //asset descriptors
  handle_rollback_events(including_height);

  WLT_LOG_L0("Detached blockchain on height " << including_height << ", transfers detached " << transfers_detached << ", blocks detached " << blocks_detached);
}
//----------------------------------------------------------------------------------------------------
void wallet2::operator()(const asset_register_event& e)
{
  auto it = m_own_asset_descriptors.find(e.asset_id);
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it != m_own_asset_descriptors.end(), "asset_id " << e.asset_id << "not found during rolling asset_register_event");
  m_own_asset_descriptors.erase(it);
}
void wallet2::operator()(const asset_update_event& e)
{
  auto it = m_own_asset_descriptors.find(e.asset_id);
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it != m_own_asset_descriptors.end(), "asset_id " << e.asset_id << "not found during rolling asset_update_event");
  it->second = e.own_context;
}
void wallet2::operator()(const asset_unown_event& e)
{
  auto it = m_own_asset_descriptors.find(e.asset_id);
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it == m_own_asset_descriptors.end(), "asset_id " << e.asset_id << "unexpectedly found during rolling asset_unown_event");
  m_own_asset_descriptors[e.asset_id] = e.own_context;
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_rollback_events(uint64_t including_height)
{
  while (m_rollback_events.size() && m_rollback_events.back().first >= including_height)
  {
    boost::apply_visitor(*this, m_rollback_events.back().second);
    m_rollback_events.pop_back();
  }
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
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::reset_all()
{
  //static_cast<wallet2_base_state&>(*this) = wallet2_base_state{};
  static_cast<wallet2_base_state&>(*this).~wallet2_base_state();
  new (static_cast<wallet2_base_state*>(this)) wallet2_base_state();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::store_keys(std::string& buff, const std::string& password, wallet2::keys_file_data& keys_file_data, bool store_as_watch_only /* = false */)
{
  currency::account_base acc = m_account;
  if (store_as_watch_only)
    acc.make_account_watch_only();

  std::string account_data;
  bool r = epee::serialization::store_t_to_binary(acc, account_data);
  WLT_CHECK_AND_ASSERT_MES(r, false, "failed to serialize wallet keys");

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
  wallet2::keys_file_data keys_file_data = AUTO_VAL_INIT(keys_file_data);
  bool r = store_keys(buff, m_password, keys_file_data);
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
void wallet2::load_keys(const std::string& buff, const std::string& password, uint64_t file_signature, keys_file_data& kf_data)
{
  bool r = false;
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
  if (m_account.is_watch_only())
    m_watch_only = true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::generate(const std::wstring& path, const std::string& pass, bool auditable_wallet)
{
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(validate_password(pass), "new wallet generation failed: password contains forbidden characters")
  clear();
  prepare_file_names(path);

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
void wallet2::restore(const std::wstring& path, const std::string& pass, const std::string& seed_or_tracking_seed, bool tracking_wallet, const std::string& seed_password)
{
  bool r = false;
  clear();
  prepare_file_names(path);
  m_password = pass;
  
  if (tracking_wallet)
  {
    r = m_account.restore_from_tracking_seed(seed_or_tracking_seed);
    init_log_prefix();
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "Could not load tracking wallet from a given seed: invalid tracking seed");
    m_watch_only = true;
  }
  else
  {
    r = m_account.restore_from_seed_phrase(seed_or_tracking_seed, seed_password);
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
void wallet2::set_votes_config_path(const std::string& path_to_config_file/* = tools::get_default_data_dir() + "\voting_config.json"*/)
{
  m_votes_config_path = path_to_config_file;
}
//----------------------------------------------------------------------------------------------------
void wallet2::load_votes_config()
{
  if (boost::filesystem::exists(m_votes_config_path))
  {
    epee::serialization::load_t_from_json_file(m_votes_config, m_votes_config_path);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::load(const std::wstring& wallet_, const std::string& password)
{
  clear();
  prepare_file_names(wallet_);

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
  THROW_IF_TRUE_WALLET_EX(
    wbh.m_cb_keys > WALLET_FILE_MAX_KEYS_SIZE, error::file_read_error, epee::string_encoding::convert_to_ansii(m_wallet_file));


  keys_buff.resize(wbh.m_cb_keys);
  data_file.read((char*)keys_buff.data(), wbh.m_cb_keys);
  wallet2::keys_file_data kf_data = AUTO_VAL_INIT(kf_data);
  load_keys(keys_buff, password, wbh.m_signature, kf_data);

  bool need_to_resync = false;
  if (wbh.m_ver == WALLET_FILE_BINARY_HEADER_VERSION_INITAL)
  {
    // old WALLET_FILE_BINARY_HEADER_VERSION version means no encryption
    need_to_resync = !tools::portable_unserialize_obj_from_stream(*this, data_file);
    WLT_LOG_L1("Detected format: WALLET_FILE_BINARY_HEADER_VERSION_INITAL (need_to_resync=" << need_to_resync << ")");
  }
  else if (wbh.m_ver == WALLET_FILE_BINARY_HEADER_VERSION_2)
  {
    tools::encrypt_chacha_in_filter decrypt_filter(password, kf_data.iv);
    boost::iostreams::filtering_istream in;
    in.push(decrypt_filter);
    in.push(data_file);
    need_to_resync = !tools::portable_unserialize_obj_from_stream(*this, in);
    WLT_LOG_L1("Detected format: WALLET_FILE_BINARY_HEADER_VERSION_2 (need_to_resync=" << need_to_resync << ")");
  }
  else
  {
    WLT_LOG_L0("Unknown wallet body version(" << wbh.m_ver << "), resync initiated.");
    need_to_resync = true;
  }
    
    

  if (m_watch_only && !is_auditable())
    load_keys2ki(true, need_to_resync);

  boost::system::error_code ec = AUTO_VAL_INIT(ec);
  m_current_wallet_file_size = boost::filesystem::file_size(wallet_, ec);

  WLT_LOG_L0("Loaded wallet file" << (m_watch_only ? " (WATCH ONLY) " : " ") << string_encoding::convert_to_ansii(m_wallet_file) 
    << " with public address: " << m_account.get_public_address_str() 
    << ", file_size=" << m_current_wallet_file_size
    << ", blockchain_size: " << m_chain.get_blockchain_current_size()
  );
  WLT_LOG_L1("[LOADING]Blockchain shortener state: " << ENDL << m_chain.get_internal_state_text());
  
  load_votes_config();

  WLT_LOG_L1("(after loading: pending_key_images: " << m_pending_key_images.size() << ", pki file elements: " << m_pending_key_images_file_container.size() << ", tx_keys: " << m_tx_keys.size() << ")");

  if (need_to_resync)
  {
    reset_history();
    WLT_LOG_L0("Unable to load history data from wallet file, wallet will be resynced!");
  } 

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

  std::string ascii_path_to_save = epee::string_encoding::convert_to_ansii(path_to_save);

  //prepare data
  std::string keys_buff;
  wallet2::keys_file_data keys_file_data = AUTO_VAL_INIT(keys_file_data);
  bool r = store_keys(keys_buff, password, keys_file_data, m_watch_only);
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "failed to store_keys for wallet " << ascii_path_to_save);

  //store data
  wallet_file_binary_header wbh = AUTO_VAL_INIT(wbh);
  wbh.m_signature = WALLET_FILE_SIGNATURE_V2;
  wbh.m_cb_keys = keys_buff.size();
  //@#@ change it to proper
  wbh.m_ver = WALLET_FILE_BINARY_HEADER_VERSION_2;
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
  //creating encryption stream
  tools::encrypt_chacha_out_filter decrypt_filter(m_password, keys_file_data.iv);
  boost::iostreams::filtering_ostream out;
  out.push(decrypt_filter);
  out.push(data_file);

  r = tools::portble_serialize_obj_to_stream(*this, out);
  if (!r)
  {
    data_file.close();
    boost::filesystem::remove(tmp_file_path); // remove tmp file if smth went wrong
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(false, "IO error while storing wallet to " << tmp_file_path.string() << " (portble_serialize_obj_to_stream failed)");
  }

  data_file.flush();
  data_file.close();
  boost::uintmax_t tmp_file_size = boost::filesystem::file_size(tmp_file_path);
  WLT_LOG_L0("Stored successfully to temporary file " << tmp_file_path.string() << ", file size=" << tmp_file_size);

  WLT_LOG_L1("[LOADING]Blockchain shortener state: " << ENDL << m_chain.get_internal_state_text());

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

    WLT_LOG_L0("Wallet was successfully stored to " << ascii_path_to_save << ", file size=" << m_current_wallet_file_size
      << " blockchain_size: " << m_chain.get_blockchain_current_size());
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
void wallet2::set_use_deffered_global_outputs(bool use)
{
  LOG_PRINT_L0("[DEFFERED_MODE]: " << use);
  m_use_deffered_global_outputs = use;
}
//----------------------------------------------------------------------------------------------------
void wallet2::set_use_assets_whitelisting(bool use)
{
  LOG_PRINT_L0("[ASSET_WHITELISTING_MODE]: " << use);
  m_use_assets_whitelisting = use;
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
    if(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].type() != typeid(tx_out_bare))
      continue;
    const currency::txout_target_v& out_t = boost::get<tx_out_bare>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]).target;
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
uint64_t wallet2::balance(uint64_t& unlocked, uint64_t& awaiting_in, uint64_t& awaiting_out, uint64_t& mined, const crypto::public_key& asset_id /* = currency::native_coin_asset_id */) const
{
  uint64_t total = 0;
  unlocked = 0;
  awaiting_in = 0;
  awaiting_out = 0;
  mined = 0;
  std::unordered_map<crypto::public_key, wallet_public::asset_balance_entry_base> balances;
  balance(balances, mined);
  auto it = balances.find(asset_id);
  if (it != balances.end())
  {
    total = it->second.total;
    unlocked = it->second.unlocked;
    awaiting_in = it->second.awaiting_in;
    awaiting_out = it->second.awaiting_out;
  }
  return total;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance(const crypto::public_key& asset_id, uint64_t& unlocked) const
{
  std::unordered_map<crypto::public_key, wallet_public::asset_balance_entry_base> balances;
  uint64_t dummy;
  balance(balances, dummy);
  auto it = balances.find(asset_id);
  if (it == balances.end())
  {
    return 0;
  }
  unlocked = it->second.unlocked;
  return it->second.total;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance(const crypto::public_key& asset_id) const
{
  uint64_t dummy = 0;
  return balance(asset_id, dummy);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::balance(std::unordered_map<crypto::public_key, wallet_public::asset_balance_entry_base>& balances, uint64_t& mined) const
{
  mined = 0;
  m_has_bare_unspent_outputs = false;
  
  for(auto& td : m_transfers)
  {
    if (td.is_spendable() || (td.is_reserved_for_escrow() && !td.is_spent()))
    {
      wallet_public::asset_balance_entry_base& e = balances[td.get_asset_id()];
      e.total += td.amount();
      if (is_transfer_unlocked(td))
        e.unlocked += td.amount();
      if (td.m_flags & WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER)
      {
        if (td.m_ptx_wallet_info->m_block_height == 0)
        {
          //for genesis block we add actual amounts
          mined += td.amount();
        }
        else {
          mined += CURRENCY_BLOCK_REWARD; //this code would work only for cases where block reward is full. For reduced block rewards might need more flexible code (TODO)
        }        
      }

      if (!td.is_zc())
        m_has_bare_unspent_outputs = true;
    }
  }

  for(auto& utx : m_unconfirmed_txs)
  {
    for (auto& subtransfer : utx.second.subtransfers)
    {
      wallet_public::asset_balance_entry_base& e = balances[subtransfer.asset_id];
      if (subtransfer.is_income)
      {
        e.total += subtransfer.amount;
        e.awaiting_in += subtransfer.amount;
      }
      else
      {
        e.awaiting_out += subtransfer.amount;
        if (subtransfer.asset_id == currency::native_coin_asset_id)
        {
          // this "if" present here only due to sophisticated checks in escrow_custom_test, which 
          // inaccuracy might be driven by tangled processing of sent transactions and unconfirmed 
          // transactions in pre-refactoring era (few weeks before this commit)
          if (!(utx.second.contract.size() && utx.second.contract[0].state == wallet_public::escrow_contract_details_basic::contract_released_burned))
          {
            e.awaiting_out -= currency::get_tx_fee(utx.second.tx);
          }
        }
      }
    }
    if (utx.second.has_outgoing_entries())
    {
      //collect change to unconfirmed
      for (const auto& emp_entry : utx.second.employed_entries.receive)
      {
        wallet_public::asset_balance_entry_base& e = balances[emp_entry.asset_id];
        e.total += emp_entry.amount;
      }

    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::balance(std::list<wallet_public::asset_balance_entry>& balances, uint64_t& mined) const
{
  load_whitelisted_tokens_if_not_loaded();
  balances.clear();
  std::unordered_map<crypto::public_key, wallet_public::asset_balance_entry_base> balances_map;
  this->balance(balances_map, mined);
  std::unordered_map<crypto::public_key, currency::asset_descriptor_base> custom_assets_local = m_custom_assets;

  for (auto& own_asset : m_own_asset_descriptors)
  {
    if (m_whitelisted_assets.find(own_asset.first) == m_whitelisted_assets.end())
    {
      custom_assets_local[own_asset.first] = own_asset.second;
    }
  }

  asset_descriptor_base native_asset_info = AUTO_VAL_INIT(native_asset_info);
  native_asset_info.full_name             = CURRENCY_NAME_SHORT_BASE;
  native_asset_info.ticker                = CURRENCY_NAME_ABR;
  native_asset_info.decimal_point         = CURRENCY_DISPLAY_DECIMAL_POINT;
  custom_assets_local[currency::native_coin_asset_id] = native_asset_info;

  for (const auto& item : balances_map)
  {
    asset_descriptor_base asset_info = AUTO_VAL_INIT(asset_info);
    //check if asset is whitelisted or customly added

    //check if it custom asset
    auto it_cust = custom_assets_local.find(item.first);
    if(it_cust == custom_assets_local.end()) 
    {
      if(!m_use_assets_whitelisting)
        continue;

      auto it_local = m_whitelisted_assets.find(item.first);
      if(it_local == m_whitelisted_assets.end())
      {
        WLT_LOG_YELLOW("WARNING: unknown asset " << item.first << " found and skipped; it's NOT included in balance", LOG_LEVEL_1);
        continue;
      }
      else 
      {
        asset_info = it_local->second;
      }
    }
    else 
    {
      asset_info = it_cust->second;
      custom_assets_local.erase(it_cust);
    }
 
    balances.push_back(wallet_public::asset_balance_entry());
    wallet_public::asset_balance_entry& new_item = balances.back();
    static_cast<wallet_public::asset_balance_entry_base&>(new_item) = item.second;
    new_item.asset_info.asset_id = item.first;
    static_cast<currency::asset_descriptor_base&>(new_item.asset_info) = asset_info;
  }
  //manually added assets should be always present, at least as zero balanced items
  for (auto& asset : custom_assets_local)
  {
    balances.push_back(wallet_public::asset_balance_entry());
    wallet_public::asset_balance_entry& new_item = balances.back();
    new_item.asset_info.asset_id = asset.first;
    static_cast<currency::asset_descriptor_base&>(new_item.asset_info) = asset.second;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_asset_id_info(const crypto::public_key& asset_id, currency::asset_descriptor_base& asset_info, bool& whitelist_) const
{
  if (asset_id == currency::native_coin_asset_id)
  {
    asset_info = currency::get_native_coin_asset_descriptor();
    whitelist_ = true;
    return true;
  }
  //check if asset is whitelisted or customly added
  whitelist_ = false;
  auto it_white = m_whitelisted_assets.find(asset_id);
  if (it_white == m_whitelisted_assets.end())
  {
    //check if it custom asset
    auto it_cust = m_custom_assets.find(asset_id);
    if (it_cust == m_custom_assets.end())
    {
      return false;
    }
    else
    {
      asset_info = it_cust->second;
    }
  }
  else
  {
    asset_info = it_white->second;
    whitelist_ = true;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------

uint64_t wallet2::balance() const
{
  uint64_t stub = 0;
  return balance(stub, stub, stub, stub);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::add_custom_asset_id(const crypto::public_key& asset_id, asset_descriptor_base& asset_descriptor)
{
  currency::COMMAND_RPC_GET_ASSET_INFO::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_ASSET_INFO::response resp = AUTO_VAL_INIT(resp);
  req.asset_id = asset_id;

  bool r = m_core_proxy->call_COMMAND_RPC_GET_ASSET_INFO(req, resp);
  if (r && resp.status == API_RETURN_CODE_OK)
  {
    m_custom_assets[asset_id] = resp.asset_descriptor;
    asset_descriptor = resp.asset_descriptor;
    return true;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::delete_custom_asset_id(const crypto::public_key& asset_id)
{
  const auto it = m_custom_assets.find(asset_id);
  if (it != m_custom_assets.end())
  {
    m_custom_assets.erase(it);
  }
 
  return true;
}
//----------------------------------------------------------------------------------------------------
const std::unordered_map<crypto::public_key, currency::asset_descriptor_base>& wallet2::get_local_whitelist() const
{
  return m_custom_assets;
}
//----------------------------------------------------------------------------------------------------
const std::unordered_map<crypto::public_key, currency::asset_descriptor_base>& wallet2::get_global_whitelist() const
{
  return m_whitelisted_assets;
}
//----------------------------------------------------------------------------------------------------
const std::unordered_map<crypto::public_key, tools::wallet_own_asset_context>& wallet2::get_own_assets() const
{
  return m_own_asset_descriptors;
}
  //----------------------------------------------------------------------------------------------------
bool wallet2::load_whitelisted_tokens() const
{
  if(!m_use_assets_whitelisting)
    return true;

  m_whitelisted_assets.clear();
  std::string body;
  wallet_public::assets_whitelist aw = AUTO_VAL_INIT(aw);
  if (epee::net_utils::get_http_json_t(WALLET_ASSETS_WHITELIST_URL, aw))
  {
    for (auto it = aw.assets.begin(); it != aw.assets.end(); it++)
    {
      m_whitelisted_assets[it->asset_id] = static_cast<currency::asset_descriptor_base>(*it);
    }
    return true;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::load_whitelisted_tokens_if_not_loaded() const
{
  if (m_whitelist_updated)
  {
    return true;
  }
  if (!load_whitelisted_tokens())
    return false;
  m_whitelist_updated = true;
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_transfers(transfer_container& incoming_transfers) const
{
  incoming_transfers = m_transfers;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::generate_utxo_defragmentation_transaction_if_needed(currency::transaction& tx)
{
  if (!m_defragmentation_tx_enabled)
    return false;

  construct_tx_param ctp = get_default_construct_tx_param();
  ctp.create_utxo_defragmentation_tx = true;
  finalized_tx ftp{};

  transfer(ctp, ftp, false, nullptr);

  if (ftp.was_not_prepared)
      return false; // no such UTXO were found, not an error

  tx = ftp.tx;
  return true;
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::get_transfers_str(bool include_spent /*= true*/, bool include_unspent /*= true*/, bool show_only_unknown /*= false*/, const std::string& filter_asset_ticker /*= std::string{}*/) const
{
  static const char* header = "index                 amount  ticker  g_index  flags       block  tx                                                                out#  asset id";
  std::stringstream ss;
  ss << header << ENDL;
  size_t count = 0;
  size_t unknown_assets_outs_count = 0;
  for (size_t i = 0; i != m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];

    if ((td.is_spent() && !include_spent) || (!td.is_spent() && !include_unspent))
      continue;

    bool native_coin = td.is_native_coin();
    asset_descriptor_base adb{};
    bool whitelisted = false;
    if (get_asset_id_info(td.get_asset_id(), adb, whitelisted) == show_only_unknown)
    {
      if (!show_only_unknown)
        ++unknown_assets_outs_count;
      continue;
    }

    if (!filter_asset_ticker.empty() && adb.ticker != filter_asset_ticker)
      continue;

    ss << std::right <<
      std::setw(5) << i << "  " <<
      std::setw(21) << print_asset_money(td.m_amount, adb.decimal_point) << "  " <<
      std::setw(6) << std::left << (native_coin ? std::string("      ") : adb.ticker) << "  " << std::right <<
      std::setw(7) << td.m_global_output_index << "  " <<
      std::setw(2) << std::setfill('0') << td.m_flags << std::setfill(' ') << ":" <<
      std::setw(5) << transfer_flags_to_str(td.m_flags) << "  " <<
      std::setw(7) << td.m_ptx_wallet_info->m_block_height << "  " <<
      get_transaction_hash(td.m_ptx_wallet_info->m_tx) << "  " <<
      std::setw(4) << td.m_internal_output_index << "  ";
    if (native_coin)
      ss << "                                                                ";
    else
      ss << td.get_asset_id();
    
    ss << ENDL;

    ++count;
  }

  ss << "printed " << count << " outputs of " << m_transfers.size() << " total" << ENDL;
  if (unknown_assets_outs_count == 1)
    ss << "(" << unknown_assets_outs_count << " output with unrecognized asset id is not shown, use 'list_outputs unknown' to see it)" << ENDL;
  else if (unknown_assets_outs_count > 1)
    ss << "(" << unknown_assets_outs_count << " outputs with unrecognized asset ids are not shown, use 'list_outputs unknown' to see them)" << ENDL;
  return ss.str();
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::get_balance_str() const
{
  // balance unlocked     / [balance total]       ticker   asset id
  // 1391306.970000000000 / 1391306.970000000000  ZANO     d6329b5b1f7c0805b5c345f4957554002a2f557845f64d7645dae0e051a6498a
  // 1391306.97                                   ZANO     d6329b5b1f7c0805b5c345f4957554002a2f557845f64d7645dae0e051a6498a
  //     106.971          /     206.4             ZANO     d6329b5b1f7c0805b5c345f4957554002a2f557845f64d7645dae0e051a6498a

  static const char* header = " balance unlocked     / [balance total]       ticker   asset id";
  std::stringstream ss;
  ss << header << ENDL;

  std::list<tools::wallet_public::asset_balance_entry> balances;
  uint64_t mined = 0;
  balance(balances, mined);
  for (const tools::wallet_public::asset_balance_entry& b : balances)
  {
    ss << " " << std::left << std::setw(20) << print_fixed_decimal_point_with_trailing_spaces(b.unlocked, b.asset_info.decimal_point);
    if (b.total == b.unlocked)
      ss << "                       ";
    else
      ss << " / " << std::setw(20) << print_fixed_decimal_point_with_trailing_spaces(b.total, b.asset_info.decimal_point);
    ss << "  " << std::setw(8) << std::left << b.asset_info.ticker << " " << b.asset_info.asset_id << ENDL;
  }

  return ss.str();
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::get_balance_str_raw() const
{
    // balance unlocked     / [balance total]       ticker   asset id
  // 1391306.970000000000 / 1391306.970000000000  ZANO     d6329b5b1f7c0805b5c345f4957554002a2f557845f64d7645dae0e051a6498a
  // 1391306.97                                   ZANO     d6329b5b1f7c0805b5c345f4957554002a2f557845f64d7645dae0e051a6498a
  //     106.971          /     206.4             ZANO     d6329b5b1f7c0805b5c345f4957554002a2f557845f64d7645dae0e051a6498a

  static const char* header = " balance unlocked     / [balance total]       asset id";
  std::stringstream ss;
  ss << header << ENDL;
  
  uint64_t dummy = 0;
  std::unordered_map<crypto::public_key, wallet_public::asset_balance_entry_base> balances_map;
  this->balance(balances_map, dummy);

  for(const auto& entry : balances_map)
  {
    ss << " " << std::left << std::setw(20) << print_fixed_decimal_point_with_trailing_spaces(entry.second.unlocked, 12);
    if(entry.second.total == entry.second.unlocked)
      ss << "                       ";
    else
      ss << " / " << std::setw(20) << print_fixed_decimal_point_with_trailing_spaces(entry.second.total, 12);
    ss << "  " << std::setw(8) << std::left << entry.first << ENDL;
  }

  //print whitelist
  ss << "WHITELIST: " << ENDL;


  for(const auto& entry : m_whitelisted_assets)
  {
    ss << " " << std::left << entry.first << "    " << entry.second.ticker << ENDL; 
  }

    // print whitelist
  ss << "CUSTOM LIST: " << ENDL;


  for(const auto& entry : m_custom_assets)
  {
    ss << " " << std::left << entry.first << "    " << entry.second.ticker << ENDL;
  }

  ss << "OWN DESCRIPTORS LIST: " << ENDL;
  
  for(const auto& entry : m_own_asset_descriptors)
  {
    ss << " " << std::left << entry.first << "    " << entry.second.ticker << ENDL;
  }

  return ss.str();
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_payments(const std::string& payment_id, std::list<payment_details>& payments, uint64_t min_height) const
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
  currency::finalized_tx ft = AUTO_VAL_INIT(ft);
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
    VARIANT_SWITCH_BEGIN(ft.tx.vout[i]);
    VARIANT_CASE_CONST(tx_out_bare, out)
    {
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
    VARIANT_CASE_CONST(tx_out_zarcanum, o);
    //@#@      
    VARIANT_SWITCH_END();
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
  //TODO@#@
  /*
  prepare_free_transfers_cache(0);
  for (auto ent : m_found_free_amounts)
  {
    distribution[ent.first] = ent.second.size();
  }
  */

  return false;
}
//----------------------------------------------------------------------------------------------------
void wallet2::submit_transfer(const std::string& signed_tx_blob, currency::transaction& tx)
{
  // decrypt sources
  std::string decrypted_src_blob = crypto::chacha_crypt(signed_tx_blob, m_account.get_keys().view_secret_key);

  // deserialize tx data
  currency::finalized_tx ft = AUTO_VAL_INIT(ft);
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

  add_sent_tx_detailed_info(tx, ft.ftp.attachments, ft.ftp.prepared_destinations, ft.ftp.selected_transfers);
  m_tx_keys.insert(std::make_pair(tx_hash, ft.one_time_key));

  if (m_watch_only)
  {
    std::vector<std::pair<crypto::public_key, crypto::key_image>> pk_ki_to_be_added;
    std::vector<std::pair<uint64_t, crypto::key_image>> tri_ki_to_be_added;

    for (auto& p : ft.outs_key_images)
    {
      THROW_IF_FALSE_WALLET_INT_ERR_EX(p.first < tx.vout.size(), "outs_key_images has invalid out index: " << p.first << ", tx.vout.size() = " << tx.vout.size());
      THROW_IF_FALSE_WALLET_INT_ERR_EX(tx.vout[p.first].type() == typeid(tx_out_bare), "Unexpected type in submit_transfer: " << tx.vout[p.first].type().name());
      auto& out = boost::get<tx_out_bare>(tx.vout[p.first]);
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
      const crypto::public_key& out_key = src.outputs[src.real_output].stealth_address;

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

template<typename callback_t, typename iterator_t>
bool enum_container(iterator_t it_begin, iterator_t it_end, callback_t cb)
{
  for (iterator_t it = it_begin; it != it_end; it++)
  {
    if (!cb(*it, it - it_begin))
      return true;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_defragmentation_transaction(const wallet_public::wallet_transfer_info& wti)
{
  if (wti.employed_entries.receive.size() && wti.employed_entries.spent.size() && wti.subtransfers.size() == 1)
  {
    if (wti.subtransfers[0].asset_id == currency::native_coin_asset_id && !wti.subtransfers[0].is_income && wti.subtransfers[0].amount == get_tx_fee(wti.tx))
      return true;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_recent_transfers_history(std::vector<wallet_public::wallet_transfer_info>& trs, size_t offset, size_t count, uint64_t& total, uint64_t& last_item_index, bool exclude_mining_txs, bool start_from_end)
{
  if (!count || offset >= m_transfer_history.size())
    return;

  auto cb = [&](wallet_public::wallet_transfer_info& wti, size_t local_offset) {
  
    if (exclude_mining_txs)
    {
      if (currency::is_coinbase(wti.tx) || is_defragmentation_transaction(wti))
        return true;
    }
    trs.push_back(wti);
    load_wallet_transfer_info_flags(trs.back());
    last_item_index = offset + local_offset;
    trs.back().transfer_internal_index = last_item_index;

    if (wti.remote_addresses.size() == 1)
    {
      wti.remote_aliases = get_aliases_for_address(wti.remote_addresses[0]);
    }

    if (trs.size() >= count)
    {
      return false;
    }
    return true;
  };
  
  if(start_from_end)
    enum_container(m_transfer_history.rbegin() + offset, m_transfer_history.rend(), cb);
  else 
    enum_container(m_transfer_history.begin() + offset, m_transfer_history.end(), cb);

  total = m_transfer_history.size();

}

void wallet2::wti_to_csv_entry(std::ostream& ss, const wallet_public::wallet_transfer_info& wti, size_t index) 
{
  for(auto& subtr: wti.subtransfers)
  {
    ss << index << ",";
    ss << epee::misc_utils::get_time_str(wti.timestamp) << ",";
    ss << print_money(subtr.amount) << ",";
    ss << subtr.asset_id << ",";
    ss << "\"" << wti.comment << "\",";
    ss << "[";
    std::copy(wti.remote_addresses.begin(), wti.remote_addresses.end(), std::ostream_iterator<std::string>(ss, " "));
    ss << "]" << ",";
    ss << wti.tx_hash << ",";
    ss << wti.height << ",";
    ss << wti.unlock_time << ",";
    ss << wti.tx_blob_size << ",";
    ss << epee::string_tools::buff_to_hex_nodelimer(wti.payment_id) << ",";
    ss << "[";
    std::copy(wti.remote_aliases.begin(), wti.remote_aliases.end(), std::ostream_iterator<std::string>(ss, " "));
    ss << "]" << ",";
    ss << (subtr.is_income ? "in" : "out") << ",";
    ss << (wti.is_service ? "[SERVICE]" : "") << (wti.is_mixing ? "[MIXINS]" : "") << (wti.is_mining ? "[MINING]" : "") << ",";
    ss << wti.tx_type << ",";
    ss << print_money(wti.fee) << ENDL;
  }

};

void wallet2::wti_to_txt_line(std::ostream& ss, const wallet_public::wallet_transfer_info& wti, size_t index) 
{
  for (auto& subtr : wti.subtransfers)
  {
    ss << (subtr.is_income ? "[INC]" : "[OUT]") << "\t"
      << epee::misc_utils::get_time_str(wti.timestamp) << "\t"
      << print_money(subtr.amount) << "\t"
      << subtr.asset_id << "\t"
      << print_money(wti.fee) << "\t"
      << wti.remote_addresses << "\t"
      << wti.comment << ENDL;
  }
};

void wallet2::wti_to_json_line(std::ostream& ss, const wallet_public::wallet_transfer_info& wti, size_t index) 
{
  ss << epee::serialization::store_t_to_json(wti, 4) << ",";
};

//----------------------------------------------------------------------------------------------------
void wallet2::set_connectivity_options(unsigned int timeout)
{
  m_core_proxy->set_connectivity(timeout, WALLET_RCP_COUNT_ATTEMNTS);
}
//----------------------------------------------------------------------------------------------------
void wallet2::export_transaction_history(std::ostream& ss, const std::string& format,  bool include_pos_transactions)
{
  //typedef int(*t_somefunc)(int, int);
  typedef void(*playout_cb_type)(std::ostream&, const wallet_public::wallet_transfer_info&, size_t);
  playout_cb_type cb_csv = &wallet2::wti_to_csv_entry;
  playout_cb_type cb_json = &wallet2::wti_to_json_line;
  playout_cb_type cb_plain_text = &wallet2::wti_to_txt_line;

  playout_cb_type cb = cb_csv;
  if (format == "json")
  {
    ss << "{ \"history\": [";
    cb = cb_json;
  }
  else if (format == "text")
  {
    cb = cb_plain_text;
  }
  else
  {
    //csv by default
    ss << "N, Date, Amount, AssetID, Comment, Address, ID, Height, Unlock timestamp, Tx size, Alias, PaymentID, In/Out, Flags, Type, Fee" << ENDL;
  }


  enum_container(m_transfer_history.begin(), m_transfer_history.end(), [&](wallet_public::wallet_transfer_info& wti, size_t index) {
    if (!include_pos_transactions)
    {
      if (currency::is_coinbase(wti.tx))
        return true;
    }
    wti.fee = currency::get_tx_fee(wti.tx);
    cb(ss, wti, index);
    return true;
  });

  if (format == "json")
  {
    ss << "{}]}";
  }

}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id)
{
  return m_core_proxy->get_transfer_address(adr_str, addr, payment_id);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_okay_for_pos(const transfer_details& tr, bool is_zarcanum_hf, uint64_t& stake_unlock_time) const
{
  if (is_zarcanum_hf)
  {
    if (!tr.is_zc())
      return false;

    if (!tr.is_native_coin())
      return false;
  }

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
void wallet2::get_mining_history(wallet_public::mining_history& hist, uint64_t timestamp_from)
{
  for (auto& tr : m_transfer_history)
  {
    if (currency::is_coinbase(tr.tx) && tr.tx.vin.size() == 2 && tr.timestamp > timestamp_from)
    {
      tools::wallet_public::mining_history_entry mhe = AUTO_VAL_INIT(mhe);
      mhe.a = tr.get_native_income_amount();
      mhe.t = tr.timestamp;
      mhe.h = tr.height;
      hist.mined_entries.push_back(mhe);
    }
  }
}
//----------------------------------------------------------------------------------------------------
size_t wallet2::get_pos_entries_count()
{
  bool is_zarcanum_hf = is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM);
  size_t counter = 0;

  for (size_t i = 0, size = m_transfers.size(); i < size; i++)
  {
    auto& tr = m_transfers[i];

    uint64_t stake_unlock_time = 0;
    if (!is_transfer_okay_for_pos(tr, is_zarcanum_hf, stake_unlock_time))
      continue;

    ++counter;
  }

  return counter;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_pos_entries(std::vector<currency::pos_entry>& entries)
{
  bool is_zarcanum_hf = is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM);
  for (size_t i = 0; i != m_transfers.size(); i++)
  {
    auto& tr = m_transfers[i];

    uint64_t stake_unlock_time = 0;
    if (!is_transfer_okay_for_pos(tr, is_zarcanum_hf, stake_unlock_time))
      continue;

    pos_entry pe = AUTO_VAL_INIT(pe);
    pe.amount = tr.amount();
    pe.g_index = tr.m_global_output_index;
    pe.keyimage = tr.m_key_image;
    pe.wallet_index = i;
    pe.stake_unlock_time = stake_unlock_time;
    pe.block_timestamp = tr.m_ptx_wallet_info->m_block_timestamp;
    entries.push_back(pe);
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_in_hardfork_zone(uint64_t hardfork_index) const
{
  return m_core_runtime_config.is_hardfork_active_for_height(hardfork_index, get_blockchain_current_size());
}
//----------------------------------------------------------------------------------------------------
bool wallet2::proxy_to_daemon(const std::string& uri, const std::string& body, int& response_code, std::string& response_body)
{
  return m_core_proxy->call_COMMAND_RPC_INVOKE(uri, body, response_code, response_body);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_and_sign_pos_block(const mining_context& cxt, uint64_t full_block_reward, const currency::pos_entry& pe, currency::tx_generation_context& miner_tx_tgc, currency::block& b) const
{
  bool r = false;
  WLT_CHECK_AND_ASSERT_MES(pe.wallet_index < m_transfers.size(), false, "invalid pe.wallet_index: " << pe.wallet_index);
  const transfer_details& td = m_transfers[pe.wallet_index];
  const transaction& source_tx = td.m_ptx_wallet_info->m_tx;
  const crypto::public_key source_tx_pub_key = get_tx_pub_key_from_extra(source_tx);
  WLT_CHECK_AND_ASSERT_MES(pe.tx_out_index < source_tx.vout.size(), false, "invalid pe.tx_out_index: " << pe.tx_out_index);
  const currency::tx_out_v& stake_out_v = source_tx.vout[pe.tx_out_index];

  // calculate stake_out_derivation and secret_x (derived ephemeral secret key)
  crypto::key_derivation stake_out_derivation = AUTO_VAL_INIT(stake_out_derivation);
  r = crypto::generate_key_derivation(source_tx_pub_key, m_account.get_keys().view_secret_key, stake_out_derivation);               // d = 8 * v * R
  WLT_CHECK_AND_ASSERT_MES(r, false, "generate_key_derivation failed, tid: " << pe.wallet_index << ", pe.tx_id: " << pe.tx_id);
  crypto::secret_key secret_x = AUTO_VAL_INIT(secret_x);
  crypto::derive_secret_key(stake_out_derivation, pe.tx_out_index, m_account.get_keys().spend_secret_key, secret_x);                // x = Hs(8 * v * R, i) + s

  if (!cxt.zarcanum)
  {
    // old PoS with non-hidden amounts
    WLT_CHECK_AND_ASSERT_MES(b.miner_tx.vin[0].type() == typeid(currency::txin_gen), false, "Wrong input 0 type in transaction: " << b.miner_tx.vin[0].type().name());
    WLT_CHECK_AND_ASSERT_MES(b.miner_tx.vin[1].type() == typeid(currency::txin_to_key), false, "Wrong input 1 type in transaction: " << b.miner_tx.vin[1].type().name());
    WLT_CHECK_AND_ASSERT_MES(b.miner_tx.signatures.size() == 1 && b.miner_tx.signatures[0].type() == typeid(NLSAG_sig), false, "wrong sig prepared in a PoS block");
    WLT_CHECK_AND_ASSERT_MES(stake_out_v.type() == typeid(tx_out_bare), false, "unexpected stake output type: " << stake_out_v.type().name() << ", expected: tx_out_bare");
    const tx_out_bare& stake_out = boost::get<tx_out_bare>(stake_out_v);
    WLT_CHECK_AND_ASSERT_MES(stake_out.target.type() == typeid(txout_to_key), false, "unexpected stake output target type: " << stake_out.target.type().name() << ", expected: txout_to_key");
    
    NLSAG_sig& sig = boost::get<NLSAG_sig>(b.miner_tx.signatures[0]);
    txin_to_key& stake_input = boost::get<txin_to_key>(b.miner_tx.vin[1]);
    const txout_to_key& stake_out_target = boost::get<txout_to_key>(stake_out.target);
    
    // partially fill stake input
    stake_input.k_image = pe.keyimage;
    stake_input.amount = pe.amount;

    // get decoys outputs and construct miner tx
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response decoys_resp = AUTO_VAL_INIT(decoys_resp);
    std::vector<const crypto::public_key*> ring;
    uint64_t secret_index = 0; // index of the real stake output
    if (m_required_decoys_count > 0 && !is_auditable())
    {
      COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request decoys_req = AUTO_VAL_INIT(decoys_req);
      decoys_req.height_upper_limit = std::min(m_last_pow_block_h, m_last_known_daemon_height > m_core_runtime_config.min_coinstake_age ? m_last_known_daemon_height - m_core_runtime_config.min_coinstake_age : m_last_pow_block_h);
      decoys_req.use_forced_mix_outs = false;
      decoys_req.decoys_count = m_required_decoys_count + 1; // one more to be able to skip a decoy in case it hits the real output
      decoys_req.amounts.push_back(pe.amount); // request one batch of decoys

      r = m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(decoys_req, decoys_resp);
      // TODO @#@# do we need these exceptions?
      THROW_IF_FALSE_WALLET_EX(r, error::no_connection_to_daemon, "getrandom_outs1.bin");
      THROW_IF_FALSE_WALLET_EX(decoys_resp.status != API_RETURN_CODE_BUSY, error::daemon_busy, "getrandom_outs1.bin");
      THROW_IF_FALSE_WALLET_EX(decoys_resp.status == API_RETURN_CODE_OK, error::get_random_outs_error, decoys_resp.status);
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(decoys_resp.outs.size() == 1, "got wrong number of decoys batches: " << decoys_resp.outs.size());
    
      // we expect that less decoys can be returned than requested, we will use them all anyway
      WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(decoys_resp.outs[0].outs.size() <= m_required_decoys_count + 1, "for PoS stake tx got greater decoys to mix than requested: " << decoys_resp.outs[0].outs.size() << " < " << m_required_decoys_count + 1);

      auto& ring_candidates = decoys_resp.outs[0].outs;
      ring_candidates.emplace_front(td.m_global_output_index, stake_out_target.key);

      std::unordered_set<uint64_t> used_gindices;
      size_t good_outs_count = 0;
      for(auto it = ring_candidates.begin(); it != ring_candidates.end(); )
      {
        if (used_gindices.count(it->global_amount_index) != 0)
        {
          it = ring_candidates.erase(it);
          continue;
        }
        used_gindices.insert(it->global_amount_index);
        if (++good_outs_count == m_required_decoys_count + 1)
        {
          ring_candidates.erase(++it, ring_candidates.end());
          break;
        }
        ++it;
      }
    
      // won't assert that ring_candidates.size() == m_required_decoys_count + 1 here as we will use all the decoys anyway
      if (ring_candidates.size() < m_required_decoys_count + 1)
        LOG_PRINT_YELLOW("PoS: using " << ring_candidates.size() - 1 << " decoys for mining tx, while " << m_required_decoys_count << " are required", LOG_LEVEL_1);

      ring_candidates.sort([](auto& l, auto& r){ return l.global_amount_index < r.global_amount_index; }); // sort them now (note absolute_sorted_output_offsets_to_relative_in_place() below)

      uint64_t i = 0;
      for(auto& el : ring_candidates)
      {
        uint64_t gindex = el.global_amount_index;
        if (gindex == td.m_global_output_index)
          secret_index = i;
        ++i;
        ring.emplace_back(&el.stealth_address);
        stake_input.key_offsets.push_back(el.global_amount_index);
      }
      r = absolute_sorted_output_offsets_to_relative_in_place(stake_input.key_offsets);
      WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "absolute_sorted_output_offsets_to_relative_in_place failed");
    }
    else
    {
      // no decoys, the ring consist of one element -- the real stake output
      ring.emplace_back(&stake_out_target.key);
      stake_input.key_offsets.push_back(td.m_global_output_index);
    }

    // sign block actually in coinbase transaction
    crypto::hash block_hash = currency::get_block_hash(b);

    // generate sring signature
    sig.s.resize(ring.size());
    crypto::generate_ring_signature(block_hash, stake_input.k_image, ring, secret_x, secret_index, sig.s.data());

    if (epee::log_space::get_set_log_detalisation_level() >= LOG_LEVEL_4)
    {
      std::stringstream ss;
      ss << "GENERATED RING SIGNATURE for PoS block coinbase:" << ENDL <<
        "    block hash: " << block_hash << ENDL <<
        "    key image:  " << stake_input.k_image << ENDL <<
        "    ring:" << ENDL;
      for(auto el: ring)
        ss << "        " << *el << ENDL;
      ss << "    signature:" << ENDL;
      for(auto el: sig.s)
        ss << "        " << el << ENDL;
      WLT_LOG_L4(ss.str());
    }

    return true;
  }

  // Zarcanum

  WLT_CHECK_AND_ASSERT_MES(td.is_zc(), false, "the transfer [" << pe.wallet_index << "] is not zc type, which is required for zarcanum");
  WLT_CHECK_AND_ASSERT_MES(b.miner_tx.vin[0].type() == typeid(currency::txin_gen), false, "Wrong input 0 type in transaction: " << b.miner_tx.vin[0].type().name());
  WLT_CHECK_AND_ASSERT_MES(b.miner_tx.vin[1].type() == typeid(currency::txin_zc_input), false, "Wrong input 1 type in transaction: " << b.miner_tx.vin[1].type().name());
  WLT_CHECK_AND_ASSERT_MES(b.miner_tx.signatures.size() == 1 && b.miner_tx.signatures[0].type() == typeid(zarcanum_sig), false, "wrong sig prepared in a PoS block");
  WLT_CHECK_AND_ASSERT_MES(stake_out_v.type() == typeid(tx_out_zarcanum), false, "unexpected stake output type: " << stake_out_v.type().name() << ", expected: tx_out_zarcanum");
  WLT_CHECK_AND_ASSERT_MES(td.m_zc_info_ptr->asset_id == currency::native_coin_asset_id, false, "attempted to stake an output with a non-native asset id");

  zarcanum_sig& sig = boost::get<zarcanum_sig>(b.miner_tx.signatures[0]);
  txin_zc_input& stake_input = boost::get<txin_zc_input>(b.miner_tx.vin[1]);
  const tx_out_zarcanum& stake_out = boost::get<tx_out_zarcanum>(stake_out_v);

  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response decoys_resp = AUTO_VAL_INIT(decoys_resp);
  std::vector<crypto::CLSAG_GGXXG_input_ref_t> ring;
  uint64_t secret_index = 0; // index of the real stake output

  // get decoys outputs and construct miner tx
  const size_t required_decoys_count = m_core_runtime_config.hf4_minimum_mixins == 0 ? 4 /* <-- for tests */ : m_core_runtime_config.hf4_minimum_mixins;
  static bool use_only_forced_to_mix = false;       // TODO @#@# set them somewhere else
  if (required_decoys_count > 0 && !is_auditable())
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request decoys_req = AUTO_VAL_INIT(decoys_req);
    decoys_req.height_upper_limit = m_last_pow_block_h; // request decoys to be either older than, or the same age as stake output's height
    decoys_req.use_forced_mix_outs = use_only_forced_to_mix;
    decoys_req.decoys_count = required_decoys_count + 1; // one more to be able to skip a decoy in case it hits the real output
    decoys_req.amounts.push_back(0); // request one batch of decoys for hidden amounts

    r = m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(decoys_req, decoys_resp);
    // TODO @#@# do we need these exceptions?
    THROW_IF_FALSE_WALLET_EX(r, error::no_connection_to_daemon, "getrandom_outs1.bin");
    THROW_IF_FALSE_WALLET_EX(decoys_resp.status != API_RETURN_CODE_BUSY, error::daemon_busy, "getrandom_outs1.bin");
    THROW_IF_FALSE_WALLET_EX(decoys_resp.status == API_RETURN_CODE_OK, error::get_random_outs_error, decoys_resp.status);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(decoys_resp.outs.size() == 1, "got wrong number of decoys batches: " << decoys_resp.outs.size());
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(decoys_resp.outs[0].outs.size() == required_decoys_count + 1, "for PoS stake tx got less decoys to mix than requested: " << decoys_resp.outs[0].outs.size() << " < " << required_decoys_count + 1);

    auto& decoys = decoys_resp.outs[0].outs;
    decoys.emplace_front(td.m_global_output_index, stake_out.stealth_address, stake_out.amount_commitment, stake_out.concealing_point, stake_out.blinded_asset_id);

    std::unordered_set<uint64_t> used_gindices;
    size_t good_outs_count = 0;
    for(auto it = decoys.begin(); it != decoys.end(); )
    {
      if (used_gindices.count(it->global_amount_index) != 0)
      {
        it = decoys.erase(it);
        continue;
      }
      used_gindices.insert(it->global_amount_index);
      if (++good_outs_count == required_decoys_count + 1)
      {
        decoys.erase(++it, decoys.end());
        break;
      }
      ++it;
    }
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(decoys.size() == required_decoys_count + 1, "for PoS stake got less good decoys than required: " << decoys.size() << " < " << required_decoys_count);

    decoys.sort([](auto& l, auto& r){ return l.global_amount_index < r.global_amount_index; }); // sort them now (note absolute_sorted_output_offsets_to_relative_in_place() below)

    uint64_t i = 0;
    for(auto& el : decoys)
    {
      uint64_t gindex = el.global_amount_index;
      if (gindex == td.m_global_output_index)
        secret_index = i;
      ++i;
      ring.emplace_back(el.stealth_address, el.amount_commitment, el.blinded_asset_id, el.concealing_point);
      stake_input.key_offsets.push_back(el.global_amount_index);
    }
    r = absolute_sorted_output_offsets_to_relative_in_place(stake_input.key_offsets);
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "absolute_sorted_output_offsets_to_relative_in_place failed");
  }
  else
  {
    // no decoys, the ring consist of one element -- the real stake output
    ring.emplace_back(stake_out.stealth_address, stake_out.amount_commitment, stake_out.blinded_asset_id, stake_out.concealing_point);
    stake_input.key_offsets.push_back(td.m_global_output_index);
  }
  stake_input.k_image = pe.keyimage;

  crypto::point_t stake_out_blinded_asset_id_pt = currency::native_coin_asset_id_pt + td.m_zc_info_ptr->asset_id_blinding_mask * crypto::c_point_X; 
#ifndef NDEBUG
  {
    crypto::point_t source_amount_commitment = crypto::c_scalar_1div8 * td.m_amount * stake_out_blinded_asset_id_pt + crypto::c_scalar_1div8 * td.m_zc_info_ptr->amount_blinding_mask * crypto::c_point_G;
    WLT_CHECK_AND_ASSERT_MES(stake_out.amount_commitment == source_amount_commitment.to_public_key(), false, "real output amount commitment check failed");
    WLT_CHECK_AND_ASSERT_MES(ring[secret_index].amount_commitment == stake_out.amount_commitment, false, "ring secret member doesn't match with the stake output");
    WLT_CHECK_AND_ASSERT_MES(cxt.stake_amount == td.m_amount, false, "stake_amount missmatch");
  }
  #endif

  crypto::hash hash_for_zarcanum_sig = get_block_hash(b);

  WLT_CHECK_AND_ASSERT_MES(miner_tx_tgc.pseudo_out_amount_blinding_masks_sum.is_zero(), false, "pseudo_out_amount_blinding_masks_sum is nonzero"); // it should be zero because there's only one input (stake), and thus one pseudo out
  crypto::scalar_t pseudo_out_amount_blinding_mask = miner_tx_tgc.amount_blinding_masks_sum; // sum of outputs' amount blinding masks

  miner_tx_tgc.pseudo_outs_blinded_asset_ids.emplace_back(currency::native_coin_asset_id_pt); // for Zarcanum stake inputs pseudo outputs commitments has explicit native asset id
  miner_tx_tgc.pseudo_outs_plus_real_out_blinding_masks.emplace_back(0);
  miner_tx_tgc.real_zc_ins_asset_ids.emplace_back(td.m_zc_info_ptr->asset_id);
  // TODO @#@# [architecture] the same value is calculated in zarcanum_generate_proof(), consider an impovement 
  miner_tx_tgc.pseudo_out_amount_commitments_sum += cxt.stake_amount * stake_out_blinded_asset_id_pt + pseudo_out_amount_blinding_mask * crypto::c_point_G;
  miner_tx_tgc.real_in_asset_id_blinding_mask_x_amount_sum += td.m_zc_info_ptr->asset_id_blinding_mask * cxt.stake_amount;

  uint8_t err = 0;
  r = crypto::zarcanum_generate_proof(hash_for_zarcanum_sig, cxt.kernel_hash, ring, cxt.last_pow_block_id_hashed, cxt.sk.kimage,
    secret_x, cxt.secret_q, secret_index, cxt.stake_amount, td.m_zc_info_ptr->asset_id_blinding_mask, cxt.stake_out_amount_blinding_mask, pseudo_out_amount_blinding_mask, 
    static_cast<crypto::zarcanum_proof&>(sig), &err);
  WLT_CHECK_AND_ASSERT_MES(r, false, "zarcanum_generate_proof failed, err: " << (int)err);

  //
  // The miner tx prefix should be sealed by now, and the tx hash should be defined.
  // Any changes made below should only affect the signatures/proofs and should not impact the prefix hash calculation.   
  //
  crypto::hash miner_tx_id = get_transaction_hash(b.miner_tx);

  // proofs for miner_tx
  
  // asset surjection proof
  currency::zc_asset_surjection_proof asp{};
  r = generate_asset_surjection_proof(miner_tx_id, false, miner_tx_tgc, asp);  // has_non_zc_inputs == false because after the HF4 PoS mining is only allowed for ZC stakes inputs 
  WLT_CHECK_AND_ASSERT_MES(r, false, "generete_asset_surjection_proof failed");
  b.miner_tx.proofs.emplace_back(std::move(asp));

  // range proofs
  currency::zc_outs_range_proof range_proofs{};
  r = generate_zc_outs_range_proof(miner_tx_id, 0, miner_tx_tgc, b.miner_tx.vout, range_proofs);
  WLT_CHECK_AND_ASSERT_MES(r, false, "Failed to generate zc_outs_range_proof()");
  b.miner_tx.proofs.emplace_back(std::move(range_proofs));

  // balance proof
  currency::zc_balance_proof balance_proof{};
  r = generate_tx_balance_proof(b.miner_tx, miner_tx_id, miner_tx_tgc, full_block_reward, balance_proof);
  WLT_CHECK_AND_ASSERT_MES(r, false, "generate_tx_balance_proof failed");
  b.miner_tx.proofs.emplace_back(std::move(balance_proof));

  // the following line are for debugging when necessary -- sowle
  //err = 0;
  //r = crypto::zarcanum_verify_proof(hash_for_zarcanum_sig, cxt.kernel_hash, ring, cxt.last_pow_block_id_hashed, cxt.sk.kimage, cxt.basic_diff, sig, &err);
  //WLT_CHECK_AND_ASSERT_MES(r, false, "zarcanum_verify_proof failed with code " << (int)err);

  return true;
}
//------------------------------------------------------------------
bool wallet2::fill_mining_context(mining_context& ctx)
{
  currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request pos_details_req = AUTO_VAL_INIT(pos_details_req);
  currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response pos_details_resp = AUTO_VAL_INIT(pos_details_resp);
  m_core_proxy->call_COMMAND_RPC_GET_POS_MINING_DETAILS(pos_details_req, pos_details_resp);
  if (pos_details_resp.status != API_RETURN_CODE_OK)
    return false;

  ctx = mining_context{};
  ctx.init(wide_difficulty_type(pos_details_resp.pos_basic_difficulty), pos_details_resp.sm, is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM));

  ctx.last_block_hash             = pos_details_resp.last_block_hash;
  ctx.is_pos_allowed              = pos_details_resp.pos_mining_allowed;
  ctx.is_pos_sequence_factor_good = pos_details_resp.pos_sequence_factor_is_good;
  ctx.starter_timestamp           = pos_details_resp.starter_timestamp;
  ctx.status = API_RETURN_CODE_NOT_FOUND;
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
  TIME_MEASURE_START_MS(mining_duration_ms);
  mining_context ctx = AUTO_VAL_INIT(ctx);
  WLT_LOG_L2("Starting PoS mining iteration");
  fill_mining_context(ctx);
  
  if (!ctx.is_pos_allowed)
  {
    WLT_LOG_YELLOW("POS MINING NOT ALLOWED YET", LOG_LEVEL_0);
    return true;
  }

  if (!ctx.is_pos_sequence_factor_good)
  {
    WLT_LOG_YELLOW("PoS sequence factor is too high, waiting for a PoW block...", LOG_LEVEL_0);
    return true;
  }

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
  
  bool res = true;
  if (ctx.status == API_RETURN_CODE_OK)
  {
    res = build_minted_block(ctx, miner_address);
  }
  TIME_MEASURE_FINISH_MS(mining_duration_ms);

  WLT_LOG_L0("PoS mining: " << ctx.iterations_processed << " iterations finished (" << std::fixed << std::setprecision(2) << (mining_duration_ms / 1000.0f) << "s), status: " << ctx.status << ", " << ctx.total_items_checked << " entries with total amount: " << print_money_brief(ctx.total_amount_checked));

  return res;
}
//------------------------------------------------------------------
void wallet2::do_pos_mining_prepare_entry(mining_context& context, size_t transfer_index)
{
  CHECK_AND_ASSERT_MES_NO_RET(transfer_index < m_transfers.size(), "transfer_index is out of bounds: " << transfer_index); 
  const transfer_details& td = m_transfers[transfer_index];

  crypto::scalar_t amount_blinding_mask{};
  if (td.is_zc())
    amount_blinding_mask = td.m_zc_info_ptr->amount_blinding_mask;

  context.prepare_entry(td.amount(), td.m_key_image, get_tx_pub_key_from_extra(td.m_ptx_wallet_info->m_tx), td.m_internal_output_index,
    amount_blinding_mask, m_account.get_keys().view_secret_key);
}
//------------------------------------------------------------------
bool wallet2::do_pos_mining_iteration(mining_context& context, size_t transfer_index, uint64_t ts)
{
  return context.do_iteration(ts);
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
bool wallet2::build_minted_block(const mining_context& cxt)
{
  return build_minted_block(cxt, m_account.get_public_address());
}
//------------------------------------------------------------------
std::string wallet2::get_extra_text_for_block(uint64_t new_block_expected_height)
{
  size_t entries_voted = 0;
  std::string extra_text = "{";
  for (const auto& e : m_votes_config.entries)
  {
    if (e.h_start <= new_block_expected_height && e.h_end >= new_block_expected_height)
    {
      //do vote for/against this
      if (entries_voted != 0)
        extra_text += ",";
      extra_text += "\"";
      extra_text += e.proposal_id;
      extra_text += "\":";
      extra_text += e.vote ? "1" : "0";
      entries_voted++;
    }
  }
  extra_text += "}";
  if (!entries_voted)
    extra_text = "";
  return extra_text;
}
//------------------------------------------------------------------
bool wallet2::build_minted_block(const mining_context& cxt, const currency::account_public_address& miner_address)
{
  //found a block, construct it, sign and push to daemon
  WLT_LOG_GREEN("Found kernel, constructing block", LOG_LEVEL_0);

  WLT_CHECK_AND_ASSERT_MES(cxt.index < m_transfers.size(), false, "cxt.index = " << cxt.index << " is out of bounds");
  const transfer_details& td = m_transfers[cxt.index];

  currency::COMMAND_RPC_GETBLOCKTEMPLATE::request tmpl_req = AUTO_VAL_INIT(tmpl_req);
  currency::COMMAND_RPC_GETBLOCKTEMPLATE::response tmpl_rsp = AUTO_VAL_INIT(tmpl_rsp);
  tmpl_req.wallet_address = get_account_address_as_str(miner_address);
  tmpl_req.stakeholder_address = get_account_address_as_str(m_account.get_public_address());
  tmpl_req.pos_block = true;
  tmpl_req.extra_text = get_extra_text_for_block(m_chain.get_top_block_height());

  tmpl_req.pe = AUTO_VAL_INIT(tmpl_req.pe);
  tmpl_req.pe.amount              = td.amount();
  tmpl_req.pe.block_timestamp     = td.m_ptx_wallet_info->m_block_timestamp;
  tmpl_req.pe.g_index             = td.m_global_output_index;
  tmpl_req.pe.keyimage            = td.m_key_image;
  tmpl_req.pe.stake_unlock_time   = cxt.stake_unlock_time;
  tmpl_req.pe.tx_id               = td.tx_hash();
  tmpl_req.pe.tx_out_index        = td.m_internal_output_index;
  tmpl_req.pe.wallet_index        = cxt.index;

  // mark stake source as spent and make sure it will be restored in case of error
  const std::vector<uint64_t> stake_transfer_idx_vec{ cxt.index };
  mark_transfers_as_spent(stake_transfer_idx_vec, "stake source");
  bool gracefull_leaving = false;
  auto stake_transfer_spent_flag_restorer = epee::misc_utils::create_scope_leave_handler([&](){
    if (!gracefull_leaving)
      clear_transfers_from_flag(stake_transfer_idx_vec, WALLET_TRANSFER_DETAIL_FLAG_SPENT, "stake source");
  });

  // generate UTXO Defragmentation Transaction - to reduce the UTXO set size
  transaction udtx{};
  if (generate_utxo_defragmentation_transaction_if_needed(udtx))
  {
    tx_to_blob(udtx, tmpl_req.explicit_transaction);
    WLT_LOG_GREEN("Note: " << udtx.vin.size() << " inputs were aggregated into UTXO defragmentation tx " << get_transaction_hash(udtx), LOG_LEVEL_0);
  }
  m_core_proxy->call_COMMAND_RPC_GETBLOCKTEMPLATE(tmpl_req, tmpl_rsp);
  WLT_CHECK_AND_ASSERT_MES(tmpl_rsp.status == API_RETURN_CODE_OK, false, "Failed to create block template after kernel hash found!");

  currency::block b = AUTO_VAL_INIT(b);
  currency::blobdata block_blob;
  bool res = epee::string_tools::parse_hexstr_to_binbuff(tmpl_rsp.blocktemplate_blob, block_blob);
  WLT_CHECK_AND_ASSERT_MES(res, false, "parse_hexstr_to_binbuff() failed after kernel hash found!");
  res = parse_and_validate_block_from_blob(block_blob, b);
  WLT_CHECK_AND_ASSERT_MES(res, false, "parse_and_validate_block_from_blob() failed after kernel hash found!");

  if (cxt.last_block_hash != b.prev_id)
  {
    WLT_LOG_YELLOW("Kernel was found but block is behindhand, b.prev_id=" << b.prev_id << ", last_block_hash=" << cxt.last_block_hash, LOG_LEVEL_0);
    return false;
  }

  // set the timestamp from stake kernel
  b.timestamp = cxt.sk.block_timestamp;
  uint64_t current_timestamp = m_core_runtime_config.get_core_time();
  set_block_datetime(current_timestamp, b);
  WLT_LOG_MAGENTA("Applying actual timestamp: " << current_timestamp, LOG_LEVEL_2);

  res = prepare_and_sign_pos_block(cxt, tmpl_rsp.block_reward, tmpl_req.pe, tmpl_rsp.miner_tx_tgc, b);
  WLT_CHECK_AND_ASSERT_MES(res, false, "Failed to prepare_and_sign_pos_block");

  crypto::hash block_hash = get_block_hash(b);
  WLT_LOG_GREEN("Block " << print16(block_hash) << " @ " << get_block_height(b) << " has been constructed, sending to core...", LOG_LEVEL_0);

  currency::COMMAND_RPC_SUBMITBLOCK2::request subm_req = AUTO_VAL_INIT(subm_req);
  currency::COMMAND_RPC_SUBMITBLOCK2::response subm_rsp = AUTO_VAL_INIT(subm_rsp);
  subm_req.b = t_serializable_object_to_blob(b);
  if (tmpl_req.explicit_transaction.size())
    subm_req.explicit_txs.push_back(hexemizer{ tmpl_req.explicit_transaction });
    
  m_core_proxy->call_COMMAND_RPC_SUBMITBLOCK2(subm_req, subm_rsp);
  if (subm_rsp.status != API_RETURN_CODE_OK)
  {
    WLT_LOG_ERROR("Constructed block " << print16(block_hash) << " was rejected by the core, status: " << subm_rsp.status);
    return false;
  }    
  WLT_LOG_GREEN("PoS block " << print16(block_hash) << " generated and accepted, congrats!", LOG_LEVEL_0);
  m_wcallback->on_pos_block_found(b);

  gracefull_leaving = true; // to prevent source transfer flags be cleared in scope leave handler
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_unconfirmed_transfers(std::vector<wallet_public::wallet_transfer_info>& trs, bool exclude_mining_txs)
{
  for (auto& u : m_unconfirmed_txs)
  {
    if (exclude_mining_txs && currency::is_coinbase(u.second.tx))
    {
      continue;
    }
    trs.push_back(u.second);
    load_wallet_transfer_info_flags(trs.back());
  }
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
  if (for_pos_mining && m_core_runtime_config.is_hardfork_active_for_height(1, get_blockchain_current_size()))
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
  transfer(destinations, 0, 0, od.fee, extra, attachments, get_current_split_strategy(), tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx);
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

  transfer(std::vector<currency::tx_destination_entry>(), 0, 0, fee, extra, attachments, get_current_split_strategy(), tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx);
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
  transfer(destinations, 0, 0, od.fee, extra, attachments, get_current_split_strategy(), tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::push_alias_info_to_extra_according_to_hf_status(const currency::extra_alias_entry& ai, std::vector<currency::extra_v>& extra)
{
  if (m_core_runtime_config.is_hardfork_active_for_height(2, get_top_block_height()))
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
uint64_t wallet2::get_alias_cost(const std::string& alias)
{
  currency::COMMAND_RPC_GET_ALIAS_REWARD::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_ALIAS_REWARD::response rsp = AUTO_VAL_INIT(rsp);
  req.alias = alias;
  if (!m_core_proxy->call_COMMAND_RPC_GET_ALIAS_REWARD(req, rsp))
  {
    throw std::runtime_error(std::string("Failed to get alias cost"));
  }
 
  return rsp.reward;
}
//----------------------------------------------------------------------------------------------------
void wallet2::request_alias_registration(currency::extra_alias_entry& ai, currency::transaction& res_tx, uint64_t fee, uint64_t reward, const crypto::secret_key& authority_key)
{
  if (!validate_alias_name(ai.m_alias))
  {
    throw std::runtime_error(std::string("wrong alias characters: ") + ai.m_alias);
  }

  if (ai.m_alias.size() < ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED)
  {
    if (authority_key == currency::null_skey)
    {
      throw std::runtime_error(std::string("Short aliases is not allowed without authority key: ") + ALIAS_SHORT_NAMES_VALIDATION_PUB_KEY);
    }
    crypto::public_key authority_pub = AUTO_VAL_INIT(authority_pub);
    bool r = crypto::secret_key_to_public_key(authority_key, authority_pub);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to generate pub key from secrete authority key");

    if (string_tools::pod_to_hex(authority_pub) != ALIAS_SHORT_NAMES_VALIDATION_PUB_KEY)
    {
      throw std::runtime_error(std::string("Short aliases is not allowed to register by this authority key"));
    }
    r = currency::sign_extra_alias_entry(ai, authority_pub, authority_key);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to sign alias update");
    WLT_LOG_L2("Generated update alias info: " << ENDL
      << "alias: " << ai.m_alias << ENDL
      << "signature: " << currency::print_t_array(ai.m_sign) << ENDL
      << "signed(owner) pub key: " << m_account.get_keys().account_address.spend_public_key << ENDL
      << "to address: " << get_account_address_as_str(ai.m_address) << ENDL
      << "sign_buff_hash: " << currency::get_sign_buff_hash_for_alias_update(ai)
    );
  }

  if (!reward)
  {
    reward = get_alias_cost(ai.m_alias);
  }

  std::vector<currency::tx_destination_entry> destinations;
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  push_alias_info_to_extra_according_to_hf_status(ai, extra);

  currency::tx_destination_entry tx_dest_alias_reward;
  tx_dest_alias_reward.addr.resize(1);
  get_aliases_reward_account(tx_dest_alias_reward.addr.back());
  tx_dest_alias_reward.amount = reward;
  tx_dest_alias_reward.flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id | tx_destination_entry_flags::tdef_zero_amount_blinding_mask;
  destinations.push_back(tx_dest_alias_reward);

  transfer(destinations, 0, 0, fee, extra, attachments, get_current_split_strategy(), tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx, CURRENCY_TO_KEY_OUT_RELAXED, false);
}
//----------------------------------------------------------------------------------------------------
void wallet2::deploy_new_asset(const currency::asset_descriptor_base& asset_info, const std::vector<currency::tx_destination_entry>& destinations, currency::transaction& result_tx, crypto::public_key& new_asset_id)
{
  asset_descriptor_operation asset_reg_info = AUTO_VAL_INIT(asset_reg_info);
  asset_reg_info.descriptor = asset_info;
  asset_reg_info.operation_type = ASSET_DESCRIPTOR_OPERATION_REGISTER;
  construct_tx_param ctp = get_default_construct_tx_param();
  ctp.dsts = destinations;
  ctp.extra.push_back(asset_reg_info);
  ctp.need_at_least_1_zc = true;

  finalized_tx ft = AUTO_VAL_INIT(ft);
  this->transfer(ctp, ft, true, nullptr);
  result_tx = ft.tx;
  //get generated asset id
  currency::asset_descriptor_operation ado = AUTO_VAL_INIT(ado);
  bool r = get_type_in_variant_container(result_tx.extra, ado);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed find asset info in tx");
  CHECK_AND_ASSERT_THROW_MES(get_or_calculate_asset_id(ado, nullptr, &new_asset_id), "get_or_calculate_asset_id failed");

  m_custom_assets[new_asset_id] = ado.descriptor;
}
//----------------------------------------------------------------------------------------------------
void wallet2::emit_asset(const crypto::public_key asset_id, std::vector<currency::tx_destination_entry>& destinations, currency::transaction& result_tx)
{

  auto own_asset_entry_it = m_own_asset_descriptors.find(asset_id);
  CHECK_AND_ASSERT_THROW_MES(own_asset_entry_it != m_own_asset_descriptors.end(), "Failed find asset_id " << asset_id << " in own assets list");
  COMMAND_RPC_GET_ASSET_INFO::request req;
  req.asset_id = asset_id;
  COMMAND_RPC_GET_ASSET_INFO::response rsp;
  bool r = m_core_proxy->call_COMMAND_RPC_GET_ASSET_INFO(req, rsp);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to call_COMMAND_RPC_GET_ASSET_INFO");
 
  asset_descriptor_operation asset_emmit_info = AUTO_VAL_INIT(asset_emmit_info);
  asset_emmit_info.descriptor = rsp.asset_descriptor;
  asset_emmit_info.operation_type = ASSET_DESCRIPTOR_OPERATION_EMIT;
  asset_emmit_info.opt_asset_id = asset_id;
  construct_tx_param ctp = get_default_construct_tx_param();
  ctp.dsts = destinations;
  ctp.extra.push_back(asset_emmit_info);
  ctp.need_at_least_1_zc = true;
  ctp.ado_current_asset_owner = rsp.asset_descriptor.owner;
  //ctp.asset_deploy_control_key = own_asset_entry_it->second.control_key;

  for(auto& dst : ctp.dsts)
  {
    if (dst.asset_id == asset_id)
      dst.asset_id = null_pkey; // emit operation requires null_pkey for emitting asset outputs, fix it ad-hoc here
  }

  finalized_tx ft = AUTO_VAL_INIT(ft);
  this->transfer(ctp, ft, true, nullptr);
  result_tx = ft.tx;
}
//----------------------------------------------------------------------------------------------------
void wallet2::update_asset(const crypto::public_key asset_id, const currency::asset_descriptor_base new_descriptor, currency::transaction& result_tx)
{
  auto own_asset_entry_it = m_own_asset_descriptors.find(asset_id);
  CHECK_AND_ASSERT_THROW_MES(own_asset_entry_it != m_own_asset_descriptors.end(), "Failed find asset_id " << asset_id << " in own assets list");

  asset_descriptor_operation asset_update_info = AUTO_VAL_INIT(asset_update_info);
  asset_update_info.descriptor = new_descriptor;
  asset_update_info.operation_type = ASSET_DESCRIPTOR_OPERATION_UPDATE;
  asset_update_info.opt_asset_id = asset_id;
  construct_tx_param ctp = get_default_construct_tx_param();
  ctp.extra.push_back(asset_update_info);
  ctp.need_at_least_1_zc = true;
  currency::asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  bool r = this->daemon_get_asset_info(asset_id, adb);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to get asset info from daemon");
  ctp.ado_current_asset_owner = adb.owner;

  finalized_tx ft = AUTO_VAL_INIT(ft);
  this->transfer(ctp, ft, true, nullptr);
  result_tx = ft.tx;
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer_asset_ownership(const crypto::public_key asset_id, const crypto::public_key& new_owner, currency::transaction& result_tx)
{
  auto own_asset_entry_it = m_own_asset_descriptors.find(asset_id);
  CHECK_AND_ASSERT_THROW_MES(own_asset_entry_it != m_own_asset_descriptors.end(), "Failed find asset_id " << asset_id << " in own assets list");

  currency::asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  bool r = this->daemon_get_asset_info(asset_id, adb);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to get asset info from daemon");

  asset_descriptor_operation asset_update_info = AUTO_VAL_INIT(asset_update_info);
  asset_update_info.descriptor = adb;
  asset_update_info.operation_type = ASSET_DESCRIPTOR_OPERATION_UPDATE;
  asset_update_info.opt_asset_id = asset_id;
  asset_update_info.descriptor.owner = new_owner;
  construct_tx_param ctp = get_default_construct_tx_param();
  ctp.ado_current_asset_owner = adb.owner;
  ctp.extra.push_back(asset_update_info);

  finalized_tx ft = AUTO_VAL_INIT(ft);
  this->transfer(ctp, ft, true, nullptr);
  result_tx = ft.tx;
}
//----------------------------------------------------------------------------------------------------
void wallet2::burn_asset(const crypto::public_key asset_id, uint64_t amount_to_burn, currency::transaction& result_tx)
{
  //auto own_asset_entry_it = m_own_asset_descriptors.find(asset_id);
  //CHECK_AND_ASSERT_THROW_MES(own_asset_entry_it != m_own_asset_descriptors.end(), "Failed find asset_id " << asset_id << " in own assets list");
  COMMAND_RPC_GET_ASSET_INFO::request req;
  req.asset_id = asset_id;
  COMMAND_RPC_GET_ASSET_INFO::response rsp;
  bool r = m_core_proxy->call_COMMAND_RPC_GET_ASSET_INFO(req, rsp);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to call_COMMAND_RPC_GET_ASSET_INFO");



  asset_descriptor_operation asset_burn_info = AUTO_VAL_INIT(asset_burn_info);
  asset_burn_info.descriptor = rsp.asset_descriptor;

  CHECK_AND_ASSERT_THROW_MES(asset_burn_info.descriptor.current_supply > amount_to_burn, "Wrong amount to burn (current_supply" << asset_burn_info.descriptor.current_supply << " is less then " << amount_to_burn << ")");

  currency::tx_destination_entry dst_to_burn = AUTO_VAL_INIT(dst_to_burn);
  dst_to_burn.amount = amount_to_burn;
  dst_to_burn.asset_id = asset_id;

  asset_burn_info.operation_type = ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN;
  asset_burn_info.opt_asset_id = asset_id;
  construct_tx_param ctp = get_default_construct_tx_param();
  ctp.extra.push_back(asset_burn_info);
  ctp.need_at_least_1_zc = true;
  ctp.ado_current_asset_owner = rsp.asset_descriptor.owner;
  ctp.dsts.push_back(dst_to_burn);

  finalized_tx ft = AUTO_VAL_INIT(ft);
  this->transfer(ctp, ft, true, nullptr);
  result_tx = ft.tx;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::daemon_get_asset_info(const crypto::public_key& asset_id, currency::asset_descriptor_base& adb)
{
  COMMAND_RPC_GET_ASSET_INFO::request req;
  req.asset_id = asset_id;
  COMMAND_RPC_GET_ASSET_INFO::response rsp;
  bool r = m_core_proxy->call_COMMAND_RPC_GET_ASSET_INFO(req, rsp);
  CHECK_AND_ASSERT_MES(r, false, "Failed to call_COMMAND_RPC_GET_ASSET_INFO");
  adb = rsp.asset_descriptor;
  return true;
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
  WLT_LOG_L2("Generated update alias info: " << ENDL
    << "alias: " << ai.m_alias << ENDL
    << "signature: " << currency::print_t_array(ai.m_sign) << ENDL
    << "signed(owner) pub key: " << m_account.get_keys().account_address.spend_public_key << ENDL
    << "transfered to address: " << get_account_address_as_str(ai.m_address) << ENDL
    << "sign_buff_hash: " << currency::get_sign_buff_hash_for_alias_update(ai)
    );

  std::vector<currency::tx_destination_entry> destinations;
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  push_alias_info_to_extra_according_to_hf_status(ai, extra);

  transfer(destinations, 0, 0, fee, extra, attachments, get_current_split_strategy(), tx_dust_policy(DEFAULT_DUST_THRESHOLD), res_tx, CURRENCY_TO_KEY_OUT_RELAXED, false);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::check_available_sources(std::list<uint64_t>& amounts)
{  
  /*
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
  */
  return false;
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
  local_transfers_struct(transfer_container& tf) :l_transfers_ref(tf)
  {}

  transfer_container& l_transfers_ref;
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
    ss << "index                 amount  spent_h  g_index   block  block_ts     flg  tx                                                                out#  key image" << ENDL;
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
  construct_params.split_strategy_id = get_current_split_strategy();
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
    currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
    ftp.tx_version = this->get_current_tx_version();
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
    currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
    ftp.tx_version = this->get_current_tx_version();
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
  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.tx_version = this->get_current_tx_version();
  construct_params.fee = it->second.amount() - (ecrow_details.amount_a_pledge + ecrow_details.amount_to_pay + ecrow_details.amount_b_pledge);
  construct_params.multisig_id = multisig_id;
  construct_params.split_strategy_id = get_current_split_strategy();
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
  ctp.split_strategy_id = get_current_split_strategy();
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

  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.tx_version = this->get_current_tx_version();
  prepare_transaction(ctp, ftp);

  selected_transfers = ftp.selected_transfers;

  finalize_transaction(ftp, tx, one_time_key, false);
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_transfers_to_expiration_list(const std::vector<uint64_t>& selected_transfers, const std::vector<payment_details_subtransfer>& received, uint64_t expiration, const crypto::hash& related_tx_id)
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
  m_money_expirations.back().related_tx_id = related_tx_id;
  m_money_expirations.back().receved = received;

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
    ss.str() << ", expire(s) at: " << expiration, LOG_LEVEL_0);
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
  if (flags_before != m_transfers[transfer_index].m_flags)
  {
    WLT_LOG_BLUE("Transfer [" << transfer_index << "] was cleared from escrow proposal reservation, flags: " << flags_before << " -> " << m_transfers[transfer_index].m_flags << ", reason: intentional removing from expiration list", LOG_LEVEL_0);
  }
  
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
  ctp.split_strategy_id = get_current_split_strategy();
  ctp.tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED;
  ctp.unlock_time = unlock_time;

  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.tx_version = this->get_current_tx_version();
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
  add_sent_tx_detailed_info(tx, ftp.attachments, ftp.prepared_destinations, ftp.selected_transfers);

  print_tx_sent_message(tx, "(from multisig)", fee);
}
//----------------------------------------------------------------------------------------------------
void wallet2::create_htlc_proposal(uint64_t amount, const currency::account_public_address& addr, uint64_t lock_blocks_count, currency::transaction &tx, const crypto::hash& htlc_hash,  std::string &origin)
{
  construct_tx_param ctp = get_default_construct_tx_param();
  ctp.fee = TX_DEFAULT_FEE;
  ctp.dsts.resize(1);
  ctp.dsts.back().addr.push_back(addr);
  ctp.dsts.back().amount = amount;
  destination_option_htlc_out& htlc_option = ctp.dsts.back().htlc_options;
  htlc_option.expiration = lock_blocks_count; //about 12 hours
  htlc_option.htlc_hash = htlc_hash;

  currency::create_and_add_tx_payer_to_container_from_address(ctp.extra, 
    get_account().get_keys().account_address, get_top_block_height(), get_core_runtime_config());

  finalized_tx ft = AUTO_VAL_INIT(ft);
  this->transfer(ctp, ft, true, nullptr);
  origin = ft.htlc_origin;
  tx = ft.tx;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_list_of_active_htlc(std::list<wallet_public::htlc_entry_info>& htlcs, bool only_redeem_txs)
{
  for (auto htlc_entry : m_active_htlcs_txid)
  {
    const transfer_details& td = m_transfers[htlc_entry.second];
    if (only_redeem_txs && !(td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_HTLC_REDEEM))
    {
      continue;
    }
    wallet_public::htlc_entry_info entry = AUTO_VAL_INIT(entry);
    entry.tx_id = htlc_entry.first;
    if (td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].type() != typeid(tx_out_bare))
    {
      //@#@
      LOG_ERROR("Unexpected output type in get_list_of_active_htlc:" << td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].type().name());
      continue;
    }
    const tx_out_bare out_b = boost::get<tx_out_bare>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]);
    entry.amount = out_b.amount;
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(out_b.target.type() == typeid(txout_htlc),
      "[get_list_of_active_htlc]Internal error: unexpected type of out");
    const txout_htlc& htlc = boost::get<txout_htlc>(out_b.target);
    entry.sha256_hash = htlc.htlc_hash;
    
    currency::tx_payer payer = AUTO_VAL_INIT(payer);
    if (currency::get_type_in_variant_container(td.varian_options, payer))
      entry.counterparty_address = payer.acc_addr;

    entry.is_redeem = td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_HTLC_REDEEM ? true : false;
    htlcs.push_back(entry);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::redeem_htlc(const crypto::hash& htlc_tx_id, const std::string& origin)
{
  currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
  return redeem_htlc(htlc_tx_id, origin, result_tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::redeem_htlc(const crypto::hash& htlc_tx_id, const std::string& origin, currency::transaction& result_tx)
{

  construct_tx_param ctp = get_default_construct_tx_param();
  ctp.fee = TX_DEFAULT_FEE;
  ctp.htlc_tx_id = htlc_tx_id;
  ctp.htlc_origin = origin;
  ctp.dsts.resize(1);
  ctp.dsts.back().addr.push_back(m_account.get_keys().account_address);

  auto it = m_active_htlcs_txid.find(htlc_tx_id);
  WLT_THROW_IF_FALSE_WITH_CODE(it != m_active_htlcs_txid.end(),
    "htlc not found with tx_id = " << htlc_tx_id, API_RETURN_CODE_NOT_FOUND);

  ctp.dsts.back().amount = m_transfers[it->second].amount() - ctp.fee;
  this->transfer(ctp, result_tx, true, nullptr);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::check_htlc_redeemed(const crypto::hash& htlc_tx_id, std::string& origin, crypto::hash& redeem_tx_id)
{
  auto it = m_active_htlcs_txid.find(htlc_tx_id);

  WLT_THROW_IF_FALSE_WITH_CODE(it != m_active_htlcs_txid.end(),
    "htlc not found with tx_id = " << htlc_tx_id, API_RETURN_CODE_NOT_FOUND);

  transfer_details_extra_option_htlc_info htlc_options = AUTO_VAL_INIT(htlc_options);
  if (!currency::get_type_in_variant_container(m_transfers[it->second].varian_options, htlc_options))
  {
    return false;
  }
  if (htlc_options.origin.size())
  {
    origin = htlc_options.origin;
    redeem_tx_id = htlc_options.redeem_tx_id;
    return true;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::create_ionic_swap_proposal(const wallet_public::ionic_swap_proposal_info& proposal_details, const currency::account_public_address& destination_addr, wallet_public::ionic_swap_proposal& proposal)
{
  std::vector<uint64_t> selected_transfers_for_template;
  
  return build_ionic_swap_template(proposal_details, destination_addr, proposal, selected_transfers_for_template);

  //const uint32_t mask_to_mark_escrow_template_locked_transfers = WALLET_TRANSFER_DETAIL_FLAG_BLOCKED | WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION;
  //mark_transfers_with_flag(selected_transfers_for_template, mask_to_mark_escrow_template_locked_transfers, "preparing ionic_swap");
  //return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::build_ionic_swap_template(const wallet_public::ionic_swap_proposal_info& proposal_detais, const currency::account_public_address& destination_addr,
  wallet_public::ionic_swap_proposal& proposal,
  std::vector<uint64_t>& selected_transfers)
{
  WLT_THROW_IF_FALSE_WITH_CODE(proposal_detais.fee_paid_by_a >= get_current_minimum_network_fee(), "Error at build_ionic_swap_template, ", API_RETURN_CODE_WALLET_FEE_TOO_LOW);

  construct_tx_param ctp = get_default_construct_tx_param();
  
  //ctp.fake_outputs_count = proposal_detais.mixins;
  ctp.fee = proposal_detais.fee_paid_by_a;
  ctp.flags = TX_FLAG_SIGNATURE_MODE_SEPARATE;
  ctp.mark_tx_as_complete = false;
  ctp.crypt_address = destination_addr;
  
  ctp.dsts.resize(proposal_detais.to_finalizer.size() + proposal_detais.to_initiator.size());
  size_t i = 0;
  // Here is an proposed for exchange funds
  for (; i != proposal_detais.to_finalizer.size(); i++)
  {
    ctp.dsts[i].amount = proposal_detais.to_finalizer[i].amount;
    ctp.dsts[i].amount_to_provide = proposal_detais.to_finalizer[i].amount;
    ctp.dsts[i].flags |= tx_destination_entry_flags::tdef_explicit_amount_to_provide;
    ctp.dsts[i].addr.push_back(destination_addr);
    ctp.dsts[i].asset_id = proposal_detais.to_finalizer[i].asset_id;
  }
  // Here is an expected in return funds
  std::vector<payment_details_subtransfer> for_expiration_list;
  for (size_t j = 0; j != proposal_detais.to_initiator.size(); j++, i++)
  {
    ctp.dsts[i].amount = proposal_detais.to_initiator[j].amount;
    ctp.dsts[i].amount_to_provide = 0;
    ctp.dsts[i].flags |= tx_destination_entry_flags::tdef_explicit_amount_to_provide;
    ctp.dsts[i].addr.push_back(m_account.get_public_address());
    ctp.dsts[i].asset_id = proposal_detais.to_initiator[j].asset_id;
    for_expiration_list.push_back(payment_details_subtransfer{ ctp.dsts[i].asset_id, ctp.dsts[i].amount});
  }

  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.mode_separate_fee = ctp.fee;
  ftp.tx_version = this->get_current_tx_version();
  prepare_transaction(ctp, ftp);

  selected_transfers = ftp.selected_transfers;
  currency::finalized_tx finalize_result = AUTO_VAL_INIT(finalize_result);
  finalize_transaction(ftp, finalize_result, false);
  for(uint64_t i: selected_transfers)
    m_transfers[i].m_flags &= ~WALLET_TRANSFER_DETAIL_FLAG_BLOCKED;

  //add_transfers_to_expiration_list(selected_transfers, for_expiration_list, this->get_core_runtime_config().get_core_time() + proposal_detais.expiration_time, currency::null_hash);

  //wrap it all 
  proposal.tx_template = finalize_result.tx;
  wallet_public::ionic_swap_proposal_context ispc = AUTO_VAL_INIT(ispc);
  ispc.gen_context = finalize_result.ftp.gen_context;
  //ispc.one_time_skey = finalize_result.one_time_key;
  std::string proposal_context_blob = t_serializable_object_to_blob(ispc);
  proposal.encrypted_context = crypto::chacha_crypt(static_cast<const std::string&>(proposal_context_blob), finalize_result.derivation); 
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_ionic_swap_proposal_info(const std::string&raw_proposal, wallet_public::ionic_swap_proposal_info& proposal_info) const
{
  wallet_public::ionic_swap_proposal proposal = AUTO_VAL_INIT(proposal);
  bool r = t_unserializable_object_from_blob(proposal, raw_proposal);
  THROW_IF_TRUE_WALLET_EX(!r, error::wallet_internal_error, "Failed to parse proposal");
  return get_ionic_swap_proposal_info(proposal, proposal_info);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_ionic_swap_proposal_info(const wallet_public::ionic_swap_proposal& proposal, wallet_public::ionic_swap_proposal_info& proposal_info) const
{
  wallet_public::ionic_swap_proposal_context ionic_context = AUTO_VAL_INIT(ionic_context);
  return get_ionic_swap_proposal_info(proposal, proposal_info, ionic_context);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_ionic_swap_proposal_info(const wallet_public::ionic_swap_proposal& proposal, wallet_public::ionic_swap_proposal_info& proposal_info, wallet_public::ionic_swap_proposal_context& ionic_context) const
{
  const transaction& tx = proposal.tx_template;
  crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
  std::vector<wallet_out_info> outs;
  bool r = lookup_acc_outs(m_account.get_keys(), tx, outs, derivation);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "Failed to lookup_acc_outs for tx: " << get_transaction_hash(tx));

  if (!outs.size())
  {
    return false;
  }

  //decrypt context
  std::string decrypted_raw_context = crypto::chacha_crypt(proposal.encrypted_context, derivation);
  r = t_unserializable_object_from_blob(ionic_context, decrypted_raw_context);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "Failed to unserialize decrypted ionic_context");

  r = validate_tx_output_details_againt_tx_generation_context(tx, ionic_context.gen_context);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "Failed to validate decrypted ionic_context");

  std::unordered_map<crypto::public_key, uint64_t> amounts_provided_by_a;

  std::unordered_map<crypto::public_key, uint64_t> ammounts_to_a; //amounts to Alice (the one who created proposal), should be NOT funded
  std::unordered_map<crypto::public_key, uint64_t> ammounts_to_b; //amounts to Bob (the one who received proposal), should BE funded
  std::vector<bool> bob_outs;
  bob_outs.resize(proposal.tx_template.vout.size());
 
  for (const auto& o : outs)
  {
    THROW_IF_FALSE_WALLET_INT_ERR_EX(ionic_context.gen_context.asset_ids.size() > o.index, "Tx gen context has mismatch with tx(asset_ids) ");
    THROW_IF_FALSE_WALLET_INT_ERR_EX(ionic_context.gen_context.asset_ids[o.index].to_public_key() == o.asset_id, "Tx gen context has mismatch with tx(asset_id != asset_id) ");
    THROW_IF_FALSE_WALLET_INT_ERR_EX(ionic_context.gen_context.amounts[o.index].m_u64[0] == o.amount, "Tx gen context has mismatch with tx(amount != amount)");

    ammounts_to_b[o.asset_id] += o.amount;
    bob_outs[o.index] = true;
  }
  size_t i = 0;
  //validate outputs against decrypted tx generation context
  for (i = 0; i != tx.vout.size(); i++)
  {

    if (bob_outs[i])
    {
      continue;
    }

    crypto::public_key asset_id = ionic_context.gen_context.asset_ids[i].to_public_key();
    uint64_t amount = ionic_context.gen_context.amounts[i].m_u64[0];
    ammounts_to_a[asset_id] += amount;
  }

  //read amounts already provided by third party
  size_t zc_current_index = 0; //some inputs might be old ones, so it's asset id assumed as native and there is no entry for it in real_zc_ins_asset_ids
  //THROW_IF_FALSE_WALLET_INT_ERR_EX(ionic_context.gen_context.input_amounts.size() == tx.vin.size(), "Tx gen context has mismatch with tx(amount != amount)");
  for (i = 0; i != tx.vin.size(); i++)
  {
    size_t mx = 0;
    uint64_t amount = 0;
    crypto::public_key in_asset_id = currency::native_coin_asset_id;
    if (tx.vin[i].type() == typeid(txin_zc_input))
    {
      in_asset_id = ionic_context.gen_context.real_zc_ins_asset_ids[zc_current_index].to_public_key();
      amount = ionic_context.gen_context.zc_input_amounts[zc_current_index];
      zc_current_index++;
      mx = boost::get<currency::txin_zc_input>(tx.vin[i]).key_offsets.size() - 1;
    }
    else if (tx.vin[i].type() == typeid(txin_to_key))
    {
      amount = boost::get<txin_to_key>(tx.vin[i]).amount;
      mx = boost::get<currency::txin_to_key>(tx.vin[i]).key_offsets.size() - 1;
    }
    else
    {
      WLT_LOG_RED("Unexpected type of input in ionic_swap tx: " << tx.vin[i].type().name(), LOG_LEVEL_0);
      return false;
    }
    amounts_provided_by_a[in_asset_id] += amount;
    
    //if (proposal_info.mixins == 0 || proposal_info.mixins > mx)
    //{
    //  proposal_info.mixins = mx;
    //}

  }

  //this might be 0, if Alice don't want to pay fee herself
  proposal_info.fee_paid_by_a = currency::get_tx_fee(tx);
  if (proposal_info.fee_paid_by_a)
  {
    THROW_IF_FALSE_WALLET_INT_ERR_EX(amounts_provided_by_a[currency::native_coin_asset_id] >= proposal_info.fee_paid_by_a, "Fee mentioned as specified but not provided by A");
    amounts_provided_by_a[currency::native_coin_asset_id] -= proposal_info.fee_paid_by_a;
  }

  //proposal_info.fee = currency::get_tx_fee(tx);
  //need to make sure that funds for Bob properly funded
  for (const auto& a : ammounts_to_b)
  {
    uint64_t amount_sent_back_to_initiator = ammounts_to_a[a.first];

    if (amounts_provided_by_a[a.first] < (a.second + amount_sent_back_to_initiator) )
    {
      WLT_LOG_RED("Amount[" << a.first << "] provided by Alice(" << amounts_provided_by_a[a.first] << ") is less then transfered to Bob(" << a.second <<")", LOG_LEVEL_0);
      return false;
    }
    amounts_provided_by_a[a.first] -= (amount_sent_back_to_initiator + a.second);
    proposal_info.to_finalizer.push_back(view::asset_funds{ a.first, a.second });
    //clean accounted assets
    ammounts_to_a.erase(ammounts_to_a.find(a.first));
    if (amounts_provided_by_a[a.first] > 0)
    {
      WLT_LOG_RED("Amount[" << a.first << "] provided by Alice has unused leftovers: " << amounts_provided_by_a[a.first], LOG_LEVEL_0);
      return false;
    }
  }

  //need to see what Alice actually expect in return
  for (const auto& a : ammounts_to_a)
  {
    //now amount provided by A should be less or equal to what we have in a.second 
    if (amounts_provided_by_a[a.first] > a.second)
    {
      //could be fee
      WLT_LOG_RED("Amount[" << a.first << "] provided by Alice has unused leftovers: " << amounts_provided_by_a[a.first], LOG_LEVEL_0);
      return false;
    }

    proposal_info.to_initiator.push_back(view::asset_funds{ a.first, a.second - amounts_provided_by_a[a.first] });
  }
    
  return true;
}

//----------------------------------------------------------------------------------------------------
bool wallet2::accept_ionic_swap_proposal(const std::string& raw_proposal, currency::transaction& result_tx)
{
  wallet_public::ionic_swap_proposal proposal = AUTO_VAL_INIT(proposal);
  bool r = t_unserializable_object_from_blob(proposal, raw_proposal);
  THROW_IF_TRUE_WALLET_EX(!r, error::wallet_internal_error, "Failed to parse proposal info");

  return accept_ionic_swap_proposal(proposal, result_tx);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::accept_ionic_swap_proposal(const wallet_public::ionic_swap_proposal& proposal, currency::transaction& result_tx)
{
  mode_separate_context msc = AUTO_VAL_INIT(msc);
  msc.tx_for_mode_separate = proposal.tx_template;
  result_tx = msc.tx_for_mode_separate;

  wallet_public::ionic_swap_proposal_context ionic_context = AUTO_VAL_INIT(ionic_context);
  bool r = get_ionic_swap_proposal_info(proposal, msc.proposal_info, ionic_context);
  THROW_IF_TRUE_WALLET_EX(!r, error::wallet_internal_error, "Failed to get info from proposal");

  std::unordered_map<crypto::public_key, wallet_public::asset_balance_entry_base> balances;
  uint64_t mined = 0;
  this->balance(balances, mined);
  //validate balances needed 
  uint64_t native_amount_required = 0;
  for (const auto& item : msc.proposal_info.to_initiator)
  {
    if (balances[item.asset_id].unlocked < item.amount)
    {
      WLT_THROW_IF_FALSE_WALLET_EX_MES(false, error::not_enough_money, "", balances[item.asset_id].unlocked, item.amount, 0 /*fee*/, item.asset_id);
    }
    if (item.asset_id == currency::native_coin_asset_id)
    {
      native_amount_required = item.amount;
    }
  }

  // balances is ok, check if fee is added to tx
  uint64_t additional_fee = 0;
  if (msc.proposal_info.fee_paid_by_a < m_core_runtime_config.tx_default_fee)
  {
    additional_fee = m_core_runtime_config.tx_default_fee - msc.proposal_info.fee_paid_by_a;
    if (balances[currency::native_coin_asset_id].unlocked < additional_fee + native_amount_required)
    {
      WLT_THROW_IF_FALSE_WALLET_EX_MES(false, error::not_enough_money, "", balances[currency::native_coin_asset_id].unlocked, native_amount_required, additional_fee, currency::native_coin_asset_id);
    }
  }

  //everything is seemed to be ok

  construct_tx_param construct_param = get_default_construct_tx_param();
  construct_param.fee = additional_fee;
  
  crypto::secret_key one_time_key = ionic_context.gen_context.tx_key.sec; // TODO: figure out this mess with tx sec key -- sowle
  construct_param.crypt_address = m_account.get_public_address();
  construct_param.flags = TX_FLAG_SIGNATURE_MODE_SEPARATE;
  construct_param.mark_tx_as_complete = true;
  construct_param.need_at_least_1_zc = true;

  //build transaction
  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.tx_version = this->get_current_tx_version();
  ftp.gen_context = ionic_context.gen_context;  
  prepare_transaction(construct_param, ftp, msc);



  try
  {
    finalize_transaction(ftp, result_tx, one_time_key, true);
  }
  catch (...)
  {
    clear_transfers_from_flag(ftp.selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_SPENT, std::string("exception in finalize_transaction, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(result_tx)));
    throw;
  }
  mark_transfers_as_spent(ftp.selected_transfers, std::string("Proposal has been accepted with tx <" + epee::string_tools::pod_to_hex(get_transaction_hash(result_tx))) + ">");
  return true;
}
//----------------------------------------------------------------------------------------------------

// Signing and auth
bool wallet2::sign_buffer(const std::string& buff, crypto::signature& sig)
{
  crypto::hash h = crypto::cn_fast_hash(buff.data(), buff.size());
  crypto::generate_signature(h, m_account.get_public_address().spend_public_key, m_account.get_keys().spend_secret_key, sig);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::validate_sign(const std::string& buff, const crypto::signature& sig, const crypto::public_key& pkey)
{
  crypto::hash h = crypto::cn_fast_hash(buff.data(), buff.size());
  return crypto::check_signature(h, pkey, sig);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::encrypt_buffer(const std::string& buff, std::string& res_buff)
{
  res_buff = buff;
  crypto::chacha_crypt(res_buff, m_account.get_keys().view_secret_key);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::decrypt_buffer(const std::string& buff, std::string& res_buff)
{
  res_buff = buff;
  crypto::chacha_crypt(res_buff, m_account.get_keys().view_secret_key);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources_for_defragmentation_tx(std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies, uint64_t& found_money)
{
  if (!m_defragmentation_tx_enabled)
    return false;

  std::stringstream ss;
  if (epee::log_space::log_singletone::get_log_detalisation_level() >= LOG_LEVEL_2)
    ss << "preparing sources for utxo defragmentation tx:";
  for (size_t i = 0, size = m_transfers.size(); i < size && selected_indicies.size() < m_max_utxo_count_for_defragmentation_tx; ++i)
  {
    const auto& td = m_transfers[i];
    if (!td.is_native_coin() || td.m_amount > m_max_allowed_output_amount_for_defragmentation_tx)
      continue;

    uint64_t fake_outs_count_for_td = m_decoys_count_for_defragmentation_tx == SIZE_MAX ? (td.is_zc() ? m_core_runtime_config.hf4_minimum_mixins : CURRENCY_DEFAULT_DECOY_SET_SIZE) : m_decoys_count_for_defragmentation_tx;
    if (is_transfer_ready_to_go(td, fake_outs_count_for_td))
    {
      found_money += td.m_amount;
      selected_indicies.push_back(i);
      if (epee::log_space::log_singletone::get_log_detalisation_level() >= LOG_LEVEL_2)
        ss << "    selected transfer #" << i << ", amount: " << print_money_brief(td.m_amount) << ", height: " << td.m_ptx_wallet_info->m_block_height << ", " << (td.is_zc() ? "ZC" : "  ");
    }
  }

  if (selected_indicies.size() < m_min_utxo_count_for_defragmentation_tx || found_money <= TX_MINIMUM_FEE)
  {
    // too few outputs were found, hence don't create a defragmentation tx
    selected_indicies.clear();
    found_money = 0;
    return false;
  }

  WLT_LOG(ss.str(), LOG_LEVEL_2);

  return prepare_tx_sources(m_decoys_count_for_defragmentation_tx == SIZE_MAX ? CURRENCY_DEFAULT_DECOY_SET_SIZE : m_decoys_count_for_defragmentation_tx, sources, selected_indicies);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources(assets_selection_context& needed_money_map, size_t fake_outputs_count, uint64_t dust_threshold, std::vector<currency::tx_source_entry>& sources, std::vector<uint64_t>& selected_indicies)
{
  try
  {
    select_transfers(needed_money_map, fake_outputs_count, dust_threshold, selected_indicies); // always returns true, TODO consider refactoring -- sowle
    return prepare_tx_sources(fake_outputs_count, sources, selected_indicies);
  }
  catch(...)
  {
    // if smth went wrong -- invalidate transfers cache to trigger its regeneration on the next use
    // it is necessary because it may be in invalid state (some items might be erased within select_indices_for_transfer() or expand_selection_with_zc_input())
    m_found_free_amounts.clear();
    throw;
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::prefetch_global_indicies_if_needed(const std::vector<uint64_t>& selected_indicies)
{
  //std::list<std::reference_wrapper<const currency::transaction>> txs;
  //std::list<uint64_t> indices_that_requested_global_indicies;
  for (uint64_t i : selected_indicies)
  {
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(m_transfers[i].m_global_output_index != WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED,
      "m_transfers[" << i << "].m_global_output_index is WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED");
      //indices_that_requested_global_indicies.push_back(i);
      //txs.push_back(m_transfers[i].m_ptx_wallet_info->m_tx);
    //}
  }

  /*
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
  }*/
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources(size_t fake_outputs_count, std::vector<currency::tx_source_entry>& sources, const std::vector<uint64_t>& selected_indicies)
{
  return prepare_tx_sources(fake_outputs_count, false, sources, selected_indicies);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources(size_t fake_outputs_count_, bool use_all_decoys_if_found_less_than_required, std::vector<currency::tx_source_entry>& sources, const std::vector<uint64_t>& selected_indicies)
{
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
  typedef currency::tx_source_entry::output_entry tx_output_entry;

  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response daemon_resp = AUTO_VAL_INIT(daemon_resp);
  //we should request even of fake_outputs_count == 0, since for for postzarcanum this era this param is redefined
  //todo: remove if(true) block later if this code will be settled 
  if (true)
  {
    size_t fake_outputs_count = fake_outputs_count_;
    uint64_t zarcanum_start_from = m_core_runtime_config.hard_forks.m_height_the_hardfork_n_active_after[ZANO_HARDFORK_04_ZARCANUM];
    uint64_t current_size = m_chain.get_blockchain_current_size();

    bool need_to_request = fake_outputs_count != 0;
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request req = AUTO_VAL_INIT(req);
    req.height_upper_limit = m_last_pow_block_h;  // request decoys to be either older than, or the same age as stake output's height
    req.use_forced_mix_outs = false; // TODO: add this feature to UI later
    //req.decoys_count = fake_outputs_count + 1;    // one more to be able to skip a decoy in case it hits the real output
    for (uint64_t i: selected_indicies)
    {
      req.amounts.push_back(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::offsets_distribution());
      COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::offsets_distribution& rdisttib = req.amounts.back();
      
      auto it = m_transfers.begin() + i;
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it->m_ptx_wallet_info->m_tx.vout.size() > it->m_internal_output_index,
        "m_internal_output_index = " << it->m_internal_output_index <<
        " is greater or equal to outputs count = " << it->m_ptx_wallet_info->m_tx.vout.size());
      
      //rdisttib.own_global_index = it->m_global_output_index;
      //check if we have Zarcanum era output of pre-Zarcanum
      if (it->is_zc())
      {
        if(this->is_auditable())
          continue;
        //Zarcanum era
        rdisttib.amount = 0;
        //generate distribution in Zarcanum hardfork
        build_distribution_for_input(rdisttib.global_offsets, it->m_global_output_index);
        need_to_request = true;
      }
      else
      {
        //for prezarcanum era use flat distribution
        rdisttib.amount = it->m_amount;
        rdisttib.global_offsets.resize(fake_outputs_count + 1, 0);
      }
    }
    if (need_to_request)
    {
      size_t attempt_count = 0;
      while (true)
      {
        daemon_resp = COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response();
        bool r = m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3(req, daemon_resp);
        THROW_IF_FALSE_WALLET_EX(r, error::no_connection_to_daemon, "getrandom_outs3.bin");
        if (daemon_resp.status == API_RETURN_CODE_FAIL)
        {
          if (attempt_count < 10)
          {
            attempt_count++;
            continue;
          }
          else
          {
            WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(daemon_resp.outs.size() == selected_indicies.size(),
              "unable to exacute getrandom_outs2.bin after 10 attempts with code API_RETURN_CODE_FAIL, there must be problems with mixins");
          }
        }
        THROW_IF_FALSE_WALLET_EX(daemon_resp.status != API_RETURN_CODE_BUSY, error::daemon_busy, "getrandom_outs.bin");
        THROW_IF_FALSE_WALLET_EX(daemon_resp.status == API_RETURN_CODE_OK, error::get_random_outs_error, daemon_resp.status);
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(daemon_resp.outs.size() == selected_indicies.size(),
          "daemon returned wrong response for getrandom_outs2.bin, wrong amounts count = " << daemon_resp.outs.size() << ", expected: " << selected_indicies.size());
        break;
      }

      std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> scanty_outs;
      THROW_IF_FALSE_WALLET_EX(daemon_resp.outs.size() == req.amounts.size(), error::not_enough_outs_to_mix, scanty_outs, fake_outputs_count);

      if (!use_all_decoys_if_found_less_than_required)
      {
        // make sure we have received the requested number of decoys
        for(size_t i = 0; i != daemon_resp.outs.size(); i++)
          if (req.amounts[i].amount != 0 && daemon_resp.outs[i].outs.size() != req.amounts[i].global_offsets.size())
            scanty_outs.push_back(daemon_resp.outs[i]);
        THROW_IF_FALSE_WALLET_EX(scanty_outs.empty(), error::not_enough_outs_to_mix, scanty_outs, fake_outputs_count);
      }
    }
  }

  //lets prefetch m_global_output_index for selected_indicies
  //this days doesn't prefetch, only validated that prefetch is not needed
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
    src.asset_id = td.get_asset_id();
    size_t fake_outputs_count = fake_outputs_count_;
    //redefine for hardfork
    if (td.is_zc() && !this->is_auditable())
      fake_outputs_count = m_core_runtime_config.hf4_minimum_mixins;
    

    //paste mixin transaction
    if (daemon_resp.outs.size())
    {
      if (td.is_zc())
      {
        //get rid of unneeded 
        select_decoys(daemon_resp.outs[i], td.m_global_output_index);
      }
      else
      {
        //TODO: make sure we have exact count needed
      }
      
      daemon_resp.outs[i].outs.sort([](const out_entry& a, const out_entry& b){return a.global_amount_index < b.global_amount_index; });
      for(out_entry& daemon_oe : daemon_resp.outs[i].outs)
      {
        if (td.m_global_output_index == daemon_oe.global_amount_index)
          continue;
        tx_output_entry oe = AUTO_VAL_INIT(oe);
        oe.amount_commitment  = daemon_oe.amount_commitment;
        oe.concealing_point   = daemon_oe.concealing_point;
        oe.out_reference      = daemon_oe.global_amount_index;
        oe.stealth_address    = daemon_oe.stealth_address;
        oe.blinded_asset_id   = daemon_oe.blinded_asset_id;       // TODO @#@# BAD DESIGN, consider refactoring -- sowle
        src.outputs.push_back(oe);
        if (src.outputs.size() >= fake_outputs_count)
          break;
      }
    }

    //paste real transaction to the random index
    auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
    {
      if (a.out_reference.type().hash_code() == typeid(uint64_t).hash_code())
        return static_cast<bool>(boost::get<uint64_t>(a.out_reference) >= td.m_global_output_index);
      return false; // TODO: implement deterministics real output placement in case there're ref_by_id outs
    });
    //size_t real_index = src.outputs.size() ? (rand() % src.outputs.size() ):0;
    tx_output_entry real_oe = AUTO_VAL_INIT(real_oe);
    real_oe.out_reference = td.m_global_output_index; // TODO: use ref_by_id when neccessary
    VARIANT_SWITCH_BEGIN(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]);
    VARIANT_CASE_CONST(tx_out_bare, o)
    {
      VARIANT_SWITCH_BEGIN(o.target);
      VARIANT_CASE_CONST(txout_to_key, o)
        real_oe.stealth_address = o.key;
      VARIANT_CASE_CONST(txout_htlc, htlc)
        real_oe.stealth_address = htlc.pkey_refund;
      VARIANT_CASE_OTHER()
      {
        WLT_THROW_IF_FALSE_WITH_CODE(false,
          "Internal error: unexpected type of target: " << o.target.type().name(),
          API_RETURN_CODE_INTERNAL_ERROR);
      }
      VARIANT_SWITCH_END();
    }
    VARIANT_CASE_CONST(tx_out_zarcanum, o);
      real_oe.amount_commitment           = o.amount_commitment; // TODO @#@# consider using shorter code like in sweep_below() (or better reuse it)
      real_oe.concealing_point            = o.concealing_point;
      real_oe.stealth_address             = o.stealth_address;
      real_oe.blinded_asset_id            = o.blinded_asset_id;
      WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(td.is_zc(), "transfer #" << J << ", amount: " << print_money_brief(td.amount()) << " is not a ZC"); 
      src.real_out_amount_blinding_mask   = td.m_zc_info_ptr->amount_blinding_mask;
      src.real_out_asset_id_blinding_mask = td.m_zc_info_ptr->asset_id_blinding_mask;
      src.asset_id                        = td.m_zc_info_ptr->asset_id;
#ifndef NDEBUG
      WLT_CHECK_AND_ASSERT_MES(crypto::point_t(src.asset_id) + src.real_out_asset_id_blinding_mask * crypto::c_point_X == crypto::point_t(real_oe.blinded_asset_id).modify_mul8(), false, "real_out_asset_id_blinding_mask doesn't match real_oe.blinded_asset_id");
      WLT_CHECK_AND_ASSERT_MES(td.m_amount * crypto::point_t(real_oe.blinded_asset_id).modify_mul8() + src.real_out_amount_blinding_mask * crypto::c_point_G == crypto::point_t(real_oe.amount_commitment).modify_mul8(), false, "real_out_amount_blinding_mask doesn't match real_oe.amount_commitment");
#endif
    VARIANT_SWITCH_END();

    auto interted_it = src.outputs.insert(it_to_insert, real_oe);
    src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_ptx_wallet_info->m_tx);
    src.real_output = interted_it - src.outputs.begin();
    src.real_output_in_tx_index = td.m_internal_output_index;
    
    std::stringstream ss;
    ss << "source entry [" << i << "], td_idx: " << J << ", ";
    print_source_entry(ss, src);
    WLT_LOG_L1(ss.str());
    
    ++i;
  }
  return true;
}


//----------------------------------------------------------------------------------------------------------------
template<typename t_obj_container>
typename t_obj_container::value_type extract_random_from_container(t_obj_container& container)
{
  auto it = container.begin();
  std::advance(it, (crypto::rand<size_t>() % container.size()));
  typename t_obj_container::value_type obj = *it;
  container.erase(it);
  return obj;
}
//----------------------------------------------------------------------------------------------------------------
void wallet2::select_decoys(currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& amount_entry, uint64_t own_g_index)
{
  THROW_IF_FALSE_WALLET_INT_ERR_EX(amount_entry.amount == 0, "Amount is not 0 in zc decoys entry");
  typedef currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;

  //TODO: This strategy would be a subject for continuous refactoring
  
  //first take all real transactions if ther are some
  std::list<out_entry> local_outs;
  std::list<out_entry> coinbases;

  while (amount_entry.outs.size() && local_outs.size() != m_core_runtime_config.hf4_minimum_mixins)
  {
    out_entry entry = extract_random_from_container(amount_entry.outs);

    //
    if (entry.global_amount_index == own_g_index)
    {
      continue;
    }

    //skip auditable
    if ((entry.flags & (RANDOM_OUTPUTS_FOR_AMOUNTS_FLAGS_NOT_ALLOWED)))
    {
      continue;
    }
    if (entry.flags & (RANDOM_OUTPUTS_FOR_AMOUNTS_FLAGS_COINBASE))
    {
      coinbases.push_back(entry);
      continue;
    }    


    local_outs.push_back(entry);
  }

  //extend with coin base outs if needed
  while (coinbases.size() && local_outs.size() != m_core_runtime_config.hf4_minimum_mixins)
  {
    out_entry entry = extract_random_from_container(coinbases);
    local_outs.push_back(entry);
  }

  THROW_IF_FALSE_WALLET_INT_ERR_EX(local_outs.size() == m_core_runtime_config.hf4_minimum_mixins, "Amount is not 0 in zc decoys entry");
  amount_entry.outs = local_outs;  
}
//----------------------------------------------------------------------------------------------------------------
void wallet2::build_distribution_for_input(std::vector<uint64_t>& offsets, uint64_t own_index)
{
  decoy_selection_generator zarcanum_decoy_set_generator;
  zarcanum_decoy_set_generator.init(get_actual_zc_global_index());

  THROW_IF_FALSE_WALLET_INT_ERR_EX(zarcanum_decoy_set_generator.is_initialized(), "zarcanum_decoy_set_generator are not initialized");
  if (m_core_runtime_config.hf4_minimum_mixins)
  {
    uint64_t actual_zc_index = get_actual_zc_global_index();
    offsets = zarcanum_decoy_set_generator.generate_unique_reversed_distribution(actual_zc_index - 1 > WALLET_FETCH_RANDOM_OUTS_SIZE ? WALLET_FETCH_RANDOM_OUTS_SIZE : actual_zc_index - 1, own_index);
  }
}
//----------------------------------------------------------------------------------------------------------------
bool wallet2::prepare_tx_sources(crypto::hash multisig_id, std::vector<currency::tx_source_entry>& sources, uint64_t& found_money)
{
  auto it = m_multisig_transfers.find(multisig_id);
  THROW_IF_FALSE_WALLET_INT_ERR_EX(it != m_multisig_transfers.end(), "can't find multisig_id: " + epee::string_tools::pod_to_hex(multisig_id));
  THROW_IF_FALSE_WALLET_INT_ERR_EX(!it->second.is_spent(), "output with multisig_id: " + epee::string_tools::pod_to_hex(multisig_id) + " has already been spent by other party at height " + epee::string_tools::num_to_string_fast(it->second.m_spent_height));

  THROW_IF_FALSE_WALLET_INT_ERR_EX(it->second.m_internal_output_index < it->second.m_ptx_wallet_info->m_tx.vout.size(), "it->second.m_internal_output_index < it->second.m_tx.vout.size()");
  //@#@
  THROW_IF_FALSE_WALLET_INT_ERR_EX(it->second.m_ptx_wallet_info->m_tx.vout[it->second.m_internal_output_index].type() == typeid(tx_out_bare), "Unknown type id in prepare_tx_sources: " << it->second.m_ptx_wallet_info->m_tx.vout[it->second.m_internal_output_index].type().name());
  const tx_out_bare& out = boost::get<tx_out_bare>(it->second.m_ptx_wallet_info->m_tx.vout[it->second.m_internal_output_index]);
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
bool wallet2::prepare_tx_sources_htlc(crypto::hash htlc_tx_id, const std::string& origin, std::vector<currency::tx_source_entry>& sources, uint64_t& found_money)
{
  typedef currency::tx_source_entry::output_entry tx_output_entry;
  //lets figure out, if we have active htlc for this htlc
  auto it = m_active_htlcs_txid.find(htlc_tx_id);
  if (it == m_active_htlcs_txid.end())
  {
    WLT_THROW_IF_FALSE_WITH_CODE(false,
      "htlc not found with tx_id = " << htlc_tx_id, API_RETURN_CODE_NOT_FOUND);
  }

  WLT_THROW_IF_FALSE_WITH_CODE(m_transfers.size() > it->second,
    "Internal error: index in m_active_htlcs_txid <" << it->second << "> is bigger then size of m_transfers <" << m_transfers.size() << ">", API_RETURN_CODE_INTERNAL_ERROR);

  const transfer_details& td = m_transfers[it->second];
  //@#@
  WLT_THROW_IF_FALSE_WITH_CODE(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].type() == typeid(tx_out_bare),
    "Unexpected out type in prepare_tx_sources_htlc:" << td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index].type().name(), API_RETURN_CODE_INTERNAL_ERROR);

  const tx_out_bare& out_bare = boost::get<tx_out_bare>(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]);
  WLT_THROW_IF_FALSE_WITH_CODE(out_bare.target.type() == typeid(txout_htlc),
    "Unexpected type in active htlc", API_RETURN_CODE_INTERNAL_ERROR);

  const txout_htlc& htlc_out = boost::get<txout_htlc>(out_bare.target);
  bool use_sha256 = !(htlc_out.flags&CURRENCY_TXOUT_HTLC_FLAGS_HASH_TYPE_MASK);

  //check origin
  WLT_THROW_IF_FALSE_WITH_CODE(origin.size() != 0,
    "Origin for htlc is empty", API_RETURN_CODE_BAD_ARG);

  crypto::hash htlc_calculated_hash = currency::null_hash;
  if (use_sha256)
  {
    htlc_calculated_hash = crypto::sha256_hash(origin.data(), origin.size());
  }
  else
  {
    htlc_calculated_hash = crypto::RIPEMD160_hash_256(origin.data(), origin.size());
  }
  WLT_THROW_IF_FALSE_WITH_CODE(htlc_calculated_hash == htlc_out.htlc_hash,
    "Origin hash is missmatched with txout_htlc", API_RETURN_CODE_HTLC_ORIGIN_HASH_MISSMATCHED);

  sources.push_back(AUTO_VAL_INIT(currency::tx_source_entry()));
  currency::tx_source_entry& src = sources.back();
  tx_output_entry real_oe = AUTO_VAL_INIT(real_oe);
  real_oe.out_reference = td.m_global_output_index; // TODO: use ref_by_id when necessary
  real_oe.stealth_address = htlc_out.pkey_redeem;
  src.outputs.push_back(real_oe); //m_global_output_index should be prefetched
  src.amount = found_money = td.amount();
  src.real_output_in_tx_index = td.m_internal_output_index;
  src.real_output = 0;//no mixins supposed to be in htlc
  src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_ptx_wallet_info->m_tx);
  src.htlc_origin = origin;
  return true;

}
//----------------------------------------------------------------------------------------------------------------
assets_selection_context wallet2::get_needed_money(uint64_t fee, const std::vector<currency::tx_destination_entry>& dsts)
{
  assets_selection_context amounts_map;
  amounts_map[currency::native_coin_asset_id].needed_amount = fee;
  for(auto& dt : dsts)
  {
    if(dt.asset_id == currency::null_pkey)
      continue;     //this destination for emmition only

    THROW_IF_TRUE_WALLET_EX(0 == dt.amount, error::zero_destination);
    uint64_t money_to_add = dt.amount;
    if (dt.amount_to_provide || dt.flags & tx_destination_entry_flags::tdef_explicit_amount_to_provide)
      money_to_add = dt.amount_to_provide;
  
    amounts_map[dt.asset_id].needed_amount += money_to_add;
    THROW_IF_TRUE_WALLET_EX(amounts_map[dt.asset_id].needed_amount < money_to_add, error::tx_sum_overflow, dsts, fee);
    //clean up empty entries
    if (amounts_map[dt.asset_id].needed_amount == 0)
    {
      amounts_map.erase(amounts_map.find(dt.asset_id));
    }
  }
  return amounts_map;
}
//----------------------------------------------------------------------------------------------------------------
void wallet2::set_disable_tor_relay(bool disable)
{
  m_disable_tor_relay = disable;
}
//----------------------------------------------------------------------------------------------------------------
void wallet2::notify_state_change(const std::string& state_code, const std::string& details)
{
  m_wcallback->on_tor_status_change(state_code);
}
//----------------------------------------------------------------------------------------------------------------
void wallet2::send_transaction_to_network(const transaction& tx)
{
#ifndef DISABLE_TOR
  if (!m_disable_tor_relay)
  {
    //TODO check that core synchronized
    //epee::net_utils::levin_client2 p2p_client;
    
    //make few attempts
    tools::levin_over_tor_client p2p_client;
    p2p_client.get_transport().set_notifier(this);
    bool succeseful_sent = false;
    for (size_t i = 0; i != 3; i++)
    {
      if (!p2p_client.connect("144.76.183.143", 2121, 10000))
      {
        continue;//THROW_IF_FALSE_WALLET_EX(false, error::no_connection_to_daemon, "Failed to connect to TOR node");
      }


      currency::NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request p2p_req = AUTO_VAL_INIT(p2p_req);
      currency::NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::response p2p_rsp = AUTO_VAL_INIT(p2p_rsp);
      p2p_req.txs.push_back(t_serializable_object_to_blob(tx));
      this->notify_state_change(WALLET_LIB_STATE_SENDING);
      epee::net_utils::invoke_remote_command2(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::ID, p2p_req, p2p_rsp, p2p_client);
      p2p_client.disconnect();
      if (p2p_rsp.code == API_RETURN_CODE_OK)
      {
        this->notify_state_change(WALLET_LIB_SENT_SUCCESS);
        succeseful_sent = true;
        break;
      }
      this->notify_state_change(WALLET_LIB_SEND_FAILED);
      //checking if transaction got relayed to other nodes and 
      //return;
    }
    if (!succeseful_sent)
    {
      this->notify_state_change(WALLET_LIB_SEND_FAILED);
      THROW_IF_FALSE_WALLET_EX(succeseful_sent, error::no_connection_to_daemon, "Faile to build TOR stream");
    }
  }
  else
#endif //
  {
    COMMAND_RPC_SEND_RAW_TX::request req;
    req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(tx));
    COMMAND_RPC_SEND_RAW_TX::response daemon_send_resp;
    bool r = m_core_proxy->call_COMMAND_RPC_SEND_RAW_TX(req, daemon_send_resp);
    THROW_IF_TRUE_WALLET_EX(!r, error::no_connection_to_daemon, "sendrawtransaction");
    THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status == API_RETURN_CODE_BUSY, error::daemon_busy, "sendrawtransaction");
    THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status == API_RETURN_CODE_DISCONNECTED, error::no_connection_to_daemon, "Transfer attempt while daemon offline");
    THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status != API_RETURN_CODE_OK, error::tx_rejected, tx, daemon_send_resp.status);

    WLT_LOG_L2("transaction " << get_transaction_hash(tx) << " generated ok and sent to daemon:" << ENDL << currency::obj_to_json_str(tx));
  }

}
//----------------------------------------------------------------------------------------------------------------
void wallet2::add_sent_tx_detailed_info(const transaction& tx, const std::vector<currency::attachment_v>& decrypted_att,
  const std::vector<currency::tx_destination_entry>& destinations,
  const std::vector<uint64_t>& selected_transfers)
{
  payment_id_t payment_id;
  get_payment_id_from_decrypted_container(decrypted_att, payment_id);

  std::vector<std::string> recipients;
  std::unordered_set<account_public_address> used_addresses;
  for (const auto& d : destinations)
  {
    for (const auto& addr : d.addr)
    {
      if (used_addresses.insert(addr).second && addr != m_account.get_public_address())
        recipients.push_back(payment_id.empty() ? get_account_address_as_str(addr) : get_account_address_and_payment_id_as_str(addr, payment_id));
    }
  }
  if (!recipients.size())
  {
	  //transaction send to ourself
	  recipients.push_back(payment_id.empty() ? get_account_address_as_str(m_account.get_public_address()) : get_account_address_and_payment_id_as_str(m_account.get_public_address(), payment_id));
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
  load_wallet_transfer_info_flags(m_transfer_history[i]);
  switch (m_transfer_history[i].tx_type)
  {
    case GUI_TX_TYPE_PUSH_OFFER:
    {
      bc_services::offer_details od;
      if (!get_type_in_variant_container(m_transfer_history[i].marketplace_entries, od))
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
      if (!get_type_in_variant_container(m_transfer_history[i].marketplace_entries, uo))
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
      if (!get_type_in_variant_container(m_transfer_history[i].marketplace_entries, co))
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
bool wallet2::expand_selection_with_zc_input(assets_selection_context& needed_money_map, uint64_t fake_outputs_count, std::vector<uint64_t>& selected_indexes)
{
  free_amounts_cache_type& found_free_amounts = m_found_free_amounts[currency::native_coin_asset_id];
  auto& asset_needed_money_item = needed_money_map[currency::native_coin_asset_id];
  //need to add ZC input
  for (auto it = found_free_amounts.begin(); it != found_free_amounts.end(); it++)
  {
    for (auto it_in_amount = it->second.begin(); it_in_amount != it->second.end(); it_in_amount++)
    {
      if (!m_transfers[*it_in_amount].is_zc())
      {
        continue;
      }

      if (is_transfer_ready_to_go(m_transfers[*it->second.begin()], fake_outputs_count))
      {
        asset_needed_money_item.found_amount += it->first;
        selected_indexes.push_back(*it_in_amount);
        it->second.erase(it_in_amount);
        if (!it->second.size())
        {
          found_free_amounts.erase(it);
        }
        return true;
      }
    }
  }
  WLT_THROW_IF_FALSE_WALLET_EX_MES(false, error::no_zc_inputs, "At least one ZC is required for the operation, but none were found");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::select_indices_for_transfer(assets_selection_context& needed_money_map, uint64_t fake_outputs_count, std::vector<uint64_t>& selected_indexes)
{
  for (auto& item : needed_money_map)
  {
    if(item.second.needed_amount == 0)
      continue;
    auto asset_cashe_it = m_found_free_amounts.find(item.first);
    WLT_THROW_IF_FALSE_WALLET_EX_MES(asset_cashe_it != m_found_free_amounts.end(), error::not_enough_money, "", item.second.found_amount, item.second.needed_amount, 0, item.first);
    item.second.found_amount = select_indices_for_transfer(selected_indexes, asset_cashe_it->second, item.second.needed_amount, fake_outputs_count);
    WLT_THROW_IF_FALSE_WALLET_EX_MES(item.second.found_amount >= item.second.needed_amount, error::not_enough_money, "", item.second.found_amount, item.second.needed_amount, 0, item.first);
  }
  if (m_current_context.pconstruct_tx_param && m_current_context.pconstruct_tx_param->need_at_least_1_zc)
  {
    bool found_zc_input = false;
    for (auto i : selected_indexes)
    {
      if (m_transfers[i].is_zc())
      {
        found_zc_input = true;
        break;
      }
    }
    if (!found_zc_input)
    {
      expand_selection_with_zc_input(needed_money_map, fake_outputs_count, selected_indexes);
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::select_indices_for_transfer(std::vector<uint64_t>& selected_indexes, free_amounts_cache_type& found_free_amounts, uint64_t needed_money, uint64_t fake_outputs_count_)
{
  WLT_LOG_GREEN("Selecting indices for transfer of " << print_money_brief(needed_money) << " with " << fake_outputs_count_ << " fake outs, found_free_amounts.size()=" << found_free_amounts.size() << "...", LOG_LEVEL_0);
  uint64_t found_money = 0;
  //uint64_t found_zc_input = false;
  std::string selected_amounts_str;
  while (found_money < needed_money && found_free_amounts.size())
  {
    auto it = found_free_amounts.lower_bound(needed_money - found_money);
    if (!(it != found_free_amounts.end() && it->second.size()))
    {
      it = --found_free_amounts.end();
      WLT_CHECK_AND_ASSERT_MES(it->second.size(), 0, "internal error: empty found_free_amounts map");
    }
    uint64_t fake_outputs_count = fake_outputs_count_;
    if (!this->is_auditable() && m_transfers[*it->second.begin()].is_zc())
    {
      fake_outputs_count = m_core_runtime_config.hf4_minimum_mixins;
    }
    if (is_transfer_ready_to_go(m_transfers[*it->second.begin()], fake_outputs_count))
    {
      found_money += it->first;
      selected_indexes.push_back(*it->second.begin());
      WLT_LOG_L2("Selected index: " << *it->second.begin() << ", transfer_details: " << ENDL << epee::serialization::store_t_to_json(m_transfers[*it->second.begin()]));
      selected_amounts_str += (selected_amounts_str.empty() ? "" : "+") + print_money_brief(it->first);
    }
    it->second.erase(it->second.begin());
    if (!it->second.size())
      found_free_amounts.erase(it);

  }
  
  WLT_LOG_GREEN("Found " << print_money_brief(found_money) << " as " << selected_indexes.size() << " out(s): " << selected_amounts_str << ", found_free_amounts.size()=" << found_free_amounts.size(), LOG_LEVEL_0);
  return found_money;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_ready_to_go(const transfer_details& td, uint64_t fake_outputs_count) const
{
  if (is_transfer_able_to_go(td, fake_outputs_count) && is_transfer_unlocked(td))
  {
    return true;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_able_to_go(const transfer_details& td, uint64_t fake_outputs_count) const
{
  if (!td.is_spendable())
    return false;

  const tx_out_v &out_v = td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index];

  uint8_t mix_attr = CURRENCY_TO_KEY_OUT_RELAXED;
  if (get_mix_attr_from_tx_out_v(out_v, mix_attr))
  {
    if (!currency::is_mixattr_applicable_for_fake_outs_counter(td.m_ptx_wallet_info->m_tx.version, mix_attr, fake_outputs_count, m_core_runtime_config))
      return false;
  }

  VARIANT_SWITCH_BEGIN(out_v);
  VARIANT_CASE_CONST(tx_out_bare, o);
    if (o.target.type() == typeid(txout_htlc))
    {
      if (fake_outputs_count != 0)
        return false;
    }
  VARIANT_SWITCH_END();

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
      uint64_t fake_outputs_count_local = fake_outputs_count;
      if (td.m_zc_info_ptr)
      {
        //zarcanum out, redefine fake_outputs_count
        fake_outputs_count_local = this->is_auditable() ? 0 : m_core_runtime_config.hf4_minimum_mixins;
      }
      if (is_transfer_able_to_go(td, fake_outputs_count_local))
      {
        //@#@
        m_found_free_amounts[td.get_asset_id()][td.amount()].insert(i);
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
  //@#@
  for (auto i : indexs)
    add_transfer_to_transfers_cache(m_transfers[i].amount(), i, m_transfers[i].get_asset_id());
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_transfer_to_transfers_cache(uint64_t amount, uint64_t index, const crypto::public_key& asset_id /* = currency::native_coin_asset_id */)
{
  m_found_free_amounts[asset_id][amount].insert(index);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::select_transfers(assets_selection_context& needed_money_map, size_t fake_outputs_count, uint64_t /*dust_threshold*/, std::vector<uint64_t>& selected_indicies)
{
  prepare_free_transfers_cache(fake_outputs_count);
  return select_indices_for_transfer(needed_money_map, fake_outputs_count, selected_indicies);
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_sent_unconfirmed_tx(const currency::transaction& tx,  
                                      const std::vector<std::string>& recipients, 
                                      const std::vector<uint64_t>& selected_indicies, 
                                      const std::vector<currency::tx_destination_entry>& splitted_dsts)
{
  PROFILE_FUNC("wallet2::add_sent_unconfirmed_tx");
  process_transaction_context ptc(tx);
  ptc.recipients = recipients;
  for (auto addr : recipients)
    ptc.remote_aliases.push_back(get_alias_for_address(addr));

  handle_unconfirmed_tx(ptc);
  wallet_public::wallet_transfer_info& unconfirmed_wti = misc_utils::get_or_insert_value_initialized(m_unconfirmed_txs, currency::get_transaction_hash(tx));  
  //override some info that might be missing
  unconfirmed_wti.selected_indicies = selected_indicies;
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::get_alias_for_address(const std::string& addr)
{
  std::vector<std::string> aliases = get_aliases_for_address(addr);
  if (aliases.size())
    return aliases.front();
  return "";
}
//----------------------------------------------------------------------------------------------------
std::vector<std::string> wallet2::get_aliases_for_address(const std::string& addr)
{
  PROFILE_FUNC("wallet2::get_alias_for_address");
  currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request req = addr;
  currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response res = AUTO_VAL_INIT(res);
  std::vector<std::string> aliases;
  if (!m_core_proxy->call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(req, res))
  {
    WLT_LOG_L0("Failed to COMMAND_RPC_GET_ALIASES_BY_ADDRESS");
    return aliases;
  }
  for (auto& e : res.alias_info_list)
  {
    aliases.push_back(e.alias);
  }
  return aliases;
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, 
  const std::vector<currency::attachment_v>& attachments, 
  currency::transaction& tx)
{
  transfer(dsts, fake_outputs_count, unlock_time, fee, extra, attachments, get_current_split_strategy(), tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx);
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
void wallet2::process_genesis_if_needed(const currency::block& genesis, const std::vector<uint64_t>* pglobal_indexes)
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
  process_new_transaction(genesis.miner_tx, 0, genesis, pglobal_indexes);
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
void wallet2::print_source_entry(std::stringstream& output, const currency::tx_source_entry& src) const
{
  std::stringstream ss;
  for(auto& el : src.outputs)
    ss << el.out_reference << " ";
  
  output << "amount: " << print_money_brief(src.amount) << (src.is_zc() ? " (hidden)" : "")
    << ", real_output: " << src.real_output
    << ", real_output_in_tx_index: " << src.real_output_in_tx_index
    << ", indexes: " << ss.str();

  if (src.asset_id != currency::native_coin_asset_id)
    output << ", asset_id: " << print16(src.asset_id);
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
bool wallet2::is_need_to_split_outputs()
{
  return !is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM);
}
//----------------------------------------------------------------------------------------------------
void wallet2::prepare_tx_destinations(const assets_selection_context& needed_money_map,
  detail::split_strategy_id_t destination_split_strategy_id,
  const tx_dust_policy& dust_policy,
  const std::vector<currency::tx_destination_entry>& dsts,
  uint8_t tx_flags,
  std::vector<currency::tx_destination_entry>& final_destinations)
{
  
  /* 
     let's account all processes assets, so if there are some destinations 
     that haven't been present in needed_money_map we can add it to final destinations
     (could be in ionic swaps for example)
  */
  std::unordered_set<crypto::public_key> processed_assets;
  for (auto& el: needed_money_map)
  {
    prepare_tx_destinations(el.second.needed_amount, el.second.found_amount, destination_split_strategy_id, dust_policy, dsts, el.first, final_destinations);    
    processed_assets.insert(el.first);
  }

  if (is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM))
  {
    // special case for asset minting destinations
    for (auto& dst : dsts)
      if (dst.asset_id == currency::null_pkey || processed_assets.count(dst.asset_id) == 0)
        final_destinations.emplace_back(dst.amount, dst.addr, dst.asset_id);

    //exclude destinations that supposed to be burned (for ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN)
    for (size_t i = 0; i < final_destinations.size(); )
    {
      if (final_destinations[i].addr.size() == 0)
      {
        final_destinations.erase(final_destinations.begin() + i);
      }
      else
      {
        i++;
      }
    }

    if (!(tx_flags & TX_FLAG_SIGNATURE_MODE_SEPARATE))
    {
      if (final_destinations.empty())
      {
        // if there's no destinations -- make CURRENCY_TX_MIN_ALLOWED_OUTS empty destinations
        for(size_t i = 0; i < CURRENCY_TX_MIN_ALLOWED_OUTS; ++i)
          final_destinations.emplace_back(0, m_account.get_public_address());
      }
      else if (final_destinations.size() < CURRENCY_TX_MIN_ALLOWED_OUTS)
      {
        // if there's not ehough destinations items (i.e. outputs), split the last one
        tx_destination_entry de = final_destinations.back();
        final_destinations.pop_back();
        size_t items_to_be_added = CURRENCY_TX_MIN_ALLOWED_OUTS - final_destinations.size();
        // TODO: consider allowing to set them somewhere
        size_t num_digits_to_keep = CURRENCY_TX_OUTS_RND_SPLIT_DIGITS_TO_KEEP;
        decompose_amount_randomly(de.amount, [&](uint64_t amount){ de.amount = amount; final_destinations.push_back(de); }, items_to_be_added, num_digits_to_keep);
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(final_destinations.size() == CURRENCY_TX_MIN_ALLOWED_OUTS,
          "can't get necessary number of outputs using decompose_amount_randomly(), got " << final_destinations.size() << " while mininum is " << CURRENCY_TX_MIN_ALLOWED_OUTS);
      }
    }
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::prepare_tx_destinations(uint64_t needed_money,
  uint64_t found_money,
  detail::split_strategy_id_t destination_split_strategy_id,
  const tx_dust_policy& dust_policy,
  const std::vector<currency::tx_destination_entry>& dsts,
  const crypto::public_key& asset_id,
  std::vector<currency::tx_destination_entry>& final_destinations)
{
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(found_money >= needed_money, "found_money = " << print_money_brief(found_money) << " is less than needed_money = " << print_money_brief(needed_money) << ", assed_id: " << asset_id);

  if (is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM))
  {
    for(auto& dst : dsts)
    {
      if (dst.asset_id == asset_id)
        final_destinations.emplace_back(dst);
    }
    if (found_money > needed_money)
      final_destinations.emplace_back(found_money - needed_money, m_account.get_public_address(), asset_id); // returning back the change
  }
  else
  {
    // pre-HF4
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(asset_id == currency::native_coin_asset_id, "assets are not allowed prior to HF4");
    currency::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
    if (needed_money < found_money)
    {
      change_dts.addr.push_back(m_account.get_keys().account_address);
      change_dts.amount = found_money - needed_money;
    }
    uint64_t dust = 0;
    bool r = detail::apply_split_strategy_by_id(destination_split_strategy_id, dsts, change_dts, dust_policy.dust_threshold, final_destinations, dust, WALLET_MAX_ALLOWED_OUTPUT_AMOUNT);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "invalid split strategy id: " << destination_split_strategy_id);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(dust_policy.dust_threshold >= dust, "invalid dust value: dust = " << dust << ", dust_threshold = " << dust_policy.dust_threshold);

    if (0 != dust && !dust_policy.add_to_fee)
    {
      final_destinations.emplace_back(dust, dust_policy.addr_for_dust);
    }
  }
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_transaction(construct_tx_param& ctp, currency::finalize_tx_param& ftp, const mode_separate_context& msc)
{

  SET_CONTEXT_OBJ_FOR_SCOPE(pconstruct_tx_param, ctp);
  SET_CONTEXT_OBJ_FOR_SCOPE(pfinalize_tx_param, ftp);
  SET_CONTEXT_OBJ_FOR_SCOPE(pmode_separate_context, msc);

  TIME_MEASURE_START_MS(get_needed_money_time);

  const currency::transaction& tx_for_mode_separate = msc.tx_for_mode_separate;
  assets_selection_context needed_money_map = get_needed_money(ctp.fee, ctp.dsts);
  ftp.ado_current_asset_owner = ctp.ado_current_asset_owner;
  ftp.pthirdparty_sign_handler = ctp.pthirdparty_sign_handler;
  //
  // TODO @#@# need to do refactoring over this part to support hidden amounts and asset_id
  //
  if (ctp.flags & TX_FLAG_SIGNATURE_MODE_SEPARATE && tx_for_mode_separate.vout.size() )
  {
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(get_tx_flags(tx_for_mode_separate) & TX_FLAG_SIGNATURE_MODE_SEPARATE, "tx_param.flags differs from tx.flags");
    if (ftp.tx_version > TRANSACTION_VERSION_PRE_HF4)
    {
      for (const auto& el : msc.proposal_info.to_initiator)
        needed_money_map[el.asset_id].needed_amount += el.amount;
    }
    
    if (msc.escrow)
      needed_money_map[currency::native_coin_asset_id].needed_amount += (currency::get_outs_money_amount(tx_for_mode_separate) - get_inputs_money_amount(tx_for_mode_separate));
  }
  TIME_MEASURE_FINISH_MS(get_needed_money_time);

  //uint64_t found_money = 0;

  TIME_MEASURE_START_MS(prepare_tx_sources_time);
  if (ctp.create_utxo_defragmentation_tx)
  {
    try
    {
      if (!prepare_tx_sources_for_defragmentation_tx(ftp.sources, ftp.selected_transfers, needed_money_map[currency::native_coin_asset_id].found_amount))
        return false;
    }
    catch(const error::not_enough_outs_to_mix&) { return false; } // if there's not enough decoys, return false to indicate minor non-fatal error
  }
  else if (ctp.htlc_tx_id != currency::null_hash)
  {
    //htlc
    //@#@ need to do refactoring over this part to support hidden amounts and asset_id
    prepare_tx_sources_htlc(ctp.htlc_tx_id, ctp.htlc_origin, ftp.sources, needed_money_map[currency::native_coin_asset_id].found_amount);
    WLT_THROW_IF_FALSE_WITH_CODE(ctp.dsts.size() == 1,
      "htlc: unexpected ctp.dsts.size() =" << ctp.dsts.size(), API_RETURN_CODE_INTERNAL_ERROR);

    WLT_THROW_IF_FALSE_WITH_CODE(needed_money_map[currency::native_coin_asset_id].found_amount > ctp.fee,
      "htlc: found money less then fee", API_RETURN_CODE_INTERNAL_ERROR);

    //fill amount
    ctp.dsts.begin()->amount = needed_money_map[currency::native_coin_asset_id].found_amount - ctp.fee;
    
  }
  else if (ctp.multisig_id != currency::null_hash)
  {
    //multisig
    //@#@ need to do refactoring over this part to support hidden amounts and asset_id
    prepare_tx_sources(ctp.multisig_id, ftp.sources, needed_money_map[currency::native_coin_asset_id].found_amount);
  }
  else
  {
    //regular tx
    prepare_tx_sources(needed_money_map, ctp.fake_outputs_count, ctp.dust_policy.dust_threshold, ftp.sources, ftp.selected_transfers);
  }
  TIME_MEASURE_FINISH_MS(prepare_tx_sources_time);

  TIME_MEASURE_START_MS(prepare_tx_destinations_time);
  prepare_tx_destinations(needed_money_map, static_cast<detail::split_strategy_id_t>(ctp.split_strategy_id), ctp.dust_policy, ctp.dsts, ctp.flags, ftp.prepared_destinations);
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
  return true;
}
//----------------------------------------------------------------------------------------------------
void wallet2::finalize_transaction(currency::finalize_tx_param& ftp, currency::transaction& tx, crypto::secret_key& tx_key, bool broadcast_tx, bool store_tx_secret_key /* = true */)
{
  currency::finalized_tx result = AUTO_VAL_INIT(result);
  result.tx = tx;
  result.one_time_key = tx_key;
  finalize_transaction(ftp, result, broadcast_tx, store_tx_secret_key);
  tx = result.tx;
  tx_key = result.one_time_key;
}
//----------------------------------------------------------------------------------------------------
void wallet2::finalize_transaction(currency::finalize_tx_param& ftp, currency::finalized_tx& result, bool broadcast_tx, bool store_tx_secret_key /* = true */)
{
  // NOTE: if broadcast_tx == true callback rise_on_transfer2() may be called at the end of this function.
  // That callback may call balance(), so it's important to have all used/spending transfers
  // to be correctly marked with corresponding flags PRIOR to calling finalize_transaction()

  // broadcasting tx without secret key storing is forbidden to avoid lost key issues
  WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(!broadcast_tx || store_tx_secret_key, "finalize_tx is requested to broadcast a tx without storing the key");

  bool r = currency::construct_tx(m_account.get_keys(),
    ftp, result);
  //TIME_MEASURE_FINISH_MS(construct_tx_time);
  THROW_IF_FALSE_WALLET_EX(r, error::tx_not_constructed, ftp.sources, ftp.prepared_destinations, ftp.unlock_time);
  uint64_t effective_fee = get_tx_fee(result.tx);
  THROW_IF_FALSE_WALLET_CMN_ERR_EX(effective_fee <= WALLET_TX_MAX_ALLOWED_FEE, "tx fee is WAY too big: " << print_money_brief(effective_fee) << ", max allowed is " << print_money_brief(WALLET_TX_MAX_ALLOWED_FEE));

  //TIME_MEASURE_START_MS(sign_ms_input_time);
  if (ftp.multisig_id != currency::null_hash)
  {
    // In case there's multisig input is used -- sign it partially with this wallet's keys (we don't have any others here).
    // NOTE: this tx will not be ready to send until all other necessary signs for ms input would made.
    auto it = m_multisig_transfers.find(ftp.multisig_id);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(it != m_multisig_transfers.end(), "can't find multisig_id: " << ftp.multisig_id);
    const currency::transaction& ms_source_tx = it->second.m_ptx_wallet_info->m_tx;
    bool is_tx_input_fully_signed = false;
    r = sign_multisig_input_in_tx(result.tx, 0, m_account.get_keys(), ms_source_tx, &is_tx_input_fully_signed); // it's assumed that ms input is the first one (index 0)
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(r && !is_tx_input_fully_signed, "sign_multisig_input_in_tx failed: r = " << r << ", is_tx_input_fully_signed = " << is_tx_input_fully_signed);
  }
  //TIME_MEASURE_FINISH_MS(sign_ms_input_time);

  THROW_IF_FALSE_WALLET_EX(get_object_blobsize(result.tx) < CURRENCY_MAX_TRANSACTION_BLOB_SIZE, error::tx_too_big, result.tx, m_upper_transaction_size_limit);

  if (store_tx_secret_key)
    m_tx_keys.insert(std::make_pair(get_transaction_hash(result.tx), result.one_time_key));

  //TIME_MEASURE_START(send_transaction_to_network_time);
  if (broadcast_tx)
    send_transaction_to_network(result.tx);
  //TIME_MEASURE_FINISH(send_transaction_to_network_time);

  //TIME_MEASURE_START(add_sent_tx_detailed_info_time);
  if (broadcast_tx)
    add_sent_tx_detailed_info(result.tx, ftp.attachments, ftp.prepared_destinations, ftp.selected_transfers);
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
  std::string* p_unsigned_filename_or_tx_blob_str)
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
  transfer(ctp, tx, send_to_network, p_unsigned_filename_or_tx_blob_str);
}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
construct_tx_param wallet2::get_default_construct_tx_param_inital()
{
  construct_tx_param ctp = AUTO_VAL_INIT(ctp);

  ctp.fee = m_core_runtime_config.tx_default_fee;
  ctp.dust_policy = tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD);
  ctp.split_strategy_id = get_current_split_strategy();
  ctp.tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED;
  ctp.shuffle = 0;
  return ctp;
}
construct_tx_param wallet2::get_default_construct_tx_param()
{
  return get_default_construct_tx_param_inital();
}
//----------------------------------------------------------------------------------------------------
bool wallet2::store_unsigned_tx_to_file_and_reserve_transfers(const currency::finalize_tx_param& ftp, const std::string& filename, std::string* p_unsigned_tx_blob_str /* = nullptr */)
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
  bool has_payment_id = get_payment_id_from_decrypted_container(ctp.attachments, pid) && !pid.empty();
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(!has_payment_id, "sending funds to yourself with payment id is not allowed");
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(construct_tx_param& ctp,
  currency::transaction &tx,
  bool send_to_network,
  std::string* p_unsigned_filename_or_tx_blob_str)
{
  currency::finalized_tx result = AUTO_VAL_INIT(result);
  transfer(ctp, result, send_to_network, p_unsigned_filename_or_tx_blob_str);
  tx = result.tx;
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(construct_tx_param& ctp,
  currency::finalized_tx& result,
  bool send_to_network,
  std::string* p_unsigned_filename_or_tx_blob_str)
{
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(!is_auditable() || !is_watch_only(), "You can't initiate coins transfer using an auditable watch-only wallet."); // btw, watch-only wallets can call transfer() within cold-signing process

  check_and_throw_if_self_directed_tx_with_payment_id_requested(ctp);

  bool asset_operation_requested = count_type_in_variant_container<asset_descriptor_operation>(ctp.extra) != 0;
  bool dont_have_zero_asset_ids_in_destinations = std::count_if(ctp.dsts.begin(), ctp.dsts.end(), [](const tx_destination_entry& de) { return de.asset_id == null_pkey; }) == 0;
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(asset_operation_requested || dont_have_zero_asset_ids_in_destinations, "zero asset id is used errounesly (no asset operation was requested)");

  if (ctp.crypt_address.spend_public_key == currency::null_pkey)
  {
    ctp.crypt_address = currency::get_crypt_address_from_destinations(m_account.get_keys(), ctp.dsts);
  }

  TIME_MEASURE_START(prepare_transaction_time);
  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.pevents_dispatcher = &m_debug_events_dispatcher;
  ftp.tx_version = this->get_current_tx_version();
  if (!prepare_transaction(ctp, ftp))
  {
    result.was_not_prepared = true;
    return;
  }
  TIME_MEASURE_FINISH(prepare_transaction_time);

  if (m_watch_only)
  {
    bool r = store_unsigned_tx_to_file_and_reserve_transfers(ftp, (p_unsigned_filename_or_tx_blob_str != nullptr ? *p_unsigned_filename_or_tx_blob_str : "zano_tx_unsigned"), p_unsigned_filename_or_tx_blob_str);
    WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(r, "failed to store unsigned tx");
    WLT_LOG_GREEN("[wallet::transfer]" << " prepare_transaction_time: " << print_fixed_decimal_point(prepare_transaction_time, 3), LOG_LEVEL_0);
    return;
  }

  TIME_MEASURE_START(mark_transfers_as_spent_time);
  mark_transfers_as_spent(ftp.selected_transfers, std::string("money transfer, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(result.tx)));
  TIME_MEASURE_FINISH(mark_transfers_as_spent_time);

  TIME_MEASURE_START(finalize_transaction_time);
  try
  {
    finalize_transaction(ftp, result, send_to_network);
  }
  catch (...)
  {
    clear_transfers_from_flag(ftp.selected_transfers, WALLET_TRANSFER_DETAIL_FLAG_SPENT, std::string("exception on money transfer, tx: ") + epee::string_tools::pod_to_hex(get_transaction_hash(result.tx)));
    throw;
  }
  TIME_MEASURE_FINISH(finalize_transaction_time);


  WLT_LOG_GREEN("[wallet::transfer]"
    //<< "  precalculation_time: " << print_fixed_decimal_point(precalculation_time, 3)
    << ", prepare_transaction_time: " << print_fixed_decimal_point(prepare_transaction_time, 3)
    << ", finalize_transaction_time: " << print_fixed_decimal_point(finalize_transaction_time, 3)
    << ", mark_transfers_as_spent_time: " << print_fixed_decimal_point(mark_transfers_as_spent_time, 3)
    , LOG_LEVEL_0);

  print_tx_sent_message(result.tx, std::string() + "(transfer)", ctp.fee);
}

//----------------------------------------------------------------------------------------------------
void wallet2::sweep_below(size_t fake_outs_count, const currency::account_public_address& destination_addr, uint64_t threshold_amount, const currency::payment_id_t& payment_id,
  uint64_t fee, size_t& outs_total, uint64_t& amount_total, size_t& outs_swept, uint64_t& amount_swept, currency::transaction* p_result_tx /* = nullptr */, std::string* p_filename_or_unsigned_tx_blob_str /* = nullptr */)
{
  static const size_t estimated_bytes_per_input = 78;
  const size_t estimated_max_inputs = static_cast<size_t>(CURRENCY_MAX_TRANSACTION_BLOB_SIZE / (estimated_bytes_per_input * (fake_outs_count + 1.5))); // estimated number of maximum tx inputs under the tx size limit
  const size_t tx_sources_for_querying_random_outs_max = estimated_max_inputs * 2;

  bool r = false;
  outs_total = 0;
  amount_total = 0;
  outs_swept = 0;
  amount_swept = 0;

  std::vector<uint64_t> selected_transfers;
  selected_transfers.reserve(m_transfers.size());
  for (uint64_t i = 0; i < m_transfers.size(); ++i)
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
  std::sort(selected_transfers.begin(), selected_transfers.end(), [this](uint64_t a, uint64_t b) { return m_transfers[b].amount() < m_transfers[a].amount(); });

  // limit RPC request with reasonable number of sources
  if (selected_transfers.size() > tx_sources_for_querying_random_outs_max)
    selected_transfers.erase(selected_transfers.begin() + tx_sources_for_querying_random_outs_max, selected_transfers.end());

  prefetch_global_indicies_if_needed(selected_transfers);

  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
  typedef currency::tx_source_entry::output_entry tx_output_entry;

  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response rpc_get_random_outs_resp = AUTO_VAL_INIT(rpc_get_random_outs_resp);
  if (fake_outs_count > 0)
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
    req.height_upper_limit = m_last_pow_block_h;
    req.use_forced_mix_outs = false;
    req.decoys_count = fake_outs_count + 1;
    for (uint64_t i : selected_transfers)
      req.amounts.push_back(m_transfers[i].is_zc() ? 0 : m_transfers[i].m_amount);

    r = m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(req, rpc_get_random_outs_resp);
    
    THROW_IF_FALSE_WALLET_EX(r, error::no_connection_to_daemon, "getrandom_outs1.bin");
    THROW_IF_FALSE_WALLET_EX(rpc_get_random_outs_resp.status != API_RETURN_CODE_BUSY, error::daemon_busy, "getrandom_outs1.bin");
    THROW_IF_FALSE_WALLET_EX(rpc_get_random_outs_resp.status == API_RETURN_CODE_OK, error::get_random_outs_error, rpc_get_random_outs_resp.status);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(rpc_get_random_outs_resp.outs.size() == selected_transfers.size(),
      "daemon returned wrong number of amounts for getrandom_outs1.bin: " << rpc_get_random_outs_resp.outs.size() << ", requested: " << selected_transfers.size());

    std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> scanty_outs;
    for (COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& amount_outs : rpc_get_random_outs_resp.outs)
    {
      if (amount_outs.outs.size() < fake_outs_count)
        scanty_outs.push_back(amount_outs);
    }
    THROW_IF_FALSE_WALLET_EX(scanty_outs.empty(), error::not_enough_outs_to_mix, scanty_outs, fake_outs_count);
  }

  currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
  ftp.tx_version = this->get_current_tx_version();
  bool is_hf4 = this->is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM);
  if (!payment_id.empty())
    set_payment_id_to_tx(ftp.attachments, payment_id, is_hf4);
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
    (size_t st_index_upper_boundary, currency::finalize_tx_param& ftp, uint64_t& amount_swept) -> try_construct_result_t
  {
    // prepare inputs
    amount_swept = 0;
    ftp.sources.clear();
    ftp.sources.resize(st_index_upper_boundary);
    WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(st_index_upper_boundary <= selected_transfers.size(), "index_upper_boundary = " << st_index_upper_boundary << ", selected_transfers.size() = " << selected_transfers.size());
    for (size_t st_index = 0; st_index < st_index_upper_boundary; ++st_index)
    {
      currency::tx_source_entry& src = ftp.sources[st_index];
      uint64_t tr_index = selected_transfers[st_index];
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
          src.outputs.emplace_back(daemon_oe.global_amount_index, daemon_oe.stealth_address, daemon_oe.concealing_point, daemon_oe.amount_commitment, daemon_oe.blinded_asset_id);
          if (src.outputs.size() >= fake_outs_count)
            break;
        }
      }

      // insert real output into src.outputs
      // TODO: bad design, we need to get rid of code duplicates below -- sowle
      auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
      {
        if (a.out_reference.type().hash_code() == typeid(uint64_t).hash_code())
          return static_cast<bool>(boost::get<uint64_t>(a.out_reference) >= td.m_global_output_index);
        return false; // TODO: implement deterministics real output placement in case there're ref_by_id outs
      });
      tx_output_entry real_oe = AUTO_VAL_INIT(real_oe);
      txout_ref_v out_reference = td.m_global_output_index; // TODO: use ref_by_id when neccessary
      std::vector<tx_output_entry>::iterator interted_it  = src.outputs.end();
      VARIANT_SWITCH_BEGIN(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]);
      VARIANT_CASE_CONST(tx_out_bare, o)
      {
        VARIANT_SWITCH_BEGIN(o.target);
        VARIANT_CASE_CONST(txout_to_key, o)
          interted_it = src.outputs.emplace(it_to_insert, out_reference, o.key);
        VARIANT_CASE_CONST(txout_htlc, htlc)
          interted_it = src.outputs.emplace(it_to_insert, out_reference, htlc.pkey_refund);
        VARIANT_CASE_OTHER()
        {
          WLT_THROW_IF_FALSE_WITH_CODE(false,
            "Internal error: unexpected type of target: " << o.target.type().name(),
            API_RETURN_CODE_INTERNAL_ERROR);
        }
        VARIANT_SWITCH_END();
      }
      VARIANT_CASE_CONST(tx_out_zarcanum, o);
        interted_it = src.outputs.emplace(it_to_insert, out_reference, o.stealth_address, o.concealing_point, o.amount_commitment, o.blinded_asset_id);
        WLT_THROW_IF_FALSE_WALLET_INT_ERR_EX(td.is_zc(), "transfer #" << tr_index << ", amount: " << print_money_brief(td.amount()) << " is not a ZC"); 
        src.real_out_amount_blinding_mask   = td.m_zc_info_ptr->amount_blinding_mask;
        src.real_out_asset_id_blinding_mask = td.m_zc_info_ptr->asset_id_blinding_mask;
        src.asset_id                        = td.m_zc_info_ptr->asset_id;
      VARIANT_SWITCH_END();
      src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_ptx_wallet_info->m_tx);
      src.real_output = interted_it - src.outputs.begin();
      src.real_output_in_tx_index = td.m_internal_output_index;
    }

    if (amount_swept <= fee)
      return rc_too_few_outputs;

    // try to construct a transaction

    assets_selection_context needed_money_map;
    needed_money_map[currency::native_coin_asset_id] = {};
    const std::vector<currency::tx_destination_entry> dsts({ tx_destination_entry(amount_swept - fee, destination_addr) });
    prepare_tx_destinations(needed_money_map, get_current_split_strategy(), tools::tx_dust_policy(), dsts, ftp.flags, ftp.prepared_destinations);

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
  try_construct_result_t res = try_construct_tx(st_index_upper_boundary, ftp, amount_swept);
  
  WLT_THROW_IF_FALSE_WALLET_CMN_ERR_EX(res != rc_too_few_outputs, st_index_upper_boundary << " biggest unspent outputs have total amount of " << print_money_brief(amount_swept)
    << " which is less than required fee: " << print_money_brief(fee) << ", transaction cannot be constructed");
  
  if (res == rc_too_many_outputs)
  {
    WLT_LOG_L1("sweep_below: first try of try_construct_tx(" << st_index_upper_boundary << ") returned " << get_result_t_str(res));
    size_t low_bound = 0;
    size_t high_bound = st_index_upper_boundary;
    currency::finalize_tx_param ftp_ok = ftp;
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
