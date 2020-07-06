// Copyright (c) 2014-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "encryption_sink.h"
#include "crypto/chacha8_stream.h"

encrypt_chacha_sink::encrypt_chacha_sink(std::ostream &underlying_stream, std::string const &pass, const crypto::chacha8_iv& iv):m_iv(iv), m_underlying_stream(underlying_stream), m_ctx(AUTO_VAL_INIT(m_ctx))
{
  crypto::generate_chacha8_key(pass, m_key);
  ECRYPT_keysetup(&m_ctx, &m_key.data[0], sizeof(m_key.data) * 8, sizeof(m_iv.data)*8);
  ECRYPT_ivsetup(&m_ctx, &m_iv.data[0]);
}

std::streamsize encrypt_chacha_sink::write(char_type const * const buf, std::streamsize const n) const
{
  if (n == 0)
    return n;
  if (n%ECRYPT_BLOCKLENGTH == 0 && m_buff.empty())
  {
    std::vector<char_type> buff(n);
    ECRYPT_encrypt_blocks(&m_ctx, (u8*)buf, (u8*)&buff[0], n / ECRYPT_BLOCKLENGTH);
    m_underlying_stream.write(&buff[0], n);
  }else    
  { 
    m_buff.append(buf, n);
    size_t encr_count = m_buff.size() - m_buff.size() % ECRYPT_BLOCKLENGTH;
    if (!encr_count)
      return n;
    std::vector<char_type> buff(encr_count);
    ECRYPT_encrypt_blocks(&m_ctx, (u8*)m_buff.data(), (u8*)&buff[0], m_buff.size() / ECRYPT_BLOCKLENGTH);
    m_underlying_stream.write(&buff[0], encr_count);
    m_buff.erase(0, encr_count);
  }
  return n;
}


void encrypt_chacha_sink::flush() const
{
  if (m_buff.empty())
    return;

  std::vector<char_type> buff(m_buff.size());
  ECRYPT_encrypt_bytes(&m_ctx, (u8*)m_buff.data(), (u8*)&buff[0], m_buff.size());
  m_underlying_stream.write(&buff[0], m_buff.size());
}

encrypt_chacha_sink::~encrypt_chacha_sink()
{

}
