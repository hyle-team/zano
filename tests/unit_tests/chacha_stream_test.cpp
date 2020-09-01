// Copyright (c) 2012-2014 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

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


TEST(chacha_stream_test, basic_test_with_serialization_on_top)
{
  LOG_PRINT_L0("chacha_stream_test");
  std::list<currency::block_extended_info> test_list;
  for(size_t i = 0; i != 1000; i++) {
    test_list.push_back(currency::block_extended_info());
    test_list.back().height = i;
    test_list.back().this_block_tx_fee_median = i;
  }

  std::list<currency::block_extended_info> verification_list;
  crypto::chacha8_iv iv = crypto::rand<crypto::chacha8_iv>();
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
    ASSERT_TRUE(res2);
    size_t i = 0;
    for(auto vle: verification_list) {
      ASSERT_TRUE(vle.height == i);
      ASSERT_TRUE(vle.this_block_tx_fee_median == i);
      i++;
    }
    //if(verification_list != test_list) 
  }
  catch (std::exception& err)
  {
    std::cout << err.what();
    ASSERT_TRUE(false);
  }
  
}


TEST(chacha_stream_test, diversity_test_on_different_stream_behaviour)
{
  LOG_PRINT_L0("chacha_stream_test");
  //prepare buff
  const size_t buff_size = 10000;
  std::string buff(buff_size, ' ');
  for(size_t i = 0; i != buff_size; i++) {
    buff[i] = i % 255;
  }

  crypto::chacha8_iv iv = crypto::rand<crypto::chacha8_iv>();
  boost::filesystem::ofstream store_data_file;
  store_data_file.open("./test.bin", std::ios_base::binary | std::ios_base::out | std::ios::trunc);
  tools::encrypt_chacha_out_filter encrypt_filter("pass", iv);
  //boost::iostreams::stream<encrypt_chacha_sink> outputStream(sink_encrypt);
  boost::iostreams::filtering_ostream out;
  out.push(encrypt_filter);
  out.push(store_data_file);

  out << buff;

  out.flush();
  store_data_file.close();
  for (size_t d = 0; d != 1000; d++)
  {
    boost::filesystem::ifstream data_file;
    data_file.open("./test.bin", std::ios_base::binary | std::ios_base::in);
    tools::encrypt_chacha_in_filter decrypt_filter("pass", iv);

    boost::iostreams::filtering_istream in;
    in.push(decrypt_filter);
    in.push(data_file);

    std::string str(buff_size + 100, ' ');
    try {

      size_t offset = 0;
      while (offset < buff_size+1)
      {
        std::streamsize count = std::rand() % 100;
        //      if (count + offset > buff_size)
        //      {
        //        count = buff_size - offset;
        //      }
        in.read((char*)&str.data()[offset], count);
        //self check
        size_t readed_sz = in.gcount();
        if (!(count + offset > buff_size || readed_sz == count))
        {
          ASSERT_TRUE(count + offset > buff_size || readed_sz == count);
        }
        offset += readed_sz;
        if (!in) {
          break;
        }
      }
      if (in) {
        ASSERT_TRUE(false);
      }
      std::cout << "OK";
    }
    catch (std::exception& err) {
      std::cout << err.what();
      ASSERT_TRUE(false);
    }
  }
  std::cout << "Finished OK!";

}