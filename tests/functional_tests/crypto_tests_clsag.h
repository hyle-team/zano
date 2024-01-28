// Copyright (c) 2022-2024 Zano Project (https://zano.org/)
// Copyright (c) 2022-2024 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <crypto/clsag.h>

inline std::ostream& operator<<(std::ostream& ss, const CLSAG_GG_signature &sig)
{
  ss << "CLSAG_GG: c: " << sig.c << ENDL;
  ss << "          r: ";
  size_t i = 0;
  for(auto el: sig.r)
  {
    if (i++ != 0)
      ss << "             ";
    ss << el << ENDL;
  }
  ss << "         K1: " << sig.K1 << ENDL;
  return ss;
}

struct clsag_gg_sig_check_t
{
  crypto::hash prefix_hash;
  crypto::key_image ki;
  std::vector<public_key> stealth_addresses;
  std::vector<public_key> amount_commitments;  // div 8
  std::vector<CLSAG_GG_input_ref_t> ring;
  crypto::public_key pseudo_output_commitment; // div 8
  scalar_t secret_x;
  scalar_t secret_f;
  size_t secret_index;
  CLSAG_GG_signature sig;

  clsag_gg_sig_check_t()
  {}

  void rebuild_ring()
  {
    ring.clear();
    ring.reserve(stealth_addresses.size());
    for(size_t i = 0; i < stealth_addresses.size(); ++i)
      ring.emplace_back(stealth_addresses[i], amount_commitments[i]);
  }

  clsag_gg_sig_check_t& operator=(const clsag_gg_sig_check_t& rhs)
  {
    prefix_hash               = rhs.prefix_hash;
    ki                        = rhs.ki;
    stealth_addresses         = rhs.stealth_addresses;
    amount_commitments        = rhs.amount_commitments;
    rebuild_ring();
    pseudo_output_commitment  = rhs.pseudo_output_commitment;
    secret_x                  = rhs.secret_x;
    secret_f                  = rhs.secret_f;
    secret_index              = rhs.secret_index;
    return *this;
  }

  void prepare_random_data(size_t ring_size)
  {
    stealth_addresses.clear();
    amount_commitments.clear();
    ring.clear();
    
    crypto::generate_random_bytes(sizeof prefix_hash, &prefix_hash);

    stealth_addresses.reserve(ring_size);
    amount_commitments.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      stealth_addresses.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key());
      amount_commitments.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key()); // div 8
      ring.emplace_back(stealth_addresses.back(), amount_commitments.back());
    }

    secret_x = scalar_t::random();
    secret_f = scalar_t::random();
    secret_index = random_in_range(0, ring_size - 1);

    stealth_addresses[secret_index] = (secret_x * c_point_G).to_public_key();
    ki = (secret_x * hash_helper_t::hp(stealth_addresses[secret_index])).to_key_image();

    pseudo_output_commitment = (point_t(amount_commitments[secret_index]) - c_scalar_1div8 * secret_f * c_point_G).to_public_key();
  }

  bool generate()
  {
    try
    {
      return generate_CLSAG_GG(prefix_hash, ring, point_t(pseudo_output_commitment).modify_mul8(), ki, secret_x, secret_f, secret_index, sig);
    }
    catch(std::exception& e)
    {
      LOG_PRINT_RED(ENDL << "EXCEPTION: " << e.what(), LOG_LEVEL_0);
      return false;
    }
  }

  bool verify()
  {
    try
    {
      return verify_CLSAG_GG(prefix_hash, ring, pseudo_output_commitment, ki, sig);
    }
    catch(std::exception& e)
    {
      LOG_PRINT_RED(ENDL << "EXCEPTION: " << e.what(), LOG_LEVEL_0);
      return false;
    }
  }
};

TEST(clsag, basics)
{
  clsag_gg_sig_check_t cc;
  cc.prepare_random_data(1);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(2);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(8);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(123);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  return true;
}

TEST(clsag, bad_pub_keys)
{
  clsag_gg_sig_check_t cc, cc_orig;
  cc.prepare_random_data(10);

  cc_orig = cc;
  ASSERT_TRUE(cc.generate());
  cc.ki = parse_tpod_from_hex_string<key_image>(noncanonical_torsion_elements[0]);
  ASSERT_FALSE(cc.verify());
  cc.ki = parse_tpod_from_hex_string<key_image>(canonical_torsion_elements[0].string); // ki not in main subgroup
  ASSERT_FALSE(cc.verify());

  // check all torsion elements (paranoid mode on)
  for(size_t t = 0; t < sizeof canonical_torsion_elements / sizeof canonical_torsion_elements[0]; ++t)
  {
    point_t tor = point_t(parse_tpod_from_hex_string<public_key>(canonical_torsion_elements[t].string));
    ASSERT_FALSE(tor.is_in_main_subgroup());
    ASSERT_FALSE(tor.is_zero());
  
    // torsion component in ki must break the protocol
    cc = cc_orig;
    ASSERT_TRUE(cc.generate());
    cc.ki = (point_t(cc.ki) + tor).to_key_image(); // ki is not in main subgroup
    ASSERT_FALSE(cc.verify());

    // torsion component in amount_commitments[i] should not affect protocol
    cc = cc_orig;
    for(size_t i = 0; i < cc.ring.size(); ++i)
      cc.amount_commitments[i] = (point_t(cc.amount_commitments[i]) + tor).to_public_key();
    ASSERT_TRUE(cc.generate());
    ASSERT_TRUE(cc.verify());

    // torsion component in K1 should break the protocol
    cc = cc_orig;
    ASSERT_TRUE(cc.generate());
    cc.sig.K1 = (point_t(cc.sig.K1) + tor).to_public_key();
    ASSERT_FALSE(cc.verify());

    // torsion component in stealth_address for secret_index (i.e. for P = xG) must break the protocol
    // 1
    cc = cc_orig;
    cc.stealth_addresses[cc.secret_index] = (point_t(cc.stealth_addresses[cc.secret_index]) + tor).to_public_key();
    ASSERT_FALSE(cc.generate());
    // 2
    cc = cc_orig;
    ASSERT_TRUE(cc.generate());
    cc.stealth_addresses[cc.secret_index] = (point_t(cc.stealth_addresses[cc.secret_index]) + tor).to_public_key();
    ASSERT_FALSE(cc.verify());

    // torsion component in pseudo_output_commitment must break the protocol
    cc = cc_orig;
    cc.pseudo_output_commitment = (point_t(cc.pseudo_output_commitment) + tor).to_public_key();
    ASSERT_TRUE(cc.generate());
    ASSERT_FALSE(cc.verify());
  }

  return true;
}

TEST(clsag, bad_sig)
{
  clsag_gg_sig_check_t cc;

  // wrong prefix hash
  cc.prepare_random_data(10);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());
  cc.prefix_hash.data[5] ^= 0x7f;
  ASSERT_FALSE(cc.verify());

  // ring size > sig.r size
  ASSERT_TRUE(cc.generate());
  cc.sig.r.clear();
  ASSERT_FALSE(cc.verify());

  // ring size < sig.r size
  ASSERT_TRUE(cc.generate());
  cc.sig.r.push_back(scalar_t::random());
  ASSERT_FALSE(cc.verify());

  return true;
}

TEST(clsag, sig_difference)
{
  int cmp_res = 0;

  clsag_gg_sig_check_t cc0, cc1;
  cc0.prepare_random_data(8);
  cc1 = cc0; // get the same input data

  {
    // make sure two signatures generated with the same input and randoms are equal 
    random_state_test_restorer rstr; // to restore random state on exit of the scope
    random_state_test_restorer::reset_random(0);
    ASSERT_TRUE(cc0.generate());
    random_state_test_restorer::reset_random(0);
    ASSERT_TRUE(cc1.generate());
    LOG_PRINT_L0("cc0: " << ENDL << cc0.sig << ", cc1: " << ENDL << cc1.sig);
    ASSERT_TRUE(cc0.sig == cc1.sig);
    ASSERT_TRUE(cc0.verify());
  }

  // make sure two signatures generated with the same input are not equal
  ASSERT_TRUE(cc0.generate());
  ASSERT_TRUE(cc1.generate());
  ASSERT_TRUE(cc0.sig != cc1.sig);

  return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
// CLSAG GGX
//

struct clsag_ggx_sig_check_t
{
  crypto::hash prefix_hash;
  crypto::key_image ki;
  std::vector<public_key> stealth_addresses;
  std::vector<public_key> amount_commitments;     // div 8
  std::vector<public_key> blinded_asset_ids;       // div 8
  std::vector<CLSAG_GGX_input_ref_t> ring;
  crypto::public_key pseudo_output_commitment;    // div 8
  crypto::public_key pseudo_out_asset_id;         // div 8
  scalar_t secret_0_xp; // xp * G = P
  scalar_t secret_1_f;  // = f - f' = amount_blinding_mask - pseudo_commitment_blinding_mask
  scalar_t secret_2_t;  // = -r' = -pseudo_asset_id_blinding_mask
  size_t secret_index;
  CLSAG_GGX_signature_serialized sig;

  clsag_ggx_sig_check_t()
  {}

  void rebuild_ring()
  {
    ring.clear();
    ring.reserve(stealth_addresses.size());
    for(size_t i = 0; i < stealth_addresses.size(); ++i)
      ring.emplace_back(stealth_addresses[i], amount_commitments[i], blinded_asset_ids[i]);
  }

  clsag_ggx_sig_check_t& operator=(const clsag_ggx_sig_check_t& rhs)
  {
    prefix_hash                 = rhs.prefix_hash;
    ki                          = rhs.ki;
    stealth_addresses           = rhs.stealth_addresses;
    amount_commitments          = rhs.amount_commitments;
    blinded_asset_ids           = rhs.blinded_asset_ids;
    rebuild_ring();
    pseudo_output_commitment    = rhs.pseudo_output_commitment;
    pseudo_out_asset_id         = rhs.pseudo_out_asset_id;
    secret_0_xp                 = rhs.secret_0_xp;
    secret_1_f                  = rhs.secret_1_f;
    secret_2_t                  = rhs.secret_2_t;
    secret_index                = rhs.secret_index;
    return *this;
  }

  void prepare_random_data(size_t ring_size)
  {
    stealth_addresses.clear();
    amount_commitments.clear();
    blinded_asset_ids.clear();
    
    crypto::generate_random_bytes(sizeof prefix_hash, &prefix_hash);

    stealth_addresses.reserve(ring_size);
    amount_commitments.reserve(ring_size);
    blinded_asset_ids.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      stealth_addresses.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key());
      amount_commitments.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key()); // div 8
      blinded_asset_ids.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key()); // div 8
    }

    secret_0_xp = scalar_t::random();
    secret_1_f  = scalar_t::random();
    secret_2_t  = scalar_t::random();
    secret_index = random_in_range(0, ring_size - 1);

    stealth_addresses[secret_index] = (secret_0_xp * c_point_G).to_public_key();
    
    ki = (secret_0_xp * hash_helper_t::hp(stealth_addresses[secret_index])).to_key_image();

    pseudo_output_commitment = (point_t(amount_commitments[secret_index]) - c_scalar_1div8 * secret_1_f * c_point_G).to_public_key();
    pseudo_out_asset_id      = (point_t(blinded_asset_ids[secret_index])  - c_scalar_1div8 * secret_2_t * c_point_X).to_public_key();

    rebuild_ring();
  }

  bool generate()
  {
    try
    {
      return generate_CLSAG_GGX(prefix_hash, ring, point_t(pseudo_output_commitment).modify_mul8(), point_t(pseudo_out_asset_id).modify_mul8(), ki,
        secret_0_xp, secret_1_f, secret_2_t, secret_index, sig);
    }
    catch(std::exception& e)
    {
      LOG_PRINT_RED(ENDL << "EXCEPTION: " << e.what(), LOG_LEVEL_0);
      return false;
    }
  }

  bool verify()
  {
    try
    {
      return verify_CLSAG_GGX(prefix_hash, ring, pseudo_output_commitment, pseudo_out_asset_id, ki, sig);
    }
    catch(std::exception& e)
    {
      LOG_PRINT_RED(ENDL << "EXCEPTION: " << e.what(), LOG_LEVEL_0);
      return false;
    }
  }
};


TEST(clsag_ggx, basics)
{
  std::string X_hash_str("X_generator");
  point_t X = hash_helper_t::hp(X_hash_str.c_str(), X_hash_str.size());
  LOG_PRINT_L0("X = " << X.to_hex_comma_separated_uint64_str());
  ASSERT_EQ(X, c_point_X);

  clsag_ggx_sig_check_t cc;

  size_t ring_sizes[] = { 1, 2, 8, 123 };
  for(size_t i = 0; i < sizeof ring_sizes / sizeof ring_sizes[0]; ++i)
  {
    std::cout << "ring size: " << ring_sizes[i] << std::endl;

    cc.prepare_random_data(ring_sizes[i]);
    ASSERT_TRUE(cc.generate());
    ASSERT_TRUE(cc.verify());

    // binary serialization 
    std::string blob = t_serializable_object_to_blob(cc.sig);
    generate_random_bytes(sizeof cc.sig, &cc.sig);
    new (&cc.sig.r_g) scalar_vec_t{scalar_t::random()};
    new (&cc.sig.r_x) scalar_vec_t{scalar_t::random(), scalar_t::random()};
    ASSERT_TRUE(t_unserializable_object_from_blob(cc.sig, blob));
    ASSERT_TRUE(cc.verify());

    // boost serialization
    ASSERT_TRUE(tools::serialize_obj_to_buff(cc.sig, blob));
    generate_random_bytes(sizeof cc.sig, &cc.sig);
    new (&cc.sig.r_g) scalar_vec_t{scalar_t::random(), scalar_t::random()};
    new (&cc.sig.r_x) scalar_vec_t{scalar_t::random(), scalar_t::random()};
    ASSERT_TRUE(tools::unserialize_obj_from_buff(cc.sig, blob));
    ASSERT_TRUE(cc.verify());
  }

  return true;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
// CLSAG GGXG (eventually not used in Zano)
//
/*
struct clsag_ggxg_sig_check_t
{
  crypto::hash prefix_hash;
  crypto::key_image ki;
  std::vector<public_key> stealth_addresses;
  std::vector<public_key> amount_commitments;     // div 8
  std::vector<public_key> concealing_points;      // div 8
  std::vector<CLSAG_GGXG_input_ref_t> ring;
  crypto::public_key pseudo_output_commitment;    // div 8
  crypto::public_key extended_amount_commitment;  // div 8
  scalar_t secret_xp;
  scalar_t secret_f; // = f - f' = amount_blinding_mask - pseudo_commitment_blinding_mask
  scalar_t secret_x;
  scalar_t secret_q;
  size_t secret_index;
  CLSAG_GGXG_signature sig;

  clsag_ggxg_sig_check_t()
  {}

  void rebuild_ring()
  {
    ring.clear();
    ring.reserve(stealth_addresses.size());
    for(size_t i = 0; i < stealth_addresses.size(); ++i)
      ring.emplace_back(stealth_addresses[i], amount_commitments[i], concealing_points[i]);
  }

  clsag_ggxg_sig_check_t& operator=(const clsag_ggxg_sig_check_t& rhs)
  {
    prefix_hash                 = rhs.prefix_hash;
    ki                          = rhs.ki;
    stealth_addresses           = rhs.stealth_addresses;
    amount_commitments          = rhs.amount_commitments;
    concealing_points           = rhs.concealing_points;
    rebuild_ring();
    pseudo_output_commitment    = rhs.pseudo_output_commitment;
    extended_amount_commitment  = rhs.extended_amount_commitment;
    secret_xp                   = rhs.secret_xp;
    secret_f                    = rhs.secret_f;
    secret_x                    = rhs.secret_x;
    secret_q                    = rhs.secret_q;
    secret_index                = rhs.secret_index;
    return *this;
  }

  void prepare_random_data(size_t ring_size)
  {
    stealth_addresses.clear();
    amount_commitments.clear();
    concealing_points.clear();
    ring.clear();

    crypto::generate_random_bytes(sizeof prefix_hash, &prefix_hash);

    stealth_addresses.reserve(ring_size);
    amount_commitments.reserve(ring_size);
    concealing_points.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      stealth_addresses.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key());
      amount_commitments.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key()); // div 8
      concealing_points.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key()); // div 8
      ring.emplace_back(stealth_addresses.back(), amount_commitments.back(), concealing_points.back());
    }

    secret_xp = scalar_t::random();
    secret_f = scalar_t::random();
    secret_x = scalar_t::random();
    secret_q = scalar_t::random();
    secret_index = random_in_range(0, ring_size - 1);

    stealth_addresses[secret_index] = (secret_xp * c_point_G).to_public_key();
    concealing_points[secret_index] = (c_scalar_1div8 * secret_q * c_point_G).to_public_key();
    ki = (secret_xp * hash_helper_t::hp(stealth_addresses[secret_index])).to_key_image();

    pseudo_output_commitment = (point_t(amount_commitments[secret_index]) - c_scalar_1div8 * secret_f * c_point_G).to_public_key();
    extended_amount_commitment = (c_scalar_1div8 * secret_x * c_point_X + point_t(amount_commitments[secret_index]) + point_t(concealing_points[secret_index])).to_public_key();
  }

  bool generate()
  {
    try
    {
      return generate_CLSAG_GGXG(prefix_hash, ring, point_t(pseudo_output_commitment).modify_mul8(), point_t(extended_amount_commitment).modify_mul8(), ki,
        secret_xp, secret_f, secret_x, secret_q, secret_index, sig);
    }
    catch(std::exception& e)
    {
      LOG_PRINT_RED(ENDL << "EXCEPTION: " << e.what(), LOG_LEVEL_0);
      return false;
    }
  }

  bool verify()
  {
    try
    {
      return verify_CLSAG_GGXG(prefix_hash, ring, pseudo_output_commitment, extended_amount_commitment, ki, sig);
    }
    catch(std::exception& e)
    {
      LOG_PRINT_RED(ENDL << "EXCEPTION: " << e.what(), LOG_LEVEL_0);
      return false;
    }
  }
};


TEST(clsag_ggxg, basics)
{
  std::string X_hash_str("X_generator");
  point_t X = hash_helper_t::hp(X_hash_str.c_str(), X_hash_str.size());
  LOG_PRINT_L0("X = " << X.to_hex_comma_separated_uint64_str());
  ASSERT_EQ(X, c_point_X);

  clsag_ggxg_sig_check_t cc;

  cc.prepare_random_data(1);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(2);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(8);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(123);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  return true;
}*/

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// CLSAG GGXXG
//

struct clsag_ggxxg_sig_check_t
{
  crypto::hash prefix_hash;
  crypto::key_image ki;
  std::vector<public_key> stealth_addresses;
  std::vector<public_key> amount_commitments;     // div 8
  std::vector<public_key> blinded_asset_ids;      // div 8
  std::vector<public_key> concealing_points;      // div 8
  std::vector<CLSAG_GGXXG_input_ref_t> ring;
  crypto::public_key pseudo_output_commitment;    // div 8
  crypto::public_key pseudo_out_blinded_asset_id; // div 8
  crypto::public_key extended_amount_commitment;  // div 8
  scalar_t secret_xp;
  scalar_t secret_f; // = f - f' = amount_blinding_mask - pseudo_commitment_blinding_mask
  scalar_t secret_t;
  scalar_t secret_x;
  scalar_t secret_q;
  size_t secret_index;
  CLSAG_GGXXG_signature sig;

  clsag_ggxxg_sig_check_t()
  {}

  void rebuild_ring()
  {
    ring.clear();
    ring.reserve(stealth_addresses.size());
    for(size_t i = 0; i < stealth_addresses.size(); ++i)
      ring.emplace_back(stealth_addresses[i], amount_commitments[i], blinded_asset_ids[i], concealing_points[i]);
  }

  clsag_ggxxg_sig_check_t& operator=(const clsag_ggxxg_sig_check_t& rhs)
  {
    prefix_hash                 = rhs.prefix_hash;
    ki                          = rhs.ki;
    stealth_addresses           = rhs.stealth_addresses;
    amount_commitments          = rhs.amount_commitments;
    blinded_asset_ids           = rhs.blinded_asset_ids;
    concealing_points           = rhs.concealing_points;
    rebuild_ring();
    pseudo_output_commitment    = rhs.pseudo_output_commitment;
    pseudo_out_blinded_asset_id = rhs.pseudo_out_blinded_asset_id;
    extended_amount_commitment  = rhs.extended_amount_commitment;
    secret_xp                   = rhs.secret_xp;
    secret_f                    = rhs.secret_f;
    secret_t                    = rhs.secret_t;
    secret_x                    = rhs.secret_x;
    secret_q                    = rhs.secret_q;
    secret_index                = rhs.secret_index;
    return *this;
  }

  void prepare_random_data(size_t ring_size)
  {
    stealth_addresses.clear();
    amount_commitments.clear();
    blinded_asset_ids.clear();
    concealing_points.clear();
    ring.clear();

    crypto::generate_random_bytes(sizeof prefix_hash, &prefix_hash);

    stealth_addresses.reserve(ring_size);
    amount_commitments.reserve(ring_size);
    blinded_asset_ids.reserve(ring_size);
    concealing_points.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      stealth_addresses.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key());
      amount_commitments.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key()); // div 8
      blinded_asset_ids.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key());  // div 8
      concealing_points.push_back(hash_helper_t::hp(scalar_t::random()).to_public_key());  // div 8
      ring.emplace_back(stealth_addresses.back(), amount_commitments.back(), blinded_asset_ids.back(), concealing_points.back());
    }

    secret_xp = scalar_t::random();
    secret_f = scalar_t::random();
    secret_t = scalar_t::random();
    secret_x = scalar_t::random();
    secret_q = scalar_t::random();
    secret_index = random_in_range(0, ring_size - 1);

    stealth_addresses[secret_index] = (secret_xp * c_point_G).to_public_key();
    concealing_points[secret_index] = (c_scalar_1div8 * secret_q * c_point_G).to_public_key();
    ki = (secret_xp * hash_helper_t::hp(stealth_addresses[secret_index])).to_key_image();

    pseudo_output_commitment = (point_t(amount_commitments[secret_index]) - c_scalar_1div8 * secret_f * c_point_G).to_public_key();
    pseudo_out_blinded_asset_id = (point_t(blinded_asset_ids[secret_index])  - c_scalar_1div8 * secret_t * c_point_X).to_public_key();
    extended_amount_commitment = (c_scalar_1div8 * secret_x * c_point_X + point_t(amount_commitments[secret_index]) + point_t(concealing_points[secret_index])).to_public_key();
  }

  bool generate()
  {
    try
    {
      return generate_CLSAG_GGXXG(prefix_hash, ring, point_t(pseudo_output_commitment).modify_mul8(), point_t(pseudo_out_blinded_asset_id).modify_mul8(), point_t(extended_amount_commitment).modify_mul8(), ki,
        secret_xp, secret_f, secret_t, secret_x, secret_q, secret_index, sig);
    }
    catch(std::exception& e)
    {
      LOG_PRINT_RED(ENDL << "EXCEPTION: " << e.what(), LOG_LEVEL_0);
      return false;
    }
  }

  bool verify()
  {
    try
    {
      return verify_CLSAG_GGXXG(prefix_hash, ring, pseudo_output_commitment, pseudo_out_blinded_asset_id, extended_amount_commitment, ki, sig);
    }
    catch(std::exception& e)
    {
      LOG_PRINT_RED(ENDL << "EXCEPTION: " << e.what(), LOG_LEVEL_0);
      return false;
    }
  }
};


TEST(clsag_ggxxg, basics)
{
  std::string X_hash_str("X_generator");
  point_t X = hash_helper_t::hp(X_hash_str.c_str(), X_hash_str.size());
  LOG_PRINT_L0("X = " << X.to_hex_comma_separated_uint64_str());
  ASSERT_EQ(X, c_point_X);

  clsag_ggxxg_sig_check_t cc;

  cc.prepare_random_data(1);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(2);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(8);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  cc.prepare_random_data(123);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());

  return true;
}
