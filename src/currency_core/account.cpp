// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <fstream>

#include "include_base_utils.h"
#include "account.h"
#include "warnings.h"
#include "crypto/crypto.h"
#include "currency_core/currency_format_utils.h"
#include "common/mnemonic-encoding.h"

using namespace std;

DISABLE_VS_WARNINGS(4244 4345)



namespace currency
{



  //-----------------------------------------------------------------
  account_base::account_base()
  {
    set_null();
  }
  //-----------------------------------------------------------------
  void account_base::set_null()
  {
    // fill sensitive data with random bytes
    crypto::generate_random_bytes(sizeof m_keys.m_spend_secret_key, &m_keys.m_spend_secret_key);
    crypto::generate_random_bytes(sizeof m_keys.m_view_secret_key, &m_keys.m_view_secret_key);
    crypto::generate_random_bytes(m_seed.size(), &m_seed[0]);
    
    // clear
    m_keys = account_keys();
    m_creation_timestamp = 0;
    m_seed.clear();
  }
  //-----------------------------------------------------------------
  void account_base::generate()
  {   
    generate_brain_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key, m_seed, BRAINWALLET_DEFAULT_SEED_SIZE);
    dependent_key(m_keys.m_spend_secret_key, m_keys.m_view_secret_key);
    if (!crypto::secret_key_to_public_key(m_keys.m_view_secret_key, m_keys.m_account_address.m_view_public_key))
      throw std::runtime_error("Failed to create public view key");


    m_creation_timestamp = time(NULL);
  }
  //-----------------------------------------------------------------
  const account_keys& account_base::get_keys() const
  {
    return m_keys;
  }
  //-----------------------------------------------------------------
  std::string account_base::get_restore_data() const
  {
    return m_seed;
  }
  //-----------------------------------------------------------------

  std::string account_base::get_restore_braindata() const 
  {
    std::string restore_buff = get_restore_data();
    if (restore_buff.empty())
      return "";
    std::vector<unsigned char> v;
    v.assign((unsigned char*)restore_buff.data(), (unsigned char*)restore_buff.data() + restore_buff.size());
    std::string seed_brain_data = tools::mnemonic_encoding::binary2text(v);
    std::string timestamp_word = currency::get_word_from_timstamp(m_creation_timestamp);
    seed_brain_data = seed_brain_data + timestamp_word;
    return seed_brain_data;
  }
  //-----------------------------------------------------------------
  bool account_base::restore_keys(const std::string& restore_data)
  {
    //CHECK_AND_ASSERT_MES(restore_data.size() == ACCOUNT_RESTORE_DATA_SIZE, false, "wrong restore data size");
    if (restore_data.size() == BRAINWALLET_DEFAULT_SEED_SIZE)
    {
      crypto::keys_from_default((unsigned char*)restore_data.data(), m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key, BRAINWALLET_DEFAULT_SEED_SIZE);
    }
    else 
    {
      LOG_ERROR("wrong restore data size=" << restore_data.size());
      return false;
    }
    m_seed = restore_data;
    crypto::dependent_key(m_keys.m_spend_secret_key, m_keys.m_view_secret_key);
    bool r = crypto::secret_key_to_public_key(m_keys.m_view_secret_key, m_keys.m_account_address.m_view_public_key);
    CHECK_AND_ASSERT_MES(r, false, "failed to secret_key_to_public_key for view key");
    set_createtime(0);
    return true;
  }
  //-----------------------------------------------------------------
  bool account_base::restore_keys_from_braindata(const std::string& restore_data_)
  {
    //cut the last timestamp word from restore_dats
    std::list<std::string> words;
    boost::split(words, restore_data_, boost::is_space());
    CHECK_AND_ASSERT_MES(words.size() == BRAINWALLET_DEFAULT_WORDS_COUNT, false, "Words count missmatch: " << words.size());

    std::string timestamp_word = words.back();
    words.erase(--words.end());

    std::string restore_data_local = boost::algorithm::join(words, " ");
    
    std::vector<unsigned char> bin = tools::mnemonic_encoding::text2binary(restore_data_local);
    if (!bin.size())
      return false;

    std::string restore_buff((const char*)&bin[0], bin.size());
    bool r = restore_keys(restore_buff);
    CHECK_AND_ASSERT_MES(r, false, "restore_keys failed");
    m_creation_timestamp = get_timstamp_from_word(timestamp_word);
    return true;
  }
  //-----------------------------------------------------------------
  std::string account_base::get_public_address_str() const
  {
    //TODO: change this code into base 58
    return get_account_address_as_str(m_keys.m_account_address);
  }
  //-----------------------------------------------------------------
  void account_base::make_account_watch_only()
  {
    // keep only:
    // timestamp
    // view pub & spend pub (public address)
    // view sec
    
    // store to local tmp
    uint64_t local_ts = m_creation_timestamp;
    account_public_address local_addr = m_keys.m_account_address;
    crypto::secret_key local_view_sec = m_keys.m_view_secret_key;

    // clear
    set_null();

    // restore
    m_creation_timestamp = local_ts;
    m_keys.m_account_address = local_addr;
    m_keys.m_view_secret_key = local_view_sec;
  }
  //-----------------------------------------------------------------
  std::string transform_addr_to_str(const account_public_address& addr)
  {
    return get_account_address_as_str(addr);
  }
  //-----------------------------------------------------------------
  account_public_address transform_str_to_addr(const std::string& str)
  {
    account_public_address ad = AUTO_VAL_INIT(ad);
    get_account_address_from_str(ad, str);
    return ad;
  }
}