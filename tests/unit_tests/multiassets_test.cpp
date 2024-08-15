// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <iostream>
#include <cctype>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "gtest/gtest.h"
#include "serialization/serialization.h"
#include "common/crypto_serialization.h"
#include "serialization/keyvalue_serialization.h" // epee key-value serialization
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"
#include "common/boost_serialization_helper.h"
#include "storages/portable_storage_template_helper.h"

namespace currency
{
  struct asset_descriptor_base_v0;
  struct asset_descriptor_operation_v0;
}

// develop, commit 0c90262e8a1c4e5e5d052f8db84c60a36691414d
struct currency::asset_descriptor_base_v0
{
  uint64_t            total_max_supply = 0;
  uint64_t            current_supply = 0;
  uint8_t             decimal_point = 0;
  std::string         ticker;
  std::string         full_name;
  std::string         meta_info;
  crypto::public_key  owner = currency::null_pkey; // consider premultipling by 1/8
  bool                hidden_supply = false;
  uint8_t             version = 0;

  BEGIN_VERSIONED_SERIALIZE(0, version)
    FIELD(total_max_supply)
    FIELD(current_supply)
    FIELD(decimal_point)
    FIELD(ticker)
    FIELD(full_name)
    FIELD(meta_info)
    FIELD(owner)
    FIELD(hidden_supply)
  END_SERIALIZE()

  BEGIN_BOOST_SERIALIZATION()
    BOOST_SERIALIZE(total_max_supply)
    BOOST_SERIALIZE(current_supply)
    BOOST_SERIALIZE(decimal_point)
    BOOST_SERIALIZE(ticker)
    BOOST_SERIALIZE(full_name)
    BOOST_SERIALIZE(meta_info)
    BOOST_SERIALIZE(owner)
    BOOST_SERIALIZE(hidden_supply)
  END_BOOST_SERIALIZATION()

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(total_max_supply)  DOC_DSCR("Maximum possible supply for given asset, can't be changed after deployment") DOC_EXMP(1000000000000000000)   DOC_END
    KV_SERIALIZE(current_supply)    DOC_DSCR("Currently emitted supply for given asset (ignored for REGISTER operation)") DOC_EXMP(500000000000000000)   DOC_END
    KV_SERIALIZE(decimal_point)     DOC_DSCR("Decimal point")                       DOC_EXMP(12)                        DOC_END
    KV_SERIALIZE(ticker)            DOC_DSCR("Ticker associated with asset")        DOC_EXMP("ZUSD")                    DOC_END
    KV_SERIALIZE(full_name)         DOC_DSCR("Full name of the asset")              DOC_EXMP("Zano wrapped USD")        DOC_END
    KV_SERIALIZE(meta_info)         DOC_DSCR("Any other information assetiaded with asset in a free form")              DOC_EXMP("Stable and private")        DOC_END
    KV_SERIALIZE_POD_AS_HEX_STRING(owner) DOC_DSCR("Owner's key, used only for EMIT and UPDATE validation, could be changed by transferring asset ownership")  DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8")        DOC_END
    KV_SERIALIZE(hidden_supply)    DOC_DSCR("This one reserved for future use, will be documented later") DOC_END
  END_KV_SERIALIZE_MAP()

  operator currency::asset_descriptor_base() const
  {
    currency::asset_descriptor_base asset_descriptor{};
    asset_descriptor.total_max_supply = total_max_supply;
    asset_descriptor.current_supply = current_supply;
    asset_descriptor.decimal_point = decimal_point;
    asset_descriptor.ticker = ticker;
    asset_descriptor.full_name = full_name;
    asset_descriptor.meta_info = meta_info;
    asset_descriptor.owner = owner;
    asset_descriptor.hidden_supply = hidden_supply;
    asset_descriptor.version = version;
    return asset_descriptor;
  }
};

struct currency::asset_descriptor_operation_v0
{
  uint8_t                             operation_type = ASSET_DESCRIPTOR_OPERATION_UNDEFINED;
  asset_descriptor_base_v0            descriptor;
  crypto::public_key                  amount_commitment; // premultiplied by 1/8
  boost::optional<crypto::public_key> opt_asset_id; // target asset_id - for update/emit
  uint8_t                             verion = ASSET_DESCRIPTOR_OPERATION_STRUCTURE_VER;

  BEGIN_VERSIONED_SERIALIZE(ASSET_DESCRIPTOR_OPERATION_STRUCTURE_VER, verion)
    FIELD(operation_type)
    FIELD(descriptor)
    FIELD(amount_commitment)
    END_VERSION_UNDER(1)
    FIELD(opt_asset_id)
  END_SERIALIZE()

  BEGIN_BOOST_SERIALIZATION()
    BOOST_SERIALIZE(operation_type)
    BOOST_SERIALIZE(descriptor)
    BOOST_SERIALIZE(amount_commitment)
    BOOST_END_VERSION_UNDER(1)
    BOOST_SERIALIZE(opt_asset_id)
  END_BOOST_SERIALIZATION()

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(operation_type)    DOC_DSCR("Asset operation type identifier") DOC_EXMP(1) DOC_END
    KV_SERIALIZE(descriptor)        DOC_DSCR("Asset descriptor") DOC_EXMP_AUTO() DOC_END
    KV_SERIALIZE_POD_AS_HEX_STRING(amount_commitment) DOC_DSCR("Amount commitment") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
    KV_SERIALIZE_POD_AS_HEX_STRING(opt_asset_id)      DOC_DSCR("ID of an asset.") DOC_EXMP("cc4e69455e63f4a581257382191de6856c2156630b3fba0db4bdd73ffcfb36b6") DOC_END
  END_KV_SERIALIZE_MAP()

  operator currency::asset_descriptor_operation() const
  {
    currency::asset_descriptor_operation operation_descriptor{};
    operation_descriptor.operation_type = operation_type;
    operation_descriptor.descriptor = descriptor;
    operation_descriptor.amount_commitment = amount_commitment;
    operation_descriptor.opt_asset_id = opt_asset_id;
    operation_descriptor.verion = verion;

    return operation_descriptor;
  }
};

BOOST_CLASS_VERSION(currency::asset_descriptor_operation_v0, 1);

currency::asset_descriptor_base get_asset_descriptor_for_test(
  const crypto::public_key& owner = currency::null_pkey)
{
  currency::asset_descriptor_base descriptor_base{};
  descriptor_base.total_max_supply = 100;
  descriptor_base.current_supply = 50;
  descriptor_base.decimal_point = 0;
  descriptor_base.ticker = "HLO";
  descriptor_base.full_name = "HELLO_WORLD";
  descriptor_base.meta_info = "Hello, world!";
  descriptor_base.owner = owner;
  descriptor_base.hidden_supply = false;
  descriptor_base.version = 1;

  return descriptor_base;
}

currency::asset_descriptor_base_v0 get_asset_descriptor_v0_for_test(
  const crypto::public_key& owner = currency::null_pkey)
{
  currency::asset_descriptor_base_v0 descriptor_base{};
  descriptor_base.total_max_supply = 100;
  descriptor_base.current_supply = 50;
  descriptor_base.decimal_point = 0;
  descriptor_base.ticker = "HLO";
  descriptor_base.full_name = "HELLO_WORLD";
  descriptor_base.meta_info = "Hello, world!";
  descriptor_base.owner = owner;
  descriptor_base.hidden_supply = false;

  return descriptor_base;
}

currency::asset_descriptor_operation get_asset_descriptor_operation_for_test(
  currency::asset_descriptor_base asset_descriptor,
  std::uint8_t operation = ASSET_DESCRIPTOR_OPERATION_UNDEFINED
)
{
  currency::asset_descriptor_operation descriptor_operation{};
  descriptor_operation.operation_type = operation;
  descriptor_operation.descriptor = asset_descriptor;
  descriptor_operation.amount_commitment = currency::null_pkey;

  return descriptor_operation;
}

currency::asset_descriptor_operation_v0
get_asset_descriptor_operation_v0_for_test(
  currency::asset_descriptor_base_v0 asset_descriptor,
  std::uint8_t operation = ASSET_DESCRIPTOR_OPERATION_UNDEFINED
)
{
  currency::asset_descriptor_operation_v0 descriptor_operation{};
  descriptor_operation.operation_type = operation;
  descriptor_operation.descriptor = asset_descriptor;
  descriptor_operation.amount_commitment = currency::null_pkey;

  return descriptor_operation;
}

TEST(multiassets, get_or_calculate_asset_id_register)
{
  bool success{false};

  crypto::point_t pt_public_key{};
  success = pt_public_key.from_string(
    "cf93bead4d2a8d6d174c1752237b2e5208a594b59618683ee50ef209cf1efb19");

  ASSERT_TRUE(success);

  crypto::public_key owner_public_key{};
  pt_public_key.to_public_key(owner_public_key);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  const auto asset_descriptor{get_asset_descriptor_for_test(owner_public_key)};

  const auto asset_operation_descriptor{
    get_asset_descriptor_operation_for_test(asset_descriptor,
      ASSET_DESCRIPTOR_OPERATION_REGISTER)};

  success = currency::get_or_calculate_asset_id(asset_operation_descriptor,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);

  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "6f46324faae448b9e3b96dac94da17be6ab7eaaba398de86d8743042c98bace0"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  const crypto::public_key expected_asset_id_key{
    expected_asset_id_pt.to_public_key()};

  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_emit)
{
  bool success{false};

  crypto::point_t pt_public_key{};
  success = pt_public_key.from_string(
    "edab571c4be9eabfea5e7883036d744c097382eb6f739a914db06f72ba35099d");

  ASSERT_TRUE(success);

  crypto::public_key owner_public_key{};
  pt_public_key.to_public_key(owner_public_key);

  const auto asset_descriptor{
    get_asset_descriptor_for_test(owner_public_key)};

  auto asset_operation_descriptor{
    get_asset_descriptor_operation_for_test(
      asset_descriptor, ASSET_DESCRIPTOR_OPERATION_REGISTER)};

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_operation_descriptor,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);

  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "49a3d6652aaa0b3b77292c534e91ff80de9120aeb6fc1c5edc728047437d667e"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  const crypto::public_key expected_asset_id_key{
    expected_asset_id_pt.to_public_key()};

  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);

  asset_operation_descriptor.opt_asset_id = calculated_asset_id_key;
  asset_operation_descriptor.operation_type = ASSET_DESCRIPTOR_OPERATION_EMIT;

  success = get_or_calculate_asset_id(asset_operation_descriptor,
                                      &calculated_asset_id_pt,
                                      &calculated_asset_id_key);

  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_update)
{
  bool success{false};

  crypto::point_t pt_public_key{};
  success = pt_public_key.from_string(
    "8cb6349f51da6599feeae7c0077293436eb6a5000f0e6e706e77886bb540e2c1");

  ASSERT_TRUE(success);

  crypto::public_key owner_public_key{};
  pt_public_key.to_public_key(owner_public_key);

  const auto asset_descriptor{
    get_asset_descriptor_for_test(owner_public_key)};

  auto asset_operation_descriptor{
    get_asset_descriptor_operation_for_test(
      asset_descriptor, ASSET_DESCRIPTOR_OPERATION_REGISTER)};

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_operation_descriptor,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);

  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "c371f60dd8333298c6aa746b71e1e20527b1ff5e1bed4ea9b5f592fadf90ed6b"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  const crypto::public_key expected_asset_id_key{
    expected_asset_id_pt.to_public_key()};

  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);

  asset_operation_descriptor.opt_asset_id = calculated_asset_id_key;
  asset_operation_descriptor.operation_type = ASSET_DESCRIPTOR_OPERATION_UPDATE;

  success = get_or_calculate_asset_id(asset_operation_descriptor,
                                      &calculated_asset_id_pt,
                                      &calculated_asset_id_key);

  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_public_burn)
{
  bool success{false};

  crypto::point_t pt_public_key{};
  success = pt_public_key.from_string(
    "0c408cf8b7fb808f40593d6eb75890e2ab3d0ccdc7014a7fc6b6ab05163be060");

  ASSERT_TRUE(success);

  crypto::public_key owner_public_key{};
  pt_public_key.to_public_key(owner_public_key);

  const auto asset_descriptor{
    get_asset_descriptor_for_test(owner_public_key)};

  auto asset_operation_descriptor{
    get_asset_descriptor_operation_for_test(
      asset_descriptor, ASSET_DESCRIPTOR_OPERATION_REGISTER)};

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_operation_descriptor,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);

  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "54f3f72c72e5b014ad2b2b9001acef954fe82dd3ed56a38cd9ddc5db57673f8f"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  const crypto::public_key expected_asset_id_key{
    expected_asset_id_pt.to_public_key()};

  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);

  asset_operation_descriptor.opt_asset_id = calculated_asset_id_key;
  asset_operation_descriptor.operation_type =
    ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN;

  success = get_or_calculate_asset_id(asset_operation_descriptor,
                                      &calculated_asset_id_pt,
                                      &calculated_asset_id_key);

  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_undefined)
{
   bool success{false};

  crypto::point_t pt_public_key{};
  success = pt_public_key.from_string(
    "e91b9a73292d6ea46fb3d4f4cc79c34bfb7d14c2e684e58093a2471c92e51c16");

  ASSERT_TRUE(success);

  crypto::public_key owner_public_key{};
  pt_public_key.to_public_key(owner_public_key);

  const auto asset_descriptor{
    get_asset_descriptor_for_test(owner_public_key)};

  auto asset_operation_descriptor{
    get_asset_descriptor_operation_for_test(
      asset_descriptor, ASSET_DESCRIPTOR_OPERATION_REGISTER)};

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_operation_descriptor,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);

  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "979eb706ace2eb83f9125658b23fb352208480cb3b90c43e2df0d298f9754ebc"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  const crypto::public_key expected_asset_id_key{
    expected_asset_id_pt.to_public_key()};

  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);

  asset_operation_descriptor.opt_asset_id = calculated_asset_id_key;
  asset_operation_descriptor.operation_type =
    ASSET_DESCRIPTOR_OPERATION_UNDEFINED;

  success = get_or_calculate_asset_id(asset_operation_descriptor,
                                      &calculated_asset_id_pt,
                                      &calculated_asset_id_key);

  ASSERT_FALSE(success);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_register_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x01', '\x01', '\x00', 'd',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '2',    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x03', 'H',    'L',    'O',    '\x0b', 'H',    'E',
    'L',    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',
    '\x0d', 'H',    'e',    'l',    'l',    'o',    ',',    ' ',    'w',
    'o',    'r',    'l',    'd',    '!',    '\xcf', '\x93', '\xbe', '\xad',
    'M',    '*',    '\x8d', 'm',    '\x17', 'L',    '\x17', 'R',    '#',
    '{',    '.',    'R',    '\x08', '\xa5', '\x94', '\xb5', '\x96', '\x18',
    'h',    '>',    '\xe5', '\x0e', '\xf2', '\x09', '\xcf', '\x1e', '\xfb',
    '\x19', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'o',
    'F',    '2',    'O',    '\xaa', '\xe4', 'H',    '\xb9', '\xe3', '\xb9',
    'm',    '\xac', '\x94', '\xda', '\x17', '\xbe', 'j',    '\xb7', '\xea',
    '\xab', '\xa3', '\x98', '\xde', '\x86', '\xd8', 't',    '0',    'B',
    '\xc9', '\x8b', '\xac', '\xe0'};

  success = t_unserializable_object_from_blob(asset_descriptor_operation,
                                              serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "6f46324faae448b9e3b96dac94da17be6ab7eaaba398de86d8743042c98bace0"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  const crypto::public_key expected_asset_id_key{
    expected_asset_id_pt.to_public_key()};

  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_emit_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x01', '\x02', '\x00', 'd',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '2',    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x03', 'H',    'L',    'O',    '\x0b', 'H',    'E',
    'L',    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',
    '\x0d', 'H',    'e',    'l',    'l',    'o',    ',',    ' ',    'w',
    'o',    'r',    'l',    'd',    '!',    '\xed', '\xab', 'W',    '\x1c',
    'K',    '\xe9', '\xea', '\xbf', '\xea', '^',    'x',    '\x83', '\x03',
    'm',    't',    'L',    '\x09', 's',    '\x82', '\xeb', 'o',    's',
    '\x9a', '\x91', 'M',    '\xb0', 'o',    'r',    '\xba', '5',    '\x09',
    '\x9d', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'I',
    '\xa3', '\xd6', 'e',    '*',    '\xaa', '\x0b', ';',    'w',    ')',
    ',',    'S',    'N',    '\x91', '\xff', '\x80', '\xde', '\x91', ' ',
    '\xae', '\xb6', '\xfc', '\x1c', '^',    '\xdc', 'r',    '\x80', 'G',
    'C',    '}',    'f',    '~'};

  success = t_unserializable_object_from_blob(asset_descriptor_operation,
                                              serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "49a3d6652aaa0b3b77292c534e91ff80de9120aeb6fc1c5edc728047437d667e"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_update_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x01', '\x03', '\x00', 'd',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '2',    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x03', 'H',    'L',    'O',    '\x0b', 'H',    'E',
    'L',    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',
    '\x0d', 'H',    'e',    'l',    'l',    'o',    ',',    ' ',    'w',
    'o',    'r',    'l',    'd',    '!',    '\x8c', '\xb6', '4',    '\x9f',
    'Q',    '\xda', 'e',    '\x99', '\xfe', '\xea', '\xe7', '\xc0', '\x07',
    'r',    '\x93', 'C',    'n',    '\xb6', '\xa5', '\x00', '\x0f', '\x0e',
    'n',    'p',    'n',    'w',    '\x88', 'k',    '\xb5', '@',    '\xe2',
    '\xc1', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\xc3',
    'q',    '\xf6', '\x0d', '\xd8', '3',    '2',    '\x98', '\xc6', '\xaa',
    't',    'k',    'q',    '\xe1', '\xe2', '\x05', '\'',   '\xb1', '\xff',
    '^',    '\x1b', '\xed', 'N',    '\xa9', '\xb5', '\xf5', '\x92', '\xfa',
    '\xdf', '\x90', '\xed', 'k'};

  success = t_unserializable_object_from_blob(asset_descriptor_operation,
                                              serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "c371f60dd8333298c6aa746b71e1e20527b1ff5e1bed4ea9b5f592fadf90ed6b"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_public_burn_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x01', '\x04', '\x00', 'd',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '2',    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x03', 'H',    'L',    'O',    '\x0b', 'H',    'E',
    'L',    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',
    '\x0d', 'H',    'e',    'l',    'l',    'o',    ',',    ' ',    'w',
    'o',    'r',    'l',    'd',    '!',    '\x0c', '@',    '\x8c', '\xf8',
    '\xb7', '\xfb', '\x80', '\x8f', '@',    'Y',    '=',    'n',    '\xb7',
    'X',    '\x90', '\xe2', '\xab', '=',    '\x0c', '\xcd', '\xc7', '\x01',
    'J',    '\x7f', '\xc6', '\xb6', '\xab', '\x05', '\x16', ';',    '\xe0',
    '`',    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'T',
    '\xf3', '\xf7', ',',    'r',    '\xe5', '\xb0', '\x14', '\xad', '+',
    '+',    '\x90', '\x01', '\xac', '\xef', '\x95', 'O',    '\xe8', '-',
    '\xd3', '\xed', 'V',    '\xa3', '\x8c', '\xd9', '\xdd', '\xc5', '\xdb',
    'W',    'g',    '?',    '\x8f'};

  success = t_unserializable_object_from_blob(asset_descriptor_operation,
                                              serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "54f3f72c72e5b014ad2b2b9001acef954fe82dd3ed56a38cd9ddc5db57673f8f"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_undefined_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x01', '\x00', '\x00', 'd',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '2',    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x03', 'H',    'L',    'O',    '\x0b', 'H',    'E',
    'L',    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',
    '\x0d', 'H',    'e',    'l',    'l',    'o',    ',',    ' ',    'w',
    'o',    'r',    'l',    'd',    '!',    '\xe9', '\x1b', '\x9a', 's',
    ')',    '-',    'n',    '\xa4', 'o',    '\xb3', '\xd4', '\xf4', '\xcc',
    'y',    '\xc3', 'K',    '\xfb', '}',    '\x14', '\xc2', '\xe6', '\x84',
    '\xe5', '\x80', '\x93', '\xa2', 'G',    '\x1c', '\x92', '\xe5', '\x1c',
    '\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x97',
    '\x9e', '\xb7', '\x06', '\xac', '\xe2', '\xeb', '\x83', '\xf9', '\x12',
    'V',    'X',    '\xb2', '?',    '\xb3', 'R',    ' ',    '\x84', '\x80',
    '\xcb', ';',    '\x90', '\xc4', '>',    '-',    '\xf0', '\xd2', '\x98',
    '\xf9', 'u',    'N',    '\xbc'};

  success = t_unserializable_object_from_blob(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_FALSE(success);

  const std::string expected_asset_id_str{
    "979eb706ace2eb83f9125658b23fb352208480cb3b90c43e2df0d298f9754ebc"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_register_boost_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's',
    'e',    'r',    'i',    'a',    'l',    'i',    'z',    'a',    't',
    'i',    'o',    'n',    ':',    ':',    'a',    'r',    'c',    'h',
    'i',    'v',    'e',    '\x11', '\x00', '\x04', '\x08', '\x04', '\x08',
    '\x01', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x01', '\x00', '\x00', '\x00', '\x00', '\x00', 'd',    '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '2',    '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', 'H',    'L',    'O',    '\x0b', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'E',    'L',
    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',    '\x0d',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'e',
    'l',    'l',    'o',    ',',    ' ',    'w',    'o',    'r',    'l',
    'd',    '!',    '\x00', '\x00', '\x00', '\x00', '\x00', ' ',    '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\xcf', '\x93', '\xbe',
    '\xad', 'M',    '*',    '\x8d', 'm',    '\x17', 'L',    '\x17', 'R',
    '#',    '{',    '.',    'R',    '\x08', '\xa5', '\x94', '\xb5', '\x96',
    '\x18', 'h',    '>',    '\xe5', '\x0e', '\xf2', '\x09', '\xcf', '\x1e',
    '\xfb', '\x19', '\x00', ' ',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};

  success = tools::unserialize_obj_from_buff(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "6f46324faae448b9e3b96dac94da17be6ab7eaaba398de86d8743042c98bace0"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  const crypto::public_key expected_asset_id_key{
    expected_asset_id_pt.to_public_key()};

  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_emit_boost_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's',
    'e',    'r',    'i',    'a',    'l',    'i',    'z',    'a',    't',
    'i',    'o',    'n',    ':',    ':',    'a',    'r',    'c',    'h',
    'i',    'v',    'e',    '\x11', '\x00', '\x04', '\x08', '\x04', '\x08',
    '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00',
    '\x02', '\x00', '\x00', '\x00', '\x00', '\x00', 'd',    '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '2',    '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', 'H',    'L',    'O',    '\x0b', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'E',    'L',
    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',    '\x0d',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'e',
    'l',    'l',    'o',    ',',    ' ',    'w',    'o',    'r',    'l',
    'd',    '!',    '\x00', '\x00', '\x00', '\x00', '\x00', ' ',    '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\xed', '\xab', 'W',
    '\x1c', 'K',    '\xe9', '\xea', '\xbf', '\xea', '^',    'x',    '\x83',
    '\x03', 'm',    't',    'L',    '\x09', 's',    '\x82', '\xeb', 'o',
    's',    '\x9a', '\x91', 'M',    '\xb0', 'o',    'r',    '\xba', '5',
    '\x09', '\x9d', '\x00', ' ',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01',
    '\x00', '\x00', '\x00', '\x01', ' ',    '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', 'I',    '\xa3', '\xd6', 'e',    '*',    '\xaa',
    '\x0b', ';',    'w',    ')',    ',',    'S',    'N',    '\x91', '\xff',
    '\x80', '\xde', '\x91', ' ',    '\xae', '\xb6', '\xfc', '\x1c', '^',
    '\xdc', 'r',    '\x80', 'G',    'C',    '}',    'f',    '~'};

  success = tools::unserialize_obj_from_buff(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "49a3d6652aaa0b3b77292c534e91ff80de9120aeb6fc1c5edc728047437d667e"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_update_boost_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's',
    'e',    'r',    'i',    'a',    'l',    'i',    'z',    'a',    't',
    'i',    'o',    'n',    ':',    ':',    'a',    'r',    'c',    'h',
    'i',    'v',    'e',    '\x11', '\x00', '\x04', '\x08', '\x04', '\x08',
    '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00',
    '\x03', '\x00', '\x00', '\x00', '\x00', '\x00', 'd',    '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '2',    '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', 'H',    'L',    'O',    '\x0b', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'E',    'L',
    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',    '\x0d',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'e',
    'l',    'l',    'o',    ',',    ' ',    'w',    'o',    'r',    'l',
    'd',    '!',    '\x00', '\x00', '\x00', '\x00', '\x00', ' ',    '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x8c', '\xb6', '4',
    '\x9f', 'Q',    '\xda', 'e',    '\x99', '\xfe', '\xea', '\xe7', '\xc0',
    '\x07', 'r',    '\x93', 'C',    'n',    '\xb6', '\xa5', '\x00', '\x0f',
    '\x0e', 'n',    'p',    'n',    'w',    '\x88', 'k',    '\xb5', '@',
    '\xe2', '\xc1', '\x00', ' ',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01',
    '\x00', '\x00', '\x00', '\x01', ' ',    '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\xc3', 'q',    '\xf6', '\x0d', '\xd8', '3',
    '2',    '\x98', '\xc6', '\xaa', 't',    'k',    'q',    '\xe1', '\xe2',
    '\x05', '\'',   '\xb1', '\xff', '^',    '\x1b', '\xed', 'N',    '\xa9',
    '\xb5', '\xf5', '\x92', '\xfa', '\xdf', '\x90', '\xed', 'k'};

  success = tools::unserialize_obj_from_buff(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "c371f60dd8333298c6aa746b71e1e20527b1ff5e1bed4ea9b5f592fadf90ed6b"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_public_burn_boost_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's',
    'e',    'r',    'i',    'a',    'l',    'i',    'z',    'a',    't',
    'i',    'o',    'n',    ':',    ':',    'a',    'r',    'c',    'h',
    'i',    'v',    'e',    '\x11', '\x00', '\x04', '\x08', '\x04', '\x08',
    '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00',
    '\x04', '\x00', '\x00', '\x00', '\x00', '\x00', 'd',    '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '2',    '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', 'H',    'L',    'O',    '\x0b', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'E',    'L',
    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',    '\x0d',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'e',
    'l',    'l',    'o',    ',',    ' ',    'w',    'o',    'r',    'l',
    'd',    '!',    '\x00', '\x00', '\x00', '\x00', '\x00', ' ',    '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x0c', '@',    '\x8c',
    '\xf8', '\xb7', '\xfb', '\x80', '\x8f', '@',    'Y',    '=',    'n',
    '\xb7', 'X',    '\x90', '\xe2', '\xab', '=',    '\x0c', '\xcd', '\xc7',
    '\x01', 'J',    '\x7f', '\xc6', '\xb6', '\xab', '\x05', '\x16', ';',
    '\xe0', '`',    '\x00', ' ',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01',
    '\x00', '\x00', '\x00', '\x01', ' ',    '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', 'T',    '\xf3', '\xf7', ',',    'r',    '\xe5',
    '\xb0', '\x14', '\xad', '+',    '+',    '\x90', '\x01', '\xac', '\xef',
    '\x95', 'O',    '\xe8', '-',    '\xd3', '\xed', 'V',    '\xa3', '\x8c',
    '\xd9', '\xdd', '\xc5', '\xdb', 'W',    'g',    '?',    '\x8f'};

  success = tools::unserialize_obj_from_buff(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "54f3f72c72e5b014ad2b2b9001acef954fe82dd3ed56a38cd9ddc5db57673f8f"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_undefined_boost_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's',
    'e',    'r',    'i',    'a',    'l',    'i',    'z',    'a',    't',
    'i',    'o',    'n',    ':',    ':',    'a',    'r',    'c',    'h',
    'i',    'v',    'e',    '\x11', '\x00', '\x04', '\x08', '\x04', '\x08',
    '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00',
    '\x04', '\x00', '\x00', '\x00', '\x00', '\x00', 'd',    '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '2',    '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', 'H',    'L',    'O',    '\x0b', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'E',    'L',
    'L',    'O',    '_',    'W',    'O',    'R',    'L',    'D',    '\x0d',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H',    'e',
    'l',    'l',    'o',    ',',    ' ',    'w',    'o',    'r',    'l',
    'd',    '!',    '\x00', '\x00', '\x00', '\x00', '\x00', ' ',    '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x0c', '@',    '\x8c',
    '\xf8', '\xb7', '\xfb', '\x80', '\x8f', '@',    'Y',    '=',    'n',
    '\xb7', 'X',    '\x90', '\xe2', '\xab', '=',    '\x0c', '\xcd', '\xc7',
    '\x01', 'J',    '\x7f', '\xc6', '\xb6', '\xab', '\x05', '\x16', ';',
    '\xe0', '`',    '\x00', ' ',    '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01',
    '\x00', '\x00', '\x00', '\x01', ' ',    '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', 'T',    '\xf3', '\xf7', ',',    'r',    '\xe5',
    '\xb0', '\x14', '\xad', '+',    '+',    '\x90', '\x01', '\xac', '\xef',
    '\x95', 'O',    '\xe8', '-',    '\xd3', '\xed', 'V',    '\xa3', '\x8c',
    '\xd9', '\xdd', '\xc5', '\xdb', 'W',    'g',    '?',    '\x8f'};

  success = tools::unserialize_obj_from_buff(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_FALSE(success);
}

TEST(multiassets, get_or_calculate_asset_id_register_key_value_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '{',    '\x0d', '\x0a', ' ',    ' ',    '"',    'a',    'm',    'o',
    'u',    'n',    't',    '_',    'c',    'o',    'm',    'm',    'i',
    't',    'm',    'e',    'n',    't',    '"',    ':',    ' ',    '"',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '"',    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',    'd',
    'e',    's',    'c',    'r',    'i',    'p',    't',    'o',    'r',
    '"',    ':',    ' ',    '{',    '\x0d', '\x0a', ' ',    ' ',    ' ',
    ' ',    '"',    'c',    'u',    'r',    'r',    'e',    'n',    't',
    '_',    's',    'u',    'p',    'p',    'l',    'y',    '"',    ':',
    ' ',    '5',    '0',    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',
    ' ',    '"',    'd',    'e',    'c',    'i',    'm',    'a',    'l',
    '_',    'p',    'o',    'i',    'n',    't',    '"',    ':',    ' ',
    '0',    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',
    'f',    'u',    'l',    'l',    '_',    'n',    'a',    'm',    'e',
    '"',    ':',    ' ',    '"',    'H',    'E',    'L',    'L',    'O',
    '_',    'W',    'O',    'R',    'L',    'D',    '"',    ',',    '\x0d',
    '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'h',    'i',    'd',
    'd',    'e',    'n',    '_',    's',    'u',    'p',    'p',    'l',
    'y',    '"',    ':',    ' ',    'f',    'a',    'l',    's',    'e',
    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'm',
    'e',    't',    'a',    '_',    'i',    'n',    'f',    'o',    '"',
    ':',    ' ',    '"',    'H',    'e',    'l',    'l',    'o',    ',',
    ' ',    'w',    'o',    'r',    'l',    'd',    '!',    '"',    ',',
    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'o',    'w',
    'n',    'e',    'r',    '"',    ':',    ' ',    '"',    'c',    'f',
    '9',    '3',    'b',    'e',    'a',    'd',    '4',    'd',    '2',
    'a',    '8',    'd',    '6',    'd',    '1',    '7',    '4',    'c',
    '1',    '7',    '5',    '2',    '2',    '3',    '7',    'b',    '2',
    'e',    '5',    '2',    '0',    '8',    'a',    '5',    '9',    '4',
    'b',    '5',    '9',    '6',    '1',    '8',    '6',    '8',    '3',
    'e',    'e',    '5',    '0',    'e',    'f',    '2',    '0',    '9',
    'c',    'f',    '1',    'e',    'f',    'b',    '1',    '9',    '"',
    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    't',
    'i',    'c',    'k',    'e',    'r',    '"',    ':',    ' ',    '"',
    'H',    'L',    'O',    '"',    ',',    '\x0d', '\x0a', ' ',    ' ',
    ' ',    ' ',    '"',    't',    'o',    't',    'a',    'l',    '_',
    'm',    'a',    'x',    '_',    's',    'u',    'p',    'p',    'l',
    'y',    '"',    ':',    ' ',    '1',    '0',    '0',    '\x0d', '\x0a',
    ' ',    ' ',    '}',    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',
    'o',    'p',    'e',    'r',    'a',    't',    'i',    'o',    'n',
    '_',    't',    'y',    'p',    'e',    '"',    ':',    ' ',    '1',
    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',    'o',    'p',    't',
    '_',    'a',    's',    's',    'e',    't',    '_',    'i',    'd',
    '"',    ':',    ' ',    '"',    '0',    '1',    '6',    'f',    '4',
    '6',    '3',    '2',    '4',    'f',    'a',    'a',    'e',    '4',
    '4',    '8',    'b',    '9',    'e',    '3',    'b',    '9',    '6',
    'd',    'a',    'c',    '9',    '4',    'd',    'a',    '1',    '7',
    'b',    'e',    '6',    'a',    'b',    '7',    'e',    'a',    'a',
    'b',    'a',    '3',    '9',    '8',    'd',    'e',    '8',    '6',
    'd',    '8',    '7',    '4',    '3',    '0',    '4',    '2',    'c',
    '9',    '8',    'b',    'a',    'c',    'e',    '0',    '"',    '\x0d',
    '\x0a', '}'};

  success = epee::serialization::load_t_from_json(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "6f46324faae448b9e3b96dac94da17be6ab7eaaba398de86d8743042c98bace0"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  const crypto::public_key expected_asset_id_key{
    expected_asset_id_pt.to_public_key()};

  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_emit_key_value_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '{',    '\x0d', '\x0a', ' ',    ' ',    '"',    'a',    'm',    'o',
    'u',    'n',    't',    '_',    'c',    'o',    'm',    'm',    'i',
    't',    'm',    'e',    'n',    't',    '"',    ':',    ' ',    '"',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '"',    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',    'd',
    'e',    's',    'c',    'r',    'i',    'p',    't',    'o',    'r',
    '"',    ':',    ' ',    '{',    '\x0d', '\x0a', ' ',    ' ',    ' ',
    ' ',    '"',    'c',    'u',    'r',    'r',    'e',    'n',    't',
    '_',    's',    'u',    'p',    'p',    'l',    'y',    '"',    ':',
    ' ',    '5',    '0',    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',
    ' ',    '"',    'd',    'e',    'c',    'i',    'm',    'a',    'l',
    '_',    'p',    'o',    'i',    'n',    't',    '"',    ':',    ' ',
    '0',    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',
    'f',    'u',    'l',    'l',    '_',    'n',    'a',    'm',    'e',
    '"',    ':',    ' ',    '"',    'H',    'E',    'L',    'L',    'O',
    '_',    'W',    'O',    'R',    'L',    'D',    '"',    ',',    '\x0d',
    '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'h',    'i',    'd',
    'd',    'e',    'n',    '_',    's',    'u',    'p',    'p',    'l',
    'y',    '"',    ':',    ' ',    'f',    'a',    'l',    's',    'e',
    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'm',
    'e',    't',    'a',    '_',    'i',    'n',    'f',    'o',    '"',
    ':',    ' ',    '"',    'H',    'e',    'l',    'l',    'o',    ',',
    ' ',    'w',    'o',    'r',    'l',    'd',    '!',    '"',    ',',
    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'o',    'w',
    'n',    'e',    'r',    '"',    ':',    ' ',    '"',    'e',    'd',
    'a',    'b',    '5',    '7',    '1',    'c',    '4',    'b',    'e',
    '9',    'e',    'a',    'b',    'f',    'e',    'a',    '5',    'e',
    '7',    '8',    '8',    '3',    '0',    '3',    '6',    'd',    '7',
    '4',    '4',    'c',    '0',    '9',    '7',    '3',    '8',    '2',
    'e',    'b',    '6',    'f',    '7',    '3',    '9',    'a',    '9',
    '1',    '4',    'd',    'b',    '0',    '6',    'f',    '7',    '2',
    'b',    'a',    '3',    '5',    '0',    '9',    '9',    'd',    '"',
    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    't',
    'i',    'c',    'k',    'e',    'r',    '"',    ':',    ' ',    '"',
    'H',    'L',    'O',    '"',    ',',    '\x0d', '\x0a', ' ',    ' ',
    ' ',    ' ',    '"',    't',    'o',    't',    'a',    'l',    '_',
    'm',    'a',    'x',    '_',    's',    'u',    'p',    'p',    'l',
    'y',    '"',    ':',    ' ',    '1',    '0',    '0',    '\x0d', '\x0a',
    ' ',    ' ',    '}',    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',
    'o',    'p',    'e',    'r',    'a',    't',    'i',    'o',    'n',
    '_',    't',    'y',    'p',    'e',    '"',    ':',    ' ',    '2',
    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',    'o',    'p',    't',
    '_',    'a',    's',    's',    'e',    't',    '_',    'i',    'd',
    '"',    ':',    ' ',    '"',    '0',    '1',    '4',    '9',    'a',
    '3',    'd',    '6',    '6',    '5',    '2',    'a',    'a',    'a',
    '0',    'b',    '3',    'b',    '7',    '7',    '2',    '9',    '2',
    'c',    '5',    '3',    '4',    'e',    '9',    '1',    'f',    'f',
    '8',    '0',    'd',    'e',    '9',    '1',    '2',    '0',    'a',
    'e',    'b',    '6',    'f',    'c',    '1',    'c',    '5',    'e',
    'd',    'c',    '7',    '2',    '8',    '0',    '4',    '7',    '4',
    '3',    '7',    'd',    '6',    '6',    '7',    'e',    '"',    '\x0d',
    '\x0a', '}'};

  success = epee::serialization::load_t_from_json(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "49a3d6652aaa0b3b77292c534e91ff80de9120aeb6fc1c5edc728047437d667e"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_update_key_value_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '{',    '\x0d', '\x0a', ' ',    ' ',    '"',    'a',    'm',    'o',
    'u',    'n',    't',    '_',    'c',    'o',    'm',    'm',    'i',
    't',    'm',    'e',    'n',    't',    '"',    ':',    ' ',    '"',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '"',    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',    'd',
    'e',    's',    'c',    'r',    'i',    'p',    't',    'o',    'r',
    '"',    ':',    ' ',    '{',    '\x0d', '\x0a', ' ',    ' ',    ' ',
    ' ',    '"',    'c',    'u',    'r',    'r',    'e',    'n',    't',
    '_',    's',    'u',    'p',    'p',    'l',    'y',    '"',    ':',
    ' ',    '5',    '0',    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',
    ' ',    '"',    'd',    'e',    'c',    'i',    'm',    'a',    'l',
    '_',    'p',    'o',    'i',    'n',    't',    '"',    ':',    ' ',
    '0',    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',
    'f',    'u',    'l',    'l',    '_',    'n',    'a',    'm',    'e',
    '"',    ':',    ' ',    '"',    'H',    'E',    'L',    'L',    'O',
    '_',    'W',    'O',    'R',    'L',    'D',    '"',    ',',    '\x0d',
    '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'h',    'i',    'd',
    'd',    'e',    'n',    '_',    's',    'u',    'p',    'p',    'l',
    'y',    '"',    ':',    ' ',    'f',    'a',    'l',    's',    'e',
    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'm',
    'e',    't',    'a',    '_',    'i',    'n',    'f',    'o',    '"',
    ':',    ' ',    '"',    'H',    'e',    'l',    'l',    'o',    ',',
    ' ',    'w',    'o',    'r',    'l',    'd',    '!',    '"',    ',',
    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'o',    'w',
    'n',    'e',    'r',    '"',    ':',    ' ',    '"',    '8',    'c',
    'b',    '6',    '3',    '4',    '9',    'f',    '5',    '1',    'd',
    'a',    '6',    '5',    '9',    '9',    'f',    'e',    'e',    'a',
    'e',    '7',    'c',    '0',    '0',    '7',    '7',    '2',    '9',
    '3',    '4',    '3',    '6',    'e',    'b',    '6',    'a',    '5',
    '0',    '0',    '0',    'f',    '0',    'e',    '6',    'e',    '7',
    '0',    '6',    'e',    '7',    '7',    '8',    '8',    '6',    'b',
    'b',    '5',    '4',    '0',    'e',    '2',    'c',    '1',    '"',
    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    't',
    'i',    'c',    'k',    'e',    'r',    '"',    ':',    ' ',    '"',
    'H',    'L',    'O',    '"',    ',',    '\x0d', '\x0a', ' ',    ' ',
    ' ',    ' ',    '"',    't',    'o',    't',    'a',    'l',    '_',
    'm',    'a',    'x',    '_',    's',    'u',    'p',    'p',    'l',
    'y',    '"',    ':',    ' ',    '1',    '0',    '0',    '\x0d', '\x0a',
    ' ',    ' ',    '}',    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',
    'o',    'p',    'e',    'r',    'a',    't',    'i',    'o',    'n',
    '_',    't',    'y',    'p',    'e',    '"',    ':',    ' ',    '3',
    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',    'o',    'p',    't',
    '_',    'a',    's',    's',    'e',    't',    '_',    'i',    'd',
    '"',    ':',    ' ',    '"',    '0',    '1',    'c',    '3',    '7',
    '1',    'f',    '6',    '0',    'd',    'd',    '8',    '3',    '3',
    '3',    '2',    '9',    '8',    'c',    '6',    'a',    'a',    '7',
    '4',    '6',    'b',    '7',    '1',    'e',    '1',    'e',    '2',
    '0',    '5',    '2',    '7',    'b',    '1',    'f',    'f',    '5',
    'e',    '1',    'b',    'e',    'd',    '4',    'e',    'a',    '9',
    'b',    '5',    'f',    '5',    '9',    '2',    'f',    'a',    'd',
    'f',    '9',    '0',    'e',    'd',    '6',    'b',    '"',    '\x0d',
    '\x0a', '}'};

  success = epee::serialization::load_t_from_json(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_TRUE(success);

  const std::string expected_asset_id_str{
    "c371f60dd8333298c6aa746b71e1e20527b1ff5e1bed4ea9b5f592fadf90ed6b"};

  crypto::point_t expected_asset_id_pt{};
  success = expected_asset_id_pt.from_string(expected_asset_id_str);
  ASSERT_TRUE(success);

  crypto::public_key expected_asset_id_key{};
  expected_asset_id_pt.to_public_key(expected_asset_id_key);
  ASSERT_EQ(calculated_asset_id_pt, expected_asset_id_pt);
  ASSERT_EQ(calculated_asset_id_key, expected_asset_id_key);
}

TEST(multiassets, get_or_calculate_asset_id_undefined_key_value_serialization)
{
  bool success{false};
  currency::asset_descriptor_operation_v0 asset_descriptor_operation{};
  const std::string serialized_asset_descriptor_operation{
    '{',    '\x0d', '\x0a', ' ',    ' ',    '"',    'a',    'm',    'o',
    'u',    'n',    't',    '_',    'c',    'o',    'm',    'm',    'i',
    't',    'm',    'e',    'n',    't',    '"',    ':',    ' ',    '"',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',    '0',
    '0',    '"',    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',    'd',
    'e',    's',    'c',    'r',    'i',    'p',    't',    'o',    'r',
    '"',    ':',    ' ',    '{',    '\x0d', '\x0a', ' ',    ' ',    ' ',
    ' ',    '"',    'c',    'u',    'r',    'r',    'e',    'n',    't',
    '_',    's',    'u',    'p',    'p',    'l',    'y',    '"',    ':',
    ' ',    '5',    '0',    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',
    ' ',    '"',    'd',    'e',    'c',    'i',    'm',    'a',    'l',
    '_',    'p',    'o',    'i',    'n',    't',    '"',    ':',    ' ',
    '0',    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',
    'f',    'u',    'l',    'l',    '_',    'n',    'a',    'm',    'e',
    '"',    ':',    ' ',    '"',    'H',    'E',    'L',    'L',    'O',
    '_',    'W',    'O',    'R',    'L',    'D',    '"',    ',',    '\x0d',
    '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'h',    'i',    'd',
    'd',    'e',    'n',    '_',    's',    'u',    'p',    'p',    'l',
    'y',    '"',    ':',    ' ',    'f',    'a',    'l',    's',    'e',
    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'm',
    'e',    't',    'a',    '_',    'i',    'n',    'f',    'o',    '"',
    ':',    ' ',    '"',    'H',    'e',    'l',    'l',    'o',    ',',
    ' ',    'w',    'o',    'r',    'l',    'd',    '!',    '"',    ',',
    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    'o',    'w',
    'n',    'e',    'r',    '"',    ':',    ' ',    '"',    'e',    '9',
    '1',    'b',    '9',    'a',    '7',    '3',    '2',    '9',    '2',
    'd',    '6',    'e',    'a',    '4',    '6',    'f',    'b',    '3',
    'd',    '4',    'f',    '4',    'c',    'c',    '7',    '9',    'c',
    '3',    '4',    'b',    'f',    'b',    '7',    'd',    '1',    '4',
    'c',    '2',    'e',    '6',    '8',    '4',    'e',    '5',    '8',
    '0',    '9',    '3',    'a',    '2',    '4',    '7',    '1',    'c',
    '9',    '2',    'e',    '5',    '1',    'c',    '1',    '6',    '"',
    ',',    '\x0d', '\x0a', ' ',    ' ',    ' ',    ' ',    '"',    't',
    'i',    'c',    'k',    'e',    'r',    '"',    ':',    ' ',    '"',
    'H',    'L',    'O',    '"',    ',',    '\x0d', '\x0a', ' ',    ' ',
    ' ',    ' ',    '"',    't',    'o',    't',    'a',    'l',    '_',
    'm',    'a',    'x',    '_',    's',    'u',    'p',    'p',    'l',
    'y',    '"',    ':',    ' ',    '1',    '0',    '0',    '\x0d', '\x0a',
    ' ',    ' ',    '}',    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',
    'o',    'p',    'e',    'r',    'a',    't',    'i',    'o',    'n',
    '_',    't',    'y',    'p',    'e',    '"',    ':',    ' ',    '0',
    ',',    '\x0d', '\x0a', ' ',    ' ',    '"',    'o',    'p',    't',
    '_',    'a',    's',    's',    'e',    't',    '_',    'i',    'd',
    '"',    ':',    ' ',    '"',    '0',    '1',    '9',    '7',    '9',
    'e',    'b',    '7',    '0',    '6',    'a',    'c',    'e',    '2',
    'e',    'b',    '8',    '3',    'f',    '9',    '1',    '2',    '5',
    '6',    '5',    '8',    'b',    '2',    '3',    'f',    'b',    '3',
    '5',    '2',    '2',    '0',    '8',    '4',    '8',    '0',    'c',
    'b',    '3',    'b',    '9',    '0',    'c',    '4',    '3',    'e',
    '2',    'd',    'f',    '0',    'd',    '2',    '9',    '8',    'f',
    '9',    '7',    '5',    '4',    'e',    'b',    'c',    '"',    '\x0d',
    '\x0a', '}'};

  success = epee::serialization::load_t_from_json(
    asset_descriptor_operation,
    serialized_asset_descriptor_operation);
  ASSERT_TRUE(success);

  crypto::point_t calculated_asset_id_pt{};
  crypto::public_key calculated_asset_id_key{};

  success = currency::get_or_calculate_asset_id(asset_descriptor_operation,
                                                &calculated_asset_id_pt,
                                                &calculated_asset_id_key);
  ASSERT_FALSE(success);
}
