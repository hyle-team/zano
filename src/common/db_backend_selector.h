// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "db_backend_lmdb.h"
#include "db_backend_mdbx.h"

namespace tools
{
  namespace db
  {
#ifdef DB_ENGINE_LMDB
    typedef lmdb_db_backend default_db_backend;
#elif DB_ENGINE_MDBX
    typedef mdbx_db_backend default_db_backend;
#endif
  }
}