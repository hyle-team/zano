// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "currency_protocol/currency_protocol_defs.h"
#include "currency_core/currency_basic.h"
#include "crypto/hash.h"
#include "net/rpc_method_name.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Seems to be obsolete. Consider removing due to stratum support.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


namespace mining
{
  struct height_info
  {
    uint64_t height;
    std::string block_id;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(height)
      KV_SERIALIZE(block_id)
    END_KV_SERIALIZE_MAP()
  };


  struct addendum
  {
    height_info hi;
    std::string prev_id;
    std::string addm;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(hi)
      KV_SERIALIZE(prev_id)
      KV_SERIALIZE(addm)
    END_KV_SERIALIZE_MAP()
  };

  struct job_details
  {
    std::string blob;
    std::string difficulty;
    std::string job_id;
    height_info prev_hi;
    std::list<addendum> addms;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(blob)
      KV_SERIALIZE(difficulty)
      KV_SERIALIZE(job_id)
      KV_SERIALIZE(prev_hi)
      KV_SERIALIZE(addms)      
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_LOGIN
  {
    RPC_METHOD_NAME("login");
    
    struct request
    {
      std::string login;
      std::string pass;
      std::string agent;
      height_info hi;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(login)
        KV_SERIALIZE(pass)
        KV_SERIALIZE(agent)
        KV_SERIALIZE(hi)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::string id;
      job_details job;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(id)
        KV_SERIALIZE(job)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GETJOB
  {
    RPC_METHOD_NAME("getjob");

    struct request
    {
      std::string id;
      height_info hi;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
        KV_SERIALIZE(hi)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
        std::string status;
        job_details jd;
        
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(status)
          KV_CHAIN_MAP(jd)
        END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_FULLSCRATCHPAD
  {
    RPC_METHOD_NAME("getfullscratchpad");

    struct request
    {
      std::string id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      height_info hi;
      std::string scratchpad_hex;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hi)
        KV_SERIALIZE(scratchpad_hex)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_SUBMITSHARE
  {
    RPC_METHOD_NAME("submit");

    struct request
    {
      std::string id;
      uint64_t nonce;
      std::string job_id;
      std::string result;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
        KV_SERIALIZE(nonce)
        KV_SERIALIZE(job_id)
        KV_SERIALIZE(result)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_STORE_SCRATCHPAD
  {
    RPC_METHOD_NAME("store_scratchpad");

    struct request
    {
      std::string local_file_path;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(local_file_path)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };



}

