// Copyright (c) 2014-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "encryption_filter.h"
#include "crypto/chacha8_stream.h"

encrypt_chacha_filter::encrypt_chacha_filter(std::string const &pass, const crypto::chacha8_iv& iv):m_iv(iv), m_ctx(AUTO_VAL_INIT(m_ctx))
{
  crypto::generate_chacha8_key(pass, m_key);
  ECRYPT_keysetup(&m_ctx, &m_key.data[0], sizeof(m_key.data) * 8, sizeof(m_iv.data)*8);
  ECRYPT_ivsetup(&m_ctx, &m_iv.data[0]);
}

//bool encrypt_chacha_filter::flush() const



encrypt_chacha_filter::~encrypt_chacha_filter()
{

}
