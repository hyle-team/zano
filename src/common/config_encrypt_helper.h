// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

#include "crypto/chacha.h"
#include "crypto/crypto.h"
#include "crypto/wallet_kdf.h"
#include "currency_core/crypto_config.h"
#include "wallet/view_iface.h"

namespace tools
{
  namespace details
  {
    static const uint64_t APP_DATA_FILE_BINARY_SIGNATURE_V2 = 0x1010110201101011ULL;
    static const uint16_t APP_DATA_FILE_BINARY_VERSION_V2 = 2;
    static const size_t APP_DATA_FILE_BINARY_MAC_SIZE_V2 = 16;
  }

#pragma pack(push, 1)
  struct app_data_file_binary_header
  {
    uint64_t m_signature;
    uint64_t m_cb_body;
  };

  struct app_data_file_binary_header_v2
  {
    uint64_t          m_signature;
    uint16_t          m_version;
    uint16_t          m_header_size;
    uint8_t           m_kdf_algo;
    uint8_t           m_kdf_N_log2;
    uint8_t           m_kdf_phase2_log2_reduction;
    uint8_t           m_reserved;
    uint64_t          m_payload_signature;
    crypto::chacha_iv m_iv;
    uint8_t           m_payload_mac[details::APP_DATA_FILE_BINARY_MAC_SIZE_V2];
    uint8_t           m_kdf_salt[WALLET_KDF_SALT_SIZE];
  };
#pragma pack (pop)

  inline bool is_v2_encrypted_file(const std::string& app_data_buff)
  {
    if (app_data_buff.size() < sizeof(app_data_file_binary_header_v2))
      return false;

    const app_data_file_binary_header_v2* phdr = reinterpret_cast<const app_data_file_binary_header_v2*>(app_data_buff.data());
    return phdr->m_signature == details::APP_DATA_FILE_BINARY_SIGNATURE_V2;
  }

  inline bool constant_time_equal(const uint8_t* lhs, const uint8_t* rhs, size_t size)
  {
    uint8_t diff = 0;
    for (size_t i = 0; i < size; ++i)
      diff |= lhs[i] ^ rhs[i];
    return diff == 0;
  }

  inline std::string validate_app_data_header_v2(const app_data_file_binary_header_v2& hdr)
  {
    if (hdr.m_version != details::APP_DATA_FILE_BINARY_VERSION_V2 || hdr.m_header_size != sizeof(app_data_file_binary_header_v2))
      return API_RETURN_CODE_INVALID_FILE;
    if (hdr.m_kdf_algo != WALLET_KDF_ALGO_ROMIX_KECCAK)
      return API_RETURN_CODE_INVALID_FILE;
    if (hdr.m_kdf_N_log2 < 10 || hdr.m_kdf_N_log2 > WALLET_KDF_ROMIX_N_LOG2)
      return API_RETURN_CODE_INVALID_FILE;
    if (hdr.m_kdf_phase2_log2_reduction >= hdr.m_kdf_N_log2)
      return API_RETURN_CODE_INVALID_FILE;
    if (hdr.m_reserved != 0)
      return API_RETURN_CODE_INVALID_FILE;

    return API_RETURN_CODE_OK;
  }

  inline std::string derive_app_data_key_v2(const std::string& key, const app_data_file_binary_header_v2& hdr, crypto::chacha_key& chacha_key)
  {
    std::string rc = validate_app_data_header_v2(hdr);
    if (rc != API_RETURN_CODE_OK)
      return rc;

    uint8_t stretched[32];
    crypto::derive_key_romix_keccak(CRYPTO_HDS_APP_CONFIG_KDF_ROMIX, key.data(), key.size(),
      hdr.m_kdf_salt, sizeof(hdr.m_kdf_salt), hdr.m_kdf_N_log2, hdr.m_kdf_phase2_log2_reduction, stretched);
    crypto::chacha_generate_key(CRYPTO_HDS_CHACHA_APP_CONFIG, stretched, sizeof(stretched), 0, chacha_key);
    crypto::wipe(stretched, sizeof(stretched));

    return API_RETURN_CODE_OK;
  }

  inline void calculate_app_data_mac_v2(const app_data_file_binary_header_v2& hdr, const crypto::chacha_key& chacha_key,
    const char* encrypted_payload_data, size_t encrypted_payload_size, uint8_t (&mac)[details::APP_DATA_FILE_BINARY_MAC_SIZE_V2])
  {
    app_data_file_binary_header_v2 hdr_for_mac = hdr;
    std::memset(hdr_for_mac.m_payload_mac, 0, sizeof(hdr_for_mac.m_payload_mac));

    std::string buff;
    buff.reserve(32 + sizeof(hdr_for_mac) + encrypted_payload_size + sizeof(chacha_key.data));
    buff.append(CRYPTO_HDS_APP_CONFIG_MAC, 32);
    buff.append(reinterpret_cast<const char*>(&hdr_for_mac), sizeof(hdr_for_mac));
    if (encrypted_payload_size)
      buff.append(encrypted_payload_data, encrypted_payload_size);
    buff.append(reinterpret_cast<const char*>(chacha_key.data), sizeof(chacha_key.data));

    crypto::hash h = crypto::cn_fast_hash(buff.data(), buff.size());
    std::memcpy(mac, &h, sizeof(mac));
    crypto::wipe(&h, sizeof(h));
    crypto::wipe(&buff[0], buff.size());
  }

  inline std::string load_encrypted_file_v2(const std::string& app_data_buff, const std::string& key, std::string& body, uint64_t signature)
  {
    if (app_data_buff.size() < sizeof(app_data_file_binary_header_v2) + sizeof(app_data_file_binary_header))
      return API_RETURN_CODE_INVALID_FILE;

    const app_data_file_binary_header_v2* phdr = reinterpret_cast<const app_data_file_binary_header_v2*>(app_data_buff.data());
    if (phdr->m_payload_signature != signature)
      return API_RETURN_CODE_INVALID_FILE;

    crypto::chacha_key chacha_key;
    std::string rc = derive_app_data_key_v2(key, *phdr, chacha_key);
    if (rc != API_RETURN_CODE_OK)
      return rc;

    const size_t encrypted_payload_offset = sizeof(app_data_file_binary_header_v2);
    const size_t encrypted_payload_size = app_data_buff.size() - encrypted_payload_offset;

    uint8_t expected_mac[details::APP_DATA_FILE_BINARY_MAC_SIZE_V2];
    calculate_app_data_mac_v2(*phdr, chacha_key, app_data_buff.data() + encrypted_payload_offset, encrypted_payload_size, expected_mac);
    if (!constant_time_equal(expected_mac, phdr->m_payload_mac, sizeof(expected_mac)))
    {
      crypto::wipe(expected_mac, sizeof(expected_mac));
      return API_RETURN_CODE_WRONG_PASSWORD;
    }
    crypto::wipe(expected_mac, sizeof(expected_mac));

    std::string payload(encrypted_payload_size, '\0');
    crypto::chacha20(app_data_buff.data() + encrypted_payload_offset, encrypted_payload_size, chacha_key, phdr->m_iv, &payload[0]);

    const app_data_file_binary_header* payload_hdr = reinterpret_cast<const app_data_file_binary_header*>(payload.data());
    if (payload_hdr->m_signature != signature)
    {
      crypto::wipe(&payload[0], payload.size());
      return API_RETURN_CODE_WRONG_PASSWORD;
    }

    if (payload_hdr->m_cb_body > std::numeric_limits<size_t>::max() - sizeof(app_data_file_binary_header))
    {
      crypto::wipe(&payload[0], payload.size());
      return API_RETURN_CODE_INVALID_FILE;
    }

    const size_t expected_body_size = static_cast<size_t>(payload_hdr->m_cb_body);
    if (payload.size() != sizeof(app_data_file_binary_header) + expected_body_size)
    {
      crypto::wipe(&payload[0], payload.size());
      return API_RETURN_CODE_INVALID_FILE;
    }

    body.assign(payload.data() + sizeof(app_data_file_binary_header), expected_body_size);
    crypto::wipe(&payload[0], payload.size());
    return API_RETURN_CODE_OK;
  }

  inline std::string load_encrypted_file_legacy(const std::string& app_data_buff_src, const std::string& key, std::string& body, uint64_t signature)
  {
    if (app_data_buff_src.size() < sizeof(app_data_file_binary_header))
      return API_RETURN_CODE_INVALID_FILE;

    std::string app_data_buff = app_data_buff_src;
    crypto::chacha_crypt_legacy(app_data_buff, key);

    const app_data_file_binary_header* phdr = reinterpret_cast<const app_data_file_binary_header*>(app_data_buff.data());
    if (phdr->m_signature != signature)
    {
      crypto::wipe(&app_data_buff[0], app_data_buff.size());
      return API_RETURN_CODE_WRONG_PASSWORD;
    }

    body.assign(app_data_buff.data() + sizeof(app_data_file_binary_header), app_data_buff.size() - sizeof(app_data_file_binary_header));
    crypto::wipe(&app_data_buff[0], app_data_buff.size());
    return API_RETURN_CODE_OK;
  }

  inline std::string load_encrypted_file(const std::string& path, const std::string& key, std::string& body, uint64_t signature)
  {
    std::string app_data_buff;
    bool r = epee::file_io_utils::load_file_to_string(path, app_data_buff);
    if (!r)
    {
      return API_RETURN_CODE_NOT_FOUND;
    }

    if (is_v2_encrypted_file(app_data_buff))
    {
      return load_encrypted_file_v2(app_data_buff, key, body, signature);
    }

    if (app_data_buff.size() < sizeof(app_data_file_binary_header))
    {
      LOG_ERROR("app_data_buff.size()(" << app_data_buff.size() << ") < sizeof(app_data_file_binary_header) (" << sizeof(app_data_file_binary_header) << ") check failed while loading from " << path);
      return API_RETURN_CODE_INVALID_FILE;
    }

    return load_encrypted_file_legacy(app_data_buff, key, body, signature);
  }

  inline std::string store_encrypted_file(const std::string& path, const std::string& key, const std::string& body, uint64_t signature)
  {
    app_data_file_binary_header_v2 hdr = {};
    hdr.m_signature = details::APP_DATA_FILE_BINARY_SIGNATURE_V2;
    hdr.m_version = details::APP_DATA_FILE_BINARY_VERSION_V2;
    hdr.m_header_size = sizeof(app_data_file_binary_header_v2);
    hdr.m_kdf_algo = WALLET_KDF_ALGO_ROMIX_KECCAK;
    hdr.m_kdf_N_log2 = WALLET_KDF_ROMIX_N_LOG2;
    hdr.m_kdf_phase2_log2_reduction = WALLET_KDF_ROMIX_PHASE2_LOG2_REDUCTION;
    hdr.m_payload_signature = signature;
    hdr.m_iv = crypto::rand<crypto::chacha_iv>();
    crypto::generate_random_bytes(sizeof(hdr.m_kdf_salt), hdr.m_kdf_salt);

    app_data_file_binary_header payload_hdr = {};
    payload_hdr.m_signature = signature;
    payload_hdr.m_cb_body = body.size();

    std::string payload(reinterpret_cast<const char*>(&payload_hdr), sizeof(payload_hdr));
    payload.append(body);

    crypto::chacha_key chacha_key;
    std::string rc = derive_app_data_key_v2(key, hdr, chacha_key);
    if (rc != API_RETURN_CODE_OK)
      return rc;

    std::string encrypted_payload(payload.size(), '\0');
    crypto::chacha20(payload.data(), payload.size(), chacha_key, hdr.m_iv, &encrypted_payload[0]);
    calculate_app_data_mac_v2(hdr, chacha_key, encrypted_payload.data(), encrypted_payload.size(), hdr.m_payload_mac);

    std::string buff(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    buff.append(encrypted_payload);

    crypto::wipe(&payload[0], payload.size());
    crypto::wipe(&encrypted_payload[0], encrypted_payload.size());

    bool r = epee::file_io_utils::save_string_to_file(path, buff);
    if (r)
      return API_RETURN_CODE_OK;
    else
      return API_RETURN_CODE_FAIL;
  }

}
