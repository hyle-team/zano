// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "account.h"

#define BC_HF_BOOSTER_SERVICE_ID                                "HF_BOOSTER"
#define BC_HF_BOOSTER_SERVICE_INSTRUCTION_UPDATE                "HF_UPDATE"

namespace bc_services
{



  struct hf_booster_update
  {
    uint64_t hf_id = 0;
    uint64_t hf_new_activation_height = 0;
    crypto::public_key tx_onetime_pub_key = AUTO_VAL_INIT(tx_onetime_pub_key);
    crypto::signature maintainer_signature = AUTO_VAL_INIT(maintainer_signature);

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_N(hf_id, "hf_id")
    KV_SERIALIZE_N(hf_new_activation_height, "hf_new_activation_height")
      KV_SERIALIZE_N(tx_onetime_pub_key, "tx_onetime_pub_key")
      KV_SERIALIZE_N(maintainer_signature, "maintainer_signature")
    END_KV_SERIALIZE_MAP()
  };

  struct hf_booster_update_proof
  {
    uint64_t hf_id                        = 0;
    uint64_t hf_new_height                = 0;
    crypto::public_key tx_onetime_pub_key = AUTO_VAL_INIT(tx_onetime_pub_key);
    uint32_t version                      = 0;

    BEGIN_VERSIONED_SERIALIZE(0, version)
      FIELD(hf_id)
      FIELD(hf_new_height)
      FIELD(tx_onetime_pub_key)
    END_SERIALIZE()
  };

}

