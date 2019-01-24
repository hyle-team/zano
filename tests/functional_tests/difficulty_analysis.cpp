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


void run_difficulty_analysis(const std::string& path)
{
  std::ifstream fstr(path);
  if (!fstr.good())
  {
    LOG_ERROR("unable to open " << path);
    return;
  }

  LOG_PRINT_L0("Loading array...");
  std::string line;
  std::vector<std::vector<uint64_t>> blocks;
  blocks.reserve(140000);
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

  LOG_PRINT_L0("Calculating hashrate...");
  std::stringstream ss;
  uint64_t curren_hashrate = 0;
  uint64_t step = 50;
  for (size_t i = 1; i != blocks.size(); i++)
  {
    
    if (i % step == 0 )
    {
      curren_hashrate = (blocks[i][3] - blocks[i - step][3])/(blocks[i][1] - blocks[i- step][1]);
      ss << std::left << std::setw(10) << i << std::left << std::setw(15) << blocks[i][1] << std::left << std::setw(15) << blocks[i][2] << std::left << std::setw(20) << curren_hashrate * 120 << ENDL;
    }
    //blocks[i][4] = curren_hashrate;
    //ss << std::left << std::setw(10) << i << std::left << std::setw(15) << blocks[i][2] << std::left << std::setw(20) << blocks[i][4] << ENDL;
  }
  std::string res_path = path + "hashrate.txt";
  file_io_utils::save_string_to_file(res_path, ss.str());
  LOG_PRINT_L0("Done, saved to file " << res_path);
  return;
}
