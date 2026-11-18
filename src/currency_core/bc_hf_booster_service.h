// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "account.h"
#include "serialization/serialization.h"
#include "serialization/binary_archive.h"
#include "serialization/json_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/variant.h"
#include "serialization/binary_utils.h"

#define BC_HF_BOOSTER_SERVICE_ID                                "HF_BOOSTER"
#define BC_HF_BOOSTER_SERVICE_INSTRUCTION_UPDATE                "HF_UPDATE"

namespace bc_services
{



  struct hf_booster_update
  {
    uint64_t hf_id = 0;
    uint64_t hf_new_activation_height = 0;
    crypto::public_key tx_onetime_pub_key = AUTO_VAL_INIT(tx_onetime_pub_key);
    crypto::generic_schnorr_sig_s maintainer_signature = AUTO_VAL_INIT(maintainer_signature);
    uint32_t version                                   = 0;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_N(hf_id, "hf_id")
      KV_SERIALIZE_N(hf_new_activation_height, "hf_new_activation_height")
      KV_SERIALIZE_POD_AS_HEX_STRING_N(tx_onetime_pub_key, "tx_onetime_pub_key")
      KV_SERIALIZE_POD_AS_HEX_STRING_N(maintainer_signature, "maintainer_signature")
      KV_SERIALIZE_N(version, "version")
    END_KV_SERIALIZE_MAP()

    // this map needed exlusively to generate blob to sign for in proof
    BEGIN_VERSIONED_SERIALIZE(0, version)
      FIELD(hf_id)
      FIELD(hf_new_activation_height)
      FIELD(tx_onetime_pub_key)
    END_SERIALIZE()

  };

  inline
  crypto::hash get_hf_booster_update_hash(const hf_booster_update& update)
  {
    crypto::hash result = AUTO_VAL_INIT(result);
    std::string blob;
    bool r = ::t_serializable_object_to_blob(update, blob);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to serialize hf_booster_update");
    crypto::cn_fast_hash(blob.data(), blob.size(), result);
    return result;
  }


}

