// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "epee/include/include_base_utils.h"
#include "wallet/wallet2.h"
#include "../core_tests/random_helper.h"

TEST(wallet, encrypt_buffer)
{
  currency::account_base acc;
  acc.generate();
  
  tools::wallet2 wallet;
  wallet.assign_account(acc);

  std::string message = get_random_text(129);
  std::string ciphertext;
  ASSERT_TRUE(wallet.encrypt_buffer(message, ciphertext)) << "encryption failed";

  std::string decrypted_message;
  ASSERT_TRUE(wallet.decrypt_buffer(ciphertext, decrypted_message)) << "decryption failed";

  ASSERT_EQ(message, decrypted_message);


  // another acc, same message
  acc.generate();
  wallet.assign_account(acc);

  std::string ciphertext2;
  ASSERT_TRUE(wallet.encrypt_buffer(message, ciphertext2)) << "encryption2 failed";
  ASSERT_NE(ciphertext, ciphertext2);

  std::string decrypted_message2;
  ASSERT_TRUE(wallet.decrypt_buffer(ciphertext2, decrypted_message2)) << "decryption2 failed";

  ASSERT_EQ(message, decrypted_message2);

  // 1 bytes message
  message = "A";
  ASSERT_TRUE(wallet.encrypt_buffer(message, ciphertext)) << "encryption failed";
  ASSERT_TRUE(wallet.decrypt_buffer(ciphertext, decrypted_message)) << "decryption failed";
  ASSERT_EQ(message, decrypted_message);

  // 0 bytes message
  message = "";
  ASSERT_TRUE(wallet.encrypt_buffer(message, ciphertext)) << "encryption failed";
  ASSERT_TRUE(wallet.decrypt_buffer(ciphertext, decrypted_message)) << "decryption failed";
  ASSERT_EQ(message, decrypted_message);
}

// Same wallet + same plaintext must produce different ciphertexts
TEST(wallet, encrypt_buffer_nonce_is_fresh_per_call)
{
  currency::account_base acc;
  acc.generate();
  tools::wallet2 wallet;
  wallet.assign_account(acc);

  std::string message = get_random_text(64);

  std::string ct1, ct2;
  ASSERT_TRUE(wallet.encrypt_buffer(message, ct1));
  ASSERT_TRUE(wallet.encrypt_buffer(message, ct2));
  ASSERT_NE(ct1, ct2) << "two encryptions of the same plaintext under the same key produced identical ciphertext";

  std::string pt1, pt2;
  ASSERT_TRUE(wallet.decrypt_buffer(ct1, pt1));
  ASSERT_TRUE(wallet.decrypt_buffer(ct2, pt2));
  ASSERT_EQ(message, pt1);
  ASSERT_EQ(message, pt2);
}

// Ciphertext produced by one wallet must not decrypt under a different wallet's key
TEST(wallet, decrypt_buffer_rejects_other_wallets_ciphertext)
{
  currency::account_base acc1, acc2;
  acc1.generate();
  acc2.generate();

  tools::wallet2 wallet1, wallet2;
  wallet1.assign_account(acc1);
  wallet2.assign_account(acc2);

  std::string ct;
  ASSERT_TRUE(wallet1.encrypt_buffer(get_random_text(100), ct));

  std::string pt;
  ASSERT_FALSE(wallet2.decrypt_buffer(ct, pt)) << "ciphertext from wallet1 unexpectedly decrypted under wallet2's key";
}

// must reject any single-bit modification of the ciphertext blob.
TEST(wallet, decrypt_buffer_detects_tampering)
{
  currency::account_base acc;
  acc.generate();
  tools::wallet2 wallet;
  wallet.assign_account(acc);

  std::string message = get_random_text(128);
  std::string ct;
  ASSERT_TRUE(wallet.encrypt_buffer(message, ct));
  ASSERT_GE(ct.size(), 16u);

  std::string pt;

  // tail bit flip (inside encrypted_data — most likely inside the embedded MAC).
  std::string ct_tampered = ct;
  ct_tampered[ct_tampered.size() - 1] ^= 0x01;
  ASSERT_FALSE(wallet.decrypt_buffer(ct_tampered, pt)) << "tail tampering not detected";

  // middle bit flip (likely iv_nonce or encrypted_data body).
  ct_tampered = ct;
  ct_tampered[ct_tampered.size() / 2] ^= 0x80;
  ASSERT_FALSE(wallet.decrypt_buffer(ct_tampered, pt)) << "middle tampering not detected";

  // head bit flip (magic number byte 0). Should be rejected before MAC even
  // runs, by the explicit magic-number check.
  ct_tampered = ct;
  ct_tampered[0] ^= 0x01;
  ASSERT_FALSE(wallet.decrypt_buffer(ct_tampered, pt)) << "magic-number corruption not detected";
}

// Truncation, empty input, and arbitrary garbage must be rejected without crashing
TEST(wallet, decrypt_buffer_rejects_malformed_input)
{
  currency::account_base acc;
  acc.generate();
  tools::wallet2 wallet;
  wallet.assign_account(acc);

  std::string message = get_random_text(64);
  std::string ct;
  ASSERT_TRUE(wallet.encrypt_buffer(message, ct));
  ASSERT_GE(ct.size(), 16u);

  std::string pt;

  // truncate the last byte.
  ASSERT_FALSE(wallet.decrypt_buffer(ct.substr(0, ct.size() - 1), pt)) << "1-byte truncation accepted";

  // truncate enough to drop the embedded 16-byte MAC plus more — should hit
  // either deserialize failure or the explicit "too small data buffer" guard.
  ASSERT_FALSE(wallet.decrypt_buffer(ct.substr(0, ct.size() / 2), pt)) << "half-truncation accepted";

  // empty input.
  ASSERT_FALSE(wallet.decrypt_buffer(std::string(), pt)) << "empty ciphertext accepted";

  // arbitrary garbage of similar length.
  ASSERT_FALSE(wallet.decrypt_buffer(get_random_text(ct.size()), pt)) << "random garbage accepted as ciphertext";

  // garbage that happens to share the magic-number prefix should still fail at
  // either deserialization or the MAC step.
  std::string garbage_with_magic = ct.substr(0, 4) + get_random_text(ct.size() - 4);
  ASSERT_FALSE(wallet.decrypt_buffer(garbage_with_magic, pt)) << "magic-prefixed garbage accepted";
}

// Round-trip a payload large enough to exercise multi-block chacha20 and a
// non-trivial keccak input
TEST(wallet, encrypt_buffer_large_payload_round_trip)
{
  currency::account_base acc;
  acc.generate();
  tools::wallet2 wallet;
  wallet.assign_account(acc);

  std::string message = get_random_text(64 * 1024); // 64 KiB
  std::string ct;
  ASSERT_TRUE(wallet.encrypt_buffer(message, ct));
  ASSERT_GT(ct.size(), message.size()); // header + MAC must add bytes

  std::string pt;
  ASSERT_TRUE(wallet.decrypt_buffer(ct, pt));
  ASSERT_EQ(message, pt);
}

TEST(wallet, decrypt_buffer_legacy_round_trip_and_isolation_from_new_path)
{
  currency::account_base acc;
  acc.generate();
  tools::wallet2 wallet;
  wallet.assign_account(acc);

  std::string message = get_random_text(200);

  // produce legacy-format ciphertext directly using the same primitive the old
  // encrypt_buffer used.
  std::string legacy_ct = message;
  crypto::chacha_crypt_legacy(legacy_ct, acc.get_keys().view_secret_key);
  ASSERT_NE(legacy_ct, message); // sanity: encryption actually changed the bytes

  std::string pt;
  ASSERT_TRUE(wallet.decrypt_buffer_legacy(legacy_ct, pt));
  ASSERT_EQ(message, pt);

  // legacy ciphertext must not be silently accepted by the new decrypt path —
  // it has no magic/version framing.
  std::string pt_new;
  ASSERT_FALSE(wallet.decrypt_buffer(legacy_ct, pt_new)) << "legacy ciphertext unexpectedly accepted by new decrypt_buffer";
}
