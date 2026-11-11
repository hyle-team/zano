// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bc_attachments_service_manager.h"

namespace currency
{
  bool bc_attachment_services_manager::add_service(i_bc_service* pservice)
  {
    if (pservice == nullptr)
      return false;

    if (m_services.count(pservice->get_id()))
    {
      LOG_ERROR("Service with id " << pservice->get_id() << "already registered");
      return false;
    }
    m_services[pservice->get_id()] = pservice;
    pservice->set_core_runtime_config(m_core_runtime_config);
    return true;
  }
  i_bc_service* bc_attachment_services_manager::get_service_by_id(const std::string& id) const
  {
    const auto& it = m_services.find(id);
    if (it == m_services.end())
      return nullptr;

    return it->second;
  }
  void bc_attachment_services_manager::set_event_handler(i_core_event_handler* event_handler)
  {
    for (auto& s : m_services)
    {
      s.second->set_event_handler(event_handler);
    }
  }
  void bc_attachment_services_manager::set_core_runtime_config(const core_runtime_config& rtc)
  {
    m_core_runtime_config = rtc;
    for (auto& s : m_services)
    {
      s.second->set_core_runtime_config(rtc);
    }
  }
  bool bc_attachment_services_manager::init(const std::string& config_folder, const boost::program_options::variables_map& vm)
  {
    //add service 
    for (auto& s : m_services)
    {
      s.second->init(config_folder, vm);
    }
    return true;
  }
  bool bc_attachment_services_manager::deinit()
  {
    //add service 
    for (auto& s : m_services)
    {
      s.second->deinit();
    }
    m_services.clear();
    return true;
  }
  bool bc_attachment_services_manager::validate_entry(const tx_service_attachment& a, size_t i, const transaction& tx)
  {
    auto it = m_services.find(a.service_id);
    if (it != m_services.end())
    {
      return it->second->validate_entry(a, i, tx);
    }
    return false;
  }
  void bc_attachment_services_manager::handle_entry_push(const tx_service_attachment& a, size_t i, const transaction& tx, uint64_t h, const crypto::hash& bl_id, uint64_t timestamp)
  {
    auto it = m_services.find(a.service_id);
    if (it != m_services.end())
    {
      it->second->handle_entry_push(a, i, tx, h, bl_id, timestamp);
    }
  }
  void bc_attachment_services_manager::handle_entry_pop(const tx_service_attachment& a, size_t i, const transaction& tx, uint64_t h, uint64_t timestamp)
  {
    auto it = m_services.find(a.service_id);
    if (it != m_services.end())
    {
      it->second->handle_entry_pop(a, i, tx, h, timestamp);
    }
  }
}
