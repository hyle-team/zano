// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

extern const uint64_t default_offer_skip_field_mask;

enum offers_fields {
  offer_field_amount_target      = 1 << 1,
  offer_field_amount_primary     = 1 << 2,
  offer_field_bonus              = 1 << 3,
  offer_field_category           = 1 << 4,
  offer_field_comment            = 1 << 5,
  offer_field_contacts           = 1 << 6,
  offer_field_deal_option        = 1 << 7,
  offer_field_expiration_time    = 1 << 8,
  offer_field_fee                = 1 << 9,
  offer_field_index_in_tx        = 1 << 10,
  offer_field_location_city      = 1 << 11,
  offer_field_location_country   = 1 << 12,
  offer_field_offer_type         = 1 << 13,
  offer_field_payment_types      = 1 << 14,
  offer_field_security           = 1 << 15,
  offer_field_stopped            = 1 << 16,
  offer_field_target             = 1 << 17,
  offer_field_timestamp          = 1 << 18,
  offer_field_tx_hash            = 1 << 19,
  offer_field_tx_original_hash   = 1 << 20
};

void fill_test_offer(bc_services::offer_details& result);
bool compare_offers(const bc_services::offer_details_ex& lhs, const bc_services::offer_details_ex& rhs, uint64_t skip_mask = default_offer_skip_field_mask);
