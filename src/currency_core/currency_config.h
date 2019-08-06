// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#define CURRENCY_FORMATION_VERSION                      84
#define CURRENCY_GENESIS_NONCE                          (CURRENCY_FORMATION_VERSION + 101011010121) //bender's nightmare

                                                        
#define CURRENCY_MAX_BLOCK_NUMBER                       500000000
#define CURRENCY_MAX_BLOCK_SIZE                         500000000  // block header blob limit, never used!
#define CURRENCY_TX_MAX_ALLOWED_OUTS                    2000
#define CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX           197   // addresses start with 'Z'
#define CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX     0x3678 // integrated addresses start with 'iZ'
#define CURRENCY_MINED_MONEY_UNLOCK_WINDOW              10
#define CURRENT_TRANSACTION_VERSION                     1
#define CURRENT_BLOCK_MAJOR_VERSION                     2
#define CURRENT_BLOCK_MINOR_VERSION                     0
#define CURRENCY_BLOCK_FUTURE_TIME_LIMIT                60*60*2
#define CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT            60*20
                                                        
#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW               60


// TOTAL_MONEY_SUPPLY - total number coins to be generated
#define TOTAL_MONEY_SUPPLY                              ((uint64_t)(-1))
                                                        
#define POS_START_HEIGHT                                0
                                                        
#define CURRENCY_REWARD_BLOCKS_WINDOW                   400
#define CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE         125000 //size of block (bytes) after which reward for block calculated using block size
#define CURRENCY_COINBASE_BLOB_RESERVED_SIZE            1100
#define CURRENCY_MAX_TRANSACTION_BLOB_SIZE              (CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE - CURRENCY_COINBASE_BLOB_RESERVED_SIZE*2) 
#define CURRENCY_FREE_TX_MAX_BLOB_SIZE                  1024 // soft txpool-based limit for free-of-charge txs (such as BC_OFFERS_SERVICE_INSTRUCTION_DEL)
#define CURRENCY_DISPLAY_DECIMAL_POINT                  12

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)1000000000000) // pow(10, CURRENCY_DISPLAY_DECIMAL_POINT)
#define BASE_REWARD_DUST_THRESHOLD                      ((uint64_t)1000000) // pow(10, 6) - change this will cause hard-fork!
#define DEFAULT_DUST_THRESHOLD                          ((uint64_t)0)

#define TX_DEFAULT_FEE                                  ((uint64_t)10000000000) // .01
#define TX_MINIMUM_FEE                                  ((uint64_t)10000000000) // .01

#define CURRENCY_BLOCK_REWARD                           1000000000000 // 1.0 coin == pow(10, CURRENCY_DISPLAY_DECIMAL_POINT)


#define WALLET_MAX_ALLOWED_OUTPUT_AMOUNT                ((uint64_t)0xffffffffffffffffLL)
#define CURRENCY_MINER_TX_MAX_OUTS                      CURRENCY_TX_MAX_ALLOWED_OUTS

#define DIFFICULTY_STARTER                              1
#define DIFFICULTY_POS_TARGET                           120 // seconds
#define DIFFICULTY_POW_TARGET                           120 // seconds
#define DIFFICULTY_TOTAL_TARGET                         ((DIFFICULTY_POS_TARGET + DIFFICULTY_POW_TARGET) / 4)
#define DIFFICULTY_WINDOW                               720 // blocks
#define DIFFICULTY_LAG                                  15  // !!!
#define DIFFICULTY_CUT                                  60  // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT                         (DIFFICULTY_WINDOW + DIFFICULTY_LAG)

#define CURRENCY_BLOCKS_PER_DAY                          ((60*60*24)/(DIFFICULTY_TOTAL_TARGET))


#define TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW            20
#define TX_EXPIRATION_MEDIAN_SHIFT                      ((TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW)/2)*DIFFICULTY_TOTAL_TARGET

#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS        (DIFFICULTY_TOTAL_TARGET * CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS)
#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS         1

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TOTAL_TARGET //just alias

#define MAX_ALIAS_PER_BLOCK                             1000
#define ALIAS_COAST_PERIOD                              CURRENCY_BLOCKS_PER_DAY*7 //week
#define ALIAS_COAST_RECENT_PERIOD                       CURRENCY_BLOCKS_PER_DAY*8 //week + 1 day (we guarantee split depth at least 1 day)
#define ALIAS_VERY_INITAL_COAST                         ((uint64_t)10000) // to avoid split when default fee changed
#define ALIAS_MEDIAN_RECALC_INTERWAL                    CURRENCY_BLOCKS_PER_DAY


#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          2000      //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              200       //by default, blocks count in blocks downloading
#define BLOCKS_SYNCHRONIZING_DEFAULT_SIZE               2000000   //by default keep synchronizing packets not bigger then 2MB
#define CURRENCY_PROTOCOL_MAX_BLOCKS_REQUEST_COUNT      500     
#define CURRENCY_PROTOCOL_MAX_TXS_REQUEST_COUNT         500    


#define CURRENCY_ALT_BLOCK_LIVETIME_COUNT               (CURRENCY_BLOCKS_PER_DAY*7)//one week
#define CURRENCY_ALT_BLOCK_MAX_COUNT                    43200 //30 days
#define CURRENCY_MEMPOOL_TX_LIVETIME                    345600 //seconds, 4 days


#ifndef TESTNET
#define P2P_DEFAULT_PORT                                11121
#define RPC_DEFAULT_PORT                                11211
#define STRATUM_DEFAULT_PORT                            11777
#define P2P_NETWORK_ID_TESTNET_FLAG                     0
#else 
#define P2P_DEFAULT_PORT                                (11112)
#define RPC_DEFAULT_PORT                                12111
#define STRATUM_DEFAULT_PORT                            11888
#define STRARUM_DEFAULT_PORT                            51113
#define P2P_NETWORK_ID_TESTNET_FLAG                     1
#endif

#define P2P_NETWORK_ID_VER                              (CURRENCY_FORMATION_VERSION+0)

#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT           4000

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT                  1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT                   5000

#define P2P_DEFAULT_CONNECTIONS_COUNT                   8
#define P2P_DEFAULT_HANDSHAKE_INTERVAL                  60           //seconds
#define P2P_DEFAULT_PACKET_MAX_SIZE                     50000000     //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE                  250
#define P2P_DEFAULT_CONNECTION_TIMEOUT                  5000       //5 seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT             2000       //2 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT                      60*2*1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT            10000      //10 seconds
#define P2P_MAINTAINERS_PUB_KEY                         "8f138bb73f6d663a3746a542770781a09579a7b84cb4125249e95530824ee607"
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       70
#define P2P_FAILED_ADDR_FORGET_SECONDS                  (60*5)     //5 minutes

#define P2P_IP_BLOCKTIME                                (60*60*24) //24 hours
#define P2P_IP_FAILS_BEFOR_BLOCK                        10
#define P2P_IDLE_CONNECTION_KILL_INTERVAL               (5*60) //5 minutes

//PoS definitions
#define POS_SCAN_WINDOW                                 60*10 //seconds // 10 minutes
#define POS_SCAN_STEP                                   15    //seconds
#define POS_MAX_ACTUAL_TIMESTAMP_TO_MINED               (POS_SCAN_WINDOW+100)                       

#define POS_STARTER_KERNEL_HASH                         "00000000000000000006382a8d8f94588ce93a1351924f6ccb9e07dd287c6e4b"
#define POS_MODFIFIER_INTERVAL                          10
#define POS_WALLET_MINING_SCAN_INTERVAL                 POS_SCAN_STEP  //seconds
#define POS_MINIMUM_COINSTAKE_AGE                       10 // blocks count


#define WALLET_FILE_SIGNATURE                           0x1111012101101011LL  //Bender's nightmare
#define WALLET_FILE_MAX_BODY_SIZE                       0x88888888L //2GB
#define WALLET_FILE_MAX_KEYS_SIZE                       10000 //
#define WALLET_BRAIN_DATE_OFFSET                        1543622400
#define WALLET_BRAIN_DATE_QUANTUM                       604800 //by last word we encode a number of week since launch of the project, 
                                                               //which let us to address tools::mnemonic_encoding::NUMWORDS weeks after project launch
                                                               //which is about 30 years

#define OFFER_MAXIMUM_LIFE_TIME                         (60*60*24*30)  // 30 days

#define GUI_BLOCKS_DISPLAY_COUNT                        40
#define GUI_DISPATCH_QUE_MAXSIZE                        100

#define ALLOW_DEBUG_COMMANDS



#define CURRENCY_NAME_ABR                               "ZANO"
#define CURRENCY_NAME_BASE                              "Zano"
#define CURRENCY_NAME_SHORT_BASE                        "Zano"
#ifndef TESTNET
#define CURRENCY_NAME                                   CURRENCY_NAME_BASE
#define CURRENCY_NAME_SHORT                             CURRENCY_NAME_SHORT_BASE
#else
#define CURRENCY_NAME                                   CURRENCY_NAME_BASE"_testnet"
#define CURRENCY_NAME_SHORT                             CURRENCY_NAME_SHORT_BASE"_testnet"
#endif

//premine
#define PREMINE_AMOUNT                                  (17517203000000000000U) // 13827203.0 reserved for coinswap, 3690000.0 - premine  

//alias registration wallet
#define ALIAS_REWARDS_ACCOUNT_SPEND_PUB_KEY             "0000000000000000000000000000000000000000000000000000000000000000" //burn alias money
#define ALIAS_REWARDS_ACCOUNT_VIEW_PUB_KEY              "0000000000000000000000000000000000000000000000000000000000000000" //burn alias money
#define ALIAS_REWARDS_ACCOUNT_VIEW_SEC_KEY              "0000000000000000000000000000000000000000000000000000000000000000" //burn alias money

#define ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED         6
#define ALIAS_SHORT_NAMES_VALIDATION_PUB_KEY            "37947f7b6a5268c5d0a48bde73d7a426f0b5f24648f74024279540207dc70031" 


#define ALIAS_NAME_MAX_LEN                              255
#define ALIAS_VALID_CHARS                               "0123456789abcdefghijklmnopqrstuvwxyz-."
#define ALIAS_COMMENT_MAX_SIZE_BYTES                     400

#define CURRENCY_CORE_INSTANCE_LOCK_FILE                "lock.lck"


#define CURRENCY_POOLDATA_FOLDERNAME                    "poolstate"
#define CURRENCY_BLOCKCHAINDATA_FOLDERNAME              "blockchain"
#define P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define MINER_CONFIG_FILENAME                           "miner_conf.json"
#define GUI_SECURE_CONFIG_FILENAME                      "gui_secure_conf.bin"
#define GUI_CONFIG_FILENAME                             "gui_settings.json"
#define GUI_INTERNAL_CONFIG                             "gui_internal_config.bin"



#define CURRENT_TRANSACTION_CHAIN_ENTRY_ARCHIVE_VER     3
#define CURRENT_BLOCK_EXTENDED_INFO_ARCHIVE_VER         1

#define BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION  CURRENCY_FORMATION_VERSION + 8
#define BLOCKCHAIN_STORAGE_MINOR_COMPATIBILITY_VERSION  1


#define BC_OFFERS_CURRENT_OFFERS_SERVICE_ARCHIVE_VER    CURRENCY_FORMATION_VERSION + BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION + 9
#define BC_OFFERS_CURRENCY_MARKET_FILENAME              "market.bin"


#define WALLET_FILE_SERIALIZATION_VERSION               (CURRENCY_FORMATION_VERSION+65)

#define CURRENT_MEMPOOL_ARCHIVE_VER                     (CURRENCY_FORMATION_VERSION+31)

//hard forks section
#define BLOCK_MAJOR_VERSION_INITAL                     1
#ifndef TESTNET
#define ZANO_HARDFORK_1_AFTER_HEIGHT               120000
#else
#define ZANO_HARDFORK_1_AFTER_HEIGHT               62102
#endif



static_assert(CURRENCY_MINER_TX_MAX_OUTS <= CURRENCY_TX_MAX_ALLOWED_OUTS, "Miner tx must obey normal tx max outs limit");
static_assert(PREMINE_AMOUNT / WALLET_MAX_ALLOWED_OUTPUT_AMOUNT < CURRENCY_MINER_TX_MAX_OUTS, "Premine can't be divided into reasonable number of outs");

