// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "gtest/gtest.h"
#include "common/encryption_filter.h"
#include "wallet/wallet_file_output.h"
#include <boost/iostreams/filtering_stream.hpp>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
  // The production output-filter template with a deliberately inert processor:
  // these tests execute output/error control flow, never crypto or wallet code.
  struct inert_processor
  {
    typedef char char_type;
    inert_processor(const std::string&, const crypto::chacha_iv&) {}
    template<class Callback>
    std::streamsize process(const char* bytes, std::streamsize count, Callback callback) const
    {
      callback(bytes, count);
      return count;
    }
    template<class Callback> bool flush(Callback) { return true; }
  };
  struct buffered_processor : inert_processor
  {
    using inert_processor::inert_processor;
    template<class Callback> bool flush(Callback callback)
    {
      callback("tail", 4);
      return true;
    }
  };
  struct false_flush_processor : inert_processor
  {
    using inert_processor::inert_processor;
    template<class Callback> bool flush(Callback) { return false; }
  };
  using inert_filter = tools::encrypt_chacha_out_filter_t<inert_processor>;
  using buffered_filter = tools::encrypt_chacha_out_filter_t<buffered_processor>;
  using false_flush_filter = tools::encrypt_chacha_out_filter_t<false_flush_processor>;

  struct sink_state
  {
    std::streamsize result = 3;
    bool throw_write = false;
    bool throw_flush = false;
    bool flush_ok = true;
    size_t writes = 0, flushes = 0;
  };
  struct controlled_sink
  {
    typedef char char_type;
    struct category : boost::iostreams::sink_tag, boost::iostreams::flushable_tag {};
    std::shared_ptr<sink_state> state;
    std::streamsize write(const char*, std::streamsize)
    {
      ++state->writes;
      if (state->throw_write) throw std::runtime_error("inert sink failure");
      return state->result;
    }
    bool flush()
    {
      ++state->flushes;
      if (state->throw_flush) throw std::runtime_error("inert flush failure");
      return state->flush_ok;
    }
  };

  struct controlled_buffer : std::streambuf
  {
    std::string bytes;
    bool short_write = false;
    bool fail_sync = false;
    size_t syncs = 0;
    std::streamsize xsputn(const char* data, std::streamsize size) override
    {
      const auto accepted = short_write && size ? size - 1 : size;
      bytes.append(data, static_cast<size_t>(accepted));
      return accepted;
    }
    int sync() override { ++syncs; return fail_sync ? -1 : 0; }
  };
  struct controlled_file : std::ostream
  {
    controlled_buffer buffer;
    bool fail_close = false, throw_close = false, closed = false;
    size_t closes = 0;
    controlled_file() : std::ostream(nullptr) { rdbuf(&buffer); }
    void close()
    {
      ++closes;
      closed = true;
      if (throw_close) throw std::runtime_error("inert close failure");
      if (fail_close) setstate(std::ios_base::failbit);
    }
  };

  const crypto::chacha_iv inert_iv{};
}

TEST(wallet_store_output, filter_accepts_exact_downstream_write)
{
  inert_filter filter("", inert_iv);
  auto state = std::make_shared<sink_state>();
  controlled_sink sink{state};
  EXPECT_EQ(3, filter.write(sink, "abc", 3));
  EXPECT_TRUE(filter.flush(sink));
  EXPECT_EQ(1u, state->writes);
  EXPECT_EQ(1u, state->flushes);
}

TEST(wallet_store_output, filter_rejects_all_nonexact_counts_and_latches)
{
  for (const auto count : {-1, 0, 1, 2, 4})
  {
    inert_filter filter("", inert_iv);
    auto state = std::make_shared<sink_state>();
    state->result = count;
    controlled_sink sink{state};
    EXPECT_THROW(filter.write(sink, "abc", 3), std::ios_base::failure);
    state->result = 3;
    EXPECT_THROW(filter.write(sink, "abc", 3), std::ios_base::failure);
    EXPECT_THROW(filter.flush(sink), std::ios_base::failure);
    EXPECT_EQ(1u, state->writes);
    EXPECT_EQ(0u, state->flushes);
  }
}

TEST(wallet_store_output, downstream_write_exception_latches)
{
  inert_filter filter("", inert_iv);
  auto state = std::make_shared<sink_state>();
  state->throw_write = true;
  controlled_sink sink{state};
  EXPECT_THROW(filter.write(sink, "abc", 3), std::runtime_error);
  state->throw_write = false;
  EXPECT_THROW(filter.write(sink, "abc", 3), std::ios_base::failure);
  EXPECT_EQ(1u, state->writes);
}

TEST(wallet_store_output, buffered_flush_rejects_short_write)
{
  buffered_filter filter("", inert_iv);
  auto state = std::make_shared<sink_state>();
  controlled_sink sink{state};
  EXPECT_THROW(filter.flush(sink), std::ios_base::failure);
  state->result = 4;
  EXPECT_THROW(filter.flush(sink), std::ios_base::failure);
  EXPECT_EQ(1u, state->writes);
}

TEST(wallet_store_output, processor_false_flush_fails)
{
  false_flush_filter filter("", inert_iv);
  auto state = std::make_shared<sink_state>();
  controlled_sink sink{state};
  EXPECT_THROW(filter.flush(sink), std::ios_base::failure);
  EXPECT_EQ(0u, state->flushes);
}

TEST(wallet_store_output, sink_false_flush_latches)
{
  inert_filter filter("", inert_iv);
  auto state = std::make_shared<sink_state>();
  state->flush_ok = false;
  controlled_sink sink{state};
  EXPECT_THROW(filter.flush(sink), std::ios_base::failure);
  state->flush_ok = true;
  EXPECT_THROW(filter.flush(sink), std::ios_base::failure);
  EXPECT_EQ(1u, state->flushes);
}

TEST(wallet_store_output, sink_throwing_flush_latches)
{
  inert_filter filter("", inert_iv);
  auto state = std::make_shared<sink_state>();
  state->throw_flush = true;
  controlled_sink sink{state};
  EXPECT_THROW(filter.flush(sink), std::runtime_error);
  state->throw_flush = false;
  EXPECT_THROW(filter.flush(sink), std::ios_base::failure);
  EXPECT_EQ(1u, state->flushes);
}

TEST(wallet_store_output, strict_sync_observes_sink_failure)
{
  auto state = std::make_shared<sink_state>();
  state->flush_ok = false;
  boost::iostreams::filtering_ostream output;
  output.push(inert_filter("", inert_iv));
  output.push(controlled_sink{state});
  output.write("abc", 3);
  output.flush();
  // Ordinary flush can discard a terminal device's false result.
  EXPECT_FALSE(output.strict_sync());
}

TEST(wallet_store_output, boost_buffered_short_write_fails_before_close)
{
  controlled_file file;
  file.buffer.short_write = true;
  boost::iostreams::filtering_ostream output;
  output.push(inert_filter("", inert_iv));
  output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file));
  output.write("abc", 3);
  EXPECT_THROW(tools::wallet_file_detail::finish_output(output, file), std::ios_base::failure);
  EXPECT_EQ(0u, file.closes);
}

TEST(wallet_store_output, finalization_drains_and_detaches_before_close)
{
  controlled_file file;
  {
    boost::iostreams::filtering_ostream output;
    output.push(inert_filter("", inert_iv));
    output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file));
    output.write("abc", 3);
    EXPECT_NO_THROW(tools::wallet_file_detail::finish_output(output, file));
    EXPECT_TRUE(output.empty());
    EXPECT_TRUE(file.closed);
    EXPECT_EQ("abc", file.buffer.bytes);
    EXPECT_EQ(1u, file.closes);
    const auto syncs = file.buffer.syncs;
    output.reset();
    EXPECT_EQ(syncs, file.buffer.syncs);
  }
  EXPECT_EQ(1u, file.closes);
}

TEST(wallet_store_output, file_flush_failure_prevents_close)
{
  controlled_file file;
  file.buffer.fail_sync = true;
  boost::iostreams::filtering_ostream output;
  output.push(inert_filter("", inert_iv));
  output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file));
  output.write("abc", 3);
  EXPECT_THROW(tools::wallet_file_detail::finish_output(output, file), std::ios_base::failure);
  EXPECT_EQ(0u, file.closes);
}

TEST(wallet_store_output, returned_close_failure_is_not_success)
{
  controlled_file file;
  file.fail_close = true;
  boost::iostreams::filtering_ostream output;
  output.push(inert_filter("", inert_iv));
  output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file));
  output.write("abc", 3);
  EXPECT_THROW(tools::wallet_file_detail::finish_output(output, file), std::ios_base::failure);
  EXPECT_TRUE(output.empty());
  EXPECT_EQ(1u, file.closes);
}

TEST(wallet_store_output, throwing_close_failure_propagates)
{
  controlled_file file;
  file.throw_close = true;
  boost::iostreams::filtering_ostream output;
  output.push(inert_filter("", inert_iv));
  output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file));
  EXPECT_THROW(tools::wallet_file_detail::finish_output(output, file), std::runtime_error);
  EXPECT_TRUE(output.empty());
  EXPECT_EQ(1u, file.closes);
}

TEST(wallet_store_output, existing_stream_error_prevents_finalization)
{
  for (const auto bits : {std::ios_base::failbit, std::ios_base::badbit})
  {
    controlled_file file;
    boost::iostreams::filtering_ostream output;
    output.push(inert_filter("", inert_iv));
    output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file));
    output.setstate(bits);
    EXPECT_THROW(tools::wallet_file_detail::finish_output(output, file), std::ios_base::failure);
    EXPECT_EQ(0u, file.closes);
  }
}

TEST(wallet_store_output, existing_file_error_prevents_finalization)
{
  for (const auto bits : {std::ios_base::failbit, std::ios_base::badbit})
  {
    controlled_file file;
    boost::iostreams::filtering_ostream output;
    output.push(inert_filter("", inert_iv));
    output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file));
    file.setstate(bits);
    EXPECT_THROW(tools::wallet_file_detail::finish_output(output, file), std::ios_base::failure);
    EXPECT_EQ(0u, file.closes);
  }
}


TEST(wallet_store_output, finalization_flushes_processor_once)
{
  controlled_file file;
  boost::iostreams::filtering_ostream output;
  output.push(buffered_filter("", inert_iv));
  output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file));
  output.write("abc", 3);
  ASSERT_NO_THROW(tools::wallet_file_detail::finish_output(output, file));
  EXPECT_EQ("abctail", file.buffer.bytes);
  EXPECT_EQ(1u, file.closes);
}

TEST(wallet_store_output, failed_filter_copy_remains_failed)
{
  inert_filter filter("", inert_iv);
  auto state = std::make_shared<sink_state>();
  state->result = 1;
  controlled_sink sink{state};
  EXPECT_THROW(filter.write(sink, "abc", 3), std::ios_base::failure);
  inert_filter copied(filter);
  state->result = 3;
  EXPECT_THROW(copied.write(sink, "abc", 3), std::ios_base::failure);
  EXPECT_THROW(copied.flush(sink), std::ios_base::failure);
  EXPECT_EQ(1u, state->writes);
  EXPECT_EQ(0u, state->flushes);
}

TEST(wallet_store_output, boundary_buffers_preserve_all_bytes)
{
  for (const auto buffer_size : {0, 1, 64, 4096})
  {
    for (const auto size : {0, 1, 63, 64, 65, 4095, 4096, 4097, 65537})
    {
      controlled_file file;
      std::string bytes(size, 'x');
      for (size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = static_cast<char>(index % 251);
      boost::iostreams::filtering_ostream output;
      output.push(inert_filter("", inert_iv), buffer_size);
      output.push(tools::wallet_file_detail::checked_file_sink<controlled_file>(file), buffer_size);
      for (size_t offset = 0; offset < bytes.size();)
      {
        const auto count = std::min<size_t>(97, bytes.size() - offset);
        output.write(bytes.data() + offset, count);
        offset += count;
      }
      ASSERT_NO_THROW(tools::wallet_file_detail::finish_output(output, file));
      EXPECT_EQ(bytes, file.buffer.bytes);
      EXPECT_EQ(1u, file.closes);
      EXPECT_TRUE(output.empty());
    }
  }
}

namespace
{
  struct fault_buffer : std::streambuf
  {
    std::string bytes;
    size_t writes = 0, syncs = 0, fail_write_at = 0, fail_sync_at = 0;
    bool throwing = false;
    std::streamsize xsputn(const char* data, std::streamsize count) override
    {
      ++writes;
      if (writes == fail_write_at && throwing)
        throw std::runtime_error("inert write failure");
      const auto result = writes == fail_write_at && count ? count - 1 : count;
      bytes.append(data, static_cast<size_t>(result));
      return result;
    }
    int sync() override
    {
      ++syncs;
      if (syncs == fail_sync_at && throwing)
        throw std::runtime_error("inert sync failure");
      return syncs == fail_sync_at ? -1 : 0;
    }
  };
  struct fault_file : std::ostream
  {
    fault_buffer buffer;
    size_t closes = 0;
    fault_file() : std::ostream(nullptr) { rdbuf(&buffer); }
    void close() { ++closes; }
  };
}

TEST(wallet_store_output, faults_at_each_reached_finalization_flush)
{
  size_t reached_flushes = 0;
  {
    fault_file file;
    boost::iostreams::filtering_ostream output;
    output.push(inert_filter("", inert_iv));
    output.push(tools::wallet_file_detail::checked_file_sink<fault_file>(file));
    output.write("abcdef", 6);
    ASSERT_NO_THROW(tools::wallet_file_detail::finish_output(output, file));
    reached_flushes = file.buffer.syncs;
    ASSERT_GT(reached_flushes, 0u);
  }
  for (size_t fail_at = 1; fail_at <= reached_flushes; ++fail_at)
  {
    for (const bool throwing : {false, true})
    {
      fault_file file;
      file.buffer.fail_sync_at = fail_at;
      file.buffer.throwing = throwing;
      {
        boost::iostreams::filtering_ostream output;
        output.push(inert_filter("", inert_iv));
        output.push(tools::wallet_file_detail::checked_file_sink<fault_file>(file));
        output.write("abcdef", 6);
        EXPECT_THROW(tools::wallet_file_detail::finish_output(output, file), std::ios_base::failure);
        EXPECT_GE(file.buffer.syncs, fail_at);
        EXPECT_EQ(0u, file.closes);
      }
      EXPECT_EQ(0u, file.closes);
    }
  }
}

TEST(wallet_store_output, large_output_write_errors_do_not_finalize)
{
  for (size_t fail_at = 1; fail_at <= 3; ++fail_at)
  {
    for (const bool throwing : {false, true})
    {
      fault_file file;
      file.buffer.fail_write_at = fail_at;
      file.buffer.throwing = throwing;
      {
        boost::iostreams::filtering_ostream output;
        output.push(inert_filter("", inert_iv));
        output.push(tools::wallet_file_detail::checked_file_sink<fault_file>(file));
        const std::string bytes(65537, 'x');
        output.write(bytes.data(), bytes.size());
        EXPECT_FALSE(output.good());
        EXPECT_THROW(tools::wallet_file_detail::finish_output(output, file), std::ios_base::failure);
        EXPECT_GE(file.buffer.writes, fail_at);
        EXPECT_EQ(0u, file.closes);
      }
      EXPECT_EQ(0u, file.closes);
    }
  }
}

TEST(wallet_store_output, failed_chain_destructor_preserves_unwinding_exception)
{
  fault_file file;
  bool caught_original = false;
  try
  {
    boost::iostreams::filtering_ostream output;
    output.push(inert_filter("", inert_iv));
    output.push(tools::wallet_file_detail::checked_file_sink<fault_file>(file));
    output.write("pending", 7);
    output.setstate(std::ios_base::badbit);
    file.close();
    file.setstate(std::ios_base::badbit);
    throw std::runtime_error("inert original failure");
  }
  catch (const std::runtime_error& error)
  {
    caught_original = std::string(error.what()) == "inert original failure";
  }
  EXPECT_TRUE(caught_original);
  EXPECT_EQ(1u, file.closes);
}
