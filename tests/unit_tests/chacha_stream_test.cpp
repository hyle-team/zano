// Copyright (c) 2012-2014 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "common/encryption_sink.h"
#include "crypto/crypto.h"

TEST(chacha_stream_test, chacha_stream_test)
{
  LOG_PRINT_L0("chacha_stream_test");
  const size_t buff_size = 10000;
  std::string buff(buff_size, ' ');
  for (size_t i = 0; i != buff_size; i++)
  {
    buff[i] = i % 255;
  }
  for (size_t i = 0;  i!= 100; i++)
  {
    std::cout << "Encrypting..." << std::endl;
    std::stringstream encrypted;
    crypto::chacha8_iv iv = crypto::rand<crypto::chacha8_iv>();

    encrypt_chacha_sink sink(encrypted, "pass", iv);

    size_t offset = 0;
    while (offset < buff_size)
    {
      std::streamsize count = std::rand() % 1000;
      if (count + offset > buff_size)
      {
        count = buff_size - offset;
      }
      sink.write(&buff[offset], count);

      offset += count;
    }
    sink.flush();
    std::string buff_encrypted = encrypted.str();

    std::cout << "Decrypting...";
    std::stringstream decrypted;
    encrypt_chacha_sink sink2(decrypted, "pass", iv);
    offset = 0;
    while (offset < buff_size)
    {
      std::streamsize count = std::rand() % 1000;
      if (count + offset > buff_size)
      {
        count = buff_size - offset;
      }
      sink2.write(&buff_encrypted[offset], count);

      offset += count;
    }
    sink2.flush();
    std::string buff_decrypted = decrypted.str();

    if (buff_decrypted != buff)
    {
      std::cout << "Failed" << std::endl;
      ASSERT_TRUE(false);
    }
    std::cout << "OK" << std::endl;
  }

}