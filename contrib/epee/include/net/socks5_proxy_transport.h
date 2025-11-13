#pragma once
#include <boost/asio.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

namespace tools {
namespace socks5 {

class socks5_proxy_transport
{
public:
  socks5_proxy_transport()
  : io_(),
    socket_(io_),
    proxy_host_("127.0.0.1"),
    proxy_port_(9050),
    use_remote_dns_(true),
    connection_timeout_ms_(0),
    recv_timeout_ms_(0)
  {}

  // proxy config
  void set_socks_proxy(const std::string& host, uint16_t port)
  {
    proxy_host_ = host;
    proxy_port_ = port;
  }

  void set_use_remote_dns(bool v) { use_remote_dns_ = v; }

  // connect(hostname, port, conn_tmo, recv_tmo, bind_ip)
  bool connect(const std::string& dest_host, int dest_port, unsigned int conn_timeout_ms, unsigned int recv_timeout_ms, const std::string& bind_ip = "0.0.0.0")
  {
    connection_timeout_ms_ = conn_timeout_ms;
    recv_timeout_ms_       = recv_timeout_ms;
    return connect(dest_host, dest_port, conn_timeout_ms, bind_ip);
  }

  // connect(ip, port, conn_tmo, recv_tmo, bind_ip)
  bool connect(unsigned long ip_be, int dest_port, unsigned int conn_timeout_ms, unsigned int recv_timeout_ms, const std::string& bind_ip = "0.0.0.0")
  {
    connection_timeout_ms_ = conn_timeout_ms;
    recv_timeout_ms_       = recv_timeout_ms;

    // convert u_long (big-endian) to string
    boost::asio::ip::address_v4::bytes_type b{};
    b[0] = static_cast<unsigned char>( ip_be        & 0xFF);
    b[1] = static_cast<unsigned char>((ip_be >> 8)  & 0xFF);
    b[2] = static_cast<unsigned char>((ip_be >> 16) & 0xFF);
    b[3] = static_cast<unsigned char>((ip_be >> 24) & 0xFF);
    const auto addr = boost::asio::ip::address_v4(b).to_string();
    return connect(addr, dest_port, conn_timeout_ms, bind_ip);
  }

  // прежний 4-арг. вариант, на него завязаны наши тесты
  bool connect(const std::string& dest_host, int dest_port, unsigned int timeout_ms, const std::string& bind_ip = "0.0.0.0")
  {
    connection_timeout_ms_ = timeout_ms;
    recv_timeout_ms_       = timeout_ms;

    boost::system::error_code ec;

    // 1) close previous connection
    if (socket_.is_open())
    {
      boost::system::error_code ignore_ec; socket_.close(ignore_ec);
    }

    // 2) (optional) bind to local address
    if (bind_ip != "0.0.0.0" && !bind_ip.empty())
    {
      boost::asio::ip::tcp::endpoint ep_local(boost::asio::ip::make_address(bind_ip, ec), 0);
      if (!ec) socket_.open(ep_local.protocol(), ec);
      if (!ec) socket_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
      if (!ec) socket_.bind(ep_local, ec);

      if (ec)
        return false;
    }
    else
    {
      socket_.open(boost::asio::ip::tcp::v4(), ec);
      if (ec)
        return false;
    }

    // 3) connect к SOCKS5-proxy
    {
      boost::asio::ip::tcp::resolver resolver(io_);
      auto results = resolver.resolve(proxy_host_, std::to_string(proxy_port_), ec);
      if (ec) return false;

      // just sync connect to the first valid endpoint
      boost::asio::connect(socket_, results, ec);
      if (ec) return false;
    }

    // 4) SOCKS5 greeting: [0x05, 0x01, 0x00] -> [0x05, 0x00]
    {
      const unsigned char greet[] = {0x05, 0x01, 0x00};
      if (!send(&greet[0], sizeof(greet)))
        return false;

      unsigned char resp[2] = {0,0};
      if (!recv_n(resp, sizeof(resp)))
        return false;

      if (!(resp[0] == 0x05 && resp[1] == 0x00))
      {
        // unsupported auth
        return false;
      }
    }

    // 5) SOCKS5 CONNECT dest_host:dest_port
    {
      std::vector<unsigned char> req;
      req.reserve(4 + 1 + dest_host.size() + 2);

      req.push_back(0x05); // VER
      req.push_back(0x01); // CMD = CONNECT
      req.push_back(0x00); // RSV

      // адрес
      boost::system::error_code ignored;
      boost::asio::ip::address ip_parsed;
      bool addr_is_ip = !use_remote_dns_ && !((ip_parsed = boost::asio::ip::make_address(dest_host, ignored)).is_unspecified());

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

      // port (network byte order)
      const uint16_t p = static_cast<uint16_t>(dest_port);
      req.push_back(static_cast<unsigned char>((p >> 8) & 0xFF));
      req.push_back(static_cast<unsigned char>( p & 0xFF));

      if (!send(req.data(), req.size()))
        return false;

      // resp: VER REP RSV ATYP [BND.ADDR] [BND.PORT]
      unsigned char head[4] = {0,0,0,0};
      if (!recv_n(head, sizeof(head)))
        return false;
      if (head[0] != 0x05)
        return false;
      if (head[1] != 0x00)
        return false; // 0x00 = succeeded

      size_t addr_len = 0;
      if (head[3] == 0x01)
        addr_len = 4;
      else if (head[3] == 0x04)
        addr_len = 16;
      else if (head[3] == 0x03)
      {
        unsigned char len = 0;
        if (!recv_n(&len, 1))
          return false;
        addr_len = len;
      }
      else
      {
        return false;
      }

      // read remaining BND.ADDR + BND.PORT (2 bytes)
      std::vector<unsigned char> drain(addr_len + 2);
      if (!recv_n(drain.data(), drain.size()))
        return false;
    }

    return true;
  }

  bool connect(unsigned long ip_be, int dest_port, unsigned int timeout_ms, const std::string& bind_ip = "0.0.0.0")
  {
    connection_timeout_ms_ = timeout_ms;
    recv_timeout_ms_ = timeout_ms;

    boost::asio::ip::address_v4::bytes_type b{};
    b[0] = static_cast<unsigned char>( ip_be        & 0xFF);
    b[1] = static_cast<unsigned char>((ip_be >> 8)  & 0xFF);
    b[2] = static_cast<unsigned char>((ip_be >> 16) & 0xFF);
    b[3] = static_cast<unsigned char>((ip_be >> 24) & 0xFF);
    const auto addr = boost::asio::ip::address_v4(b).to_string();
    return connect(addr, dest_port, timeout_ms, bind_ip);
  }

  bool is_connected() const { return socket_.is_open(); }

  bool disconnect()
  {
    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
    return !ec;
  }

  bool send(const void* data, size_t cb)
  {
    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(data, cb), ec);
    return !ec;
  }

  bool send(const std::string& s)
  {
    return send(s.data(), s.size());
  }

  // read "as much as available" (not necessarily needed for levin, but useful)
  bool recv(std::string& out)
  {
    out.clear();
    boost::system::error_code ec;
    std::size_t avail = socket_.available(ec);
    if (ec)
      return false;
    if (avail == 0)
    {
      // read at least 1 byte to avoid returning empty
      char ch = 0;
      std::size_t n = socket_.read_some(boost::asio::buffer(&ch, 1), ec);
      if (ec || n == 0)
        return false;
      out.push_back(ch);
      return true;
    }
    out.resize(avail);
    std::size_t n = socket_.read_some(boost::asio::buffer(&out[0], out.size()), ec);
    if (ec)
      return false;
    out.resize(n);
    return true;
  }

  // exactly cb bytes
  bool recv_n(void* data, size_t cb)
  {
    boost::system::error_code ec;
    boost::asio::read(socket_, boost::asio::buffer(data, cb), ec);
    return !ec;
  }

  // override for levin_client.inl: resize + read_exact
  bool recv_n(std::string& buf, size_t cb)
  {
    buf.resize(cb);
    return recv_n(&buf[0], cb);
  }

  boost::asio::ip::tcp::socket& get_socket() { return socket_; }

private:
  boost::asio::io_service io_;
  boost::asio::ip::tcp::socket socket_;

  std::string proxy_host_;
  uint16_t proxy_port_;
  bool use_remote_dns_;

  unsigned int connection_timeout_ms_;
  unsigned int recv_timeout_ms_;
};

} // namespace socks5
} // namespace tools
