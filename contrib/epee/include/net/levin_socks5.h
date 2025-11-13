#pragma once
#include "net/levin_client.h"
#include "net/socks5_proxy_transport.h"

namespace tools {
using levin_over_socks5_client = epee::levin::levin_client_impl2<tools::socks5::socks5_proxy_transport>;
} // namespace tools
