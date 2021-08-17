// Copyright (c) 2018-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <sstream>
#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto/RIPEMD160_helper.h"

bool parse_hash256(const std::string str_hash, crypto::hash& hash);

template <class T>
std::ostream &print_t(std::ostream &o, const T &v)
{
  return o << "" << epee::string_tools::pod_to_hex(v) << "";
}


template <class T>
std::ostream &print16(std::ostream &o, const T &v)
{
  return o << "" << epee::string_tools::pod_to_hex(v).substr(0, 5) << "..";
}

template <class T>
std::string print16(const T &v)
{
  return std::string("") + epee::string_tools::pod_to_hex(v).substr(0, 5) + "..";
}


namespace crypto
{
  inline std::ostream &operator <<(std::ostream &o, const crypto::public_key &v)      { return print_t(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::secret_key &v)      { return print_t(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::key_derivation &v)  { return print_t(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::key_image &v)       { return print_t(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::signature &v)       { return print_t(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::hash &v)            { return print_t(o, v); }
  inline std::ostream &operator <<(std::ostream &o, const crypto::hash160 &v)         { return print_t(o, v); }
} // namespace crypto
