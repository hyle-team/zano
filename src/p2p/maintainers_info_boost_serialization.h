// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "p2p_protocol_defs.h"
#include "common/crypto_serialization.h"

namespace boost
{
  namespace serialization
  {
    template <class Archive, class ver_type>
    inline void serialize(Archive &a,  nodetool::alert_condition& ac, const ver_type ver)
    {
      a & ac.alert_mode;
      a & ac.if_build_less_then;
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a,  nodetool::maintainers_info& mi, const ver_type ver)
    {
      a & mi.timestamp;
      a & mi.ver_major;
      a & mi.ver_minor;
      a & mi.ver_revision;
      a & mi.build_no;
      a & mi.conditions;
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a,  nodetool::maintainers_entry& me, const ver_type ver)
    {
      a & me.maintainers_info_buff;
      a & me.sign;
    }    
  }
}
