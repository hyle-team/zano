// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
//
// Alias operation v1: the pure semantic rule for one retained counterparty-free
// pending row. SDK-internal (included by wallet2.cpp and the alias unit only;
// not an installed header). Standard library only, so host checks compile the
// exact production rule over public synthetic descriptors.
//
// The one caller that decodes a real row is wallet2::native_signed_pending_supported
// (wallet2.cpp), the single retained-pending reader used by ordinary payment
// assessment, alias preparation (file_session::typed_pending_supported) and
// wallet2::native_history_replay_state_supported. Around this rule that caller
// proves the exact retained hash, canonical bytes, tx key, checked miner fee,
// flags and expiration, then the burn in the outputs, authenticated holds on
// every selected input (disjoint across all pending rows of both kinds), the
// owned change and the spent/returned/leg balance. This header only decides
// what the core-decoded alias extra and the row's one net leg mean. It never
// releases, skips, replays or re-describes a row as a payment.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace tools { namespace native_session { namespace alias_pending
{
  // Equal to native_wallet_session.h min/max_alias_operation_name_bytes,
  // max_alias_public_comment_bytes, and currency_format_utils.h
  // GUI_TX_TYPE_NEW_ALIAS/GUI_TX_TYPE_UPDATE_ALIAS (static_assert'ed in the SDK units).
  constexpr std::size_t name_min_bytes = 6;
  constexpr std::size_t name_max_bytes = 255;
  constexpr std::size_t comment_max_bytes = 400;
  constexpr uint64_t tx_type_new = 4;
  constexpr uint64_t tx_type_update = 5;

  // Ordinary public names: length and the core alphabet [a-z0-9.-]. Shorter
  // names need the special authority key. The SDK name check additionally
  // requires currency::validate_alias_name (valid_alias_operation_name).
  inline bool name_shape(const std::string& alias) noexcept
  {
    if (alias.size() < name_min_bytes || alias.size() > name_max_bytes) return false;
    for (const char c : alias)
      if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.')) return false;
    return true;
  }

  // RFC 3629 shortest-form UTF-8 (no surrogate, nothing above U+10FFFF, no
  // truncated sequence), at most 400 bytes. Public on chain; unrelated to the
  // app's private on-device payment note.
  inline bool public_comment(const std::string& text) noexcept
  {
    if (text.size() > comment_max_bytes) return false;
    const auto* p = reinterpret_cast<const unsigned char*>(text.data());
    const std::size_t n = text.size();
    const auto tail = [&](std::size_t at) { return at < n && (p[at] & 0xC0) == 0x80; };
    for (std::size_t i = 0; i < n;)
    {
      const unsigned c = p[i];
      if (c < 0x80) { ++i; continue; }
      if (c >= 0xC2 && c <= 0xDF) { if (!tail(i + 1)) return false; i += 2; continue; }
      if (c >= 0xE0 && c <= 0xEF)
      {
        if (i + 1 >= n) return false;
        const unsigned d = p[i + 1];
        const bool second = c == 0xE0 ? (d >= 0xA0 && d <= 0xBF) :
                            c == 0xED ? (d >= 0x80 && d <= 0x9F) : (d >= 0x80 && d <= 0xBF);
        if (!second || !tail(i + 2)) return false;
        i += 3; continue;
      }
      if (c >= 0xF0 && c <= 0xF4)
      {
        if (i + 1 >= n) return false;
        const unsigned d = p[i + 1];
        const bool second = c == 0xF0 ? (d >= 0x90 && d <= 0xBF) :
                            c == 0xF4 ? (d >= 0x80 && d <= 0x8F) : (d >= 0x80 && d <= 0xBF);
        if (!second || !tail(i + 2) || !tail(i + 3)) return false;
        i += 4; continue;
      }
      return false;
    }
    return true;
  }

  // Decoded public facts of one retained counterparty-free pending row. The
  // SDK caller computes every field from the exact canonical transaction and
  // its retained row; nothing here is a caller claim or a node value.
  struct descriptor_v1
  {
    std::string name;                    // extra_alias_entry::m_alias (core-parsed)
    bool core_name_valid = false;        // currency::validate_alias_name(name)
    std::string extra_comment;           // extra_alias_entry::m_text_comment
    std::string row_comment;             // retained row comment
    bool view_key_present = true;        // !m_view_key.empty()
    bool bound_to_own_address = false;   // m_address == this wallet's address
    std::size_t owner_signatures = 0;    // m_sign.size()
    bool owner_signature_valid = false;  // update only: check_signature(update hash, own spend key, m_sign.back())
    uint64_t tx_type = 0;                // currency::get_tx_type(transaction)
    bool leg_outgoing = false;           // the row's one leg: !is_income
    bool leg_native = false;             // the row's one leg: native asset
    uint64_t leg_amount = 0;             // net native leg as the core records it: burn + fee
    uint64_t fee = 0;                    // the transaction's checked miner fee (== row fee)
  };

  enum class verdict_v1
  {
    Register,     // ordinary registration of this wallet's address: burn > 0
    Update,       // current-owner update, owner-signed: burn == 0 (miner fee only)
    Unsupported,  // not an alias operation this build recognizes: fail closed
    Invalid       // claims to be this wallet's update but the owner signature is wrong
  };

  // `burn` is written only for Register/Update. Register: no signature, core
  // type NEW_ALIAS, positive burn. Update: exactly one owner signature that
  // verifies, core type UPDATE_ALIAS, zero burn. Both: an ordinary name, a
  // strict public comment equal to the retained row comment, this wallet's own
  // address, no tracking (view) key and one net outgoing native leg of
  // burn + fee. Ownership transfer, tracking keys, short names and foreign
  // addresses stay Unsupported.
  inline verdict_v1 classify(const descriptor_v1& d, uint64_t& burn) noexcept
  {
    if (!name_shape(d.name) || !d.core_name_valid || !public_comment(d.extra_comment) ||
        d.row_comment != d.extra_comment || d.view_key_present || !d.bound_to_own_address ||
        d.owner_signatures > 1)
      return verdict_v1::Unsupported;
    const bool update = d.owner_signatures == 1;
    if (d.tx_type != (update ? tx_type_update : tx_type_new)) return verdict_v1::Unsupported;
    if (update && !d.owner_signature_valid) return verdict_v1::Invalid;
    if (!d.leg_outgoing || !d.leg_native || d.fee == 0 || d.leg_amount < d.fee) return verdict_v1::Unsupported;
    const uint64_t value = d.leg_amount - d.fee;
    if (update ? value != 0 : value == 0) return verdict_v1::Unsupported;
    burn = value;
    return update ? verdict_v1::Update : verdict_v1::Register;
  }
}}}
