// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "eddsa_signature.h"
#include "crypto.h"
#include "crypto-sugar.h"
#include "sha512.h"
#include "random.h"
#include "misc_language.h"
#include <string_tools.h>

namespace crypto
{
  bool eddsa_seed_to_secret_key_public_key_and_prefix(const eddsa_seed& seed, eddsa_secret_key& sec_key, eddsa_public_key& pub_key, eddsa_sec_prefix& prefix) noexcept
  {
    static_assert(sizeof seed == 32);

    try
    {
      hash64 h64{};
      if (!sha512(seed.data, 32, h64))
        return false;

      scalar_t sk{};
      
      static_assert(sizeof sk == 32 && sizeof prefix == 32 && sizeof h64 == 64);
      memcpy(sk.data(), h64.data, 32);        // first 32 bytes
      memcpy(prefix.data, h64.data + 32, 32); // last 32 bytes
      
      // prune the expanded key per RFC 8032 
      sk.m_s[0]  &= 248;
      sk.m_s[31] &= 63;
      sk.m_s[31] |= 64;
      // do not reduce sk here additionally as per RFC 8032 -- sowle

      static_assert(sizeof sec_key == 32 && sizeof sk == 32);
      memcpy(&sec_key, sk.data(), 32);

      const public_key pk = (sk * c_point_G).to_public_key();

      static_assert(sizeof pub_key == 32 && sizeof pk == 32);
      memcpy(&pub_key, &pk, 32);

      return true;
    }
    catch(...)
    {
      return false;
    }
  }

  bool eddsa_generate_random_seed(eddsa_seed& seed) noexcept
  {
    try
    {
      generate_random_bytes(sizeof seed, &seed); // thread-safe version
      return true;
    }
    catch(...)
    {
      return false;
    }
  }

  bool generate_eddsa_signature(const std::string& m, const eddsa_sec_prefix& prefix, const eddsa_secret_key& sec_key, const eddsa_public_key& pub_key, eddsa_signature& sig) noexcept
  {
    try
    {
      std::string buff;
      buff.reserve(sizeof prefix + m.size());
      epst::append_pod_to_strbuff(prefix, buff);
      buff += m;

      hash64 h64{};
      if (!sha512(buff.data(), buff.size(), h64))  // sha512(prefix | m)
        return false;

      sc_reduce(h64.data);
      scalar_t r{};
      static_assert(sizeof r == 32);
      memcpy(r.data(), h64.data, 32);

      point_t R = r * c_point_G;
      public_key R_pk = R.to_public_key();

      const scalar_t& sk = *reinterpret_cast<const scalar_t*>(&sec_key);

#if !defined(NDEBUG)
      point_t p = sk * c_point_G;
      const public_key& pk = p.to_public_key();
      static_assert(sizeof pk == 32 && sizeof pub_key == 32);
      if (memcmp(pk.data, pub_key.data, 32) != 0)
        return false;
#endif

      buff.clear();
      buff.reserve(sizeof R_pk + sizeof pub_key + m.size());
      epst::append_pod_to_strbuff(R_pk, buff);
      epst::append_pod_to_strbuff(pub_key, buff);
      buff += m;

      if (!sha512(buff.data(), buff.size(), h64))  // sha512(R_pk | pub_key | m)
        return false;

      sc_reduce(h64.data);
      scalar_t h{};
      memcpy(h.data(), h64.data, 32);

      scalar_t s = c_scalar_0;
      s.assign_muladd(h, sk, r); // s = h * sk + r

      // sig = {R, s}
      static_assert(sizeof sig == 64 && sizeof R_pk == 32 && sizeof s == 32);
      memcpy(sig.data,      R_pk.data,  32);
      memcpy(sig.data + 32, s.data(),   32);

      return true;
    }
    catch(...)
    {
      return false;
    }
  }

  bool verify_eddsa_signature(const std::string& m, const eddsa_public_key& pub_key, const eddsa_signature& sig) noexcept
  {
    try
    {
      static_assert(sizeof(public_key) == sizeof pub_key && sizeof pub_key == 32);
      const public_key& pk = reinterpret_cast<const public_key&>(pub_key);

      point_t P = c_point_0;
      if (!P.from_public_key(pk))
        return false;

      // RFC8032 does not require a subgroup check on P and R_pk -- sowle

      public_key R_pk{};
      scalar_t s = c_scalar_0;
      static_assert(sizeof s == 32 && sizeof R_pk == 32 && sizeof sig == 64);
      memcpy(R_pk.data, sig.data,       32);
      memcpy(s.data(),  sig.data + 32,  32);

      // 5.1.7: "If any of the decodings fail (including S being out of range), the signature is invalid."
      // so we must check s and decode R_pk
      if (!s.is_reduced())
        return false;

      point_t R_p = c_point_0;
      if (!R_p.from_public_key(R_pk))
        return false;

      std::string buff;
      buff.reserve(sizeof R_pk + sizeof pk + m.size());
      epst::append_pod_to_strbuff(R_pk, buff);
      epst::append_pod_to_strbuff(pk, buff);
      buff += m;

      hash64 h64{};
      if (!sha512(buff.data(), buff.size(), h64))  // sha512(R_pk | pk | m)
        return false;

      sc_reduce(h64.data);
      scalar_t h{};
      static_assert(sizeof h == 32 && sizeof h64 == 64);
      memcpy(h.data(), h64.data, 32);

      // s     = r     + h * sk
      // s * G = r * G + h * sk * G
      // s * G = R     + h * P

      // here's my original approach, however I found out (IACR 2020/1244) that it is STRICTIER than Dalek and rejects case 3 which Dalek accepts
      // therefore I moved from using assign_mul_plus_G / ge_double_scalarmult_base_vartime_p3 to direct multiplication, and this approach gives me
      // only two differences with Dalek: case 1 reject (small A) and case 11 reject (non canonical pk), which is good -- sowle
      // (s.a. crypto_eddsa test)
      //
      // point_t R = c_point_0;
      // R.assign_mul_plus_G(c_scalar_0 - h, P, s);
      // if (R_p != R)

      if (R_p + h * P != s * c_point_G)
        return false;

      return true;
    }
    catch(...)
    {
      return false;
    }
  }

  bool generate_eddsa_signature(const hash& m, const eddsa_sec_prefix& prefix, const eddsa_secret_key& sec_key, const eddsa_public_key& pub_key, eddsa_signature& sig) noexcept
  {
    try
    {
      std::string buff;
      buff.reserve(sizeof m);
      epst::append_pod_to_strbuff(m, buff);
      return generate_eddsa_signature(buff, prefix, sec_key, pub_key, sig);
    }
    catch(...)
    {
      return false;
    }
  }

  bool verify_eddsa_signature(const hash& m, const eddsa_public_key& pub_key, const eddsa_signature& sig) noexcept
  {
    try
    {
      std::string buff;
      buff.reserve(sizeof m);
      epst::append_pod_to_strbuff(m, buff);
      return verify_eddsa_signature(buff, pub_key, sig);
    }
    catch(...)
    {
      return false;
    }
  }


  std::ostream& operator<<(std::ostream& o, const eddsa_secret_key& v)
  {
    return o << epee::string_tools::pod_to_hex(v);
  }

  std::ostream& operator<<(std::ostream& o, const eddsa_public_key& v)
  {
    return o << epee::string_tools::pod_to_hex(v);
  }
  std::ostream& operator<<(std::ostream& o, const eddsa_signature& v)
  {
    return o << epee::string_tools::pod_to_hex(v);
  }


} // namespace crypto
