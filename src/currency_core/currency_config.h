// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#ifndef TESTNET
#define CURRENCY_FORMATION_VERSION                      84
#else
#define CURRENCY_FORMATION_VERSION                      99
#endif

#define CURRENCY_GENESIS_NONCE                          (CURRENCY_FORMATION_VERSION + 101011010121) //bender's nightmare


                                                        
#define CURRENCY_MAX_BLOCK_NUMBER                       500000000
#define CURRENCY_MAX_BLOCK_SIZE                         500000000  // block header blob limit, never used!
#define CURRENCY_TX_MAX_ALLOWED_INPUTS                  256        // limited primarily by asset surjection proof
#define CURRENCY_TX_MAX_ALLOWED_OUTS                    2000
#define CURRENCY_TX_MIN_ALLOWED_OUTS                    2      // effective starting HF4 Zarcanum
#define CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX           0xc5   // addresses start with 'Zx'
#define CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX     0x3678 // integrated addresses start with 'iZ'
#define CURRENCY_PUBLIC_INTEG_ADDRESS_V2_BASE58_PREFIX  0x36f8 // integrated addresses start with 'iZ' (new format)
#define CURRENCY_PUBLIC_AUDITABLE_ADDRESS_BASE58_PREFIX 0x98c8 // auditable addresses start with 'aZx'
#define CURRENCY_PUBLIC_AUDITABLE_INTEG_ADDRESS_BASE58_PREFIX 0x8a49 // auditable integrated addresses start with 'aiZX'
#define CURRENCY_MINED_MONEY_UNLOCK_WINDOW              10
#define CURRENT_TRANSACTION_VERSION                     2
#define TRANSACTION_VERSION_INITAL                      0
#define TRANSACTION_VERSION_PRE_HF4                     1
#define TRANSACTION_VERSION_POST_HF4                    2 
#define HF1_BLOCK_MAJOR_VERSION                         1
#define HF3_BLOCK_MAJOR_VERSION                         2
#define HF3_BLOCK_MINOR_VERSION                         0
#define CURRENT_BLOCK_MAJOR_VERSION                     3

#define CURRENCY_DEFAULT_DECOY_SET_SIZE                 10
#define CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE           15
#define CURRENCY_HF4_MANDATORY_MIN_COINAGE              10

#define CURRENT_BLOCK_MINOR_VERSION                     0
#define CURRENCY_BLOCK_FUTURE_TIME_LIMIT                60*60*2
#define CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT            60*20
                                                        
#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW               60
                                                        
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
#define CURRENCY_TX_OUTS_RND_SPLIT_DIGITS_TO_KEEP       3

#define DIFFICULTY_POW_STARTER                          1
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
#define P2P_MAINTAINERS_PUB_KEY                         "8f138bb73f6d663a3746a542770781a09579a7b84cb4125249e95530824ee607"
#define DIFFICULTY_POS_STARTER                          1
#else 
#define P2P_DEFAULT_PORT                                (11211 + CURRENCY_FORMATION_VERSION)
#define RPC_DEFAULT_PORT                                12111
#define STRATUM_DEFAULT_PORT                            11888
#define STRARUM_DEFAULT_PORT                            51113
#define P2P_NETWORK_ID_TESTNET_FLAG                     1
#define P2P_MAINTAINERS_PUB_KEY                         "aaa2d7aabc8d383fd53a3ae898697b28f236ceade6bafc1eecff413a6a02272a"
#define DIFFICULTY_POS_STARTER                          1
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
#define POS_MAX_DIFFICULTY_ALLOWED                      "25000000000000000000000" // maximum expected PoS difficuty (need to change it probaly in 20 years)


#ifndef TESTNET
#  define BLOCKCHAIN_HEIGHT_FOR_POS_STRICT_SEQUENCE_LIMITATION 57000
#else
#  define BLOCKCHAIN_HEIGHT_FOR_POS_STRICT_SEQUENCE_LIMITATION 18000
#endif
#define BLOCK_POS_STRICT_SEQUENCE_LIMIT                 20  // the highest allowed sequence factor for a PoS block (i.e., the max total number of sequential PoS blocks is BLOCK_POS_STRICT_SEQUENCE_LIMIT + 1)


#define CORE_FEE_BLOCKS_LOOKUP_WINDOW                   60  //number of blocks used to check if transaction flow is big enought to rise default fee

#define WALLET_FILE_SIGNATURE_OLD                       0x1111012101101011LL  // Bender's nightmare
#define WALLET_FILE_SIGNATURE_V2                        0x1111011201101011LL  // another Bender's nightmare
#define WALLET_FILE_BINARY_HEADER_VERSION_INITAL        1000
#define WALLET_FILE_BINARY_HEADER_VERSION_2             1001
//#define WALLET_FILE_BINARY_HEADER_VERSION_3             1002

#define WALLET_FILE_MAX_KEYS_SIZE                       10000 //
#define WALLET_BRAIN_DATE_OFFSET                        1543622400
#define WALLET_BRAIN_DATE_QUANTUM                       604800 //by last word we encode a number of week since launch of the project AND password flag, 
                                                               //which let us to address tools::mnemonic_encoding::NUMWORDS weeks after project launch
                                                               //which is about 15 years
#define WALLET_BRAIN_DATE_MAX_WEEKS_COUNT               800

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
#define ALIAS_COMMENT_MAX_SIZE_BYTES                    400

#define CURRENCY_CORE_INSTANCE_LOCK_FILE                "lock.lck"


#define CURRENCY_POOLDATA_FOLDERNAME_OLD                "poolstate"
#define CURRENCY_BLOCKCHAINDATA_FOLDERNAME_OLD          "blockchain"


#define CURRENCY_POOLDATA_FOLDERNAME_PREFIX             "poolstate_"
#define CURRENCY_POOLDATA_FOLDERNAME_SUFFIX             "_v1"
#define CURRENCY_BLOCKCHAINDATA_FOLDERNAME_PREFIX       "blockchain_" 
#define CURRENCY_BLOCKCHAINDATA_FOLDERNAME_SUFFIX       "_v2"

#define P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define MINER_CONFIG_FILENAME                           "miner_conf.json"
#define GUI_SECURE_CONFIG_FILENAME                      "gui_secure_conf.bin"
#define GUI_CONFIG_FILENAME                             "gui_settings.json"
#define GUI_INTERNAL_CONFIG2                            "gui_internal_config.json"
#define GUI_IPC_MESSAGE_CHANNEL_NAME                    CURRENCY_NAME_BASE "_message_que"

#define CURRENCY_VOTING_CONFIG_DEFAULT_FILENAME         "voting_config.json"


#define CURRENT_TRANSACTION_CHAIN_ENTRY_ARCHIVE_VER     3
#define CURRENT_BLOCK_EXTENDED_INFO_ARCHIVE_VER         1

#define BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION  CURRENCY_FORMATION_VERSION + 11
#define BLOCKCHAIN_STORAGE_MINOR_COMPATIBILITY_VERSION  2


#define BC_OFFERS_CURRENT_OFFERS_SERVICE_ARCHIVE_VER    CURRENCY_FORMATION_VERSION + BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION + 9
#define BC_OFFERS_CURRENCY_MARKET_FILENAME              "market.bin"


#define WALLET_FILE_SERIALIZATION_VERSION               168
#define WALLET_FILE_LAST_SUPPORTED_VERSION              165

#define CURRENT_MEMPOOL_ARCHIVE_VER                     (CURRENCY_FORMATION_VERSION+31)

#define BLOCK_MAJOR_VERSION_GENESIS                     1
#define BLOCK_MINOR_VERSION_GENESIS                     0
#define BLOCK_MAJOR_VERSION_INITIAL                     0

/////// Hard forks setup //////////////////////////////
#ifndef TESTNET
// Mainnet
#define ZANO_HARDFORK_01_AFTER_HEIGHT                   194624    // 2019-09-21 20:25:16
#define ZANO_HARDFORK_02_AFTER_HEIGHT                   999999    // 2021-04-05 09:11:45
#define ZANO_HARDFORK_03_AFTER_HEIGHT                   1082577   // 2021-06-01 23:28:10
#define ZANO_HARDFORK_04_AFTER_HEIGHT                   2555000   // 2024-03-21 11:49:55
#define ZANO_HARDFORK_04_TIMESTAMP_ACTUAL               1711021795ull // block 2555000, 2024-03-21 11:49:55 UTC
#define ZANO_HARDFORK_05_AFTER_HEIGHT                   999999999999999999  
#define ZANO_HARDFORK_05_MIN_BUILD_VER                  354
#else
// Testnet
#define ZANO_HARDFORK_01_AFTER_HEIGHT                   0
#define ZANO_HARDFORK_02_AFTER_HEIGHT                   0
#define ZANO_HARDFORK_03_AFTER_HEIGHT                   0
#define ZANO_HARDFORK_04_AFTER_HEIGHT                   100
#define ZANO_HARDFORK_04_TIMESTAMP_ACTUAL               1712800000ull // block 100, 2024-00-00 00:00:00 UTC
#define ZANO_HARDFORK_05_AFTER_HEIGHT                   200
#define ZANO_HARDFORK_05_MIN_BUILD_VER                  356
#endif


#define ZANO_HARDFORK_00_INITAL                         0
#define ZANO_HARDFORK_01                                1
#define ZANO_HARDFORK_02                                2
#define ZANO_HARDFORK_03                                3
#define ZANO_HARDFORK_04_ZARCANUM                       4
#define ZANO_HARDFORK_05                                5
#define ZANO_HARDFORKS_TOTAL                            6




static_assert(CURRENCY_MINER_TX_MAX_OUTS <= CURRENCY_TX_MAX_ALLOWED_OUTS, "Miner tx must obey normal tx max outs limit");
static_assert(PREMINE_AMOUNT / WALLET_MAX_ALLOWED_OUTPUT_AMOUNT < CURRENCY_MINER_TX_MAX_OUTS, "Premine can't be divided into reasonable number of outs");

#define CURRENCY_RELAY_TXS_MAX_COUNT                    5

#ifndef TESTNET
  #define WALLET_ASSETS_WHITELIST_URL                     "https://api.zano.org/assets_whitelist.json"
#else
  #define WALLET_ASSETS_WHITELIST_URL                     "https://api.zano.org/assets_whitelist_testnet.json"
#endif


#define WALLET_ASSETS_WHITELIST_VALIDATION_PUBLIC_KEY   "" //TODO@#@
