#pragma once
#include <string>
#include <cstdint>

namespace epee { namespace tests {

// just a connect probe via SOCKS5 (no Levin invoke)
bool run_levin_socks5_connect_probe(const std::string& proxy_host,
  uint16_t proxy_port, const std::string& dest_host, uint16_t dest_port, unsigned int timeout_ms);

}} // namespace
