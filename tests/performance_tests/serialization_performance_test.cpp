// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "serialization_performance_test.h"
#include "currency_core/blockchain_storage.h"
#include "profile_tools.h"

bool prepare_average_tx_chain_entry(currency::transaction_chain_entry& test_entry)
{
  test_entry.m_global_output_indexes.push_back(12321);
  test_entry.m_global_output_indexes.push_back(23);
  test_entry.m_global_output_indexes.push_back(1341551);
  test_entry.m_global_output_indexes.push_back(243);
  test_entry.m_global_output_indexes.push_back(22444);
  test_entry.m_global_output_indexes.push_back(243);
  test_entry.m_global_output_indexes.push_back(22444);
  test_entry.m_keeper_block_height = 112313;
  test_entry.m_spent_flags.push_back(true);
  test_entry.m_spent_flags.push_back(true);
  test_entry.m_spent_flags.push_back(true);
  test_entry.m_spent_flags.push_back(true);
  test_entry.m_spent_flags.push_back(true);
  std::string tx_hex =
    "010101c0843d0118b5170000000000002b50f8ecb2d511d6690641e319274deb962b22806f046391720275a5dd07e051000002164a889edcc9ede33a00fe82ce3dfc450a5e7dab59666a1f1d03f"
    "ff5e065dd3cb912c00628a287d77b888256aaf26d1196857628c8042fd2c0f901d0d1a09cdb8891ea120201013653df108e62f036a276a245371cb5b7b620b70cce766590557248677d7acf0dd4"
    "252351f9169ff7ca2df1e76e76319cc043dad38a8adbc80f56f1b0ac257902020c01450450524f508e06991d10666a19373802553c157750bad9447f2c99581fcdd00cc5e2aacc939c2d28a2d67"
    "ef770c059f22d193226888c9c760d1d65762a7255d83ac6ecf7299cc03dd3395d7d9203887c2c2a2f20ba5241ff1feb43cc0faced663b78b4d12b79e045dd50aa5b1fa076b990399fec1b405d91"
    "f638dbb25dc2813ac6183ee0a4efc5a0912058911d537541b4ee60d2ac04c0bfa158ac4c31774eb2d793453f1ad558ddefb059bff7ec31748d4864c4e23caf6f61e2bc477b8413969255d113660"
    "578485294ae9373a896152318e7d099979edcd4a197a05e372488373e724d9caed058e924cf9854efa810b4be19ac7f8664a443cd1786bb79e8f604f05ab6a0f3118a285ade2b048fddc68fdb53"
    "89504824ca6a07b7f4efe17027b2e8be03620a86cbba9fcf2dc03958c28b29bb17b84ac2251032f57f7f546581b47c77091d161f97276f13c0c65487b3e3b84891f2d20840b0d8cd388e0c5a2a7"
    "c00064975ef5fdfe527b329b42c901439c136b87ce638f17267f63c96d998f31146e16778bd170e167e0da8e507a4a27dcf215ff1c1ac51dc338039c64a421c49dc06cfb63ab58af41dc5654da0"
    "945194f3edde7936f95246ef7ebe3a3493749d70355feba31b606ae5d9ce28e501f51e466b4f9f6c1adbf6d3f34f61645170ece1a01c9a74dd8095db4f3cd4f0d5d7fde8407ee86f620b3f1a868"
    "2aab9ee01c86dfff9f2e06df2ee2978a401afe135ac68d5a0ab5c63baa4de44612d223dabf2c1792b2e394eb996299741e78c853a13c5a7bd3210de1f70678f02a6f03b38f3d130b08b3108ec66"
    "9f77facb0035d546c31dc3afd54a4213744886cb181dcc58d60faff25dc9d6bc714dd9e6deb3cf155bb70a058f8ecc92bdb27ebaff3a279c806da3af3adb6f09b095eb0f0f0fa29e55b770d04bd"
    "3f4bd6476ad5b816556274ef10fc507f8a88300760b9562673479f9c3cb81419b599e8fdccb0a9e9711a36022d676a000a0e37dbec5b89864c64087238cf6c78675d1ebb7be219756f210e3e66c"
    "7d8ee9a0972f9d50ef387046e70079d362dd7cdd0563084dba6e666bae1d4512872d65a04303c06d226d7dc5673be5f57c00030a95416d6daa61377a33415c7b9f0fb5006056833e9667f7a2701"
    "daa7e21c9c880739f30ab";
  std::string tx_blob;
  bool r = epee::string_tools::parse_hexstr_to_binbuff(tx_hex, tx_blob);
  CHECK_AND_NO_ASSERT_MES(r, false, "");

  r = t_unserializable_object_from_blob(test_entry.tx, tx_blob);
  CHECK_AND_NO_ASSERT_MES(r, false, "");
  return true;
}

bool run_serialization_performance_test()
{

#define SERIALIZATION_TEST_REPEAT_COUNT 1000000

  currency::transaction_chain_entry test_entry = AUTO_VAL_INIT(test_entry);
  //make fake transaction_chain_entry that looks like average entry
  prepare_average_tx_chain_entry(test_entry);
  

  uint64_t total_sz = 0;  // this variable kept to avoid compiler optimization of unused objects 
  std::string blb;

  LOG_PRINT_L0("crypto_serialization_store....");
  TIME_MEASURE_START(crypto_serialization_store);
  for (uint64_t i = 0; i != SERIALIZATION_TEST_REPEAT_COUNT; i++)
  {
    t_serializable_object_to_blob(test_entry, blb);
    total_sz += blb.size();
  }
  TIME_MEASURE_FINISH(crypto_serialization_store);

  LOG_PRINT_L0("crypto_serialization_load....");
  TIME_MEASURE_START(crypto_serialization_load);
  for (uint64_t i = 0; i != SERIALIZATION_TEST_REPEAT_COUNT; i++)
  {
    t_unserializable_object_from_blob(test_entry, blb);
    total_sz += test_entry.tx.attachment.size();
  }
  TIME_MEASURE_FINISH(crypto_serialization_load);


  LOG_PRINT_L0("boost_serialization_store....");
  TIME_MEASURE_START(boost_serialization_store);
  for (uint64_t i = 0; i != SERIALIZATION_TEST_REPEAT_COUNT; i++)
  {
    tools::serialize_obj_to_buff(test_entry, blb);
    total_sz += blb.size();
  }
  TIME_MEASURE_FINISH(boost_serialization_store);

  LOG_PRINT_L0("boost_serialization_load....");
  TIME_MEASURE_START(boost_serialization_load);
  for (uint64_t i = 0; i != SERIALIZATION_TEST_REPEAT_COUNT; i++)
  {
    tools::unserialize_obj_from_buff(test_entry, blb);
    total_sz += test_entry.tx.attachment.size();
  }
  TIME_MEASURE_FINISH(boost_serialization_load);

  LOG_PRINT_GREEN("Results: " << ENDL
    << "crypto_serialization_store: " << crypto_serialization_store << ENDL
    << "crypto_serialization_load: " << crypto_serialization_load << ENDL
    << "boost_serialization_store: " << boost_serialization_store << ENDL
    << "boost_serialization_load: " << boost_serialization_load << ENDL, 
    LOG_LEVEL_0)
    
    return true;

}