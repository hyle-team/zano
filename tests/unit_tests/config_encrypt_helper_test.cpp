// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <utility>

#include "include_base_utils.h"
#include "file_io_utils.h"
#include "common/config_encrypt_helper.h"
#include "crypto/chacha.h"
#include "crypto/crypto.h"
#include "crypto/wallet_kdf.h"
#include "currency_core/currency_config.h"

namespace
{
  const uint64_t kAppDataSignature = 0x21436587abcdef10ULL;

  std::string make_test_path(const char* name)
  {
    std::ostringstream s;
    s << "config_encrypt_helper_" << name << "_" << crypto::rand<uint64_t>() << ".bin";
    return s.str();
  }

  struct test_file_guard
  {
    explicit test_file_guard(std::string p) : path(std::move(p)) {}
    ~test_file_guard() { std::remove(path.c_str()); }
    std::string path;
  };
}

TEST(config_encrypt_helper, store_uses_v2_romix_kdf_and_loads_round_trip)
{
  test_file_guard file(make_test_path("v2"));
  const std::string password = "master-password";
  std::string body = "{\"wallets\":[{\"pass\":\"secret\"}]}";
  body.push_back('\0');
  body += "binary-tail";

  ASSERT_EQ(std::string(API_RETURN_CODE_OK), tools::store_encrypted_file(file.path, password, body, kAppDataSignature));

  std::string file_blob;
  ASSERT_TRUE(epee::file_io_utils::load_file_to_string(file.path, file_blob));
  ASSERT_TRUE(tools::is_v2_encrypted_file(file_blob));
  ASSERT_GT(file_blob.size(), sizeof(tools::app_data_file_binary_header_v2));

  const tools::app_data_file_binary_header_v2* hdr = reinterpret_cast<const tools::app_data_file_binary_header_v2*>(file_blob.data());
  EXPECT_EQ(tools::details::APP_DATA_FILE_BINARY_SIGNATURE_V2, hdr->m_signature);
  EXPECT_EQ(tools::details::APP_DATA_FILE_BINARY_VERSION_V2, hdr->m_version);
  EXPECT_EQ(sizeof(tools::app_data_file_binary_header_v2), hdr->m_header_size);
  EXPECT_EQ(WALLET_KDF_ALGO_ROMIX_KECCAK, hdr->m_kdf_algo);
  EXPECT_EQ(WALLET_KDF_ROMIX_N_LOG2, hdr->m_kdf_N_log2);
  EXPECT_EQ(WALLET_KDF_ROMIX_PHASE2_LOG2_REDUCTION, hdr->m_kdf_phase2_log2_reduction);
  EXPECT_EQ(kAppDataSignature, hdr->m_payload_signature);

  std::string loaded;
  ASSERT_EQ(std::string(API_RETURN_CODE_OK), tools::load_encrypted_file(file.path, password, loaded, kAppDataSignature));
  EXPECT_EQ(body, loaded);

  loaded.clear();
  EXPECT_EQ(std::string(API_RETURN_CODE_WRONG_PASSWORD), tools::load_encrypted_file(file.path, "wrong-password", loaded, kAppDataSignature));

  file_blob[file_blob.size() - 1] ^= 0x01;
  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(file.path, file_blob));
  loaded.clear();
  EXPECT_NE(std::string(API_RETURN_CODE_OK), tools::load_encrypted_file(file.path, password, loaded, kAppDataSignature));
}

TEST(config_encrypt_helper, load_keeps_legacy_app_data_compatible)
{
  test_file_guard file(make_test_path("legacy"));
  const std::string password = "master-password";
  std::string body = "legacy secure app data";
  body.push_back('\0');
  body += "with-binary-tail";

  tools::app_data_file_binary_header hdr = {};
  hdr.m_signature = kAppDataSignature;
  hdr.m_cb_body = body.size();

  std::string legacy_blob(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
  legacy_blob.append(body);
  crypto::chacha_crypt_legacy(legacy_blob, password);

  ASSERT_TRUE(epee::file_io_utils::save_string_to_file(file.path, legacy_blob));

  std::string loaded;
  ASSERT_EQ(std::string(API_RETURN_CODE_OK), tools::load_encrypted_file(file.path, password, loaded, kAppDataSignature));
  EXPECT_EQ(body, loaded);

  loaded.clear();
  EXPECT_EQ(std::string(API_RETURN_CODE_WRONG_PASSWORD), tools::load_encrypted_file(file.path, "wrong-password", loaded, kAppDataSignature));
}
