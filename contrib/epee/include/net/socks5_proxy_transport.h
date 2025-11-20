#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <boost/system/error_code.hpp>
#include <boost/asio/ip/address.hpp>

namespace tools {
namespace socks5 {

template<class base_transport>
class socks5_proxy_transport : public base_transport
{
public:
  socks5_proxy_transport() = default;

  void set_socks_proxy(const std::string& host, uint16_t port)
  {
    m_proxy_host = host;
    m_proxy_port = port;
  }

  void set_use_remote_dns(bool v) { m_use_remote_dns = v; }

  bool connect(const std::string& dest_host, int dest_port, unsigned int connect_timeout_ms, unsigned int recv_timeout_ms,
    const std::string& bind_ip = "0.0.0.0")
  {
    // 1) TCP connect to SOCKS5 proxy through BASE transport
    if (!this->base_transport::connect(m_proxy_host, std::to_string(m_proxy_port), connect_timeout_ms, recv_timeout_ms, bind_ip))
      return false;

    // 2) SOCKS5 greeting: [0x05, 0x01, 0x00] -> [0x05, 0x00]
    {
      const unsigned char greet[] = {0x05, 0x01, 0x00};
      if (!this->base_transport::send(greet, sizeof(greet)))
        return false;

      std::string resp;
      if (!this->base_transport::recv_n(resp, 2))
        return false;
      if (resp.size() != 2)
        return false;

      const unsigned char ver = static_cast<unsigned char>(resp[0]);
      const unsigned char mth = static_cast<unsigned char>(resp[1]);
      if (!(ver == 0x05 && mth == 0x00)) // 0x00 — "no authentication"
        return false; // unsupported auth
    }

    // 3) SOCKS5 CONNECT to the target host:port through the proxy
    {
      std::vector<unsigned char> req;
      req.reserve(4 + 1 + dest_host.size() + 2);

      req.push_back(0x05); // VER
      req.push_back(0x01); // CMD = CONNECT
      req.push_back(0x00); // RSV

      // ATYP + DST.ADDR
      boost::system::error_code ec;
      boost::asio::ip::address ip_parsed;
      const bool addr_is_ip =
        !m_use_remote_dns &&
        !((ip_parsed = boost::asio::ip::make_address(dest_host, ec)).is_unspecified());

      if (addr_is_ip && ip_parsed.is_v4())
      {
        req.push_back(0x01); // ATYP = IPv4
        const auto bytes = ip_parsed.to_v4().to_bytes();
        req.insert(req.end(), bytes.begin(), bytes.end());
      }
      else if (addr_is_ip && ip_parsed.is_v6())
      {
        req.push_back(0x04); // ATYP = IPv6
        const auto bytes = ip_parsed.to_v6().to_bytes();
        req.insert(req.end(), bytes.begin(), bytes.end());
      }
      else
      {
        req.push_back(0x03); // ATYP = DOMAIN
        if (dest_host.size() > 255)
          return false;
        req.push_back(static_cast<unsigned char>(dest_host.size()));
        req.insert(req.end(), dest_host.begin(), dest_host.end());
      }

      // DST.PORT (network byte order)
      const uint16_t p = static_cast<uint16_t>(dest_port);
      req.push_back(static_cast<unsigned char>((p >> 8) & 0xFF));
      req.push_back(static_cast<unsigned char>( p       & 0xFF));

      if (!this->base_transport::send(req.data(), req.size()))
        return false;

      // resp: VER REP RSV ATYP [BND.ADDR] [BND.PORT]
      std::string head;
      if (!this->base_transport::recv_n(head, 4))
        return false;
      if (head.size() != 4)
        return false;

      const unsigned char r_ver  = static_cast<unsigned char>(head[0]);
      const unsigned char r_rep  = static_cast<unsigned char>(head[1]);
      const unsigned char r_atyp = static_cast<unsigned char>(head[3]);

      if (r_ver != 0x05)
        return false;
      if (r_rep != 0x00)
        return false; // success

      size_t addr_len = 0;
      if (r_atyp == 0x01)
        addr_len = 4;
      else if (r_atyp == 0x04)
        addr_len = 16;
      else if (r_atyp == 0x03)
      {
        std::string len_buf;
        if (!this->base_transport::recv_n(len_buf, 1))
          return false;
        if (len_buf.size() != 1)
          return false;
        addr_len = static_cast<unsigned char>(len_buf[0]);
      }
      else
      {
        return false;
      }

      // continue read BND.ADDR + BND.PORT (2 bytes)
      std::string drain;
      if (!this->base_transport::recv_n(drain, static_cast<int64_t>(addr_len + 2)))
        return false;
      if (drain.size() != addr_len + 2)
        return false;
    }

    // on this step, we have established a TUNNEL to dest_host:dest_port
    // TODO_SSL_HANDSHAKE: if HTTPS through SOCKS5 is ever needed,
    // the base transport should be able to perform SSL handshake AFTER CONNECT,
    // not immediately after TCP-connect to the proxy (see on_after_connect() in blocked_mode_client_t). :contentReference[oaicite:3]{index=3}
    return true;
  }

  bool connect(const std::string& dest_host, int dest_port, unsigned int timeout_ms, const std::string& bind_ip = "0.0.0.0")
  {
    return connect(dest_host, dest_port, timeout_ms, timeout_ms, bind_ip);
  }

  // u_long (BE) overload for compatibility
  bool connect(unsigned long ip_be, int dest_port, unsigned int connect_timeout_ms, unsigned int recv_timeout_ms,
    const std::string& bind_ip = "0.0.0.0")
  {
    unsigned char b0 = static_cast<unsigned char>( ip_be        & 0xFF);
    unsigned char b1 = static_cast<unsigned char>((ip_be >> 8)  & 0xFF);
    unsigned char b2 = static_cast<unsigned char>((ip_be >> 16) & 0xFF);
    unsigned char b3 = static_cast<unsigned char>((ip_be >> 24) & 0xFF);
    const std::string addr = std::to_string(b0) + "." + std::to_string(b1) + "." + std::to_string(b2) + "." + std::to_string(b3);
    return connect(addr, dest_port, connect_timeout_ms, recv_timeout_ms, bind_ip);
  }

  bool connect(unsigned long ip_be, int dest_port, unsigned int timeout_ms, const std::string& bind_ip = "0.0.0.0")
  {
    return connect(ip_be, dest_port, timeout_ms, timeout_ms, bind_ip);
  }

private:
  std::string m_proxy_host = "127.0.0.1";
  uint16_t m_proxy_port = 9050;
  bool m_use_remote_dns = true;
};

} // namespace socks5
} // namespace tools
