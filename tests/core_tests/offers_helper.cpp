// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "offers_helper.h"
#include "random_helper.h"

using namespace epee;
using namespace crypto;
using namespace currency;

const uint64_t default_offer_skip_field_mask = offer_field_fee | offer_field_index_in_tx | offer_field_security | offer_field_stopped | offer_field_timestamp | offer_field_tx_hash | offer_field_tx_original_hash;

void fill_test_offer(bc_services::offer_details& result)
{
  static const uint8_t offer_types[] = { OFFER_TYPE_PRIMARY_TO_TARGET, OFFER_TYPE_TARGET_TO_PRIMARY, OFFER_TYPE_GOODS_TO_PRIMARY, OFFER_TYPE_PRIMARY_TO_GOODS, OFFER_TYPE_UNDEFINED };
  bc_services::offer_details od = AUTO_VAL_INIT(od);
  od.offer_type = crypto::rand<uint8_t>() % sizeof offer_types;
  od.amount_primary = crypto::rand<uint64_t>();
  od.amount_target = crypto::rand<uint64_t>();
  od.bonus = get_random_text(25);
  od.target = get_random_text(25);
  od.location_country = get_random_text(3);
  od.location_city = get_random_text(25);
  od.contacts = get_random_text(25);
  od.comment = get_random_text(2000);
  od.payment_types = get_random_text(25);
  od.deal_option = get_random_text(25);
  od.category = get_random_text(10);
  od.expiration_time = 3;
  od.primary = get_random_text(10);

  result = od;
}

#define COMPARE_OFFER_FIELD(field)   CHECK_AND_ASSERT_MES((skip_mask & offer_field_##field) || (lhs.field == rhs.field), false, "compare_offers failed: " #field " is not equal")
bool compare_offers(const bc_services::offer_details_ex& lhs, const bc_services::offer_details_ex& rhs, uint64_t skip_mask)
{
  COMPARE_OFFER_FIELD(amount_target);
  COMPARE_OFFER_FIELD(amount_primary);
  COMPARE_OFFER_FIELD(bonus);
  COMPARE_OFFER_FIELD(category);
  COMPARE_OFFER_FIELD(comment);
  COMPARE_OFFER_FIELD(contacts);
  COMPARE_OFFER_FIELD(deal_option);
  COMPARE_OFFER_FIELD(expiration_time);
  COMPARE_OFFER_FIELD(fee);
  COMPARE_OFFER_FIELD(index_in_tx);
  COMPARE_OFFER_FIELD(location_city);
  COMPARE_OFFER_FIELD(location_country);
  COMPARE_OFFER_FIELD(offer_type);
  COMPARE_OFFER_FIELD(payment_types);
  COMPARE_OFFER_FIELD(security);
  COMPARE_OFFER_FIELD(stopped);
  COMPARE_OFFER_FIELD(target);
  COMPARE_OFFER_FIELD(timestamp);
  COMPARE_OFFER_FIELD(tx_hash);
  COMPARE_OFFER_FIELD(tx_original_hash);
  return true;
}
#undef COMPARE_OFFER_FIELD
