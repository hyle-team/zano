// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2016 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define KEEP_WALLET_LOG_MACROS
#include "wallet2.h"
#include "currency_core/currency_format_utils.h"
#include "common/variant_helper.h"

#undef LOG_DEFAULT_CHANNEL
#define LOG_DEFAULT_CHANNEL "wallet"

using namespace currency;

namespace tools
{
//-----------------------------------------------------------------------------------------------------
bool wallet2::validate_escrow_proposal(const wallet_public::wallet_transfer_info& wti, const bc_services::proposal_body& prop,
  std::vector<payload_items_v>& decrypted_items,       /* OUT */
  crypto::hash& ms_id,                                 /* OUT */
  bc_services::contract_private_details& cpd           /* OUT */
)
{
#define LOC_CHK(cond, mes) WLT_CHECK_AND_ASSERT_MES(cond, false, "Invalid escrow proposal: " << mes << ". tx: " << get_transaction_hash(wti.tx));

  // I. validate escrow proposal tx
  const transaction& escrow_proposal_tx = wti.tx;
  uint64_t escrow_proposal_tx_unlock_time = get_tx_max_unlock_time(escrow_proposal_tx);
  LOC_CHK(escrow_proposal_tx_unlock_time == 0, "proposal tx unlock time is non-zero: " << escrow_proposal_tx_unlock_time);

  uint64_t escrow_proposal_expiration_time = get_tx_expiration_time(escrow_proposal_tx);
  LOC_CHK(escrow_proposal_expiration_time == 0, "proposal tx expiration time is non-zero: " << escrow_proposal_expiration_time);

  // II. validate escrow template tx
  // (1/5) inputs
  LOC_CHK(!prop.tx_template.vin.empty(), "template has empty inputs");
  size_t i = 0;
  uint64_t inputs_amount = 0;
  for (auto& inp : prop.tx_template.vin)
  {
    LOC_CHK(inp.type() == typeid(txin_to_key), "template's input #" << i << " has wrong type: " << inp.type().name());
    const txin_to_key& in_to_key = boost::get<txin_to_key>(inp);
    
    signed_parts sp = AUTO_VAL_INIT(sp);
    bool r = currency::get_type_in_variant_container<signed_parts>(in_to_key.etc_details, sp);
    LOC_CHK(r, "template's input #" << i << " doen't contain signed_parts in etc_details");

    LOC_CHK(sp.n_extras == prop.tx_template.extra.size(), "template's input #" << i << " has wrong etc_details signed_parts.n_extras: " << sp.n_extras);
    LOC_CHK(sp.n_outs == prop.tx_template.vout.size(), "template's input #" << i << " has wrong etc_details signed_parts.n_outs: " << sp.n_outs);
    inputs_amount += in_to_key.amount;
    ++i;
  }

  // (2/5) extra
  decrypted_items.clear();
  bool r = decrypt_payload_items(wti.is_income_mode_encryption(), prop.tx_template, m_account.get_keys(), decrypted_items);
  LOC_CHK(r, "failed to decrypt payload items in proposal tx");

  currency::tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  r = bc_services::get_first_service_attachment_by_id(decrypted_items, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_PRIVATE_DETAILS, tsa);
  LOC_CHK(r, "BC_ESCROW_SERVICE_INSTRUCTION_PRIVATE_DETAILS not found in proposal template");
  LOC_CHK((tsa.flags & TX_SERVICE_ATTACHMENT_ENCRYPT_BODY) != 0, "tx_service_attachment was not encrypted, tsa.flags: " << static_cast<size_t>(tsa.flags));
  r = epee::serialization::load_t_from_json(cpd, tsa.body);
  LOC_CHK(r, "proposal template couldn't be loaded from json");

  uint64_t flags = get_tx_flags(prop.tx_template);
  LOC_CHK(flags == TX_FLAG_SIGNATURE_MODE_SEPARATE, "template has invalid tx flags: " << flags);

  uint64_t template_expiration_time = get_tx_expiration_time(prop.tx_template);
  LOC_CHK(template_expiration_time != 0, "template has no expiration time");

  uint64_t template_unlock_time = get_tx_max_unlock_time(prop.tx_template);
  LOC_CHK(template_unlock_time == 0, "template has non-zero unlock time: " << template_unlock_time);

  // (3/5) outputs
  uint64_t to_key_outputs_amount = 0;
  size_t ms_out_index = SIZE_MAX;
  for (size_t i = 0; i != prop.tx_template.vout.size(); ++i)
  {
    VARIANT_SWITCH_BEGIN(prop.tx_template.vout[i]);
    VARIANT_CASE_CONST(tx_out_bare, o)
    {
      if (o.target.type() == typeid(txout_multisig))
      {
        LOC_CHK(ms_out_index == SIZE_MAX, "template has more than one multisig output");
        ms_out_index = i;
      }
      else if (o.target.type() == typeid(txout_to_key))
        to_key_outputs_amount += o.amount;
      else
        LOC_CHK(false, "Invalid output type: " << o.target.type().name());
    }
    VARIANT_CASE_CONST(tx_out_zarcanum, o);
    //@#@      
    VARIANT_SWITCH_END();
  }
  LOC_CHK(ms_out_index != SIZE_MAX, "template has no multisig outputs");
  ms_id = currency::get_multisig_out_id(prop.tx_template, ms_out_index);
  // @#@ using of get_tx_out_bare_from_out_v might be not safe, TODO: review this code
  uint64_t ms_amount = currency::get_tx_out_bare_from_out_v(prop.tx_template.vout[ms_out_index]).amount;
  const txout_multisig& ms = boost::get<txout_multisig>(currency::get_tx_out_bare_from_out_v(prop.tx_template.vout[ms_out_index]).target);
  LOC_CHK(ms.minimum_sigs == 2, "template has multisig output with wrong minimum_sigs==" << ms.minimum_sigs);
  LOC_CHK(ms.keys.size() == 2, "template has multisig output with wrong keys size: " << ms.keys.size());

  crypto::public_key a_key = AUTO_VAL_INIT(a_key), b_key = AUTO_VAL_INIT(b_key);
  crypto::key_derivation der = AUTO_VAL_INIT(der);
  r = crypto::generate_key_derivation(cpd.a_addr.view_public_key, prop.tx_onetime_secret_key, der);
  LOC_CHK(r, "generate_key_derivation failed: A");
  r = crypto::derive_public_key(der, ms_out_index, cpd.a_addr.spend_public_key, a_key);
  LOC_CHK(r, "derive_public_key failed: A");
  r = crypto::generate_key_derivation(cpd.b_addr.view_public_key, prop.tx_onetime_secret_key, der);
  LOC_CHK(r, "generate_key_derivation failed: B");
  r = crypto::derive_public_key(der, ms_out_index, cpd.b_addr.spend_public_key, b_key);
  LOC_CHK(r, "derive_public_key failed: B");
  bool correct_keys = (ms.keys[0] == a_key && ms.keys[1] == b_key) || (ms.keys[0] == b_key && ms.keys[1] == a_key);
  LOC_CHK(correct_keys, "template has mulisig output with invalid keys: 0:" << ms.keys[0] << " 1:" << ms.keys[1]);

  LOC_CHK(cpd.amount_b_pledge + cpd.amount_to_pay > 0, "template has zero (b pledge + amount to pay)");

  uint64_t min_ms_amount = cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay + TX_DEFAULT_FEE;
  LOC_CHK(ms_amount >= min_ms_amount, "template multisig amount " << ms_amount << " is less than contract expected value: " << min_ms_amount << ", a_pledge=" << cpd.amount_a_pledge << ", b_pledge=" << cpd.amount_b_pledge << ", amount_to_pay=" << cpd.amount_to_pay);
  uint64_t min_a_inputs = cpd.amount_a_pledge + cpd.amount_to_pay;
  LOC_CHK(inputs_amount > to_key_outputs_amount, "template inputs amount from (A): " << inputs_amount << " are not greater than to_key_outputs_amount: " << to_key_outputs_amount);
  LOC_CHK(inputs_amount - to_key_outputs_amount >= min_a_inputs, "template inputs amount from (A) minus change-like to_key outputs: " << inputs_amount - to_key_outputs_amount << " is less than contract expected minimum: " << min_a_inputs << ", a_pledge=" << cpd.amount_a_pledge << ", amount_to_pay=" << cpd.amount_to_pay);

  // (4/5) attachments

  // (5/5) signatures
  
  // TODO: correst signatures checking requires RPC calls to blockchain source to obtain outputs' keys for all txin_to_key inputs

  return true;
#undef LOC_CHK
}
//----------------------------------------------------------------------------------------------------
// TODO move here:
// bool wallet2::handle_proposal(const currency::transaction& escrow_proposal_tx, wallet_rpc::wallet_transfer_info& wti, const bc_services::proposal_body& prop)
//----------------------------------------------------------------------------------------------------

bool wallet2::validate_escrow_release(const transaction& tx, bool release_type_normal, const bc_services::contract_private_details& cpd,
  const txout_multisig& source_ms_out, const crypto::hash& ms_id, size_t source_ms_out_index, const transaction& source_tx, const currency::account_keys& a_keys) const
{
#define LOC_CHK(cond, mes) WLT_CHECK_AND_ASSERT_MES(cond, false, "Invalid escrow " << (release_type_normal ? "normal" : "burn") << " release template: " << mes);

  // (1/5) inputs
  LOC_CHK(tx.vin.size() == 1, "vin size expected to be 1, actual is " << tx.vin.size());
  LOC_CHK(tx.vin.back().type() == typeid(txin_multisig), "input 0 is not txin_multisig");
  const txin_multisig& ms = boost::get<txin_multisig>(tx.vin.back());
  LOC_CHK(ms.multisig_out_id == ms_id, "multisig input references to wrong ms output: " << ms.multisig_out_id << ", expected: " << ms_id);
  LOC_CHK(source_ms_out.minimum_sigs == 2, "multisig output has wrong minimim_sigs: " <<source_ms_out.minimum_sigs << ", expected: 2");
  LOC_CHK(ms.sigs_count == source_ms_out.minimum_sigs, "multisig input has wrong sig_count: " << ms.sigs_count << ", expected: " << source_ms_out.minimum_sigs);
  LOC_CHK(source_tx.vout[source_ms_out_index].type() == typeid(tx_out_bare), "multisig input has wrong output type:");
  LOC_CHK(ms.amount == currency::get_tx_out_bare_from_out_v(source_tx.vout[source_ms_out_index]).amount, "multisig input amount: " << ms.amount << " does not match with source tx out amount: " << currency::get_tx_out_bare_from_out_v(source_tx.vout[source_ms_out_index]).amount);
  
  uint64_t min_ms_amount = cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay + TX_DEFAULT_FEE;
  LOC_CHK(ms.amount >= min_ms_amount, "multisig input amount " << ms.amount << " is less than contract expected value: " << min_ms_amount << ", a_pledge=" << cpd.amount_a_pledge << ", b_pledge=" << cpd.amount_b_pledge << ", amount_to_pay=" << cpd.amount_to_pay);

  bool r = have_type_in_variant_container<signed_parts>(ms.etc_details);
  LOC_CHK(!r, "multisig input contains signed_parts in etc_details");

  r = have_type_in_variant_container<extra_attachment_info>(ms.etc_details);
  LOC_CHK(!r, "multisig input contains extra_attachment_info in etc_details");

  // (2/5) extra
  uint64_t flags = get_tx_flags(tx);
  LOC_CHK(flags == 0, "invalid tx flags: " << flags);

  uint64_t expiration_time = get_tx_expiration_time(tx);
  LOC_CHK(expiration_time == 0, "tx has non-zero expiration time: " << expiration_time);

  uint64_t unlock_time = get_tx_max_unlock_time(tx);
  LOC_CHK(unlock_time == 0, "tx has non-zero unlock time: " << unlock_time);

  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  size_t cnt = count_type_in_variant_container<tx_service_attachment>(tx.extra);
  r = get_type_in_variant_container(tx.extra, tsa);
  LOC_CHK(r && cnt == 1, "invalid number of tx_service_attachment: " << cnt);
  LOC_CHK(tsa.flags == 0, "invalid tx_service_attachment flags: " << static_cast<size_t>(tsa.flags));
  //LOC_CHK(tsa.body.empty(), "tx_service_attachment has non-empty body");
  LOC_CHK(tsa.service_id == BC_ESCROW_SERVICE_ID, "tx_service_attachment has invalid service id: " << tsa.service_id);
  r = (tsa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL && release_type_normal) || (tsa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN && !release_type_normal);
  LOC_CHK(r, "tx_service_attachment has invalid instruction: " << tsa.instruction);
  LOC_CHK(tsa.security.empty(), "tx_service_attachment has non-empty secrurity vector");

  r = validate_attachment_info(tx.extra, tx.attachment, false); // note: having no attachment info for empty attachments is totally OK
  LOC_CHK(r, "there is invalid attachment info in the extra");

  // (3/5) outputs
  crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);
  crypto::key_derivation der = AUTO_VAL_INIT(der);
  r = crypto::generate_key_derivation(tx_pub_key, a_keys.view_secret_key, der);
  LOC_CHK(r, "generate_key_derivation failed");
  uint64_t total_outputs_amount = 0, outputs_to_A_amount = 0, outputs_to_null_addr_amount = 0;
  for (size_t i = 0; i != tx.vout.size(); ++i)
  {
    VARIANT_SWITCH_BEGIN(tx.vout[i]);
    VARIANT_CASE_CONST(tx_out_bare, o);
    {
      if (o.target.type() == typeid(txout_to_key))
      {
        total_outputs_amount += o.amount;
        const txout_to_key& otk = boost::get<txout_to_key>(o.target);
        crypto::public_key ephemeral_pub_key = AUTO_VAL_INIT(ephemeral_pub_key);
        r = crypto::derive_public_key(der, i, cpd.a_addr.spend_public_key, ephemeral_pub_key);
        LOC_CHK(r, "derive_public_key failed for output #" << i);
        if (otk.key == ephemeral_pub_key)
          outputs_to_A_amount += o.amount;
        else if (otk.key == null_pkey)
          outputs_to_null_addr_amount += o.amount;
      }
      else
        LOC_CHK(false, "Invalid output type: " << o.target.type().name());
    }
    VARIANT_CASE_CONST(tx_out_zarcanum, o)
      LOC_CHK(false, "Invalid output type: " << typeid(o).name());       
    VARIANT_SWITCH_END();
  }

  if (release_type_normal)
  {
    LOC_CHK(outputs_to_A_amount >= cpd.amount_a_pledge, "A-addressed outs total amount: " << outputs_to_A_amount << " is less than a_pledge: " << cpd.amount_a_pledge);
  }
  else
  { // release type burn
    LOC_CHK(outputs_to_null_addr_amount == total_outputs_amount, "not all outputs are burnt");
    uint64_t amount_expected_to_be_burnt = cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay;
    LOC_CHK(outputs_to_null_addr_amount == amount_expected_to_be_burnt, "total amount of burnt outputs: " << outputs_to_null_addr_amount << " is less than contract expected value: " << amount_expected_to_be_burnt << ", a_pledge=" << cpd.amount_a_pledge << ", b_pledge=" << cpd.amount_b_pledge << ", amount_to_pay=" << cpd.amount_to_pay);
  }

  int64_t tx_fee = ms.amount - total_outputs_amount;
  LOC_CHK(tx_fee >= static_cast<int64_t>(TX_DEFAULT_FEE), "too small fee: " << tx_fee);

  // (4/5) attachment

  // nothing to validate


  // (5/5) signatures
  LOC_CHK(tx.signatures.size() == 1, "invalid singatures size: " << tx.signatures.size()); // only 1 input means only 1 signature vector

  VARIANT_SWITCH_BEGIN(tx.signatures[0]);
  VARIANT_CASE_CONST(NLSAG_sig, signature_NLSAG)
  {
    auto& signature = signature_NLSAG.s;
    // As we don't have b_keys we can't be sure which signature is B's and which is reserved for A (should be a null-placeholder, if present).
    // Having a_keys, we determine index of A key in multisig output keys array.
    // Thus it's possible to determine the order of signatures (A, B or B, A), and, eventually, validate B signature.
    crypto::public_key source_tx_pub_key = get_tx_pub_key_from_extra(source_tx);
    r = crypto::generate_key_derivation(source_tx_pub_key, a_keys.view_secret_key, der);
    LOC_CHK(r, "generate_key_derivation failed");
    crypto::public_key ephemeral_pub_key = AUTO_VAL_INIT(ephemeral_pub_key);
    r = crypto::derive_public_key(der, source_ms_out_index, a_keys.account_address.spend_public_key, ephemeral_pub_key);
    LOC_CHK(r, "derive_public_key failed");

    LOC_CHK(source_ms_out.keys.size() == 2, "internal error: invalid ms output keys array, size: " << source_ms_out.keys.size());
    LOC_CHK(signature.size() == 2, "internal error: invalid signature size for input #0: " << signature.size())
      size_t ms_out_key_a_index = std::find(source_ms_out.keys.begin(), source_ms_out.keys.end(), ephemeral_pub_key) - source_ms_out.keys.begin();
    LOC_CHK(ms_out_key_a_index < source_ms_out.keys.size(), "internal error: can't find A ephemeral pub key within ms output keys");
    size_t ms_out_key_b_index = 1 - ms_out_key_a_index;

    // in this particular case (source_ms_out.minimum_sigs == source_ms_out.keys.size() == 2) index in 'keys' is the same as index in signature
    crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, 0, get_transaction_hash(tx));
    r = crypto::check_signature(tx_hash_for_signature, source_ms_out.keys[ms_out_key_b_index], signature[ms_out_key_b_index]);
    LOC_CHK(r, "B signature for multisig input is invalid");
  }
  VARIANT_CASE_CONST(ZC_sig, s);
  //@#@
  VARIANT_CASE_THROW_ON_OTHER();
  VARIANT_SWITCH_END();

  return true;
#undef LOC_CHK
}
//----------------------------------------------------------------------------------------------------
bool wallet2::validate_escrow_contract(const wallet_public::wallet_transfer_info& wti, const bc_services::contract_private_details& cpd, bool is_a,
  const std::vector<currency::payload_items_v>& decrypted_items,
  crypto::hash& ms_id,                             /* OUT */
  bc_services::escrow_relese_templates_body& rtb   /* OUT */)
{
#define LOC_CHK(cond, mes) WLT_CHECK_AND_ASSERT_MES(cond, false, "Invalid escrow contract: " << mes << ". Escrow party: " << (is_a ? "A" : "B") << ". tx: " << get_transaction_hash(wti.tx));
  bool r = false;

  // TODO: validate A-part of transaction?

  size_t n = get_multisig_out_index(wti.tx.vout);
  LOC_CHK(n < wti.tx.vout.size(), "multisig output was not found");
  ms_id = currency::get_multisig_out_id(wti.tx, n);
  LOC_CHK(ms_id != null_hash, "failed to obtain ms output id");
  const txout_multisig& ms = boost::get<txout_multisig>( boost::get<tx_out_bare>(wti.tx.vout[n]).target);

  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  r = bc_services::get_first_service_attachment_by_id(decrypted_items, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_TEMPLATES, tsa);
  LOC_CHK(r, "BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_TEMPLATES was not found");
  LOC_CHK(tsa.flags & TX_SERVICE_ATTACHMENT_ENCRYPT_BODY, "tx_service_attachment was not encrypted, tsa.flags: " << static_cast<size_t>(tsa.flags));
  LOC_CHK(tsa.security.empty(), "tx_service_attachment has non-empty secrurity vector");
  r = t_unserializable_object_from_blob(rtb, tsa.body);
  LOC_CHK(r, "can't deserialize tx_service_attachment body");

  if (is_a)
  {
    // Validate release templates only for A party, because:
    // 1. We need a_keys to validate A-targeted outputs.
    // 2. It's B-party who accepts the proposal and makes release templates for A, so it does not make much sense to validate release templates if it's B's wallet.
    r = validate_escrow_release(rtb.tx_normal_template, true, cpd, ms, ms_id, n, wti.tx, m_account.get_keys());
    LOC_CHK(r, "invalid normal release template");
    r = validate_escrow_release(rtb.tx_burn_template,  false, cpd, ms, ms_id, n, wti.tx, m_account.get_keys());
    LOC_CHK(r, "invalid burn release template");
  }

  uint64_t flags = get_tx_flags(wti.tx);
  LOC_CHK(flags == TX_FLAG_SIGNATURE_MODE_SEPARATE, "invalid tx flags: " << flags);

  uint64_t tx_expiration_time = get_tx_expiration_time(wti.tx);
  LOC_CHK(tx_expiration_time != 0, "no or zero expiration time specified");

  uint64_t tx_unlock_time = get_tx_max_unlock_time(wti.tx);
  LOC_CHK(tx_unlock_time == 0, "non-zero unlock time: " << tx_unlock_time);

#undef LOC_CHK
  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::get_minimum_allowed_fee_for_contract(const crypto::hash& ms_id)
{
  auto it = m_multisig_transfers.find(ms_id);
  if (it == m_multisig_transfers.end())
  {
    WLT_LOG_ERROR("get_minimum_allowed_fee_for_contract called with unknown id: " << ms_id << ", assuming TX_MINIMUM_FEE=" << TX_MINIMUM_FEE);
    return TX_MINIMUM_FEE;
  }
  uint64_t tx_fee = get_tx_fee(it->second.m_ptx_wallet_info->m_tx);
  return std::min(tx_fee, TX_MINIMUM_FEE);
}
//----------------------------------------------------------------------------------------------------
// TODO move here:
// bool wallet2::handle_contract()
//----------------------------------------------------------------------------------------------------
bool wallet2::validate_escrow_cancel_release(const currency::transaction& tx, const wallet_public::wallet_transfer_info& wti, const bc_services::escrow_cancel_templates_body& ectb,
  const std::vector<currency::payload_items_v>& decrypted_items, crypto::hash& ms_id, bc_services::contract_private_details& cpd, const currency::transaction& source_tx,
  size_t source_ms_out_index, const currency::account_keys& b_keys, uint64_t minimum_release_fee) const
{
#define LOC_CHK(cond, mes) WLT_CHECK_AND_ASSERT_MES(cond, false, "Invalid escrow cancel release template: " << mes);

  // (1/5) inputs
  LOC_CHK(tx.vin.size() == 1, "vin size expected to be 1, actual is " << tx.vin.size());
  LOC_CHK(tx.vin.back().type() == typeid(txin_multisig), "input 0 is not txin_multisig");
  const txin_multisig& ms = boost::get<txin_multisig>(tx.vin.back());
  LOC_CHK(source_ms_out_index < source_tx.vout.size(), "internal invariant failed: source_ms_out_index is out of bounds: " << source_ms_out_index);
  const txout_multisig& source_ms_out = boost::get<txout_multisig>(currency::get_tx_out_bare_from_out_v(source_tx.vout[source_ms_out_index]).target);
  LOC_CHK(ms.multisig_out_id == ms_id, "multisig input references to wrong ms output: " << ms.multisig_out_id << ", expected: " << ms_id);
  LOC_CHK(source_ms_out.minimum_sigs == 2, "source multisig output has wrong minimim_sigs: " <<source_ms_out.minimum_sigs << ", expected: 2");
  LOC_CHK(ms.sigs_count == source_ms_out.minimum_sigs, "multisig input has wrong sig_count: " << ms.sigs_count << ", expected: " << source_ms_out.minimum_sigs);
  LOC_CHK(ms.amount == currency::get_tx_out_bare_from_out_v(source_tx.vout[source_ms_out_index]).amount, "multisig input amount: " << ms.amount << " does not match with source tx out amount: " << currency::get_tx_out_bare_from_out_v(source_tx.vout[source_ms_out_index]).amount);
  

  uint64_t min_ms_amount = cpd.amount_a_pledge + cpd.amount_b_pledge + cpd.amount_to_pay + minimum_release_fee;
  LOC_CHK(ms.amount >= min_ms_amount, "multisig input amount " << ms.amount << " is less than contract expected value: " << min_ms_amount << ", a_pledge=" << cpd.amount_a_pledge << ", b_pledge=" << cpd.amount_b_pledge << ", amount_to_pay=" << cpd.amount_to_pay);

  bool r = have_type_in_variant_container<signed_parts>(ms.etc_details);
  LOC_CHK(!r, "multisig input contains signed_parts in etc_details");

  r = have_type_in_variant_container<extra_attachment_info>(ms.etc_details);
  LOC_CHK(!r, "multisig input contains extra_attachment_info in etc_details");

  // (2/5) extra
  uint64_t flags = get_tx_flags(tx);
  LOC_CHK(flags == 0, "invalid tx flags: " << flags);

  uint64_t expiration_time = get_tx_expiration_time(tx);
  LOC_CHK(expiration_time != 0, "tx has zero or not specified expiration time");

  uint64_t unlock_time = get_tx_max_unlock_time(tx);
  LOC_CHK(unlock_time == 0, "tx has non-zero unlock time: " << unlock_time);

  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  size_t cnt = count_type_in_variant_container<tx_service_attachment>(tx.extra);
  r = get_type_in_variant_container(tx.extra, tsa);
  LOC_CHK(r && cnt == 1, "invalid number of tx_service_attachment: " << cnt);
  LOC_CHK(tsa.flags == 0, "invalid tx_service_attachment flags: " << static_cast<size_t>(tsa.flags));
  LOC_CHK(tsa.service_id == BC_ESCROW_SERVICE_ID, "tx_service_attachment has invalid service id: " << tsa.service_id);
  LOC_CHK(tsa.instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL, "tx_service_attachment has invalid instruction: " << tsa.instruction);

  r = validate_attachment_info(tx.extra, tx.attachment, false); // note: having no attachment info for empty attachments is totally OK
  LOC_CHK(r, "there is invalid attachment info in the extra");

  // (3/5) outputs
  crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);
  crypto::key_derivation der = AUTO_VAL_INIT(der);
  r = crypto::generate_key_derivation(tx_pub_key, b_keys.view_secret_key, der);
  LOC_CHK(r, "generate_key_derivation failed");
  uint64_t total_outputs_amount = 0, outputs_to_B_amount = 0;
  for (size_t i = 0; i != tx.vout.size(); ++i)
  {
    VARIANT_SWITCH_BEGIN(tx.vout[i]);
    VARIANT_CASE_CONST(tx_out_bare, o)
      if (o.target.type() == typeid(txout_to_key))
      {
        total_outputs_amount += o.amount;
        const txout_to_key& otk = boost::get<txout_to_key>(o.target);
        crypto::public_key ephemeral_pub_key = AUTO_VAL_INIT(ephemeral_pub_key);
        r = crypto::derive_public_key(der, i, cpd.b_addr.spend_public_key, ephemeral_pub_key);
        LOC_CHK(r, "derive_public_key failed for output #" << i);
        if (otk.key == ephemeral_pub_key)
          outputs_to_B_amount += o.amount;
      }
      else
        LOC_CHK(false, "Invalid output type: " << o.target.type().name());
    VARIANT_CASE_CONST(tx_out_zarcanum, o)
      LOC_CHK(false, "Invalid output type: " << typeid(o).name());
    VARIANT_SWITCH_END();
  }

  LOC_CHK(outputs_to_B_amount >= cpd.amount_b_pledge, "B-addressed outs total amount: " << print_money(outputs_to_B_amount) << " is less than b_pledge: " << print_money(cpd.amount_b_pledge));

  int64_t tx_fee = ms.amount - total_outputs_amount;
  LOC_CHK(tx_fee >= static_cast<int64_t>(minimum_release_fee), "too small fee: " << print_money(tx_fee) << " expected at least " << static_cast<int64_t>(minimum_release_fee));

  // (4/5) attachment

  // nothing to validate


  // (5/5) signatures
  LOC_CHK(tx.signatures.size() == 1, "invalid singatures size: " << tx.signatures.size()); // only 1 input means only 1 signature vector
  VARIANT_SWITCH_BEGIN(tx.signatures[0]);
  VARIANT_CASE_CONST(NLSAG_sig, signature_NLSAG)
  {
    auto& signature = signature_NLSAG.s;
    LOC_CHK(signature.size() == 2, "invalid signature[0] size: " << signature.size()); // it's expected to contain A-party signature and null-sig placeholder
    LOC_CHK(source_ms_out.keys.size() == 2, "internal error: invalid source ms output keys array, size: " << source_ms_out.keys.size());

    size_t a_sign_index = (signature[0] != null_sig) ? 0 : 1;

    crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, 0, get_transaction_hash(tx));
    r = crypto::check_signature(tx_hash_for_signature, source_ms_out.keys[a_sign_index], signature[a_sign_index]);
    LOC_CHK(r, "A signature for multisig input is invalid");
  }
  VARIANT_CASE_CONST(ZC_sig, s);
  //@#@
  VARIANT_CASE_THROW_ON_OTHER();
  VARIANT_SWITCH_END();


  return true;
#undef LOC_CHK
}
//----------------------------------------------------------------------------------------------------
bool wallet2::validate_escrow_cancel_proposal(const wallet_public::wallet_transfer_info& wti, const bc_services::escrow_cancel_templates_body& ectb,
  const std::vector<currency::payload_items_v>& decrypted_items, crypto::hash& ms_id, bc_services::contract_private_details& cpd, const currency::transaction& proposal_template_tx)
{
#define LOC_CHK(cond, mes) WLT_CHECK_AND_ASSERT_MES(cond, false, "Invalid escrow cancellation request: " << mes << ". ms id: " << ms_id << ", tx: " << get_transaction_hash(wti.tx));

  const transaction& cancellation_request_tx = wti.tx;

  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  bool r = bc_services::get_first_service_attachment_by_id(decrypted_items, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL, tsa);
  LOC_CHK(r, "BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL was not found");
  LOC_CHK(tsa.flags & TX_SERVICE_ATTACHMENT_ENCRYPT_BODY, "tx_service_attachment was not encrypted, tsa.flags: " << static_cast<size_t>(tsa.flags));

  bool is_a = cpd.a_addr == m_account.get_public_address();
  bool is_b = cpd.b_addr == m_account.get_public_address();
  LOC_CHK(is_a ^ is_b, "internal error: account address: " << get_account_address_as_str(m_account.get_public_address()) << " match neither a_addr: " << get_account_address_as_str(cpd.a_addr) << ", nor b_addr: " << get_account_address_as_str(cpd.b_addr));

  if (is_b)
  {
    // A-party assembles and sends cancel request, so there is no much sense in checking it in case it's A.
    size_t ms_out_idx = get_multisig_out_index(proposal_template_tx.vout);
    uint64_t min_release_fee = get_minimum_allowed_fee_for_contract(ms_id);
    r = validate_escrow_cancel_release(ectb.tx_cancel_template, wti, ectb, decrypted_items, ms_id, cpd, proposal_template_tx, ms_out_idx, m_account.get_keys(), min_release_fee);
    LOC_CHK(r, "invalid cancel release template");
  }

  uint64_t flags = get_tx_flags(wti.tx);
  LOC_CHK(flags == 0, "invalid tx flags: " << flags);

  uint64_t unlock_time = get_tx_max_unlock_time(cancellation_request_tx);
  LOC_CHK(unlock_time == 0, "invalid unlock time: " << unlock_time);

  uint64_t expiration_time = get_tx_expiration_time(cancellation_request_tx);
  LOC_CHK(expiration_time == 0, "invalid expiration time: " << expiration_time);

#undef LOC_CHK
  return true;
}
//----------------------------------------------------------------------------------------------------

} // namespace tools
