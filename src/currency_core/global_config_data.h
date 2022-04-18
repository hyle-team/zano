// Copyright (c) 2014-2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once



namespace currency
{
  struct global_config_data
  {
    global_config_data(): pvm(&vm_stub)
    {}

    const boost::program_options::variables_map* pvm;
    std::string data_storage_path;
    
    const boost::program_options::variables_map& vm_stub;
  };
}

