#include "gtest/gtest.h"
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

