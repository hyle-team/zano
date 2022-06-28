// Copyright (c) 2014-2019 Zano Project
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
#include "serialization/json_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/keyvalue_serialization.h" // epee key-value serialization
#include "string_tools.h"
#include "currency_config.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto/range_proofs.h"
#include "misc_language.h"
#include "block_flags.h"
#include "etc_custom_serialization.h"

namespace currency
{

  const static crypto::hash null_hash = AUTO_VAL_INIT(null_hash);
  const static crypto::public_key null_pkey = AUTO_VAL_INIT(null_pkey);
  const static crypto::key_image null_ki = AUTO_VAL_INIT(null_ki);
  const static crypto::secret_key null_skey = AUTO_VAL_INIT(null_skey);
  const static crypto::signature null_sig = AUTO_VAL_INIT(null_sig);
  const static crypto::key_derivation null_derivation = AUTO_VAL_INIT(null_derivation);

  const static crypto::hash gdefault_genesis = epee::string_tools::hex_to_pod<crypto::hash>("CC608F59F8080E2FBFE3C8C80EB6E6A953D47CF2D6AEBD345BADA3A1CAB99852");

  typedef std::string payment_id_t;


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  
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
  //#pragma pack(push, 1)
  struct zarcanum_input : public referring_input
  {
    zarcanum_input() {}
    // Boost's Assignable concept
    zarcanum_input(const zarcanum_input&)           = default;
    zarcanum_input& operator=(const zarcanum_input&)= default;

    crypto::key_image               k_image;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(k_image)
      FIELD(key_offsets) // referring_input
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(k_image)
      BOOST_SERIALIZE(key_offsets) // referring_input
    END_BOOST_SERIALIZATION()
  };

  // txin_zarcanum_inputs contains several zarcanum_input instances and corresponds to one zarcanum_sig
  struct txin_zarcanum_inputs
  {
    txin_zarcanum_inputs() {}

    // Boost's Assignable concept
    txin_zarcanum_inputs(const txin_zarcanum_inputs&)             = default;
    txin_zarcanum_inputs& operator=(const txin_zarcanum_inputs&)  = default;

    std::vector<zarcanum_input>     elements;
    std::vector<txin_etc_details_v> etc_details;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(elements)
      FIELD(etc_details)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(elements)
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
    crypto::public_key  concealing_point; // group element Q, see also Zarcanum paper
    crypto::public_key  amount_commitment;
    uint64_t            encrypted_amount;
    //crypto::public_key  token_masked_generator;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(stealth_address)
      FIELD(concealing_point)
      FIELD(amount_commitment)
      FIELD(encrypted_amount)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(stealth_address)
      BOOST_SERIALIZE(concealing_point)
      BOOST_SERIALIZE(amount_commitment)
      BOOST_SERIALIZE(encrypted_amount)
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

  struct zarcanum_sig
  {
    struct input_proofs_t
    {
      crypto::public_key real_out_amount_commitment;
      // crypto::public_key real_out_token_masked_generator;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(real_out_amount_commitment)
      END_SERIALIZE()

      BEGIN_BOOST_SERIALIZATION()
        BOOST_SERIALIZE(real_out_amount_commitment)
      END_BOOST_SERIALIZATION()
    };

    crypto::bpp_signature_serialized outputs_range_proof; // aggregated range proof for some or all outputs
    std::vector<input_proofs_t> input_proofs; // for each input

    BEGIN_SERIALIZE_OBJECT()
      FIELD(outputs_range_proof)
      FIELD(input_proofs)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(outputs_range_proof)
      BOOST_SERIALIZE(input_proofs)
    END_BOOST_SERIALIZATION()
  };
//#pragma pack(pop)

  typedef boost::variant<txin_gen, txin_to_key, txin_multisig, txin_htlc, txin_zarcanum_inputs> txin_v;

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
      KV_SERIALIZE(service_id)
      KV_SERIALIZE(instruction)
      KV_SERIALIZE_BLOB_AS_HEX_STRING(body)
      KV_SERIALIZE_CONTAINER_POD_AS_BLOB(security)
      KV_SERIALIZE(flags)
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

  typedef boost::mpl::vector22<
    tx_service_attachment, tx_comment, tx_payer_old, tx_receiver_old, tx_derivation_hint, std::string, tx_crypto_checksum, etc_tx_time, etc_tx_details_unlock_time, etc_tx_details_expiration_time,
    etc_tx_details_flags, crypto::public_key, extra_attachment_info, extra_alias_entry_old, extra_user_data, extra_padding, etc_tx_flags16_t, etc_tx_details_unlock_time2,
    tx_payer, tx_receiver, extra_alias_entry, zarcanum_tx_data_v1
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

  //TODO: @val, should we call it something like schnorr_sig ? 
  struct simple_sig
  {
    crypto::signature s;

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


  typedef boost::variant<NLSAG_sig, void_sig, zarcanum_sig> signature_v;




  //include backward compatibility defintions
  #include "currency_basic_backward_comp.inl"

  class transaction_prefix
  {
  public:
    // tx version information
    uint64_t   version{};
    //extra
    std::vector<extra_v> extra;
    std::vector<txin_v> vin;
    std::vector<tx_out_v> vout;

    BEGIN_SERIALIZE()
      VARINT_FIELD(version)
      CHAIN_TRANSITION_VER(TRANSACTION_VERSION_INITAL, transaction_prefix_v1)
      CHAIN_TRANSITION_VER(TRANSACTION_VERSION_PRE_HF4, transaction_prefix_v1)
      if(CURRENT_TRANSACTION_VERSION < version) return false;
      FIELD(vin)
      FIELD(vout)
      FIELD(extra)
    END_SERIALIZE()

  protected:
    transaction_prefix(){}
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
    std::vector<signature_v> signatures;
    std::vector<attachment_v> attachment;

    transaction();

    BEGIN_SERIALIZE_OBJECT()
      FIELDS(*static_cast<transaction_prefix *>(this))
      CHAIN_TRANSITION_VER(TRANSACTION_VERSION_INITAL, transaction_v1)
      CHAIN_TRANSITION_VER(TRANSACTION_VERSION_PRE_HF4, transaction_v1)
      FIELD(signatures)
      FIELD(attachment)
    END_SERIALIZE()
  };

  


  inline
  transaction::transaction()
  {
    version = 0;
    vin.clear();
    vout.clear();
    extra.clear();
    signatures.clear();
    attachment.clear();
    
  }
  /*
  inline
  transaction::~transaction()
  {
    //set_null();
  }

  inline
  void transaction::set_null()
  {
    version = 0;
    unlock_time = 0;
    vin.clear();
    vout.clear();
    extra.clear();
    signatures.clear();
  }
  */




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
    uint64_t index;
    crypto::key_image keyimage;
    uint64_t block_timestamp;
    uint64_t stake_unlock_time;
    //not for serialization
    uint64_t wallet_index;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(index)
      KV_SERIALIZE(stake_unlock_time)
      KV_SERIALIZE(block_timestamp)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(keyimage)
    END_KV_SERIALIZE_MAP()
  };

} // namespace currency

POD_MAKE_HASHABLE(currency, account_public_address);
POD_MAKE_HASHABLE(currency, account_public_address_old);

BLOB_SERIALIZER(currency::txout_to_key);

#define SET_VARIANT_TAGS(type_name, id, json_tag)  \
  VARIANT_TAG(binary_archive, type_name, id); \
  VARIANT_TAG(json_archive, type_name, json_tag)



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
SET_VARIANT_TAGS(currency::txin_zarcanum_inputs, 37, "txin_zarcanum_inputs");
SET_VARIANT_TAGS(currency::tx_out_zarcanum, 38, "tx_out_zarcanum");
SET_VARIANT_TAGS(currency::zarcanum_tx_data_v1, 39, "zarcanum_tx_data_v1");
SET_VARIANT_TAGS(crypto::bpp_signature_serialized, 40, "bpp_signature_serialized");
SET_VARIANT_TAGS(crypto::bppe_signature_serialized, 41, "bppe_signature_serialized");
SET_VARIANT_TAGS(currency::NLSAG_sig, 42, "NLSAG_sig");
SET_VARIANT_TAGS(currency::zarcanum_sig, 43, "zarcanum_sig");
SET_VARIANT_TAGS(currency::void_sig, 44, "void_sig");





#undef SET_VARIANT_TAGS
