// Copyright (c) 2018-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"


namespace currency
{
  namespace hardfork_specific_terms
  {
    template<typename x_type>
    struct visitor_proxy : public boost::static_visitor<const x_type*>
    {
      const x_type* operator()(const x_type& v)const
      {
        return &v;
      }
      template<typename t_type>
      const x_type* operator()(const t_type& v)const { return nullptr; }
    };

    //------------------------------------------------------------------
    template<typename T>
    struct type_hf_terms_traits;

    enum allowance
    {
      no = 0,
      one = 1,
      many = 2
    };

    const uint8_t input   = 1 << 0;
    const uint8_t output  = 1 << 1;
    const uint8_t extra   = 1 << 2;
    const uint8_t attach  = 1 << 3;
    const uint8_t signtr  = 1 << 4;
    const uint8_t proofs  = 1 << 5;
    const uint8_t all_containers[] = { input, output, extra, attach, signtr, proofs };

    inline const char* get_container_name(uint8_t container_id)
    {
      switch(container_id)
      {
      case input :  return "inputs";
      case output:  return "outputs";
      case extra:   return "extra";
      case attach:  return "attachment";
      case signtr:  return "signatures";
      case proofs:  return "proofs";
      default:      return "unknown";
      }
    }

    inline std::string get_container_mask_name(uint8_t container_mask)
    {
      std::string result;
      for(size_t i = 0; i < sizeof all_containers / sizeof all_containers[0]; ++i)
        if (all_containers[i] & container_mask)
          result = result + get_container_name(all_containers[i]) + ",";
      return result.empty() ? result : result.substr(0, result.size() - 1);
    }


  #define DEFINE_TERMS(hf0_val, hf1_val, hf2_val, hf3_val, hf4_val, hf5_val, hf6_val, hf7_val, container_mask_val, Type) \
    template <> struct type_hf_terms_traits<Type> {                     \
      static constexpr allowance hf[] = { hf0_val, hf1_val, hf2_val, hf3_val, hf4_val, hf5_val, hf6_val, hf7_val }; \
      static constexpr uint8_t container_mask = container_mask_val;   \
    };
    
   
   //hard fork id:  0     1     2     3     4     5     6     7
   // 
   //payloads
    DEFINE_TERMS(   many, many, many, many, many, many, many, many,   extra|attach,      tx_service_attachment              );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             tx_comment                         );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  no,   no,     extra|attach,      tx_payer_old                       );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  no,   no,     extra|attach,      tx_receiver_old                    );
    DEFINE_TERMS(   no,   no,   one,  one,  one,  one,  no,   no,     extra|attach,      tx_payer                           );
    DEFINE_TERMS(   no,   no,   one,  one,  one,  one,  no,   no,     extra|attach,      tx_receiver                        );
    DEFINE_TERMS(   many, many, many, many, many, many, many, many,   extra,             tx_derivation_hint                 );
    DEFINE_TERMS(   many, many, many, many, many, many, many, many,   extra|attach,      std::string                        );
    DEFINE_TERMS(   many, many, many, many, many, many, many, many,   extra|attach,      tx_crypto_checksum                 );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             etc_tx_time                        );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  no,   no,     extra,             etc_tx_details_unlock_time         );
    DEFINE_TERMS(   no,   one,  one,  one,  one,  one,  no,   no,     extra,             etc_tx_details_unlock_time2        );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             etc_tx_details_expiration_time     );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             etc_tx_details_flags               );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             crypto::public_key                 );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             extra_attachment_info              );
    DEFINE_TERMS(   one,  one,  no,   no,   no,   no,   no,   no,     extra,             extra_alias_entry_old              );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             extra_user_data                    );   //how we use this?
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             extra_padding                      );
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    extra,             etc_tx_flags16_t                   );
    DEFINE_TERMS(   no,   no,   one,  one,  one,  one,  one,  one,    extra,             extra_alias_entry                  );
    DEFINE_TERMS(   no,   no,   no,   no,   one,  one,  one,  one,    extra,             zarcanum_tx_data_v1                );
    DEFINE_TERMS(   no,   no,   no,   no,   one,  one,  one,  one,    extra,             asset_descriptor_operation         );
    DEFINE_TERMS(   no,   no,   no,   no,   no,   no,   one,  one,    extra,             gateway_address_descriptor_operation);
    DEFINE_TERMS(   no,   no,   no,   no,   no,   no,   one,  one,    extra,             etc_coinbase_block_cumulative_size );    
      //inputs
    DEFINE_TERMS(   one,  one,  one,  one,  one,  one,  one,  one,    input,             txin_gen                           );
    DEFINE_TERMS(   many, many, many, many, many, many, many, many,   input,             txin_to_key                        );
    DEFINE_TERMS(   many, many, many, many, many, many, many, many,   input,             txin_multisig                      );
    DEFINE_TERMS(   no,   no,   no,   no,   many, many, many, many,   input,             txin_zc_input                      );
    DEFINE_TERMS(   no,   no,   no,   no,   no,   no,   many, many,   input,             txin_gateway                       );
      //outputs
    DEFINE_TERMS(   many, many, many, many, no,   no,   no,   no,     output,            tx_out_bare                        );
    DEFINE_TERMS(   no,   no,   no,   no,   many, many, many, many,   output,            tx_out_zarcanum                    );
    DEFINE_TERMS(   no,   no,   no,   no,   no,   no,   many, many,   output,            tx_out_gateway                     );
      //signatures
    DEFINE_TERMS(   many, many, many, many, many, many, many, many,   signtr,            NLSAG_sig                          );
    DEFINE_TERMS(   no,   no,   no,   no,   many, many, many, many,   signtr,            void_sig                           );
    DEFINE_TERMS(   no,   no,   no,   no,   many, many, many, many,   signtr,            ZC_sig                             );
    DEFINE_TERMS(   no,   no,   no,   no,   many, many, many, many,   signtr,            zarcanum_sig                       );
    DEFINE_TERMS(   no,   no,   no,   no,   no  , no,   many, many,   signtr,            gateway_sig                        );
      //proofs
    DEFINE_TERMS(   no,   no,   no,   no,   many, many, many, many,   proofs,            zc_asset_surjection_proof          );
    DEFINE_TERMS(   no,   no,   no,   no,   many, many, many, many,   proofs,            zc_outs_range_proof                );
    DEFINE_TERMS(   no,   no,   no,   no,   one,  one,  one,  one,    proofs,            zc_balance_proof                   );
    DEFINE_TERMS(   no,   no,   no,   no,   no,   no,   one,  one,    proofs,            zc_gw_balance_proof                );
    DEFINE_TERMS(   no,   no,   no,   no,   one,  one,  one,  one,    proofs,            asset_operation_proof              );
    DEFINE_TERMS(   no,   no,   no,   no,   one,  one,  one,  one,    proofs,            asset_operation_ownership_proof    );
    DEFINE_TERMS(   no,   no,   no,   no,   one,  one,  one,  one,    proofs,            asset_operation_ownership_proof_eth);
    DEFINE_TERMS(   no,   no,   no,   no,   no,   no,   one,  one,    proofs,            gateway_address_ownership_proof    );



    struct proxy_visitor : public boost::static_visitor<bool>
    {
      proxy_visitor(size_t current_hard_fork_id) :m_current_hard_fork_id(current_hard_fork_id)
      {
      }
      size_t m_current_hard_fork_id;
      uint8_t m_container_type;
      std::map<std::type_index, size_t> m_type_occurence_count;

      template<typename T>
      bool operator()(const T& in) 
      {
        using t_traits = type_hf_terms_traits<T>;

        //check container type
        if (!(m_container_type & t_traits::container_mask))
        {
          LOG_ERROR("Transaction container '" << get_container_name(m_container_type) << "' contains type " << typeid(T).name() << ", while allowed container(s) for this type: '" << get_container_mask_name(t_traits::container_mask) << "', HF: " << m_current_hard_fork_id);
          return false; //not allowed in this container
        }

        //check if type is allowed in this container during this hardfork
        size_t type_count = ++m_type_occurence_count[std::type_index(typeid(T))];

        CHECK_AND_ASSERT_THROW_MES(std::size(t_traits::hf) > m_current_hard_fork_id, "unexpected: m_current_hard_fork_id is bigger than hf array size");
        switch (t_traits::hf[m_current_hard_fork_id])
        {
        case no:
          LOG_ERROR("Transaction container '" << get_container_name(m_container_type) << "' contains type '" << typeid(T).name() << "', which is not allowed at hardfork " << m_current_hard_fork_id);
          return false;
        case one:
          if (type_count > 1)
          {
            LOG_ERROR("Transaction container '" << get_container_name(m_container_type) << "' contains multiple entries of type '" << typeid(T).name() << "', which is not allowed at hardfork " << m_current_hard_fork_id);
            return false;
          }
          else
            return true;
        case many:
          return true;
        default:
          LOG_ERROR("Transaction container '" << get_container_name(m_container_type) << "' contains type '" << typeid(T).name() << "', which has unknown allowance for hardfork " << m_current_hard_fork_id);
          return false; //unknown type on terms map
        }

        LOG_ERROR("Transaction container '" << get_container_name(m_container_type) << "' contains type '" << typeid(T).name() << "' at hf " << m_current_hard_fork_id << ", unknown error"); // should never get there
        return false;
      }
    };
  } // namespace hardfork_specific_terms


  inline bool validate_tx_for_hardfork_specific_terms_types_HF6(const transaction& tx, const crypto::hash& tx_id, size_t current_hard_fork_id)
  {

    // go over every container in tx and check if its type is allowed at current hardfork version

    hardfork_specific_terms::proxy_visitor vstr(current_hard_fork_id);

    vstr.m_container_type = hardfork_specific_terms::extra;
    for (const auto& el : tx.extra) 
    {    
      if (!boost::apply_visitor(vstr, el))
      {
        return false;
      }
    }

    vstr.m_container_type = hardfork_specific_terms::attach;
    for (const auto& el : tx.attachment)
    {
      if (!boost::apply_visitor(vstr, el))
      {
        return false;
      }
    }

    vstr.m_container_type = hardfork_specific_terms::input;
    for (const auto& el : tx.vin)
    {
      if (!boost::apply_visitor(vstr, el))
      {
        return false;
      }
    }

    vstr.m_container_type = hardfork_specific_terms::output;
    for (const auto& el : tx.vout)
    {
      if (!boost::apply_visitor(vstr, el))
      {
        return false;
      }
    }


    vstr.m_container_type = hardfork_specific_terms::proofs;
    for (const auto& el : tx.proofs)
    {
      if (!boost::apply_visitor(vstr, el))
      {
        return false;
      }
    }

    vstr.m_container_type = hardfork_specific_terms::signtr;
    for (const auto& el : tx.signatures)
    {
      if (!boost::apply_visitor(vstr, el))
      {
        return false;
      }
    }

    return true;
  }

} // namespace currency
