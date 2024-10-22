// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <iostream>

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

// branch = develop, HEAD = 0c90262e8a1c4e5e5d052f8db84c60a36691414d
struct currency::asset_descriptor_base_v0
{
  uint64_t           total_max_supply;
  uint64_t           current_supply;
  uint8_t            decimal_point;
  std::string        ticker;
  std::string        full_name;
  std::string        meta_info;
  crypto::public_key owner = currency::null_pkey; // consider premultipling by 1/8
  bool               hidden_supply;
  uint8_t            version;

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
    BOOST_END_VERSION_UNDER(1)
  END_BOOST_SERIALIZATION()

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(total_max_supply)        DOC_DSCR("Maximum possible supply for a given asset, cannot be changed after deployment.")                           DOC_EXMP(1000000000000000000)                                                DOC_END
    KV_SERIALIZE(current_supply)          DOC_DSCR("Currently emitted supply for the given asset (ignored for REGISTER operation).")                           DOC_EXMP(500000000000000000)                                                 DOC_END
    KV_SERIALIZE(decimal_point)           DOC_DSCR("Decimal point.")                                                                                           DOC_EXMP(12)                                                                 DOC_END
    KV_SERIALIZE(ticker)                  DOC_DSCR("Ticker associated with the asset.")                                                                        DOC_EXMP("ZABC")                                                             DOC_END
    KV_SERIALIZE(full_name)               DOC_DSCR("Full name of the asset.")                                                                                  DOC_EXMP("Zano wrapped ABC")                                                 DOC_END
    KV_SERIALIZE(meta_info)               DOC_DSCR("Any other information associated with the asset in free form.")                                            DOC_EXMP("Stable and private")                                               DOC_END
    KV_SERIALIZE_POD_AS_HEX_STRING(owner) DOC_DSCR("Owner's key, used only for EMIT and UPDATE validation, can be changed by transferring asset ownership.")   DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
    KV_SERIALIZE(hidden_supply)           DOC_DSCR("This field is reserved for future use and will be documented later.")                                                                                                                   DOC_END
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

// branch = develop, HEAD = 0c90262e8a1c4e5e5d052f8db84c60a36691414d
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
    KV_SERIALIZE(operation_type)                      DOC_DSCR("Asset operation type identifier") DOC_EXMP(1)                                                                  DOC_END
    KV_SERIALIZE(descriptor)                          DOC_DSCR("Asset descriptor")                DOC_EXMP_AUTO()                                                              DOC_END
    KV_SERIALIZE_POD_AS_HEX_STRING(amount_commitment) DOC_DSCR("Amount commitment")               DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
    KV_SERIALIZE_POD_AS_HEX_STRING(opt_asset_id)      DOC_DSCR("ID of an asset.")                 DOC_EXMP("cc4e69455e63f4a581257382191de6856c2156630b3fba0db4bdd73ffcfb36b6") DOC_END
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

BOOST_CLASS_VERSION(currency::asset_descriptor_base_v0, 0);
BOOST_CLASS_VERSION(currency::asset_descriptor_operation_v0, 1);

template<typename asset_descriptor>
static asset_descriptor get_adb(const crypto::public_key& owner = currency::null_pkey)
{
  if (const auto& id{typeid(asset_descriptor)}; id != typeid(currency::asset_descriptor_base) && id != typeid(currency::asset_descriptor_base_v0))
  {
    throw "Unsupported type of an asset descriptor";
  }

  asset_descriptor descriptor{};

  descriptor.total_max_supply = 100;
  descriptor.current_supply = 50;
  descriptor.decimal_point = 0;
  descriptor.ticker = "HLO";
  descriptor.full_name = "HELLO_WORLD";
  descriptor.meta_info = "Hello, world!";
  descriptor.owner = owner;
  descriptor.hidden_supply = false;

  if (typeid(asset_descriptor) == typeid(currency::asset_descriptor_base))
  {
    descriptor.version = 1;
  }

  return descriptor;
}

template<typename asset_operation_descriptor, typename asset_base_descriptor>
static asset_operation_descriptor get_ado(const asset_base_descriptor& base_descriptor, std::uint8_t operation = ASSET_DESCRIPTOR_OPERATION_UNDEFINED,
                                          std::optional<crypto::public_key> asset_id = currency::null_pkey)
{
  if (const auto& id{typeid(asset_operation_descriptor)}; id != typeid(currency::asset_descriptor_operation) && id != typeid(currency::asset_descriptor_operation_v0))
  {
    throw "Unsupported type of an asset operation descriptor";
  }

  if (const auto& id{typeid(asset_base_descriptor)}; id != typeid(currency::asset_descriptor_base) && id != typeid(currency::asset_descriptor_base_v0))
  {
    throw "Unsupported type of an asset base descriptor";
  }

  asset_operation_descriptor descriptor{};
  descriptor.operation_type = operation;
  descriptor.descriptor = base_descriptor;
  descriptor.amount_commitment = currency::null_pkey;

  if (asset_id.has_value())
  {
    descriptor.opt_asset_id = asset_id.value();
  }

    return descriptor;
}

enum class serialization_method : uint8_t
{
  native,
  boost,
  key_value
};

template<typename asset_operation_descriptor>
static std::optional<std::string> serialize(serialization_method method, const asset_operation_descriptor& descriptor)
{
  if (const auto& id{typeid(asset_operation_descriptor)}; id != typeid(currency::asset_descriptor_operation) && id != typeid(currency::asset_descriptor_operation_v0))
  {
    throw "Unsupported type of an asset descriptor operation";
  }

  std::function<bool(const asset_operation_descriptor&, std::string&)> serialization_function{};

  switch (method)
  {
  case serialization_method::native:
    serialization_function = static_cast<bool(*)(const asset_operation_descriptor&, std::string&)>(t_serializable_object_to_blob);
    break;

  case serialization_method::boost:
    serialization_function = static_cast<bool(*)(const asset_operation_descriptor&, std::string&)>(tools::serialize_obj_to_buff);
    break;

  case serialization_method::key_value:
    serialization_function = [](const asset_operation_descriptor& descriptor, std::string& presentation) -> bool
      {
        return epee::serialization::store_t_to_json<asset_operation_descriptor>(descriptor, presentation);
      };
    break;
  }

  if (std::string presentation{}; serialization_function.operator bool() && serialization_function(descriptor, presentation))
  {
    return presentation;
  }

  return {};
}

template<typename asset_operation_descriptor>
static std::optional<asset_operation_descriptor> deserialize(serialization_method method, const std::string& presentation)
{
  if (const auto& id{typeid(asset_operation_descriptor)}; id != typeid(currency::asset_descriptor_operation) && id != typeid(currency::asset_descriptor_operation_v0))
  {
    throw "Unsupported type of an asset descriptor operation";
  }

  std::function<bool(asset_operation_descriptor&, const std::string&)> deserialization_function{};

  switch (method)
  {
  case serialization_method::native:
    deserialization_function = t_unserializable_object_from_blob<asset_operation_descriptor>;
    break;

  case serialization_method::boost:
    deserialization_function = tools::unserialize_obj_from_buff<asset_operation_descriptor>;
    break;

  case serialization_method::key_value:
    deserialization_function = epee::serialization::load_t_from_json<asset_operation_descriptor>;
    break;
  }

  if (asset_operation_descriptor descriptor{}; deserialization_function.operator bool() && deserialization_function(descriptor, presentation))
  {
    return descriptor;
  }

  return {};
}

static std::string get_string_presentation(const std::string& data)
{
  std::string presentation{};

  for (int position{}; position < data.size(); ++position)
  {
    const auto character{static_cast<unsigned char>(data.at(position))};

    if (std::isprint(character))
    {
      presentation += '\'';

      if (character == '\'')
      {
        presentation += '\\';
      }

      presentation += character;
      presentation += '\'';
    }

    else
    {
      std::stringstream stream{};

      presentation += "\'\\x";
      stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint16_t>(character);
      presentation += stream.str();
      presentation += '\'';
    }

    if (position < data.size() - 1)
    {
      presentation += ", ";
    }
  }

  return presentation;
}

TEST(multiassets, get_or_calculate_asset_id_undefined)
{
  bool success{};
  crypto::point_t point_owner{};
  crypto::public_key owner{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  currency::asset_descriptor_base adb{};
  currency::asset_descriptor_operation ado{};
  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};

  success = point_owner.from_string("e91b9a73292d6ea46fb3d4f4cc79c34bfb7d14c2e684e58093a2471c92e51c16");
  ASSERT_TRUE(success);
  owner = point_owner.to_public_key();
  adb = get_adb<decltype(adb)>(owner);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_REGISTER);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("979eb706ace2eb83f9125658b23fb352208480cb3b90c43e2df0d298f9754ebc");
  ASSERT_TRUE(success);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_UNDEFINED, calculated_asset_id);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_FALSE(success);
}

TEST(multiassets, get_or_calculate_asset_id_register)
{
  bool success{};
  crypto::point_t point_owner{};
  crypto::public_key owner{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  currency::asset_descriptor_base adb{};
  currency::asset_descriptor_operation ado{};
  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};

  success = point_owner.from_string("cf93bead4d2a8d6d174c1752237b2e5208a594b59618683ee50ef209cf1efb19");
  ASSERT_TRUE(success);
  point_owner.to_public_key(owner);
  adb = get_adb<decltype(adb)>(owner);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_REGISTER);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("6f46324faae448b9e3b96dac94da17be6ab7eaaba398de86d8743042c98bace0");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, get_or_calculate_asset_id_emit)
{
  bool success{};
  crypto::point_t point_owner{};
  crypto::public_key owner{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  currency::asset_descriptor_base adb{};
  currency::asset_descriptor_operation ado{};
  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};

  success = point_owner.from_string("edab571c4be9eabfea5e7883036d744c097382eb6f739a914db06f72ba35099d");
  ASSERT_TRUE(success);
  owner = point_owner.to_public_key();
  adb = get_adb<decltype(adb)>(owner);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_REGISTER);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("49a3d6652aaa0b3b77292c534e91ff80de9120aeb6fc1c5edc728047437d667e");
  ASSERT_TRUE(success);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_EMIT, calculated_asset_id);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, get_or_calculate_asset_id_update)
{
  bool success{};
  crypto::point_t point_owner{};
  crypto::public_key owner{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  currency::asset_descriptor_base adb{};
  currency::asset_descriptor_operation ado{};
  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};

  success = point_owner.from_string("8cb6349f51da6599feeae7c0077293436eb6a5000f0e6e706e77886bb540e2c1");
  ASSERT_TRUE(success);
  owner = point_owner.to_public_key();
  adb = get_adb<decltype(adb)>(owner);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_REGISTER);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("c371f60dd8333298c6aa746b71e1e20527b1ff5e1bed4ea9b5f592fadf90ed6b");
  ASSERT_TRUE(success);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_UPDATE, calculated_asset_id);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, get_or_calculate_asset_id_public_burn)
{
  bool success{};
  crypto::point_t point_owner{};
  crypto::public_key owner{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  currency::asset_descriptor_base adb{};
  currency::asset_descriptor_operation ado{};
  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};

  success = point_owner.from_string("0c408cf8b7fb808f40593d6eb75890e2ab3d0ccdc7014a7fc6b6ab05163be060");
  ASSERT_TRUE(success);
  owner = point_owner.to_public_key();
  adb = get_adb<decltype(adb)>(owner);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_REGISTER);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("54f3f72c72e5b014ad2b2b9001acef954fe82dd3ed56a38cd9ddc5db57673f8f");
  ASSERT_TRUE(success);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
  ado = get_ado<decltype(ado)>(adb, ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN, calculated_asset_id);
  success = currency::get_or_calculate_asset_id(ado, &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, native_serialization_get_or_calculate_asset_id_undefined)
{
  bool success{};
  const std::string serialized_ado{'\x01', '\x00', '\x00', 'd', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03',
    'H', 'L', 'O', '\x0b', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\xe9', '\x1b', '\x9a', 's', ')', '-', 'n',
    '\xa4', 'o', '\xb3', '\xd4', '\xf4', '\xcc', 'y', '\xc3', 'K', '\xfb', '}', '\x14', '\xc2', '\xe6', '\x84', '\xe5', '\x80', '\x93', '\xa2', 'G', '\x1c', '\x92', '\xe5', '\x1c', '\x16', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x97', '\x9e', '\xb7', '\x06', '\xac', '\xe2', '\xeb', '\x83', '\xf9', '\x12', 'V', 'X', '\xb2', '?', '\xb3', 'R', ' ',
    '\x84', '\x80', '\xcb', ';', '\x90', '\xc4', '>', '-', '\xf0', '\xd2', '\x98', '\xf9', 'u', 'N', '\xbc'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::native, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  ASSERT_EQ(ado.value().descriptor.version, 0);
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_UNDEFINED);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_FALSE(success);
}

TEST(multiassets, native_serialization_get_or_calculate_asset_id_register)
{
  bool success{};
  const std::string serialized_ado{'\x01', '\x01', '\x00', 'd', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03',
    'H', 'L', 'O', '\x0b', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\xcf', '\x93', '\xbe', '\xad', 'M', '*',
    '\x8d', 'm', '\x17', 'L', '\x17', 'R', '#', '{', '.', 'R', '\x08', '\xa5', '\x94', '\xb5', '\x96', '\x18', 'h', '>', '\xe5', '\x0e', '\xf2', '\x09', '\xcf', '\x1e', '\xfb', '\x19', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::native, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  ASSERT_EQ(ado.value().descriptor.version, 0);
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_REGISTER);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("6f46324faae448b9e3b96dac94da17be6ab7eaaba398de86d8743042c98bace0");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, native_serialization_get_or_calculate_asset_id_emit)
{
  bool success{};
  const std::string serialized_ado{'\x01', '\x02', '\x00', 'd', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03',
    'H', 'L', 'O', '\x0b', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\xed', '\xab', 'W', '\x1c', 'K', '\xe9',
    '\xea', '\xbf', '\xea', '^', 'x', '\x83', '\x03', 'm', 't', 'L', '\x09', 's', '\x82', '\xeb', 'o', 's', '\x9a', '\x91', 'M', '\xb0', 'o', 'r', '\xba', '5', '\x09', '\x9d', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'I', '\xa3', '\xd6', 'e', '*', '\xaa', '\x0b', ';', 'w', ')', ',', 'S', 'N', '\x91', '\xff', '\x80', '\xde', '\x91', ' ', '\xae', '\xb6',
    '\xfc', '\x1c', '^', '\xdc', 'r', '\x80', 'G', 'C', '}', 'f', '~'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::native, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  ASSERT_EQ(ado.value().descriptor.version, 0);
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_EMIT);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("49a3d6652aaa0b3b77292c534e91ff80de9120aeb6fc1c5edc728047437d667e");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, native_serialization_get_or_calculate_asset_id_update)
{
  bool success{};
  const std::string serialized_ado{'\x01', '\x03', '\x00', 'd', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03',
    'H', 'L', 'O', '\x0b', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\x8c', '\xb6', '4', '\x9f', 'Q', '\xda',
    'e', '\x99', '\xfe', '\xea', '\xe7', '\xc0', '\x07', 'r', '\x93', 'C', 'n', '\xb6', '\xa5', '\x00', '\x0f', '\x0e', 'n', 'p', 'n', 'w', '\x88', 'k', '\xb5', '@', '\xe2', '\xc1', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\xc3', 'q', '\xf6', '\x0d', '\xd8', '3', '2', '\x98', '\xc6', '\xaa', 't', 'k', 'q', '\xe1', '\xe2', '\x05', '\'', '\xb1', '\xff',
    '^', '\x1b', '\xed', 'N', '\xa9', '\xb5', '\xf5', '\x92', '\xfa', '\xdf', '\x90', '\xed', 'k'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::native, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  ASSERT_EQ(ado.value().descriptor.version, 0);
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_UPDATE);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("c371f60dd8333298c6aa746b71e1e20527b1ff5e1bed4ea9b5f592fadf90ed6b");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, native_serialization_get_or_calculate_asset_id_public_burn)
{
  bool success{};
  const std::string serialized_ado{'\x01', '\x04', '\x00', 'd', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03',
    'H', 'L', 'O', '\x0b', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\x0c', '@', '\x8c', '\xf8', '\xb7', '\xfb',
    '\x80', '\x8f', '@', 'Y', '=', 'n', '\xb7', 'X', '\x90', '\xe2', '\xab', '=', '\x0c', '\xcd', '\xc7', '\x01', 'J', '\x7f', '\xc6', '\xb6', '\xab', '\x05', '\x16', ';', '\xe0', '`', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'T', '\xf3', '\xf7', ',', 'r', '\xe5', '\xb0', '\x14', '\xad', '+', '+', '\x90', '\x01', '\xac', '\xef', '\x95', 'O', '\xe8', '-',
    '\xd3', '\xed', 'V', '\xa3', '\x8c', '\xd9', '\xdd', '\xc5', '\xdb', 'W', 'g', '?', '\x8f'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::native, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  ASSERT_EQ(ado.value().descriptor.version, 0);
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("54f3f72c72e5b014ad2b2b9001acef954fe82dd3ed56a38cd9ddc5db57673f8f");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, boost_serialization_get_or_calculate_asset_id_undefined)
{
  bool success{};
  const std::string serialized_ado{'\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's', 'e', 'r', 'i', 'a', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', ':', ':', 'a', 'r', 'c', 'h', 'i',
    'v', 'e', '\x14', '\x00', '\x04', '\x04', '\x04', '\x08', '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'd', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'L', 'O',
    '\x0b', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'e',
    'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\x00', '\x00', '\x00', '\x00', '\x00', ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\xe9', '\x1b', '\x9a', 's', ')', '-',
    'n', '\xa4', 'o', '\xb3', '\xd4', '\xf4', '\xcc', 'y', '\xc3', 'K', '\xfb', '}', '\x14', '\xc2', '\xe6', '\x84', '\xe5', '\x80', '\x93', '\xa2', 'G', '\x1c', '\x92', '\xe5', '\x1c', '\x16',
    '\x00', ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x01', ' ', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x97', '\x9e', '\xb7', '\x06', '\xac', '\xe2', '\xeb', '\x83', '\xf9', '\x12', 'V', 'X', '\xb2', '?', '\xb3', 'R', ' ', '\x84', '\x80', '\xcb', ';',
    '\x90', '\xc4', '>', '-', '\xf0', '\xd2', '\x98', '\xf9', 'u', 'N', '\xbc'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::boost, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the boost serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_UNDEFINED);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_FALSE(success);
}

TEST(multiassets, boost_serialization_get_or_calculate_asset_id_register)
{
  bool success{};
  const std::string serialized_ado{'\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's', 'e', 'r', 'i', 'a', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', ':', ':', 'a', 'r', 'c', 'h', 'i',
    'v', 'e', '\x14', '\x00', '\x04', '\x04', '\x04', '\x08', '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x00', '\x00', 'd', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'L', 'O',
    '\x0b', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'e',
    'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\x00', '\x00', '\x00', '\x00', '\x00', ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\xcf', '\x93', '\xbe', '\xad', 'M',
    '*', '\x8d', 'm', '\x17', 'L', '\x17', 'R', '#', '{', '.', 'R', '\x08', '\xa5', '\x94', '\xb5', '\x96', '\x18', 'h', '>', '\xe5', '\x0e', '\xf2', '\x09', '\xcf', '\x1e', '\xfb', '\x19', '\x00',
    ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x01', ' ', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::boost, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the boost serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_REGISTER);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("6f46324faae448b9e3b96dac94da17be6ab7eaaba398de86d8743042c98bace0");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, boost_serialization_get_or_calculate_asset_id_emit)
{
  bool success{};
  const std::string serialized_ado{'\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's', 'e', 'r', 'i', 'a', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', ':', ':', 'a', 'r', 'c', 'h', 'i',
    'v', 'e', '\x14', '\x00', '\x04', '\x04', '\x04', '\x08', '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x02', '\x00', '\x00', '\x00', '\x00', '\x00', 'd', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'L', 'O',
    '\x0b', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'e',
    'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\x00', '\x00', '\x00', '\x00', '\x00', ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\xed', '\xab', 'W', '\x1c', 'K',
    '\xe9', '\xea', '\xbf', '\xea', '^', 'x', '\x83', '\x03', 'm', 't', 'L', '\x09', 's', '\x82', '\xeb', 'o', 's', '\x9a', '\x91', 'M', '\xb0', 'o', 'r', '\xba', '5', '\x09', '\x9d', '\x00', ' ',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x01', ' ', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', 'I', '\xa3', '\xd6', 'e', '*', '\xaa', '\x0b', ';', 'w', ')', ',', 'S', 'N', '\x91', '\xff', '\x80', '\xde', '\x91', ' ', '\xae', '\xb6', '\xfc', '\x1c', '^',
    '\xdc', 'r', '\x80', 'G', 'C', '}', 'f', '~'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::boost, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the boost serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_EMIT);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("49a3d6652aaa0b3b77292c534e91ff80de9120aeb6fc1c5edc728047437d667e");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, boost_serialization_get_or_calculate_asset_id_update)
{
  bool success{};
  const std::string serialized_ado{'\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's', 'e', 'r', 'i', 'a', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', ':', ':', 'a', 'r', 'c', 'h', 'i',
    'v', 'e', '\x14', '\x00', '\x04', '\x04', '\x04', '\x08', '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00', 'd', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'L', 'O',
    '\x0b', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'e',
    'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\x00', '\x00', '\x00', '\x00', '\x00', ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x8c', '\xb6', '4', '\x9f', 'Q',
    '\xda', 'e', '\x99', '\xfe', '\xea', '\xe7', '\xc0', '\x07', 'r', '\x93', 'C', 'n', '\xb6', '\xa5', '\x00', '\x0f', '\x0e', 'n', 'p', 'n', 'w', '\x88', 'k', '\xb5', '@', '\xe2', '\xc1', '\x00',
    ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x01', ' ', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\xc3', 'q', '\xf6', '\x0d', '\xd8', '3', '2', '\x98', '\xc6', '\xaa', 't', 'k', 'q', '\xe1', '\xe2', '\x05', '\'', '\xb1', '\xff', '^', '\x1b', '\xed', 'N',
    '\xa9', '\xb5', '\xf5', '\x92', '\xfa', '\xdf', '\x90', '\xed', 'k'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::boost, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the boost serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_UPDATE);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("c371f60dd8333298c6aa746b71e1e20527b1ff5e1bed4ea9b5f592fadf90ed6b");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, boost_serialization_get_or_calculate_asset_id_public_burn)
{
  bool success{};
  const std::string serialized_ado{'\x16', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 's', 'e', 'r', 'i', 'a', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', ':', ':', 'a', 'r', 'c', 'h', 'i',
    'v', 'e', '\x14', '\x00', '\x04', '\x04', '\x04', '\x08', '\x01', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x04', '\x00', '\x00', '\x00', '\x00', '\x00', 'd', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '2', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'L', 'O',
    '\x0b', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '\x0d', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 'H', 'e',
    'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\x00', '\x00', '\x00', '\x00', '\x00', ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x0c', '@', '\x8c', '\xf8', '\xb7',
    '\xfb', '\x80', '\x8f', '@', 'Y', '=', 'n', '\xb7', 'X', '\x90', '\xe2', '\xab', '=', '\x0c', '\xcd', '\xc7', '\x01', 'J', '\x7f', '\xc6', '\xb6', '\xab', '\x05', '\x16', ';', '\xe0', '`', '\x00',
    ' ', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x01', ' ', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00', 'T', '\xf3', '\xf7', ',', 'r', '\xe5', '\xb0', '\x14', '\xad', '+', '+', '\x90', '\x01', '\xac', '\xef', '\x95', 'O', '\xe8', '-', '\xd3', '\xed', 'V', '\xa3',
    '\x8c', '\xd9', '\xdd', '\xc5', '\xdb', 'W', 'g', '?', '\x8f'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::boost, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the boost serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("54f3f72c72e5b014ad2b2b9001acef954fe82dd3ed56a38cd9ddc5db57673f8f");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, key_value_serialization_get_or_calculate_asset_id_undefined)
{
  bool success{};
  const std::string serialized_ado{'{', '\x0d', '\x0a', ' ', ' ', '"', 'a', 'm', 'o', 'u', 'n', 't', '_', 'c', 'o', 'm', 'm', 'i', 't', 'm', 'e', 'n', 't', '"', ':', ' ', '"', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '"', ',', '\x0d', '\x0a', ' ', ' ', '"', 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', '"',
    ':', ' ', '{', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'c', 'u', 'r', 'r', 'e', 'n', 't', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '5', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"',
    'd', 'e', 'c', 'i', 'm', 'a', 'l', '_', 'p', 'o', 'i', 'n', 't', '"', ':', ' ', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'f', 'u', 'l', 'l', '_', 'n', 'a', 'm', 'e', '"', ':', ' ', '"',
    'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'h', 'i', 'd', 'd', 'e', 'n', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', 'f', 'a',
    'l', 's', 'e', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'm', 'e', 't', 'a', '_', 'i', 'n', 'f', 'o', '"', ':', ' ', '"', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '"',
    ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'o', 'w', 'n', 'e', 'r', '"', ':', ' ', '"', 'e', '9', '1', 'b', '9', 'a', '7', '3', '2', '9', '2', 'd', '6', 'e', 'a', '4', '6', 'f', 'b', '3', 'd',
    '4', 'f', '4', 'c', 'c', '7', '9', 'c', '3', '4', 'b', 'f', 'b', '7', 'd', '1', '4', 'c', '2', 'e', '6', '8', '4', 'e', '5', '8', '0', '9', '3', 'a', '2', '4', '7', '1', 'c', '9', '2', 'e', '5',
    '1', 'c', '1', '6', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'i', 'c', 'k', 'e', 'r', '"', ':', ' ', '"', 'H', 'L', 'O', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'o',
    't', 'a', 'l', '_', 'm', 'a', 'x', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '1', '0', '0', '\x0d', '\x0a', ' ', ' ', '}', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 'e', 'r', 'a', 't',
    'i', 'o', 'n', '_', 't', 'y', 'p', 'e', '"', ':', ' ', '0', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 't', '_', 'a', 's', 's', 'e', 't', '_', 'i', 'd', '"', ':', ' ', '"', '9', '7', '9', 'e',
    'b', '7', '0', '6', 'a', 'c', 'e', '2', 'e', 'b', '8', '3', 'f', '9', '1', '2', '5', '6', '5', '8', 'b', '2', '3', 'f', 'b', '3', '5', '2', '2', '0', '8', '4', '8', '0', 'c', 'b', '3', 'b', '9',
    '0', 'c', '4', '3', 'e', '2', 'd', 'f', '0', 'd', '2', '9', '8', 'f', '9', '7', '5', '4', 'e', 'b', 'c', '"', '\x0d', '\x0a', '}'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::key_value, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the key-value serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_UNDEFINED);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_FALSE(success);
}

TEST(multiassets, key_value_serialization_get_or_calculate_asset_id_register)
{
  bool success{};
  const std::string serialized_ado{'{', '\x0d', '\x0a', ' ', ' ', '"', 'a', 'm', 'o', 'u', 'n', 't', '_', 'c', 'o', 'm', 'm', 'i', 't', 'm', 'e', 'n', 't', '"', ':', ' ', '"', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '"', ',', '\x0d', '\x0a', ' ', ' ', '"', 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', '"',
    ':', ' ', '{', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'c', 'u', 'r', 'r', 'e', 'n', 't', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '5', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"',
    'd', 'e', 'c', 'i', 'm', 'a', 'l', '_', 'p', 'o', 'i', 'n', 't', '"', ':', ' ', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'f', 'u', 'l', 'l', '_', 'n', 'a', 'm', 'e', '"', ':', ' ', '"',
    'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'h', 'i', 'd', 'd', 'e', 'n', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', 'f', 'a',
    'l', 's', 'e', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'm', 'e', 't', 'a', '_', 'i', 'n', 'f', 'o', '"', ':', ' ', '"', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '"',
    ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'o', 'w', 'n', 'e', 'r', '"', ':', ' ', '"', 'c', 'f', '9', '3', 'b', 'e', 'a', 'd', '4', 'd', '2', 'a', '8', 'd', '6', 'd', '1', '7', '4', 'c', '1',
    '7', '5', '2', '2', '3', '7', 'b', '2', 'e', '5', '2', '0', '8', 'a', '5', '9', '4', 'b', '5', '9', '6', '1', '8', '6', '8', '3', 'e', 'e', '5', '0', 'e', 'f', '2', '0', '9', 'c', 'f', '1', 'e',
    'f', 'b', '1', '9', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'i', 'c', 'k', 'e', 'r', '"', ':', ' ', '"', 'H', 'L', 'O', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'o',
    't', 'a', 'l', '_', 'm', 'a', 'x', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '1', '0', '0', '\x0d', '\x0a', ' ', ' ', '}', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 'e', 'r', 'a', 't',
    'i', 'o', 'n', '_', 't', 'y', 'p', 'e', '"', ':', ' ', '1', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 't', '_', 'a', 's', 's', 'e', 't', '_', 'i', 'd', '"', ':', ' ', '"', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '"', '\x0d', '\x0a', '}'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::key_value, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the key-value serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_REGISTER);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("6f46324faae448b9e3b96dac94da17be6ab7eaaba398de86d8743042c98bace0");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, key_value_serialization_get_or_calculate_asset_id_emit)
{
  bool success{};
  const std::string serialized_ado{'{', '\x0d', '\x0a', ' ', ' ', '"', 'a', 'm', 'o', 'u', 'n', 't', '_', 'c', 'o', 'm', 'm', 'i', 't', 'm', 'e', 'n', 't', '"', ':', ' ', '"', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '"', ',', '\x0d', '\x0a', ' ', ' ', '"', 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', '"',
    ':', ' ', '{', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'c', 'u', 'r', 'r', 'e', 'n', 't', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '5', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"',
    'd', 'e', 'c', 'i', 'm', 'a', 'l', '_', 'p', 'o', 'i', 'n', 't', '"', ':', ' ', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'f', 'u', 'l', 'l', '_', 'n', 'a', 'm', 'e', '"', ':', ' ', '"',
    'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'h', 'i', 'd', 'd', 'e', 'n', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', 'f', 'a',
    'l', 's', 'e', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'm', 'e', 't', 'a', '_', 'i', 'n', 'f', 'o', '"', ':', ' ', '"', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '"',
    ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'o', 'w', 'n', 'e', 'r', '"', ':', ' ', '"', 'e', 'd', 'a', 'b', '5', '7', '1', 'c', '4', 'b', 'e', '9', 'e', 'a', 'b', 'f', 'e', 'a', '5', 'e', '7',
    '8', '8', '3', '0', '3', '6', 'd', '7', '4', '4', 'c', '0', '9', '7', '3', '8', '2', 'e', 'b', '6', 'f', '7', '3', '9', 'a', '9', '1', '4', 'd', 'b', '0', '6', 'f', '7', '2', 'b', 'a', '3', '5',
    '0', '9', '9', 'd', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'i', 'c', 'k', 'e', 'r', '"', ':', ' ', '"', 'H', 'L', 'O', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'o',
    't', 'a', 'l', '_', 'm', 'a', 'x', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '1', '0', '0', '\x0d', '\x0a', ' ', ' ', '}', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 'e', 'r', 'a', 't',
    'i', 'o', 'n', '_', 't', 'y', 'p', 'e', '"', ':', ' ', '2', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 't', '_', 'a', 's', 's', 'e', 't', '_', 'i', 'd', '"', ':', ' ', '"', '4', '9', 'a', '3',
    'd', '6', '6', '5', '2', 'a', 'a', 'a', '0', 'b', '3', 'b', '7', '7', '2', '9', '2', 'c', '5', '3', '4', 'e', '9', '1', 'f', 'f', '8', '0', 'd', 'e', '9', '1', '2', '0', 'a', 'e', 'b', '6', 'f',
    'c', '1', 'c', '5', 'e', 'd', 'c', '7', '2', '8', '0', '4', '7', '4', '3', '7', 'd', '6', '6', '7', 'e', '"', '\x0d', '\x0a', '}'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::key_value, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the key-value serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_EMIT);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("49a3d6652aaa0b3b77292c534e91ff80de9120aeb6fc1c5edc728047437d667e");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, key_value_serialization_get_or_calculate_asset_id_update)
{
  bool success{};
  const std::string serialized_ado{'{', '\x0d', '\x0a', ' ', ' ', '"', 'a', 'm', 'o', 'u', 'n', 't', '_', 'c', 'o', 'm', 'm', 'i', 't', 'm', 'e', 'n', 't', '"', ':', ' ', '"', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '"', ',', '\x0d', '\x0a', ' ', ' ', '"', 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', '"',
    ':', ' ', '{', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'c', 'u', 'r', 'r', 'e', 'n', 't', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '5', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"',
    'd', 'e', 'c', 'i', 'm', 'a', 'l', '_', 'p', 'o', 'i', 'n', 't', '"', ':', ' ', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'f', 'u', 'l', 'l', '_', 'n', 'a', 'm', 'e', '"', ':', ' ', '"',
    'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'h', 'i', 'd', 'd', 'e', 'n', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', 'f', 'a',
    'l', 's', 'e', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'm', 'e', 't', 'a', '_', 'i', 'n', 'f', 'o', '"', ':', ' ', '"', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '"',
    ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'o', 'w', 'n', 'e', 'r', '"', ':', ' ', '"', '8', 'c', 'b', '6', '3', '4', '9', 'f', '5', '1', 'd', 'a', '6', '5', '9', '9', 'f', 'e', 'e', 'a', 'e',
    '7', 'c', '0', '0', '7', '7', '2', '9', '3', '4', '3', '6', 'e', 'b', '6', 'a', '5', '0', '0', '0', 'f', '0', 'e', '6', 'e', '7', '0', '6', 'e', '7', '7', '8', '8', '6', 'b', 'b', '5', '4', '0',
    'e', '2', 'c', '1', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'i', 'c', 'k', 'e', 'r', '"', ':', ' ', '"', 'H', 'L', 'O', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'o',
    't', 'a', 'l', '_', 'm', 'a', 'x', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '1', '0', '0', '\x0d', '\x0a', ' ', ' ', '}', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 'e', 'r', 'a', 't',
    'i', 'o', 'n', '_', 't', 'y', 'p', 'e', '"', ':', ' ', '3', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 't', '_', 'a', 's', 's', 'e', 't', '_', 'i', 'd', '"', ':', ' ', '"', 'c', '3', '7', '1',
    'f', '6', '0', 'd', 'd', '8', '3', '3', '3', '2', '9', '8', 'c', '6', 'a', 'a', '7', '4', '6', 'b', '7', '1', 'e', '1', 'e', '2', '0', '5', '2', '7', 'b', '1', 'f', 'f', '5', 'e', '1', 'b', 'e',
    'd', '4', 'e', 'a', '9', 'b', '5', 'f', '5', '9', '2', 'f', 'a', 'd', 'f', '9', '0', 'e', 'd', '6', 'b', '"', '\x0d', '\x0a', '}'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::key_value, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the key-value serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_UPDATE);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("c371f60dd8333298c6aa746b71e1e20527b1ff5e1bed4ea9b5f592fadf90ed6b");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}

TEST(multiassets, key_value_serialization_get_or_calculate_asset_id_public_burn)
{
  bool success{};
  const std::string serialized_ado{'{', '\x0d', '\x0a', ' ', ' ', '"', 'a', 'm', 'o', 'u', 'n', 't', '_', 'c', 'o', 'm', 'm', 'i', 't', 'm', 'e', 'n', 't', '"', ':', ' ', '"', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '"', ',', '\x0d', '\x0a', ' ', ' ', '"', 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', '"',
    ':', ' ', '{', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'c', 'u', 'r', 'r', 'e', 'n', 't', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '5', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"',
    'd', 'e', 'c', 'i', 'm', 'a', 'l', '_', 'p', 'o', 'i', 'n', 't', '"', ':', ' ', '0', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'f', 'u', 'l', 'l', '_', 'n', 'a', 'm', 'e', '"', ':', ' ', '"',
    'H', 'E', 'L', 'L', 'O', '_', 'W', 'O', 'R', 'L', 'D', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'h', 'i', 'd', 'd', 'e', 'n', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', 'f', 'a',
    'l', 's', 'e', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'm', 'e', 't', 'a', '_', 'i', 'n', 'f', 'o', '"', ':', ' ', '"', 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '"',
    ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 'o', 'w', 'n', 'e', 'r', '"', ':', ' ', '"', '0', 'c', '4', '0', '8', 'c', 'f', '8', 'b', '7', 'f', 'b', '8', '0', '8', 'f', '4', '0', '5', '9', '3',
    'd', '6', 'e', 'b', '7', '5', '8', '9', '0', 'e', '2', 'a', 'b', '3', 'd', '0', 'c', 'c', 'd', 'c', '7', '0', '1', '4', 'a', '7', 'f', 'c', '6', 'b', '6', 'a', 'b', '0', '5', '1', '6', '3', 'b',
    'e', '0', '6', '0', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'i', 'c', 'k', 'e', 'r', '"', ':', ' ', '"', 'H', 'L', 'O', '"', ',', '\x0d', '\x0a', ' ', ' ', ' ', ' ', '"', 't', 'o',
    't', 'a', 'l', '_', 'm', 'a', 'x', '_', 's', 'u', 'p', 'p', 'l', 'y', '"', ':', ' ', '1', '0', '0', '\x0d', '\x0a', ' ', ' ', '}', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 'e', 'r', 'a', 't',
    'i', 'o', 'n', '_', 't', 'y', 'p', 'e', '"', ':', ' ', '4', ',', '\x0d', '\x0a', ' ', ' ', '"', 'o', 'p', 't', '_', 'a', 's', 's', 'e', 't', '_', 'i', 'd', '"', ':', ' ', '"', '5', '4', 'f', '3',
    'f', '7', '2', 'c', '7', '2', 'e', '5', 'b', '0', '1', '4', 'a', 'd', '2', 'b', '2', 'b', '9', '0', '0', '1', 'a', 'c', 'e', 'f', '9', '5', '4', 'f', 'e', '8', '2', 'd', 'd', '3', 'e', 'd', '5',
    '6', 'a', '3', '8', 'c', 'd', '9', 'd', 'd', 'c', '5', 'd', 'b', '5', '7', '6', '7', '3', 'f', '8', 'f', '"', '\x0d', '\x0a', '}'};

  crypto::point_t expected_point_asset_id{};
  crypto::public_key expected_asset_id{};
  crypto::point_t calculated_point_asset_id{};
  crypto::public_key calculated_asset_id{};
  const std::optional ado{deserialize<currency::asset_descriptor_operation>(serialization_method::key_value, serialized_ado)};

  ASSERT_TRUE(ado.has_value());
  ASSERT_EQ(ado.value().verion, 1);
  /* TODO: fix the key-value serialization of the asset_descriptor_operation_v0 objects: .description.version must be equals to 0.
  ASSERT_EQ(ado.value().descriptor.version, 0); */
  ASSERT_EQ(ado.value().operation_type, ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN);
  success = currency::get_or_calculate_asset_id(ado.value(), &calculated_point_asset_id, &calculated_asset_id);
  ASSERT_TRUE(success);
  success = expected_point_asset_id.from_string("54f3f72c72e5b014ad2b2b9001acef954fe82dd3ed56a38cd9ddc5db57673f8f");
  ASSERT_TRUE(success);
  ASSERT_EQ(calculated_point_asset_id, expected_point_asset_id);
  expected_asset_id = expected_point_asset_id.to_public_key();
  ASSERT_EQ(calculated_asset_id, expected_asset_id);
}
