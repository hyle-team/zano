#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <boost/property_tree/json_parser.hpp>
#include "gtest/gtest.h"
#define UTF_CPP_CPLUSPLUS 201703L
#include "common/utf8.h"
#include "string_tools.h"
#include "epee/include/serialization/keyvalue_serialization.h"
#include "epee/include/storages/portable_storage_template_helper.h"

// encoding the raw bytes 00..FF produces these JSON characters U+0000..U+00FF
static const std::string g_json_str_256(R"string(\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\b\t\n\u000b\f\r\u000e\u000f\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001a\u001b\u001c\u001d\u001e\u001f !\"#$%&'()*+,-.\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~)string" "\x7f" R"string(\u0080\u0081\u0082\u0083\u0084\u0085\u0086\u0087\u0088\u0089\u008a\u008b\u008c\u008d\u008e\u008f\u0090\u0091\u0092\u0093\u0094\u0095\u0096\u0097\u0098\u0099\u009a\u009b\u009c\u009d\u009e\u009f\u00a0\u00a1\u00a2\u00a3\u00a4\u00a5\u00a6\u00a7\u00a8\u00a9\u00aa\u00ab\u00ac\u00ad\u00ae\u00af\u00b0\u00b1\u00b2\u00b3\u00b4\u00b5\u00b6\u00b7\u00b8\u00b9\u00ba\u00bb\u00bc\u00bd\u00be\u00bf\u00c0\u00c1\u00c2\u00c3\u00c4\u00c5\u00c6\u00c7\u00c8\u00c9\u00ca\u00cb\u00cc\u00cd\u00ce\u00cf\u00d0\u00d1\u00d2\u00d3\u00d4\u00d5\u00d6\u00d7\u00d8\u00d9\u00da\u00db\u00dc\u00dd\u00de\u00df\u00e0\u00e1\u00e2\u00e3\u00e4\u00e5\u00e6\u00e7\u00e8\u00e9\u00ea\u00eb\u00ec\u00ed\u00ee\u00ef\u00f0\u00f1\u00f2\u00f3\u00f4\u00f5\u00f6\u00f7\u00f8\u00f9\u00fa\u00fb\u00fc\u00fd\u00fe\u00ff)string");

struct test_struct_1
{
  std::string str;

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(str)
  END_KV_SERIALIZE_MAP()
};

// test JSON string escaping correctness for each 8-bit character from 00 to FF
TEST(portable_storage_tests, json_string_escaping)
{

  test_struct_1 obj;
  for(size_t i = 0; i < 256; ++i)
    obj.str += static_cast<char>(i);

  std::string json = epee::serialization::store_t_to_json(obj);
  //LOG_PRINT_CYAN("JSON: " << json, LOG_LEVEL_0);

  ASSERT_EQ(std::string("{\r\n  \"str\": \"") + g_json_str_256 + "\"\r\n}", json);
}

TEST(portable_storage_tests, json_string_unescaping_match_string2)
{
  // 1. recover the original bytes 00..FF
  std::string sample_json_string = std::string("\"") + g_json_str_256 + "\"";
  std::string output_buffer;
  std::string::const_iterator it = sample_json_string.begin();
  epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  ASSERT_EQ(256u, output_buffer.size());
  for (size_t i = 0; i < 256; ++i)
  {
    ASSERT_EQ(output_buffer[i], static_cast<char>(i));
  }

  sample_json_string = R"("\u0100\u7707\uD83D\uDE00")";
  it = sample_json_string.begin();
  epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  ASSERT_EQ("\xC4\x80\xE7\x9C\x87\xF0\x9F\x98\x80", output_buffer);

  sample_json_string = R"("it\'s\vok")";
  it = sample_json_string.begin();
  epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  ASSERT_EQ("it's\vok", output_buffer);
  EXPECT_EQ(R"(it's\u000bok)", epee::misc_utils::parse::transform_to_json_escape_sequence(output_buffer));

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

  const std::string expected = R"(\u00e8\u0080 \u00e2\u0082 \u00c0\u00af \u00ed\u00a0\u0080 \u00f4\u0090\u0080\u0080 )" "\xC3\xA8";
  ASSERT_EQ("{\r\n  \"str\": \"" + expected + "\"\r\n}", epee::serialization::store_t_to_json(value));
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
    const std::string expected = json == literal ? utf8 : "\xE8\xF0\x9F\x98\x80";
    EXPECT_EQ(expected, value.str);
    const std::string serialized = epee::serialization::store_t_to_json(value);
    std::cout << "ser JSON: " << serialized << '\n';
    ASSERT_TRUE(epee::serialization::load_t_from_json(decoded, serialized));
    std::cout << "reparsed bytes (" << decoded.str.size() << "): " << epee::string_tools::buff_to_hex(decoded.str) << '\n';
    EXPECT_EQ(expected, decoded.str);
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
    "", std::string("\0\"\\", 3), R"(\u00a6)", "\x80", "\xBF", "\xFF", "\xC3\xA8",
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
  for (const auto& input : json_byte_inputs())
  {
    SCOPED_TRACE(epee::string_tools::buff_to_hex(input));
    std::string escaped;
    for (unsigned char byte : input)
      escaped += examples[byte].writer_output;
    test_struct_1 decoded;
    ASSERT_TRUE(epee::serialization::load_t_from_json(decoded, R"({"str":")" + escaped + R"("})"));
    ASSERT_EQ(input, decoded.str);
  }
}

TEST(portable_storage_tests, json_new_to_old)
{
  std::array<legacy_json_byte, 256> examples;
  ASSERT_NO_FATAL_FAILURE(load_legacy_json_bytes(examples));
  for (const auto& input : json_byte_inputs())
  {
    SCOPED_TRACE(epee::string_tools::buff_to_hex(input));
    std::ostringstream stream;
    epee::serialization::strategy_json::handle_value(stream, input, 0);
    const std::string token = stream.str();
    ASSERT_GE(token.size(), 2u);
    ASSERT_EQ('"', token.front());
    ASSERT_EQ('"', token.back());
    // each input byte must have a spelling understood by the released reader
    size_t pos = 1;
    for (unsigned char byte : input)
    {
      const auto& example = examples[byte];
      if (token.compare(pos, example.writer_output.size(), example.writer_output) == 0)
        pos += example.writer_output.size();
      else
      {
        ASSERT_EQ(0, token.compare(pos, example.reader_input.size(), example.reader_input));
        pos += example.reader_input.size();
      }
    }
    ASSERT_EQ(token.size() - 1, pos);
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
      ASSERT_EQ(input, external.get<std::string>("str"));

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



