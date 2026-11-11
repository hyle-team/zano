// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once
#include "crypto/crypto.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
namespace tools { namespace native_submission {
inline constexpr std::size_t max_transaction_bytes=122800;
inline constexpr std::size_t max_request_bytes=256*1024;
inline constexpr std::size_t max_response_bytes=256*1024;
inline constexpr std::size_t max_status_bytes=256;
inline constexpr std::chrono::milliseconds exchange_budget{15000};
enum class route { Submit=0, Lookup=1 };
// Fresh native submission authority for Submit; existing current native Open
// for explicit Lookup. Never a JS Boolean, endpoint or native pointer.
class call_guard { public: virtual ~call_guard()=default; virtual bool current() const noexcept=0;
 virtual std::chrono::milliseconds remaining() const noexcept { return exchange_budget; } };
class exchange_context final {
 public: bool stopped() const noexcept; std::chrono::milliseconds remaining() const noexcept;
 private:
  friend struct executor;
  exchange_context(const call_guard&,const std::atomic<bool>&);
  const call_guard& guard_;const std::atomic<bool>& stop_;
  const std::chrono::steady_clock::time_point deadline_;
};
class response_sink final {
 public: bool append(const unsigned char*,std::size_t) noexcept;
 private: friend struct executor;explicit response_sink(const exchange_context& c):context_(c){}
  const exchange_context& context_;std::string bytes_;bool failed_{false};
};
// Trusted fixed TESTNET HTTPS implementation. Two SDK-defined routes only;
// no arbitrary URL, redirects, implicit retry, logging or payload JS surface.
class exchange { public:virtual ~exchange()=default;
 virtual bool exchange_json(route,const unsigned char*,std::size_t,const exchange_context&,response_sink&) noexcept=0;
};
enum class outcome { NotAttempted, Uncertain, DaemonAccepted, DaemonRefused, ObservedPoolOrChain, NotObserved };
struct result { outcome status{outcome::Uncertain};std::string daemon_status; };
// Caller MUST have committed its authenticated attempt before Submit. Both
// operations parse canonical signed bytes and independently match SDK tx hash.
// Exactly one exchange; NotObserved never authorizes resend or input release.
result submit(const std::string&,const crypto::hash&,const call_guard&,exchange&,const std::atomic<bool>&) noexcept;
result lookup(const std::string&,const crypto::hash&,const call_guard&,exchange&,const std::atomic<bool>&) noexcept;
} }
