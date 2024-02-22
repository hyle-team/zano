// Copyright (c) 2020 Zano Project
// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <boost/program_options.hpp>
#include "net/http_client.h"
#include "db_backend_selector.h"
#include "crypto/crypto.h"
#include "currency_core/currency_core.h"

namespace tools
{
  struct pre_download_entry
  {
    const char* url;
    const char* hash;
    uint64_t packed_size;
    uint64_t unpacked_size;
  };

#ifndef TESTNET
  static constexpr pre_download_entry c_pre_download_mdbx = { "http://95.217.42.247/pre-download/zano_mdbx_95_2500000.pak", "8ffa2cb4213f4f96f97033c65a9e52bc350f683237808597784e79b24d5bfee7", 3242348793, 5905489920 };
  static constexpr pre_download_entry c_pre_download_lmdb = { "http://95.217.42.247/pre-download/zano_lmdb_95_2500000.pak", "5509650e12c8f901e6731a2bfaf3abfd64409e3e1366d3d94cd11db8beddb0c3", 4239505801, 5893566464 };
#else
  static constexpr pre_download_entry c_pre_download_mdbx = { "", "", 0, 0 };
  static constexpr pre_download_entry c_pre_download_lmdb = { "", "", 0, 0 };
#endif

  static constexpr uint64_t pre_download_min_size_difference = 512 * 1024 * 1024; // minimum difference in size between local DB and the downloadable one to start downloading

  template<class callback_t>
  bool process_predownload(const boost::program_options::variables_map& vm, callback_t cb_should_stop)
  {
    tools::db::db_backend_selector dbbs;
    bool r = dbbs.init(vm);
    CHECK_AND_ASSERT_MES(r, false, "db_backend_selector failed to initialize");

    std::string config_folder = dbbs.get_config_folder();
    std::string working_folder = dbbs.get_db_folder_path();
    std::string db_main_file_path = working_folder + "/" + dbbs.get_db_main_file_name();

    pre_download_entry pre_download = dbbs.get_engine_type() == db::db_lmdb ? c_pre_download_lmdb : c_pre_download_mdbx;
    
    // override pre-download link if necessary
    std::string url = pre_download.url;
    if (command_line::has_arg(vm, command_line::arg_predownload_link))
      url = command_line::get_arg(vm, command_line::arg_predownload_link);

    boost::system::error_code ec;
    uint64_t sz = boost::filesystem::file_size(db_main_file_path, ec);
    if (ec) sz = 0;
    bool flag_force_predownload = command_line::has_arg(vm, command_line::arg_force_predownload);
    if (pre_download.unpacked_size == 0 || !(ec || (pre_download.unpacked_size > sz && pre_download.unpacked_size - sz > pre_download_min_size_difference) || flag_force_predownload) )
    {
      LOG_PRINT_MAGENTA("Pre-downloading not needed (db file size = " << sz << ")", LOG_LEVEL_0);
      return true;
    }

    // okay, let's download

    std::string downloading_file_path = db_main_file_path + ".download";
    if (!command_line::has_arg(vm, command_line::arg_process_predownload_from_path))
    {

      LOG_PRINT_MAGENTA("Trying to download blockchain database from " << url << " ...", LOG_LEVEL_0);
      epee::net_utils::http::interruptible_http_client cl;

      crypto::stream_cn_hash hash_stream;
      auto last_update = std::chrono::system_clock::now();

      auto cb = [&hash_stream, &last_update, &cb_should_stop](const std::string& buff, uint64_t total_bytes, uint64_t received_bytes)
      {
        if (cb_should_stop(total_bytes, received_bytes))
        {
          LOG_PRINT_MAGENTA(ENDL << "Interrupting download", LOG_LEVEL_0);
          return false;
        }

        hash_stream.update(buff.data(), buff.size());

        auto dif = std::chrono::system_clock::now() - last_update;
        if (dif >= std::chrono::milliseconds(300))
        {
          boost::io::ios_flags_saver ifs(std::cout);
          std::cout << "Received " << received_bytes / 1048576 << " of " << total_bytes / 1048576 << " MiB ( " << std::fixed << std::setprecision(1) << 100.0 * received_bytes / total_bytes << " %)\r";
          last_update = std::chrono::system_clock::now();
        }

        return true;
      };

      tools::create_directories_if_necessary(working_folder);
      r = cl.download_and_unzip(cb, downloading_file_path, url, 5000 /* timout */, "GET", std::string(), 30 /* fails count */);
      if (!r)
      {
        LOG_PRINT_RED("Downloading failed", LOG_LEVEL_0);
        return !flag_force_predownload;  // fatal error only if force-predownload
      }

      crypto::hash data_hash = hash_stream.calculate_hash();
      if (epee::string_tools::pod_to_hex(data_hash) != pre_download.hash)
      {
        LOG_ERROR("hash missmatch in downloaded file, got: " << epee::string_tools::pod_to_hex(data_hash) << ", expected: " << pre_download.hash);
        return !flag_force_predownload;  // fatal error only if force-predownload
      }

      LOG_PRINT_GREEN("Download succeeded, hash " << pre_download.hash << " is correct", LOG_LEVEL_0);
    }
    else
    {
      downloading_file_path = command_line::get_arg(vm, command_line::arg_process_predownload_from_path);
    }

    if (!command_line::has_arg(vm, command_line::arg_validate_predownload))
    {
      boost::filesystem::remove(db_main_file_path, ec);
      if (ec)
      {
        LOG_ERROR("Failed to remove " << db_main_file_path);
        return false;
      }
      LOG_PRINT_L1("Removed " << db_main_file_path);

      boost::filesystem::rename(downloading_file_path, db_main_file_path, ec);
      if (ec)
      {
        LOG_ERROR("Failed to rename " << downloading_file_path << " -> " << db_main_file_path);
        return false;
      }
      LOG_PRINT_L1("Renamed " << downloading_file_path << " -> " << db_main_file_path);

      LOG_PRINT_GREEN("Blockchain successfully replaced with the new pre-downloaded data file", LOG_LEVEL_0);
      return true;
    }

    //
    // paranoid mode
    // move downloaded blockchain into a temporary folder
    //
    LOG_PRINT_MAGENTA(ENDL << "Downloaded blockchain database is about to be validated and added to the local database block-by-block" << ENDL, LOG_LEVEL_0);
    std::string path_to_temp_datafolder = dbbs.get_temp_config_folder();
    std::string path_to_temp_blockchain = dbbs.get_temp_db_folder_path();
    std::string path_to_temp_blockchain_file = path_to_temp_blockchain + "/" + dbbs.get_db_main_file_name();

    tools::create_directories_if_necessary(path_to_temp_blockchain);
    if (downloading_file_path != path_to_temp_blockchain_file)
    {
      boost::filesystem::rename(downloading_file_path, path_to_temp_blockchain_file, ec);
      if (ec)
      {
        LOG_ERROR("Rename failed: " << downloading_file_path << " -> " << path_to_temp_blockchain_file);
        return false;
      }
    }

    // remove old blockchain database from disk
    boost::filesystem::remove_all(working_folder, ec);
    if (ec)
    {
      LOG_ERROR("Failed to remove all from " << working_folder);
      return false;
    }

    std::string pool_db_path = dbbs.get_pool_db_folder_path();
    boost::filesystem::remove_all(pool_db_path, ec);
    if (ec)
    {
      LOG_ERROR("Failed to remove all from " << pool_db_path);
      return false;
    }

    // source core
    currency::core source_core(nullptr);
    boost::program_options::variables_map source_core_vm;
    source_core_vm.insert(std::make_pair("data-dir", boost::program_options::variable_value(path_to_temp_datafolder, false)));
    source_core_vm.insert(std::make_pair("db-engine", boost::program_options::variable_value(dbbs.get_engine_name(), false)));
    //source_core_vm.insert(std::make_pair("db-sync-mode", boost::program_options::variable_value(std::string("fast"), false)));

    r = source_core.init(source_core_vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to init source core");

    // target core
    currency::core target_core(nullptr);
    boost::program_options::variables_map target_core_vm(vm);
    target_core_vm.insert(std::make_pair("db-engine", boost::program_options::variable_value(dbbs.get_engine_name(), false)));
    //vm_with_fast_sync.insert(std::make_pair("db-sync-mode", boost::program_options::variable_value(std::string("fast"), false)));

    r = target_core.init(target_core_vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to init target core");

    if (true/*TODO: copnfigure with command line option*/)
    {
      //set checkpoints
      {
        currency::checkpoints checkpoints;
        bool res = currency::create_checkpoints(checkpoints);
        CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize checkpoints");
        res = source_core.set_checkpoints(std::move(checkpoints));
        CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core");
      }
      {
        currency::checkpoints checkpoints;
        bool res = currency::create_checkpoints(checkpoints);
        CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize checkpoints");
        res = target_core.set_checkpoints(std::move(checkpoints));
        CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core");
      }
    }




    CHECK_AND_ASSERT_MES(target_core.get_top_block_height() == 0, false, "Target blockchain initialized not empty");
    uint64_t total_blocks = source_core.get_current_blockchain_size();

    LOG_PRINT_GREEN("Manually processing blocks from 1 to " << total_blocks << "...", LOG_LEVEL_0);

    for (uint64_t i = 1; i != total_blocks; i++)
    { 
      std::list<currency::block> blocks;
      std::list<currency::transaction> txs;
      bool r = source_core.get_blocks(i, 1, blocks, txs);
      CHECK_AND_ASSERT_MES(r && blocks.size()==1, false, "Failed to get block " << i << " from core");
      currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      crypto::hash tx_hash = AUTO_VAL_INIT(tx_hash);
      for (auto& tx : txs)
      {
        r = target_core.handle_incoming_tx(tx, tvc, true /* kept_by_block */);
        CHECK_AND_ASSERT_MES(r && tvc.m_added_to_pool == true, false, "Failed to add a tx from block " << i << " from core");
      }
      currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
      r = target_core.handle_incoming_block(*blocks.begin(), bvc);
      CHECK_AND_ASSERT_MES(r && bvc.m_added_to_main_chain == true, false, "Failed to add block " << i << " to core");
      if (!(i % 100))
        std::cout << "Block " << i << "(" << (i * 100) / total_blocks << "%) \r";

      if (cb_should_stop(total_blocks, i))
      {
        LOG_PRINT_MAGENTA(ENDL << "Interrupting updating db...", LOG_LEVEL_0);
        return false;
      }
    }
    
    LOG_PRINT_GREEN("Processing finished, " << total_blocks << " successfully added.", LOG_LEVEL_0);
    target_core.deinit();
    source_core.deinit();

    boost::filesystem::remove_all(path_to_temp_datafolder, ec);
    if (ec)
    {
      LOG_ERROR("Failed to remove all from " << path_to_temp_datafolder);
    }

    return true;
  }
}

