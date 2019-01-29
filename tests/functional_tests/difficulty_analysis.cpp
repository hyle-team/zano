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


void run_difficulty_analysis(const std::string& path)
{
  //hash_rate_analysis(path);
  run_emulation(path);

}

void run_emulation(const std::string& path)
{
  //0 - timestamp, 1 - difficulty/120, 2 net hashrate (h/s)

  std::vector<std::vector<uint64_t>> blocks;
  parse_file(path, blocks, 500);



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
