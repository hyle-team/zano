// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "chaingen.h"
#include "include_base_utils.h"
#include "currency_core/account.h"
#include "currency_core/currency_format_utils.h"
#include "misc_language.h"
#include "chaingen_helpers.h"
using namespace currency;



bool test_transaction_generation_and_ring_signature()
{
  currency::hard_forks_descriptor hf = AUTO_VAL_INIT(hf);

  account_base miner_acc1;
  miner_acc1.generate();
  account_base miner_acc2;
  miner_acc2.generate();
  account_base miner_acc3;
  miner_acc3.generate();
  account_base miner_acc4;
  miner_acc4.generate();
  account_base miner_acc5;
  miner_acc5.generate();
  account_base miner_acc6;
  miner_acc6.generate();

  std::string add_str = miner_acc3.get_public_address_str();

  uint64_t block_reward_without_fee = 0;
  uint64_t block_reward = 0;

  account_base rv_acc;
  rv_acc.generate();
  account_base rv_acc2;
  rv_acc2.generate();
  transaction tx_mine_1;
  construct_miner_tx(0, 0, 0, 10, 0, miner_acc1.get_keys().account_address, miner_acc1.get_keys().account_address, tx_mine_1, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4);
  transaction tx_mine_2;
  construct_miner_tx(0, 0, 0, 0, 0, miner_acc2.get_keys().account_address, miner_acc2.get_keys().account_address, tx_mine_2, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4);
  transaction tx_mine_3;
  construct_miner_tx(0, 0, 0, 0, 0, miner_acc3.get_keys().account_address, miner_acc3.get_keys().account_address, tx_mine_3, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4);
  transaction tx_mine_4;
  construct_miner_tx(0, 0, 0, 0, 0, miner_acc4.get_keys().account_address, miner_acc4.get_keys().account_address, tx_mine_4, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4);
  transaction tx_mine_5;
  construct_miner_tx(0, 0, 0, 0, 0, miner_acc5.get_keys().account_address, miner_acc5.get_keys().account_address, tx_mine_5, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4);
  transaction tx_mine_6;
  construct_miner_tx(0, 0, 0, 0, 0, miner_acc6.get_keys().account_address, miner_acc6.get_keys().account_address, tx_mine_6, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4);

  //fill inputs entry
  typedef tx_source_entry::output_entry tx_output_entry;
  std::vector<tx_source_entry> sources;
  sources.push_back(AUTO_VAL_INIT(tx_source_entry()));
  tx_source_entry& src = sources.back();
  src.amount = 70368744177663;
  {
    tx_output_entry oe;
    oe.out_reference = 0;
    oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_1.vout[0]).target).key;
    src.outputs.push_back(oe);

    oe.out_reference = 1;
    oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_2.vout[0]).target).key;
    src.outputs.push_back(oe);

    oe.out_reference = 2;
    oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_3.vout[0]).target).key;
    src.outputs.push_back(oe);

    oe.out_reference = 3;
    oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_4.vout[0]).target).key;
    src.outputs.push_back(oe);

    oe.out_reference = 4;
    oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_5.vout[0]).target).key;
    src.outputs.push_back(oe);

    oe.out_reference = 5;
    oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_6.vout[0]).target).key;
    src.outputs.push_back(oe);

    crypto::public_key tx_pub_key = null_pkey;
    currency::parse_and_validate_tx_extra(tx_mine_2, tx_pub_key);
    src.real_out_tx_key = tx_pub_key;
    src.real_output = 1;
    src.real_output_in_tx_index = 0;
  }
  //fill outputs entry
  tx_destination_entry td;
  td.addr.push_back(rv_acc.get_keys().account_address);
  td.amount = 69368744177663;
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(td);

  transaction tx_rc1;
  std::vector<currency::attachment_v> attachments;
  bool r = construct_tx(miner_acc2.get_keys(), sources, destinations, attachments, tx_rc1, get_tx_version(0, hf), 0);
  CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");

  crypto::hash pref_hash = get_transaction_prefix_hash(tx_rc1);
  std::vector<const crypto::public_key *> output_keys;
  output_keys.push_back(&boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_1.vout[0]).target).key);
  output_keys.push_back(&boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_2.vout[0]).target).key);
  output_keys.push_back(&boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_3.vout[0]).target).key);
  output_keys.push_back(&boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_4.vout[0]).target).key);
  output_keys.push_back(&boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_5.vout[0]).target).key);
  output_keys.push_back(&boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx_mine_6.vout[0]).target).key);
  r = crypto::check_ring_signature(pref_hash, boost::get<txin_to_key>(tx_rc1.vin[0]).k_image, output_keys, &boost::get<currency::NLSAG_sig>(tx_rc1.signatures[0]).s[0]);
  CHECK_AND_ASSERT_MES(r, false, "failed to check ring signature");

  std::vector<wallet_out_info> outs;
  uint64_t money = 0;
  crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
  r = lookup_acc_outs(rv_acc.get_keys(), tx_rc1, outs,  money, derivation);
  CHECK_AND_ASSERT_MES(r, false, "failed to lookup_acc_outs");
  CHECK_AND_ASSERT_MES(td.amount == money, false, "wrong money amount in new transaction");
  money = 0;
  derivation = AUTO_VAL_INIT(derivation);
  r = lookup_acc_outs(rv_acc2.get_keys(), tx_rc1, outs,  money, derivation);
  CHECK_AND_ASSERT_MES(r, false, "failed to lookup_acc_outs");
  CHECK_AND_ASSERT_MES(0 == money, false, "wrong money amount in new transaction");
  return true;
}

bool test_block_creation()
{
  uint64_t vszs[] = {80,476,476,475,475,474,475,474,474,475,472,476,476,475,475,474,475,474,474,475,472,476,476,475,475,474,475,474,474,475,9391,476,476,475,475,474,475,8819,8301,475,472,4302,5316,14347,16620,19583,19403,19728,19442,19852,19015,19000,19016,19795,19749,18087,19787,19704,19750,19267,19006,19050,19445,19407,19522,19546,19788,19369,19486,19329,19370,18853,19600,19110,19320,19746,19474,19474,19743,19494,19755,19715,19769,19620,19368,19839,19532,23424,28287,30707};
  std::vector<uint64_t> szs(&vszs[0], &vszs[90]);
  account_public_address adr;
  bool r = get_account_address_from_str(adr, "ZxDLGBGXbjo5w51tJkvxEPHFRr7Xft4hf33N8EkJPndoGCqocQF1mzpZqYwXByx5gMbfQuPAAB9vj79EFR6Jwkgu1o3aMQPwJ");
  CHECK_AND_ASSERT_MES(r, false, "failed to import");
  uint64_t block_reward_without_fee = 0;
  uint64_t block_reward = 0;
  block b;
  r = construct_miner_tx(90, epee::misc_utils::median(szs), 3553616528562147, 33094, 10000000, adr, adr, b.miner_tx, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4);
  return r;
}

bool test_example_key_derivation()
{
  keypair A = keypair::generate();
  keypair B = keypair::generate();
  keypair C = keypair::generate();

  LOG_PRINT_CYAN(ENDL <<
    "keypair A: pub:" << A.pub << " sec:" << A.sec << ENDL <<
    "keypair B: pub:" << B.pub << " sec:" << B.sec,
    LOG_LEVEL_0);

  crypto::key_derivation derivation_Ap_Bs = AUTO_VAL_INIT(derivation_Ap_Bs);
  bool r = crypto::generate_key_derivation(A.pub, B.sec, derivation_Ap_Bs);
  CHECK_AND_ASSERT_MES(r, false, "call failed: generate_key_derivation(" << A.pub << ", " << B.sec << ")");

  crypto::key_derivation derivation_Bp_As = AUTO_VAL_INIT(derivation_Bp_As);
  r = crypto::generate_key_derivation(B.pub, A.sec, derivation_Bp_As);
  CHECK_AND_ASSERT_MES(r, false, "call failed: generate_key_derivation(" << B.pub << ", " << A.sec << ")");

  LOG_PRINT_CYAN(ENDL <<
    "derivation_Ap_Bs: " << derivation_Ap_Bs << ENDL <<
    "derivation_Bp_As: " << derivation_Bp_As,
    LOG_LEVEL_0);

  // derive a public key and see what happens
  crypto::public_key derived_pub_key_1 = AUTO_VAL_INIT(derived_pub_key_1);
  r = crypto::derive_public_key(derivation_Ap_Bs, 42, C.pub, derived_pub_key_1);
  CHECK_AND_ASSERT_MES(r, false, "call failed: derive_public_key(" << derivation_Ap_Bs << ", 42, " << C.pub << ")");

  LOG_PRINT_CYAN(ENDL <<
    "original public key: " << C.pub << ENDL <<
    "  -->> via derivation_Ap_Bs " << derivation_Ap_Bs << ENDL <<
    "derived public key:  " << derived_pub_key_1,
    LOG_LEVEL_0);

  crypto::public_key derived_pub_key_2 = AUTO_VAL_INIT(derived_pub_key_2);
  r = crypto::derive_public_key(derivation_Bp_As, 42, C.pub, derived_pub_key_2);
  CHECK_AND_ASSERT_MES(r, false, "call failed: derive_public_key(" << derivation_Bp_As << ", 42, " << C.pub << ")");

  LOG_PRINT_CYAN(ENDL <<
    "original public key: " << C.pub << ENDL <<
    "  -->> via derivation_Bp_As " << derivation_Bp_As << ENDL <<
    "derived public key:  " << derived_pub_key_2,
    LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(derived_pub_key_1 == derived_pub_key_2, false, "derived_pub_key_1 != derived_pub_key_2");

  // derive a secrec key and see what happens
  crypto::secret_key derived_secret_key_1 = AUTO_VAL_INIT(derived_secret_key_1);
  crypto::derive_secret_key(derivation_Ap_Bs, 42, C.sec, derived_secret_key_1);
  LOG_PRINT_CYAN(ENDL <<
    "original secret key: " << C.sec << ENDL <<
    "  -->> via derivation_Ap_Bs " << derivation_Ap_Bs << ENDL <<
    "derived secret key:  " << derived_secret_key_1,
    LOG_LEVEL_0);

  crypto::secret_key derived_secret_key_2 = AUTO_VAL_INIT(derived_secret_key_2);
  crypto::derive_secret_key(derivation_Bp_As, 42, C.sec, derived_secret_key_2);
  LOG_PRINT_CYAN(ENDL <<
    "original secret key: " << C.sec << ENDL <<
    "  -->> via derivation_Bp_As " << derivation_Bp_As << ENDL <<
    "derived secret key:  " << derived_secret_key_2,
    LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(derived_secret_key_1 == derived_secret_key_2, false, "derived_secret_key_1 != derived_secret_key_2");

  crypto::public_key pub_key = AUTO_VAL_INIT(pub_key);
  r = crypto::secret_key_to_public_key(derived_secret_key_1, pub_key);
  CHECK_AND_ASSERT_MES(r, false, "call failed: secret_key_to_public_key(" << derived_secret_key_1 << ", )");

  CHECK_AND_ASSERT_MES(pub_key == derived_pub_key_1, false, "pub_key (obtained from derived_secret_key_1) != derived_pub_key_1");

  return true;
}

bool test_transactions()
{
  if(!test_transaction_generation_and_ring_signature())
    return false;
  if(!test_block_creation())
    return false;
  if (!test_example_key_derivation())
    return false;


  return true;
}
