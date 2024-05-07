// Copyright (c) 2018-2019 Zano Project
// Copyright (c) 2018-2019 Hyle Team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "include_base_utils.h"
using namespace epee;

#include "basic_pow_helpers.h"
#include "currency_format_utils.h"
#include "serialization/binary_utils.h"
#include "serialization/stl_containers.h"
#include "currency_core/currency_config.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "common/int-util.h"
#include "ethereum/libethash/ethash/ethash.hpp"
#include "ethereum/libethash/ethash/progpow.hpp"

namespace currency
{

  //--------------------------------------------------------------
  int ethash_custom_log_get_level()
  {
    return epee::log_space::get_set_log_detalisation_level();
  }
  //--------------------------------------------------------------
  void ethash_custom_log(const std::string& m, bool add_callstack)
  {
    std::string msg = epee::log_space::log_singletone::get_prefix_entry() + "[ETHASH]" + m;
    if (add_callstack)
      msg = msg + "callstask: " + epee::misc_utils::get_callstack();

    epee::log_space::log_singletone::do_log_message(msg, LOG_LEVEL_0, epee::log_space::console_color_default, true, LOG_DEFAULT_TARGET);
  }
  //--------------------------------------------------------------
  void init_ethash_log_if_necessary()
  {
    static bool inited = false;
    if (inited)
      return;

    ethash::access_custom_log_level_function() = &ethash_custom_log_get_level;
    ethash::access_custom_log_function() = &ethash_custom_log;

    inited = true;
  }
  //------------------------------------------------------------------
  int ethash_height_to_epoch(uint64_t height)
  {
    return static_cast<int>(height / ETHASH_EPOCH_LENGTH);
  }
  //--------------------------------------------------------------
  crypto::hash ethash_epoch_to_seed(int epoch)
  {
    auto res_eth = ethash_calculate_epoch_seed(epoch);
    crypto::hash result = currency::null_hash;
    memcpy(&result.data, &res_eth, sizeof(res_eth));
    return result;
  }
  //--------------------------------------------------------------
  crypto::hash get_block_longhash(uint64_t height, const crypto::hash& block_header_hash, uint64_t nonce)
  {
    init_ethash_log_if_necessary();
    int epoch = ethash_height_to_epoch(height);
    std::shared_ptr<ethash::epoch_context_full> p_context = progpow::get_global_epoch_context_full(static_cast<int>(epoch));
    if (!p_context)
    {
      LOG_ERROR("fatal error: get_global_epoch_context_full failed, throwing bad_alloc...");
      throw std::bad_alloc();
    }
    auto res_eth = progpow::hash(*p_context,  static_cast<int>(height), *(ethash::hash256*)&block_header_hash, nonce);
    crypto::hash result = currency::null_hash;
    memcpy(&result.data, &res_eth.final_hash, sizeof(res_eth.final_hash));
    return result;
  }
  //---------------------------------------------------------------
  crypto::hash get_block_header_mining_hash(const block& b)
  {
    blobdata bd = get_block_hashing_blob(b);

    access_nonce_in_block_blob(bd) = 0;
    return crypto::cn_fast_hash(bd.data(), bd.size());
  }
  //---------------------------------------------------------------
  void get_block_longhash(const block& b, crypto::hash& res)
  {
    /*
    Since etherium hash has a bit different approach in minig, to adopt our code we made little hack:
    etherium hash calculates from block's hash and(!) nonce, both passed into PoW hash function.
    To achieve the same effect we make blob of data from block in normal way, but then set to zerro nonce
    inside serialized buffer, and then pass this nonce to ethash algo as a second argument, as it expected.
    */
    crypto::hash bl_hash = get_block_header_mining_hash(b);
    res = get_block_longhash(get_block_height(b), bl_hash, b.nonce);
  }
  //---------------------------------------------------------------
  crypto::hash get_block_longhash(const block& b)
  {
    crypto::hash p = null_hash;
    get_block_longhash(b, p);
    return p;
  }
}