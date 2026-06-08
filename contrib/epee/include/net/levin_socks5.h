#pragma once
#include "net/levin_client.h"
#include "net/net_helper.h"
#include "net/socks5_proxy_transport.h"
#include "net/http_client.h"

namespace tools
{
  namespace socks5
  {
    using proxy = socks5_proxy_transport<false>;
    using ssl_proxy = socks5_proxy_transport<true>;
  }
  using levin_over_socks5_client        = epee::levin::levin_client_impl2<socks5::proxy>;
  using levin_over_https_socks5_client  = epee::levin::levin_client_impl2<socks5::ssl_proxy>;
  using http_socks5_client_http         = epee::net_utils::http::http_simple_client_t<false, socks5::proxy>;
  using http_socks5_client_https        = epee::net_utils::http::http_simple_client_t<true, socks5::ssl_proxy>;
}
