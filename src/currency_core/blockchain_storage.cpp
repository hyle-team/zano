// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cstdio>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "include_base_utils.h"

#include "common/db_backend_selector.h"
#include "common/command_line.h"

#include "blockchain_storage.h"
#include "currency_format_utils.h"
#include "currency_boost_serialization.h"
#include "currency_core/currency_config.h"
#include "miner.h"
#include "misc_language.h"
#include "profile_tools.h"
#include "file_io_utils.h"
#include "common/boost_serialization_helper.h"
#include "warnings.h"
#include "crypto/hash.h"
#include "miner_common.h"
#include "storages/portable_storage_template_helper.h"
#include "basic_pow_helpers.h"
#include "version.h"
#include "tx_semantic_validation.h"
#include "crypto/RIPEMD160_helper.h"
#include "crypto/bitcoin/sha256_helper.h"

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "core"
ENABLE_CHANNEL_BY_DEFAULT("core");

using namespace std;
using namespace epee;
using namespace currency;

#define BLOCKCHAIN_STORAGE_CONTAINER_BLOCKS           "blocks"
#define BLOCKCHAIN_STORAGE_CONTAINER_BLOCKS_INDEX     "blocks_index"
#define BLOCKCHAIN_STORAGE_CONTAINER_TRANSACTIONS     "transactions"
#define BLOCKCHAIN_STORAGE_CONTAINER_SPENT_KEYS       "spent_keys"
#define BLOCKCHAIN_STORAGE_CONTAINER_OUTPUTS          "outputs"
#define BLOCKCHAIN_STORAGE_CONTAINER_MULTISIG_OUTS    "multisig_outs"
#define BLOCKCHAIN_STORAGE_CONTAINER_INVALID_BLOCKS   "invalid_blocks"
#define BLOCKCHAIN_STORAGE_CONTAINER_SOLO_OPTIONS     "solo"
#define BLOCKCHAIN_STORAGE_CONTAINER_ALIASES          "aliases"
#define BLOCKCHAIN_STORAGE_CONTAINER_ADDR_TO_ALIAS    "addr_to_alias"
#define BLOCKCHAIN_STORAGE_CONTAINER_TX_FEE_MEDIAN    "median_fee2"
#define BLOCKCHAIN_STORAGE_CONTAINER_GINDEX_INCS      "gindex_increments"

#define BLOCKCHAIN_STORAGE_OPTIONS_ID_CURRENT_BLOCK_CUMUL_SZ_LIMIT          0
#define BLOCKCHAIN_STORAGE_OPTIONS_ID_CURRENT_PRUNED_RS_HEIGHT              1
#define BLOCKCHAIN_STORAGE_OPTIONS_ID_LAST_WORKED_VERSION                   2
#define BLOCKCHAIN_STORAGE_OPTIONS_ID_STORAGE_MAJOR_COMPATIBILITY_VERSION   3 //DON'T CHANGE THIS, if you need to resync db change BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION
#define BLOCKCHAIN_STORAGE_OPTIONS_ID_STORAGE_MINOR_COMPATIBILITY_VERSION   4 //mismatch here means some reinitializations

#define TARGETDATA_CACHE_SIZE                          DIFFICULTY_WINDOW + 10

#ifndef TESTNET
#define BLOCKCHAIN_HEIGHT_FOR_POS_STRICT_SEQUENCE_LIMITATION          57000
#else
#define BLOCKCHAIN_HEIGHT_FOR_POS_STRICT_SEQUENCE_LIMITATION          18000
#endif
#define BLOCK_POS_STRICT_SEQUENCE_LIMIT                               20



DISABLE_VS_WARNINGS(4267)

namespace 
{
  const command_line::arg_descriptor<uint32_t>      arg_db_cache_l1 = { "db-cache-l1", "Specify size of memory mapped db cache file", 0, true };
  const command_line::arg_descriptor<uint32_t>      arg_db_cache_l2 = { "db-cache-l2", "Specify cached elements in db helpers", 0, true };
}

//------------------------------------------------------------------
blockchain_storage::blockchain_storage(tx_memory_pool& tx_pool) :m_db(nullptr, m_rw_lock),
                                                                 m_db_blocks(m_db),
                                                                 m_db_blocks_index(m_db),
                                                                 m_db_transactions(m_db),
                                                                 m_db_spent_keys(m_db),
                                                                 m_db_outputs(m_db),
                                                                 m_db_multisig_outs(m_db),
                                                                 m_db_solo_options(m_db),
                                                                 m_db_aliases(m_db),
                                                                 m_db_addr_to_alias(m_db), 
                                                                 m_read_lock(m_rw_lock),
                                                                 m_db_current_block_cumul_sz_limit(BLOCKCHAIN_STORAGE_OPTIONS_ID_CURRENT_BLOCK_CUMUL_SZ_LIMIT, m_db_solo_options),
                                                                 m_db_current_pruned_rs_height(BLOCKCHAIN_STORAGE_OPTIONS_ID_CURRENT_PRUNED_RS_HEIGHT, m_db_solo_options),
                                                                 m_db_last_worked_version(BLOCKCHAIN_STORAGE_OPTIONS_ID_LAST_WORKED_VERSION, m_db_solo_options),
                                                                 m_db_storage_major_compatibility_version(BLOCKCHAIN_STORAGE_OPTIONS_ID_STORAGE_MAJOR_COMPATIBILITY_VERSION, m_db_solo_options),
                                                                 m_db_storage_minor_compatibility_version(BLOCKCHAIN_STORAGE_OPTIONS_ID_STORAGE_MINOR_COMPATIBILITY_VERSION, m_db_solo_options),
                                                                 m_db_per_block_gindex_incs(m_db),
                                                                 m_tx_pool(tx_pool), 
                                                                 m_is_in_checkpoint_zone(false), 
                                                                 m_is_blockchain_storing(false), 
                                                                 m_core_runtime_config(get_default_core_runtime_config()),
                                                                 //m_bei_stub(AUTO_VAL_INIT(m_bei_stub)),
                                                                 m_event_handler(&m_event_handler_stub), 
                                                                 m_interprocess_locker_file(0), 
                                                                 m_current_fee_median(0), 
                                                                 m_current_fee_median_effective_index(0), 
                                                                 m_is_reorganize_in_process(false), 
                                                                 m_deinit_is_done(false), 
                                                                 m_cached_next_pow_difficulty(0), 
                                                                 m_cached_next_pos_difficulty(0), 
                                                                 m_blockchain_launch_timestamp(0)


{
  m_services_mgr.set_core_runtime_config(m_core_runtime_config);
  m_performance_data.epic_failure_happend = false;
}
blockchain_storage::~blockchain_storage()
{
  if (!m_deinit_is_done)
    deinit();

}
//------------------------------------------------------------------
bool blockchain_storage::have_tx(const crypto::hash &id) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return m_db_transactions.find(id) != m_db_transactions.end();
}
//------------------------------------------------------------------
bool blockchain_storage::have_tx_keyimg_as_spent(const crypto::key_image &key_im, uint64_t before_height /* = UINT64_MAX */) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto it_ptr = m_db_spent_keys.get(key_im);
  if (!it_ptr)
    return false;
  return *it_ptr < before_height;
}
//------------------------------------------------------------------
std::shared_ptr<transaction> blockchain_storage::get_tx(const crypto::hash &id) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto it = m_db_transactions.find(id);
  if (it == m_db_transactions.end())
    return std::shared_ptr<transaction>(nullptr);
  
  return std::make_shared<transaction>(it->tx);
}
//------------------------------------------------------------------
void blockchain_storage::init_options(boost::program_options::options_description& desc)
{
  command_line::add_arg(desc, arg_db_cache_l1);
  command_line::add_arg(desc, arg_db_cache_l2);
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_block_h_older_then(uint64_t timestamp) const 
{
  // get avarage block position
  uint64_t last_block_timestamp = m_db_blocks.back()->bl.timestamp;
  if (timestamp >= last_block_timestamp)
    return get_top_block_height();
  uint64_t difference = last_block_timestamp - timestamp;
  uint64_t n_blocks = difference / (DIFFICULTY_TOTAL_TARGET);
  if (n_blocks >= get_top_block_height())
    return 0;
  uint64_t index = get_top_block_height() - n_blocks;
  while (true)
  {
    if (index == 0)
      return 0;
    if (m_db_blocks[index]->bl.timestamp < timestamp)
      return index;
    index--;
  }
  return 0;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_current_blockchain_size() const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return m_db_blocks.size();
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_top_block_height() const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return m_db_blocks.size() - 1;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_instance(const std::string& path)
{
  std::string locker_name = path + "/" + std::string(CURRENCY_CORE_INSTANCE_LOCK_FILE);
  bool r = epee::file_io_utils::open_and_lock_file(locker_name, m_interprocess_locker_file);

  if (r)
    return true;
  else
  {
    LOG_ERROR("Failed to initialize db: some other instance is already running");
    return false;
  }
}
//------------------------------------------------------------------
bool blockchain_storage::init(const std::string& config_folder, const boost::program_options::variables_map& vm)
{
//  CRITICAL_REGION_LOCAL(m_read_lock);

  tools::db::db_backend_selector dbbs;
  dbbs.init(vm);
  auto p_backend = dbbs.create_backend();
  if (!p_backend)
  {
    LOG_PRINT_RED_L0("Failed to create db engine");
    return false;
  }
  m_db.reset_backend(p_backend);
  LOG_PRINT_L0("DB ENGINE USED BY CORE: " << m_db.get_backend()->name());
  
  if (!validate_instance(config_folder))
  {
    LOG_ERROR("Failed to initialize instance");
    return false;
  }

  uint64_t cache_size_l1 = CACHE_SIZE;
  if (command_line::has_arg(vm, arg_db_cache_l1))
  {
    cache_size_l1 = command_line::get_arg(vm, arg_db_cache_l1);
  }
  LOG_PRINT_GREEN("Using db file cache size(L1): " << cache_size_l1, LOG_LEVEL_0);

  m_config_folder = config_folder;

  // remove old incompatible DB
  const std::string old_db_folder_path = m_config_folder + "/" CURRENCY_BLOCKCHAINDATA_FOLDERNAME_OLD;
  if (boost::filesystem::exists(epee::string_encoding::utf8_to_wstring(old_db_folder_path)))
  {
    LOG_PRINT_YELLOW("Removing old DB in " << old_db_folder_path << "...", LOG_LEVEL_0);
    boost::filesystem::remove_all(epee::string_encoding::utf8_to_wstring(old_db_folder_path));
  }

  const std::string db_folder_path = dbbs.get_db_folder_path();
  LOG_PRINT_L0("Loading blockchain from " << db_folder_path);

  bool db_opened_okay = false;
  for(size_t loading_attempt_no = 0; loading_attempt_no < 2; ++loading_attempt_no)
  {
    bool res = m_db.open(db_folder_path, cache_size_l1);
    if (!res)
    {
      // if DB could not be opened -- try to remove the whole folder and re-open DB
      LOG_PRINT_YELLOW("Failed to initialize database in folder: " << db_folder_path << ", first attempt", LOG_LEVEL_0);
      boost::filesystem::remove_all(epee::string_encoding::utf8_to_wstring(db_folder_path));
      res = m_db.open(db_folder_path, cache_size_l1);
      CHECK_AND_ASSERT_MES(res, false, "Failed to initialize database in folder: " << db_folder_path << ", second attempt");
    }

    res = m_db_blocks.init(BLOCKCHAIN_STORAGE_CONTAINER_BLOCKS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_blocks_index.init(BLOCKCHAIN_STORAGE_CONTAINER_BLOCKS_INDEX);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_transactions.init(BLOCKCHAIN_STORAGE_CONTAINER_TRANSACTIONS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_spent_keys.init(BLOCKCHAIN_STORAGE_CONTAINER_SPENT_KEYS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_outputs.init(BLOCKCHAIN_STORAGE_CONTAINER_OUTPUTS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_multisig_outs.init(BLOCKCHAIN_STORAGE_CONTAINER_MULTISIG_OUTS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_solo_options.init(BLOCKCHAIN_STORAGE_CONTAINER_SOLO_OPTIONS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_aliases.init(BLOCKCHAIN_STORAGE_CONTAINER_ALIASES);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_addr_to_alias.init(BLOCKCHAIN_STORAGE_CONTAINER_ADDR_TO_ALIAS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
    res = m_db_per_block_gindex_incs.init(BLOCKCHAIN_STORAGE_CONTAINER_GINDEX_INCS);
    CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");

    if (command_line::has_arg(vm, arg_db_cache_l2))
    {
      uint64_t cache_size = command_line::get_arg(vm, arg_db_cache_l2);
      LOG_PRINT_GREEN("Using db items cache size(L2): " << cache_size, LOG_LEVEL_0);
      m_db_blocks_index.set_cache_size(cache_size);
      m_db_blocks.set_cache_size(cache_size);
      m_db_blocks_index.set_cache_size(cache_size);
      m_db_transactions.set_cache_size(cache_size);
      m_db_spent_keys.set_cache_size(cache_size);
      //m_db_outputs.set_cache_size(cache_size);
      m_db_multisig_outs.set_cache_size(cache_size);
      m_db_solo_options.set_cache_size(cache_size);
      m_db_aliases.set_cache_size(cache_size);
      m_db_addr_to_alias.set_cache_size(cache_size);
    }

    bool need_reinit = false;
    if (m_db_blocks.size() != 0)
    {
#ifndef TESTNET
      if ((m_db_storage_major_compatibility_version == 93 || m_db_storage_major_compatibility_version == 94) && BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION == 95)
      {
        // migrate DB to rebuild aliases container
        LOG_PRINT_MAGENTA("Migrating DB: " << m_db_storage_major_compatibility_version << " -> " << BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION, LOG_LEVEL_0);

        res = m_db_aliases.deinit();
        CHECK_AND_ASSERT_MES(res, false, "Unable to deinit m_db_aliases");
        res = m_db_addr_to_alias.deinit();
        CHECK_AND_ASSERT_MES(res, false, "Unable to deinit m_db_addr_to_alias");

        typedef tools::db::cached_key_value_accessor<std::string, std::list<extra_alias_entry_base_old>, true, true> aliases_container_old;
        aliases_container_old db_aliases_old(m_db);
        res = db_aliases_old.init(BLOCKCHAIN_STORAGE_CONTAINER_ALIASES);
        CHECK_AND_ASSERT_MES(res, false, "Unable to init db_aliases_old");
        
        typedef tools::db::cached_key_value_accessor<account_public_address_old, std::set<std::string>, true, false> address_to_aliases_container_old;
        address_to_aliases_container_old db_addr_to_alias_old(m_db);
        res = db_addr_to_alias_old.init(BLOCKCHAIN_STORAGE_CONTAINER_ADDR_TO_ALIAS);
        CHECK_AND_ASSERT_MES(res, false, "Unable to init db_addr_to_alias_old");

        // temporary set db compatibility version to zero during migration in order to trigger db reinit on the next lanunch in case the process stops in the middle
        m_db.begin_transaction();
        uint64_t tmp_db_maj_version = m_db_storage_major_compatibility_version;
        m_db_storage_major_compatibility_version = 0;
        m_db.commit_transaction();

        typedef std::vector<std::pair<std::string, std::list<extra_alias_entry_base>>> tmp_container_t;
        tmp_container_t temp_container;
        db_aliases_old.enumerate_items([&temp_container](uint64_t i, const std::string& alias, const std::list<extra_alias_entry_base_old>& alias_entries)
        {
          std::pair<std::string, std::list<extra_alias_entry_base>> p(alias, std::list<extra_alias_entry_base>());
          for(auto& entry : alias_entries)
            p.second.push_back(static_cast<extra_alias_entry_base>(entry)); // here conversion to the new format goes
          temp_container.emplace_back(p);
          return true;
        });

        typedef std::vector<std::pair<account_public_address, std::set<std::string>>> add_to_alias_container_t;
        add_to_alias_container_t addr_to_alias_container;
        db_addr_to_alias_old.enumerate_items([&addr_to_alias_container](uint64_t n, const account_public_address_old& addr_old, const std::set<std::string>& aliases){
          addr_to_alias_container.emplace_back(std::make_pair(account_public_address::from_old(addr_old), aliases));
          return true;
        });

        // clear and close old format container
        m_db.begin_transaction();
        db_aliases_old.clear();
        db_addr_to_alias_old.clear();
        m_db.commit_transaction();
        db_aliases_old.deinit();
        db_addr_to_alias_old.deinit();
  
        res = m_db_aliases.init(BLOCKCHAIN_STORAGE_CONTAINER_ALIASES);
        CHECK_AND_ASSERT_MES(res, false, "Unable to init m_db_aliases");
        res = m_db_addr_to_alias.init(BLOCKCHAIN_STORAGE_CONTAINER_ADDR_TO_ALIAS);
        CHECK_AND_ASSERT_MES(res, false, "Unable to init m_db_addr_to_alias");

        // re-populate all alias entries back
        m_db.begin_transaction();
        for(auto& el : temp_container)
          m_db_aliases.set(el.first, el.second);

        for(auto& el : addr_to_alias_container)
          m_db_addr_to_alias.set(el.first, el.second);
        m_db.commit_transaction();

        // restore db maj compartibility
        m_db.begin_transaction();
        m_db_storage_major_compatibility_version = tmp_db_maj_version;
        m_db.commit_transaction();

        LOG_PRINT_MAGENTA("Migrating DB: successfully done", LOG_LEVEL_0);
      }
      else if (m_db_storage_major_compatibility_version == 93 && BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION == 94)
      {
        // do not reinit db if moving from version 93 to version 94
        LOG_PRINT_MAGENTA("DB storage does not need reinit because moving from v93 to v94", LOG_LEVEL_0);
      }
#else
      // TESTNET
      if (m_db_storage_major_compatibility_version == 95 && BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION == 96)
      {
        // do not reinit TESTNET db if moving from version 95 to version 96
        LOG_PRINT_MAGENTA("DB storage does not need reinit because moving from v95 to v96", LOG_LEVEL_0);
      }
#endif
      else if (m_db_storage_major_compatibility_version != BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION)
      {
        need_reinit = true;
        LOG_PRINT_MAGENTA("DB storage needs reinit because it has major compatibility ver " << m_db_storage_major_compatibility_version << ", expected : " << BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION, LOG_LEVEL_0); 
      }
      else if (m_db_storage_minor_compatibility_version > BLOCKCHAIN_STORAGE_MINOR_COMPATIBILITY_VERSION)
      {
        // reinit db only if minor version in the DB is greather (i.e. newer) than minor version in the code 
        need_reinit = true;
        LOG_PRINT_MAGENTA("DB storage needs reinit because it has minor compatibility ver " << m_db_storage_minor_compatibility_version << " that is greater than BLOCKCHAIN_STORAGE_MINOR_COMPATIBILITY_VERSION: " << BLOCKCHAIN_STORAGE_MINOR_COMPATIBILITY_VERSION, LOG_LEVEL_0);
      }
    }

    if (need_reinit)
    {
      LOG_PRINT_L1("DB at " << db_folder_path << " is about to be deleted and re-created...");
      m_db_blocks.deinit();
      m_db_blocks_index.deinit();
      m_db_transactions.deinit();
      m_db_spent_keys.deinit();
      m_db_outputs.deinit();
      m_db_multisig_outs.deinit();
      m_db_solo_options.deinit();
      m_db_aliases.deinit();
      m_db_addr_to_alias.deinit();
      m_db_per_block_gindex_incs.deinit();
      m_db.close();
      size_t files_removed = boost::filesystem::remove_all(epee::string_encoding::utf8_to_wstring(db_folder_path));
      LOG_PRINT_L1(files_removed << " files at " << db_folder_path << " removed");

      // try to re-create DB and re-init containers
      continue;
    }

    db_opened_okay = true;
    break;
  }

  CHECK_AND_ASSERT_MES(db_opened_okay, false, "All attempts to open DB at " << db_folder_path << " failed");

  if (!m_db_blocks.size())
  {
    // empty DB: generate and add genesis block
    block bl = boost::value_initialized<block>();
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    generate_genesis_block(bl);
    add_new_block(bl, bvc);
    CHECK_AND_ASSERT_MES(!bvc.m_verification_failed, false, "Failed to add genesis block to blockchain");
    LOG_PRINT_MAGENTA("Storage initialized with genesis", LOG_LEVEL_0);
  } 

  store_db_solo_options_values();

  m_services_mgr.init(config_folder, vm);

  //print information message
  uint64_t timestamp_diff = m_core_runtime_config.get_core_time() - m_db_blocks.back()->bl.timestamp;
  if(!m_db_blocks.back()->bl.timestamp)
    timestamp_diff = m_core_runtime_config.get_core_time() - 1341378000;

  m_db.begin_transaction();
  set_lost_tx_unmixable();
  m_db.commit_transaction();

  LOG_PRINT_GREEN("Blockchain initialized. (v:" << m_db_storage_major_compatibility_version << ") last block: " << m_db_blocks.size() - 1 << ENDL 
    << "genesis: " << get_block_hash(m_db_blocks[0]->bl) << ENDL
    << "last block: " << m_db_blocks.size() - 1 << ", " << misc_utils::get_time_interval_string(timestamp_diff) << " time ago" << ENDL
    << "current pos difficulty: " << get_next_diff_conditional(true) << ENDL
    << "current pow difficulty: " << get_next_diff_conditional(false) << ENDL
    << "total transactions: " << m_db_transactions.size(),
    LOG_LEVEL_0);

  return true;
}

//------------------------------------------------------------------
bool blockchain_storage::set_lost_tx_unmixable_for_height(uint64_t height)
{
#ifndef TESTNET
  if (height == 75738)
    return set_lost_tx_unmixable();  
#endif
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::set_lost_tx_unmixable()
{  
#ifndef TESTNET
  if (m_db_blocks.size() > 75738)
  {
     crypto::hash tx_id_1 = epee::string_tools::parse_tpod_from_hex_string<crypto::hash>("c2a2229d614e7c026433efbcfdbd0be1f68d9b419220336df3e2c209f5d57314");
     crypto::hash tx_id_2 = epee::string_tools::parse_tpod_from_hex_string<crypto::hash>("647f936c6ffbd136f5c95d9a90ad554bdb4c01541c6eb5755ad40b984d80da67");
 
     auto tx_ptr_1 = m_db_transactions.find(tx_id_1);
     CHECK_AND_ASSERT_MES(tx_ptr_1, false, "Internal error: filed to find lost tx");
     transaction_chain_entry tx1_local_entry(*tx_ptr_1);
     for (size_t i = 0; i != tx1_local_entry.m_spent_flags.size(); i++)
     {
       tx1_local_entry.m_spent_flags[i] = true;
     }
     m_db_transactions.set(tx_id_1, tx1_local_entry);

     auto tx_ptr_2 = m_db_transactions.find(tx_id_2);
     transaction_chain_entry tx2_local_entry(*tx_ptr_2);
     CHECK_AND_ASSERT_MES(tx_ptr_1, false, "Internal error: filed to find lost tx");
     for (size_t i = 0; i != tx2_local_entry.m_spent_flags.size(); i++)
     {
       tx2_local_entry.m_spent_flags[i] = true;
     }
     m_db_transactions.set(tx_id_2, tx2_local_entry);
  }
#endif
  return true;
}
//------------------------------------------------------------------
void  blockchain_storage::patch_out_if_needed(txout_to_key& out, const crypto::hash& tx_id, uint64_t n) const 
{
#ifndef TESTNET
  static crypto::hash tx_id_1 = epee::string_tools::parse_tpod_from_hex_string<crypto::hash>("c2a2229d614e7c026433efbcfdbd0be1f68d9b419220336df3e2c209f5d57314");
  static crypto::hash tx_id_2 = epee::string_tools::parse_tpod_from_hex_string<crypto::hash>("647f936c6ffbd136f5c95d9a90ad554bdb4c01541c6eb5755ad40b984d80da67");

  if (tx_id == tx_id_1 && n == 12)
  {
    out.mix_attr = CURRENCY_TO_KEY_OUT_FORCED_NO_MIX;
  }else if(tx_id == tx_id_2 && n == 5)
  {
    out.mix_attr = CURRENCY_TO_KEY_OUT_FORCED_NO_MIX;
  }
#endif
}
//------------------------------------------------------------------
void blockchain_storage::store_db_solo_options_values()
{
  m_db.begin_transaction();
  m_db_storage_major_compatibility_version = BLOCKCHAIN_STORAGE_MAJOR_COMPATIBILITY_VERSION;
  m_db_storage_minor_compatibility_version = BLOCKCHAIN_STORAGE_MINOR_COMPATIBILITY_VERSION;
  m_db_last_worked_version = std::string(PROJECT_VERSION_LONG);
  m_db.commit_transaction();
}
//------------------------------------------------------------------
bool blockchain_storage::deinit()
{
  m_db.close();
  epee::file_io_utils::unlock_and_close_file(m_interprocess_locker_file);
  m_deinit_is_done = true;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::pop_block_from_blockchain(transactions_map& onboard_transactions)
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  CHECK_AND_ASSERT_MES(m_db_blocks.size() > 1, false, "pop_block_from_blockchain: can't pop from blockchain with size = " << m_db_blocks.size());
  size_t h = m_db_blocks.size()-1;
  auto bei_ptr = m_db_blocks[h];
  CHECK_AND_ASSERT_MES(bei_ptr.get(), false, "pop_block_from_blockchain: can't pop from blockchain");

  uint64_t fee_total = 0;
  bool r = purge_block_data_from_blockchain(bei_ptr->bl, bei_ptr->bl.tx_hashes.size(), fee_total, onboard_transactions);
  CHECK_AND_ASSERT_MES(r, false, "Failed to purge_block_data_from_blockchain for block " << get_block_hash(bei_ptr->bl) << " on height " << h);

  pop_block_from_per_block_increments(bei_ptr->height);

  //remove from index
  r = m_db_blocks_index.erase_validate(get_block_hash(bei_ptr->bl));
  CHECK_AND_ASSERT_MES_NO_RET(r, "pop_block_from_blockchain: block id not found in m_blocks_index while trying to delete it");

  //pop block from core
  m_db_blocks.pop_back();

  on_block_removed(*bei_ptr);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::set_checkpoints(checkpoints&& chk_pts) 
{
  m_checkpoints = chk_pts;
  try
  {
    m_db.begin_transaction();
    if (m_db_blocks.size() < m_checkpoints.get_top_checkpoint_height())
      m_is_in_checkpoint_zone = true;
    prune_ring_signatures_and_attachments_if_need();
    m_db.commit_transaction();
    return true;
  }
  catch (const std::exception& ex)
  {
    m_db.abort_transaction();
    LOG_ERROR("UNKNOWN EXCEPTION WHILE SETTING CHECKPOINTS: " << ex.what());
    return false;
  }
  catch (...)
  {
    m_db.abort_transaction();
    LOG_ERROR("UNKNOWN EXCEPTION WHILE SETTING CHECKPOINTS.");
    return false;
  }
  
}
//------------------------------------------------------------------
bool blockchain_storage::prune_ring_signatures_and_attachments(uint64_t height, uint64_t& transactions_pruned, uint64_t& signatures_pruned, uint64_t& attachments_pruned)
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  CHECK_AND_ASSERT_MES(height < m_db_blocks.size(), false, "prune_ring_signatures called with wrong parameter: " << height << ", m_blocks.size() = " << m_db_blocks.size());
  auto vptr = m_db_blocks[height];
  CHECK_AND_ASSERT_MES(vptr.get(), false, "Failed to get block on height");

  for (const auto& h : vptr->bl.tx_hashes)
  {
    auto it = m_db_transactions.find(h);
    CHECK_AND_ASSERT_MES(it != m_db_transactions.end(), false, "failed to find transaction " << h << " in blockchain index, in block on height = " << height);    


    CHECK_AND_ASSERT_MES(it->m_keeper_block_height == height, false, 
      "failed to validate extra check, it->second.m_keeper_block_height = " << it->m_keeper_block_height  << 
      "is mot equal to height = " << height << " in blockchain index, for block on height = " << height);
    
    transaction_chain_entry lolcal_chain_entry = *it;
    signatures_pruned += lolcal_chain_entry.tx.signatures.size();
    attachments_pruned += lolcal_chain_entry.tx.attachment.size();
    lolcal_chain_entry.tx.signatures.clear();
    lolcal_chain_entry.tx.attachment.clear();

    //reassign to db
    m_db_transactions.set(h, lolcal_chain_entry);

    ++transactions_pruned;
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::prune_ring_signatures_and_attachments_if_need()
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  uint64_t top_block_height = get_top_block_height();
  uint64_t pruning_end_height = m_checkpoints.get_checkpoint_before_height(top_block_height);
  if (pruning_end_height > m_db_current_pruned_rs_height)
  {
    LOG_PRINT_CYAN("Starting pruning ring signatues and attachments from height " << m_db_current_pruned_rs_height + 1 << " to height " << pruning_end_height
      << " (" << pruning_end_height - m_db_current_pruned_rs_height << " blocks), top block height is " << top_block_height, LOG_LEVEL_0);
    uint64_t tx_count = 0, sig_count = 0, attach_count = 0;
    for(uint64_t height = m_db_current_pruned_rs_height + 1; height <= pruning_end_height; height++)
    {
      bool res = prune_ring_signatures_and_attachments(height, tx_count, sig_count, attach_count);
      CHECK_AND_ASSERT_MES(res, false, "failed to prune_ring_signatures_and_attachments for height = " << height);
    }
    m_db_current_pruned_rs_height = pruning_end_height;
    LOG_PRINT_CYAN("Transaction pruning finished: " << sig_count << " signatures and " << attach_count << " attachments released in " << tx_count << " transactions.", LOG_LEVEL_0);
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::clear()
{
  //CRITICAL_REGION_LOCAL(m_read_lock);
  m_db.begin_transaction();

  m_db_blocks.clear();
  m_db_blocks_index.clear();
  m_db_transactions.clear();
  m_db_spent_keys.clear();
  m_db_solo_options.clear();
  store_db_solo_options_values();
  m_db_outputs.clear();
  m_db_multisig_outs.clear();
  m_db_aliases.clear();
  m_db_addr_to_alias.clear();
  m_db_per_block_gindex_incs.clear();
  m_pos_targetdata_cache.clear();
  m_pow_targetdata_cache.clear();

  m_db.commit_transaction();
  

  {
    CRITICAL_REGION_LOCAL(m_invalid_blocks_lock);
    m_invalid_blocks.clear();     // crypto::hash -> block_extended_info
  }
  {
    CRITICAL_REGION_LOCAL(m_alternative_chains_lock);
    m_alternative_chains.clear();
    m_altblocks_keyimages.clear();
    m_alternative_chains_txs.clear();
  }
  
  
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::reset_and_set_genesis_block(const block& b)
{
  clear();
  block_verification_context bvc = boost::value_initialized<block_verification_context>();
  add_new_block(b, bvc);
  if(!bvc.m_added_to_main_chain || bvc.m_verification_failed)
  {
    LOG_ERROR("Blockchain reset failed.")
    return false;
  }
  LOG_PRINT_GREEN("Blockchain reset. Genesis block: " << get_block_hash(b) << ", " << misc_utils::get_time_interval_string(m_core_runtime_config.get_core_time() - b.timestamp) << " ago", LOG_LEVEL_0);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::purge_transaction_keyimages_from_blockchain(const transaction& tx, bool strict_check)
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  struct purge_transaction_visitor: public boost::static_visitor<bool>
  {
    blockchain_storage& m_bcs;
    key_images_container& m_spent_keys;
    bool m_strict_check;
    purge_transaction_visitor(blockchain_storage& bcs, key_images_container& spent_keys, bool strict_check):
      m_bcs(bcs),
      m_spent_keys(spent_keys), 
      m_strict_check(strict_check){}

    bool operator()(const txin_to_key& inp) const
    {
      bool r = m_spent_keys.erase_validate(inp.k_image);
      CHECK_AND_ASSERT_MES( !(!r && m_strict_check), false, "purge_transaction_keyimages_from_blockchain: key image " << inp.k_image << " was not found");

      if(inp.key_offsets.size() == 1)
      {
        //direct spend detected
        if(!m_bcs.update_spent_tx_flags_for_input(inp.amount, inp.key_offsets[0], false))
        {
          //internal error
          LOG_PRINT_L0("Failed to  update_spent_tx_flags_for_input");
          return false;
        }
      }

      return true;
    }
    bool operator()(const txin_gen& inp) const
    {
      return true;
    }
    bool operator()(const txin_multisig& inp) const
    {
      if (!m_bcs.update_spent_tx_flags_for_input(inp.multisig_out_id, 0))
      {
        LOG_PRINT_L0("update_spent_tx_flags_for_input failed for multisig id " << inp.multisig_out_id << " amount: " << inp.amount);
        return false;
      }
      return true;
    }
    bool operator()(const txin_htlc& inp) const
    {
      return this->operator()(static_cast<const txin_to_key&>(inp));
    }
  };

  for(const txin_v& in : tx.vin)
  {
    bool r = boost::apply_visitor(purge_transaction_visitor(*this, m_db_spent_keys, strict_check), in);
    CHECK_AND_ASSERT_MES(!strict_check || r, false, "failed to process purge_transaction_visitor");
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::purge_transaction_from_blockchain(const crypto::hash& tx_id, uint64_t& fee, transaction& tx_)
{
  fee = 0;
  CRITICAL_REGION_LOCAL(m_read_lock);

  auto tx_res_ptr = m_db_transactions.find(tx_id);
  CHECK_AND_ASSERT_MES(tx_res_ptr != m_db_transactions.end(), false, "transaction " << tx_id << " is not found in blockchain index!!");
  const transaction& tx = tx_res_ptr->tx;
  tx_ = tx;

  fee = get_tx_fee(tx_res_ptr->tx);
  purge_transaction_keyimages_from_blockchain(tx, true);
  
  bool r = unprocess_blockchain_tx_extra(tx);
  CHECK_AND_ASSERT_MES(r, false, "failed to unprocess_blockchain_tx_extra for tx " << tx_id);
 
  r = unprocess_blockchain_tx_attachments(tx, get_current_blockchain_size(), 0/*TODO: add valid timestamp here in future if need*/);

  bool added_to_the_pool = false;
  if(!is_coinbase(tx))
  {
    currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    added_to_the_pool = m_tx_pool.add_tx(tx, tvc, true, true);
    CHECK_AND_ASSERT_MES(added_to_the_pool, false, "failed to add transaction " << tx_id << " to transaction pool");
  }

  bool res = pop_transaction_from_global_index(tx, tx_id);
  CHECK_AND_ASSERT_MES_NO_RET(res, "pop_transaction_from_global_index failed for tx " << tx_id);
  bool res_erase = m_db_transactions.erase_validate(tx_id);
  CHECK_AND_ASSERT_MES_NO_RET(res_erase, "Failed to m_transactions.erase with id = " << tx_id);
  
  LOG_PRINT_L1("transaction " << tx_id << (added_to_the_pool ? " was removed from blockchain history -> to the pool" : " was removed from blockchain history"));
  return res;
}

//------------------------------------------------------------------
bool blockchain_storage::purge_block_data_from_blockchain(const block& b, size_t processed_tx_count)
{
  uint64_t total_fee = 0;
  transactions_map onboard_transactions;
  return purge_block_data_from_blockchain(b, processed_tx_count, total_fee, onboard_transactions);
}
//------------------------------------------------------------------
bool blockchain_storage::purge_block_data_from_blockchain(const block& bl, size_t processed_tx_count, uint64_t& fee_total, transactions_map& onboard_transactions)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  fee_total = 0;
  uint64_t fee = 0;
  bool res = true;
  CHECK_AND_ASSERT_MES(processed_tx_count <= bl.tx_hashes.size(), false, "wrong processed_tx_count in purge_block_data_from_blockchain");
  for(size_t count = 0; count != processed_tx_count; count++)
  {
    transaction tx = AUTO_VAL_INIT(tx);
    res = purge_transaction_from_blockchain(bl.tx_hashes[(processed_tx_count -1)- count], fee, tx) && res;
    fee_total += fee;
    onboard_transactions[bl.tx_hashes[(processed_tx_count - 1) - count]] = tx;
  }
  transaction tx = AUTO_VAL_INIT(tx);
  res = purge_transaction_from_blockchain(get_transaction_hash(bl.miner_tx), fee, tx) && res;
  return res;
}
//------------------------------------------------------------------
crypto::hash blockchain_storage::get_top_block_id(uint64_t& height) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  height = get_top_block_height();
  return get_top_block_id();
}
//------------------------------------------------------------------
crypto::hash blockchain_storage::get_top_block_id() const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  crypto::hash id = null_hash;
  if(m_db_blocks.size())
  {
    auto val_ptr = m_db_blocks.back();
    CHECK_AND_ASSERT_MES(val_ptr, null_hash, "m_blocks.back() returned null");
    get_block_hash(val_ptr->bl, id);
  }
  return id;
}
//------------------------------------------------------------------
bool blockchain_storage::get_top_block(block& b) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CHECK_AND_ASSERT_MES(m_db_blocks.size(), false, "Wrong blockchain state, m_blocks.size()=0!");
  auto val_ptr = m_db_blocks.back();
  CHECK_AND_ASSERT_MES(val_ptr.get(), false, "m_blocks.back() returned null");
  b = val_ptr->bl;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_short_chain_history(std::list<crypto::hash>& ids)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  size_t i = 0;
  size_t current_multiplier = 1;
  size_t sz = m_db_blocks.size();
  if(!sz)
    return true;
  size_t current_back_offset = 1;
  bool genesis_included = false;
  while(current_back_offset < sz)
  {
    ids.push_back(get_block_hash(m_db_blocks[sz-current_back_offset]->bl));
    if(sz-current_back_offset == 0)
      genesis_included = true;
    if(i < 10)
    {
      ++current_back_offset;
    }else
    {
      current_back_offset += current_multiplier *= 2;
    }
    ++i;
  }
  if(!genesis_included)
    ids.push_back(get_block_hash(m_db_blocks[0]->bl));

  return true;
}
//------------------------------------------------------------------
crypto::hash blockchain_storage::get_block_id_by_height(uint64_t height) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if(height >= m_db_blocks.size())
    return null_hash;

  return get_block_hash(m_db_blocks[height]->bl);
}
//------------------------------------------------------------------
bool blockchain_storage::get_block_by_hash(const crypto::hash &h, block &blk)  const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  // try to find block in main chain
  auto it = m_db_blocks_index.find(h);
  if (m_db_blocks_index.end() != it) 
  {
    blk = m_db_blocks[*it]->bl;
    return true;
  }

  // try to find block in alternative chain
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);

  auto it_alt = m_alternative_chains.find(h);
  if (m_alternative_chains.end() != it_alt) 
  {
    blk = it_alt->second.bl;
    return true;
  }

  return false;
}
bool blockchain_storage::is_tx_related_to_altblock(crypto::hash tx_id) const
{
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);
  auto it = m_alternative_chains_txs.find(tx_id);
  return it != m_alternative_chains_txs.end();
}

//------------------------------------------------------------------
bool blockchain_storage::get_block_extended_info_by_hash(const crypto::hash &h, block_extended_info &blk) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  // try to find block in main chain
  auto vptr = m_db_blocks_index.find(h);
  if (vptr)
  {
    return get_block_extended_info_by_height(*vptr, blk);
  }

  // try to find block in alternative chain
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);
  auto it_alt = m_alternative_chains.find(h);
  if (m_alternative_chains.end() != it_alt) 
  {
    blk = it_alt->second;
    return true;
  }

  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::get_block_extended_info_by_height(uint64_t h, block_extended_info &blk) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  if (h >= m_db_blocks.size())
    return false;
  
  blk = *m_db_blocks[h];
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_block_by_height(uint64_t h, block &blk) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if(h >= m_db_blocks.size() )
    return false;
  blk = m_db_blocks[h]->bl;
  return true;
}
//------------------------------------------------------------------
// void blockchain_storage::get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid)  const
// {
//   CRITICAL_REGION_LOCAL(m_blockchain_lock);
// 
//   for (auto &v : m_blocks_index)
//     main.push_back(v.first);
// 
//   for (auto &v : m_alternative_chains)
//     alt.push_back(v.first);
// 
//   for(auto &v: m_invalid_blocks)
//     invalid.push_back(v.first);
// } 
//------------------------------------------------------------------
bool blockchain_storage::rollback_blockchain_switching(std::list<block_ws_txs>& original_chain, size_t rollback_height)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  //remove failed subchain
  for(size_t i = m_db_blocks.size()-1; i >=rollback_height; i--)
  {
    transactions_map ot;
    bool r = pop_block_from_blockchain(ot);
    CHECK_AND_ASSERT_MES(r, false, "PANIC!!! failed to remove block while chain switching during the rollback!");
  }
  //return back original chain
  BOOST_FOREACH(auto& oce, original_chain)
  {
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    bvc.m_onboard_transactions.swap(oce.onboard_transactions);
    bool r = handle_block_to_main_chain(oce.b, bvc);
    CHECK_AND_ASSERT_MES(r && bvc.m_added_to_main_chain, false, "PANIC!!! failed to add (again) block while chain switching during the rollback!");
  }

  LOG_PRINT_L0("Rollback success.");
  return true;
}
//------------------------------------------------------------------
void blockchain_storage::add_alt_block_txs_hashs(const block& b)
{
  CRITICAL_REGION_LOCAL(m_alternative_chains_lock);
  for (const auto& tx_hash : b.tx_hashes)
  {
    m_alternative_chains_txs[tx_hash]++;
  }
}
//------------------------------------------------------------------
void blockchain_storage::purge_alt_block_txs_hashs(const block& b)
{
  CRITICAL_REGION_LOCAL(m_alternative_chains_lock);
  for (const auto& h : b.tx_hashes)
  {
    auto it = m_alternative_chains_txs.find(h);
    if (it == m_alternative_chains_txs.end())
    {
      LOG_ERROR("Internal error: tx with hash " << h << " not found in m_alternative_chains_txs while removing block " << get_block_hash(b));
      continue;
    }

    if (it->second >= 1)
    {
      it->second--;
    }
    else
    {
      LOG_ERROR("Internal error: tx with hash " << h << " has invalid m_alternative_chains_txs entry (zero count) while removing block " << get_block_hash(b));
    }
    if (it->second == 0)
      m_alternative_chains_txs.erase(it);
  }
}
//------------------------------------------------------------------
void blockchain_storage::do_erase_altblock(alt_chain_container::iterator it)
{
  crypto::hash id = get_block_hash(it->second.bl);
  LOG_PRINT_L1("erasing alt block " << print16(id) << " @ " << get_block_height(it->second.bl));
  purge_altblock_keyimages_from_big_heap(it->second.bl, id);
  purge_alt_block_txs_hashs(it->second.bl);
  m_alternative_chains.erase(it);
}
//------------------------------------------------------------------
bool blockchain_storage::switch_to_alternative_blockchain(alt_chain_type& alt_chain)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL!");

  CHECK_AND_ASSERT_MES(alt_chain.size(), false, "switch_to_alternative_blockchain: empty chain passed");

  size_t split_height = alt_chain.front()->second.height;
  CHECK_AND_ASSERT_MES(m_db_blocks.size() >= split_height, false, "switch_to_alternative_blockchain: blockchain size is lower than split height" << ENDL
    << " alt chain: " << ENDL << print_alt_chain(alt_chain) << ENDL
    << " main chain: " << ENDL << get_blockchain_string(m_db_blocks.size() - 10, CURRENCY_MAX_BLOCK_NUMBER)
    );

  //disconnecting old chain
  std::list<block_ws_txs> disconnected_chain;
  for(size_t i = m_db_blocks.size()-1; i >=split_height; i--)
  {
    disconnected_chain.push_front(block_ws_txs());
    block_ws_txs& bwt = disconnected_chain.front();
    bwt.b = m_db_blocks[i]->bl;
    bool r = pop_block_from_blockchain(bwt.onboard_transactions);
    CHECK_AND_ASSERT_MES(r, false, "failed to remove block " << get_block_hash(bwt.b) << " @ " << get_block_height(bwt.b) << " on chain switching");
    
    CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL!");
  }

  //connecting new alternative chain
  for(auto alt_ch_iter = alt_chain.begin(); alt_ch_iter != alt_chain.end(); alt_ch_iter++)
  {
    auto ch_ent = *alt_ch_iter;
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    bvc.m_onboard_transactions = ch_ent->second.onboard_transactions;
    bool r = handle_block_to_main_chain(ch_ent->second.bl, bvc);
    if(!r || !bvc.m_added_to_main_chain)
    {
      LOG_PRINT_L0("Failed to switch to alternative blockchain");
      rollback_blockchain_switching(disconnected_chain, split_height);
      LOG_PRINT_L0("The block was inserted as invalid while connecting new alternative chain,  block_id: " << get_block_hash(ch_ent->second.bl));

      for(; alt_ch_iter != alt_chain.end(); ++alt_ch_iter)
      {
        add_block_as_invalid((*alt_ch_iter)->second, (*alt_ch_iter)->first);
        do_erase_altblock(*alt_ch_iter);
      }
      CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL!");
      return false;
    }
  }

  //pushing old chain as alternative chain
  for(auto& old_ch_ent : disconnected_chain)
  {
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    bvc.m_onboard_transactions.swap(old_ch_ent.onboard_transactions);
    bool r = handle_alternative_block(old_ch_ent.b, get_block_hash(old_ch_ent.b), bvc);
    if(!r)
    {
      LOG_ERROR("Failed to push ex-main chain blocks to alternative chain ");
      rollback_blockchain_switching(disconnected_chain, split_height);
      CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL!");

      //can't do return false here, because of the risc to get stuck in "PANIC" mode when nor of 
      //new chain nor altchain can be inserted into main chain. Got core caught in this trap when
      //when machine time was wrongly set for a few hours back, then blocks which was detached from main chain 
      //couldn't be added as alternative due to timestamps validation(timestamps assumed as from future)
      //thanks @Gigabyted for reporting this problem
      break;
    }
  }

  //removing all_chain entries from alternative chain
  for(auto ch_ent : alt_chain)
  {
    do_erase_altblock(ch_ent);
  }

  LOG_PRINT_GREEN("REORGANIZE SUCCESS! on height: " << split_height << ", new blockchain size: " << m_db_blocks.size(), LOG_LEVEL_0);
  return true;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_next_diff_conditional(bool pos) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  std::vector<uint64_t> timestamps;
  std::vector<wide_difficulty_type> commulative_difficulties;
  if (!m_db_blocks.size())
    return DIFFICULTY_STARTER;
  //skip genesis timestamp
  TIME_MEASURE_START_PD(target_calculating_enum_blocks);
  CRITICAL_REGION_BEGIN(m_targetdata_cache_lock);
  std::list<std::pair<wide_difficulty_type, uint64_t>>& targetdata_cache = pos ? m_pos_targetdata_cache : m_pow_targetdata_cache;
  //if (targetdata_cache.empty())
  load_targetdata_cache(pos);

  size_t count = 0;
  for (auto it = targetdata_cache.rbegin(); it != targetdata_cache.rend() && count < DIFFICULTY_WINDOW; it++)
  {
    timestamps.push_back(it->second);
    commulative_difficulties.push_back(it->first);
    ++count;
  }
  CRITICAL_REGION_END();

  wide_difficulty_type& dif = pos ? m_cached_next_pos_difficulty : m_cached_next_pow_difficulty;
  TIME_MEASURE_FINISH_PD(target_calculating_enum_blocks);
  TIME_MEASURE_START_PD(target_calculating_calc);
  if (m_db_blocks.size() > m_core_runtime_config.hard_fork_01_starts_after_height)
  {
    dif = next_difficulty_2(timestamps, commulative_difficulties, pos ? DIFFICULTY_POS_TARGET : DIFFICULTY_POW_TARGET);
  }
  else
  {
    dif = next_difficulty_1(timestamps, commulative_difficulties, pos ? DIFFICULTY_POS_TARGET : DIFFICULTY_POW_TARGET);
  }
  

  TIME_MEASURE_FINISH_PD(target_calculating_calc);
  return dif;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_next_diff_conditional2(bool pos, const alt_chain_type& alt_chain, uint64_t split_height, const alt_block_extended_info& abei) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  std::vector<uint64_t> timestamps;
  std::vector<wide_difficulty_type> commulative_difficulties;
  size_t count = 0;
  if (!m_db_blocks.size())
    return DIFFICULTY_STARTER;

  auto cb = [&](const block_extended_info& bei, bool is_main){
    if (!bei.height)
      return false;
    bool is_pos_bl = is_pos_block(bei.bl);
    if (pos != is_pos_bl)
      return true;
    timestamps.push_back(bei.bl.timestamp);
    commulative_difficulties.push_back(bei.cumulative_diff_precise);
    ++count;
    if (count >= DIFFICULTY_WINDOW)
      return false;
    return true;
  };
  enum_blockchain(cb, alt_chain, split_height);

  wide_difficulty_type diff = 0;
  if(abei.height > m_core_runtime_config.hard_fork_01_starts_after_height)
    diff = next_difficulty_2(timestamps, commulative_difficulties, pos ? DIFFICULTY_POS_TARGET : DIFFICULTY_POW_TARGET);
  else
    diff = next_difficulty_1(timestamps, commulative_difficulties, pos ? DIFFICULTY_POS_TARGET : DIFFICULTY_POW_TARGET);
  return diff;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_cached_next_difficulty(bool pos) const
{
  wide_difficulty_type res = pos ? m_cached_next_pos_difficulty : m_cached_next_pow_difficulty;
  if (!res)
  {
    get_next_diff_conditional(pos);
    res = pos ? m_cached_next_pos_difficulty : m_cached_next_pow_difficulty;
  }
  return res;
}
//------------------------------------------------------------------
std::shared_ptr<block_extended_info> blockchain_storage::get_last_block_of_type(bool looking_for_pos, const alt_chain_type& alt_chain, uint64_t split_height) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  std::shared_ptr<block_extended_info> pbei(nullptr);

  auto cb = [&](const block_extended_info& bei_local, bool is_main){
    if (looking_for_pos == is_pos_block(bei_local.bl))
    {
      pbei.reset(new block_extended_info(bei_local));
      return false;
    }
    return true;
  };


  enum_blockchain(cb, alt_chain, split_height);
  return pbei;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_next_difficulty_for_alternative_chain(const alt_chain_type& alt_chain, block_extended_info& bei, bool pos) const
{
  std::vector<uint64_t> timestamps;
  std::vector<wide_difficulty_type> commulative_difficulties;

  for (auto it = alt_chain.rbegin(); it != alt_chain.rend() && timestamps.size() < DIFFICULTY_BLOCKS_COUNT; it++)
  {
    bool is_pos_bl = is_pos_block((*it)->second.bl);
    if (pos != is_pos_bl)
      continue;
    timestamps.push_back((*it)->second.bl.timestamp);
    commulative_difficulties.push_back((*it)->second.cumulative_diff_precise);
  }

  size_t main_chain_start_offset = (alt_chain.size() ? alt_chain.front()->second.height : bei.height)-1;

  for (uint64_t i = main_chain_start_offset; i != 0 && timestamps.size() < DIFFICULTY_BLOCKS_COUNT; --i)
  {
    bool is_pos_bl = is_pos_block(m_db_blocks[i]->bl);
    if (pos != is_pos_bl)
      continue;

    timestamps.push_back(m_db_blocks[i]->bl.timestamp);
    commulative_difficulties.push_back(m_db_blocks[i]->cumulative_diff_precise);
  } 

  return next_difficulty_1(timestamps, commulative_difficulties, pos ? DIFFICULTY_POS_TARGET:DIFFICULTY_POW_TARGET);
}
//------------------------------------------------------------------
bool blockchain_storage::prevalidate_miner_transaction(const block& b, uint64_t height, bool pos) const
{
  CHECK_AND_ASSERT_MES((pos ? (b.miner_tx.vin.size() == 2) : (b.miner_tx.vin.size() == 1)), false, "coinbase transaction in the block has no inputs");
  CHECK_AND_ASSERT_MES(b.miner_tx.vin[0].type() == typeid(txin_gen), false, "coinbase transaction in the block has the wrong type");
  if(boost::get<txin_gen>(b.miner_tx.vin[0]).height != height)
  {
    LOG_PRINT_RED_L0("The miner transaction in block has invalid height: " << boost::get<txin_gen>(b.miner_tx.vin[0]).height << ", expected: " << height);
    return false;
  }
  if (pos)
  {
    CHECK_AND_ASSERT_MES(b.miner_tx.vin[1].type() == typeid(txin_to_key), false, "coinstake transaction in the block has the wrong type");
  }

  if (height > m_core_runtime_config.hard_fork_01_starts_after_height)
  {
    // new rules that allow different unlock time in coinbase outputs
    uint64_t max_unlock_time = 0;
    uint64_t min_unlock_time = 0;
    bool r = get_tx_max_min_unlock_time(b.miner_tx, max_unlock_time, min_unlock_time);
    CHECK_AND_ASSERT_MES(r && min_unlock_time >= height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW,
      false,
      "coinbase transaction has wrong min_unlock_time: " << min_unlock_time << ", expected: " << height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  }
  else
  {
    //------------------------------------------------------------------
    //bool blockchain_storage::
    // pre-hard fork rules that don't allow different unlock time in coinbase outputs
    uint64_t max_unlock_time = 0;
    uint64_t min_unlock_time = 0;
    bool r = get_tx_max_min_unlock_time(b.miner_tx, max_unlock_time, min_unlock_time);
    CHECK_AND_ASSERT_MES(r && max_unlock_time == min_unlock_time && min_unlock_time == height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW,
      false,
      "coinbase transaction has wrong min_unlock_time: " << min_unlock_time << ", expected: " << height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  }


  //check outs overflow
  if(!check_outs_overflow(b.miner_tx))
  {
    LOG_PRINT_RED_L0("miner transaction have money overflow in block " << get_block_hash(b));
    return false;
  }

  CHECK_AND_ASSERT_MES(b.miner_tx.attachment.empty(), false, "coinbase transaction has attachments; attachments are not allowed for coinbase transactions.");

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_miner_transaction(const block& b, 
                                                    size_t cumulative_block_size, 
                                                    uint64_t fee, 
                                                    uint64_t& base_reward, 
                                                    const boost::multiprecision::uint128_t& already_generated_coins) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  //validate reward
  uint64_t money_in_use = get_outs_money_amount(b.miner_tx);

  uint64_t pos_income = 0;
  if (is_pos_block(b))
  {
    CHECK_AND_ASSERT_MES(b.miner_tx.vin[1].type() == typeid(txin_to_key), false, "Wrong miner tx_in");
    pos_income = boost::get<txin_to_key>(b.miner_tx.vin[1]).amount;
  }

  std::vector<size_t> last_blocks_sizes;
  get_last_n_blocks_sizes(last_blocks_sizes, CURRENCY_REWARD_BLOCKS_WINDOW);
  size_t blocks_size_median = misc_utils::median(last_blocks_sizes);
  if (!get_block_reward(is_pos_block(b), blocks_size_median, cumulative_block_size, already_generated_coins, base_reward, get_block_height(b)))
  {
    LOG_PRINT_L0("block size " << cumulative_block_size << " is bigger than allowed for this blockchain");
    return false;
  }
  if (base_reward + pos_income + fee < money_in_use)
  {
    LOG_ERROR("coinbase transaction spend too much money (" << print_money(money_in_use) << "). Block reward is " << print_money(base_reward + pos_income + fee) << "(" << print_money(base_reward) << "+" << print_money(pos_income) << "+" << print_money(fee)
      << ", blocks_size_median = " << blocks_size_median
      << ", cumulative_block_size = " << cumulative_block_size
      << ", fee = " << fee
      << ", already_generated_coins = " << already_generated_coins
      << "), tx:");
    LOG_PRINT_L0(currency::obj_to_json_str(b.miner_tx));
    return false;
  }
  if (base_reward + pos_income + fee != money_in_use)
  {
    LOG_ERROR("coinbase transaction doesn't use full amount of block reward:  spent: (" << print_money(money_in_use) << "). Block reward is " << print_money(base_reward + pos_income + fee) << "(" << print_money(base_reward) << "+" << print_money(pos_income) << "+" << print_money(fee)
      << ", blocks_size_median = " << blocks_size_median
      << ", cumulative_block_size = " << cumulative_block_size
      << ", fee = " << fee
      << ", already_generated_coins = " << already_generated_coins
      << "), tx:");
    LOG_PRINT_L0(currency::obj_to_json_str(b.miner_tx));
    return false;
  }
  LOG_PRINT_MAGENTA("Mining tx verification ok, blocks_size_median = " << blocks_size_median, LOG_LEVEL_2);
  return true;
}
//------------------------------------------------------------------
blockchain_storage::performnce_data& blockchain_storage::get_performnce_data()const
{
  m_db.get_backend()->get_stat_info(m_performance_data.si);
  return m_performance_data; 
}
//------------------------------------------------------------------
bool blockchain_storage::get_backward_blocks_sizes(size_t from_height, std::vector<size_t>& sz, size_t count)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CHECK_AND_ASSERT_MES(from_height < m_db_blocks.size(), false, "Internal error: get_backward_blocks_sizes called with from_height=" << from_height << ", blockchain height = " << m_db_blocks.size());

  size_t start_offset = (from_height+1) - std::min((from_height+1), count);
  for(size_t i = start_offset; i != from_height+1; i++)
    sz.push_back(m_db_blocks[i]->block_cumulative_size);

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_last_n_blocks_sizes(std::vector<size_t>& sz, size_t count) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if(!m_db_blocks.size())
    return true;
  return get_backward_blocks_sizes(m_db_blocks.size() -1, sz, count);
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_current_comulative_blocksize_limit() const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return m_db_current_block_cumul_sz_limit;
}
//------------------------------------------------------------------
bool blockchain_storage::create_block_template(block& b,
                                               const account_public_address& miner_address, 
                                               wide_difficulty_type& diffic, 
                                               uint64_t& height, 
                                               const blobdata& ex_nonce) const
{
  return create_block_template(b, miner_address, miner_address, diffic, height, ex_nonce, false, pos_entry());
}
//------------------------------------------------------------------
bool blockchain_storage::create_block_template(block& b, 
                                               const account_public_address& miner_address, 
                                               const account_public_address& stakeholder_address,
                                               wide_difficulty_type& diffic, 
                                               uint64_t& height, 
                                               const blobdata& ex_nonce, 
                                               bool pos, 
                                               const pos_entry& pe,
                                               fill_block_template_func_t custom_fill_block_template_func /* = nullptr */) const
{
  create_block_template_params params = AUTO_VAL_INIT(params);
  params.miner_address = miner_address;
  params.stakeholder_address = stakeholder_address;
  params.ex_nonce = ex_nonce;
  params.pos = pos;
  params.pe = pe;
  params.pcustom_fill_block_template_func = custom_fill_block_template_func;
  create_block_template_response resp = AUTO_VAL_INIT(resp);
  bool r = create_block_template(params, resp);
  b = resp.b;
  diffic = resp.diffic;
  height = resp.height;  
  return r;
}

bool blockchain_storage::create_block_template(const create_block_template_params& params, create_block_template_response& resp) const
{
  const account_public_address& miner_address = params.miner_address; 
  const account_public_address& stakeholder_address = params.stakeholder_address;
  const blobdata& ex_nonce = params.ex_nonce;
  bool pos = params.pos;
  const pos_entry& pe = params.pe;
  fill_block_template_func_t* pcustom_fill_block_template_func = params.pcustom_fill_block_template_func;

  uint64_t& height = resp.height;
  block& b = resp.b;
  wide_difficulty_type& diffic = resp.diffic;


  size_t median_size;
  boost::multiprecision::uint128_t already_generated_coins;
  CRITICAL_REGION_BEGIN(m_read_lock);
  height = m_db_blocks.size();
  if(height <= m_core_runtime_config.hard_fork_01_starts_after_height)
    b.major_version = BLOCK_MAJOR_VERSION_INITIAL;
  else if(height <= m_core_runtime_config.hard_fork_03_starts_after_height)
    b.major_version = HF1_BLOCK_MAJOR_VERSION;
  else
    b.major_version = CURRENT_BLOCK_MAJOR_VERSION;

  b.minor_version = CURRENT_BLOCK_MINOR_VERSION;
  b.prev_id = get_top_block_id();
  b.timestamp = m_core_runtime_config.get_core_time();
  b.nonce = 0;
  b.flags = 0;
  if (pos)
  {
    b.flags |= CURRENCY_BLOCK_FLAG_POS_BLOCK;
    b.timestamp = 0;
  }

  diffic = get_next_diff_conditional(pos);

  CHECK_AND_ASSERT_MES(diffic, false, "difficulty owverhead.");


  

  median_size = m_db_current_block_cumul_sz_limit / 2;
  already_generated_coins = m_db_blocks.back()->already_generated_coins;

  CRITICAL_REGION_END();

  size_t txs_size = 0;
  uint64_t fee = 0;
  bool block_filled = false;
  if (pcustom_fill_block_template_func == nullptr)
    block_filled = m_tx_pool.fill_block_template(b, pos, median_size, already_generated_coins, txs_size, fee, height, params.explicit_txs);
  else
    block_filled = (*pcustom_fill_block_template_func)(b, pos, median_size, already_generated_coins, txs_size, fee, height);

  if (!block_filled)
    return false;

  /* 
      instead of complicated two-phase template construction and adjustment of cumulative size with block reward we
      use CURRENCY_COINBASE_BLOB_RESERVED_SIZE as penalty-free coinbase transaction reservation.
  */
  bool r = construct_miner_tx(height, median_size, already_generated_coins,  
                                                   txs_size, 
                                                   fee, 
                                                   miner_address, 
                                                   stakeholder_address,
                                                   b.miner_tx, ex_nonce, 
                                                   CURRENCY_MINER_TX_MAX_OUTS, 
                                                   pos,
                                                   pe);
  CHECK_AND_ASSERT_MES(r, false, "Failed to construc miner tx, first chance");
  uint64_t coinbase_size = get_object_blobsize(b.miner_tx);
  // "- 100" - to reserve room for PoS additions into miner tx
  CHECK_AND_ASSERT_MES(coinbase_size < CURRENCY_COINBASE_BLOB_RESERVED_SIZE - 100, false, "Failed to get block template (coinbase_size = " << coinbase_size << ") << coinbase structue: " 
    << ENDL << obj_to_json_str(b.miner_tx));
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::print_transactions_statistics() const 
{
  LOG_ERROR("print_transactions_statistics not implemented yet");
//   CRITICAL_REGION_LOCAL(m_blockchain_lock);
//   LOG_PRINT_L0("Started to collect transaction statistics, pleas wait...");
//   size_t total_count = 0;
//   size_t coinbase_count = 0;
//   size_t total_full_blob = 0;
//   size_t total_cropped_blob = 0;
//   for(auto tx_entry: m_db_transactions)
//   {
//     ++total_count;
//     if(is_coinbase(tx_entry.second.tx))
//       ++coinbase_count;
//     else
//     {
//       total_full_blob += get_object_blobsize<transaction>(tx_entry.second.tx);
//       transaction tx = tx_entry.second.tx;
//       tx.signatures.clear();
//       total_cropped_blob += get_object_blobsize<transaction>(tx);
//     }    
//   }
//   LOG_PRINT_L0("Done" << ENDL
//       << "total transactions: " << total_count << ENDL 
//       << "coinbase transactions: " << coinbase_count << ENDL 
//       << "avarage size of transaction: " << total_full_blob/(total_count-coinbase_count) << ENDL
//       << "avarage size of transaction without ring signatures: " << total_cropped_blob/(total_count-coinbase_count) << ENDL
//       );
  return true;
}
void blockchain_storage::reset_db_cache() const
{
  m_db_blocks.clear_cache();
  m_db_blocks_index.clear_cache();
  m_db_transactions.clear_cache();
  m_db_spent_keys.clear_cache();
  m_db_solo_options.clear_cache();
  m_db_multisig_outs.clear_cache();
  m_db_aliases.clear_cache();
  m_db_addr_to_alias.clear_cache();

}
//------------------------------------------------------------------
void blockchain_storage::clear_altblocks()
{
  CRITICAL_REGION_LOCAL(m_alternative_chains_lock);
  m_alternative_chains.clear();
  m_alternative_chains_txs.clear();
  m_altblocks_keyimages.clear();
}
//------------------------------------------------------------------
bool blockchain_storage::complete_timestamps_vector(uint64_t start_top_height, std::vector<uint64_t>& timestamps)
{

  if(timestamps.size() >= BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW)
    return true;

  CRITICAL_REGION_LOCAL(m_read_lock);
  size_t need_elements = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW - timestamps.size();
  CHECK_AND_ASSERT_MES(start_top_height < m_db_blocks.size(), false, "internal error: passed start_height = " << start_top_height << " not less then m_blocks.size()=" << m_db_blocks.size());
  size_t stop_offset = start_top_height > need_elements ? start_top_height - need_elements:0;
  do
  {
    timestamps.push_back(m_db_blocks[start_top_height]->bl.timestamp);
    if(start_top_height == 0)
      break;
    --start_top_height;
  }while(start_top_height != stop_offset);
  return true;
}
//------------------------------------------------------------------
std::string blockchain_storage::print_alt_chain(alt_chain_type alt_chain)
{
  std::stringstream ss;
  for (auto it = alt_chain.begin(); it != alt_chain.end(); it++)
  {
    ss << "[" << (*it)->second.height << "] " << (*it)->first << "(cumul_dif: " << (*it)->second.cumulative_diff_adjusted << "), parent_id: " << (*it)->second.bl.prev_id << ENDL;
  }
  return ss.str();
}
//------------------------------------------------------------------
bool blockchain_storage::append_altblock_keyimages_to_big_heap(const crypto::hash& block_id, const std::unordered_set<crypto::key_image>& alt_block_keyimages)
{
  for (auto& ki : alt_block_keyimages)
    m_altblocks_keyimages[ki].push_back(block_id);
  
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::purge_keyimage_from_big_heap(const crypto::key_image& ki, const crypto::hash& id)
{
  auto it = m_altblocks_keyimages.find(ki);
  CHECK_AND_ASSERT_MES(it != m_altblocks_keyimages.end(), false, "internal error: keyimage " << ki 
    << "from altblock " << id << "not found in m_altblocks_keyimages in purge_keyimage_from_big_heap");
  
  //TODO: at the moment here is simple liner algo since having the same txs in few altblocks is rare case
  std::list<crypto::hash>& ki_blocks_ids = it->second;
  bool found = false;
  for (auto it_in_blocks = ki_blocks_ids.begin(); it_in_blocks != ki_blocks_ids.end(); it_in_blocks++)
  {
    if (*it_in_blocks == id)
    {
      //found, erase this entry
      ki_blocks_ids.erase(it_in_blocks);
      found = true;
      break;
    }
  }
  CHECK_AND_ASSERT_MES(found, false, "internal error: keyimage " << ki
    << "from altblock " << id << "not found in m_altblocks_keyimages in purge_keyimage_from_big_heap");
  if (!ki_blocks_ids.size())
    m_altblocks_keyimages.erase(it);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::purge_altblock_keyimages_from_big_heap(const block& b, const crypto::hash& id)
{
  if (is_pos_block(b))
  {
    CHECK_AND_ASSERT_MES(b.miner_tx.vin.size()>=2, false, "paranoid check failed");
    CHECK_AND_ASSERT_MES(b.miner_tx.vin[1].type() == typeid(txin_to_key), false, "paranoid type check failed");
    purge_keyimage_from_big_heap(boost::get<txin_to_key>(b.miner_tx.vin[1]).k_image, id);
  }
  for (auto tx_id : b.tx_hashes)
  {
    std::shared_ptr<transaction> tx_ptr;
    if (!get_transaction_from_pool_or_db(tx_id, tx_ptr))
    {
      continue;
    }
    transaction& tx = *tx_ptr;
    for (size_t n = 0; n < tx.vin.size(); ++n)
    {
      if (tx.vin[n].type() == typeid(txin_to_key) || tx.vin[n].type() == typeid(txin_htlc))
      {
        purge_keyimage_from_big_heap(get_to_key_input_from_txin_v(tx.vin[n]).k_image, id);
      }
    }
  }
  return true;
}

//------------------------------------------------------------------
bool blockchain_storage::handle_alternative_block(const block& b, const crypto::hash& id, block_verification_context& bvc)
{
  uint64_t coinbase_height = get_block_height(b);
  if (m_checkpoints.is_height_passed_zone(coinbase_height, get_top_block_height()))
  {
    LOG_PRINT_RED_L0("Block with id: " << id << "[" << coinbase_height << "]" << ENDL << " for alternative chain, is under checkpoint zone, declined");
    bvc.m_verification_failed = true;
    return false;
  }

  TRY_ENTRY();


  CRITICAL_REGION_LOCAL(m_read_lock);
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);

  //block is not related with head of main chain
  //first of all - look in alternative chains container

  auto ptr_main_prev = m_db_blocks_index.find(b.prev_id);
  auto it_prev = m_alternative_chains.find(b.prev_id);
  if (it_prev != m_alternative_chains.end() || ptr_main_prev != m_db_blocks_index.end())
  {
    alt_chain_type alt_chain;
    {
      //we have new block in alternative chain
      //build alternative subchain, front -> mainchain, back -> alternative head
      alt_chain_container::iterator alt_it = it_prev; //m_alternative_chains.find()
      std::vector<uint64_t> timestamps;
      std::list<alt_chain_container::iterator> temp_container;
      while (alt_it != m_alternative_chains.end())
      {
        temp_container.push_front(alt_it);
        timestamps.push_back(alt_it->second.bl.timestamp);
        alt_it = m_alternative_chains.find(alt_it->second.bl.prev_id);
      }
      //TODO: refactoring needed: vector push_front is dramatically ineffective
      alt_chain.resize(temp_container.size());
      auto it_vec = alt_chain.begin();
      for (auto it = temp_container.begin(); it != temp_container.end();it++, it_vec++)
      {
        *it_vec = *it;
      }


      if (alt_chain.size())
      {
        //make sure that it has right connection to main chain
        CHECK_AND_ASSERT_MES_CUSTOM(m_db_blocks.size() >= alt_chain.front()->second.height, false, bvc.m_verification_failed = true, "main blockchain wrong height:  m_db_blocks.size() = " << m_db_blocks.size()
          << " and alt_chain.front()->second.height = " << alt_chain.front()->second.height
          << " for block " << id << ", prev_id=" << b.prev_id << ENDL
          << " alt chain: " << ENDL << print_alt_chain(alt_chain) << ENDL
          << " main chain: " << ENDL << get_blockchain_string(m_db_blocks.size() - 10, CURRENCY_MAX_BLOCK_NUMBER)
        );

        crypto::hash h = null_hash;
        get_block_hash(m_db_blocks[alt_chain.front()->second.height - 1]->bl, h);
        CHECK_AND_ASSERT_MES_CUSTOM(h == alt_chain.front()->second.bl.prev_id, false, bvc.m_verification_failed = true, "alternative chain have wrong connection to main chain");
        complete_timestamps_vector(alt_chain.front()->second.height - 1, timestamps);
      }
      else
      {
        CHECK_AND_ASSERT_MES_CUSTOM(ptr_main_prev != m_db_blocks_index.end(), false, bvc.m_verification_failed = true, "internal error: broken imperative condition it_main_prev != m_blocks_index.end()");
        complete_timestamps_vector(*ptr_main_prev, timestamps);
      }
      //check timestamp correct
      if (!check_block_timestamp(std::move(timestamps), b))
      {
        LOG_PRINT_RED_L0("Block with id: " << id
          << ENDL << " for alternative chain, have invalid timestamp: " << b.timestamp);
        //add_block_as_invalid(b, id);//do not add blocks to invalid storage before proof of work check was passed
        bvc.m_verification_failed = true;
        return false;
      }
    }

    alt_block_extended_info abei = AUTO_VAL_INIT(abei);
    abei.bl = b;
    abei.onboard_transactions.swap(bvc.m_onboard_transactions);
    //for altblocks we should be sure that all transactions kept in onboard_transactions
    for (auto& h: abei.bl.tx_hashes)
    {
      if (abei.onboard_transactions.count(h) == 0)
      {
        //need to take if from pool
        transaction tx = AUTO_VAL_INIT(tx);
        bool r = m_tx_pool.get_transaction(h, tx);
        if (!r)
        {
          //transaction could be in main chain 
          auto tx_ptr = m_db_transactions.find(h);
          if (!tx_ptr)
          {
            LOG_ERROR("Transaction " << h  << " for altblock " << get_block_hash(abei.bl) << " not found");
          }
          else
          {
            abei.onboard_transactions[h] = tx_ptr->tx;
          }
        }
        else
        {
          abei.onboard_transactions[h] = tx;
        }
      }
    }

    abei.timestamp = m_core_runtime_config.get_core_time();
    abei.height = alt_chain.size() ? it_prev->second.height + 1 : *ptr_main_prev + 1;
    CHECK_AND_ASSERT_MES_CUSTOM(coinbase_height == abei.height, false, bvc.m_verification_failed = true, "block coinbase height doesn't match with altchain height, declined");
    uint64_t connection_height = alt_chain.size() ? alt_chain.front()->second.height:abei.height;
    CHECK_AND_ASSERT_MES_CUSTOM(connection_height, false, bvc.m_verification_failed = true, "INTERNAL ERROR: Wrong connection_height==0 in handle_alternative_block");

    if (!m_checkpoints.is_in_checkpoint_zone(abei.height))
    {
      m_is_in_checkpoint_zone = false;
    }
    else
    {
      m_is_in_checkpoint_zone = true;
      if (!m_checkpoints.check_block(abei.height, id))
      {
        LOG_ERROR("CHECKPOINT VALIDATION FAILED");
        bvc.m_verification_failed = true;
        return false;
      }
    }

    bool pos_block = is_pos_block(abei.bl);
    //check if PoS block allowed on this height
    CHECK_AND_ASSERT_MES_CUSTOM(!(pos_block && abei.height < m_core_runtime_config.pos_minimum_heigh), false, bvc.m_verification_failed = true, "PoS block is not allowed on this height");


    wide_difficulty_type current_diff = get_next_diff_conditional2(pos_block, alt_chain, connection_height, abei);
    
    CHECK_AND_ASSERT_MES_CUSTOM(current_diff, false, bvc.m_verification_failed = true, "!!!!!!! DIFFICULTY OVERHEAD !!!!!!!");
    
    crypto::hash proof_of_work = null_hash;
    uint64_t pos_amount = 0;
    wide_difficulty_type pos_diff_final = 0;
    if (pos_block)
    {
      //POS
      bool res = validate_pos_block(abei.bl, current_diff, pos_amount, pos_diff_final, abei.stake_hash, id, true, alt_chain, connection_height);
      CHECK_AND_ASSERT_MES_CUSTOM(res, false, bvc.m_verification_failed = true, "Failed to validate_pos_block on alternative block, height = "
                                        << abei.height 
                                        << ", block id: " << get_block_hash(abei.bl));
    }
    else
    {
      proof_of_work = get_block_longhash(abei.bl);

      if (!check_hash(proof_of_work, current_diff))
      {
        LOG_PRINT_RED_L0("Block with id: " << id
          << ENDL << " for alternative chain, have not enough proof of work: " << proof_of_work
          << ENDL << " expected difficulty: " << current_diff);
        bvc.m_verification_failed = true;
        return false;
      }
      //
    }

    if (!prevalidate_miner_transaction(b, abei.height, pos_block))
    {
      LOG_PRINT_RED_L0("Block with id: " << string_tools::pod_to_hex(id)
        << " (as alternative) have wrong miner transaction.");
      bvc.m_verification_failed = true;
      return false;
    }
    std::unordered_set<crypto::key_image> alt_block_keyimages;
    uint64_t ki_lookup_total = 0;
    if (!validate_alt_block_txs(b, id, alt_block_keyimages, abei, alt_chain, connection_height, ki_lookup_total))
    {
      LOG_PRINT_RED_L0("Alternative block " << id << " @ " << abei.height << " has invalid transactions. Rejected.");
      bvc.m_verification_failed = true;
      return false;
    }

    abei.difficulty = current_diff;
    wide_difficulty_type cumulative_diff_delta = 0;
    abei.cumulative_diff_adjusted = alt_chain.size() ? it_prev->second.cumulative_diff_adjusted : m_db_blocks[*ptr_main_prev]->cumulative_diff_adjusted;
    
    if (pos_block)
      cumulative_diff_delta = get_adjusted_cumulative_difficulty_for_next_alt_pos(alt_chain, abei.height, current_diff, connection_height);
    else
      cumulative_diff_delta = current_diff;

    size_t sequence_factor = get_current_sequence_factor_for_alt(alt_chain, pos_block, connection_height);
    if (abei.height >= m_core_runtime_config.pos_minimum_heigh)
      cumulative_diff_delta = correct_difficulty_with_sequence_factor(sequence_factor, cumulative_diff_delta);

    if (abei.height > BLOCKCHAIN_HEIGHT_FOR_POS_STRICT_SEQUENCE_LIMITATION && abei.height <= m_core_runtime_config.hard_fork_01_starts_after_height && pos_block && sequence_factor > BLOCK_POS_STRICT_SEQUENCE_LIMIT)
    {
      LOG_PRINT_RED_L0("Alternative block " << id << " @ " << abei.height << " has too big sequence factor: " << sequence_factor << ", rejected");
      bvc.m_verification_failed = true;
      return false;
    }

    abei.cumulative_diff_adjusted += cumulative_diff_delta;
    wide_difficulty_type last_x_cumul_dif_precise_adj = 0;
    abei.cumulative_diff_precise = get_last_alt_x_block_cumulative_precise_difficulty(alt_chain, abei.height-1, pos_block, last_x_cumul_dif_precise_adj);
    abei.cumulative_diff_precise += current_diff;
    //////////////////////////////////////////////////////////////////////////

    wide_difficulty_type diff_precise_adj = correct_difficulty_with_sequence_factor(sequence_factor, current_diff);
    abei.cumulative_diff_precise_adjusted = last_x_cumul_dif_precise_adj + diff_precise_adj;
    
    //////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
    auto i_dres = m_alternative_chains.find(id);
    CHECK_AND_ASSERT_MES_CUSTOM(i_dres == m_alternative_chains.end(), false, bvc.m_verification_failed = true, "insertion of new alternative block " << id << " returned as it already exist");
#endif
    auto i_res = m_alternative_chains.insert(alt_chain_container::value_type(id, abei));
    CHECK_AND_ASSERT_MES_CUSTOM(i_res.second, false, bvc.m_verification_failed = true, "insertion of new alternative block " << id << " returned as it already exist");
    append_altblock_keyimages_to_big_heap(id, alt_block_keyimages);
    add_alt_block_txs_hashs(i_res.first->second.bl);
    alt_chain.push_back(i_res.first);
    //check if difficulty bigger then in main chain

    bvc.m_height_difference = get_top_block_height() >= abei.height ? get_top_block_height() - abei.height : 0;

    crypto::hash proof = null_hash;
    std::stringstream ss_pow_pos_info;
    if (pos_block)
    {
      ss_pow_pos_info << "PoS:\t" << abei.stake_hash << ", stake amount: " << print_money(pos_amount) << ", final_difficulty: " << pos_diff_final;
      proof = abei.stake_hash;
    }
    else
    {
      ss_pow_pos_info << "PoW:\t" << proof_of_work;
      proof = proof_of_work;
    }
        
    LOG_PRINT_BLUE("----- BLOCK ADDED AS ALTERNATIVE ON HEIGHT " << abei.height << (pos_block ? " [PoS] Sq: " : " [PoW] Sq: ") << sequence_factor << ", altchain sz: " << alt_chain.size() << ", split h: " << connection_height
      << ENDL << "id:\t" << id
      << ENDL << "prev\t" << abei.bl.prev_id
      << ENDL << ss_pow_pos_info.str()
      << ENDL << "HEIGHT " << abei.height << ", difficulty: " << abei.difficulty << ", cumul_diff_precise: " << abei.cumulative_diff_precise << ", cumul_diff_adj: " << abei.cumulative_diff_adjusted << " (current mainchain cumul_diff_adj: " << m_db_blocks.back()->cumulative_diff_adjusted << ", ki lookup total: " << ki_lookup_total <<")"
      , LOG_LEVEL_0);

    if (is_reorganize_required(*m_db_blocks.back(), alt_chain, proof))
    {
      auto a = epee::misc_utils::create_scope_leave_handler([&]() { m_is_reorganize_in_process = false; });
      CHECK_AND_ASSERT_THROW_MES(!m_is_reorganize_in_process, "Detected recursive reorganzie");
      m_is_reorganize_in_process = true;
      //do reorganize!
      LOG_PRINT_GREEN("###### REORGANIZE on height: " << alt_chain.front()->second.height << " of " << m_db_blocks.size() - 1 << " with cumulative_diff_adjusted " << m_db_blocks.back()->cumulative_diff_adjusted
        << ENDL << " alternative blockchain size: " << alt_chain.size() << " with cumulative_diff_adjusted " << abei.cumulative_diff_adjusted, LOG_LEVEL_0);
      bool r = switch_to_alternative_blockchain(alt_chain);
      if(r) 
        bvc.m_added_to_main_chain = true;
      else 
        bvc.m_verification_failed = true;
      return r;
    }
    bvc.m_added_to_altchain = true;

    //protect ourself from altchains container flood
    if (m_alternative_chains.size() > m_core_runtime_config.max_alt_blocks)
      prune_aged_alt_blocks();

    return true;
  }else
  {
    //block orphaned
    bvc.m_marked_as_orphaned = true;

    if (m_invalid_blocks.count(id) != 0)
    {
      LOG_PRINT_RED_L0("Block recognized as blacklisted and rejected, id = " << id << "," << ENDL << "parent id = " << b.prev_id << ENDL << "height = " << coinbase_height);
    }
    else if (m_invalid_blocks.count(b.prev_id) != 0)
    {
      LOG_PRINT_RED_L0("Block recognized as orphaned (parent " << b.prev_id << " is in blacklist) and rejected, id = " << id << "," << ENDL << "parent id = " << b.prev_id << ENDL << "height = " << coinbase_height);
    }
    else
    {
      LOG_PRINT_RED_L0("Block recognized as orphaned and rejected, id = " << id << "," << ENDL << "parent id = " << b.prev_id << ENDL << "height = " << coinbase_height);
    }
  }
  
  CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL!");
  return true;
  CATCH_ENTRY_CUSTOM("blockchain_storage::handle_alternative_block", bvc.m_verification_failed = true, false);
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_x_difficulty_after_height(uint64_t height, bool is_pos)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CHECK_AND_ASSERT_THROW_MES(height < m_db_blocks.size(), "Internal error: condition failed: height (" << height << ") < m_db_blocks.size() " << m_db_blocks.size());
  wide_difficulty_type diff = 0;
  for (uint64_t i = height + 1; i != m_db_blocks.size(); i++)
  {
    auto bei_ptr = m_db_blocks[i];
    if (is_pos_block(bei_ptr->bl) == is_pos)
    {
      diff = bei_ptr->difficulty;
      break;
    }
  }
  if (diff == 0)
  {
    //never met x type of block, that meanst that difficulty is current
    diff = get_cached_next_difficulty(is_pos);
  }
  return diff;
}
//------------------------------------------------------------------
bool blockchain_storage::is_reorganize_required(const block_extended_info& main_chain_bei, const alt_chain_type& alt_chain, const crypto::hash& proof_alt)
{
  //alt_chain - back is latest(top), first - connection with main chain 
  const block_extended_info& alt_chain_bei = alt_chain.back()->second;
  const block_extended_info& connection_point = alt_chain.front()->second;

  if (connection_point.height <= m_core_runtime_config.hard_fork_01_starts_after_height)
  {
    //use pre-hard fork, old-style comparing
    if (main_chain_bei.cumulative_diff_adjusted < alt_chain_bei.cumulative_diff_adjusted)
      return true;
    else if (main_chain_bei.cumulative_diff_adjusted > alt_chain_bei.cumulative_diff_adjusted)
      return false;
    else // main_chain_bei.cumulative_diff_adjusted == alt_chain_bei.cumulative_diff_adjusted
    {
      if (!is_pos_block(main_chain_bei.bl))
        return false; // do not reorganize on the same cummul diff if it's a PoW block

                      //in case of simultaneous PoS blocks are happened on the same height (quite common for PoS) 
                      //we also try to weight them to guarantee consensus in network
      if (std::memcmp(&main_chain_bei.stake_hash, &proof_alt, sizeof(main_chain_bei.stake_hash)) >= 0)
        return false;

      LOG_PRINT_L2("[is_reorganize_required]:TRUE, \"by order of memcmp\" main_stake_hash:" << &main_chain_bei.stake_hash << ", alt_stake_hash" << proof_alt);
      return true;
    }
  }
  else if (alt_chain_bei.height > m_core_runtime_config.hard_fork_01_starts_after_height)
  {
    //new rules, applied after HARD_FORK_1
    //to learn this algo please read https://github.com/hyle-team/docs/blob/master/zano/PoS_Analysis_and_improvements_proposal.pdf

    wide_difficulty_type difficulty_pos_at_split_point = get_x_difficulty_after_height(connection_point.height - 1, true);
    wide_difficulty_type difficulty_pow_at_split_point = get_x_difficulty_after_height(connection_point.height - 1, false);

    difficulties main_cumul_diff = AUTO_VAL_INIT(main_cumul_diff);
    difficulties alt_cumul_diff = AUTO_VAL_INIT(alt_cumul_diff);
    //we use get_last_alt_x_block_cumulative_precise_adj_difficulty for getting both alt chain and main chain diff of given block types

    wide_difficulty_type alt_pos_diff_end = get_last_alt_x_block_cumulative_precise_adj_difficulty(alt_chain, alt_chain_bei.height, true);
    wide_difficulty_type alt_pos_diff_begin = get_last_alt_x_block_cumulative_precise_adj_difficulty(alt_chain_type(), connection_point.height-1, true);
    alt_cumul_diff.pos_diff = alt_pos_diff_end - alt_pos_diff_begin;
    
    wide_difficulty_type alt_pow_diff_end = get_last_alt_x_block_cumulative_precise_adj_difficulty(alt_chain, alt_chain_bei.height, false);
    wide_difficulty_type alt_pow_diff_begin = get_last_alt_x_block_cumulative_precise_adj_difficulty(alt_chain_type(), connection_point.height - 1, false);
    alt_cumul_diff.pow_diff = alt_pow_diff_end - alt_pow_diff_begin;

    wide_difficulty_type main_pos_diff_end = get_last_alt_x_block_cumulative_precise_adj_difficulty(alt_chain_type(), m_db_blocks.size()-1, true);
    wide_difficulty_type main_pos_diff_begin = get_last_alt_x_block_cumulative_precise_adj_difficulty(alt_chain_type(), connection_point.height - 1, true);
    main_cumul_diff.pos_diff = main_pos_diff_end - main_pos_diff_begin;

    wide_difficulty_type main_pow_diff_end = get_last_alt_x_block_cumulative_precise_adj_difficulty(alt_chain_type(), m_db_blocks.size() - 1, false);
    wide_difficulty_type main_pow_diff_begin = get_last_alt_x_block_cumulative_precise_adj_difficulty(alt_chain_type(), connection_point.height - 1, false);
    main_cumul_diff.pow_diff = main_pow_diff_end - main_pow_diff_begin;

    //TODO: measurement of precise cumulative difficult
    boost::multiprecision::uint1024_t alt = get_a_to_b_relative_cumulative_difficulty(difficulty_pos_at_split_point, difficulty_pow_at_split_point, alt_cumul_diff, main_cumul_diff);
    boost::multiprecision::uint1024_t main = get_a_to_b_relative_cumulative_difficulty(difficulty_pos_at_split_point, difficulty_pow_at_split_point, main_cumul_diff, alt_cumul_diff);
    LOG_PRINT_L1("[FORK_CHOICE]: " << ENDL 
      << "difficulty_pow_at_split_point:" << difficulty_pow_at_split_point << ENDL
      << "difficulty_pos_at_split_point:" << difficulty_pos_at_split_point << ENDL
      << "alt_cumul_diff.pow_diff:" << alt_cumul_diff.pow_diff << ENDL
      << "alt_cumul_diff.pos_diff:" << alt_cumul_diff.pos_diff << ENDL
      << "main_cumul_diff.pow_diff:" << main_cumul_diff.pow_diff << ENDL
      << "main_cumul_diff.pos_diff:" << main_cumul_diff.pos_diff << ENDL
      << "alt:" << alt << ENDL
      << "main:" << main << ENDL
    );
    if (main < alt)
      return true;
    else if (main > alt)
      return false;
    else
    {
      if (!is_pos_block(main_chain_bei.bl))
        return false; // do not reorganize on the same cummul diff if it's a PoW block

                      //in case of simultaneous PoS blocks are happened on the same height (quite common for PoS) 
                      //we also try to weight them to guarantee consensus in network
      if (std::memcmp(&main_chain_bei.stake_hash, &proof_alt, sizeof(main_chain_bei.stake_hash)) >= 0)
        return false;

      LOG_PRINT_L1("[is_reorganize_required]:TRUE, \"by order of memcmp\" main_stake_hash:" << &main_chain_bei.stake_hash << ", alt_stake_hash" << proof_alt);
      return true;
    }
  }
  else
  {
    ASSERT_MES_AND_THROW("Unknown version of block");
  }
}

//------------------------------------------------------------------
bool blockchain_storage::pre_validate_relayed_block(block& bl, block_verification_context& bvc, const crypto::hash& id)const 
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if (!(bl.prev_id == get_top_block_id()))
  {
    bvc.m_added_to_main_chain = false;
    bvc.m_verification_failed = false;
    return true;
  }
  //check proof of work
  bool is_pos_bl = is_pos_block(bl);
  wide_difficulty_type current_diffic = get_next_diff_conditional(is_pos_bl);
  CHECK_AND_ASSERT_MES_CUSTOM(current_diffic, false, bvc.m_verification_failed = true, "!!!!!!!!! difficulty overhead !!!!!!!!!");
  crypto::hash proof_hash = AUTO_VAL_INIT(proof_hash);
  if (is_pos_bl)
  {
    wide_difficulty_type this_coin_diff = 0;
    uint64_t amount = 0;
    bool r = validate_pos_block(bl, current_diffic, amount, this_coin_diff, proof_hash, id, false);
    CHECK_AND_ASSERT_MES_CUSTOM(r, false, bvc.m_verification_failed = true, "validate_pos_block failed!!");
    bvc.m_added_to_main_chain = true;
  }
  else
  {    
    proof_hash = get_block_longhash(bl); //get_block_longhash(bl);

    if (!check_hash(proof_hash, current_diffic))
    {
      LOG_PRINT_L0("Block with id: " << id << ENDL
        << "	: " << proof_hash << ENDL
        << "unexpected difficulty: " << current_diffic);
      bvc.m_verification_failed = true;
      bvc.m_added_to_main_chain = true;
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if(start_offset >= m_db_blocks.size())
    return false;
  for(size_t i = start_offset; i < start_offset + count && i < m_db_blocks.size();i++)
  {
    blocks.push_back(m_db_blocks[i]->bl);
    std::list<crypto::hash> missed_ids;
    get_transactions(m_db_blocks[i]->bl.tx_hashes, txs, missed_ids);
    CHECK_AND_ASSERT_MES(!missed_ids.size(), false, "have missed transactions in own block in main blockchain");
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_tx_rpc_details(const crypto::hash& h, tx_rpc_extended_info& tei, uint64_t timestamp, bool is_short) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto tx_ptr = m_db_transactions.get(h);
  if (!tx_ptr)
  {
    tei.keeper_block = -1; // tx is not confirmed yet, probably it's in the pool
    return false;
  }
  

  if (tx_ptr && !timestamp)
  {
    timestamp = get_block_datetime(m_db_blocks[tx_ptr->m_keeper_block_height]->bl);
  }
  tei.keeper_block = static_cast<int64_t>(tx_ptr->m_keeper_block_height);
  fill_tx_rpc_details(tei, tx_ptr->tx, &(*tx_ptr), h, timestamp, is_short);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::search_by_id(const crypto::hash& id, std::list<std::string>& res) const
{
  auto block_ptr = m_db_blocks_index.get(id);
  if (block_ptr)
  {
    res.push_back("block");
  }

  auto tx_ptr = m_db_transactions.get(id);
  if (tx_ptr)
  {
    res.push_back("tx");
  }

  auto ki_ptr = m_db_spent_keys.get( *reinterpret_cast<const crypto::key_image*>(&id));
  if (ki_ptr)
  {
    res.push_back("key_image");
  }

  auto ms_ptr = m_db_multisig_outs.get(id);
  if (ms_ptr)
  {
    res.push_back(std::string("multisig_id:") + epee::string_tools::pod_to_hex(ms_ptr->tx_id) + ":" + std::to_string(ms_ptr->out_no));
  }

  if (m_alternative_chains.end() != m_alternative_chains.find(id))
  {
    res.push_back("alt_block");
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_global_index_details(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::response & resp) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  try
  {
    auto out_ptr = m_db_outputs.get_subitem(req.amount, req.i); // get_subitem can rise an out_of_range exception
    if (!out_ptr)
      return false;
    resp.tx_id = out_ptr->tx_id;
    resp.out_no = out_ptr->out_no;
    return true;
  }
  catch (std::out_of_range&)
  {
    return false;
  }
}
//------------------------------------------------------------------
bool blockchain_storage::get_multisig_id_details(const COMMAND_RPC_GET_MULTISIG_INFO::request& req, COMMAND_RPC_GET_MULTISIG_INFO::response & resp) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return get_multisig_id_details(req.ms_id, resp.tx_id, resp.out_no);
}
//------------------------------------------------------------------
bool blockchain_storage::get_multisig_id_details(const crypto::hash& ms_id, crypto::hash& tx_id, uint64_t& out_no) const
{
  auto out_ptr = m_db_multisig_outs.get(ms_id);
  if (!out_ptr)
    return false;

  tx_id = out_ptr->tx_id;
  out_no = out_ptr->out_no;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_main_block_rpc_details(uint64_t i, block_rpc_extended_info& bei) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto core_bei_ptr = m_db_blocks[i];
  crypto::hash id = get_block_hash(core_bei_ptr->bl);
  bei.is_orphan = false;
  bei.total_fee = 0;
  bei.total_txs_size = 0;
  if (true/*!ignore_transactions*/)
  {
    crypto::hash coinbase_id = get_transaction_hash(core_bei_ptr->bl.miner_tx);
    //load transactions details
    bei.transactions_details.push_back(tx_rpc_extended_info());
    get_tx_rpc_details(coinbase_id, bei.transactions_details.back(), get_block_datetime(core_bei_ptr->bl), true);
    for (auto& h : core_bei_ptr->bl.tx_hashes)
    {
      bei.transactions_details.push_back(tx_rpc_extended_info());
      get_tx_rpc_details(h, bei.transactions_details.back(), get_block_datetime(core_bei_ptr->bl), true);
      bei.total_fee += bei.transactions_details.back().fee;
      bei.total_txs_size += bei.transactions_details.back().blob_size;
    }
  }
  fill_block_rpc_details(bei, *core_bei_ptr, id);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_main_block_rpc_details(const crypto::hash& id, block_rpc_extended_info& bei) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto iptr = m_db_blocks_index.get(id);
  if (!iptr)
    return false;
  return get_main_block_rpc_details(*iptr, bei);
}
//------------------------------------------------------------------
bool blockchain_storage::get_alt_blocks_rpc_details(uint64_t start_offset, uint64_t count, std::vector<block_rpc_extended_info>& blocks) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);

  if (start_offset >= m_alternative_chains.size() || count == 0)
    return true; // empty result

  if (start_offset + count >= m_alternative_chains.size())
    count = m_alternative_chains.size() - start_offset; // correct count if it's too big

  // collect iterators to all the alt blocks for speedy sorting
  std::vector<alt_chain_container::const_iterator> blocks_its;
  blocks_its.reserve(m_alternative_chains.size());
  for (alt_chain_container::const_iterator it = m_alternative_chains.begin(); it != m_alternative_chains.end(); ++it)
    blocks_its.push_back(it);

  // partially sort blocks by height, so only 0...(start_offset+count-1) first blocks are sorted
  std::partial_sort(blocks_its.begin(), blocks_its.begin() + start_offset + count, blocks_its.end(),
    [](const alt_chain_container::const_iterator &lhs, const alt_chain_container::const_iterator& rhs) ->bool {
      return lhs->second.height < rhs->second.height;
    }
  );

  // erase blocks from 0 till start_offset-1
  blocks_its.erase(blocks_its.begin(), blocks_its.begin() + start_offset);
  
  // erase the tail
  blocks_its.erase(blocks_its.begin() + count, blocks_its.end());

  // populate the result
  blocks.reserve(blocks_its.size());
  for(auto it : blocks_its)
  {
    blocks.push_back(block_rpc_extended_info());
    get_alt_block_rpc_details(it->second, it->first, blocks.back());
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_alt_block_rpc_details(const crypto::hash& id, block_rpc_extended_info& bei) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);
  auto it = m_alternative_chains.find(id);
  if (it == m_alternative_chains.end())
    return false;

  const block_extended_info& bei_core = it->second;
  return get_alt_block_rpc_details(bei_core, id, bei);
}
//------------------------------------------------------------------
bool blockchain_storage::get_alt_block_rpc_details(const block_extended_info& bei_core, const crypto::hash& id, block_rpc_extended_info& bei) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);

  bei.is_orphan = true;

  crypto::hash coinbase_id = get_transaction_hash(bei_core.bl.miner_tx);
  //load transactions details
  bei.transactions_details.push_back(tx_rpc_extended_info());
  fill_tx_rpc_details(bei.transactions_details.back(), bei_core.bl.miner_tx, nullptr, coinbase_id, get_block_datetime(bei_core.bl));

  bei.total_fee = 0;
  for (auto& h : bei_core.bl.tx_hashes)
  {
    bei.transactions_details.push_back(tx_rpc_extended_info());
    if (!get_tx_rpc_details(h, bei.transactions_details.back(), get_block_datetime(bei_core.bl), true))
    {
      //tx not in blockchain, supposed to be in tx pool
      m_tx_pool.get_transaction_details(h, bei.transactions_details.back());
    }
    bei.total_fee += bei.transactions_details.back().fee;
  }

  fill_block_rpc_details(bei, bei_core, id);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_main_blocks_rpc_details(uint64_t start_offset, size_t count, bool ignore_transactions, std::list<block_rpc_extended_info>& blocks) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if (start_offset >= m_db_blocks.size())
    return false;

  for (size_t i = start_offset; i < start_offset + count && i < m_db_blocks.size(); i++)
  {
    blocks.push_back(block_rpc_extended_info());
    block_rpc_extended_info& bei = blocks.back();
    get_main_block_rpc_details(i, bei);
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks)const 
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if(start_offset >= m_db_blocks.size())
    return false;

  for(size_t i = start_offset; i < start_offset + count && i < m_db_blocks.size();i++)
    blocks.push_back(m_db_blocks[i]->bl);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  rsp.current_blockchain_height = get_current_blockchain_size();
  std::list<block> blocks;
  get_blocks(arg.blocks, blocks, rsp.missed_ids);

  BOOST_FOREACH(const auto& bl, blocks)
  {
    std::list<transaction> txs;
    get_transactions(bl.tx_hashes, txs, rsp.missed_ids);

    CHECK_AND_ASSERT_MES(!rsp.missed_ids.size(), false, "Host have requested block with missed transactions missed_tx_id.size()=" << rsp.missed_ids.size()
      << ENDL << "for block id = " << get_block_hash(bl));
   rsp.blocks.push_back(block_complete_entry());
   block_complete_entry& e = rsp.blocks.back();
   //pack block
   e.block = t_serializable_object_to_blob(bl);
   //pack transactions
   BOOST_FOREACH(transaction& tx, txs)
     e.txs.push_back(t_serializable_object_to_blob(tx));

  }
  //get another transactions, if need
  std::list<transaction> txs;
  get_transactions(arg.txs, txs, rsp.missed_ids);
  //pack aside transactions
  BOOST_FOREACH(const auto& tx, txs)
    rsp.txs.push_back(t_serializable_object_to_blob(tx));

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_transactions_daily_stat(uint64_t& daily_cnt, uint64_t& daily_volume) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  daily_cnt = daily_volume = 0;
  for(size_t i =  (m_db_blocks.size() > CURRENCY_BLOCKS_PER_DAY ? m_db_blocks.size() - CURRENCY_BLOCKS_PER_DAY:0 ); i!=m_db_blocks.size(); i++)
  {
    auto ptr = m_db_blocks[i];
    for(auto& h : ptr->bl.tx_hashes)
    {
      ++daily_cnt;
      auto tx_ptr = m_db_transactions.find(h);
      CHECK_AND_ASSERT_MES(tx_ptr, false, "Wrong transaction hash " << h << " in block on height " << i);
      uint64_t am = 0;
      bool r = get_inputs_money_amount(tx_ptr->tx, am);
      CHECK_AND_ASSERT_MES(r, false, "failed to get_inputs_money_amount");
      daily_volume += am;
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::check_keyimages(const std::list<crypto::key_image>& images, std::list<uint64_t>& images_stat) const
{
  //true - unspent, false - spent
  CRITICAL_REGION_LOCAL(m_read_lock);
  for (auto& ki : images)
  {
    auto ki_ptr = m_db_spent_keys.get(ki);
    if(ki_ptr)
      images_stat.push_back(*ki_ptr);
    else
      images_stat.push_back(0);
  }
  return true;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_seconds_between_last_n_block(size_t n) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if (m_db_blocks.size() <= n)
    return 0;

  uint64_t top_block_ts = get_block_datetime(m_db_blocks[m_db_blocks.size() - 1]->bl);
  uint64_t n_block_ts   = get_block_datetime(m_db_blocks[m_db_blocks.size() - 1 - n]->bl);

  return top_block_ts > n_block_ts ? top_block_ts - n_block_ts : 0;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_current_hashrate(size_t aprox_count) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if (aprox_count == 0 || m_db_blocks.size() <= aprox_count)
    return 0; // incorrect parameters

  uint64_t nearest_front_pow_block_i = m_db_blocks.size() - 1;
  while (nearest_front_pow_block_i != 0)
  {
    if (!is_pos_block(m_db_blocks[nearest_front_pow_block_i]->bl))
      break;
    --nearest_front_pow_block_i;
  }

  uint64_t nearest_back_pow_block_i = m_db_blocks.size() - aprox_count;
  while (nearest_back_pow_block_i != 0)
  {
    if (!is_pos_block(m_db_blocks[nearest_back_pow_block_i]->bl))
      break;
    --nearest_back_pow_block_i;
  }

  std::shared_ptr<const block_extended_info> front_blk_ptr = m_db_blocks[nearest_front_pow_block_i];
  std::shared_ptr<const block_extended_info> back_blk_ptr  = m_db_blocks[nearest_back_pow_block_i];
  uint64_t front_blk_ts = front_blk_ptr->bl.timestamp;
  uint64_t back_blk_ts  = back_blk_ptr->bl.timestamp;
  
  uint64_t ts_delta = front_blk_ts > back_blk_ts ? front_blk_ts - back_blk_ts : DIFFICULTY_POW_TARGET;

  wide_difficulty_type w_hr = (front_blk_ptr->cumulative_diff_precise - back_blk_ptr->cumulative_diff_precise) / ts_delta;
  
  return w_hr.convert_to<uint64_t>();
}
//------------------------------------------------------------------
bool blockchain_storage::get_alternative_blocks(std::list<block>& blocks) const 
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);
  BOOST_FOREACH(const auto& alt_bl, m_alternative_chains)
  {
    blocks.push_back(alt_bl.second.bl);
  }
  return true;
}
//------------------------------------------------------------------
size_t blockchain_storage::get_alternative_blocks_count() const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);
  return m_alternative_chains.size();
}
//------------------------------------------------------------------
bool blockchain_storage::add_out_to_get_random_outs(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, uint64_t amount, size_t i, uint64_t mix_count, bool use_only_forced_to_mix) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto out_ptr = m_db_outputs.get_subitem(amount, i);
  auto tx_ptr = m_db_transactions.find(out_ptr->tx_id);
  CHECK_AND_ASSERT_MES(tx_ptr, false, "internal error: transaction with id " << out_ptr->tx_id << ENDL <<
    ", used in mounts global index for amount=" << amount << ": i=" << i << "not found in transactions index");
  CHECK_AND_ASSERT_MES(tx_ptr->tx.vout.size() > out_ptr->out_no, false, "internal error: in global outs index, transaction out index="
    << out_ptr->out_no << " more than transaction outputs = " << tx_ptr->tx.vout.size() << ", for tx id = " << out_ptr->tx_id);
  
  const transaction& tx = tx_ptr->tx;
  if (tx.vout[out_ptr->out_no].target.type() == typeid(txout_htlc))
  {
    //silently return false, it's ok
    return false;
  }
  CHECK_AND_ASSERT_MES(tx.vout[out_ptr->out_no].target.type() == typeid(txout_to_key), false, "unknown tx out type");
  const txout_to_key& otk = boost::get<txout_to_key>(tx.vout[out_ptr->out_no].target);

  CHECK_AND_ASSERT_MES(tx_ptr->m_spent_flags.size() == tx.vout.size(), false, "internal error");

  //do not use outputs that obviously spent for mixins
  if (tx_ptr->m_spent_flags[out_ptr->out_no])
    return false;

  // do not use burned coins
  if (otk.key == null_pkey)
    return false;

  //check if transaction is unlocked
  if (!is_tx_spendtime_unlocked(get_tx_unlock_time(tx, out_ptr->out_no)))
    return false;

  //use appropriate mix_attr out 
  uint8_t mix_attr = otk.mix_attr;
  
  if(mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
    return false; //COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS call means that ring signature will have more than one entry.
  else if(use_only_forced_to_mix && mix_attr == CURRENCY_TO_KEY_OUT_RELAXED)
    return false; //relaxed not allowed
  else if(mix_attr != CURRENCY_TO_KEY_OUT_RELAXED && mix_attr > mix_count)
    return false;//mix_attr set to specific minimum, and mix_count is less then desired count


  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry& oen = *result_outs.outs.insert(result_outs.outs.end(), COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry());
  oen.global_amount_index = i;
  oen.out_key = otk.key;
  return true;
}
//------------------------------------------------------------------
size_t blockchain_storage::find_end_of_allowed_index(uint64_t amount) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  uint64_t sz = m_db_outputs.get_item_size(amount);
  
  if (!sz)
    return 0;
  uint64_t i = sz;
  do
  {
    --i;
    auto out_ptr = m_db_outputs.get_subitem(amount, i);
    auto tx_ptr = m_db_transactions.find(out_ptr->tx_id);
    CHECK_AND_ASSERT_MES(tx_ptr, 0, "internal error: failed to find transaction from outputs index with tx_id=" << out_ptr->tx_id);
    if (tx_ptr->m_keeper_block_height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW <= get_current_blockchain_size())
      return i+1;
  } while (i != 0);
  return 0;
}
//------------------------------------------------------------------
bool blockchain_storage::get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)const
{  
  CRITICAL_REGION_LOCAL(m_read_lock);
  BOOST_FOREACH(uint64_t amount, req.amounts)
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs = *res.outs.insert(res.outs.end(), COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount());
    result_outs.amount = amount;
    uint64_t outs_container_size = m_db_outputs.get_item_size(amount);
    if (!outs_container_size)
    {
      LOG_ERROR("COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS: not outs for amount " << amount << ", wallet should use some real outs when it lookup for some mix, so, at least one out for this amount should exist");
      continue;//actually this is strange situation, wallet should use some real outs when it lookup for some mix, so, at least one out for this amount should exist
    }
    //it is not good idea to use top fresh outs, because it increases possibility of transaction canceling on split
    //lets find upper bound of not fresh outs
    size_t up_index_limit = find_end_of_allowed_index(amount);
    CHECK_AND_ASSERT_MES(up_index_limit <= outs_container_size, false, "internal error: find_end_of_allowed_index returned wrong index=" << up_index_limit << ", with amount_outs.size = " << outs_container_size);
    if (up_index_limit >= req.outs_count)
    {
      std::set<size_t> used;
      size_t try_count = 0;
      for(uint64_t j = 0; j != req.outs_count && try_count < up_index_limit;)
      {
        size_t i = crypto::rand<size_t>()%up_index_limit;
        if(used.count(i))
          continue;
        bool added = add_out_to_get_random_outs(result_outs, amount, i, req.outs_count, req.use_forced_mix_outs);
        used.insert(i);
        if(added)
          ++j;
        ++try_count;
      }
      if (result_outs.outs.size() < req.outs_count)
      {
        LOG_PRINT_RED_L0("Not enough inputs for amount " << print_money_brief(amount) << ", needed " << req.outs_count << ", added " << result_outs.outs.size() << " good outs from " << up_index_limit << " unlocked of " << outs_container_size << " total");
      }
    }else
    {
      size_t added = 0;
      for (size_t i = 0; i != up_index_limit; i++)
        added += add_out_to_get_random_outs(result_outs, amount, i, req.outs_count, req.use_forced_mix_outs) ? 1 : 0;
      LOG_PRINT_RED_L0("Not enough inputs for amount " << print_money_brief(amount) << ", needed " << req.outs_count << ", added " << added << " good outs from " << up_index_limit << " unlocked of " << outs_container_size << " total - respond with all good outs");
    }
  }
  return true;
}
//------------------------------------------------------------------
boost::multiprecision::uint128_t blockchain_storage::total_coins() const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if (!m_db_blocks.size())
    return 0;

  return m_db_blocks.back()->already_generated_coins;
}
//------------------------------------------------------------------
bool blockchain_storage::is_pos_allowed() const
{
  return get_top_block_height() >= m_core_runtime_config.pos_minimum_heigh;
}
//------------------------------------------------------------------
bool blockchain_storage::update_spent_tx_flags_for_input(uint64_t amount, const txout_ref_v& o, bool spent)
{
  if (o.type() == typeid(ref_by_id))
    return update_spent_tx_flags_for_input(boost::get<ref_by_id>(o).tx_id, boost::get<ref_by_id>(o).n, spent);
  else if (o.type() == typeid(uint64_t))
    return update_spent_tx_flags_for_input(amount, boost::get<uint64_t>(o), spent);

  LOG_ERROR("Unknown txout_v type");
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::update_spent_tx_flags_for_input(uint64_t amount, uint64_t global_index, bool spent)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  uint64_t outs_count = m_db_outputs.get_item_size(amount);
  CHECK_AND_ASSERT_MES(outs_count, false, "Amount " << amount << " have not found during update_spent_tx_flags_for_input()");
  CHECK_AND_ASSERT_MES(global_index < outs_count, false, "Global index" << global_index << " for amount " << amount << " bigger value than amount's vector size()=" << outs_count);
  auto out_ptr = m_db_outputs.get_subitem(amount, global_index);
  return update_spent_tx_flags_for_input(out_ptr->tx_id, out_ptr->out_no, spent);
}
//------------------------------------------------------------------
bool blockchain_storage::update_spent_tx_flags_for_input(const crypto::hash& tx_id, size_t n, bool spent)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto tx_ptr = m_db_transactions.find(tx_id);
  CHECK_AND_ASSERT_MES(tx_ptr, false, "Can't find transaction id: " << tx_id);
  transaction_chain_entry tce_local = *tx_ptr;

  CHECK_AND_ASSERT_MES(n < tce_local.m_spent_flags.size(), false, "Wrong input offset: " << n << " in transaction id: " << tx_id);

  tce_local.m_spent_flags[n] = spent;
  m_db_transactions.set(tx_id, tce_local);

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::update_spent_tx_flags_for_input(const crypto::hash& multisig_id, uint64_t spent_height)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto ms_ptr = m_db_multisig_outs.find(multisig_id);
  CHECK_AND_ASSERT_MES(ms_ptr, false, "unable to find multisig_id " << multisig_id);

  // update spent height at ms container
  ms_output_entry msoe_local = *ms_ptr;
  if (msoe_local.spent_height != 0 && spent_height != 0)
  {
    LOG_PRINT_YELLOW(LOCATION_SS << ": WARNING: ms out " << multisig_id << " was already marked as SPENT at height " << msoe_local.spent_height << ", new spent_height: " << spent_height , LOG_LEVEL_0);
  }
  msoe_local.spent_height = spent_height;
  m_db_multisig_outs.set(multisig_id, msoe_local);

  return update_spent_tx_flags_for_input(ms_ptr->tx_id, ms_ptr->out_no, spent_height != 0);
}
//------------------------------------------------------------------
bool blockchain_storage::has_multisig_output(const crypto::hash& multisig_id) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return static_cast<bool>(m_db_multisig_outs.find(multisig_id));
}
//------------------------------------------------------------------
bool blockchain_storage::is_multisig_output_spent(const crypto::hash& multisig_id) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  auto multisig_ptr = m_db_multisig_outs.find(multisig_id);
  if (!multisig_ptr)
    return false; // there's no such output - treat as not spent

  const crypto::hash& source_tx_id = multisig_ptr->tx_id;
  size_t ms_out_index = multisig_ptr->out_no; // index of multisig output in source tx

  auto source_tx_ptr = m_db_transactions.find(source_tx_id);
  CHECK_AND_ASSERT_MES(source_tx_ptr, true, "Internal error: source tx not found for ms out " << multisig_id << ", ms out is treated as spent for safety");
  CHECK_AND_ASSERT_MES(ms_out_index < source_tx_ptr->m_spent_flags.size(), true, "Internal error: ms out " << multisig_id << " has incorrect index " << ms_out_index << " in source tx " << source_tx_id << ", ms out is treated as spent for safety");

  return source_tx_ptr->m_spent_flags[ms_out_index];
}
//------------------------------------------------------------------
bool blockchain_storage::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, uint64_t& starter_offset)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  if(!qblock_ids.size() /*|| !req.m_total_height*/)
  {
    LOG_ERROR("Client sent wrong NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << qblock_ids.size() << /*", m_height=" << req.m_total_height <<*/ ", dropping connection");
    return false;
  } 
  //check genesis match
  if(qblock_ids.back() != get_block_hash(m_db_blocks[0]->bl))
  {
    LOG_ERROR("Client sent wrong NOTIFY_REQUEST_CHAIN: genesis block missmatch: " << ENDL << "id: "
      << string_tools::pod_to_hex(qblock_ids.back()) << ", " << ENDL << "expected: " << get_block_hash(m_db_blocks[0]->bl)
      << "," << ENDL << " dropping connection");
    return false;
  }

  /* Figure out what blocks we should request to get state_normal */
  for(auto& bl: qblock_ids)
  {
    auto block_index_ptr = m_db_blocks_index.find(bl);
    if(block_index_ptr)
    {
      //we start to put block ids INCLUDING last known id, just to make other side be sure
      starter_offset = *block_index_ptr;
      return true;
    }
  }

  LOG_ERROR("Internal error handling connection, can't find split point");
  return false;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::block_difficulty(size_t i) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CHECK_AND_ASSERT_MES(i < m_db_blocks.size(), false, "wrong block index i = " << i << " at blockchain_storage::block_difficulty()");
  return m_db_blocks[i]->difficulty;
}
//------------------------------------------------------------------
bool blockchain_storage::forecast_difficulty(std::vector<std::pair<uint64_t, wide_difficulty_type>> &out_height_2_diff_vector, bool pos) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

   std::vector<uint64_t> timestamps;
   std::vector<wide_difficulty_type> cumulative_difficulties;
 
   uint64_t blocks_size = m_db_blocks.size();
   size_t count = 0;
 
   uint64_t max_block_height_for_this_type = 0;
   uint64_t min_block_height_for_this_type = UINT64_MAX;
   wide_difficulty_type last_block_diff_for_this_type = 0;
 
   for (uint64_t cur_ind = blocks_size - 1; cur_ind != 0 && count < DIFFICULTY_WINDOW; cur_ind--)
   {
     auto beiptr = m_db_blocks[cur_ind];
     bool is_pos_bl = is_pos_block(beiptr->bl);
     if (pos != is_pos_bl)
       continue;
 
     if (max_block_height_for_this_type < beiptr->height)
         max_block_height_for_this_type = beiptr->height;
     if (min_block_height_for_this_type > beiptr->height)
         min_block_height_for_this_type = beiptr->height;
     if (last_block_diff_for_this_type == 0)
       last_block_diff_for_this_type = beiptr->difficulty;
 
     timestamps.push_back(beiptr->bl.timestamp);
     cumulative_difficulties.push_back(beiptr->cumulative_diff_precise);
     ++count;
   }
 
   if (count != DIFFICULTY_WINDOW)
     return false;
 
   const uint64_t target_seconds = pos ? DIFFICULTY_POS_TARGET : DIFFICULTY_POW_TARGET;
   const uint64_t avg_interval = std::max(static_cast<uint64_t>(1), (max_block_height_for_this_type - min_block_height_for_this_type) / count);
   uint64_t height = max_block_height_for_this_type;
   out_height_2_diff_vector.clear();
   out_height_2_diff_vector.push_back(std::make_pair(height, last_block_diff_for_this_type)); // the first element corresponds to the last block of this type
   for (size_t i = 0; i < DIFFICULTY_CUT; ++i)
   {
     wide_difficulty_type diff = next_difficulty_1(timestamps, cumulative_difficulties, target_seconds);
     height += avg_interval;
     out_height_2_diff_vector.push_back(std::make_pair(height, diff));
 
     timestamps.pop_back(); // keep sorted in descending order
     timestamps.insert(timestamps.begin(), timestamps.front() + target_seconds); // increment time so it won't be affected by sorting in next_difficulty
 
     cumulative_difficulties.pop_back();
     cumulative_difficulties.insert(cumulative_difficulties.begin(), 0); // does not matter
   }

  return true;
}
//------------------------------------------------------------------
size_t blockchain_storage::get_current_sequence_factor(bool pos) const
{
  size_t n = 0;
  CRITICAL_REGION_LOCAL(m_read_lock);
  uint64_t sz = m_db_blocks.size();
  if (!sz)
    return n;

  for (uint64_t i = sz - 1; i != 0; --i, n++)
  {
    if (pos != is_pos_block(m_db_blocks[i]->bl))
      break;
  }
  return n;
}
//------------------------------------------------------------------
size_t blockchain_storage::get_current_sequence_factor_for_alt(alt_chain_type& alt_chain, bool pos, uint64_t connection_height) const
{
  size_t n = 0;
  for (auto it = alt_chain.rbegin(); it != alt_chain.rend(); it++, n++)
  {
    if (pos != is_pos_block((*it)->second.bl))
      return n;
  }

  CRITICAL_REGION_LOCAL(m_read_lock);
  for (uint64_t h = connection_height - 1; h != 0; --h, n++)
  {
    if (pos != is_pos_block(m_db_blocks[h]->bl))
    {
      return n;
    }
  }
  return n;
}
//------------------------------------------------------------------
std::string blockchain_storage::get_blockchain_string(uint64_t start_index, uint64_t end_index) const
{ 
  std::stringstream ss;
  CRITICAL_REGION_LOCAL(m_read_lock);
  if (start_index >= m_db_blocks.size())
  {
    LOG_PRINT_L0("Wrong starter index set: " << start_index << ", expected max index " << m_db_blocks.size() - 1);
    return ss.str();
  }

  for (size_t i = start_index; i != m_db_blocks.size() && i != end_index; i++)
  {
    ss << (is_pos_block(m_db_blocks[i]->bl) ? "[PoS]" : "[PoW]") << "h: " << i << ", timestamp: " << m_db_blocks[i]->bl.timestamp << "(" << epee::misc_utils::get_time_str_v2(m_db_blocks[i]->bl.timestamp) << ")"
      << ", cumul_diff_adj: " << m_db_blocks[i]->cumulative_diff_adjusted
      << ", cumul_diff_pcs: " << m_db_blocks[i]->cumulative_diff_precise
      << ", cumul_size: " << m_db_blocks[i]->block_cumulative_size
      << ", id: " << get_block_hash(m_db_blocks[i]->bl)
      << ", difficulty: " << block_difficulty(i) << ", nonce " << m_db_blocks[i]->bl.nonce << ", tx_count " << m_db_blocks[i]->bl.tx_hashes.size() << ENDL;
  }
  return ss.str();
}
//------------------------------------------------------------------
void blockchain_storage::print_blockchain(uint64_t start_index, uint64_t end_index) const
{
  //LOG_ERROR("NOT IMPLEMENTED YET");

  LOG_PRINT_L1("Current blockchain:" << ENDL << get_blockchain_string(start_index, end_index));
  LOG_PRINT_L0("Blockchain printed with log level 1");
}

void blockchain_storage::print_blockchain_with_tx(uint64_t start_index, uint64_t end_index) const
{
  boost::filesystem::ofstream ss;
  ss.exceptions(/*std::ifstream::failbit |*/ std::ifstream::badbit);
  ss.open(epee::string_encoding::utf8_to_wstring(log_space::log_singletone::get_default_log_folder() + "/blockchain_with_tx.txt"),
    std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);


  CRITICAL_REGION_LOCAL(m_read_lock);
  if (start_index >= m_db_blocks.size())
  {
    LOG_PRINT_L0("Wrong starter index set: " << start_index << ", expected max index " << m_db_blocks.size() - 1);
    return;
  }

  for (size_t i = start_index; i != m_db_blocks.size() && i != end_index; i++)
  {
    ss << (is_pos_block(m_db_blocks[i]->bl) ? "[PoS]" : "[PoW]") << "h: " << i << ", timestamp: " << m_db_blocks[i]->bl.timestamp << "(" << epee::misc_utils::get_time_str_v2(m_db_blocks[i]->bl.timestamp) << ")"
      << ", cumul_diff_adj: " << m_db_blocks[i]->cumulative_diff_adjusted
      << ", cumul_diff_pcs: " << m_db_blocks[i]->cumulative_diff_precise
      << ", cumul_size: " << m_db_blocks[i]->block_cumulative_size
      << ", id: " << get_block_hash(m_db_blocks[i]->bl)
      << ", difficulty: " << block_difficulty(i) << ", nonce " << m_db_blocks[i]->bl.nonce << ", tx_count " << m_db_blocks[i]->bl.tx_hashes.size() << ENDL;

    ss << "[miner id]: " << get_transaction_hash(m_db_blocks[i]->bl.miner_tx) << ENDL << currency::obj_to_json_str(m_db_blocks[i]->bl.miner_tx) << ENDL;

    for (size_t j = 0; j != m_db_blocks[i]->bl.tx_hashes.size(); j++)
    {
      auto tx_it = m_db_transactions.find(m_db_blocks[i]->bl.tx_hashes[j]);
      if (tx_it == m_db_transactions.end())
      {
        LOG_ERROR("internal error: tx id " << m_db_blocks[i]->bl.tx_hashes[j] << " not found in transactions index");
        continue;   
      }
      ss << "[id]:" << m_db_blocks[i]->bl.tx_hashes[j] << ENDL << currency::obj_to_json_str(tx_it->tx) << ENDL;
    }

  }
  ss.close();
}
//------------------------------------------------------------------
void blockchain_storage::print_blockchain_index() const
{
  LOG_ERROR("NOT IMPLEMENTED YET");
//   std::stringstream ss;
//   CRITICAL_REGION_LOCAL(m_blockchain_lock);
//   BOOST_FOREACH(const blocks_by_id_index::value_type& v, m_db_blocks_index)
//     ss << "id\t\t" <<  v.first << " height" <<  v.second << ENDL << "";
// 
//   LOG_PRINT_L0("Current blockchain index:" << ENDL << ss.str());
}
//------------------------------------------------------------------
void blockchain_storage::print_db_cache_perfeormance_data() const
{
#define DB_CONTAINER_PERF_DATA_ENTRY(container_name) \
  << #container_name << ": hit_percent: " << m_db_blocks.get_performance_data().hit_percent.get_avg() << "%," \
  << " read_cache: " << container_name.get_performance_data().read_cache_microsec.get_avg() \
    << " read_db: " << container_name.get_performance_data().read_db_microsec.get_avg() \
    << " upd_cache: " << container_name.get_performance_data().update_cache_microsec.get_avg() \
    << " write_cache: " << container_name.get_performance_data().write_to_cache_microsec.get_avg() \
    << " write_db: " << container_name.get_performance_data().write_to_db_microsec.get_avg() \
    << " native_db_set_t: " << container_name.get_performance_data_native().backend_set_t_time.get_avg() \
    << " native_db_set_pod: " << container_name.get_performance_data_native().backend_set_pod_time.get_avg() \
    << " native_db_seriz: " << container_name.get_performance_data_native().set_serialize_t_time.get_avg()


  LOG_PRINT_L0("DB_PERFORMANCE_DATA: " << ENDL 
    DB_CONTAINER_PERF_DATA_ENTRY(m_db_blocks) << ENDL
    DB_CONTAINER_PERF_DATA_ENTRY(m_db_blocks_index) << ENDL
    DB_CONTAINER_PERF_DATA_ENTRY(m_db_transactions) << ENDL
    DB_CONTAINER_PERF_DATA_ENTRY(m_db_spent_keys) << ENDL
    //DB_CONTAINER_PERF_DATA_ENTRY(m_db_outputs) << ENDL
    DB_CONTAINER_PERF_DATA_ENTRY(m_db_multisig_outs) << ENDL
    DB_CONTAINER_PERF_DATA_ENTRY(m_db_solo_options) << ENDL
    DB_CONTAINER_PERF_DATA_ENTRY(m_db_aliases) << ENDL
    DB_CONTAINER_PERF_DATA_ENTRY(m_db_addr_to_alias) << ENDL
    //DB_CONTAINER_PERF_DATA_ENTRY(m_db_per_block_gindex_incs) << ENDL
    //DB_CONTAINER_PERF_DATA_ENTRY(m_tx_fee_median) << ENDL
  );
}
//------------------------------------------------------------------
void blockchain_storage::get_last_n_x_blocks(uint64_t n, bool pos_blocks, std::list<std::shared_ptr<const block_extended_info>>& blocks) const
{
  uint64_t count = 0;
  for (uint64_t i = m_db_blocks.size() - 1; i != 0; --i)
  {
    auto block_ptr = m_db_blocks[i];
    if (is_pos_block(block_ptr->bl) == pos_blocks)
    {
      blocks.push_back(block_ptr);
      ++count;
      if (count >= n)
        break;
    }
  }
}
//------------------------------------------------------------------
void blockchain_storage::print_last_n_difficulty_numbers(uint64_t n) const
{

  std::stringstream ss;
  std::list<std::shared_ptr<const block_extended_info>> pos_blocks;
  std::list<std::shared_ptr<const block_extended_info>> pow_blocks;
  
  get_last_n_x_blocks(n, true, pos_blocks);
  get_last_n_x_blocks(n, false, pow_blocks);
  ss << "PoS blocks difficulty:" << ENDL;
  for (auto& bl_ptr : pos_blocks)
  {
    ss << bl_ptr->difficulty << ENDL;
  }

  ss << "PoW blocks difficulty:" << ENDL;
  for (auto& bl_ptr : pow_blocks)
  {
    ss << bl_ptr->difficulty << ENDL;
  }
  LOG_PRINT_L0("LAST BLOCKS:" << ss.str());
}
//------------------------------------------------------------------
void blockchain_storage::print_blockchain_outs_stats() const
{
  std::stringstream ss;
  CRITICAL_REGION_LOCAL(m_read_lock);

  struct output_stat_t
  {
    uint64_t total = 0;
    uint64_t unspent = 0;
    uint64_t mixable = 0;
  };

  std::map<uint64_t, output_stat_t> outputs_stats;
  
  const uint64_t subitems_cnt = m_db_outputs.size();
  uint64_t progress = 0;

  auto lambda_handler = [&](uint64_t i, uint64_t amount, uint64_t index, const currency::global_output_entry& output_entry) -> bool
  {
    uint64_t progress_current = 20 * i / subitems_cnt;
    if (progress_current != progress)
    {
      progress = progress_current;
      LOG_PRINT_L0(progress * 5 << "%");
    }

    auto p_tx = m_db_transactions.find(output_entry.tx_id);
    if (!p_tx)
    {
      LOG_ERROR("tx " << output_entry.tx_id << " not found");
      return true; // continue
    }
    if (output_entry.out_no >= p_tx->m_spent_flags.size())
    {
      LOG_ERROR("tx with id " << output_entry.tx_id << " has wrong entry in global index, out_no = " << output_entry.out_no
        << ", p_tx->m_spent_flags.size() = " << p_tx->m_spent_flags.size()
        << ", p_tx->tx.vin.size() = " << p_tx->tx.vin.size());
      return true; // continue
    }
    if (p_tx->tx.vout.size() != p_tx->m_spent_flags.size())
    {
      LOG_ERROR("Tx with id " << output_entry.tx_id << " has wrong entry in global index, out_no = " << output_entry.out_no
        << ", p_tx->tx.vout.size() = " << p_tx->tx.vout.size()
        << ", p_tx->m_spent_flags.size() = " << p_tx->m_spent_flags.size());
      return true; // continue
    }

    auto& stat = outputs_stats[amount];
    ++stat.total;
      
    bool spent = p_tx->m_spent_flags[output_entry.out_no];
    if (!spent)
      ++stat.unspent;
      
    if (!spent && p_tx->tx.vout[output_entry.out_no].target.type() == typeid(txout_to_key))
    {
      if (boost::get<txout_to_key>(p_tx->tx.vout[output_entry.out_no].target).mix_attr != CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
        ++stat.mixable;
    }
    return true;
  };

  m_db_outputs.enumerate_subitems(lambda_handler);

  ss << std::right << std::setw(15) << "amount" << std::setw(10) << "total" << std::setw(10) << "unspent" << std::setw(10) << "mixable" << ENDL;
  for(auto it = outputs_stats.begin(); it != outputs_stats.end(); ++it)
    ss << std::setw(15) << print_money_brief(it->first) << std::setw(10) << it->second.total << std::setw(10) << it->second.unspent << std::setw(10) << it->second.mixable << ENDL;

  LOG_PRINT_L0("OUTS: " << ENDL << ss.str());
}
//------------------------------------------------------------------
void blockchain_storage::print_blockchain_outs(const std::string& file) const
{
  LOG_ERROR("NOT IMPLEMENTED YET");
//   std::stringstream ss;
//   CRITICAL_REGION_LOCAL(m_blockchain_lock);
//   BOOST_FOREACH(const outputs_container::value_type& v, m_db_outputs)
//   {
//     const std::vector<std::pair<crypto::hash, size_t> >& vals = v.second;
//     if(vals.size())
//     {
//       ss << "amount: " << print_money(v.first) << ENDL;
//       for (size_t i = 0; i != vals.size(); i++)
//       {
//         bool used = false;
//         auto it_tx = m_db_transactions.find(vals[i].first);
//         if (it_tx == m_db_transactions.end())
//         {
//           LOG_ERROR("Tx with id not found " << vals[i].first);
//         }
//         else
//         {
//           if (vals[i].second >= it_tx->second.m_spent_flags.size())
//           {
//             LOG_ERROR("Tx with id " << vals[i].first << " in global index have wrong entry in global index, offset in tx = " << vals[i].second 
//               << ", it_tx->second.m_spent_flags.size()=" << it_tx->second.m_spent_flags.size() 
//               << ", it_tx->second.tx.vin.size()=" << it_tx->second.tx.vin.size());
//           }
//           used = it_tx->second.m_spent_flags[vals[i].second];
//         }
//         
//         ss << "\t" << vals[i].first << ": " << vals[i].second << ",used:" << used << ENDL;
//       }
//     }
//   }
//   if(file_io_utils::save_string_to_file(file, ss.str()))
//   {
//     LOG_PRINT_L0("Current outputs index writen to file: " << file);
//   }else
//   {
//     LOG_PRINT_L0("Failed to write current outputs index to file: " << file);
//   }
}
//------------------------------------------------------------------

//------------------------------------------------------------------
bool blockchain_storage::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if(!find_blockchain_supplement(qblock_ids, resp.start_height))
    return false;

  resp.total_height = get_current_blockchain_size();
  size_t count = 0;
  
  block_context_info* pprevinfo = nullptr;
  size_t i = 0;
  for (i = resp.start_height; i != m_db_blocks.size() && count < BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT; i++, count++)
  {
    resp.m_block_ids.push_back(block_context_info());
    
    if (pprevinfo)
      pprevinfo->h = m_db_blocks[i]->bl.prev_id;
    resp.m_block_ids.back().cumul_size = m_db_blocks[i]->block_cumulative_size;
    pprevinfo = &resp.m_block_ids.back();
  }    
  if (pprevinfo)
    pprevinfo->h = get_block_hash(m_db_blocks[--i]->bl);

  return true;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_blockchain_launch_timestamp()const
{
  if (m_blockchain_launch_timestamp)
    return m_blockchain_launch_timestamp;

  if (m_db_blocks.size() > 2)
  {
    m_blockchain_launch_timestamp = m_db_blocks[1]->bl.timestamp;
  }
  return m_blockchain_launch_timestamp;
}
//------------------------------------------------------------------
bool blockchain_storage::get_est_height_from_date(uint64_t date, uint64_t& res_h)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
#define GET_EST_HEIGHT_FROM_DATE_THRESHOLD              1440

  res_h = 0;

  if (date < get_blockchain_launch_timestamp())
  {
    return true;
  }


  uint64_t calculated_estimated_height = (date - get_blockchain_launch_timestamp()) / DIFFICULTY_TOTAL_TARGET;
  
  if (date > m_db_blocks[m_db_blocks.size() - 1]->bl.timestamp)
  {
    //that suspicious but also could be(in case someone just created wallet offline in
    //console and then got it synchronyzing and last block had a little timestamp shift)
    //let's just return 1 day behind for safety reasons. 
    if (m_db_blocks.size() > 1440)
    {
      res_h = m_db_blocks.size() - 1440;
    }
    else 
    {
      //likely impossible, but just in case
      res_h = 0;
    }
    return true;
  }
  if (calculated_estimated_height > m_db_blocks.size() - 1)
    calculated_estimated_height = m_db_blocks.size() - 1;

  //goal is to get timestamp in window in between 1day+1hour  and 1 hour before target(1 hour is just to be sure that
  //we didn't miss actual wallet start because of timestamp and difficulty fluctuations)
  uint64_t low_boundary = date - 90000; //1 day + 1 hour
  uint64_t high_boundary = date - 3600; //1 hour

  //std::cout << "ENTRY: low_boundary(minutes):" << low_boundary/60 << " high_boundary(minutes): " << high_boundary / 60 << std::endl;

  uint64_t iteration_coun = 0;
  uint64_t current_low_boundary = 0;
  uint64_t current_height_boundary = m_db_blocks.size() - 1;
  while (true)
  {
    iteration_coun++;
    if (iteration_coun > 29) // Log2(CURRENCY_MAX_BLOCK_NUMBER) 
    {
      LOG_ERROR("Internal error: too much iterations on get_est_height_from_date, date = " << date);
      return true;
    }
    uint64_t ts = m_db_blocks[calculated_estimated_height]->bl.timestamp;
    if (ts > high_boundary)
    {
      //we moved too much forward
      
      current_height_boundary = calculated_estimated_height;
      CHECK_AND_ASSERT_MES(current_height_boundary > current_low_boundary, true, 
        "Internal error: current_hight_boundary(" << current_height_boundary << ") > current_low_boundary("<< current_low_boundary << ")");
      uint64_t offset = (current_height_boundary - current_low_boundary)/2;
      if (offset <= 2)
      {
        //something really wrong with distribution of blocks, just use current_low_boundary to be sure that we didn't mess any transactions 
        res_h = current_low_boundary;
        return true;
      }

      //std::cout << "est_h:" << calculated_estimated_height << ", ts(min): " << ts / 60 << " distance to RIGHT minutes: " << int64_t((int64_t(ts) - int64_t(high_boundary))) / 60 << std::endl;
      //std::cout << "OOFFSET: -" << offset << std::endl;
      calculated_estimated_height -= offset;
    }
    else if (ts < low_boundary)
    {
      //we too much in past
      current_low_boundary = calculated_estimated_height;
      CHECK_AND_ASSERT_MES(current_height_boundary > current_low_boundary, true,
        "Internal error: current_hight_boundary(" << current_height_boundary << ") > current_low_boundary(" << current_low_boundary << ")");
      uint64_t offset = (current_height_boundary - current_low_boundary) / 2;
      if (offset <= 2)
      {
        //something really wrong with distribution of blocks, just use current_low_boundary to be sure that we didn't mess any transactions 
        res_h = current_low_boundary;
        return true;
      }
      //CHECK_AND_ASSERT_MES(offset > 2, true,
      //  "offset is too low = " << offset);

      //std::cout << "est_h:" << calculated_estimated_height << ", ts(min): " << ts / 60 << " distance to LEFT minutes: " << int64_t((int64_t(low_boundary) - int64_t(ts))) / 60 << std::endl;
      //std::cout << "OOFFSET: +" << offset << std::endl;
      calculated_estimated_height += offset;
    }
    else
    {
      res_h = calculated_estimated_height;
      break;
    }
  }

  LOG_PRINT_L0("[get_est_height_from_date] returned " << calculated_estimated_height << " with " << iteration_coun << " iterations");
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count, uint64_t minimum_height, bool need_global_indexes)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  blocks_direct_container blocks_direct;
  if (!find_blockchain_supplement(qblock_ids, blocks_direct, total_height, start_height, max_count, minimum_height))
    return false;
  
  for (auto& bd : blocks_direct)
  {
    blocks.push_back(std::pair<block, std::list<transaction> >());
    blocks.back().first = bd.first->bl;
    for (auto& tx_ptr : bd.second)
    {
      blocks.back().second.push_back(tx_ptr->tx);
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, blocks_direct_container& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count, uint64_t minimum_height, bool request_coinbase_info)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if (!find_blockchain_supplement(qblock_ids, start_height))
    return false;
  if (minimum_height > start_height)
    start_height = minimum_height;

  total_height = get_current_blockchain_size();
  size_t count = 0;
  for (size_t i = start_height; i != m_db_blocks.size() && count < max_count; i++, count++)
  {
    blocks.resize(blocks.size() + 1);
    blocks.back().first = m_db_blocks[i];
    std::list<crypto::hash> mis;
    get_transactions_direct(m_db_blocks[i]->bl.tx_hashes, blocks.back().second, mis);
    CHECK_AND_ASSERT_MES(!mis.size(), false, "internal error, block " << get_block_hash(m_db_blocks[i]->bl) << " [" << i << "] contains missing transactions: " << mis);
    if(request_coinbase_info)
      blocks.back().third = m_db_transactions.find(get_transaction_hash(m_db_blocks[i]->bl.miner_tx));
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::add_block_as_invalid(const block& bl, const crypto::hash& h)
{
  block_extended_info bei = AUTO_VAL_INIT(bei);
  bei.bl = bl;
  return add_block_as_invalid(bei, h);
}
//------------------------------------------------------------------
bool blockchain_storage::add_block_as_invalid(const block_extended_info& bei, const crypto::hash& h)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CRITICAL_REGION_LOCAL1(m_invalid_blocks_lock);
  auto i_res = m_invalid_blocks.insert(std::map<crypto::hash, block_extended_info>::value_type(h, bei));
  CHECK_AND_ASSERT_MES(i_res.second, false, "at insertion invalid by tx returned status existed");
  LOG_PRINT_L0("BLOCK ADDED AS INVALID: " << h << ENDL << ", prev_id=" << bei.bl.prev_id << ", m_invalid_blocks count=" << m_invalid_blocks.size());
  CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL!");
  return true;
}
//------------------------------------------------------------------
void blockchain_storage::inspect_blocks_index()const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  LOG_PRINT_L0("Started block index inspecting....");
  m_db_blocks_index.enumerate_items([&](uint64_t count, const crypto::hash& id, uint64_t index)
  {
    CHECK_AND_ASSERT_MES(index < m_db_blocks.size(), true, "invalid index " << index << "(m_db_blocks.size()=" << m_db_blocks.size() << ") for id " << id << " ");
    crypto::hash calculated_id = get_block_hash(m_db_blocks[index]->bl);
    CHECK_AND_ASSERT_MES(id == calculated_id, true, "ID MISSMATCH ON INDEX " << index << ENDL
      << "m_db_blocks_index keeps: " << id << ENDL
      << "referenced to block with id " << calculated_id
    );
    return true;
  });
  LOG_PRINT_L0("Block index inspecting finished");
}
//------------------------------------------------------------------
bool blockchain_storage::have_block(const crypto::hash& id)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  if(m_db_blocks_index.find(id))
    return true;
  {
    CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);
    if (m_alternative_chains.count(id))
      return true;

  }
  /*if(m_orphaned_blocks.get<by_id>().count(id))
    return true;*/

  /*if(m_orphaned_by_tx.count(id))
    return true;*/
  {
    CRITICAL_REGION_LOCAL1(m_invalid_blocks_lock);
    if (m_invalid_blocks.count(id))
      return true;
  }

  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::handle_block_to_main_chain(const block& bl, block_verification_context& bvc)
{
  crypto::hash id = get_block_hash(bl);
  return handle_block_to_main_chain(bl, id, bvc);
}
//------------------------------------------------------------------
bool blockchain_storage::push_transaction_to_global_outs_index(const transaction& tx, const crypto::hash& tx_id, std::vector<uint64_t>& global_indexes)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  size_t i = 0;
  BOOST_FOREACH(const auto& ot, tx.vout)
  {
    if (ot.target.type() == typeid(txout_to_key) || ot.target.type() == typeid(txout_htlc))
    {
      m_db_outputs.push_back_item(ot.amount, global_output_entry::construct(tx_id, i));
      global_indexes.push_back(m_db_outputs.get_item_size(ot.amount) - 1);
      if (ot.target.type() == typeid(txout_htlc) && !is_after_hardfork_3_zone())
      {
        LOG_ERROR("Error: Transaction with txout_htlc before is_after_hardfork_3_zone(before height " << m_core_runtime_config.hard_fork_03_starts_after_height <<")");
        return false;
      }
    }
    else if (ot.target.type() == typeid(txout_multisig))
    {

      crypto::hash multisig_out_id = get_multisig_out_id(tx, i);
      CHECK_AND_ASSERT_MES(multisig_out_id != null_hash, false, "internal error during handling get_multisig_out_id() with tx id " << tx_id);
      CHECK_AND_ASSERT_MES(!m_db_multisig_outs.find(multisig_out_id), false, "Internal error: already have multisig_out_id " << multisig_out_id << "in multisig outs index");
      m_db_multisig_outs.set(multisig_out_id, ms_output_entry::construct(tx_id, i));
      global_indexes.push_back(0); // just stub to make other code easier
    }

    ++i;
  }
  return true;
}
//------------------------------------------------------------------
size_t blockchain_storage::get_total_transactions()const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return m_db_transactions.size();
}
//------------------------------------------------------------------
bool blockchain_storage::get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  uint64_t sz = m_db_outputs.get_item_size(amount);

  if (!sz)
    return true;

  for (uint64_t i = 0; i != sz; i++)
  {
    auto out_entry_ptr = m_db_outputs.get_subitem(amount, i);

    auto tx_ptr = m_db_transactions.find(out_entry_ptr->tx_id);
    CHECK_AND_ASSERT_MES(tx_ptr, false, "transactions outs global index consistency broken: can't find tx " << out_entry_ptr->tx_id << " in DB, for amount: " << amount << ", gindex: " << i);
    CHECK_AND_ASSERT_MES(tx_ptr->tx.vout.size() > out_entry_ptr->out_no, false, "transactions outs global index consistency broken: index in tx_outx == " << out_entry_ptr->out_no << " is greather than tx.vout size == " << tx_ptr->tx.vout.size() << ", for amount: " << amount << ", gindex: " << i);
    //CHECK_AND_ASSERT_MES(tx_ptr->tx.vout[out_entry_ptr->out_no].target.type() == typeid(txout_to_key), false, "transactions outs global index consistency broken: out #" << out_entry_ptr->out_no << " in tx " << out_entry_ptr->tx_id << " has wrong type, for amount: " << amount << ", gindex: " << i);
    if (tx_ptr->tx.vout[out_entry_ptr->out_no].target.type() == typeid(txout_to_key))
    {
      pkeys.push_back(boost::get<txout_to_key>(tx_ptr->tx.vout[out_entry_ptr->out_no].target).key);
    }else if(tx_ptr->tx.vout[out_entry_ptr->out_no].target.type() == typeid(txout_htlc))
    {
      pkeys.push_back(boost::get<txout_htlc>(tx_ptr->tx.vout[out_entry_ptr->out_no].target).pkey_redeem);
    }
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::pop_transaction_from_global_index(const transaction& tx, const crypto::hash& tx_id)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  size_t i = tx.vout.size()-1;
  BOOST_REVERSE_FOREACH(const auto& ot, tx.vout)
  {
    if (ot.target.type() == typeid(txout_to_key) || ot.target.type() == typeid(txout_htlc))
    {
      uint64_t sz= m_db_outputs.get_item_size(ot.amount);
      CHECK_AND_ASSERT_MES(sz, false, "transactions outs global index: empty index for amount: " << ot.amount);
      auto back_item = m_db_outputs.get_subitem(ot.amount, sz - 1);
      CHECK_AND_ASSERT_MES(back_item->tx_id == tx_id, false, "transactions outs global index consistency broken: tx id missmatch");
      CHECK_AND_ASSERT_MES(back_item->out_no == i, false, "transactions outs global index consistency broken: in transaction index missmatch");
      m_db_outputs.pop_back_item(ot.amount);
      //if (!it->second.size())
      //  m_db_outputs.erase(it);
    }
    else if (ot.target.type() == typeid(txout_multisig))
    {
      crypto::hash multisig_out_id = get_multisig_out_id(tx, i);
      CHECK_AND_ASSERT_MES(multisig_out_id != null_hash, false, "internal error during handling get_multisig_out_id() with tx id " << tx_id);
      bool res = m_db_multisig_outs.erase_validate(multisig_out_id);
      CHECK_AND_ASSERT_MES(res, false, "Internal error: multisig out not found, multisig_out_id " << multisig_out_id << "in multisig outs index");
    }
    --i;
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::unprocess_blockchain_tx_extra(const transaction& tx)
{
  tx_extra_info ei = AUTO_VAL_INIT(ei);
  bool r = parse_and_validate_tx_extra(tx, ei);
  CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra on unprocess_blockchain_tx_extra");
  if(ei.m_alias.m_alias.size())
  {
    r = pop_alias_info(ei.m_alias);
    CHECK_AND_ASSERT_MES(r, false, "failed to pop_alias_info");
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_alias_info(const std::string& alias, extra_alias_entry_base& info)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto al_ptr = m_db_aliases.find(alias);
  if (al_ptr)
  {
    if (al_ptr->size())
    {
      info = al_ptr->back();
      return true;
    }
  }
  return false;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_aliases_count() const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return m_db_aliases.size();
}
//------------------------------------------------------------------
std::string blockchain_storage::get_alias_by_address(const account_public_address& addr)const
{
  auto alias_ptr = m_db_addr_to_alias.find(addr);
  if (alias_ptr && alias_ptr->size())
  {
    return *(alias_ptr->begin());
  }
  
  return "";
}
//------------------------------------------------------------------
bool blockchain_storage::pop_alias_info(const extra_alias_entry& ai)
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  CHECK_AND_ASSERT_MES(ai.m_alias.size(), false, "empty name in pop_alias_info");
  auto alias_history_ptr = m_db_aliases.find(ai.m_alias);
  CHECK_AND_ASSERT_MES(alias_history_ptr && alias_history_ptr->size(), false, "empty name list in pop_alias_info");
  
  auto addr_to_alias_ptr = m_db_addr_to_alias.find(alias_history_ptr->back().m_address);
  if (addr_to_alias_ptr)
  {
    //update db
    address_to_aliases_container::t_value_type local_v = *addr_to_alias_ptr;

    auto it_in_set = local_v.find(ai.m_alias);
    CHECK_AND_ASSERT_MES(it_in_set != local_v.end(), false, "it_in_set != it->second.end() validation failed");

    local_v.erase(it_in_set);
    if (!local_v.size())
    {
      //delete the whole record from db
      m_db_addr_to_alias.erase(alias_history_ptr->back().m_address);
    }
    else
    {
      //update db
      m_db_addr_to_alias.set(alias_history_ptr->back().m_address, local_v);
    }
  }
  else
  {
    LOG_ERROR("In m_addr_to_alias not found " << get_account_address_as_str(alias_history_ptr->back().m_address));
  }

  aliases_container::t_value_type local_alias_hist = *alias_history_ptr;
  local_alias_hist.pop_back();
  if(local_alias_hist.size())
    m_db_aliases.set(ai.m_alias, local_alias_hist);
  else 
    m_db_aliases.erase(ai.m_alias);

  if (local_alias_hist.size())
  {
    address_to_aliases_container::t_value_type local_copy = AUTO_VAL_INIT(local_copy);
    auto set_ptr = m_db_addr_to_alias.get(local_alias_hist.back().m_address);
    if (set_ptr)
      local_copy = *set_ptr;

    local_copy.insert(ai.m_alias);
    m_db_addr_to_alias.set(local_alias_hist.back().m_address, local_copy);
  }

  LOG_PRINT_MAGENTA("[ALIAS_UNREGISTERED]: " << ai.m_alias << ": " << get_account_address_as_str(ai.m_address) << " -> " << (!local_alias_hist.empty() ? get_account_address_as_str(local_alias_hist.back().m_address) : "(available)"), LOG_LEVEL_1);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::put_alias_info(const transaction & tx, extra_alias_entry & ai)
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  CHECK_AND_ASSERT_MES(ai.m_alias.size(), false, "empty name in put_alias_info");
  aliases_container::t_value_type local_alias_history = AUTO_VAL_INIT(local_alias_history);
  auto alias_history_ptr_ = m_db_aliases.get(ai.m_alias);
  if (alias_history_ptr_)
    local_alias_history = *alias_history_ptr_;
  
  if (!local_alias_history.size())
  {

    //update alias entry in db
    local_alias_history.push_back(ai);
    m_db_aliases.set(ai.m_alias, local_alias_history);

    //update addr-to-alias db entry
    address_to_aliases_container::t_value_type addr_to_alias_local = AUTO_VAL_INIT(addr_to_alias_local);
    auto addr_to_alias_ptr_ = m_db_addr_to_alias.get(local_alias_history.back().m_address);
    if (addr_to_alias_ptr_)
      addr_to_alias_local = *addr_to_alias_ptr_;

    addr_to_alias_local.insert(ai.m_alias);
    m_db_addr_to_alias.set(local_alias_history.back().m_address, addr_to_alias_local);

    //@@ remove get_tx_fee_median();
    LOG_PRINT_MAGENTA("[ALIAS_REGISTERED]: " << ai.m_alias << ": " << get_account_address_as_str(ai.m_address) << ", fee median: " << get_tx_fee_median(), LOG_LEVEL_1);
    rise_core_event(CORE_EVENT_ADD_ALIAS, alias_info_to_rpc_alias_info(ai));
  }else
  {
    //update procedure
    CHECK_AND_ASSERT_MES(ai.m_sign.size() == 1, false, "alias " << ai.m_alias << " can't be update, wrong ai.m_sign.size() count: " << ai.m_sign.size());
    //std::string signed_buff;
    //make_tx_extra_alias_entry(signed_buff, ai, true);
    std::string old_address = currency::get_account_address_as_str(local_alias_history.back().m_address);
    bool r = crypto::check_signature(get_sign_buff_hash_for_alias_update(ai), local_alias_history.back().m_address.spend_public_key, ai.m_sign.back());
    CHECK_AND_ASSERT_MES(r, false, "Failed to check signature, alias update failed." << ENDL 
      << "alias: " << ai.m_alias << ENDL
      << "signed_buff_hash: " << get_sign_buff_hash_for_alias_update(ai) << ENDL
      << "public key: " << local_alias_history.back().m_address.spend_public_key << ENDL
      << "new_address: " << get_account_address_as_str(ai.m_address) << ENDL
      << "signature: " << epee::string_tools::pod_to_hex(ai.m_sign) << ENDL 
      << "alias_history.size() = " << local_alias_history.size());

    //update adr-to-alias db
    auto addr_to_alias_ptr_ = m_db_addr_to_alias.find(local_alias_history.back().m_address);
    if (addr_to_alias_ptr_)
    {
      address_to_aliases_container::t_value_type addr_to_alias_local = *addr_to_alias_ptr_;
      auto it_in_set = addr_to_alias_local.find(ai.m_alias);
      if (it_in_set == addr_to_alias_local.end())
      {
        LOG_ERROR("it_in_set == it->second.end()");
      }
      else
      {
        addr_to_alias_local.erase(it_in_set);
      }

      if (!addr_to_alias_local.size())
        m_db_addr_to_alias.erase(local_alias_history.back().m_address);
      else
        m_db_addr_to_alias.set(local_alias_history.back().m_address, addr_to_alias_local);
    }
    else
    {
      LOG_ERROR("Wrong m_addr_to_alias state: address not found " << get_account_address_as_str(local_alias_history.back().m_address));

      std::stringstream ss;
      ss << "History for alias " << ai.m_alias << ":" << ENDL;
      size_t i = 0;
      for (auto el : local_alias_history)
      {
        ss << std::setw(2) << i++ << " "
          << get_account_address_as_str(el.m_address) << " "
          << (el.m_sign.empty() ? " no sig " : " SIGNED ") << " "
          << el.m_text_comment << ENDL;
      }

      LOG_PRINT_L0(ss.str());

    }

    //update alias db
    local_alias_history.push_back(ai);
    m_db_aliases.set(ai.m_alias, local_alias_history);

    //update addr_to_alias db
    address_to_aliases_container::t_value_type addr_to_alias_local2 = AUTO_VAL_INIT(addr_to_alias_local2);
    auto addr_to_alias_ptr_2 = m_db_addr_to_alias.get(local_alias_history.back().m_address);
    if (addr_to_alias_ptr_2)
      addr_to_alias_local2 = *addr_to_alias_ptr_2;
    
    addr_to_alias_local2.insert(ai.m_alias);
    m_db_addr_to_alias.set(local_alias_history.back().m_address, addr_to_alias_local2);

    LOG_PRINT_MAGENTA("[ALIAS_UPDATED]: " << ai.m_alias << ": from: " << old_address << " to " << get_account_address_as_str(ai.m_address), LOG_LEVEL_1);
    rise_core_event(CORE_EVENT_UPDATE_ALIAS, alias_info_to_rpc_update_alias_info(ai, old_address));

  }
  return true;
}
//------------------------------------------------------------------
void blockchain_storage::set_event_handler(i_core_event_handler* event_handler) const 
{
  if (event_handler == nullptr)
  {
    m_event_handler = &m_event_handler_stub;
    m_services_mgr.set_event_handler(nullptr);
  }
  else
  {
    m_event_handler = event_handler;
    m_services_mgr.set_event_handler(event_handler);
  }
    
}
//------------------------------------------------------------------
i_core_event_handler* blockchain_storage::get_event_handler() const
{
  return m_event_handler;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::validate_alias_reward(const transaction& tx, const std::string& alias) const
{

  //validate alias coast
  uint64_t fee_for_alias = get_alias_coast(alias);
  
  //validate the price had been paid
  uint64_t found_alias_reward = get_amount_for_zero_pubkeys(tx);

  //@#@ 
  //work around for net 68's generation
#if CURRENCY_FORMATION_VERSION == 68
  if (alias == "bhrfrrrtret" && get_transaction_hash(tx) == epee::string_tools::parse_tpod_from_hex_string<crypto::hash>("760b85546678d2235a1843e18d8a016a2e4d9b8273cc4d7c09bebff1f6fa7eaf")  )
    return true; 
  if (alias == "test-420" &&    get_transaction_hash(tx) == epee::string_tools::parse_tpod_from_hex_string<crypto::hash>("10f8a2539b2551bd0919bf7e3b1dfbae7553eca63e58cd2264ae60f90030edf8"))
    return true;
#endif

  CHECK_AND_ASSERT_MES(found_alias_reward >= fee_for_alias, false, "registration of alias '" 
    << alias << "' goes with a reward of " << print_money(found_alias_reward) << " which is less than expected: " << print_money(fee_for_alias) 
    <<"(fee median: " << get_tx_fee_median() << ")"
    << ", tx: " << get_transaction_hash(tx));

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::prevalidate_alias_info(const transaction& tx, const extra_alias_entry& eae)
{
  
  bool r = validate_alias_name(eae.m_alias);
  CHECK_AND_ASSERT_MES(r, false, "failed to validate alias name!");
  bool already_have_alias = false;
  {
    CRITICAL_REGION_LOCAL(m_read_lock);
    auto existing_ptr = m_db_aliases.find(eae.m_alias);
    if (existing_ptr && existing_ptr->size())
    {
      already_have_alias = true;
    }
      
  }
  //auto alias_history_ptr_ = m_db_aliases.get(eae.m_alias);
  if (!already_have_alias)
  {
    bool r = validate_alias_reward(tx, eae.m_alias);
    CHECK_AND_ASSERT_MES(r, false, "failed to validate_alias_reward");

    if (eae.m_alias.size() < ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED)
    {
      //short alias name, this aliases should be issued only by specific authority
      CHECK_AND_ASSERT_MES(eae.m_sign.size() == 1, false, "alias " << eae.m_alias << " can't be update, wrong ai.m_sign.size() count: " << eae.m_sign.size());

      bool r = crypto::check_signature(get_sign_buff_hash_for_alias_update(eae), m_core_runtime_config.alias_validation_pubkey, eae.m_sign.back());
      CHECK_AND_ASSERT_MES(r, false, "Failed to check signature, short alias registration failed." << ENDL
        << "alias: " << eae.m_alias << ENDL
        << "signed_buff_hash: " << get_sign_buff_hash_for_alias_update(eae) << ENDL
        << "public key: " << m_core_runtime_config.alias_validation_pubkey << ENDL
        << "new_address: " << get_account_address_as_str(eae.m_address) << ENDL
        << "signature: " << epee::string_tools::pod_to_hex(eae.m_sign));
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::process_blockchain_tx_extra(const transaction& tx)
{
  //check transaction extra
  tx_extra_info ei = AUTO_VAL_INIT(ei);
  bool r = parse_and_validate_tx_extra(tx, ei);
  CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra");
  if (ei.m_alias.m_alias.size())
  {
    r = prevalidate_alias_info(tx, ei.m_alias);
    CHECK_AND_ASSERT_MES(r, false, "failed to prevalidate_alias_info");

    r = put_alias_info(tx, ei.m_alias);
    CHECK_AND_ASSERT_MES(r, false, "failed to put_alias_info");
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_outs_index_stat(outs_index_stat& outs_stat) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  outs_stat.amount_0_001 = m_db_outputs.get_item_size(COIN / 1000);
  outs_stat.amount_0_01 = m_db_outputs.get_item_size(COIN / 100);
  outs_stat.amount_0_1 = m_db_outputs.get_item_size(COIN / 10);
  outs_stat.amount_1 = m_db_outputs.get_item_size(COIN);
  outs_stat.amount_10 = m_db_outputs.get_item_size(COIN * 10);
  outs_stat.amount_100 = m_db_outputs.get_item_size(COIN * 100);
  outs_stat.amount_1000 = m_db_outputs.get_item_size(COIN * 1000);
  outs_stat.amount_10000 = m_db_outputs.get_item_size(COIN * 10000);
  outs_stat.amount_100000 = m_db_outputs.get_item_size(COIN * 100000);
  outs_stat.amount_1000000 = m_db_outputs.get_item_size(COIN * 1000000);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::process_blockchain_tx_attachments(const transaction& tx, uint64_t h, const crypto::hash& bl_id, uint64_t timestamp)
{
  //check transaction extra
  uint64_t count = 0;

  for (const auto& at : tx.attachment)
  {
    if (at.type() == typeid(tx_service_attachment))
    {
      m_services_mgr.handle_entry_push(boost::get<tx_service_attachment>(at), count, tx, h, bl_id, timestamp); //handle service
      ++count;
    }
  }
  return true;
}

//------------------------------------------------------------------
uint64_t blockchain_storage::get_tx_fee_median() const
{ 
  uint64_t h = m_db_blocks.size();
  if (m_current_fee_median_effective_index != get_tx_fee_median_effective_index(h))
  {
    m_current_fee_median = tx_fee_median_for_height(h);
    m_current_fee_median_effective_index = get_tx_fee_median_effective_index(h);
  }

  if (!m_current_fee_median)
    m_current_fee_median = ALIAS_VERY_INITAL_COAST;
  return m_current_fee_median;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_alias_coast(const std::string& alias) const
{
  uint64_t median_fee = get_tx_fee_median();
  //CHECK_AND_ASSERT_MES_NO_RET(median_fee, "can't calculate median");

  if (!median_fee)
    median_fee = ALIAS_VERY_INITAL_COAST;


  return get_alias_coast_from_fee(alias, median_fee);
}
//------------------------------------------------------------------
bool blockchain_storage::unprocess_blockchain_tx_attachments(const transaction& tx, uint64_t h, uint64_t timestamp)
{

  size_t cnt_serv_attach = get_service_attachments_count_in_tx(tx);
  if (cnt_serv_attach == 0)
    return true;

  --cnt_serv_attach;
  for (auto it = tx.attachment.rbegin(); it != tx.attachment.rend(); it++)
  {
    auto& at = *it;
    if (at.type() == typeid(tx_service_attachment))
    {
      m_services_mgr.handle_entry_pop(boost::get<tx_service_attachment>(at), cnt_serv_attach, tx, h, timestamp);
      --cnt_serv_attach;
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_tx_service_attachmens_in_services(const tx_service_attachment& a, size_t i, const transaction& tx) const
{
  return m_services_mgr.validate_entry(a, i, tx);
}
//------------------------------------------------------------------
namespace currency
{
  struct add_transaction_input_visitor : public boost::static_visitor<bool>
  {
    blockchain_storage& m_bcs;
    blockchain_storage::key_images_container& m_db_spent_keys;
    const crypto::hash& m_tx_id;
    const crypto::hash& m_bl_id;
    const uint64_t m_bl_height;
    uint64_t &m_mixins_count;
    add_transaction_input_visitor(blockchain_storage& bcs, blockchain_storage::key_images_container& m_db_spent_keys, const crypto::hash& tx_id, const crypto::hash& bl_id, const uint64_t bl_height, uint64_t& mixins_count) :
      m_bcs(bcs),
      m_db_spent_keys(m_db_spent_keys),
      m_tx_id(tx_id),
      m_bl_id(bl_id),
      m_bl_height(bl_height), 
      m_mixins_count(mixins_count)
    {}
    bool operator()(const txin_to_key& in) const
    {
      const crypto::key_image& ki = in.k_image;

      auto ki_ptr = m_db_spent_keys.get(ki);
      if (ki_ptr)
      {
        //double spend detected
        LOG_PRINT_RED_L0("tx with id: " << m_tx_id << " in block id: " << m_bl_id << " have input marked as spent with key image: " << ki << ", block declined");
        return false;
      }
      m_db_spent_keys.set(ki, m_bl_height);

      if (in.key_offsets.size() == 1)
      {
        //direct spend detected
        if (!m_bcs.update_spent_tx_flags_for_input(in.amount, in.key_offsets[0], true))
        {
          //internal error
          LOG_PRINT_RED_L0("Failed to  update_spent_tx_flags_for_input");
          return false;
        }
      }
      if (m_mixins_count < in.key_offsets.size())
        m_mixins_count = in.key_offsets.size();
      return true;
    }
    bool operator()(const txin_htlc& in) const
    {
      if (!m_bcs.is_after_hardfork_3_zone())
      {
        LOG_ERROR("Error: Transaction with txin_htlc before is_after_hardfork_3_zone(before height " << m_bcs.get_core_runtime_config().hard_fork_03_starts_after_height << ")");
        return false;
      }
      return this->operator()(static_cast<const txin_to_key&>(in));
    }
    bool operator()(const txin_gen& in) const { return true; }
    bool operator()(const txin_multisig& in) const
    {
      //mark out as spent
      if (!m_bcs.update_spent_tx_flags_for_input(in.multisig_out_id, m_bl_height))
      {
        //internal error
        LOG_PRINT_RED_L0("Failed to  update_spent_tx_flags_for_input");
        return false;
      }
      return true;
    }
  };
}


bool blockchain_storage::add_transaction_from_block(const transaction& tx, const crypto::hash& tx_id, const crypto::hash& bl_id, uint64_t bl_height, uint64_t timestamp)
{
  bool need_to_profile = !is_coinbase(tx);
  TIME_MEASURE_START_PD(tx_append_rl_wait);
  CRITICAL_REGION_LOCAL(m_read_lock);
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_append_rl_wait);
  
  TIME_MEASURE_START_PD(tx_append_is_expired);
  CHECK_AND_ASSERT_MES(!is_tx_expired(tx), false, "Transaction can't be added to the blockchain since it's already expired. tx expiration time: " << get_tx_expiration_time(tx) << ", blockchain median time: " << get_tx_expiration_median());
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_append_is_expired);

  CHECK_AND_ASSERT_MES(validate_tx_for_hardfork_specific_terms(tx, tx_id, bl_height), false, "tx " << tx_id << ": hardfork-specific validation failed");
  
  TIME_MEASURE_START_PD(tx_process_extra);
  bool r = process_blockchain_tx_extra(tx);
  CHECK_AND_ASSERT_MES(r, false, "failed to process_blockchain_tx_extra");
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_process_extra);

  TIME_MEASURE_START_PD(tx_process_attachment);
  process_blockchain_tx_attachments(tx, bl_height, bl_id, timestamp);
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_process_attachment);

  uint64_t mixins_count = 0;
  TIME_MEASURE_START_PD(tx_process_inputs);
  for(const txin_v& in : tx.vin)
  {
    if(!boost::apply_visitor(add_transaction_input_visitor(*this, m_db_spent_keys, tx_id, bl_id, bl_height, mixins_count), in))
    {
      LOG_ERROR("critical internal error: add_transaction_input_visitor failed. but key_images should be already checked");
      purge_transaction_keyimages_from_blockchain(tx, false);
      bool r = unprocess_blockchain_tx_extra(tx);
      CHECK_AND_ASSERT_MES(r, false, "failed to unprocess_blockchain_tx_extra");
      
      unprocess_blockchain_tx_attachments(tx, bl_height, timestamp);

      return false;
    }
  }
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_process_inputs);
  if (need_to_profile && mixins_count > 0)
  {
    m_performance_data.tx_mixin_count.push(mixins_count);
#ifdef _DEBUG
    LOG_PRINT_L0("[TX_MIXINS]: " <<  mixins_count);
#endif
  }

  //check if there is already transaction with this hash
  TIME_MEASURE_START_PD(tx_check_exist);
  auto tx_entry_ptr = m_db_transactions.get(tx_id);
  if (tx_entry_ptr)
  {
    LOG_ERROR("critical internal error: tx with id: " << tx_id << " in block id: " << bl_id << " already in blockchain");
    purge_transaction_keyimages_from_blockchain(tx, true);
    bool r = unprocess_blockchain_tx_extra(tx);
    CHECK_AND_ASSERT_MES(r, false, "failed to unprocess_blockchain_tx_extra");

    unprocess_blockchain_tx_attachments(tx, bl_height, timestamp);
    return false;
  }
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_check_exist);
  TIME_MEASURE_START_PD(tx_push_global_index);
  transaction_chain_entry ch_e;
  ch_e.m_keeper_block_height = bl_height;
  ch_e.m_spent_flags.resize(tx.vout.size(), false);
  ch_e.tx = tx;
  r = push_transaction_to_global_outs_index(tx, tx_id, ch_e.m_global_output_indexes);
  CHECK_AND_ASSERT_MES(r, false, "failed to return push_transaction_to_global_outs_index tx id " << tx_id);
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_push_global_index);
  
  //store everything to db
  TIME_MEASURE_START_PD(tx_store_db);
  m_db_transactions.set(tx_id, ch_e);
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_store_db);

  TIME_MEASURE_START_PD(tx_print_log);
  LOG_PRINT_L1("Added tx to blockchain: " << tx_id << " via block at " << bl_height << " id " << print16(bl_id)
    << ", ins: " << tx.vin.size() << ", outs: " << tx.vout.size() << ", outs sum: " << print_money_brief(get_outs_money_amount(tx)) << " (fee: " << (is_coinbase(tx) ? "0[coinbase]" : print_money_brief(get_tx_fee(tx))) << ")");
  TIME_MEASURE_FINISH_PD_COND(need_to_profile, tx_print_log);
  //@#@ del me
//   LOG_PRINT_L0("APPEND_TX_TIME_INNER: " << m_performance_data.tx_append_rl_wait.get_last_val() 
//     << " | " << m_performance_data.tx_append_is_expired.get_last_val()
//     << " | " << m_performance_data.tx_process_extra.get_last_val()
//     << " | " << m_performance_data.tx_process_attachment.get_last_val()
//     << " | " << m_performance_data.tx_process_inputs.get_last_val()
//     << " | " << m_performance_data.tx_check_exist.get_last_val()
//     << " | " << m_performance_data.tx_push_global_index.get_last_val()
//     << " | " << m_performance_data.tx_store_db.get_last_val()
//     << " | " << m_performance_data.tx_print_log.get_last_val()
//   );
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto tx_ptr = m_db_transactions.find(tx_id);
  if (!tx_ptr)
  {
    LOG_PRINT_RED_L0("warning: get_tx_outputs_gindexs failed to find transaction with id = " << tx_id);
    return false;
  }

  CHECK_AND_ASSERT_MES(tx_ptr->m_global_output_indexes.size(), false, "internal error: global indexes for transaction " << tx_id << " is empty");
  indexs = tx_ptr->m_global_output_indexes;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash, uint64_t& max_used_block_height, crypto::hash& max_used_block_id) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  bool res = check_tx_inputs(tx, tx_prefix_hash, max_used_block_height);
  if(!res) return false;
  CHECK_AND_ASSERT_MES(max_used_block_height < m_db_blocks.size(), false,  "internal error: max used block index=" << max_used_block_height << " is not less then blockchain size = " << m_db_blocks.size());
  get_block_hash(m_db_blocks[max_used_block_height]->bl, max_used_block_id);
  return true;
}
//------------------------------------------------------------------
#define PERIOD_DISABLED   0xffffffffffffffffLL

bool blockchain_storage::rebuild_tx_fee_medians()
{
  uint64_t sz = m_db_blocks.size();
  m_db.begin_transaction(); 
  LOG_PRINT_L0("Started reinitialization of median fee...");
  math_helper::once_a_time_seconds<10> log_idle;

  epee::misc_utils::median_helper<uint64_t, uint64_t> blocks_median;
  for (uint64_t i = 0; i != sz; i++)
  {
    log_idle.do_call([&]() {std::cout << "block " << i << " of " << sz << std::endl; return true; });

    auto bptr = m_db_blocks[i];
    block_extended_info new_bei = *bptr;
    //assign effective median
    new_bei.effective_tx_fee_median = blocks_median.get_median();

    //calculate current median for this particular block
    std::vector<uint64_t> fees;
    for (auto& h: new_bei.bl.tx_hashes)
    {
      auto txptr = get_tx_chain_entry(h);
      CHECK_AND_ASSERT_MES(txptr, false, "failed to find tx id " << h << " from block " << i);
      fees.push_back(get_tx_fee(txptr->tx));
    }
    new_bei.this_block_tx_fee_median = epee::misc_utils::median(fees);
    m_db_blocks.set(i, new_bei);
    
      
      //prepare median helper for next block
    if(new_bei.this_block_tx_fee_median)
      blocks_median.push_item(new_bei.this_block_tx_fee_median, i);

    //create callbacks 

    bool is_pos_allowed_l = i >= m_core_runtime_config.pos_minimum_heigh;
    uint64_t period = is_pos_allowed_l ? ALIAS_COAST_PERIOD : ALIAS_COAST_PERIOD / 2;
    if (period >= i+1)
      continue;
    uint64_t starter_block_index = i+1 - period;

    uint64_t purge_recent_period = is_pos_allowed_l ? ALIAS_COAST_RECENT_PERIOD : ALIAS_COAST_RECENT_PERIOD / 2;
    uint64_t purge_recent_block_index = 0;
    if (purge_recent_period >= i + 1)
      purge_recent_block_index = PERIOD_DISABLED;
    else
      purge_recent_block_index = i + 1 - purge_recent_period;


    auto cb = [&](uint64_t fee, uint64_t height)
    {
      if (height >= starter_block_index)
        return true;
      return false;
    };

    auto cb_final_eraser = [&](uint64_t fee, uint64_t height)
    {
      if (purge_recent_block_index == PERIOD_DISABLED)
        return true;
      if (height >= purge_recent_block_index)
        return true;

      return false;
    };

    blocks_median.scan_items(cb, cb_final_eraser);
  }

  m_db.commit_transaction();
  LOG_PRINT_L0("Reinitialization of median fee finished!")
  return true;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_tx_fee_median_effective_index(uint64_t h)const
{
  if (h <= ALIAS_MEDIAN_RECALC_INTERWAL + 1)
    return 0;

  h -= 1; // for transactions of block that h%ALIAS_MEDIAN_RECALC_INTERWAL==0 we still handle fee median from previous day
  return h - (h % ALIAS_MEDIAN_RECALC_INTERWAL);
}
//------------------------------------------------------------------
uint64_t blockchain_storage::tx_fee_median_for_height(uint64_t h)const 
{
  uint64_t effective_index = get_tx_fee_median_effective_index(h);
  CHECK_AND_ASSERT_THROW_MES(effective_index < m_db_blocks.size(), "internal error: effective_index= " << effective_index << " while m_db_blocks.size()=" << m_db_blocks.size());
  return m_db_blocks[effective_index]->effective_tx_fee_median;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_all_aliases_for_new_median_mode()
{
    LOG_PRINT_L0("Started reinitialization of median fee...");
  math_helper::once_a_time_seconds<10> log_idle;
  uint64_t sz = m_db_blocks.size();
  for (uint64_t i = 0; i != sz; i++)
  {
    log_idle.do_call([&]() {std::cout << "block " << i << " of " << sz << std::endl; return true; });

    auto bptr = m_db_blocks[i];
    for (auto& tx_id : bptr->bl.tx_hashes)
    {
      auto tx_ptr = m_db_transactions.find(tx_id);
      CHECK_AND_ASSERT_MES(tx_ptr, false, "Internal error: tx  " << tx_id << " from block " << i << " not found");
      tx_extra_info tei = AUTO_VAL_INIT(tei);
      bool r = parse_and_validate_tx_extra(tx_ptr->tx, tei);
      CHECK_AND_ASSERT_MES(r, false, "Internal error: tx  " << tx_id << " from block " << i << " was failed to parse");
      if (tei.m_alias.m_alias.size() && !tei.m_alias.m_sign.size())
      {
        //reward
        //validate alias coast
        uint64_t median_fee = tx_fee_median_for_height(i);
        uint64_t fee_for_alias =  get_alias_coast_from_fee(tei.m_alias.m_alias, median_fee);
        
        //validate the price had been paid
        uint64_t found_alias_reward = get_amount_for_zero_pubkeys(tx_ptr->tx);
        if (found_alias_reward < fee_for_alias)
        {
          LOG_PRINT_RED_L0("[" << i <<  "]Found collision on alias: " << tei.m_alias.m_alias 
            << ", expected fee: " << print_money(fee_for_alias) << "(median:" << print_money(median_fee) << ")"
            <<" found reward: " << print_money(found_alias_reward) <<". tx_id: " << tx_id);
        }
      }
    }
  }

  LOG_PRINT_L0("Finished.");

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::print_tx_outputs_lookup(const crypto::hash& tx_id)const 
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto tx_ptr = m_db_transactions.find(tx_id);

  if (!tx_ptr)
  {
    LOG_PRINT_RED_L0("Tx " << tx_id << " not found");
    return true;
  }

  //amount -> index -> [{tx_id, rind_count}]
  std::map<uint64_t, std::map<uint64_t, std::list<std::pair<crypto::hash,uint64_t > > > > usage_stat;
  std::stringstream strm_tx;
  CHECK_AND_ASSERT_MES(tx_ptr->tx.vout.size() == tx_ptr->m_global_output_indexes.size(), false, "Internal error: output size missmatch");
  for (uint64_t i = 0; i!= tx_ptr->tx.vout.size();i++)
  {
    strm_tx << "[" << i << "]: " << print_money(tx_ptr->tx.vout[i].amount) << ENDL;
    if (tx_ptr->tx.vout[i].target.type() != typeid(currency::txout_to_key))
      continue;
    
    usage_stat[tx_ptr->tx.vout[i].amount][tx_ptr->m_global_output_indexes[i]];

  }
  
  LOG_PRINT_L0("Lookup in all transactions....");
  for (uint64_t i = 0; i != m_db_blocks.size(); i++)
  {
    auto block_ptr = m_db_blocks[i];
    for (auto block_tx_id : block_ptr->bl.tx_hashes)
    {
      auto block_tx_ptr = m_db_transactions.find(block_tx_id);
      for (auto txi_in : block_tx_ptr->tx.vin)
      {
        if(txi_in.type() != typeid(currency::txin_to_key))
          continue;
        currency::txin_to_key& txi_in_tokey = boost::get<currency::txin_to_key>(txi_in);
        uint64_t amount = txi_in_tokey.amount;
        auto amount_it = usage_stat.find(amount);
        if(amount_it == usage_stat.end())
          continue;

        for (txout_ref_v& off : txi_in_tokey.key_offsets)
        {
          if(off.type() != typeid(uint64_t))
            continue;
          uint64_t index = boost::get<uint64_t>(off);
          auto index_it = amount_it->second.find(index);
          if(index_it == amount_it->second.end())
            continue;
          index_it->second.push_back(std::pair<crypto::hash, uint64_t>(block_tx_id, txi_in_tokey.key_offsets.size()));
        }
      }
    }
  }

  std::stringstream ss;
  for (auto& amount : usage_stat)
  {
    for (auto& index : amount.second)
    {
      ss << "[" << print_money(amount.first) << ":" << index.first << "]" << ENDL ;
      for (auto& list_entry : index.second)
      {
        ss << "      " << list_entry.first << ": " << list_entry.second << ENDL; 
      }
    }
  }
  
  LOG_PRINT_L0("Results: " << ENDL << strm_tx.str() << ENDL  << ss.str());

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::have_tx_keyimges_as_spent(const transaction &tx) const
{
  // check all tx's inputs for being already spent
  for (const txin_v& in : tx.vin)
  {
    if (in.type() == typeid(txin_to_key) || in.type() == typeid(txin_htlc))
    {
      if (have_tx_keyimg_as_spent(get_to_key_input_from_txin_v(in).k_image))
      {
        return true;
      }        
    }
    else if (in.type() == typeid(txin_multisig))
    {
      if (is_multisig_output_spent(boost::get<const txin_multisig>(in).multisig_out_id))
        return true;
    }
    else if (in.type() == typeid(txin_gen))
    {
      // skip txin_gen
    }
    else
    {
      LOG_ERROR("Unexpected input type: " << in.type().name());
    }
  }
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash) const
{
  uint64_t stub = 0;
  return check_tx_inputs(tx, tx_prefix_hash, stub);
}
//------------------------------------------------------------------
bool blockchain_storage::check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash, uint64_t& max_used_block_height) const
{
  size_t sig_index = 0;
  max_used_block_height = 0;
  
  std::vector<crypto::signature> sig_stub;
  const std::vector<crypto::signature>* psig = &sig_stub;

  TIME_MEASURE_START_PD(tx_check_inputs_loop);
  for(const auto& txin : tx.vin)
  {
    if (!m_is_in_checkpoint_zone)
    {
      CHECK_AND_ASSERT_MES(sig_index < tx.signatures.size(), false, "Wrong transaction: missing signature entry for input #" << sig_index << " tx: " << tx_prefix_hash);
      psig = &tx.signatures[sig_index];
    }

    if (txin.type() == typeid(txin_to_key))
    {
      const txin_to_key& in_to_key = boost::get<txin_to_key>(txin);

      CHECK_AND_ASSERT_MES(in_to_key.key_offsets.size(), false, "Empty in_to_key.key_offsets for input #" << sig_index << " tx: " << tx_prefix_hash);
      TIME_MEASURE_START_PD(tx_check_inputs_loop_kimage_check);
      if (have_tx_keyimg_as_spent(in_to_key.k_image))
      {
        LOG_ERROR("Key image was already spent in blockchain: " << string_tools::pod_to_hex(in_to_key.k_image) << " for input #" << sig_index << " tx: " << tx_prefix_hash);
        return false;
      }
      TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_kimage_check);
      uint64_t max_unlock_time = 0;
      if (!check_tx_input(tx, sig_index, in_to_key, tx_prefix_hash, *psig, max_used_block_height, max_unlock_time))
      {
        LOG_ERROR("Failed to validate input #" << sig_index << " tx: " << tx_prefix_hash);
        return false;
      }
    }
    else if (txin.type() == typeid(txin_multisig))
    {
      const txin_multisig& in_ms = boost::get<txin_multisig>(txin);
      if (!check_tx_input(tx, sig_index, in_ms, tx_prefix_hash, *psig, max_used_block_height))
      {
        LOG_ERROR("Failed to validate multisig input #" << sig_index << " (ms out id: " << in_ms.multisig_out_id << ") in tx: " << tx_prefix_hash);
        return false;
      }
    }
    else if (txin.type() == typeid(txin_htlc))
    {
      if (!is_after_hardfork_3_zone())
      {
        LOG_ERROR("Error: Transaction with txin_htlc before is_after_hardfork_3_zone(before height " << m_core_runtime_config.hard_fork_03_starts_after_height << ")");
        return false;
      }

      const txin_htlc& in_htlc = boost::get<txin_htlc>(txin);
      CHECK_AND_ASSERT_MES(in_htlc.key_offsets.size(), false, "Empty in_to_key.key_offsets for input #" << sig_index << " tx: " << tx_prefix_hash);
      TIME_MEASURE_START_PD(tx_check_inputs_loop_kimage_check);
      if (have_tx_keyimg_as_spent(in_htlc.k_image))
      {
        LOG_ERROR("Key image was already spent in blockchain: " << string_tools::pod_to_hex(in_htlc.k_image) << " for input #" << sig_index << " tx: " << tx_prefix_hash);
        return false;
      }
      TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_kimage_check);
      if (!check_tx_input(tx, sig_index, in_htlc, tx_prefix_hash, *psig, max_used_block_height))
      {
        LOG_ERROR("Failed to validate multisig input #" << sig_index << " (ms out id: " << obj_to_json_str(in_htlc) << ") in tx: " << tx_prefix_hash);
        return false;
      }
    }
    sig_index++;
  }
  TIME_MEASURE_FINISH_PD(tx_check_inputs_loop);

  TIME_MEASURE_START_PD(tx_check_inputs_attachment_check);
  if (!m_is_in_checkpoint_zone)
  {
    CHECK_AND_ASSERT_MES(tx.signatures.size() == sig_index, false, "tx signatures count differs from inputs");
    if (!(get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE))
    {
      bool r = validate_attachment_info(tx.extra, tx.attachment, false);
      CHECK_AND_ASSERT_MES(r, false, "Failed to validate attachments in tx " << tx_prefix_hash << ": incorrect extra_attachment_info in tx.extra");
    }
  }
  TIME_MEASURE_FINISH_PD(tx_check_inputs_attachment_check);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::is_tx_spendtime_unlocked(uint64_t unlock_time) const
{
  return currency::is_tx_spendtime_unlocked(unlock_time, get_current_blockchain_size(), m_core_runtime_config.get_core_time());
}
//------------------------------------------------------------------
bool blockchain_storage::check_tx_input(const transaction& tx, size_t in_index, const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, uint64_t& max_related_block_height, uint64_t& source_max_unlock_time_for_pos_coinbase) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  //TIME_MEASURE_START_PD(tx_check_inputs_loop_ch_in_get_keys_loop);

  std::vector<crypto::public_key> output_keys;
  scan_for_keys_context scan_context = AUTO_VAL_INIT(scan_context);
  if(!get_output_keys_for_input_with_checks(tx, txin, output_keys, max_related_block_height, source_max_unlock_time_for_pos_coinbase))
  {
    LOG_PRINT_L0("Failed to get output keys for input #" << in_index << " (amount = " << print_money(txin.amount) << ", key_offset.size = " << txin.key_offsets.size() << ")");
    return false;
  }
  //TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_ch_in_get_keys_loop);

  std::vector<const crypto::public_key *> output_keys_ptrs;
  output_keys_ptrs.reserve(output_keys.size());
  for (auto& ptr : output_keys)
    output_keys_ptrs.push_back(&ptr);

  return check_input_signature(tx, in_index, txin, tx_prefix_hash, sig, output_keys_ptrs);
}
//----------------------------------------------------------------
struct outputs_visitor
{
  std::vector<crypto::public_key >&  m_results_collector;
  blockchain_storage::scan_for_keys_context& m_scan_context;
  const blockchain_storage& m_bch;
  uint64_t& m_source_max_unlock_time_for_pos_coinbase;
  outputs_visitor(std::vector<crypto::public_key>& results_collector,
    const blockchain_storage& bch,
    uint64_t& source_max_unlock_time_for_pos_coinbase, 
    blockchain_storage::scan_for_keys_context& scan_context)
    : m_results_collector(results_collector)
    , m_bch(bch)
    , m_source_max_unlock_time_for_pos_coinbase(source_max_unlock_time_for_pos_coinbase)
    , m_scan_context(scan_context)
  {}
  bool handle_output(const transaction& source_tx, const transaction& validated_tx, const tx_out& out, uint64_t out_i)
  {
    //check tx unlock time
    uint64_t source_out_unlock_time = get_tx_unlock_time(source_tx, out_i);
    //let coinbase sources for PoS block to have locked inputs, the outputs supposed to be locked same way, except the reward 
    if (is_coinbase(validated_tx) && is_pos_block(validated_tx))
    {
      CHECK_AND_ASSERT_MES(should_unlock_value_be_treated_as_block_height(source_out_unlock_time), false, "source output #" << out_i << " is locked by time, not by height, which is not allowed for PoS coinbase");
      if (source_out_unlock_time > m_source_max_unlock_time_for_pos_coinbase)
        m_source_max_unlock_time_for_pos_coinbase = source_out_unlock_time;
    }
    else
    {
      if (!m_bch.is_tx_spendtime_unlocked(source_out_unlock_time))
      {
        LOG_PRINT_L0("One of outputs for one of inputs have wrong tx.unlock_time = " << get_tx_unlock_time(source_tx, out_i));
        return false;
      }
    }
    if (out.target.type() == typeid(txout_to_key))
    {
      crypto::public_key pk = boost::get<txout_to_key>(out.target).key;
      m_results_collector.push_back(pk);
    }
    else if (out.target.type() == typeid(txout_htlc))
    {
      m_scan_context.htlc_outs.push_back(boost::get<txout_htlc>(out.target));
      crypto::public_key pk = null_pkey;
      if (m_scan_context.htlc_is_expired)
      {
        pk = boost::get<txout_htlc>(out.target).pkey_refund;
      }
      else
      {
        pk = boost::get<txout_htlc>(out.target).pkey_redeem;
      }
      m_results_collector.push_back(pk);
    }else 
    {
      LOG_PRINT_L0("Output have wrong type id, which=" << out.target.which());
      return false;
    }   

    return true;
  }
};

//------------------------------------------------------------------
// Checks each referenced output for:
// 1) source tx unlock time validity
// 2) mixin restrictions
// 3) general gindex/ref_by_id corectness
bool blockchain_storage::get_output_keys_for_input_with_checks(const transaction& tx, const txin_v& verifying_input, std::vector<crypto::public_key>& output_keys, uint64_t& max_related_block_height, uint64_t& source_max_unlock_time_for_pos_coinbase, scan_for_keys_context& scan_context) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  outputs_visitor vi(output_keys, *this, source_max_unlock_time_for_pos_coinbase, scan_context);
  return scan_outputkeys_for_indexes(tx, verifying_input, vi, max_related_block_height, scan_context);
}
//------------------------------------------------------------------
bool blockchain_storage::get_output_keys_for_input_with_checks(const transaction& tx, const txin_v& verifying_input, std::vector<crypto::public_key>& output_keys, uint64_t& max_related_block_height, uint64_t& source_max_unlock_time_for_pos_coinbase) const
{
  scan_for_keys_context scan_context_dummy = AUTO_VAL_INIT(scan_context_dummy);
  return get_output_keys_for_input_with_checks(tx, verifying_input, output_keys, max_related_block_height, source_max_unlock_time_for_pos_coinbase, scan_context_dummy);
}

//------------------------------------------------------------------
// Note: this function can be used for checking to_key inputs against either main chain or alt chain, that's why it has output_keys_ptrs parameter
// Doesn't check spent flags, the caller must check it.
bool blockchain_storage::check_input_signature(const transaction& tx, size_t in_index, const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, const std::vector<const crypto::public_key*>& output_keys_ptrs) const
{
  if (txin.key_offsets.size() != output_keys_ptrs.size())
  {
    LOG_PRINT_L0("Output keys for tx with amount = " << txin.amount << " and count indexes " << txin.key_offsets.size() << " returned wrong keys count " << output_keys_ptrs.size());
    return false;
  }

  return check_input_signature(tx, in_index, /*txin.key_offsets,*/ txin.amount, txin.k_image, txin.etc_details, tx_prefix_hash, sig, output_keys_ptrs);
}
//------------------------------------------------------------------
bool blockchain_storage::check_input_signature(const transaction& tx,
  size_t in_index,
  uint64_t in_amount,
  const crypto::key_image& in_k_image,
  const std::vector<txin_etc_details_v>& in_etc_details,
  const crypto::hash& tx_prefix_hash,
  const std::vector<crypto::signature>& sig,
  const std::vector<const crypto::public_key*>& output_keys_ptrs) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  TIME_MEASURE_START_PD(tx_check_inputs_loop_ch_in_val_sig);
  
  if(m_is_in_checkpoint_zone)
    return true;

  if (get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE)
  {
    // check attachments, mentioned directly in this input
    bool r = validate_attachment_info(in_etc_details, tx.attachment, in_index != tx.vin.size() - 1); // attachment info can be omitted for all inputs, except the last one
    CHECK_AND_ASSERT_MES(r, false, "Failed to validate attachments in tx " << tx_prefix_hash << ": incorrect extra_attachment_info in etc_details in input #" << in_index);
  }
  else
  {
    // make sure normal tx does not have extra_attachment_info in etc_details
    CHECK_AND_ASSERT_MES(!have_type_in_variant_container<extra_attachment_info>(in_etc_details), false, "Incorrect using of extra_attachment_info in etc_details in input #" << in_index << " for tx " << tx_prefix_hash);
  }

  // check signatures
  size_t expected_signatures_count = output_keys_ptrs.size();
  bool need_to_check_extra_sign = false;
  if (get_tx_flags(tx)&TX_FLAG_SIGNATURE_MODE_SEPARATE && in_index == tx.vin.size() - 1)
  {
    expected_signatures_count++;
    need_to_check_extra_sign = true;
  }

  CHECK_AND_ASSERT_MES(expected_signatures_count == sig.size(), false, "internal error: tx signatures count=" << sig.size() << " mismatch with outputs keys count for inputs=" << expected_signatures_count);
  crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, in_index, tx_prefix_hash);
  CHECK_AND_ASSERT_MES(tx_hash_for_signature != null_hash, false, "failed to  prepare_prefix_hash_for_sign");

  LOG_PRINT_L4("CHECK RING SIGNATURE: tx_prefix_hash " << tx_prefix_hash
    << "tx_hash_for_signature" << tx_hash_for_signature
    << "in_k_image" << in_k_image
    << "key_ptr:" << *output_keys_ptrs[0]
    << "signature:" << sig[0]);
  bool r = crypto::validate_key_image(in_k_image);
  CHECK_AND_ASSERT_MES(r, false, "key image for input #" << in_index << " is invalid: " << in_k_image);

  r = crypto::check_ring_signature(tx_hash_for_signature, in_k_image, output_keys_ptrs, sig.data());
  CHECK_AND_ASSERT_MES(r, false, "failed to check ring signature for input #" << in_index << ENDL << dump_ring_sig_data(tx_hash_for_signature, in_k_image, output_keys_ptrs, sig));
  if (need_to_check_extra_sign)
  {
    //here we check extra signature to validate that transaction was finalized by authorized subject
    r = crypto::check_signature(tx_prefix_hash, get_tx_pub_key_from_extra(tx), sig.back());
    CHECK_AND_ASSERT_MES(r, false, "failed to check extra signature for last input with TX_FLAG_SIGNATURE_MODE_SEPARATE");
  }

  TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_ch_in_val_sig);
  return r;
}
//------------------------------------------------------------------
// Note: this function doesn't check spent flags by design (to be able to use either for main chain and alt chains).
// The caller MUST check spent flags.
bool blockchain_storage::check_ms_input(const transaction& tx, size_t in_index, const txin_multisig& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, const transaction& source_tx, size_t out_n) const
{
#define LOC_CHK(cond, msg) CHECK_AND_ASSERT_MES(cond, false, "ms input check failed: ms_id: " << txin.multisig_out_id << ", input #" << in_index << " in tx " << tx_prefix_hash << ", refers to ms output #" << out_n << " in source tx " << get_transaction_hash(source_tx) << ENDL << msg)
  CRITICAL_REGION_LOCAL(m_read_lock);

  uint64_t unlock_time = get_tx_unlock_time(source_tx, out_n);
  LOC_CHK(is_tx_spendtime_unlocked(unlock_time), "Source transaction is LOCKED! unlock_time: " << unlock_time << ", now is " << m_core_runtime_config.get_core_time() << ", blockchain size is " << get_current_blockchain_size());

  LOC_CHK(source_tx.vout.size() > out_n, "internal error: out_n==" << out_n << " is out-of-bounds of source_tx.vout, size=" << source_tx.vout.size());
  const tx_out& source_tx_out = source_tx.vout[out_n];
  const txout_multisig& source_ms_out_target = boost::get<txout_multisig>(source_tx_out.target);

  LOC_CHK(txin.sigs_count == source_ms_out_target.minimum_sigs,
    "ms input's sigs_count (" << txin.sigs_count << ") does not match to ms output's minimum signatures expected (" << source_ms_out_target.minimum_sigs << ")"
    << ", source_tx_out.amount=" << print_money(source_tx_out.amount)
    << ", txin.amount = " << print_money(txin.amount));

  LOC_CHK(source_tx_out.amount == txin.amount,
    "amount missmatch"
    << ", source_tx_out.amount=" << print_money(source_tx_out.amount)
    << ", txin.amount = " << print_money(txin.amount));

  if (m_is_in_checkpoint_zone)
    return true;

  if (get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE)
  {
    // check attachments, mentioned directly in this input
    bool r = validate_attachment_info(txin.etc_details, tx.attachment, in_index != tx.vin.size() - 1); // attachment info can be omitted for all inputs, except the last one
    LOC_CHK(r, "Failed to validate attachments in tx " << tx_prefix_hash << ": incorrect extra_attachment_info in etc_details in input #" << in_index);
  }
  else
  {
    // make sure normal tx does not have extra_attachment_info in etc_details
    LOC_CHK(!have_type_in_variant_container<extra_attachment_info>(txin.etc_details), "Incorrect using of extra_attachment_info in etc_details in input #" << in_index << " for tx " << tx_prefix_hash);
  }

  LOC_CHK(tx.signatures.size() > in_index, "ms input index is out of signatures container bounds, tx.signatures.size() = " << tx.signatures.size());
  const std::vector<crypto::signature>& input_signatures = tx.signatures[in_index];

  size_t expected_signatures_count = txin.sigs_count;
  bool need_to_check_extra_sign = false;
  if (get_tx_flags(tx)&TX_FLAG_SIGNATURE_MODE_SEPARATE && in_index == tx.vin.size() - 1) // last input in TX_FLAG_SIGNATURE_MODE_SEPARATE must contain one more signature to ensure that tx was completed by an authorized subject
  {
    expected_signatures_count++;
    need_to_check_extra_sign = true;
  }

  LOC_CHK(expected_signatures_count == input_signatures.size(), "Invalid input's signatures count: " << input_signatures.size() << ", expected: " << expected_signatures_count);

  crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, in_index, tx_prefix_hash);
  LOC_CHK(tx_hash_for_signature != null_hash, "prepare_prefix_hash_for_sign failed");

  LOC_CHK(txin.sigs_count <= source_ms_out_target.keys.size(), "source tx invariant failed: ms output's minimum sigs == ms input's sigs_count (" << txin.sigs_count << ") is GREATHER than keys.size() = " << source_ms_out_target.keys.size()); // NOTE: sig_count == minimum_sigs as checked above
  size_t out_key_index = 0; // index in source_ms_out_target.keys
  for (size_t i = 0; i != txin.sigs_count; /* nothing */)
  {
    // if we run out of keys for this signature, then it's invalid signature
    LOC_CHK(out_key_index < source_ms_out_target.keys.size(), "invalid signature #" << i << ": " << input_signatures[i]);

    // check signature #i against ms output key #out_key_index
    if (crypto::check_signature(tx_hash_for_signature, source_ms_out_target.keys[out_key_index], input_signatures[i]))
    {
      // match: go for the next signature and the next key
      i++;
      out_key_index++;
    }
    else
    {
      // missmatch: go for the next key for this signature
      out_key_index++;
    }
  }
  if (need_to_check_extra_sign)
  {
    //here we check extra signature to validate that transaction was finilized by authorized subject
    bool r = crypto::check_signature(tx_prefix_hash, get_tx_pub_key_from_extra(tx), tx.signatures[in_index].back());
    LOC_CHK(r, "failed to check extra signature for last out with TX_FLAG_SIGNATURE_MODE_SEPARATE");
  }

  return true;
#undef LOC_CHK
}

//------------------------------------------------------------------
bool blockchain_storage::check_tx_input(const transaction& tx, size_t in_index, const txin_multisig& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, uint64_t& max_related_block_height) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  auto multisig_ptr = m_db_multisig_outs.find(txin.multisig_out_id);
  CHECK_AND_ASSERT_MES(multisig_ptr, false, "Unable to find txin.multisig_out_id=" << txin.multisig_out_id << " for ms input #" << in_index << " in tx " << tx_prefix_hash);

  const crypto::hash& source_tx_id = multisig_ptr->tx_id; // source tx hash
  size_t n = multisig_ptr->out_no; // index of multisig output in source tx

#define LOC_CHK(cond, msg) CHECK_AND_ASSERT_MES(cond, false, "ms input check failed: ms_id: " << txin.multisig_out_id << ", input #" << in_index << " in tx " << tx_prefix_hash << ", refers to ms output #" << n << " in source tx " << source_tx_id << ENDL << msg)

  LOC_CHK(multisig_ptr->spent_height == 0, "ms output is already spent on height " << multisig_ptr->spent_height);

  auto source_tx_ptr = m_db_transactions.find(source_tx_id);
  LOC_CHK(source_tx_ptr, "Can't find source transaction");
  LOC_CHK(source_tx_ptr->tx.vout.size() > n, "ms output index is incorrect, source tx's vout size is " << source_tx_ptr->tx.vout.size());
  LOC_CHK(source_tx_ptr->tx.vout[n].target.type() == typeid(txout_multisig), "ms output has wrong type, txout_multisig expected");
  LOC_CHK(source_tx_ptr->m_spent_flags.size() > n, "Internal error, m_spent_flags size (" << source_tx_ptr->m_spent_flags.size() << ") less then expected, n: " << n);
  LOC_CHK(source_tx_ptr->m_spent_flags[n] == false, "Internal error, ms output is already spent"); // should never happen as multisig_ptr->spent_height is checked above

  if (!check_ms_input(tx, in_index, txin, tx_prefix_hash, sig, source_tx_ptr->tx, n))
    return false;

  max_related_block_height = source_tx_ptr->m_keeper_block_height;

  return true;
#undef LOC_CHK
} 
//------------------------------------------------------------------
bool blockchain_storage::check_tx_input(const transaction& tx, size_t in_index, const txin_htlc& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, uint64_t& max_related_block_height)const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  //TIME_MEASURE_START_PD(tx_check_inputs_loop_ch_in_get_keys_loop);

  std::vector<crypto::public_key> output_keys;
  scan_for_keys_context scan_contex = AUTO_VAL_INIT(scan_contex);
  uint64_t source_max_unlock_time_for_pos_coinbase_dummy = AUTO_VAL_INIT(source_max_unlock_time_for_pos_coinbase_dummy);
  if (!get_output_keys_for_input_with_checks(tx, txin, output_keys, max_related_block_height, source_max_unlock_time_for_pos_coinbase_dummy, scan_contex))
  {
    LOG_PRINT_L0("Failed to get output keys for input #" << in_index << " (amount = " << print_money(txin.amount) << ", key_offset.size = " << txin.key_offsets.size() << ")");
    return false;
  }

  CHECK_AND_ASSERT_THROW_MES(scan_contex.htlc_outs.size() == 1, "htlc output not found for input, tx: " << get_transaction_hash(tx));
  const txout_htlc& related_out = *scan_contex.htlc_outs.begin();
  bool use_sha256 = !(related_out.flags&CURRENCY_TXOUT_HTLC_FLAGS_HASH_TYPE_MASK);
  if (use_sha256)
  {
    //doing sha256 hash
    crypto::hash sha256 = crypto::sha256_hash(txin.hltc_origin.data(), txin.hltc_origin.size());
    CHECK_AND_ASSERT_THROW_MES(sha256 == related_out.htlc_hash, "htlc hash missmatched for tx: " << get_transaction_hash(tx) 
      << " calculated hash: " << sha256 << " expected hash(related_out.htlc_hash): " << related_out.htlc_hash);
  }
  else
  {
    //doing RIPEMD160
    crypto::hash160 ripemd160 = crypto::RIPEMD160_hash(txin.hltc_origin.data(), txin.hltc_origin.size());
    crypto::hash160 expected_ripemd160 = *(crypto::hash160*)&related_out.htlc_hash;
    CHECK_AND_ASSERT_THROW_MES(ripemd160 == expected_ripemd160, "htlc hash missmatched for tx: " << get_transaction_hash(tx)
      << " calculated hash: " << ripemd160 << " expected hash(related_out.htlc_hash): " << expected_ripemd160);
  }


  //TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_ch_in_get_keys_loop);

  

  std::vector<const crypto::public_key *> output_keys_ptrs;
  output_keys_ptrs.reserve(output_keys.size());
  for (auto& ptr : output_keys)
    output_keys_ptrs.push_back(&ptr);

  CHECK_AND_ASSERT_THROW_MES(output_keys_ptrs.size() == 1, "Internal error: output_keys_ptrs.size() is not equal 1  for HTLC");

  return check_input_signature(tx, in_index, txin.amount, txin.k_image, txin.etc_details, tx_prefix_hash, sig, output_keys_ptrs);
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_adjusted_time() const
{
  //TODO: add collecting median time
  return m_core_runtime_config.get_core_time();
}
//------------------------------------------------------------------
std::vector<uint64_t> blockchain_storage::get_last_n_blocks_timestamps(size_t n) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  std::vector<uint64_t> timestamps;
  size_t offset = m_db_blocks.size() <= n ? 0 : m_db_blocks.size() - n;
  for (; offset != m_db_blocks.size(); ++offset)
    timestamps.push_back(m_db_blocks[offset]->bl.timestamp);
  return timestamps;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_last_n_blocks_timestamps_median(size_t n) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto it = m_timestamps_median_cache.find(n);
  if (it != m_timestamps_median_cache.end())
    return it->second;

  std::vector<uint64_t> timestamps = get_last_n_blocks_timestamps(n);
  uint64_t median_res = epee::misc_utils::median(timestamps);
  m_timestamps_median_cache[n] = median_res;
  return median_res;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_last_timestamps_check_window_median() const
{
  return get_last_n_blocks_timestamps_median(BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_tx_expiration_median() const
{
  return get_last_n_blocks_timestamps_median(TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW);
}
//------------------------------------------------------------------
bool blockchain_storage::check_block_timestamp_main(const block& b) const
{
  if(b.timestamp > get_adjusted_time() + CURRENCY_BLOCK_FUTURE_TIME_LIMIT)
  {
    LOG_PRINT_L0("Timestamp of block with id: " << get_block_hash(b) << ", " << b.timestamp << ", bigger than adjusted time + " + epee::misc_utils::get_time_interval_string(CURRENCY_BLOCK_FUTURE_TIME_LIMIT));
    return false;
  }
  if (is_pos_block(b) && b.timestamp > get_adjusted_time() + CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT)
  {
    LOG_PRINT_L0("Timestamp of PoS block with id: " << get_block_hash(b) << ", " << b.timestamp << ", bigger than adjusted time + " + epee::misc_utils::get_time_interval_string(CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT) + ": " << get_adjusted_time() << " (" << b.timestamp - get_adjusted_time() << ")");
    return false;
  }

  std::vector<uint64_t> timestamps = get_last_n_blocks_timestamps(BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);

  return check_block_timestamp(std::move(timestamps), b);
}
//------------------------------------------------------------------
bool blockchain_storage::check_block_timestamp(std::vector<uint64_t> timestamps, const block& b) const
{
  if(timestamps.size() < BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW)
    return true;

  if (is_pos_block(b) && b.timestamp > get_adjusted_time() + CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT)
  {
    LOG_PRINT_L0("Timestamp of PoS block with id: " << get_block_hash(b) << ", " << b.timestamp << ", bigger than adjusted time + " + epee::misc_utils::get_time_interval_string(CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT) + ": " << get_adjusted_time() << " (" << b.timestamp - get_adjusted_time() << ")");
    return false;
  }

  uint64_t median_ts = epee::misc_utils::median(timestamps);

  if(b.timestamp < median_ts)
  {
    LOG_PRINT_L0("Timestamp of block with id: " << get_block_hash(b) << ", " << b.timestamp << ", less than median of last " << BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW << " blocks, " << median_ts);
    return false;
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::is_tx_expired(const transaction& tx) const
{
  return currency::is_tx_expired(tx, get_tx_expiration_median());
}
//------------------------------------------------------------------
std::shared_ptr<const transaction_chain_entry> blockchain_storage::find_key_image_and_related_tx(const crypto::key_image& ki, crypto::hash& id_result) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  auto ki_index_ptr = m_db_spent_keys.find(ki);
  if (!ki_index_ptr)
    return std::shared_ptr<transaction_chain_entry>();

  auto block_entry = m_db_blocks[*ki_index_ptr];
  if (!block_entry)
  {
    LOG_ERROR("Internal error: broken index, key image " << ki 
      << " reffered to height " << *ki_index_ptr << " but couldnot find block on this height");
    return std::shared_ptr<const transaction_chain_entry>();
  }

  //look up coinbase
  for (auto& in: block_entry->bl.miner_tx.vin)
  {
    if (in.type() == typeid(txin_to_key))
    {
      if (boost::get<txin_to_key>(in).k_image == ki)
      {
        id_result = get_transaction_hash(block_entry->bl.miner_tx);
        return get_tx_chain_entry(id_result);
      }
    }
  }
  //lookup transactions
  for (auto& tx_id : block_entry->bl.tx_hashes)
  {
    auto tx_chain_entry = get_tx_chain_entry(tx_id);
    if (!tx_chain_entry)
    {
      LOG_ERROR("Internal error: broken index, tx_id " << tx_id
        << " not found in tx index, block no " << *ki_index_ptr);
      return std::shared_ptr<const transaction_chain_entry>();
    }
    for (auto& in : tx_chain_entry->tx.vin)
    {
      if (in.type() == typeid(txin_to_key) || in.type() == typeid(txin_htlc))
      {
        if (get_to_key_input_from_txin_v(in).k_image == ki)
        {
          id_result = get_transaction_hash(tx_chain_entry->tx);
          return tx_chain_entry;
        }
      }
    }
  }
  return std::shared_ptr<const transaction_chain_entry>();
}
//------------------------------------------------------------------
bool blockchain_storage::prune_aged_alt_blocks()
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  CRITICAL_REGION_LOCAL1(m_alternative_chains_lock);
  uint64_t current_height = get_current_blockchain_size();

  size_t count_to_delete = 0;
  if(m_alternative_chains.size() > m_core_runtime_config.max_alt_blocks)
    count_to_delete = m_alternative_chains.size() - m_core_runtime_config.max_alt_blocks;

  std::map<uint64_t, alt_chain_container::iterator> alts_to_delete;

  for(auto it = m_alternative_chains.begin(); it != m_alternative_chains.end();)
  {
    if (current_height > it->second.height && current_height - it->second.height > CURRENCY_ALT_BLOCK_LIVETIME_COUNT)
    {
      do_erase_altblock(it++);
    }
    else
    {
      if (count_to_delete)
      {
        if (!alts_to_delete.size())
          alts_to_delete[it->second.timestamp] = it;
        else
        {
          if (it->second.timestamp >= alts_to_delete.rbegin()->first)
            alts_to_delete[it->second.timestamp] = it;

          if (alts_to_delete.size() > count_to_delete)
            alts_to_delete.erase(alts_to_delete.begin());
        }
      }

      ++it;
    }
  }
  //now, if there was count_to_delete we should erase most oldest entries of altblocks
  for (auto& itd : alts_to_delete)
  {
    m_alternative_chains.erase(itd.second);
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_pos_block(const block& b, const crypto::hash& id, bool for_altchain) const
{
  //validate 
  wide_difficulty_type basic_diff = get_next_diff_conditional(true);
  return validate_pos_block(b, basic_diff, id, for_altchain);
}
//------------------------------------------------------------------
bool blockchain_storage::validate_pos_block(const block& b, wide_difficulty_type basic_diff, const crypto::hash& id, bool for_altchain) const
{
  uint64_t coin_age = 0;
  wide_difficulty_type final_diff = 0;
  crypto::hash proof_hash = null_hash;
  return validate_pos_block(b, basic_diff, coin_age, final_diff, proof_hash, id, for_altchain);
}
//------------------------------------------------------------------
#define POS_STAKE_TO_DIFF_COEFF   100  // total_coins_in_minting * POS_STAKE_TO_DIFF_COEFF =~ pos_difficulty

void blockchain_storage::get_pos_mining_estimate(uint64_t amount_coins, 
  uint64_t time, 
  uint64_t& estimate_result, 
  uint64_t& pos_diff_and_amount_rate, 
  std::vector<uint64_t>& days) const 
{
  estimate_result = 0;
  if (!is_pos_allowed())
    return;

  // new algo
  // 1. get (CURRENCY_BLOCKS_PER_DAY / 2) PoS blocks (a day in case of PoS/PoW==50/50)
  // 2. calculate total minted money C (normalized for 1 day interval) and average difficulty D (based on last (CURRENCY_BLOCKS_PER_DAY / 2) PoS blocks)
  // 3. calculate total coins participating in PoS minting as M = D / pos_diff_ratio
  // 4. calculate owner's money proportion as P = amount_coins / (M + amount_coins)
  // 5. estimate PoS minting income for this day as I = C * P
  // 6. amount_coins += I, goto 3

  epee::critical_region_t<decltype(m_read_lock)> read_lock_region(m_read_lock);

  size_t estimated_pos_blocks_count_per_day = CURRENCY_BLOCKS_PER_DAY / 2; // 50% of all blocks in a perfect world
  
  uint64_t pos_ts_min = UINT64_MAX, pos_ts_max = 0;
  size_t pos_blocks_count = 0;
  uint64_t pos_total_minted_money = 0;
  wide_difficulty_type pos_avg_difficulty = 0;
  // scan blockchain backward for PoS blocks and collect data
  for (size_t h = m_db_blocks.size() - 1; h != 0 && pos_blocks_count < estimated_pos_blocks_count_per_day; --h)
  {
    auto bei = m_db_blocks[h];
    if (!is_pos_block(bei->bl))
      continue;
    uint64_t ts = get_block_datetime(bei->bl);
    pos_ts_min = min(pos_ts_min, ts);
    pos_ts_max = max(pos_ts_max, ts);
    pos_total_minted_money += get_reward_from_miner_tx(bei->bl.miner_tx);
    pos_avg_difficulty += bei->difficulty;
    ++pos_blocks_count;
  }
  if (pos_blocks_count < estimated_pos_blocks_count_per_day || pos_ts_max <= pos_ts_min)
    return; // too little pos blocks found or invalid ts
  pos_avg_difficulty /= pos_blocks_count;
  uint64_t found_blocks_interval = pos_ts_max - pos_ts_min; // will be close to 24 * 60 * 60 in case of PoS/PoW == 50/50
  uint64_t pos_last_day_total_minted_money = pos_total_minted_money * (24 * 60 * 60) / found_blocks_interval; // total minted money normalized for 1 day interval

  uint64_t estimated_total_minting_coins = static_cast<uint64_t>(pos_avg_difficulty / POS_STAKE_TO_DIFF_COEFF);

  uint64_t current_amount = amount_coins;
  uint64_t days_count = time / (60 * 60 * 24);
  for (uint64_t d = 0; d != days_count; d++)
  {
    double user_minting_coins_proportion = static_cast<double>(current_amount) / (estimated_total_minting_coins + current_amount);
    current_amount += pos_last_day_total_minted_money * user_minting_coins_proportion;
    days.push_back(current_amount);
  }
  estimate_result = current_amount;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_tx_for_hardfork_specific_terms(const transaction& tx, const crypto::hash& tx_id) const
{
  uint64_t block_height = m_db_blocks.size();
  return validate_tx_for_hardfork_specific_terms(tx, tx_id, block_height);
}
//------------------------------------------------------------------
bool blockchain_storage::validate_tx_for_hardfork_specific_terms(const transaction& tx, const crypto::hash& tx_id, uint64_t block_height) const
{
  auto is_allowed_before_hardfork2 = [&](const payload_items_v& el) -> bool
  {
    CHECK_AND_ASSERT_MES(el.type() != typeid(tx_payer), false, "tx " << tx_id << " contains tx_payer which is not allowed on height " << block_height);
    CHECK_AND_ASSERT_MES(el.type() != typeid(tx_receiver), false, "tx " << tx_id << " contains tx_receiver which is not allowed on height " << block_height);
    CHECK_AND_ASSERT_MES(el.type() != typeid(extra_alias_entry), false, "tx " << tx_id << " contains extra_alias_entry which is not allowed on height " << block_height);
    return true;
  };

  auto is_allowed_before_hardfork1 = [&](const payload_items_v& el) -> bool
  {
    CHECK_AND_ASSERT_MES(el.type() != typeid(etc_tx_details_unlock_time2), false, "tx " << tx_id << " contains etc_tx_details_unlock_time2 which is not allowed on height " << block_height);
    return true;
  };

  bool var_is_after_hardfork_1_zone = is_after_hardfork_1_zone(block_height);
  bool var_is_after_hardfork_2_zone = is_after_hardfork_2_zone(block_height);
  bool var_is_after_hardfork_3_zone = is_after_hardfork_3_zone(block_height);
  
  //inputs
  for (const auto in : tx.vin)
  {
    if (in.type() == typeid(txin_htlc))
    {
      if (!var_is_after_hardfork_3_zone)
        return false;
    }
  }
  //outputs
  for (const auto out : tx.vout)
  {
    if (out.target.type() == typeid(txout_htlc))
    {
      if (!var_is_after_hardfork_3_zone)
        return false;
    }
  }

  //extra
  for (const auto el : tx.extra)
  {
    if (!var_is_after_hardfork_1_zone && !is_allowed_before_hardfork1(el))
      return false;
    if (!var_is_after_hardfork_2_zone && !is_allowed_before_hardfork2(el))
      return false;
  }

  //attachments
  for (const auto el : tx.attachment)
  {
    if (!var_is_after_hardfork_2_zone && !is_allowed_before_hardfork2(el))
      return false;
  }
    

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_pos_coinbase_outs_unlock_time(const transaction& miner_tx, uint64_t staked_amount, uint64_t source_max_unlock_time)const
{
  uint64_t major_unlock_time = get_tx_x_detail<etc_tx_details_unlock_time>(miner_tx);
  if (major_unlock_time)
  {
    //if there was etc_tx_details_unlock_time present in tx, then ignore etc_tx_details_unlock_time2
    if (major_unlock_time < source_max_unlock_time)
      return false;
    else
      return true;
  }
  
  CHECK_AND_ASSERT_MES(get_block_height(miner_tx) > m_core_runtime_config.hard_fork_01_starts_after_height, false, "error in block [" << get_block_height(miner_tx) << "] etc_tx_details_unlock_time2 can exist only after hard fork point : " << m_core_runtime_config.hard_fork_01_starts_after_height);

  //etc_tx_details_unlock_time2 can be kept only after hard_fork_1 point
  etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
  get_type_in_variant_container(miner_tx.extra, ut2);
  CHECK_AND_ASSERT_MES(ut2.unlock_time_array.size() == miner_tx.vout.size(), false, "ut2.unlock_time_array.size()<" << ut2.unlock_time_array.size() 
    << "> != miner_tx.vout.size()<" << miner_tx.vout.size() << ">");
  
  uint64_t amount_of_coins_in_unlock_in_range = 0; // amount of outputs locked for at least the same time

  for (uint64_t i = 0; i != miner_tx.vout.size(); i++)
  {
    uint64_t unlock_value = ut2.unlock_time_array[i];
    CHECK_AND_ASSERT_MES(should_unlock_value_be_treated_as_block_height(unlock_value), false, "output #" << i << " is locked by time, not buy height, which is not allowed for PoS coinbase");
    if (unlock_value >= source_max_unlock_time)
      amount_of_coins_in_unlock_in_range += miner_tx.vout[i].amount;
  }
  
  if (amount_of_coins_in_unlock_in_range >= staked_amount)
    return true;
  LOG_ERROR("amount_of_coins_in_unlock_in_range<" << amount_of_coins_in_unlock_in_range << "> is less then staked_amount<" << staked_amount);
  return false;

}
//------------------------------------------------------------------
bool blockchain_storage::validate_pos_block(const block& b,   
                                            wide_difficulty_type basic_diff, 
                                            uint64_t& amount, 
                                            wide_difficulty_type& final_diff, 
                                            crypto::hash& proof_hash, 
                                            const crypto::hash& id, 
                                            bool for_altchain, 
                                            const alt_chain_type& alt_chain,
                                            uint64_t split_height
                                            )const 
{
  bool is_pos = is_pos_block(b);
  CHECK_AND_ASSERT_MES(is_pos, false, "is_pos_block() returned false validate_pos_block()");

  //check timestamp
  CHECK_AND_ASSERT_MES(b.timestamp%POS_SCAN_STEP == 0, false, "wrong timestamp in PoS block(b.timestamp%POS_SCAN_STEP == 0), b.timestamp = " <<b.timestamp);

  //check keyimage
  CHECK_AND_ASSERT_MES(b.miner_tx.vin[1].type() == typeid(txin_to_key), false, "coinstake transaction in the block has the wrong type");
  const txin_to_key& in_to_key = boost::get<txin_to_key>(b.miner_tx.vin[1]);
  if (!for_altchain && have_tx_keyimg_as_spent(in_to_key.k_image))
  {
      LOG_PRINT_L0("Key image in coinstake already spent in blockchain: " << string_tools::pod_to_hex(in_to_key.k_image));
      return false;
  }


  // the following check is de-facto not applicable since 2021-10, but left intact to avoid consensus issues
  // PoS blocks don't use etc_tx_time anymore to store actual timestamp; instead, they use tx_service_attachment in mining tx extra
  uint64_t actual_ts = get_actual_timestamp(b);
  if ((actual_ts > b.timestamp && actual_ts - b.timestamp > POS_MAX_ACTUAL_TIMESTAMP_TO_MINED) ||
    (actual_ts < b.timestamp && b.timestamp - actual_ts > POS_MAX_ACTUAL_TIMESTAMP_TO_MINED)
     )
  {
    LOG_PRINT_L0("PoS block actual timestamp " << actual_ts << " differs from b.timestamp " << b.timestamp << " by " << ((int64_t)actual_ts - (int64_t)b.timestamp) << " s, it's more than allowed " << POS_MAX_ACTUAL_TIMESTAMP_TO_MINED << " s.");
    return false;
  }

  //check kernel
  stake_kernel sk = AUTO_VAL_INIT(sk);

  stake_modifier_type sm = AUTO_VAL_INIT(sm);
  bool r = build_stake_modifier(sm, alt_chain, split_height);
  CHECK_AND_ASSERT_MES(r, false, "failed to build_stake_modifier");
  amount = 0;
  r = build_kernel(b, sk, amount, sm);
  CHECK_AND_ASSERT_MES(r, false, "failed to build kernel_stake");
  CHECK_AND_ASSERT_MES(amount!=0, false, "failed to build kernel_stake, amount == 0");

  proof_hash = crypto::cn_fast_hash(&sk, sizeof(sk));

  LOG_PRINT_L2("STAKE KERNEL for bl ID: " << get_block_hash(b) << ENDL
    << print_stake_kernel_info(sk)
    << "amount: " << print_money(amount) << ENDL
    << "kernel_hash: " << proof_hash);


  final_diff = basic_diff / amount;
  if (!check_hash(proof_hash, final_diff))
  {
    LOG_ERROR("PoS difficulty check failed for block " << get_block_hash(b) << " @ HEIGHT " << get_block_height(b) << ":" << ENDL
      << "  basic_diff:  " << basic_diff << ENDL
      << "  final_diff:  " << final_diff << ENDL
      << "  amount:      " << print_money_brief(amount) << ENDL
      << "  kernel_hash: " << proof_hash << ENDL
      );
    return false;
  }

  //validate signature
  uint64_t max_related_block_height = 0;
  const txin_to_key& coinstake_in = boost::get<txin_to_key>(b.miner_tx.vin[1]);
  CHECK_AND_ASSERT_MES(b.miner_tx.signatures.size() == 1, false, "PoS block's miner_tx has incorrect signatures size = " << b.miner_tx.signatures.size() << ", block_id = " << get_block_hash(b));
  if (!for_altchain)
  {
    // Do coinstake input validation for main chain only.
    // Txs in alternative PoS blocks (including miner_tx) are validated by validate_alt_block_txs()
    uint64_t source_max_unlock_time_for_pos_coinbase = 0;
    r = check_tx_input(b.miner_tx, 1, coinstake_in, id, b.miner_tx.signatures[0], max_related_block_height, source_max_unlock_time_for_pos_coinbase);
    CHECK_AND_ASSERT_MES(r, false, "Failed to validate coinstake input in miner tx, block_id = " << get_block_hash(b));

    if (get_block_height(b) > m_core_runtime_config.hard_fork_01_starts_after_height)
    {
      uint64_t last_pow_h = get_last_x_block_height(false);
      CHECK_AND_ASSERT_MES(max_related_block_height <= last_pow_h, false, "Failed to validate coinbase in PoS block, condition failed: max_related_block_height(" << max_related_block_height << ") <= last_pow_h(" << last_pow_h << ")");
      //let's check that coinbase amount and unlock time
      r = validate_pos_coinbase_outs_unlock_time(b.miner_tx, coinstake_in.amount, source_max_unlock_time_for_pos_coinbase);
      CHECK_AND_ASSERT_MES(r, false, "Failed to validate_pos_coinbase_outs_unlock_time() in miner tx, block_id = " << get_block_hash(b) 
        << "source_max_unlock_time_for_pos_coinbase=" << source_max_unlock_time_for_pos_coinbase);
    }
    else
    {
      CHECK_AND_ASSERT_MES(is_tx_spendtime_unlocked(source_max_unlock_time_for_pos_coinbase), false, "Failed to validate coinbase in PoS block, condition failed: is_tx_spendtime_unlocked(source_max_unlock_time_for_pos_coinbase)(" << source_max_unlock_time_for_pos_coinbase << ")");
    }
  }

  uint64_t block_height = for_altchain ? split_height + alt_chain.size() : m_db_blocks.size();
  uint64_t coinstake_age = block_height - max_related_block_height - 1;

  CHECK_AND_ASSERT_MES(coinstake_age >= m_core_runtime_config.min_coinstake_age, false,
    "Coinstake age is: " << coinstake_age << " is less than minimum expected: " << m_core_runtime_config.min_coinstake_age);

  return true;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_adjusted_cumulative_difficulty_for_next_pos(wide_difficulty_type next_diff) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  wide_difficulty_type last_pow_diff = 0;
  wide_difficulty_type last_pos_diff = 0;
  uint64_t sz = m_db_blocks.size();
  if (!sz)
    return next_diff;

  uint64_t i = sz - 1;
  
  for (; i < sz && !(last_pow_diff && last_pos_diff); i--)
  {
    if (is_pos_block(m_db_blocks[i]->bl))
    {
      if (!last_pos_diff)
      {
        last_pos_diff = m_db_blocks[i]->difficulty;
      }
    }else
    {
      if (!last_pow_diff)
      {
        last_pow_diff = m_db_blocks[i]->difficulty;
      }
    }
  }
  if (!last_pos_diff)
    return next_diff;
  return next_diff*last_pow_diff/last_pos_diff;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_adjusted_cumulative_difficulty_for_next_alt_pos(alt_chain_type& alt_chain, uint64_t block_height, wide_difficulty_type next_diff, uint64_t connection_height) const
{

  wide_difficulty_type last_pow_diff = 0;
  wide_difficulty_type last_pos_diff = 0;

  for (auto it = alt_chain.rbegin(); it != alt_chain.rend() && !(last_pos_diff && last_pow_diff); it++)
  {
    if (is_pos_block((*it)->second.bl ))
    {
      if (!last_pos_diff)
      {
        last_pos_diff = (*it)->second.difficulty;
      }
    }
    else
    {
      if (!last_pow_diff)
      {
        last_pow_diff = (*it)->second.difficulty;
      }
    }
  }

  CRITICAL_REGION_LOCAL(m_read_lock);
  for (uint64_t h = connection_height - 1; h != 0 && !(last_pos_diff && last_pow_diff); --h)
  {
    if (is_pos_block(m_db_blocks[h]->bl))
    {
      if (!last_pos_diff)
      {
        last_pos_diff = m_db_blocks[h]->difficulty;
      }
    }
    else
    {
      if (!last_pow_diff)
      {
        last_pow_diff = m_db_blocks[h]->difficulty;
      }
    }
  }
  if (!last_pos_diff)
    return next_diff;
  return next_diff*last_pow_diff / last_pos_diff;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_last_x_block_height(bool pos) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  uint64_t sz = m_db_blocks.size();
  if (!sz)
    return 0;

  uint64_t i = sz-1;

  for (; i < sz; i--)
  {
    if (is_pos_block(m_db_blocks[i]->bl) == pos)
      return i;
  }
  return 0;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_last_alt_x_block_cumulative_precise_adj_difficulty(const alt_chain_type& alt_chain, uint64_t block_height, bool pos) const
{
  wide_difficulty_type res = 0;
  get_last_alt_x_block_cumulative_precise_difficulty(alt_chain, block_height, pos, res);
  return res;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_last_alt_x_block_cumulative_precise_difficulty(const alt_chain_type& alt_chain, uint64_t block_height, bool pos, wide_difficulty_type& cumulative_diff_precise_adj) const
{
  uint64_t main_chain_first_block = block_height;
  for (auto it = alt_chain.rbegin(); it != alt_chain.rend(); it++)
  {
    if (is_pos_block((*it)->second.bl) == pos)
    {
      cumulative_diff_precise_adj = (*it)->second.cumulative_diff_precise_adjusted;
      return (*it)->second.cumulative_diff_precise;
    }
    main_chain_first_block = (*it)->second.height - 1;
  }


  CRITICAL_REGION_LOCAL(m_read_lock);
  CHECK_AND_ASSERT_MES(main_chain_first_block < m_db_blocks.size(), false, "Intrnal error: main_chain_first_block(" << main_chain_first_block << ") < m_blocks.size() (" << m_db_blocks.size() << ")");

  for (uint64_t i = main_chain_first_block; i != 0; i--)
  {
    if (is_pos_block(m_db_blocks[i]->bl) == pos)
    {
      cumulative_diff_precise_adj = m_db_blocks[i]->cumulative_diff_precise_adjusted;
      return m_db_blocks[i]->cumulative_diff_precise;
    }
  }
  cumulative_diff_precise_adj = 0;
  return 0;
}
//------------------------------------------------------------------
bool get_tx_from_cache(const crypto::hash& tx_id, transactions_map& tx_cache, transaction& tx, size_t& blob_size, uint64_t& fee)
{
  auto it = tx_cache.find(tx_id);
  if (it == tx_cache.end())
    return false;

  tx = it->second;
  blob_size = get_object_blobsize(tx);
  fee = get_tx_fee(tx);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::handle_block_to_main_chain(const block& bl, const crypto::hash& id, block_verification_context& bvc)
{
  TIME_MEASURE_START_PD_MS(block_processing_time_0_ms);
  CRITICAL_REGION_LOCAL(m_read_lock);
  TIME_MEASURE_START_PD(block_processing_time_1);
  if(bl.prev_id != get_top_block_id())
  {
    LOG_PRINT_L0("Block with id: " << id << ENDL
      << "have wrong prev_id: " << bl.prev_id << ENDL
      << "expected: " << get_top_block_id());
    return false;
  }

  if(!check_block_timestamp_main(bl))
  {
    LOG_PRINT_L0("Block with id: " << id << ENDL
      << "have invalid timestamp: " << bl.timestamp);
    //add_block_as_invalid(bl, id);//do not add blocks to invalid storage befor proof of work check was passed
    bvc.m_verification_failed = true;
    return false;
  }

  if (m_checkpoints.is_in_checkpoint_zone(get_current_blockchain_size()))
  {
    m_is_in_checkpoint_zone = true;
    if (!m_checkpoints.check_block(get_current_blockchain_size(), id))
    {
      LOG_ERROR("CHECKPOINT VALIDATION FAILED");
      bvc.m_verification_failed = true;
      return false;
    }
  }
  else
    m_is_in_checkpoint_zone = false;

  crypto::hash proof_hash = null_hash;
  uint64_t pos_coinstake_amount = 0; 
  wide_difficulty_type this_coin_diff = 0;
  bool is_pos_bl = is_pos_block(bl);
  //check if PoS allowed in this height
  CHECK_AND_ASSERT_MES_CUSTOM(!(is_pos_bl && m_db_blocks.size() < m_core_runtime_config.pos_minimum_heigh), false, bvc.m_verification_failed = true, "PoS block not allowed on height " << m_db_blocks.size());

  //check proof of work
  TIME_MEASURE_START_PD(target_calculating_time_2);
  wide_difficulty_type current_diffic = get_next_diff_conditional(is_pos_bl);
  CHECK_AND_ASSERT_MES_CUSTOM(current_diffic, false, bvc.m_verification_failed = true, "!!!!!!!!! difficulty overhead !!!!!!!!!");
 TIME_MEASURE_FINISH_PD(target_calculating_time_2);

 TIME_MEASURE_START_PD(longhash_calculating_time_3);
  if (is_pos_bl)
  {
    bool r = validate_pos_block(bl, current_diffic, pos_coinstake_amount, this_coin_diff, proof_hash, id, false);
     CHECK_AND_ASSERT_MES_CUSTOM(r, false, bvc.m_verification_failed = true, "validate_pos_block failed!!");
  }
  else
  {

    proof_hash = get_block_longhash(bl);

    if (!check_hash(proof_hash, current_diffic))
    {
      LOG_ERROR("Block with id: " << id << ENDL
        << "PoW hash: " << proof_hash << ENDL 
        << "nonce: " << bl.nonce << ENDL
        << "header_mining_hash: " << get_block_header_mining_hash(bl) << ENDL        
        << "expected difficulty: " << current_diffic);
      bvc.m_verification_failed = true;
      return false;
    }
  }
 TIME_MEASURE_FINISH_PD(longhash_calculating_time_3);

  size_t aliases_count_befor_block = m_db_aliases.size();

  if (!prevalidate_miner_transaction(bl, m_db_blocks.size(), is_pos_bl))
  {
    LOG_PRINT_L0("Block with id: " << id
      << " failed to pass prevalidation");
    bvc.m_verification_failed = true;
    return false;
  }
  size_t cumulative_block_size = 0;
  size_t coinbase_blob_size = get_object_blobsize(bl.miner_tx);

  /* 
      instead of complicated two-phase template construction and adjustment of cumulative size with block reward we
      use CURRENCY_COINBASE_BLOB_RESERVED_SIZE as penalty-free coinbase transaction reservation.
  */
  if (coinbase_blob_size > CURRENCY_COINBASE_BLOB_RESERVED_SIZE)
  {
    cumulative_block_size += coinbase_blob_size;
    LOG_PRINT_CYAN("Big coinbase transaction detected: coinbase_blob_size = " << coinbase_blob_size, LOG_LEVEL_0);
  }
  
  std::vector<uint64_t> block_fees;
  block_fees.reserve(bl.tx_hashes.size());
  //process transactions
  TIME_MEASURE_START_PD(all_txs_insert_time_5);
  if (!add_transaction_from_block(bl.miner_tx, get_transaction_hash(bl.miner_tx), id, get_current_blockchain_size(), get_block_datetime(bl)))
  {
    LOG_PRINT_L0("Block with id: " << id << " failed to add transaction to blockchain storage");
    bvc.m_verification_failed = true;
    return false;
  }
  //preserve extra data to support alt pos blocks validation
  //amount = > gindex_increment; a map to store how many txout_to_key outputs with such amount this block has in its transactions(including miner tx)
  std::unordered_map<uint64_t, uint32_t> gindices;
  append_per_block_increments_for_tx(bl.miner_tx, gindices);



  size_t tx_processed_count = 0;
  uint64_t fee_summary = 0;
  uint64_t burned_coins = 0;
  std::list<crypto::key_image> block_summary_kimages;

  for(const crypto::hash& tx_id : bl.tx_hashes)
  {
    transaction tx;
    size_t blob_size = 0;
    uint64_t fee = 0;

    bool taken_from_cache = get_tx_from_cache(tx_id, bvc.m_onboard_transactions, tx, blob_size, fee);
    bool taken_from_pool = m_tx_pool.take_tx(tx_id, tx, blob_size, fee);
    if(!taken_from_cache && !taken_from_pool)
    {
      LOG_PRINT_L0("Block with id: " << id  << " has at least one unknown transaction with id: " << tx_id);
      purge_block_data_from_blockchain(bl, tx_processed_count);
      //add_block_as_invalid(bl, id);
      bvc.m_verification_failed = true;
      return false;
    }

    if (!validate_tx_semantic(tx, blob_size))
    {
      LOG_PRINT_L0("Block with id: " << id << " has at least one transaction with wrong semantic, tx_id: " << tx_id);
      purge_block_data_from_blockchain(bl, tx_processed_count);
      //add_block_as_invalid(bl, id);  
      bvc.m_verification_failed = true;
      return false;
    }

    append_per_block_increments_for_tx(tx, gindices);
    
    //If we under checkpoints, ring signatures should be pruned    
    if(m_is_in_checkpoint_zone)
    {
      tx.signatures.clear();
      tx.attachment.clear();
    }
    TIME_MEASURE_START_PD(tx_add_one_tx_time);
    TIME_MEASURE_START_PD(tx_check_inputs_time);
    if(!check_tx_inputs(tx, tx_id))
    {
      LOG_PRINT_L0("Block with id: " << id << " has at least one transaction (id: " << tx_id << ") with wrong inputs.");
      currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      if (taken_from_pool)
      {
        bool add_res = m_tx_pool.add_tx(tx, tvc, true, true);
        m_tx_pool.add_transaction_to_black_list(tx);
        CHECK_AND_ASSERT_MES_NO_RET(add_res, "handle_block_to_main_chain: failed to add transaction back to transaction pool");
      }
      purge_block_data_from_blockchain(bl, tx_processed_count);
      add_block_as_invalid(bl, id);
      LOG_PRINT_L0("Block with id " << id << " added as invalid because of wrong inputs in transactions");
      bvc.m_verification_failed = true;
      return false;
    }
    TIME_MEASURE_FINISH_PD(tx_check_inputs_time);
    burned_coins += get_burned_amount(tx);

    TIME_MEASURE_START_PD(tx_prapare_append);
    uint64_t current_bc_size = get_current_blockchain_size();
    uint64_t actual_timestamp = get_block_datetime(bl);
    TIME_MEASURE_FINISH_PD(tx_prapare_append);
    TIME_MEASURE_START_PD(tx_append_time);
    if(!add_transaction_from_block(tx, tx_id, id, current_bc_size, actual_timestamp))
    {
       LOG_PRINT_L0("Block " << id << " contains tx " << tx_id << " that can't be added to the blockchain storage");
       if (taken_from_pool)
       {
         currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
         bool add_res = m_tx_pool.add_tx(tx, tvc, true, true);
         m_tx_pool.add_transaction_to_black_list(tx);
         CHECK_AND_ASSERT_MES_NO_RET(add_res, "handle_block_to_main_chain: failed to add transaction back to transaction pool");
       }
       purge_block_data_from_blockchain(bl, tx_processed_count);
       bvc.m_verification_failed = true;
       return false;
    }
    TIME_MEASURE_FINISH_PD(tx_append_time);
    //LOG_PRINT_L0("APPEND_TX_TIME: " << m_performance_data.tx_append_time.get_last_val());
    TIME_MEASURE_FINISH_PD(tx_add_one_tx_time);
    fee_summary += fee;
    cumulative_block_size += blob_size;
    ++tx_processed_count;
    if (fee)
      block_fees.push_back(fee);

    read_keyimages_from_tx(tx, block_summary_kimages);
  }
  TIME_MEASURE_FINISH_PD(all_txs_insert_time_5);

  TIME_MEASURE_START_PD(etc_stuff_6);
  //check aliases count
  if (m_db_aliases.size() - aliases_count_befor_block > MAX_ALIAS_PER_BLOCK)
  {
    LOG_PRINT_L0("Block with id: " << id
      << " have registered too many aliases " << m_db_aliases.size() - aliases_count_befor_block << ", expected no more than " << MAX_ALIAS_PER_BLOCK);
    purge_block_data_from_blockchain(bl, tx_processed_count);
    bvc.m_verification_failed = true;
    return false;
  }

  uint64_t base_reward = 0;
  boost::multiprecision::uint128_t already_generated_coins = m_db_blocks.size() ? m_db_blocks.back()->already_generated_coins:0;
  if (!validate_miner_transaction(bl, cumulative_block_size, fee_summary, base_reward, already_generated_coins))
  {
    LOG_PRINT_L0("Block with id: " << id
      << " have wrong miner transaction");
    purge_block_data_from_blockchain(bl, tx_processed_count);
    bvc.m_verification_failed = true;
    return false;
  }

  //fill block_extended_info
  block_extended_info bei = boost::value_initialized<block_extended_info>();
  bei.bl = bl;
  bei.height = m_db_blocks.size();
  bei.block_cumulative_size = cumulative_block_size;
  bei.difficulty = current_diffic;
  if (is_pos_bl)
    bei.stake_hash = proof_hash;

  //////////////////////////////////////////////////////////////////////////

  //old style cumulative difficulty collecting

  //precise difficulty - difficulty used to calculate next difficulty
  uint64_t last_x_h = get_last_x_block_height(is_pos_bl);
  if (!last_x_h)
    bei.cumulative_diff_precise = current_diffic;
  else
    bei.cumulative_diff_precise = m_db_blocks[last_x_h]->cumulative_diff_precise + current_diffic;

  if (m_db_blocks.size())
  {
    bei.cumulative_diff_adjusted = m_db_blocks.back()->cumulative_diff_adjusted;
  }

  //adjusted difficulty - difficulty used to switch blockchain
  wide_difficulty_type cumulative_diff_delta = 0;
  if (is_pos_bl)
    cumulative_diff_delta = get_adjusted_cumulative_difficulty_for_next_pos(current_diffic);
  else
    cumulative_diff_delta = current_diffic;


  size_t sequence_factor = get_current_sequence_factor(is_pos_bl);
  if (bei.height >= m_core_runtime_config.pos_minimum_heigh)
    cumulative_diff_delta = correct_difficulty_with_sequence_factor(sequence_factor, cumulative_diff_delta);

  if (bei.height > BLOCKCHAIN_HEIGHT_FOR_POS_STRICT_SEQUENCE_LIMITATION && is_pos_bl && sequence_factor > BLOCK_POS_STRICT_SEQUENCE_LIMIT)
  {
    LOG_PRINT_RED_L0("Block " << id << " @ " << bei.height << " has too big sequence factor: " << sequence_factor << ", rejected");
    purge_block_data_from_blockchain(bl, tx_processed_count);
    bvc.m_verification_failed = true;
    return false;
  }

  bei.cumulative_diff_adjusted += cumulative_diff_delta;

  //////////////////////////////////////////////////////////////////////////
  // rebuild cumulative_diff_precise_adjusted for whole period 
  wide_difficulty_type diff_precise_adj = correct_difficulty_with_sequence_factor(sequence_factor, current_diffic);
  bei.cumulative_diff_precise_adjusted = last_x_h ? m_db_blocks[last_x_h]->cumulative_diff_precise_adjusted + diff_precise_adj : diff_precise_adj;

  //////////////////////////////////////////////////////////////////////////

  //etc 
  if (already_generated_coins < burned_coins)
  {
    LOG_ERROR("Condition failed: already_generated_coins(" << already_generated_coins << ") >= burned_coins(" << burned_coins << ")");
    purge_block_data_from_blockchain(bl, tx_processed_count);
    bvc.m_verification_failed = true;
    return false;
  }
  bei.already_generated_coins = already_generated_coins - burned_coins + base_reward;

  auto blocks_index_ptr = m_db_blocks_index.get(id);
  if (blocks_index_ptr)
  {
    LOG_ERROR("block with id: " << id << " already in block indexes");
    purge_block_data_from_blockchain(bl, tx_processed_count);
    bvc.m_verification_failed = true;
    return false;
  }
  if (bei.height%ALIAS_MEDIAN_RECALC_INTERWAL)
  {
    bei.effective_tx_fee_median = get_tx_fee_median();
  }
  else
  {
    if (bei.height == 0)
      bei.effective_tx_fee_median = 0;
    else 
    {
      LOG_PRINT_L0("Recalculating median fee...");
      std::vector<uint64_t> blocks_medians;
      blocks_medians.reserve(ALIAS_COAST_PERIOD);
      for (uint64_t i = bei.height - 1; i != 0 && ALIAS_COAST_PERIOD >= bei.height - i ; i--)
      {
        uint64_t i_median = m_db_blocks[i]->this_block_tx_fee_median;
        if (i_median)
          blocks_medians.push_back(i_median);
      }
      bei.effective_tx_fee_median = epee::misc_utils::median(blocks_medians);
      LOG_PRINT_L0("Median fee recalculated for h = " << bei.height << " as " << print_money(bei.effective_tx_fee_median));
    }
  }
  if (block_fees.size())
  {
    uint64_t block_fee_median = epee::misc_utils::median(block_fees);
    bei.this_block_tx_fee_median = block_fee_median;
  }

  m_db_blocks_index.set(id, bei.height);
  push_block_to_per_block_increments(bei.height, gindices);
  TIME_MEASURE_FINISH_PD(etc_stuff_6);


  TIME_MEASURE_START_PD(insert_time_4);
  m_db_blocks.push_back(bei);
  TIME_MEASURE_FINISH_PD(insert_time_4);
  TIME_MEASURE_FINISH_PD(block_processing_time_1);
  TIME_MEASURE_FINISH_PD_MS(block_processing_time_0_ms);

  //print result
  stringstream powpos_str_entry, timestamp_str_entry;
  if (is_pos_bl)
  { // PoS
    int64_t actual_ts = get_block_datetime(bei.bl); // signed int is intentionally used here
    int64_t ts_diff = actual_ts - m_core_runtime_config.get_core_time();
    powpos_str_entry << "PoS:\t" << proof_hash << ", stake amount: " << print_money_brief(pos_coinstake_amount) << ", final_difficulty: " << this_coin_diff;
    timestamp_str_entry << ", actual ts: " << actual_ts << " (diff: " << std::showpos << ts_diff << "s) block ts: " << std::noshowpos << bei.bl.timestamp << " (shift: " << std::showpos << static_cast<int64_t>(bei.bl.timestamp) - actual_ts << ")";
  }
  else
  { // PoW
    int64_t ts_diff = bei.bl.timestamp - static_cast<int64_t>(m_core_runtime_config.get_core_time());
    powpos_str_entry << "PoW:\t" << proof_hash;
    timestamp_str_entry << ", block ts: " << bei.bl.timestamp << " (diff: " << std::showpos << ts_diff << "s)";
  }
  //explanation of this code will be provided later with public announce
  set_lost_tx_unmixable_for_height(bei.height);
    

  LOG_PRINT_L1("+++++ BLOCK SUCCESSFULLY ADDED " << (is_pos_bl ? "[PoS]" : "[PoW]") << "["<< static_cast<uint64_t>(bei.bl.major_version) << "." << static_cast<uint64_t>(bei.bl.minor_version) << "] "<<  " Sq: " << sequence_factor
    << ENDL << "id:\t" << id << timestamp_str_entry.str()
    << ENDL << powpos_str_entry.str()
    << ENDL << "HEIGHT " << bei.height << ", difficulty: " << current_diffic << ", cumul_diff_precise: " << bei.cumulative_diff_precise << ", cumul_diff_adj: " << bei.cumulative_diff_adjusted << " (+" << cumulative_diff_delta << ")"
    << ENDL << "block reward: " << print_money_brief(base_reward + fee_summary) << " (" << print_money_brief(base_reward) << " + " << print_money_brief(fee_summary) 
    << ")" << ", coinbase_blob_size: " << coinbase_blob_size << ", cumulative size: " << cumulative_block_size << ", tx_count: " << bei.bl.tx_hashes.size()
    << ", timing: " << block_processing_time_0_ms <<  "ms" 
    << "(micrsec:" << block_processing_time_1 
    << "(" << target_calculating_time_2 << "(" << m_performance_data.target_calculating_enum_blocks.get_last_val() << "/" << m_performance_data.target_calculating_calc.get_last_val() << ")"
    << "/" << longhash_calculating_time_3 
    << "/" << insert_time_4 
    << "/" << all_txs_insert_time_5
    << "/" << etc_stuff_6    
    << "))");

  on_block_added(bei, id, block_summary_kimages);

  bvc.m_added_to_main_chain = true;
  return true;
}
//------------------------------------------------------------------
void blockchain_storage::on_block_added(const block_extended_info& bei, const crypto::hash& id, const std::list<crypto::key_image>& bsk)
{
  update_next_comulative_size_limit();
  m_timestamps_median_cache.clear();
  m_tx_pool.on_blockchain_inc(bei.height, id, bsk);

  update_targetdata_cache_on_block_added(bei);

  TIME_MEASURE_START_PD(raise_block_core_event);
  rise_core_event(CORE_EVENT_BLOCK_ADDED, void_struct());
  TIME_MEASURE_FINISH_PD(raise_block_core_event);

}
//------------------------------------------------------------------
void blockchain_storage::on_block_removed(const block_extended_info& bei)
{
  m_tx_pool.on_blockchain_dec(m_db_blocks.size() - 1, get_top_block_id());
  m_timestamps_median_cache.clear();
  update_targetdata_cache_on_block_removed(bei);
  LOG_PRINT_L2("block at height " << bei.height << " was removed from the blockchain");
}
//------------------------------------------------------------------
void blockchain_storage::update_targetdata_cache_on_block_added(const block_extended_info& bei)
{
  CRITICAL_REGION_LOCAL(m_targetdata_cache_lock);
  if (bei.height == 0)
    return; //skip genesis
  std::list<std::pair<wide_difficulty_type, uint64_t>>& targetdata_cache = is_pos_block(bei.bl) ? m_pos_targetdata_cache : m_pow_targetdata_cache;
  targetdata_cache.push_back(std::pair<wide_difficulty_type, uint64_t>(bei.cumulative_diff_precise, bei.bl.timestamp));
  while (targetdata_cache.size() > TARGETDATA_CACHE_SIZE)
    targetdata_cache.pop_front();
}
//------------------------------------------------------------------
void blockchain_storage::update_targetdata_cache_on_block_removed(const block_extended_info& bei)
{
  CRITICAL_REGION_LOCAL(m_targetdata_cache_lock);
  std::list<std::pair<wide_difficulty_type, uint64_t>>& targetdata_cache = is_pos_block(bei.bl) ? m_pos_targetdata_cache : m_pow_targetdata_cache;
  if (targetdata_cache.size())
    targetdata_cache.pop_back();
  if (targetdata_cache.size() < DIFFICULTY_WINDOW)
    targetdata_cache.clear();
}
//------------------------------------------------------------------
void blockchain_storage::load_targetdata_cache(bool is_pos)const
{
  CRITICAL_REGION_LOCAL(m_targetdata_cache_lock);
  std::list<std::pair<wide_difficulty_type, uint64_t>>& targetdata_cache = is_pos? m_pos_targetdata_cache: m_pow_targetdata_cache;
  targetdata_cache.clear();
  uint64_t stop_ind = 0;
  uint64_t blocks_size = m_db_blocks.size();
  size_t count = 0;
  for (uint64_t cur_ind = blocks_size - 1; cur_ind != stop_ind && count < DIFFICULTY_WINDOW + 5; cur_ind--)
  {
    auto beiptr = m_db_blocks[cur_ind];

    bool is_pos_bl = is_pos_block(beiptr->bl);
    if (is_pos != is_pos_bl)
      continue;
    targetdata_cache.push_front(std::pair<wide_difficulty_type, uint64_t>(beiptr->cumulative_diff_precise, beiptr->bl.timestamp));
    ++count;
  }
}
//------------------------------------------------------------------
void blockchain_storage::on_abort_transaction()
{
  if (m_event_handler) m_event_handler->on_clear_events();
  CHECK_AND_ASSERT_MES_NO_RET(validate_blockchain_prev_links(), "EPIC FAIL! 4");
  m_timestamps_median_cache.clear();
}
//------------------------------------------------------------------
bool blockchain_storage::update_next_comulative_size_limit()
{
  std::vector<size_t> sz;
  get_last_n_blocks_sizes(sz, CURRENCY_REWARD_BLOCKS_WINDOW);

  uint64_t median = misc_utils::median(sz);
  if(median <= CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE)
    median = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE;

  m_db_current_block_cumul_sz_limit = median * 2;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::is_after_hardfork_1_zone()const
{
  return is_after_hardfork_1_zone(m_db_blocks.size());
}
//------------------------------------------------------------------
bool blockchain_storage::is_after_hardfork_1_zone(uint64_t height)const
{
  if (height > m_core_runtime_config.hard_fork_01_starts_after_height)
    return true;
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::is_after_hardfork_2_zone()const
{
  return is_after_hardfork_2_zone(m_db_blocks.size());
}
//------------------------------------------------------------------
bool blockchain_storage::is_after_hardfork_3_zone()const
{
  return is_after_hardfork_3_zone(m_db_blocks.size());
}
//------------------------------------------------------------------
bool blockchain_storage::is_after_hardfork_2_zone(uint64_t height)const
{
  if (height > m_core_runtime_config.hard_fork_02_starts_after_height)
    return true;
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::is_after_hardfork_3_zone(uint64_t height)const
{
  if (height > m_core_runtime_config.hard_fork_03_starts_after_height)
    return true;
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::prevalidate_block(const block& bl)
{
  //before hard_fork1
  if (bl.major_version == BLOCK_MAJOR_VERSION_INITIAL && get_block_height(bl) <= m_core_runtime_config.hard_fork_01_starts_after_height)
    return true;


  //after hard_fork1 and before hard_fork3
  if ( get_block_height(bl) > m_core_runtime_config.hard_fork_01_starts_after_height  &&
       get_block_height(bl) <= m_core_runtime_config.hard_fork_03_starts_after_height
    )
  {
    if (bl.major_version <= HF1_BLOCK_MAJOR_VERSION )
      return true;
    else
      return false;
  }

  //after hard_fork3
  if (bl.major_version > CURRENT_BLOCK_MAJOR_VERSION)
  {
    LOG_ERROR("prevalidation failed for block " << get_block_hash(bl) << ": major block version " << static_cast<size_t>(bl.major_version) << " is incorrect, " << CURRENT_BLOCK_MAJOR_VERSION << " is expected" << ENDL
      << obj_to_json_str(bl));
    return false;
  }

  if (bl.minor_version > CURRENT_BLOCK_MINOR_VERSION)
  {
    //this means that binary block is compatible, but semantics got changed due to hardfork, daemon should be updated
    LOG_PRINT_MAGENTA("Block's MINOR_VERSION is: " << bl.minor_version 
      << ", while current build supports not bigger then " <<  CURRENT_BLOCK_MINOR_VERSION 
      << ", please make sure you using latest version.", LOG_LEVEL_0
    );
    return false;
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::add_new_block(const block& bl, block_verification_context& bvc)
{
  try
  {
    m_db.begin_transaction();

    //block bl = bl_;
    crypto::hash id = get_block_hash(bl);
    CRITICAL_REGION_LOCAL(m_tx_pool);
    //CRITICAL_REGION_LOCAL1(m_read_lock);
    if (have_block(id))
    {
      LOG_PRINT_L3("block with id = " << id << " already exists");
      bvc.m_already_exists = true;
      m_db.commit_transaction();
      CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL! 1");
      return false;
    }

    if (!prevalidate_block(bl))
    {
      LOG_PRINT_RED_L0("block with id = " << id << " failed to prevalidate");
      bvc.m_added_to_main_chain = false;
      bvc.m_verification_failed = true;
      m_db.commit_transaction();
      return false;
    }


    //check that block refers to chain tail

    
    if (!(bl.prev_id == get_top_block_id()))
    {
      //chain switching or wrong block
      bvc.m_added_to_main_chain = false;
      bool r = handle_alternative_block(bl, id, bvc);
      if (!r || bvc.m_verification_failed)
      {
        m_db.abort_transaction();
        CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL! 2.2");
        m_tx_pool.on_finalize_db_transaction();
        return r;
      }
      m_db.commit_transaction();
      CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL! 2");
      m_tx_pool.on_finalize_db_transaction();
      return r;
      //never relay alternative blocks
    }


    bool res = handle_block_to_main_chain(bl, id, bvc);
    if (bvc.m_verification_failed || !res)
    {
      m_db.abort_transaction();
      m_tx_pool.on_finalize_db_transaction();
      on_abort_transaction();
      if (m_event_handler) m_event_handler->on_clear_events();
      return res;
    }
    m_db.commit_transaction();
    CHECK_AND_ASSERT_MES(validate_blockchain_prev_links(), false, "EPIC FAIL! 3");
    m_tx_pool.on_finalize_db_transaction();
    if (m_event_handler) m_event_handler->on_complete_events();

    return res;
  }  
  catch (const std::exception& ex)
  {
    bvc.m_verification_failed = true;
    bvc.m_added_to_main_chain = false;
    m_db.abort_transaction();
    m_tx_pool.on_finalize_db_transaction();
    on_abort_transaction();
    LOG_ERROR("UNKNOWN EXCEPTION WHILE ADDINIG NEW BLOCK: " << ex.what());

    return false;
  }
  catch (...)
  {
    bvc.m_verification_failed = true;
    bvc.m_added_to_main_chain = false;
    m_db.abort_transaction();
    m_tx_pool.on_finalize_db_transaction();
    on_abort_transaction();
    LOG_ERROR("UNKNOWN EXCEPTION WHILE ADDINIG NEW BLOCK.");
    return false;
  }
}
//------------------------------------------------------------------
bool blockchain_storage::truncate_blockchain(uint64_t to_height)
{
  m_db.begin_transaction();
  uint64_t inital_height = get_current_blockchain_size();
  while (get_current_blockchain_size() > to_height)
  {
    transactions_map ot;
    pop_block_from_blockchain(ot);
  }
  CRITICAL_REGION_LOCAL(m_alternative_chains_lock);
  m_alternative_chains.clear();
  m_altblocks_keyimages.clear();
  m_alternative_chains_txs.clear();
  LOG_PRINT_MAGENTA("Blockchain truncated from " << inital_height << " to " << get_current_blockchain_size(), LOG_LEVEL_0);
  m_db.commit_transaction();
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::calc_tx_cummulative_blob(const block& bl)const 
{
  uint64_t cummulative_size_pool = 0;
  uint64_t cummulative_size_calc = 0;
  uint64_t cummulative_size_serialized = 0;
  uint64_t i = 0;
  std::stringstream ss;
  uint64_t calculated_sz_miner = get_object_blobsize(bl.miner_tx);
  blobdata b = t_serializable_object_to_blob(bl.miner_tx);
  uint64_t serialized_size_miner = b.size();

  ss << "[COINBASE]: " << calculated_sz_miner << "|" << serialized_size_miner << ENDL;

  for (auto& h : bl.tx_hashes)
  {
    uint64_t calculated_sz = 0;
    uint64_t serialized_size = 0;
    tx_memory_pool::tx_details td = AUTO_VAL_INIT(td);
    bool in_pool = m_tx_pool.get_transaction(h, td);
    if (in_pool)
    {
      calculated_sz = get_object_blobsize(td.tx);
      blobdata b = t_serializable_object_to_blob(td.tx);
      serialized_size = b.size();
      if (td.blob_size != calculated_sz || calculated_sz != serialized_size)
      {
        LOG_PRINT_RED("BLOB SIZE MISMATCH IN TX: "  << h << " calculated_sz: " << calculated_sz
          << " serialized_size: " << serialized_size
          << " td.blob_size: " << td.blob_size, LOG_LEVEL_0);
      }
      cummulative_size_pool += td.blob_size;
    }
    else
    {
      auto tx_ptr = m_db_transactions.get(h);
      CHECK_AND_ASSERT_MES(tx_ptr, false, "tx " << h << " not found in blockchain nor tx_pool");
      calculated_sz = get_object_blobsize(tx_ptr->tx);
      blobdata b = t_serializable_object_to_blob(tx_ptr->tx);
      serialized_size = b.size();
      if (calculated_sz != serialized_size)
      {
        LOG_PRINT_RED("BLOB SIZE MISMATCH IN TX: " << h << " calculated_sz: " << calculated_sz
         << " serialized_size: " << serialized_size, LOG_LEVEL_0);
      }     
    }
    cummulative_size_calc += calculated_sz;
    cummulative_size_serialized += serialized_size;
    ss << "[" << i << "]" << h << ": " << calculated_sz << ENDL;
    i++;

  }
  LOG_PRINT_MAGENTA("CUMMULATIVE_BLOCK_SIZE_TRACE: " << get_block_hash(bl) << ENDL
    << "cummulative_size_calc: " << cummulative_size_calc << ENDL 
    << "cummulative_size_serialized: " << cummulative_size_serialized << ENDL
    << "cummulative_size_pool: " << cummulative_size_pool << ENDL
    << ss.str(), LOG_LEVEL_0);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::build_kernel(const block& bl, stake_kernel& kernel, uint64_t& amount, const stake_modifier_type& stake_modifier) const
{
  CHECK_AND_ASSERT_MES(bl.miner_tx.vin.size() == 2, false, "wrong miner transaction");
  CHECK_AND_ASSERT_MES(bl.miner_tx.vin[0].type() == typeid(txin_gen), false, "wrong miner transaction");
  CHECK_AND_ASSERT_MES(bl.miner_tx.vin[1].type() == typeid(txin_to_key), false, "wrong miner transaction");

  const txin_to_key& txin = boost::get<txin_to_key>(bl.miner_tx.vin[1]);
  CHECK_AND_ASSERT_MES(txin.key_offsets.size(), false, "wrong miner transaction");
  amount = txin.amount;

  return build_kernel(txin.amount, txin.k_image, kernel, stake_modifier, bl.timestamp);
}
//------------------------------------------------------------------
bool blockchain_storage::build_stake_modifier(stake_modifier_type& sm, const alt_chain_type& alt_chain, uint64_t split_height, crypto::hash *p_last_block_hash /* = nullptr */) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  sm = stake_modifier_type();

  auto pbei_last_pos = get_last_block_of_type(true, alt_chain, split_height);
  auto pbei_last_pow = get_last_block_of_type(false, alt_chain, split_height);
  CHECK_AND_ASSERT_THROW_MES(pbei_last_pow, "Internal error: pbei_last_pow is null");

  if (pbei_last_pos)
    sm.last_pos_kernel_id = pbei_last_pos->stake_hash;
  else
  {
    bool r = string_tools::parse_tpod_from_hex_string(POS_STARTER_KERNEL_HASH, sm.last_pos_kernel_id);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse POS_STARTER_KERNEL_HASH");
  }

  sm.last_pow_id = get_block_hash(pbei_last_pow->bl);

  if (p_last_block_hash != nullptr)
    *p_last_block_hash = get_block_hash(m_db_blocks.back()->bl);

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::build_kernel(uint64_t amount,
                                      const crypto::key_image& ki, 
                                      stake_kernel& kernel, 
                                      const stake_modifier_type& stake_modifier, 
                                      uint64_t timestamp)const 
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  kernel = stake_kernel();
  kernel.kimage = ki;
  kernel.stake_modifier = stake_modifier;
  kernel.block_timestamp = timestamp;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::scan_pos(const COMMAND_RPC_SCAN_POS::request& sp, COMMAND_RPC_SCAN_POS::response& rsp) const
{
  uint64_t timstamp_start = 0;
  wide_difficulty_type basic_diff = 0;
  CRITICAL_REGION_BEGIN(m_read_lock);
  timstamp_start = m_db_blocks.back()->bl.timestamp;
  basic_diff = get_next_diff_conditional(true);
  CRITICAL_REGION_END();

  stake_modifier_type sm = AUTO_VAL_INIT(sm);
  bool r = build_stake_modifier(sm);
  CHECK_AND_ASSERT_MES(r, false, "failed to build_stake_modifier");

  for (size_t i = 0; i != sp.pos_entries.size(); i++)
  {
    stake_kernel sk = AUTO_VAL_INIT(sk);
    build_kernel(sp.pos_entries[i].amount, sp.pos_entries[i].keyimage, sk, sm, 0);

    for (uint64_t ts = timstamp_start; ts < timstamp_start + POS_SCAN_WINDOW; ts++)
    {
      sk.block_timestamp = ts;
      crypto::hash kernel_hash = crypto::cn_fast_hash(&sk, sizeof(sk));
      wide_difficulty_type this_coin_diff = basic_diff / sp.pos_entries[i].amount;
      if (!check_hash(kernel_hash, this_coin_diff))
        continue;
      else
      {
        //found kernel
        LOG_PRINT_GREEN("Found kernel: amount=" << print_money(sp.pos_entries[i].amount) << ", key_image" << sp.pos_entries[i].keyimage, LOG_LEVEL_0);
        rsp.index = i;
        rsp.block_timestamp = ts;
        rsp.status = API_RETURN_CODE_OK;
        return true;
      }
    }
  }
  rsp.status = API_RETURN_CODE_NOT_FOUND;
  return false;
}
//------------------------------------------------------------------
void blockchain_storage::set_core_runtime_config(const core_runtime_config& pc) const
{
  m_core_runtime_config = pc;
  m_services_mgr.set_core_runtime_config(pc);
}
//------------------------------------------------------------------
const core_runtime_config& blockchain_storage::get_core_runtime_config() const
{
  return m_core_runtime_config;
}
//------------------------------------------------------------------
std::shared_ptr<const transaction_chain_entry> blockchain_storage::get_tx_chain_entry(const crypto::hash& tx_hash) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  return m_db_transactions.find(tx_hash);
}
//------------------------------------------------------------------
bool blockchain_storage::get_tx_chain_entry(const crypto::hash& tx_hash, transaction_chain_entry& entry) const
{
  auto ch_entry_ptr = get_tx_chain_entry(tx_hash);
  if (!ch_entry_ptr)
    return false;
  entry = *ch_entry_ptr;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_blockchain_prev_links(size_t last_n_blocks_to_check /* = 10 */) const
{
  CRITICAL_REGION_LOCAL(m_read_lock);

  TRY_ENTRY()

  if (m_db_blocks.size() < 2 || last_n_blocks_to_check < 1)
    return true;

  bool ok = true;
  for(size_t attempt_counter = 1; attempt_counter <= 2; ++attempt_counter)
  {
    ok = true;
    for (size_t height = m_db_blocks.size() - 1, blocks_to_check = last_n_blocks_to_check; height != 0 && blocks_to_check != 0; --height, --blocks_to_check)
    {
      auto bei_ptr = m_db_blocks[height];
      auto& bei = *bei_ptr;
      if (bei.height != height)
      {
        LOG_ERROR("bei.height = " << bei.height << ", expected: " << height);
        ok = false;
      }

      auto prev_bei_ptr = m_db_blocks[height - 1];
      auto& prev_bei = *prev_bei_ptr;
      crypto::hash prev_id = get_block_hash(prev_bei.bl);
      if (bei.bl.prev_id != prev_id)
      {
        LOG_ERROR("EPIC FAIL: Block " << get_block_hash(bei.bl) << " @ " << height << " has prev_id == " << bei.bl.prev_id <<
        " while block @ " << height - 1 << " has id: " << prev_id << ENDL <<
        "Block @" << height << ":" << ENDL << currency::obj_to_json_str(bei.bl) << ENDL <<
        "Block @" << height - 1 << ":" << ENDL << currency::obj_to_json_str(prev_bei.bl));
        m_performance_data.epic_failure_happend = true;
        ok = false;
      }
    }

    if (ok && attempt_counter == 1)
    {
      break;
    }
    else if (!ok && attempt_counter == 1)
    {
      LOG_PRINT_YELLOW("************* EPIC FAIL workaround attempt: try to reset DB cache and re-check *************", LOG_LEVEL_0);
      reset_db_cache();
      continue;
    }
    else if (!ok && attempt_counter == 2)
    {
      LOG_PRINT_RED("************* EPIC FAIL workaround attempt failed *************", LOG_LEVEL_0);
      break;
    }
    else if (ok && attempt_counter == 2)
    {
      LOG_PRINT_GREEN("************* EPIC FAIL workaround attempt succeded! *************", LOG_LEVEL_0);
      break;
    }
    else
    {
      LOG_ERROR("should never get here");
      return false;
    }

    LOG_ERROR("should never get here also");
    return false;
  }

  return ok;

  CATCH_ENTRY2(false);
}
//------------------------------------------------------------------
void blockchain_storage::push_block_to_per_block_increments(uint64_t height_, std::unordered_map<uint64_t, uint32_t>& gindices)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  uint32_t height = static_cast<uint32_t>(height_);

  block_gindex_increments bgi = AUTO_VAL_INIT(bgi);
  bgi.increments.reserve(gindices.size());
  for (auto& v : gindices)
      bgi.increments.push_back(gindex_increment::construct(v.first, v.second));

  m_db_per_block_gindex_incs.set(height, bgi);
}
//------------------------------------------------------------------
void blockchain_storage::pop_block_from_per_block_increments(uint64_t height_)
{
  CRITICAL_REGION_LOCAL(m_read_lock);
  uint32_t height = static_cast<uint32_t>(height_);
  m_db_per_block_gindex_incs.erase(height);
}
//------------------------------------------------------------------
void blockchain_storage::calculate_local_gindex_lookup_table_for_height(uint64_t height, std::map<uint64_t, uint64_t>& gindexes) const
{
  gindexes.clear();

  CHECK_AND_ASSERT_THROW_MES(m_db_per_block_gindex_incs.size() == m_db_blocks.size(), "invariant failure: m_db_per_block_gindex_incs.size() == " << m_db_per_block_gindex_incs.size() << ", m_db_blocks.size() == " << m_db_blocks.size());

  CRITICAL_REGION_LOCAL(m_read_lock);

  // Calculate total number of outputs for each amount in the main chain from given height
  size_t top_block_height = static_cast<size_t>(m_db_blocks.size()) - 1;
  uint64_t h = height;
  while (h <= top_block_height)
  {
    auto p = m_db_per_block_gindex_incs.get(h);
    CHECK_AND_ASSERT_THROW_MES(p, "null p for height " << h);
    
    for (auto& el : p->increments)
      gindexes[el.amount] += el.increment;

    ++h;
  }

  // Translate total number of outputs for each amount into global output indexes these amounts had at given height.
  // (those amounts which are not present in the main chain after given height have the same gindexes at given height and at the most recent block)
  for (auto it = gindexes.begin(); it != gindexes.end(); ++it)
  {
    uint64_t amount = it->first;
    uint64_t gindexes_in_mainchain = m_db_outputs.get_item_size(amount);
    CHECK_AND_ASSERT_THROW_MES(gindexes_in_mainchain >= it->second, "inconsistent gindex increments for amount " << amount << ": gindexes_in_mainchain == " << gindexes_in_mainchain << ", gindex increment for height " << height << " is " << it->second);
    it->second = gindexes_in_mainchain - it->second;
  }
}
//------------------------------------------------------------------
bool blockchain_storage::validate_alt_block_input(const transaction& input_tx,
  std::unordered_set<crypto::key_image>& collected_keyimages, 
  const txs_by_id_and_height_altchain& alt_chain_tx_ids,
  const crypto::hash& bl_id, 
  const crypto::hash& input_tx_hash, 
  size_t input_index,
  const std::vector<crypto::signature>& input_sigs, 
  uint64_t split_height, 
  const alt_chain_type& alt_chain, 
  const std::unordered_set<crypto::hash>& alt_chain_block_ids, 
  uint64_t& ki_lookuptime,
  uint64_t* p_max_related_block_height /* = nullptr */) const
{
  // Main and alt chain outline:
  //
  // A- A- A- B- B- B- B-    <-- main chain
  //       |
  //       \- C- C- C-       <-- alt chain
  //          ^
  //          |
  //     split height
  //
  // List of possible cases
  //
  //   | src tx | another  | this tx |
  // # | output | tx input |  input  | meaning
  // ------------ bad cases ------------------
  // b1    A         A          C      already spent in main chain
  // b2    A         C          C      already spent in alt chain
  // b3    C         C          C      already spent in alt chain
  // b4    B        B/-         C      output not found (in case there's no valid outs in altchain C)
  // ------------ good cases ------------------
  // g1    A         -          C      normal spending output from main chain A
  // g2    A         B          C      normal spending output from main chain A (although it is spent in main chain B as well)
  // g3    C         -          C      normal spending output from alt chain C
  // g4   B,C        -          C      src tx added to both main chain B and alt chain C
  // g5   B,C        B          C      src tx added to both main chain B and alt chain C, also spent in B

  CRITICAL_REGION_LOCAL(m_read_lock);
  bool r = false;

  if (p_max_related_block_height != nullptr)
    *p_max_related_block_height = 0;

  CHECK_AND_ASSERT_MES(input_index < input_tx.vin.size(), false, "invalid input index: " << input_index);
  const txin_v& input_v = input_tx.vin[input_index];
  const txin_to_key& input_to_key = get_to_key_input_from_txin_v(input_v);

  // check case b1: key_image spent status in main chain, should be either non-spent or has spent height >= split_height
  auto p = m_db_spent_keys.get(input_to_key.k_image);
  CHECK_AND_ASSERT_MES(p == nullptr || *p >= split_height, false, "key image " << input_to_key.k_image << " has been already spent in main chain at height " << *p << ", split height: " << split_height);

  TIME_MEASURE_START(ki_lookup_time);
  //check key_image in altchain  
  //check among this alt block already collected key images first
  if (collected_keyimages.find(input_to_key.k_image) != collected_keyimages.end())
  {
    // cases b2, b3
    LOG_ERROR("key image " << input_to_key.k_image << " already spent in this alt block");
    return false;
  }
  auto ki_it = m_altblocks_keyimages.find(input_to_key.k_image);
  if (ki_it != m_altblocks_keyimages.end())
  {    
    //have some entry for this key image. Check if this key image belongs to this alt chain
    const std::list<crypto::hash>& key_image_block_ids = ki_it->second;
    for (auto& h : key_image_block_ids)
    {
      if (alt_chain_block_ids.find(h) != alt_chain_block_ids.end())
      {
        // cases b2, b3
        LOG_ERROR("key image " << input_to_key.k_image << " already spent in altchain");
        return false;
      }
    }
  }
  //update altchain with key image
  collected_keyimages.insert(input_to_key.k_image);
  TIME_MEASURE_FINISH(ki_lookup_time);
  ki_lookuptime = ki_lookup_time;

  std::vector<txout_ref_v> abs_key_offsets = relative_output_offsets_to_absolute(input_to_key.key_offsets);
  CHECK_AND_ASSERT_MES(abs_key_offsets.size() > 0 && abs_key_offsets.size() == input_to_key.key_offsets.size(), false, "internal error: abs_key_offsets.size()==" << abs_key_offsets.size() << ", input_to_key.key_offsets.size()==" << input_to_key.key_offsets.size());
  // eventually we should found all public keys for all outputs this input refers to, for checking ring signature
  std::vector<crypto::public_key> pub_keys(abs_key_offsets.size(), null_pkey); 

  //
  // let's try to resolve references and populate pub keys container for check_tokey_input()
  //
  uint64_t global_outs_for_amount = 0;
  //figure out if this amount touched alt_chain amount's index and if it is, get 
  bool amount_touched_altchain = false;
  //auto abg_it = abei.gindex_lookup_table.find(input_to_key.amount);
  //if (abg_it == abei.gindex_lookup_table.end())
  
  if (!alt_chain.empty())
  {
    auto abg_it = alt_chain.back()->second.gindex_lookup_table.find(input_to_key.amount);
    if (abg_it != alt_chain.back()->second.gindex_lookup_table.end())
    {
      amount_touched_altchain = true;
      // local gindex lookup table contains last used gindex, so we can't get total number of outs
      // just skip setting global_outs_for_amount
    }
    else
    {
      //quite easy, 
      global_outs_for_amount = m_db_outputs.get_item_size(input_to_key.amount);
    }
  }
  else
  {
    //quite easy, 
    global_outs_for_amount = m_db_outputs.get_item_size(input_to_key.amount);
  }
  
  CHECK_AND_ASSERT_MES(pub_keys.size() == abs_key_offsets.size(), false, "pub_keys.size()==" << pub_keys.size() << "  !=  abs_key_offsets.size()==" << abs_key_offsets.size()); // just a little bit of paranoia
  std::vector<const crypto::public_key*> pub_key_pointers;

  uint64_t height_of_current_alt_block = alt_chain.size() ? alt_chain.back()->second.height + 1 : split_height + 1;

  for (size_t pk_n = 0; pk_n < pub_keys.size(); ++pk_n)
  {
    crypto::public_key& pk = pub_keys[pk_n];
    crypto::hash tx_id = null_hash;
    uint64_t out_n = UINT64_MAX;
    auto &off = abs_key_offsets[pk_n];
    if (off.type() == typeid(uint64_t))
    {
      uint64_t offset_gindex = boost::get<uint64_t>(off);
      CHECK_AND_ASSERT_MES(amount_touched_altchain || (offset_gindex < global_outs_for_amount), false,
        "invalid global output index " << offset_gindex << " for amount=" << input_to_key.amount << 
        ", max is " << global_outs_for_amount << 
        ", referred to by offset #" << pk_n << 
        ", amount_touched_altchain = " << amount_touched_altchain);
      if (amount_touched_altchain)
      {
        bool found_the_key = false;
        for (auto alt_it = alt_chain.rbegin(); alt_it != alt_chain.rend(); alt_it++)
        {
          auto it_aag = (*alt_it)->second.gindex_lookup_table.find(input_to_key.amount);
          if (it_aag == (*alt_it)->second.gindex_lookup_table.end())
          {
            CHECK_AND_ASSERT_MES(alt_it != alt_chain.rbegin(), false, "internal error: was marked as amount_touched_altchain but unable to find on first entry");
            //item supposed to be in main chain
            break;
          }
          if (offset_gindex >= it_aag->second)
          {
            //source tx found in altchain
            //GOT IT!!
            //TODO: At the moment we ignore check of mix_attr against mixing to simplify alt chain check, but in future consider it for stronger validation
            uint64_t local_offset = offset_gindex - it_aag->second;
            auto& alt_keys = (*alt_it)->second.outputs_pub_keys;            
            CHECK_AND_ASSERT_MES(local_offset < alt_keys[input_to_key.amount].size(), false, "Internal error: local_offset=" << local_offset << " while alt_keys[" << input_to_key.amount << " ].size()=" << alt_keys.size());
            const output_key_or_htlc_v& out_in_alt = alt_keys[input_to_key.amount][local_offset];
            
            /*
            here we do validation against compatibility of input and output type

            TxOutput | TxInput | Allowed
            ----------------------------
            HTLC     |  HTLC   | ONLY IF HTLC NOT EXPIRED
            HTLC     |  TO_KEY | ONLY IF HTLC IS EXPIRED
            TO_KEY   |  HTLC   | NOT
            TO_KEY   |  TO_KEY | YES
            */
            uint64_t height_of_source_block = (*alt_it)->second.height;
            CHECK_AND_ASSERT_MES(height_of_current_alt_block > height_of_source_block, false, "Intenral error: height_of_current_alt_block > height_of_source_block failed");
            bool r = is_output_allowed_for_input(out_in_alt, input_v, height_of_current_alt_block - height_of_source_block);
            CHECK_AND_ASSERT_MES(r, false, "Input and output incompatible type");
            
            if (out_in_alt.type() == typeid(crypto::public_key))
            {
              pk = boost::get<crypto::public_key>(out_in_alt);
            }
            else
            {
              const txout_htlc& out_htlc = boost::get<txout_htlc>(out_in_alt);
              bool htlc_expired = out_htlc.expiration > (height_of_current_alt_block - height_of_source_block) ? false:true;
              pk = htlc_expired ? out_htlc.pkey_refund : out_htlc.pkey_redeem;
              //input_v
            }
            pub_key_pointers.push_back(&pk);
            found_the_key = true;
            break;
          }
        }
        if (found_the_key)
          continue;
        //otherwise lookup in main chain index
      }
      auto p = m_db_outputs.get_subitem(input_to_key.amount, offset_gindex);
      CHECK_AND_ASSERT_MES(p != nullptr, false, "global output was not found, amount: " << input_to_key.amount << ", gindex: " << offset_gindex << ", referred to by offset #" << pk_n);
      tx_id = p->tx_id;
      out_n = p->out_no;
    }
    else if (off.type() == typeid(ref_by_id))
    {
      auto &rbi = boost::get<ref_by_id>(off);
      tx_id = rbi.tx_id;
      out_n = rbi.n;
      //look up in alt-chain transactions fist
      auto it = alt_chain_tx_ids.find(tx_id);
      if (it != alt_chain_tx_ids.end())
      {
        uint64_t height_of_source_block = it->second.second;
        CHECK_AND_ASSERT_MES(height_of_current_alt_block > height_of_source_block, false, "Intenral error: height_of_current_alt_block > height_of_source_block failed");
        
        /*
        here we do validation against compatibility of input and output type

        TxOutput | TxInput | Allowed
        ----------------------------
        HTLC     |  HTLC   | ONLY IF HTLC NOT EXPIRED
        HTLC     |  TO_KEY | ONLY IF HTLC IS EXPIRED
        TO_KEY   |  HTLC   | NOT
        TO_KEY   |  TO_KEY | YES
        */

        //source tx found in altchain
        CHECK_AND_ASSERT_MES(it->second.first.vout.size() > out_n, false, "Internal error: out_n(" << out_n << ") >= it->second.vout.size()(" << it->second.first.vout.size() << ")");
        txout_target_v out_target_v = it->second.first.vout[out_n].target;

        bool r = is_output_allowed_for_input(out_target_v, input_v, height_of_current_alt_block - height_of_source_block);
        CHECK_AND_ASSERT_MES(r, false, "Input and output incompatible type");


        if (out_target_v.type() == typeid(txout_htlc))
        {
          //source is hltc out
          const txout_htlc& htlc = boost::get<txout_htlc>(out_target_v);          
          bool htlc_expired = htlc.expiration > (height_of_current_alt_block - height_of_source_block) ? false : true;
          pk = htlc_expired ? htlc.pkey_refund : htlc.pkey_redeem;
          pub_key_pointers.push_back(&pk);
          continue;
        }
        else if (out_target_v.type() == typeid(txout_to_key))
        {
          //source is to_key out
          pk = boost::get<txout_to_key>(out_target_v).key;
          pub_key_pointers.push_back(&pk);
          continue;
        }
        else
        {
          ASSERT_MES_AND_THROW("Unexpected out type for tx_in in altblock: " << out_target_v.type().name());
        }
      }

    }

    auto p = m_db_transactions.get(tx_id);
    CHECK_AND_ASSERT_MES(p != nullptr && out_n < p->tx.vout.size(), false, "can't find output #" << out_n << " for tx " << tx_id << " referred by offset #" << pk_n);
    auto &t = p->tx.vout[out_n].target;
    
    /*
    here we do validation against compatibility of input and output type

    TxOutput | TxInput | Allowed
    ----------------------------
    HTLC     |  HTLC   | ONLY IF HTLC NOT EXPIRED
    HTLC     |  TO_KEY | ONLY IF HTLC IS EXPIRED
    TO_KEY   |  HTLC   | NOT
    TO_KEY   |  TO_KEY | YES
    */
    uint64_t height_of_source_block = p->m_keeper_block_height;
    CHECK_AND_ASSERT_MES(height_of_current_alt_block > height_of_source_block, false, "Intenral error: height_of_current_alt_block > height_of_source_block failed");
    bool r = is_output_allowed_for_input(t, input_v,   height_of_current_alt_block - height_of_source_block);
    CHECK_AND_ASSERT_MES(r, false, "Input and output incompatible type");

    if (t.type() == typeid(txout_to_key))
    {
      const txout_to_key& out_tk = boost::get<txout_to_key>(t);
      pk = out_tk.key;

      bool mixattr_ok = is_mixattr_applicable_for_fake_outs_counter(out_tk.mix_attr, abs_key_offsets.size() - 1);
      CHECK_AND_ASSERT_MES(mixattr_ok, false, "input offset #" << pk_n << " violates mixin restrictions: mix_attr = " << static_cast<uint32_t>(out_tk.mix_attr) << ", input's key_offsets.size = " << abs_key_offsets.size());

    }
    else if (t.type() == typeid(txout_htlc))
    {
      const txout_htlc& htlc = boost::get<txout_htlc>(t);
      bool htlc_expired = htlc.expiration > (height_of_current_alt_block - height_of_source_block) ? false : true;
      pk = htlc_expired ? htlc.pkey_refund : htlc.pkey_redeem;
    }

    // case b4 (make sure source tx in the main chain is preceding split point, otherwise this referece is invalid)
    CHECK_AND_ASSERT_MES(p->m_keeper_block_height < split_height, false, "input offset #" << pk_n << " refers to main chain tx " << tx_id << " at height " << p->m_keeper_block_height << " while split height is " << split_height);

    if (p_max_related_block_height != nullptr && *p_max_related_block_height < p->m_keeper_block_height)
      *p_max_related_block_height = p->m_keeper_block_height;

    // TODO: consider checking p->tx for unlock time validity as it's checked in get_output_keys_for_input_with_checks()
    // make sure it was actually found
    
    // let's disable this check due to missing equal check in main chain validation code
    //TODO: implement more strict validation with next hard fork
    //CHECK_AND_ASSERT_MES(pk != null_pkey, false, "Can't determine output public key for offset " << pk_n << " in related tx: " << tx_id << ", out_n = " << out_n);
    pub_key_pointers.push_back(&pk);
  }

  // do input checks (attachment_info, ring signature and extra signature, etc.)
  r = check_input_signature(input_tx, input_index, input_to_key, input_tx_hash, input_sigs, pub_key_pointers);
  CHECK_AND_ASSERT_MES(r, false, "to_key input validation failed");

  // TODO: consider checking input_tx for valid extra attachment info as it's checked in check_tx_inputs()

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::is_output_allowed_for_input(const txout_target_v& out_v, const txin_v& in_v, uint64_t top_minus_source_height)const
{

  /*
  TxOutput | TxInput | Allowed
  ----------------------------
  HTLC     |  HTLC   | ONLY IF HTLC NOT EXPIRED
  HTLC     |  TO_KEY | ONLY IF HTLC IS EXPIRED
  TO_KEY   |  HTLC   | NOT
  TO_KEY   |  TO_KEY | YES
  */


  if (out_v.type() == typeid(txout_to_key))
  {
    return is_output_allowed_for_input(boost::get<txout_to_key>(out_v), in_v);
  }
  else if (out_v.type() == typeid(txout_htlc))
  {
    return is_output_allowed_for_input(boost::get<txout_htlc>(out_v), in_v, top_minus_source_height);
  }
  else
  {
    LOG_ERROR("[scan_outputkeys_for_indexes]: Wrong output type in : " << out_v.type().name());
    return false;
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::is_output_allowed_for_input(const txout_htlc& out_v, const txin_v& in_v, uint64_t top_minus_source_height)const 
{
  bool htlc_expired = out_v.expiration > (top_minus_source_height) ? false : true;
  if (!htlc_expired)
  {
    //HTLC IS NOT expired, can be used ONLY by pkey_before_expiration and ONLY by HTLC input
    CHECK_AND_ASSERT_MES(in_v.type() == typeid(txin_htlc), false, "[TXOUT_HTLC]: Unexpected output type of non-HTLC input");
  }
  else
  {
    //HTLC IS expired, can be used ONLY by pkey_after_expiration and ONLY by to_key input
    CHECK_AND_ASSERT_MES(in_v.type() == typeid(txin_to_key), false, "[TXOUT_HTLC]: Unexpected output type of HTLC input");
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::is_output_allowed_for_input(const txout_to_key& out_v, const txin_v& in_v)const
{
  //HTLC input CAN'T refer to regular to_key output
  CHECK_AND_ASSERT_MES(in_v.type() != typeid(txin_htlc), false, "[TXOUT_TO_KEY]: Unexpected output type of HTLC input");
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::is_output_allowed_for_input(const output_key_or_htlc_v& out_v, const txin_v& in_v, uint64_t top_minus_source_height)const
{
  if (out_v.type() == typeid(crypto::public_key))
  {
    return is_output_allowed_for_input(txout_to_key(), in_v);
  }
  else if (out_v.type() == typeid(txout_htlc))
  {
    return is_output_allowed_for_input(boost::get<txout_htlc>(out_v), in_v, top_minus_source_height);
  }
  else {
    ASSERT_MES_AND_THROW("Unexpected type in output_key_or_htlc_v: " << out_v.type().name());
  }
}
//------------------------------------------------------------------
bool blockchain_storage::validate_alt_block_ms_input(const transaction& input_tx, const crypto::hash& input_tx_hash, size_t input_index, const std::vector<crypto::signature>& input_sigs, uint64_t split_height, const alt_chain_type& alt_chain) const
{
  // Main and alt chain outline:
  //
  // A- A- A- B- B- B- B-    <-- main chain
  //       |
  //       \- C- C- C-       <-- alt chain
  //          ^
  //          |
  //     split height
  //
  // List of possible cases
  //
  //   | src tx | another  | this tx |
  // # | output | tx input |  input  | meaning
  // ------------ bad cases ------------------
  // b1    A         A          C      already spent in main chain
  // b2    A         C          C      already spent in alt chain
  // b3    C         C          C      already spent in alt chain
  // b4    B        B/-         C      output not found (in case there's no valid outs in altchain C)
  // ------------ good cases ------------------
  // g1    A         -          C      normal spending output from main chain A
  // g2    A         B          C      normal spending output from main chain A (although it is spent in main chain B as well)
  // g3    C         -          C      normal spending output from alt chain C
  // g4   B,C        -          C      src tx added to both main chain B and alt chain C
  // g5   B,C        B          C      src tx added to both main chain B and alt chain C, also spent in B

  CRITICAL_REGION_LOCAL(m_read_lock);
  bool r = false;

  CHECK_AND_ASSERT_MES(input_index < input_tx.vin.size() && input_tx.vin[input_index].type() == typeid(txin_multisig), false, "invalid ms input index: " << input_index << " or type");
  const txin_multisig& input = boost::get<txin_multisig>(input_tx.vin[input_index]);

  // check corresponding ms out in the main chain
  auto p = m_db_multisig_outs.get(input.multisig_out_id);
  if (p != nullptr)
  {
    // check case b1 (note: need to distinguish from case g2)
    CHECK_AND_ASSERT_MES(p->spent_height == 0 || p->spent_height >= split_height, false, "ms output was already spent in main chain at height " << p->spent_height << " while split_height is " << split_height);

    const crypto::hash& source_tx_id = p->tx_id;
    auto p_source_tx = m_db_transactions.get(source_tx_id);
    CHECK_AND_ASSERT_MES(p_source_tx != nullptr, false, "source tx for ms output " << source_tx_id << " was not found, ms out id: " << input.multisig_out_id);
    
    if (p_source_tx->m_keeper_block_height < split_height)
    {
      // cases g1, g2
      return check_ms_input(input_tx, input_index, input, input_tx_hash, input_sigs, p_source_tx->tx, p->out_no);
    }
    
    // p_source_tx is above split_height in main chain B, so it can't be a source for this input
    // do nohting here and proceed to alt chain scan for possible cases b4, g4, g5
  }
  else
  {
    // ms out was not found in DB
    // do nothing here and proceed to alt chain scan for possible cases b2, b3, g3, g4, g5
  }


  // walk the alt chain backward (from the the last added alt block towards split point -- it important as we stop scanning when find correct output, need to make sure it was not already spent)
  for (alt_chain_type::const_reverse_iterator it = alt_chain.rbegin(); it != alt_chain.rend(); ++it)
  {
    const block_extended_info& bei = (*it)->second;
    const block& b = bei.bl;
    bool output_found = false;

    auto tx_altchain_checker = [&](const transaction& tx, const crypto::hash& tx_id) -> bool
    {
      // check ms out being already spent in current alt chain
      for (auto& in : tx.vin)
      {
        if (in.type() == typeid(txin_multisig))
        {
          // check cases b2, b3
          CHECK_AND_ASSERT_MES(input.multisig_out_id != boost::get<txin_multisig>(in).multisig_out_id, false, "ms out " << input.multisig_out_id << " has been already spent in altchain by tx " << tx_id << " in block " << get_block_hash(b) << " height " << bei.height);
        }
      }

      for (size_t out_n = 0; out_n < tx.vout.size(); ++out_n)
      {
        const tx_out& out = tx.vout[out_n];
        if (out.target.type() == typeid(txout_multisig))
        {
          const crypto::hash& ms_out_id = get_multisig_out_id(tx, out_n);
          if (ms_out_id == input.multisig_out_id)
          {
            // cases g3, g4, g5
            output_found = true;
            return check_ms_input(input_tx, input_index, input, input_tx_hash, input_sigs, tx, out_n);
          }
        }
      }
      return true;
    };

    // for each alt block look into miner_tx and txs
    CHECK_AND_ASSERT_MES(tx_altchain_checker(b.miner_tx, get_transaction_hash(b.miner_tx)), false, "tx_altchain_checker failed for miner tx");
    if (output_found)
      return true; // g3, g4, g5
    for (auto& tx_id : b.tx_hashes)
    {
      std::shared_ptr<transaction> tx_ptr;
      r = get_transaction_from_pool_or_db(tx_id, tx_ptr, split_height);
      CHECK_AND_ASSERT_MES(r, false, "can't get transaction " << tx_id << " for alt block " << get_block_hash(b) << " height " << bei.height << ", split height is " << split_height);
      transaction& tx = *tx_ptr;
      CHECK_AND_ASSERT_MES(tx_altchain_checker(tx, tx_id), false, "tx_altchain_checker failed for tx " << tx_id);
      if (output_found)
        return true; // g3, g4, g5
    }
  }

  // case b4
  LOG_ERROR("ms outout " << input.multisig_out_id << " was not found neither in main chain, nor in alt chain");
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::get_transaction_from_pool_or_db(const crypto::hash& tx_id, std::shared_ptr<transaction>& tx_ptr, uint64_t min_allowed_block_height /* = 0 */) const
{                    
  tx_ptr.reset(new transaction());

  if (!m_tx_pool.get_transaction(tx_id, *tx_ptr)) // first try to get from the pool
  {
    auto p = m_db_transactions.get(tx_id); // if not found in the pool -- get from the DB
    if (p == nullptr)
    {
      return false;
    }
    //CHECK_AND_ASSERT_MES(p != nullptr, false, "can't get tx " << tx_id << " neither from the pool, nor from db_transactions");
    CHECK_AND_ASSERT_MES(p->m_keeper_block_height >= min_allowed_block_height, false, "tx " << tx_id << " found in the main chain at height " << p->m_keeper_block_height << " while required min allowed height is " << min_allowed_block_height);
    *tx_ptr = p->tx;
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::update_alt_out_indexes_for_tx_in_block(const transaction& tx, alt_block_extended_info& abei) const 
{
  //add tx outputs to gindex_lookup_table
  for (auto o : tx.vout)
  {
    if (o.target.type() == typeid(txout_to_key) || o.target.type() == typeid(txout_htlc))
    {
      //LOG_PRINT_MAGENTA("ALT_OUT KEY ON H[" << abei.height << "] AMOUNT: " << o.amount, LOG_LEVEL_0);
      // first, look at local gindexes tables
      if (abei.gindex_lookup_table.find(o.amount) == abei.gindex_lookup_table.end())
      {
        // amount was not found in altchain gindexes container, start indexing from current main chain gindex
        abei.gindex_lookup_table[o.amount] = m_db_outputs.get_item_size(o.amount);
        //LOG_PRINT_MAGENTA("FIRST TOUCH: size=" << abei.gindex_lookup_table[o.amount], LOG_LEVEL_0);
      }
      if (o.target.type() == typeid(txout_to_key))
      {
        abei.outputs_pub_keys[o.amount].push_back(boost::get<txout_to_key>(o.target).key);
      }
      else
      {
        abei.outputs_pub_keys[o.amount].push_back(boost::get<txout_htlc>(o.target));
      }
      
      //TODO: At the moment we ignore check of mix_attr again mixing to simplify alt chain check, but in future consider it for stronger validation
    }
  }
  return true;
}

bool blockchain_storage::validate_alt_block_txs(const block& b, const crypto::hash& id, std::unordered_set<crypto::key_image>& collected_keyimages, alt_block_extended_info& abei, const alt_chain_type& alt_chain, uint64_t split_height, uint64_t& ki_lookup_time_total) const
{
  uint64_t height = abei.height;
  bool r = false;
  std::unordered_set<crypto::hash> alt_chain_block_ids;
  txs_by_id_and_height_altchain alt_chain_tx_ids;

  
  alt_chain_block_ids.insert(id);
  // prepare data structure for output global indexes tracking within current alt chain
  if (alt_chain.size())
  {
    //TODO: in this two lines we scarify memory to earn speed for algo, but need to be careful with RAM consuming on long switches
    abei.gindex_lookup_table = alt_chain.back()->second.gindex_lookup_table;

    //adjust indices for next alt block entry according to emount of pubkeys in txs of prev block
    auto& prev_alt_keys = alt_chain.back()->second.outputs_pub_keys;
    for (auto it = prev_alt_keys.begin(); it != prev_alt_keys.end(); it++)
    {
      auto it_amont_in_abs_ind = abei.gindex_lookup_table.find(it->first);
      CHECK_AND_ASSERT_MES(it_amont_in_abs_ind != abei.gindex_lookup_table.end(), false, "internal error: not found amount " << it->first << "in gindex_lookup_table");
      //increase index starter for amount of outputs in prev block
      it_amont_in_abs_ind->second += it->second.size();
    }
    //generate set of alt block ids
    for (auto& ch : alt_chain)
    {
      alt_chain_block_ids.insert(get_block_hash(ch->second.bl));
      for (auto & on_board_tx : ch->second.onboard_transactions)
      {
        alt_chain_tx_ids.insert(txs_by_id_and_height_altchain::value_type(on_board_tx.first, txs_by_id_and_height_altchain::value_type::second_type(on_board_tx.second, ch->second.height)));
      }
      //TODO: consider performance optimization (get_transaction_hash might slow down deep reorganizations )
      alt_chain_tx_ids.insert(txs_by_id_and_height_altchain::value_type(get_transaction_hash(ch->second.bl.miner_tx), txs_by_id_and_height_altchain::value_type::second_type(ch->second.bl.miner_tx, ch->second.height)));
    }
  }
  else
  {
    // initialize alt chain entry with initial gindexes
    calculate_local_gindex_lookup_table_for_height(split_height, abei.gindex_lookup_table);
  }

  if (is_pos_block(b))
  {
    // check PoS block miner tx in a special way
    CHECK_AND_ASSERT_MES(b.miner_tx.signatures.size() == 1 && b.miner_tx.vin.size() == 2, false, "invalid PoS block's miner_tx, signatures size = " << b.miner_tx.signatures.size() << ", miner_tx.vin.size() = " << b.miner_tx.vin.size());
    uint64_t max_related_block_height = 0;
    uint64_t ki_lookup = 0;
    r = validate_alt_block_input(b.miner_tx, collected_keyimages, alt_chain_tx_ids, id, get_block_hash(b), 1, b.miner_tx.signatures[0], split_height, alt_chain, alt_chain_block_ids, ki_lookup, &max_related_block_height);
    CHECK_AND_ASSERT_MES(r, false, "miner tx " << get_transaction_hash(b.miner_tx) << ": validation failed");
    ki_lookup_time_total += ki_lookup;
    // check stake age
    uint64_t coinstake_age = height - max_related_block_height - 1;
    CHECK_AND_ASSERT_MES(coinstake_age >= m_core_runtime_config.min_coinstake_age, false,
      "miner tx's coinstake age is " << coinstake_age << ", that is less than minimum required " << m_core_runtime_config.min_coinstake_age << "; max_related_block_height == " << max_related_block_height);
  }
  update_alt_out_indexes_for_tx_in_block(b.miner_tx, abei);

  CHECK_AND_ASSERT_MES(validate_tx_for_hardfork_specific_terms(b.miner_tx, null_hash, height), false, "miner tx hardfork-specific validation failed");

  for (auto tx_id : b.tx_hashes)
  {
    std::shared_ptr<transaction> tx_ptr;
    auto it = abei.onboard_transactions.find(tx_id);
    if (it == abei.onboard_transactions.end())
    {
      CHECK_AND_ASSERT_MES(get_transaction_from_pool_or_db(tx_id, tx_ptr, split_height), false, "failed to get alt block tx " << tx_id << " with split_height == " << split_height);
    }
    const transaction& tx = it == abei.onboard_transactions.end() ? *tx_ptr : it->second;
    CHECK_AND_ASSERT_MES(tx.signatures.size() == tx.vin.size(), false, "invalid tx: tx.signatures.size() == " << tx.signatures.size() << ", tx.vin.size() == " << tx.vin.size());
    for (size_t n = 0; n < tx.vin.size(); ++n)
    {
      if (tx.vin[n].type() == typeid(txin_to_key) || tx.vin[n].type() == typeid(txin_htlc))
      {
        uint64_t ki_lookup = 0;
        r = validate_alt_block_input(tx, collected_keyimages, alt_chain_tx_ids, id, tx_id, n, tx.signatures[n], split_height, alt_chain, alt_chain_block_ids, ki_lookup);
        CHECK_AND_ASSERT_MES(r, false, "tx " << tx_id << ", input #" << n << ": validation failed");
        ki_lookup_time_total += ki_lookup;
      }
      else if (tx.vin[n].type() == typeid(txin_multisig))
      {
        r = validate_alt_block_ms_input(tx, tx_id, n, tx.signatures[n], split_height, alt_chain);
        CHECK_AND_ASSERT_MES(r, false, "tx " << tx_id << ", input #" << n << " (multisig): validation failed");
      }
      else if (tx.vin[n].type() == typeid(txin_gen))
      {
        // genesis can't be in tx_hashes
        CHECK_AND_ASSERT_MES(false, false, "input #" << n << " has unexpected type (" << tx.vin[n].type().name() << ", genesis can't be in tx_hashes), tx " << tx_id);
      }
      else
      {
        CHECK_AND_ASSERT_MES(false, false, "input #" << n << " has unexpected type (" << tx.vin[n].type().name() << "), tx " << tx_id);
      }
    }

    CHECK_AND_ASSERT_MES(validate_tx_for_hardfork_specific_terms(tx, tx_id, height), false, "tx " << tx_id << ": hardfork-specific validation failed");
    
    // Updating abei (and not updating alt_chain) during this cycle is safe because txs in the same block can't reference one another,
    // so only valid references are either to previous alt blocks (accessed via alt_chain) or to main chain blocks.
    update_alt_out_indexes_for_tx_in_block(tx, abei);
  }


  return true;
}


