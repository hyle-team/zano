// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*



DISCLAIMER:
Due to hash change (Wild Keccak -> x11 -> Ethash) this test should be completely rewritten.


Also consider using this function:

  crypto::hash get_block_longhash(const blobdata& bd)
  {
    crypto::hash result = null_hash;
    block b = AUTO_VAL_INIT(b);
    if (parse_and_validate_block_from_blob(bd, b))
    {
      blobdata local_bd = bd;
      access_nonce_in_block_blob(local_bd) = 0;
      crypto::hash bl_hash = crypto::cn_fast_hash(local_bd.data(), local_bd.size());
      result = get_block_longhash(get_block_height(b), bl_hash, b.nonce);
    }
    return result;
  }



#include "gtest/gtest.h"

#include "currency_core/currency_format_utils.h"

using namespace currency;

const char* test_hashes[51] = {
  "0069fbf4c58a4d48ef3134f277c7a711da33fc030fdbf4a6a9eee372616c0f57",
  "2ea7ddcbc819ad4fe96def105335773a19e83b8ee04272c63ec034425fbbcccd",
  "995cd314592747e673487a8eab8bd8a97bc32c078cc2b975b73cae2c031af7c3",
  "8daba3abaeaf44848d4800c44b237638e2ac1650ac58917e34ede513825696ff",
  "95d5d7e08e2cc6d81211342b43f7a6ccdd0f05248bd59d5390bcc6c5cea89aae",
  "7949d3d906bb66871bca78ef0e4b56f999e000fff987a58b90efef5073b6b27c",
  "87ce7ee6527a533c5ac900b0b28d88996ad965d18a8182329e187cfaa96e1272",
  "cd6385547b7f64cd2e09455d0b540334d542302600a8f08c7ccd265759a4b324",
  "4ba77c0da65bee3088eaca598bfe63a71c257850a16535f3e2e2518cef306aed",
  "794372f55692df868c4ca53ff879aa7dddbb290f3fe4a19297ddf4d5e08fe6ad",
  "d16e207f9fdb2e61d738cd4318a6024cba9512d56b1624b76d03e5453fb88818",
  "39a7669648a0f54f0a1bc311267a0cc124ceee34bfbd6a60a8d43ba5437e8a4f",
  "0e6d1192662c71b65201a27d35f1da933e7070c42186f104c9ae1c231ea09f8a",
  "9b8cfe7c7acef37ff11b3e9bdfd69c998c63a42adf42b569ab111ef1cfa12afe",
  "f83187e1e7c6c3f4d0f7ab964da16f39a78a47dc8ce0aa8df55abc689283f3ed",
  "7e2fd86fb9de617bba76a9382772a7aa4369d626c5e127f9699fc464ea1f21b2",
  "9a53eb68c7e8b1a4177029014350dea2b7023aae777d5a2509cedb946e794ccb",
  "127442cf4d970764cd903b0620c613ffdf92ad43aef9054edf54647ee8ee9a79",
  "d7fc147f5a5ddf4f8131c2721498f2bd10a36a50d79bd7f7b1c03e765c4fe16a",
  "a7b3729a2a4a9c393e342003edf265aa88a704ff4a659f3ef5326a666f73ebe3",
  "3574f8e0c1dd85b704d2dbd9dddc5e9007bb9903e5654f921acd2e1e7be8d6ea",
  "a5037e77c4d2c789e32b22e74366c32ee0697c5b4f958d98e3743a704ffc977a",
  "d417520014ada3ec182b5cc1ce05ab29b204b1e3545ebee44136eb15801ec042",
  "7c17de8607cdcb7776671803504f1f78015df93ee8ae9c1e269ec5dc228e0f31",
  "c4679efc2fc306fcb1f3ea7c509ee87daa3b66e54696e63ca4a58235e15f72fc",
  "4de02c7066962e6f31c1a686d241d067498d2d326ae69b6588baf49fd62b927d",
  "035c335ce07a60387e835858f690376c87993585f4e26964cf33623a90decf2d",
  "14ccc27cfb8f2d643cdb775c0afbed38bd30cbe64087bfb12a2a8ee5d185f16a",
  "4e19d5a360b3c3de7364d8b7fa53f1c0d051ddc993b32f8356a2123020bc4d94",
  "e10e90853db58de03423c5c9f02751c002b11db0baef5428612fc3f2e71fd52b",
  "e5f91c2a5a6b33652d2af2d473e07996a26aadf9c545be361555191ef4d8b101",
  "2f595481d3fc0d8636d73b95b1b94ea50aeb956a4eeeb06d45d3d8691de5984c",
  "5f53e64b1a06be5b2b7b597f462a0ac079cb4a05243b560165688c90867580f2",
  "3254d508fe51c42526045048bee5cd29516477180ea8522f1f3b0cc249d98959",
  "df364e5c4c10083b8d19639ea04cb23c8268fef04c6601f8fbff284cca4a6e64",
  "1a15e83c9bc8bd6ca38e05f2e3318eb50deaa1b4f0d44316d0a16ac608fcf8ec",
  "1e4614f61d7c78480fbac8d79863b05ac7c47fd7856489796b10aada9b472b87",
  "1ea359c55bef9fa54fc0a89dcdab63281f93ffd0d373d7f29d96ba4f9df0f560",
  "f4f58ce5f6afba27075b0d1566a2534cd12a1d546792c50acc0506ebfaf6e5b6",
  "4b8feee88ff37929cab4c2aa50334b4908f6b0019da75a4cbb00d759737b0b97",
  "07c6c8f4303e456b7f350ffc19a14c0daa303ef38cbf8ed729d0a65cc0d0fe31",
  "2cc3c3c065689c1e2639e76aa75d7c7a2356045038bd8e39492f9b5752356b0c",
  "ab6c1c109b0ca67f75a54a8b353f817b0704b8dc70ffc7d97eaa45616348f4eb",
  "53e23a7291704817fc8a84cf332ef99aa5388cf6cd0d3ca59f2483ff57703849",
  "408a8c0141abbdef767669a1624f2d49b24cc277a11d042e318cc64fd41001dd",
  "3daf24a36a87fd5c2eda7cf4d58da3eafc3cc26e2a343d97a02a12ecf76eb0a1",
  "0c53057b47dac5a7539ec806148fe6534186af5325b0f35014f1abfb4fcf0be1",
  "b5404f47ddb58667422ef37a1fa9611565b0c6d4ce07b106ef43832e5a57988d",
  "4ae02032ed82d1aed440b4351bd4f0baf54e3c18b853b48f60aea2556d0b1e2b",
  "5a017e9303c9d18efaf8716445a3786d81126030f2803646db01d424161461e1",
  "83325584cdca2a153a26fb3f5b7a9cc89171bb62c1db85bdba7e113c773f5f41"
  };

  const char* test_srings[] = {
    "",
    "President Obama is ready to use American air power in targeted and precise military action",
    "Roy Hodgson insisted last night that he would not quit despite admitting that England's World Cup dream was all but over",
  };

  struct hash_result
    {
      size_t scratch_size;
      size_t string_no;
      const char* expected_hash;
    };

  
  hash_result expected_results[] = 
  {
    {1,0,"b2767f554a15e3471c854118f62c2cc0959ccd6c67a70e75f80f85f548a715bb"},
    {10,0,"7071b26139d8469916adba2ada612adc7c767344564907e977bb7466d7cc69ab"},
    {100,0,"3fd77bcacf0c579618a9b5f054223692a5cf56926f95a1a5df4a57e044100a20"},
    {1000,0,"5a7fc927e8eff077557cc4ad6a4691d4036548ba3131374a543d685e94ea1ea0"},
    {10000,0,"d672d0d532fcd0e3a6796975a1ee976f0eae0572d531c5eb45fe0c178099f48f"},
    {100000,0,"c7cadb46cbb5575d5bd01d9a8272e427a0f5bc5df69e400c93f5595da78c52d1"},
    {1,1,"1a7656a4f0403c3cd2ef7a1471bd5d7a18c7965921e1d03ad12209cca6302cb9"},
    {10,1,"94a747ddb33cb6d3908a1e47930903dfd06c365020db0a176b4b5609a37a19f3"},
    {100,1,"bcabc30513f4f2b7ff907e86d0fed06894b11a3b11e3e273c26e08f7b5f0666f"},
    {1000,1,"03e2ede9f00079404457c385375122e9a0e6f10216f62f70d789cc9ab4491694"},
    {10000,1,"ed9594fc45b5161af4d16293449c042ad2508f0c86ef6f42352ec72ae6d94a0a"},
    {100000,1,"6650ca0ad0132f85ae757f9a5e41efb638fde7efe2bf2a7e113a7d3d6bd8189d"},
    {1,2,"b1d3119e86600023f18aa1e633059de545c835dff2392457e6cac92f1a0da5be"},
    {10,2,"fdb1ff6b103d056fe3d68de3deabf0e568cbca56ab5b3b46197a078d5fc2a83b"},
    {100,2,"f9acedad2cdca5d5fe2eb0da62d651bb5a8f7dfc9becc4ba9530d71f531effba"},
    {1000,2,"4f319beb818183d51cb203c9fa26cd250c67bf3e1fec62995d122dbf8df7292b"},
    {10000,2,"e5d95f8465af6e41f132e6e3a625d8b4e6f41c30a85808137b1ae07635bd7804"},
    {100000,2,"8ea12900e89a9002080605f06d85b6aab3c584cef761886f450c163ee65168f3"}
  };
    


bool add_hash_str_to_hash_vector(const std::string& str, std::vector<crypto::hash>& scratchpad)
{
  crypto::hash h = null_hash;
  bool r = epee::string_tools::parse_tpod_from_hex_string(str, h);
  CHECK_AND_ASSERT_THROW_MES(r, "wrong hash str");
  scratchpad.push_back(h);
  return true;
}

void get_scratchpad(size_t hashes_count, std::vector<crypto::hash>& scratchpad)
{
  scratchpad.reserve(hashes_count);

  size_t count_text_hashes = (sizeof(test_hashes)/sizeof(test_hashes[0]));

  for(size_t i = 0; i != hashes_count;i++)
    if(i < count_text_hashes)
    {
      add_hash_str_to_hash_vector(test_hashes[i], scratchpad);
    }else
    {
      scratchpad.push_back(scratchpad[i%count_text_hashes]);
    }
}

template<typename hash_cb>
bool check_hash(hash_cb hcb)
{
  for(size_t i = 0; i < sizeof(expected_results)/sizeof(expected_results[0]); i++) 
  {
    std::vector<crypto::hash> scratchpad;
    get_scratchpad(expected_results[i].scratch_size, scratchpad);
    crypto::hash h_expected = null_hash;
    bool r = epee::string_tools::parse_tpod_from_hex_string(expected_results[i].expected_hash, h_expected);
    CHECK_AND_ASSERT_THROW_MES(r, "wrong expected hash");

    crypto::hash res = hcb(test_srings[expected_results[i].string_no], scratchpad);
    CHECK_AND_ASSERT_MES(res == h_expected, false, "wrong pow_hash: " << res << ", expected: " );

  }
  return true;
}


TEST(pow_tests, test_reference_func)
{

  bool r = check_hash([](const std::string& blob, std::vector<crypto::hash>& scratchpad){
    
    return get_block_longhash(blob);
  });
  ASSERT_TRUE(r);

  r = check_hash([](const std::string& blob, std::vector<crypto::hash>& scratchpad){

    return get_block_longhash(blob);
  });
  ASSERT_TRUE(r);
}

*/