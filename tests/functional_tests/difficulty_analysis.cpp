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
#include "currency_core/basic_pow_helpers.h"

 
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
  if (length <= REDEF_DIFFICULTY_WINDOW - (REDEF_DIFFICULTY_CUT_OLD+ REDEF_DIFFICULTY_CUT_LAST))
  {
    cut_begin = 0;
    cut_end = length;
  }
  else
  {
    cut_begin = REDEF_DIFFICULTY_CUT_LAST;
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



currency::wide_difficulty_type bbr_next_difficulty_composit(std::vector<uint64_t>& timestamps, std::vector<currency::wide_difficulty_type>& cumulative_difficulties, size_t target_seconds, size_t REDEF_DIFFICULTY_WINDOW, size_t REDEF_DIFFICULTY_CUT_OLD, size_t REDEF_DIFFICULTY_CUT_LAST)
{
  sort(timestamps.begin(), timestamps.end(), std::greater<uint64_t>());
  std::vector<uint64_t> timestamps_local = timestamps;
  currency::wide_difficulty_type dif = bbr_next_difficulty_configurable(timestamps_local, cumulative_difficulties, target_seconds, REDEF_DIFFICULTY_WINDOW, REDEF_DIFFICULTY_CUT_OLD, REDEF_DIFFICULTY_CUT_LAST);
  currency::wide_difficulty_type dif2 = bbr_next_difficulty_configurable(timestamps_local, cumulative_difficulties, target_seconds, 240, 5, 5);
  currency::wide_difficulty_type dif3 = bbr_next_difficulty_configurable(timestamps_local, cumulative_difficulties, target_seconds, 40, 1, 1);
  return (dif3 + dif2 + dif) / 3;
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
      ss << std::left << std::setw(15) << blocks[i][j];
    }
    ss << std::left << std::setw(20) << blocks[i][blocks[i].size() - 1] << ENDL;
  }
  file_io_utils::save_string_to_file(res_path, ss.str());
  LOG_PRINT_L0("Done, saved to file " << res_path);
}

uint64_t get_hashrate_by_timestamp(const std::map<uint64_t, uint64_t> timestamp_to_hashrate, uint64_t timestamp)
{
  auto it = timestamp_to_hashrate.lower_bound(timestamp);
  if (it == timestamp_to_hashrate.end())
  {
    return 0;
  }
  if(it->first == timestamp)
    return it->second;

  if (it == timestamp_to_hashrate.begin())
  {
    LOG_ERROR("Internal error, lower_bound returned begin for timestamp " << timestamp);
    return 0;
  }

  return (--it)->second;;
}


template<typename cb_t>
void perform_simulation_for_function(const std::map<uint64_t, uint64_t>& timestamp_to_hashrate, uint64_t index_in_result, const std::vector<std::vector<uint64_t>>& blocks, std::vector<std::vector<uint64_t>>& result_blocks, cb_t cb)
{
  std::vector<uint64_t> timestamps;
  std::vector<currency::wide_difficulty_type> cumul_difficulties;
  timestamps.reserve(4010);
  cumul_difficulties.reserve(4010);
  timestamps.push_back(blocks[0][0]);
  cumul_difficulties.push_back(blocks[0][1] * 120);
  currency::wide_difficulty_type curren_difficulty = 0;

  size_t index_in_result_blocks = 0;
  while (true)
  {
    uint64_t hr = 0;
    for (size_t i = 0; i != 10; i++)
    {
      if (timestamps.size() < BBR_DIFFICULTY_WINDOW)
      {
        curren_difficulty = blocks[index_in_result_blocks][1] * 120;
      }
      else
      {
        std::vector<uint64_t> backward_timestamps;
        backward_timestamps.reserve(BBR_DIFFICULTY_WINDOW);
        std::copy(timestamps.rbegin(), timestamps.rbegin() + BBR_DIFFICULTY_WINDOW - 1, std::back_inserter(backward_timestamps));
        std::vector<currency::wide_difficulty_type> backward_cumul_difficulties;
        backward_cumul_difficulties.reserve(BBR_DIFFICULTY_WINDOW);
        std::copy(cumul_difficulties.rbegin(), cumul_difficulties.rbegin() + BBR_DIFFICULTY_WINDOW - 1, std::back_inserter(backward_cumul_difficulties));
        uint64_t ts = timestamps.back();
        curren_difficulty = cb(backward_timestamps, backward_cumul_difficulties, BBR_DIFFICULTY_TARGET);
      }
      cumul_difficulties.push_back(cumul_difficulties.back() + curren_difficulty);
      hr = get_hashrate_by_timestamp(timestamp_to_hashrate, timestamps.back());
      if (!hr)
        break;
      timestamps.push_back(get_next_timestamp_by_difficulty_and_hashrate(timestamps.back(), curren_difficulty, hr));
    }
    if (!hr)
      break;

    result_blocks[index_in_result_blocks][index_in_result] = timestamps.back();
    result_blocks[index_in_result_blocks][index_in_result + 1] = curren_difficulty.convert_to<uint64_t>() / 120;
    index_in_result_blocks++;
    std::cout << index_in_result_blocks << "\r";
  }
  if (index_in_result_blocks < 410)
  {
    for (size_t k = index_in_result_blocks; k != 410; k++)
      result_blocks[k][index_in_result] = result_blocks[k-1][index_in_result];
  }

  std::cout << "\n";
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
  result_blocks.resize(blocks.size() * 2);
  for (auto& b : result_blocks) {b.resize(20);}
  
  std::map<uint64_t, uint64_t> timestamp_to_hashrate;
  for (uint64_t b_no = 0; b_no != blocks.size(); b_no++)
  {
    auto& b_line = blocks[b_no];
    timestamp_to_hashrate[b_line[0]] = b_line[2];
    result_blocks[b_no][0] = b_line[0];
    result_blocks[b_no][1] = b_line[2];
  }

  uint64_t current_index = 2;

#define PERFORME_SIMULATION_FOR_FUNCTION(func_name, window_size, cut_old, cut_new ) \
  perform_simulation_for_function(timestamp_to_hashrate, current_index, blocks, result_blocks, \
    [&](std::vector<uint64_t>& timestamps, std::vector<currency::wide_difficulty_type>& cumulative_difficulties, size_t target_seconds) \
  { \
    return func_name(timestamps, cumulative_difficulties, target_seconds, window_size, cut_old, cut_new); \
  }); \
  current_index+=2;


#define PERFORME_SIMULATION_FOR_FUNCTION_NO_WINDOW(func_name) \
  perform_simulation_for_function(timestamp_to_hashrate, current_index, blocks, result_blocks, \
    [&](std::vector<uint64_t>& timestamps, std::vector<currency::wide_difficulty_type>& cumulative_difficulties, size_t target_seconds) \
  { \
    return func_name(timestamps, cumulative_difficulties, target_seconds, DIFFICULTY_POW_STARTER); \
  }); \
  current_index+=2;

  PERFORME_SIMULATION_FOR_FUNCTION(bbr_next_difficulty_configurable, BBR_DIFFICULTY_WINDOW, BBR_DIFFICULTY_CUT, BBR_DIFFICULTY_CUT);
  PERFORME_SIMULATION_FOR_FUNCTION(bbr_next_difficulty_configurable, 500, 60, 60);
  PERFORME_SIMULATION_FOR_FUNCTION(bbr_next_difficulty_configurable, 300, 60, 60);
  PERFORME_SIMULATION_FOR_FUNCTION_NO_WINDOW(currency::next_difficulty_1);

  print_blocks(result_blocks, path + "result.txt");
  LOG_PRINT_L0("Done");
}

void hash_rate_analysis(const std::string& path);

void run_difficulty_analysis(const std::string& path)
{
//   currency::block b = AUTO_VAL_INIT(b);
//   std::string s("sdsccasc");
//   b.miner_tx.extra.push_back(s);
// 
//   crypto::hash mining_hash = currency::null_hash;
//   bool r = string_tools::parse_tpod_from_hex_string("7759031ee0f014fe45476724df268f61c890b4a4637df1489a6c94c0135efbd8", mining_hash);
//   uint64_t nonce = 13704307308123612296;
//   crypto::hash pow_hash = currency::get_block_longhash(6, mining_hash, nonce);
//   
//   std::cout << mining_hash << ENDL;
//   std::cout << pow_hash << ENDL;

  //crypto::hash mining_hash = currency::get_block_header_mining_hash(b);
  //crypto::hash id_hash = currency::get_block_hash(b);



  hash_rate_analysis(path);
  //run_emulation(path);

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
