// Copyright (c) 2018-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma  once
#include <string>
#include "epee/include/misc_language.h"
#include "epee/include/storages/parserse_base_utils.h"
#include "epee/include/storages/portable_storage.h"
#include "ethereum/libethash/ethash/ethash.h"
#include "ethereum/libethash/ethash/keccak.h"
#include "currency_core/currency_format_utils.h"

namespace stratum
{
//------------------------------------------------------------------------------------------------------------------------------

  // Small helper for extracting separate JSON-RPC requests from input buffer.
  // TODO: currently it does not handle curly brackets within strings to make things simplier
  struct json_helper
  {
    void feed(const std::string& s)
    {
      feed(s.c_str(), s.size());
    }

    void feed(const char* str, size_t size)
    {
      m_buffer.append(str, size);

      int b_count = 0;
      for(size_t i = 0; i < m_buffer.size(); )
      {
        char c = m_buffer[i];
        if (c == '{')
          ++b_count;
        else if (c == '}')
        {
          if (--b_count == 0)
          {
            m_objects.push_back(m_buffer.substr(0, i + 1));
            m_buffer.erase(0, i + 1);
            i = 0;
            continue;
          }
        }
        ++i;
      }
    }

    bool has_objects() const
    {
      return !m_objects.empty();
    }

    bool pop_object(std::string &destination)
    {
      if (m_objects.empty())
        return false;

      destination = m_objects.front();
      m_objects.pop_front();
      return true;
    }

    std::string m_buffer;
    std::list<std::string> m_objects;
  };

  template<class t_value>
  bool ps_get_value_noexcept(epee::serialization::portable_storage& ps, const std::string& value_name, t_value& val, epee::serialization::portable_storage::hsection hparent_section)
  {
    try
    {
      return ps.get_value(value_name, val, hparent_section);
    }
    catch (...)
    {
      return false;
    }
  }

  std::string trim_0x(const std::string& s)
  {
    if (s.length() >= 2 && s[0] == '0' && s[1] == 'x')
      return s.substr(2);
    return s;
  }

  constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  constexpr char hexmap_backward[] =
  { //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 0
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 1
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 2
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   20,  20,  20,  20,  20,  20, // 3
      20,  10,  11,  12,  13,  14,  15,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 4
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 5
      20,  10,  11,  12,  13,  14,  15,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 6
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 7
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 8
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // 9
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // A
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // B
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // C
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // D
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20, // E
      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20  // F
  };

  template<class pod_t>
  std::string pod_to_net_format(const pod_t &h)
  {
    const char* data = reinterpret_cast<const char*>(&h);
    size_t len = sizeof h;

    std::string s(len * 2, ' ');
    for (size_t i = 0; i < len; ++i) {
      s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
      s[2 * i + 1] = hexmap[(data[i] & 0x0F)];
    }

    return "0x" + s;
  }

  template<class pod_t>
  std::string pod_to_net_format_reverse(const pod_t &h)
  {
    const char* data = reinterpret_cast<const char*>(&h);
    size_t len = sizeof h;

    std::string s(len * 2, ' ');
    for (size_t i = 0; i < len; ++i) {
      s[2 * i]     = hexmap[(data[len - i - 1] & 0xF0) >> 4]; // reverse bytes order in data
      s[2 * i + 1] = hexmap[(data[len - i - 1] & 0x0F)];
    }

    return "0x" + s;
  }
  template<class pod_t>
  bool pod_from_net_format(const std::string& str, pod_t& result, bool assume_following_zeroes = false)
  {
    std::string s = trim_0x(str);
    if (s.size() != sizeof(pod_t) * 2)
    {
      if (!assume_following_zeroes || s.size() > sizeof(pod_t) * 2)
        return false; // invalid string length
      s.insert(s.size() - 1, sizeof(pod_t) * 2 - s.size(), '0'); // add zeroes at the end
    }

    const unsigned char* hex_str = reinterpret_cast<const unsigned char*>(s.c_str());
    char* pod_data = reinterpret_cast<char*>(&result);

    for (size_t i = 0; i < sizeof(pod_t); ++i)
    {
      char a = hexmap_backward[hex_str[2 * i + 1]];
      char b = hexmap_backward[hex_str[2 * i + 0]];
      if (a > 15 || b > 15)
        return false; // invalid character
      pod_data[i] = a | (b << 4);
    }
    return true;
  }

  template<class pod_t>
  bool pod_from_net_format_reverse(const std::string& str, pod_t& result, bool assume_leading_zeroes = false)
  {
    std::string s = trim_0x(str);
    if (s.size() != sizeof(pod_t) * 2)
    {
      if (!assume_leading_zeroes || s.size() > sizeof(pod_t) * 2)
        return false; // invalid string length
      s.insert(0, sizeof(pod_t) * 2 - s.size(), '0'); // add zeroes at the beginning
    }

    const unsigned char* hex_str = reinterpret_cast<const unsigned char*>(s.c_str());
    char* pod_data = reinterpret_cast<char*>(&result);

    for (size_t i = 0; i < sizeof(pod_t); ++i)
    {
      char a = hexmap_backward[hex_str[2 * i + 1]];
      char b = hexmap_backward[hex_str[2 * i + 0]];
      if (a > 15 || b > 15)
        return false; // invalid character
      pod_data[sizeof(pod_t) - i - 1] = a | (b << 4); // reverse byte order
    }
    return true;
  }

  uint64_t epoch_by_seedhash(const ethash_hash256& seed_hash)
  {
    ethash_hash256 epoch_seed = {};
	  for (uint32_t i = 0; i < 2016; ++i) // 2016 epoches will be enough until 2038
    {
      if (memcmp(&seed_hash, &epoch_seed, sizeof seed_hash) == 0)
        return i;
      epoch_seed = ethash_keccak256_32(epoch_seed.bytes);
    }
	  return UINT64_MAX;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  
  // http://www.jsonrpc.org/specification  
  // 'id' -- an identifier established by the Client that MUST contain a String, Number, or NULL value if included.
  // The Server MUST reply with the same value in the Response object if included.
  struct jsonrpc_id_null_t {};
  typedef boost::variant<int64_t, std::string, jsonrpc_id_null_t> jsonrpc_id_t;
  
  std::string jsonrpc_id_to_value_str(const jsonrpc_id_t& id)
  {
    if (id.type() == typeid(int64_t))
      return boost::to_string(boost::get<int64_t>(id));
    if (id.type() == typeid(std::string))
      return '"' + boost::to_string(boost::get<std::string>(id)) + '"';
    return "null";
  }

  namespace details
  {
    struct jsonrpc_id_visitor : public boost::static_visitor<>
    {
      explicit jsonrpc_id_visitor(jsonrpc_id_t &value) : m_value(value) {}
      void operator()(const uint64_t id)      { m_value = id; }
      void operator()(const int64_t id)       { m_value = id; }
      void operator()(const std::string& id)  { m_value = id; }
      template<typename T>
      void operator()(const T&)               { /* nothing */ }

      jsonrpc_id_t &m_value;
    };
  }

  bool read_jsonrpc_id(epee::serialization::portable_storage& ps, jsonrpc_id_t& result)
  {
    epee::serialization::storage_entry se;
    if (!ps.get_value("id", se, nullptr))
      return false;

    details::jsonrpc_id_visitor vis(result);
    boost::apply_visitor(vis, se);
    return true;
  }

} // namespace stratum 

inline std::ostream &operator <<(std::ostream &o, const ethash_hash256 &v) { return print_t(o, v); }
