#pragma once
#include <string>
#include <cstdint>

namespace epee { namespace tests {
bool run_http_socks5_http_probe(const std::string& proxy_host, uint16_t proxy_port, const std::string& url, unsigned int timeout_ms = 15000);
}} // namespaces
