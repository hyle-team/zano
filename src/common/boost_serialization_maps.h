// Copyright (c) 2014-2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define BEGIN_BOOST_SERIALIZATION()     template <class t_archive> inline void serialize(t_archive &_arch, const unsigned int ver) {

#define BOOST_SERIALIZE(x)    _arch & x;

#define END_BOOST_SERIALIZATION()       }


/*
  example of use: 

  struct tx_extra_info 
  {
    crypto::public_key m_tx_pub_key;
    extra_alias_entry m_alias;
    std::string m_user_data_blob;
    extra_attachment_info m_attachment_info;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(m_tx_pub_key)
      BOOST_SERIALIZE(m_alias)
      if(ver < xxx) return;
      BOOST_SERIALIZE(m_user_data_blob)
      BOOST_SERIALIZE(m_attachment_info)
    END_BOOST_SERIALIZATION()
  };
*/
