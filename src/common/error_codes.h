// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "../currency_core/basic_api_response_codes.h"

#define API_RETURN_CODE_OK                                      BASIC_RESPONSE_STATUS_OK
#define API_RETURN_CODE_FAIL                                    BASIC_RESPONSE_STATUS_FAILED
#define API_RETURN_CODE_NOT_FOUND                               BASIC_RESPONSE_STATUS_NOT_FOUND
#define API_RETURN_CODE_ACCESS_DENIED                           "ACCESS_DENIED"
#define API_RETURN_CODE_INTERNAL_ERROR                          "INTERNAL_ERROR"
#define API_RETURN_CODE_NOT_ENOUGH_MONEY                        "NOT_ENOUGH_MONEY"
#define API_RETURN_CODE_NOT_ENOUGH_OUTPUTS_FOR_MIXING           "NOT_ENOUGH_OUTPUTS_FOR_MIXING"
#define API_RETURN_CODE_INTERNAL_ERROR_QUE_FULL                 "INTERNAL_ERROR_QUE_FULL"
#define API_RETURN_CODE_BAD_ARG                                 "BAD_ARG"
#define API_RETURN_CODE_BAD_ARG_EMPTY_DESTINATIONS              "BAD_ARG_EMPTY_DESTINATIONS"
#define API_RETURN_CODE_BAD_ARG_WRONG_FEE                       "BAD_ARG_WRONG_FEE"
#define API_RETURN_CODE_BAD_ARG_INVALID_ADDRESS                 "BAD_ARG_INVALID_ADDRESS"
#define API_RETURN_CODE_BAD_ARG_WRONG_AMOUNT                    "BAD_ARG_WRONG_AMOUNT"
#define API_RETURN_CODE_BAD_ARG_WRONG_PAYMENT_ID                "BAD_ARG_WRONG_PAYMENT_ID"
#define API_RETURN_CODE_WRONG_PASSWORD                          "WRONG_PASSWORD"
#define API_RETURN_CODE_WALLET_WRONG_ID                         "WALLET_WRONG_ID"
#define API_RETURN_CODE_WALLET_WATCH_ONLY_NOT_SUPPORTED         "WALLET_WATCH_ONLY_NOT_SUPPORTED"
#define API_RETURN_CODE_WALLET_AUDITABLE_NOT_SUPPORTED          "WALLET_AUDITABLE_NOT_SUPPORTED"
#define API_RETURN_CODE_WALLET_FEE_TOO_LOW                      "API_RETURN_CODE_WALLET_FEE_TOO_LOW"
#define API_RETURN_CODE_FILE_NOT_FOUND                          "FILE_NOT_FOUND"
#define API_RETURN_CODE_ALREADY_EXISTS                          "ALREADY_EXISTS"
#define API_RETURN_CODE_CANCELED                                "CANCELED"
#define API_RETURN_CODE_FILE_RESTORED                           "FILE_RESTORED"
#define API_RETURN_CODE_TRUE                                    "TRUE"
#define API_RETURN_CODE_FALSE                                   "FALSE"
#define API_RETURN_CODE_CORE_BUSY                               "CORE_BUSY"
#define API_RETURN_CODE_OVERFLOW                                "OVERFLOW"
#define API_RETURN_CODE_BUSY                                    "BUSY"
#define API_RETURN_CODE_INVALID_FILE                            "INVALID_FILE"
#define API_RETURN_CODE_WRONG_SEED                              "WRONG_SEED"
#define API_RETURN_CODE_GENESIS_MISMATCH                        "GENESIS_MISMATCH"
#define API_RETURN_CODE_DISCONNECTED                            "DISCONNECTED"
#define API_RETURN_CODE_UNINITIALIZED                           "UNINITIALIZED"
#define API_RETURN_CODE_TX_IS_TOO_BIG                           "TX_IS_TOO_BIG"
#define API_RETURN_CODE_TX_REJECTED                             "TX_REJECTED"
#define API_RETURN_CODE_HTLC_ORIGIN_HASH_MISSMATCHED            "HTLC_ORIGIN_HASH_MISSMATCHED"
#define API_RETURN_CODE_WRAP                                    "WRAP"
#define API_RETURN_CODE_MISSING_ZC_INPUTS                       "MISSING_ZC_INPUTS"
