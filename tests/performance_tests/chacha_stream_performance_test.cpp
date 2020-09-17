// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <vector>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/invert.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "common/encryption_filter.h"
#include "crypto/crypto.h"
#include "currency_core/blockchain_storage_basic.h"
#include "currency_core/blockchain_storage_boost_serialization.h"
#include "common/boost_serialization_helper.h"


bool perform_crypt_stream_iteration(const std::list<currency::block_extended_info>& test_list, const crypto::chacha8_iv& iv)
{
  std::list<currency::block_extended_info> verification_list;
  boost::filesystem::ofstream store_data_file;
  store_data_file.open("./test.bin", std::ios_base::binary | std::ios_base::out | std::ios::trunc);
  tools::encrypt_chacha_out_filter encrypt_filter("pass", iv);
  boost::iostreams::filtering_ostream out;
  out.push(encrypt_filter);
  out.push(store_data_file);


  //out << buff;
  bool res = tools::portble_serialize_obj_to_stream(test_list, out);

  out.flush();
  store_data_file.close();

  boost::filesystem::ifstream data_file;
  data_file.open("./test.bin", std::ios_base::binary | std::ios_base::in);
  tools::encrypt_chacha_in_filter decrypt_filter("pass", iv);

  boost::iostreams::filtering_istream in;
  in.push(decrypt_filter);
  in.push(data_file);
  try {

    bool res2 = tools::portable_unserialize_obj_from_stream(verification_list, in);
    CHECK_AND_ASSERT_MES(res, false, "Failed to unserialize wallet");
    size_t i = 0;
    CHECK_AND_ASSERT_MES(test_list.size() == verification_list.size(), false, "restored list is wrong size");
    return true;
  }
  catch (std::exception& err)
  {
    LOG_ERROR("Exception: " << err.what());
    return false;
  }
  return true;
}

bool perform_just_substream_stream_iteration(const std::list<currency::block_extended_info>& test_list, const crypto::chacha8_iv& iv)
{
  std::list<currency::block_extended_info> verification_list;
  boost::filesystem::ofstream store_data_file;
  store_data_file.open("./test3.bin", std::ios_base::binary | std::ios_base::out | std::ios::trunc);
  //encrypt_chacha_out_filter encrypt_filter("pass", iv);
  boost::iostreams::filtering_ostream out;
  //out.push(encrypt_filter);
  out.push(store_data_file);


  //out << buff;
  bool res = tools::portble_serialize_obj_to_stream(test_list, out);

  out.flush();
  store_data_file.close();

  boost::filesystem::ifstream data_file;
  data_file.open("./test3.bin", std::ios_base::binary | std::ios_base::in);
  //encrypt_chacha_in_filter decrypt_filter("pass", iv);

  boost::iostreams::filtering_istream in;
  //in.push(decrypt_filter);
  in.push(data_file);
  try {

    bool res2 = tools::portable_unserialize_obj_from_stream(verification_list, in);
    CHECK_AND_ASSERT_MES(res, false, "Failed to unserialize wallet");
    size_t i = 0;
    CHECK_AND_ASSERT_MES(test_list.size() == verification_list.size(), false, "restored list is wrong size");
    return true;
  }
  catch (std::exception& err)
  {
    LOG_ERROR("Exception: " << err.what());
    return false;
  }
  return true;
}


bool perform_no_crypt_stream_iteration(const std::list<currency::block_extended_info>& test_list, const crypto::chacha8_iv& iv)
{
  std::list<currency::block_extended_info> verification_list;
  boost::filesystem::ofstream store_data_file;
  store_data_file.open("./test2.bin", std::ios_base::binary | std::ios_base::out | std::ios::trunc);
//  encrypt_chacha_out_filter encrypt_filter("pass", iv);
//  boost::iostreams::filtering_ostream out;
//  out.push(encrypt_filter);
//  out.push(store_data_file);


  //out << buff;
  bool res = tools::portble_serialize_obj_to_stream(test_list, store_data_file);

  store_data_file.flush();
  store_data_file.close();

  boost::filesystem::ifstream data_file;
  data_file.open("./test2.bin", std::ios_base::binary | std::ios_base::in);
//  encrypt_chacha_in_filter decrypt_filter("pass", iv);

//  boost::iostreams::filtering_istream in;
//  in.push(decrypt_filter);
//  in.push(data_file);
  try {

    bool res2 = tools::portable_unserialize_obj_from_stream(verification_list, data_file);
    CHECK_AND_ASSERT_MES(res, false, "Failed to unserialize wallet");
    size_t i = 0;
    CHECK_AND_ASSERT_MES(test_list.size() == verification_list.size(), false, "restored list is wrong size");
    return true;
  }
  catch (std::exception& err)
  {
    LOG_ERROR("Exception: " << err.what());
    return false;
  }
}



bool do_chacha_stream_performance_test()
{
  LOG_PRINT_L0("chacha_stream_test");
  std::list<currency::block_extended_info> test_list;
  for (size_t i = 0; i != 10000; i++) {
    test_list.push_back(currency::block_extended_info());
    test_list.back().height = i;
    test_list.back().this_block_tx_fee_median = i;
  }


  crypto::chacha8_iv iv = crypto::rand<crypto::chacha8_iv>();
  LOG_PRINT_L0("Running substream stream performance tests...");
  TIME_MEASURE_START(substream_version);
  for (size_t i = 0; i != 100; i++)
  {
    bool r = perform_just_substream_stream_iteration(test_list, iv);
    CHECK_AND_ASSERT_MES(r, false, "Failed to perform_crypt_stream_iteration");
  }
  TIME_MEASURE_FINISH(substream_version);
  LOG_PRINT_L0("OK.time: " << substream_version);
  LOG_PRINT_L0("Running crypt stream performance tests...");
  TIME_MEASURE_START(crypted_version);
  for (size_t i = 0; i != 100; i++)
  {
    bool r = perform_crypt_stream_iteration(test_list, iv);
    CHECK_AND_ASSERT_MES(r, false, "Failed to perform_crypt_stream_iteration");
  }
  TIME_MEASURE_FINISH(crypted_version);
  LOG_PRINT_L0("OK.time: " << crypted_version);
  LOG_PRINT_L0("Running non-crypt stream performance tests...");
  TIME_MEASURE_START(non_crypted_version);
  for (size_t i = 0; i != 100; i++)
  {
    bool r = perform_no_crypt_stream_iteration(test_list, iv);
    CHECK_AND_ASSERT_MES(r, false, "Failed to perform_crypt_stream_iteration");
  }
  TIME_MEASURE_FINISH(non_crypted_version);
  LOG_PRINT_L0("OK.time: " << non_crypted_version);


  return true;
}




