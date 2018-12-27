// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <boost/program_options.hpp>
#include "common/command_line.h"

bool core_concurrency_test(boost::program_options::variables_map& vm, size_t wthreads, size_t rthreads, size_t blocks_count);

extern const command_line::arg_descriptor<std::string> arg_test_core_prepare_and_store;
extern const command_line::arg_descriptor<std::string> arg_test_core_load_and_replay;
