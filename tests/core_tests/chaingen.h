// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <iostream>

#define USE_INSECURE_RANDOM_RPNG_ROUTINES   // turns on pseudorandom number generator manupulations for tests
#define TX_POOL_USE_UNSECURE_TEST_FUNCTIONS // turns on special tests functions of tx pool

#include "currency_core/currency_basic.h"
#include "currency_core/currency_core.h"
#include "wallet/wallet2.h"
#include "test_core_time.h"

#define TESTS_DEFAULT_FEE                   ((uint64_t)TX_DEFAULT_FEE)
#define MK_TEST_COINS(amount)               (static_cast<uint64_t>(amount) * TX_DEFAULT_FEE)
#define TESTS_POS_CONFIG_MIN_COINSTAKE_AGE  4
#define TESTS_POS_CONFIG_POS_MINIMUM_HEIGH  4

namespace concolor
{
  inline std::basic_ostream<char, std::char_traits<char> >& bright_white(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_white, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& red(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_red, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& green(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_green, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& magenta(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_magenta, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& yellow(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_yellow, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& normal(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::reset_console_color();
    return ostr;
  }
}


struct callback_entry
{
  callback_entry() {}
  callback_entry(const std::string& cb_name, const currency::blobdata& cb_params) : callback_name(cb_name), callback_params(cb_params) {}

  std::string callback_name;
  currency::blobdata callback_params;
  BEGIN_SERIALIZE_OBJECT()
    FIELD(callback_name)
    FIELD(callback_params)
  END_SERIALIZE()

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & callback_name;
    ar & callback_params;
  }
};

template<typename T>
struct serialized_object
{
  serialized_object() { }

  serialized_object(const currency::blobdata& a_data)
    : data(a_data)
  {
  }

  currency::blobdata data;
  BEGIN_SERIALIZE_OBJECT()
    FIELD(data)
    END_SERIALIZE()

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & data;
  }
};

typedef serialized_object<currency::block> serialized_block;
typedef serialized_object<currency::transaction> serialized_transaction;

struct event_visitor_settings
{
  int valid_mask;
  bool txs_kept_by_block;
  bool skip_txs_blobsize_check;

  enum settings
  {
    set_txs_kept_by_block         = 1 << 0,
    set_skip_txs_blobsize_check   = 1 << 1,
  };

  event_visitor_settings(int a_valid_mask = 0, bool a_txs_kept_by_block = false, bool skip_txs_blobsize_check = false)
    : valid_mask(a_valid_mask)
    , txs_kept_by_block(a_txs_kept_by_block)
    , skip_txs_blobsize_check(skip_txs_blobsize_check)
  {
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & valid_mask;
    ar & txs_kept_by_block;
    ar & skip_txs_blobsize_check;
  }
};

struct event_special_block
{
  currency::block block;
  int special_flags;

  enum special_flags_t
  {
    flag_skip = 1 << 0   // block won't be passed to the core to simulate delays and/or orphanness
  };

  event_special_block()
    : special_flags(0)
  {
  }

  event_special_block(const currency::block& block, int special_flags = 0)
    : block(block)
    , special_flags(special_flags)
  {
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & block;
    ar & special_flags;
  }
};

struct event_core_time
{
  uint64_t time_shift;
  bool relative;

  event_core_time()
    : time_shift(0)
    , relative(true)
  {}

  event_core_time(uint64_t time_shift, bool relative = false)
    : time_shift(time_shift)
    , relative(relative)
  {}

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & time_shift;
    ar & relative;
  }
};

struct core_hardforks_config
{
  std::array<uint64_t, currency::hard_forks_descriptor::m_total_count> hardforks;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & hardforks;
  }
};

VARIANT_TAG(binary_archive, callback_entry, 0xcb);
VARIANT_TAG(binary_archive, currency::account_base, 0xcc);
VARIANT_TAG(binary_archive, serialized_block, 0xcd);
VARIANT_TAG(binary_archive, serialized_transaction, 0xce);
VARIANT_TAG(binary_archive, event_visitor_settings, 0xcf);
VARIANT_TAG(binary_archive, event_special_block, 0xd0);
VARIANT_TAG(binary_archive, event_core_time, 0xd1);
VARIANT_TAG(binary_archive, core_hardforks_config, 0xd2);


typedef boost::variant<currency::block, currency::transaction, currency::account_base, callback_entry, serialized_block, serialized_transaction, event_visitor_settings, event_special_block, event_core_time, core_hardforks_config> test_event_entry;
typedef std::unordered_map<crypto::hash, const currency::transaction*> map_hash2tx_t;

enum test_tx_split_strategy { tests_default_split_strategy /*height-based, TODO*/, tests_void_split_strategy, tests_null_split_strategy, tests_digits_split_strategy, tests_random_split_strategy };
struct test_gentime_settings
{
  test_tx_split_strategy  split_strategy            = tests_digits_split_strategy;
  size_t                  miner_tx_max_outs         = CURRENCY_MINER_TX_MAX_OUTS;
  uint64_t                tx_max_out_amount         = WALLET_MAX_ALLOWED_OUTPUT_AMOUNT;
  uint64_t                dust_threshold            = DEFAULT_DUST_THRESHOLD;
  size_t                  rss_min_number_of_outputs = CURRENCY_TX_MIN_ALLOWED_OUTS; // for random split strategy: min (target) number of tx outputs, one output will be split into this many parts
  size_t                  rss_num_digits_to_keep    = CURRENCY_TX_OUTS_RND_SPLIT_DIGITS_TO_KEEP; // for random split strategy: number of digits to keep
  bool                    ignore_invalid_blocks     = true; // gen-time blockchain building: don't take into account blocks marked as invalid ("mark_invalid_block")
  bool                    ignore_invalid_txs        = true; // gen-time blockchain building: don't take into account txs marked as invalid ("mark_invalid_txs")
};

class test_generator;

class test_chain_unit_base
{
public:
  test_chain_unit_base();

  typedef boost::function<bool(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)> verify_callback;
  typedef std::map<std::string, verify_callback> callbacks_map;

  void register_callback(const std::string& cb_name, verify_callback cb);
  bool verify(const std::string& cb_name, currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);

  bool need_core_proxy() const { return false; }  // tests can override this in order to obtain core proxy (e.g. for wallet)
  void set_core_proxy(std::shared_ptr<tools::i_core_proxy>) { /* do nothing */ }
  uint64_t get_tx_version_from_events(const std::vector<test_event_entry> &events) const;

  virtual void on_test_constructed() {} // called right after test class is constructed by the chaingen 
  void on_test_generator_created(test_generator& generator) const; // tests can override this for special initialization
  
  currency::core_runtime_config get_runtime_info_for_core() const; // tests can override this for special initialization

  void set_hardforks_for_old_tests();
  currency::hard_forks_descriptor& get_hardforks() { return m_hardforks; }
  const currency::hard_forks_descriptor& get_hardforks() const { return m_hardforks; }

private:
  callbacks_map m_callbacks;

protected: 
  mutable currency::hard_forks_descriptor m_hardforks;
};

struct offers_count_param
{
  offers_count_param(size_t offers_count = 0, size_t offers_count_raw = SIZE_MAX)
    : offers_count(offers_count), offers_count_raw(offers_count_raw)
  {}
  size_t offers_count;
  size_t offers_count_raw;
};

class test_chain_unit_enchanced : virtual public test_chain_unit_base
{
public:
  test_chain_unit_enchanced();

  bool check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_idx, const currency::transaction& /*tx*/)
  {
    if (m_invalid_tx_index == event_idx)
    {
      CHECK_AND_ASSERT_MES(tvc.m_verification_failed, false, ENDL << "event #" << event_idx << ": the tx passed the verification, although it had been marked as invalid" << ENDL);
      return tvc.m_verification_failed;
    }
    
    if (m_unverifiable_tx_index == event_idx)
    {
      CHECK_AND_ASSERT_MES(tvc.m_verification_impossible, false, ENDL << "event #" << event_idx << ": the tx passed normally, although it had been marked as unverifiable" << ENDL);
      return tvc.m_verification_impossible;
    }

    CHECK_AND_ASSERT_MES(tx_added, false, ENDL << "event #" << event_idx << ": the tx has not been added for some reason" << ENDL);

    return !tvc.m_verification_failed && tx_added;
  }

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*block*/)
  {
    if (m_invalid_block_index == event_idx)
    {
      CHECK_AND_ASSERT_MES(bvc.m_verification_failed, false, ENDL << "event #" << event_idx << ": the block passed the verification, although it had been marked as invalid" << ENDL);
      return bvc.m_verification_failed;
    }

    if (m_orphan_block_index == event_idx)
    {
      CHECK_AND_ASSERT_MES(bvc.m_marked_as_orphaned, false, ENDL << "event #" << event_idx << ": the block passed normally, although it had been marked as orphaned" << ENDL);
      return bvc.m_marked_as_orphaned;
    }

    return !bvc.m_verification_failed;
  }

  bool mark_invalid_block(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_block_index = ev_index + 1;
    return true;
  }

  bool mark_orphan_block(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_orphan_block_index = ev_index + 1;
    return true;
  }

  bool mark_invalid_tx(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_tx_index = ev_index + 1;
    return true;
  }

  bool mark_unverifiable_tx(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_unverifiable_tx_index = ev_index + 1;
    return true;
  }

  void set_hard_fork_heights_to_generator(test_generator& generator) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_top_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool clear_tx_pool(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_tx_pool_empty(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_tx_pool_count(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool print_tx_pool(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool remove_stuck_txs(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_offers_count(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_hardfork_active(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_hardfork_inactive(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  static bool is_event_mark_invalid_block(const test_event_entry& ev, bool use_global_gentime_settings = true);
  static bool is_event_mark_invalid_tx(const test_event_entry& ev, bool use_global_gentime_settings = true);

protected:
  struct params_top_block
  {
    params_top_block(const currency::block& b) : height(currency::get_block_height(b)), hash(currency::get_block_hash(b)) {}
    params_top_block(uint64_t height = 0, crypto::hash hash = currency::null_hash) : height(height), hash(hash) {}
    uint64_t height;
    crypto::hash hash;
  };
  size_t m_invalid_block_index;
  size_t m_invalid_tx_index;
  size_t m_unverifiable_tx_index;
  size_t m_orphan_block_index;
};

struct wallet_test_core_proxy;

class test_generator
{
public:
  struct block_info
  {
    block_info()
      : b()
      , already_generated_coins(0)
      , block_size(0)
      , cumul_difficulty(0)
    {
    }

    block_info(const currency::block& b_, 
               uint64_t an_already_generated_coins, 
               size_t a_block_size, 
               currency::wide_difficulty_type diff,
               const std::list<currency::transaction>& tx_list, 
               const crypto::hash& k_hash)
      : b(b_)
      , already_generated_coins(an_already_generated_coins)
      , block_size(a_block_size)
      , cumul_difficulty(diff), 
      m_transactions(tx_list.begin(), tx_list.end()), 
      ks_hash(k_hash)
    {}

    currency::block b;
    uint64_t already_generated_coins;
    size_t block_size;
    currency::wide_difficulty_type cumul_difficulty;
    std::vector<currency::transaction> m_transactions;
    crypto::hash ks_hash;
  };


  struct out_index_info
  {
    size_t block_height;
    size_t in_block_tx_index;
    size_t in_tx_out_index;
  };

  typedef std::map<uint64_t, std::vector<out_index_info> > outputs_index;                     // amount  -> [gindex -> out_index_info]
  typedef std::unordered_map<crypto::hash, std::vector<uint64_t> > tx_global_indexes;         // tx_hash -> vector of tx's outputs global indices

  typedef std::vector<const block_info*> blockchain_vector;
  
  struct gen_wallet_info
  {
    tools::wallet2::mining_context  mining_context;
    std::shared_ptr<tools::wallet2> wallet;
  };
  typedef std::vector<gen_wallet_info> wallets_vector;


  enum block_fields
  {
    bf_none      = 0,
    bf_major_ver = 1 << 0,
    bf_minor_ver = 1 << 1,
    bf_timestamp = 1 << 2,
    bf_prev_id   = 1 << 3,
    bf_miner_tx  = 1 << 4,
    bf_tx_hashes = 1 << 5,
    bf_diffic    = 1 << 6
  };

  test_generator();

  //-----------
  static currency::wide_difficulty_type get_difficulty_for_next_block(const std::vector<const block_info*>& blocks, bool pow = true);
  currency::wide_difficulty_type get_difficulty_for_next_block(const crypto::hash& head_id, bool pow = true) const;
  bool get_params_for_next_pos_block(const crypto::hash& head_id, currency::wide_difficulty_type& pos_difficulty, crypto::hash& last_pow_block_hash, crypto::hash& last_pos_block_kernel_hash) const;
  currency::wide_difficulty_type get_cumul_difficulty_for_next_block(const crypto::hash& head_id, bool pow = true) const;
  void get_block_chain(std::vector<const block_info*>& blockchain, const crypto::hash& head, size_t n) const;
  void get_last_n_block_sizes(std::vector<size_t>& block_sizes, const crypto::hash& head, size_t n) const;
  uint64_t get_last_n_blocks_fee_median(const crypto::hash& head_block_hash, size_t n = ALIAS_COAST_PERIOD);
  template<typename cb_t>
  bool construct_block_gentime_with_coinbase_cb(const currency::block& prev_block, const currency::account_base& acc, cb_t cb, currency::block& b);
  bool construct_pow_block_with_alias_info_in_coinbase(const currency::account_base& acc, const currency::block& prev_block, const currency::extra_alias_entry& ai, currency::block& b);
  uint64_t get_base_reward_for_next_block(const crypto::hash& head_id, bool pow = true) const;


  //POS 
  static bool build_stake_modifier(currency::stake_modifier_type& sm, const test_generator::blockchain_vector& blck_chain);
  
  bool find_kernel(const std::list<currency::account_base>& accs,
                   const blockchain_vector& blck_chain,
                   wallets_vector& wallets,
                   currency::pos_entry& pe,
                   size_t& found_wallet_index,
                   uint64_t& found_timestamp,
                   crypto::hash& found_kh);

  bool build_wallets(const blockchain_vector& blocks,
                     const std::list<currency::account_base>& accs,
                     const tx_global_indexes& txs_outs,
                     const outputs_index& oi,
                     wallets_vector& wallets,
                     const currency::core_runtime_config& cc = currency::get_default_core_runtime_config());
  
  bool build_wallets(const crypto::hash& blockchain_head,
                     const std::list<currency::account_base>& accounts,
                     wallets_vector& wallets,
                     const currency::core_runtime_config& cc = currency::get_default_core_runtime_config());

  bool init_test_wallet(const currency::account_base& account, const crypto::hash& genesis_hash, std::shared_ptr<tools::wallet2> &result);
  bool refresh_test_wallet(const std::vector<test_event_entry>& events, tools::wallet2* w, const crypto::hash& top_block_hash, size_t expected_blocks_to_be_fetched = std::numeric_limits<size_t>::max());
  
  bool sign_block(const tools::wallet2::mining_context& mining_context,
                  const currency::pos_entry& pe,
                  uint64_t full_block_reward,
                  const tools::wallet2& w,
                  currency::tx_generation_context& miner_tx_tgc,
                  currency::block& b);
  
  /*bool get_output_details_by_global_index(const test_generator::blockchain_vector& blck_chain,
                                          const test_generator::outputs_index& indexes,
                                          uint64_t amount,
                                          uint64_t global_index,
                                          uint64_t& h,
                                          const currency::transaction* tx,
                                          uint64_t& tx_out_index,
                                          crypto::public_key& tx_pub_key,
                                          crypto::public_key& output_key);*/


  
  uint64_t get_already_generated_coins(const crypto::hash& blk_id) const;
  uint64_t get_already_generated_coins(const currency::block& blk) const;
  currency::wide_difficulty_type get_block_difficulty(const crypto::hash& blk_id) const;
  currency::wide_difficulty_type get_cumul_difficulty(const crypto::hash& head_id) const;
  static uint64_t get_timestamps_median(const blockchain_vector& blck_chain, size_t window_size = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);
  uint64_t get_timestamps_median(const crypto::hash& blockchain_head, size_t window_size = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);

  bool build_outputs_indext_for_chain(const std::vector<const block_info*>& blocks, outputs_index& index, tx_global_indexes& txs_outs) const;

  size_t get_tx_out_gindex(const crypto::hash& blockchain_head, const crypto::hash& tx_hash, const size_t output_index) const;

  void add_block(const currency::block& blk, 
    size_t tsx_size, 
    std::vector<size_t>& block_sizes, 
    uint64_t already_generated_coins, 
    currency::wide_difficulty_type cum_diff, 
    const std::list<currency::transaction>& tx_list,
    const crypto::hash& ks_hash);

  void add_block_info(const block_info& bi);

  bool add_block_info(const currency::block& b, const std::list<currency::transaction>& tx_list);

  bool remove_block_info(const currency::block& blk);
  bool remove_block_info(const crypto::hash& block_id);

  bool construct_block(currency::block& blk, 
    uint64_t height, 
    const crypto::hash& prev_id,
    const currency::account_base& miner_acc, 
    uint64_t timestamp, 
    uint64_t already_generated_coins,  
    std::vector<size_t>& block_sizes, 
    const std::list<currency::transaction>& tx_list, 
    const std::list<currency::account_base>& coin_stake_sources = std::list<currency::account_base>() //in case of PoS block
    );
  bool construct_genesis_block(currency::block& blk, 
    const currency::account_base& miner_acc, 
    uint64_t timestamp);

  bool construct_block(const std::vector<test_event_entry>& events,
    currency::block& blk,
    const currency::block& blk_prev, 
    const currency::account_base& miner_acc, 
    const std::list<currency::transaction>& tx_list = std::list<currency::transaction>(), 
    const std::list<currency::account_base>& coin_stake_sources = std::list<currency::account_base>() //in case of PoS block
    );
  bool construct_block(int64_t manual_timestamp_adjustment,
    const std::vector<test_event_entry>& events,
    currency::block& blk,
    const currency::block& blk_prev,
    const currency::account_base& miner_acc,
    const std::list<currency::transaction>& tx_list = std::list<currency::transaction>(),
    const std::list<currency::account_base>& coin_stake_sources = std::list<currency::account_base>() //in case of PoS block
  );


  bool construct_block_manually(currency::block& blk, const currency::block& prev_block,
    const currency::account_base& miner_acc, int actual_params = bf_none, uint8_t major_ver = 0,
    uint8_t minor_ver = 0, uint64_t timestamp = 0, const crypto::hash& prev_id = crypto::hash(),
    const currency::wide_difficulty_type& diffic = 1, const currency::transaction& miner_tx = currency::transaction(),
    const std::vector<crypto::hash>& tx_hashes = std::vector<crypto::hash>(), size_t txs_sizes = 0);
  bool construct_block_manually_tx(currency::block& blk, const currency::block& prev_block,
    const currency::account_base& miner_acc, const std::vector<crypto::hash>& tx_hashes, size_t txs_size);
  bool find_nounce(currency::block& blk, std::vector<const block_info*>& blocks, currency::wide_difficulty_type dif, uint64_t height) const;
  //bool find_nounce(currency::block& blk, currency::wide_difficulty_type dif, uint64_t height);
  
  crypto::secret_key last_tx_generated_secret_key;
  std::shared_ptr<wallet_test_core_proxy> m_wallet_test_core_proxy;

  /////////////////////////
  // test gentime settings (all made static as test_generator not necessary for tx construction)
  static const test_gentime_settings& get_test_gentime_settings() { return m_test_gentime_settings; }
  static void set_test_gentime_settings(const test_gentime_settings& s) { m_test_gentime_settings = s; }
  static void set_test_gentime_settings_default() { m_test_gentime_settings = m_test_gentime_settings_default; }
  void set_ignore_last_pow_in_wallets(bool ignore_last_pow_in_wallets) { m_ignore_last_pow_in_wallets = ignore_last_pow_in_wallets; }
  void set_hardfork_height(size_t hardfork_id, uint64_t h);
  void set_hardforks(const currency::hard_forks_descriptor& hardforks);
  const currency::hard_forks_descriptor& get_hardforks() const { return m_hardforks; }

  void load_hardforks_from(const test_chain_unit_base* pthis) { m_hardforks = pthis->get_hardforks(); }
  template<typename t_type>
  void load_hardforks_from(const t_type* pthis) {}

private:
  bool m_ignore_last_pow_in_wallets;
  
  mutable currency::hard_forks_descriptor m_hardforks;

  std::unordered_map<crypto::hash, block_info> m_blocks_info;
  static test_gentime_settings m_test_gentime_settings;
  static const test_gentime_settings m_test_gentime_settings_default;
}; // class class test_generator

struct test_gentime_settings_restorer
{
  test_gentime_settings_restorer() : m_settings(test_generator::get_test_gentime_settings()) {}
  ~test_gentime_settings_restorer() { test_generator::set_test_gentime_settings(m_settings); }
  test_gentime_settings m_settings;
};

extern const crypto::signature invalid_signature; // invalid non-null signature for test purpose
static const std::vector<currency::extra_v> empty_extra;
static const std::vector<currency::attachment_v> empty_attachment;

inline currency::wide_difficulty_type get_test_difficulty() {return 1;}
void fill_nonce(currency::block& blk, const currency::wide_difficulty_type& diffic, uint64_t height);

bool construct_miner_tx_manually(size_t height, uint64_t already_generated_coins,
                                 const currency::account_public_address& miner_address, currency::transaction& tx,
                                 uint64_t fee, currency::keypair* p_txkey = 0);

bool construct_tx_to_key(const currency::hard_forks_descriptor& hf,
                         const std::vector<test_event_entry>& events, 
                         currency::transaction& tx,
                         const currency::block& blk_head, 
                         const currency::account_base& from, 
                         const currency::account_base& to,
                         uint64_t amount, 
                         uint64_t fee, 
                         size_t nmix,
                         uint8_t mix_attr = CURRENCY_TO_KEY_OUT_RELAXED, 
                         const std::vector<currency::extra_v>& extr = std::vector<currency::extra_v>(),
                         const std::vector<currency::attachment_v>& att = std::vector<currency::attachment_v>(),
                         bool check_for_spends = true,
                         bool check_for_unlocktime = true);


bool construct_tx_to_key(const currency::hard_forks_descriptor& hf,
                         const std::vector<test_event_entry>& events, 
                         currency::transaction& tx,
                         const currency::block& blk_head, 
                         const currency::account_base& from, 
                         const currency::account_base& to,
                         uint64_t amount, 
                         uint64_t fee, 
                         size_t nmix, 
                         crypto::secret_key& sk,
                         uint8_t mix_attr = CURRENCY_TO_KEY_OUT_RELAXED, 
                         const std::vector<currency::extra_v>& extr = std::vector<currency::extra_v>(),
                         const std::vector<currency::attachment_v>& att = std::vector<currency::attachment_v>(),
                         bool check_for_spends = true,
                         bool check_for_unlocktime = true);

bool construct_tx_to_key(const currency::hard_forks_descriptor& hf,
                         const std::vector<test_event_entry>& events, 
                         currency::transaction& tx, 
                         const currency::block& blk_head,
                         const currency::account_base& from,
                         const std::vector<currency::tx_destination_entry>& destinations,
                         uint64_t fee = TESTS_DEFAULT_FEE, 
                         size_t nmix = 0, 
                         uint8_t mix_attr = CURRENCY_TO_KEY_OUT_RELAXED, 
                         const std::vector<currency::extra_v>& extr = empty_extra,
                         const std::vector<currency::attachment_v>& att = empty_attachment, 
                         bool check_for_spends = true,
                         bool check_for_unlocktime = true,
                         bool use_ref_by_id = false);

currency::transaction construct_tx_with_fee(const currency::hard_forks_descriptor& hf, std::vector<test_event_entry>& events, const currency::block& blk_head,
                                            const currency::account_base& acc_from, const currency::account_base& acc_to,
                                            uint64_t amount, uint64_t fee);

bool construct_tx_with_many_outputs(const currency::hard_forks_descriptor& hf, std::vector<test_event_entry>& events, const currency::block& blk_head,
                                            const currency::account_keys& keys_from, const currency::account_public_address& addr_to,
                                            uint64_t total_amount, size_t outputs_count, uint64_t fee, currency::transaction& tx, bool use_ref_by_id = false);

void get_confirmed_txs(const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, map_hash2tx_t& confirmed_txs);
bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<currency::block>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head);
bool fill_tx_sources(std::vector<currency::tx_source_entry>& sources, const std::vector<test_event_entry>& events,
                     const currency::block& blk_head, const currency::account_keys& from, uint64_t amount, size_t nmix, bool check_for_spends = true, bool check_for_unlocktime = true, bool use_ref_by_id = false);
bool fill_tx_sources(std::vector<currency::tx_source_entry>& sources, const std::vector<test_event_entry>& events,
                     const currency::block& blk_head, const currency::account_keys& from, uint64_t amount, size_t nmix,
                     const std::vector<currency::tx_source_entry>& sources_to_avoid, bool check_for_spends = true, bool check_for_unlocktime = true,
                     bool use_ref_by_id = false, uint64_t* p_sources_amount_found = nullptr);
bool fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const currency::block& blk_head,
                                      const currency::account_keys& from, const std::list<currency::account_public_address>& to,
                                      uint64_t amount, uint64_t fee, size_t nmix, std::vector<currency::tx_source_entry>& sources,
                                      std::vector<currency::tx_destination_entry>& destinations,
                                      bool check_for_spends = true,
                                      bool check_for_unlocktime = true,
                                      size_t minimum_sigs = SIZE_MAX,
                                      bool use_ref_by_id = false);
bool fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const currency::block& blk_head,
                                      const currency::account_keys& from, const currency::account_public_address& to,
                                      uint64_t amount, uint64_t fee, size_t nmix,
                                      std::vector<currency::tx_source_entry>& sources,
                                      std::vector<currency::tx_destination_entry>& destinations, 
                                      bool check_for_spends = true, 
                                      bool check_for_unlocktime = true,
                                      bool use_ref_by_id = false);
bool fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const currency::block& blk_head,
                                      const currency::account_base& from, const currency::account_base& to,
                                      uint64_t amount, uint64_t fee, size_t nmix,
                                      std::vector<currency::tx_source_entry>& sources,
                                      std::vector<currency::tx_destination_entry>& destinations, 
                                      bool check_for_spends = true, 
                                      bool check_for_unlocktime = true,
                                      bool use_ref_by_id = false);
uint64_t get_balance(const currency::account_keys& addr, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, bool dbg_log = false);
uint64_t get_balance(const currency::account_base& addr, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, bool dbg_log = false);
void balance_via_wallet(const tools::wallet2& w, const crypto::public_key& asset_id, uint64_t* p_total, uint64_t* p_unlocked = 0, uint64_t* p_awaiting_in = 0, uint64_t* p_awaiting_out = 0, uint64_t* p_mined = 0);
#define INVALID_BALANCE_VAL std::numeric_limits<uint64_t>::max()
bool check_balance_via_wallet(const tools::wallet2& w, const char* account_name,
                              uint64_t expected_total,
                              uint64_t expected_mined = INVALID_BALANCE_VAL,
                              uint64_t expected_unlocked = INVALID_BALANCE_VAL,
                              uint64_t expected_awaiting_in = INVALID_BALANCE_VAL,
                              uint64_t expected_awaiting_out = INVALID_BALANCE_VAL,
                              const crypto::public_key& asset_id = currency::native_coin_asset_id);

bool calculate_amounts_many_outs_have_and_no_outs_have(const uint64_t first_blocks_reward, uint64_t& amount_many_outs_have, uint64_t& amount_no_outs_have);
bool find_global_index_for_output(const std::vector<test_event_entry>& events, const crypto::hash& head_block_hash, const currency::transaction& reference_tx, const size_t reference_tx_out_index, uint64_t& global_index);
size_t get_tx_out_index_by_amount(const currency::transaction& tx, const uint64_t amount);
bool sign_multisig_input_in_tx_custom(currency::transaction& tx, size_t ms_input_index, const currency::account_keys& keys, const currency::transaction& source_tx, bool *p_is_input_fully_signed = nullptr, bool compact_sigs = true);
bool make_tx_multisig_to_key(const currency::transaction& source_tx,
                             size_t source_tx_out_idx,
                             const std::list<currency::account_keys>& participants,
                             const currency::account_public_address& target_address,
                             currency::transaction& tx,
                             uint64_t fee = TESTS_DEFAULT_FEE,
                             const std::vector<currency::attachment_v>& attachments = empty_attachment,
                             const std::vector<currency::extra_v>& extra = empty_extra);

bool estimate_wallet_balance_blocked_for_escrow(const tools::wallet2& w, uint64_t& result, bool substruct_change_from_result = true);
bool check_wallet_balance_blocked_for_escrow(const tools::wallet2& w, const char* wallet_name, uint64_t expected_amount);
bool refresh_wallet_and_check_balance(const char* intro_log_message, const char* wallet_name, std::shared_ptr<tools::wallet2> wallet, uint64_t expected_total,
                                      bool print_transfers = false,
                                      size_t block_to_be_fetched = SIZE_MAX,
                                      uint64_t expected_unlocked = UINT64_MAX,
                                      uint64_t expected_mined = UINT64_MAX,
                                      uint64_t expected_awaiting_in = UINT64_MAX,
                                      uint64_t expected_awaiting_out = UINT64_MAX);

uint64_t get_last_block_of_type(bool looking_for_pos, const test_generator::blockchain_vector& blck_chain);
bool generate_pos_block_with_given_coinstake(test_generator& generator, const std::vector<test_event_entry> &events, const currency::account_base& miner, const currency::block& prev_block, const currency::transaction& stake_tx, size_t stake_output_idx, currency::block& result, uint64_t stake_output_gidx = UINT64_MAX);
bool check_ring_signature_at_gen_time(const std::vector<test_event_entry>& events, const crypto::hash& last_block_id, const currency::txin_to_key& in_t_k,
  const crypto::hash& hash_for_sig, const std::vector<crypto::signature> &sig);

bool check_mixin_value_for_each_input(size_t mixin, const crypto::hash& tx_id, currency::core& c);

bool shuffle_source_entry(currency::tx_source_entry& se);
bool shuffle_source_entries(std::vector<currency::tx_source_entry>& sources);

// one output will be created for each destination entry and one additional output to add up to old coinbase total amount
bool replace_coinbase_in_genesis_block(const std::vector<currency::tx_destination_entry>& destinations, test_generator& generator, std::vector<test_event_entry>& events, currency::block& genesis_block);

template<typename t_map>
const std::vector<uint64_t>& get_tx_gindex_from_map(const crypto::hash& tx_id, const t_map& id_to_vector)
{
  auto it_global_indexes = id_to_vector.find(tx_id);
  if (it_global_indexes == id_to_vector.end())
  {
    throw std::runtime_error("TX ID NOT FOUND");
  }
  return it_global_indexes->second;
}

//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_index, const currency::transaction& tx, t_test_class& validator, int)
  -> decltype(validator.check_tx_verification_context(tvc, tx_added, event_index, tx))
{
  return validator.check_tx_verification_context(tvc, tx_added, event_index, tx);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t /*event_index*/, const currency::transaction& /*tx*/, t_test_class&, long)
{
  // Default block verification context check
  if (tvc.m_verification_failed)
    throw std::runtime_error("Transaction verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_index, const currency::transaction& tx, t_test_class& validator)
{
  // SFINAE in action
  return do_check_tx_verification_context(tvc, tx_added, event_index, tx, validator, 0);
}
//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_block_verification_context(const currency::block_verification_context& bvc, size_t event_index, const currency::block& blk, t_test_class& validator, int)
  -> decltype(validator.check_block_verification_context(bvc, event_index, blk))
{
  return validator.check_block_verification_context(bvc, event_index, blk);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_block_verification_context(const currency::block_verification_context& bvc, size_t /*event_index*/, const currency::block& /*blk*/, t_test_class&, long)
{
  // Default block verification context check
  if (bvc.m_verification_failed)
    throw std::runtime_error("Block verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_index, const currency::block& blk, t_test_class& validator)
{
  // SFINAE in action
  return do_check_block_verification_context(bvc, event_index, blk, validator, 0);
}

template<class attachment_cb>
bool construct_broken_tx(const currency::account_keys& sender_account_keys, const std::vector<currency::tx_source_entry>& sources,
  const std::vector<currency::tx_destination_entry>& destinations,
  const std::vector<currency::extra_v>& extra,
  const std::vector<currency::attachment_v>& attachments,
  currency::transaction& tx,
  uint64_t unlock_time,
  uint8_t tx_outs_attr, attachment_cb acb)
{
  tx.vin.clear();
  tx.vout.clear();
  tx.signatures.clear();
  tx.extra = extra;

  tx.version = TRANSACTION_VERSION_PRE_HF4;
  set_tx_unlock_time(tx, unlock_time);

  currency::keypair txkey = currency::keypair::generate();
  add_tx_pub_key_to_extra(tx, txkey.pub);

  struct input_generation_context_data
  {
    currency::keypair in_ephemeral;
    std::vector<currency::tx_source_entry::output_entry> sorted_outputs;
    size_t real_out_index = 0;
  };
  std::vector<input_generation_context_data> in_contexts;


  uint64_t summary_inputs_money = 0;
  // fill inputs
  for(const currency::tx_source_entry& src_entr : sources)
  {
    CHECK_AND_ASSERT_MES(src_entr.real_output < src_entr.outputs.size(), false, "real_output index = " << src_entr.real_output << " is bigger than output_keys.size() =  " << src_entr.outputs.size());

    input_generation_context_data& igc = in_contexts.emplace_back();
    igc.sorted_outputs = prepare_outputs_entries_for_key_offsets(src_entr.outputs, src_entr.real_output, igc.real_out_index);
    summary_inputs_money += src_entr.amount;

    crypto::key_image img;
    if (!currency::generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, igc.in_ephemeral, img))
      return false;

    //check that derivated key is equal with real output key
    if (!(igc.in_ephemeral.pub == igc.sorted_outputs[igc.real_out_index].stealth_address))
    {
      LOG_ERROR("derived public key missmatch with output public key! " << ENDL << "derived_key:"
        << epst::pod_to_hex(igc.in_ephemeral.pub) << ENDL << "real output_public_key:"
        << epst::pod_to_hex(igc.sorted_outputs[igc.real_out_index].stealth_address));
      return false;
    }

    // fill tx input
    currency::txin_to_key input_to_key;
    input_to_key.amount = src_entr.amount;
    input_to_key.k_image = img;

    // fill ring references
    for(const currency::tx_source_entry::output_entry& out_entry : igc.sorted_outputs)
      input_to_key.key_offsets.push_back(out_entry.out_reference);

    tx.vin.push_back(input_to_key);
  }

  // "Shuffle" outs
  std::vector<currency::tx_destination_entry> shuffled_dsts(destinations);
  std::sort(shuffled_dsts.begin(), shuffled_dsts.end(), [](const currency::tx_destination_entry& de1, const currency::tx_destination_entry& de2) { return de1.amount < de2.amount; });

  uint64_t summary_outs_money = 0;
  //fill outputs
  size_t output_index = 0;
  std::set<uint16_t> der_hints;
  BOOST_FOREACH(const currency::tx_destination_entry& dst_entr, shuffled_dsts)
  {
    CHECK_AND_ASSERT_MES(dst_entr.amount > 0, false, "Destination with wrong amount: " << dst_entr.amount);
    bool r = construct_tx_out(dst_entr, txkey.sec, output_index, tx, der_hints, sender_account_keys, tx_outs_attr);
    CHECK_AND_ASSERT_MES(r, false, "Failed to construc tx out");
    output_index++;
    summary_outs_money += dst_entr.amount;
  }

  //check money
  if (summary_outs_money > summary_inputs_money)
  {
    LOG_ERROR("Transaction inputs money (" << summary_inputs_money << ") less than outputs money (" << summary_outs_money << ")");
    return false;
  }

  //include offers if need
  tx.attachment = attachments;

  if (!acb(tx))
    return false;

  //generate ring signatures
  crypto::hash tx_prefix_hash;
  get_transaction_prefix_hash(tx, tx_prefix_hash);

  std::stringstream ss_ring_s;
  size_t i = 0;
  for(const currency::tx_source_entry& src_entr : sources)
  {
    ss_ring_s << "pub_keys:" << ENDL;
    std::vector<const crypto::public_key*> keys_ptrs;
    for(const currency::tx_source_entry::output_entry& o : in_contexts[i].sorted_outputs)
    {
      keys_ptrs.push_back(&o.stealth_address);
      ss_ring_s << o.stealth_address << ENDL;
    }

    tx.signatures.push_back(currency::NLSAG_sig());
    std::vector<crypto::signature>& sigs = boost::get<currency::NLSAG_sig>(tx.signatures.back()).s;
    sigs.resize(src_entr.outputs.size());
    crypto::generate_ring_signature(tx_prefix_hash, boost::get<currency::txin_to_key>(tx.vin[i]).k_image, keys_ptrs, in_contexts[i].in_ephemeral.sec, in_contexts[i].real_out_index, sigs.data());
    ss_ring_s << "signatures:" << ENDL;
    std::for_each(sigs.begin(), sigs.end(), [&](const crypto::signature& s){ss_ring_s << s << ENDL; });
    ss_ring_s << "prefix_hash:" << tx_prefix_hash << ENDL << "in_ephemeral_key: " << in_contexts[i].in_ephemeral.sec << ENDL << "real_output: " << in_contexts[i].real_out_index;
    i++;
  }

  LOG_PRINT2("construct_tx.log", "transaction_created: " << get_transaction_hash(tx) << ENDL << obj_to_json_str(tx) << ENDL << ss_ring_s.str(), LOG_LEVEL_3);

  return true;
}
template<typename callback_t>
bool construct_broken_tx(std::list<currency::transaction>& txs_set, 
  std::vector<test_event_entry>& events,
  const currency::account_base& sender_account_keys,
  const currency::account_base& rcvr_account_keys,
  const currency::block& blk_head, 
  const std::vector<currency::attachment_v>& att, 
  callback_t cb)
{
  currency::transaction t = AUTO_VAL_INIT(t);
  std::vector<currency::tx_source_entry> sources;
  std::vector<currency::tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_head, sender_account_keys, rcvr_account_keys, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations, true);

  bool r = construct_broken_tx(sender_account_keys.get_keys(), sources, destinations, std::vector<currency::extra_v>(), att, t, 0, CURRENCY_TO_KEY_OUT_RELAXED, cb);
  txs_set.push_back(t);
  events.push_back(t);
  return r;
  }

template<typename cb_t>
bool test_generator::construct_block_gentime_with_coinbase_cb(const currency::block& prev_block, const currency::account_base& acc, cb_t cb, currency::block& b)
{
  bool r = false;
  crypto::hash prev_id = get_block_hash(prev_block);
  uint64_t already_generated_coins = get_already_generated_coins(prev_block);
  std::vector<size_t> block_sizes;
  get_last_n_block_sizes(block_sizes, prev_id, CURRENCY_REWARD_BLOCKS_WINDOW);

  currency::transaction miner_tx = AUTO_VAL_INIT(miner_tx);
  size_t height = get_block_height(prev_block) + 1;
  //size_t current_block_size = get_object_blobsize(miner_tx);

  uint64_t block_reward_without_fee = 0;
  uint64_t block_reward = 0;

  currency::keypair tx_sec_key = currency::keypair::generate();
  r = construct_miner_tx(height, epee::misc_utils::median(block_sizes), already_generated_coins, 0 /* current_block_size !HACK! */, 0,
    acc.get_public_address(), acc.get_public_address(), miner_tx, block_reward_without_fee, block_reward, get_tx_version(height, m_hardforks), currency::blobdata(), /* max outs: */ 1,
    /* pos: */ false, currency::pos_entry(), /* ogc_ptr: */ nullptr, &tx_sec_key);
  CHECK_AND_ASSERT_MES(r, false, "construct_miner_tx failed");

  if (!cb(miner_tx, tx_sec_key))
    return false;

  currency::wide_difficulty_type diff = get_difficulty_for_next_block(prev_id, true);
  r = construct_block_manually(b, prev_block, acc, 
    test_generator::block_fields::bf_miner_tx| test_generator::block_fields::bf_major_ver | test_generator::block_fields::bf_minor_ver,
    m_hardforks.get_block_major_version_by_height(height),
    m_hardforks.get_block_minor_version_by_height(height),
    0, prev_id, diff, miner_tx);
  CHECK_AND_ASSERT_MES(r, false, "construct_block_manually failed");

  return true;
}


namespace crypto {
  inline std::ostream &operator <<(std::ostream &o, const currency::account_base &acc)
  {
    return o <<
      "account: " << std::endl <<
      "  addr:             " << get_account_address_as_str(acc.get_public_address()) << std::endl <<
      "  spend secret key: " << acc.get_keys().spend_secret_key << std::endl <<
      "  spend public key: " << acc.get_public_address().spend_public_key << std::endl <<
      "  view  secret key: " << acc.get_keys().view_secret_key << std::endl <<
      "  view  public key: " << acc.get_public_address().view_public_key << std::endl <<
      "  timestamp:        " << acc.get_createtime();
  }
}


inline uint64_t get_sources_total_amount(const std::vector<currency::tx_source_entry>& s)
{
  uint64_t result = 0;
  for (auto& e : s)
    result += e.amount;
  return result;
}

inline void count_ref_by_id_and_gindex_refs_for_tx_inputs(const currency::transaction& tx, size_t& refs_by_id, size_t& refs_by_gindex)
{
  refs_by_id = 0;
  refs_by_gindex = 0;
  for (auto& in : tx.vin)
  {
    if (in.type() != typeid(currency::txin_to_key))
      continue;

    const currency::txin_to_key& in2key = boost::get<currency::txin_to_key>(in);
    for (auto& ko : in2key.key_offsets)
    {
      if (ko.type() == typeid(currency::ref_by_id))
        ++refs_by_id;
      else if (ko.type() == typeid(uint64_t))
        ++refs_by_gindex;
    }
  }
}

template<typename U, typename V>
void append_vector_by_another_vector(U& dst, const V& src)
{
  dst.reserve(dst.size() + src.size());
  dst.insert(dst.end(), src.begin(), src.end());
}


//--------------------------------------------------------------------------


#define PRINT_EVENT_N(VEC_EVENTS)            std::cout << concolor::yellow << ">EVENT # " << VEC_EVENTS.size() << ", line " << STR(__LINE__) << concolor::normal << std::endl
#define PRINT_EVENT_N_TEXT(VEC_EVENTS, text) std::cout << concolor::yellow << ">EVENT # " << VEC_EVENTS.size() << ", line " << STR(__LINE__) << "  " << text << concolor::normal << std::endl


#define GENERATE_ACCOUNT(account)                                                     \
  currency::account_base account;                                                     \
  account.generate()

#define MAKE_ACCOUNT(VEC_EVENTS, account)                                             \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_ACCOUNT(" << #account << ")");                 \
  currency::account_base account;                                                     \
  account.generate();                                                                 \
  VEC_EVENTS.push_back(account)

#define DO_CALLBACK(VEC_EVENTS, CB_NAME) \
  {                                                                                   \
    PRINT_EVENT_N_TEXT(VEC_EVENTS, "DO_CALLBACK(" << #CB_NAME << ")");                \
    callback_entry CALLBACK_ENTRY;                                                    \
    CALLBACK_ENTRY.callback_name = CB_NAME;                                           \
    VEC_EVENTS.push_back(CALLBACK_ENTRY);                                             \
  }

#define DO_CALLBACK_PARAMS(VEC_EVENTS, CB_NAME, PARAMS_POD_OBJ)                       \
  {                                                                                   \
    PRINT_EVENT_N_TEXT(VEC_EVENTS, "DO_CALLBACK_PARAMS(" << #CB_NAME << ")");         \
    callback_entry ce = AUTO_VAL_INIT(ce);                                            \
    ce.callback_name = CB_NAME;                                                       \
    ce.callback_params = epst::pod_to_hex(PARAMS_POD_OBJ);                            \
    VEC_EVENTS.push_back(ce);                                                         \
  }

#define DO_CALLBACK_PARAMS_STR(VEC_EVENTS, CB_NAME, STR_PARAMS)                       \
  {                                                                                   \
    PRINT_EVENT_N_TEXT(VEC_EVENTS, "DO_CALLBACK_PARAMS_STR(" << #CB_NAME << ")");     \
    callback_entry ce = AUTO_VAL_INIT(ce);                                            \
    ce.callback_name = CB_NAME;                                                       \
    ce.callback_params = STR_PARAMS;                                                  \
    VEC_EVENTS.push_back(ce);                                                         \
  }




#define REGISTER_CALLBACK(CB_NAME, CLBACK) \
  register_callback(CB_NAME, boost::bind(&CLBACK, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3))

#define REGISTER_CALLBACK_METHOD(CLASS, METHOD) \
  register_callback(#METHOD, boost::bind(&CLASS::METHOD, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3))

#define MAKE_GENESIS_BLOCK(VEC_EVENTS, BLK_NAME, MINER_ACC, TS)                       \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_GENESIS_BLOCK(" << #BLK_NAME << ")");          \
  test_generator generator;                                                           \
  this->on_test_generator_created(generator);                                         \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_genesis_block(BLK_NAME, MINER_ACC, TS);                         \
  VEC_EVENTS.push_back(BLK_NAME)

#define MAKE_NEXT_BLOCK(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC)                  \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_NEXT_BLOCK(" << #BLK_NAME << ")");             \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC);             \
  VEC_EVENTS.push_back(BLK_NAME)



#define MAKE_NEXT_BLOCK_TIMESTAMP_ADJUSTMENT(ADJ, VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC) \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_NEXT_BLOCK_TIMESTAMP_ADJUSTMENT(" << #BLK_NAME << ")"); \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(ADJ, VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC);        \
  VEC_EVENTS.push_back(BLK_NAME)

#define MAKE_NEXT_POS_BLOCK(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, MINERS_ACC_LIST) \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_NEXT_POS_BLOCK(" << #BLK_NAME << ")");         \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, std::list<currency::transaction>(), MINERS_ACC_LIST); \
  VEC_EVENTS.push_back(BLK_NAME)

#define MAKE_NEXT_POS_BLOCK_TX1(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, MINERS_ACC_LIST, TX_1)         \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_NEXT_POS_BLOCK_TX1(" << #BLK_NAME << ")");                           \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                                       \
  {                                                                                                         \
    std::list<currency::transaction> tx_list;                                                               \
    tx_list.push_back(TX_1);                                                                                \
    if (!generator.construct_block(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, tx_list, MINERS_ACC_LIST))  \
      return false;                                                                                         \
  }                                                                                                         \
  VEC_EVENTS.push_back(BLK_NAME)

#define MAKE_NEXT_BLOCK_NO_ADD(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC)           \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_NEXT_BLOCK_NO_ADD(" << #BLK_NAME << ")");      \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC);             \
  VEC_EVENTS.push_back(event_special_block(BLK_NAME, event_special_block::flag_skip))

#define ADD_BLOCK(VEC_EVENTS, BLK_NAME)                                               \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "ADD_BLOCK(" << #BLK_NAME << ")");                   \
  VEC_EVENTS.push_back(BLK_NAME)

#define MAKE_NEXT_BLOCK_TX1(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TX1)         \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_NEXT_BLOCK_TX1(" << #BLK_NAME << ", " << #TX1 << ")"); \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  {                                                                                   \
    std::list<currency::transaction> tx_list;                                         \
    tx_list.push_back(TX1);                                                           \
    generator.construct_block(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, tx_list);  \
  }                                                                                   \
  VEC_EVENTS.push_back(BLK_NAME)


#define MAKE_NEXT_BLOCK_TX_LIST(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST)  \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "MAKE_NEXT_BLOCK_TX_LIST(" << #BLK_NAME << ", " << #TXLIST << ")"); \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST);     \
  VEC_EVENTS.push_back(BLK_NAME)

#define REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT)           \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  {                                                                                   \
    currency::block blk_last = PREV_BLOCK;                                            \
    for (size_t i = 0; i < COUNT; ++i)                                                \
    {                                                                                 \
      MAKE_NEXT_BLOCK(VEC_EVENTS, blk, blk_last, MINER_ACC);                          \
      blk_last = blk;                                                                 \
    }                                                                                 \
    BLK_NAME = blk_last;                                                              \
  }

#define REWIND_BLOCKS_N_WITH_TIME(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT) \
  currency::block BLK_NAME = PREV_BLOCK;                                              \
  for (size_t i = 0; i < COUNT; ++i)                                                  \
  {                                                                                   \
    PRINT_EVENT_N_TEXT(VEC_EVENTS, "REWIND_BLOCKS_N_WITH_TIME(" << #BLK_NAME << ", " << #PREV_BLOCK << ", " << #MINER_ACC << ", " << #COUNT << ")"); \
    currency::block next_block = AUTO_VAL_INIT(next_block);                           \
    generator.construct_block(VEC_EVENTS, next_block, BLK_NAME, MINER_ACC);           \
    VEC_EVENTS.push_back(event_core_time(next_block.timestamp - 10));                 \
    VEC_EVENTS.push_back(next_block);                                                 \
    BLK_NAME = next_block;                                                            \
  }

#define REWIND_BLOCKS(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC) REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, CURRENCY_MINED_MONEY_UNLOCK_WINDOW)


#define MAKE_TX_MIX_ATTR_EXTRA(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD, MIX_ATTR, EXTRA, CHECK_SPENDS) \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "transaction " << #TX_NAME);                          \
  currency::transaction TX_NAME;                                                       \
  {                                                                                    \
    bool txr = construct_tx_to_key(generator.get_hardforks(), VEC_EVENTS, TX_NAME, HEAD, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, NMIX, generator.last_tx_generated_secret_key, MIX_ATTR, EXTRA, std::vector<currency::attachment_v>(), CHECK_SPENDS);  \
    CHECK_AND_ASSERT_THROW_MES(txr, "failed to construct transaction");                \
  }                                                                                    \
  VEC_EVENTS.push_back(TX_NAME)

#define MAKE_TX_FEE_MIX_ATTR_EXTRA(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, FEE, NMIX, HEAD, MIX_ATTR, EXTRA, CHECK_SPENDS) \
  PRINT_EVENT_N_TEXT(VEC_EVENTS, "transaction " << #TX_NAME);                          \
  currency::transaction TX_NAME;                                                       \
  {                                                                                    \
    bool txr = construct_tx_to_key(generator.get_hardforks(), VEC_EVENTS, TX_NAME, HEAD, FROM, TO, AMOUNT, FEE, NMIX, generator.last_tx_generated_secret_key, MIX_ATTR, EXTRA, std::vector<currency::attachment_v>(), CHECK_SPENDS); \
    CHECK_AND_ASSERT_THROW_MES(txr, "failed to construct transaction");                \
  }                                                                                    \
  VEC_EVENTS.push_back(TX_NAME)

#define MAKE_TX_MIX_ATTR(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD, MIX_ATTR, CHECK_SPENDS) \
  MAKE_TX_MIX_ATTR_EXTRA(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD, MIX_ATTR, std::vector<currency::extra_v>(), CHECK_SPENDS);

#define MAKE_TX_FEE_MIX_ATTR(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, FEE, NMIX, HEAD, MIX_ATTR, CHECK_SPENDS) \
  MAKE_TX_FEE_MIX_ATTR_EXTRA(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, FEE, NMIX, HEAD, MIX_ATTR, std::vector<currency::extra_v>(), CHECK_SPENDS);

#define MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD)   MAKE_TX_MIX_ATTR(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD, CURRENCY_TO_KEY_OUT_RELAXED, true)

#define MAKE_TX_FEE_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, FEE, NMIX, HEAD)   MAKE_TX_FEE_MIX_ATTR(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, FEE, NMIX, HEAD, CURRENCY_TO_KEY_OUT_RELAXED, true)

#define MAKE_TX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, 0, HEAD)

#define MAKE_TX_FEE(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, FEE, HEAD) MAKE_TX_FEE_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, FEE, 0, HEAD)


#define MAKE_TX_MIX_LIST_EXTRA_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD, MIX_ATTR, EXTRA, ATTACH) \
  {                                                                                                \
    PRINT_EVENT_N_TEXT(VEC_EVENTS, "transaction list " << #SET_NAME);                              \
    currency::transaction t;                                                                       \
    bool r = construct_tx_to_key(generator.get_hardforks(), VEC_EVENTS, t, HEAD, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, NMIX, generator.last_tx_generated_secret_key, MIX_ATTR, EXTRA, ATTACH); \
    if (!r) { LOG_PRINT_YELLOW("ERROR in tx @ EVENT #" << VEC_EVENTS.size(), LOG_LEVEL_0); }       \
    CHECK_AND_ASSERT_THROW_MES(r, "failed to construct transaction");                              \
    SET_NAME.push_back(t);                                                                         \
    VEC_EVENTS.push_back(t);                                                                       \
  }

#define MAKE_TX_MIX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD, MIX_ATTR, ATTACH)    \
  MAKE_TX_MIX_LIST_EXTRA_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD, MIX_ATTR, std::vector<currency::extra_v>(), ATTACH)


#define MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD, ATTACH) MAKE_TX_MIX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD, CURRENCY_TO_KEY_OUT_RELAXED, ATTACH)

#define MAKE_TX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD, std::vector<currency::attachment_v>())

#define MAKE_TX_LIST_ATTACH(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, ATTACH) MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD, ATTACH)


#define MAKE_TX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, MIX_ATTR) MAKE_TX_MIX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD, MIX_ATTR, std::vector<currency::attachment_v>())

#define MAKE_TX_LIST_START_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, MIX_ATTR, ATTACHS) \
    std::list<currency::transaction> SET_NAME; \
    MAKE_TX_MIX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD, MIX_ATTR, ATTACHS)

#define MAKE_TX_LIST_START(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_LIST_START_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, CURRENCY_TO_KEY_OUT_RELAXED, std::vector<currency::attachment_v>())

#define MAKE_TX_LIST_START_WITH_ATTACHS(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, ATTACHS) MAKE_TX_LIST_START_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, CURRENCY_TO_KEY_OUT_RELAXED, ATTACHS)

#define MAKE_TX_ATTACH_FEE(EVENTS, TX_VAR, FROM, TO, AMOUNT, FEE, HEAD, ATTACH)        \
  PRINT_EVENT_N(EVENTS);                                                               \
  currency::transaction TX_VAR = AUTO_VAL_INIT(TX_VAR);                                \
  CHECK_AND_ASSERT_MES(construct_tx_to_key(generator.get_hardforks(), EVENTS, TX_VAR, HEAD, FROM, TO, AMOUNT, FEE, 0, generator.last_tx_generated_secret_key, CURRENCY_TO_KEY_OUT_RELAXED, empty_extra, ATTACH), false, "construct_tx_to_key failed"); \
  EVENTS.push_back(TX_VAR)

#define MAKE_TX_ATTACH(EVENTS, TX_VAR, FROM, TO, AMOUNT, HEAD, ATTACH) MAKE_TX_ATTACH_FEE(EVENTS, TX_VAR, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, HEAD, ATTACH)

#define MAKE_MINER_TX_AND_KEY_MANUALLY(TX, PREV_BLOCK, P_KEYPAIR)                                                       \
  transaction TX;                                                                                                       \
  if (!construct_miner_tx_manually(get_block_height(PREV_BLOCK) + 1, generator.get_already_generated_coins(PREV_BLOCK), \
    miner_account.get_keys().account_address, TX, 0, P_KEYPAIR))                                                      \
    return false;

#define MAKE_MINER_TX_MANUALLY(TX, PREV_BLOCK) MAKE_MINER_TX_AND_KEY_MANUALLY(TX, PREV_BLOCK, nullptr)

#define SET_EVENT_VISITOR_SETT(VEC_EVENTS, SETT, VAL) VEC_EVENTS.push_back(event_visitor_settings(SETT, VAL));

#define QUOTEME(x) #x
#define CHECK_TEST_CONDITION(cond) CHECK_AND_FORCE_ASSERT_MES(cond, false, "failed: \"" << QUOTEME(cond) << "\"")
#define CHECK_EQ(v1, v2) CHECK_AND_FORCE_ASSERT_MES(v1 == v2, false, "failed: \"" << QUOTEME(v1) << " == " << QUOTEME(v2) << "\", " << v1 << " != " << v2)
#define CHECK_NOT_EQ(v1, v2) CHECK_AND_FORCE_ASSERT_MES(!(v1 == v2), false, "failed: \"" << QUOTEME(v1) << " != " << QUOTEME(v2) << "\", " << v1 << " == " << v2)
#define CHECK_V_EQ_EXPECTED_AND_ASSERT(value, expected) CHECK_AND_ASSERT_MES((value) == (expected), false, QUOTEME(value) << " has wrong value: " << value << ", expected: " << expected)

// Adjust gentime and playtime "time" at once
#define ADJUST_TEST_CORE_TIME(desired_time)                                            \
  PRINT_EVENT_N_TEXT(events, "ADJUST_TEST_CORE_TIME(" << desired_time << ")");         \
  test_core_time::adjust(desired_time);                                                \
  events.push_back(event_core_time(desired_time))

#define ADD_CUSTOM_EVENT_CODE(VEC_EVENTS, CODE) PRINT_EVENT_N_TEXT(VEC_EVENTS, #CODE); CODE

#define ADD_CUSTOM_EVENT(VEC_EVENTS, EVENT_OBJ) PRINT_EVENT_N_TEXT(VEC_EVENTS, #EVENT_OBJ); VEC_EVENTS.push_back(EVENT_OBJ)


#define SET_HARDFORKS_TO_OLD_TESTS()                                                                                               \
  core_hardforks_config hardforks_update = AUTO_VAL_INIT(hardforks_update);                                                        \
  hardforks_update.hardforks = get_default_core_runtime_config().hard_forks.m_height_the_hardfork_n_active_after;                  \
  hardforks_update.hardforks[1] = 1440;                                                                                            \
  hardforks_update.hardforks[2] = 1800;                                                                                            \
  hardforks_update.hardforks[3] = 1801;                                                                                            \
  currency::hard_forks_descriptor hardforks_desc = AUTO_VAL_INIT(hardforks_desc);                                                  \
  hardforks_desc.m_height_the_hardfork_n_active_after = hardforks_update.hardforks;                                                \
  generator.set_hardforks(hardforks_desc);                                                                                         \
  m_hardforks.m_height_the_hardfork_n_active_after = hardforks_desc.m_height_the_hardfork_n_active_after;                          \
  events.push_back(hardforks_update);

// --- gentime wallet helpers -----------------------------------------------------------------------

#define CREATE_TEST_WALLET(WLT_VAR, ACCOUNT, GENESIS_BLOCK) \
  std::shared_ptr<tools::wallet2> WLT_VAR; \
  generator.init_test_wallet(ACCOUNT, get_block_hash(GENESIS_BLOCK), WLT_VAR)

#define REFRESH_TEST_WALLET_AT_GEN_TIME(EVENTS_VEC, WLT_WAR, TOP_BLOCK, EXPECTED_BLOCKS_TO_BE_FETCH) \
  { \
    bool r = generator.refresh_test_wallet(EVENTS_VEC, WLT_WAR.get(), get_block_hash(TOP_BLOCK), EXPECTED_BLOCKS_TO_BE_FETCH); \
    CHECK_AND_ASSERT_MES(r, false, "refresh_test_wallet failed"); \
  }

#define MAKE_TEST_WALLET_TX(EVENTS_VEC, TX_VAR, WLT_WAR, MONEY, DEST_ACC)              \
  PRINT_EVENT_N(EVENTS_VEC);                                                           \
  transaction TX_VAR = AUTO_VAL_INIT(TX_VAR);                                          \
  {                                                                                    \
    std::vector<tx_destination_entry> destinations(1, tx_destination_entry(MONEY, DEST_ACC.get_public_address())); \
    WLT_WAR->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>(), TX_VAR); \
  }                                                                                    \
  EVENTS_VEC.push_back(TX_VAR)

#define MAKE_TEST_WALLET_TX_ATTACH(EVENTS_VEC, TX_VAR, WLT_WAR, MONEY, DEST_ACC, ATTACH) \
  PRINT_EVENT_N(EVENTS_VEC);                                                           \
  transaction TX_VAR = AUTO_VAL_INIT(TX_VAR);                                          \
  {                                                                                    \
    std::vector<tx_destination_entry> destinations(1, tx_destination_entry(MONEY, DEST_ACC.get_public_address())); \
    WLT_WAR->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), ATTACH, TX_VAR); \
  }                                                                                    \
  EVENTS_VEC.push_back(TX_VAR)

#define MAKE_TEST_WALLET_TX_EXTRA(EVENTS_VEC, TX_VAR, WLT_WAR, MONEY, DEST_ACC, EXTRA) \
  PRINT_EVENT_N(EVENTS_VEC);                                                           \
  transaction TX_VAR = AUTO_VAL_INIT(TX_VAR);                                          \
  {                                                                                    \
    std::vector<tx_destination_entry> destinations(1, tx_destination_entry(MONEY, DEST_ACC.get_public_address())); \
    WLT_WAR->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, EXTRA, std::vector<attachment_v>(), TX_VAR); \
  }                                                                                    \
  EVENTS_VEC.push_back(TX_VAR)

#define CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(WLT_WAR, TOTAL_BALANCE) \
  if (!check_balance_via_wallet(*WLT_WAR.get(), #WLT_WAR, TOTAL_BALANCE)) \
    return false

#define CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(WLT_WAR, TOTAL_BALANCE, MINED_BALANCE, UNLOCKED_BALANCE, AWAITING_IN_BALANCE, AWAITING_OUT_BALANCE) \
  if (!check_balance_via_wallet(*WLT_WAR.get(), #WLT_WAR, TOTAL_BALANCE, MINED_BALANCE, UNLOCKED_BALANCE, AWAITING_IN_BALANCE, AWAITING_OUT_BALANCE)) \
    return false

// --- end of gentime wallet helpers -----------------------------------------------------------------------

#include "chaingen_helpers.h"
