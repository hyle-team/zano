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


TEST(chacha_stream_test, chacha_stream_test)
{
  LOG_PRINT_L0("chacha_stream_test");
  const size_t buff_size = 10000;
  std::string buff(buff_size, ' ');
  for (size_t i = 0; i != buff_size; i++)
  {
    buff[i] = i % 255;
  }
//   for (size_t i = 0;  i!= 100; i++)
//   {
//     std::cout << "Encrypting..." << std::endl;
//     std::stringstream encrypted;
//     crypto::chacha8_iv iv = crypto::rand<crypto::chacha8_iv>();
// 
//     encrypt_chacha_sink sink(encrypted, "pass", iv);
// 
//     size_t offset = 0;
//     while (offset < buff_size)
//     {
//       std::streamsize count = std::rand() % 1000;
//       if (count + offset > buff_size)
//       {
//         count = buff_size - offset;
//       }
//       sink.write(&buff[offset], count);
// 
//       offset += count;
//     }
//     sink.flush();
//     std::string buff_encrypted = encrypted.str();
// 
//     std::cout << "Decrypting...";
//     std::stringstream decrypted;
//     encrypt_chacha_sink sink2(decrypted, "pass", iv);
//     offset = 0;
//     while (offset < buff_size)
//     {
//       std::streamsize count = std::rand() % 1000;
//       if (count + offset > buff_size)
//       {
//         count = buff_size - offset;
//       }
//       sink2.write(&buff_encrypted[offset], count);
// 
//       offset += count;
//     }
//     sink2.flush();
//     std::string buff_decrypted = decrypted.str();
// 
//     if (buff_decrypted != buff)
//     {
//       std::cout << "Failed" << std::endl;
//       ASSERT_TRUE(false);
//     }
//     std::cout << "OK" << std::endl;
//   }

//   struct category_out: //public boost::iostreams::seekable_device_tag,
//                    public boost::iostreams::multichar_output_filter_tag,
//        	           public boost::iostreams::flushable_tag
//                    //public boost::iostreams::seekable_filter_tag
//   { };
  
//   struct category_in : //public boost::iostreams::seekable_device_tag,
//     public boost::iostreams::multichar_output_filter_tag,
//     public boost::iostreams::flushable_tag
//     //public boost::iostreams::seekable_filter_tag
//   { };

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
  encrypt_chacha_out_filter encrypt_filter("pass", iv);
  //boost::iostreams::stream<encrypt_chacha_sink> outputStream(sink_encrypt);
  boost::iostreams::filtering_ostream out;
  out.push(encrypt_filter);
  out.push(store_data_file);


  //out << buff;
  bool res = tools::portble_serialize_obj_to_stream(test_list, out);
  
  out.flush();
  store_data_file.close();

  boost::filesystem::ifstream data_file;
  data_file.open("./test.bin", std::ios_base::binary | std::ios_base::in);
  encrypt_chacha_in_filter decrypt_filter("pass", iv);

  boost::iostreams::filtering_istream in;
  //in.push(boost::iostreams::invert(decrypt_filter));
  in.push(decrypt_filter);
  in.push(data_file);
 
  //todo: read from stream
  //size_t size = buff_size;//in.tellg();
  ///std::string str(size+10, '\0'); // construct string to stream size
  //in.seekg(0);
  try {
    
    bool res2 = tools::portable_unserialize_obj_from_stream(verification_list, in);
    //if(verification_list != test_list) 
    {
      std::cout << "erroor";
    }
    //in.read(&str[0], size+10);
    //std::streamsize sz = in.gcount();
    //if(!in) {
    //  std::cout << "error";
    //}
    /*
    in.read(&str[0], size + 10);
    sz = in.gcount();
    if(!in) {
      std::cout << "error";
    }

    in.read(&str[0], size + 10);
    sz = in.gcount();
    if(!in) {
      std::cout << "error";
    }
    */
    std::cout << "ddd";

  }
  catch (std::exception& err)
  {
    std::cout << err.what();
    std::cout << err.what();
  }
  



}