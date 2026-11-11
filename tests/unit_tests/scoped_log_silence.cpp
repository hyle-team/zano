// Copyright (c) 2026, Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
namespace scoped_log_silence_test { void record_assert(bool value); }
#define LOCAL_ASSERT(expr) scoped_log_silence_test::record_assert(!!(expr))
#define FORCE_ASSERT(expr) scoped_log_silence_test::record_assert(!!(expr))
#endif

#include "misc_log_ex.h"
#include "gtest/gtest.h"

#include <atomic>
#include <cstdint>
#include <future>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace scoped_log_silence_test
{
bool silenced_in_other_tu();
void emit_macros(const char*, int&, int&, int&);
void emit_nested_then_throw(const char*, int&, int&, int&);
void emit_frame(int&, int&, int&);
bool emit_direct(const std::string&);
int catch_l0(int&, bool);
void nested_catch(int&, bool);
int checked_return(int&, bool&);
void checked_throw();
void message_throw(int&);

#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
std::atomic<unsigned> failed_assertions{0};
void record_assert(bool value)
{
  if (!value)
    ++failed_assertions;
}
#endif

namespace
{
using epee::log_space::log_singletone;
using epee::log_space::scoped_log_silence;
using epee::log_space::is_thread_log_silenced;

static_assert(!std::is_copy_constructible<scoped_log_silence>::value, "scope must not copy");
static_assert(!std::is_copy_assignable<scoped_log_silence>::value, "scope must not copy");
static_assert(!std::is_move_constructible<scoped_log_silence>::value, "scope must not move");
static_assert(!std::is_move_assignable<scoped_log_silence>::value, "scope must not move");
static_assert(std::is_nothrow_default_constructible<scoped_log_silence>::value, "scope must not throw");
static_assert(std::is_nothrow_destructible<scoped_log_silence>::value, "scope must not throw");
static_assert(noexcept(is_thread_log_silenced()), "query must not throw");

const int memory_sink_type = 0x534c53;
const char* const test_channel = "scoped-log-silence-default-test";

struct entry
{
  std::string text;
  int level;
  int color;
  std::string target;
};

struct capture
{
  std::mutex mutex;
  std::vector<entry> entries;

  std::vector<entry> read()
  {
    std::lock_guard<std::mutex> lock(mutex);
    return entries;
  }
};

struct memory_sink : epee::log_space::ibase_log_stream
{
  explicit memory_sink(std::shared_ptr<capture> output) : output(std::move(output)) {}
  bool out_buffer(const char* buffer, int length, int level, int color,
    const char* target = nullptr) override
  {
    std::lock_guard<std::mutex> lock(output->mutex);
    output->entries.push_back({std::string(buffer, length), level, color, target ? target : ""});
    return true;
  }
  int get_type() override { return memory_sink_type; }
  std::shared_ptr<capture> output;
};

uint64_t error_count()
{
  auto errors = epee::log_space::get_channels_errors_stat_container();
  const auto found = errors->find("");
  return found == errors->end() ? 0 : found->second;
}

std::list<std::string> journal()
{
  std::list<std::string> entries;
  log_singletone::get_journal_items(entries, LOG_JOURNAL_MAX_ELEMENTS);
  return entries;
}

class scoped_log_silence_test_case : public ::testing::Test
{
protected:
  void SetUp() override
  {
    old_level = log_singletone::get_set_log_detalisation_level();
    old_time = log_singletone::get_set_time_level();
    old_thread_id = log_singletone::get_set_need_thread_id();
    channel_was_enabled = log_singletone::channel_enabled(test_channel);
    log_singletone::get_enabled_channels().insert(test_channel);
    log_singletone::get_set_log_detalisation_level(true, LOG_LEVEL_4);
    log_singletone::get_set_time_level(true, LOG_LEVEL_4 + 1);
    log_singletone::get_set_need_thread_id(true, false);
    output = std::make_shared<capture>();
    // The logger owns the sink. No files/console sinks are added and no existing
    // sink, journal or live singleton is reset by these tests.
    ASSERT_TRUE(log_singletone::add_logger(new memory_sink(output)));
  }

  void TearDown() override
  {
    EXPECT_TRUE(log_singletone::remove_logger(memory_sink_type));
    log_singletone::get_set_log_detalisation_level(true, old_level);
    log_singletone::get_set_time_level(true, old_time);
    log_singletone::get_set_need_thread_id(true, old_thread_id);
    if (!channel_was_enabled)
      log_singletone::get_enabled_channels().erase(test_channel);
  }

  int old_level = 0;
  int old_time = 0;
  bool old_thread_id = false;
  bool channel_was_enabled = false;
  std::shared_ptr<capture> output;
};

TEST_F(scoped_log_silence_test_case, cross_translation_unit_nesting_and_exception_restore_output)
{
  int formats = 0, arguments = 0, callbacks = 0;
  const auto previous_journal = journal();
  const auto previous_channels = log_singletone::get_enabled_channels();
  EXPECT_FALSE(is_thread_log_silenced());
  {
    scoped_log_silence outer;
    EXPECT_TRUE(silenced_in_other_tu());
    emit_macros("scoped-log-silence-hidden", formats, arguments, callbacks);
    EXPECT_THROW(emit_nested_then_throw("scoped-log-silence-hidden", formats, arguments, callbacks),
      std::runtime_error);
    EXPECT_TRUE(is_thread_log_silenced());
    EXPECT_TRUE(silenced_in_other_tu());
    emit_macros("scoped-log-silence-hidden", formats, arguments, callbacks);
    EXPECT_EQ(0, formats);
    EXPECT_EQ(0, arguments);
    EXPECT_EQ(0, callbacks);
    EXPECT_TRUE(output->read().empty());
    EXPECT_EQ(previous_journal, journal());
  }
  EXPECT_FALSE(is_thread_log_silenced());
  EXPECT_FALSE(silenced_in_other_tu());
  EXPECT_EQ(LOG_LEVEL_4, log_singletone::get_set_log_detalisation_level());
  EXPECT_EQ(previous_channels, log_singletone::get_enabled_channels());
  emit_macros("scoped-log-silence-visible", formats, arguments, callbacks);
  EXPECT_EQ(8, formats);
  // Legacy print macros evaluate level() twice; message() also counts once.
  // The eight entries contribute 5 + 5 + 5 + 6 + 7 + 7 + 5 + 3.
  EXPECT_EQ(43, arguments);
  EXPECT_EQ(4, callbacks);
  const auto entries = output->read();
  ASSERT_EQ(8u, entries.size());
  for (const auto& value : entries)
    EXPECT_NE(std::string::npos, value.text.find("scoped-log-silence-visible"));
  const auto now = journal();
  ASSERT_GE(now.size(), 2u);
  auto tail = now.rbegin();
  EXPECT_NE(std::string::npos, tail++->find("scoped-log-silence-visible"));
  EXPECT_NE(std::string::npos, tail->find("scoped-log-silence-visible"));
}

TEST_F(scoped_log_silence_test_case, a_concurrent_thread_still_emits_to_existing_sinks)
{
  int formats = 0, arguments = 0, callbacks = 0;
  bool worker_silenced = true;
  std::promise<void> start;
  auto ready = start.get_future();
  std::thread worker([&]() {
    ready.wait();
    worker_silenced = silenced_in_other_tu();
    emit_macros("scoped-log-silence-worker", formats, arguments, callbacks);
  });
  {
    scoped_log_silence outer;
    start.set_value();
    worker.join();
    EXPECT_TRUE(is_thread_log_silenced());
    int hidden_formats = 0, hidden_arguments = 0, hidden_callbacks = 0;
    emit_macros("scoped-log-silence-hidden", hidden_formats, hidden_arguments, hidden_callbacks);
    EXPECT_EQ(0, hidden_formats + hidden_arguments + hidden_callbacks);
  }
  EXPECT_FALSE(worker_silenced);
  EXPECT_EQ(8, formats);
  EXPECT_EQ(43, arguments);
  EXPECT_EQ(4, callbacks);
  EXPECT_EQ(8u, output->read().size());
  EXPECT_FALSE(is_thread_log_silenced());
}

TEST_F(scoped_log_silence_test_case, direct_singleton_and_instance_sinks_suppress_journals)
{
  epee::log_space::logger instance;
  auto direct_output = std::make_shared<capture>();
  ASSERT_TRUE(instance.add_logger(new memory_sink(direct_output)));
  const auto previous_journal = journal();
  int evaluated_arguments = 0;
  const auto argument = [&]() { ++evaluated_arguments; return std::string("scoped-log-silence-direct"); };
  {
    scoped_log_silence silence;
    EXPECT_TRUE(emit_direct(argument()));
    EXPECT_TRUE(instance.do_log_message(argument(), LOG_LEVEL_0, LOG_COLOR_RED, true));
  }
  // C++ evaluates direct function arguments before entering a guarded callee.
  EXPECT_EQ(2, evaluated_arguments);
  EXPECT_TRUE(output->read().empty());
  EXPECT_TRUE(direct_output->read().empty());
  EXPECT_EQ(previous_journal, journal());
  std::list<std::string> local_journal;
  instance.get_journal_items(local_journal, 100);
  EXPECT_TRUE(local_journal.empty());
  EXPECT_TRUE(emit_direct("scoped-log-silence-direct-visible"));
  EXPECT_TRUE(instance.do_log_message("scoped-log-silence-instance-visible", LOG_LEVEL_1, LOG_COLOR_BLUE, true, "target"));
  ASSERT_EQ(1u, direct_output->read().size());
  const auto direct = direct_output->read().front();
  EXPECT_EQ("scoped-log-silence-instance-visible", direct.text);
  EXPECT_EQ(LOG_LEVEL_1, direct.level);
  EXPECT_EQ(LOG_COLOR_BLUE, direct.color);
  EXPECT_EQ("target", direct.target);
  instance.get_journal_items(local_journal, 100);
  ASSERT_EQ(1u, local_journal.size());
  EXPECT_EQ(direct.text, local_journal.front());
  EXPECT_EQ(1u, output->read().size());
}

TEST_F(scoped_log_silence_test_case, unsilenced_macro_formats_callbacks_filters_and_errors_are_unchanged)
{
  const auto prefix = log_singletone::get_prefix_entry();
  std::string callback_text;
  const auto callback = [&](const std::string& text) { callback_text = text; };
  LOG_PRINT_CHANNEL_NO_PREFIX2(test_channel, nullptr, "a", LOG_LEVEL_0);
  LOG_PRINT_CHANNEL_NO_PREFIX_NO_POSTFIX2(test_channel, nullptr, "b", LOG_LEVEL_0);
  LOG_PRINT_CHANNEL_NO_POSTFIX2(test_channel, nullptr, "c", LOG_LEVEL_0);
  LOG_PRINT_CHANNEL_COLOR2_CB_NO_PREFIX(test_channel, "target", "d", LOG_LEVEL_0, LOG_COLOR_BLUE, callback);
  auto entries = output->read();
  ASSERT_EQ(4u, entries.size());
  EXPECT_EQ("a\n", entries[0].text);
  EXPECT_EQ("b", entries[1].text);
  EXPECT_EQ(prefix + "c", entries[2].text);
  EXPECT_EQ("d\n", entries[3].text);
  EXPECT_EQ("d\n", callback_text);
  EXPECT_EQ(LOG_COLOR_BLUE, entries[3].color);
  EXPECT_EQ("target", entries[3].target);
  int formats = 0;
  LOG_PRINT_CHANNEL2("scoped-log-silence-disabled", nullptr, ++formats, LOG_LEVEL_0);
  log_singletone::get_set_log_detalisation_level(true, LOG_LEVEL_SILENT);
  LOG_PRINT_L0(++formats);
  EXPECT_EQ(0, formats);
  const auto before_errors = error_count();
  LOG_ERROR_CB("scoped-log-silence-error", callback);
  EXPECT_EQ(before_errors + 1, error_count());
  entries = output->read();
  ASSERT_EQ(5u, entries.size());
  EXPECT_NE(std::string::npos, entries.back().text.find("[ERROR] Location:"));
  EXPECT_NE(std::string::npos, entries.back().text.find("scoped-log-silence-error"));
  EXPECT_EQ(entries.back().text, callback_text);
  EXPECT_EQ(LOG_COLOR_RED, entries.back().color);
  ASSERT_FALSE(journal().empty());
  EXPECT_EQ(entries.back().text, journal().back());
}

TEST_F(scoped_log_silence_test_case, forwarding_macro_variants_keep_arguments_lazy)
{
  int arguments = 0;
  const auto text = [&]() { ++arguments; return "scoped-log-silence-alias"; };
  const auto callback = [&](const std::string&) { ++arguments; };
  const auto before_journal = journal();
  {
    scoped_log_silence silence;
    LOG_PRINT_CHANNEL2(test_channel, nullptr, text(), LOG_LEVEL_0);
    LOG_PRINT_CHANNEL_COLOR2(test_channel, nullptr, text(), LOG_LEVEL_0, LOG_COLOR_RED);
    LOG_PRINT_CHANNEL_COLOR2_NO_PREFIX(test_channel, nullptr, text(), LOG_LEVEL_0, LOG_COLOR_RED);
    LOG_PRINT_NO_PREFIX2(nullptr, text(), LOG_LEVEL_0);
    LOG_PRINT_NO_PREFIX_NO_POSTFIX2(nullptr, text(), LOG_LEVEL_0);
    LOG_PRINT_NO_POSTFIX2(nullptr, text(), LOG_LEVEL_0);
    LOG_PRINT2(nullptr, text(), LOG_LEVEL_0);
    LOG_PRINT2_CB(nullptr, text(), LOG_LEVEL_0, callback);
    LOG_PRINT_COLOR2(nullptr, text(), LOG_LEVEL_0, LOG_COLOR_RED);
    LOG_PRINT_COLOR2_NO_PREFIX(nullptr, text(), LOG_LEVEL_0, LOG_COLOR_RED);
    LOG_PRINT_COLOR2_CB(nullptr, text(), LOG_LEVEL_0, LOG_COLOR_RED, callback);
    LOG_PRINT2_JORNAL(nullptr, text(), LOG_LEVEL_0);
    LOG_ERROR2(nullptr, text());
    LOG_PRINT_NO_POSTFIX(text(), LOG_LEVEL_0);
    LOG_PRINT_NO_PREFIX(text(), LOG_LEVEL_0);
    LOG_PRINT_NO_PREFIX_NO_POSTFIX(text(), LOG_LEVEL_0);
    LOG_PRINT(text(), LOG_LEVEL_0);
    LOG_PRINT_CB(text(), LOG_LEVEL_0, callback);
    LOG_PRINT_COLOR_CB(text(), LOG_LEVEL_0, LOG_COLOR_RED, callback);
    LOG_PRINT_COLOR(text(), LOG_LEVEL_0, LOG_COLOR_RED);
    LOG_PRINT_COLOR_NO_PREFIX(text(), LOG_LEVEL_0, LOG_COLOR_RED);
    LOG_PRINT_RED(text(), LOG_LEVEL_0);
    LOG_PRINT_GREEN(text(), LOG_LEVEL_0);
    LOG_PRINT_BLUE(text(), LOG_LEVEL_0);
    LOG_PRINT_YELLOW(text(), LOG_LEVEL_0);
    LOG_PRINT_CYAN(text(), LOG_LEVEL_0);
    LOG_PRINT_MAGENTA(text(), LOG_LEVEL_0);
    LOG_PRINT_RED_L0(text());
    LOG_PRINT_GREEN_L0(text());
    LOG_PRINT_L0(text());
    LOG_PRINT_L1(text());
    LOG_PRINT_L2(text());
    LOG_PRINT_L3(text());
    LOG_PRINT_L4(text());
    LOG_PRINT_J(text(), LOG_LEVEL_0);
    LOG_ERROR(text());
    LOG_ERROR_CB(text(), callback);
  }
  EXPECT_EQ(0, arguments);
  EXPECT_TRUE(output->read().empty());
  EXPECT_EQ(before_journal, journal());
}

TEST_F(scoped_log_silence_test_case, frame_macros_are_lazy_and_silenced_entry_never_emits_an_exit)
{
  int names = 0, levels = 0, targets = 0;
  std::unique_ptr<epee::log_space::log_frame> frame;
  {
    scoped_log_silence silence;
    emit_frame(names, levels, targets);
    frame.reset(new epee::log_space::log_frame(
      (++names, "scoped-log-silence-direct-frame"), LOG_LEVEL_0, "target"));
  }
  EXPECT_EQ(1, names); // Only the caller-evaluated direct constructor argument.
  EXPECT_EQ(0, levels);
  EXPECT_EQ(0, targets);
  frame.reset();
  {
    std::unique_ptr<scoped_log_silence> silence(new scoped_log_silence);
    LOG_FRAME((++names, "scoped-log-silence-macro-crossing"), (++levels, LOG_LEVEL_0));
    silence.reset(); // The macro-created frame also outlives the silence.
  }
  EXPECT_EQ(1, names);
  EXPECT_EQ(0, levels);
  EXPECT_TRUE(output->read().empty());
  emit_frame(names, levels, targets);
  EXPECT_EQ(2, names);
  EXPECT_EQ(1, levels);
  EXPECT_EQ(1, targets);
  const auto entries = output->read();
  ASSERT_EQ(2u, entries.size());
  EXPECT_NE(std::string::npos, entries[0].text.find("-->>scoped-log-silence-frame"));
  EXPECT_NE(std::string::npos, entries[1].text.find("<<--scoped-log-silence-frame"));
}

TEST_F(scoped_log_silence_test_case, frame_exit_during_silence_is_suppressed)
{
  std::unique_ptr<epee::log_space::log_frame> frame(
    new epee::log_space::log_frame("scoped-log-silence-before", LOG_LEVEL_0));
  ASSERT_EQ(1u, output->read().size());
  {
    scoped_log_silence silence;
    frame.reset();
  }
  EXPECT_EQ(1u, output->read().size());
}

TEST_F(scoped_log_silence_test_case, unsilenced_frames_preserve_level_changes_nesting_and_unwind)
{
  const auto prefix = log_singletone::get_prefix_entry();
  log_singletone::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  {
    epee::log_space::log_frame frame("scoped-log-silence-filtered", LOG_LEVEL_4);
    log_singletone::get_set_log_detalisation_level(true, LOG_LEVEL_4);
  }
  // Legacy behavior: level-filtered entry stores no name, but a later enabled
  // destructor emits an empty exit. Silence must not change that level case.
  ASSERT_EQ(1u, output->read().size());
  EXPECT_EQ(prefix + "<<--\n", output->read()[0].text);
  {
    epee::log_space::log_frame frame("scoped-log-silence-level-down", LOG_LEVEL_4);
    log_singletone::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  }
  EXPECT_EQ(2u, output->read().size());
  log_singletone::get_set_log_detalisation_level(true, LOG_LEVEL_4);
  try
  {
    LOG_FRAME("scoped-log-silence-outer", LOG_LEVEL_0);
    {
      LOG_FRAME("scoped-log-silence-inner", LOG_LEVEL_0);
      throw std::runtime_error("scoped-log-silence-unwind");
    }
  }
  catch (const std::runtime_error&) {}
  const auto entries = output->read();
  ASSERT_EQ(6u, entries.size());
  EXPECT_EQ(prefix + "-->>scoped-log-silence-outer\n", entries[2].text);
  EXPECT_EQ(prefix + "-->>scoped-log-silence-inner\n", entries[3].text);
  EXPECT_EQ(prefix + "<<--scoped-log-silence-inner\n", entries[4].text);
  EXPECT_EQ(prefix + "<<--scoped-log-silence-outer\n", entries[5].text);
}

TEST_F(scoped_log_silence_test_case, silence_preserves_assert_counts_returns_throws_and_custom_control)
{
  int formats = 0, callbacks = 0;
  bool custom = false;
  const auto before_errors = error_count();
  const auto previous_journal = journal();
#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
  const auto before_asserts = failed_assertions.load();
#endif
  {
    scoped_log_silence silence;
    EXPECT_EQ(29, checked_return(formats, custom));
    EXPECT_TRUE(custom);
    EXPECT_THROW(checked_throw(), std::runtime_error);
    try { message_throw(formats); FAIL() << "missing exception"; }
    catch (const std::runtime_error& ex)
    {
      EXPECT_STREQ("scoped-log-silence-thrown-message", ex.what());
    }
    const auto callback = [&](const std::string&) { ++callbacks; };
    LOG_ERROR_CB(++formats, callback);
  }
  EXPECT_EQ(1, formats); // Required construction of the thrown message remains.
  EXPECT_EQ(0, callbacks);
  EXPECT_EQ(before_errors + 4, error_count());
#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
  EXPECT_EQ(before_asserts + 4, failed_assertions.load());
#endif
  EXPECT_EQ(previous_journal, journal());
  EXPECT_TRUE(output->read().empty());
}

TEST_F(scoped_log_silence_test_case, catch_entry_and_nested_catch_do_not_evaluate_raw_what_under_silence)
{
  int what_calls = 0;
  const auto previous_journal = journal();
  const auto before_errors = error_count();
  {
    scoped_log_silence silence;
    EXPECT_EQ(23, catch_l0(what_calls, false));
    EXPECT_EQ(23, catch_l0(what_calls, true));
    nested_catch(what_calls, false);
    nested_catch(what_calls, true);
  }
  EXPECT_EQ(0, what_calls);
  EXPECT_EQ(before_errors + 2, error_count());
  EXPECT_TRUE(output->read().empty());
  EXPECT_EQ(previous_journal, journal());
  const auto prefix = log_singletone::get_prefix_entry();
  EXPECT_EQ(23, catch_l0(what_calls, false));
  EXPECT_EQ(before_errors + 2, error_count());
  nested_catch(what_calls, false);
  EXPECT_EQ(2, what_calls);
  EXPECT_EQ(before_errors + 3, error_count());
  EXPECT_EQ(23, catch_l0(what_calls, true));
  EXPECT_EQ(2, what_calls);
  EXPECT_EQ(before_errors + 3, error_count());
  nested_catch(what_calls, true);
  EXPECT_EQ(2, what_calls);
  EXPECT_EQ(before_errors + 4, error_count());
  const auto entries = output->read();
  ASSERT_EQ(4u, entries.size());
  EXPECT_EQ(prefix + "Exception at [scoped-log-silence-catch], what=scoped-log-silence-raw-what\n",
    entries[0].text);
  EXPECT_EQ(prefix + "Exception at [scoped-log-silence-catch], generic exception \"...\"\n",
    entries[2].text);
  const std::string typed_suffix = " Message:\nException at [scoped-log-silence-nested-catch], what=scoped-log-silence-raw-what\n";
  const std::string generic_suffix = " Message:\nException at [scoped-log-silence-nested-catch], generic exception \"...\"\n";
  ASSERT_GE(entries[1].text.size(), typed_suffix.size());
  ASSERT_GE(entries[3].text.size(), generic_suffix.size());
  EXPECT_EQ(typed_suffix, entries[1].text.substr(entries[1].text.size() - typed_suffix.size()));
  EXPECT_EQ(generic_suffix, entries[3].text.substr(entries[3].text.size() - generic_suffix.size()));
  EXPECT_EQ(0u, entries[1].text.find(prefix + "[ERROR] Location: \n"));
  EXPECT_EQ(0u, entries[3].text.find(prefix + "[ERROR] Location: \n"));
}

TEST_F(scoped_log_silence_test_case, unsilenced_formatting_failure_keeps_legacy_error_control_order)
{
  int evaluated = 0, callbacks = 0;
  const auto throwing = [&]() -> std::string { ++evaluated; throw std::runtime_error("format"); };
  const auto callback = [&](const std::string&) { ++callbacks; };
  const auto before_errors = error_count();
#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
  const auto before_asserts = failed_assertions.load();
#endif
  LOG_ERROR_CB(throwing(), callback);
  EXPECT_EQ(1, evaluated);
  EXPECT_EQ(before_errors, error_count());
#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
  EXPECT_EQ(before_asserts, failed_assertions.load());
#endif
  {
    scoped_log_silence silence;
    LOG_ERROR_CB(throwing(), callback);
  }
  EXPECT_EQ(1, evaluated);
  EXPECT_EQ(0, callbacks);
  EXPECT_EQ(before_errors + 1, error_count());
#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
  EXPECT_EQ(before_asserts + 1, failed_assertions.load());
#endif
  EXPECT_TRUE(output->read().empty());
}
} // namespace
} // namespace scoped_log_silence_test

#ifdef EPEE_SCOPED_LOG_SILENCE_STANDALONE_TEST
namespace epee
{
namespace log_space
{
struct scoped_log_silence_test_observer
{
  static bool is_cold()
  {
    return log_singletone::get_set_instance_internal() == nullptr
      && !log_singletone::get_set_is_uninitialized();
  }
};
}
}

namespace
{
// A null result means success. Every failure stage is a fixed diagnostic;
// no logged payload, path, exception text or counter value is returned.
const char* check_cold_logging()
{
  using namespace scoped_log_silence_test;
  using namespace epee::log_space;
  if (!scoped_log_silence_test_observer::is_cold())
    return "initial state";
  int formats = 0, arguments = 0, callbacks = 0;
  int names = 0, levels = 0, targets = 0, what_calls = 0;
  std::unique_ptr<log_frame> frame;
  {
    scoped_log_silence silence;
    if (!silenced_in_other_tu())
      return "cross-TU entry";
    emit_macros("scoped-log-silence-cold", formats, arguments, callbacks);
    if (!scoped_log_silence_test_observer::is_cold())
      return "macro emission state";
    if (!emit_direct("scoped-log-silence-cold-direct"))
      return "direct sink result";
    if (!scoped_log_silence_test_observer::is_cold())
      return "direct sink state";
    emit_frame(names, levels, targets);
    if (!scoped_log_silence_test_observer::is_cold())
      return "macro frame state";
    frame.reset(new log_frame("scoped-log-silence-cold-frame", LOG_LEVEL_0));
    if (!scoped_log_silence_test_observer::is_cold())
      return "direct frame entry state";
    try { emit_nested_then_throw("scoped-log-silence-cold-nested", formats, arguments, callbacks); }
    catch (const std::runtime_error&) {}
    if (!is_thread_log_silenced())
      return "nested unwind depth";
    if (!scoped_log_silence_test_observer::is_cold())
      return "nested unwind state";
    if (catch_l0(what_calls, false) != 23)
      return "catch return";
    if (!scoped_log_silence_test_observer::is_cold())
      return "catch state";
    nested_catch(what_calls, false);
    if (!scoped_log_silence_test_observer::is_cold())
      return "nested catch state";
  }
  frame.reset(); // A silenced frame may outlive the silence, even when cold.
  if (!scoped_log_silence_test_observer::is_cold())
    return "direct frame exit state";
  if (formats != 0)
    return "payload formatting";
  if (arguments != 0)
    return "macro arguments";
  if (callbacks != 0)
    return "payload callbacks";
  if (names != 0)
    return "frame names";
  if (levels != 0)
    return "frame levels";
  if (targets != 0)
    return "frame targets";
  if (what_calls != 0)
    return "exception what";
  if (is_thread_log_silenced())
    return "local scope exit";
  if (silenced_in_other_tu())
    return "cross-TU scope exit";
  return nullptr;
}
}

// Compile only these two test files and bundled gtest-all.cc in this mode.
// Exclude unit_tests/main.cpp and use this define consistently in every TU.
// No live singleton is reset to manufacture a preinitialization observation.
int main(int argc, char** argv)
{
  if (const char* failure = check_cold_logging())
  {
    std::cerr << "scoped logging cold-state check failed: " << failure << '\n';
    return 2;
  }
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
