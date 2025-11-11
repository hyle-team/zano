#pragma once
#include "http_client.h"         // epee::net_utils::http::http_universal_client & friends
#include "epee_socks5_handshake.h" // connect_through_socks5()

inline bool connect_through_socks5(boost::asio::ip::tcp::socket& sock, const std::string& dst_host, uint16_t dst_port, boost::system::error_code& ec)
{
  return true;
}

struct i_tcp_connector
{
  virtual ~i_tcp_connector() = default;

  // establish a direct-like channel to dst_host:dst_port in sock.
  // tcp_connect(host, port, timeout) must open tcp (used to connect to proxy or to dst, depending on impl)
  virtual bool establish(boost::asio::ip::tcp::socket& sock, const std::function<bool(const std::string&, int, unsigned int)>& tcp_connect, const std::string& dst_host, uint16_t dst_port, unsigned int timeout_ms) = 0;
};

// direct TCP (no proxy)
class direct_connector final : public i_tcp_connector
{
public:
  explicit direct_connector(std::string bind_if = {}) : _bind_if(std::move(bind_if)) {}

  bool establish(boost::asio::ip::tcp::socket& /*sock*/, const std::function<bool(const std::string&, int, unsigned int)>& tcp_connect, const std::string& dst_host,
    uint16_t dst_port, unsigned int timeout_ms) override
  {
    // just connect to the destination directly via provided tcp_connect
    (void)_bind_if;
    return tcp_connect(dst_host, static_cast<int>(dst_port), timeout_ms);
  }

private:
  std::string _bind_if; // reserved for future "bind to interface"
};

// no auth connector
class socks5_connector final : public i_tcp_connector
{
public:
  socks5_connector(std::string proxy_host, uint16_t proxy_port)
    : _proxy_host(std::move(proxy_host)), _proxy_port(proxy_port) {}

  bool establish(boost::asio::ip::tcp::socket& sock, const std::function<bool(const std::string&, int, unsigned int)>& tcp_connect, const std::string& dst_host,
    uint16_t dst_port, unsigned int timeout_ms) override
  {
    (void)timeout_ms; // we need stuff for block IO?

    // 1 TCP to proxy
    if (!tcp_connect(_proxy_host, static_cast<int>(_proxy_port), timeout_ms))
      return false;

    // 2 SOCKS5  connect dst_host:dst_port
    boost::system::error_code ec;
    if (!epee::net_utils::socks5::connect_through_socks5(sock, dst_host, dst_port, ec))
      return false;

    return true;
  }

private:
  std::string _proxy_host;
  uint16_t    _proxy_port;
};

// adapter client: http_universal_client + injected connector
template<class base_client = http_universal_client>
class http_client_with_connector : public base_client
{
public:
  explicit http_client_with_connector(std::unique_ptr<i_tcp_connector> connector)
    : _connector(std::move(connector)) {}

  // main entry used by invoke_request(u_c, tr,...)
  bool connect(const std::string& host, int port, unsigned int timeout) override
  {
    this->set_is_ssl(false);

    auto& sock = this->get_socket();

    auto tcp_connect = [&](const std::string& h, int p, unsigned int t) -> bool
    {
      return base_client::connect(h, p, t);
    };

    if (!_connector->establish(sock, tcp_connect, host, static_cast<uint16_t>(port), timeout))
      return false;

    // make sure http host: header matches destination (not proxy)
    this->set_host_name(host);
    return true;
  }

  using base_client::connect;

private:
  std::unique_ptr<i_tcp_connector> _connector;
};