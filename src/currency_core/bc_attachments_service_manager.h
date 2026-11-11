// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <boost/program_options/variables_map.hpp>
#include "dispatch_core_events.h"
#include "core_runtime_config.h"

namespace currency
{
  struct i_bc_service
  {
    virtual ~i_bc_service() = default;

    virtual std::string get_id() = 0;
    virtual bool init(const std::string& config_folder, const boost::program_options::variables_map& vm) = 0;
    virtual bool deinit() = 0;
    virtual void handle_entry_push(const tx_service_attachment& a, size_t i, const transaction& tx, uint64_t h, const crypto::hash& bl_id, uint64_t timestamp) = 0;
    virtual void handle_entry_pop(const tx_service_attachment& a, size_t i, const transaction& tx, uint64_t h, uint64_t timestamp) = 0;
    virtual bool validate_entry(const tx_service_attachment& a, size_t i, const transaction& tx) = 0;
    virtual void set_event_handler(i_core_event_handler* event_handler) = 0;
    virtual void set_core_runtime_config(const core_runtime_config& rtc) = 0;
  };

  class bc_attachment_services_manager
  {
  public:
    bc_attachment_services_manager(/* i_core_event_handler* pcore_event_handler*/) : /*m_pcore_event_handler(pcore_event_handler),*/ m_core_runtime_config(get_default_core_runtime_config())
    {}
  

    void set_event_handler(i_core_event_handler* event_handler);
    void set_core_runtime_config(const core_runtime_config& rtc);
    bool add_service(i_bc_service* psrv);
    i_bc_service* get_service_by_id(const std::string& id) const;
    bool init(const std::string& config_folder, const boost::program_options::variables_map& vm);
    bool deinit();
    bool validate_entry(const tx_service_attachment& a, size_t i, const transaction& tx);
    void handle_entry_push(const tx_service_attachment& a, size_t i, const transaction& tx, uint64_t h, const crypto::hash& bl_id, uint64_t timestamp);
    void handle_entry_pop(const tx_service_attachment& a, size_t i, const transaction& tx, uint64_t h, uint64_t timestamp);
  
  private:
    std::map<std::string, i_bc_service*> m_services;
    //i_core_event_handler* m_pcore_event_handler;
    core_runtime_config m_core_runtime_config;

  };

}
