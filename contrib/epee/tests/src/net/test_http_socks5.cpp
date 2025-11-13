#include "include_base_utils.h"
#include "net/http_base.h"
#include "net/http_client.h"
#include "net/http_socks5_client.h"

#include "net/test_http_socks5.h"

namespace epee { namespace tests {

bool run_http_socks5_http_probe(const std::string& proxy_host, uint16_t proxy_port, const std::string& url, unsigned int timeout_ms)
{
  using epee::net_utils::http::http_socks5_client;
  using epee::net_utils::http::http_response_info;

  LOG_PRINT_L0("[SOCKS5 TEST] proxy=" << proxy_host << ":" << proxy_port << " url=" << url);

  http_socks5_client client(proxy_host, proxy_port);
  const http_response_info* resp = nullptr;

  bool ok = epee::net_utils::http::invoke_request(url, client, timeout_ms, &resp, "GET");
  if (!ok || !resp)
  {
    LOG_ERROR("[SOCKS5 TEST] request failed");
    return false;
  }

  LOG_PRINT_L0("[SOCKS5 TEST] HTTP " << resp->m_response_code);
  LOG_PRINT_L0("[SOCKS5 TEST] Body (first 512b): " << std::string(resp->m_body.data(), resp->m_body.data() + std::min<size_t>(resp->m_body.size(), 512)));

  if (resp->m_response_code < 200 || resp->m_response_code >= 300)
    return false;
  if (resp->m_body.empty())
    return false;
  return true;
}

}} // namespaces
