// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

struct tx_builder
{
  void step1_init(size_t version = TRANSACTION_VERSION_PRE_HF4, uint64_t unlock_time = 0)
  {
    m_tx = AUTO_VAL_INIT(m_tx);
    m_in_contexts.clear();

    m_tx.version = version;
    set_tx_unlock_time(m_tx, unlock_time);

    m_tx_key = currency::keypair::generate();
    add_tx_pub_key_to_extra(m_tx, m_tx_key.pub);
  }

  void step2_fill_inputs(const currency::account_keys& sender_account_keys, const std::vector<currency::tx_source_entry>& sources)
  {
    for(const currency::tx_source_entry& src_entr : sources)
    {
      m_in_contexts.push_back(currency::keypair());
      currency::keypair& in_ephemeral = m_in_contexts.back();
      crypto::key_image img;
      generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, in_ephemeral, img);

      // put key image into tx input
      currency::txin_to_key input_to_key = AUTO_VAL_INIT(input_to_key);
      input_to_key.amount = src_entr.amount;
      input_to_key.k_image = img;

      // fill outputs array and use relative offsets
      for(const currency::tx_source_entry::output_entry& out_entry : src_entr.outputs)
        input_to_key.key_offsets.push_back(out_entry.out_reference);

      // if the following line fails, consider using prepare_outputs_entries_for_key_offsets() for correct calculation of real out index
      CHECK_AND_ASSERT_THROW_MES(absolute_sorted_output_offsets_to_relative_in_place(input_to_key.key_offsets), "absolute_sorted_output_offsets_to_relative_in_place failed");
      m_tx.vin.push_back(input_to_key);
    }
  }

  void step3_fill_outputs(const std::vector<currency::tx_destination_entry>& destinations, uint8_t mix_attr = CURRENCY_TO_KEY_OUT_RELAXED, uint32_t minimum_sigs = UINT32_MAX)
  {
    size_t output_index = 0;
    for(const currency::tx_destination_entry& dst_entr : destinations)
    {
      CHECK_AND_ASSERT_MES(!dst_entr.addr.empty(), void(0), "Destination entry #" << output_index << " contains empty addr list");

      currency::tx_out_bare out = AUTO_VAL_INIT(out);
      out.amount = dst_entr.amount;

      if (dst_entr.addr.size() == 1)
      {
        // one destination address - create txout_to_key
        crypto::public_key out_eph_public_key = currency::null_pkey; // null_pkey means "burn" money
        currency::account_public_address addr = dst_entr.addr.front();
        if (addr.view_public_key != currency::null_pkey && addr.spend_public_key != currency::null_pkey)
        {
          crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
          crypto::generate_key_derivation(addr.view_public_key, m_tx_key.sec, derivation);
          crypto::derive_public_key(derivation, output_index, addr.spend_public_key, out_eph_public_key);
        }
        currency::txout_to_key tk = AUTO_VAL_INIT(tk);
        tk.mix_attr = mix_attr;
        tk.key = out_eph_public_key;
        out.target = tk;
      }
      else
      {
        // many destination addresses - create txout_multisig
        currency::txout_multisig ms = AUTO_VAL_INIT(ms);
        ms.minimum_sigs = (minimum_sigs == UINT32_MAX) ? dst_entr.addr.size() : minimum_sigs;
        for (auto& addr : dst_entr.addr)
        {
          crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
          bool r = crypto::generate_key_derivation(addr.view_public_key, m_tx_key.sec, derivation);
          CHECK_AND_ASSERT_MES(r, void(0), "generate_key_derivation failed for ms output #" << output_index);
          crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
          r = crypto::derive_public_key(derivation, output_index, addr.spend_public_key, out_eph_public_key);
          CHECK_AND_ASSERT_MES(r, void(0), "derive_public_key failed for ms output #" << output_index);
          ms.keys.push_back(out_eph_public_key);
        }
        out.target = ms;
      }

      m_tx.vout.push_back(out);
      output_index++;
    }
  }

  void step4_calc_hash()
  {
    get_transaction_prefix_hash(m_tx, m_tx_prefix_hash);
  }

  void step5_sign(const std::vector<currency::tx_source_entry>& sources)
  {
    m_tx.signatures.clear();

    size_t i = 0;
    for(const currency::tx_source_entry& src_entr : sources)
    {
      std::vector<const crypto::public_key*> keys_ptrs;
      for(const currency::tx_source_entry::output_entry& o : src_entr.outputs)
      {
        keys_ptrs.push_back(&o.stealth_address);
      }

      m_tx.signatures.push_back(currency::NLSAG_sig());
      std::vector<crypto::signature>& sigs = boost::get<currency::NLSAG_sig>(m_tx.signatures.back()).s;
      sigs.resize(src_entr.outputs.size());
      generate_ring_signature(m_tx_prefix_hash, boost::get<currency::txin_to_key>(m_tx.vin[i]).k_image, keys_ptrs, m_in_contexts[i].sec, src_entr.real_output, sigs.data());
      i++;
    }
  }

  static currency::transaction make_simple_tx_with_unlock_time(const std::vector<test_event_entry>& events,
    const currency::block& blk_head, const currency::account_base& from, const currency::account_base& to,
    uint64_t amount, uint64_t unlock_time, bool check_for_unlock_time = true)
  {
    std::vector<currency::tx_source_entry> sources;
    std::vector<currency::tx_destination_entry> destinations;
    fill_tx_sources_and_destinations(events, blk_head, from, to, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, check_for_unlock_time);

    tx_builder builder;
    builder.step1_init(TRANSACTION_VERSION_PRE_HF4, unlock_time);
    builder.step2_fill_inputs(from.get_keys(), sources);
    builder.step3_fill_outputs(destinations);
    builder.step4_calc_hash();
    builder.step5_sign(sources);
    return builder.m_tx;
  };

  static crypto::public_key generate_invalid_pub_key()
  {
    for (int i = 0; i <= 0xFF; ++i)
    {
      crypto::public_key key;
      memset(&key, i, sizeof(crypto::public_key));
      if (!crypto::check_key(key))
      {
        return key;
      }
    }

    throw std::runtime_error("invalid public key wasn't found");
    return crypto::public_key();
  }

  currency::transaction m_tx;
  currency::keypair m_tx_key;
  std::vector<currency::keypair> m_in_contexts;
  crypto::hash m_tx_prefix_hash;
};
