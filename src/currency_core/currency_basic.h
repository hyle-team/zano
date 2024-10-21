// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <type_traits>
#include <boost/variant.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/unique.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/vector/vector30.hpp>
#include <boost/type_traits/is_same.hpp>

#include <vector>
#include <cstring>  // memcmp
#include <sstream>
#include <chrono>

#include "include_base_utils.h"

#include "serialization/binary_archive.h"
#include "common/crypto_serialization.h"
#include "serialization/stl_containers.h"
#include "serialization/serialization.h"
#include "serialization/variant.h"
#include "serialization/boost_types.h"
#include "serialization/json_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/keyvalue_serialization.h" // epee key-value serialization
#include "string_tools.h"
#include "currency_config.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto/range_proofs.h"
#include "crypto/zarcanum.h"
#include "crypto/eth_signature.h"
#include "misc_language.h"
#include "block_flags.h"
#include "etc_custom_serialization.h"
#include "difficulty.h"

namespace currency
{

  const static crypto::hash null_hash = AUTO_VAL_INIT(null_hash);
  const static crypto::public_key null_pkey = AUTO_VAL_INIT(null_pkey);
  const static crypto::key_image null_ki = AUTO_VAL_INIT(null_ki);
  const static crypto::secret_key null_skey = AUTO_VAL_INIT(null_skey);
  const static crypto::signature null_sig = AUTO_VAL_INIT(null_sig);
  const static crypto::key_derivation null_derivation = AUTO_VAL_INIT(null_derivation);

  const static crypto::hash gdefault_genesis = epee::string_tools::hex_to_pod<crypto::hash>("CC608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852");

  // Using C++17 extended aggregate initialization (P0017R1). C++17, finally! -- sowle
  const static crypto::public_key native_coin_asset_id       = {{'\xd6', '\x32', '\x9b', '\x5b', '\x1f', '\x7c', '\x08', '\x05', '\xb5', '\xc3', '\x45', '\xf4', '\x95', '\x75', '\x54', '\x00', '\x2a', '\x2f', '\x55', '\x78', '\x45', '\xf6', '\x4d', '\x76', '\x45', '\xda', '\xe0', '\xe0', '\x51', '\xa6', '\x49', '\x8a'}}; // == crypto::c_point_H, checked in crypto_constants
  const static crypto::public_key native_coin_asset_id_1div8 = {{'\x74', '\xc3', '\x2d', '\x3e', '\xaa', '\xfa', '\xfc', '\x62', '\x3b', '\xf4', '\x83', '\xe8', '\x58', '\xd4', '\x2e', '\x8b', '\xf4', '\xec', '\x7d', '\xf0', '\x64', '\xad', '\xa2', '\xe3', '\x49', '\x34', '\x46', '\x9c', '\xff', '\x6b', '\x62', '\x68'}}; // == 1/8 * crypto::c_point_H, checked in crypto_constants
  const static crypto::point_t    native_coin_asset_id_pt      {{ 20574939, 16670001, -29137604, 14614582, 24883426, 3503293, 2667523, 420631, 2267646, -4769165, -11764015, -12206428, -14187565, -2328122, -16242653, -788308, -12595746, -8251557, -10110987, 853396, -4982135, 6035602, -21214320, 16156349, 977218, 2807645, 31002271, 5694305, -16054128, 5644146, -15047429, -568775, -22568195, -8089957, -27721961, -10101877, -29459620, -13359100, -31515170, -6994674 }}; // c_point_H

  const static wide_difficulty_type global_difficulty_pow_starter = DIFFICULTY_POW_STARTER;
  const static wide_difficulty_type global_difficulty_pos_starter = DIFFICULTY_POS_STARTER;
  const static uint64_t             global_difficulty_pos_target  = DIFFICULTY_POS_TARGET;
  const static uint64_t             global_difficulty_pow_target  = DIFFICULTY_POW_TARGET;

  typedef std::string payment_id_t;


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  struct asset_descriptor_operation_v1;

//since structure used in blockchain as a key accessor, then be sure that there is no padding inside
#pragma pack(push, 1)
  struct account_public_address_old
  {
    crypto::public_key spend_public_key;
    crypto::public_key view_public_key;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(spend_public_key)
      FIELD(view_public_key)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(spend_public_key, "m_spend_public_key")
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(view_public_key, "m_view_public_key")
    END_KV_SERIALIZE_MAP()
  };
#pragma pack(pop)


#define ACCOUNT_PUBLIC_ADDRESS_SERIZALIZATION_VER 1

#define ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE 0x01 // auditable address

//since structure used in blockchain as a key accessor, then be sure that there is no padding inside
#pragma pack(push, 1)
  struct account_public_address
  {
    crypto::public_key spend_public_key;
    crypto::public_key view_public_key;
    uint8_t flags;

    DEFINE_SERIALIZATION_VERSION(ACCOUNT_PUBLIC_ADDRESS_SERIZALIZATION_VER)
    BEGIN_SERIALIZE_OBJECT()
      FIELD(spend_public_key)
      FIELD(view_public_key)
      FIELD(flags)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(spend_public_key, "m_spend_public_key")
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(view_public_key, "m_view_public_key")
      KV_SERIALIZE(flags)
    END_KV_SERIALIZE_MAP()

    bool is_auditable() const
    {
      return (flags & ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE) != 0;
    }

    static account_public_address from_old(const account_public_address_old& rhs)
    {
      account_public_address result = AUTO_VAL_INIT(result);
      result.spend_public_key = rhs.spend_public_key;
      result.view_public_key = rhs.view_public_key;
      return result;
    }

    account_public_address_old to_old() const
    {
      account_public_address_old result = AUTO_VAL_INIT(result);
      result.spend_public_key = spend_public_key;
      result.view_public_key = view_public_key;
      return result;
    }
  };
#pragma pack(pop)


  const static account_public_address null_pub_addr = AUTO_VAL_INIT(null_pub_addr);

  typedef std::vector<crypto::signature> ring_signature;

  /************************************************************************/
  /* extra structures                                                     */
  /************************************************************************/
  struct extra_attachment_info
  {
    uint64_t sz;
    crypto::hash hsh;
    uint64_t cnt;

    BEGIN_SERIALIZE()
      VARINT_FIELD(sz)
      FIELD(hsh)
      VARINT_FIELD(cnt)
    END_SERIALIZE()
  };

  /* outputs */

// 'mix_attr' possible values
#define CURRENCY_TO_KEY_OUT_RELAXED                   0 // the output may be mixed with any fake outputs
#define CURRENCY_TO_KEY_OUT_FORCED_NO_MIX             1 // the output can't be mixed, only direct spend allowed
#define CURRENCY_TO_KEY_OUT_FORCED_MIX_LOWER_BOUND    2 // this and greather values means minimum number of total outputs (fakes + 1) must be mixed together for using that one

  #pragma pack(push, 1)
  struct txout_to_key
  {
    txout_to_key() : key(null_pkey), mix_attr(0) { }
    txout_to_key(const crypto::public_key &_key) : key(_key), mix_attr(0) { }

    crypto::public_key key;
    uint8_t mix_attr;
  };
  #pragma pack(pop)

  /* inputs */

  struct txin_gen
  {
    size_t height;

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(height)
    END_SERIALIZE()
  };

  // ref_by_id is used by txin_to_key to reference an output by source transaction hash and output's internal index,
  // rather than amount and global output index (by default). Useful when output global index is unknown or may change.
  struct ref_by_id
  {
    crypto::hash tx_id; // source transaction hash
    uint32_t n;         // output index in source transaction

    BEGIN_SERIALIZE_OBJECT()
      FIELD(tx_id)
      VARINT_FIELD(n)
    END_SERIALIZE()
  };

  typedef boost::variant<uint64_t, ref_by_id> txout_ref_v;
  

  struct signed_parts
  {
    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(n_outs)
      VARINT_FIELD(n_extras)
    END_SERIALIZE()

    uint32_t n_outs;
    uint32_t n_extras;
  };


  typedef boost::variant<signed_parts, extra_attachment_info> txin_etc_details_v;


  struct referring_input
  {
    std::vector<txout_ref_v> key_offsets;
  };

  struct txin_to_key : public referring_input
  {
    uint64_t amount;
    crypto::key_image k_image;                    // double spending protection
    std::vector<txin_etc_details_v> etc_details;  // see also TX_FLAG_SIGNATURE_MODE_SEPARATE

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(key_offsets) // from referring_input
      FIELD(k_image)
      FIELD(etc_details)
    END_SERIALIZE()
  };

  struct txin_htlc: public txin_to_key
  {
    std::string hltc_origin;
    BEGIN_SERIALIZE_OBJECT()
      FIELD(hltc_origin)
      FIELDS(*static_cast<txin_to_key*>(this))
    END_SERIALIZE()
  };

  struct txin_multisig
  {
    uint64_t amount;
    crypto::hash multisig_out_id;
    uint32_t sigs_count; // actual number of signatures that are used to sign this input; needed to calculate tx blob size; must be equal to minimum_sigs of corresponding ms output
    std::vector<txin_etc_details_v> etc_details; 

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(multisig_out_id)
      VARINT_FIELD(sigs_count)
      FIELD(etc_details)
    END_SERIALIZE()
  };


  struct txout_multisig
  {
    uint32_t minimum_sigs;
    std::vector<crypto::public_key> keys;
    
    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(minimum_sigs)
      FIELD(keys)
    END_SERIALIZE()
  };

#define CURRENCY_TXOUT_HTLC_FLAGS_HASH_TYPE_MASK   0x01 // 0 - SHA256, 1 - RIPEMD160

  struct txout_htlc
  {
    crypto::hash htlc_hash;
    uint8_t flags;      //select type of the hash, may be some extra info in future
    uint64_t expiration; 
    crypto::public_key pkey_redeem; //works before expiration
    crypto::public_key pkey_refund; //works after expiration

    BEGIN_SERIALIZE_OBJECT()
      FIELD(htlc_hash)
      FIELD(flags)
      VARINT_FIELD(expiration)
      FIELD(pkey_redeem)
      FIELD(pkey_refund)
    END_SERIALIZE()
  };

  typedef boost::variant<txout_to_key, txout_multisig, txout_htlc> txout_target_v;

  //typedef std::pair<uint64_t, txout> out_t;
  struct tx_out_bare
  {
    uint64_t amount;
    txout_target_v target;

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(target)
    END_SERIALIZE()
  };


  /////////////////////////////////////////////////////////////////////////////
  // Zarcanum structures
  //

  struct txin_zc_input : public referring_input
  {
    txin_zc_input() {}
    // Boost's Assignable concept
    txin_zc_input(const txin_zc_input&)           = default;
    txin_zc_input& operator=(const txin_zc_input&)= default;

    crypto::key_image               k_image;
    std::vector<txin_etc_details_v> etc_details;


    BEGIN_SERIALIZE_OBJECT()
      FIELD(key_offsets) // referring_input
      FIELD(k_image)
      FIELD(etc_details)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(key_offsets) // referring_input
      BOOST_SERIALIZE(k_image)
      BOOST_SERIALIZE(etc_details)
    END_BOOST_SERIALIZATION()
  };

  struct tx_out_zarcanum
  {
    tx_out_zarcanum() {}
    
    // Boost's Assignable concept
    tx_out_zarcanum(const tx_out_zarcanum&)            = default;
    tx_out_zarcanum& operator=(const tx_out_zarcanum&) = default;

    crypto::public_key  stealth_address;
    crypto::public_key  concealing_point;  // group element Q, see also Zarcanum paper, premultiplied by 1/8
    crypto::public_key  amount_commitment; // premultiplied by 1/8
    crypto::public_key  blinded_asset_id;  // group element T, premultiplied by 1/8
    uint64_t            encrypted_amount = 0;
    uint8_t             mix_attr = 0;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(stealth_address)
      FIELD(concealing_point)
      FIELD(amount_commitment)
      FIELD(blinded_asset_id)
      FIELD(encrypted_amount)
      FIELD(mix_attr)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(stealth_address)
      BOOST_SERIALIZE(concealing_point)
      BOOST_SERIALIZE(amount_commitment)
      BOOST_SERIALIZE(blinded_asset_id)
      BOOST_SERIALIZE(encrypted_amount)
      BOOST_SERIALIZE(mix_attr)
    END_BOOST_SERIALIZATION()
  };

  struct zarcanum_tx_data_v1
  {
    uint64_t fee;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(fee)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(fee)
    END_BOOST_SERIALIZATION()
  };

  struct zc_asset_surjection_proof
  {
    std::vector<crypto::BGE_proof_s> bge_proofs; // one per output, non-aggregated version of Groth-Bootle-Esgin yet, need to be upgraded later -- sowle

    BEGIN_SERIALIZE_OBJECT()
      FIELD(bge_proofs)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(bge_proofs)
    END_BOOST_SERIALIZATION()
  };

  // non-consoditated txs must have one of this objects in the attachments (elements count == vout.size())
  // consolidated -- one pre consolidated part (sum(elements count) == vout.size())
  struct zc_outs_range_proof
  {
    crypto::bpp_signature_serialized bpp; // for commitments in form: amount * U + mask * G
    crypto::vector_UG_aggregation_proof_serialized aggregation_proof; // E'_j = e_j * U + y'_j * G    +   vector Shnorr

    BEGIN_SERIALIZE_OBJECT()
      FIELD(bpp)
      FIELD(aggregation_proof)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(bpp)
      BOOST_SERIALIZE(aggregation_proof)
    END_BOOST_SERIALIZATION()
  };

  // Zarcanum-aware CLSAG signature (one per ZC input)
  struct ZC_sig
  {
    crypto::public_key pseudo_out_amount_commitment = null_pkey; // premultiplied by 1/8
    crypto::public_key pseudo_out_blinded_asset_id  = null_pkey; // premultiplied by 1/8
    crypto::CLSAG_GGX_signature_serialized clsags_ggx;
    
    BEGIN_SERIALIZE_OBJECT()
      FIELD(pseudo_out_amount_commitment)
      FIELD(pseudo_out_blinded_asset_id)
      FIELD(clsags_ggx)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(pseudo_out_amount_commitment)
      BOOST_SERIALIZE(pseudo_out_blinded_asset_id)
      BOOST_SERIALIZE(clsags_ggx)
    END_BOOST_SERIALIZATION()
  };

  // First part of a double Schnorr proof:
  //   1) for txs without ZC inputs: proves that balance point = lin(G) (cancels out G component of outputs' amount commitments, asset tags assumed to be H (native coin) and non-blinded)
  //   2) for txs with    ZC inputs: proves that balance point = lin(X) (cancels out X component of blinded asset tags within amount commitments for both outputs and inputs (pseudo outs))
  // Second part:
  //   proof of knowing transaction secret key (with respect to G)
  struct zc_balance_proof
  {
    crypto::generic_double_schnorr_sig_s dss;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(dss)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(dss)
    END_BOOST_SERIALIZATION()
  };


  struct zarcanum_sig : public crypto::zarcanum_proof
  {
    BEGIN_SERIALIZE_OBJECT()
      FIELD(d)
      FIELD(C)
      FIELD(C_prime);
      FIELD(E);
      FIELD(c);
      FIELD(y0);
      FIELD(y1);
      FIELD(y2);
      FIELD(y3);
      FIELD(y4);
      FIELD_N("E_range_proof", (crypto::bppe_signature_serialized&)E_range_proof);
      FIELD(pseudo_out_amount_commitment);
      FIELD_N("clsag_ggxxg", (crypto::CLSAG_GGXXG_signature_serialized&)clsag_ggxxg);
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(d)
      BOOST_SERIALIZE(C)
      BOOST_SERIALIZE(C_prime);
      BOOST_SERIALIZE(E);
      BOOST_SERIALIZE(c);
      BOOST_SERIALIZE(y0);
      BOOST_SERIALIZE(y1);
      BOOST_SERIALIZE(y2);
      BOOST_SERIALIZE(y3);
      BOOST_SERIALIZE(y4);
      BOOST_SERIALIZE((crypto::bppe_signature_serialized&)E_range_proof);
      BOOST_SERIALIZE(pseudo_out_amount_commitment);
      BOOST_SERIALIZE((crypto::CLSAG_GGXXG_signature_serialized&)clsag_ggxxg);
    END_BOOST_SERIALIZATION()
  };

//#pragma pack(pop)

  typedef boost::variant<txin_gen, txin_to_key, txin_multisig, txin_htlc, txin_zc_input> txin_v;

  typedef boost::variant<tx_out_bare, tx_out_zarcanum> tx_out_v;


  struct tx_comment
  {
    std::string comment;

    BEGIN_SERIALIZE()
      FIELD(comment)
    END_SERIALIZE()
  };

  struct tx_payer_old
  {
    account_public_address_old acc_addr;

    BEGIN_SERIALIZE()
      FIELD(acc_addr)
    END_SERIALIZE()
  };

  struct tx_payer
  {
    tx_payer() = default;
    tx_payer(const tx_payer_old& old) : acc_addr(account_public_address::from_old(old.acc_addr)) {}

    account_public_address acc_addr{};

    BEGIN_SERIALIZE()
      FIELD(acc_addr)
    END_SERIALIZE()
  };

  struct tx_receiver_old
  {
    account_public_address_old acc_addr;

    BEGIN_SERIALIZE()
      FIELD(acc_addr)
    END_SERIALIZE()
  };

  struct tx_receiver
  {
    tx_receiver() = default;
    tx_receiver(const tx_receiver_old& old) : acc_addr(account_public_address::from_old(old.acc_addr)) {}

    account_public_address acc_addr{};

    BEGIN_SERIALIZE()
      FIELD(acc_addr)
    END_SERIALIZE()
  };

  struct tx_crypto_checksum
  {
    //we put tx encrypted key_derivation into tx attachment/extra, to let sender decrypt attachments that had been encrypted. 
    // key_derivation encrypted on sender private send address
    crypto::key_derivation encrypted_key_derivation;
    uint32_t derivation_hash;

    BEGIN_SERIALIZE()
      FIELD(encrypted_key_derivation)
      FIELD(derivation_hash)
    END_SERIALIZE()
  };

  struct tx_derivation_hint
  {
    std::string msg;

    BEGIN_SERIALIZE()
      FIELD(msg)
    END_SERIALIZE()
  };

  struct tx_service_attachment
  {
    std::string service_id;    //string identifying service which addressed this attachment
    std::string instruction;   //string identifying specific instructions for service/way to interpret data
    std::string body;          //any data identifying service, options etc
    std::vector<crypto::public_key> security; //some of commands need proof of owner
    uint8_t flags;             //special flags (ex: TX_SERVICE_ATTACHMENT_ENCRYPT_BODY), see below

    BEGIN_SERIALIZE()
      FIELD(service_id)
      FIELD(instruction)
      FIELD(body)
      FIELD(security)
      FIELD(flags)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(service_id)                     DOC_DSCR("Service ID, identificator that diferent one service from another") DOC_EXMP("C")     DOC_END
      KV_SERIALIZE(instruction)                    DOC_DSCR("Instruction that make sence for this particular service") DOC_EXMP("K")              DOC_END
      KV_SERIALIZE_BLOB_AS_HEX_STRING(body)        DOC_DSCR("Hex-encoded body of the attachment") DOC_EXMP("dcfd7e055a6a3043ea3541a571a57a63e25dcc64e4a270f14fa9a58ac5dbec85dcfd7e055a6a3043ea3541a571a57a63e25dcc64e4a270f14fa9a58ac5dbec85")              DOC_END
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(security)  DOC_DSCR("Hex-encoded public key of the owner, optional") DOC_EXMP("d8f6e37f28a632c06b0b3466db1b9d2d1b36a580ee35edfd971dc1423bc412a5")              DOC_END
      KV_SERIALIZE(flags)                          DOC_DSCR("Flags that help wallet to automatically process some properties of the attachment(combination of TX_SERVICE_ATTACHMENT_ENCRYPT_BODY=1, TX_SERVICE_ATTACHMENT_DEFLATE_BODY=2, TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE=4,TX_SERVICE_ATTACHMENT_ENCRYPT_ADD_PROOF=8 )")  DOC_END
    END_KV_SERIALIZE_MAP()
  };

// applicable flags for tx_service_attachment::flags, can be combined using bitwise OR
#define TX_SERVICE_ATTACHMENT_ENCRYPT_BODY                    static_cast<uint8_t>(1 << 0)
#define TX_SERVICE_ATTACHMENT_DEFLATE_BODY                    static_cast<uint8_t>(1 << 1)

// with this flag enabled body encrypted/decrypted with the key created as a derivation from onetime key and "spend keys" of receiver
#define TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE  static_cast<uint8_t>(1 << 2)  
// add proof of content, without revealing secrete
#define TX_SERVICE_ATTACHMENT_ENCRYPT_ADD_PROOF               static_cast<uint8_t>(1 << 3)  

  //,

  
  struct extra_user_data
  {
    std::string buff;
    
    BEGIN_SERIALIZE()
      FIELD(buff)
    END_SERIALIZE()
  };


  struct extra_alias_entry_base_old
  {
    account_public_address_old m_address;
    std::string m_text_comment;
    std::vector<crypto::secret_key> m_view_key; // only one or zero elments expected (std::vector is using as memory efficient container for such a case)
    std::vector<crypto::signature> m_sign;      // only one or zero elments expected (std::vector is using as memory efficient container for such a case)

    BEGIN_SERIALIZE()
      FIELD(m_address)
      FIELD(m_text_comment)
      FIELD(m_view_key)
      FIELD(m_sign)
    END_SERIALIZE()
  };

  struct extra_alias_entry_old : public extra_alias_entry_base_old
  {
    std::string m_alias;

    BEGIN_SERIALIZE()
      FIELD(m_alias)
      FIELDS(*static_cast<extra_alias_entry_base_old*>(this))
   END_SERIALIZE()
  };

  struct extra_alias_entry_base
  {
    extra_alias_entry_base() = default;
    extra_alias_entry_base(const extra_alias_entry_base_old& old)
      : m_address(account_public_address::from_old(old.m_address))
      , m_text_comment(old.m_text_comment)
      , m_view_key(old.m_view_key)
      , m_sign(old.m_sign)
    {
    }

    account_public_address m_address;
    std::string m_text_comment;
    std::vector<crypto::secret_key> m_view_key; // only one or zero elments expected (std::vector is using as memory efficient container for such a case)
    std::vector<crypto::signature> m_sign;      // only one or zero elments expected (std::vector is using as memory efficient container for such a case)

    BEGIN_SERIALIZE()
      FIELD(m_address)
      FIELD(m_text_comment)
      FIELD(m_view_key)
      FIELD(m_sign)
    END_SERIALIZE()
  };

  struct extra_alias_entry : public extra_alias_entry_base
  {
    extra_alias_entry() = default;
    extra_alias_entry(const extra_alias_entry_old& old)
      : extra_alias_entry_base(old)
      , m_alias(old.m_alias)
    {}
    
    std::string m_alias;

    BEGIN_SERIALIZE()
      FIELD(m_alias)
      FIELDS(*static_cast<extra_alias_entry_base*>(this))
    END_SERIALIZE()

    extra_alias_entry_old to_old() const
    {
      extra_alias_entry_old result = AUTO_VAL_INIT(result);
      result.m_address = m_address.to_old();
      result.m_text_comment = m_text_comment;
      result.m_view_key = m_view_key;
      result.m_sign = m_sign;
      result.m_alias = m_alias;
      return result;
    }
  };

#define ASSET_DESCRIPTOR_BASE_HF4_VER    0
#define ASSET_DESCRIPTOR_BASE_HF5_VER    2
#define ASSET_DESCRIPTOR_BASE_LAST_VER   2

  struct dummy
  {
      BEGIN_SERIALIZE() 
      END_SERIALIZE()

      BEGIN_BOOST_SERIALIZATION()
      END_BOOST_SERIALIZATION_TOTAL_FIELDS(0)
  };

  typedef boost::variant<dummy> asset_descriptor_base_etc_fields;
  typedef boost::variant<crypto::public_key, crypto::eth_public_key> asset_owner_pub_key_v;

  struct asset_descriptor_base
  {
    uint64_t            total_max_supply = 0;
    uint64_t            current_supply = 0;
    uint8_t             decimal_point = 0;
    std::string         ticker;
    std::string         full_name;
    std::string         meta_info;
    crypto::public_key  owner = currency::null_pkey; // consider premultipling by 1/8
    bool                hidden_supply = false;
    uint8_t             version = ASSET_DESCRIPTOR_BASE_HF4_VER;
    //version 1 members
    boost::optional<crypto::eth_public_key> owner_eth_pub_key; // note: the size is 33 bytes (if present) // NOTE: using boost::optional instead of std::optional because of the Boost compilation issue: https://github.com/boostorg/serialization/issues/319 -- sowle
    //version 2 members
    std::vector<asset_descriptor_base_etc_fields> etc;  //container for future use if we would be adding some optional parameters that is not known yet, but without mess related to format version


    BEGIN_VERSIONED_SERIALIZE(ASSET_DESCRIPTOR_BASE_LAST_VER, version)
      FIELD(total_max_supply)
      FIELD(current_supply)
      FIELD(decimal_point)
      FIELD(ticker)
      FIELD(full_name)
      FIELD(meta_info)
      FIELD(owner)
      FIELD(hidden_supply)
      END_VERSION_UNDER(1)
      FIELD(owner_eth_pub_key)
      END_VERSION_UNDER(2)
      FIELD(etc)
    END_SERIALIZE()

    BOOST_SERIALIZATION_CURRENT_ARCHIVE_VER(2)
    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(total_max_supply)
      BOOST_SERIALIZE(current_supply)
      BOOST_SERIALIZE(decimal_point)
      BOOST_SERIALIZE(ticker)
      BOOST_SERIALIZE(full_name)
      BOOST_SERIALIZE(meta_info)
      BOOST_SERIALIZE(owner)
      BOOST_SERIALIZE(hidden_supply)
      BOOST_END_VERSION_UNDER(1)
      BOOST_SERIALIZE(owner_eth_pub_key)
      BOOST_END_VERSION_UNDER(2)
      BOOST_SERIALIZE(etc)
      BOOST_SERIALIZE(version)
    END_BOOST_SERIALIZATION_TOTAL_FIELDS(11)

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(total_max_supply)  DOC_DSCR("Maximum possible supply for a given asset, cannot be changed after deployment.") DOC_EXMP(1000000000000000000)   DOC_END
      KV_SERIALIZE(current_supply)    DOC_DSCR("Currently emitted supply for the given asset (ignored for REGISTER operation).") DOC_EXMP(500000000000000000)    DOC_END
      KV_SERIALIZE(decimal_point)     DOC_DSCR("Decimal point.")                      DOC_EXMP(12)                        DOC_END
      KV_SERIALIZE(ticker)            DOC_DSCR("Ticker associated with the asset.")   DOC_EXMP("ZABC")                    DOC_END
      KV_SERIALIZE(full_name)         DOC_DSCR("Full name of the asset.")             DOC_EXMP("Zano wrapped ABC")        DOC_END
      KV_SERIALIZE(meta_info)         DOC_DSCR("Any other information associated with the asset in free form.")           DOC_EXMP("Stable and private")      DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(owner) DOC_DSCR("Owner's key, used only for EMIT and UPDATE validation, can be changed by transferring asset ownership.")   DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8")        DOC_END
      KV_SERIALIZE(hidden_supply)     DOC_DSCR("This field is reserved for future use and will be documented later.") DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(owner_eth_pub_key) DOC_DSCR("[Optional] Owner's key in the case when ETH signature is used.") DOC_END
    END_KV_SERIALIZE_MAP()
  };


  struct asset_descriptor_with_id: public asset_descriptor_base
  {
    crypto::public_key asset_id = currency::null_pkey;

    /*
    BEGIN_VERSIONED_SERIALIZE()
      FIELD(*static_cast<asset_descriptor_base>(this))
      FIELD(asset_id)
    END_SERIALIZE()
    */

    BEGIN_KV_SERIALIZE_MAP()
      KV_CHAIN_BASE(asset_descriptor_base)
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)      DOC_DSCR("Asset ID") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8")   DOC_END
    END_KV_SERIALIZE_MAP()
  };


#define ASSET_DESCRIPTOR_OPERATION_UNDEFINED     0
#define ASSET_DESCRIPTOR_OPERATION_REGISTER      1
#define ASSET_DESCRIPTOR_OPERATION_EMIT          2
#define ASSET_DESCRIPTOR_OPERATION_UPDATE        3
#define ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN   4

#define ASSET_DESCRIPTOR_OPERATION_HF4_VER       1
#define ASSET_DESCRIPTOR_OPERATION_HF5_VER       2
#define ASSET_DESCRIPTOR_OPERATION_LAST_VER      2

  typedef boost::variant<dummy> asset_descriptor_operation_etc_fields;

  struct asset_descriptor_operation
  {
    uint8_t                                     operation_type = ASSET_DESCRIPTOR_OPERATION_UNDEFINED;
    uint8_t                                     version = ASSET_DESCRIPTOR_OPERATION_HF4_VER;
                                                                       // register  emit  burn  update
    boost::optional<crypto::public_key>         opt_amount_commitment; //    +       +     +      -      (premultiplied by 1/8)
    boost::optional<crypto::public_key>         opt_asset_id;          //    -       +     +      +
    boost::optional<asset_descriptor_base>      opt_descriptor;        //    +       -     -      +
    boost::optional<uint64_t>                   opt_amount;            //    ?       ?     ?      -      (only for non-hidden supply)
    boost::optional<uint32_t>                   opt_asset_id_salt;     //    ?       -     -      -      (optional)
    std::vector<asset_descriptor_operation_etc_fields> etc;            //                                (reserved for future use)


    BEGIN_VERSIONED_SERIALIZE(ASSET_DESCRIPTOR_OPERATION_LAST_VER, version)
      CHAIN_TRANSITION_VER(0, asset_descriptor_operation_v1)
      CHAIN_TRANSITION_VER(1, asset_descriptor_operation_v1)
      FIELD(operation_type)
      FIELD(opt_amount_commitment)
      FIELD(opt_asset_id)
      FIELD(opt_descriptor)
      FIELD(opt_amount)
      FIELD(opt_asset_id_salt)
      FIELD(etc)
    END_SERIALIZE()

    BOOST_SERIALIZATION_CURRENT_ARCHIVE_VER(2)
    BEGIN_BOOST_SERIALIZATION()
      BOOST_CHAIN_TRANSITION_VER(1, asset_descriptor_operation_v1)
      BOOST_CHAIN_TRANSITION_VER(0, asset_descriptor_operation_v1)
      BOOST_SERIALIZE(version)
      BOOST_SERIALIZE(operation_type)
      BOOST_SERIALIZE(opt_amount_commitment)
      BOOST_SERIALIZE(opt_asset_id)
      BOOST_SERIALIZE(opt_descriptor)
      BOOST_SERIALIZE(opt_amount)
      BOOST_SERIALIZE(opt_asset_id_salt)
      BOOST_SERIALIZE(etc)
    END_BOOST_SERIALIZATION_TOTAL_FIELDS(8)

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(version)                                     DOC_DSCR("Asset operation type struct version") DOC_EXMP(2) DOC_END
      KV_SERIALIZE(operation_type)                              DOC_DSCR("Asset operation type identifier") DOC_EXMP(1) DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(opt_amount_commitment)     DOC_DSCR("(optional) Asset operation amount commitment (register/emit/burn).") DOC_EXMP("5688b56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(opt_asset_id)              DOC_DSCR("(optional) ID of an asset (emit/burn/update).") DOC_EXMP("cc4e69455e63f4a581257382191de6856c2156630b3fba0db4bdd73ffcfb36b6") DOC_END
      KV_SERIALIZE(opt_descriptor)                              DOC_DSCR("(optional) Asset operation descriptor (register/update).") DOC_EXMP_AUTO()   DOC_END
      KV_SERIALIZE(opt_amount)                                  DOC_DSCR("(optional) Asset operation amount (register/emit/burn when supply is non-hidden).") DOC_EXMP_AUTO() DOC_END
      KV_SERIALIZE(opt_asset_id_salt)                           DOC_DSCR("(optional) Asset ID salt. May only be used for asset registration.") DOC_EXMP_AUTO() DOC_END
      //KV_SERIALIZE(etc)                                         DOC_DSCR("Extra operations") DOC_EXMP_AUTO() DOC_END <---- serialization for variant not supported yet
    END_KV_SERIALIZE_MAP()

  };

  struct asset_operation_proof
  {
    // linear composition proof for the fact amount_commitment = lin(asset_id, G)
    boost::optional<crypto::linear_composition_proof_s> opt_amount_commitment_composition_proof; // for hidden supply
    boost::optional<crypto::signature> opt_amount_commitment_g_proof; // for non-hidden supply, proofs that amount_commitment - supply * asset_id = lin(G)
    uint8_t version = 0;

    BEGIN_VERSIONED_SERIALIZE(0, version)
      FIELD(opt_amount_commitment_composition_proof)
      FIELD(opt_amount_commitment_g_proof)
    END_SERIALIZE()

    BOOST_SERIALIZATION_CURRENT_ARCHIVE_VER(1)
    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(opt_amount_commitment_composition_proof)
      BOOST_SERIALIZE(opt_amount_commitment_g_proof)
      BOOST_END_VERSION_UNDER(1)
      BOOST_SERIALIZE(version)
    END_BOOST_SERIALIZATION()
  };


  struct asset_operation_ownership_proof
  {
    crypto::generic_schnorr_sig_s gss;
    uint8_t version = 0;

    BEGIN_VERSIONED_SERIALIZE(0, version)
      FIELD(gss)
    END_SERIALIZE()

    BOOST_SERIALIZATION_CURRENT_ARCHIVE_VER(1)
    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(gss)
      BOOST_END_VERSION_UNDER(1)
      BOOST_SERIALIZE(version)
    END_BOOST_SERIALIZATION()
  };


  struct asset_operation_ownership_proof_eth
  {
    crypto::eth_signature eth_sig;            // 64 bytes
    uint8_t version = 0;

    BEGIN_VERSIONED_SERIALIZE(0, version)
      FIELD(eth_sig)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(eth_sig)
      BOOST_SERIALIZE(version)
    END_BOOST_SERIALIZATION()

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING(eth_sig)    DOC_DSCR("HEX-encoded ETH signature (64 bytes)") DOC_EXMP("674bb56a5b4fa562e679ccacc4e69455e63f4a581257382191de6856c2156630b3fba0db4bdd73ffcfb36b6add697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
      KV_SERIALIZE(version)                      DOC_DSCR("Structure version") DOC_EXMP(0) DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct extra_padding
  {
    std::vector<uint8_t> buff; //stub
    
    BEGIN_SERIALIZE()
      FIELD(buff)
    END_SERIALIZE()
  };


  //number of block (or timestamp if v bigger then CURRENCY_MAX_BLOCK_NUMBER), used as a limitation: spend this tx not early then block/time
  struct etc_tx_details_unlock_time
  {
    uint64_t v;
    BEGIN_SERIALIZE()
      VARINT_FIELD(v)
    END_SERIALIZE()
  };

  //number of block (or timestamp if unlock_time_array[i] bigger then CURRENCY_MAX_BLOCK_NUMBER), used as a limitation: spend this tx not early then block/time
  //unlock_time_array[i], i - index of output, unlock_time_array.size() == vout.size()
  struct etc_tx_details_unlock_time2
  {
    std::vector<uint64_t> unlock_time_array;
    BEGIN_SERIALIZE()
      FIELD(unlock_time_array)
    END_SERIALIZE()
  };

  struct etc_tx_details_expiration_time
  {
    uint64_t v;
    BEGIN_SERIALIZE()
      VARINT_FIELD(v)
    END_SERIALIZE()
  };

  /*
  this structure used to put real time into PoS block(or could be other meaning), to have more suitable dates in transactions
  */
  struct etc_tx_time
  {
    uint64_t v;
    BEGIN_SERIALIZE()
      VARINT_FIELD(v)
    END_SERIALIZE()
  };

  struct etc_tx_details_flags
  {
    uint64_t v;
    BEGIN_SERIALIZE()
      VARINT_FIELD(v)
    END_SERIALIZE()
  };

  struct etc_tx_flags16_t 
  {
    uint16_t v;
    BEGIN_SERIALIZE()
      FIELD(v)
    END_SERIALIZE()
  };

  typedef boost::mpl::vector23<
    tx_service_attachment, tx_comment, tx_payer_old, tx_receiver_old, tx_derivation_hint, std::string, tx_crypto_checksum, etc_tx_time, etc_tx_details_unlock_time, etc_tx_details_expiration_time,
    etc_tx_details_flags, crypto::public_key, extra_attachment_info, extra_alias_entry_old, extra_user_data, extra_padding, etc_tx_flags16_t, etc_tx_details_unlock_time2,
    tx_payer, tx_receiver, extra_alias_entry, zarcanum_tx_data_v1, asset_descriptor_operation
  > all_payload_types;
  
  typedef boost::make_variant_over<all_payload_types>::type payload_items_v;
  typedef payload_items_v extra_v;
  typedef payload_items_v attachment_v;




  //classic CryptoNote signature by Nicolas Van Saberhagen
  struct NLSAG_sig
  {
    std::vector<crypto::signature> s;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(s)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(s)
    END_BOOST_SERIALIZATION()
  };

  struct void_sig
  {
    //TODO:
    BEGIN_SERIALIZE_OBJECT()
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
    END_BOOST_SERIALIZATION()
  };

  typedef boost::variant<NLSAG_sig, void_sig, ZC_sig, zarcanum_sig> signature_v;

  typedef boost::variant<zc_asset_surjection_proof, zc_outs_range_proof, zc_balance_proof, asset_operation_proof, asset_operation_ownership_proof, asset_operation_ownership_proof_eth> proof_v;


  //include backward compatibility defintions
  #include "currency_basic_backward_comp.inl"

  class transaction_prefix
  {
  public:
    uint64_t version = 0;
    std::vector<txin_v> vin;
    std::vector<extra_v> extra;
    std::vector<tx_out_v> vout;

    BEGIN_SERIALIZE()
      VARINT_FIELD(version)
      CHAIN_TRANSITION_VER(TRANSACTION_VERSION_INITAL, transaction_prefix_v1)
      CHAIN_TRANSITION_VER(TRANSACTION_VERSION_PRE_HF4, transaction_prefix_v1)
      if(CURRENT_TRANSACTION_VERSION < version) return false;
      FIELD(vin)
      FIELD(extra)
      FIELD(vout)
    END_SERIALIZE()
  };

/*
  TX_FLAG_SIGNATURE_MODE_SEPARATE - flag that set different signature validation mode.
  With this mode each signature sign prefix with it's own txin entry only, that allow 
  construct transaction partially, supposed to be in use to construct multisig-based escrow transaction. 
*/
#define TX_FLAG_SIGNATURE_MODE_SEPARATE  0x01




  class transaction: public transaction_prefix
  {
  public:
    std::vector<attachment_v> attachment;
    std::vector<signature_v> signatures;
    std::vector<proof_v> proofs;

    BEGIN_SERIALIZE_OBJECT()
      FIELDS(*static_cast<transaction_prefix *>(this))
      CHAIN_TRANSITION_VER(TRANSACTION_VERSION_INITAL, transaction_v1)
      CHAIN_TRANSITION_VER(TRANSACTION_VERSION_PRE_HF4, transaction_v1)
      FIELD(attachment)
      FIELD(signatures)
      FIELD(proofs)
    END_SERIALIZE()


    BOOST_SERIALIZATION_CURRENT_ARCHIVE_VER(0)
    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(version)
      BOOST_SERIALIZE(vin)
      BOOST_SERIALIZE(vout)
      BOOST_SERIALIZE(extra)
      BOOST_SERIALIZE(signatures)
      BOOST_SERIALIZE(attachment)
      BOOST_END_VERSION_UNDER(1)
      BOOST_SERIALIZE(proofs)
      END_BOOST_SERIALIZATION()
  };

  



  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct block_header
  {
    uint8_t major_version;
    uint8_t minor_version;
    uint64_t timestamp;
    crypto::hash  prev_id;
    uint64_t nonce;
    uint8_t flags;

    BEGIN_SERIALIZE()
      FIELD(major_version)
      if(major_version > CURRENT_BLOCK_MAJOR_VERSION) return false;
      FIELD(nonce)
      FIELD(prev_id)
      VARINT_FIELD(minor_version)
      VARINT_FIELD(timestamp)
      FIELD(flags)
    END_SERIALIZE()
  };

  struct block: public block_header
  {
    transaction miner_tx;
    std::vector<crypto::hash> tx_hashes;
    
    BEGIN_SERIALIZE_OBJECT()
      FIELDS(*static_cast<block_header *>(this))
      FIELD(miner_tx)
      FIELD(tx_hashes)
    END_SERIALIZE()
  };




  struct keypair
  {
    crypto::public_key pub;
    crypto::secret_key sec;

    static inline keypair generate()
    {
      keypair k;
      generate_keys(k.pub, k.sec);
      return k;
    }
  };
  const static keypair null_keypair = AUTO_VAL_INIT(null_keypair);
  //---------------------------------------------------------------
  //PoS
  //based from ppcoin/novacoin approach

  /*
  POS PROTOCOL, stake modifier
  */
  //-------------------------------------------------------------------------------------------------------------------

#pragma pack(push, 1)
  struct stake_modifier_type
  {
    crypto::hash last_pow_id;
    crypto::hash last_pos_kernel_id;
  };

  struct stake_kernel
  {
    stake_modifier_type stake_modifier;
    uint64_t block_timestamp;             //this block timestamp
    crypto::key_image kimage;
  };
#pragma pack(pop)

  struct pos_entry
  {
    uint64_t amount;
    uint64_t g_index;              // global output index. (could be WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED)
    crypto::key_image keyimage;
    uint64_t block_timestamp;
    uint64_t stake_unlock_time;

    crypto::hash tx_id;          // stake output source tx id
    uint64_t tx_out_index;       // stake output local index in its tx

    //not for serialization

    uint64_t wallet_index;       // transfer id index

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(g_index)
      KV_SERIALIZE(stake_unlock_time)
      KV_SERIALIZE(block_timestamp)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(keyimage)
      KV_SERIALIZE(tx_id)
      KV_SERIALIZE(tx_out_index)
    END_KV_SERIALIZE_MAP()
  };

  bool operator ==(const currency::transaction& a, const currency::transaction& b);
  bool operator ==(const currency::block& a, const currency::block& b);
  bool operator ==(const currency::extra_attachment_info& a, const currency::extra_attachment_info& b);
  bool operator ==(const currency::NLSAG_sig& a, const currency::NLSAG_sig& b);
  bool operator ==(const currency::void_sig& a, const currency::void_sig& b);
  bool operator ==(const currency::ZC_sig& a, const currency::ZC_sig& b);
  bool operator ==(const currency::zarcanum_sig& a, const currency::zarcanum_sig& b);
  bool operator ==(const currency::ref_by_id& a, const currency::ref_by_id& b);

  // TODO: REPLACE all of the following operators to "bool operator==(..) const = default" once we moved to C++20 -- sowle
  bool operator ==(const currency::signed_parts& a, const currency::signed_parts& b);
  bool operator ==(const currency::txin_gen& a, const currency::txin_gen& b);
  bool operator ==(const currency::txin_to_key& a, const currency::txin_to_key& b);
  bool operator ==(const currency::txin_multisig& a, const currency::txin_multisig& b);
  bool operator ==(const currency::txin_htlc& a, const currency::txin_htlc& b);
  bool operator ==(const currency::txin_zc_input& a, const currency::txin_zc_input& b);
} // namespace currency

POD_MAKE_HASHABLE(currency, account_public_address);
POD_MAKE_HASHABLE(currency, account_public_address_old);

BLOB_SERIALIZER(currency::txout_to_key);

#define SET_VARIANT_TAGS(type_name, id, json_tag)  \
  VARIANT_TAG(binary_archive, type_name, id); \
  VARIANT_TAG(json_archive, type_name, json_tag)



LOOP_BACK_BOOST_SERIALIZATION_VERSION(currency::asset_descriptor_operation);
LOOP_BACK_BOOST_SERIALIZATION_VERSION(currency::asset_descriptor_base);
LOOP_BACK_BOOST_SERIALIZATION_VERSION(currency::asset_operation_proof);
LOOP_BACK_BOOST_SERIALIZATION_VERSION(currency::asset_operation_ownership_proof);
LOOP_BACK_BOOST_SERIALIZATION_VERSION(currency::transaction);


// txin_v variant currency
SET_VARIANT_TAGS(currency::txin_gen, 0, "gen");
SET_VARIANT_TAGS(currency::txin_to_key, 1, "key");
SET_VARIANT_TAGS(currency::txin_multisig, 2, "multisig");
// txout_target_v variant definitions
SET_VARIANT_TAGS(currency::txout_to_key, 3, "key");
SET_VARIANT_TAGS(currency::txout_multisig, 4, "multisig");
SET_VARIANT_TAGS(currency::transaction, 5, "tx");
SET_VARIANT_TAGS(currency::block, 6, "block");
//attachment_v definitions 
SET_VARIANT_TAGS(currency::tx_comment, 7, "comment");
SET_VARIANT_TAGS(currency::tx_payer_old, 8, "payer");
SET_VARIANT_TAGS(std::string, 9, "string");
SET_VARIANT_TAGS(currency::tx_crypto_checksum, 10, "checksum");
SET_VARIANT_TAGS(currency::tx_derivation_hint, 11, "derivation_hint");
SET_VARIANT_TAGS(currency::tx_service_attachment, 12, "attachment");
//SET_VARIANT_TAGS(currency::tx_onetime_secret_key, 13, "sec_key");
SET_VARIANT_TAGS(currency::etc_tx_details_unlock_time, 14, "unlock_time");
SET_VARIANT_TAGS(currency::etc_tx_details_expiration_time, 15, "expiration_time");
SET_VARIANT_TAGS(currency::etc_tx_details_flags, 16, "flags");
//txin_etc_details_v definitions 
SET_VARIANT_TAGS(currency::signed_parts, 17, "signed_outs");
//extra_v definitions
SET_VARIANT_TAGS(currency::extra_attachment_info, 18, "extra_attach_info");
SET_VARIANT_TAGS(currency::extra_user_data, 19, "user_data");
SET_VARIANT_TAGS(currency::extra_alias_entry_old, 20, "alias_entry");
SET_VARIANT_TAGS(currency::extra_padding, 21, "extra_padding");
SET_VARIANT_TAGS(crypto::public_key, 22, "pub_key");
SET_VARIANT_TAGS(currency::etc_tx_flags16_t, 23, "etc_tx_flags16");
SET_VARIANT_TAGS(uint16_t, 24, "derive_xor");
//txout_v 
SET_VARIANT_TAGS(currency::ref_by_id, 25, "ref_by_id");
SET_VARIANT_TAGS(uint64_t, 26, "uint64_t");
//etc
SET_VARIANT_TAGS(currency::etc_tx_time, 27, "etc_tx_time");
SET_VARIANT_TAGS(uint32_t, 28, "uint32_t");
SET_VARIANT_TAGS(currency::tx_receiver_old, 29, "payer"); // -- original
//SET_VARIANT_TAGS(currency::tx_receiver_old, 29, "receiver");
SET_VARIANT_TAGS(currency::etc_tx_details_unlock_time2, 30, "unlock_time2");

SET_VARIANT_TAGS(currency::tx_payer, 31, "payer2");
SET_VARIANT_TAGS(currency::tx_receiver, 32, "receiver2");

// @#@ TODO @#@
SET_VARIANT_TAGS(currency::extra_alias_entry, 33, "alias_entry2");

//htlc
SET_VARIANT_TAGS(currency::txin_htlc, 34, "txin_htlc");
SET_VARIANT_TAGS(currency::txout_htlc, 35, "txout_htlc");

SET_VARIANT_TAGS(currency::tx_out_bare, 36, "tx_out_bare");

// Zarcanum
SET_VARIANT_TAGS(currency::txin_zc_input, 37, "txin_zc_input");
SET_VARIANT_TAGS(currency::tx_out_zarcanum, 38, "tx_out_zarcanum");
SET_VARIANT_TAGS(currency::zarcanum_tx_data_v1, 39, "zarcanum_tx_data_v1");
SET_VARIANT_TAGS(crypto::bpp_signature_serialized, 40, "bpp_signature_serialized");
SET_VARIANT_TAGS(crypto::bppe_signature_serialized, 41, "bppe_signature_serialized");
SET_VARIANT_TAGS(currency::NLSAG_sig, 42, "NLSAG_sig");
SET_VARIANT_TAGS(currency::ZC_sig, 43, "ZC_sig");
SET_VARIANT_TAGS(currency::void_sig, 44, "void_sig");
SET_VARIANT_TAGS(currency::zarcanum_sig, 45, "zarcanum_sig");
SET_VARIANT_TAGS(currency::zc_asset_surjection_proof, 46, "zc_asset_surjection_proof");
SET_VARIANT_TAGS(currency::zc_outs_range_proof, 47, "zc_outs_range_proof");
SET_VARIANT_TAGS(currency::zc_balance_proof, 48, "zc_balance_proof");

SET_VARIANT_TAGS(currency::asset_descriptor_operation, 49, "asset_descriptor_base");
SET_VARIANT_TAGS(currency::asset_operation_proof, 50, "asset_operation_proof");
SET_VARIANT_TAGS(currency::asset_operation_ownership_proof, 51, "asset_operation_ownership_proof");
SET_VARIANT_TAGS(currency::asset_operation_ownership_proof_eth, 52, "asset_operation_ownership_proof_eth");

SET_VARIANT_TAGS(crypto::eth_public_key, 60, "eth_public_key");
//SET_VARIANT_TAGS(crypto::eth_signature, 61, "eth_signature");
SET_VARIANT_TAGS(currency::dummy, 62, "dummy");




#undef SET_VARIANT_TAGS
