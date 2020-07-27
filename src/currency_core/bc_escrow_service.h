// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "account.h"

#define BC_ESCROW_SERVICE_ID                                "E"

#define BC_ESCROW_SERVICE_INSTRUCTION_PROPOSAL               "PROP"
#define BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_TEMPLATES      "REL_TEMPL"
#define BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL        "CAN_PROP"
#define BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL         "REL_N"
#define BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL         "REL_C"
#define BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN           "REL_B"
//#define BC_ESCROW_SERVICE_INSTRUCTION_ACCEPT                 "ACC"
#define BC_ESCROW_SERVICE_INSTRUCTION_CHANGE                 "CHANGE"
#define BC_ESCROW_SERVICE_INSTRUCTION_PRIVATE_DETAILS        "DETAILS"   //this part supposed to be in "extra" part, not attachment
#define BC_ESCROW_SERVICE_INSTRUCTION_PUBLIC_DETAILS         "PUB"   //this part supposed to be in "extra" part, not attachment

namespace bc_services
{

  struct contract_private_details
  {
    std::string title;
    std::string comment;
    currency::account_public_address a_addr; // usually buyer
    currency::account_public_address b_addr; // usually seller
    uint64_t amount_to_pay;
    uint64_t amount_a_pledge;
    uint64_t amount_b_pledge;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_N(title, "t")
      KV_SERIALIZE_N(comment, "c")
      KV_SERIALIZE_ADDRESS_AS_TEXT_N(a_addr, "a_addr")
      KV_SERIALIZE_ADDRESS_AS_TEXT_N(b_addr, "b_addr")
      KV_SERIALIZE_N(amount_to_pay, "to_pay")
      KV_SERIALIZE_N(amount_a_pledge, "a_pledge")
      KV_SERIALIZE_N(amount_b_pledge, "b_pledge")
    END_KV_SERIALIZE_MAP()
  };

  struct contract_public_details
  {
    currency::account_public_address b_addr; // usually seller
    uint64_t amount_to_pay;
    crypto::signature address_proof; //multisig_id signed by private spend key

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_ADDRESS_AS_TEXT_N(b_addr, "a_b")
      KV_SERIALIZE_N(amount_to_pay, "to_pay")
      KV_SERIALIZE_POD_AS_HEX_STRING_N(address_proof, "proof")
    END_KV_SERIALIZE_MAP()
  };


  // BC_ESCROW_SERVICE_INSTRUCTION_PROPOSAL
  struct proposal_body
  {
    currency::transaction   tx_template;
    crypto::secret_key      tx_onetime_secret_key;
    BEGIN_SERIALIZE()
      FIELD(tx_template)
      FIELD(tx_onetime_secret_key)
    END_SERIALIZE()
  };

  struct escrow_relese_templates_body
  {
    currency::transaction   tx_normal_template;
    currency::transaction   tx_burn_template;

    BEGIN_SERIALIZE()
      FIELD(tx_normal_template)
      FIELD(tx_burn_template)
    END_SERIALIZE()
  };

  struct escrow_cancel_templates_body
  {
    currency::transaction   tx_cancel_template;

    BEGIN_SERIALIZE()
      FIELD(tx_cancel_template)
    END_SERIALIZE()
  };

}


#define CURRENCY_CPD_ARCHIVE_VER 1
BOOST_CLASS_VERSION(bc_services::contract_private_details, CURRENCY_CPD_ARCHIVE_VER)

namespace boost
{
  namespace serialization
  {
    template<class archive_t>
    void serialize(archive_t & ar, bc_services::contract_private_details& cpd, const unsigned int version)
    {
      ar & cpd.title;
      ar & cpd.comment;
      if (version == 0 && !archive_t::is_saving::value)
      {
        // loading from version 0 (pre-auditable address format)
        currency::account_public_address_old old_addr = AUTO_VAL_INIT(old_addr);
        ar & old_addr;
        cpd.a_addr = currency::account_public_address::from_old(old_addr);
        ar & old_addr;
        cpd.b_addr = currency::account_public_address::from_old(old_addr);
      }
      else
      {
        ar & cpd.a_addr; // usually buyer
        ar & cpd.b_addr; // usually seller
      }
      ar & cpd.amount_to_pay;
      ar & cpd.amount_a_pledge;
      ar & cpd.amount_b_pledge;
    }


    template <class Archive>
    inline void serialize(Archive& a, bc_services::escrow_relese_templates_body& x, const unsigned int ver)
    {
      a & x.tx_normal_template;
      a & x.tx_burn_template;
    }

    template <class Archive>
    inline void serialize(Archive& a, bc_services::escrow_cancel_templates_body& x, const unsigned int ver)
    {
      a & x.tx_cancel_template;
    }

    template <class Archive>
    inline void serialize(Archive& a, bc_services::proposal_body& x, const unsigned int ver)
    {
      a & x.tx_onetime_secret_key;
      a & x.tx_template;
    }
  }
}