// Copyright (c) 2022-2025 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#pragma once

// hash domain separation strings, 32 bytes long (31 chars + \0)
//

#define CRYPTO_HDS_OUT_AMOUNT_MASK                  "ZANO_HDS_OUT_AMOUNT_MASK_______"
#define CRYPTO_HDS_OUT_AMOUNT_BLINDING_MASK         "ZANO_HDS_OUT_AMOUNT_BLIND_MASK_"
#define CRYPTO_HDS_OUT_ASSET_BLINDING_MASK          "ZANO_HDS_OUT_ASSET_BLIND_MASK__"
#define CRYPTO_HDS_OUT_CONCEALING_POINT             "ZANO_HDS_OUT_CONCEALING_POINT__"
                                                    
#define CRYPTO_HDS_CLSAG_GG_LAYER_0                 "ZANO_HDS_CLSAG_GG_LAYER_ZERO___"
#define CRYPTO_HDS_CLSAG_GG_LAYER_1                 "ZANO_HDS_CLSAG_GG_LAYER_ONE____"
#define CRYPTO_HDS_CLSAG_GG_CHALLENGE               "ZANO_HDS_CLSAG_GG_CHALLENGE____"
                                                    
#define CRYPTO_HDS_CLSAG_GGX_LAYER_0                "ZANO_HDS_CLSAG_GGX_LAYER_ZERO__"
#define CRYPTO_HDS_CLSAG_GGX_LAYER_1                "ZANO_HDS_CLSAG_GGX_LAYER_ONE___"
#define CRYPTO_HDS_CLSAG_GGX_LAYER_2                "ZANO_HDS_CLSAG_GGX_LAYER_TWO___"
#define CRYPTO_HDS_CLSAG_GGX_CHALLENGE              "ZANO_HDS_CLSAG_GGX_CHALLENGE___"
                                                    
#define CRYPTO_HDS_CLSAG_GGXG_LAYER_0               "ZANO_HDS_CLSAG_GGXG_LAYER_ZERO_"
#define CRYPTO_HDS_CLSAG_GGXG_LAYER_1               "ZANO_HDS_CLSAG_GGXG_LAYER_ONE__"
#define CRYPTO_HDS_CLSAG_GGXG_LAYER_2               "ZANO_HDS_CLSAG_GGXG_LAYER_TWO__"
#define CRYPTO_HDS_CLSAG_GGXG_LAYER_3               "ZANO_HDS_CLSAG_GGXG_LAYER_THREE"
#define CRYPTO_HDS_CLSAG_GGXG_CHALLENGE             "ZANO_HDS_CLSAG_GGXG_CHALLENGE__"
                                                    
#define CRYPTO_HDS_CLSAG_GGXXG_LAYER_0              "ZANO_HDS_CLSAG_GGXXG_LAYER_ZERO"
#define CRYPTO_HDS_CLSAG_GGXXG_LAYER_1              "ZANO_HDS_CLSAG_GGXXG_LAYER_ONE_"
#define CRYPTO_HDS_CLSAG_GGXXG_LAYER_2              "ZANO_HDS_CLSAG_GGXXG_LAYER_TWO_"
#define CRYPTO_HDS_CLSAG_GGXXG_LAYER_3              "ZANO_HDS_CLSAG_GGXXG_LAYER_3___"
#define CRYPTO_HDS_CLSAG_GGXXG_LAYER_4              "ZANO_HDS_CLSAG_GGXXG_LAYER_FOUR"
#define CRYPTO_HDS_CLSAG_GGXXG_CHALLENGE            "ZANO_HDS_CLSAG_GGXXG_CHALLENGE_"
                                                    
#define CRYPTO_HDS_ZARCANUM_LAST_POW_HASH           "ZANO_HDS_ZARCANUM_LAST_POW_HASH"
#define CRYPTO_HDS_ZARCANUM_PROOF_HASH              "ZANO_HDS_ZARCANUM_PROOF_HASH___"
                                                    
#define CRYPTO_HDS_ASSET_CONTROL_KEY                "ZANO_HDS_ASSET_CONTROL_KEY_____"
#define CRYPTO_HDS_ASSET_CONTROL_ABM                "ZANO_HDS_ASSET_CONTROL_ABM_____"
#define CRYPTO_HDS_ASSET_ID                         "ZANO_HDS_ASSET_ID______________"
#define CRYPTO_HDS_DETERMINISTIC_TX_KEY             "ZANO_HDS_DETERMINISTIC_TX_KEY__"

#define CRYPTO_HDS_CHACHA_TX_PAYLOAD_ITEMS          "ZANO_HDS_CHACHA_TX_PAYLOAD_ITEM"
#define CRYPTO_HDS_CHACHA_TX_PI_ISOLATE_AUDITABLE   "ZANO_HDS_CHACHA_TX_PI_ISLT_AUDT"
#define CRYPTO_HDS_CHACHA_TX_PAYLOAD_SENDER_DER     "ZANO_HDS_CHACHA_TX_PI_SENDR_DER"
