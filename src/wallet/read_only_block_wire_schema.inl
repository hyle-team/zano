// Copyright (c) 2026 Zano Project
// Finite schema mirror. No unknown-type fallback.
template<> struct schema<currency::account_public_address_old> { static bool read(cursor&, currency::account_public_address_old&, bool); };
template<> struct schema<currency::account_public_address> { static bool read(cursor&, currency::account_public_address&, bool); };
template<> struct schema<currency::gateway_address_ownership_proof> { static bool read(cursor&, currency::gateway_address_ownership_proof&, bool); };
template<> struct schema<currency::extra_attachment_info> { static bool read(cursor&, currency::extra_attachment_info&, bool); };
template<> struct schema<currency::txin_gen> { static bool read(cursor&, currency::txin_gen&, bool); };
template<> struct schema<currency::ref_by_id> { static bool read(cursor&, currency::ref_by_id&, bool); };
template<> struct schema<currency::signed_parts> { static bool read(cursor&, currency::signed_parts&, bool); };
template<> struct schema<currency::txin_to_key> { static bool read(cursor&, currency::txin_to_key&, bool); };
template<> struct schema<currency::txin_dummy> { static bool read(cursor&, currency::txin_dummy&, bool); };
template<> struct schema<currency::txin_multisig> { static bool read(cursor&, currency::txin_multisig&, bool); };
template<> struct schema<currency::txout_multisig> { static bool read(cursor&, currency::txout_multisig&, bool); };
template<> struct schema<currency::tx_out_bare> { static bool read(cursor&, currency::tx_out_bare&, bool); };
template<> struct schema<currency::txin_gateway> { static bool read(cursor&, currency::txin_gateway&, bool); };
template<> struct schema<currency::tx_out_gateway> { static bool read(cursor&, currency::tx_out_gateway&, bool); };
template<> struct schema<currency::txin_zc_input> { static bool read(cursor&, currency::txin_zc_input&, bool); };
template<> struct schema<currency::tx_out_zarcanum> { static bool read(cursor&, currency::tx_out_zarcanum&, bool); };
template<> struct schema<currency::zarcanum_tx_data_v1> { static bool read(cursor&, currency::zarcanum_tx_data_v1&, bool); };
template<> struct schema<currency::zc_asset_surjection_proof> { static bool read(cursor&, currency::zc_asset_surjection_proof&, bool); };
template<> struct schema<currency::zc_outs_range_proof> { static bool read(cursor&, currency::zc_outs_range_proof&, bool); };
template<> struct schema<currency::ZC_sig> { static bool read(cursor&, currency::ZC_sig&, bool); };
template<> struct schema<currency::zc_balance_proof> { static bool read(cursor&, currency::zc_balance_proof&, bool); };
template<> struct schema<currency::zc_gw_balance_proof> { static bool read(cursor&, currency::zc_gw_balance_proof&, bool); };
template<> struct schema<currency::zarcanum_sig> { static bool read(cursor&, currency::zarcanum_sig&, bool); };
template<> struct schema<currency::tx_comment> { static bool read(cursor&, currency::tx_comment&, bool); };
template<> struct schema<currency::tx_payer_old> { static bool read(cursor&, currency::tx_payer_old&, bool); };
template<> struct schema<currency::tx_payer> { static bool read(cursor&, currency::tx_payer&, bool); };
template<> struct schema<currency::tx_receiver_old> { static bool read(cursor&, currency::tx_receiver_old&, bool); };
template<> struct schema<currency::tx_receiver> { static bool read(cursor&, currency::tx_receiver&, bool); };
template<> struct schema<currency::tx_crypto_checksum> { static bool read(cursor&, currency::tx_crypto_checksum&, bool); };
template<> struct schema<currency::tx_derivation_hint> { static bool read(cursor&, currency::tx_derivation_hint&, bool); };
template<> struct schema<currency::tx_service_attachment> { static bool read(cursor&, currency::tx_service_attachment&, bool); };
template<> struct schema<currency::extra_user_data> { static bool read(cursor&, currency::extra_user_data&, bool); };
template<> struct schema<currency::extra_alias_entry_base_old> { static bool read(cursor&, currency::extra_alias_entry_base_old&, bool); };
template<> struct schema<currency::extra_alias_entry_old> { static bool read(cursor&, currency::extra_alias_entry_old&, bool); };
template<> struct schema<currency::extra_alias_entry_base> { static bool read(cursor&, currency::extra_alias_entry_base&, bool); };
template<> struct schema<currency::extra_alias_entry> { static bool read(cursor&, currency::extra_alias_entry&, bool); };
template<> struct schema<currency::dummy> { static bool read(cursor&, currency::dummy&, bool); };
template<> struct schema<currency::asset_descriptor_base> { static bool read(cursor&, currency::asset_descriptor_base&, bool); };
template<> struct schema<currency::asset_descriptor_operation> { static bool read(cursor&, currency::asset_descriptor_operation&, bool); };
template<> struct schema<currency::asset_operation_proof> { static bool read(cursor&, currency::asset_operation_proof&, bool); };
template<> struct schema<currency::asset_operation_ownership_proof> { static bool read(cursor&, currency::asset_operation_ownership_proof&, bool); };
template<> struct schema<currency::asset_operation_ownership_proof_eth> { static bool read(cursor&, currency::asset_operation_ownership_proof_eth&, bool); };
template<> struct schema<currency::extra_padding> { static bool read(cursor&, currency::extra_padding&, bool); };
template<> struct schema<currency::etc_tx_details_unlock_time> { static bool read(cursor&, currency::etc_tx_details_unlock_time&, bool); };
template<> struct schema<currency::etc_tx_details_unlock_time2> { static bool read(cursor&, currency::etc_tx_details_unlock_time2&, bool); };
template<> struct schema<currency::etc_tx_details_expiration_time> { static bool read(cursor&, currency::etc_tx_details_expiration_time&, bool); };
template<> struct schema<currency::etc_tx_time> { static bool read(cursor&, currency::etc_tx_time&, bool); };
template<> struct schema<currency::etc_tx_details_flags> { static bool read(cursor&, currency::etc_tx_details_flags&, bool); };
template<> struct schema<currency::etc_tx_flags16_t> { static bool read(cursor&, currency::etc_tx_flags16_t&, bool); };
template<> struct schema<currency::etc_coinbase_block_cumulative_size> { static bool read(cursor&, currency::etc_coinbase_block_cumulative_size&, bool); };
template<> struct schema<currency::gateway_address_descriptor_base> { static bool read(cursor&, currency::gateway_address_descriptor_base&, bool); };
template<> struct schema<currency::gateway_address_descriptor_operation_register> { static bool read(cursor&, currency::gateway_address_descriptor_operation_register&, bool); };
template<> struct schema<currency::gateway_address_descriptor_operation_update> { static bool read(cursor&, currency::gateway_address_descriptor_operation_update&, bool); };
template<> struct schema<currency::gateway_address_descriptor_operation> { static bool read(cursor&, currency::gateway_address_descriptor_operation&, bool); };
template<> struct schema<currency::NLSAG_sig> { static bool read(cursor&, currency::NLSAG_sig&, bool); };
template<> struct schema<currency::void_sig> { static bool read(cursor&, currency::void_sig&, bool); };
template<> struct schema<currency::gateway_sig> { static bool read(cursor&, currency::gateway_sig&, bool); };
template<> struct schema<currency::tx_out_zarcanum_v1> { static bool read(cursor&, currency::tx_out_zarcanum_v1&, bool); };
template<> struct schema<crypto::bpp_signature_serialized> { static bool read(cursor&, crypto::bpp_signature_serialized&, bool); };
template<> struct schema<crypto::bppe_signature_serialized> { static bool read(cursor&, crypto::bppe_signature_serialized&, bool); };
template<> struct schema<crypto::CLSAG_GG_signature_serialized> { static bool read(cursor&, crypto::CLSAG_GG_signature_serialized&, bool); };
template<> struct schema<crypto::CLSAG_GGX_signature_serialized> { static bool read(cursor&, crypto::CLSAG_GGX_signature_serialized&, bool); };
template<> struct schema<crypto::CLSAG_GGXXG_signature_serialized> { static bool read(cursor&, crypto::CLSAG_GGXXG_signature_serialized&, bool); };
template<> struct schema<crypto::vector_UG_aggregation_proof_serialized> { static bool read(cursor&, crypto::vector_UG_aggregation_proof_serialized&, bool); };
template<> struct schema<crypto::linear_composition_proof_s> { static bool read(cursor&, crypto::linear_composition_proof_s&, bool); };
template<> struct schema<crypto::generic_schnorr_sig_s> { static bool read(cursor&, crypto::generic_schnorr_sig_s&, bool); };
template<> struct schema<crypto::generic_double_schnorr_sig_s> { static bool read(cursor&, crypto::generic_double_schnorr_sig_s&, bool); };
template<> struct schema<crypto::generic_linear_composition_and_schnorr_sig_s> { static bool read(cursor&, crypto::generic_linear_composition_and_schnorr_sig_s&, bool); };
template<> struct schema<crypto::BGE_proof_s> { static bool read(cursor&, crypto::BGE_proof_s&, bool); };

// Field order from currency_core/currency_basic.h
inline bool schema<currency::account_public_address_old>::read(cursor& c, currency::account_public_address_old& value, bool keep) {

      if (!c.field(value.spend_public_key, keep)) return false;
      if (!c.field(value.view_public_key, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::account_public_address>::read(cursor& c, currency::account_public_address& value, bool keep) {

      if (!c.field(value.spend_public_key, keep)) return false;
      if (!c.field(value.view_public_key, keep)) return false;
      if (!c.field(value.flags, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::gateway_address_ownership_proof>::read(cursor& c, currency::gateway_address_ownership_proof& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.sign, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::extra_attachment_info>::read(cursor& c, currency::extra_attachment_info& value, bool keep) {

      if (!c.varint(value.sz)) return false;
      if (!c.field(value.hsh, keep)) return false;
      if (!c.varint(value.cnt)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::txin_gen>::read(cursor& c, currency::txin_gen& value, bool keep) {

      if (!c.varint(value.height)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::ref_by_id>::read(cursor& c, currency::ref_by_id& value, bool keep) {

      if (!c.field(value.tx_id, keep)) return false;
      if (!c.varint(value.n)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::signed_parts>::read(cursor& c, currency::signed_parts& value, bool keep) {

      if (!c.varint(value.n_outs)) return false;
      if (!c.varint(value.n_extras)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::txin_to_key>::read(cursor& c, currency::txin_to_key& value, bool keep) {

      if (!c.varint(value.amount)) return false;
      if (!c.field(value.key_offsets, keep)) return false; 
      if (!c.field(value.k_image, keep)) return false;
      if (!c.field(value.etc_details, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::txin_dummy>::read(cursor& c, currency::txin_dummy& value, bool keep) {

    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::txin_multisig>::read(cursor& c, currency::txin_multisig& value, bool keep) {

      if (!c.varint(value.amount)) return false;
      if (!c.field(value.multisig_out_id, keep)) return false;
      if (!c.varint(value.sigs_count)) return false;
      if (!c.field(value.etc_details, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::txout_multisig>::read(cursor& c, currency::txout_multisig& value, bool keep) {

      if (!c.varint(value.minimum_sigs)) return false;
      if (!c.field(value.keys, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_out_bare>::read(cursor& c, currency::tx_out_bare& value, bool keep) {

      if (!c.varint(value.amount)) return false;
      if (!c.field(value.target, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::txin_gateway>::read(cursor& c, currency::txin_gateway& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.gateway_addr, keep)) return false;
      if (!c.field(value.asset_id, keep)) return false;
      if (!c.varint(value.amount)) return false;
      if (!c.varint(value.version)) return false; 
    
if (value.version != 0) c.canonical_invalid = true;

return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_out_gateway>::read(cursor& c, currency::tx_out_gateway& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.gateway_addr, keep)) return false;
      if (!c.field(value.asset_id, keep)) return false;
      if (!c.varint(value.amount)) return false;
      if (!c.field(value.payment_id, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::txin_zc_input>::read(cursor& c, currency::txin_zc_input& value, bool keep) {

      if (!c.field(value.key_offsets, keep)) return false; 
      if (!c.field(value.k_image, keep)) return false;
      if (!c.field(value.etc_details, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_out_zarcanum>::read(cursor& c, currency::tx_out_zarcanum& value, bool keep) {
if (!c.varint(value.version) || value.version > TX_OUT_ZARCANUM_CURRENT_VERSION) return false;

      if (!c.field(value.stealth_address, keep)) return false;
      if (!c.field(value.concealing_point, keep)) return false;
      if (!c.field(value.amount_commitment, keep)) return false;
      if (!c.field(value.blinded_asset_id, keep)) return false;
      if (!c.field(value.encrypted_amount, keep)) return false;
      if (!c.field(value.encrypted_payment_id, keep)) return false;
      if (!c.field(value.mix_attr, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::zarcanum_tx_data_v1>::read(cursor& c, currency::zarcanum_tx_data_v1& value, bool keep) {

      if (!c.field(value.fee, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::zc_asset_surjection_proof>::read(cursor& c, currency::zc_asset_surjection_proof& value, bool keep) {

      if (!c.field(value.bge_proofs, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::zc_outs_range_proof>::read(cursor& c, currency::zc_outs_range_proof& value, bool keep) {

      if (!c.field(value.bpp, keep)) return false;
      if (!c.field(value.aggregation_proof, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::ZC_sig>::read(cursor& c, currency::ZC_sig& value, bool keep) {

      if (!c.field(value.pseudo_out_amount_commitment, keep)) return false;
      if (!c.field(value.pseudo_out_blinded_asset_id, keep)) return false;
      if (!c.field(value.clsags_ggx, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::zc_balance_proof>::read(cursor& c, currency::zc_balance_proof& value, bool keep) {

      if (!c.field(value.dss, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::zc_gw_balance_proof>::read(cursor& c, currency::zc_gw_balance_proof& value, bool keep) {

      if (!c.field(value.lcss, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::zarcanum_sig>::read(cursor& c, currency::zarcanum_sig& value, bool keep) {

      if (!c.field(value.d, keep)) return false;
      if (!c.field(value.C, keep)) return false;
      if (!c.field(value.C_prime, keep)) return false;;
      if (!c.field(value.E, keep)) return false;;
      if (!c.field(value.c, keep)) return false;;
      if (!c.field(value.y0, keep)) return false;;
      if (!c.field(value.y1, keep)) return false;;
      if (!c.field(value.y2, keep)) return false;;
      if (!c.field(value.y3, keep)) return false;;
      if (!c.field(value.y4, keep)) return false;;
      if (!c.field((crypto::bppe_signature_serialized&)value.E_range_proof, keep)) return false;;
      if (!c.field(value.pseudo_out_amount_commitment, keep)) return false;;
      if (!c.field((crypto::CLSAG_GGXXG_signature_serialized&)value.clsag_ggxxg, keep)) return false;;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_comment>::read(cursor& c, currency::tx_comment& value, bool keep) {

      if (!c.field(value.comment, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_payer_old>::read(cursor& c, currency::tx_payer_old& value, bool keep) {

      if (!c.field(value.acc_addr, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_payer>::read(cursor& c, currency::tx_payer& value, bool keep) {

      if (!c.field(value.acc_addr, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_receiver_old>::read(cursor& c, currency::tx_receiver_old& value, bool keep) {

      if (!c.field(value.acc_addr, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_receiver>::read(cursor& c, currency::tx_receiver& value, bool keep) {

      if (!c.field(value.acc_addr, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_crypto_checksum>::read(cursor& c, currency::tx_crypto_checksum& value, bool keep) {

      if (!c.field(value.encrypted_key_derivation, keep)) return false;
      if (!c.field(value.derivation_hash, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_derivation_hint>::read(cursor& c, currency::tx_derivation_hint& value, bool keep) {

      if (!c.field(value.msg, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::tx_service_attachment>::read(cursor& c, currency::tx_service_attachment& value, bool keep) {

      if (!c.field(value.service_id, keep)) return false;
      if (!c.field(value.instruction, keep)) return false;
      if (!c.field(value.body, keep)) return false;
      if (!c.field(value.security, keep)) return false;
      if (!c.field(value.flags, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::extra_user_data>::read(cursor& c, currency::extra_user_data& value, bool keep) {

      if (!c.field(value.buff, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::extra_alias_entry_base_old>::read(cursor& c, currency::extra_alias_entry_base_old& value, bool keep) {

      if (!c.field(value.m_address, keep)) return false;
      if (!c.field(value.m_text_comment, keep)) return false;
      if (!c.field(value.m_view_key, keep)) return false;
      if (!c.field(value.m_sign, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::extra_alias_entry_old>::read(cursor& c, currency::extra_alias_entry_old& value, bool keep) {

      if (!c.field(value.m_alias, keep)) return false;
      if (!c.field(*static_cast<extra_alias_entry_base_old*>(&value), keep)) return false;
   
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::extra_alias_entry_base>::read(cursor& c, currency::extra_alias_entry_base& value, bool keep) {

      if (!c.field(value.m_address, keep)) return false;
      if (!c.field(value.m_text_comment, keep)) return false;
      if (!c.field(value.m_view_key, keep)) return false;
      if (!c.field(value.m_sign, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::extra_alias_entry>::read(cursor& c, currency::extra_alias_entry& value, bool keep) {

      if (!c.field(value.m_alias, keep)) return false;
      if (!c.field(*static_cast<extra_alias_entry_base*>(&value), keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::dummy>::read(cursor& c, currency::dummy& value, bool keep) {
 
      
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::asset_descriptor_base>::read(cursor& c, currency::asset_descriptor_base& value, bool keep) {
if (!c.varint(value.version) || value.version > ASSET_DESCRIPTOR_BASE_LAST_VER) return false;

      if (!c.field(value.total_max_supply, keep)) return false;
      if (!c.field(value.current_supply, keep)) return false;
      if (!c.field(value.decimal_point, keep)) return false;
      if (!c.field(value.ticker, keep)) return false;
      if (!c.field(value.full_name, keep)) return false;
      if (!c.field(value.meta_info, keep)) return false;
      if (!c.field(value.owner, keep)) return false;
      if (!c.field(value.hidden_supply, keep)) return false;
      if (value.version < 1) return true;
      if (!c.field(value.owner_eth_pub_key, keep)) return false;
      if (value.version < 2) return true;
      if (!c.field(value.etc, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::asset_descriptor_operation>::read(cursor& c, currency::asset_descriptor_operation& value, bool keep) {
if (!c.varint(value.version) || value.version > ASSET_DESCRIPTOR_OPERATION_LAST_VER) return false;

      if (value.version == 0) return c.transition<asset_descriptor_operation_v1>(value, keep);
      if (value.version == 1) return c.transition<asset_descriptor_operation_v1>(value, keep);
      if (!c.field(value.operation_type, keep)) return false;
      if (!c.field(value.opt_amount_commitment, keep)) return false;
      if (!c.field(value.opt_asset_id, keep)) return false;
      if (!c.field(value.opt_descriptor, keep)) return false;
      if (!c.field(value.opt_amount, keep)) return false;
      if (!c.field(value.opt_asset_id_salt, keep)) return false;
      if (!c.field(value.etc, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::asset_operation_proof>::read(cursor& c, currency::asset_operation_proof& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.opt_amount_commitment_composition_proof, keep)) return false;
      if (!c.field(value.opt_amount_commitment_g_proof, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::asset_operation_ownership_proof>::read(cursor& c, currency::asset_operation_ownership_proof& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.gss, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::asset_operation_ownership_proof_eth>::read(cursor& c, currency::asset_operation_ownership_proof_eth& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.eth_sig, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::extra_padding>::read(cursor& c, currency::extra_padding& value, bool keep) {

      if (!c.field(value.buff, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::etc_tx_details_unlock_time>::read(cursor& c, currency::etc_tx_details_unlock_time& value, bool keep) {

      if (!c.varint(value.v)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::etc_tx_details_unlock_time2>::read(cursor& c, currency::etc_tx_details_unlock_time2& value, bool keep) {

      if (!c.field(value.unlock_time_array, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::etc_tx_details_expiration_time>::read(cursor& c, currency::etc_tx_details_expiration_time& value, bool keep) {

      if (!c.varint(value.v)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::etc_tx_time>::read(cursor& c, currency::etc_tx_time& value, bool keep) {

      if (!c.varint(value.v)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::etc_tx_details_flags>::read(cursor& c, currency::etc_tx_details_flags& value, bool keep) {

      if (!c.varint(value.v)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::etc_tx_flags16_t>::read(cursor& c, currency::etc_tx_flags16_t& value, bool keep) {

      if (!c.field(value.v, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::etc_coinbase_block_cumulative_size>::read(cursor& c, currency::etc_coinbase_block_cumulative_size& value, bool keep) {

      if (!c.varint(value.v)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::gateway_address_descriptor_base>::read(cursor& c, currency::gateway_address_descriptor_base& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.owner_key, keep)) return false;
      if (!c.field(value.etc, keep)) return false;
      if (!c.field(value.meta_info, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::gateway_address_descriptor_operation_register>::read(cursor& c, currency::gateway_address_descriptor_operation_register& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.view_pub_key, keep)) return false;
      if (!c.field(value.descriptor, keep)) return false;
      
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::gateway_address_descriptor_operation_update>::read(cursor& c, currency::gateway_address_descriptor_operation_update& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.address_id, keep)) return false;
      if (!c.field(value.descriptor, keep)) return false;
      
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::gateway_address_descriptor_operation>::read(cursor& c, currency::gateway_address_descriptor_operation& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.operation, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::NLSAG_sig>::read(cursor& c, currency::NLSAG_sig& value, bool keep) {

      if (!c.field(value.s, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::void_sig>::read(cursor& c, currency::void_sig& value, bool keep) {

    
return true;
}
// Field order from currency_core/currency_basic.h
inline bool schema<currency::gateway_sig>::read(cursor& c, currency::gateway_sig& value, bool keep) {
if (!c.varint(value.version) || value.version > 0) return false;

      if (!c.field(value.s, keep)) return false;
    
return true;
}
// Field order from currency_core/currency_basic_backward_comp.inl
inline bool schema<currency::tx_out_zarcanum_v1>::read(cursor& c, currency::tx_out_zarcanum_v1& value, bool keep) {

    if (!c.field(value.stealth_address, keep)) return false;
    if (!c.field(value.concealing_point, keep)) return false;
    if (!c.field(value.amount_commitment, keep)) return false;
    if (!c.field(value.blinded_asset_id, keep)) return false;
    if (!c.field(value.encrypted_amount, keep)) return false;
    if (!c.field(value.mix_attr, keep)) return false;
  
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::bpp_signature_serialized>::read(cursor& c, crypto::bpp_signature_serialized& value, bool keep) {

      if (!c.field(value.L, keep)) return false;
      if (!c.field(value.R, keep)) return false;
      if (!c.field(value.A0, keep)) return false;
      if (!c.field(value.A, keep)) return false;
      if (!c.field(value.B, keep)) return false;
      if (!c.field(value.r, keep)) return false;
      if (!c.field(value.s, keep)) return false;
      if (!c.field(value.delta, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::bppe_signature_serialized>::read(cursor& c, crypto::bppe_signature_serialized& value, bool keep) {

      if (!c.field(value.L, keep)) return false;
      if (!c.field(value.R, keep)) return false;
      if (!c.field(value.A0, keep)) return false;
      if (!c.field(value.A, keep)) return false;
      if (!c.field(value.B, keep)) return false;
      if (!c.field(value.r, keep)) return false;
      if (!c.field(value.s, keep)) return false;
      if (!c.field(value.delta_1, keep)) return false;
      if (!c.field(value.delta_2, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::CLSAG_GG_signature_serialized>::read(cursor& c, crypto::CLSAG_GG_signature_serialized& value, bool keep) {

      if (!c.field(value.c, keep)) return false;
      if (!c.field((std::vector<scalar_t>&)(value.r), keep)) return false;
      if (!c.field(value.K1, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::CLSAG_GGX_signature_serialized>::read(cursor& c, crypto::CLSAG_GGX_signature_serialized& value, bool keep) {

      if (!c.field(value.c, keep)) return false;
      if (!c.field((std::vector<scalar_t>&)(value.r_g), keep)) return false;
      if (!c.field((std::vector<scalar_t>&)(value.r_x), keep)) return false;
      if (!c.field(value.K1, keep)) return false;
      if (!c.field(value.K2, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::CLSAG_GGXXG_signature_serialized>::read(cursor& c, crypto::CLSAG_GGXXG_signature_serialized& value, bool keep) {

      if (!c.field(value.c, keep)) return false;
      if (!c.field((std::vector<scalar_t>&)(value.r_g), keep)) return false;
      if (!c.field((std::vector<scalar_t>&)(value.r_x), keep)) return false;
      if (!c.field(value.K1, keep)) return false;
      if (!c.field(value.K2, keep)) return false;
      if (!c.field(value.K3, keep)) return false;
      if (!c.field(value.K4, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::vector_UG_aggregation_proof_serialized>::read(cursor& c, crypto::vector_UG_aggregation_proof_serialized& value, bool keep) {

      if (!c.field(value.amount_commitments_for_rp_aggregation, keep)) return false;
      if (!c.field((std::vector<scalar_t>&)(value.y0s), keep)) return false;
      if (!c.field((std::vector<scalar_t>&)(value.y1s), keep)) return false;
      if (!c.field(value.c, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::linear_composition_proof_s>::read(cursor& c, crypto::linear_composition_proof_s& value, bool keep) {

      if (!c.field(value.c, keep)) return false;
      if (!c.field(value.y0, keep)) return false;
      if (!c.field(value.y1, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::generic_schnorr_sig_s>::read(cursor& c, crypto::generic_schnorr_sig_s& value, bool keep) {

      if (!c.field(value.c, keep)) return false;
      if (!c.field(value.y, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::generic_double_schnorr_sig_s>::read(cursor& c, crypto::generic_double_schnorr_sig_s& value, bool keep) {

      if (!c.field(value.c, keep)) return false;
      if (!c.field(value.y0, keep)) return false;
      if (!c.field(value.y1, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::generic_linear_composition_and_schnorr_sig_s>::read(cursor& c, crypto::generic_linear_composition_and_schnorr_sig_s& value, bool keep) {

      if (!c.field(value.c, keep)) return false;
      if (!c.field(value.y0, keep)) return false;
      if (!c.field(value.y1, keep)) return false;
      if (!c.field(value.y2, keep)) return false;
    
return true;
}
// Field order from common/crypto_serialization.h
inline bool schema<crypto::BGE_proof_s>::read(cursor& c, crypto::BGE_proof_s& value, bool keep) {

      if (!c.field(value.A, keep)) return false;
      if (!c.field(value.B, keep)) return false;
      if (!c.field(value.Pk, keep)) return false;
      if (!c.field((std::vector<scalar_t>&)(value.f), keep)) return false;
      if (!c.field(value.y, keep)) return false;
      if (!c.field(value.z, keep)) return false;
    
return true;
}
