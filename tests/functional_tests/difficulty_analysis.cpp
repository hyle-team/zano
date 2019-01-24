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

    blocks.push_back(array_num);
  }
  LOG_PRINT_L0("Loaded " << blocks.size() << " lines");


  return;
}
