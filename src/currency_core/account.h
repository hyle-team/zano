// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_core/currency_basic.h"
#include "crypto/crypto.h"
#include "serialization/keyvalue_serialization.h"

#define BRAINWALLET_DEFAULT_SEED_SIZE 32 
#define ACCOUNT_RESTORE_DATA_SIZE     BRAINWALLET_DEFAULT_SEED_SIZE    
#define BRAINWALLET_DEFAULT_WORDS_COUNT 25



#define KV_SERIALIZE_ADDRESS_AS_TEXT_N(varialble, val_name) \
  KV_SERIALIZE_CUSTOM_N(varialble, std::string, currency::transform_addr_to_str, currency::transform_str_to_addr, val_name)

#define KV_SERIALIZE_ADDRESS_AS_TEXT(varialble)  KV_SERIALIZE_ADDRESS_AS_TEXT_N(varialble, #varialble)


namespace currency
{



  struct account_keys
  {
    account_public_address m_account_address;
    crypto::secret_key   m_spend_secret_key;
    crypto::secret_key   m_view_secret_key;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_account_address)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_spend_secret_key)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_view_secret_key)
    END_KV_SERIALIZE_MAP()
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class account_base
  {
  public:
    account_base();
    void generate();
    const account_keys& get_keys() const;
    const account_public_address& get_public_address() const { return m_keys.m_account_address; };
    std::string get_public_address_str() const;
    std::string get_restore_data() const;
    std::string get_restore_braindata() const;

    bool restore_keys(const std::string& restore_data);
    bool restore_keys_from_braindata(const std::string& restore_data);

    uint64_t get_createtime() const { return m_creation_timestamp; }
    void set_createtime(uint64_t val) { m_creation_timestamp = val; }

    bool load(const std::string& file_path);
    bool store(const std::string& file_path);

    void make_account_watch_only();

    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int /*ver*/)
    {
      a & m_keys;
      a & m_creation_timestamp;
      a & m_seed;
    }

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_keys)
      KV_SERIALIZE(m_creation_timestamp)
      KV_SERIALIZE(m_seed)
    END_KV_SERIALIZE_MAP()

  private:
    void set_null();
    account_keys m_keys;
    uint64_t m_creation_timestamp;
    std::string m_seed;
  };


  std::string transform_addr_to_str(const account_public_address& addr);
  account_public_address transform_str_to_addr(const std::string& str);

  inline bool operator==(const account_keys& lhs, const account_keys& rhs)
  {
    return lhs.m_account_address  == rhs.m_account_address &&
           lhs.m_spend_secret_key == rhs.m_spend_secret_key &&
           lhs.m_view_secret_key  == rhs.m_view_secret_key;
  }
  inline bool operator!=(const account_keys& lhs, const account_keys& rhs)
  {
    return !operator==(lhs, rhs);
  }
}
