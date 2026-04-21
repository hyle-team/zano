// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2015 Boolberry developers
// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

namespace gui_tools
{
  bool GetStartOnSystemStartup();
  bool SetStartOnSystemStartup(bool fAutoStart);
  bool calculate_html_folder_hash(const std::string& html_path, std::string& result_hash);
}