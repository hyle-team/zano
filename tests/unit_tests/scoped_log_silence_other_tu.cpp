// Copyright (c) 2026, Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Keep assertion instrumentation identical in both isolated test units. The
// ordinary unit_tests build uses its normal platform assertion definitions.
#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
namespace scoped_log_silence_test { void record_assert(bool value); }
#define LOCAL_ASSERT(expr) scoped_log_silence_test::record_assert(!!(expr))
#define FORCE_ASSERT(expr) scoped_log_silence_test::record_assert(!!(expr))
#endif

#include "misc_log_ex.h"
#include "misc_language.h"

#include <exception>
#include <ostream>
#include <stdexcept>
#include <string>

ENABLE_CHANNEL_BY_DEFAULT("scoped-log-silence-default-test")

namespace scoped_log_silence_test
{
namespace
{
  struct payload
  {
    const char* text;
    int& formats;
  };

  std::ostream& operator<<(std::ostream& stream, const payload& value)
  {
    ++value.formats;
    return stream << value.text;
  }

  struct observed_exception : std::exception
  {
    explicit observed_exception(int& calls) : what_calls(calls) {}
    const char* what() const noexcept override
    {
      ++what_calls;
      return "scoped-log-silence-raw-what";
    }
    int& what_calls;
  };
}

bool silenced_in_other_tu()
{
  return epee::log_space::is_thread_log_silenced();
}

// Every independent macro entry point. Each argument category is observable,
// including callback selection (not just invocation), level, channel and target.
void emit_macros(const char* text, int& formats, int& arguments, int& callbacks)
{
  const auto message = [&]() { ++arguments; return payload{text, formats}; };
  const auto channel = [&]() { ++arguments; return "scoped-log-silence-default-test"; };
  const auto target = [&]() -> const char* { ++arguments; return nullptr; };
  const auto level = [&]() { ++arguments; return LOG_LEVEL_0; };
  const auto color = [&]() { ++arguments; return LOG_COLOR_GREEN; };
  const auto callback = [&]() {
    ++arguments;
    return [&](const std::string&) { ++callbacks; };
  };
  LOG_PRINT_CHANNEL_NO_PREFIX2(channel(), target(), message(), level());
  LOG_PRINT_CHANNEL_NO_PREFIX_NO_POSTFIX2(channel(), target(), message(), level());
  LOG_PRINT_CHANNEL_NO_POSTFIX2(channel(), target(), message(), level());
  LOG_PRINT_CHANNEL2_CB(channel(), target(), message(), level(), callback());
  LOG_PRINT_CHANNEL_COLOR2_CB(channel(), target(), message(), level(), color(), callback());
  LOG_PRINT_CHANNEL_COLOR2_CB_NO_PREFIX(channel(), target(), message(), level(), color(), callback());
  LOG_PRINT_CHANNEL_2_JORNAL(channel(), target(), message(), level());
  LOG_ERROR2_CB(target(), message(), callback());
}

void emit_nested_then_throw(const char* text, int& formats, int& arguments, int& callbacks)
{
  epee::log_space::scoped_log_silence inner;
  emit_macros(text, formats, arguments, callbacks);
  throw std::runtime_error("scoped-log-silence-unwind");
}

void emit_frame(int& names, int& levels, int& targets)
{
  const auto name = [&]() { ++names; return "scoped-log-silence-frame"; };
  const auto level = [&]() { ++levels; return LOG_LEVEL_0; };
  const auto target = [&]() -> const char* { ++targets; return nullptr; };
  LOG_FRAME2(target(), name(), level());
}

bool emit_direct(const std::string& message)
{
  return epee::log_space::log_singletone::do_log_message(
    message, LOG_LEVEL_0, LOG_COLOR_RED, true);
}

int catch_l0(int& what_calls, bool unknown)
{
  TRY_ENTRY();
  if (unknown)
    throw 17;
  throw observed_exception(what_calls);
  CATCH_ENTRY_L0("scoped-log-silence-catch", 23);
}

void nested_catch(int& what_calls, bool unknown)
{
  NESTED_TRY_ENTRY();
  if (unknown)
    throw 17;
  throw observed_exception(what_calls);
  NESTED_CATCH_ENTRY("scoped-log-silence-nested-catch");
}

int checked_return(int& formats, bool& custom)
{
  CHECK_AND_ASSERT_MES_CUSTOM(false, 29, custom = true,
    (payload{"scoped-log-silence-return", formats}));
  return 0;
}

void checked_throw()
{
  CHECK_AND_ASSERT_THROW(false, std::runtime_error("scoped-log-silence-throw"));
}

void message_throw(int& formats)
{
  // This expression is still required to construct the thrown exception.
  ASSERT_MES_AND_THROW((payload{"scoped-log-silence-thrown-message", formats}));
}
} // namespace scoped_log_silence_test
