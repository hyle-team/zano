#include <string>
#include <cstdint>
#include "net/levin_socks5.h"
#include "net/socks5_proxy_transport.h"

namespace epee { namespace tests {

bool run_levin_socks5_connect_probe(const std::string& proxy_host, uint16_t proxy_port,
  const std::string& dest_host,uint16_t dest_port, unsigned int timeout_ms)
{
  tools::levin_over_socks5_client client;
  client.get_transport().set_socks_proxy(proxy_host, proxy_port);
  client.get_transport().set_use_remote_dns(true);

  if (!client.connect(dest_host, static_cast<int>(dest_port), timeout_ms))
    return false;

  client.disconnect();
  return true;
}

}} // namespace
