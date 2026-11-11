// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/list.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "include_base_utils.h"

#include "bc_offers_service_basic.h"
#include "common/boost_serialization_helper.h"
#include "bc_offers_serialization.h"
#include "bc_attachments_service_manager.h"
#include "offers_services_helpers.h"
#include "currency_format_utils.h"
#include "common/command_line.h"


extern command_line::arg_descriptor<bool> arg_market_disable;

namespace bc_services
{
  class bc_offers_service : public currency::i_bc_service
  {
  public:
    bc_offers_service(currency::i_core_event_handler* pcore_event_handler);
    ~bc_offers_service();

    typedef boost::multi_index::multi_index_container <
      odeh,
      boost::multi_index::indexed_by<
      boost::multi_index::hashed_unique<boost::multi_index::tag<by_id>, boost::multi_index::global_fun<const odeh&, crypto::hash, extract_id>>,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_timestamp>, boost::multi_index::global_fun<const odeh&, uint64_t, extrct_timestamp> >,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_amount_primary>, boost::multi_index::global_fun<const odeh&, uint64_t, extract_amount_primary> >,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_amount_target>, boost::multi_index::global_fun<const odeh&, uint64_t, extract_amount_target> >,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_rate>, boost::multi_index::global_fun<const odeh&, double, extract_rate> >,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_payment_types>, boost::multi_index::global_fun<const odeh&, size_t, extract_payment_types> >,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_contacts>, boost::multi_index::global_fun<const odeh&, std::string, extract_contacts> >,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_location>, boost::multi_index::global_fun<const odeh&, std::string, extract_location> >,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_name>, boost::multi_index::global_fun<const odeh&, std::string, extract_name> >
      >
    > offers_container;
    template<class archive_t>
    void serialize(archive_t & ar, const unsigned int version);
    bool validate_cancel_order(const cancel_offer& co, offers_container::index<by_id>::type::iterator& oit);
    bool get_offers_by_id(const std::list<offer_id>& ids, std::list<offer_details_ex>& offers);
    bool get_offers_ex(const core_offers_filter& cof, std::list<offer_details_ex>& offers, uint64_t& total_count, uint64_t current_core_time);
    void print_market(const std::string& file);

  public:
    // these members are made public only to be accessible from tests
    offers_container& get_offers_container(){ return m_offers; } //TODO: need refactoring, bad design, atm just for performance tests
    bool trim_offers();
    crypto::hash get_last_seen_block_id();
    void set_last_seen_block_id(const crypto::hash& h);
    bool clear();
    bool set_disabled(bool is_disabled) { m_disabled = is_disabled; return m_disabled; }
    bool is_disabled(){ return m_disabled; }
    static void init_options(boost::program_options::options_description& desc);
  private:
    //-------------------- currency::i_bc_service ---------------------------------------
    virtual std::string get_id() override { return BC_OFFERS_SERVICE_ID; }
    virtual void set_event_handler(currency::i_core_event_handler* event_handler) override;
    virtual bool init(const std::string& config_folder, const boost::program_options::variables_map& vm) override;
    virtual bool deinit() override;
    virtual void handle_entry_push(const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, const crypto::hash& bl_id, uint64_t timestamp) override;
    virtual void handle_entry_pop(const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp) override;
    virtual bool validate_entry(const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx) override;
    virtual void set_core_runtime_config(const currency::core_runtime_config& rtc) override;
    //-----------------------------------------------------------------------------------

    bool handle_push(offer_details& od, const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp);
    bool handle_push(update_offer& od, const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp);
    bool handle_push(cancel_offer& od, const currency::tx_service_attachment& a, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp);
    bool handle_pop(offer_details& od, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp);
    bool handle_pop(update_offer& od, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp);
    bool handle_pop(cancel_offer& od, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp);
    bool put_offer_to_container(const offer_details_ex_with_hash& offer);

    template<class t_event_details>
    void rise_core_event(const std::string& event_name, const t_event_details& ed);
    template<class t_offer>
    bool handle_entry_push(const std::string& body, const currency::tx_service_attachment& a, t_offer& offer_instruction, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp);
    template<class t_offer>
    bool handle_entry_pop(const std::string& body, t_offer& offer_instruction, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp);
    template<class t_modify_offer>
    bool validate_modify_order_signature(const offer_details_ex_with_hash &odeh, const t_modify_offer& co);
    template<class market_index_type>
    bool get_offers_ex_for_index(const core_offers_filter& cof, std::list<offer_details_ex>& offers, uint64_t current_core_time);


    //offers
    offers_container m_offers; //offers indexed by 
    currency::i_core_event_handler* m_pcore_event_handler;
    uint64_t m_last_trimed_height;
    std::string m_config_folder;
    crypto::hash m_last_seen_block_id;
    epee::critical_section m_lock; 
    currency::core_runtime_config m_core_runtime_config;
    bool m_deinitialized;
    bool m_disabled;
    //---------------------------------------------------------------------------------------------------------------------------------------------

  };

  template<class t_event_details>
  void bc_offers_service::rise_core_event(const std::string& event_name, const t_event_details& ed)
  {
    currency::core_event_v e(ed);
    if (m_pcore_event_handler)
      m_pcore_event_handler->on_core_event(event_name, e);
  }
  //---------------------------------------------------------------------------------------------------------------------------------------------
  template<class t_offer>
  bool bc_offers_service::handle_entry_push(const std::string& body, const currency::tx_service_attachment& a, t_offer& offer_instruction, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
    bool r = epee::serialization::load_t_from_json(offer_instruction, body);
    if (!r)
    {
      LOG_ERROR("Filed to load json from tx_service_attachment in bc_offers_service, tx_id = " << currency::get_transaction_hash(tx) << ", body: " << body);
      return false;
    }
    return handle_push(offer_instruction, a, i, tx, h, timestamp);
  }
  //---------------------------------------------------------------------------------------------------------------------------------------------
  template<class t_offer>
  bool bc_offers_service::handle_entry_pop(const std::string& body, t_offer& offer_instruction, size_t i, const currency::transaction& tx, uint64_t h, uint64_t timestamp)
  {
    bool r = epee::serialization::load_t_from_json(offer_instruction, body);
    if (!r)
    {
      LOG_ERROR("Filed to load json from tx_service_attachment in bc_offers_service, tx_id = " << currency::get_transaction_hash(tx) << ", body: " << body);
      return false;
    }
    return handle_pop(offer_instruction, i, tx, h, timestamp);
  }
  //---------------------------------------------------------------------------------------------------------------------------------------------
  template<class t_modify_offer>
  bool bc_offers_service::validate_modify_order_signature(const offer_details_ex_with_hash &odeh, const t_modify_offer& co)
  {
    if (odeh.stopped)
    {
      LOG_PRINT_YELLOW("offer command validation failed: already stopped, tx: " << co.tx_id << ", offer_index: " << co.offer_index, LOG_LEVEL_0);
      return false;
    }

    currency::blobdata buff_to_check_sig = make_offer_sig_blob(co);
    bool res = crypto::check_signature(crypto::cn_fast_hash(buff_to_check_sig.data(), buff_to_check_sig.size()), odeh.security, co.sig);
    CHECK_AND_ASSERT_MES(res, false, "Signature check failed for offer command: tx " << co.tx_id << ", onetime_offer_pub_key=" << epee::string_tools::pod_to_hex(odeh.security));

    return true;
  }
  //---------------------------------------------------------------------------------------------------------------------------------------------
  template<class market_index_type>
  bool bc_offers_service::get_offers_ex_for_index(const core_offers_filter& cof, std::list<offer_details_ex>& offers, uint64_t current_core_time)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    if (!m_offers.size())
      return false;

    auto& current_index = m_offers.get<market_index_type>();

    auto it = current_index.begin();
    if (cof.reverse)
      it = --current_index.end();

    uint64_t selected_index = 0;
    while (it != current_index.end())
    {
      //--------
      if (selected_index >= cof.offset + cof.limit)
        break;

      if (is_offer_matched_by_filter(*it, cof, current_core_time))
      {
        //if we after offset position
        if (selected_index >= cof.offset)
          offers.push_back(*it);

        selected_index++;
      }

      //--------
      //prepare next iteration
      if (cof.reverse)
      {
        if (it == current_index.begin())
          break;
        else
          --it;
      }
      else
      {
        ++it;
      }
    }

    return true;
  }
  //---------------------------------------------------------------------------------------------------------------------------------------------
  template<class archive_t>
  void bc_offers_service::serialize(archive_t & ar, const unsigned int version)
  {
    if (version < BC_OFFERS_CURRENT_OFFERS_SERVICE_ARCHIVE_VER)
    {
      LOG_PRINT_BLUE("[OFFERS_SERVICE] data truncated cz new version", LOG_LEVEL_0);
      return;
    }
    CHECK_PROJECT_NAME();
    ar & m_last_trimed_height;
    ar & m_last_seen_block_id;

    std::list<odeh> temp;
    if (archive_t::is_saving::value)
    {     
      auto& index = m_offers.get<by_timestamp>();
      for (auto it = index.begin(); it != index.end();it++)
      {
        temp.push_back(*it);
      }
      ar & temp;
    }
    else
    {
      ar & temp;
      for (const auto& o : temp)
      {
        TRY_ENTRY()
          m_offers.insert(o);
        CATCH_ENTRY("error while reading market storage ", void());
      }

    }   
    //ar & m_offers;
  }
}
BOOST_CLASS_VERSION(bc_services::bc_offers_service, BC_OFFERS_CURRENT_OFFERS_SERVICE_ARCHIVE_VER)
