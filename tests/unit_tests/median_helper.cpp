// Copyright (c) 2012-2014 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "misc_language.h"

namespace epee::misc_utils
{
  // old impelementation that uses std::sort
  template<class type_vec_type>
  type_vec_type old_median(std::vector<type_vec_type> &v)
  {
    //CRITICAL_REGION_LOCAL(m_lock);
    if(v.empty())
      return boost::value_initialized<type_vec_type>();
    if(v.size() == 1)
      return v[0];

    size_t n = (v.size()) / 2;
    std::sort(v.begin(), v.end());
    //nth_element(v.begin(), v.begin()+n-1, v.end());
    if(v.size()%2)
    {//1, 3, 5...
      return v[n];
    }else 
    {//2, 4, 6...
      return (v[n-1] + v[n])/2;
    }

  }
} // namespace epee::misc_utils

//------------------------------------------------------------------------------


bool test_median(const std::vector<uint64_t>& v_)
{
  std::vector<uint64_t> v(v_);
  epee::misc_utils::median_helper<uint64_t, uint64_t> mh;
  uint64_t count = 0;
  for (auto i : v)
  {
    mh.push_item(i, count);
    count++;
  }
  uint64_t m1 = epee::misc_utils::median(v);
  uint64_t m2 = mh.get_median();
  uint64_t m3 = epee::misc_utils::old_median(v);
  CHECK_AND_ASSERT_MES(m1 == m2, false, "m1 != m2");
  CHECK_AND_ASSERT_MES(m2 == m3, false, "m2 != m3");
  return true;
}

TEST(median_helper_test, median_helper_test)
{
  std::vector<uint64_t> v;
  bool r = test_median(v);
  ASSERT_TRUE(r);

  v.push_back(10);
  r = test_median(v);
  ASSERT_TRUE(r);

  v.push_back(10);
  r = test_median(v);
  ASSERT_TRUE(r);

  v.push_back(15);
  r = test_median(v);
  ASSERT_TRUE(r);
  v.push_back(15);
  r = test_median(v);
  ASSERT_TRUE(r);


  v.push_back(3);
  r = test_median(v);
  ASSERT_TRUE(r);

  v.push_back(3);
  r = test_median(v);
  ASSERT_TRUE(r);


  v.push_back(3);
  r = test_median(v);
  ASSERT_TRUE(r);

  v.push_back(4);
  r = test_median(v);
  ASSERT_TRUE(r);

  v.push_back(4);
  r = test_median(v);
  ASSERT_TRUE(r);
  v.push_back(4);
  r = test_median(v);
  ASSERT_TRUE(r);
  v.push_back(4);
  r = test_median(v);
  ASSERT_TRUE(r);


  epee::misc_utils::median_helper<uint64_t, uint64_t> mh;
  uint64_t count = 0;
  for (auto i : v)
  {
    mh.push_item(i, count);
    count++;
  }

  auto cb = [](uint64_t k, uint64_t count)
  {
    if (count > 3)
      return true;
    return false;
  };

  auto cb_finalizer = [](uint64_t k, uint64_t count)
  {
    if (count > 2)
      return true;
    return false;
  };

  mh.scan_items(cb, cb_finalizer);

}
