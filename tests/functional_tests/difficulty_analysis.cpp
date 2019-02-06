// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include "include_base_utils.h"
using namespace epee;
#include "wallet/wallet2.h"
#include "currency_core/blockchain_storage.h"

 
using std::size_t;
using std::uint64_t;
using std::vector;

bool parse_file(const std::string& path, std::vector<std::vector<uint64_t>>& blocks, uint64_t reserve_size)
{
  std::ifstream fstr(path);
  if (!fstr.good())
  {
    LOG_ERROR("unable to open " << path);
    return false;
  }

  LOG_PRINT_L0("Loading array...");
  std::string line;
  blocks.reserve(reserve_size);
  while (std::getline(fstr, line))
  {
    std::vector<uint64_t> array_num;
    boost::tokenizer<> tok(line);
    std::transform(tok.begin(), tok.end(), std::back_inserter(array_num),
      &boost::lexical_cast<uint64_t, std::string>);

    array_num.push_back(0); //reserve space for hashrate value
    blocks.push_back(array_num);
  }
  LOG_PRINT_L0("Loaded " << blocks.size() << " lines");
  return true;
}


#define BBR_DIFFICULTY_TARGET                               120 // seconds
#define BBR_DIFFICULTY_WINDOW                               720 // blocks
#define BBR_DIFFICULTY_LAG                                  15  // !!!
#define BBR_DIFFICULTY_CUT                                  60  // timestamps to cut after sorting
#define BBR_DIFFICULTY_STARTER                              1

#define NEW_DIFFICULTY_WINDOW                                   360
#define NEW_DIFFICULTY_CUT_OLD                                  60  // timestamps to cut after sorting on the oldest timestamps
#define NEW_DIFFICULTY_CUT_LAST                                 0  // timestamps to cut after sorting on the most recent timestamps

const boost::multiprecision::uint256_t max128bit(std::numeric_limits<boost::multiprecision::uint128_t>::max());
currency::wide_difficulty_type bbr_next_difficulty(std::vector<uint64_t>& timestamps, std::vector<currency::wide_difficulty_type>& cumulative_difficulties, size_t target_seconds)
{
  // timestamps  - first is latest, back - is oldest timestamps
  //cutoff DIFFICULTY_LAG
  if (timestamps.size() > BBR_DIFFICULTY_WINDOW)
  {
    timestamps.resize(BBR_DIFFICULTY_WINDOW);
    cumulative_difficulties.resize(BBR_DIFFICULTY_WINDOW);
  }


  size_t length = timestamps.size();
  CHECK_AND_ASSERT_MES(length == cumulative_difficulties.size(), 0, "Check \"length == cumulative_difficulties.size()\" failed");
  if (length <= 1) {
    return BBR_DIFFICULTY_STARTER;
  }
  static_assert(BBR_DIFFICULTY_WINDOW >= 2, "Window is too small");
  CHECK_AND_ASSERT_MES(length <= BBR_DIFFICULTY_WINDOW, 0, "length <= BBR_DIFFICULTY_WINDOW check failed, length=" << length);
  sort(timestamps.begin(), timestamps.end(), std::greater<uint64_t>());
  size_t cut_begin, cut_end;
  static_assert(2 * BBR_DIFFICULTY_CUT <= BBR_DIFFICULTY_WINDOW - 2, "Cut length is too large");
  if (length <= BBR_DIFFICULTY_WINDOW - 2 * BBR_DIFFICULTY_CUT) {
    cut_begin = 0;
    cut_end = length;
  }
  else {
    cut_begin = (length - (BBR_DIFFICULTY_WINDOW - 2 * BBR_DIFFICULTY_CUT) + 1) / 2;
    cut_end = cut_begin + (BBR_DIFFICULTY_WINDOW - 2 * BBR_DIFFICULTY_CUT);
  }
  CHECK_AND_ASSERT_THROW_MES(/*cut_begin >= 0 &&*/ cut_begin + 2 <= cut_end && cut_end <= length, "validation in next_difficulty is failed");
  uint64_t time_span = timestamps[cut_begin] - timestamps[cut_end - 1];
  if (time_span == 0) {
    time_span = 1;
  }
  currency::wide_difficulty_type total_work = cumulative_difficulties[cut_begin] - cumulative_difficulties[cut_end - 1];
  boost::multiprecision::uint256_t res = (boost::multiprecision::uint256_t(total_work) * target_seconds + time_span - 1) / time_span;
  if (res > max128bit)
    return 0; // to behave like previous implementation, may be better return max128bit?
  return res.convert_to<currency::wide_difficulty_type>();
}


void get_cut_location_from_len(size_t length, size_t& cut_begin, size_t& cut_end, size_t REDEF_DIFFICULTY_WINDOW, size_t REDEF_DIFFICULTY_CUT_OLD, size_t REDEF_DIFFICULTY_CUT_LAST)
{
  if (length <= REDEF_DIFFICULTY_WINDOW)
  {
    cut_begin = 0;
    cut_end = length;
  }
  else
  {
    cut_begin = REDEF_DIFFICULTY_WINDOW - REDEF_DIFFICULTY_CUT_LAST + 1;
    cut_end = cut_begin + (REDEF_DIFFICULTY_WINDOW - (REDEF_DIFFICULTY_CUT_OLD + REDEF_DIFFICULTY_CUT_LAST));
  }
}

currency::wide_difficulty_type bbr_next_difficulty_configurable(std::vector<uint64_t>& timestamps, std::vector<currency::wide_difficulty_type>& cumulative_difficulties, size_t target_seconds, size_t REDEF_DIFFICULTY_WINDOW, size_t REDEF_DIFFICULTY_CUT_OLD, size_t REDEF_DIFFICULTY_CUT_LAST)
{
  // timestamps  - first is latest, back - is oldest timestamps
  //cutoff DIFFICULTY_LAG
  if (timestamps.size() > REDEF_DIFFICULTY_WINDOW)
  {
    timestamps.resize(REDEF_DIFFICULTY_WINDOW);
    cumulative_difficulties.resize(REDEF_DIFFICULTY_WINDOW);
  }


  size_t length = timestamps.size();
  CHECK_AND_ASSERT_MES(length == cumulative_difficulties.size(), 0, "Check \"length == cumulative_difficulties.size()\" failed");
  if (length <= 1) {
    return BBR_DIFFICULTY_STARTER;
  }
  CHECK_AND_ASSERT_THROW_MES(REDEF_DIFFICULTY_WINDOW >= 2, "Window is too small");
  CHECK_AND_ASSERT_MES(length <= REDEF_DIFFICULTY_WINDOW, 0, "length <= REDEF_DIFFICULTY_WINDOW check failed, length=" << length);
  sort(timestamps.begin(), timestamps.end(), std::greater<uint64_t>());
  size_t cut_begin, cut_end;
  CHECK_AND_ASSERT_THROW_MES( (REDEF_DIFFICULTY_CUT_OLD + REDEF_DIFFICULTY_CUT_LAST) <= REDEF_DIFFICULTY_WINDOW - 2, "Cut length is too large");
  get_cut_location_from_len(length, cut_begin, cut_end, REDEF_DIFFICULTY_WINDOW, REDEF_DIFFICULTY_CUT_OLD, REDEF_DIFFICULTY_CUT_LAST);

  CHECK_AND_ASSERT_THROW_MES(/*cut_begin >= 0 &&*/ cut_begin + 2 <= cut_end && cut_end <= length, "validation in next_difficulty is failed");
  uint64_t time_span = timestamps[cut_begin] - timestamps[cut_end - 1];
  if (time_span == 0) {
    time_span = 1;
  }
  currency::wide_difficulty_type total_work = cumulative_difficulties[cut_begin] - cumulative_difficulties[cut_end - 1];
  boost::multiprecision::uint256_t res = (boost::multiprecision::uint256_t(total_work) * target_seconds + time_span - 1) / time_span;
  if (res > max128bit)
    return 0; // to behave like previous implementation, may be better return max128bit?
  return res.convert_to<currency::wide_difficulty_type>();
}

currency::wide_difficulty_type bbr_next_difficulty2(std::vector<uint64_t>& timestamps, std::vector<currency::wide_difficulty_type>& cumulative_difficulties, size_t target_seconds)
{
  return bbr_next_difficulty_configurable(timestamps, cumulative_difficulties, target_seconds, NEW_DIFFICULTY_WINDOW, NEW_DIFFICULTY_CUT_OLD, NEW_DIFFICULTY_CUT_LAST);
}

uint64_t get_next_timestamp_by_difficulty_and_hashrate(uint64_t last_timestamp, currency::wide_difficulty_type difficulty, uint64_t hashrate)
{
  uint64_t seconds = (difficulty / hashrate).convert_to<uint64_t>();
  return last_timestamp + seconds;
}

void print_blocks(const std::vector<std::vector<uint64_t>>& blocks, const std::string& res_path)
{
  std::stringstream ss;
  for (size_t i = 0; i != blocks.size(); i++)
  {
    ss << std::left << std::setw(10) << i << std::left << std::setw(15) << blocks[i][0];
    for (size_t j = 1; j != blocks[i].size()-1; j++)
    {
      ss << std::left << std::setw(15) << blocks[i][j] / 120;
    }
    ss << std::left << std::setw(20) << blocks[i][blocks[i].size() - 1] << ENDL;
  }
  file_io_utils::save_string_to_file(res_path, ss.str());
  LOG_PRINT_L0("Done, saved to file " << res_path);
}

void run_emulation(const std::string& path)
{
  //0 - timestamp, 1 - difficulty/120, 2 net hashrate (h/s)

  std::vector<std::vector<uint64_t>> blocks;
  std::vector<std::vector<uint64_t>> result_blocks;
  blocks.reserve(401);
  result_blocks.reserve(401);
//   std::vector<uint64_t> timestamps, timestamps_new;
//   std::vector<currency::wide_difficulty_type> cumul_difficulties, cumul_difficulties_new;
//   timestamps.reserve(4010);
//   cumul_difficulties.reserve(4010);
//   timestamps_new.reserve(4010);
//   cumul_difficulties_new.reserve(4010);


  parse_file(path, blocks, 500);

#define DEFINE_DIFFICULTY_SIMULATION(sim_name) \
  std::vector<uint64_t> timestamps_##sim_name; \
  std::vector<currency::wide_difficulty_type> cumul_difficulties_##sim_name; \
  timestamps_##sim_name.reserve(4010); \
  cumul_difficulties_##sim_name.reserve(4010); \
  timestamps_##sim_name.push_back(blocks[0][0]); \
  cumul_difficulties_##sim_name.push_back(blocks[0][1]); \
  currency::wide_difficulty_type curren_difficulty_##sim_name = 0;

  DEFINE_DIFFICULTY_SIMULATION(original);
  DEFINE_DIFFICULTY_SIMULATION(new_720_5_60);
  DEFINE_DIFFICULTY_SIMULATION(new_300_5_60);
  DEFINE_DIFFICULTY_SIMULATION(new_200_5_5);

  for (uint64_t b_no = 0; b_no != blocks.size(); b_no++)
  {
    auto& b_line = blocks[b_no];


    for (size_t i = 0; i != 10; i++)
    {
#define PROCESS_DIFFICULTY_SIMULATION(sim_name, func_name, window_size, cut_old, cut_new) \
      if (timestamps_##sim_name.size() < BBR_DIFFICULTY_WINDOW) \
      { \
        curren_difficulty_##sim_name = b_line[1] * 120; \
      } \
      else \
      { \
        std::vector<uint64_t> backward_timestamps; \
        backward_timestamps.reserve(BBR_DIFFICULTY_WINDOW); \
        std::copy(timestamps_##sim_name.rbegin(), timestamps_##sim_name.rbegin() + BBR_DIFFICULTY_WINDOW - 1, std::back_inserter(backward_timestamps)); \
        std::vector<currency::wide_difficulty_type> backward_cumul_difficulties; \
        backward_cumul_difficulties.reserve(BBR_DIFFICULTY_WINDOW); \
        std::copy(cumul_difficulties_##sim_name.rbegin(), cumul_difficulties_##sim_name.rbegin() + BBR_DIFFICULTY_WINDOW - 1, std::back_inserter(backward_cumul_difficulties)); \
        curren_difficulty_##sim_name = func_name(backward_timestamps, backward_cumul_difficulties, BBR_DIFFICULTY_TARGET, window_size, cut_old, cut_new); \
      } \
      cumul_difficulties_##sim_name.push_back(cumul_difficulties_##sim_name.back() + curren_difficulty_##sim_name); \
      timestamps_##sim_name.push_back(get_next_timestamp_by_difficulty_and_hashrate(timestamps_##sim_name.back(), curren_difficulty_##sim_name, b_line[2]));        

      PROCESS_DIFFICULTY_SIMULATION(original, bbr_next_difficulty_configurable, BBR_DIFFICULTY_WINDOW, BBR_DIFFICULTY_CUT, BBR_DIFFICULTY_CUT);
      PROCESS_DIFFICULTY_SIMULATION(new_720_5_60, bbr_next_difficulty_configurable, NEW_DIFFICULTY_WINDOW, NEW_DIFFICULTY_CUT_OLD, NEW_DIFFICULTY_CUT_LAST);
      PROCESS_DIFFICULTY_SIMULATION(new_300_5_60, bbr_next_difficulty_configurable, 300, 5, 60);
      PROCESS_DIFFICULTY_SIMULATION(new_200_5_5, bbr_next_difficulty_configurable, 200, 5, 5);

      std::cout << b_no << "\r";
    }
    result_blocks.push_back(std::vector<uint64_t>());
    result_blocks.back().resize(20);
    size_t index = 0;

#define SAVE_DIFFICULTY_SIMULATION_ENTRY(sim_name) \
    result_blocks.back()[index] = timestamps_##sim_name.back(); \
    result_blocks.back()[index+1] = curren_difficulty_##sim_name.convert_to<uint64_t>(); \
    index +=2;

    SAVE_DIFFICULTY_SIMULATION_ENTRY(original);
    SAVE_DIFFICULTY_SIMULATION_ENTRY(new_720_5_60);
    SAVE_DIFFICULTY_SIMULATION_ENTRY(new_300_5_60);
    SAVE_DIFFICULTY_SIMULATION_ENTRY(new_200_5_5);

    result_blocks.back()[index] = b_line[2];
  }
  print_blocks(result_blocks, path + "result.txt");
  LOG_PRINT_L0("Done");
}


void run_difficulty_analysis(const std::string& path)
{
  //hash_rate_analysis(path);
  run_emulation(path);

}

void hash_rate_analysis(const std::string& path)
{

  //0 = height, 1 - timestamp, 2 - difficulty, 3 cumulative_diff
  std::vector<std::vector<uint64_t>> blocks;
  parse_file(path, blocks, 140000);

  LOG_PRINT_L0("Calculating hashrate...");
  std::stringstream ss;
  uint64_t curren_hashrate = 0;
  uint64_t step = 10;
  uint64_t hash_rate_range = 10;
  uint64_t second_windowf_or_hashrate = 20*60;
  
  for (size_t i = hash_rate_range; i != blocks.size(); i++)
  {
    
    if (i % step == 0)
    {
      //curren_hashrate = (blocks[i][3] - blocks[i - hash_rate_range][3])/(blocks[i][1] - blocks[i- hash_rate_range][1]);

//       uint64_t cumul_dif = 0;
//       for (size_t j = i; j != 0 && blocks[j][1] > blocks[i][1]- second_windowf_or_hashrate; j--)
//       {
//         cumul_dif += blocks[j][2];
//       }
//       curren_hashrate = cumul_dif / second_windowf_or_hashrate;

      curren_hashrate = (blocks[i][3] - blocks[i - hash_rate_range][3]) / (blocks[i][1] - blocks[i - hash_rate_range][1]);
      //std::setw(45) << epee::misc_utils::get_time_str(blocks[i][1])
      ss << std::left << std::setw(10) << i << std::left << std::setw(15) << blocks[i][1]  << std::left << std::setw(15) << blocks[i][2]/120 << std::left << std::setw(20) << curren_hashrate << ENDL;
    }
    //blocks[i][4] = curren_hashrate;
    //ss << std::left << std::setw(10) << i << std::left << std::setw(15) << blocks[i][2] << std::left << std::setw(20) << blocks[i][4] << ENDL;
  }
  std::string res_path = path + "hashrate.txt";
  file_io_utils::save_string_to_file(res_path, ss.str());
  LOG_PRINT_L0("Done, saved to file " << res_path);
  return;
}
