// Copyright (c) 2014-2020 Zano Project
// Copyright (c) 2014-2017 The The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

//
// std
//
#include <assert.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <fstream>
#include <set>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <numeric>
#include <atomic>

#include "warnings.h" // epee header for warnings disable aid

//
// boost
//
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/archive/basic_archive.hpp>
PUSH_VS_WARNINGS
  DISABLE_VS_WARNINGS(4800)
  #include <boost/archive/basic_binary_iarchive.hpp>
POP_VS_WARNINGS
#include <boost/archive/basic_binary_iprimitive.hpp>
#include <boost/archive/basic_binary_oarchive.hpp>
#include <boost/archive/basic_binary_oprimitive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/detail/polymorphic_oarchive_route.hpp>
#include <boost/atomic.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/limits.hpp>
#include <boost/locale.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/unique.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/shared_ptr_helper.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/thread.hpp>
#include <boost/timer/timer.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/value_init.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/version.hpp>

//
// epee
//
#include "include_base_utils.h"
#include "misc_language.h"
#include "misc_log_ex.h"

#include "cache_helper.h"
#include "copyable_atomic.h"
#include "file_io_utils.h"
#include "math_helper.h"
#include "net/net_utils_base.h"
#include "net/rpc_method_name.h"
#include "profile_tools.h"
#include "readwrite_lock.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "string_tools.h"
#include "syncobj.h"
#include "zlib_helper.h"

//
// contrib
//
#include "eos_portable_archive/eos/portable_archive.hpp"
#include "db/liblmdb/lmdb.h"
