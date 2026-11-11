#include "gtest/gtest.h"
#include <limits>
#include "epee/include/serialization/keyvalue_serialization.h"
#include "epee/include/storages/portable_storage_template_helper.h"

// valid JSON string with full range of ASCII chars {0..255}
// it must be correctly decoded to full-range
static const std::string g_json_str_256(R"string(\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\b\t\n\u000b\f\r\u000e\u000f\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001a\u001b\u001c\u001d\u001e\u001f !\"#$%&'()*+,-.\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\u007f\u0080\u0081\u0082\u0083\u0084\u0085\u0086\u0087\u0088\u0089\u008a\u008b\u008c\u008d\u008e\u008f\u0090\u0091\u0092\u0093\u0094\u0095\u0096\u0097\u0098\u0099\u009a\u009b\u009c\u009d\u009e\u009f\u00a0\u00a1\u00a2\u00a3\u00a4\u00a5\u00a6\u00a7\u00a8\u00a9\u00aa\u00ab\u00ac\u00ad\u00ae\u00af\u00b0\u00b1\u00b2\u00b3\u00b4\u00b5\u00b6\u00b7\u00b8\u00b9\u00ba\u00bb\u00bc\u00bd\u00be\u00bf\u00c0\u00c1\u00c2\u00c3\u00c4\u00c5\u00c6\u00c7\u00c8\u00c9\u00ca\u00cb\u00cc\u00cd\u00ce\u00cf\u00d0\u00d1\u00d2\u00d3\u00d4\u00d5\u00d6\u00d7\u00d8\u00d9\u00da\u00db\u00dc\u00dd\u00de\u00df\u00e0\u00e1\u00e2\u00e3\u00e4\u00e5\u00e6\u00e7\u00e8\u00e9\u00ea\u00eb\u00ec\u00ed\u00ee\u00ef\u00f0\u00f1\u00f2\u00f3\u00f4\u00f5\u00f6\u00f7\u00f8\u00f9\u00fa\u00fb\u00fc\u00fd\u00fe\u00ff)string");

// UTF-8-aware JSON string with full range of ASCII chars {0..255}, only 00h-1fh are escaped
// it sould be a result of correctly encoded 00-ff bytes range
static const std::string g_json_utf8_str_256(R"string(\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\b\t\n\u000b\f\r\u000e\u000f\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001a\u001b\u001c\u001d\u001e\u001f)string" " !\\\"#$%&'()*+,-.\\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff");


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

  ASSERT_EQ(std::string("{\r\n  \"str\": \"") + g_json_utf8_str_256 + "\"\r\n}", json);
}

TEST(portable_storage_tests, json_string_unescaping_match_string2)
{
  // 1. test all-octets string for decoding
  std::string sample_json_string = std::string("\"") + g_json_str_256 + "\"";
  std::string output_buffer;
  std::string::const_iterator it = sample_json_string.begin();
  epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  ASSERT_EQ(256, output_buffer.size());
  for (size_t i = 0; i < 256; ++i)
  {
    ASSERT_EQ(output_buffer[i], static_cast<char>(i));
  }

  // 2. test incorrect 4-octets \uXYZT sequences
  sample_json_string = R"xxx("\uDDDD\u012T\u0100\u7707")xxx";
  it = sample_json_string.begin();
  epee::misc_utils::parse::match_string2(it, sample_json_string.end(), output_buffer);
  ASSERT_EQ(output_buffer, "");

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

// Strict mode is opt-in; every test also pins the unchanged legacy default.
namespace
{
  epee::serialization::portable_storage_limits strict_limits()
  {
    epee::serialization::portable_storage_limits l;
    l.strict = true;
    l.max_storage_entries_count = 1000;
    l.max_sections_entries_count = 1000;
    l.max_array_elements_count = 1000;
    l.max_nesting_depth = 16;
    l.max_string_length = 1000;
    return l;
  }

  struct strict_fixture
  {
    uint64_t a = 1;
    uint64_t b = 2;
    std::string text = "abc";
    std::vector<uint64_t> list = {1, 2, 3, 4, 5};

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(a)
      KV_SERIALIZE(b)
      KV_SERIALIZE(text)
      KV_SERIALIZE(list)
    END_KV_SERIALIZE_MAP()
  };

  bool strict_load(const std::string& buff, const epee::serialization::portable_storage_limits& limits)
  {
    epee::serialization::portable_storage ps;
    return ps.load_from_binary(buff, limits);
  }

  std::string raw_header()
  {
    std::string s;
    for (uint32_t v : {uint32_t(PORTABLE_STORAGE_SIGNATUREA), uint32_t(PORTABLE_STORAGE_SIGNATUREB)})
      for (int i = 0; i != 4; ++i) s.push_back(static_cast<char>((v >> (8 * i)) & 0xff));
    s.push_back(static_cast<char>(PORTABLE_STORAGE_FORMAT_VER));
    return s;
  }
  std::string raw_u64_entry(const std::string& name, uint64_t v)
  {
    std::string s(1, static_cast<char>(name.size()));
    s += name;
    s.push_back(static_cast<char>(SERIALIZE_TYPE_UINT64));
    for (int i = 0; i != 8; ++i) s.push_back(static_cast<char>((v >> (8 * i)) & 0xff));
    return s;
  }
  // Root section with one entry "f": a scalar bool, or a bool array with a
  // one-byte size varint (fewer than 64 elements), carrying raw bytes.
  std::string raw_bool_scalar(uint8_t byte)
  {
    return raw_header() + std::string(1, static_cast<char>(1 << 2)) + std::string(1, '\1') + "f" +
      std::string(1, static_cast<char>(SERIALIZE_TYPE_BOOL)) + std::string(1, static_cast<char>(byte));
  }
  std::string raw_bool_array(const std::vector<uint8_t>& bytes)
  {
    std::string s = raw_header() + std::string(1, static_cast<char>(1 << 2)) + std::string(1, '\1') + "f";
    s.push_back(static_cast<char>(SERIALIZE_TYPE_BOOL | SERIALIZE_FLAG_ARRAY));
    s.push_back(static_cast<char>(bytes.size() << 2));
    for (uint8_t b : bytes) s.push_back(static_cast<char>(b));
    return s;
  }
  // Portable varint with an explicit width mark (1, 2, 4 or 8 bytes).
  std::string raw_varint(uint64_t v, size_t width)
  {
    const uint64_t mark = width == 1 ? PORTABLE_RAW_SIZE_MARK_BYTE : width == 2 ? PORTABLE_RAW_SIZE_MARK_WORD :
      width == 4 ? PORTABLE_RAW_SIZE_MARK_DWORD : PORTABLE_RAW_SIZE_MARK_INT64;
    const uint64_t encoded = (v << 2) | mark;
    std::string s;
    for (size_t i = 0; i != width; ++i) s.push_back(static_cast<char>((encoded >> (8 * i)) & 0xff));
    return s;
  }
  // Root section with one string entry "s": raw length encoding, then payload.
  std::string raw_string_entry(const std::string& length, const std::string& payload)
  {
    return raw_header() + std::string(1, static_cast<char>(1 << 2)) + std::string(1, '\1') + "s" +
      std::string(1, static_cast<char>(SERIALIZE_TYPE_STRING)) + length + payload;
  }
}

TEST(portable_storage_tests, strict_bool_accepts_zero_and_one)
{
  for (uint8_t byte : {uint8_t(0), uint8_t(1)})
  {
    const std::string buff = raw_bool_scalar(byte);
    epee::serialization::portable_storage ps;
    ASSERT_TRUE(ps.load_from_binary(buff, strict_limits()));
    bool value = byte == 0;
    ASSERT_TRUE(ps.get_value("f", value, nullptr));
    EXPECT_EQ(byte == 1, value);
    std::string again;
    ASSERT_TRUE(ps.store_to_binary(again));
    EXPECT_EQ(buff, again);
    // Legacy control on the same valid encoding.
    epee::serialization::portable_storage legacy;
    EXPECT_TRUE(legacy.load_from_binary(buff));
  }
  const std::string array = raw_bool_array({0, 1, 1, 0});
  epee::serialization::portable_storage ps;
  ASSERT_TRUE(ps.load_from_binary(array, strict_limits()));
  std::string again;
  ASSERT_TRUE(ps.store_to_binary(again));
  EXPECT_EQ(array, again); // Elements re-encode as exactly 0/1.
  epee::serialization::portable_storage legacy;
  EXPECT_TRUE(legacy.load_from_binary(array));
}

TEST(portable_storage_tests, strict_bool_rejects_other_bytes)
{
  for (uint8_t byte : {uint8_t(2), uint8_t(255)})
  {
    EXPECT_FALSE(strict_load(raw_bool_scalar(byte), strict_limits()));
    EXPECT_FALSE(strict_load(raw_bool_array({byte}), strict_limits()));
    EXPECT_FALSE(strict_load(raw_bool_array({0, 1, byte}), strict_limits())); // After valid elements.
    EXPECT_FALSE(strict_load(raw_bool_array({byte, 0}), strict_limits()));
  }
  // Control: the same array framing with only valid bytes still loads.
  EXPECT_TRUE(strict_load(raw_bool_array({0, 1, 0}), strict_limits()));
}

TEST(portable_storage_tests, strict_mode_requires_complete_input_and_keeps_encoding)
{
  const std::string buff = epee::serialization::store_t_to_binary(strict_fixture{});
  epee::serialization::portable_storage ps;
  ASSERT_TRUE(ps.load_from_binary(buff, strict_limits()));
  std::string again;
  ASSERT_TRUE(ps.store_to_binary(again));
  EXPECT_EQ(buff, again); // Strict loading preserves the tree exactly.
  strict_fixture loaded;
  loaded.a = 0; loaded.text.clear(); loaded.list.clear();
  ASSERT_TRUE(epee::serialization::load_t_from_binary(loaded, buff, strict_limits()));
  EXPECT_EQ(1u, loaded.a);
  EXPECT_EQ("abc", loaded.text);
  EXPECT_EQ((std::vector<uint64_t>{1, 2, 3, 4, 5}), loaded.list);

  const std::string trailing = buff + std::string(1, '\0');
  EXPECT_FALSE(strict_load(trailing, strict_limits()));
  EXPECT_FALSE(strict_load(buff.substr(0, buff.size() - 1), strict_limits()));
  // Legacy default: trailing bytes are still accepted.
  epee::serialization::portable_storage legacy;
  EXPECT_TRUE(legacy.load_from_binary(trailing));
}

TEST(portable_storage_tests, strict_mode_rejects_duplicate_names_default_keeps_first)
{
  const std::string buff = raw_header() + std::string(1, static_cast<char>(2 << 2)) +
    raw_u64_entry("a", 1) + raw_u64_entry("a", 2);
  EXPECT_FALSE(strict_load(buff, strict_limits()));
  epee::serialization::portable_storage legacy;
  ASSERT_TRUE(legacy.load_from_binary(buff));
  uint64_t value = 0;
  ASSERT_TRUE(legacy.get_value("a", value, nullptr));
  EXPECT_EQ(1u, value);
}

TEST(portable_storage_tests, strict_limits_are_inclusive_and_charged_before_loading)
{
  // Root section: 4 entries, 1 section, 5 array elements.
  const std::string buff = epee::serialization::store_t_to_binary(strict_fixture{});
  auto l = strict_limits();
  l.max_storage_entries_count = 4;
  EXPECT_TRUE(strict_load(buff, l));
  l.max_storage_entries_count = 3;
  EXPECT_FALSE(strict_load(buff, l));
  l = strict_limits();
  l.max_sections_entries_count = 1;
  EXPECT_TRUE(strict_load(buff, l));
  l = strict_limits();
  l.max_array_elements_count = 5;
  EXPECT_TRUE(strict_load(buff, l));
  l.max_array_elements_count = 4;
  EXPECT_FALSE(strict_load(buff, l));
  l.max_string_length = 3;
  l.max_array_elements_count = 5;
  EXPECT_TRUE(strict_load(buff, l));
  l.max_string_length = 2;
  EXPECT_FALSE(strict_load(buff, l));

  // Legacy semantics are unchanged: exclusive maxima, no strict fields used.
  epee::serialization::portable_storage_limits legacy_limits;
  legacy_limits.max_array_elements_count = 5;
  EXPECT_FALSE(strict_load(buff, legacy_limits));
  legacy_limits.max_array_elements_count = 6;
  EXPECT_TRUE(strict_load(buff, legacy_limits));
}

TEST(portable_storage_tests, strict_nesting_limit_default_unchanged)
{
  epee::serialization::portable_storage deep;
  auto h = deep.open_section("n", nullptr, true);
  for (int i = 0; i != 15; ++i)
    h = deep.open_section("n", h, true); // Root plus 16 nested sections: depth 17.
  std::string buff;
  ASSERT_TRUE(deep.store_to_binary(buff));
  auto l = strict_limits();
  EXPECT_FALSE(strict_load(buff, l));
  l.max_nesting_depth = 17;
  EXPECT_TRUE(strict_load(buff, l));
  epee::serialization::portable_storage legacy;
  EXPECT_TRUE(legacy.load_from_binary(buff));
}

TEST(portable_storage_tests, strict_mode_polls_cooperative_cancellation)
{
  strict_fixture big;
  big.list.assign(300, 7); // Enough reader steps to reach a periodic poll.
  const std::string buff = epee::serialization::store_t_to_binary(big);
  auto l = strict_limits();
  size_t polls = 0;
  l.stop_context = &polls;
  l.should_stop = [](void* context) { ++*static_cast<size_t*>(context); return false; };
  EXPECT_TRUE(strict_load(buff, l));
  EXPECT_GT(polls, 0u);
  l.should_stop = [](void*) { return true; };
  EXPECT_FALSE(strict_load(buff, l));
  // Default (legacy) limits have no stop hook and still load the same bytes.
  EXPECT_TRUE(strict_load(buff, epee::serialization::gdefault_portable_storage_limits));
}

TEST(portable_storage_tests, strict_string_length_is_checked_before_narrowing)
{
  // Encoded lengths are refused from the header alone: none is allocated and
  // no payload is supplied. On 64-bit size_t, 2^32 would survive narrowing
  // anyway; the fixture is a 32-bit regression but only executes 64-bit here.
  const uint64_t max_varint = (uint64_t(1) << 62) - 1; // Largest legal portable varint.
  for (uint64_t len : {uint64_t(1) << 32, (uint64_t(1) << 32) + 3, max_varint})
  {
    SCOPED_TRACE(len);
    const std::string buff = raw_string_entry(raw_varint(len, 8), "abc");
    EXPECT_FALSE(strict_load(buff, strict_limits()));
    // Even with every configurable maximum wide open.
    auto open = strict_limits();
    open.max_string_length = std::numeric_limits<size_t>::max();
    EXPECT_FALSE(strict_load(buff, open));
  }

  auto l = strict_limits(); // max_string_length = 1000
  const std::string at_limit(1000, 'q');
  // Exact-limit controls: accepted in strict mode and by the legacy default.
  for (size_t width : {size_t(2), size_t(4), size_t(8)})
  {
    SCOPED_TRACE(width);
    const std::string buff = raw_string_entry(raw_varint(at_limit.size(), width), at_limit);
    epee::serialization::portable_storage ps;
    ASSERT_TRUE(ps.load_from_binary(buff, l));
    std::string value;
    ASSERT_TRUE(ps.get_value("s", value, nullptr));
    EXPECT_EQ(at_limit, value);
    epee::serialization::portable_storage legacy;
    EXPECT_TRUE(legacy.load_from_binary(buff));
    // One byte over the configured maximum is refused.
    EXPECT_FALSE(strict_load(raw_string_entry(raw_varint(at_limit.size() + 1, width), at_limit + "q"), l));
  }
  const std::string empty = raw_string_entry(raw_varint(0, 1), "");
  EXPECT_TRUE(strict_load(empty, l));
  EXPECT_TRUE(strict_load(empty, epee::serialization::gdefault_portable_storage_limits));

  // Insufficient input: the declared length exceeds the remaining bytes.
  EXPECT_FALSE(strict_load(raw_string_entry(raw_varint(4, 1), "abc"), l));
  EXPECT_FALSE(strict_load(raw_string_entry(raw_varint(1000, 8), std::string(999, 'q')), l));
  // Truncated length varint (8-byte mark with fewer bytes present).
  const std::string wide = raw_varint(3, 8);
  for (size_t cut : {size_t(1), size_t(4), size_t(7)})
    EXPECT_FALSE(strict_load(raw_string_entry(wide.substr(0, cut), ""), l));
  // Exact-length controls, strict and legacy default.
  EXPECT_TRUE(strict_load(raw_string_entry(raw_varint(3, 1), "abc"), l));
  epee::serialization::portable_storage legacy;
  EXPECT_TRUE(legacy.load_from_binary(raw_string_entry(raw_varint(3, 1), "abc")));
}



