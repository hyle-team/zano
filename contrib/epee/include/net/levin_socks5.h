#pragma once
#include "net/levin_client.h"
#include "net/net_helper.h"
#include "net/socks5_proxy_transport.h"

namespace tools
{
  namespace socks5
  {
    using proxy = socks5::socks5_proxy_transport<epee::net_utils::blocked_mode_client>;
  }
  using levin_over_socks5_client = epee::levin::levin_client_impl2<socks5::proxy>;
}
