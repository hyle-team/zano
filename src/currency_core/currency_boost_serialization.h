// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/foreach.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include "currency_basic.h"
#include "common/unordered_containers_boost_serialization.h"
#include "common/crypto_serialization.h"
#include "offers_services_helpers.h"

#define CURRENT_BLOCK_ARCHIVE_VER   2
BOOST_CLASS_VERSION(currency::block, CURRENT_BLOCK_ARCHIVE_VER)

namespace boost
{
  namespace serialization
  {

    template <class Archive>
    inline void serialize(Archive &a, currency::account_public_address &x, const boost::serialization::version_type ver)
    {
      //a & x.version;
      a & x.flags;
      a & x.spend_public_key;
      a & x.view_public_key;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::account_public_address_old &x, const boost::serialization::version_type ver)
    {
      a & x.spend_public_key;
      a & x.view_public_key;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::txout_to_key &x, const boost::serialization::version_type ver)
    {
      a & x.key;
      a & x.mix_attr;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::txout_multisig &x, const boost::serialization::version_type ver)
    {
      a & x.minimum_sigs;
      a & x.keys;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::txout_htlc &x, const boost::serialization::version_type ver)
    {
      a & x.expiration;
      a & x.flags;
      a & x.htlc_hash;
      a & x.pkey_redeem;
      a & x.pkey_refund;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::txin_gen &x, const boost::serialization::version_type ver)
    {
      a & x.height;
    }


    template <class Archive>
    inline void serialize(Archive &a, currency::txin_multisig &x, const boost::serialization::version_type ver)
    {

      a & x.amount;
      a & x.multisig_out_id;
      a & x.sigs_count;
      a & x.etc_details;
    }


    template <class Archive>
    inline void serialize(Archive &a, currency::txin_to_key &x, const boost::serialization::version_type ver)
    {
      a & x.amount;
      a & x.key_offsets;
      a & x.k_image;
      a & x.etc_details;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::txin_htlc &x, const boost::serialization::version_type ver)
    {
      a & x.amount;
      a & x.etc_details;
      a & x.hltc_origin;
      a & x.k_image;
      a & x.key_offsets;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_out_bare &x, const boost::serialization::version_type ver)
    {
      a & x.amount;
      a & x.target;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_out_zarcanum &x, const boost::serialization::version_type ver)
    {
      a & x.stealth_address;
      a & x.concealing_point;
      a & x.amount_commitment;
      a & x.blinded_asset_id;
      a & x.encrypted_amount;
      a & x.mix_attr;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_comment &x, const boost::serialization::version_type ver)
    {
      a & x.comment;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_payer_old &x, const boost::serialization::version_type ver)
    {
      a & x.acc_addr;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_payer &x, const boost::serialization::version_type ver)
    {
      a & x.acc_addr;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_receiver_old &x, const boost::serialization::version_type ver)
    {
      a & x.acc_addr;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_receiver &x, const boost::serialization::version_type ver)
    {
      a & x.acc_addr;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_crypto_checksum &x, const boost::serialization::version_type ver)
    {
      a & x.encrypted_key_derivation;
      a & x.derivation_hash;
    }
    template <class Archive>
    inline void serialize(Archive &a, currency::tx_derivation_hint &x, const boost::serialization::version_type ver)
    {
      a & x.msg;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::extra_attachment_info &x, const boost::serialization::version_type ver)
    {
      a & x.hsh;
      a & x.sz;
      a & x.cnt;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::extra_user_data &x, const boost::serialization::version_type ver)
    {
      a & x.buff;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::extra_alias_entry_base &x, const boost::serialization::version_type ver)
    {
      a & x.m_address;
      a & x.m_text_comment;
      a & x.m_view_key;
      a & x.m_sign;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::extra_alias_entry &x, const boost::serialization::version_type ver)
    {
      a & x.m_alias;
      a & static_cast<currency::extra_alias_entry_base&>(x);
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::extra_alias_entry_base_old &x, const boost::serialization::version_type ver)
    {
      a & x.m_address;
      a & x.m_text_comment;
      a & x.m_view_key;
      a & x.m_sign;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::extra_alias_entry_old &x, const boost::serialization::version_type ver)
    {
      a & x.m_alias;
      a & static_cast<currency::extra_alias_entry_base_old&>(x);
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::signed_parts &x, const boost::serialization::version_type ver)
    {
      a & x.n_outs;
      a & x.n_extras;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::extra_padding &x, const boost::serialization::version_type ver)
    {
      a & x.buff;
    }



    template <class Archive>
    inline void serialize(Archive &a, currency::keypair &kp, const boost::serialization::version_type ver)
    {
      a & kp.pub;
      a & kp.sec; 
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::tx_service_attachment &at, const boost::serialization::version_type ver)
    {
      a & at.service_id;
      a & at.security;
      a & at.instruction;
      a & at.body;
      a & at.flags;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::etc_tx_details_unlock_time &at, const boost::serialization::version_type ver)
    {
      a & at.v;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::etc_tx_details_unlock_time2 &at, const boost::serialization::version_type ver)
    {
      a & at.unlock_time_array;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::etc_tx_details_expiration_time &at, const boost::serialization::version_type ver)
    {
      a & at.v;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::etc_tx_details_flags &at, const boost::serialization::version_type ver)
    {
      a & at.v;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::etc_tx_time &at, const boost::serialization::version_type ver)
    {
      a & at.v;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::etc_tx_flags16_t&at, const boost::serialization::version_type ver)
    {
      a & at.v;
    }


    template <class Archive>
    inline void serialize(Archive &a, currency::block &b, const boost::serialization::version_type ver)
    {
      if (ver < CURRENT_BLOCK_ARCHIVE_VER)
      {
        throw std::runtime_error("wrong block serialization version");
      }
      a & b.major_version;
      a & b.minor_version;
      a & b.timestamp;
      a & b.prev_id;
      a & b.nonce;
      //------------------
      a & b.miner_tx;
      a & b.tx_hashes;
      a & b.flags;
    }

    
    template <class Archive>
      inline void serialize(Archive &a, currency::ref_by_id &o, const boost::serialization::version_type ver)
    {
      a & o.tx_id;
      a & o.n;
    }


  }
}
