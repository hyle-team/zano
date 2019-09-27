// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
#include <boost/foreach.hpp>
#include <boost/locale.hpp>
using namespace epee;

#include "print_fixed_point_helper.h"
#include "currency_format_utils.h"
#include "serialization/binary_utils.h"
#include "serialization/stl_containers.h"
#include "currency_core/currency_config.h"
#include "miner.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "common/int-util.h"
#include "common/base58.h"
#include "bc_offers_service_basic.h"
#include "bc_payments_id_service.h"
#include "bc_escrow_service.h"
#include "bc_attachments_helpers.h"
#include "genesis.h"
#include "genesis_acc.h"
#include "common/mnemonic-encoding.h"

namespace currency
{

  //---------------------------------------------------------------
  bool add_tx_extra_alias(transaction& tx, const extra_alias_entry& alinfo)
  {
    tx.extra.push_back(alinfo);
    return true;
  }

  //---------------------------------------------------------------
  /*
  bool construct_miner_tx(size_t height, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins,
  size_t current_block_size,
  uint64_t fee,
  const account_public_address &miner_address,
  transaction& tx,
  const blobdata& extra_nonce,
  size_t max_outs)
  {

  alias_info alias = AUTO_VAL_INIT(alias);
  return construct_miner_tx(height, median_size, already_generated_coins, current_block_size,
  fee,
  miner_address,
  tx,
  extra_nonce,
  max_outs,
  alias,
  false,
  pos_entry());
  }*/
  //---------------------------------------------------------------
  uint64_t get_coinday_weight(uint64_t amount)
  {
    return amount;
  }
  //---------------------------------------------------------------
  wide_difficulty_type correct_difficulty_with_sequence_factor(size_t sequence_factor, wide_difficulty_type diff)
  {
    //delta=delta*(0.75^n)
    for (size_t i = 0; i != sequence_factor; i++)
    {
      diff = diff - diff / 4;
    }
    return diff;
  }
  //------------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins,
    size_t current_block_size,
    uint64_t fee,
    const account_public_address &miner_address,
    const account_public_address &stakeholder_address,
    transaction& tx,
    const blobdata& extra_nonce,
    size_t max_outs,
    bool pos,
    const pos_entry& pe)
  {
    uint64_t block_reward;
    if (!get_block_reward(pos, median_size, current_block_size, already_generated_coins, block_reward, height))
    {
      LOG_ERROR("Block is too big");
      return false;
    }
    block_reward += fee;
      
    std::vector<size_t> out_amounts;
    decompose_amount_into_digits(block_reward, DEFAULT_DUST_THRESHOLD,
      [&out_amounts](uint64_t a_chunk) { out_amounts.push_back(a_chunk); },
      [&out_amounts](uint64_t a_dust) { out_amounts.push_back(a_dust); });

    CHECK_AND_ASSERT_MES(1 <= max_outs, false, "max_out must be non-zero");
    while (max_outs < out_amounts.size())
    {
      out_amounts[out_amounts.size() - 2] += out_amounts.back();
      out_amounts.resize(out_amounts.size() - 1);
    }


    std::vector<tx_destination_entry> destinations;
    for (auto a : out_amounts)
    {
      tx_destination_entry de = AUTO_VAL_INIT(de);
      de.addr.push_back(miner_address);
      de.amount = a;
      if (pe.stake_unlock_time && pe.stake_unlock_time > height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
      {
        //this means that block is creating after hardfork_1 and unlock_time is needed to set for every destination separately
        de.unlock_time = height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
      }
      destinations.push_back(de);
    }

    if (pos)
    {
      uint64_t stake_lock_time = 0;
      if (pe.stake_unlock_time && pe.stake_unlock_time > height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
        stake_lock_time = pe.stake_unlock_time;
      destinations.push_back(tx_destination_entry(pe.amount, stakeholder_address, stake_lock_time));
    }
      

    return construct_miner_tx(height, median_size, already_generated_coins, current_block_size, fee, destinations, tx, extra_nonce, max_outs, pos, pe);
  }
  //------------------------------------------------------------------
  bool apply_unlock_time(const std::vector<tx_destination_entry>& destinations, transaction& tx)
  {
    currency::etc_tx_details_unlock_time2 unlock_time2 = AUTO_VAL_INIT(unlock_time2);
    unlock_time2.unlock_time_array.resize(destinations.size());
    bool found_unlock_time = false;
    for (size_t i = 0; i != unlock_time2.unlock_time_array.size(); i++)
    {
      if (destinations[i].unlock_time)
      {
        found_unlock_time = true;
        unlock_time2.unlock_time_array[i] = destinations[i].unlock_time;
      }
    }
    if (found_unlock_time)
    {
      tx.extra.push_back(unlock_time2);
    }

    return true;
  }
  //------------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins,
    size_t current_block_size,
    uint64_t fee,
    const std::vector<tx_destination_entry>& destinations,
    transaction& tx,
    const blobdata& extra_nonce,
    size_t max_outs,
    bool pos,
    const pos_entry& pe)
  {
    CHECK_AND_ASSERT_MES(destinations.size() <= CURRENCY_TX_MAX_ALLOWED_OUTS || height == 0, false, "Too many outs (" << destinations.size() << ")! Miner tx can't be constructed.");

    tx.vin.clear();
    tx.vout.clear();
    tx.extra.clear();

    keypair txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);
    if (extra_nonce.size())
      if (!add_tx_extra_userdata(tx, extra_nonce))
        return false;

    //at this moment we do apply_unlock_time only for coin_base transactions 
    apply_unlock_time(destinations, tx);
    //we always add extra_padding with 2 bytes length to make possible for get_block_template to adjust cumulative size
    tx.extra.push_back(extra_padding());


    txin_gen in;
    in.height = height;
    tx.vin.push_back(in);

    if (pos)
    {
      txin_to_key posin;
      posin.amount = pe.amount;
      posin.key_offsets.push_back(pe.index);
      posin.k_image = pe.keyimage;
      tx.vin.push_back(posin);
      //reserve place for ring signature
      tx.signatures.resize(1);
      tx.signatures[0].resize(posin.key_offsets.size());
    }

    uint64_t no = 0;
    std::set<uint16_t> deriv_cache;
    for (auto& d : destinations)
    {
      bool r = construct_tx_out(d, txkey.sec, no, tx, deriv_cache);
      CHECK_AND_ASSERT_MES(r, false, "Failed to contruct miner tx out");
      no++;
    }
    

    tx.version = CURRENT_TRANSACTION_VERSION;
    if (!have_type_in_variant_container<etc_tx_details_unlock_time2>(tx.extra))
    {
      //if stake unlock time was not set, then we can use simple "whole transaction" lock scheme 
      set_tx_unlock_time(tx, height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
    }
    
    return true;
  }
  //---------------------------------------------------------------
  bool derive_ephemeral_key_helper(const account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, keypair& in_ephemeral)
  {
    crypto::key_derivation recv_derivation = AUTO_VAL_INIT(recv_derivation);
    bool r = crypto::generate_key_derivation(tx_public_key, ack.m_view_secret_key, recv_derivation);
    CHECK_AND_ASSERT_MES(r, false, "key image helper: failed to generate_key_derivation(" << tx_public_key << ", " << ack.m_view_secret_key << ")");

    r = crypto::derive_public_key(recv_derivation, real_output_index, ack.m_account_address.m_spend_public_key, in_ephemeral.pub);
    CHECK_AND_ASSERT_MES(r, false, "key image helper: failed to derive_public_key(" << recv_derivation << ", " << real_output_index << ", " << ack.m_account_address.m_spend_public_key << ")");

    crypto::derive_secret_key(recv_derivation, real_output_index, ack.m_spend_secret_key, in_ephemeral.sec);
    return true;
  }
  //---------------------------------------------------------------
  alias_rpc_details alias_info_to_rpc_alias_info(const currency::extra_alias_entry& ai)
  {
    alias_rpc_details ari;
    alias_info_to_rpc_alias_info(ai, ari);
    return ari;
  }
  //---------------------------------------------------------------
  update_alias_rpc_details alias_info_to_rpc_update_alias_info(const currency::extra_alias_entry& ai, const std::string& old_address)
  {
    update_alias_rpc_details uard;
    alias_info_to_rpc_alias_info(ai, uard);
    uard.old_address = old_address;
    return uard;
  }
  //---------------------------------------------------------------
  bool generate_key_image_helper(const account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, keypair& in_ephemeral, crypto::key_image& ki)
  {
    bool r = derive_ephemeral_key_helper(ack, tx_public_key, real_output_index, in_ephemeral);
    CHECK_AND_ASSERT_MES(r, false, "Failed to call derive_ephemeral_key_helper(...)");

    crypto::generate_key_image(in_ephemeral.pub, in_ephemeral.sec, ki);
    return true;
  }
  //---------------------------------------------------------------
  uint64_t power_integral(uint64_t a, uint64_t b)
  {
    if (b == 0)
      return 1;
    uint64_t total = a;
    for (uint64_t i = 1; i != b; i++)
      total *= a;
    return total;
  }
  //---------------------------------------------------------------
  bool is_mixattr_applicable_for_fake_outs_counter(uint8_t mix_attr, uint64_t fake_outputs_count)
  {
    if (mix_attr >= CURRENCY_TO_KEY_OUT_FORCED_MIX_LOWER_BOUND)
      return fake_outputs_count + 1 >= mix_attr;
    else if (mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
      return fake_outputs_count == 0;
    else
      return true;
  }
  //---------------------------------------------------------------
  bool parse_amount(uint64_t& amount, const std::string& str_amount_)
  {
    std::string str_amount = str_amount_;
    boost::algorithm::trim(str_amount);

    size_t point_index = str_amount.find_first_of('.');
    size_t fraction_size;
    if (std::string::npos != point_index)
    {
      fraction_size = str_amount.size() - point_index - 1;
      while (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size && '0' == str_amount.back())
      {
        str_amount.erase(str_amount.size() - 1, 1);
        --fraction_size;
      }
      if (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size)
        return false;
      str_amount.erase(point_index, 1);
    }
    else
    {
      fraction_size = 0;
    }

    if (str_amount.empty())
      return false;

    if (fraction_size < CURRENCY_DISPLAY_DECIMAL_POINT)
    {
      str_amount.append(CURRENCY_DISPLAY_DECIMAL_POINT - fraction_size, '0');
    }

    return string_tools::get_xtype_from_string(amount, str_amount);
  }
  //--------------------------------------------------------------------------------
  std::string print_stake_kernel_info(const stake_kernel& sk)
  {
    std::stringstream ss;
    ss << "block_timestamp: " << sk.block_timestamp << ENDL
      << "kimage: " << sk.kimage << ENDL
      << "stake_modifier.last_pos_kernel_id: " << sk.stake_modifier.last_pos_kernel_id << ENDL
      << "stake_modifier.last_pow_id: " << sk.stake_modifier.last_pow_id << ENDL;
    return ss.str();
  }

  //---------------------------------------------------------------
  bool get_tx_fee(const transaction& tx, uint64_t & fee)
  {
    fee = 0;
    if (is_coinbase(tx))
      return true;
    uint64_t amount_in = 0;
    uint64_t amount_out = get_outs_money_amount(tx);
   
    BOOST_FOREACH(auto& in, tx.vin)
    {
      amount_in += get_amount_from_variant(in);
    }



    CHECK_AND_ASSERT_MES(amount_in >= amount_out, false, "transaction spend (" << amount_in << ") more than it has (" << amount_out << ")");
    fee = amount_in - amount_out;
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_tx_fee(const transaction& tx)
  {
    uint64_t r = 0;
    if (!get_tx_fee(tx, r))
      return 0;
    return r;
  }
  //---------------------------------------------------------------
  crypto::public_key get_tx_pub_key_from_extra(const transaction& tx)
  {
    crypto::public_key pk = null_pkey;
    get_type_in_variant_container(tx.extra, pk);
    return pk;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_extra(const transaction& tx, crypto::public_key& tx_pub_key)
  {
    tx_extra_info e = AUTO_VAL_INIT(e);
    bool r = parse_and_validate_tx_extra(tx, e);
    tx_pub_key = e.m_tx_pub_key;
    return r;
  }
  //---------------------------------------------------------------
  bool sign_extra_alias_entry(extra_alias_entry& ai, const crypto::public_key& pkey, const crypto::secret_key& skey)
  {
    ai.m_sign.clear();
    get_sign_buff_hash_for_alias_update(ai);
    crypto::signature sig = AUTO_VAL_INIT(sig);
    crypto::generate_signature(get_sign_buff_hash_for_alias_update(ai), pkey, skey, sig);
    ai.m_sign.push_back(sig);
    return true;
  }
  //---------------------------------------------------------------
  crypto::hash get_sign_buff_hash_for_alias_update(const extra_alias_entry& ai)
  {
    const extra_alias_entry* pale = &ai;
    extra_alias_entry eae_local = AUTO_VAL_INIT(eae_local);
    if (ai.m_sign.size())
    {
      eae_local = ai;
      eae_local.m_sign.clear();
      pale = &eae_local;
    }
    return get_object_hash(*pale);
  }
  //---------------------------------------------------------------
  struct tx_extra_handler : public boost::static_visitor<bool>
  {
    mutable bool was_padding; //let the padding goes only at the end
    mutable bool was_pubkey;
    mutable bool was_attachment;
    mutable bool was_userdata;
    mutable bool was_alias;

    tx_extra_info& rei;
    const transaction& rtx;

    tx_extra_handler(tx_extra_info& ei, const transaction& tx) :rei(ei), rtx(tx)
    {
      was_padding = was_pubkey = was_attachment = was_userdata = was_alias = false;
    }

#define ENSURE_ONETIME(varname, entry_name) CHECK_AND_ASSERT_MES(varname == false, false, "double entry in tx_extra: " entry_name); varname = true;


    bool operator()(const crypto::public_key& k) const
    {
      ENSURE_ONETIME(was_pubkey, "public_key");
      rei.m_tx_pub_key = k;
      return true;
    }
    bool operator()(const extra_attachment_info& ai) const
    {
      ENSURE_ONETIME(was_attachment, "attachment");
      rei.m_attachment_info = ai;
      return true;
    }

    bool operator()(const extra_alias_entry& ae) const
    {
      ENSURE_ONETIME(was_alias, "alias");
      rei.m_alias = ae;
      return true;
    }
    bool operator()(const extra_user_data& ud) const
    {
      ENSURE_ONETIME(was_userdata, "userdata");
      if (!ud.buff.empty())
        rei.m_user_data_blob.assign((const char*)&ud.buff[0], ud.buff.size());
      return true;
    }
    bool operator()(const extra_padding& k) const
    {
      ENSURE_ONETIME(was_padding, "padding");
      return true;
    }
    template<class t_extra_typename>
    bool operator()(const t_extra_typename& k) const
    {
      //do notheing for rest
      return true;
    }
  };
  //------------------------------------------------------------------------------------
  bool parse_and_validate_tx_extra(const transaction& tx, tx_extra_info& extra)
  {
    extra.m_tx_pub_key = null_pkey;

    tx_extra_handler vtr(extra, tx);

    for (const auto& ex_v : tx.extra)
    {
      if (!boost::apply_visitor(vtr, ex_v))
        return false;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool add_tx_pub_key_to_extra(transaction& tx, const crypto::public_key& tx_pub_key)
  {
    tx.extra.push_back(tx_pub_key);
    return true;
  }
  //---------------------------------------------------------------
  //---------------------------------------------------------------

  struct multisig_id_generator
  {
    //std::vector<txin_v> vin;
    crypto::public_key onetime_key;
    std::vector<tx_out> vout;

    BEGIN_SERIALIZE()
      //FIELD(vin)
      FIELD(onetime_key)
      FIELD(vout)
    END_SERIALIZE()
  };
  crypto::hash get_multisig_out_id(const transaction& tx, size_t n)
  {
    multisig_id_generator msg = AUTO_VAL_INIT(msg);
    //msg.vin = tx.vin;
    msg.onetime_key = get_tx_pub_key_from_extra(tx);
    CHECK_AND_ASSERT_MES(tx.vout.size() > n, null_hash, "tx.vout.size() > n condition failed ");
    CHECK_AND_ASSERT_MES(tx.vout[n].target.type() == typeid(txout_multisig), null_hash, "tx.vout[n].target.type() == typeid(txout_multisig) condition failed");
    msg.vout.push_back(tx.vout[n]);
    return get_object_hash(msg);
  }
  //---------------------------------------------------------------
  bool add_tx_extra_userdata(transaction& tx, const blobdata& extra_nonce)
  {
    CHECK_AND_ASSERT_MES(extra_nonce.size() <= 255, false, "extra nonce could be 255 bytes max");
    extra_user_data eud = AUTO_VAL_INIT(eud);
    eud.buff = extra_nonce;
    tx.extra.push_back(eud);
    return true;
  }
  //---------------------------------------------------------------
  bool derive_public_key_from_target_address(const account_public_address& destination_addr, const crypto::secret_key& tx_sec_key, size_t index, crypto::public_key& out_eph_public_key)
  {
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    return derive_public_key_from_target_address(destination_addr, tx_sec_key, index, out_eph_public_key, derivation);
  }
  //---------------------------------------------------------------
  bool derive_public_key_from_target_address(const account_public_address& destination_addr, const crypto::secret_key& tx_sec_key, size_t index, crypto::public_key& out_eph_public_key, crypto::key_derivation& derivation)
  {    
    bool r = crypto::generate_key_derivation(destination_addr.m_view_public_key, tx_sec_key, derivation);
    CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to generate_key_derivation(" << destination_addr.m_view_public_key << ", " << tx_sec_key << ")");

    r = crypto::derive_public_key(derivation, index, destination_addr.m_spend_public_key, out_eph_public_key);
    CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to derive_public_key(" << derivation << ", " << index << ", " << destination_addr.m_view_public_key << ")");
    return r;
  }
  //---------------------------------------------------------------
  uint16_t get_derivation_hint(const crypto::key_derivation& derivation)
  {
    crypto::hash h = blake2_hash(&derivation, sizeof(derivation));

    uint16_t* pderiv_hash_as_array = (uint16_t*)&h;
    uint16_t res = pderiv_hash_as_array[0];
    for (size_t i = 1; i != sizeof(h) / sizeof(uint16_t); i++)
      res ^= pderiv_hash_as_array[i];
    return res;
  }
  //---------------------------------------------------------------
  uint64_t get_string_uint64_hash(const std::string& str)
  {
    crypto::hash h = crypto::cn_fast_hash(str.data(), str.size());
    uint64_t* phash_as_array = (uint64_t*)&h;
    return phash_as_array[0] ^ phash_as_array[1] ^ phash_as_array[2] ^ phash_as_array[3];
  }
  //---------------------------------------------------------------
  tx_derivation_hint make_tx_derivation_hint_from_uint16(uint16_t hint)
  {
    tx_derivation_hint dh = AUTO_VAL_INIT(dh);
    dh.msg.assign((const char*)&hint, sizeof(hint));
    return dh;
  }
  //---------------------------------------------------------------
//   bool get_uint16_from_tx_derivation_hint(const tx_derivation_hint& dh, uint16_t& hint)
//   {
//     tx_derivation_hint dh;
//     if (dh.msg.size() != sizeof(hint))
//       return false;
//     hint = *((uint16_t*)dh.msg.data());
//     return true;
//   }  
  //---------------------------------------------------------------
  bool construct_tx_out(const tx_destination_entry& de, const crypto::secret_key& tx_sec_key, size_t output_index, transaction& tx, std::set<uint16_t>& deriv_cache, uint8_t tx_outs_attr)
  {
    CHECK_AND_ASSERT_MES(de.addr.size() == 1 || (de.addr.size() > 1 && de.minimum_sigs <= de.addr.size()), false, "Invalid destination entry: amount: " << de.amount << " minimum_sigs: " << de.minimum_sigs << " addr.size(): " << de.addr.size());

    std::vector<crypto::public_key> target_keys;
    target_keys.reserve(de.addr.size());
    for (auto& apa : de.addr)
    {
      crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
      if (apa.m_spend_public_key == null_pkey && apa.m_view_public_key == null_pkey)
      {
        //burning money(for example alias reward)
        out_eph_public_key = null_pkey;
      }
      else
      {
        crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
        bool r = derive_public_key_from_target_address(apa, tx_sec_key, output_index, out_eph_public_key, derivation);
        CHECK_AND_ASSERT_MES(r, false, "failed to derive_public_key_from_target_address");

        uint16_t hint = get_derivation_hint(derivation);
        if (deriv_cache.count(hint) == 0)
        {          
          tx.extra.push_back(make_tx_derivation_hint_from_uint16(hint));
          deriv_cache.insert(hint);
        }
      }
      target_keys.push_back(out_eph_public_key);
    }

    tx_out out;
    out.amount = de.amount;
    if (target_keys.size() == 1)
    {
      //out to key
      txout_to_key tk;
      tk.key = target_keys.back();
      tk.mix_attr = tx_outs_attr;
      out.target = tk;
    }
    else
    {
      //multisig out
      txout_multisig ms = AUTO_VAL_INIT(ms);
      ms.keys = std::move(target_keys);
      ms.minimum_sigs = de.minimum_sigs;
      out.target = ms;
    }
    tx.vout.push_back(out);
    return true;
  }
  //---------------------------------------------------------------
  bool have_attachment_service_in_container(const std::vector<attachment_v>& av, const std::string& service_id, const std::string& instruction)
  {
    for (auto& ai : av)
    {
      if (ai.type() == typeid(tx_service_attachment))
      {
        const tx_service_attachment& tsa = boost::get<tx_service_attachment>(ai);
        if (tsa.service_id == service_id && tsa.instruction == instruction)
          return true;
      }
    }
    return false;
  }
  //---------------------------------------------------------------
  void get_attachment_extra_info_details(const std::vector<attachment_v>& attachment, extra_attachment_info& eai)
  {
    eai = extra_attachment_info();
    if (!attachment.size())
      return;

    //put hash into extra
    std::stringstream ss;
    binary_archive<true> oar(ss);
    ::do_serialize(oar, const_cast<std::vector<attachment_v>&>(attachment));
    std::string buff = ss.str();
    eai.sz = buff.size();
    eai.hsh = get_blob_hash(buff);
    eai.cnt = attachment.size();
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources,
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<attachment_v>& attachments,
    transaction& tx,
    uint64_t unlock_time,
    uint8_t tx_outs_attr, bool shuffle)
  {
    crypto::secret_key one_time_secret_key = AUTO_VAL_INIT(one_time_secret_key);
    return construct_tx(sender_account_keys, sources, destinations, std::vector<extra_v>(), attachments, tx, one_time_secret_key, unlock_time, tx_outs_attr, shuffle);
  }
  //---------------------------------------------------------------
  struct encrypt_attach_visitor : public boost::static_visitor<void>
  {
    bool& m_was_crypted_entries;
    const crypto::key_derivation& m_key;
    encrypt_attach_visitor(bool& was_crypted_entries, const crypto::key_derivation& key) :m_was_crypted_entries(was_crypted_entries), m_key(key)
    {}
    void operator()(tx_comment& comment)
    {
      crypto::chacha_crypt(comment.comment, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_payer& pr)
    {
      crypto::chacha_crypt(pr.acc_addr, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_receiver& m)
    {
      crypto::chacha_crypt(m.acc_addr, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_service_attachment& sa)
    {
      if (sa.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY)
      {
        zlib_helper::pack(sa.body);
      }

      if (sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY)
      {
        crypto::chacha_crypt(sa.body, m_key);
        m_was_crypted_entries = true;
      }
    }

    template<typename attachment_t>
    void operator()(attachment_t& comment)
    {}
  };

  struct decrypt_attach_visitor : public boost::static_visitor<void>
  {
    const crypto::key_derivation& rkey;
    std::vector<payload_items_v>& rdecrypted_att;
    decrypt_attach_visitor(const crypto::key_derivation& key,
      std::vector<payload_items_v>& decrypted_att) :
      rkey(key),
      rdecrypted_att(decrypted_att)
    {}
    void operator()(const tx_comment& comment)
    {
      tx_comment local_comment = comment;
      crypto::chacha_crypt(local_comment.comment, rkey);
      rdecrypted_att.push_back(local_comment);
    }

    void operator()(const tx_service_attachment& sa)
    {
      tx_service_attachment local_sa = sa;
      if (sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY)
      {
        crypto::chacha_crypt(local_sa.body, rkey);
      }

      if (sa.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY)
      {
        zlib_helper::unpack(local_sa.body);
      }
      rdecrypted_att.push_back(local_sa);
    }

    void operator()(const tx_payer& pr)
    {
      tx_payer payer_local = pr;
      crypto::chacha_crypt(payer_local.acc_addr, rkey);
      rdecrypted_att.push_back(payer_local);
    }
    void operator()(const tx_receiver& pr)
    {
      tx_receiver receiver_local = pr;
      crypto::chacha_crypt(receiver_local.acc_addr, rkey);
      rdecrypted_att.push_back(receiver_local);
    }

    template<typename attachment_t>
    void operator()(const attachment_t& att)
    {
      rdecrypted_att.push_back(att);
    }
  };

  //---------------------------------------------------------------
  template<class items_container_t>
  bool decrypt_payload_items(const crypto::key_derivation& derivation, const items_container_t& items_to_decrypt, std::vector<payload_items_v>& decrypted_att)
  {
    decrypt_attach_visitor v(derivation, decrypted_att);
    for (auto& a : items_to_decrypt)
      boost::apply_visitor(v, a);

    return true;
  }
  //---------------------------------------------------------------
  bool is_derivation_used_to_encrypt(const transaction& tx, const crypto::key_derivation& derivation)
  {
    tx_crypto_checksum crypto_info = AUTO_VAL_INIT(crypto_info);
    if (!get_type_in_variant_container(tx.extra, crypto_info) && !get_type_in_variant_container(tx.attachment, crypto_info))
    {
      //no crypt info in tx
      return false;
    }
    //validate derivation we here.
    crypto::hash hash_for_check_sum = crypto::cn_fast_hash(&derivation, sizeof(derivation));
    if (*(uint32_t*)&hash_for_check_sum == crypto_info.derivation_hash)
      return true;

    return false;
  }
  //---------------------------------------------------------------
  crypto::key_derivation get_encryption_key_derivation(bool is_income, const transaction& tx, const account_keys& acc_keys)
  {
    crypto::key_derivation derivation = null_derivation;
    tx_crypto_checksum crypto_info = AUTO_VAL_INIT(crypto_info);
    if (!get_type_in_variant_container(tx.extra, crypto_info) && !get_type_in_variant_container(tx.attachment, crypto_info))
    {
      //no crypt info in tx
      return null_derivation;
    }

    if (is_income)
    {
      crypto::public_key tx_pub_key = currency::get_tx_pub_key_from_extra(tx);

      bool r = crypto::generate_key_derivation(tx_pub_key, acc_keys.m_view_secret_key, derivation);
      CHECK_AND_ASSERT_MES(r, null_derivation, "failed to generate_key_derivation");
      LOG_PRINT_GREEN("DECRYPTING ON KEY: " << epee::string_tools::pod_to_hex(derivation) << ", key derived from destination addr: " << currency::get_account_address_as_str(acc_keys.m_account_address), LOG_LEVEL_0);
    }
    else
    {
      derivation = crypto_info.encrypted_key_derivation;
      crypto::chacha_crypt(derivation, acc_keys.m_spend_secret_key);
      LOG_PRINT_GREEN("DECRYPTING ON KEY: " << epee::string_tools::pod_to_hex(derivation) << ", key decrypted from sender address: " << currency::get_account_address_as_str(acc_keys.m_account_address), LOG_LEVEL_0);
    }

    //validate derivation we here. Yoda style
    crypto::hash hash_for_check_sum = crypto::cn_fast_hash(&derivation, sizeof(derivation));
    CHECK_AND_ASSERT_MES(*(uint32_t*)&hash_for_check_sum == crypto_info.derivation_hash, null_derivation, "Derivation hash missmatched in tx id " << currency::get_transaction_hash(tx));
    return derivation;
  }
  //---------------------------------------------------------------
  template<class total_t_container>
  struct back_inserter : public boost::static_visitor<void>
  {
    total_t_container& ttc;
    back_inserter(total_t_container& tt) :ttc(tt)
    {}
    template<typename attachment_t>
    void operator()(const attachment_t& att)
    {
      ttc.push_back(att);
    }
  };

  bool decrypt_payload_items(bool is_income, const transaction& tx, const account_keys& acc_keys, std::vector<payload_items_v>& decrypted_items)
  {
    PROFILE_FUNC("currency::decrypt_payload_items");
    crypto::key_derivation derivation = get_encryption_key_derivation(is_income, tx, acc_keys);
    if (derivation == null_derivation)
    {
      back_inserter<std::vector<payload_items_v> > v(decrypted_items);
      for (auto& a : tx.extra)
        boost::apply_visitor(v, a);

      for (auto& a : tx.attachment)
        boost::apply_visitor(v, a);
      
      return true;
    }
    
    decrypt_payload_items(derivation, tx.extra, decrypted_items);
    decrypt_payload_items(derivation, tx.attachment, decrypted_items);
    return true;
  }

  //---------------------------------------------------------------
  void encrypt_attachments(transaction& tx, const account_keys& sender_keys, const account_public_address& destination_addr, const keypair& tx_random_key)
  {
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    bool r = crypto::generate_key_derivation(destination_addr.m_view_public_key, tx_random_key.sec, derivation);
    CHECK_AND_ASSERT_MES(r, void(), "failed to generate_key_derivation");
    bool was_attachment_crypted_entries = false;
    bool was_extra_crypted_entries = false;

    encrypt_attach_visitor v(was_attachment_crypted_entries, derivation);
    for (auto& a : tx.attachment)
      boost::apply_visitor(v, a);

    encrypt_attach_visitor v2(was_extra_crypted_entries, derivation);
    for (auto& a : tx.extra)
      boost::apply_visitor(v2, a);


    if (was_attachment_crypted_entries || was_extra_crypted_entries)
    {
      crypto::hash hash_for_check_sum = crypto::cn_fast_hash(&derivation, sizeof(derivation));
      tx_crypto_checksum chs;
      //put derivation hash to be sure that we decrypting right way
      chs.derivation_hash = *(uint32_t*)&hash_for_check_sum;
      //put encrypted derivation to let sender decrypt all this data from attachment/extra
      chs.encrypted_key_derivation = derivation;
      crypto::chacha_crypt(chs.encrypted_key_derivation, sender_keys.m_spend_secret_key);
      if (was_extra_crypted_entries)
        tx.extra.push_back(chs);
      else
        tx.attachment.push_back(chs);

      LOG_PRINT_GREEN("ENCRYPTING ATTACHMENTS ON KEY: " << epee::string_tools::pod_to_hex(derivation) 
        << " destination addr: " << currency::get_account_address_as_str(destination_addr) 
        << " tx_random_key.sec" << tx_random_key.sec 
        << " tx_random_key.pub" << tx_random_key.pub
        << " sender address: " << currency::get_account_address_as_str(sender_keys.m_account_address)
        , LOG_LEVEL_0);

    }
  }
  //---------------------------------------------------------------
  uint64_t get_tx_type(const transaction& tx)
  {
    if (is_coinbase(tx))
      return GUI_TX_TYPE_COIN_BASE;

    // aliases
    tx_extra_info ei = AUTO_VAL_INIT(ei);
    parse_and_validate_tx_extra(tx, ei);
    if (ei.m_alias.m_alias.size())
    {
      if (ei.m_alias.m_sign.size())
        return GUI_TX_TYPE_UPDATE_ALIAS;
      else
        return GUI_TX_TYPE_NEW_ALIAS;
    }
    
    // offers
    tx_service_attachment a = AUTO_VAL_INIT(a);
    if (get_type_in_variant_container(tx.attachment, a))
    {
      if (a.service_id == BC_OFFERS_SERVICE_ID)
      {
        if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_ADD)
          return GUI_TX_TYPE_PUSH_OFFER;
        else if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_UPD)
          return GUI_TX_TYPE_UPDATE_OFFER;
        else if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_DEL)
          return GUI_TX_TYPE_CANCEL_OFFER;
      }
    }
    
    // escrow
    tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_TEMPLATES, tsa))
      return GUI_TX_TYPE_ESCROW_TRANSFER;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_PROPOSAL, tsa))
      return GUI_TX_TYPE_ESCROW_PROPOSAL;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL, tsa))
      return GUI_TX_TYPE_ESCROW_RELEASE_NORMAL;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN, tsa))
      return GUI_TX_TYPE_ESCROW_RELEASE_BURN;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL, tsa))
      return GUI_TX_TYPE_ESCROW_RELEASE_CANCEL;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL, tsa))
      return GUI_TX_TYPE_ESCROW_CANCEL_PROPOSAL;

    return GUI_TX_TYPE_NORMAL;
  }
  //---------------------------------------------------------------
  size_t get_multisig_out_index(const std::vector<tx_out>& outs)
  {
    size_t n = 0;
    for (; n != outs.size(); n++)
    {
      if (outs[n].target.type() == typeid(txout_multisig))
        break;
    }
    return n;
  }
  //---------------------------------------------------------------
  size_t get_multisig_in_index(const std::vector<txin_v>& inputs)
  {
    size_t n = 0;
    for (; n != inputs.size(); n++)
    {
      if (inputs[n].type() == typeid(txin_multisig))
        break;
    }
    return n;
  }

  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys,
    const std::vector<tx_source_entry>& sources,
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<extra_v>& extra,
    const std::vector<attachment_v>& attachments,
    transaction& tx,
    crypto::secret_key& one_time_secret_key,
    uint64_t unlock_time,
    uint8_t tx_outs_attr,
    bool shuffle,
    uint64_t flags)
  {
    //
    //find target account to encrypt attachment.
    //for now we use just firs target account that is not sender, 
    //in case if there is no real targets we use sender credentials to encrypt attachments
    account_public_address crypt_destination_addr = get_crypt_address_from_destinations(sender_account_keys, destinations);

    return construct_tx(sender_account_keys, sources, destinations, extra, attachments, tx, one_time_secret_key, unlock_time,
      crypt_destination_addr,
      0,
      tx_outs_attr,
      shuffle,
      flags);
  }

  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources,
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<extra_v>& extra,
    const std::vector<attachment_v>& attachments,
    transaction& tx,
    crypto::secret_key& one_time_secret_key,
    uint64_t unlock_time,
    const account_public_address& crypt_destination_addr,
    uint64_t expiration_time,
    uint8_t tx_outs_attr,
    bool shuffle,
    uint64_t flags)
  {
    CHECK_AND_ASSERT_MES(destinations.size() <= CURRENCY_TX_MAX_ALLOWED_OUTS, false, "Too many outs (" << destinations.size() << ")! Tx can't be constructed.");

    bool append_mode = false;
    if (flags&TX_FLAG_SIGNATURE_MODE_SEPARATE && tx.vin.size())
      append_mode = true;

    keypair txkey = AUTO_VAL_INIT(txkey);


    if (!append_mode)
    {
      tx.vin.clear();
      tx.vout.clear();
      tx.signatures.clear();
      tx.extra = extra;

      tx.version = CURRENT_TRANSACTION_VERSION;
      if (unlock_time != 0)
        set_tx_unlock_time(tx, unlock_time);

      if (flags != 0)
        set_tx_flags(tx, flags);
      //generate key pair  
      if (expiration_time != 0)
        set_tx_expiration_time(tx, expiration_time);

      txkey = keypair::generate();
      add_tx_pub_key_to_extra(tx, txkey.pub);
      one_time_secret_key = txkey.sec;

      //include offers if need
      tx.attachment = attachments;
      encrypt_attachments(tx, sender_account_keys, crypt_destination_addr, txkey);
    }
    else
    {
      txkey.pub = get_tx_pub_key_from_extra(tx);
      txkey.sec = one_time_secret_key;
      CHECK_AND_ASSERT_MES(txkey.pub != null_pkey && txkey.sec != null_skey, false, "In append mode both public and secret keys must be provided");

      //separately encrypt attachments without putting extra
      crypto::key_derivation derivation = get_encryption_key_derivation(true, tx, sender_account_keys);
      CHECK_AND_ASSERT_MES(derivation != null_derivation, false, "failed to generate_key_derivation");
      bool was_attachment_crypted_entries = false;
      std::vector<extra_v> extra_local = extra;
      std::vector<attachment_v> attachments_local = attachments;

      encrypt_attach_visitor v(was_attachment_crypted_entries, derivation);
      for (auto& a : attachments_local)
        boost::apply_visitor(v, a);
      for (auto& a : extra_local)
        boost::apply_visitor(v, a);


      tx.attachment.insert(tx.attachment.end(), attachments_local.begin(), attachments_local.end());
      tx.extra.insert(tx.extra.end(), extra_local.begin(), extra_local.end());
    }



    // prepare inputs
    struct input_generation_context_data
    {
      keypair in_ephemeral;
      //std::vector<keypair> participants_derived_keys;
    };
    std::vector<input_generation_context_data> in_contexts;

    size_t input_starter_index = tx.vin.size();
    uint64_t summary_inputs_money = 0;
    //fill inputs
    for (const tx_source_entry& src_entr : sources)
    {
      in_contexts.push_back(input_generation_context_data());
      if (!src_entr.is_multisig())
      {
        keypair& in_ephemeral = in_contexts.back().in_ephemeral;
        //txin_to_key
        if (src_entr.real_output >= src_entr.outputs.size())
        {
          LOG_ERROR("real_output index (" << src_entr.real_output << ") greater than or equal to output_keys.size()=" << src_entr.outputs.size());
          return false;
        }
        summary_inputs_money += src_entr.amount;

        //key_derivation recv_derivation;
        crypto::key_image img;
        if (!generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, in_ephemeral, img))
          return false;

        //check that derivated key is equal with real output key
        if (!(in_ephemeral.pub == src_entr.outputs[src_entr.real_output].second))
        {
          LOG_ERROR("derived public key missmatch with output public key! " << ENDL << "derived_key:"
            << string_tools::pod_to_hex(in_ephemeral.pub) << ENDL << "real output_public_key:"
            << string_tools::pod_to_hex(src_entr.outputs[src_entr.real_output].second));
          return false;
        }

        //put key image into tx input
        txin_to_key input_to_key;
        input_to_key.amount = src_entr.amount;
        input_to_key.k_image = img;

        //fill outputs array and use relative offsets
        BOOST_FOREACH(const tx_source_entry::output_entry& out_entry, src_entr.outputs)
          input_to_key.key_offsets.push_back(out_entry.first);

        input_to_key.key_offsets = absolute_output_offsets_to_relative(input_to_key.key_offsets);
        tx.vin.push_back(input_to_key);
      }
      else
      {//multisig input
        txin_multisig input_multisig = AUTO_VAL_INIT(input_multisig);
        summary_inputs_money += input_multisig.amount = src_entr.amount;
        input_multisig.multisig_out_id = src_entr.multisig_id;
        input_multisig.sigs_count = src_entr.ms_sigs_count;
        tx.vin.push_back(input_multisig);
      }
    }

    // "Shuffle" outs
    std::vector<tx_destination_entry> shuffled_dsts(destinations);
    if (shuffle)
      std::sort(shuffled_dsts.begin(), shuffled_dsts.end(), [](const tx_destination_entry& de1, const tx_destination_entry& de2) { return de1.amount < de2.amount; });

    uint64_t summary_outs_money = 0;
    //fill outputs
    size_t output_index = tx.vout.size(); // in case of append mode we need to start output indexing from the last one + 1
    std::set<uint16_t> deriv_cache;
    BOOST_FOREACH(const tx_destination_entry& dst_entr, shuffled_dsts)
    {
      CHECK_AND_ASSERT_MES(dst_entr.amount > 0, false, "Destination with wrong amount: " << dst_entr.amount);
      bool r = construct_tx_out(dst_entr, txkey.sec, output_index, tx, deriv_cache, tx_outs_attr);
      CHECK_AND_ASSERT_MES(r, false, "Failed to construc tx out");
      output_index++;
      summary_outs_money += dst_entr.amount;
    }

    //check money
    if (!(flags&TX_FLAG_SIGNATURE_MODE_SEPARATE))
    {
      if (summary_outs_money > summary_inputs_money)
      {
        LOG_ERROR("Transaction inputs money (" << summary_inputs_money << ") less than outputs money (" << summary_outs_money << ")");
        return false;
      }
    }



    //process offers and put there offers derived keys
    uint64_t att_count = 0;
    for (auto& o : tx.attachment)
    {
      if (o.type() == typeid(tx_service_attachment))
      {
        tx_service_attachment& tsa = boost::get<tx_service_attachment>(o);
        if (tsa.security.size())
        {
          CHECK_AND_ASSERT_MES(tsa.security.size() == 1, false, "Wrong tsa.security.size() = " << tsa.security.size());

          bool r = derive_public_key_from_target_address(sender_account_keys.m_account_address, one_time_secret_key, att_count, tsa.security.back());
          CHECK_AND_ASSERT_MES(r, false, "Failed to derive_public_key_from_target_address");
        }
        att_count++;
      }
    }
    if (!(flags & TX_FLAG_SIGNATURE_MODE_SEPARATE))
    {
      //take hash from attachment and put into extra
      if (tx.attachment.size())
        add_attachments_info_to_extra(tx.extra, tx.attachment);
    }
    else
    {
      // for separately signed tx each input has to contain information about corresponding outputs, extra entries and attachments
      for (size_t in_index = input_starter_index; in_index != tx.vin.size(); in_index++)
      {
        // add signed_parts to each input's details (number of outputs and extra entries)
        signed_parts so = AUTO_VAL_INIT(so);
        so.n_outs = tx.vout.size();
        so.n_extras = tx.extra.size();
        get_txin_etc_options(tx.vin[in_index]).push_back(so);
        
        // put attachment extra info to each input's details (in case there are attachments)
        add_attachments_info_to_extra(get_txin_etc_options(tx.vin[in_index]), tx.attachment);
      }
    }

    //generate ring signatures
    crypto::hash tx_prefix_hash;
    get_transaction_prefix_hash(tx, tx_prefix_hash);

    std::stringstream ss_ring_s;
    size_t input_index = input_starter_index;
    size_t in_context_index = 0;
    BOOST_FOREACH(const tx_source_entry& src_entr, sources)
    {
      crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, input_index, tx_prefix_hash);
      CHECK_AND_ASSERT_MES(tx_hash_for_signature != null_hash, false, "failed to  prepare_prefix_hash_for_sign");

      tx.signatures.push_back(std::vector<crypto::signature>());
      std::vector<crypto::signature>& sigs = tx.signatures.back();

      if (!src_entr.is_multisig())
      {
        // txin_to_key
        ss_ring_s << "input #" << input_index << ", pub_keys:" << ENDL;
        std::vector<const crypto::public_key*> keys_ptrs;
        BOOST_FOREACH(const tx_source_entry::output_entry& o, src_entr.outputs)
        {
          keys_ptrs.push_back(&o.second);
          ss_ring_s << o.second << ENDL;
        }
        sigs.resize(src_entr.outputs.size());

        crypto::generate_ring_signature(tx_hash_for_signature, boost::get<txin_to_key>(tx.vin[input_index]).k_image, keys_ptrs, in_contexts[in_context_index].in_ephemeral.sec, src_entr.real_output, sigs.data());
        ss_ring_s << "signatures:" << ENDL;
        std::for_each(sigs.begin(), sigs.end(), [&](const crypto::signature& s){ss_ring_s << s << ENDL; });
        ss_ring_s << "prefix_hash:" << tx_prefix_hash << ENDL << "in_ephemeral_key: " << in_contexts[in_context_index].in_ephemeral.sec << ENDL << "real_output: " << src_entr.real_output << ENDL;
      }
      else
      {
        // txin_multisig -- don't sign anything here (see also sign_multisig_input_in_tx())
        sigs.resize(src_entr.ms_keys_count, null_sig); // just reserve keys.size() null signatures (NOTE: not minimum_sigs!)
      }
      if (src_entr.separately_signed_tx_complete)
      {
        // if separately signed tx is complete, put one more signature to the last bunch using tx secret key, which confirms that transaction has been generated by authorized subject
        CHECK_AND_ASSERT_MES(input_index == tx.vin.size() - 1, false, "separately_signed_tx_complete flag is set for source entry #" << input_index << ", allowed only for the last one");
        CHECK_AND_ASSERT_MES(flags & TX_FLAG_SIGNATURE_MODE_SEPARATE, false, "sorce entry separately_signed_tx_complete flag is set for tx with no TX_FLAG_SIGNATURE_MODE_SEPARATE flag");
        CHECK_AND_ASSERT_MES(tx_hash_for_signature == tx_prefix_hash, false, "internal error: hash_for_sign for the last input of separately signed complete tx expected to be the same as tx prefix hash");
        sigs.resize(sigs.size() + 1);
        crypto::generate_signature(tx_prefix_hash, txkey.pub, txkey.sec, sigs.back());
      }

      input_index++;
      in_context_index++;
    }

    LOG_PRINT2("construct_tx.log", "transaction_created: " << get_transaction_hash(tx) << ENDL << obj_to_json_str(tx) << ENDL << ss_ring_s.str(), LOG_LEVEL_3);

    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_reward_from_miner_tx(const transaction& tx)
  {
    uint64_t income = 0;
    for (auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_gen))
      {
        continue;
      }
      else if (in.type() == typeid(txin_to_key))
      {
        income += boost::get<txin_to_key>(in).amount;
      }
    }
    uint64_t reward = 0;
    for (auto& out : tx.vout)
    {
      reward += out.amount;
    }
    reward -= income;
    return reward;
  }
  //---------------------------------------------------------------
  std::string get_word_from_timstamp(uint64_t timestamp)
  {
    uint64_t date_offset = timestamp > WALLET_BRAIN_DATE_OFFSET ? timestamp - WALLET_BRAIN_DATE_OFFSET : 0;
    uint64_t weeks_count = date_offset / WALLET_BRAIN_DATE_QUANTUM;
    CHECK_AND_ASSERT_THROW_MES(weeks_count < std::numeric_limits<uint32_t>::max(), "internal error: unable to converto to uint32, val = " << weeks_count);
    uint32_t weeks_count_32 = static_cast<uint32_t>(weeks_count);

    return tools::mnemonic_encoding::word_by_num(weeks_count_32);
  }
  //---------------------------------------------------------------
  uint64_t get_timstamp_from_word(std::string word)
  {
    uint64_t count_of_weeks = tools::mnemonic_encoding::num_by_word(word);
    uint64_t timestamp = count_of_weeks * WALLET_BRAIN_DATE_QUANTUM + WALLET_BRAIN_DATE_OFFSET;

    return timestamp;
  }
  //---------------------------------------------------------------
  bool sign_multisig_input_in_tx(currency::transaction& tx, size_t ms_input_index, const currency::account_keys& keys, const currency::transaction& source_tx, bool *p_is_input_fully_signed /* = nullptr */)
  {
#define LOC_CHK(cond, msg) CHECK_AND_ASSERT_MES(cond, false, msg << ", ms input index: " << ms_input_index << ", tx: " << get_transaction_hash(tx) << ", source tx: " << get_transaction_hash(source_tx))
    if (p_is_input_fully_signed != nullptr)
      *p_is_input_fully_signed = false;

    LOC_CHK(ms_input_index < tx.vin.size(), "ms input index is out of bounds, vin.size() = " << tx.vin.size());
    LOC_CHK(tx.vin[ms_input_index].type() == typeid(txin_multisig), "ms input has wrong type, txin_multisig expected");
    const txin_multisig& ms_in = boost::get<txin_multisig>(tx.vin[ms_input_index]);

    // search ms output in source tx by ms_in.multisig_out_id
    size_t ms_out_index = SIZE_MAX;
    for (size_t i = 0; i < source_tx.vout.size(); ++i)
    {
      if (source_tx.vout[i].target.type() == typeid(txout_multisig) && ms_in.multisig_out_id == get_multisig_out_id(source_tx, i))
      {
        ms_out_index = i;
        break;
      }
    }
    LOC_CHK(ms_out_index != SIZE_MAX, "failed to find ms output in source tx " << get_transaction_hash(source_tx) << " by ms id " << ms_in.multisig_out_id);
    const txout_multisig& out_ms = boost::get<txout_multisig>(source_tx.vout[ms_out_index].target);

    crypto::public_key source_tx_pub_key = get_tx_pub_key_from_extra(source_tx);

    keypair ms_in_ephemeral_key = AUTO_VAL_INIT(ms_in_ephemeral_key);
    bool r = currency::derive_ephemeral_key_helper(keys, source_tx_pub_key, ms_out_index, ms_in_ephemeral_key);
    LOC_CHK(r, "derive_ephemeral_key_helper failed");

    size_t participant_index = std::find(out_ms.keys.begin(), out_ms.keys.end(), ms_in_ephemeral_key.pub) - out_ms.keys.begin();
    LOC_CHK(participant_index < out_ms.keys.size(), "Can't find given participant's ms key in ms output keys list");
    LOC_CHK(ms_input_index < tx.signatures.size(), "transaction does not have signatures vectory entry for ms input #" << ms_input_index);

    auto& sigs = tx.signatures[ms_input_index];
    LOC_CHK(!sigs.empty(), "empty signatures container");

    bool extra_signature_expected = (get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE) && ms_input_index == tx.vin.size() - 1;
    size_t allocated_sigs_for_participants = extra_signature_expected ? sigs.size() - 1 : sigs.size();
    LOC_CHK(participant_index < allocated_sigs_for_participants, "participant index (" << participant_index << ") is out of bound: " << allocated_sigs_for_participants); // NOTE: this may fail if the input has already been fully signed and 'sigs' was compacted
  
    crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, ms_input_index, get_transaction_hash(tx));
    LOC_CHK(tx_hash_for_signature != null_hash, "failed to  prepare_prefix_hash_for_sign");

    crypto::generate_signature(tx_hash_for_signature, ms_in_ephemeral_key.pub, ms_in_ephemeral_key.sec, sigs[participant_index]);

    // check whether the input is fully signed
    size_t non_null_sigs_count = 0;
    for (size_t i = 0; i < allocated_sigs_for_participants; ++i)
    {
      if (sigs[i] != null_sig)
        ++non_null_sigs_count;
    }
    LOC_CHK(non_null_sigs_count <= out_ms.minimum_sigs, "somehow there are too many non-null signatures for this input: " << non_null_sigs_count << ", minimum_sigs: " << out_ms.minimum_sigs);
    if (non_null_sigs_count == out_ms.minimum_sigs)
    {
      // this input is fully signed, now we gonna compact sigs container by removing null signatures
      sigs.erase(std::remove(sigs.begin(), sigs.end(), null_sig), sigs.end());

      if (p_is_input_fully_signed != nullptr)
        *p_is_input_fully_signed = true;
    }

    return true;
#undef LOC_CHK
  }
  //---------------------------------------------------------------
  uint64_t get_inputs_money_amount(const transaction& tx)
  {
    uint64_t r = 0;
    get_inputs_money_amount(tx, r);
    return r;
  }
  //---------------------------------------------------------------
  bool get_inputs_money_amount(const transaction& tx, uint64_t& money)
  {
    money = 0;
    for(const auto& in : tx.vin)
    {
      uint64_t this_amount = get_amount_from_variant(in);
      if (!this_amount)
        return false;
      money += this_amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_block_height(const transaction& coinbase)
  {
    CHECK_AND_ASSERT_MES(coinbase.vin.size() == 1 || coinbase.vin.size() == 2, 0, "wrong miner tx in block: -----, b.miner_tx.vin.size() != 1");
    CHECKED_GET_SPECIFIC_VARIANT(coinbase.vin[0], const txin_gen, coinbase_in, 0);
    return coinbase_in.height;
  }
  //---------------------------------------------------------------
  uint64_t get_block_height(const block& b)
  {
    return get_block_height(b.miner_tx);
  }
  //---------------------------------------------------------------
  bool check_inputs_types_supported(const transaction& tx)
  {
    for(const auto& in : tx.vin)
    {
      CHECK_AND_ASSERT_MES(in.type() == typeid(txin_to_key) || in.type() == typeid(txin_multisig), false, "wrong variant type: "
        << in.type().name() << ", expected " << typeid(txin_to_key).name()
        << ", in transaction id=" << get_transaction_hash(tx));

    }
    return true;
  }
  //------------------------------------------------------------------
  /*
  bool add_padding_to_tx(transaction& tx, size_t count)
  {
    
    WARNING: potantially unsafe implementation!
    1) requires extra_padding being previously added to tx's extra;
    2) transaction size may increase by more than 'count' bytes due to varint encoding (thus, if 'count' is 128 it will add 129 bytes)
    See also check_add_padding_to_tx test (uncomment and update it, if necessary).

    if (!count)
      return true;

    for (auto& ex : tx.extra)
    {
      if (ex.type() == typeid(extra_padding))
      {
        boost::get<extra_padding>(ex).buff.insert(boost::get<extra_padding>(ex).buff.end(), count, 0);
        return true;
      }
    }
    CHECK_AND_ASSERT_THROW_MES(false, "extra_padding entry not found in template mining transaction");
    return false;
  }
  */
  //------------------------------------------------------------------
  bool remove_padding_from_tx(transaction& tx, size_t count)
  {
    if (!count)
      return true;

    for (auto ex : tx.extra)
    {
      if (ex.type() == typeid(extra_padding))
      {
        std::vector<uint8_t>& buff = boost::get<extra_padding>(ex).buff;
        CHECK_AND_ASSERT_MES(buff.size() >= count, false, "Attempt to remove_padding_from_tx for count = " << count << ", while buff.size()=" << buff.size());
        buff.resize(buff.size() - count);
        return true;
      }
    }
    CHECK_AND_ASSERT_THROW_MES(false, "extra_padding entry not found in template mining transaction");
    return false;
  }
  //------------------------------------------------------------------
  bool is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t current_blockchain_size, uint64_t current_time)
  {
    if (unlock_time < CURRENCY_MAX_BLOCK_NUMBER)
    {
      //interpret as block index
      return unlock_time <= current_blockchain_size - 1 + CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS;
    }

    //interpret as time
    return unlock_time <= current_time + CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_outs_valid(const transaction& tx)
  {
    for(const tx_out& out : tx.vout)
    {
      CHECK_AND_NO_ASSERT_MES(0 < out.amount, false, "zero amount output in transaction id=" << get_transaction_hash(tx));
      if (out.target.type() == typeid(txout_to_key))
      {
        if (!check_key(boost::get<txout_to_key>(out.target).key))
          return false;
      }
      else if (out.target.type() == typeid(txout_multisig))
      {
        const txout_multisig& ms = boost::get<txout_multisig>(out.target);
        if (!(ms.keys.size() > 0 && ms.minimum_sigs > 0 && ms.minimum_sigs <= ms.keys.size()))
        {
          LOG_ERROR("wrong multisig in transaction id=" << get_transaction_hash(tx));
          return false;
        }
      }
      else
      {
        LOG_ERROR("wrong variant type: " << out.target.type().name() << ", expected " << typeid(txout_to_key).name()
          << ", in transaction id=" << get_transaction_hash(tx));
      }
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_money_overflow(const transaction& tx)
  {
    return check_inputs_overflow(tx) && check_outs_overflow(tx);
  }
  //---------------------------------------------------------------
  bool check_inputs_overflow(const transaction& tx)
  {
    uint64_t money = 0;
    BOOST_FOREACH(const auto& in, tx.vin)
    {
      uint64_t this_amount = 0;
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
        this_amount = tokey_in.amount;
      }
      else if (in.type() == typeid(txin_multisig))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_multisig, tokey_in, false);
        this_amount = tokey_in.amount;
      }
      else
      {
        LOG_ERROR("Unknow type in in: " << in.type().name());
        return false;
      }
      if (money > this_amount + money)
        return false;
      money += this_amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool check_outs_overflow(const transaction& tx)
  {
    uint64_t money = 0;
    BOOST_FOREACH(const auto& o, tx.vout)
    {
      if (money > o.amount + money)
        return false;
      money += o.amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_outs_money_amount(const transaction& tx)
  {
    uint64_t outputs_amount = 0;
    for(const auto& o : tx.vout)
      outputs_amount += o.amount;
    return outputs_amount;
  }
  //---------------------------------------------------------------
  std::string short_hash_str(const crypto::hash& h)
  {
    std::string res = string_tools::pod_to_hex(h);
    CHECK_AND_ASSERT_MES(res.size() == 64, res, "wrong hash256 with string_tools::pod_to_hex conversion");
    auto erased_pos = res.erase(8, 48);
    res.insert(8, "....");
    return res;
  }
  //---------------------------------------------------------------
  bool is_out_to_acc(const account_keys& acc, const txout_to_key& out_key, const crypto::key_derivation& derivation, size_t output_index)
  {
    crypto::public_key pk;
    derive_public_key(derivation, output_index, acc.m_account_address.m_spend_public_key, pk);
    return pk == out_key.key;
  }
  //---------------------------------------------------------------
  bool is_out_to_acc(const account_keys& acc, const txout_multisig& out_multisig, const crypto::key_derivation& derivation, size_t output_index)
  {
    crypto::public_key pk;
    derive_public_key(derivation, output_index, acc.m_account_address.m_spend_public_key, pk);
    auto it = std::find(out_multisig.keys.begin(), out_multisig.keys.end(), pk);
    if (out_multisig.keys.end() == it)
      return false;
    return true;
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, std::vector<size_t>& outs, uint64_t& money_transfered, crypto::key_derivation& derivation)
  {
    crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);
    if (null_pkey == tx_pub_key)
      return false;
    return lookup_acc_outs(acc, tx, get_tx_pub_key_from_extra(tx), outs, money_transfered, derivation);
  }
  //---------------------------------------------------------------
  bool check_tx_derivation_hint(const transaction& tx, const crypto::key_derivation& derivation)
  {
    bool found_der_xor = false;
    uint16_t my_derive_xor = get_derivation_hint(derivation);
    tx_derivation_hint dh = make_tx_derivation_hint_from_uint16(my_derive_xor);
    for (auto& e : tx.extra)
    {
      if (e.type() == typeid(tx_derivation_hint))
      {
        const tx_derivation_hint& tdh = boost::get<tx_derivation_hint>(e);
        if (tdh.msg.size() == sizeof(uint16_t))
        {
          found_der_xor = true;
          if (dh.msg == tdh.msg)
            return true;
        }
      }
    }
    //if tx doesn't have any hints - feature is not supported, use full scan
    if (!found_der_xor)
      return true;

    return false;
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs_genesis(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, std::vector<size_t>& outs, uint64_t& money_transfered, crypto::key_derivation& derivation)
  {
    uint64_t offset = 0;
    bool r = get_account_genesis_offset_by_address(get_account_address_as_str(acc.m_account_address), offset);
    if (!r)
      return true;

    CHECK_AND_ASSERT_MES(offset < tx.vout.size(), false, "condition failed: offset(" << offset << ") < tx.vout.size() (" << tx.vout.size() << ")");
    auto& o = tx.vout[offset];
    CHECK_AND_ASSERT_MES(o.target.type() == typeid(txout_to_key), false, "condition failed: o.target.type() == typeid(txout_to_key)");
    if (is_out_to_acc(acc, boost::get<txout_to_key>(o.target), derivation, offset))
    {
      outs.push_back(offset);
      money_transfered += o.amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, std::vector<size_t>& outs, uint64_t& money_transfered, crypto::key_derivation& derivation)
  {
    money_transfered = 0;
    generate_key_derivation(tx_pub_key, acc.m_view_secret_key, derivation);

    if (is_coinbase(tx) && get_block_height(tx) == 0 &&  tx_pub_key == ggenesis_tx_pub_key)
    {
      //genesis coinbase
      return lookup_acc_outs_genesis(acc, tx, tx_pub_key, outs, money_transfered, derivation);
    }

    
    if (!check_tx_derivation_hint(tx, derivation))
      return true;

    size_t i = 0;
    BOOST_FOREACH(const tx_out& o, tx.vout)
    {
      if (o.target.type() == typeid(txout_to_key))
      {
        if (is_out_to_acc(acc, boost::get<txout_to_key>(o.target), derivation, i))
        {
          outs.push_back(i);
          money_transfered += o.amount;
        }
      }
      else if (o.target.type() == typeid(txout_multisig))
      {
        if (is_out_to_acc(acc, boost::get<txout_multisig>(o.target), derivation, i))
        {
          outs.push_back(i);
          //don't count this money
        }
      }
      else
      {
        LOG_ERROR("Wrong type at lookup_acc_outs, unexpected type is: " << o.target.type().name());
        return false;
      }
      i++;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool set_payment_id_to_tx(std::vector<attachment_v>& att, const std::string& payment_id)
  {
    if (!is_payment_id_size_ok(payment_id))
      return false;

    tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
    tsa.service_id = BC_PAYMENT_ID_SERVICE_ID;
    tsa.body = payment_id;
    att.push_back(tsa);
    return true;
  }



  //---------------------------------------------------------------
  std::string print_money_brief(uint64_t amount)
  {
    uint64_t remainder = amount % COIN;
    amount /= COIN;
    if (remainder == 0)
      return std::to_string(amount) + ".0";
    std::string r = std::to_string(remainder);
    if (r.size() < CURRENCY_DISPLAY_DECIMAL_POINT)
      r.insert(0, CURRENCY_DISPLAY_DECIMAL_POINT - r.size(), '0');
    return std::to_string(amount) + '.' + r.substr(0, r.find_last_not_of('0') + 1);
  }
  //---------------------------------------------------------------
  /*bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t& blob_size)
  {


  return get_object_hash(t, res, blob_size);
  }*/
  //------------------------------------------------------------------
  template<typename pod_operand_a, typename pod_operand_b>
  crypto::hash hash_together(const pod_operand_a& a, const pod_operand_b& b)
  {
    std::string blob;
    string_tools::apped_pod_to_strbuff(blob, a);
    string_tools::apped_pod_to_strbuff(blob, b);
    return crypto::cn_fast_hash(blob.data(), blob.size());
  }
  //------------------------------------------------------------------
  //--------------------------------------------------------------
  bool unserialize_block_complete_entry(const currency::COMMAND_RPC_GET_BLOCKS_FAST::response& serialized,
    currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& unserialized)
  {
    for (const auto& bl_entry : serialized.blocks)
    {
      unserialized.blocks.push_back(block_direct_data_entry());
      block_direct_data_entry& bdde = unserialized.blocks.back();
      auto blextin_ptr = std::make_shared<currency::block_extended_info>();
      bool r = currency::parse_and_validate_block_from_blob(bl_entry.block, blextin_ptr->bl);
      bdde.block_ptr = blextin_ptr;
      CHECK_AND_ASSERT_MES(r, false, "failed to parse block from blob: " << string_tools::buff_to_hex_nodelimer(bl_entry.block));
      for (const auto& tx_blob : bl_entry.txs)
      {
        std::shared_ptr<currency::transaction_chain_entry> tche_ptr(new currency::transaction_chain_entry());
        r = parse_and_validate_tx_from_blob(tx_blob, tche_ptr->tx);
        CHECK_AND_ASSERT_MES(r, false, "failed to parse tx from blob: " << string_tools::buff_to_hex_nodelimer(tx_blob));
        bdde.txs_ptr.push_back(tche_ptr);
      }
    }
    return true;
  }

  //---------------------------------------------------------------
  uint64_t get_alias_coast_from_fee(const std::string& alias, uint64_t median_fee)
  {
    return median_fee * 10;
  }
  //---------------------------------------------------------------
  uint64_t get_actual_timestamp(const block& b)
  {
    uint64_t tes_ts = b.timestamp;
    if (is_pos_block(b))
    {
      etc_tx_time t = AUTO_VAL_INIT(t);
      if(get_type_in_variant_container(b.miner_tx.extra, t))
        tes_ts = t.v;
    }
    return tes_ts;
  }
  //------------------------------------------------------------------
  bool validate_alias_name(const std::string& al)
  {
    CHECK_AND_ASSERT_MES(al.size() <= ALIAS_NAME_MAX_LEN, false, "Too long alias name, please use name no longer than " << ALIAS_NAME_MAX_LEN);
    /*allowed symbols "0-9", "a-z", "-", "." */
    static bool alphabet[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    for (const auto ch : al)
    {
      CHECK_AND_ASSERT_MES(alphabet[static_cast<unsigned char>(ch)], false, "Wrong character in alias: '" << ch << "'");
    }
    return true;
  }


  //------------------------------------------------------------------
#define ANTI_OVERFLOW_AMOUNT       1000000
#define GET_PERECENTS_BIG_NUMBERS(per, total) (per/ANTI_OVERFLOW_AMOUNT)*100 / (total/ANTI_OVERFLOW_AMOUNT) 

  void print_reward_change()
  {
    std::cout << std::endl << "Reward change for 10 years:" << std::endl;
    std::cout << std::setw(10) << std::left << "day" << std::setw(19) << "block reward" << std::setw(19) << "generated coins" << std::endl;

    boost::multiprecision::uint128_t already_generated_coins = PREMINE_AMOUNT;
    boost::multiprecision::uint128_t money_was_at_begining_of_year = already_generated_coins;
    boost::multiprecision::uint128_t total_generated_in_year_by_pos = 0;
    boost::multiprecision::uint128_t total_generated_in_year_by_pow = 0;
    //uint64_t total_money_supply = TOTAL_MONEY_SUPPLY;
    uint64_t h = 0;
    for (uint64_t day = 0; day != 365 * 10; ++day)
    {
      if (!(day % 366))
      {
        uint64_t emission_reward = 0;
        get_block_reward(h % 2, 0, 0, already_generated_coins, emission_reward, h);

        std::cout << std::left
          << std::setw(10) << day
          << std::setw(19) << print_money(emission_reward)
          << std::setw(4) << print_money(already_generated_coins) 
          << "(POS: " << boost::lexical_cast<std::string>(GET_PERECENTS_BIG_NUMBERS(total_generated_in_year_by_pos, money_was_at_begining_of_year)) << "%"
          << ", POW: " << boost::lexical_cast<std::string>(GET_PERECENTS_BIG_NUMBERS(total_generated_in_year_by_pow, money_was_at_begining_of_year)) << "%)"

          << std::setw(19) << ",PoS coins/year: " << print_money(total_generated_in_year_by_pos)
          << std::setw(19) << ",PoW coins/year:" << print_money(total_generated_in_year_by_pow)


          << std::endl;


          total_generated_in_year_by_pos = total_generated_in_year_by_pow = 0;
          money_was_at_begining_of_year = already_generated_coins;

      }


      for (size_t i = 0; i != CURRENCY_BLOCKS_PER_DAY; i++)
      {
        uint64_t emission_reward = 0;
        h++;
        get_block_reward(h % 2, 0, 0, already_generated_coins, emission_reward, h);
        already_generated_coins += emission_reward;
        if (h % 2)
          total_generated_in_year_by_pos += emission_reward;
        else
          total_generated_in_year_by_pow += emission_reward;

      }
    }
  }
  void print_reward_change_short()
  {
//     std::cout << std::endl << "Reward change for 20 days:" << std::endl;
//     std::cout << std::setw(10) << std::left << "day" << std::setw(19) << "block reward" << std::setw(19) << "generated coins" << std::endl;
// 
//     const boost::multiprecision::uint128_t& already_generated_coins = PREMINE_AMOUNT;
//     //uint64_t total_money_supply = TOTAL_MONEY_SUPPLY;
//     uint64_t h = 0;
//     for (uint64_t day = 0; day != 20; ++day)
//     {
//       uint64_t emission_reward = 0;
//       get_block_reward(h % 2, 0, 0, already_generated_coins, emission_reward, h, (already_generated_coins - PREMINE_AMOUNT) * 140);
// 
//       std::cout << std::left
//         << std::setw(10) << day
//         << std::setw(19) << print_money(emission_reward)
//         << std::setw(4) << print_money(already_generated_coins)//std::string(std::to_string(GET_PERECENTS_BIG_NUMBERS((already_generated_coins), total_money_supply)) + "%")
//         << std::endl;
// 
// 
// 
//       for (size_t i = 0; i != 720; i++)
//       {
//         h++;
//         get_block_reward(h % 2, 0, 0, already_generated_coins, emission_reward, h, (already_generated_coins - PREMINE_AMOUNT) * 140);
//         already_generated_coins += emission_reward;
//         if (h < POS_START_HEIGHT && i > 360)
//           break;
//       }
//     }
  }
  
  std::string print_reward_change_first_blocks(size_t n_of_first_blocks)
  {
    std::stringstream ss;
    ss << std::endl << "Reward change for the first " << n_of_first_blocks << " blocks:" << std::endl;
    ss << std::setw(10) << std::left << "block #" << std::setw(20) << "block reward" << std::setw(20) << "generated coins" << std::setw(8) << "type" << std::endl;

    boost::multiprecision::uint128_t already_generated_coins = 0;
    uint64_t total_generated_pos = 0;
    uint64_t total_generated_pow = 0;

    for (size_t h = 0; h != n_of_first_blocks; ++h)
    {
      bool is_pos = h < 10 ? false : h % 2 == 0;
      uint64_t emission_reward = 0;

      if (h == 0)
      {
        emission_reward = PREMINE_AMOUNT;
      }
      else
      {
        currency::get_block_reward(is_pos, 0, 0, already_generated_coins, emission_reward, h);
        (is_pos ? total_generated_pos : total_generated_pow) += emission_reward;
      }

      already_generated_coins += emission_reward;

      ss << std::left
        << std::setw(10) << h
        << std::setw(20) << currency::print_money(emission_reward)
        << std::setw(20) << currency::print_money(already_generated_coins)
        << std::setw(8) << (h == 0 ? "genesis" : (is_pos ? "PoS" : "PoW"))
        << std::endl;
    }
    ss << "total generated PoW: " << currency::print_money(total_generated_pow) << std::endl;
    ss << "total generated PoS: " << currency::print_money(total_generated_pos) << std::endl;
    return ss.str();
  }
  //------------------------------------------------------------------
  void print_currency_details()
  {
    //for future forks 

    std::cout << "Currency name: \t\t" << CURRENCY_NAME << "(" << CURRENCY_NAME_SHORT << ")" << std::endl;
    std::cout << "Money supply: \t\t " << CURRENCY_BLOCK_REWARD * CURRENCY_BLOCKS_PER_DAY * 365 << " coins per year" << std::endl;
    std::cout << "PoS block interval: \t" << DIFFICULTY_POS_TARGET << " seconds" << std::endl;
    std::cout << "PoW block interval: \t" << DIFFICULTY_POW_TARGET << " seconds" << std::endl;
    std::cout << "Total blocks per day: \t" << CURRENCY_BLOCKS_PER_DAY << " seconds" << std::endl;
    std::cout << "Default p2p port: \t" << P2P_DEFAULT_PORT << std::endl;
    std::cout << "Default rpc port: \t" << RPC_DEFAULT_PORT << std::endl;
#ifndef TEST_FAST_EMISSION_CURVE
    print_reward_change();
#else
    print_reward_change_short();
#endif
  }
  //------------------------------------------------------------------
  std::string dump_patch(const std::map<uint64_t, crypto::hash>& patch)
  {
    std::stringstream ss;
    for (auto& p : patch)
    {
      ss << "[" << p.first << "]" << p.second << ENDL;
    }
    return ss.str();
  }
  //---------------------------------------------------------------
  bool generate_genesis_block(block& bl)
  {
    //genesis block
    bl = boost::value_initialized<block>();

#ifndef TESTNET
//    std::string genesis_coinbase_tx_hex((const char*)&ggenesis_tx_raw, sizeof(ggenesis_tx_raw));

#else 
    std::string genesis_coinbase_tx_hex = "";
#endif

    //genesis proof phrase: "Liverpool beat Barcelona: Greatest Champions League comebacks of all time"
    //taken from: https://www.bbc.com/sport/football/48163330
    //sha3-256 from proof phrase:  a074236b1354901d5dbc029c0ac4c05c948182c34f3030f32b0c93aee7ba275c (included in genesis block)


    blobdata tx_bl((const char*)&ggenesis_tx_raw, sizeof(ggenesis_tx_raw));
    //string_tools::parse_hexstr_to_binbuff(genesis_coinbase_tx_hex, tx_bl);
    bool r = parse_and_validate_tx_from_blob(tx_bl, bl.miner_tx);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse coinbase tx from hard coded blob");
    bl.major_version = BLOCK_MAJOR_VERSION_GENESIS;
    bl.minor_version = BLOCK_MINOR_VERSION_GENESIS;
    bl.timestamp = 0;
    bl.nonce = CURRENCY_GENESIS_NONCE;
    LOG_PRINT_GREEN("Generated genesis: " << get_block_hash(bl), LOG_LEVEL_0);
    return true;
  }
  //----------------------------------------------------------------------------------------------------
  const crypto::hash& get_genesis_hash(bool need_to_set, const crypto::hash& h)
  {
    static crypto::hash genesis_id = null_hash;
    if (genesis_id == null_hash && !need_to_set)
    {
      LOG_PRINT_GREEN("Calculating genesis....", LOG_LEVEL_0);
      block b = AUTO_VAL_INIT(b);
      bool r = generate_genesis_block(b);
      CHECK_AND_ASSERT_MES(r, null_hash, "failed to generate genesis block");
      genesis_id = get_block_hash(b);
    }

    if (need_to_set)
    {
      genesis_id = h;
    }
    return genesis_id;
  }
  //---------------------------------------------------------------
  std::vector<txout_v> relative_output_offsets_to_absolute(const std::vector<txout_v>& off)
  {
    //if array has both types of outs, then global index (uint64_t) should be first, and then the rest could be out_by_id

    std::vector<txout_v> res = off;
    for (size_t i = 1; i < res.size(); i++)
    {
      if (res[i].type() == typeid(ref_by_id))
        break;
      boost::get<uint64_t>(res[i]) += boost::get<uint64_t>(res[i - 1]);
    }

    return res;
  }
  //---------------------------------------------------------------
  std::vector<txout_v> absolute_output_offsets_to_relative(const std::vector<txout_v>& off)
  {
    std::vector<txout_v> res = off;
    if (off.size() < 2)
      return res;

    std::sort(res.begin(), res.end(), [](const txout_v& lft, const txout_v& rght)
    {
      if (lft.type() == typeid(uint64_t))
      {
        if (rght.type() == typeid(uint64_t))
          return boost::get<uint64_t>(lft) < boost::get<uint64_t>(rght);
        else if (rght.type() == typeid(ref_by_id))
          return true;
        else
          LOG_ERROR("Unknown type in txout_v");
      }
      else if (lft.type() == typeid(ref_by_id))
      {
        if (rght.type() == typeid(uint64_t))
          return false;
        else if (rght.type() == typeid(ref_by_id))
          return false; // don't change the order of ref_by_id elements
        else
          LOG_ERROR("Unknown type in txout_v");
      }
      return false;
    });//just to be sure, actually it is already should be sorted

    //find starter index - skip ref_by_id entries
    size_t i = res.size() - 1;
    while (i != 0 && res[i].type() == typeid(ref_by_id))
      --i;

    for (; i != 0; i--)
    {
      boost::get<uint64_t>(res[i]) -= boost::get<uint64_t>(res[i - 1]);
    }


    return res;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_block_from_blob(const blobdata& b_blob, block& b)
  {
    return parse_and_validate_object_from_blob(b_blob, b);
  }

  //---------------------------------------------------------------
  bool is_service_tx(const transaction& tx)
  {
    auto tx_type = get_tx_type(tx);
    if (GUI_TX_TYPE_NORMAL == tx_type ||
      GUI_TX_TYPE_COIN_BASE == tx_type)
      return false;

    return true;
  }
  //---------------------------------------------------------------
  bool is_showing_sender_addres(const transaction& tx)
  {
    return have_type_in_variant_container<tx_payer>(tx.attachment);
  }
  //---------------------------------------------------------------
  bool is_mixin_tx(const transaction& tx)
  {
    for (const auto& e : tx.vin)
    {
      if (e.type() != typeid(txin_to_key))
        return false;
      if (boost::get<txin_to_key>(e).key_offsets.size() < 2)
        return false;
    }

    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_amount_for_zero_pubkeys(const transaction& tx)
  {
    uint64_t found_alias_reward = 0;
    for (const auto& out : tx.vout)
    {
      if (out.target.type() != typeid(txout_to_key))
        continue;

      const txout_to_key& o = boost::get<txout_to_key>(out.target);
      if (o.key == null_pkey)
        found_alias_reward += out.amount;
    }
    return found_alias_reward;
  }

  //---------------------------------------------------------------
  bool get_aliases_reward_account(account_public_address& acc)
  {
    bool r = string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_SPEND_PUB_KEY, acc.m_spend_public_key);
    r &= string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_VIEW_PUB_KEY, acc.m_view_public_key);
    return r;
  }
  //------------------------------------------------------------------
  size_t get_service_attachments_count_in_tx(const transaction& tx)
  {
    size_t cnt = 0;
    for (const auto& at : tx.attachment)
    {
      if (at.type() == typeid(tx_service_attachment))
        ++cnt;
    }
    return cnt;
  }

  struct rpc_tx_payload_handler : public boost::static_visitor<bool>
  {
    tx_extra_rpc_entry& tv;
    rpc_tx_payload_handler(tx_extra_rpc_entry& t) : tv(t)
    {}

    bool operator()(const tx_service_attachment& ee)
    {
      tv.type = "service";
      tv.short_view = ee.service_id + ":" + ee.instruction;
      if (ee.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY)
        tv.short_view += "(encrypted)";
      else
      {
        std::string deflated_buff;
        const std::string* pfinalbuff = &ee.body;
        if (ee.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY)
        {
          bool r = epee::zlib_helper::unpack(ee.body, deflated_buff);
          CHECK_AND_ASSERT_MES(r, false, "Failed to unpack");
          pfinalbuff = &deflated_buff;
        }
        if (ee.service_id == BC_PAYMENT_ID_SERVICE_ID || ee.service_id == BC_OFFERS_SERVICE_ID)
          tv.datails_view = *pfinalbuff;
        else
          tv.datails_view = "BINARY DATA";
      }
      return true;
    }
    bool operator()(const tx_crypto_checksum& ee)
    {
      tv.type = "crypto_checksum";
      tv.short_view = std::string("derivation_hash: ") + epee::string_tools::pod_to_hex(ee.derivation_hash);
      tv.datails_view = std::string("derivation_hash: ") + epee::string_tools::pod_to_hex(ee.derivation_hash) + "\n"
        + "encrypted_key_derivation: " + epee::string_tools::pod_to_hex(ee.encrypted_key_derivation);

      return true;
    }
    bool operator()(const etc_tx_time& ee)
    {
      tv.type = "pos_time";
      tv.short_view = std::string("timestamp: ") + std::to_string(ee.v) + " " + epee::misc_utils::get_internet_time_str(ee.v);
      return true;
    }
    bool operator()(const etc_tx_details_unlock_time& ee)
    {
      tv.type = "unlock_time";
      if (ee.v < CURRENCY_MAX_BLOCK_NUMBER)
        tv.short_view = std::string("height: ") + std::to_string(ee.v);
      else
        tv.short_view = std::string("timestamp: ") + std::to_string(ee.v) + " " + epee::misc_utils::get_internet_time_str(ee.v);

      return true;
    }
    bool operator()(const etc_tx_details_unlock_time2& ee)
    {
      tv.type = "unlock_time";
      std::stringstream ss;
      ss << "[";
      for (auto v : ee.unlock_time_array)
      {
        ss << " " << v;
      }
      ss << "]";
      tv.short_view = ss.str();

      return true;
    }
    bool operator()(const etc_tx_details_expiration_time& ee)
    {
      tv.type = "expiration_time";
      if (ee.v < CURRENCY_MAX_BLOCK_NUMBER)
        tv.short_view = std::string("height: ") + std::to_string(ee.v);
      else
        tv.short_view = std::string("timestamp: ") + std::to_string(ee.v) + " " + epee::misc_utils::get_internet_time_str(ee.v);

      return true;
    }
    bool operator()(const etc_tx_details_flags& ee)
    {
      tv.type = "details_flags";
      tv.short_view = epee::string_tools::pod_to_hex(ee.v);
      return true;
    }
    bool operator()(const crypto::public_key& ee)
    {
      tv.type = "pub_key";
      tv.short_view = epee::string_tools::pod_to_hex(ee);
      return true;
    }
    bool operator()(const extra_attachment_info& ee)
    {
      tv.type = "attachment_info";
      tv.short_view = std::to_string(ee.sz) + " bytes";
      tv.datails_view = currency::obj_to_json_str(ee);
      return true;
    }
    bool operator()(const extra_alias_entry& ee)
    {
      tv.type = "alias_info";
      tv.short_view = ee.m_alias + "-->" + get_account_address_as_str(ee.m_address);
      tv.datails_view = currency::obj_to_json_str(ee);

      return true;
    }
    bool operator()(const extra_user_data& ee)
    {
      tv.type = "user_data";
      tv.short_view = std::to_string(ee.buff.size()) + " bytes";
      tv.datails_view = epee::string_tools::buff_to_hex_nodelimer(ee.buff);

      return true;
    }
    bool operator()(const extra_padding& ee)
    {
      tv.type = "extra_padding";
      tv.short_view = std::to_string(ee.buff.size()) + " bytes";
      if (!ee.buff.empty())
        tv.datails_view = epee::string_tools::buff_to_hex_nodelimer(std::string(reinterpret_cast<const char*>(&ee.buff[0]), ee.buff.size()));

      return true;
    }
    bool operator()(const tx_comment& ee)
    {
      tv.type = "comment";
      tv.short_view = std::to_string(ee.comment.size()) + " bytes(encrypted)";
      tv.datails_view = epee::string_tools::buff_to_hex_nodelimer(ee.comment);

      return true;
    }
    bool operator()(const tx_payer& ee)
    {
      //const tx_payer& ee = boost::get<tx_payer>(extra);
      tv.type = "payer";
      tv.short_view = "(encrypted)";

      return true;
    }

    bool operator()(const tx_receiver& ee)
    {
      //const tx_payer& ee = boost::get<tx_payer>(extra);
      tv.type = "receiver";
      tv.short_view = "(encrypted)";

      return true;
    }
    bool operator()(const tx_derivation_hint& ee)
    {
      tv.type = "derivation_hint";
      tv.short_view = std::to_string(ee.msg.size()) + " bytes";
      tv.datails_view = epee::string_tools::buff_to_hex_nodelimer(ee.msg);

      return true;
    }
    bool operator()(const std::string& ee)
    {
      tv.type = "string";
      tv.short_view = std::to_string(ee.size()) + " bytes";
      tv.datails_view = epee::string_tools::buff_to_hex_nodelimer(ee);

      return true;
    }
    bool operator()(const etc_tx_uint16_t& dh)
    {
      tv.type = "XOR";
      tv.short_view = epee::string_tools::pod_to_hex(dh);
      tv.datails_view = epee::string_tools::pod_to_hex(dh);

      return true;
    }

  };


  //------------------------------------------------------------------
  template<class t_container>
  bool fill_tx_rpc_payload_items(std::vector<tx_extra_rpc_entry>& target_vector, const t_container& tc)
  {
    //handle extra
    for (auto& extra : tc)
    {
      target_vector.push_back(tx_extra_rpc_entry());
      tx_extra_rpc_entry& tv = target_vector.back();

      rpc_tx_payload_handler vstr(tv);
      boost::apply_visitor(vstr, extra);
    }
    return true;
  }
  //------------------------------------------------------------------
  bool fill_tx_rpc_outputs(tx_rpc_extended_info& tei, const transaction& tx, const transaction_chain_entry* ptce)
  {
    size_t i = 0;
    for (auto& out : tx.vout)
    {
      tei.outs.push_back(tx_out_rpc_entry());
      tei.outs.back().amount = out.amount;
      tei.outs.back().is_spent = ptce ? ptce->m_spent_flags[i] : false;
      tei.outs.back().global_index = ptce ? ptce->m_global_output_indexes[i] : 0;

      if (out.target.type() == typeid(txout_to_key))
      {
        const txout_to_key& otk = boost::get<txout_to_key>(out.target);
        tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(otk.key));
        if (otk.mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
          tei.outs.back().pub_keys.back() += "(FORCED_NO_MIX)";
        if (otk.mix_attr >= CURRENCY_TO_KEY_OUT_FORCED_MIX_LOWER_BOUND)
          tei.outs.back().pub_keys.back() += std::string("(FORCED_MIX_LOWER_BOUND: ") + std::to_string(otk.mix_attr) + ")";
      }
      else if (out.target.type() == typeid(txout_multisig))
      {
        const txout_multisig& otm = boost::get<txout_multisig>(out.target);
        for (auto& k : otm.keys)
        {
          tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(k));
        }
        tei.outs.back().minimum_sigs = otm.minimum_sigs;
      }

      ++i;
    }
    return true;
  }
  //------------------------------------------------------------------
  bool fill_tx_rpc_inputs(tx_rpc_extended_info& tei, const transaction& tx)
  {
    //handle inputs
    for (auto in : tx.vin)
    {
      tei.ins.push_back(tx_in_rpc_entry());
      if (in.type() == typeid(txin_gen))
      {
        tei.ins.back().amount = 0;
      }
      else if (in.type() == typeid(txin_to_key))
      {
        txin_to_key& tk = boost::get<txin_to_key>(in);
        tei.ins.back().amount = tk.amount;
        tei.ins.back().kimage_or_ms_id = epee::string_tools::pod_to_hex(tk.k_image);
        std::vector<txout_v> absolute_offsets = relative_output_offsets_to_absolute(tk.key_offsets);
        for (auto& ao : absolute_offsets)
        {
          tei.ins.back().global_indexes.push_back(0);
          if (ao.type() == typeid(uint64_t))
          {
            tei.ins.back().global_indexes.back() = boost::get<uint64_t>(ao);
          }
          else// if (ao.type() == typeid(ref_by_id))
          {
            //disable for the reset at the moment 
            tei.ins.back().global_indexes.back() = std::numeric_limits<uint64_t>::max();
          }
        }
        //tk.etc_details -> visualize it may be later
      }
      else if (in.type() == typeid(txin_multisig))
      {
        txin_multisig& tms = boost::get<txin_multisig>(in);
        tei.ins.back().amount = tms.amount;
        tei.ins.back().kimage_or_ms_id = epee::string_tools::pod_to_hex(tms.multisig_out_id);
        if (tx.signatures.size() >= tei.ins.size())
          tei.ins.back().multisig_count = tx.signatures[tei.ins.size() - 1].size();
      }
    }
    return true;
  }
  bool fill_block_rpc_details(block_rpc_extended_info& pei_rpc, const block_extended_info& bei_chain, const crypto::hash& h)
  {
    pei_rpc.difficulty = bei_chain.difficulty.convert_to<std::string>();
    pei_rpc.cumulative_diff_adjusted = bei_chain.cumulative_diff_adjusted.convert_to<std::string>();
    pei_rpc.cumulative_diff_precise = bei_chain.cumulative_diff_precise.convert_to<std::string>();
    pei_rpc.block_cumulative_size = bei_chain.block_cumulative_size;
    pei_rpc.timestamp = bei_chain.bl.timestamp;
    pei_rpc.id = epee::string_tools::pod_to_hex(h);
    pei_rpc.prev_id = epee::string_tools::pod_to_hex(bei_chain.bl.prev_id);
    pei_rpc.actual_timestamp = get_actual_timestamp(bei_chain.bl);
    pei_rpc.type = is_pos_block(bei_chain.bl) ? 0 : 1;
    pei_rpc.already_generated_coins = boost::lexical_cast<std::string>(bei_chain.already_generated_coins);
    pei_rpc.this_block_fee_median = bei_chain.this_block_tx_fee_median;
    pei_rpc.effective_fee_median = bei_chain.effective_tx_fee_median;
    pei_rpc.height = bei_chain.height;
    pei_rpc.object_in_json = currency::obj_to_json_str(bei_chain.bl);

    extra_user_data eud = AUTO_VAL_INIT(eud);
    if (get_type_in_variant_container(bei_chain.bl.miner_tx.extra, eud))
    {
      pei_rpc.miner_text_info = eud.buff;
    }

    pei_rpc.base_reward = get_base_block_reward(is_pos_block(bei_chain.bl), bei_chain.already_generated_coins, bei_chain.height);
    pei_rpc.summary_reward = get_reward_from_miner_tx(bei_chain.bl.miner_tx);
    pei_rpc.penalty = (pei_rpc.base_reward + pei_rpc.total_fee) - pei_rpc.summary_reward;
    return true;
  }
  //---------------------------------------------------------------
  bool fill_tx_rpc_details(tx_rpc_extended_info& tei, const transaction& tx, const transaction_chain_entry* ptce, const crypto::hash& h, uint64_t timestamp, bool is_short)
  {
    //tei.blob = tx_ptr->tx
    tei.id = epee::string_tools::pod_to_hex(h);
    if(!tei.blob_size)
      tei.blob_size = get_object_blobsize(tx);

    tei.fee = get_tx_fee(tx);
    tei.pub_key = epee::string_tools::pod_to_hex(get_tx_pub_key_from_extra(tx));
    tei.timestamp = timestamp;
    tei.amount = get_outs_money_amount(tx);

    if (is_short)
      return true;

    fill_tx_rpc_inputs(tei, tx);
    fill_tx_rpc_outputs(tei, tx, ptce);
    fill_tx_rpc_payload_items(tei.extra, tx.extra);
    fill_tx_rpc_payload_items(tei.attachments, tx.attachment);
    return true;
  }
  //------------------------------------------------------------------
  void append_per_block_increments_for_tx(const transaction& tx, std::unordered_map<uint64_t, uint32_t>& gindices)
  {
    for (size_t n = 0; n < tx.vout.size(); ++n)
    {
      if (tx.vout[n].target.type() == typeid(txout_to_key))
      {
        uint64_t amount = tx.vout[n].amount;
        gindices[amount] += 1;
      }
    }
  }
  //---------------------------------------------------------------
  bool get_aliases_reward_account(account_public_address& acc, crypto::secret_key& acc_view_key)
  {
    bool r = get_aliases_reward_account(acc);
    r &= string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_VIEW_SEC_KEY, acc_view_key);
    return r;
  }
  //---------------------------------------------------------------
  bool is_pos_block(const block& b)
  {
    if (!(b.flags & CURRENCY_BLOCK_FLAG_POS_BLOCK))
      return false;
    return is_pos_block(b.miner_tx);
  }
  //---------------------------------------------------------------
  bool is_pos_block(const transaction& tx)
  {
    if (tx.vin.size() == 2 &&
      tx.vin[0].type() == typeid(txin_gen) &&
      tx.vin[1].type() == typeid(txin_to_key))
      return true;
    return false;
  }
  size_t get_max_block_size()
  {
    return CURRENCY_MAX_BLOCK_SIZE;
  }
  //-----------------------------------------------------------------------------------------------
  size_t get_max_tx_size()
  {
    return CURRENCY_MAX_TRANSACTION_BLOB_SIZE;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t get_base_block_reward(bool is_pos, const boost::multiprecision::uint128_t& already_generated_coins, uint64_t height)
  {
    if (!height)
      return PREMINE_AMOUNT;
  
    return CURRENCY_BLOCK_REWARD;
  }
  //-----------------------------------------------------------------------------------------------
  bool get_block_reward(bool is_pos, size_t median_size, size_t current_block_size, const boost::multiprecision::uint128_t& already_generated_coins, uint64_t &reward, uint64_t height)
  {
    uint64_t base_reward = get_base_block_reward(is_pos, already_generated_coins, height);

    //make it soft
    if (median_size < CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE)
    {
      median_size = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE;
    }


    if (current_block_size <= median_size || height == 0)
    {
      reward = base_reward;
      return true;
    }

    if (current_block_size > 2 * median_size)
    {
      LOG_PRINT_L4("Block cumulative size is too big: " << current_block_size << ", expected less than " << 2 * median_size);
      return false;
    }

    uint64_t product_hi;
    // BUGFIX(taken from Monero): 32-bit saturation bug (e.g. ARM7), the result was being
    // treated as 32-bit by default.
    uint64_t multiplicand = 2 * median_size - current_block_size;
    multiplicand *= current_block_size;

    uint64_t product_lo = mul128(base_reward, multiplicand, &product_hi);

    uint64_t reward_hi;
    uint64_t reward_lo;
    div128_32(product_hi, product_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
    div128_32(reward_hi, reward_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
    CHECK_AND_ASSERT_MES(0 == reward_hi, false, "0 == reward_hi");
    CHECK_AND_ASSERT_MES(reward_lo < base_reward, false, "reward_lo < base_reward, reward: " << reward << ", base_reward: " << base_reward << ", current_block_size: " << current_block_size << ", median_size: " << median_size);

    reward = reward_lo;
    return true;
  }

  //-----------------------------------------------------------------------
  bool is_coinbase(const transaction& tx)
  {
    if (!tx.vin.size() || tx.vin.size() > 2)
      return false;

    if (tx.vin[0].type() != typeid(txin_gen))
      return false;

    return true;
  }
  //-----------------------------------------------------------------------
  bool is_coinbase(const transaction& tx, bool& pos_coinbase)
  {
    if (!is_coinbase(tx))
      return false;

    pos_coinbase = (tx.vin.size() == 2 && tx.vin[1].type() == typeid(txin_to_key));
    return true;
  }
  //-----------------------------------------------------------------------
  bool is_payment_id_size_ok(const payment_id_t& payment_id)
  {
    return payment_id.size() <= BC_PAYMENT_ID_SERVICE_SIZE_MAX;
  }
  //-----------------------------------------------------------------------
  std::string get_account_address_as_str(const account_public_address& addr)
  {
    return tools::base58::encode_addr(CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(addr));
  }
  //-----------------------------------------------------------------------
  std::string get_account_address_and_payment_id_as_str(const account_public_address& addr, const payment_id_t& payment_id)
  {
    return tools::base58::encode_addr(CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(addr) + payment_id);
  }
  //-----------------------------------------------------------------------
  bool get_account_address_from_str(account_public_address& addr, const std::string& str)
  {
    std::string integrated_payment_id; // won't be used
    return get_account_address_and_payment_id_from_str(addr, integrated_payment_id, str);
  }
  //-----------------------------------------------------------------------
  bool get_account_address_and_payment_id_from_str(account_public_address& addr, payment_id_t& payment_id, const std::string& str)
  {
    static const size_t addr_blob_size = sizeof(account_public_address);
    blobdata blob;
    uint64_t prefix;
    if (!tools::base58::decode_addr(str, prefix, blob))
    {
      LOG_PRINT_L1("Invalid address format: base58 decoding failed for \"" << str << "\"");
      return false;
    }

    if (blob.size() < addr_blob_size)
    {
      LOG_PRINT_L1("Address " << str << " has invalid format: blob size is " << blob.size() << " which is less, than expected " << addr_blob_size);
      return false;
    }

    if (blob.size() > addr_blob_size + BC_PAYMENT_ID_SERVICE_SIZE_MAX)
    {
      LOG_PRINT_L1("Address " << str << " has invalid format: blob size is " << blob.size() << " which is more, than allowed " << addr_blob_size + BC_PAYMENT_ID_SERVICE_SIZE_MAX);
      return false;
    }

    if (prefix == CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX)
    {
      // nothing
    }
    else if (prefix == CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX)
    {
      payment_id = blob.substr(addr_blob_size);
      blob = blob.substr(0, addr_blob_size);
    }
    else
    {
      LOG_PRINT_L1("Address " << str << " has wrong prefix " << prefix << ", expected " << CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX << " or " << CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX);
      return false;
    }

    if (!::serialization::parse_binary(blob, addr))
    {
      LOG_PRINT_L1("Account public address keys can't be parsed for address \"" << str << "\"");
      return false;
    }

    if (!crypto::check_key(addr.m_spend_public_key) || !crypto::check_key(addr.m_view_public_key))
    {
      LOG_PRINT_L1("Failed to validate address keys for address \"" << str << "\"");
      return false;
    }

    return true;
  }
  //---------------------------------------------------------------
  bool parse_payment_id_from_hex_str(const std::string& payment_id_str, payment_id_t& payment_id)
  {
    return epee::string_tools::parse_hexstr_to_binbuff(payment_id_str, payment_id);
  }
  //--------------------------------------------------------------------------------
  crypto::hash prepare_prefix_hash_for_sign(const transaction& tx, uint64_t in_index, const crypto::hash& tx_id)
  {
    CHECK_AND_ASSERT_MES(tx.vin.size() > in_index, null_hash, "condition failed: tx.vin.size(" << tx.vin.size() << ") > in_index( " << in_index << " )");

    crypto::hash tx_hash_for_signature = tx_id;
    if (get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE)
    {
      // separately signed transaction case - figure out all the outputs, extra entries and attachments the given input refers to
      size_t signed_outputs_count = 0;
      size_t signed_extras_count = 0;
      size_t signed_att_count = 0;

      // extra_attachment_info can be stored either in the tx extra (default) and/or in the input's etc_details (it overrides the default one)
      extra_attachment_info eai = AUTO_VAL_INIT(eai);
      if (get_type_in_variant_container(tx.extra, eai))
        signed_att_count = static_cast<size_t>(eai.cnt); // get signed attachments count from default extra_attachment_info (if exists)

      for (size_t i = 0; i != in_index + 1; i++)
      {
        // get signed outputs and extra entries count from input's details (each input has to have it)
        signed_parts so = AUTO_VAL_INIT(so);
        bool r = get_type_in_variant_container(get_txin_etc_options(tx.vin[i]), so);
        CHECK_AND_ASSERT_MES(r, null_hash, "Invalid input #" << i << " (of " << tx.vin.size() << ") contains no signed_parts");
        CHECK_AND_ASSERT_MES(signed_outputs_count <= so.n_outs, null_hash, "Invalid input #" << i << " (of " << tx.vin.size() << "): Next signed_outputs_count is less then prev");
        CHECK_AND_ASSERT_MES(signed_extras_count <= so.n_extras, null_hash, "Invalid input #" << i << " (of " << tx.vin.size() << "): Next signed_extras_count is less then prev");
        signed_outputs_count = so.n_outs;
        signed_extras_count = so.n_extras;

        // get signed attachments count from input's extra_attachment_info (it's optional, if exists - override default)
        if (get_type_in_variant_container(get_txin_etc_options(tx.vin[i]), eai))
        {
          CHECK_AND_ASSERT_MES(signed_att_count <= eai.cnt, null_hash, "Invalid input #" << i << " (of " << tx.vin.size() << "): Next signed_att_count is less then prev");
          signed_att_count = static_cast<size_t>(eai.cnt);
        }
      }

      if (in_index == tx.vin.size() - 1)
      {
        // for the last input make sure all outs, extra entries and attachments was counted correctly
        CHECK_AND_ASSERT_MES(signed_outputs_count == tx.vout.size(),   null_hash, "Separately signed complete tx has to mention all the outputs in its inputs: signed_outputs_count=" << signed_outputs_count << " tx.vout.size()=" << tx.vout.size());
        CHECK_AND_ASSERT_MES(signed_extras_count == tx.extra.size(),   null_hash, "Separately signed complete tx has to mention all the extra entries in its inputs: signed_extras_count=" << signed_extras_count << " tx.extra.size()=" << tx.extra.size());
        CHECK_AND_ASSERT_MES(signed_att_count == tx.attachment.size(), null_hash, "Separately signed complete tx has to mention all the attachments in its inputs: signed_att_count=" << signed_att_count << " tx.attachment.size()=" << tx.attachment.size());
        // okay, there's nothing to crop - tx_hash_for_signature is tx prefix hash
      }
      else
      {
        // the given input isn't the last one - we have to crop some data in order to calculate correct hash for signing
        transaction tx_local = tx;

        // crop all inputs past the given one
        tx_local.vin.resize(static_cast<size_t>(in_index) + 1);

        // crop outs
        CHECK_AND_ASSERT_MES(signed_outputs_count <= tx_local.vout.size(), null_hash, "signed_outputs_count(" << signed_outputs_count << " ) more than tx_local.vout.size()" << tx_local.vout.size());
        tx_local.vout.resize(signed_outputs_count);

        // crop extra
        CHECK_AND_ASSERT_MES(signed_extras_count <= tx_local.extra.size(), null_hash, "signed_extras_count(" << signed_extras_count << " ) more than tx_local.extra.size()" << tx_local.vout.size());
        tx_local.extra.resize(signed_extras_count);

        // crop attachments
        CHECK_AND_ASSERT_MES(signed_att_count <= tx_local.attachment.size(), null_hash, "signed_att_count(" << signed_att_count << " ) more than tx_local.attachment.size()" << tx_local.attachment.size());
        tx_local.attachment.resize(signed_att_count);

        // calculate hash of cropped tx as the result
        tx_hash_for_signature = get_transaction_hash(tx_local);
      }
    }

    return tx_hash_for_signature;
  }
  //------------------------------------------------------------------
  std::string dump_ring_sig_data(const crypto::hash& hash_for_sig, const crypto::key_image& k_image, const std::vector<const crypto::public_key*>& output_keys_ptrs, const std::vector<crypto::signature>& sig)
  {
    std::stringstream s;
    s << "  hash for sig: " << hash_for_sig << ENDL
      << "  k_image:      " << k_image << ENDL
      << "  out keys (" << output_keys_ptrs.size() << ")" << ENDL;
    size_t i = 0;
    for (auto& p_out_k : output_keys_ptrs)
      s << "    " << std::setw(2) << i++ << " " << *p_out_k << ENDL;
  
    s << "  signatures (" << sig.size() << ")" << ENDL;
    i = 0;
    for (auto& sig_el : sig)
      s << "    " << std::setw(2) << i++ << " " << sig_el << ENDL;
  
    return s.str();
  }


  //--------------------------------------------------------------------------------
  std::ostream& operator <<(std::ostream& o, const ref_by_id& r)
  {
    return o << "<" << r.n << ":" << r.tx_id << ">";
  }
  //--------------------------------------------------------------------------------
  const std::locale& utf8_get_conversion_locale()
  {
    static std::locale loc = boost::locale::generator().generate("en_US.UTF-8");
    return loc;
  }
  std::string utf8_to_upper(const std::string& s)
  {
    return boost::locale::to_upper(s, utf8_get_conversion_locale());
  }
  std::string utf8_to_lower(const std::string& s)
  {
    return boost::locale::to_lower(s, utf8_get_conversion_locale());
  }
  bool utf8_substring_test_case_insensitive(const std::string& match, const std::string& s)
  {
    if (match.empty())
      return true;
    return utf8_to_lower(s).find(utf8_to_lower(match), 0) != std::string::npos;
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::transaction& a, const currency::transaction& b) {
    return currency::get_transaction_hash(a) == currency::get_transaction_hash(b);
  }

  bool operator ==(const currency::block& a, const currency::block& b) {
    return currency::get_block_hash(a) == currency::get_block_hash(b);
  }
  bool operator ==(const currency::extra_attachment_info& a, const currency::extra_attachment_info& b)
  {
    if (a.cnt == b.cnt && a.hsh == b.hsh && a.sz == b.sz)
      return true;
    else 
      return false;
  }


  boost::multiprecision::uint1024_t get_a_to_b_relative_cumulative_difficulty(const wide_difficulty_type& difficulty_pos_at_split_point,
    const wide_difficulty_type& difficulty_pow_at_split_point,
    const difficulties& a_diff,
    const difficulties& b_diff )
  {
    static const wide_difficulty_type difficulty_starter = DIFFICULTY_STARTER;
    const wide_difficulty_type& a_pos_cumulative_difficulty = a_diff.pos_diff > 0 ? a_diff.pos_diff : difficulty_starter;
    const wide_difficulty_type& b_pos_cumulative_difficulty = b_diff.pos_diff > 0 ? b_diff.pos_diff : difficulty_starter;
    const wide_difficulty_type& a_pow_cumulative_difficulty = a_diff.pow_diff > 0 ? a_diff.pow_diff : difficulty_starter;
    const wide_difficulty_type& b_pow_cumulative_difficulty = b_diff.pow_diff > 0 ? b_diff.pow_diff : difficulty_starter;

    boost::multiprecision::uint1024_t basic_sum = boost::multiprecision::uint1024_t(a_pow_cumulative_difficulty) + (boost::multiprecision::uint1024_t(a_pos_cumulative_difficulty)*difficulty_pow_at_split_point) / difficulty_pos_at_split_point;
    boost::multiprecision::uint1024_t res =
      (basic_sum * a_pow_cumulative_difficulty * a_pos_cumulative_difficulty) / (boost::multiprecision::uint1024_t(b_pow_cumulative_difficulty)*b_pos_cumulative_difficulty);

//     if (res > boost::math::tools::max_value<wide_difficulty_type>())
//     {
//       ASSERT_MES_AND_THROW("[INTERNAL ERROR]: Failed to get_a_to_b_relative_cumulative_difficulty, res = " << res << ENDL
//         << ", difficulty_pos_at_split_point: " << difficulty_pos_at_split_point << ENDL
//         << ", difficulty_pow_at_split_point:" << difficulty_pow_at_split_point << ENDL
//         << ", a_pos_cumulative_difficulty:" << a_pos_cumulative_difficulty << ENDL
//         << ", b_pos_cumulative_difficulty:" << b_pos_cumulative_difficulty << ENDL
//         << ", a_pow_cumulative_difficulty:" << a_pow_cumulative_difficulty << ENDL
//         << ", b_pow_cumulative_difficulty:" << b_pow_cumulative_difficulty << ENDL       
//       );
//     }
    TRY_ENTRY();
//    wide_difficulty_type short_res = res.convert_to<wide_difficulty_type>();
    return res;
    CATCH_ENTRY_WITH_FORWARDING_EXCEPTION();
  }



} // namespace currency


