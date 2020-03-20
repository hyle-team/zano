// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "wallet/view_iface.h"


namespace tools
{

#pragma pack(push, 1)
  struct app_data_file_binary_header
  {
    uint64_t m_signature;
    uint64_t m_cb_body;
  };
#pragma pack (pop)

  inline
  std::string load_encrypted_file(const std::string& path, const std::string& key, std::string& body, uint64_t signature)
  {
    std::string app_data_buff;
    bool r = epee::file_io_utils::load_file_to_string(path, app_data_buff);
    if (!r)
    {
      return API_RETURN_CODE_NOT_FOUND;
    }

    if (app_data_buff.size() < sizeof(app_data_file_binary_header))
    {
      LOG_ERROR("app_data_buff.size()(" << app_data_buff.size() << ") < sizeof(app_data_file_binary_header) (" << sizeof(app_data_file_binary_header) << ") check failed while loading from " << path);
      return API_RETURN_CODE_INVALID_FILE;
    }

    crypto::chacha_crypt(app_data_buff, key);

    const app_data_file_binary_header* phdr = reinterpret_cast<const app_data_file_binary_header*>(app_data_buff.data());
    if (phdr->m_signature != signature)
    {
      return API_RETURN_CODE_WRONG_PASSWORD;
    }
    body = app_data_buff.substr(sizeof(app_data_file_binary_header)).c_str();
    return API_RETURN_CODE_OK;
  }
  inline
  std::string store_encrypted_file(const std::string& path, const std::string& key, const std::string& body, uint64_t signature)
  {
    std::string buff(sizeof(app_data_file_binary_header), 0);
    app_data_file_binary_header* phdr = (app_data_file_binary_header*)buff.data();
    phdr->m_signature = signature;
    phdr->m_cb_body = 0; // for future use

    buff.append(body);
    crypto::chacha_crypt(buff, key);

    bool r = epee::file_io_utils::save_string_to_file(path, buff);
    if (r)
      return API_RETURN_CODE_OK;
    else
      return API_RETURN_CODE_FAIL;
  }

}