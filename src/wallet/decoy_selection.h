// Copyright (c) 2014-2023 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <memory>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/serialization/extended_type_info.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/optional.hpp>
#include <atomic>


#include "include_base_utils.h"
#include "profile_tools.h"
#include "sync_locked_object.h"



class scaler
{
public: 
  //See the graph on https://www.desmos.com/calculator/zfx4bolfqx 
  bool config_scale(uint64_t original, uint64_t scale_to);
  uint64_t scale(uint64_t h);
private:
  uint64_t m_x_m;
  uint64_t m_y_m;
};


class decoy_selection_generator
{
public:
  struct distribution_entry
  {
    uint64_t h;
    double v;
  };
 
  void init(uint64_t max_h);
  bool load_distribution_from_file(const char* path);
  std::vector<uint64_t> generate_distribution(uint64_t count);
  std::vector<uint64_t> generate_unique_reversed_distribution(uint64_t count, uint64_t preincluded_item);
  std::vector<uint64_t> generate_unique_reversed_distribution(uint64_t count);
  void generate_unique_reversed_distribution(uint64_t count, std::set<uint64_t>& set_to_extend);
  bool is_initialized() { return m_is_initialized; }

private: 
  bool load_distribution(const std::vector<decoy_selection_generator::distribution_entry>& entries, uint64_t max_h);
  bool m_is_initialized = false;
  uint64_t m_max = 0;
  std::map<double, uint64_t> m_distribution_mapping;
};
