// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chaingen_tests_list.h"
#include "common/command_line.h"
#include "transaction_tests.h"
#include "../../src/gui/qt-daemon/application/core_fast_rpc_proxy.h"
#include "test_core_proxy.h"
#include "currency_core/bc_offers_service.h"
#include "random_helper.h"
#include "core_state_helper.h"

#define TX_BLOBSIZE_CHECKER_LOG_FILENAME "get_object_blobsize(tx).log"

namespace po = boost::program_options;

namespace
{
  const command_line::arg_descriptor<std::string> arg_test_data_path              = {"test-data-path", "", ""};
  const command_line::arg_descriptor<bool>        arg_generate_test_data          = {"generate-test-data", ""};
  const command_line::arg_descriptor<bool>        arg_play_test_data              = {"play-test-data", ""};
  const command_line::arg_descriptor<bool>        arg_generate_and_play_test_data = {"generate-and-play-test-data", ""};
  const command_line::arg_descriptor<bool>        arg_test_transactions           = {"test-transactions", ""};
  const command_line::arg_descriptor<std::string> arg_run_single_test             = {"run-single-test", "" };
  const command_line::arg_descriptor<bool>        arg_enable_debug_asserts        = {"enable-debug-asserts", "" };

  boost::program_options::variables_map g_vm;
}

#define GENERATE(filename, genclass) \
  { \
    test_core_time::init(); \
    test_generator::set_test_gentime_settings_default(); \
    std::vector<test_event_entry> events; \
    genclass g; \
    g.generate(events); \
    if (!tools::serialize_obj_to_file(events, filename)) \
    { \
      std::cout << concolor::magenta << "Failed to serialize data to file: " << filename << concolor::normal << std::endl; \
      throw std::runtime_error("Failed to serialize data to file"); \
    } \
  }

#define PLAY(filename, genclass) \
  if(!do_replay_file<genclass>(filename)) \
  { \
    std::cout << concolor::magenta << "Failed to pass test : " << #genclass << concolor::normal << std::endl; \
    return 1; \
  }

bool clean_data_directory()
{
  std::string config_folder = command_line::get_arg(g_vm, command_line::arg_data_dir);

  static const char* const files[] = { CURRENCY_BLOCKCHAINDATA_FOLDERNAME, CURRENCY_POOLDATA_FOLDERNAME, MINER_CONFIG_FILENAME };
  for (size_t i = 0; i < sizeof files / sizeof files[0]; ++i)
  {
    boost::filesystem::path filename(config_folder + "/" + files[i]);
    if (boost::filesystem::exists(filename))
      CHECK_AND_ASSERT_MES(boost::filesystem::remove_all(filename), false, "boost::filesystem::remove failed to remove this: " << filename);
  }

  return true;
}

template<class genclass>
bool generate_and_play(const char* const genclass_name)
{
  std::vector<test_event_entry> events;
  bool generated = false;
  bool result = true;
  std::cout << concolor::bright_white << "#TEST# " << genclass_name << concolor::normal << std::endl;
  LOG_PRINT2("get_object_blobsize.log", "#TEST# " << genclass_name, LOG_LEVEL_3);

  if (!clean_data_directory())
    return false;

  test_core_time::init();
  test_generator::set_test_gentime_settings_default();
  genclass g;
  try
  {
    generated = g.generate(events);;
  }
  catch (const std::exception& ex)
  {
    LOG_ERROR(genclass_name << " generation failed: what=" << ex.what());
  }
  catch (...)
  {
    LOG_ERROR(genclass_name << " generation failed: generic exception");
  }

  std::cout << concolor::bright_white << "#TEST# " << genclass_name << ": start replaying events" << concolor::normal << std::endl;

  if (generated && do_replay_events(events, g))
  {
    std::cout << concolor::green << "#TEST# Succeeded " << genclass_name << concolor::normal << std::endl;
  }
  else
  {
    std::cout << concolor::magenta << "#TEST# Failed " << genclass_name << concolor::normal << std::endl;
    LOG_PRINT_RED_L0("#TEST# Failed " << genclass_name);
    result = false;
  }
  std::cout << std::endl;
  return result;
}

template<class genclass>
bool gen_and_play_intermitted_by_blockchain_saveload(const char* const genclass_name)
{
  const size_t blockchain_store_events_interval = 7; // after every such number of events blockchain will be stored and then loaded, chosen arbitrary
  bool r = false;

  misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler([&]()
  {
    if (r)
      std::cout << concolor::green << "#TEST# Succeeded " << genclass_name << concolor::normal << std::endl;
    else
      std::cout << concolor::magenta << "#TEST# Failed " << genclass_name << concolor::normal << std::endl;
    std::cout << std::endl;
  });

  std::vector<test_event_entry> events;
  std::cout << concolor::bright_white << "#TEST# " << genclass_name << concolor::normal << std::endl;

  if (!clean_data_directory())
    return false;

  // STAGE 0: generete events
 
  test_core_time::init();
  test_generator::set_test_gentime_settings_default();
  genclass g;
  try
  {
    r = g.generate(events);
  }
  catch (const std::exception& ex)
  {
    LOG_ERROR(genclass_name << " generation failed: what=" << ex.what());
  }
  catch (...)
  {
    LOG_ERROR(genclass_name << " generation failed: generic exception");
  }

  if (!r)
    return false;

  
  // STAGE 1: replay all events normally and remember state of the core
  core_state_helper core_state_before;
  uint64_t core_time = test_core_time::get_time();
  {
    random_state_test_restorer random_restorer; // use it to achive equal random generator's state before Stage 1 and before Stage 2
    r = do_replay_events(events, g, 0, SIZE_MAX, false, [](currency::core&) {}, [&core_state_before](currency::core& c) { core_state_before.fill(c); });
    if (!r)
      return false;
    std::cout << concolor::green << "#TEST# " << genclass_name << ": STAGE 1: Succeeded (all events were played back normally)" << concolor::normal << std::endl;
    // random_restorer::dtor is expected to be invoked here
  }

  if (!clean_data_directory())
    return false;

  // STAGE 2: replay all events chunk by chunk, storing and loading blockchain every time. Then remember state of the core.
  core_state_helper core_state_after;
  currency::core_runtime_config crc = currency::get_default_core_runtime_config(); // to store runtime config between chunks replaying
  // Note: blockchain_storage doesn't dump alt blocks onto disk by design. It means on each do_replay_events() call blockchain_storage will load the blockchain from file where alt blocks are absent.
  // In the production environment it is not a issue, because node sync protocol will always start syncing from the most recent common block. Thus a node can't stuck on altchain.
  // Here, in this specific case we DO need to keep all the altchains, just because we have no aid from sync protocol in tests. Removing alt blocks between events chunks would break the tests.
  // So, here is a coniainer for them:
  currency::blockchain_storage::alt_chain_container alt_chains;
  currency::checkpoints cps;

  for (size_t i = 0; i <= events.size() / blockchain_store_events_interval; ++i)
  {
    size_t ev_idx_from = blockchain_store_events_interval * i;
    size_t ev_idx_to = std::min(blockchain_store_events_interval * (i + 1) - 1, events.size() - 1);
    bool last_iter = (ev_idx_to == events.size() - 1);
    
    std::cout << ENDL << ENDL << concolor::yellow << "===== #TEST# " << genclass_name << ": playing chunk of events from " << ev_idx_from << " to " << ev_idx_to << " (of " << events.size() << ")" << concolor::normal << ENDL;
    
    auto after_init_cb = [crc, &alt_chains, &cps, core_time](currency::core& c)
    {
      c.get_blockchain_storage().set_core_runtime_config(crc);           // restore runtime config
      c.get_blockchain_storage().set_alternative_chains(alt_chains);  // restore alt chains
      c.get_blockchain_storage().get_checkpoints() = cps;                // restore checkpoints
      test_core_time::adjust(core_time);                                 // restore core time
    };

    auto before_deinit_cb = [&core_state_after, last_iter, &crc, &alt_chains, &cps, &core_time](currency::core& c)
    {
      core_time = test_core_time::get_time();                            // save core time
      crc = c.get_blockchain_storage().get_core_runtime_config();        // save runtime config
      c.get_blockchain_storage().get_alternative_chains(alt_chains);  // save altchains
      cps = c.get_blockchain_storage().get_checkpoints();                // save checkpoints
      if (last_iter)
        core_state_after.fill(c);
    };

    r = do_replay_events(events, g, ev_idx_from, ev_idx_to, true, after_init_cb, before_deinit_cb);
    if (!r)
      return false;
  }

  // STAGE 3: compare core state from stage 1 and 2
  r = core_state_before == core_state_after;

  return r;
}


#define GENERATE_AND_PLAY(genclass)                                                                        \
  if(!postponed_tests.count(#genclass) && (run_single_test.empty() || run_single_test == #genclass))                                              \
  {                                                                                                        \
    TIME_MEASURE_START_MS(t);                                                                              \
    ++tests_count;                                                                                         \
    if (!generate_and_play<genclass>(#genclass))                                                           \
    {                                                                                                      \
      failed_tests.insert(#genclass);                                                                      \
      LOCAL_ASSERT(false);                                                                                 \
    }                                                                                                      \
    TIME_MEASURE_FINISH_MS(t);                                                                             \
    tests_running_time.push_back(std::make_pair(#genclass, t));                                            \
  }

#define GENERATE_AND_PLAY_INTERMITTED_BY_BLOCKCHAIN_SAVELOAD(genclass)                                     \
  if(run_single_test.empty() || run_single_test == #genclass)                                              \
  {                                                                                                        \
    const char* testname = #genclass " (BC saveload)";                                                     \
    TIME_MEASURE_START_MS(t);                                                                              \
    ++tests_count;                                                                                         \
    if (!gen_and_play_intermitted_by_blockchain_saveload<genclass>(testname))                              \
    {                                                                                                      \
      failed_tests.insert(testname);                                                                       \
      LOCAL_ASSERT(false);                                                                                 \
    }                                                                                                      \
    TIME_MEASURE_FINISH_MS(t);                                                                             \
    tests_running_time.push_back(std::make_pair(testname, t));                                             \
  }

//#define GENERATE_AND_PLAY(genclass) GENERATE_AND_PLAY_INTERMITTED_BY_BLOCKCHAIN_SAVELOAD(genclass)


#define CALL_TEST(test_name, function)                                                                     \
  {                                                                                                        \
    if(!function())                                                                                        \
    {                                                                                                      \
      std::cout << concolor::magenta << "#TEST# Failed " << test_name << concolor::normal << std::endl;    \
      LOCAL_ASSERT(false);                                                                                 \
      return 1;                                                                                            \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      std::cout << concolor::green << "#TEST# Succeeded " << test_name << concolor::normal << std::endl;   \
    }                                                                                                      \
  }

class tx_blobsize_checker : public test_core_listener
{
public:
  bool m_status;
  tx_blobsize_checker() : m_status(true) {}
  // test_core_listener
  virtual void before_tx_pushed_to_core(const currency::transaction& tx, const currency::blobdata& blob, currency::core& c, bool invalid_tx) override
  {
    if (invalid_tx)
      return; // skip certainly invalid txs

    bool b_cp = c.get_blockchain_storage().is_in_checkpoint_zone();
    if (b_cp && tx.signatures.empty() && tx.attachment.empty())
      return; // skip pruned txs in CP zone

    size_t tx_expected_blob_size = get_object_blobsize(tx);
    if (!b_cp && tx_expected_blob_size != blob.size())
    {
      size_t prefix_blobsize = currency::get_object_blobsize(static_cast<const currency::transaction_prefix&>(tx));
      currency::blobdata prefix_blob = t_serializable_object_to_blob(static_cast<const currency::transaction_prefix&>(tx));

      std::stringstream s;
      s << "CP zone: " << b_cp << ", transaction: " << get_transaction_hash(tx) << ENDL <<
        "prefix_blobsize=" << prefix_blobsize << " prefix_blob.size()=" << prefix_blob.size() << " tx_expected_blob_size=" << tx_expected_blob_size << " blob.size()=" << blob.size();
      LOG_PRINT2(TX_BLOBSIZE_CHECKER_LOG_FILENAME, s.str() << ENDL << obj_to_json_str(tx) << ENDL << ENDL, LOG_LEVEL_3);
      LOG_PRINT_RED_L0("transaction blobsize check failed: " << s.str());
      m_status = false;
    }
    else
    {
      // tx seems to be correct (  tx_expected_blob_size == blob.size()  ) or BCS is in CP zone, prune sigs and attachments and check again
      currency::transaction pruned_tx = tx;
      pruned_tx.signatures.clear();
      pruned_tx.attachment.clear();
      size_t pruned_tx_expected_blob_size = get_object_blobsize(pruned_tx);

      if (pruned_tx_expected_blob_size != tx_expected_blob_size)
      {
        size_t prefix_blobsize = currency::get_object_blobsize(static_cast<const currency::transaction_prefix&>(tx));
        currency::blobdata prefix_blob = t_serializable_object_to_blob(static_cast<const currency::transaction_prefix&>(tx));
        std::stringstream s;
        s << "PRUNED TX MISSMATCH! CP zone: " << b_cp << ", transaction: " << get_transaction_hash(tx) << ENDL <<
          "pruned_tx_expected_blob_size=" << pruned_tx_expected_blob_size << " prefix_blobsize=" << prefix_blobsize << " prefix_blob.size()=" << prefix_blob.size() << " tx_expected_blob_size=" << tx_expected_blob_size << " blob.size()=" << blob.size();
        LOG_PRINT2(TX_BLOBSIZE_CHECKER_LOG_FILENAME, s.str() << ENDL << obj_to_json_str(tx) << ENDL << ENDL, LOG_LEVEL_3);
        LOG_PRINT_RED_L0("transaction blobsize check failed: " << s.str());
        m_status = false;
      }
    }
  }
};

/************************************************************************/
/* push_core_event_visitor                                              */
/************************************************************************/
template<class t_test_class>
struct push_core_event_visitor : public boost::static_visitor<bool>
{
private:
  currency::core& m_c;
  const std::vector<test_event_entry>& m_events;
  t_test_class& m_validator;
  size_t m_ev_index;
  test_core_listener* m_core_listener;

  bool m_txs_kept_by_block;
  bool m_skip_txs_blobsize_check;

public:
  push_core_event_visitor(currency::core& c, const std::vector<test_event_entry>& events, t_test_class& validator, test_core_listener* core_listener)
    : m_c(c)
    , m_events(events)
    , m_validator(validator)
    , m_ev_index(0)
    , m_txs_kept_by_block(false)
    , m_skip_txs_blobsize_check(false)
    , m_core_listener(core_listener)
  {
  }

  void event_index(size_t ev_index)
  {
    m_ev_index = ev_index;
  }

  bool handle_tx(const currency::transaction& tx, const currency::blobdata& tx_blob) const
  {
    if (m_core_listener)
    {
      bool invalid_tx = m_ev_index > 0 && m_events[m_ev_index - 1].type().hash_code() == typeid(callback_entry).hash_code() && boost::get<callback_entry>(m_events[m_ev_index - 1]).callback_name == "mark_invalid_tx";
      m_core_listener->before_tx_pushed_to_core(tx, tx_blob, m_c, invalid_tx || m_skip_txs_blobsize_check);
    }

    size_t pool_size = m_c.get_pool_transactions_count();
    currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    m_c.handle_incoming_tx(tx_blob, tvc, m_txs_kept_by_block);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();
    bool r = check_tx_verification_context(tvc, tx_added, m_ev_index, tx, m_validator);
    LOCAL_ASSERT(r);
    CHECK_AND_NO_ASSERT_MES(r, false, "tx verification context check failed");
    return true;
  }

  bool handle_block(const currency::block& b, const currency::blobdata& blob_blk) const
  {
    if (m_core_listener)
      m_core_listener->before_block_pushed_to_core(b, blob_blk, m_c);

    currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    m_c.handle_incoming_block(blob_blk, bvc);
    bool r = check_block_verification_context(bvc, m_ev_index, b, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");
    return true;
  }

  bool operator()(const event_visitor_settings& settings)
  {
    log_event("event_visitor_settings");

    if (settings.valid_mask & event_visitor_settings::set_txs_kept_by_block)
      m_txs_kept_by_block = settings.txs_kept_by_block;
    if (settings.valid_mask & event_visitor_settings::set_skip_txs_blobsize_check)
      m_skip_txs_blobsize_check = settings.skip_txs_blobsize_check;

    return true;
  }

  bool operator()(const currency::transaction& tx) const
  {
    log_event("currency::transaction");

    return handle_tx(tx, t_serializable_object_to_blob(tx));
  }

  bool operator()(const currency::block& b) const
  {
    log_event("currency::block");

    return handle_block(b, t_serializable_object_to_blob(b));
  }

  bool operator()(const event_special_block& sp) const
  {
    if (sp.special_flags & event_special_block::flag_skip)
    {
      log_event("currency::block -- SKIPPED!");
      return true;
    }
    CHECK_AND_NO_ASSERT_MES(sp.special_flags == 0, false, "unsupported special flag");

    // fallback to normal block handling
    return operator()(sp.block);
  }

  bool operator()(const callback_entry& cb) const
  {
    log_event(std::string("callback_entry ") + cb.callback_name);
    return m_validator.verify(cb.callback_name, m_c, m_ev_index, m_events);
  }

  bool operator()(const currency::account_base& ab) const
  {
    log_event("currency::account_base");
    return true;
  }

  bool operator()(const serialized_block& sr_block) const
  {
    log_event("serialized_block");

    currency::block b = AUTO_VAL_INIT(b);
    bool r = ::t_unserializable_object_from_blob(b, sr_block.data);
    CHECK_AND_ASSERT_MES(r, false, "failed to deserialize block from blob");
    return handle_block(b, sr_block.data);
  }

  bool operator()(const serialized_transaction& sr_tx) const
  {
    log_event("serialized_transaction");

    currency::transaction tx = AUTO_VAL_INIT(tx);
    bool r = ::t_unserializable_object_from_blob(tx, sr_tx.data);
    CHECK_AND_ASSERT_MES(r, false, "failed, to deserialize tx from blob");
    return handle_tx(tx, sr_tx.data);
  }

  bool operator()(const event_core_time& e) const
  {
    test_core_time::adjust(e.time_shift, e.relative);
    std::stringstream ss; ss << "core_time (shift: " << e.time_shift << ", now is: " << test_core_time::get_time() << ")";
    log_event(ss.str());

    // make sure the core is using special time function
    currency::core_runtime_config crc = m_c.get_blockchain_storage().get_core_runtime_config();
    CHECK_AND_ASSERT_MES(crc.get_core_time == &test_core_time::get_time, false, "Time function in the core runtime config is wrong. Have you rewritten the config accidentally?");
    return true;
  }

private:
  void log_event(const std::string& event_type) const
  {
    std::cout << concolor::yellow << "=== EVENT # " << m_ev_index << ": " << event_type << concolor::normal << std::endl;
  }
};
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool replay_events_through_core(currency::core& cr, const std::vector<test_event_entry>& events, t_test_class& validator, test_core_listener* core_listener, size_t event_index_from = 0, size_t event_index_to = SIZE_MAX)
{
  TRY_ENTRY();

  bool r = true;
  push_core_event_visitor<t_test_class> visitor(cr, events, validator, core_listener);
  event_index_to = std::min(event_index_to, events.size() - 1);
  for (size_t i = event_index_from; i <= event_index_to && r; ++i)
  {
    if (i == 0)
    {
      CHECK_AND_ASSERT_MES(typeid(currency::block) == events[i].type(), false, "First event must be genesis block creation");
      cr.set_genesis_block(boost::get<currency::block>(events[i]));
      continue;
    }

    visitor.event_index(i);
    r = boost::apply_visitor(visitor, events[i]);
  }

  return r;

  CATCH_ENTRY_L0("replay_events_through_core", false);
}
//--------------------------------------------------------------------------
std::shared_ptr<boost::program_options::variables_map> prepare_variables_map()
{
  boost::program_options::options_description desc("Allowed options");
  currency::core::init_options(desc);
  command_line::add_arg(desc, command_line::arg_data_dir);
  std::shared_ptr<boost::program_options::variables_map> vm(new boost::program_options::variables_map);
  bool r = command_line::handle_error_helper(desc, [&]()
  {
    boost::program_options::store(boost::program_options::basic_parsed_options<char>(&desc), *vm.get());
    boost::program_options::notify(*vm.get());
    return true;
  });

  if (!r)
    return nullptr;

  return vm;
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_events(const std::vector<test_event_entry>& events, t_test_class& validator,
  size_t event_index_from = 0, size_t event_index_to = SIZE_MAX, bool deinit_core = true)
{
  auto stub = [](currency::core&) {};
  return do_replay_events(events, validator, event_index_from, event_index_to, deinit_core, stub, stub);
}

template<class t_test_class, typename cb_1_t, typename cb_2_t>
inline bool do_replay_events(const std::vector<test_event_entry>& events, t_test_class& validator,
  size_t event_index_from, size_t event_index_to, bool deinit_core, cb_1_t after_init_cb, cb_2_t before_deinit_cb)
{
  std::shared_ptr<tx_blobsize_checker> core_listener = nullptr;
  // todo: make tx_blobsize_checker creation conditional (some test may want not to enable it)
  {
    std::shared_ptr<tx_blobsize_checker> checker(new tx_blobsize_checker);
    core_listener = checker;
  }

  bool r = false;
  currency::core c(nullptr);
  test_protocol_handler protocol_handler(c, core_listener.get());
  protocol_handler.init(g_vm);
  c.set_currency_protocol(&protocol_handler);
  if (!c.init(g_vm))
  {
    std::cout << concolor::magenta << "Failed to init core" << concolor::normal << std::endl;
    return false;
  }

  after_init_cb(c);

  bc_services::bc_offers_service offers_service(nullptr);
  c.get_blockchain_storage().get_attachment_services_manager().add_service(&offers_service);

  currency::core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  crc.get_core_time = &test_core_time::get_time;
  crc.tx_pool_min_fee = TESTS_DEFAULT_FEE;
  crc.tx_default_fee = TESTS_DEFAULT_FEE;
  c.get_blockchain_storage().set_core_runtime_config(crc);

  if (validator.need_core_proxy())
  {
    test_node_server p2psrv(protocol_handler);
    currency::core_rpc_server rpc_server(c, p2psrv, offers_service);
    std::shared_ptr<tools::core_fast_rpc_proxy> fast_proxy(new tools::core_fast_rpc_proxy(rpc_server));
    validator.set_core_proxy(fast_proxy);
    r = replay_events_through_core<t_test_class>(c, events, validator, core_listener.get(), event_index_from, event_index_to); // call it here to make easier objects lifetime control; we don't need all that proxy stuff after this func has returned
    validator.set_core_proxy(nullptr);
    CHECK_AND_ASSERT_MES(fast_proxy.unique(), false, "Seems that test class has wrongly given core proxy to a 3rd party");
  }
  else
  {
    r = replay_events_through_core<t_test_class>(c, events, validator, core_listener.get(), event_index_from, event_index_to);
  }

  before_deinit_cb(c);

  c.deinit(); // always deinitialize the core
  protocol_handler.deinit();

  if (!core_listener->m_status)
    LOG_PRINT_RED("transaction blobsize check failed. See " << TX_BLOBSIZE_CHECKER_LOG_FILENAME << " for details.", LOG_LEVEL_0);
  
  return r && core_listener->m_status;
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_file(const std::string& filename)
{
  std::vector<test_event_entry> events;
  if (!tools::unserialize_obj_from_file(events, filename))
  {
    std::cout << concolor::magenta << "Failed to deserialize data from file: " << filename << concolor::normal << std::endl;
    return false;
  }
  t_test_class g;
  return do_replay_events(events, g);
}


int main(int argc, char* argv[])
{
  TRY_ENTRY();
  string_tools::set_module_name_and_folder(argv[0]);

  //set up logging options
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_4);
  
  log_space::log_singletone::add_logger(LOGGER_FILE, 
    log_space::log_singletone::get_default_log_file().c_str(), 
    log_space::log_singletone::get_default_log_folder().c_str());

  log_space::log_singletone::enable_channels("core,currency_protocol,tx_pool,p2p,wallet");

  tools::signal_handler::install_fatal([](int sig_number, void* address) {
    LOG_ERROR("\n\nFATAL ERROR\nsig: " << sig_number << ", address: " << address);
    std::fflush(nullptr); // all open output streams are flushed
  });

  po::options_description desc_options("Allowed options");
  command_line::add_arg(desc_options, command_line::arg_help);
  command_line::add_arg(desc_options, arg_test_data_path);
  command_line::add_arg(desc_options, arg_generate_test_data);
  command_line::add_arg(desc_options, arg_play_test_data);
  command_line::add_arg(desc_options, arg_generate_and_play_test_data);
  command_line::add_arg(desc_options, arg_test_transactions);
  command_line::add_arg(desc_options, arg_run_single_test);  
  command_line::add_arg(desc_options, arg_enable_debug_asserts);
  command_line::add_arg(desc_options, command_line::arg_data_dir, std::string("."));

  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), g_vm);
    po::notify(g_vm);
    return true;
  });
  if (!r)
    return 1;

  if (command_line::get_arg(g_vm, command_line::arg_help))
  {
    std::cout << desc_options << std::endl;
    return 0;
  }

  size_t tests_count = 0;
  size_t serious_failures_count = 0;
  std::set<std::string> failed_tests;
  std::string tests_folder = command_line::get_arg(g_vm, arg_test_data_path);
  typedef std::vector<std::pair<std::string, uint64_t>> tests_running_time_t;
  tests_running_time_t tests_running_time;
  if (command_line::get_arg(g_vm, arg_generate_test_data))
  {
    GENERATE("chain001.dat", gen_simple_chain_001);
  }
  else if (command_line::get_arg(g_vm, arg_play_test_data))
  {
    PLAY("chain001.dat", gen_simple_chain_001);
  }
  else if (command_line::get_arg(g_vm, arg_test_transactions))
  {
    CALL_TEST("TRANSACTIONS TESTS", test_transactions);
  }
  else
  {
    epee::debug::get_set_enable_assert(true, command_line::get_arg(g_vm, arg_enable_debug_asserts)); // don't comment out this: many tests have normal-negative checks (i.e. tx with invalid amount shouldn't be created), so be ready for MANY assertion breaks

    std::string run_single_test;
    if (command_line::has_arg(g_vm, arg_run_single_test))
    {
      run_single_test = command_line::get_arg(g_vm, arg_run_single_test);
    }
    
    if (run_single_test.empty())
    {
      CALL_TEST("Random text test", get_random_text_test);
      CALL_TEST("Random state manipulation test", random_state_manupulation_test);
      CALL_TEST("Random evenness test", random_evenness_test);
      CALL_TEST("TRANSACTIONS TESTS", test_transactions);
      CALL_TEST("check_allowed_types_in_variant_container() test", check_allowed_types_in_variant_container_test);
      CALL_TEST("check_u8_str_case_funcs", check_u8_str_case_funcs);
      CALL_TEST("chec_u8_str_matching", chec_u8_str_matching);
    }

    //CALL_TEST("check_hash_and_difficulty_monte_carlo_test", check_hash_and_difficulty_monte_carlo_test); // it's rather an experiment with unclean results than a solid test, for further research...
    std::set<std::string> postponed_tests;

    // Postponed tests - tests that may fail for the time being (believed that it's a serious issue and should be fixed later for some reason).
    // In a perfect world this list is empty.
#define MARK_TEST_AS_POSTPONED(genclass) postponed_tests.insert(#genclass)
    MARK_TEST_AS_POSTPONED(gen_checkpoints_reorganize);
    MARK_TEST_AS_POSTPONED(gen_alias_update_after_addr_changed);
    MARK_TEST_AS_POSTPONED(gen_alias_blocking_reg_by_invalid_tx);
    MARK_TEST_AS_POSTPONED(gen_alias_blocking_update_by_invalid_tx);
    MARK_TEST_AS_POSTPONED(gen_wallet_fake_outputs_randomness);
    MARK_TEST_AS_POSTPONED(gen_wallet_fake_outputs_not_enough);
    MARK_TEST_AS_POSTPONED(gen_wallet_spending_coinstake_after_minting);
    MARK_TEST_AS_POSTPONED(gen_wallet_fake_outs_while_having_too_little_own_outs);
    MARK_TEST_AS_POSTPONED(gen_uint_overflow_1);

#undef MARK_TEST_AS_POSTPONED


    GENERATE_AND_PLAY(multisig_wallet_test);
    GENERATE_AND_PLAY(multisig_wallet_test_many_dst);
    GENERATE_AND_PLAY(multisig_wallet_heterogenous_dst);
    GENERATE_AND_PLAY(multisig_wallet_same_dst_addr);
    GENERATE_AND_PLAY(multisig_wallet_ms_to_ms);
    GENERATE_AND_PLAY(multisig_minimum_sigs);
    GENERATE_AND_PLAY(multisig_and_fake_outputs);
    GENERATE_AND_PLAY(multisig_and_unlock_time);
    GENERATE_AND_PLAY(multisig_and_coinbase);
    GENERATE_AND_PLAY(multisig_with_same_id_in_pool);
    GENERATE_AND_PLAY(multisig_and_checkpoints);
    GENERATE_AND_PLAY(multisig_and_checkpoints_bad_txs);
    GENERATE_AND_PLAY(multisig_and_altchains);
    GENERATE_AND_PLAY(multisig_out_make_and_spent_in_altchain);
    GENERATE_AND_PLAY(multisig_unconfirmed_transfer_and_multiple_scan_pool_calls);
    GENERATE_AND_PLAY(multisig_out_spent_in_altchain_case_b4);

    GENERATE_AND_PLAY(ref_by_id_basics);
    GENERATE_AND_PLAY(ref_by_id_mixed_inputs_types);

    GENERATE_AND_PLAY(escrow_wallet_test);
    GENERATE_AND_PLAY(escrow_w_and_fake_outputs);
    GENERATE_AND_PLAY(escrow_incorrect_proposal);
    GENERATE_AND_PLAY(escrow_proposal_expiration);
    GENERATE_AND_PLAY(escrow_proposal_and_accept_expiration);
    GENERATE_AND_PLAY(escrow_incorrect_proposal_acceptance);
    GENERATE_AND_PLAY(escrow_custom_test);
    GENERATE_AND_PLAY(escrow_incorrect_cancel_proposal);
    GENERATE_AND_PLAY(escrow_proposal_not_enough_money);
    GENERATE_AND_PLAY(escrow_cancellation_and_tx_order);
    GENERATE_AND_PLAY(escrow_cancellation_proposal_expiration);
    GENERATE_AND_PLAY(escrow_cancellation_acceptance_expiration);
    // GENERATE_AND_PLAY(escrow_proposal_acceptance_in_alt_chain); -- work in progress
    GENERATE_AND_PLAY(escrow_zero_amounts);

    GENERATE_AND_PLAY(escrow_altchain_meta_test<0>);
    GENERATE_AND_PLAY(escrow_altchain_meta_test<1>);
    GENERATE_AND_PLAY(escrow_altchain_meta_test<2>);
    GENERATE_AND_PLAY(escrow_altchain_meta_test<3>);
    GENERATE_AND_PLAY(escrow_altchain_meta_test<4>);
    GENERATE_AND_PLAY(escrow_altchain_meta_test<5>);
    GENERATE_AND_PLAY(escrow_altchain_meta_test<6>);
    GENERATE_AND_PLAY(escrow_altchain_meta_test<7>);
    GENERATE_AND_PLAY(escrow_altchain_meta_test<8>);
    static_assert(escrow_altchain_meta_test_data<9>::empty_marker, ""); // make sure there are no sub-tests left

    GENERATE_AND_PLAY(offers_expiration_test);
    GENERATE_AND_PLAY(offers_tests);
    GENERATE_AND_PLAY(offers_filtering_1);
    GENERATE_AND_PLAY(offers_handling_on_chain_switching);
    GENERATE_AND_PLAY(offer_removing_and_selected_output);
    GENERATE_AND_PLAY(offers_multiple_update);
    GENERATE_AND_PLAY(offer_sig_validity_in_update_and_cancel);
    GENERATE_AND_PLAY(offer_lifecycle_via_tx_pool);
    GENERATE_AND_PLAY(offers_updating_hack);
    GENERATE_AND_PLAY(offer_cancellation_with_zero_fee);

    GENERATE_AND_PLAY(gen_crypted_attachments);
    GENERATE_AND_PLAY(gen_checkpoints_attachments_basic);
    GENERATE_AND_PLAY(gen_checkpoints_invalid_keyimage);
    GENERATE_AND_PLAY(gen_checkpoints_altblock_before_and_after_cp);
    GENERATE_AND_PLAY(gen_checkpoints_block_in_future);
    GENERATE_AND_PLAY(gen_checkpoints_altchain_far_before_cp);
    GENERATE_AND_PLAY(gen_checkpoints_block_in_future_after_cp);
    GENERATE_AND_PLAY(gen_checkpoints_prun_txs_after_blockchain_load);
    GENERATE_AND_PLAY(gen_checkpoints_reorganize);
    GENERATE_AND_PLAY(gen_checkpoints_pos_validation_on_altchain);
    GENERATE_AND_PLAY(gen_no_attchments_in_coinbase);
    GENERATE_AND_PLAY(gen_no_attchments_in_coinbase_gentime);

    GENERATE_AND_PLAY(gen_alias_tests);
    GENERATE_AND_PLAY(gen_alias_strange_data);
    GENERATE_AND_PLAY(gen_alias_concurrency_with_switch);
    GENERATE_AND_PLAY(gen_alias_same_alias_in_tx_pool);   
    GENERATE_AND_PLAY(gen_alias_switch_and_tx_pool);
    GENERATE_AND_PLAY(gen_alias_update_after_addr_changed);
    GENERATE_AND_PLAY(gen_alias_blocking_reg_by_invalid_tx);
    GENERATE_AND_PLAY(gen_alias_blocking_update_by_invalid_tx);
    GENERATE_AND_PLAY(gen_alias_reg_with_locked_money);
    GENERATE_AND_PLAY(gen_alias_too_small_reward);
    GENERATE_AND_PLAY(gen_alias_too_much_reward);
    GENERATE_AND_PLAY(gen_alias_tx_no_outs);
    GENERATE_AND_PLAY(gen_alias_switch_and_check_block_template);
    GENERATE_AND_PLAY(gen_alias_too_many_regs_in_block_template);
    GENERATE_AND_PLAY(gen_alias_update_for_free);
    GENERATE_AND_PLAY(gen_alias_in_coinbase);

    GENERATE_AND_PLAY(gen_wallet_basic_transfer);
    GENERATE_AND_PLAY(gen_wallet_refreshing_on_chain_switch);
    GENERATE_AND_PLAY(gen_wallet_refreshing_on_chain_switch_2);
    GENERATE_AND_PLAY(gen_wallet_unconfirmed_tx_from_tx_pool);
    GENERATE_AND_PLAY(gen_wallet_save_load_and_balance);
    GENERATE_AND_PLAY(gen_wallet_mine_pos_block);
    GENERATE_AND_PLAY(gen_wallet_unconfirmed_outdated_tx);
    GENERATE_AND_PLAY(gen_wallet_unlock_by_block_and_by_time);
    GENERATE_AND_PLAY(gen_wallet_payment_id);
    GENERATE_AND_PLAY(gen_wallet_oversized_payment_id);
    GENERATE_AND_PLAY(gen_wallet_transfers_and_outdated_unconfirmed_txs);
    GENERATE_AND_PLAY(gen_wallet_transfers_and_chain_switch);
    GENERATE_AND_PLAY(gen_wallet_decrypted_attachments);
    GENERATE_AND_PLAY(gen_wallet_alias_and_unconfirmed_txs);
    GENERATE_AND_PLAY(gen_wallet_alias_via_special_wallet_funcs);
    GENERATE_AND_PLAY(gen_wallet_fake_outputs_randomness);
    GENERATE_AND_PLAY(gen_wallet_fake_outputs_not_enough);
    GENERATE_AND_PLAY(gen_wallet_offers_basic);
    GENERATE_AND_PLAY(gen_wallet_offers_size_limit);
    GENERATE_AND_PLAY(gen_wallet_dust_to_account);
    GENERATE_AND_PLAY(gen_wallet_selecting_pos_entries);
    GENERATE_AND_PLAY(gen_wallet_spending_coinstake_after_minting);
    GENERATE_AND_PLAY(gen_wallet_fake_outs_while_having_too_little_own_outs);
    // GENERATE_AND_PLAY(premine_wallet_test);  // tests premine wallets; wallet files nedded; by demand only
    GENERATE_AND_PLAY(mined_balance_wallet_test);
    GENERATE_AND_PLAY(wallet_outputs_with_same_key_image);
    GENERATE_AND_PLAY(wallet_unconfirmed_tx_expiration);
    GENERATE_AND_PLAY(wallet_unconfimed_tx_balance);

    GENERATE_AND_PLAY(wallet_rpc_integrated_address);
    GENERATE_AND_PLAY(wallet_rpc_integrated_address_transfer);
    GENERATE_AND_PLAY(wallet_chain_switch_with_spending_the_same_ki);

    // GENERATE_AND_PLAY(emission_test); // simulate 1 year of blockchain, too long run (1 y ~= 1 hr), by demand only
    // LOG_ERROR2("print_reward_change_first_blocks.log", currency::print_reward_change_first_blocks(525601).str()); // outputs first 1 year of blocks' rewards (simplier)

    // pos tests
    GENERATE_AND_PLAY_INTERMITTED_BY_BLOCKCHAIN_SAVELOAD(gen_pos_basic_tests);
    // GENERATE_AND_PLAY(gen_pos_basic_tests); -- commented as this test is run intermittedly by previous line; uncomment if necessary (ex. for debugging)
    GENERATE_AND_PLAY(gen_pos_coinstake_already_spent);
    GENERATE_AND_PLAY(gen_pos_incorrect_timestamp);
    GENERATE_AND_PLAY(gen_pos_too_early_pos_block);
    GENERATE_AND_PLAY(gen_pos_extra_nonce);
    GENERATE_AND_PLAY(gen_pos_min_allowed_height);
    GENERATE_AND_PLAY(gen_pos_invalid_coinbase);
    // GENERATE_AND_PLAY(pos_wallet_minting_same_amount_diff_outs); // Long test! Takes ~10 hours to simulate 6000 blocks on 2015 middle-end computer
    //GENERATE_AND_PLAY(pos_emission_test); // Long test! by demand only
    GENERATE_AND_PLAY(pos_wallet_big_block_test);
    GENERATE_AND_PLAY(block_template_against_txs_size);
    GENERATE_AND_PLAY(pos_altblocks_validation);

    // alternative blocks and generic chain-switching tests
    GENERATE_AND_PLAY(gen_chain_switch_pow_pos);
    GENERATE_AND_PLAY(pow_pos_reorganize_specific_case);
    GENERATE_AND_PLAY(gen_chain_switch_1);
    GENERATE_AND_PLAY(bad_chain_switching_with_rollback);
    GENERATE_AND_PLAY(chain_switching_and_tx_with_attachment_blobsize);
    GENERATE_AND_PLAY(chain_switching_when_gindex_spent_in_both_chains);
    GENERATE_AND_PLAY(alt_chain_coins_pow_mined_then_spent);
    GENERATE_AND_PLAY(gen_simple_chain_split_1);
    GENERATE_AND_PLAY(alt_blocks_validation_and_same_new_amount_in_two_txs);
    GENERATE_AND_PLAY(alt_blocks_with_the_same_txs);

    // miscellaneous tests
    GENERATE_AND_PLAY(test_blockchain_vs_spent_keyimges);
    GENERATE_AND_PLAY(test_blockchain_vs_spent_multisig_outs);
    GENERATE_AND_PLAY(block_template_vs_invalid_txs_from_pool);
    GENERATE_AND_PLAY(cumulative_difficulty_adjustment_test);
    GENERATE_AND_PLAY(cumulative_difficulty_adjustment_test);
    GENERATE_AND_PLAY(cumulative_difficulty_adjustment_test_alt);
    GENERATE_AND_PLAY(prun_ring_signatures);
    GENERATE_AND_PLAY(gen_simple_chain_001);
    GENERATE_AND_PLAY(one_block);
    GENERATE_AND_PLAY(gen_ring_signature_1);
    GENERATE_AND_PLAY(gen_ring_signature_2);
    //GENERATE_AND_PLAY(gen_ring_signature_big); // Takes up to XXX hours (if CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10)

    // tests for outputs mixing in
    GENERATE_AND_PLAY(get_random_outs_test);
    GENERATE_AND_PLAY(mix_attr_tests);
    GENERATE_AND_PLAY(mix_in_spent_outs);

    // Block verification tests
    GENERATE_AND_PLAY(gen_block_big_major_version);
    GENERATE_AND_PLAY(gen_block_big_minor_version);
    GENERATE_AND_PLAY(gen_block_ts_not_checked);
    GENERATE_AND_PLAY(gen_block_ts_in_past);
    GENERATE_AND_PLAY(gen_block_ts_in_future);
    //GENERATE_AND_PLAY(gen_block_invalid_prev_id); disabled becouse impossible to generate text chain with wrong prev_id - pow hash not works without chaining
    GENERATE_AND_PLAY(gen_block_invalid_nonce);
    GENERATE_AND_PLAY(gen_block_no_miner_tx);
    GENERATE_AND_PLAY(gen_block_unlock_time_is_low);
    GENERATE_AND_PLAY(gen_block_unlock_time_is_high);
    GENERATE_AND_PLAY(gen_block_unlock_time_is_timestamp_in_past);
    GENERATE_AND_PLAY(gen_block_unlock_time_is_timestamp_in_future);
    GENERATE_AND_PLAY(gen_block_height_is_low);
    GENERATE_AND_PLAY(gen_block_height_is_high);
    GENERATE_AND_PLAY(gen_block_miner_tx_has_2_tx_gen_in);
    GENERATE_AND_PLAY(gen_block_miner_tx_has_2_in);
    GENERATE_AND_PLAY(gen_block_miner_tx_with_txin_to_key);
    GENERATE_AND_PLAY(gen_block_miner_tx_out_is_small);
    GENERATE_AND_PLAY(gen_block_miner_tx_out_is_big);
    GENERATE_AND_PLAY(gen_block_miner_tx_has_no_out);
    GENERATE_AND_PLAY(gen_block_miner_tx_has_out_to_alice);
    GENERATE_AND_PLAY(gen_block_has_invalid_tx);
    GENERATE_AND_PLAY(gen_block_is_too_big);
    //GENERATE_AND_PLAY(gen_block_invalid_binary_format); // Takes up to 3 hours, if CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 500, up to 30 minutes, if CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10


    
    // Transaction verification tests
    GENERATE_AND_PLAY(gen_broken_attachments);
    GENERATE_AND_PLAY(gen_tx_big_version);
    GENERATE_AND_PLAY(gen_tx_unlock_time);
    GENERATE_AND_PLAY(gen_tx_input_is_not_txin_to_key);
    GENERATE_AND_PLAY(gen_tx_no_inputs_no_outputs);
    GENERATE_AND_PLAY(gen_tx_no_inputs_has_outputs);
    GENERATE_AND_PLAY(gen_tx_has_inputs_no_outputs);
    GENERATE_AND_PLAY(gen_tx_invalid_input_amount);
    GENERATE_AND_PLAY(gen_tx_input_wo_key_offsets);
    GENERATE_AND_PLAY(gen_tx_sender_key_offest_not_exist);
    GENERATE_AND_PLAY(gen_tx_key_offest_points_to_foreign_key);
    GENERATE_AND_PLAY(gen_tx_mixed_key_offest_not_exist);
    GENERATE_AND_PLAY(gen_tx_key_image_not_derive_from_tx_key);
    GENERATE_AND_PLAY(gen_tx_key_image_is_invalid);
    GENERATE_AND_PLAY(gen_tx_check_input_unlock_time);
    GENERATE_AND_PLAY(gen_tx_txout_to_key_has_invalid_key);
    GENERATE_AND_PLAY(gen_tx_output_with_zero_amount);
    GENERATE_AND_PLAY(gen_tx_output_is_not_txout_to_key);
    GENERATE_AND_PLAY(gen_tx_signatures_are_invalid);
    GENERATE_AND_PLAY(gen_tx_extra_double_entry);
    GENERATE_AND_PLAY(gen_tx_double_key_image);
    GENERATE_AND_PLAY(tx_expiration_time);
    GENERATE_AND_PLAY(tx_expiration_time_and_block_template);
    GENERATE_AND_PLAY(tx_expiration_time_and_chain_switching);

    // Double spend
    GENERATE_AND_PLAY(gen_double_spend_in_tx<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_tx<true>);
    GENERATE_AND_PLAY(gen_double_spend_in_the_same_block<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_the_same_block<true>);
    GENERATE_AND_PLAY(gen_double_spend_in_different_blocks<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_different_blocks<true>);
    GENERATE_AND_PLAY(gen_double_spend_in_different_chains);
    GENERATE_AND_PLAY(gen_double_spend_in_alt_chain_in_the_same_block<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_alt_chain_in_the_same_block<true>);
    GENERATE_AND_PLAY(gen_double_spend_in_alt_chain_in_different_blocks<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_alt_chain_in_different_blocks<true>);

    GENERATE_AND_PLAY(gen_uint_overflow_1);
    GENERATE_AND_PLAY(gen_uint_overflow_2);


    // Hardfok1 tests
    GENERATE_AND_PLAY(before_hard_fork_1_cumulative_difficulty);
    GENERATE_AND_PLAY(inthe_middle_hard_fork_1_cumulative_difficulty);
    GENERATE_AND_PLAY(after_hard_fork_1_cumulative_difficulty);
    GENERATE_AND_PLAY(hard_fork_1_locked_mining_test);
    GENERATE_AND_PLAY(hard_fork_1_bad_pos_source);
    GENERATE_AND_PLAY(hard_fork_1_unlock_time_2_in_normal_tx);
    GENERATE_AND_PLAY(hard_fork_1_unlock_time_2_in_coinbase);
    GENERATE_AND_PLAY(hard_fork_1_chain_switch_pow_only);
    GENERATE_AND_PLAY(hard_fork_1_checkpoint_basic_test);
    GENERATE_AND_PLAY(hard_fork_1_pos_locked_height_vs_time);
    //GENERATE_AND_PLAY(gen_block_reward); */




    size_t failed_postponed_tests_count = 0;
    uint64_t total_time = 0;
    if (!tests_running_time.empty())
    {
      uint64_t max_time = std::max_element(tests_running_time.begin(), tests_running_time.end(), [](tests_running_time_t::value_type& lhs, tests_running_time_t::value_type& rhs)->bool { return lhs.second < rhs.second; })->second;
      uint64_t max_test_name_len = std::max_element(tests_running_time.begin(), tests_running_time.end(), [](tests_running_time_t::value_type& lhs, tests_running_time_t::value_type& rhs)->bool { return lhs.first.size() < rhs.first.size(); })->first.size();
      size_t bar_width = 70;
      for (auto i : tests_running_time)
      {
        bool failed = failed_tests.count(i.first) != 0;
        bool postponed = postponed_tests.count(i.first) != 0;
        if (failed && postponed)
          ++failed_postponed_tests_count;
        std::string bar(bar_width * i.second / max_time, '#');
        std::cout << (failed ? (postponed ? concolor::yellow : concolor::magenta) : concolor::green) << std::left << std::setw(max_test_name_len + 1) << i.first << "\t" << std::setw(10) << i.second << " ms \t" << bar << std::endl;
        total_time += i.second;
      }
    }

    serious_failures_count = failed_tests.size() - failed_postponed_tests_count;
    
    std::cout << (serious_failures_count == 0 ? concolor::green : concolor::magenta);
    std::cout << "\nREPORT:\n";
    std::cout << "  Test run:   " << tests_count << std::endl;
    std::cout << "  Failures:   " << serious_failures_count << " (postponed failures: " << failed_postponed_tests_count << ")" << std::endl;
    std::cout << "  Postponed:  " << postponed_tests.size() << std::endl;
    std::cout << "  Total time: " << total_time / 1000 << " s. (" << (tests_count > 0 ? total_time / tests_count : 0) << " ms per test in average)" << std::endl;
    if (!failed_tests.empty())
    {
      std::cout << "FAILED/POSTPONED TESTS:\n";
      for (auto test_name : failed_tests)
      {
        bool postponed = postponed_tests.count(test_name);
        std::cout << "  " << (postponed ? "POSTPONED: " : "FAILED:    ") << test_name << '\n';
      }
    }
    std::cout << concolor::normal << std::endl;
  }

  /*{
    std::cout << concolor::magenta << "Wrong arguments" << concolor::normal << std::endl;
    std::cout << desc_options << std::endl;
    return 2;
  }*/
#ifdef _WIN32
  ::Beep(1050, 1000);
#endif

  return serious_failures_count == 0 ? 0 : 1;

  CATCH_ENTRY_L0("main", 1);
}

// Global scope debug helper functions. Can be called from VS Immediate window, for instance.
const char* hash2str(const crypto::hash& h)
{
  static std::string s;
  s = epee::string_tools::pod_to_hex(h);
  return s.c_str();
}

void hash2log(const crypto::hash& h)
{
  LOG_PRINT("!dbg hash: " << h, LOG_LEVEL_0);
}

void tx2log(const currency::transaction& tx)
{
  currency::transaction ltx = tx;
  LOG_PRINT("!dbg transaction: " << currency::get_transaction_hash(ltx) << ENDL << currency::obj_to_json_str(ltx), LOG_LEVEL_0);
}
