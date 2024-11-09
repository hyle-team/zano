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

//DISABLE_VS_WARNINGS(4244 4345)

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
    crypto::generate_random_bytes(sizeof m_keys.spend_secret_key, &m_keys.spend_secret_key);
    crypto::generate_random_bytes(sizeof m_keys.view_secret_key, &m_keys.view_secret_key);
    if (m_keys_seed_binary.size())
      crypto::generate_random_bytes(m_keys_seed_binary.size(), &m_keys_seed_binary[0]);
    
    // clear
    m_keys = account_keys();
    m_creation_timestamp = 0;
    m_keys_seed_binary.clear();
  }
  //-----------------------------------------------------------------
  void account_base::generate(bool auditable /* = false */)
  {   
    if (auditable)
      m_keys.account_address.flags = ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE;

    crypto::generate_seed_keys(m_keys.account_address.spend_public_key, m_keys.spend_secret_key, m_keys_seed_binary, BRAINWALLET_DEFAULT_SEED_SIZE);
    crypto::dependent_key(m_keys.spend_secret_key, m_keys.view_secret_key);
    if (!crypto::secret_key_to_public_key(m_keys.view_secret_key, m_keys.account_address.view_public_key))
      throw std::runtime_error("Failed to create public view key");


    m_creation_timestamp = time(NULL);
  }
  //-----------------------------------------------------------------
  const account_keys& account_base::get_keys() const
  {
    return m_keys;
  }
  //-----------------------------------------------------------------
  void account_base::crypt_with_pass(const void* scr_data, std::size_t src_length, void* dst_data, const std::string& password)
  {
    crypto::chacha8_key key = AUTO_VAL_INIT(key);
    crypto::generate_chacha8_key(password, key);
    crypto::hash pass_hash = crypto::cn_fast_hash(password.data(), password.size());
    crypto::chacha8_iv  iv = AUTO_VAL_INIT(iv);
    CHECK_AND_ASSERT_THROW_MES(sizeof(pass_hash) >= sizeof(iv), "Invalid configuration: hash size is less than keys_file_data.iv");
    iv = *((crypto::chacha8_iv*)&pass_hash);
    crypto::chacha8(scr_data, src_length, key, iv, (char*)dst_data);
  }
  //-----------------------------------------------------------------
  std::string account_base::get_seed_phrase(const std::string& password) const
  {
    if (m_keys_seed_binary.empty())
      return "";
    return get_seed_phrase(password, m_keys_seed_binary);
  }
  //-----------------------------------------------------------------
  std::string account_base::get_seed_phrase(const std::string& password, const std::vector<unsigned char>& keys_seed_binary) const
  {
    if (keys_seed_binary.empty())
      return "";

    std::vector<unsigned char> processed_seed_binary = keys_seed_binary;
    if (!password.empty())
    {
      CHECK_AND_ASSERT_THROW_MES(currency::validate_password(password), "seed phrase password contains invalid characters, seed phrase cannot be created with such a password");
      //encrypt seed phrase binary data
      crypt_with_pass(&keys_seed_binary[0], keys_seed_binary.size(), &processed_seed_binary[0], password);      
    }

    std::string keys_seed_text = tools::mnemonic_encoding::binary2text(processed_seed_binary);
    std::string timestamp_word = currency::get_word_from_timestamp(m_creation_timestamp, !password.empty());

    // floor creation time to WALLET_BRAIN_DATE_QUANTUM to make checksum calculation stable
    bool self_check_is_password_used = false;
    uint64_t creation_timestamp_rounded = get_timestamp_from_word(timestamp_word, self_check_is_password_used);
    CHECK_AND_ASSERT_THROW_MES(self_check_is_password_used == !password.empty(), "Account seed phrase internal error: password flag encoded wrong");

    constexpr uint16_t checksum_max = tools::mnemonic_encoding::NUMWORDS >> 1; // maximum value of checksum
    std::string binary_for_check_sum((const char*)&keys_seed_binary[0], keys_seed_binary.size());
    binary_for_check_sum.append(password);
    crypto::hash h = crypto::cn_fast_hash(binary_for_check_sum.data(), binary_for_check_sum.size());
    *reinterpret_cast<uint64_t*>(&h) = creation_timestamp_rounded;
    h = crypto::cn_fast_hash(&h, sizeof h);
    uint64_t h_64 = *reinterpret_cast<uint64_t*>(&h);
    uint16_t checksum = h_64 % (checksum_max + 1);
    
    if (checksum == checksum_max) // workaround for incorrect checksum calculation (trying to keep the whole scheme untouched) -- sowle
      checksum = 0;
    
    uint8_t auditable_flag = 0;
    if (m_keys.account_address.flags & ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE)
      auditable_flag = 1;

    uint32_t auditable_flag_and_checksum = (auditable_flag & 1) | (checksum << 1);
    std::string auditable_flag_and_checksum_word = tools::mnemonic_encoding::word_by_num(auditable_flag_and_checksum);

    return keys_seed_text + " " + timestamp_word + " " + auditable_flag_and_checksum_word;
  }
  //-----------------------------------------------------------------
  std::string account_base::get_tracking_seed() const
  {
    return get_public_address_str() + ":" +
      epee::string_tools::pod_to_hex(m_keys.view_secret_key) +
      (m_creation_timestamp ? ":" : "") + (m_creation_timestamp ? epee::string_tools::num_to_string_fast(m_creation_timestamp) : "");
  }
  //-----------------------------------------------------------------
  bool account_base::restore_keys(const std::vector<unsigned char>& keys_seed_binary)
  {
    CHECK_AND_ASSERT_MES(keys_seed_binary.size() == BRAINWALLET_DEFAULT_SEED_SIZE, false, "wrong restore data size: " << keys_seed_binary.size());
    crypto::keys_from_default(keys_seed_binary.data(), m_keys.account_address.spend_public_key, m_keys.spend_secret_key, keys_seed_binary.size());
    crypto::dependent_key(m_keys.spend_secret_key, m_keys.view_secret_key);
    bool r = crypto::secret_key_to_public_key(m_keys.view_secret_key, m_keys.account_address.view_public_key);
    CHECK_AND_ASSERT_MES(r, false, "failed to secret_key_to_public_key for view key");
    return true;
  }
  //-----------------------------------------------------------------
  bool account_base::restore_from_seed_phrase(const std::string& seed_phrase_, const std::string& seed_password)
  {
    //cut the last timestamp word from restore_dats
    std::list<std::string> words;
    std::string seed_phrase = epee::string_tools::trim(seed_phrase_);
    boost::split(words, seed_phrase, boost::is_space(), boost::token_compress_on);
    
    std::string keys_seed_text, timestamp_word, auditable_flag_and_checksum_word;
    if (words.size() == SEED_PHRASE_V1_WORDS_COUNT)
    {
      // 24 seed words + one timestamp word = 25 total
      timestamp_word = words.back();
      words.erase(--words.end());
      keys_seed_text = boost::algorithm::join(words, " ");
    }
    else if (words.size() == SEED_PHRASE_V2_WORDS_COUNT)
    {
      // 24 seed words + one timestamp word + one flags & checksum = 26 total
      auditable_flag_and_checksum_word = words.back();
      words.erase(--words.end());
      timestamp_word = words.back();
      words.erase(--words.end());
      keys_seed_text = boost::algorithm::join(words, " ");
    }
    else
    {
      LOG_ERROR("Invalid seed words count: " << words.size());
      return false;
    }
    
    uint64_t auditable_flag_and_checksum = UINT64_MAX;
    if (!auditable_flag_and_checksum_word.empty())
    {
      try {
        auditable_flag_and_checksum = tools::mnemonic_encoding::num_by_word(auditable_flag_and_checksum_word);
      }
      catch (...)
      {
        return false;
      }
      
    }
      

    std::vector<unsigned char> keys_seed_binary = tools::mnemonic_encoding::text2binary(keys_seed_text);
    std::vector<unsigned char> keys_seed_processed_binary = keys_seed_binary;


    bool has_password = false;
    try {
      m_creation_timestamp = get_timestamp_from_word(timestamp_word, has_password);
    }
    catch (...)
    {
      return false;
    }
    //double check is password setting from timestamp word match with passed parameters
    CHECK_AND_ASSERT_MES(has_password != seed_password.empty(), false, "Seed phrase password wrong interpretation");
    if (has_password)
    {
      CHECK_AND_ASSERT_MES(!seed_password.empty(), false, "Seed phrase password wrong interpretation: internal error");
      crypt_with_pass(&keys_seed_binary[0], keys_seed_binary.size(), &keys_seed_processed_binary[0], seed_password);
    }

    CHECK_AND_ASSERT_MES(keys_seed_processed_binary.size(), false, "text2binary failed to convert the given text"); // don't prints event incorrect seed into the log for security

    bool auditable_flag = false;

    // check the checksum if checksum word provided
    if (auditable_flag_and_checksum != UINT64_MAX)
    {
      auditable_flag = (auditable_flag_and_checksum & 1) != 0; // auditable flag is the lower 1 bit
      uint16_t checksum = static_cast<uint16_t>(auditable_flag_and_checksum >> 1); // checksum -- everything else
      constexpr uint16_t checksum_max = tools::mnemonic_encoding::NUMWORDS >> 1; // maximum value of checksum
      std::string binary_for_check_sum((const char*)&keys_seed_processed_binary[0], keys_seed_processed_binary.size());
      binary_for_check_sum.append(seed_password);
      crypto::hash h = crypto::cn_fast_hash(binary_for_check_sum.data(), binary_for_check_sum.size());
      *reinterpret_cast<uint64_t*>(&h) = m_creation_timestamp;
      h = crypto::cn_fast_hash(&h, sizeof h);
      uint64_t h_64 = *reinterpret_cast<uint64_t*>(&h);
      uint16_t checksum_calculated = h_64 % (checksum_max + 1);
      
      if (checksum_calculated == checksum_max) // workaround for incorrect checksum calculation (trying to keep the whole scheme untouched) -- sowle
        checksum_calculated = 0;

      if (checksum != checksum_calculated)
      {
        LOG_PRINT_L0("seed phase has invalid checksum: " << checksum_calculated << ", while " << checksum << " is expected, check your words");
        return false;
      }      
    }

    bool r = restore_keys(keys_seed_processed_binary);
    CHECK_AND_ASSERT_MES(r, false, "restore_keys failed");

    m_keys_seed_binary = keys_seed_processed_binary;

    if (auditable_flag)
      m_keys.account_address.flags |= ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE;
    else
      m_keys.account_address.flags &= ~ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE;

    return true;
  }
  //-----------------------------------------------------------------
  bool account_base::is_seed_tracking(const std::string& seed_phrase)
  {
    return seed_phrase.find(':') != std::string::npos;
  }
  //-----------------------------------------------------------------
  bool account_base::is_seed_password_protected(const std::string& seed_phrase_, bool& is_password_protected)
  {
    //cut the last timestamp word from restore_dats
    std::list<std::string> words;

    std::string seed_phrase = epee::string_tools::trim(seed_phrase_);
    boost::split(words, seed_phrase, boost::is_space(), boost::token_compress_on);

    //let's validate each word 
    for (const auto& w: words)
    {
      if (!tools::mnemonic_encoding::valid_word(w))
        return false;
    }

    std::string timestamp_word;
    if (words.size() == SEED_PHRASE_V1_WORDS_COUNT)
    {
      // 24 seed words + one timestamp word = 25 total
      timestamp_word = words.back();
    }
    else if (words.size() == SEED_PHRASE_V2_WORDS_COUNT)
    {
      // 24 seed words + one timestamp word + one flags & checksum = 26 total
      words.erase(--words.end());
      timestamp_word = words.back();
    }
    else
    {
      return false;
    }

    get_timestamp_from_word(timestamp_word, is_password_protected);

    return true;
  }
  //-----------------------------------------------------------------
  bool account_base::restore_from_tracking_seed(const std::string& tracking_seed)
  {
    set_null();
    bool r = parse_tracking_seed(tracking_seed, m_keys.account_address, m_keys.view_secret_key, m_creation_timestamp);
    return r;
  }
  //-----------------------------------------------------------------
  std::string account_base::get_public_address_str() const
  {
    //TODO: change this code into base 58
    return get_account_address_as_str(m_keys.account_address);
  }
  //-----------------------------------------------------------------
  void account_base::make_account_watch_only()
  {
    // keep only:
    // timestamp
    // view pub & spend pub + flags (public address)
    // view sec
    
    // store to local tmp
    uint64_t local_ts = m_creation_timestamp;
    account_public_address local_addr = m_keys.account_address;
    crypto::secret_key local_view_sec = m_keys.view_secret_key;

    // clear
    set_null();

    // restore
    m_creation_timestamp = local_ts;
    m_keys.account_address = local_addr;
    m_keys.view_secret_key = local_view_sec;
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
    if (!get_account_address_from_str(ad, str))
    {
      CHECK_AND_ASSERT_THROW_MES(false, "cannot parse address from string: " << str);
    }
    return ad;
  }
}