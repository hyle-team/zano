#include <string>
#include <vector>
#include <iostream>

#include "string_tools.h"
#include "net/test_levin_socks5.h"

int main(int argc, char* argv[])
{
  std::string proxy_host  = "127.0.0.1";
  std::string proxy_port_s = "9050"; // Tor
  std::string host         = "httpbin.org";
  std::string port_s       = "80";
  std::string timeout_s    = "15000";

  std::vector<std::string> args(argv + 1, argv + argc);
  for (const auto& a : args)
  {
    const auto pos = a.find('=');
    if (pos == std::string::npos) continue;
    const auto k = a.substr(0, pos);
    const auto v = a.substr(pos + 1);
    if      (k == "/proxy_host")  proxy_host   = v;
    else if (k == "/proxy_port")  proxy_port_s = v;
    else if (k == "/host")        host         = v;
    else if (k == "/port")        port_s       = v;
    else if (k == "/timeout_ms")  timeout_s    = v;
  }

  uint16_t proxy_port = 9050;
  try { proxy_port = static_cast<uint16_t>(std::stoi(proxy_port_s)); } catch (...) {}

  uint16_t port = 80;
  try { port = static_cast<uint16_t>(std::stoi(port_s)); } catch (...) {}

  unsigned int timeout_ms = 15000;
  try { timeout_ms = static_cast<unsigned int>(std::stoul(timeout_s)); } catch (...) {}

  const bool ok = epee::tests::run_levin_socks5_connect_probe(proxy_host, proxy_port, host, port, timeout_ms);
  std::cout << (ok ? "OK\n" : "FAIL\n");
  return ok ? 0 : 1;
}
