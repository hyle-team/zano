// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <memory>
#include "crypto/crypto.h"
#include "gtest/gtest.h"
#include "common/db_backend_lmdb.h"
#include "common/db_abstract_accessor.h"

using namespace tools;

namespace lmdb_test
{

  TEST(lmdb, 2gb_test)
  {
    bool r = false;
    epee::shared_recursive_mutex rw_lock;

    static const uint64_t buffer_size = 64 * 1024;                                         // 64 KB
    static const uint64_t db_total_size = static_cast<uint64_t>(2.1 * 1024 * 1024 * 1024); // 2.1 GB

    std::shared_ptr<db::lmdb_db_backend> lmdb_ptr = std::make_shared<db::lmdb_db_backend>();
    db::basic_db_accessor bdba(lmdb_ptr, rw_lock);


    r = bdba.open("2gb_lmdb_test", CACHE_SIZE);
    ASSERT_TRUE(r);

    db::container_handle h;
    r = lmdb_ptr->open_container("c1", h);
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->begin_transaction());
    ASSERT_TRUE(lmdb_ptr->clear(h));

    std::vector<uint8_t> buffer;
    buffer.resize(buffer_size);
    crypto::generate_random_bytes(buffer_size, buffer.data());

    uint64_t total_data = 0;
    for (uint64_t key = 0; key < db_total_size / buffer_size; ++key)
    {
      r = lmdb_ptr->set(h, (char*)&key, sizeof key, reinterpret_cast<const char*>(buffer.data()), buffer_size);
      ASSERT_TRUE(r);
      total_data += buffer_size;

      if (key % 1024 == 0)
        std::cout << total_data / 1024 / 1024 << " MB written to DB" << ENDL;
    }

    ASSERT_TRUE(lmdb_ptr->commit_transaction());

    r = bdba.close();
    ASSERT_TRUE(r);




    r = bdba.open("2gb_lmdb_test");
    ASSERT_TRUE(r);
    r = lmdb_ptr->open_container("c1", h);
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->begin_transaction());

    std::string out_buffer;
    total_data = 0;
    for (uint64_t key = 0; key < db_total_size / buffer_size; ++key)
    {
      r = lmdb_ptr->get(h, (char*)&key, sizeof key, out_buffer);
      ASSERT_TRUE(r);
      ASSERT_EQ(buffer_size, out_buffer.size());

      ASSERT_TRUE(0 == memcmp(buffer.data(), out_buffer.c_str(), buffer_size));

      total_data += buffer_size;
      if (key % 1024 == 0)
        std::cout << total_data / 1024 / 1024 << " MB read from DB" << ENDL;
    }

    ASSERT_TRUE(lmdb_ptr->commit_transaction());

    r = bdba.close();
    ASSERT_TRUE(r);

  }

}
