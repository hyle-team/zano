// Copyright (c) 2025
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "epee/include/include_base_utils.h"
#include "crypto/crypto.h"
#include "gtest/gtest.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <cstdint>
#include <vector>

#include "common/pod_array_file_container.h"

// helper: returns a unique temp file path
static boost::filesystem::path make_temp_file() {
  return boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("pod_test_%%%%-%%%%.bin");
}

//======================================================================
// typed test fixture for pod_array_file_container<T>
// container      = alias for pod_array_file_container<T>
// tmp_path       = temp file for test
// SetUp()        = generate temp path (SetUp from ::testing::Test)
// TearDown()     = remove temp file (TearDown from ::testing::Test)
//======================================================================

template <typename T>
class pod_array_file_typed_test : public ::testing::Test {
protected:
  using container = tools::pod_array_file_container<T>;
  boost::filesystem::path tmp_path;

  void SetUp() override {
    tmp_path = make_temp_file();
  }

  void TearDown() override {
    if (boost::filesystem::exists(tmp_path)) {
      boost::filesystem::remove(tmp_path);
    }
  }
};

// list of integral types to test
using integral_types = ::testing::Types<
  int8_t, uint8_t,
  int16_t, uint16_t,
  int32_t, uint32_t,
  int64_t, uint64_t
>;
// register typed tests
TYPED_TEST_CASE(pod_array_file_typed_test, integral_types);

// push back and get items:
// write values [-1,0,1] and verify they are read back correctly
TYPED_TEST(pod_array_file_typed_test, push_back_and_get_items) {
  typename TestFixture::container c;
  ASSERT_TRUE(c.open(this->tmp_path.wstring(), true));

  std::vector<TypeParam> values = { static_cast<TypeParam>(-1), 0, static_cast<TypeParam>(1) };
  for (auto v : values) {
    ASSERT_TRUE(c.push_back(v));
  }
  EXPECT_EQ(c.size(), values.size());

  TypeParam read_value;
  for (size_t i = 0; i < values.size(); ++i) {
    ASSERT_TRUE(c.get_item(i, read_value));
    EXPECT_EQ(read_value, values[i]);
  }
}

// ensure get_item returns false for index >= size
TYPED_TEST(pod_array_file_typed_test, get_item_out_of_range) {
  typename TestFixture::container c;
  ASSERT_TRUE(c.open(this->tmp_path.wstring(), true));

  TypeParam dummy;
  EXPECT_FALSE(c.get_item(0, dummy));
  EXPECT_FALSE(c.get_item(100, dummy));
}

// --------------------------------------------------------------------

typedef uint32_t pod_t;
typedef tools::pod_array_file_container<pod_t> pod_container;

// open fails if not exist without create:
// open() false when file missing and create_if_not_exist=false
TEST(pod_array_file_container, open_fails_if_not_exist_without_create) {
  auto path = make_temp_file();
  pod_container c;
  std::string reason;
  bool opened = c.open(path.wstring(), false, nullptr, &reason);
  EXPECT_FALSE(opened);
  EXPECT_TRUE(reason.find("not exist") != std::string::npos);
}

// open creates file when not exist:
// open() with create flag creates empty file and size=0
TEST(pod_array_file_container, open_creates_file_when_not_exist) {
  auto path = make_temp_file();
  pod_container c;
  bool corrupted = true;
  ASSERT_TRUE(c.open(path.wstring(), true, &corrupted, nullptr));
  EXPECT_FALSE(corrupted);
  EXPECT_EQ(c.size(), 0u);
  c.close();
  EXPECT_TRUE(boost::filesystem::exists(path));
}

// --------------------------------------------------------------------

// POD struct with fixed-size fields
struct test_struct {
  crypto::public_key pubkey;
  crypto::key_image key;
};

typedef tools::pod_array_file_container<test_struct> struct_container;

// push back and get items struct:
// write two test_struct and verify memory equality
TEST(pod_array_file_container_struct, push_back_and_get_items_struct) {
  auto path = make_temp_file();
  struct_container c;
  ASSERT_TRUE(c.open(path.wstring(), true));

  test_struct a1{}, a2{};
  memset(a1.pubkey.data, 'a', sizeof(a1.pubkey.data));
  memset(a1.key.data, 'A', sizeof(a1.key.data));
  memset(a2.pubkey.data, 'b', sizeof(a2.pubkey.data));
  memset(a2.key.data, 'B', sizeof(a2.key.data));

  ASSERT_TRUE(c.push_back(a1));
  ASSERT_TRUE(c.push_back(a2));
  EXPECT_EQ(c.size(), 2u);

  test_struct got;
  ASSERT_TRUE(c.get_item(0, got));
  EXPECT_EQ(0, memcmp(&got, &a1, sizeof(test_struct)));
  ASSERT_TRUE(c.get_item(1, got));
  EXPECT_EQ(0, memcmp(&got, &a2, sizeof(test_struct)));
}

// get item out of range struct:
// get_item false when no items written
TEST(pod_array_file_container_struct, get_item_out_of_range_struct) {
  auto path = make_temp_file();
  struct_container c;
  ASSERT_TRUE(c.open(path.wstring(), true));
  test_struct dummy;
  EXPECT_FALSE(c.get_item(0, dummy));
}

// corrupted file truncation struct:
// simulate corrupted file tail and check truncation flag and size
TEST(pod_array_file_container_struct, corrupted_file_truncation_struct) {
  auto path = make_temp_file();
  {
    // write a valid test_struct followed by garbage data
    boost::filesystem::ofstream out(path, std::ios::binary | std::ios::out);
    test_struct tmp{};
    memset(tmp.pubkey.data, 'X', sizeof(tmp.pubkey.data));
    memset(tmp.key.data, 'Y', sizeof(tmp.key.data));
    out.write(reinterpret_cast<char*>(&tmp), sizeof(tmp));
    const char garbage[5] = {1,2,3,4,5};
    out.write(garbage, sizeof(garbage));
  }

  struct_container c;
  bool corrupted = false;
  std::string reason;
  ASSERT_TRUE(c.open(path.wstring(), false, &corrupted, &reason));
  EXPECT_TRUE(corrupted);
  EXPECT_EQ(c.size(), 1u);

  test_struct got;
  ASSERT_TRUE(c.get_item(0, got));
  EXPECT_EQ(0, memcmp(&got.pubkey.data, std::vector<char>(sizeof(got.pubkey.data), 'X').data(), sizeof(got.pubkey.data)));
  EXPECT_EQ(0, memcmp(&got.key.data, std::vector<char>(sizeof(got.key.data), 'Y').data(), sizeof(got.key.data)));
  EXPECT_TRUE(reason.find("truncated") != std::string::npos);
}

// persistence between opens struct:
// write multiple structs, reopen file, verify data persists
TEST(pod_array_file_container_struct, persistence_between_opens_struct) {
  auto path = make_temp_file();
  {
    // write 3 test_struct with different pubkey and key values
    struct_container c;
    ASSERT_TRUE(c.open(path.wstring(), true));
    for (int i = 0; i < 3; ++i) {
      test_struct tmp{};
      tmp.pubkey.data[0] = '0' + i;
      tmp.key.data[0] = 'A' + i;
      ASSERT_TRUE(c.push_back(tmp));
    }
    EXPECT_EQ(c.size(), 3u);
  }

  // reopen and verify data
  struct_container c2;
  ASSERT_TRUE(c2.open(path.wstring(), false));
  EXPECT_EQ(c2.size(), 3u);
  test_struct got;
  for (int i = 0; i < 3; ++i) {
    ASSERT_TRUE(c2.get_item(i, got));
    EXPECT_EQ(got.pubkey.data[0], '0' + i);
    EXPECT_EQ(got.key.data[0], 'A' + i);
  }
}

// size bytes and size:
// check size_bytes() matches raw byte count and size() element count
TEST(pod_array_file_container, size_bytes_and_size) {
  auto path = make_temp_file();
  pod_container c;
  ASSERT_TRUE(c.open(path.wstring(), true));
  EXPECT_EQ(c.size_bytes(), 0u);
  EXPECT_EQ(c.size(), 0u);
  // push one element
  pod_t value = 42;
  ASSERT_TRUE(c.push_back(value));
  EXPECT_EQ(c.size_bytes(), sizeof(pod_t));
  EXPECT_EQ(c.size(), 1u);
}

// operations after close:
// ensure push_back and get_item fail after close()
TEST(pod_array_file_container, operations_after_close) {
  auto path = make_temp_file();
  pod_container c;
  ASSERT_TRUE(c.open(path.wstring(), true));
  ASSERT_TRUE(c.push_back(123u));
  c.close();
  // after close, operations should return false
  EXPECT_FALSE(c.push_back(456u));
  pod_t dummy = 0;
  EXPECT_FALSE(c.get_item(0, dummy));
}

// open fails if cannot open (directory):
// attempt to open a directory path should fail with "file could not be opened"
TEST(pod_array_file_container, open_fails_if_cannot_open) {
  // create a directory instead of a file
  auto dir_path = make_temp_file();
  boost::filesystem::create_directory(dir_path);
  pod_container c;
  std::string reason;
  bool opened = c.open(dir_path.wstring(), true, nullptr, &reason);
  EXPECT_FALSE(opened);
  EXPECT_TRUE(reason.find("could not be opened") != std::string::npos);
  boost::filesystem::remove(dir_path);
}

// corrupted file truncation uint32:
// simulate corrupted file tail on uint32_t and check truncation
TEST(pod_array_file_container, corrupted_file_truncation_uint32) {
  auto path = make_temp_file();
  {
    boost::filesystem::ofstream out(path, std::ios::binary | std::ios::out);
    pod_t v = 0x12345678u;
    out.write(reinterpret_cast<char*>(&v), sizeof(v));
    const char junk[3] = {9,8,7};
    out.write(junk, sizeof(junk));
  }
  pod_container c;
  bool corrupted = false;
  std::string reason;
  ASSERT_TRUE(c.open(path.wstring(), false, &corrupted, &reason));
  EXPECT_TRUE(corrupted);
  EXPECT_EQ(c.size(), 1u);
  pod_t read_v;
  ASSERT_TRUE(c.get_item(0, read_v));
  EXPECT_EQ(read_v, 0x12345678u);
  EXPECT_TRUE(reason.find("truncated") != std::string::npos);
}

// operations without open:
// ensure push_back/get_item/size_bytes/size behave when container never opened
TEST(pod_array_file_container, operations_without_open) {
  pod_container c;
  EXPECT_FALSE(c.push_back(1u));
  pod_t dummy;
  EXPECT_FALSE(c.get_item(0, dummy));
  EXPECT_EQ(c.size_bytes(), 0u);
  EXPECT_EQ(c.size(), 0u);
}
