// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <typeindex>
#include <unordered_set>
#include <unordered_map>

#include "account.h"
#include "include_base_utils.h"

#include "currency_format_utils_abstract.h"
#include "common/crypto_stream_operators.h"
#include "currency_protocol/currency_protocol_defs.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "difficulty.h"
//#include "offers_services_helpers.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "bc_payments_id_service.h"
#include "bc_attachments_helpers_basic.h"
#include "blockchain_storage_basic.h"
#include "currency_format_utils_blocks.h"
#include "currency_format_utils_transactions.h"

// ------ get_tx_type_definition -------------
#define       GUI_TX_TYPE_NORMAL                  0
#define       GUI_TX_TYPE_PUSH_OFFER              1
#define       GUI_TX_TYPE_UPDATE_OFFER            2
#define       GUI_TX_TYPE_CANCEL_OFFER            3
#define       GUI_TX_TYPE_NEW_ALIAS               4
#define       GUI_TX_TYPE_UPDATE_ALIAS            5
#define       GUI_TX_TYPE_COIN_BASE               6
#define       GUI_TX_TYPE_ESCROW_PROPOSAL         7
#define       GUI_TX_TYPE_ESCROW_TRANSFER         8
#define       GUI_TX_TYPE_ESCROW_RELEASE_NORMAL   9
#define       GUI_TX_TYPE_ESCROW_RELEASE_BURN     10
#define       GUI_TX_TYPE_ESCROW_CANCEL_PROPOSAL  11
#define       GUI_TX_TYPE_ESCROW_RELEASE_CANCEL   12







namespace currency
{
  bool operator ==(const currency::transaction& a, const currency::transaction& b);
  bool operator ==(const currency::block& a, const currency::block& b);
  bool operator ==(const currency::extra_attachment_info& a, const currency::extra_attachment_info& b);



  typedef boost::multiprecision::uint128_t uint128_tl;
  struct tx_source_entry
  {
    typedef std::pair<txout_v, crypto::public_key> output_entry; // txout_v is either global output index or ref_by_id; public_key - is output ephemeral pub key

    std::vector<output_entry> outputs;  //index + key
    uint64_t real_output;               //index in outputs vector of real output_entry
    crypto::public_key real_out_tx_key; //real output's transaction's public key
    size_t real_output_in_tx_index;     //index in transaction outputs vector
    uint64_t amount;                    //money
    crypto::hash multisig_id;           //if txin_multisig: multisig output id
    size_t ms_sigs_count;               //if txin_multisig: must be equal to output's minimum_sigs
    size_t ms_keys_count;               //if txin_multisig: must be equal to size of output's keys container
    bool separately_signed_tx_complete; //for separately signed tx only: denotes the last source entry in complete tx to explicitly mark the final step of tx creation

    bool is_multisig() const { return ms_sigs_count > 0; }
  };

  struct tx_destination_entry
  {
    uint64_t amount;                                 //money
    std::list<account_public_address>   addr;        //destination address, in case of 1 address - txout_to_key, in case of more - txout_multisig
    size_t   minimum_sigs;                           // if txout_multisig: minimum signatures that are required to spend this output (minimum_sigs <= addr.size())  IF txout_to_key - not used
    uint64_t amount_to_provide;                      //amount money that provided by initial creator of tx, used with partially created transactions

    tx_destination_entry() : amount(0), minimum_sigs(0), amount_to_provide(0){}
    tx_destination_entry(uint64_t a, const account_public_address& ad) : amount(a), addr(1, ad), minimum_sigs(0), amount_to_provide(0){}
    tx_destination_entry(uint64_t a, const std::list<account_public_address>& addr) : amount(a), addr(addr), minimum_sigs(addr.size()), amount_to_provide(0){}
  };

  struct tx_extra_info 
  {
    crypto::public_key m_tx_pub_key;
    //crypto::hash m_offers_hash;
    extra_alias_entry m_alias;
    std::string m_user_data_blob;
    extra_attachment_info m_attachment_info;
  };

  //---------------------------------------------------------------------------------------------------------------
  struct genesis_payment_entry
  {
    std::string paid_prm;
    std::string prm_usd_price;
    std::string paid_xmr;
    std::string xmr_usd_price;
    std::string paid_qtum;
    std::string qtum_usd_price;
    std::string paid_bch;
    std::string bch_usd_price;
    std::string paid_rep;
    std::string rep_usd_price;
    std::string paid_dash;
    std::string dash_usd_price;
    std::string paid_ltc;
    std::string ltc_usd_price;
    std::string paid_eos;
    std::string eos_usd_price;
    std::string paid_eth;
    std::string eth_usd_price;
    std::string paid_btc;
    std::string btc_usd_price;
	  std::string address_this;
    double amount_this_coin_fl;
    double amount_this_coin_int;
    std::string this_usd_price;

	  BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(paid_prm)
      KV_SERIALIZE(prm_usd_price)
      KV_SERIALIZE(paid_xmr)
      KV_SERIALIZE(xmr_usd_price)
      KV_SERIALIZE(paid_qtum)
      KV_SERIALIZE(qtum_usd_price)
      KV_SERIALIZE(paid_bch)
      KV_SERIALIZE(bch_usd_price)
      KV_SERIALIZE(paid_rep)
      KV_SERIALIZE(rep_usd_price)
      KV_SERIALIZE(paid_dash)
      KV_SERIALIZE(dash_usd_price)
      KV_SERIALIZE(paid_ltc)
      KV_SERIALIZE(ltc_usd_price)
      KV_SERIALIZE(paid_eos)
      KV_SERIALIZE(eos_usd_price)
      KV_SERIALIZE(paid_eth)
      KV_SERIALIZE(eth_usd_price)
      KV_SERIALIZE(paid_btc)
      KV_SERIALIZE(btc_usd_price)
      KV_SERIALIZE(address_this)
      KV_SERIALIZE_N(amount_this_coin_fl, "amount_this")
      KV_SERIALIZE(this_usd_price)
	  END_KV_SERIALIZE_MAP()
  };
  struct genesis_config_json_struct
  {
	  std::list<genesis_payment_entry> payments;
	  std::string proof_string;

	  BEGIN_KV_SERIALIZE_MAP()
		  KV_SERIALIZE(payments)
		  KV_SERIALIZE(proof_string)
	  END_KV_SERIALIZE_MAP()
  };


  //---------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins, 
                                                             size_t current_block_size, 
                                                             uint64_t fee, 
                                                             const account_public_address &miner_address, 
                                                             const account_public_address &stakeholder_address,
                                                             transaction& tx, 
                                                             const blobdata& extra_nonce = blobdata(), 
                                                             size_t max_outs = CURRENCY_MINER_TX_MAX_OUTS, 
                                                             bool pos = false,
                                                             const pos_entry& pe = pos_entry());

  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins, 
                                                             size_t current_block_size, 
                                                             uint64_t fee, 
                                                             const std::vector<tx_destination_entry>& destinations,
                                                             transaction& tx, 
                                                             const blobdata& extra_nonce = blobdata(),
                                                             size_t max_outs = CURRENCY_MINER_TX_MAX_OUTS,
                                                             bool pos = false,
                                                             const pos_entry& pe = pos_entry());


  //---------------------------------------------------------------
  template<class extra_type_t>
  uint64_t get_tx_x_detail(const transaction& tx)
  {
    extra_type_t e = AUTO_VAL_INIT(e);
    get_type_in_variant_container(tx.extra, e);
    return e.v;
  }
  template<class extra_type_t>
  void set_tx_x_detail(transaction& tx, uint64_t v)
  {
    extra_type_t e = AUTO_VAL_INIT(e);
    e.v = v;
    update_or_add_field_to_extra(tx.extra, e);
  }

  inline uint64_t get_tx_unlock_time(const transaction& tx){ return get_tx_x_detail<etc_tx_details_unlock_time>(tx);}
  inline uint64_t get_tx_flags(const transaction& tx){ return get_tx_x_detail<etc_tx_details_flags>(tx); }
  inline uint64_t get_tx_expiration_time(const transaction& tx){ return get_tx_x_detail<etc_tx_details_expiration_time>(tx); }
  inline void set_tx_unlock_time(transaction& tx, uint64_t v){ set_tx_x_detail<etc_tx_details_unlock_time>(tx, v); }
  inline void set_tx_flags(transaction& tx, uint64_t v){ set_tx_x_detail<etc_tx_details_flags>(tx, v); }
  inline void set_tx_expiration_time(transaction& tx, uint64_t v){ set_tx_x_detail<etc_tx_details_expiration_time>(tx, v); }
  account_public_address get_crypt_address_from_destinations(const account_keys& sender_account_keys, const std::vector<tx_destination_entry>& destinations);

  bool is_tx_expired(const transaction& tx, uint64_t expiration_ts_median);


  uint64_t get_string_uint64_hash(const std::string& str);
  bool construct_tx_out(const tx_destination_entry& de, const crypto::secret_key& tx_sec_key, size_t output_index, transaction& tx, std::set<uint16_t>& deriv_cache, uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED);
  bool validate_alias_name(const std::string& al);
  void get_attachment_extra_info_details(const std::vector<attachment_v>& attachment, extra_attachment_info& eai);
  bool construct_tx(const account_keys& sender_account_keys, 
    const std::vector<tx_source_entry>& sources, 
    const std::vector<tx_destination_entry>& destinations, 
    const std::vector<attachment_v>& attachments,
    transaction& tx, 
    uint64_t unlock_time, 
    uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED, 
    bool shuffle = true);
  bool construct_tx(const account_keys& sender_account_keys, 
    const std::vector<tx_source_entry>& sources, 
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<extra_v>& extra,
    const std::vector<attachment_v>& attachments,
    transaction& tx, 
    crypto::secret_key& one_time_secret_key,
    uint64_t unlock_time,
    uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED, 
    bool shuffle = true,
    uint64_t flags = 0);
  bool construct_tx(const account_keys& sender_account_keys,
    const std::vector<tx_source_entry>& sources,
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<extra_v>& extra,
    const std::vector<attachment_v>& attachments,
    transaction& tx,
    crypto::secret_key& one_time_secret_key,
    uint64_t unlock_time,
    const account_public_address& crypt_account,
    uint64_t expiration_time,
    uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED,
    bool shuffle = true,
    uint64_t flags = 0);

  bool sign_multisig_input_in_tx(currency::transaction& tx, size_t ms_input_index, const currency::account_keys& keys, const currency::transaction& source_tx, bool *p_is_input_fully_signed = nullptr);

  bool sign_extra_alias_entry(extra_alias_entry& ai, const crypto::public_key& pkey, const crypto::secret_key& skey);
  crypto::hash get_sign_buff_hash_for_alias_update(const extra_alias_entry& ai);
  bool add_tx_extra_alias(transaction& tx, const extra_alias_entry& alinfo);
  bool parse_and_validate_tx_extra(const transaction& tx, tx_extra_info& extra);
  bool parse_and_validate_tx_extra(const transaction& tx, crypto::public_key& tx_pub_key);
  crypto::public_key get_tx_pub_key_from_extra(const transaction& tx);
  bool add_tx_pub_key_to_extra(transaction& tx, const crypto::public_key& tx_pub_key);
  bool add_tx_extra_userdata(transaction& tx, const blobdata& extra_nonce);

  crypto::hash get_multisig_out_id(const transaction& tx, size_t n);
  bool is_out_to_acc(const account_keys& acc, const txout_to_key& out_key, const crypto::key_derivation& derivation, size_t output_index);
  bool is_out_to_acc(const account_keys& acc, const txout_multisig& out_multisig, const crypto::key_derivation& derivation, size_t output_index);
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, std::vector<size_t>& outs, uint64_t& money_transfered, crypto::key_derivation& derivation);
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, std::vector<size_t>& outs, uint64_t& money_transfered, crypto::key_derivation& derivation);
  bool get_tx_fee(const transaction& tx, uint64_t & fee);
  uint64_t get_tx_fee(const transaction& tx);
  bool derive_ephemeral_key_helper(const account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, keypair& in_ephemeral);
  bool generate_key_image_helper(const account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, keypair& in_ephemeral, crypto::key_image& ki);
  bool derive_public_key_from_target_address(const account_public_address& destination_addr, const crypto::secret_key& tx_sec_key, size_t index, crypto::public_key& out_eph_public_key, crypto::key_derivation& derivation);
  bool derive_public_key_from_target_address(const account_public_address& destination_addr, const crypto::secret_key& tx_sec_key, size_t index, crypto::public_key& out_eph_public_key);
  std::string short_hash_str(const crypto::hash& h);
  bool is_mixattr_applicable_for_fake_outs_counter(uint8_t mix_attr, uint64_t fake_attr_count);
  bool is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t current_blockchain_size, uint64_t current_time);
  bool decrypt_payload_items(bool is_income, const transaction& tx, const account_keys& acc_keys, std::vector<payload_items_v>& decrypted_items);
  void encrypt_attachments(transaction& tx, const account_keys& sender_keys, const account_public_address& destination_addr, const keypair& tx_random_key);
  bool is_derivation_used_to_encrypt(const transaction& tx, const crypto::key_derivation& derivation);
  uint64_t get_tx_type(const transaction& tx);
  size_t get_multisig_out_index(const std::vector<tx_out>& outs);
  size_t get_multisig_in_index(const std::vector<txin_v>& inputs);

  uint64_t get_reward_from_miner_tx(const transaction& tx);

  //bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t& blob_size);
  bool generate_genesis_block(block& bl);
  const crypto::hash& get_genesis_hash(bool need_to_set = false, const crypto::hash& h = null_hash);
  bool parse_and_validate_block_from_blob(const blobdata& b_blob, block& b);
  uint64_t get_inputs_money_amount(const transaction& tx);
  bool get_inputs_money_amount(const transaction& tx, uint64_t& money);
  uint64_t get_outs_money_amount(const transaction& tx);
  bool check_inputs_types_supported(const transaction& tx);
  bool check_outs_valid(const transaction& tx);
  bool parse_amount(uint64_t& amount, const std::string& str_amount);


  
//  crypto::hash get_block_longhash(uint64_t h, const crypto::hash& block_long_ash, uint64_t nonce);
//   void get_block_longhash(const block& b, crypto::hash& res);
//   crypto::hash get_block_longhash(const block& b);

  bool unserialize_block_complete_entry(const COMMAND_RPC_GET_BLOCKS_FAST::response& serialized,
    COMMAND_RPC_GET_BLOCKS_DIRECT::response& unserialized);

  
  uint64_t get_alias_coast_from_fee(const std::string& alias, uint64_t fee_median);
  //const crypto::public_key get_offer_secure_key_by_index_from_tx(const transaction& tx, size_t index);

  bool check_money_overflow(const transaction& tx);
  bool check_outs_overflow(const transaction& tx);
  bool check_inputs_overflow(const transaction& tx);
  uint64_t get_block_height(const transaction& coinbase);
  uint64_t get_block_height(const block& b);
  std::vector<txout_v> relative_output_offsets_to_absolute(const std::vector<txout_v>& off);
  std::vector<txout_v> absolute_output_offsets_to_relative(const std::vector<txout_v>& off);
  // prints amount in format "3.14000000", "0.00000000"
  std::string print_money(uint64_t amount);
  std::string print_fixed_decimal_point(uint64_t amount, size_t decimal_point);
  // prints amount in format "3.14", "0.0"
  std::string print_money_brief(uint64_t amount);
  uint64_t get_actual_timestamp(const block& b);
  
  bool addendum_to_hexstr(const std::vector<crypto::hash>& add, std::string& hex_buff);
  bool hexstr_to_addendum(const std::string& hex_buff, std::vector<crypto::hash>& add);
  bool set_payment_id_to_tx(std::vector<attachment_v>& att, const std::string& payment_id);
  bool add_padding_to_tx(transaction& tx, size_t count);
  bool is_service_tx(const transaction& tx);
  bool is_mixin_tx(const transaction& tx);
  bool is_showing_sender_addres(const transaction& tx);
  uint64_t get_amount_for_zero_pubkeys(const transaction& tx);
  //std::string get_comment_from_tx(const transaction& tx);
  std::string print_stake_kernel_info(const stake_kernel& sk);
  std::string dump_ring_sig_data(const crypto::hash& hash_for_sig, const crypto::key_image& k_image, const std::vector<const crypto::public_key*>& output_keys_ptrs, const std::vector<crypto::signature>& sig);

  //PoS
  bool is_pos_block(const block& b);
  bool is_pos_block(const transaction& tx);
  uint64_t get_coinday_weight(uint64_t amount);
  wide_difficulty_type correct_difficulty_with_sequence_factor(size_t sequence_factor, wide_difficulty_type diff);
  void print_currency_details();
  std::string print_reward_change_first_blocks(size_t n_of_first_blocks);
  bool get_aliases_reward_account(account_public_address& acc, crypto::secret_key& acc_view_key);
  bool get_aliases_reward_account(account_public_address& acc);
  alias_rpc_details alias_info_to_rpc_alias_info(const currency::extra_alias_entry& ai);
  update_alias_rpc_details alias_info_to_rpc_update_alias_info(const currency::extra_alias_entry& ai, const std::string& old_address);
  size_t get_service_attachments_count_in_tx(const transaction& tx);
  bool fill_tx_rpc_outputs(tx_rpc_extended_info& tei, const transaction& tx, const transaction_chain_entry* ptce);
  bool fill_tx_rpc_inputs(tx_rpc_extended_info& tei, const transaction& tx);
  bool fill_tx_rpc_details(tx_rpc_extended_info& tei, const transaction& tx, const transaction_chain_entry* ptce, const crypto::hash& h, uint64_t timestamp, bool is_short = false);
  bool fill_block_rpc_details(block_rpc_extended_info& pei_rpc, const block_extended_info& bei_chain, const crypto::hash& h);
  void append_per_block_increments_for_tx(const transaction& tx, std::unordered_map<uint64_t, uint32_t>& gindices);
  std::string get_word_from_timstamp(uint64_t timestamp);
  uint64_t get_timstamp_from_word(std::string word);

  template<class t_txin_v>
  typename std::conditional<std::is_const<t_txin_v>::value, const std::vector<txin_etc_details_v>, std::vector<txin_etc_details_v> >::type& get_txin_etc_options(t_txin_v& in)
  {
    static typename std::conditional<std::is_const<t_txin_v>::value, const std::vector<txin_etc_details_v>, std::vector<txin_etc_details_v> >::type stub;


    //static  stub;

    if (in.type() == typeid(txin_to_key))
      return boost::get<txin_to_key>(in).etc_details;
    else if (in.type() == typeid(txin_multisig))
      return boost::get<txin_multisig>(in).etc_details;
     else
       return stub;
  }

  template<class t_extra_container>
  bool add_attachments_info_to_extra(t_extra_container& extra_container, const std::vector<attachment_v>& attachments)
  {
    if (!attachments.empty())
    {
      extra_attachment_info eai = AUTO_VAL_INIT(eai);
      get_attachment_extra_info_details(attachments, eai);
      extra_container.push_back(eai);
    }
    return true;
  }


  /************************************************************************/
  /* helper functions                                                     */
  /************************************************************************/
  size_t get_max_block_size();
  size_t get_max_tx_size();
  bool get_block_reward(bool is_pos, size_t median_size, size_t current_block_size, uint64_t already_generated_coins, uint64_t &reward, uint64_t height);
  uint64_t get_base_block_reward(bool is_pos, uint64_t already_generated_coins, uint64_t height);
  uint64_t get_scratchpad_last_update_rebuild_height(uint64_t h);
  uint64_t get_scratchpad_size_for_height(uint64_t h);
  bool is_payment_id_size_ok(const std::string& payment_id);
  std::string get_account_address_as_str(const account_public_address& addr);
  std::string get_account_address_and_payment_id_as_str(const account_public_address& addr, const std::string& payment_id);
  bool get_account_address_from_str(account_public_address& addr, const std::string& str);
  bool get_account_address_and_payment_id_from_str(account_public_address& addr, std::string& payment_id, const std::string& str);
  bool is_coinbase(const transaction& tx);
  bool is_coinbase(const transaction& tx, bool& pos_coinbase);
  bool have_attachment_service_in_container(const std::vector<attachment_v>& av, const std::string& service_id, const std::string& instruction);
  crypto::hash prepare_prefix_hash_for_sign(const transaction& tx, uint64_t in_index, const crypto::hash& tx_id);

  //---------------------------------------------------------------
  template<class tx_out_t>
  bool is_out_to_acc(const account_keys& acc, const tx_out_t& out_key, const crypto::public_key& tx_pub_key, size_t output_index)
  {
    crypto::key_derivation derivation;
    generate_key_derivation(tx_pub_key, acc.m_view_secret_key, derivation);
    return is_out_to_acc(acc, out_key, derivation, output_index);
  }
  //----------------------------------------------------------------------------------------------------
  template<class t_container>
  bool validate_attachment_info(const t_container& container, const std::vector<attachment_v>& attachments, bool allow_no_info_for_non_empty_attachments_container)
  {
    // this routine makes sure the container has correct and valid extra_attachment_info
    extra_attachment_info eai = AUTO_VAL_INIT(eai);
    bool r = get_type_in_variant_container(container, eai);
    if (!r && allow_no_info_for_non_empty_attachments_container)
      return true;

    // two valid options here: 1) no attachment info and empty attachements container; 2) attachment info present and the container is not empty
    CHECK_AND_ASSERT_MES(r == !attachments.empty(), false, "Invalid attachment info: extra_attachment_info is " << (r ? "present" : "not present") << " while attachments size is " << attachments.size());

    CHECK_AND_ASSERT_MES(eai.cnt <= attachments.size(), false, "Incorrect attachments counter: " << eai.cnt << " while attachments size is " << attachments.size());

    extra_attachment_info eai_local = AUTO_VAL_INIT(eai_local);
    if (eai.cnt == attachments.size())
    {
      get_attachment_extra_info_details(attachments, eai_local);
    }
    else
    {
      std::vector<attachment_v> attachments_local(attachments.begin(), attachments.begin() + static_cast<size_t>(eai.cnt)); // make a local copy of eai.cnt attachments
      get_attachment_extra_info_details(attachments_local, eai_local);
    }

    // ...and compare with the one from the container
    CHECK_AND_ASSERT_MES(eai == eai_local, false, "Invalid attachments info: (" << eai.cnt << "," << eai.hsh << "," << eai.sz << ")  expected: (" << eai_local.cnt << "," << eai_local.hsh << "," << eai_local.sz << ")");

    return true;
  }

  //------------------------------------------------------------------
  template<class alias_rpc_details_t>
  bool alias_info_to_rpc_alias_info(const currency::extra_alias_entry& ai, alias_rpc_details_t& ari)
  {
    return alias_info_to_rpc_alias_info(ai.m_alias, ai, ari);
  }

  template<class alias_rpc_details_t>
  bool alias_info_to_rpc_alias_info(const std::string& alias, const currency::extra_alias_entry_base& aib, alias_rpc_details_t& ari)
  {
    ari.alias = alias;
    ari.details.address = currency::get_account_address_as_str(aib.m_address);
    ari.details.comment = aib.m_text_comment;
    if (aib.m_view_key.size())
      ari.details.tracking_key = epee::string_tools::pod_to_hex(aib.m_view_key.back());

    return true;
  }

  template<class alias_rpc_details_t>
  bool alias_rpc_details_to_alias_info(const alias_rpc_details_t& ard, currency::extra_alias_entry& ai)
  {
    if (!currency::get_account_address_from_str(ai.m_address, ard.details.address))
    {
      LOG_ERROR("Failed to parse address: " << ard.details.address);
      return false;
    }
    if (ard.details.tracking_key.size())
    {
      if (!epee::string_tools::parse_tpod_from_hex_string(ard.details.tracking_key, ai.m_view_key))
      {
        LOG_ERROR("Failed to parse tracking_key: " << ard.details.tracking_key);
        return false;
      }
    }
    //ai.m_sign; atm alias updating disabled
    ai.m_text_comment = ard.details.comment;
    ai.m_alias = ard.alias;
    return true;
  }


  template<class extra_t>
  extra_t& get_or_add_field_to_extra(std::vector<extra_v>& extra)
  {
    for (auto& ev : extra)
    {
      if (ev.type() == typeid(extra_t))
        return boost::get<extra_t>(ev);
    }
    extra.push_back(extra_t());
    return boost::get<extra_t>(extra.back());
  }
  template<class variant_t, class variant_type_t>
  void update_or_add_field_to_extra(std::vector<variant_t>& variant_container, const variant_type_t& v)
  {
    for (auto& ev : variant_container)
    {
      if (ev.type() == typeid(variant_type_t))
      {
        boost::get<variant_type_t>(ev) = v;
        return;
      }
    }
    variant_container.push_back(v);
  }

  //---------------------------------------------------------------
  template<typename t_container>
  bool get_payment_id_from_tx(const t_container& att, std::string& payment_id)
  {
    tx_service_attachment sa = AUTO_VAL_INIT(sa);
    if (bc_services::get_first_service_attachment_by_id(att, BC_PAYMENT_ID_SERVICE_ID, "", sa))
    {
      payment_id = sa.body;
      return true;
    }
    return false;
  }
  //---------------------------------------------------------------
  template<class block_chain_accessor_t>
  bool get_seed_for_scratchpad_cb(uint64_t height, crypto::hash& seed, block_chain_accessor_t cb)
  {
    //CHECK_AND_ASSERT_THROW_MES(m_db_blocks.size() > height, "Internal error: m_db_blocks.size()=" << m_db_blocks.size() << " > height=" << height);
    uint64_t last_upd_h = get_scratchpad_last_update_rebuild_height(height);
    std::vector<crypto::hash> seed_data;
    if (last_upd_h == 0)
    {
      crypto::hash genesis_seed = null_hash;
      bool r = epee::string_tools::hex_to_pod(CURRENCY_SCRATCHPAD_GENESIS_SEED, genesis_seed);
      CHECK_AND_ASSERT_THROW_MES(r, "Unable to parse CURRENCY_SCRATCHPAD_GENESIS_SEED " << CURRENCY_SCRATCHPAD_GENESIS_SEED);
      LOG_PRINT_MAGENTA("[SCRATCHPAD] GENESIS SEED SELECTED: " << genesis_seed, LOG_LEVEL_0);
      seed = genesis_seed;
      return true;
    }
    uint64_t low_bound_window = 0;
    CHECK_AND_ASSERT_THROW_MES(last_upd_h >= CURRENCY_SCRATCHPAD_SEED_BLOCKS_WINDOW, "Internal error: last_upd_h(" << last_upd_h << ") < CURRENCY_SCRATCHPAD_SEED_BLOCKS_WINDOW(" << CURRENCY_SCRATCHPAD_SEED_BLOCKS_WINDOW << ")");
    low_bound_window = last_upd_h - CURRENCY_SCRATCHPAD_SEED_BLOCKS_WINDOW;

    crypto::hash selector_id = cb(last_upd_h - CURRENCY_SCRATCHPAD_BASE_INDEX_ID_OFFSET);

    const uint64_t* pselectors = (const uint64_t*)&selector_id;
    std::stringstream ss;
    for (size_t i = 0; i != 4; i++)
    {
      seed_data.push_back(cb(low_bound_window + pselectors[i] % CURRENCY_SCRATCHPAD_SEED_BLOCKS_WINDOW));
      ss << "[" << std::setw(8) << std::hex << pselectors[i] << "->" << low_bound_window + pselectors[i] % CURRENCY_SCRATCHPAD_SEED_BLOCKS_WINDOW << "]" << seed_data.back() << ENDL;
    }
    seed = crypto::cn_fast_hash(&seed_data[0], sizeof(seed_data[0]) * seed_data.size());

    LOG_PRINT_MAGENTA("[SCRATCHPAD] SEED SELECTED: h = " << last_upd_h << ", selector: " << selector_id << ENDL << ss.str() << "SEED: " << seed, LOG_LEVEL_0);

    return true;
  }

  //---------------------------------------------------------------
  // 62387455827 -> 455827 + 7000000 + 80000000 + 300000000 + 2000000000 + 60000000000, where 455827 <= dust_threshold
  template<typename chunk_handler_t, typename dust_handler_t>
  void decompose_amount_into_digits(uint64_t amount, uint64_t dust_threshold, const chunk_handler_t& chunk_handler, const dust_handler_t& dust_handler, uint64_t max_output_allowed, size_t max_outs_count, size_t outs_count = 0)
  {
    CHECK_AND_ASSERT_MES(amount != 0, void(), "zero amount");
    CHECK_AND_ASSERT_MES(amount / max_output_allowed <= max_outs_count, void(), "too big amount: " << print_money(amount) << " for given max_output_allowed: " << print_money(max_output_allowed) << ", it would require >" << amount / max_output_allowed << " outputs");

    bool is_dust_handled = false;
    uint64_t dust = 0;
    uint64_t multiplier = 1;
    while (0 != amount )
    {
      uint64_t chunk = (amount % 10) * multiplier;
      if (chunk > max_output_allowed)
        break;
      amount /= 10;
      multiplier *= 10;
      


      if (dust + chunk <= dust_threshold)
      {
        dust += chunk;
      }
      else
      {
        if (!is_dust_handled && 0 != dust)
        {
          dust_handler(dust);
          is_dust_handled = true;
        }
        if (0 != chunk)
        {
          chunk_handler(chunk);
          ++outs_count;
        }
      }
    }
    if (amount)
    {
      //CHECK_AND_ASSERT_MES_NO_RET(max_output_allowed >= multiplier, "max_output_allowed > multiplier");
      CHECK_AND_ASSERT_MES(multiplier != 0, void(), "decompose_amount_into_digits: internal error: multiplier == 0");
      uint64_t amount_multiplied = amount * multiplier;
      while (amount_multiplied >= max_output_allowed)
      {
        amount_multiplied -= max_output_allowed;
        chunk_handler(max_output_allowed);
        ++outs_count;
        CHECK_AND_ASSERT_MES(outs_count < max_outs_count || amount_multiplied == 0, void(), "decompose_amount_into_digits: too many outputs");
      }

      if (amount_multiplied != 0)
        decompose_amount_into_digits(amount_multiplied, dust_threshold, chunk_handler, dust_handler, max_output_allowed, max_outs_count, outs_count);
    }


    if (!is_dust_handled && 0 != dust)
    {
      dust_handler(dust);
    }
  }
  
  template<typename chunk_handler_t, typename dust_handler_t>
  void decompose_amount_into_digits(uint64_t amount, uint64_t dust_threshold, const chunk_handler_t& chunk_handler, const dust_handler_t& dust_handler)
  {
    decompose_amount_into_digits(amount, dust_threshold, chunk_handler, dust_handler, WALLET_MAX_ALLOWED_OUTPUT_AMOUNT, CURRENCY_TX_MAX_ALLOWED_OUTS, 0);
  }

  template<typename chunk_handler_t, typename dust_handler_t>
  void decompose_amount_into_digits(uint64_t amount, uint64_t dust_threshold, const chunk_handler_t& chunk_handler, const dust_handler_t& dust_handler, uint64_t max_output_allowed)
  {
    decompose_amount_into_digits(amount, dust_threshold, chunk_handler, dust_handler, max_output_allowed, CURRENCY_TX_MAX_ALLOWED_OUTS, 0);
  }
  //---------------------------------------------------------------
  inline size_t get_input_expected_signatures_count(const txin_v& tx_in)
  {
    struct txin_signature_size_visitor : public boost::static_visitor<size_t>
    {
      size_t operator()(const txin_gen& /*txin*/) const   { return 0; }
      size_t operator()(const txin_to_key& txin) const    { return txin.key_offsets.size(); }
      size_t operator()(const txin_multisig& txin) const  { return txin.sigs_count; }
    };

    return boost::apply_visitor(txin_signature_size_visitor(), tx_in);
  }
  //---------------------------------------------------------------
  inline const std::vector<txin_etc_details_v>* get_input_etc_details(const txin_v& in)
  {
    if (in.type().hash_code() == typeid(txin_to_key).hash_code())
      return &boost::get<txin_to_key>(in).etc_details;
    if (in.type().hash_code() == typeid(txin_multisig).hash_code())
      return &boost::get<txin_multisig>(in).etc_details;
    return nullptr;
  }
  //---------------------------------------------------------------
  inline std::vector<txin_etc_details_v>* get_input_etc_details(txin_v& in)
  {
    if (in.type().hash_code() == typeid(txin_to_key).hash_code())
      return &boost::get<txin_to_key>(in).etc_details;
    if (in.type().hash_code() == typeid(txin_multisig).hash_code())
      return &boost::get<txin_multisig>(in).etc_details;
    return nullptr;
  }
  //---------------------------------------------------------------

  struct input_amount_getter : public boost::static_visitor<uint64_t>
  {
    template<class t_input>
    uint64_t operator()(const t_input& i) const{return i.amount;}
    uint64_t operator()(const txin_gen& i) const {return 0;}
  };

  inline uint64_t get_amount_from_variant(const txin_v& v)
  {
    return boost::apply_visitor(input_amount_getter(), v);
  }
  //---------------------------------------------------------------
  std::ostream& operator <<(std::ostream& o, const ref_by_id& r);
  //---------------------------------------------------------------
  std::string utf8_to_upper(const std::string& s);
  std::string utf8_to_lower(const std::string& s);
  bool utf8_substring_test_case_insensitive(const std::string& match, const std::string& s); // Returns true is 's' contains 'match' (case-insensitive)

} // namespace currency



