#pragma once
#include <boost/asio.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <cstdint>
#include <vector>
#include <string>

#include "net/http_client.h"

namespace epee { namespace net_utils { namespace http {

class http_socks5_client : public http_universal_client
{
public:
  http_socks5_client(std::string proxy_host, uint16_t proxy_port)
    : m_proxy_host(std::move(proxy_host)), m_proxy_port(proxy_port) {}

  bool connect(const std::string& host, int port, unsigned int timeout) override
  {
    return connect_impl(host, std::to_string(port), timeout);
  }

  bool connect(const std::string& host, std::string port) override
  {
    return connect_impl(host, port, m_default_timeout);
  }

  bool connect(const std::string& host, const std::string& port, unsigned int timeout) override
  {
    return connect_impl(host, port, timeout);
  }

  void set_host_name(const std::string& name) override
  {
    http_universal_client::set_host_name(name);
  }

private:
  static bool socks5_handshake(boost::asio::ip::tcp::socket& sock, const std::string& dest_host, uint16_t dest_port)
  {
    using boost::asio::buffer;
    using boost::asio::read;
    using boost::asio::write;

    boost::system::error_code ec;

    // 1) greeting: ver=5, nmethods=1, methods={0x00 no-auth}
    {
      const uint8_t greet[3] = {0x05, 0x01, 0x00};
      write(sock, buffer(greet, 3), ec);
      if (ec)
        return false;

      uint8_t resp[2] = {0,0};
      read(sock, buffer(resp, 2), ec);
      if (ec)
        return false;
      if (resp[0] != 0x05 || resp[1] != 0x00)
      {
        // сервер не согласился на no-auth
        return false;
      }
    }

    // 2) CONNECT request: ver=5, cmd=1, rsv=0, atyp=3 (domain),
    //                     dst.addr=LEN + host bytes, dst.port=2 bytes (BE)
    {
      std::vector<uint8_t> req;
      req.reserve(4 + 1 + dest_host.size() + 2);
      req.push_back(0x05); // ver
      req.push_back(0x01); // cmd = CONNECT
      req.push_back(0x00); // rsv
      req.push_back(0x03); // atyp = domain
      if (dest_host.size() > 255)
        return false;
      req.push_back(static_cast<uint8_t>(dest_host.size()));
      req.insert(req.end(), dest_host.begin(), dest_host.end());
      req.push_back(static_cast<uint8_t>((dest_port >> 8) & 0xFF));
      req.push_back(static_cast<uint8_t>(dest_port & 0xFF));

      write(sock, buffer(req.data(), req.size()), ec);
      if (ec)
        return false;

      // response: ver, rep, rsv, atyp, bnd.addr, bnd.port
      // read 4 bytes.
      uint8_t head[4] = {0,0,0,0};
      read(sock, buffer(head, 4), ec);
      if (ec)
        return false;
      if (head[0] != 0x05)
        return false;
      if (head[1] != 0x00)
        return false; // rep != 0 => ошибка

      // read the bound address (dummy, just consume bytes by type) 
      size_t to_read = 0;
      if (head[3] == 0x01) // IPv4
        to_read = 4;
      else if (head[3] == 0x03) // Domain
      {
        uint8_t len = 0;
        read(sock, buffer(&len, 1), ec);
        if (ec)
          return false;
        to_read = static_cast<size_t>(len);
      }
      else if (head[3] == 0x04) // IPv6
        to_read = 16;
      else
        return false;

      if (to_read)
      {
        std::vector<uint8_t> sink(to_read);
        read(sock, buffer(sink.data(), sink.size()), ec);
        if (ec) return false;
      }

      // bnd.port (2 bytes)
      uint8_t portb[2];
      read(sock, buffer(portb, 2), ec);
      if (ec)
        return false;
    }

    return true;
  }

  bool connect_impl(const std::string& dest_host, const std::string& dest_port_s, unsigned int timeout)
  {
    // 1) for correct HTTP Host:
    set_host_name(dest_host);

    // 2) TCP to SOCKS5-proxy
    if (!http_universal_client::connect(m_proxy_host, std::to_string(m_proxy_port), timeout ? timeout : m_default_timeout))
      return false;

    // 3) SOCKS5 CONNECT to the target host
    uint16_t dport = 0;
    try
    {
      dport = static_cast<uint16_t>(std::stoul(dest_port_s));
    }
    catch (...)
    {
      return false;
    }

    auto& sock = get_socket();
    if (!socks5_handshake(sock, dest_host, dport))
      return false;

    return true;
  }

private:
  std::string m_proxy_host;
  uint16_t    m_proxy_port;
  unsigned int m_default_timeout = 15000;
};

}}} // namespace epee::net_utils::http