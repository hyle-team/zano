#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include <boost/property_tree/json_parser.hpp>
#include "gtest/gtest.h"
#define UTF_CPP_CPLUSPLUS 201703L
#include "common/utf8.h"
#include "string_tools.h"
#include "epee/include/serialization/keyvalue_serialization.h"
#include "epee/include/storages/portable_storage_template_helper.h"

struct test_struct_1
{
  std::string str;

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(str)
  END_KV_SERIALIZE_MAP()
};

TEST(portable_storage_tests, json_string_escaping)
{
  const std::pair<std::string, std::string> cases[] = {
    {"\xC2\xA6", R"(\u00A6)"},
    {"\xF0\x9F\x98\x80", R"(\uD83D\uDE00)"},
    {"\xA6", R"(\\xA6)"},
    {R"(\xA6)", R"(\\\\xA6)"},
    {"\\\xA6", R"(\\\\\\xA6)"},
    {std::string("\0\n\x1F\x7F", 4), R"(\u0000\u000A\u001F\u007F)"},
    {"\"/", R"(\"/)"}
  };
  for (const auto& [bytes, escaped] : cases)
  {
    SCOPED_TRACE(escaped);
    test_struct_1 sent, received;
    sent.str = bytes;
    const std::string wire = epee::serialization::store_t_to_json(sent);
    std::cout << "\nInput (hex)       " << epee::string_tools::buff_to_hex(sent.str)
              << "\nJSON on the wire\n" << wire << '\n';
    ASSERT_EQ("{\r\n  \"str\": \"" + escaped + "\"\r\n}", wire);
    std::istringstream stream(wire);
    boost::property_tree::ptree plain_json;
    ASSERT_NO_THROW(boost::property_tree::read_json(stream, plain_json));
    std::cout << "JSON reader (hex) " << epee::string_tools::buff_to_hex(plain_json.get<std::string>("str")) << '\n';
    ASSERT_TRUE(epee::serialization::load_t_from_json(received, wire));
    std::cout << "Zano reader (hex) " << epee::string_tools::buff_to_hex(received.str) << '\n';
    ASSERT_EQ(bytes, received.str);
  }
}

TEST(portable_storage_tests, json_string_unescaping_match_string2)
{
  const std::pair<std::string, std::string> markers[] = {
    {R"("\u005C\u0078\u0041\u0036")", "\xA6"},
    {R"("\u005C\u005CxA6")", R"(\xA6)"},
    {R"("\\x5CxA6")", R"(\xA6)"},
    {R"("\\x00\\xff")", std::string("\0\xFF", 2)},
    {R"("\\xG1\\xA\\")", R"(\xG1\xA\)"}
  };
  std::string output_buffer;
  for (const auto& [wire, bytes] : markers)
  {
    SCOPED_TRACE(wire);
    auto it = wire.cbegin();
    epee::misc_utils::parse::match_string2(it, wire.cend(), output_buffer);
    ASSERT_EQ(bytes, output_buffer);
    ASSERT_TRUE(it == wire.cend() - 1);
  }

  std::string sample_json_string = R"("\u0100\u7707\uD83D\uDE00")";
  std::string::const_iterator it = sample_json_string.begin();
  epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  ASSERT_EQ("\xC4\x80\xE7\x9C\x87\xF0\x9F\x98\x80", output_buffer);

  sample_json_string = R"("it\'s\vok")";
  it = sample_json_string.begin();
  epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  ASSERT_EQ("it's\vok", output_buffer);
  EXPECT_EQ(R"(it's\u000Bok)", epee::misc_utils::parse::transform_to_json_escape_sequence(output_buffer));

  sample_json_string = "\"a\\qb\\" "\xC3\xA8" "\"";
  it = sample_json_string.begin();
  epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  ASSERT_EQ("aqb\xC3\xA8", output_buffer);

  // 2. test incorrect 4-octets \uXYZT sequences
  sample_json_string = R"xxx("\uDDDD\u012T\u0100\u7707")xxx";
  it = sample_json_string.begin();
  EXPECT_THROW(epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer), std::runtime_error);

  // 3. test incorrect 3-octets \uXYZ sequences
  bool caught = false;
  try
  {
    sample_json_string = R"xxx("\u031")xxx";
    it = sample_json_string.begin();
    epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  }
  catch (std::runtime_error&)
  {
    caught = true;
  }
  ASSERT_TRUE(caught);

  // 4. test incorrect 2-octets \uXY sequences
  caught = false;
  try
  {
    sample_json_string = R"xxx("\u21")xxx";
    it = sample_json_string.begin();
    epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  }
  catch (std::runtime_error&)
  {
    caught = true;
  }
  ASSERT_TRUE(caught);

  // 5. test incorrect 1-octets \uX sequences
  caught = false;
  try
  {
    sample_json_string = R"xxx("\u2")xxx";
    it = sample_json_string.begin();
    epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  }
  catch (std::runtime_error&)
  {
    caught = true;
  }
  ASSERT_TRUE(caught);

  const std::string invalid_strings[] = {
    R"("\uDC00")", R"("\uD800")", R"("\uD800\u0041")", R"("\u012T")",
    "\"\n\"", "\"\x01\"", std::string("\"\0\"", 3), "\"\\\n\""
  };
  for (const auto& invalid : invalid_strings)
  {
    SCOPED_TRACE(::testing::PrintToString(invalid));
    it = invalid.begin();
    EXPECT_THROW(epee::misc_utils::parse::match_string2(it, invalid.end(), output_buffer), std::runtime_error);
  }
}

TEST(portable_storage_tests, json_legacy_raw_bytes)
{
  const std::string legacy = "\xE8\x80 \xE2\x82 \xC0\xAF \xED\xA0\x80 \xF4\x90\x80\x80 \xC3\xA8";
  const std::string input = R"({"str":")" + legacy + R"("})";
  test_struct_1 value;
  ASSERT_TRUE(epee::serialization::load_t_from_json(value, input));
  ASSERT_EQ(legacy, value.str);

  const std::string expected = R"(\\xE8\\x80 \\xE2\\x82 \\xC0\\xAF \\xED\\xA0\\x80 \\xF4\\x90\\x80\\x80 \u00E8)";
  ASSERT_EQ("{\r\n  \"str\": \"" + expected + "\"\r\n}", epee::serialization::store_t_to_json(value));
}

TEST(portable_storage_tests, json_marker_in_keys_and_array_values)
{
  const std::string byte = "\xA6", literal = R"(\xA6)";
  epee::serialization::portable_storage sent, received;
  ASSERT_TRUE(sent.set_value(literal, byte, nullptr));
  auto array = sent.insert_first_value(byte, literal, nullptr);
  ASSERT_TRUE(array != nullptr);
  ASSERT_TRUE(sent.insert_next_value(array, byte));

  std::string wire;
  ASSERT_TRUE(sent.dump_as_json(wire));
  EXPECT_NE(std::string::npos, wire.find(R"("\\\\xA6":)"));
  EXPECT_NE(std::string::npos, wire.find(R"("\\xA6":)"));
  ASSERT_TRUE(received.load_from_json(wire));
  std::string value;
  ASSERT_TRUE(received.get_value(literal, value, nullptr));
  EXPECT_EQ(byte, value);
  array = received.get_first_value(byte, value, nullptr);
  ASSERT_TRUE(array != nullptr);
  EXPECT_EQ(literal, value);
  ASSERT_TRUE(received.get_next_value(array, value));
  EXPECT_EQ(byte, value);
}

TEST(portable_storage_tests, json_unicode_round_trip)
{
  const std::string utf8 = "\xC3\xA8\xF0\x9F\x98\x80";
  const std::string literal = R"({"str":")" + utf8 + R"("})";
  const std::string escaped = R"({"str":"\u00e8\uD83D\uDE00"})";
  for (const auto& json : {literal, escaped})
  {
    test_struct_1 value, decoded;
    std::cout << "\ninput json: " << json << '\n';
    ASSERT_TRUE(epee::serialization::load_t_from_json(value, json));
    std::cout << "parsed bytes (" << value.str.size() << "): " << epee::string_tools::buff_to_hex(value.str) << '\n';
    EXPECT_EQ(utf8, value.str);
    const std::string serialized = epee::serialization::store_t_to_json(value);
    std::cout << "ser JSON: " << serialized << '\n';
    ASSERT_TRUE(epee::serialization::load_t_from_json(decoded, serialized));
    std::cout << "reparsed bytes (" << decoded.str.size() << "): " << epee::string_tools::buff_to_hex(decoded.str) << '\n';
    EXPECT_EQ(utf8, decoded.str);
  }
}

static std::vector<std::string> json_byte_inputs()
{
  std::vector<std::string> inputs = {""};
  for (unsigned first = 0; first < 256; ++first)
  {
    inputs.emplace_back(1, static_cast<char>(first));
    for (unsigned second = 0; second < 256; ++second)
      inputs.push_back({static_cast<char>(first), static_cast<char>(second)});
  }

  const std::string fragments[] = {
    "", std::string("\0\"\\", 3), R"(\u00a6)", R"(\xA6)", R"(\\)", "\x80", "\xBF", "\xFF", "\xC3\xA8",
    // valid UTF-8 at the three- and four-byte boundaries, including either side of the surrogate range
    "\xE0\xA0\x80", "\xED\x9F\xBF", "\xEE\x80\x80", "\xEF\xBF\xBF", "\xF0\x90\x80\x80", "\xF4\x8F\xBF\xBF",
    // overlongs, a surrogate, a value above U+10FFFF, and incomplete sequences
    "\xE0\x9F\xBF", "\xED\xA0\x80", "\xF0\x8F\xBF\xBF", "\xF4\x90\x80\x80", "\xE2\x82", "\xF0\x9F\x98"
  };
  for (const auto& prefix : fragments)
    for (const auto& suffix : fragments)
      inputs.push_back(prefix + suffix);
  return inputs;
}

struct legacy_json_byte
{
  std::string writer_output;
  std::string reader_input;
};

// from the 2.2.3.609 portable_storage_test.cpp vectors g_json_utf8_str_256 (writer) and g_json_str_256 (reader), уach byte 00..FF has two NULterminated wire spellings
static void load_legacy_json_bytes(std::array<legacy_json_byte, 256>& examples)
{
  std::ifstream file(ZANO_LEGACY_JSON_FIXTURE, std::ios::binary);
  ASSERT_TRUE(file.is_open()) << ZANO_LEGACY_JSON_FIXTURE;
  for (auto& example : examples)
  {
    ASSERT_TRUE(std::getline(file, example.writer_output, '\0') && !example.writer_output.empty());
    ASSERT_TRUE(std::getline(file, example.reader_input, '\0') && !example.reader_input.empty());
  }
  ASSERT_FALSE(file.eof()); // The last spelling must also end in NUL.
  ASSERT_EQ(std::char_traits<char>::eof(), file.peek());
}

TEST(portable_storage_tests, json_old_to_new)
{
  std::array<legacy_json_byte, 256> examples;
  ASSERT_NO_FATAL_FAILURE(load_legacy_json_bytes(examples));
  const auto check = [&](const std::string& input, const std::string& expected)
  {
    SCOPED_TRACE(epee::string_tools::buff_to_hex(input));
    std::string escaped;
    for (unsigned char byte : input)
      escaped += examples[byte].writer_output;
    test_struct_1 decoded;
    ASSERT_TRUE(epee::serialization::load_t_from_json(decoded, R"({"str":")" + escaped + R"("})"));
    ASSERT_EQ(expected, decoded.str);
  };
  for (const auto& input : json_byte_inputs())
  {
    if (input.find(R"(\\)") == std::string::npos && input.find(R"(\x)") == std::string::npos)
      ASSERT_NO_FATAL_FAILURE(check(input, input));
  }
  // Old literal text can have the same spelling as a new byte marker or a protected backslash.
  check(R"(\x)", R"(\x)");
  check(R"(\xA6)", "\xA6");
  check(R"(\\)", R"(\)");
  check(R"(\\xA6)", R"(\xA6)");
}

TEST(portable_storage_tests, json_new_to_old)
{
  std::array<legacy_json_byte, 256> examples;
  ASSERT_NO_FATAL_FAILURE(load_legacy_json_bytes(examples));
  // Match the outgoing wire against release fixtures for the bytes that reader would return.
  const std::pair<std::string, std::string> cases[] = {
    {"Zano", "Zano"},
    {"\x11", "\x11"},
    {R"(\)", R"(\\)"},
    {"\xA6", R"(\xA6)"},
    {R"(\xA6)", R"(\\xA6)"},
    {"\xC2\x99", "\x99"} // The released reader treated \u0099 as a single byte.
  };
  for (const auto& [sent, old_result] : cases)
  {
    SCOPED_TRACE(epee::string_tools::buff_to_hex(sent));
    std::string expected_wire;
    for (unsigned char byte : old_result)
      expected_wire += examples[byte].reader_input;
    EXPECT_EQ(expected_wire, epee::misc_utils::parse::transform_to_json_escape_sequence(sent));
  }
}

TEST(portable_storage_tests, json_byte_string_round_trip)
{
  for (const auto& input : json_byte_inputs())
  {
    SCOPED_TRACE(epee::string_tools::buff_to_hex(input));
    test_struct_1 original, decoded;
    original.str = input;
    const std::string json = epee::serialization::store_t_to_json(original);

    ASSERT_TRUE(utf8::is_valid(json.begin(), json.end()));
    std::istringstream stream(json);
    boost::property_tree::ptree external;
    ASSERT_NO_THROW(boost::property_tree::read_json(stream, external));
    if (utf8::is_valid(input.begin(), input.end()))
    {
      std::string expected_text;
      for (char c : input)
      {
        if (c == '\\')
          expected_text += '\\';
        expected_text += c;
      }
      ASSERT_EQ(expected_text, external.get<std::string>("str"));
    }

    ASSERT_TRUE(epee::serialization::load_t_from_json(decoded, json));
    ASSERT_EQ(input, decoded.str);
  }
}

void populate_storate(epee::serialization::portable_storage& ps, epee::serialization::portable_storage::hsection h_sec, size_t recursion_count)
{
  recursion_count++;
  if (recursion_count > 7)
    return;

  epee::serialization::portable_storage::hsection h_child = nullptr;
  auto h_array = ps.insert_first_section("n", h_child, h_sec);
  populate_storate(ps, h_child, recursion_count);
  for (size_t i = 0; i != recursion_count; i++)
  {
    ps.insert_next_section(h_array, h_child);
    populate_storate(ps, h_child, recursion_count);
  }
}



TEST(portable_storage_tests, mem_limitations_test)
{

  epee::serialization::portable_storage ps;
  size_t recursion_counter = 0;
  populate_storate(ps, nullptr, recursion_counter);

  bool caught = false;
  bool res = false;
  try
  {
    std::string buff;
    ps.store_to_binary(buff);

    epee::serialization::portable_storage ps2;
    bool res = ps2.load_from_binary(buff, epee::serialization::gdefault_portable_storage_limits);
  }
  catch (...)
  {
    caught = true;
  }
  ASSERT_TRUE(caught || !res);

}



