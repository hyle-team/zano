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
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_same.hpp>

#include <vector>
#include <cstring>  // memcmp
#include <sstream>
#include <chrono>

#include "include_base_utils.h"

#include "serialization/binary_archive.h"
#include "serialization/crypto.h"
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

  typedef std::string payment_id_t;


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  
  //since structure used in blockchain as a key accessor, then be sure that there is no padding inside
#pragma pack(push, 1)
  struct account_public_address
  {
    crypto::public_key m_spend_public_key;
    crypto::public_key m_view_public_key;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(m_spend_public_key)
      FIELD(m_view_public_key)
    END_SERIALIZE()

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_spend_public_key)
        KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_view_public_key)
      END_KV_SERIALIZE_MAP()
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

  typedef boost::variant<uint64_t, ref_by_id> txout_v;
  

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

  struct txin_to_key
  {
    uint64_t amount;
    std::vector<txout_v> key_offsets;
    crypto::key_image k_image;                    // double spending protection
    std::vector<txin_etc_details_v> etc_details;  //this flag used when TX_FLAG_SIGNATURE_MODE_SEPARATE flag is set, point to which amount of outputs(starting from zero) used in signature

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(key_offsets)
      FIELD(k_image)
      FIELD(etc_details)
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

  typedef boost::variant<txin_gen, txin_to_key, txin_multisig> txin_v;

  typedef boost::variant<txout_to_key, txout_multisig> txout_target_v;

  //typedef std::pair<uint64_t, txout> out_t;
  struct tx_out
  {
    uint64_t amount;
    txout_target_v target;

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(target)
    END_SERIALIZE()
  };



  struct tx_comment
  {
    std::string comment;

    BEGIN_SERIALIZE()
      FIELD(comment)
    END_SERIALIZE()
  };

  struct tx_payer
  {
    account_public_address acc_addr;

    BEGIN_SERIALIZE()
      FIELD(acc_addr)
    END_SERIALIZE()
  };

  struct tx_receiver
  {
    account_public_address acc_addr;

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

  struct tx_message
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
  };

// applicable flags for tx_service_attachment::flags, can be combined using bitwise OR
#define TX_SERVICE_ATTACHMENT_ENCRYPT_BODY  static_cast<uint8_t>(1 << 0)
#define TX_SERVICE_ATTACHMENT_DEFLATE_BODY  static_cast<uint8_t>(1 << 1)
  //,

  
  struct extra_user_data
  {
    std::string buff;
    
    BEGIN_SERIALIZE()
      FIELD(buff)
    END_SERIALIZE()
  };


  struct extra_alias_entry_base
  {
    account_public_address m_address;
    std::string m_text_comment;
    std::vector<crypto::secret_key> m_view_key; // only one or zero elments expected (std::vector is using as memory efficient container for such a case)
    std::vector<crypto::signature> m_sign;      // only one or zero elments expected (std::vector is using as memory efficient container for such a case)
    //uint8_t flags;

    BEGIN_SERIALIZE()
      FIELD(m_address)
      FIELD(m_text_comment)
      FIELD(m_view_key)
      FIELD(m_sign)
      //FIELD(flags)
    END_SERIALIZE()
  };

  struct extra_alias_entry: public extra_alias_entry_base
  {
    std::string m_alias;

    BEGIN_SERIALIZE()
      FIELD(m_alias)
      FIELDS(*static_cast<extra_alias_entry_base *>(this))
   END_SERIALIZE()
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

  struct etc_tx_derivation_hint 
  {
    uint16_t v;
    BEGIN_SERIALIZE()
      FIELD(v)
    END_SERIALIZE()
  };

  typedef boost::mpl::vector<tx_service_attachment, tx_comment, tx_payer, tx_receiver, tx_message, std::string, tx_crypto_checksum, etc_tx_time, etc_tx_details_unlock_time, etc_tx_details_expiration_time, etc_tx_details_flags, crypto::public_key, extra_attachment_info, extra_alias_entry, extra_user_data, extra_padding, etc_tx_derivation_hint, etc_tx_details_unlock_time2> all_payload_types;
  typedef boost::make_variant_over<all_payload_types>::type attachment_v;
  typedef boost::make_variant_over<all_payload_types>::type extra_v;
  typedef boost::make_variant_over<all_payload_types>::type payload_items_v;

  class transaction_prefix
  {
  public:
    // tx version information
    size_t   version{};
    //extra
    std::vector<extra_v> extra;  
    std::vector<txin_v> vin;
    std::vector<tx_out> vout;

    BEGIN_SERIALIZE()
      VARINT_FIELD(version)
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
    std::vector<std::vector<crypto::signature> > signatures; //count signatures  always the same as inputs count
    std::vector<attachment_v> attachment;

    transaction();

    BEGIN_SERIALIZE_OBJECT()
      FIELDS(*static_cast<transaction_prefix *>(this))
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
      //if(major_version > CURRENT_BLOCK_MAJOR_VERSION) return false;
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
SET_VARIANT_TAGS(currency::tx_payer, 8, "payer");
SET_VARIANT_TAGS(std::string, 9, "string");
SET_VARIANT_TAGS(currency::tx_crypto_checksum, 10, "checksum");
SET_VARIANT_TAGS(currency::tx_message, 11, "message");
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
SET_VARIANT_TAGS(currency::extra_alias_entry, 20, "alias_entry");
SET_VARIANT_TAGS(currency::extra_padding, 21, "extra_padding");
SET_VARIANT_TAGS(crypto::public_key, 22, "pub_key");
SET_VARIANT_TAGS(currency::etc_tx_derivation_hint, 23, "derive_hint");
SET_VARIANT_TAGS(uint16_t, 24, "derive_xor");
//txout_v 
SET_VARIANT_TAGS(currency::ref_by_id, 25, "ref_by_id");
SET_VARIANT_TAGS(uint64_t, 26, "uint64_t");
//etc
SET_VARIANT_TAGS(currency::etc_tx_time, 27, "etc_tx_time");
SET_VARIANT_TAGS(uint32_t, 28, "uint32_t");
SET_VARIANT_TAGS(currency::tx_receiver, 29, "payer");
SET_VARIANT_TAGS(currency::etc_tx_details_unlock_time2, 30, "unlock_time2");

#undef SET_VARIANT_TAGS
