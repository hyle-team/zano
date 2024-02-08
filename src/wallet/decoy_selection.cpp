// Copyright (c) 2014-2023 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "decoy_selection.h"
#include "decoy_selection_default_distribution.hpp"
#include "crypto/crypto.h"

bool scaler::config_scale(uint64_t original, uint64_t scale_to)
{
  m_x_m = original;
  m_y_m = scale_to;
  return true;
}

uint64_t scaler::scale(uint64_t h)
{
  double k = double(m_x_m) / m_y_m;
  double e_pow_minus_k = std::exp(-1 * k);
  double a = e_pow_minus_k / (k - 1 + e_pow_minus_k);
  double y = h * a + (1 - std::exp(-1 * (double(h) / double(m_y_m))  )) * m_y_m * (1 - a);
  return static_cast<uint64_t>(std::round(y));
}

void decoy_selection_generator::init(uint64_t max_h)
{
  load_distribution(g_default_distribution, max_h);
  m_is_initialized = true;
  
}
bool decoy_selection_generator::load_distribution_from_file(const char* path)
{
  return true;
}

#define TWO63 0x8000000000000000u 
#define TWO64f (TWO63*2.0)

double map_uint_to_double(uint64_t u) {
  double y = (double)u;
  return y / TWO64f;
}

std::vector<uint64_t> decoy_selection_generator::generate_distribution(uint64_t count)
{
  std::vector<uint64_t> res;
  for (size_t i = 0; i != count; i++)
  {
    uint64_t r = 0;
    crypto::generate_random_bytes(sizeof(r), &r);
    double r_ = map_uint_to_double(r);
    auto it = m_distribution_mapping.upper_bound(r_);
    if (it == m_distribution_mapping.end())
    {
      throw(std::runtime_error(std::string("_r not found in m_distribution_mapping: ") + std::to_string(r_) ));
    }
    uint64_t h = it->second;
    if (it != m_distribution_mapping.begin())
    {
      uint64_t h_0 = (--it)->second;
      crypto::generate_random_bytes(sizeof(r), &r);
      h = h_0 + r % (h - h_0) + 1;
    }
    //scale from nominal to max_h
    res.push_back(h);  }
  return res;
}

uint64_t get_distance(const std::vector<decoy_selection_generator::distribution_entry> entries, size_t i)
{
  if (i == 0)
    return 1;
  return entries[i].h - entries[i - 1].h;
}

bool decoy_selection_generator::load_distribution(const std::vector<decoy_selection_generator::distribution_entry>& original_distribution, uint64_t max_h)
{

  //do prescale of distribution
  std::vector<decoy_selection_generator::distribution_entry> derived_distribution;
  scaler scl;
  scl.config_scale(original_distribution.back().h, max_h);

  uint64_t  last_scaled_h = 0;
  std::list<double> last_scaled_array;


  for (size_t i = 0; i <= original_distribution.size(); i++)
  {
    if (i == original_distribution.size()  || (scl.scale(original_distribution[i].h) != last_scaled_h && last_scaled_array.size()))
    {
      //put avg to data_scaled
      double summ = 0;
      for (auto item: last_scaled_array)
      {
        summ += item;
      }
      double avg = summ / last_scaled_array.size();
      uint64_t prev_h = scl.scale(original_distribution[i - 1].h);
      derived_distribution.push_back(decoy_selection_generator::distribution_entry{ prev_h, avg});
      last_scaled_array.clear();
    }
    if (i == original_distribution.size())
    {
      break;
    }
    last_scaled_array.push_back(original_distribution[i].v);
    last_scaled_h = scl.scale(original_distribution[i].h);
  }
  

  double total_v = 0;

  for (size_t i = 0; i != derived_distribution.size(); i++)
  {
    total_v += derived_distribution[i].v * get_distance(derived_distribution, i);
  }

  double summ_current = 0;
  for (size_t i = 0; i != derived_distribution.size(); i++)
  {
    double k = (derived_distribution[i].v * get_distance(derived_distribution, i))/ total_v;
    summ_current += k;
    m_distribution_mapping[summ_current] = derived_distribution[i].h;
  }

  return true;
}
