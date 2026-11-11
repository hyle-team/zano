// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include <thread>
#include <string>
#include <boost/thread.hpp>
#include "include_base_utils.h"
extern "C" {
#include "miniupnp/miniupnpc/miniupnpc.h"
#include "miniupnp/miniupnpc/upnpcommands.h"
#include "miniupnp/miniupnpc/upnperrors.h"
}

#include "misc_language.h"
#include "currency_core/currency_config.h"
#include "version.h"

namespace tools
{
  class miniupnp_helper
  {
    UPNPDev *m_devlist;
    UPNPUrls m_urls;
    IGDdatas m_data;
    char m_lanaddr[64];
    int m_IGD;
    boost::thread m_mapper_thread;
    boost::thread m_initializer_thread;
    uint32_t m_external_port;
    uint32_t m_internal_port;
    uint32_t m_period_ms;
  public: 
    miniupnp_helper():m_devlist(nullptr),
      m_urls(AUTO_VAL_INIT(m_urls)), 
      m_data(AUTO_VAL_INIT(m_data)),
      m_IGD(0),
      m_external_port(0),
      m_internal_port(0),
      m_period_ms(0)
    {
      m_lanaddr[0] = 0;
    }
    ~miniupnp_helper()
    {
      NESTED_TRY_ENTRY();

      deinit();

      NESTED_CATCH_ENTRY(__func__);
    }

    bool start_regular_mapping(uint32_t internal_port, uint32_t external_port, uint32_t period_ms)
    {
      m_external_port = external_port;
      m_internal_port = internal_port;
      m_period_ms = period_ms;
      if(!init())
        return false;
      return true;
    }

    bool stop_mapping()
    {
      if(m_mapper_thread.joinable())
      {
        m_mapper_thread.interrupt();
        m_mapper_thread.join();
      }

      if(m_IGD == 1)
      {
        do_port_unmapping();
      }
      return true;
    }

  private:

    bool init()
    {
      m_initializer_thread = boost::thread([=]()
      {
        deinit();

        int error = 0;
        m_devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, &error);
        if (error)
        {
          LOG_PRINT_L0("Failed to call upnpDiscover");
          return false;
        }

        m_IGD = UPNP_GetValidIGD(m_devlist, &m_urls, &m_data, m_lanaddr, sizeof(m_lanaddr));
        if (m_IGD != 1)
        {
          LOG_PRINT_L2("IGD not found");
          return false;
        }

        m_mapper_thread = boost::thread([=]() {run_port_mapping_loop(m_internal_port, m_external_port, m_period_ms); });
        return true;
      });
      return true;
    }

    bool deinit()
    {
      if(m_initializer_thread.get_id() != boost::this_thread::get_id())
      {
        if (m_initializer_thread.joinable())
        {
          m_initializer_thread.interrupt();
          m_initializer_thread.join();
        }
      }

      stop_mapping();

      if(m_devlist)
      {
        freeUPNPDevlist(m_devlist); 
        m_devlist = nullptr;
      }

      if(m_IGD > 0)
      {
        FreeUPNPUrls(&m_urls);
        m_IGD = 0;
      }
      return true;
    }

    bool run_port_mapping_loop(uint32_t internal_port, uint32_t external_port, uint32_t period_ms)
    {
      while(true)
      {
        do_port_mapping(external_port, internal_port);
        boost::this_thread::sleep_for(boost::chrono::milliseconds( period_ms ));
      }
      return true;
    }

    bool do_port_mapping(uint32_t external_port, uint32_t internal_port)
    {
      std::string internal_port_str = std::to_string(internal_port);
      std::string external_port_str = std::to_string(external_port);
      std::string str_desc = CURRENCY_NAME " v" PROJECT_VERSION_LONG;

      int r = UPNP_AddPortMapping(m_urls.controlURL, m_data.first.servicetype,
              external_port_str.c_str(), internal_port_str.c_str(), m_lanaddr, str_desc.c_str(), "TCP", nullptr, "0");

      if(r!=UPNPCOMMAND_SUCCESS)
      {
        LOG_PRINT_L1("AddPortMapping with external_port_str= " << external_port_str << 
                                         ", internal_port_str="  << internal_port_str << 
                                         ", failed with code=" << r << "(" << strupnperror(r) << ")");
      }else
      {
        LOG_PRINT_L0("[upnp] port mapped successful (ext: " << external_port_str << ", int:"  << internal_port_str << ")");
      }
      return true;
    }

    void do_port_unmapping()
    {
      std::string external_port_str = std::to_string(m_external_port);

      int r = UPNP_DeletePortMapping(m_urls.controlURL, m_data.first.servicetype, external_port_str.c_str(), "TCP", nullptr);
      if(r!=UPNPCOMMAND_SUCCESS)
      {
        LOG_PRINT_L1("DeletePortMapping with external_port_str= " << external_port_str <<
                                         ", failed with code=" << r << "(" << strupnperror(r) << ")");
      }else
      {
        LOG_PRINT_L0("[upnp] port unmapped successful (ext: " << external_port_str << ")");
      }
    }
  };
}

