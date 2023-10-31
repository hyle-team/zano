// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

using namespace epee;
using namespace crypto;
using namespace currency;

//------------------------------------------------------------------------------
// escrow-specific test helpers
//------------------------------------------------------------------------------

inline bool are_all_inputs_mixed(const std::vector<currency::txin_v>& vin, size_t* p_min_fake_outs_count = nullptr)
{
  if (p_min_fake_outs_count)
    *p_min_fake_outs_count = SIZE_MAX;
  for (const auto& in : vin)
  {
    if (in.type().hash_code() == typeid(txin_gen).hash_code())
    {
      // skip it
    }
    else if (in.type().hash_code() == typeid(txin_to_key).hash_code())
    {
      size_t sz = boost::get<txin_to_key>(in).key_offsets.size();
      if (p_min_fake_outs_count && sz > 0)
        *p_min_fake_outs_count = std::min(*p_min_fake_outs_count, sz - 1);
      if (sz <= 1)
        return false;
    }
    else if (in.type().hash_code() == typeid(txin_multisig).hash_code())
    {
      return false; // multisig inputs are pretty much direct by design
    }
    else
    {
      return false; // unknown input type
    }
  }
  return true;
}

template<typename entry_t, typename container_t>
void remove_all_entries_from_variant_container_by_type(container_t& container)
{
  for(auto it = container.begin(); it != container.end(); /* nothing */)
  {
    if (it->type().hash_code() == typeid(entry_t).hash_code())
      it = container.erase(it);
    else
      ++it;
  }
}

inline bool refresh_wallet_and_check_1_contract_state(const char* wallet_name, std::shared_ptr<tools::wallet2> wallet, uint32_t expected_contract_state, size_t block_to_be_fetched = SIZE_MAX)
{
  bool stub_bool = false, r = false;
  size_t blocks_fetched = 0;
  LOG_PRINT_CYAN("Refreshing " << wallet_name << "'s wallet...", LOG_LEVEL_0);
  wallet->refresh(blocks_fetched);
  CHECK_AND_ASSERT_MES(block_to_be_fetched == SIZE_MAX || blocks_fetched == block_to_be_fetched, false, wallet_name << ": incorrect amount of fetched blocks: " << blocks_fetched << ", expected: " << block_to_be_fetched);
  LOG_PRINT_CYAN("Scan tx pool for " << wallet_name << "'s wallet...", LOG_LEVEL_0);
  wallet->scan_tx_pool(stub_bool);

  tools::escrow_contracts_container contracts;
  r = wallet->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for " << wallet_name << " failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == expected_contract_state, false, wallet_name << " has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state)
    << ", expected: " << tools::wallet_public::get_escrow_contract_state_name(expected_contract_state));
  
  return true;
}

inline bool refresh_wallet_and_check_contract_state(const char* wallet_name, std::shared_ptr<tools::wallet2> wallet, uint32_t expected_contract_state, crypto::hash contract_id, size_t block_to_be_fetched = SIZE_MAX)
{
  bool stub_bool = false, r = false;
  size_t blocks_fetched = 0;
  LOG_PRINT_CYAN("Refreshing " << wallet_name << "'s wallet...", LOG_LEVEL_0);
  wallet->refresh(blocks_fetched);
  CHECK_AND_ASSERT_MES(block_to_be_fetched == SIZE_MAX || blocks_fetched == block_to_be_fetched, false, wallet_name << ": incorrect amount of fetched blocks: " << blocks_fetched << ", expected: " << block_to_be_fetched);
  wallet->scan_tx_pool(stub_bool);

  tools::escrow_contracts_container contracts;
  r = wallet->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for " << wallet_name << " failed");
  for (const auto& c : contracts)
  {
    if (c.first == contract_id)
    {
      CHECK_AND_ASSERT_MES(c.second.state == expected_contract_state, false, wallet_name << " has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(c.second.state)
        << ", expected: " << tools::wallet_public::get_escrow_contract_state_name(expected_contract_state));
      return true;
    }
  }
  
  LOG_ERROR("No contracts found for " << wallet_name << ", by contract_id = " << contract_id);
  return false;
}

inline bool extract_cancellation_template_tx_from_request_tx(const transaction& cancel_request_tx, const account_keys& b_keys, transaction& cancel_template_tx)
{
  std::vector<currency::payload_items_v> decrypted_att;
  CHECK_AND_ASSERT_MES(decrypt_payload_items(true, cancel_request_tx, b_keys, decrypted_att), false, "decrypt_payload_items failed");
  for (auto& v : decrypted_att)
  {
    if (v.type() == typeid(tx_service_attachment))
    {
      const tx_service_attachment& sa = boost::get<tx_service_attachment>(v);
      if (sa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL)
      {
        bc_services::escrow_cancel_templates_body cpb = AUTO_VAL_INIT(cpb);
        CHECK_AND_ASSERT_MES(t_unserializable_object_from_blob(cpb, sa.body), false, "t_unserializable_object_from_blob failed");
        cancel_template_tx = cpb.tx_cancel_template;  
        return true;
      }
    }
  }
  return false; // not found
}


//------------------------------------------------------------------------------

enum escrow_custom_config_field
{
  eccf_normal                              = 0,
  eccf_template_inv_crypt_address          = (uint64_t)1 << 0,
  eccf_template_inv_a_inputs               = (uint64_t)1 << 1,
  eccf_template_inv_ms_amount              = (uint64_t)1 << 2,
  eccf_template_inv_sa_instruction         = (uint64_t)1 << 3,
  eccf_template_inv_sa_flags               = (uint64_t)1 << 4,
  eccf_template_no_multisig                = (uint64_t)1 << 5,
  eccf_template_inv_multisig               = (uint64_t)1 << 6,
  eccf_template_inv_multisig_low_min_sig   = (uint64_t)1 << 7,
  eccf_template_no_a_sigs                  = (uint64_t)1 << 8,
  eccf_template_no_tx_flags                = (uint64_t)1 << 9,
  eccf_proposal_inv_sa_flags               = (uint64_t)1 << 10,
  eccf_proposal_sa_empty_body              = (uint64_t)1 << 11,
  eccf_proposal_inv_flags                  = (uint64_t)1 << 12,
  eccf_proposal_additional_attach          = (uint64_t)1 << 13,
  eccf_template_additional_extra           = (uint64_t)1 << 14,
  eccf_template_additional_attach          = (uint64_t)1 << 15,
  eccf_template_more_than_1_multisig       = (uint64_t)1 << 16,
  eccf_template_inv_multisig_2             = (uint64_t)1 << 17,
  eccf_rel_template_inv_sigs_count         = (uint64_t)1 << 18,
  eccf_rel_template_all_money_to_b         = (uint64_t)1 << 19,
  eccf_rel_template_no_extra_att_inf       = (uint64_t)1 << 20,
  eccf_rel_template_inv_ms_amount          = (uint64_t)1 << 21,
  eccf_rel_template_inv_tsa_flags          = (uint64_t)1 << 22,
  eccf_rel_template_inv_instruction        = (uint64_t)1 << 23,
  eccf_rel_template_signed_parts_in_etc    = (uint64_t)1 << 24,
  eccf_rel_template_inv_tx_flags           = (uint64_t)1 << 25,
  eccf_acceptance_no_tsa_encryption        = (uint64_t)1 << 26,
  eccf_acceptance_no_tsa_compression       = (uint64_t)1 << 27,
  eccf_rel_template_all_money_to_a         = (uint64_t)1 << 28,
  eccf_cancellation_no_tsa_compression     = (uint64_t)1 << 29,
  eccf_cancellation_no_tsa_encryption      = (uint64_t)1 << 30,
  eccf_cancellation_inv_crypt_address      = (uint64_t)1 << 31,
};

inline bool build_custom_escrow_template(const std::vector<test_event_entry>& events, const block& head, const account_keys& a_keys,
  bc_services::contract_private_details& cpd, uint64_t unlock_time, uint64_t expiration_time, size_t nmix, uint64_t b_fee_release,
  uint64_t custom_config_mask,
  uint64_t tx_version,
  transaction& escrow_template_tx,           /* OUT */
  crypto::secret_key& tx_key_sec,            /* OUT */
  std::vector<tx_source_entry>& used_sources /* IN/OUT */)
{
  // NOTE: this function is intended to produce incorrect output, customized by custom_config_mask. To create correct output use custom_config_mask == 0.
  // coding style:
  // value_to_be_used = (~custom_config_mask & eccf_some_desctiption_here) ? correct_value                                      : incorrect_value;

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  bool r = false, need_to_resign = false;

  uint64_t a_inputs_amount = (~custom_config_mask & eccf_template_inv_a_inputs) ? cpd.amount_a_pledge + cpd.amount_to_pay : TESTS_DEFAULT_FEE * 2;
  uint64_t ms_amount = (~custom_config_mask & eccf_template_inv_ms_amount) ? cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay + b_fee_release : cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay;

  r = fill_tx_sources(sources, events, head, a_keys, a_inputs_amount, nmix, used_sources);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  int64_t change = get_sources_total_amount(sources) - a_inputs_amount;
  CHECK_AND_ASSERT_MES(change >= 0, false, "fill_tx_sources failed (2)");
  if (change > 0)
    destinations.push_back(tx_destination_entry(change, a_keys.account_address));

  if (custom_config_mask & eccf_template_no_multisig)
    destinations.push_back(tx_destination_entry(ms_amount, a_keys.account_address)); // incorrect
  else if (custom_config_mask & eccf_template_inv_multisig)
    destinations.push_back(tx_destination_entry(ms_amount, std::list<account_public_address>({ a_keys.account_address, a_keys.account_address }))); // incorrect
  else if (custom_config_mask & eccf_template_inv_multisig_2)
  {
    destinations.push_back(tx_destination_entry(ms_amount, std::list<account_public_address>({ a_keys.account_address, a_keys.account_address, a_keys.account_address }))); // incorrect
    destinations.back().minimum_sigs = 2; // pretend to be correct
  }
  else if (custom_config_mask & eccf_template_inv_multisig_low_min_sig)
  {
    destinations.push_back(tx_destination_entry(ms_amount, std::list<account_public_address>({ a_keys.account_address, cpd.b_addr }))); // seems to be correct
    destinations.back().minimum_sigs = 1; // incorrect
  }
  else if (custom_config_mask & eccf_template_more_than_1_multisig)
  {
    destinations.push_back(tx_destination_entry(ms_amount, std::list<account_public_address>({ a_keys.account_address, cpd.b_addr }))); // seems to be correct
    destinations.push_back(tx_destination_entry(TESTS_DEFAULT_FEE, std::list<account_public_address>({ a_keys.account_address, cpd.b_addr }))); // double multisig - incorrect
  }
  else
    destinations.push_back(tx_destination_entry(ms_amount, std::list<account_public_address>({ a_keys.account_address, cpd.b_addr }))); // truly correct

  tx_service_attachment sa = AUTO_VAL_INIT(sa);
  sa.service_id = BC_ESCROW_SERVICE_ID;
  sa.instruction = (~custom_config_mask & eccf_template_inv_sa_instruction) ? BC_ESCROW_SERVICE_INSTRUCTION_PRIVATE_DETAILS     : BC_ESCROW_SERVICE_INSTRUCTION_PUBLIC_DETAILS;
  sa.flags =       (~custom_config_mask & eccf_template_inv_sa_flags)       ? TX_SERVICE_ATTACHMENT_ENCRYPT_BODY                : 0;
  bc_services::pack_attachment_as_gzipped_json(cpd, sa);

  account_public_address crypt_addr = (~custom_config_mask & eccf_template_inv_crypt_address) ? cpd.b_addr                      : null_pub_addr;
  uint64_t flags = TX_FLAG_SIGNATURE_MODE_SEPARATE;

  std::vector<extra_v> extra;
  extra.push_back(sa);

  if (custom_config_mask & eccf_template_additional_extra)
    extra.push_back(extra_user_data({ get_random_text(1024) }));

  std::vector<attachment_v> attachments;

  if (custom_config_mask & eccf_template_additional_attach)
    attachments.push_back(tx_comment({ get_random_text(1024) }));

  r = construct_tx(a_keys, sources, destinations, extra, attachments, escrow_template_tx, tx_version, tx_key_sec, unlock_time, crypt_addr, expiration_time, CURRENCY_TO_KEY_OUT_RELAXED, true, flags);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  if (custom_config_mask & eccf_template_no_tx_flags)
  {
    for(auto it = escrow_template_tx.extra.begin(); it != escrow_template_tx.extra.end(); ++it)
      if (it->type().hash_code() == typeid(etc_tx_details_flags).hash_code())
      {
        escrow_template_tx.extra.erase(it);
        break;
      }
    need_to_resign = true;
  }

  if (need_to_resign)
  {
    r = resign_tx(a_keys, sources, escrow_template_tx);
    CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");
  }

  if (custom_config_mask & eccf_template_no_a_sigs)
  {
    escrow_template_tx.signatures.clear();
    escrow_template_tx.signatures.push_back(currency::NLSAG_sig());
  }

  append_vector_by_another_vector(used_sources, sources);
  return true;
}

inline bool build_custom_escrow_proposal(const std::vector<test_event_entry>& events, const block& head, const account_keys& a_keys,
  bc_services::contract_private_details& cpd, uint64_t unlock_time, uint64_t expiration_time, uint64_t template_unlock_time, uint64_t template_expiration_time, size_t nmix,
  uint64_t a_fee_proposal, uint64_t b_fee_release,
  uint64_t custom_config_mask,
  uint64_t tx_version,
  transaction& escrow_proposal_tx,           /* OUT */
  std::vector<tx_source_entry>& used_sources,/* IN/OUT */
  bc_services::proposal_body* p_pb = nullptr /* OUT */ )
{
  // NOTE: this function is intended to produce incorrect output, customized by custom_config_mask. To create correct output use custom_config_mask == 0.
  // coding style:
  // value_to_be_used = (~custom_config_mask & eccf_some_desctiption_here) ? correct_value                                      : incorrect_value;

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  bool r = false;

  bc_services::proposal_body local_pb = AUTO_VAL_INIT(local_pb);
  if (p_pb == nullptr)
    p_pb = &local_pb;
  if (~custom_config_mask & eccf_proposal_sa_empty_body)
  {
    r = build_custom_escrow_template(events, head, a_keys, cpd, template_unlock_time, template_expiration_time, nmix, b_fee_release, custom_config_mask, tx_version, p_pb->tx_template, p_pb->tx_onetime_secret_key, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_template failed");
  }

  tx_service_attachment sa = AUTO_VAL_INIT(sa);
  sa.service_id = BC_ESCROW_SERVICE_ID;
  sa.instruction = BC_ESCROW_SERVICE_INSTRUCTION_PROPOSAL;
  bc_services::pack_attachment_as_gzipped_bin(*p_pb, sa);
  sa.flags = (~custom_config_mask & eccf_proposal_inv_sa_flags) ? (sa.flags | TX_SERVICE_ATTACHMENT_ENCRYPT_BODY) : 0;

  std::vector<attachment_v> attachments;
  attachments.push_back(sa);

  if (custom_config_mask & eccf_proposal_additional_attach)
    attachments.push_back(std::string(get_random_text(1024)));

  r = fill_tx_sources(sources, events, head, a_keys, a_fee_proposal, nmix, used_sources);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  int64_t change = get_sources_total_amount(sources) - a_fee_proposal;
  CHECK_AND_ASSERT_MES(change >= 0, false, "fill_tx_sources failed (2)");
  if (change > 0)
    destinations.push_back(tx_destination_entry(change, a_keys.account_address));

  account_public_address crypt_addr = cpd.b_addr;

  crypto::secret_key tx_key_sec; // stub, not used
  r = construct_tx(a_keys, sources, destinations, empty_extra, attachments, escrow_proposal_tx, tx_version, tx_key_sec, unlock_time, crypt_addr, expiration_time, 0, true, (~custom_config_mask & eccf_proposal_inv_flags) ? 0 : TX_FLAG_SIGNATURE_MODE_SEPARATE);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  append_vector_by_another_vector(used_sources, sources);
  return true;
}

inline bool build_custom_escrow_release_template(
  const std::string& release_type,
  const account_keys& b_keys,
  const bc_services::contract_private_details& cpd,
  uint64_t unlock_time, uint64_t expiration_time,
  uint64_t tx_version, 
  const transaction& escrow_template_tx, /* IN  (needed for ms output, tx pub key) */
  uint64_t custom_config_mask,           /* IN */
  transaction& tx                        /* OUT */
)
{
  // NOTE: this function is intended to produce incorrect output, customized by custom_config_mask. To create correct output use custom_config_mask == 0.
  // coding style:
  // value_to_be_used = (~custom_config_mask & eccf_some_desctiption_here) ? correct_value                                      : incorrect_value;

  size_t ms_idx = get_multisig_out_index(escrow_template_tx.vout);
  CHECK_AND_ASSERT_MES(ms_idx < escrow_template_tx.vout.size(), false, "Incorrect ms output index: " << ms_idx);
  crypto::hash ms_id = get_multisig_out_id(escrow_template_tx, ms_idx);

  // inputs
  // create multisig (A-B) source, add keys from B
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = (~custom_config_mask & eccf_rel_template_inv_ms_amount) ?boost::get<currency::tx_out_bare>( escrow_template_tx.vout[ms_idx]).amount :boost::get<currency::tx_out_bare>( escrow_template_tx.vout[ms_idx]).amount + 10 * TESTS_DEFAULT_FEE;
  se.multisig_id = ms_id;
  se.real_output_in_tx_index = ms_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(escrow_template_tx);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(escrow_template_tx.vout[ms_idx]).target).keys.size();
  se.ms_sigs_count = (~custom_config_mask & eccf_rel_template_inv_sigs_count) ? 2 : 1;
  std::vector<tx_source_entry> sources({ se });

  // extra
  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  tsa.service_id = BC_ESCROW_SERVICE_ID;
  tsa.instruction = (~custom_config_mask & eccf_rel_template_inv_instruction) ? release_type : BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN;
  tsa.body = release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL ? "" : "the roof, the roof, the roof is on fire"; // allow non-empty body for release templates (only one will have it for test purposes)
  tsa.flags = (~custom_config_mask & eccf_rel_template_inv_tsa_flags) ? 0 : TX_SERVICE_ATTACHMENT_DEFLATE_BODY;
  //tsa.security = null_pkey;
  std::vector<extra_v> extra({ tsa });
  
  // outputs
  std::vector<tx_destination_entry> destinations;
  if (release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL)
  {
    if (custom_config_mask & eccf_rel_template_all_money_to_b)
    {
      destinations.push_back(tx_destination_entry(cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay, cpd.b_addr));
    }
    else if (custom_config_mask & eccf_rel_template_inv_ms_amount)
    {
      destinations.push_back(tx_destination_entry(cpd.amount_a_pledge, cpd.a_addr));
      destinations.push_back(tx_destination_entry(cpd.amount_b_pledge + cpd.amount_to_pay, cpd.b_addr));
      destinations.push_back(tx_destination_entry(10 * TESTS_DEFAULT_FEE, cpd.b_addr));
    }
    else
    { // correct
      destinations.push_back(tx_destination_entry(cpd.amount_a_pledge, cpd.a_addr));
      destinations.push_back(tx_destination_entry(cpd.amount_b_pledge + cpd.amount_to_pay, cpd.b_addr));
    }
  }
  else if (release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN)
  {
    if (custom_config_mask & eccf_rel_template_all_money_to_b)
    {
      destinations.push_back(tx_destination_entry(cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay, cpd.b_addr));
    }
    else
    { // correct
      destinations.push_back(tx_destination_entry(cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay, null_pub_addr));
    }
  }
  else if (release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL)
  {
    if (custom_config_mask & eccf_rel_template_all_money_to_a)
    {
      destinations.push_back(tx_destination_entry(cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay, cpd.a_addr));
    }
    else
    { // correct
      destinations.push_back(tx_destination_entry(cpd.amount_a_pledge + cpd.amount_to_pay, cpd.a_addr));
      destinations.push_back(tx_destination_entry(cpd.amount_b_pledge, cpd.b_addr));
    }
  }
  else
  {
    return false;
  }

  // attachments
  std::vector<attachment_v> attachments;
  if (release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN)
  {
    attachments.push_back(std::string("We don't need no water -- so let all the coins burn!")); // attachments are allowed and must be normally handled, so one of release template will always have them
    attachments.push_back(tx_comment({ "Burn, all the coins, burn!" }));
  }
  
  uint64_t tx_flags = 0;
  if (custom_config_mask & eccf_rel_template_inv_tx_flags)
  {
    tx_flags = TX_FLAG_SIGNATURE_MODE_SEPARATE;
    sources.back().separately_signed_tx_complete = true; // correctly finalize separately-constructed tx to avoid errors in construct_tx and sign_multisig_input_in_tx
  }

  crypto::secret_key one_time_secret_key = AUTO_VAL_INIT(one_time_secret_key);
  account_public_address crypt_address = AUTO_VAL_INIT(crypt_address);
  bool r = construct_tx(b_keys, sources, destinations, extra, attachments, tx, tx_version, one_time_secret_key, unlock_time, crypt_address, expiration_time, 0, true, tx_flags);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  bool tx_fully_signed = false;
  r = sign_multisig_input_in_tx(tx, 0, b_keys, escrow_template_tx, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, returned: " << r << ", tx_fully_signed: " << tx_fully_signed);
  
  bool need_to_resign = false;

  if (custom_config_mask & eccf_rel_template_no_extra_att_inf)
  {
    // remove extra_attachment_info
    remove_all_entries_from_variant_container_by_type<extra_attachment_info>(tx.extra);
    for(auto in : tx.vin)
    {
      auto p_etc_details = get_input_etc_details(in);
      if (p_etc_details != nullptr)
        remove_all_entries_from_variant_container_by_type<extra_attachment_info>(*p_etc_details);
    }
    need_to_resign = true;
  }

  if (custom_config_mask & eccf_rel_template_signed_parts_in_etc)
  {
    CHECK_AND_ASSERT_MES(tx.vin.size() == 1, false, "Invalid vin size: " << tx.vin.size());
    signed_parts sp;
    sp.n_extras = 10;
    sp.n_outs = 10;
    boost::get<txin_multisig>(tx.vin.back()).etc_details.push_back(sp);
    need_to_resign = true;
  }

  if (need_to_resign)
  {
    r = resign_tx(b_keys, sources, tx, &escrow_template_tx);
    CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");
  }

  return true;
}

inline bool build_custom_escrow_accept_proposal(
  const std::vector<test_event_entry>& events, const block& head, size_t nmix,  
  const account_keys& b_keys,
  const bc_services::contract_private_details& cpd,
  uint64_t unlock_time, uint64_t expiration_time,
  uint64_t release_unlock_time, uint64_t release_expiration_time,
  uint64_t b_fee_accept, uint64_t b_fee_release,
  uint64_t custom_config_mask,                        /* IN */
  crypto::secret_key one_time_secret_key,             /* IN */
  uint64_t tx_version,                                /* IN */
  transaction& tx,                                    /* IN (escrow template), OUT */
  std::vector<tx_source_entry>& used_sources          /* IN/OUT */
)
{
  // NOTE: this function is intended to produce incorrect output, customized by custom_config_mask. To create correct output use custom_config_mask == 0.
  // coding style:
  // value_to_be_used = (~custom_config_mask & eccf_some_desctiption_here) ? correct_value                                      : incorrect_value;

  bool r = false;
  // complete separately-signed tx: B-part inputs
  std::vector<tx_source_entry> sources;
  uint64_t b_sources_amount = cpd.amount_b_pledge + b_fee_accept + b_fee_release;
  r = fill_tx_sources(sources, events, head, b_keys, b_sources_amount, nmix, used_sources, true, true, false);
  CHECK_AND_ASSERT_MES(r && !sources.empty(), false, "fill_tx_sources failed");
  uint64_t b_found_sources_amount = get_sources_total_amount(sources);
  CHECK_AND_ASSERT_MES(b_found_sources_amount >= b_sources_amount, false, "fill_tx_sources returned incorrect sourses, found amount: " << b_found_sources_amount << ", requested: " << b_sources_amount);
  uint64_t b_change = b_found_sources_amount - b_sources_amount;

  // complete separately-signed tx: B-part outputs
  std::vector<tx_destination_entry> destinations;
  if (b_change > 0)
    destinations.push_back(tx_destination_entry(b_change, cpd.b_addr));


  // generate release templates
  bc_services::escrow_relese_templates_body rtb = AUTO_VAL_INIT(rtb);
  r = build_custom_escrow_release_template(BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL, b_keys, cpd, release_unlock_time, release_expiration_time, tx_version, tx, custom_config_mask, rtb.tx_normal_template);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_release_template(normal) failed");
  r = build_custom_escrow_release_template(BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN,   b_keys, cpd, release_unlock_time, release_expiration_time, tx_version, tx, custom_config_mask, rtb.tx_burn_template);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_release_template(burn) failed");

  // put release templates into the extra
  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  tsa.service_id = BC_ESCROW_SERVICE_ID;
  tsa.instruction = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_TEMPLATES;
  bc_services::pack_attachment_as_gzipped_bin(rtb, tsa);
  if (custom_config_mask & eccf_acceptance_no_tsa_compression)
    tsa.flags &= ~TX_SERVICE_ATTACHMENT_DEFLATE_BODY; // clear flag
  if (~custom_config_mask & eccf_acceptance_no_tsa_encryption)
    tsa.flags |= TX_SERVICE_ATTACHMENT_ENCRYPT_BODY;
  
  std::vector<extra_v> extra;
  extra.push_back(tsa);

  if (unlock_time != 0)
    extra.push_back(etc_tx_details_unlock_time({ unlock_time })); // should be explicitly specified here, as construct_tx ignores it when append_mode is true

  if (expiration_time != 0)
    extra.push_back(etc_tx_details_expiration_time({expiration_time})); // should be explicitly specified here, as construct_tx ignores it when append_mode is true

  // attachments
  std::vector<attachment_v> attachments;

  // construct tx (2nd phase - adding B-part and complete)
  uint64_t tx_flags = TX_FLAG_SIGNATURE_MODE_SEPARATE;

  sources.back().separately_signed_tx_complete = true;

  account_public_address crypt_address = AUTO_VAL_INIT(crypt_address);
  r = construct_tx(b_keys, sources, destinations, extra, attachments, tx, tx_version, one_time_secret_key, 0, crypt_address, 0, 0, true, tx_flags); // see comment above regarding unlock_time and expiration_time
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  return true;
}

inline bool build_custom_escrow_cancel_proposal(
  const std::vector<test_event_entry>& events, const block& head, size_t nmix,  
  const account_keys& a_keys,
  const bc_services::contract_private_details& cpd,
  uint64_t unlock_time, uint64_t expiration_time,
  uint64_t release_unlock_time, uint64_t release_expiration_time,
  uint64_t a_fee_cancellation_request,
  uint64_t custom_config_mask,                        /* IN */
  const transaction& escrow_template_tx,              /* IN */
  uint64_t tx_version,                                /* IN */
  transaction& tx,                                    /* OUT */
  std::vector<tx_source_entry>& used_sources          /* IN/OUT */
)
{
  // NOTE: this function is intended to produce incorrect output, customized by custom_config_mask. To create correct output use custom_config_mask == 0.
  // coding style:
  // value_to_be_used = (~custom_config_mask & eccf_some_desctiption_here) ? correct_value                                      : incorrect_value;

  bool r = false;
  // inputs
  std::vector<tx_source_entry> sources;
  r = fill_tx_sources(sources, events, head, a_keys, a_fee_cancellation_request, nmix, used_sources, true, true, false);
  CHECK_AND_ASSERT_MES(r && !sources.empty(), false, "fill_tx_sources failed");
  uint64_t a_found_sources_amount = get_sources_total_amount(sources);
  CHECK_AND_ASSERT_MES(a_found_sources_amount >= a_fee_cancellation_request, false, "fill_tx_sources returned incorrect sourses, found amount: " << a_found_sources_amount << ", requested: " << a_fee_cancellation_request);
  uint64_t a_change = a_found_sources_amount - a_fee_cancellation_request;
  // outputs
  std::vector<tx_destination_entry> destinations;
  if (a_change > 0)
    destinations.push_back(tx_destination_entry(a_change, cpd.a_addr));

  // generate cancel release template
  bc_services::escrow_cancel_templates_body ctb = AUTO_VAL_INIT(ctb);
  r = build_custom_escrow_release_template(BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL, a_keys, cpd, release_unlock_time, release_expiration_time, tx_version, escrow_template_tx, custom_config_mask, ctb.tx_cancel_template);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_release_template(cancel) failed");

  // put release templates into the extra
  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  tsa.service_id = BC_ESCROW_SERVICE_ID;
  tsa.instruction = BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL;
  bc_services::pack_attachment_as_gzipped_bin(ctb, tsa);
  if (custom_config_mask & eccf_cancellation_no_tsa_compression)
    tsa.flags &= ~TX_SERVICE_ATTACHMENT_DEFLATE_BODY; // clear flag
  if (~custom_config_mask & eccf_cancellation_no_tsa_encryption)
    tsa.flags |= TX_SERVICE_ATTACHMENT_ENCRYPT_BODY;
  
  std::vector<extra_v> extra;
  extra.push_back(tsa);

  // attachments
  std::vector<attachment_v> attachments;

  uint64_t tx_flags = 0;

  account_public_address crypt_address = AUTO_VAL_INIT(crypt_address);
  if (~custom_config_mask & eccf_cancellation_inv_crypt_address)
    crypt_address = cpd.b_addr;
  crypto::secret_key one_time_secret_key = AUTO_VAL_INIT(one_time_secret_key);
  r = construct_tx(a_keys, sources, destinations, extra, attachments, tx, tx_version, one_time_secret_key, unlock_time, crypt_address, expiration_time, 0, true, tx_flags);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  return true;
}


inline bool operator==(const bc_services::contract_private_details& lhs, const bc_services::contract_private_details& rhs)
{
  return lhs.amount_a_pledge == rhs.amount_a_pledge &&
    lhs.amount_b_pledge == rhs.amount_b_pledge &&
    lhs.amount_to_pay == rhs.amount_to_pay &&
    lhs.a_addr == rhs.a_addr &&
    lhs.b_addr == rhs.b_addr &&
    lhs.comment == rhs.comment &&
    lhs.title == rhs.title;
}

inline bool check_contract_state(const tools::escrow_contracts_container& contracts, const bc_services::contract_private_details& cpd, tools::wallet_public::escrow_contract_details::contract_state state, const char* party)
{
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, party << " has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == cpd, false, party << " has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == state, false, party << " has invalid contract state: " <<
    tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state) << ", expected state: " << tools::wallet_public::get_escrow_contract_state_name(state));
  return true;
}
