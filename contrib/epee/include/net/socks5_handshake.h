#pragma once
#include <string>
#include <vector>
#include <boost/asio.hpp>

namespace epee {
namespace net_utils {
namespace socks5 {
inline bool connect_through_socks5(boost::asio::ip::tcp::socket& sock, const std::string& proxy_host,
  uint16_t proxy_port, const std::string& dest_host, uint16_t dest_port, unsigned int /*timeout_ms*/, bool use_remote_dns = true)
{
  boost::system::error_code ec;

  // already connected to proxy_host:proxy_port? if not — connect
  if (!sock.is_open())
  {
    boost::asio::ip::tcp::resolver r(sock.get_executor());
    auto eps = r.resolve(proxy_host, std::to_string(proxy_port), ec);
    if (ec)
      return false;
    boost::asio::connect(sock, eps, ec);
    if (ec)
      return false;
  }

  // greeting: VER=5, NMETHODS=1, METHOD=0x00
  const unsigned char greeting[3] = {0x05, 0x01, 0x00};
  if (boost::asio::write(sock, boost::asio::buffer(greeting, 3), ec) != 3 || ec)
    return false;

  unsigned char method_reply[2] = {0,0};
  if (boost::asio::read(sock, boost::asio::buffer(method_reply, 2), ec) != 2 || ec)
    return false;
  if (method_reply[0] != 0x05 || method_reply[1] != 0x00)
    return false;

  // request
  std::vector<unsigned char> req;
  req.reserve(4 + 1 + dest_host.size() + 2);
  req.push_back(0x05); // VER
  req.push_back(0x01); // CMD=CONNECT
  req.push_back(0x00); // RSV

  boost::asio::ip::address addr;
  boost::asio::ip::address tmp;
  if (!use_remote_dns && !dest_host.empty() && !ec)
  {
    tmp = boost::asio::ip::make_address(dest_host, ec);
  }
  if (!use_remote_dns && !ec)
  {
    addr = tmp;
    if (addr.is_v4())
    {
      req.push_back(0x01);
      auto b = addr.to_v4().to_bytes();
      req.insert(req.end(), b.begin(), b.end());
    }
    else
    {
      req.push_back(0x04);
      auto b = addr.to_v6().to_bytes();
      req.insert(req.end(), b.begin(), b.end());
    }
  }
  else
  {
    if (dest_host.size() > 255)
      return false;
    req.push_back(0x03); // domain name
    req.push_back(static_cast<unsigned char>(dest_host.size()));
    req.insert(req.end(), dest_host.begin(), dest_host.end());
  }

  req.push_back(static_cast<unsigned char>((dest_port >> 8) & 0xFF));
  req.push_back(static_cast<unsigned char>(dest_port & 0xFF));

  if (boost::asio::write(sock, boost::asio::buffer(req.data(), req.size()), ec) != req.size() || ec)
    return false;

  // reply: VER, REP, RSV, ATYP, BND.ADDR..., BND.PORT
  unsigned char head[4] = {0};
  if (boost::asio::read(sock, boost::asio::buffer(head, 4), ec) != 4 || ec)
    return false;
  if (head[0] != 0x05 || head[1] != 0x00)
    return false;

  size_t addr_len = 0;
  if (head[3] == 0x01)
    addr_len = 4; // IPv4
  else if (head[3] == 0x04)
    addr_len = 16; // IPv6
  else if (head[3] == 0x03)
  {
    unsigned char len = 0;
    if (boost::asio::read(sock, boost::asio::buffer(&len, 1), ec) != 1 || ec)
      return false;
    addr_len = len;
  }
  else
  {
    return false;
  }

  std::vector<unsigned char> skip(addr_len + 2);
  if (boost::asio::read(sock, boost::asio::buffer(skip.data(), skip.size()), ec) != skip.size() || ec)
    return false;

  return true;
}

} // socks5
} // net_utils
} // epee
