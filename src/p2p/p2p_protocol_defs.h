// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/uuid/uuid.hpp>
#include "serialization/keyvalue_serialization.h"
#include "misc_language.h"
#include "currency_core/currency_config.h"
#include "crypto/crypto.h"

namespace nodetool
{
  typedef boost::uuids::uuid uuid;
  typedef uint64_t peerid_type;
  typedef std::string blobdata;

#pragma pack (push, 1)
  
  struct net_address
  {
    uint32_t ip;
    uint32_t port;
  };

  struct peerlist_entry
  {
    net_address adr;
    peerid_type id;
    time_t last_seen;
  };

#define P2P_CONNECTION_ENTRY_VERSION_MAX_SIZE 50

  struct connection_entry
  {
    net_address adr;
    peerid_type id;
    bool is_income;
    uint64_t time_started;
    uint64_t last_recv;
    uint64_t last_send;
    char version[P2P_CONNECTION_ENTRY_VERSION_MAX_SIZE];
  };

#pragma pack(pop)

  inline
  bool operator < (const net_address& a, const net_address& b)
  {
    return  epee::misc_utils::is_less_as_pod(a, b);
  }

  inline
    bool operator == (const net_address& a, const net_address& b)
  {
    return  memcmp(&a, &b, sizeof(a)) == 0;
  }
  inline 
  std::string print_peerlist_to_string(const std::list<peerlist_entry>& pl)
  {
    time_t now_time = 0;
    time(&now_time);
    std::stringstream ss;
    ss << std::setfill ('0') << std::setw (8) << std::hex << std::noshowbase;
    BOOST_FOREACH(const peerlist_entry& pe, pl)
    {
      ss << pe.id << "\t" << epee::string_tools::get_ip_string_from_int32(pe.adr.ip) << ":" << boost::lexical_cast<std::string>(pe.adr.port) << " \tlast_seen: " << epee::misc_utils::get_time_interval_string(now_time - pe.last_seen) << std::endl;
    }
    return ss.str();
  }


  struct network_config
  {
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(connections_count)
      KV_SERIALIZE(handshake_interval)
      KV_SERIALIZE(packet_max_size)
      KV_SERIALIZE(config_id)
    END_KV_SERIALIZE_MAP()

    uint32_t connections_count;
    uint32_t connection_timeout;
    uint32_t ping_connection_timeout;
    uint32_t handshake_interval;
    uint32_t packet_max_size;
    uint32_t config_id;
    uint32_t send_peerlist_sz;
  };

  struct basic_node_data
  {
    uuid network_id;                   
    int64_t local_time;
    uint32_t my_port;
    peerid_type peer_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_VAL_POD_AS_BLOB(network_id)
      KV_SERIALIZE(peer_id)
      KV_SERIALIZE(local_time)
      KV_SERIALIZE(my_port)
    END_KV_SERIALIZE_MAP()
  };
  

#define P2P_COMMANDS_POOL_BASE 1000

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
#define  ALERT_TYPE_CALM            1
#define  ALERT_TYPE_URGENT          2
#define  ALERT_TYPE_CRITICAL        3

  /*
  Don't put any strings into maintainers message: if secret key will
  be stolen it can be used maliciously.
  */
  struct alert_condition
  {
    uint32_t if_build_less_then; //apply alert if build less then
    uint8_t  alert_mode;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_N(if_build_less_then, "build")
      KV_SERIALIZE_N(alert_mode, "mode")
    END_KV_SERIALIZE_MAP()    
  };

  struct maintainers_info
  {
    uint64_t timestamp;
    /*actual version*/
    uint8_t  ver_major;
    uint8_t  ver_minor;
    uint8_t  ver_revision;
    uint32_t build_no;
    /*conditions for alerting*/
    std::list<alert_condition> conditions;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_N(timestamp, "ts")
      KV_SERIALIZE_N(ver_major, "maj")
      KV_SERIALIZE_N(ver_minor, "min")
      KV_SERIALIZE_N(ver_revision, "rev")
      KV_SERIALIZE_N(build_no, "build")
      KV_SERIALIZE_N(conditions, "cs")
    END_KV_SERIALIZE_MAP()
  };

  struct maintainers_entry
  {
    blobdata maintainers_info_buff;
    crypto::signature sign;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(maintainers_info_buff)
      KV_SERIALIZE_VAL_POD_AS_BLOB(sign)
    END_KV_SERIALIZE_MAP()
  };

  struct maintainers_info_external
  {
    uint8_t  ver_major;
    uint8_t  ver_minor;
    uint8_t  ver_revision;
    uint32_t build_no;
    uint8_t  mode;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(ver_major)
      KV_SERIALIZE(ver_minor)
      KV_SERIALIZE(ver_revision)
      KV_SERIALIZE(build_no)
      KV_SERIALIZE(mode)
    END_KV_SERIALIZE_MAP()
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_playload_type>
	struct COMMAND_HANDSHAKE_T
	{
		const static int ID = P2P_COMMANDS_POOL_BASE + 1;

    struct request
    {
      basic_node_data node_data;
      t_playload_type payload_data;
      maintainers_entry maintrs_entry;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(node_data)
        KV_SERIALIZE(payload_data)
        KV_SERIALIZE(maintrs_entry)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      basic_node_data node_data;
      t_playload_type payload_data;
      std::list<peerlist_entry> local_peerlist;
      maintainers_entry maintrs_entry;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(node_data)
        KV_SERIALIZE(payload_data)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(local_peerlist)
        KV_SERIALIZE(maintrs_entry)
      END_KV_SERIALIZE_MAP()
    };
	};


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_playload_type>
  struct COMMAND_TIMED_SYNC_T
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 2;

    struct request
    {
      t_playload_type payload_data;
      maintainers_entry maintrs_entry;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payload_data)
        KV_SERIALIZE(maintrs_entry)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      int64_t local_time;
      t_playload_type payload_data;
      std::list<peerlist_entry> local_peerlist; 
      maintainers_entry maintrs_entry;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(local_time)
        KV_SERIALIZE(payload_data)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(local_peerlist)
        KV_SERIALIZE(maintrs_entry)
      END_KV_SERIALIZE_MAP()
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  struct COMMAND_PING
  {
    /*
      Used to make "callback" connection, to be sure that opponent node 
      have accessible connection point. Only other nodes can add peer to peerlist,
      and ONLY in case when peer has accepted connection and answered to ping.
    */
    const static int ID = P2P_COMMANDS_POOL_BASE + 3;

#define PING_OK_RESPONSE_STATUS_TEXT "OK"

    struct request
    {
      /*actually we don't need to send any real data*/

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      peerid_type peer_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(peer_id)
      END_KV_SERIALIZE_MAP()    
    };
  };

  
#ifdef ALLOW_DEBUG_COMMANDS
  //These commands are considered as insecure, and made in debug purposes for a limited lifetime. 
  //Anyone who feel unsafe with this commands can disable the ALLOW_GET_STAT_COMMAND macro.

  struct proof_of_trust
  {
    peerid_type peer_id;
    int64_t    time;
    crypto::signature sign;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(peer_id)
      KV_SERIALIZE(time)        
      KV_SERIALIZE_VAL_POD_AS_BLOB(sign)  
    END_KV_SERIALIZE_MAP()    
  };


  template<class payload_stat_info>
  struct COMMAND_REQUEST_STAT_INFO_T
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 4;

    struct request
    {
      typename payload_stat_info::params pr;
      proof_of_trust tr;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(pr)
        KV_SERIALIZE(tr)
      END_KV_SERIALIZE_MAP()    
    };
    
    struct response
    {
      std::string version;
      uint64_t connections_count;
      uint64_t incoming_connections_count;
      uint64_t current_log_size;
      payload_stat_info payload_info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(version)
        KV_SERIALIZE(connections_count)
        KV_SERIALIZE(incoming_connections_count)
        KV_SERIALIZE(current_log_size)
        KV_SERIALIZE(payload_info)
      END_KV_SERIALIZE_MAP()    
    };
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_NETWORK_STATE
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 5;

    struct request
    {
      proof_of_trust tr;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tr)
      END_KV_SERIALIZE_MAP()    
    };

    struct response
    {
      std::list<peerlist_entry> local_peerlist_white; 
      std::list<peerlist_entry> local_peerlist_gray; 
      std::list<connection_entry> connections_list; 
      peerid_type my_id;
      uint64_t    local_time;
      uint64_t    up_time;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(local_peerlist_white)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(local_peerlist_gray)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(connections_list)
        KV_SERIALIZE(my_id)
        KV_SERIALIZE(local_time)
        KV_SERIALIZE(up_time)
      END_KV_SERIALIZE_MAP()    
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_PEER_ID
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 6;

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()    
    };

    struct response
    {
      peerid_type my_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(my_id)
      END_KV_SERIALIZE_MAP()    
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_LOG
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 7;

    struct request
    {
      proof_of_trust  tr;
      uint64_t        log_chunk_offset;
      uint64_t        log_chunk_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tr)
        KV_SERIALIZE(log_chunk_offset)
        KV_SERIALIZE(log_chunk_size)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      int64_t     current_log_level;
      uint64_t    current_log_size;
      std::string error;
      std::string log_chunk;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(current_log_level)
        KV_SERIALIZE(current_log_size)
        KV_SERIALIZE(error)
        KV_SERIALIZE(log_chunk)
      END_KV_SERIALIZE_MAP()
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_SET_LOG_LEVEL
  {
    const static int ID = P2P_COMMANDS_POOL_BASE + 8;

    struct request
    {
      proof_of_trust  tr;
      int64_t         new_log_level;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tr)
        KV_SERIALIZE(new_log_level)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      int64_t     old_log_level;
      int64_t     current_log_level;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(old_log_level)
        KV_SERIALIZE(current_log_level)
      END_KV_SERIALIZE_MAP()
    };
  };

#endif // #ifdef ALLOW_DEBUG_COMMANDS
    
}



