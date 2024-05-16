// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/list.hpp>

#include <boost/foreach.hpp>
#include <atomic>

#include "file_io_utils.h"
#include "serialization/serialization.h"
#include "serialization/string.h"
#include "serialization/multiprecision.h"

#include "tx_pool.h"
#include "blockchain_storage_basic.h"
#include "common/util.h"
#include "common/db_abstract_accessor.h"
#include "currency_protocol/currency_protocol_defs.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "difficulty.h"
#include "common/difficulty_boost_serialization.h"
#include "currency_core/currency_format_utils.h"
#include "verification_context.h"
#include "crypto/hash.h"
#include "checkpoints.h"
#include "core_runtime_config.h"
#include "dispatch_core_events.h"
#include "bc_attachments_service_manager.h"
#include "common/median_db_cache.h"
#include "common/variant_helper.h"


MARK_AS_POD_C11(crypto::key_image);
typedef std::pair<crypto::hash, uint64_t> macro_alias_1;
MARK_AS_POD_C11(macro_alias_1);

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "core"

namespace currency
{

  // blue core

  class blockchain_storage
  {
  public:
    struct performnce_data
    {
      //block processing zone
      epee::math_helper::average<uint64_t, 5> block_processing_time_0_ms;
      epee::math_helper::average<uint64_t, 5> block_processing_time_1;
      epee::math_helper::average<uint64_t, 5> target_calculating_time_2;
      epee::math_helper::average<uint64_t, 5> longhash_calculating_time_3;
      epee::math_helper::average<uint64_t, 5> all_txs_insert_time_5;
      epee::math_helper::average<uint64_t, 5> etc_stuff_6;
      epee::math_helper::average<uint64_t, 5> insert_time_4;
      epee::math_helper::average<uint64_t, 5> raise_block_core_event;
      epee::math_helper::average<uint64_t, 5> validate_miner_transaction_time;
      epee::math_helper::average<uint64_t, 5> collect_rangeproofs_data_from_tx_time;
      epee::math_helper::average<uint64_t, 5> verify_multiple_zc_outs_range_proofs_time;
      
      
      //target_calculating_time_2
      epee::math_helper::average<uint64_t, 5> target_calculating_enum_blocks;
      epee::math_helper::average<uint64_t, 5> target_calculating_calc;

      //longhash_calculating_time_3
      epee::math_helper::average<uint64_t, 1> pos_validate_ki_search;
      epee::math_helper::average<uint64_t, 1> pos_validate_get_out_keys_for_inputs;
      epee::math_helper::average<uint64_t, 1> pos_validate_zvp;

      //tx processing zone
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_time;
      epee::math_helper::average<uint64_t, 1> tx_add_one_tx_time;
      epee::math_helper::average<uint64_t, 1> tx_process_extra;
      epee::math_helper::average<uint64_t, 1> tx_process_attachment;
      epee::math_helper::average<uint64_t, 1> tx_process_inputs ;
      epee::math_helper::average<uint64_t, 1> tx_push_global_index;
      epee::math_helper::average<uint64_t, 1> tx_check_exist;
      epee::math_helper::average<uint64_t, 1> tx_print_log;
      epee::math_helper::average<uint64_t, 1> tx_prapare_append;
              
      epee::math_helper::average<uint64_t, 1> tx_append_time;
      epee::math_helper::average<uint64_t, 1> tx_append_rl_wait;
      epee::math_helper::average<uint64_t, 1> tx_append_is_expired;

      epee::math_helper::average<uint64_t, 1> tx_store_db;

      epee::math_helper::average<uint64_t, 1> tx_check_inputs_prefix_hash;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_attachment_check;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop_kimage_check;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop_ch_in_val_sig;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop_scan_outputkeys_get_item_size;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop_scan_outputkeys_relative_to_absolute;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop_scan_outputkeys_loop;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop_scan_outputkeys_loop_get_subitem;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop_scan_outputkeys_loop_find_tx;
      epee::math_helper::average<uint64_t, 1> tx_check_inputs_loop_scan_outputkeys_loop_handle_output;

      epee::math_helper::average<uint64_t, 1> tx_mixin_count;

      
      //TODO: move this to suitable place or remove it all
      std::atomic<bool> epic_failure_happend;
      tools::db::stat_info si;
    };


    struct key_images_ptr_compare
    {
      bool operator()(const std::shared_ptr<crypto::key_image>& a, const std::shared_ptr<crypto::key_image>& b) const
      {
        return *a == *b;
      }
    };

    struct key_images_ptr_hash
    {
      std::size_t operator()(const std::shared_ptr<crypto::key_image>& i) const 
      {
        return crypto::hash_value(*i); // TODO: BAD hash function, replace with something better!
      }
    };

    struct key_images_ptr_less
    {
      typedef bool result_type;
      bool operator()(const std::shared_ptr<crypto::key_image>& _Left, const std::shared_ptr<crypto::key_image>& _Right) const
      {
        return (*_Left < *_Right);
      }
    };

    struct scan_for_keys_context
    {
      bool htlc_is_expired;
      std::list<txout_htlc> htlc_outs;
      std::list<tx_out_zarcanum> zc_outs;
    };

    // == Output indexes local lookup table conception ==
    // Main chain gindex table (outputs_container) contains data which is valid only for the most recent block.
    // Thus it can't be used to get output's global index for any arbitrary height because there's no height data.
    // Having local gindex lookup table for a given height one could retrieve global output index table as it was
    // at the given height using the following algorithm:
    //
    // local_gindex_lookup_table = calculate_local_gindex_lookup_table_for_height(height)
    // if amount exists in local_gindex_lookup_table:
    //   retrieve gindex from local_gindex_lookup_table   # there are outputs having given amount after the given height
    // else:
    //   retrieve gindex from main chain gindex table     # not outputs having given amount are present after the given height
    //

    typedef boost::variant<crypto::public_key, txout_htlc> output_key_or_htlc_v;

    struct alt_block_extended_info: public block_extended_info
    {
      // {amount -> gindex } output global index lookup table for this altblock (if an amount isn't present -- it's retreived from main outputs_container)
      std::map<uint64_t, uint64_t> gindex_lookup_table; 
      
      // {amount -> pub_keys} map of outputs' pub_keys appeared in this alt block ( index_in_vector == output_gindex - gindex_lookup_table[output_amount] )
      std::map<uint64_t, std::vector<output_key_or_htlc_v> > outputs_pub_keys;
      
      //date added to alt chain storage
      uint64_t timestamp; 
      
      //transactions associated with the block
      transactions_map onboard_transactions;
    };
    typedef std::unordered_map<crypto::hash, alt_block_extended_info> alt_chain_container;
    typedef std::vector<alt_chain_container::iterator> alt_chain_type; // alternative subchain, front -> mainchain(split point), back -> alternative head

    typedef std::unordered_map<crypto::hash, block_extended_info> blocks_ext_by_hash;

    typedef tools::db::basic_key_to_array_accessor<uint64_t, global_output_entry, false>  outputs_container; // out_amount => ['global_output', ...]
    typedef tools::db::cached_key_value_accessor<crypto::key_image, uint64_t, false, false> key_images_container;
    typedef std::list<epee::misc_utils::triple<std::shared_ptr<const block_extended_info>, std::list<std::shared_ptr<const transaction_chain_entry> >, std::shared_ptr<const transaction_chain_entry> > > blocks_direct_container;

    friend struct add_transaction_input_visitor;
    //---------------------------------------------------------------------------------

    blockchain_storage(tx_memory_pool& tx_pool);
    ~blockchain_storage();


    bool init(const boost::program_options::variables_map& vm) { return init(tools::get_default_data_dir(), vm); }
    bool init(const std::string& config_folder, const boost::program_options::variables_map& vm);
    bool deinit();
    static void init_options(boost::program_options::options_description& desc);

    bool set_checkpoints(checkpoints&& chk_pts);

    //TODO: set this method to const
    checkpoints& get_checkpoints() { return m_checkpoints; }
    bool is_in_checkpoint_zone() const { return m_is_in_checkpoint_zone; }
   
    //------------- modifying members --------------
    bool add_new_block(const block& bl_, block_verification_context& bvc);
    bool prevalidate_block(const block& bl);
    bool clear();
    bool reset_and_set_genesis_block(const block& b);
    //debug function
    bool truncate_blockchain(uint64_t to_height);
    //------------- readers members -----------------
    bool pre_validate_relayed_block(block& b, block_verification_context& bvc, const crypto::hash& id)const ;
    //bool push_new_block();
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs) const ;
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks) const;
    bool get_main_blocks_rpc_details(uint64_t start_offset, size_t count, bool ignore_transactions, std::list<block_rpc_extended_info>& blocks) const;
    bool get_main_block_rpc_details(uint64_t i, block_rpc_extended_info& bei) const;
    bool get_main_block_rpc_details(const crypto::hash& id, block_rpc_extended_info& bei) const;
    bool get_alt_block_rpc_details(const crypto::hash& id, block_rpc_extended_info& bei) const;
    bool get_alt_block_rpc_details(const block_extended_info& bei_core, const crypto::hash& id, block_rpc_extended_info& bei) const;
    bool get_alt_blocks_rpc_details(uint64_t start_offset, uint64_t count, std::vector<block_rpc_extended_info>& blocks) const;
    bool get_tx_rpc_details(const crypto::hash&, tx_rpc_extended_info& tei, uint64_t timestamp, bool is_short) const;
    bool search_by_id(const crypto::hash& id, std::list<std::string>& res) const;
    bool get_global_index_details(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::response & resp) const;
    bool get_multisig_id_details(const COMMAND_RPC_GET_MULTISIG_INFO::request& req, COMMAND_RPC_GET_MULTISIG_INFO::response & resp) const;
    bool get_multisig_id_details(const crypto::hash& ms_id, crypto::hash& tx_id, uint64_t& out_no) const;
    bool get_alternative_blocks(std::list<block>& blocks) const;
    size_t get_alternative_blocks_count() const;
    crypto::hash get_block_id_by_height(uint64_t height) const;
    bool get_block_by_hash(const crypto::hash &h, block &blk) const;
    bool get_block_extended_info_by_height(uint64_t h, block_extended_info &blk) const;
    bool get_block_extended_info_by_hash(const crypto::hash &h, block_extended_info &blk) const;
    bool get_block_by_height(uint64_t h, block &blk) const;
    bool is_tx_related_to_altblock(crypto::hash tx_id) const;
    //void get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid) const;

    bc_attachment_services_manager& get_attachment_services_manager(){ return m_services_mgr; }

    bool have_tx(const crypto::hash &id) const;
    bool have_tx_keyimges_as_spent(const transaction &tx) const;
    bool have_tx_keyimg_as_spent(const crypto::key_image &key_im, uint64_t before_height = UINT64_MAX) const;
    std::shared_ptr<transaction> get_tx(const crypto::hash &id) const;


    template<class visitor_t>
    bool scan_outputkeys_for_indexes(const transaction &validated_tx, const txin_v& in_v, visitor_t& vis) 
    { 
      scan_for_keys_context cntx_stub = AUTO_VAL_INIT(cntx_stub);
      uint64_t stub = 0; 
      return scan_outputkeys_for_indexes(validated_tx, in_v, vis, stub, cntx_stub);
    }
    template<class visitor_t>
    bool scan_outputkeys_for_indexes(const transaction &validated_tx, const txin_v& verified_input, visitor_t& vis, uint64_t& max_related_block_height, scan_for_keys_context& /*scan_context*/) const;
                                     

    uint64_t get_current_blockchain_size() const;
    uint64_t get_top_block_height() const;
    crypto::hash get_top_block_id() const;
    crypto::hash get_top_block_id(uint64_t& height) const;
    bool get_top_block(block& b) const;
    wide_difficulty_type get_next_diff_conditional(bool pos) const;
    wide_difficulty_type get_next_diff_conditional_alt(bool pos, const alt_chain_type& alt_chain, uint64_t split_height, const alt_block_extended_info& abei) const;
    wide_difficulty_type get_cached_next_difficulty(bool pos) const;
    wide_difficulty_type calc_diff_at_h_from_timestamps(std::vector<uint64_t>& timestamps, std::vector<wide_difficulty_type>& commulative_difficulties, uint64_t h, bool pos) const;
    void collect_timestamps_and_c_difficulties_main(std::vector<uint64_t>& timestamps, std::vector<wide_difficulty_type>& commulative_difficulties, bool pos) const;
    void collect_timestamps_and_c_difficulties_alt(std::vector<uint64_t>& timestamps, std::vector<wide_difficulty_type>& commulative_difficulties, bool pos, const alt_chain_type& alt_chain, uint64_t split_height) const;


    
    bool create_block_template(const account_public_address& miner_address, const blobdata& ex_nonce, block& b, wide_difficulty_type& di, uint64_t& height) const;
    bool create_block_template(const account_public_address& miner_address, const account_public_address& stakeholder_address, const blobdata& ex_nonce, bool pos, const pos_entry& pe, fill_block_template_func_t custom_fill_block_template_func, block& b, wide_difficulty_type& di, uint64_t& height, tx_generation_context* miner_tx_tgc_ptr = nullptr) const;
    bool create_block_template(const create_block_template_params& params, create_block_template_response& resp) const;

    bool have_block(const crypto::hash& id) const;
    size_t get_total_transactions()const;
    bool get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys)const;
    bool get_short_chain_history(std::list<crypto::hash>& ids)const;
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp)const;
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, uint64_t& starter_offset)const;
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count, uint64_t minimum_height = 0)const;
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, blocks_direct_container& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count, uint64_t minimum_height = 0)const;
    //bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count)const;
    bool handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp)const;
    bool handle_get_objects(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)const;
    bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)const;
    bool get_random_outs_for_amounts3(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response& res)const;
    bool get_backward_blocks_sizes(size_t from_height, std::vector<size_t>& sz, size_t count)const;
    bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs)const;
    bool get_alias_info(const std::string& alias, extra_alias_entry_base& info)const;
    std::string get_alias_by_address(const account_public_address& addr)const;
    std::set<std::string> get_aliases_by_address(const account_public_address& addr)const;
    template<typename cb_t>
    bool enumerate_aliases(cb_t cb) const;
    template<typename cb_t>
    bool get_aliases(cb_t cb, uint64_t offset, uint64_t count) const;
    uint64_t get_aliases_count()const;
    uint64_t get_block_h_older_then(uint64_t timestamp) const;
    bool validate_tx_service_attachmens_in_services(const tx_service_attachment& a, size_t i, const transaction& tx)const;
    bool get_asset_history(const crypto::public_key& asset_id, std::list<asset_descriptor_operation>& result) const;
    bool get_asset_info(const crypto::public_key& asset_id, asset_descriptor_base& info)const;
    uint64_t get_assets_count() const;
    uint64_t get_assets(uint64_t offset, uint64_t count, std::list<asset_descriptor_with_id>& assets) const;
    bool check_tx_input(const transaction& tx, size_t in_index, const txin_to_key& txin, const crypto::hash& tx_prefix_hash, uint64_t& max_related_block_height, uint64_t& source_max_unlock_time_for_pos_coinbase)const;
    bool check_tx_input(const transaction& tx, size_t in_index, const txin_multisig& txin, const crypto::hash& tx_prefix_hash, uint64_t& max_related_block_height)const;
    bool check_tx_input(const transaction& tx, size_t in_index, const txin_htlc& txin, const crypto::hash& tx_prefix_hash, uint64_t& max_related_block_height)const;
    bool check_tx_input(const transaction& tx, size_t in_index, const txin_zc_input& zc_in, const crypto::hash& tx_prefix_hash, uint64_t& max_related_block_height, bool& all_tx_ins_have_explicit_native_asset_ids) const;
    bool check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash, uint64_t& max_used_block_height)const;
    bool check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash) const;
    bool check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash, uint64_t& max_used_block_height, crypto::hash& max_used_block_id)const;
    bool check_ms_input(const transaction& tx, size_t in_index, const txin_multisig& txin, const crypto::hash& tx_prefix_hash, const transaction& source_tx, size_t out_n) const;
    bool validate_tx_for_hardfork_specific_terms(const transaction& tx, const crypto::hash& tx_id, uint64_t block_height) const;
    bool validate_tx_for_hardfork_specific_terms(const transaction& tx, const crypto::hash& tx_id) const;
    bool get_output_keys_for_input_with_checks(const transaction& tx, const txin_v& verified_input, std::vector<crypto::public_key>& output_keys, uint64_t& max_related_block_height, uint64_t& source_max_unlock_time_for_pos_coinbase, scan_for_keys_context& scan_context) const;
    bool get_output_keys_for_input_with_checks(const transaction& tx, const txin_v& verified_input, std::vector<crypto::public_key>& output_keys, uint64_t& max_related_block_height, uint64_t& source_max_unlock_time_for_pos_coinbase) const;
    bool check_input_signature(const transaction& tx, size_t in_index, const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<const crypto::public_key*>& output_keys_ptrs) const;
    bool check_input_signature(const transaction& tx, 
      size_t in_index, 
      uint64_t in_amount, 
      const crypto::key_image& k_image,
      const std::vector<txin_etc_details_v>& in_etc_details,
      const crypto::hash& tx_prefix_hash, 
      const std::vector<const crypto::public_key*>& output_keys_ptrs) const;

    uint64_t get_current_comulative_blocksize_limit()const;
    uint64_t get_current_hashrate(size_t aprox_count)const;
    uint64_t get_seconds_between_last_n_block(size_t n)const;
    bool has_multisig_output(const crypto::hash& multisig_id) const;
    bool is_multisig_output_spent(const crypto::hash& multisig_id) const;
    boost::multiprecision::uint128_t total_coins()const;
    bool is_pos_allowed()const;
    uint64_t get_tx_fee_median()const;
    uint64_t get_tx_fee_window_value_median() const;
    uint64_t get_tx_expiration_median() const;
    uint64_t validate_alias_reward(const transaction& tx, const std::string& ai)const;
    void set_event_handler(i_core_event_handler* event_handler) const;
    i_core_event_handler* get_event_handler() const;
    uint64_t get_last_timestamps_check_window_median() const;
    uint64_t get_last_n_blocks_timestamps_median(size_t n) const;
    bool prevalidate_alias_info(const transaction& tx, const extra_alias_entry& eae);
    bool calculate_block_reward_for_next_top_block(size_t next_block_cumulative_size, uint64_t& block_reward_without_fee) const;
    bool validate_miner_transaction(const transaction& miner_tx, uint64_t fee, uint64_t block_reward_without_fee) const;
    performnce_data& get_performnce_data()const;
    bool validate_instance(const std::string& path);
    bool is_tx_expired(const transaction& tx) const;
    std::shared_ptr<const transaction_chain_entry> find_key_image_and_related_tx(const crypto::key_image& ki, crypto::hash& id_result) const;

    // returns true as soon as the hardfork is active for the NEXT upcoming block (not for the top block in the blockchain storage)
    bool is_hardfork_active(size_t hardfork_id) const;
    bool is_hardfork_active_for_height(size_t hardfork_id, uint64_t height) const;
    bool fill_tx_rpc_inputs(tx_rpc_extended_info& tei, const transaction& tx) const;
    bool fill_tx_rpc_details(tx_rpc_extended_info& tei, const transaction& tx, const transaction_chain_entry* ptce, const crypto::hash& h, uint64_t timestamp, bool is_short = false) const;

    wide_difficulty_type block_difficulty(size_t i)const;
    bool forecast_difficulty(std::vector<std::pair<uint64_t, wide_difficulty_type>> &out_height_2_diff_vector, bool pos) const;
    bool prune_aged_alt_blocks();
    bool get_transactions_daily_stat(uint64_t& daily_cnt, uint64_t& daily_volume)const;
    bool check_keyimages(const std::list<crypto::key_image>& images, std::list<uint64_t>& images_stat)const;//true - unspent, false - spent
    bool build_kernel(const block& bl, stake_kernel& kernel, uint64_t& amount, const stake_modifier_type& stake_modifier)const;
    // --- PoS ---  
    bool build_kernel(const crypto::key_image& ki,
      stake_kernel& kernel,
      const stake_modifier_type& stake_modifier,
      uint64_t timestamp) const;
    bool build_stake_modifier(stake_modifier_type& sm, const alt_chain_type& alt_chain = alt_chain_type(), uint64_t split_height = 0, crypto::hash* p_last_block_hash = nullptr, uint64_t* p_last_pow_block_height = nullptr) const;

    bool validate_pre_zarcanum_pos_coinbase_outs_unlock_time(const transaction& miner_tx, uint64_t staked_amount, uint64_t source_max_unlock_time)const;
    bool validate_pos_block(const block& b, const crypto::hash& id, bool for_altchain)const;
    bool validate_pos_block(const block& b, wide_difficulty_type basic_diff, const crypto::hash& id, bool for_altchain)const;
    bool validate_pos_block(const block& b,
      wide_difficulty_type basic_diff,
      uint64_t& amount,
      wide_difficulty_type& final_diff,
      crypto::hash& kernel_hash,
      const crypto::hash& id,
      bool for_altchain,
      const alt_chain_type& alt_chain = alt_chain_type(),
      uint64_t split_height = 0)const;
    bool validate_asset_operation_against_current_blochain_state(asset_op_verification_context& avc) const;

    void set_core_runtime_config(const core_runtime_config& pc) const;
    const core_runtime_config& get_core_runtime_config()const;
    size_t get_current_sequence_factor(bool pos)const;
    bool get_last_n_blocks_sizes(std::vector<size_t>& sz, size_t count)const;
    std::shared_ptr<block_extended_info> get_last_block_of_type(bool looking_for_pos, const alt_chain_type& alt_chain = alt_chain_type(), uint64_t split_height = 0)const;
    void get_pos_mining_estimate(uint64_t amuont_coins,
      uint64_t time,
      uint64_t& estimate_result,
      uint64_t& pos_coins_and_pos_diff_rate,
      std::vector<uint64_t>& days)const;

    uint64_t get_alias_coast(const std::string& alias)const;
    const outputs_container& get_outputs_container() const { return m_db_outputs; }
    std::shared_ptr<const transaction_chain_entry> get_tx_chain_entry(const crypto::hash& tx_hash) const;
    bool get_tx_chain_entry(const crypto::hash& tx_hash, transaction_chain_entry& entry) const;
    template<typename cb_t>
    void enumerate_transactions(cb_t cb) const { CRITICAL_REGION_LOCAL(m_read_lock); m_db_transactions.enumerate_keys(cb); }



    //this function mostly made for debug purposes
    template<class t_event_details>
    void rise_core_event(const std::string& event_name, const t_event_details& ed)
    {
      core_event_v e(ed);
      m_event_handler->on_core_event(event_name, e);
    }

    template<class t_ids_container, class t_blocks_container, class t_missed_container>
    bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs)const
    {
      CRITICAL_REGION_LOCAL(m_read_lock);

      for(const auto& bl_id: block_ids)
      {
        auto block_ind_ptr = m_db_blocks_index.find(bl_id);
        if (!block_ind_ptr)
          missed_bs.push_back(bl_id);
        else
        {
          CHECK_AND_ASSERT_MES(*block_ind_ptr < m_db_blocks.size(), false, "Internal error: bl_id=" << epst::pod_to_hex(bl_id)
            << " have index record with offset=" << *block_ind_ptr << ", bigger then m_db_blocks.size()=" << m_db_blocks.size());
          blocks.push_back(m_db_blocks[*block_ind_ptr]->bl);
        }
      }
      return true;
    } 

    template<class t_ids_container, class t_tx_container, class t_missed_container>
    bool get_transactions(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs)const
    {
      CRITICAL_REGION_LOCAL(m_read_lock);

      BOOST_FOREACH(const auto& tx_id, txs_ids)
      {
        auto tx_ptr = m_db_transactions.find(tx_id);
        if (!tx_ptr)
        {
          transaction tx;
          if (!m_tx_pool.get_transaction(tx_id, tx))
            missed_txs.push_back(tx_id);
          else
            txs.push_back(tx);
        }
        else
          txs.push_back(tx_ptr->tx);
      }
      return true;
    }


    template<class t_ids_container, class t_tx_container, class t_missed_container>
    bool get_transactions_direct(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs)const
    {
      CRITICAL_REGION_LOCAL(m_read_lock);

      for(const auto& tx_id: txs_ids)
      {
        auto tx_ptr = m_db_transactions.find(tx_id);
        if (!tx_ptr)
          missed_txs.push_back(tx_id);
        else
          txs.push_back(tx_ptr);
      }
      return true;
    }

    //TODO: set to const
    void get_alternative_chains(alt_chain_container& ach)  { CRITICAL_REGION_LOCAL(m_alternative_chains_lock); ach = m_alternative_chains; }
    void set_alternative_chains(const alt_chain_container& ach)  { CRITICAL_REGION_LOCAL(m_alternative_chains_lock); m_alternative_chains = ach; }

    template<class archive_t>
    void serialize(archive_t & ar, const unsigned int version);
    bool get_est_height_from_date(uint64_t date, uint64_t& res_h)const;

    bool get_pos_votes(uint64_t start_h, uint64_t end_h, vote_results& r);

    //debug functions
    bool validate_blockchain_prev_links(size_t last_n_blocks_to_check = 10) const;
    bool print_transactions_statistics()const;
    void print_blockchain(uint64_t start_index, uint64_t end_index) const ;
    std::string get_blockchain_string(uint64_t start_index, uint64_t end_index) const;
    void print_blockchain_with_tx(uint64_t start_index, uint64_t end_index) const;
    void print_blockchain_index() const;
    void print_blockchain_outs(const std::string& file) const;
    void print_blockchain_outs_stats() const;
    void print_db_cache_perfeormance_data() const;
    void print_last_n_difficulty_numbers(uint64_t n) const;
    bool calc_tx_cummulative_blob(const block& bl)const;
    bool get_outs_index_stat(outs_index_stat& outs_stat)const;
    bool print_lookup_key_image(const crypto::key_image& ki) const;
    void reset_db_cache() const;
    void clear_altblocks();
    void inspect_blocks_index() const;
    bool rebuild_tx_fee_medians();
    bool validate_all_aliases_for_new_median_mode();
    bool print_tx_outputs_lookup(const crypto::hash& tx_id) const;
    uint64_t get_last_x_block_height(bool pos)const;
    bool is_tx_spendtime_unlocked(uint64_t unlock_time)const;

  private:

    //-------------- DB containers --------------
    typedef tools::db::cached_key_value_accessor<crypto::hash, uint64_t, false, false> blocks_by_id_index;
    typedef tools::db::cached_key_value_accessor<crypto::hash, transaction_chain_entry, true, false> transactions_container; 
    
    typedef tools::db::array_accessor<block_extended_info, true> blocks_container;      

    typedef tools::db::cached_key_value_accessor<std::string, std::list<extra_alias_entry_base>, true, true> aliases_container; 
    typedef tools::db::cached_key_value_accessor<account_public_address, std::set<std::string>, true, false> address_to_aliases_container;
    typedef tools::db::cached_key_value_accessor<crypto::hash, ms_output_entry, false, false> multisig_outs_container;// ms out id => ms_output_entry
    typedef tools::db::cached_key_value_accessor<uint64_t, uint64_t, false, true> solo_options_container;
    typedef tools::db::basic_key_value_accessor<uint32_t, block_gindex_increments, true> per_block_gindex_increments_container; // height => [(amount, gindex_increment), ...]
    
    typedef tools::db::cached_key_value_accessor<crypto::public_key, std::list<asset_descriptor_operation>, true, false> assets_container; // TODO @#@# consider storing tx_id as well for reference -- sowle 


    //-----------------------------------------

    typedef std::unordered_map<crypto::hash, std::pair<const transaction&, uint64_t> > txs_by_id_and_height_altchain;
    
    tx_memory_pool& m_tx_pool;
    mutable bc_attachment_services_manager m_services_mgr;

    /*
    At the moment we don't use synchronization for read operations since db do all dirty work.
    Write operation synchronized by db_backend.begin_transaction() if it called with write permissions
    and caches disabled for all other threads while writer is acting.
    The only problem left is that when readers threads still do some work, writer can commit changes and enable cache,
    which can lead to wrong interpretation of core data structure (state of the core changed while reader assume that state is the same).
    To avoid this cases we use read-write lock, which acquired exclusive access only when writer finished his work and want to commit changes
    and enable caches - with this acquiring writer wait wile all readers finish their reading.
    */
    epee::shared_recursive_mutex m_rw_lock;
    //mutable dummy_critical_section m_read_lock; 
    mutable epee::shared_membership<epee::shared_recursive_mutex> m_read_lock;

    //---------- db members ---------------------
    //main accessor
    tools::db::basic_db_accessor m_db;
    //containers
    blocks_container m_db_blocks;
    blocks_by_id_index m_db_blocks_index;
    transactions_container m_db_transactions;
    key_images_container m_db_spent_keys;
    solo_options_container m_db_solo_options;
    tools::db::solo_db_value<uint64_t, uint64_t, solo_options_container> m_db_current_block_cumul_sz_limit;
    tools::db::solo_db_value<uint64_t, uint64_t, solo_options_container> m_db_current_pruned_rs_height;
    tools::db::solo_db_value<uint64_t, std::string, solo_options_container, true> m_db_last_worked_version;
    tools::db::solo_db_value<uint64_t, uint64_t, solo_options_container> m_db_storage_major_compatibility_version;
    tools::db::solo_db_value<uint64_t, uint64_t, solo_options_container> m_db_storage_minor_compatibility_version;
    tools::db::solo_db_value<uint64_t, bool, solo_options_container> m_db_major_failure; //safety fuse

    outputs_container m_db_outputs;
    multisig_outs_container m_db_multisig_outs;
    aliases_container m_db_aliases;
    address_to_aliases_container m_db_addr_to_alias;
    per_block_gindex_increments_container m_db_per_block_gindex_incs;
    
    assets_container m_db_assets;




    mutable epee::critical_section m_invalid_blocks_lock;
    blocks_ext_by_hash m_invalid_blocks;     // crypto::hash -> block_extended_info
    mutable epee::critical_section m_alternative_chains_lock;
    alt_chain_container m_alternative_chains; // crypto::hash -> alt_block_extended_info
    std::unordered_map<crypto::hash, size_t> m_alternative_chains_txs; // tx_id -> how many alt blocks it related to (always >= 1)
    std::unordered_map<crypto::key_image, std::list<crypto::hash>> m_altblocks_keyimages; // key image -> list of alt blocks hashes where it appears in inputs

    std::atomic<bool> m_is_in_checkpoint_zone;
    std::atomic<bool> m_is_blockchain_storing;

    std::string m_config_folder;
    //events
    checkpoints m_checkpoints;
    mutable core_runtime_config m_core_runtime_config;
    mutable i_core_event_handler* m_event_handler;
    mutable i_core_event_handler m_event_handler_stub;

    //tools::median_db_cache<uint64_t, uint64_t> m_tx_fee_median;
    mutable std::unordered_map<size_t, uint64_t> m_timestamps_median_cache;
    mutable performnce_data m_performance_data;
    std::list<core_event> m_core_events_pack;
    mutable epee::file_io_utils::native_filesystem_handle m_interprocess_locker_file;
    //just informational 
    mutable wide_difficulty_type m_cached_next_pow_difficulty;
    mutable wide_difficulty_type m_cached_next_pos_difficulty;

    mutable epee::critical_section m_targetdata_cache_lock;
    mutable std::list <std::pair<wide_difficulty_type, uint64_t>> m_pos_targetdata_cache;
    mutable std::list <std::pair<wide_difficulty_type, uint64_t>> m_pow_targetdata_cache;
    //work like a cache to avoid recalculation on read operations
    mutable uint64_t m_current_fee_median;
    mutable uint64_t m_current_fee_median_effective_index;
    bool m_is_reorganize_in_process;    
    mutable std::atomic<bool> m_deinit_is_done;
    mutable uint64_t m_blockchain_launch_timestamp;

    //bool init_tx_fee_median();
    //bool update_tx_fee_median();
    void store_db_solo_options_values();
    bool set_lost_tx_unmixable();
    bool set_lost_tx_unmixable_for_height(uint64_t height);
    void patch_out_if_needed(txout_to_key& out, const crypto::hash& tx_id, uint64_t n)const ;
    bool switch_to_alternative_blockchain(alt_chain_type& alt_chain);
    void purge_alt_block_txs_hashs(const block& b);
    void add_alt_block_txs_hashs(const block& b);   
    bool pop_block_from_blockchain(transactions_map& onboard_transactions);
    bool purge_block_data_from_blockchain(const block& b, size_t processed_tx_count);
    bool purge_block_data_from_blockchain(const block& b, size_t processed_tx_count, uint64_t& fee, transactions_map& onboard_transactions);
    bool purge_transaction_from_blockchain(const crypto::hash& tx_id, uint64_t& fee, transaction& tx);
    bool purge_transaction_keyimages_from_blockchain(const transaction& tx, bool strict_check);
    wide_difficulty_type get_next_difficulty_for_alternative_chain(const alt_chain_type& alt_chain, block_extended_info& bei, bool pos) const;
    bool handle_block_to_main_chain(const block& bl, block_verification_context& bvc);
    bool handle_block_to_main_chain(const block& bl, const crypto::hash& id, block_verification_context& bvc);
    bool collect_rangeproofs_data_from_tx(const transaction& tx, const crypto::hash& tx_id, std::vector<zc_outs_range_proofs_with_commitments>& agregated_proofs);
    std::string print_alt_chain(alt_chain_type alt_chain);
    bool handle_alternative_block(const block& b, const crypto::hash& id, block_verification_context& bvc);
    bool is_reorganize_required(const block_extended_info& main_chain_bei, const alt_chain_type& alt_chain, const crypto::hash& proof_alt);
    wide_difficulty_type get_x_difficulty_after_height(uint64_t height, bool is_pos);
    bool purge_keyimage_from_big_heap(const crypto::key_image& ki, const crypto::hash& block_id);
    bool purge_altblock_keyimages_from_big_heap(const block& b, const crypto::hash& id);
    bool append_altblock_keyimages_to_big_heap(const crypto::hash& block_id, const std::unordered_set<crypto::key_image>& alt_block_keyimages);
    bool validate_alt_block_input(const transaction& input_tx, 
      std::unordered_set<crypto::key_image>& collected_keyimages, 
      const txs_by_id_and_height_altchain& alt_chain_tx_ids,
      const crypto::hash& bl_id, 
      const crypto::hash& input_tx_hash, 
      size_t input_index, 
      uint64_t split_height, 
      const alt_chain_type& alt_chain, 
      const std::unordered_set<crypto::hash>& alt_chain_block_ids, 
      uint64_t& ki_lookuptime, 
      uint64_t* p_max_related_block_height = nullptr) const;
    bool validate_alt_block_ms_input(const transaction& input_tx, 
      const crypto::hash& input_tx_hash, 
      size_t input_index, 
      //const signature_v& input_sigs,//const std::vector<crypto::signature>& input_sigs, 
      uint64_t split_height, 
      const alt_chain_type& alt_chain) const;
    bool validate_alt_block_txs(const block& b, const crypto::hash& id, std::unordered_set<crypto::key_image>& collected_keyimages, alt_block_extended_info& abei, const alt_chain_type& alt_chain, uint64_t split_height, uint64_t& ki_lookup_time_total) const;
    bool update_alt_out_indexes_for_tx_in_block(const transaction& tx, alt_block_extended_info& abei)const;
    bool get_transaction_from_pool_or_db(const crypto::hash& tx_id, std::shared_ptr<transaction>& tx_ptr, uint64_t min_allowed_block_height = 0) const;
    void get_last_n_x_blocks(uint64_t n, bool pos_blocks, std::list<std::shared_ptr<const block_extended_info>>& blocks) const;
    bool prevalidate_miner_transaction(const block& b, uint64_t height, bool pos)const;
    bool rollback_blockchain_switching(std::list<block_ws_txs>& original_chain, size_t rollback_height);
    bool add_transaction_from_block(const transaction& tx, const crypto::hash& tx_id, const crypto::hash& bl_id, uint64_t bl_height, uint64_t timestamp);
    bool push_transaction_to_global_outs_index(const transaction& tx, const crypto::hash& tx_id, std::vector<uint64_t>& global_indexes);
    bool pop_transaction_from_global_index(const transaction& tx, const crypto::hash& tx_id);
    bool add_out_to_get_random_outs(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, uint64_t amount, size_t i, uint64_t mix_count, bool use_only_forced_to_mix = false, uint64_t height_upper_limit = 0) const;
    bool get_target_outs_for_amount_prezarcanum(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request& req, const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::offsets_distribution& details, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, std::map<uint64_t, uint64_t>& amounts_to_up_index_limit_cache) const;
    bool get_target_outs_for_postzarcanum(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request& req, const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::offsets_distribution& details, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, std::map<uint64_t, uint64_t>& amounts_to_up_index_limit_cache) const;
    bool add_block_as_invalid(const block& bl, const crypto::hash& h);
    bool add_block_as_invalid(const block_extended_info& bei, const crypto::hash& h);
    size_t find_end_of_allowed_index(uint64_t amount)const;
    bool check_block_timestamp_main(const block& b)const;
    bool check_block_timestamp(std::vector<uint64_t> timestamps, const block& b)const;
    std::vector<uint64_t> get_last_n_blocks_timestamps(size_t n)const;
    void on_block_added(const block_extended_info& bei, const crypto::hash& id, const std::list<crypto::key_image>& bsk);
    void on_block_removed(const block_extended_info& bei);
    void update_targetdata_cache_on_block_added(const block_extended_info& bei);
    void update_targetdata_cache_on_block_removed(const block_extended_info& bei);
    uint64_t tx_fee_median_for_height(uint64_t h) const;
    uint64_t get_tx_fee_median_effective_index(uint64_t h) const;    
    void on_abort_transaction();
    void load_targetdata_cache(bool is_pos) const;

    

    uint64_t get_adjusted_time()const;
    bool complete_timestamps_vector(uint64_t start_height, std::vector<uint64_t>& timestamps);
    bool update_next_comulative_size_limit();
    bool process_blockchain_tx_extra(const transaction& tx, const crypto::hash& tx_id);
    bool unprocess_blockchain_tx_extra(const transaction& tx);
    bool process_blockchain_tx_attachments(const transaction& tx, uint64_t h, const crypto::hash& bl_id, uint64_t timestamp);
    bool unprocess_blockchain_tx_attachments(const transaction& tx, uint64_t h, uint64_t timestamp);
    bool pop_alias_info(const extra_alias_entry& ai);
    bool put_alias_info(const transaction& tx, extra_alias_entry& ai);
    bool pop_asset_info(const crypto::public_key& asset_id);
    bool put_asset_info(const transaction& tx, const crypto::hash& tx_id, const asset_descriptor_operation& ado);
    void fill_addr_to_alias_dict();
    //bool resync_spent_tx_flags();
    bool prune_ring_signatures_and_attachments_if_need();
    bool prune_ring_signatures_and_attachments(uint64_t height, uint64_t& transactions_pruned, uint64_t& signatures_pruned, uint64_t& attachments_pruned);

    template<class visitor_t>
    bool enum_blockchain(visitor_t& v, const alt_chain_type& alt_chain = alt_chain_type(), uint64_t split_height = 0) const;
    bool update_spent_tx_flags_for_input(uint64_t amount, const txout_ref_v& o, bool spent);
    bool update_spent_tx_flags_for_input(uint64_t amount, uint64_t global_index, bool spent);
    bool update_spent_tx_flags_for_input(const crypto::hash& multisig_id, uint64_t spent_height);
    bool update_spent_tx_flags_for_input(const crypto::hash& tx_id, size_t n, bool spent);
    
    void push_block_to_per_block_increments(uint64_t height_, std::unordered_map<uint64_t, uint32_t>& gindices);
    void pop_block_from_per_block_increments(uint64_t height_);
    void calculate_local_gindex_lookup_table_for_height(uint64_t split_height, std::map<uint64_t, uint64_t>& increments) const;
    void do_erase_altblock(alt_chain_container::iterator it);
    uint64_t get_blockchain_launch_timestamp()const;
    bool is_output_allowed_for_input(const txout_target_v& out_v, const txin_v& in_v, uint64_t top_minus_source_height)const;
    bool is_output_allowed_for_input(const output_key_or_htlc_v& out_v, const txin_v& in_v, uint64_t top_minus_source_height)const;
    bool is_output_allowed_for_input(const txout_to_key& out_v, const txin_v& in_v)const;
    bool is_output_allowed_for_input(const txout_htlc& out_v, const txin_v& in_v, uint64_t top_minus_source_height)const;
    bool is_output_allowed_for_input(const tx_out_zarcanum& out, const txin_v& in_v) const;



    //POS
    wide_difficulty_type get_adjusted_cumulative_difficulty_for_next_pos(wide_difficulty_type next_diff)const;
    wide_difficulty_type get_adjusted_cumulative_difficulty_for_next_alt_pos(alt_chain_type& alt_chain, uint64_t block_height, wide_difficulty_type next_diff, uint64_t connection_height)const;
    wide_difficulty_type get_last_alt_x_block_cumulative_precise_difficulty(const alt_chain_type& alt_chain, uint64_t block_height, bool pos, wide_difficulty_type& cumulative_diff_precise_adj)const;
    wide_difficulty_type get_last_alt_x_block_cumulative_precise_adj_difficulty(const alt_chain_type& alt_chain, uint64_t block_height, bool pos) const;
    size_t get_current_sequence_factor_for_alt(alt_chain_type& alt_chain, bool pos, uint64_t connection_height)const;
  };

  /************************************************************************/
    /* end of class class blockchain_storage                                */
  /************************************************************************/

  template<class visitor_t>
  bool blockchain_storage::enum_blockchain(visitor_t& v, const alt_chain_type& alt_chain, uint64_t split_height) const
  {

    bool keep_going = true;
    for (auto it = alt_chain.rbegin(); it != alt_chain.rend() && keep_going; it++)
    {
      keep_going = v((*it)->second, false);
    }

    if (!keep_going || !m_db_blocks.size())
      return !keep_going;

    size_t main_chain_start_offset = 0;
    if (split_height)
      main_chain_start_offset = split_height - 1;
    else
      main_chain_start_offset = (alt_chain.size() ? alt_chain.front()->second.height : m_db_blocks.size()) - 1;

    CRITICAL_REGION_LOCAL(m_read_lock);
    for (uint64_t i = main_chain_start_offset; i != 0 && keep_going; --i)
    {
      keep_going = v(*m_db_blocks[i], true);
    }

    return !keep_going;
  }
  //------------------------------------------------------------------
  template<class visitor_t>
  bool blockchain_storage::scan_outputkeys_for_indexes(const transaction &validated_tx, const txin_v& verified_input, visitor_t& vis, uint64_t& max_related_block_height, scan_for_keys_context& scan_context) const
  {
    bool hf4 = this->is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM);

    uint64_t amount = get_amount_from_variant(verified_input);
    const std::vector<txout_ref_v>& key_offsets = get_key_offsets_from_txin_v(verified_input);

    CRITICAL_REGION_LOCAL(m_read_lock);
    TIME_MEASURE_START_PD(tx_check_inputs_loop_scan_outputkeys_get_item_size);

    uint64_t outs_count_for_amount = m_db_outputs.get_item_size(amount);
    TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_scan_outputkeys_get_item_size);
    if (!outs_count_for_amount)
      return false;
    TIME_MEASURE_START_PD(tx_check_inputs_loop_scan_outputkeys_relative_to_absolute);
    std::vector<txout_ref_v> absolute_offsets = relative_output_offsets_to_absolute(key_offsets);
    TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_scan_outputkeys_relative_to_absolute);
    TIME_MEASURE_START_PD(tx_check_inputs_loop_scan_outputkeys_loop);
    size_t output_index = 0;
    for (const txout_ref_v& o : absolute_offsets)
    {
      crypto::hash tx_id = null_hash;
      size_t n = 0;
      if (o.type() == typeid(ref_by_id))
      {
        tx_id = boost::get<ref_by_id>(o).tx_id;
        n = boost::get<ref_by_id>(o).n;
      }
      else if (o.type() == typeid(uint64_t))
      {
        TIME_MEASURE_START_PD(tx_check_inputs_loop_scan_outputkeys_loop_get_subitem);
        uint64_t i = boost::get<uint64_t>(o);
        if (i >= outs_count_for_amount)
        {
          LOG_ERROR("Wrong index in transaction inputs: " << i << ", expected maximum " << outs_count_for_amount - 1);
          return false;
        }
        auto out_ptr = m_db_outputs.get_subitem(amount, i);
        tx_id = out_ptr->tx_id;
        n = out_ptr->out_no;
        TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_scan_outputkeys_loop_get_subitem);
      }
      TIME_MEASURE_START_PD(tx_check_inputs_loop_scan_outputkeys_loop_find_tx);
      auto tx_ptr = m_db_transactions.find(tx_id);
      CHECK_AND_ASSERT_MES(tx_ptr, false, "Wrong transaction id in output indexes: " << epst::pod_to_hex(tx_id));
      CHECK_AND_ASSERT_MES(n < tx_ptr->tx.vout.size(), false,
        "Wrong index in transaction outputs: " << n << ", expected less then " << tx_ptr->tx.vout.size());
      //check mix_attr
      TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_scan_outputkeys_loop_find_tx);

      //CHECKED_GET_SPECIFIC_VARIANT(tx_ptr->tx.vout[n].target, const txout_to_key, outtk, false);
      CHECK_AND_ASSERT_MES(key_offsets.size() >= 1, false, "internal error: tx input has empty key_offsets"); // should never happen as input correctness must be handled by the caller

      /*
      TxOutput | TxInput | Allowed
      ----------------------------
      HTLC     |  HTLC   | ONLY IF HTLC NOT EXPIRED
      HTLC     |  TO_KEY | ONLY IF HTLC IS EXPIRED
      TO_KEY   |  HTLC   | NOT
      TO_KEY   |  TO_KEY | YES
      */

      VARIANT_SWITCH_BEGIN(tx_ptr->tx.vout[n]);
      VARIANT_CASE_CONST(tx_out_bare, o)
      {
        bool r = is_output_allowed_for_input(o.target, verified_input, get_current_blockchain_size() - tx_ptr->m_keeper_block_height);
        CHECK_AND_ASSERT_MES(r, false, "Input and output incompatible type");

        if (o.target.type() == typeid(txout_to_key))
        {
          CHECKED_GET_SPECIFIC_VARIANT(o.target, const txout_to_key, outtk, false);
          //fix for burned money
          patch_out_if_needed(const_cast<txout_to_key&>(outtk), tx_id, n);

          bool mixattr_ok = is_mixattr_applicable_for_fake_outs_counter(tx_ptr->tx.version, outtk.mix_attr, key_offsets.size() - 1, this->get_core_runtime_config());
          CHECK_AND_ASSERT_MES(mixattr_ok, false, "tx input ref #" << output_index << " violates mixin restrictions: tx.version = " << tx_ptr->tx.version << ", mix_attr = " << static_cast<uint32_t>(outtk.mix_attr) << ", key_offsets.size = " << key_offsets.size());
          if (hf4)
          {
            bool legit_output_key = validate_output_key_legit(outtk.key);
            CHECK_AND_ASSERT_MES(legit_output_key, false, "tx input ref #" << output_index << " violates public key restrictions: tx.version = " << tx_ptr->tx.version << ", outtk.key = " << outtk.key);
          }
        }
        else if (o.target.type() == typeid(txout_htlc))
        {
          //check for spend flags
          CHECK_AND_ASSERT_MES(tx_ptr->m_spent_flags.size() > n, false,
            "Internal error: tx_ptr->m_spent_flags.size(){" << tx_ptr->m_spent_flags.size() << "} > n{" << n << "}");
          CHECK_AND_ASSERT_MES(tx_ptr->m_spent_flags[n] == false, false, "HTLC out already spent, double spent attempt detected");

          const txout_htlc& htlc_out = boost::get<txout_htlc>(o.target);
          if (htlc_out.expiration > get_current_blockchain_size() - tx_ptr->m_keeper_block_height)
          {
            //HTLC IS NOT expired, can be used ONLY by pkey_before_expiration and ONLY by HTLC input
            scan_context.htlc_is_expired = false;
          }
          else
          {
            //HTLC IS expired, can be used ONLY by pkey_after_expiration and ONLY by to_key input
            scan_context.htlc_is_expired = true;
          }
          if (hf4)
          {
            bool legit_output_key = validate_output_key_legit(scan_context.htlc_is_expired ? htlc_out.pkey_refund : htlc_out.pkey_redeem);
            CHECK_AND_ASSERT_MES(legit_output_key, false, "tx input ref #" << output_index << " violates public key restrictions: tx.version = " << tx_ptr->tx.version << ", outtk.key = " << static_cast<const crypto::public_key&>(scan_context.htlc_is_expired ? htlc_out.pkey_refund : htlc_out.pkey_redeem));
          }

        }
        else
        {
          LOG_ERROR("[scan_outputkeys_for_indexes]: Wrong output type in : " << o.target.type().name());
          return false;
        }

        TIME_MEASURE_START_PD(tx_check_inputs_loop_scan_outputkeys_loop_handle_output);

        if (!vis.handle_output(tx_ptr->tx, validated_tx, o, n))
        {
          size_t verified_input_index = std::find(validated_tx.vin.begin(), validated_tx.vin.end(), verified_input) - validated_tx.vin.begin();
          LOG_PRINT_RED_L0("handle_output failed for output #" << n << " in " << tx_id << " referenced by input #" << verified_input_index << " in tx " << get_transaction_hash(validated_tx));
          return false;
        }
        TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_scan_outputkeys_loop_handle_output);
      }
      VARIANT_CASE_CONST(tx_out_zarcanum, out_zc)
        bool r = is_output_allowed_for_input(out_zc, verified_input);
      CHECK_AND_ASSERT_MES(r, false, "Input and output are incompatible");

      r = is_mixattr_applicable_for_fake_outs_counter(tx_ptr->tx.version, out_zc.mix_attr, key_offsets.size() - 1, this->get_core_runtime_config());
      CHECK_AND_ASSERT_MES(r, false, "tx input ref #" << output_index << " violates mixin restrictions: tx.version = " << tx_ptr->tx.version << ", mix_attr = " << static_cast<uint32_t>(out_zc.mix_attr) << ", key_offsets.size = " << key_offsets.size());

      bool legit_output_key = validate_output_key_legit(out_zc.stealth_address);
      CHECK_AND_ASSERT_MES(legit_output_key, false, "tx input ref #" << output_index << " violates public key restrictions: tx.version = " << tx_ptr->tx.version << ", outtk.key = " << out_zc.stealth_address);


      TIME_MEASURE_START_PD(tx_check_inputs_loop_scan_outputkeys_loop_handle_output);
      if (!vis.handle_output(tx_ptr->tx, validated_tx, out_zc, n))
      {
        size_t verified_input_index = std::find(validated_tx.vin.begin(), validated_tx.vin.end(), verified_input) - validated_tx.vin.begin();
        LOG_PRINT_RED_L0("handle_output failed for output #" << n << " in " << tx_id << " referenced by input #" << verified_input_index << " in tx " << get_transaction_hash(validated_tx));
        return false;
      }
      TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_scan_outputkeys_loop_handle_output);
      VARIANT_CASE_THROW_ON_OTHER();
      VARIANT_SWITCH_END();



      if (max_related_block_height < tx_ptr->m_keeper_block_height)
        max_related_block_height = tx_ptr->m_keeper_block_height;

      ++output_index;
    }

    if (m_core_runtime_config.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, this->get_current_blockchain_size()))
    { 
      //with hard fork 4 make it network rule to have at least 10 confirmations
      
      if (this->get_current_blockchain_size() - max_related_block_height < CURRENCY_HF4_MANDATORY_MIN_COINAGE)
      {
        LOG_ERROR("Coinage rule broken(mainblock): h = " << this->get_current_blockchain_size() << ", max_related_block_height=" << max_related_block_height << ", tx: " << get_transaction_hash(validated_tx));
        return false;
      }
    }
    

    TIME_MEASURE_FINISH_PD(tx_check_inputs_loop_scan_outputkeys_loop);
    return true;
  }

  //------------------------------------------------------------------
  template<typename cb_t>
  bool blockchain_storage::get_aliases(cb_t cb, uint64_t offset, uint64_t count) const
  {
    CRITICAL_REGION_LOCAL(m_read_lock);

    uint64_t counter = 0;

    m_db_aliases.enumerate_items([&](uint64_t i, const std::string& alias, const std::list<extra_alias_entry_base>& alias_entries)
    {
      if (alias_entries.empty())
        return true; // continue

      if (counter < offset)
      {
        ++counter;
        return true; // continue
      }

      if (counter >= offset + count)
        return false; // stop

      ++counter;

      cb(alias, alias_entries.back());
      return true;
    });
    return true;
  }
  //------------------------------------------------------------------
  // callback: (uint64_t i, const std::string& alias, const extra_alias_entry_base& alias_entry) -> bool
  // callback should return false to stop enumeration, true - to continue
  template<typename cb_t>
  bool blockchain_storage::enumerate_aliases(cb_t cb) const
  {
    CRITICAL_REGION_LOCAL(m_read_lock);

    m_db_aliases.enumerate_items([&](uint64_t i, const std::string& alias, const std::list<extra_alias_entry_base>& alias_entries)
    {
      return cb(i, alias, alias_entries.back());
    });

    return true;
  }



} // namespace currency


BOOST_CLASS_VERSION(currency::transaction_chain_entry, CURRENT_TRANSACTION_CHAIN_ENTRY_ARCHIVE_VER)
BOOST_CLASS_VERSION(currency::block_extended_info, CURRENT_BLOCK_EXTENDED_INFO_ARCHIVE_VER)

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL

#include "blockchain_storage_boost_serialization.h"
#include "currency_boost_serialization.h"
