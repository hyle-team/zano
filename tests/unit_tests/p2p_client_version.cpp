// Copyright (c) 2019-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "common/util.h"

enum class reponse_check_parse_client_version : uint8_t
{
  parsed,
  not_parsed,
  parsed_unexpect
};

static reponse_check_parse_client_version check_parse_client_version(const std::string& str, const std::optional<int>& expected_major, const std::optional<int>& expected_minor,
                                                                     const std::optional<int>& expected_revision, const std::optional<int>& expected_build_number,
                                                                     const std::optional<std::string>& expected_commit_id, const std::optional<bool>& expected_dirty)
{
  enum class version_integer_component : uint8_t { major, minor, revision, build_number };
  // 3 not in {0; 1} and low-order bit doesn't equals to 0.
  constexpr uint8_t out_of_logicals_value{3};
  std::array<int32_t, 4> values_on_not_written{INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN};
  int32_t major{}, minor{}, revision{}, build_number{};
  std::string commit_id{};
  bool dirty{};

  if (expected_major.has_value() && expected_major.value() == INT32_MIN)
  {
    values_on_not_written.at(static_cast<uint8_t>(version_integer_component::major)) = INT32_MAX;
  }

  if (expected_minor.has_value() && expected_minor.value() == INT32_MIN)
  {
    values_on_not_written.at(static_cast<uint8_t>(version_integer_component::minor)) = INT32_MAX;
  }

  if (expected_revision.has_value() && expected_revision.value() == INT32_MIN)
  {
    values_on_not_written.at(static_cast<uint8_t>(version_integer_component::revision)) = INT32_MAX;
  }

  if (expected_build_number.has_value() && expected_build_number.value() == INT32_MIN)
  {
    values_on_not_written.at(static_cast<uint8_t>(version_integer_component::build_number)) = INT32_MAX;
  }

  major = values_on_not_written.at(static_cast<uint8_t>(version_integer_component::major));
  minor = values_on_not_written.at(static_cast<uint8_t>(version_integer_component::minor));
  revision = values_on_not_written.at(static_cast<uint8_t>(version_integer_component::revision));
  build_number = values_on_not_written.at(static_cast<uint8_t>(version_integer_component::build_number));

  if (expected_commit_id.has_value() && !expected_commit_id.value().empty())
  {
    const auto length{expected_commit_id.value().length()};

    assert(length + 1 > length);
    commit_id = std::string(length + 1, '\0');
  }

  if (!tools::parse_client_version(str, major, minor, revision, build_number, commit_id, dirty))
  {
    return reponse_check_parse_client_version::not_parsed;
  }

  if (expected_major.has_value())
  {
    if (major == values_on_not_written.at(static_cast<uint8_t>(version_integer_component::major)) || major != expected_major.value())
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  else
  {
    if (major != values_on_not_written.at(static_cast<uint8_t>(version_integer_component::major)))
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  if (expected_minor.has_value())
  {
    if (minor == values_on_not_written.at(static_cast<uint8_t>(version_integer_component::minor)) || minor != expected_minor.value())
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  else
  {
    if (minor != values_on_not_written.at(static_cast<uint8_t>(version_integer_component::minor)))
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  if (expected_revision.has_value())
  {
    if (revision == values_on_not_written.at(static_cast<uint8_t>(version_integer_component::revision)) || revision != expected_revision.value())
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  else
  {
    if (revision != values_on_not_written.at(static_cast<uint8_t>(version_integer_component::revision)))
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  if (expected_commit_id.has_value() && commit_id != expected_commit_id.value())
  {
    return reponse_check_parse_client_version::parsed_unexpect;
  }

  else
  {
    if (!commit_id.empty())
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  if (expected_dirty.has_value())
  {
    if (dirty != expected_dirty.value())
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  else
  {
    if (dirty != out_of_logicals_value)
    {
      return reponse_check_parse_client_version::parsed_unexpect;
    }
  }

  return reponse_check_parse_client_version::parsed;
}

TEST(p2p_client_version, test_0)
{
  ASSERT_EQ(check_parse_client_version("10.101.999.28391[deadbeef31337-dirty]", 10, 101, 999, 28391, "deadbeef31337", true), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("+67.+43.+50.+83", 67, 43, 50, 83, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-12.-90.17.-95", -12, -90, 17, -95, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("54.-100.-76.21[]", 54, -100, -76, 21, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-93.8.-81.75[-dirty]", -93, 8, -81, 75, "", true), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-8.-85.79.24[--dirty]", -8, -85, 79, 24, "-", true), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-62.53.79.80[\\]", -62, 53, 79, 80, "\\", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-27.91.-12.34[-]", -27, 91, -12, 34, "-", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-51.-66.-10.58\0[--dirty]", -51, -66, -10, 58, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-24.27.-81.79[" "\0" "-dirty]", {}, {}, {}, {}, {}, {}), reponse_check_parse_client_version::not_parsed);
  ASSERT_EQ(check_parse_client_version("0.0.0.0", 0, 0, 0, 0, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("27 . 33 . -59 . 47", 27, 33, -59, 47, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-2147483648.-2147483648.-2147483648.-2147483648", INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("2147483647.2147483647.2147483647.2147483647", INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("2147483648.2147483648.2147483648.2147483648", INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("-2147483649.-2147483649.-2147483649.-2147483649", INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("0098.+0096.0081.-0056", 98, 96, 81, -56, "", false), reponse_check_parse_client_version::parsed);
  ASSERT_EQ(check_parse_client_version("\0" "38.67.31.-24", 38, 67, 31, -24, "", false), reponse_check_parse_client_version::not_parsed);
  ASSERT_EQ(check_parse_client_version({'-', '6', '8', '.', '\0', '2', '9', '.', '5', '9', '.', '-', '7', '9'}, {}, {}, {}, {}, {}, {}), reponse_check_parse_client_version::not_parsed);
  ASSERT_EQ(check_parse_client_version("....", {}, {}, {}, {}, {}, {}), reponse_check_parse_client_version::not_parsed);
  ASSERT_EQ(check_parse_client_version("54.12.-10", {}, {}, {}, {}, {}, {}), reponse_check_parse_client_version::not_parsed);
  ASSERT_EQ(check_parse_client_version("-.-.-.-", {}, {}, {}, {}, {}, {}), reponse_check_parse_client_version::not_parsed);
  ASSERT_EQ(check_parse_client_version(" . . . ", {}, {}, {}, {}, {}, {}), reponse_check_parse_client_version::not_parsed);

  ASSERT_EQ(check_parse_client_version({'-', '2', '3', '.', '6', '.', '-', '1', '8', '.', '-', '1', '1', '[', '\0', ']'}, -23, 6, -18, -11, std::string{'\0'}, false),
                                       reponse_check_parse_client_version::parsed);

  ASSERT_EQ(check_parse_client_version({'9', '8', '.', '3', '.', '8', '9', '.', '-', '1', '[', '\0', '-', 'd', 'i','r', 't', 'y', ']'}, 98, 3, 89, -1, std::string{'\0'}, true),
            reponse_check_parse_client_version::parsed);

  //ASSERT_EQ(check_parse_client_version("5.42.25.-42[].", 5, 42, 25, -42, "", false), reponse_check_parse_client_version::parsed);
  //ASSERT_EQ(check_parse_client_version("-84.91.-10.1[-dirty].", 5, 42, 25, -42, "", true), reponse_check_parse_client_version::parsed);
  //ASSERT_EQ(check_parse_client_version("33.62.-92.-44.", 33, 62, -92, -44, "", false), reponse_check_parse_client_version::parsed);
  //ASSERT_EQ(check_parse_client_version("...", {}, {}, {}, {}, {}, {}), reponse_check_parse_client_version::not_parsed);
  //ASSERT_EQ(check_parse_client_version("-80.28.-6.1[", -80, 28, -6, 1, "", false), reponse_check_parse_client_version::parsed);
  //ASSERT_EQ(check_parse_client_version("-88.-36.11.-25[", -80, 28, -6, 1, "", false), reponse_check_parse_client_version::parsed);
  //ASSERT_EQ(check_parse_client_version("0.0.0.[]", {}, {}, {}, {}, {}, {}), reponse_check_parse_client_version::not_parsed);
}


TEST(p2p_client_version, test_1)
{
  using namespace tools;

  // good (>= 2.x since HF4)
  
  ASSERT_TRUE(check_remote_client_version("10.101.999.28391[deadbeef31337-dirty]"));
  ASSERT_TRUE(check_remote_client_version("3.0.2.7[aa00bcd]"));

  ASSERT_TRUE(check_remote_client_version("2.0.0.000[a]"));
  ASSERT_TRUE(check_remote_client_version("2.4.2.7[aabcd]"));
  ASSERT_TRUE(check_remote_client_version("2.99.0.67[26c00a8-dirty]"));
  ASSERT_TRUE(check_remote_client_version("4.0.0.0[7dd61ae-dirty]"));
  ASSERT_TRUE(check_remote_client_version("5.0.0.0[7dd61ae-dirty]"));

  // bad
  
  ASSERT_FALSE(check_remote_client_version(""));
  ASSERT_FALSE(check_remote_client_version(" "));
  ASSERT_FALSE(check_remote_client_version("  "));
  ASSERT_FALSE(check_remote_client_version("   "));

  ASSERT_FALSE(check_remote_client_version("."));
  ASSERT_FALSE(check_remote_client_version(".."));
  ASSERT_FALSE(check_remote_client_version("..."));

  ASSERT_FALSE(check_remote_client_version("1.0.999"));

  ASSERT_FALSE(check_remote_client_version("1.0.40[f77f0d7]"));
  ASSERT_FALSE(check_remote_client_version("1.0.40[734b726]"));
  ASSERT_FALSE(check_remote_client_version("1.0.41[488e369]"));

  ASSERT_FALSE(check_remote_client_version("1.0.40[469]"));
  ASSERT_FALSE(check_remote_client_version("1.0.39[f77f0d7]"));
  ASSERT_FALSE(check_remote_client_version("1.0.38[f77f0d7-dirty]"));
  ASSERT_FALSE(check_remote_client_version("1.99.37[7dd61ae-dirty]"));
  ASSERT_FALSE(check_remote_client_version("0.0.500[000]"));

  ASSERT_FALSE(check_remote_client_version("a.1.9.237[aabcd]"));
  ASSERT_FALSE(check_remote_client_version("x.0.57"));
}
