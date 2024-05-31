// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define VARIANT_SWITCH_BEGIN(v_type_obj) {auto & local_reference_eokcmeokmeokcm ATTRIBUTE_UNUSED = v_type_obj; if(false) {;
#define VARIANT_CASE_CONST(v_type, typed_name) } else if(local_reference_eokcmeokmeokcm.type() == typeid(v_type)) {  const v_type& typed_name ATTRIBUTE_UNUSED = boost::get<v_type>(local_reference_eokcmeokmeokcm);
#define VARIANT_CASE(v_type, typed_name) } else if(local_reference_eokcmeokmeokcm.type() == typeid(v_type)) {  v_type& typed_name ATTRIBUTE_UNUSED = boost::get<v_type>(local_reference_eokcmeokmeokcm);
#define VARIANT_CASE_TV(v_type) VARIANT_CASE(v_type, tv) 
#define VARIANT_CASE_OTHER() } else { 
#define VARIANT_CASE_THROW_ON_OTHER() } else { ASSERT_MES_AND_THROW("Unknown type in switch statement: " << local_reference_eokcmeokmeokcm.type().name());
#define VARIANT_CASE_THROW_ON_OTHER_MSG(err_msg) } else { ASSERT_MES_AND_THROW(err_msg << local_reference_eokcmeokmeokcm.type().name());

#define VARIANT_SWITCH_END() } }

#define VARIANT_OBJ_TYPENAME local_reference_eokcmeokmeokcm.type().name()

/*

usage: 


      VARIANT_SWITCH_BEGIN(o);
      VARIANT_CASE(tx_out_bare, o);
        
      VARIANT_CASE_TV(tx_out_zarcanum);
        //@#@      
      VARIANT_SWITCH_END();

      VARIANT_SWITCH_BEGIN(o);
      VARIANT_CASE_CONST(txout_to_key, o);
      VARIANT_CASE_CONST(txout_multisig, ms);
      VARIANT_CASE_CONST(txout_htlc, htlc);
      VARIANT_CASE_THROW_ON_OTHER();
      VARIANT_SWITCH_END();

 

      VARIANT_SWITCH_BEGIN(s);
      VARIANT_CASE(void_sig, v);
      VARIANT_CASE(NLSAG_sig, signatures);
      VARIANT_CASE(zarcanum_sig, s);
      //@#@
      VARIANT_CASE_THROW_ON_OTHER();
      VARIANT_SWITCH_END();


      VARIANT_SWITCH_BEGIN(o);
      VARIANT_CASE(tx_out_bare, o)
        
      VARIANT_CASE_TV(tx_out_zarcanum)
        //@#@
      VARIANT_CASE_THROW_ON_OTHER();        
      VARIANT_SWITCH_END();


*/

