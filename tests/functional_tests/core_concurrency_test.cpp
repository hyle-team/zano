// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <thread>

#include "core_concurrency_test.h"
#include <boost/variant.hpp>

#include "include_base_utils.h"
using namespace epee;
#include "currency_core/currency_core.h"
#include "rpc/core_rpc_server.h"
#include "wallet/core_fast_rpc_proxy.h"
#include "version.h"
#include "common/command_line.h"
#include "common/boost_serialization_helper.h"
using namespace currency;

#include "../core_tests/test_core_time.h"
std::atomic<int64_t> test_core_time::m_time_shift;

#include "../core_tests/random_helper.h"
#include "../core_tests/test_core_proxy.h"
#include "../core_tests/chaingen_helpers.h"
#include "../core_tests/core_state_helper.h"

//#define TESTS_DEFAULT_FEE TX_DEFAULT_FEE

static std::atomic<uint64_t> s_generated_money_total(0); // TODO: consiger changing to boost::multiprecision::uint128_t
static size_t s_wallets_total_count           = 10; // total number of wallet that will be randomly used to generate transactions
//static size_t s_althchains_minimum_height     = 150; // height at which althchaining is started
static size_t s_tx_generation_minimum_height  = 100; // height at which tx generation is started
static size_t s_tx_count_per_block_max        = 0; // maximum tx count to be generated per block
static size_t s_althchain_max_depth           = 9; // maximum possible length of alternative chain to be generated

typedef boost::variant<currency::block, currency::transaction> cct_event_t; // CCT = core concurrency test
typedef std::vector<cct_event_t> cct_events_t;
typedef std::vector<currency::account_base> cct_accounts_t;
typedef std::vector<std::shared_ptr<tools::wallet2>> cct_wallets_t;

//static const std::vector<currency::extra_v> empty_extra;
//static const std::vector<currency::attachment_v> empty_attachment;

bool create_block_template_manually(const currency::block& prev_block, boost::multiprecision::uint128_t already_generated_coins, const std::vector<const currency::transaction*>& txs, const currency::account_public_address& miner_addr, currency::block& result)
{
  result.flags = 0;
  result.major_version = BLOCK_MAJOR_VERSION_INITIAL;
  result.minor_version = CURRENT_BLOCK_MINOR_VERSION;
  result.nonce = 0;
  result.prev_id = get_block_hash(prev_block);
  result.timestamp = prev_block.timestamp != 0 ? prev_block.timestamp + DIFFICULTY_POW_TARGET : test_core_time::get_time();
  
  uint64_t fee = 0;
  size_t txs_size = 0;
  for(auto& ptx : txs)
  {
    result.tx_hashes.push_back(get_transaction_hash(*ptx));
    fee += get_tx_fee(*ptx);
    txs_size += get_object_blobsize(*ptx);
  }

  // make things really simple by assuming block size is less than CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE
  size_t median_size = 0;
  uint64_t block_reward_without_fee = 0;
  uint64_t block_reward = 0;

  bool r = construct_miner_tx(get_block_height(prev_block) + 1,
    median_size,
    already_generated_coins,
    txs_size,
    fee,
    miner_addr,
    miner_addr,
    result.miner_tx,
    block_reward_without_fee,
    block_reward,
    TRANSACTION_VERSION_PRE_HF4);
  CHECK_AND_ASSERT_MES(r, false, "construct_miner_tx failed");

  size_t coinbase_size = get_object_blobsize(result.miner_tx);
  // "- 100" - to reserve room for PoS additions into miner tx
  CHECK_AND_ASSERT_MES(coinbase_size < CURRENCY_COINBASE_BLOB_RESERVED_SIZE - 100, false, "Failed to get block template (coinbase_size = " << coinbase_size << ")");

  size_t block_size = get_object_blobsize(result);
  CHECK_AND_ASSERT_MES(block_size < CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE, false, "block is bigger than CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE");

  return true;
}


bool generate_events(currency::core& c, cct_events_t& events, const cct_wallets_t& wallets, size_t blocks_count)
{
  blockchain_storage& bcs = c.get_blockchain_storage();
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == 1, false, "");
  bool r = false;

  uint64_t height = 1;
  size_t altchain_max_size = 0; // used to limit size of alt chains in some cases
  size_t last_altchain_block_height = 0;
  bool alt_chains_enabled = false;
  block_extended_info prev_block = AUTO_VAL_INIT(prev_block), current_block = AUTO_VAL_INIT(current_block);
  r = bcs.get_block_extended_info_by_height(0, prev_block);
  CHECK_AND_ASSERT_MES(r, false, "get_block_extended_info_by_height failed");

  for (size_t block_index = 1; block_index < blocks_count; ++block_index)
  {
    //block_extended_info prev_block = AUTO_VAL_INIT(prev_block);
    //r = bcs.get_block_extended_info_by_height(height - 1, prev_block);
    //CHECK_AND_ASSERT_MES(r, false, "get_block_extended_info_by_height failed");
    const bool is_in_main_chain = c.get_current_blockchain_size() == prev_block.height + 1 && bcs.get_top_block_id() == get_block_hash(prev_block.bl);

    const currency::account_public_address& miner_addr = wallets[random_in_range(0, wallets.size() - 1)]->get_account().get_public_address();
    currency::block b = AUTO_VAL_INIT(b);

    if (is_in_main_chain)
    {
      blobdata ex_nonce;
      wide_difficulty_type diff = 0;
      if (prev_block.height != 0)
        test_core_time::adjust(prev_block.bl.timestamp + DIFFICULTY_POW_TARGET);
      r = bcs.create_block_template(miner_addr, ex_nonce, b, diff, height);
      CHECK_AND_ASSERT_MES(r, false, "create_block_template failed");
    }
    else
    {
      // walk events container backward to collect txs
      std::vector<const currency::transaction*> txs;
      for (cct_events_t::const_reverse_iterator it = events.rbegin(); it != events.rend(); ++it)
      {
        if (it->type() == typeid(currency::transaction))
        {
          const transaction& tx = boost::get<const currency::transaction>(*it);
          uint64_t max_used_block_height = 0;
          r = bcs.check_tx_inputs(tx, get_transaction_hash(tx), max_used_block_height);
          if (r && max_used_block_height <= prev_block.height)
            txs.push_back(&tx); // filter out tx that are using too recent outputs -- yep, some txs will be dropped forever
        }
        else if (it->type() == typeid(currency::block))
        {
          const block& b = boost::get<currency::block>(*it);
          size_t h = get_block_height(b);
          if (h == prev_block.height)
            break;
          CHECK_AND_ASSERT_MES(h > prev_block.height, false, "internal invariant failed: h: " << h << ", prev_block.height: " << prev_block.height); // should never seen as h < prev_block.height
        }
      }

      r = create_block_template_manually(prev_block.bl, prev_block.already_generated_coins, txs, miner_addr, b);
      CHECK_AND_ASSERT_MES(r, false, "create_block_template_manually failed");
    }

    test_core_time::adjust(b.timestamp);

    currency::wide_difficulty_type diff = 0;
    r = currency::miner::find_nonce_for_given_block(b, diff, height);
    CHECK_AND_ASSERT_MES(r, false, "find_nonce_for_given_block failed");

    currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
    if (!is_in_main_chain && bvc.m_verification_failed)
    {
      // alt chain gone wild (ex: block triggered reorganization which failed) -- return back to main chain
      events.push_back(b);
      LOG_PRINT_CYAN("\n==============================================\n" "EVENT[" << events.size() - 1 << "]: INVALID BLOCK at " << current_block.height << " in alt chain\n==============================================", LOG_LEVEL_1);
      bcs.get_block_extended_info_by_height(bcs.get_top_block_height(), prev_block); // return back to main chain
      continue;
    }
    CHECK_AND_NO_ASSERT_MES(!bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists, false, "block verification context check failed");

    if (is_in_main_chain && c.get_pool_transactions_count() > 0)
    {
      LOG_PRINT("!!! txs in the pool: " << c.get_pool_transactions_count(), LOG_LEVEL_0);
    }

    crypto::hash current_block_hash = get_block_hash(b);
    r = bcs.get_block_extended_info_by_hash(current_block_hash, current_block);
    CHECK_AND_ASSERT_MES(r, false, "get_block_extended_info_by_hash failed");
    if (current_block.already_generated_coins == 0)
      current_block.already_generated_coins = prev_block.already_generated_coins + get_outs_money_amount(current_block.bl.miner_tx); // workaround for altchains -- BCS does not calculate already generated coins

    if (!is_in_main_chain)
      last_altchain_block_height = current_block.height;

    events.push_back(b);
    LOG_PRINT_CYAN("\n==============================================\n" "EVENT[" << events.size() - 1 << "]: BLOCK at " << current_block.height << " in " << (is_in_main_chain ? "MAIN" : "alt") << " chain\n==============================================", LOG_LEVEL_1);

    // wait few blocks for mined money to be unlocked and collected
    if (/*is_in_main_chain && */current_block.height >= s_tx_generation_minimum_height)
    {
      // generate some txs
      std::shared_ptr<tools::wallet2> alice_wlt = wallets[random_in_range(0, wallets.size() - 1)];
      alice_wlt->refresh();

      size_t txs_to_generate = random_in_range(0, s_tx_count_per_block_max);
      for(size_t i = 0; i < txs_to_generate; ++i)
      {
        std::vector<tx_destination_entry> destinations({ tx_destination_entry(TESTS_DEFAULT_FEE, miner_addr) });
        transaction tx = AUTO_VAL_INIT(tx);
        r = true;
        try
        {
          alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tx);
        }
        catch (std::exception& e)
        {
          LOG_ERROR("transfer failed with an exception, what = " << e.what());

          /*
          // Try to find key image in BCS for debug purpose...
          crypto::key_image ki = boost::get<txin_to_key>(tx.vin[0]).k_image;
          bool found = false;
          auto tx_ki_finder = [&bcs, &ki, &found] (uint64_t i, crypto::hash tx_id) -> bool {
            transaction_chain_entry tce = AUTO_VAL_INIT(tce);
            if (!bcs.get_tx_chain_entry(tx_id, tce))
            {
              static std::stringstream ss;
              ss.clear(); ss << "get_tx_chain_entry failed with id = " << tx_id;
              throw std::exception(ss.str().c_str());
            }
            size_t inp_idx = 0;
            for (auto& in : tce.tx.vin)
            {
              if (in.type() == typeid(txin_to_key) && boost::get<txin_to_key>(in).k_image == ki)
              {
                LOG_ERROR("key image " << ki << " was already spent by input #" << inp_idx << " in tx " << get_transaction_hash(tce.tx) << " at height " << tce.m_keeper_block_height);
                found = true;
                return false; // stop enumeration
              }
              ++inp_idx;
            }
            return true; // continue enumaration
          };

          bcs.enumerate_transactions(tx_ki_finder);

          if (!found)
          {
            LOG_PRINT("tx that used key image " << ki << " was NOT found in BCS. Perhaps such tx is in the pool.", LOG_LEVEL_0);
          }
          */

          r = false;
        }

        if (r)
        {
          events.push_back(tx);
          LOG_PRINT_CYAN("\n==============================================\n" "EVENT[" << events.size() - 1 << "]: TX " << get_transaction_hash(tx) << "\n==============================================", LOG_LEVEL_1);
        }
      }
    }

    // chain swithcing mechanism
    if (!alt_chains_enabled && current_block.height == s_tx_generation_minimum_height)
      alt_chains_enabled = true; // one-time trigger to enable alt-chaining after height s_tx_generation_minimum_height

    prev_block = current_block; // default behaviour is to continue current chain whatever it is, prev_block is to be overriden below
    if (alt_chains_enabled)
    {
      // do alt chaining
      if (is_in_main_chain && last_altchain_block_height + 2 * s_althchain_max_depth < current_block.height && random_in_range(0, 5) == 0)
      {
        // we are in main chain -- desided to start alt chain
        size_t altchain_depth = random_in_range(1, s_althchain_max_depth);
        altchain_max_size = random_in_range(0, 1) == 0 ? SIZE_MAX : random_in_range(1, altchain_depth); // limit altchain size to certain value (or don't limit so switching do will eventually occur)

        // start next block as altchain from old block
        bcs.get_block_extended_info_by_height(current_block.height - altchain_depth, prev_block);
      }
      else if (!is_in_main_chain)
      {
        // we are in alt chain, check whether we should continue it or stop it and return to the main
        if (altchain_max_size > 0)
        {
          // do nothing -- just continue current chain
          --altchain_max_size;
        }
        else
        {
          // return back to main chain
          bcs.get_block_extended_info_by_height(bcs.get_top_block_height(), prev_block);
        }
      }

      LOG_PRINT2("cct_altchains.log", "tx pool size:\t" << c.get_pool_transactions_count() << "\tcurrent height:\t" << current_block.height << "\tprev height:\t" << prev_block.height << "\tchain:\t" << (is_in_main_chain ? "MAIN" : "alt"), LOG_LEVEL_0);
    }

  }


  return true;
}

bool clean_data_directory(boost::program_options::variables_map& vm)
{
  std::string config_folder = command_line::get_arg(vm, command_line::arg_data_dir);
  const std::string bch_db_folder_path = config_folder + ("/" CURRENCY_BLOCKCHAINDATA_FOLDERNAME_PREFIX) + "lmdb" + CURRENCY_BLOCKCHAINDATA_FOLDERNAME_SUFFIX;
  const std::string pool_db_folder_path = config_folder + ("/" CURRENCY_POOLDATA_FOLDERNAME_PREFIX) + "lmdb" + CURRENCY_POOLDATA_FOLDERNAME_SUFFIX;

  static const char* const files[] = { bch_db_folder_path.c_str(), pool_db_folder_path.c_str(), MINER_CONFIG_FILENAME };
  for (size_t i = 0; i < sizeof files / sizeof files[0]; ++i)
  {
    boost::filesystem::path filename(config_folder + "/" + files[i]);
    if (boost::filesystem::exists(filename))
      CHECK_AND_ASSERT_MES(boost::filesystem::remove_all(filename), false, "boost::filesystem::remove failed to remove this: " << filename);
  }

  return true;
}

struct writer_context
{
  writer_context() : blocks_total(0), blocks_added(0), blocks_already_existed(0), blocks_failed(0) {}
  size_t blocks_total;
  size_t blocks_added;
  size_t blocks_already_existed;
  size_t blocks_failed;
};

bool replay_events(currency::core& c, const cct_events_t& events, size_t thread_index, writer_context& context)
{
  //bool r = false;

  for(size_t event_idx = 0; event_idx < events.size(); ++event_idx)
  {
    if (events[event_idx].type() == typeid(currency::block))
    {
      const currency::block& b = boost::get<const currency::block>(events[event_idx]);
      if (thread_index == 0)
        test_core_time::adjust(b.timestamp); // only the first thread adjusts time
      currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
      c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);

      context.blocks_total++;
      if (!bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists)
      {
        context.blocks_added++;
        size_t sleep_time = random_in_range(50, 70);
        LOG_PRINT_L0("writer thread #" << thread_index << ", going to sleep: " << sleep_time << " ms");
        epee::misc_utils::sleep_no_w(sleep_time);
      }
      else if (bvc.m_already_exists)
        context.blocks_already_existed++;
      else
        context.blocks_failed++;

      //CHECK_AND_NO_ASSERT_MES(!bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists, false, "block verification context check failed");
    }
    else if (events[event_idx].type() == typeid(currency::transaction))
    {
      const currency::transaction& tx = boost::get<const currency::transaction>(events[event_idx]);
      currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      c.handle_incoming_tx(t_serializable_object_to_blob(tx), tvc, false);
      //CHECK_AND_ASSERT_MES(!tvc.m_verification_failed && tvc.m_added_to_pool, false, "tx verification context check failed");
    }
  }

  return true;
}

void performe_core_reads(const currency::core& c, std::atomic<bool>& stop)
{
  NOTIFY_REQUEST_GET_OBJECTS::request req_objs_req = AUTO_VAL_INIT(req_objs_req);
  req_objs_req.blocks.push_back(c.get_block_id_by_height(0));
  NOTIFY_RESPONSE_CHAIN_ENTRY::request resp = AUTO_VAL_INIT(resp);
  c.find_blockchain_supplement(req_objs_req.blocks, resp);
  NOTIFY_RESPONSE_GET_OBJECTS::request res_objs_seq = AUTO_VAL_INIT(res_objs_seq);
  currency::currency_connection_context context = AUTO_VAL_INIT(context);
  c.handle_get_objects(req_objs_req, res_objs_seq, context);
}


void blockchain_reader(const currency::core& c, std::atomic<bool>& stop)
{
  auto& bcs = c.get_blockchain_storage();
  
  size_t prev_block_height = 0;
  uint64_t steps_to_stop = UINT_MAX;
  while (--steps_to_stop > 0)
  {
    if (steps_to_stop > 100 && stop)
      steps_to_stop = 2;

    size_t top_block_height = bcs.get_top_block_height();
    if (top_block_height == prev_block_height)
    {
      std::this_thread::yield();
      continue;
    }
    
    performe_core_reads(c, stop);

    for(size_t h = (prev_block_height == 0 ? 0 : prev_block_height + 1); h <= top_block_height; ++h)
    {
      block b = AUTO_VAL_INIT(b);
      if (bcs.get_block_by_height(h, b))
      {
        uint64_t generated_money = get_outs_money_amount(b.miner_tx);
        s_generated_money_total.fetch_add(generated_money, std::memory_order_relaxed);
        prev_block_height = h; // update prev_block_height only if get_block_by_height succeeded, retry on the next step otherwise
      }
    }

  }
  
}


class core_checker : public test_core_listener
{
public:
  virtual void before_tx_pushed_to_core(const currency::transaction& tx, const currency::blobdata& blob, currency::core& c, bool invalid_tx = false) override // invalid_tx is true when processing a tx, marked as invalid in a test
  {
  }
  
  virtual void before_block_pushed_to_core(const currency::block& block, const currency::blobdata& blob, currency::core& c) override
  {
  }
};

struct test_context
{
  cct_events_t events;
  core_state_helper core_state_after_generation;
};

namespace boost
{
  namespace serialization
  {
    template<class archive_t>
    void serialize(archive_t & ar, test_context& tc, const unsigned int version)
    {
      ar & tc.events;
      ar & tc.core_state_after_generation;
    }
    template<class archive_t>
    void serialize(archive_t & ar, core_state_helper& csh, const unsigned int version)
    {
      ar & csh.blocks_hashes;
      ar & csh.pool_txs_hashes;
      ar & csh.txs_hashes;
    }
  }
}

bool core_concurrency_test(boost::program_options::variables_map& vm, size_t wthreads, size_t rthreads, size_t blocks_count)
{
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  //epee::debug::get_set_enable_assert(true, false);
  log_space::get_set_need_thread_id(true, true);

  cct_accounts_t accounts(s_wallets_total_count);
  for (auto& a: accounts)
    a.generate();
  

  test_context tc = AUTO_VAL_INIT(tc);
  cct_events_t& events(tc.events);
  core_state_helper& core_state_after_generation(tc.core_state_after_generation);
  core_state_helper core_state_after_playback;
  
  // Stage 1.

  if (!command_line::has_arg(vm, arg_test_core_load_and_replay))
  {
    // Generate events
    clean_data_directory(vm);
    currency::core c(nullptr);
    std::shared_ptr<core_checker> core_listener(new core_checker);

    test_protocol_handler protocol_handler(c, core_listener.get());
    c.set_currency_protocol(&protocol_handler);
    if (!c.init(vm))
    {
      LOG_ERROR("Failed to init core");
      return false;
    }

    test_core_time::init();
    currency::core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
    crc.get_core_time = &test_core_time::get_time;
    crc.tx_pool_min_fee = TESTS_DEFAULT_FEE;
    crc.tx_default_fee = TESTS_DEFAULT_FEE;
    c.get_blockchain_storage().set_core_runtime_config(crc);

    test_node_server p2psrv(protocol_handler);
    bc_services::bc_offers_service offers_service(nullptr);
    currency::core_rpc_server rpc_server(c, p2psrv, offers_service);
    std::shared_ptr<tools::core_fast_rpc_proxy> fast_proxy(new tools::core_fast_rpc_proxy(rpc_server));

    crypto::hash genesis_hash = c.get_block_id_by_height(0);
    cct_wallets_t wallets;
    for (size_t i = 0; i < s_wallets_total_count; ++i)
    {
      std::shared_ptr<tools::wallet2> w(new tools::wallet2());
      w->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
      w->assign_account(accounts[i]);
      w->set_genesis(genesis_hash);
      w->set_core_proxy(fast_proxy);
      wallets.push_back(w);
    }

    bool r = generate_events(c, events, wallets, blocks_count);
    core_state_after_generation.fill(c);
    c.deinit();
    if (!r)
    {
      LOG_ERROR("generate_events failed");
      return false;
    }
  }else
  {
    // Load events
    std::string generated_test_context_file = command_line::get_arg(vm, arg_test_core_load_and_replay);
    bool r = tools::unserialize_obj_from_file(tc, generated_test_context_file);
    CHECK_AND_ASSERT_MES(r, false, "Failed to load test context");
    LOG_PRINT_GREEN("EVENTS SUCCESSEFUL LOADED", LOG_LEVEL_0);
  }

  // Stage 2. 
  if (command_line::has_arg(vm, arg_test_core_prepare_and_store))
  {
    // Store context
    std::string generated_test_context_file = command_line::get_arg(vm, arg_test_core_prepare_and_store);
    bool r = tools::serialize_obj_to_file(tc, generated_test_context_file);
    CHECK_AND_ASSERT_MES(r, false, "Failed to load test context");
    LOG_PRINT_GREEN("EVENTS SUCCESSEFUL STORED", LOG_LEVEL_0);
    return true;
  }
  else
  {
    // Replay events
    clean_data_directory(vm);
    currency::core c(nullptr);
    std::shared_ptr<core_checker> core_listener(new core_checker);

    test_protocol_handler protocol_handler(c, core_listener.get());
    c.set_currency_protocol(&protocol_handler);
    if (!c.init(vm))
    {
      LOG_ERROR("Failed to init core");
      return false;
    }

    test_core_time::init();
    currency::core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
    crc.get_core_time = &test_core_time::get_time;
    crc.tx_pool_min_fee = TESTS_DEFAULT_FEE;
    crc.tx_default_fee = TESTS_DEFAULT_FEE;
    c.get_blockchain_storage().set_core_runtime_config(crc);

    LOG_PRINT("\n\n\n\n\n\n\n", LOG_LEVEL_0);

    LOG_PRINT("Start replaing " << events.size() << " events with " << wthreads << " writing threads and " << rthreads << " reading threads, logging will be switched off now...", LOG_LEVEL_0);
    int log_level_before = log_space::get_set_log_detalisation_level(false);
//    log_space::get_set_log_detalisation_level(true, LOG_LEVEL_SILENT);

    TIME_MEASURE_START_MS(replay_time_ms);

    std::vector<writer_context> writers_contexts(wthreads);

    std::atomic<bool> stop_readers(false);
    std::vector<std::thread> writing_threads, reading_threads;
    for(size_t thread_index = 0; thread_index < wthreads; ++thread_index)
      writing_threads.emplace_back(std::thread(replay_events, std::ref(c), std::cref(events), thread_index, std::ref(writers_contexts[thread_index])));

    for(size_t thread_index = 0; thread_index < rthreads; ++thread_index)
      reading_threads.emplace_back(std::thread(blockchain_reader, std::cref(c), std::ref(stop_readers)));
    
    for(auto& t : writing_threads)
      t.join();

    stop_readers = true;

    for(auto& t : reading_threads)
      t.join();

    TIME_MEASURE_FINISH_MS(replay_time_ms);

    log_space::get_set_log_detalisation_level(true, log_level_before);
    LOG_PRINT("logging switched on", LOG_LEVEL_0);
    LOG_PRINT_YELLOW("Total replay time: " << replay_time_ms << " ms (" << misc_utils::get_time_interval_string(replay_time_ms / (uint64_t)1000) << ") with an average of "
      << replay_time_ms / (events.empty() ? 1 : events.size()) << " ms per event, " << events.size() << " events total", LOG_LEVEL_0);

    core_state_after_playback.fill(c);
    boost::multiprecision::uint128_t already_generated_coins = 0;
    {
      block_extended_info bei = AUTO_VAL_INIT(bei);
      c.get_blockchain_storage().get_block_extended_info_by_hash(c.get_blockchain_storage().get_top_block_id(), bei);
      already_generated_coins = bei.already_generated_coins;
    }
    c.deinit();

    if (rthreads > 0)
    {
      s_generated_money_total = s_generated_money_total / rthreads;
      LOG_PRINT("Generated coins: " << print_money(already_generated_coins) << ", counted by readers (with fee): " << print_money(s_generated_money_total.load()), LOG_LEVEL_0);
    }

    LOG_PRINT("Writers' stats:", LOG_LEVEL_0);
    for (size_t ti = 0; ti < wthreads; ++ti)
    {
      auto& w = writers_contexts[ti];
      LOG_PRINT_L0("writer thread #" << ti <<
        ": blocks added: " << std::setw(5) << w.blocks_added <<
        ", already existed: " << std::setw(5) << w.blocks_already_existed <<
        ", failed: " << std::setw(5) << w.blocks_failed <<
        ", total: " << std::setw(5) << w.blocks_total);
    }

    if (core_state_after_generation == core_state_after_playback)
    {
      LOG_PRINT_GREEN("SUCCESS! Core state is correct!", LOG_LEVEL_0);
      return true;
    }
    else
    {
      LOG_ERROR("Core state after events playback is incorrect!");
      return false;
    }
  }

    
}

