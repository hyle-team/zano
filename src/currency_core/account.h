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
#define SEED_PHRASE_V1_WORDS_COUNT 25
#define SEED_PHRASE_V2_WORDS_COUNT 26


#ifndef FORCE_HEADER_ONLY 
  #define KV_SERIALIZE_ADDRESS_AS_TEXT_N(varialble, val_name) \
    KV_SERIALIZE_CUSTOM_N(varialble, std::string, currency::transform_addr_to_str, currency::transform_str_to_addr, val_name)

  #define KV_SERIALIZE_ADDRESS_AS_TEXT(varialble)  KV_SERIALIZE_ADDRESS_AS_TEXT_N(varialble, #varialble)
#else
  #define KV_SERIALIZE_ADDRESS_AS_TEXT_N(varialble, val_name)     
  #define KV_SERIALIZE_ADDRESS_AS_TEXT(varialble) 
#endif
namespace currency
{



  struct account_keys
  {
    account_public_address account_address;
    crypto::secret_key   spend_secret_key;
    crypto::secret_key   view_secret_key;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_N(account_address, "m_account_address")
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(spend_secret_key, "m_spend_secret_key")
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(view_secret_key, "m_view_secret_key")
    END_KV_SERIALIZE_MAP()
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class account_base
  {
  public:
    account_base();
    void generate(bool auditable = false);
    const account_keys& get_keys() const;
    const account_public_address& get_public_address() const { return m_keys.account_address; };
    std::string get_public_address_str() const;
    
    std::string get_seed_phrase(const std::string& seed_password) const;
    std::string get_seed_phrase(const std::string& password, const std::vector<unsigned char>& keys_seed_binary) const;
    std::string get_tracking_seed() const;
    bool restore_from_seed_phrase(const std::string& seed_phrase, const std::string& seed_password);
    bool restore_from_tracking_seed(const std::string& tracking_seed);

    uint64_t get_createtime() const { return m_creation_timestamp; }
    void set_createtime(uint64_t val) { m_creation_timestamp = val; }

    bool load(const std::string& file_path);
    bool store(const std::string& file_path);

    void make_account_watch_only();
    bool is_watch_only() const { return m_keys.spend_secret_key == currency::null_skey; }
    bool is_auditable() const { return m_keys.account_address.is_auditable(); }

    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int /*ver*/)
    {
      a & m_keys;
      a & m_creation_timestamp;
      a & m_keys_seed_binary;
    }

    static std::string vector_of_chars_to_string(const std::vector<unsigned char>& v) { return std::string(v.begin(), v.end()); }
    static std::vector<unsigned char> string_to_vector_of_chars(const std::string& v) { return std::vector<unsigned char>(v.begin(), v.end()); }
    static bool is_seed_password_protected(const std::string& seed_phrase, bool& is_password_protected);
    static bool is_seed_tracking(const std::string& seed_phrase);
    static void crypt_with_pass(const void* scr_data, std::size_t src_length, void* dst_data, const std::string& password);


    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_keys)
      KV_SERIALIZE(m_creation_timestamp)
      KV_SERIALIZE_CUSTOM_N(m_keys_seed_binary, std::string, vector_of_chars_to_string, string_to_vector_of_chars, "m_seed")
    END_KV_SERIALIZE_MAP()

  private:
    void set_null();
    bool restore_keys(const std::vector<unsigned char>& keys_seed_binary);

    account_keys m_keys;
    uint64_t m_creation_timestamp;

    std::vector<unsigned char> m_keys_seed_binary;
  };

  const static account_keys null_acc_keys = AUTO_VAL_INIT(null_acc_keys);

  std::string transform_addr_to_str(const account_public_address& addr);
  account_public_address transform_str_to_addr(const std::string& str);

  inline bool operator==(const account_keys& lhs, const account_keys& rhs)
  {
    return lhs.account_address  == rhs.account_address &&
           lhs.spend_secret_key == rhs.spend_secret_key &&
           lhs.view_secret_key  == rhs.view_secret_key;
  }
  inline bool operator!=(const account_keys& lhs, const account_keys& rhs)
  {
    return !operator==(lhs, rhs);
  }
}
