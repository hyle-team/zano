#include <cstdint>
#include <string>

#include <emscripten/emscripten.h>

#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"

enum ExitCode {
  ok = 0,
  invalid = 1,
  bad_args = 2,
  zano_integrated_ok = 3,
  wrapped_like_ok = 4,
};

extern "C"
__attribute__((used))
__attribute__((visibility("default")))
EMSCRIPTEN_KEEPALIVE
uint8_t zano_validate_address(const char* addr_cstr) {
  if (!addr_cstr) {
    return static_cast<uint8_t>(bad_args);
  }

  const std::string addr_str(addr_cstr);

  if (currency::is_address_like_wrapped(addr_str)) {
    return static_cast<uint8_t>(wrapped_like_ok);
  }

  currency::account_public_address addr = AUTO_VAL_INIT(addr);
  currency::payment_id_t pid;

  if (!currency::get_account_address_and_payment_id_from_str(addr, pid, addr_str)) {
    return static_cast<uint8_t>(invalid);
  }

  return static_cast<uint8_t>(pid.empty() ? ok : zano_integrated_ok);
}
