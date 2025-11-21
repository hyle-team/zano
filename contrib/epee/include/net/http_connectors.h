#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <functional>

#include "http_client.h"
#include "net/socks5_handshake.h"

namespace epee {
namespace net_utils {
namespace http {

struct i_tcp_connector
{
  virtual ~i_tcp_connector() = default;

  virtual bool establish(boost::asio::ip::tcp::socket& sock, const std::function<bool(const std::string&, int, unsigned int)>& tcp_connect,
    const std::string& dst_host, uint16_t dst_port, unsigned int timeout_ms) = 0;
};

// direct TCP (no proxy)
class direct_connector final : public i_tcp_connector
{
public:
  explicit direct_connector(std::string bind_if = {})
    : m_bind_if(std::move(bind_if)) {}

  bool establish(boost::asio::ip::tcp::socket& /*sock*/, const std::function<bool(const std::string&, int, unsigned int)>& tcp_connect,
    const std::string& dst_host, uint16_t dst_port, unsigned int timeout_ms) override
  {
    // just connect to the destination directly via provided tcp_connect
    (void)m_bind_if;
    return tcp_connect(dst_host, static_cast<int>(dst_port), timeout_ms);
  }

private:
  std::string m_bind_if; // reserved for future "bind to interface"
};

// SOCKS5 (NO-AUTH) connector
class socks5_connector final : public i_tcp_connector
{
public:
  socks5_connector(std::string proxy_host, uint16_t proxy_port)
    : m_proxy_host(std::move(proxy_host)), m_proxy_port(proxy_port) {}

  bool establish(boost::asio::ip::tcp::socket& sock, const std::function<bool(const std::string&, int, unsigned int)>& tcp_connect,
    const std::string& dst_host, uint16_t dst_port, unsigned int timeout_ms) override
  {
    (void)timeout_ms;

    // 1) TCP to proxy
    if (!tcp_connect(m_proxy_host, static_cast<int>(m_proxy_port), timeout_ms))
      return false;

    // 2) SOCKS5 CONNECT dst_host:dst_port
    boost::system::error_code ec;
    if (!epee::net_utils::socks5::connect_through_socks5(sock, dst_host, dst_port, ec))
      return false;

    return true;
  }

private:
  std::string m_proxy_host;
  uint16_t    m_proxy_port;
};

// adapter client: http_universal_client + injected connector
template<class base_client = http_universal_client>
class http_client_with_connector : public base_client
{
public:
  explicit http_client_with_connector(std::unique_ptr<i_tcp_connector> connector)
    : m_connector(std::move(connector)) {}

  // main entry used by invoke_request(u_c, tr,...)
  bool connect(const std::string& host, int port, unsigned int timeout) override
  {
    // force non-SSL at TCP stage; TLS-over-SOCKS can be added later after CONNECT
    this->set_is_ssl(false);

    auto& sock = this->get_socket();

    auto tcp_connect = [&](const std::string& h, int p, unsigned int t) -> bool
    {
      return base_client::connect(h, p, t);
    };

    if (!m_connector->establish(sock, tcp_connect, host, static_cast<uint16_t>(port), timeout))
      return false;

    // make sure HTTP Host: header matches destination (not proxy).
    this->set_host_name(host);
    return true;
  }

  using base_client::connect; // keep other overloads visible

private:
  std::unique_ptr<i_tcp_connector> m_connector;
};

} // http
} // net_utils
} // epee