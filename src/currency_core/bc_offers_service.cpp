// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bc_offers_service.h"

#include "include_base_utils.h"

#include "zlib_helper.h"
#include "currency_basic.h"
#include "currency_format_utils.h"
#include "storages/portable_storage_template_helper.h"


command_line::arg_descriptor<bool>      arg_market_disable  ( "disable-market", "Start GUI with market service disabled");


namespace bc_services
{

  bc_offers_service::bc_offers_service(currency::i_core_event_handler* pcore_event_handler)
    : m_pcore_event_handler(pcore_event_handler)
    , m_last_trimed_height(0)
    , m_core_runtime_config(currency::get_default_core_runtime_config())
    , m_last_seen_block_id(currency::null_hash)
    , m_deinitialized(false)
    , m_disabled(false)

  {}
  //------------------------------------------------------------------
  bc_offers_service::~bc_offers_service()
  {
    TRY_ENTRY();
    if (!m_deinitialized)
      deinit();
    CATCH_ENTRY_NO_RETURN();
  }
  //------------------------------------------------------------------
  bool bc_offers_service::init(const std::string& config_folder, const boost::program_options::variables_map& vm)
  {
    if (command_line::get_arg(vm, arg_market_disable))
    {
      LOG_PRINT_CYAN("Market DISABLED", LOG_LEVEL_0);
      m_disabled = true;
      return true;
    }
    LOG_PRINT_L0("Loading market...");
    m_config_folder = config_folder;
    std::string filename = m_config_folder + "/" BC_OFFERS_CURRENCY_MARKET_FILENAME;
    if (!tools::unserialize_obj_from_file(*this, filename))
    {
      LOG_PRINT_L0("Can't load market from file");
      //TODO: may be rebuild it from blockchain
    }
    return true;
  }
  //------------------------------------------------------------------
  void bc_offers_service::set_event_handler(currency::i_core_event_handler* event_handler)
  {
    m_pcore_event_handler = event_handler;
  }
  //------------------------------------------------------------------
  bool bc_offers_service::deinit()
  {
    LOG_PRINT_L0("Storing market...");
    CRITICAL_REGION_LOCAL(m_lock);
    std::string filename = m_config_folder + "/" BC_OFFERS_CURRENCY_MARKET_FILENAME;
    if (!tools::serialize_obj_to_file(*this, filename))
    {
      LOG_PRINT_L0("Can't store market from file");
      //TODO: may be rebuild it from blockchain
    }
    m_deinitialized = true;
    LOG_PRINT_L0("Market stored OK");
    return true;
  }
  //------------------------------------------------------------------
  void bc_offers_service::handle_entry_push(const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, const crypto::hash& bl_id, uint64_t timestamp)
  {
    if (m_disabled)
      return;
    //unpack data and pars json
    std::string json_buff;
    if (!epee::zlib_helper::unpack(a.body, json_buff))
    {
      LOG_ERROR("Filed to unpack tx_service_attachment in bc_offers_service, tx_id = " << currency::get_transaction_hash(tx));
      return;
    }

    bool r = false;
    if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_ADD)
    {
      offer_details od = AUTO_VAL_INIT(od);
      r = handle_entry_push(json_buff, a, od, i, tx, h, timestamp);
      CHECK_AND_ASSERT_MES_NO_RET(r, "offers service instruction " << a.instruction << " failed, offer's tx:oid : " << get_transaction_hash(tx) << ":" << i);
    }else if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_UPD)
    {
      update_offer od = AUTO_VAL_INIT(od);
      r = handle_entry_push(json_buff, a, od, i, tx, h, timestamp);
    }else if(a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_DEL)
    {
      cancel_offer od = AUTO_VAL_INIT(od);
      r = handle_entry_push(json_buff, a, od, i, tx, h, timestamp);
    }

  }
  //------------------------------------------------------------------


  bool bc_offers_service::trim_offers()
  {
    CRITICAL_REGION_LOCAL(m_lock);
    uint64_t current_time = m_core_runtime_config.get_core_time();
    auto& index = m_offers.get<by_timestamp>();
    uint64_t size_before = m_offers.size();
    for (auto it = index.begin(); it != index.end();)
    {
      if (it->timestamp + OFFER_MAXIMUM_LIFE_TIME < current_time)
        index.erase(it++);
      else
        break;
    }
    LOG_PRINT_GREEN("TRIM OFFERS: " << size_before - m_offers.size() << " offers removed", LOG_LEVEL_0);
    return true;
  }
  //------------------------------------------------------------------
  crypto::hash bc_offers_service::get_last_seen_block_id()
  {
    return m_last_seen_block_id;
  }
  //------------------------------------------------------------------
  void bc_offers_service::set_last_seen_block_id(const crypto::hash& h)
  {
    m_last_seen_block_id = h;
  }
  //------------------------------------------------------------------
  bool bc_offers_service::clear()
  {
    m_last_trimed_height = 0;
    m_last_seen_block_id = currency::null_hash;
    CRITICAL_REGION_LOCAL(m_lock);
    m_offers.clear();
    return true;
  }
  //------------------------------------------------------------------
  void bc_offers_service::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_market_disable);    
  }
  //------------------------------------------------------------------
  void bc_offers_service::handle_entry_pop(const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
    if (m_disabled)
      return;
    //unpack data and pars json
    std::string json_buff;
    if (!epee::zlib_helper::unpack(a.body, json_buff))
    {
      LOG_ERROR("Filed to unpack tx_service_attachment in bc_offers_service, tx_id = " << get_transaction_hash(tx));
      return;
    }

    bool r = false;
    if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_ADD)
    {
      offer_details od = AUTO_VAL_INIT(od);
      r = handle_entry_pop(json_buff, od, i, tx, h, timestamp);
    }
    else if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_UPD)
    {
      update_offer od = AUTO_VAL_INIT(od);
      r = handle_entry_pop(json_buff, od, i, tx, h, timestamp);
    }
    else if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_DEL)
    {
      cancel_offer od = AUTO_VAL_INIT(od);
      r = handle_entry_pop(json_buff, od, i, tx, h, timestamp);
    }
    if (!r)
    {
      LOG_ERROR("offers service instruction " << a.instruction << " failed, offer's tx:oid : " << get_transaction_hash(tx) << ":" << i);
    }


    //trim offers once a day
    if (m_last_trimed_height != h && !(h%CURRENCY_BLOCKS_PER_DAY))
    {
      trim_offers();
      m_last_trimed_height = h;
    }


  }
  //------------------------------------------------------------------------------------------------------------------------
  //      adding new offer 

  bool bc_offers_service::handle_push(offer_details& od_, const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
      offer_details_ex_with_hash od = AUTO_VAL_INIT(od);
      static_cast<offer_details&>(od) = od_;
      od.timestamp = timestamp;
      od.index_in_tx = i;
      crypto::hash tx_hash = get_transaction_hash(tx);
      od.tx_hash = tx_hash;
      od.stopped = false;
      CHECK_AND_ASSERT_MES(a.security.size(), false, "Add offer command: no security entry");
      od.security = a.security.back();
      od.fee = currency::get_tx_fee(tx) / currency::get_service_attachments_count_in_tx(tx);
      od.h = offer_id_from_hash_and_index(tx_hash, od.index_in_tx);
      bool r = put_offer_to_container(od);
      if (r)
        rise_core_event(CORE_EVENT_ADD_OFFER, od);
      return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  bool bc_offers_service::validate_entry(const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx)
  {
    if (a.service_id != BC_OFFERS_SERVICE_ID)
      return true;

    //don't validate other atm
    if (a.instruction != BC_OFFERS_SERVICE_INSTRUCTION_DEL)
      return true;

    std::string json_buff;
    if (!epee::zlib_helper::unpack(a.body, json_buff))
    {
      LOG_ERROR("Filed to unpack tx_service_attachment in bc_offers_service, tx_id = " << get_transaction_hash(tx));
      return false;
    }

    cancel_offer co = AUTO_VAL_INIT(co);
    bool r = epee::serialization::load_t_from_json(co, json_buff);
    if (!r)
    {
      LOG_ERROR("Filed to load json from tx_service_attachment in bc_offers_service, tx_id = " << get_transaction_hash(tx) << ", body: " << json_buff);
      return false;
    }
    offers_container::index<by_id>::type::iterator oit;
    return validate_cancel_order(co, oit);
  }
  //------------------------------------------------------------------------------------------------------------------------
  //      updating offer 
  bool bc_offers_service::handle_push(update_offer& uo, const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    auto& index = m_offers.get<by_id>();
    crypto::hash offer_id = offer_id_from_hash_and_index(uo.tx_id, uo.offer_index);
    auto it = index.find(offer_id);
    if (it == index.end())
    {
      LOG_PRINT_L2("Update offer command failed: offer not found, tx:idx : " << uo.tx_id << ":" << uo.offer_index << ", offer_id: " << offer_id);
      return false;
    }

      
    bool r = validate_modify_order_signature(*it, uo);
    CHECK_AND_NO_ASSERT_MES(r, false, "validate_modify_order_signature failed while trying to update an offer " << uo.tx_id << ":" << uo.offer_index);

    crypto::hash tx_id = get_transaction_hash(tx);

    offer_details_ex_with_hash new_off = AUTO_VAL_INIT(new_off);
    static_cast<offer_details&>(new_off) = uo.of;
    new_off.timestamp = it->timestamp;// keep original offer's timestmap
    new_off.index_in_tx = i;
    new_off.tx_hash = tx_id;
    new_off.stopped = false;
    CHECK_AND_ASSERT_MES(a.security.size(), false, "Update offer command: no security entry");
    new_off.security = a.security.back();

    new_off.fee = currency::get_tx_fee(tx) / get_service_attachments_count_in_tx(tx);
    new_off.h = offer_id_from_hash_and_index(tx_id, new_off.index_in_tx);
    put_offer_to_container(new_off);

    //update old order
    it->nxt_offer = epee::string_tools::pod_to_hex(new_off.h);
    it->stopped = true;

    //notify
    update_offer_details uop = AUTO_VAL_INIT(uop);
    uop.od = static_cast<offer_details_ex&>(new_off);
    uop.tx_id = uo.tx_id;
    uop.no = uo.offer_index;
    uop.wallet_id = 0; // ?? is 0 okay here?
    rise_core_event(CORE_EVENT_UPDATE_OFFER, uop);

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  //      canceling offer 
  bool bc_offers_service::handle_push(cancel_offer& co, const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    offers_container::index<by_id>::type::iterator it = m_offers.get<by_id>().end();
    bool r = validate_cancel_order(co, it);
    if (!r)
      return false;

    it->stopped = true;
    LOG_PRINT_MAGENTA("Offer " << co.tx_id << ":" << co.offer_index << " cancelled", LOG_LEVEL_0);
    rise_core_event(CORE_EVENT_REMOVE_OFFER, static_cast<const offer_details_ex&>(*it));
    return true;
  }
  //------------------------------------------------------------------
  bool bc_offers_service::validate_cancel_order(const cancel_offer& co, offers_container::index<by_id>::type::iterator& oit)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    auto& index = m_offers.get<by_id>();
    crypto::hash offer_id = offer_id_from_hash_and_index(co.tx_id, co.offer_index);
    oit = index.find(offer_id);
    if (oit == index.end())
    {
      LOG_PRINT_L2("Cancel offer command: tx " << co.tx_id << " not found in offers");
      return false;
    }

    bool r = validate_modify_order_signature(*oit, co);
    CHECK_AND_NO_ASSERT_MES(r, false, "failed to validate_modify_order_signature at validate_cancel_order");

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  bool bc_offers_service::handle_pop(offer_details& od, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
    crypto::hash tx_hash = currency::get_transaction_hash(tx);
    CRITICAL_REGION_LOCAL(m_lock);
    auto& index = m_offers.get<by_id>();
    crypto::hash oid = offer_id_from_hash_and_index(tx_hash, i);
    auto it = index.find(oid);
    if (it == index.end())
    {
      LOG_ERROR("Internal error: unprocess_blockchain_tx_attachments tx_id: " << tx_hash << " cnt_offers_ins=" << i << ", oid: " << oid << " not found");
    }
    else
    {
      rise_core_event(CORE_EVENT_REMOVE_OFFER, *it);
      index.erase(it);
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  bool bc_offers_service::handle_pop(update_offer& uo, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
    crypto::hash tx_id = currency::get_transaction_hash(tx);
    CRITICAL_REGION_LOCAL(m_lock);
    auto& index = m_offers.get<by_id>();
    crypto::hash oid = offer_id_from_hash_and_index(tx_id, i);
    crypto::hash original_oid = offer_id_from_hash_and_index(uo.tx_id, uo.offer_index);
    auto it = index.find(oid);
    CHECK_AND_ASSERT_MES(it != index.end(), false, "Unprocess update offer command: tx " << tx_id << "(" << oid << ") not found in offers");
    auto original_it = index.find(original_oid);
    CHECK_AND_ASSERT_MES(original_it != index.end(), false, "Unprocess update offer command: original tx " << uo.tx_id << "(" << original_oid << ") not found in offers");

    rise_core_event(CORE_EVENT_REMOVE_OFFER, *it);
    original_it->stopped = false;
    original_it->nxt_offer.clear();
    index.erase(it);
    rise_core_event(CORE_EVENT_ADD_OFFER, *original_it);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  bool bc_offers_service::handle_pop(cancel_offer& co, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    auto& index = m_offers.get<by_id>();
    crypto::hash offer_id = offer_id_from_hash_and_index(co.tx_id, co.offer_index);
    auto oit = index.find(offer_id);
    CHECK_AND_ASSERT_MES(oit != index.end(), false, "Cancel offer command: tx " << co.tx_id << " not found in offers");


    CHECK_AND_ASSERT_MES(oit->stopped, false, "Cancel offer command unprocess: tx "
      << co.tx_id << ": co.offer_index " << co.offer_index << ": not stopped yet");
    oit->stopped = false;
    rise_core_event(CORE_EVENT_ADD_OFFER, static_cast<const offer_details_ex&>(*oit));
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  bool bc_offers_service::put_offer_to_container(const offer_details_ex_with_hash& offer)
  {
    try
    {
      CRITICAL_REGION_LOCAL(m_lock);
      auto& offers_index = m_offers.get<by_id>();
      offers_index.insert(offer);
      return true;
    }
    catch (const std::exception& err)
    {
      LOG_ERROR("Failed to put offer to index, error: " << err.what() << ", offer[" << offer.h << "]: " << epee::serialization::store_t_to_json(offer));
      return false;
    }
    catch (...)
    {
      LOG_ERROR("Failed to put offer to index, offer[" << offer.h << "]: " << epee::serialization::store_t_to_json(offer));
      return false;
    }
  }
  //------------------------------------------------------------------------------------------------------------------------
  bool bc_offers_service::get_offers_by_id(const std::list<offer_id>& ids, std::list<offer_details_ex>& offers)
  {
    for (auto& id : ids)
    {
      crypto::hash original_id = id.tx_id;
      crypto::hash of_id = currency::null_hash;
      of_id = offer_id_from_hash_and_index(id);
      if (of_id == currency::null_hash)
        continue;

      CRITICAL_REGION_LOCAL(m_lock);
      auto& index_by_id = m_offers.get<by_id>();
      auto it = index_by_id.find(of_id);
      if (it == index_by_id.end())
      {
        LOG_PRINT_L2("Unable to find offer by tx id: " << id.tx_id << ", offer id: " << of_id);
        continue;
      }

      if (it != index_by_id.end() && it->nxt_offer.size())
      {
        crypto::hash nxt_h = currency::null_hash;
        if (!epee::string_tools::parse_tpod_from_hex_string(it->nxt_offer, nxt_h))
        {
          LOG_PRINT_L2("Unable to parse hash from  nxt_h: " << it->nxt_offer);
        }
        else
        {
          it = index_by_id.find(nxt_h);
        }
      }

      if (it == index_by_id.end())
      {
        LOG_PRINT_L2("Unable to find offer by tx id: " << id.tx_id << ", offer id: " << of_id);
        continue;
      }
      offers.push_back(*it);

      if (offers.back().tx_hash != original_id)
        offers.back().tx_original_hash = original_id;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  void bc_offers_service::print_market(const std::string& file)
  {
    std::stringstream ss;
    CRITICAL_REGION_LOCAL(m_lock);
    for (auto& o : m_offers)
    {
      ss << "id: " << o.h << ENDL
        << "nxt_id: " << o.nxt_offer << ENDL
        << epee::serialization::store_t_to_json(o) << ENDL
        << "----------------------------------------------" << ENDL;
    }
    if (epee::file_io_utils::save_string_to_file(file, ss.str()))
    {
      LOG_PRINT_L0("Market writen to file: " << file);
    }
    else
    {
      LOG_PRINT_L0("Failed to write market to file: " << file);
    }
  }
  //------------------------------------------------------------------------------------------------------------------------
  void bc_offers_service::set_core_runtime_config(const currency::core_runtime_config& rtc)
  {
    m_core_runtime_config = rtc;
  }
  //------------------------------------------------------------------
  bool bc_offers_service::get_offers_ex(const core_offers_filter& cof, std::list<offer_details_ex>& offers, uint64_t& total_count, uint64_t current_core_time)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    total_count = m_offers.size();
#define SET_CASE_FOR_ORDER_TYPE(order_type_name) case order_type_name: return get_offers_ex_for_index<sort_id_to_type<order_type_name>::index_type>(cof, offers, current_core_time);

    switch (cof.order_by)
    {
      SET_CASE_FOR_ORDER_TYPE(ORDER_BY_TIMESTAMP);
      SET_CASE_FOR_ORDER_TYPE(ORDER_BY_AMOUNT_PRIMARY);
      SET_CASE_FOR_ORDER_TYPE(ORDER_BY_AMOUNT_TARGET);
      SET_CASE_FOR_ORDER_TYPE(ORDER_BY_AMOUNT_RATE);
      SET_CASE_FOR_ORDER_TYPE(ORDER_BY_PAYMENT_TYPES);
      SET_CASE_FOR_ORDER_TYPE(ORDER_BY_CONTACTS);
      SET_CASE_FOR_ORDER_TYPE(ORDER_BY_LOCATION);
      SET_CASE_FOR_ORDER_TYPE(ORDER_BY_NAME);
    default:
      LOG_ERROR("Unknown order_by id: " << cof.order_by);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------

}
