// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "readwrite_lock.h"

namespace
{
  // The mutex is always kept in a shared_ptr and passed to the threads by value: if a test detects a deadlock,
  // the stuck threads are left behind and the mutex has to outlive the test.
  typedef std::shared_ptr<epee::shared_recursive_mutex> mutex_ptr;

  // a healthy mutex never needs that long, so if a thread is still waiting after this time it's a deadlock
  const std::chrono::seconds c_deadlock_timeout(20);
  // how long a thread is watched to make sure that it's really waiting for the mutex
  const std::chrono::milliseconds c_blocked_timeout(100);

  mutex_ptr make_mutex()
  {
    return std::make_shared<epee::shared_recursive_mutex>();
  }

  std::function<void()> op_lock(mutex_ptr m)            { return [m]() { m->lock(); }; }
  std::function<void()> op_unlock(mutex_ptr m)          { return [m]() { m->unlock(); }; }
  std::function<void()> op_lock_shared(mutex_ptr m)     { return [m]() { m->lock_shared(); }; }
  std::function<void()> op_unlock_shared(mutex_ptr m)   { return [m]() { m->unlock_shared(); }; }
  std::function<bool()> op_try_lock(mutex_ptr m)        { return [m]() { return m->try_lock(); }; }
  std::function<bool()> op_try_lock_shared(mutex_ptr m) { return [m]() { return m->try_lock_shared(); }; }

  // true if the function posted to a test_thread is still running, i.e. the thread is waiting for the mutex
  template<typename result_t>
  bool is_blocked(std::future<result_t>& f)
  {
    return f.wait_for(c_blocked_timeout) == std::future_status::timeout;
  }

  // waits until the function posted to a test_thread is finished, returns its result or rethrows its exception
  template<typename result_t>
  result_t finish(std::future<result_t>& f)
  {
    if (f.wait_for(c_deadlock_timeout) != std::future_status::ready)
      throw std::logic_error("deadlock: the thread is still waiting for the mutex");
    return f.get();
  }

  // A thread that executes the functions posted to it one by one, so a test decides what each thread does and in which order.
  class test_thread
  {
    struct queue
    {
      std::mutex lock;
      std::condition_variable cv;
      std::deque<std::function<void()>> tasks;
      bool stop = false;
    };

    std::shared_ptr<queue> m_queue;
    std::future<void> m_finished;
    std::thread m_thread;

  public:
    test_thread()
      : m_queue(std::make_shared<queue>())
    {
      std::shared_ptr<queue> q = m_queue;
      std::packaged_task<void()> loop([q]()
      {
        for (;;)
        {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lk(q->lock);
            q->cv.wait(lk, [&]() { return q->stop || !q->tasks.empty(); });
            if (q->tasks.empty())
              return;
            task = std::move(q->tasks.front());
            q->tasks.pop_front();
          }
          task();
        }
      });
      m_finished = loop.get_future();
      m_thread = std::thread(std::move(loop));
    }

    ~test_thread()
    {
      {
        std::lock_guard<std::mutex> lk(m_queue->lock);
        m_queue->stop = true;
      }
      m_queue->cv.notify_one();

      if (m_finished.wait_for(c_deadlock_timeout) == std::future_status::ready)
        m_thread.join();
      else
        m_thread.detach(); // the thread is stuck in a deadlock, the test has already failed
    }

    // posts f to the thread, the returned future becomes ready when f is finished
    template<typename callback_t>
    auto post(callback_t f) -> std::future<decltype(f())>
    {
      auto task = std::make_shared<std::packaged_task<decltype(f())()>>(std::move(f));
      auto result = task->get_future();
      {
        std::lock_guard<std::mutex> lk(m_queue->lock);
        m_queue->tasks.push_back([task]() { (*task)(); });
      }
      m_queue->cv.notify_one();
      return result;
    }

    // executes f in the thread and waits until it's finished
    template<typename callback_t>
    auto run(callback_t f) -> decltype(f())
    {
      auto result = post(std::move(f));
      return finish(result);
    }
  };

  // tells if a thread that has no ownership could acquire the exclusive one right now
  bool can_lock(mutex_ptr m)
  {
    return test_thread().run([m]()
    {
      bool r = m->try_lock();
      if (r)
        m->unlock();
      return r;
    });
  }

  // tells if a thread that has no ownership could acquire the shared one right now
  bool can_lock_shared(mutex_ptr m)
  {
    return test_thread().run([m]()
    {
      bool r = m->try_lock_shared();
      if (r)
        m->unlock_shared();
      return r;
    });
  }

  // Waits until some thread starts waiting in lock(): new readers are not let in from that moment.
  // To be used only when nobody has exclusive ownership.
  bool wait_for_waiting_writer(mutex_ptr m)
  {
    const auto deadline = std::chrono::steady_clock::now() + c_deadlock_timeout;
    while (can_lock_shared(m))
    {
      if (std::chrono::steady_clock::now() > deadline)
        return false;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
  }
}

TEST(shared_recursive_mutex, exclusive_ownership)
{
  mutex_ptr m = make_mutex();
  test_thread t;
  ASSERT_TRUE(can_lock(m));
  ASSERT_TRUE(can_lock_shared(m));

  t.run(op_lock(m));
  ASSERT_FALSE(can_lock(m));
  ASSERT_FALSE(can_lock_shared(m));

  // recursion
  t.run(op_lock(m));
  ASSERT_TRUE(t.run(op_try_lock(m)));
  t.run(op_unlock(m));
  t.run(op_unlock(m));
  ASSERT_FALSE(can_lock(m));
  ASSERT_FALSE(can_lock_shared(m));

  t.run(op_unlock(m));
  ASSERT_TRUE(can_lock(m));
  ASSERT_TRUE(can_lock_shared(m));
}

TEST(shared_recursive_mutex, shared_ownership)
{
  mutex_ptr m = make_mutex();
  test_thread t1, t2, t3;

  // readers don't block each other
  t1.run(op_lock_shared(m));
  t2.run(op_lock_shared(m));
  t3.run(op_lock_shared(m));
  ASSERT_TRUE(can_lock_shared(m));
  ASSERT_FALSE(can_lock(m));

  // recursion
  t1.run(op_lock_shared(m));
  ASSERT_TRUE(t1.run(op_try_lock_shared(m)));
  t1.run(op_unlock_shared(m));
  t1.run(op_unlock_shared(m));
  ASSERT_FALSE(can_lock(m));

  // every reader releases its own ownership only, whatever the order is
  t1.run(op_unlock_shared(m));
  ASSERT_FALSE(can_lock(m));
  t3.run(op_unlock_shared(m));
  ASSERT_FALSE(can_lock(m));
  t2.run(op_unlock_shared(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, writers_exclude_each_other)
{
  mutex_ptr m = make_mutex();
  test_thread t1, t2;

  t1.run(op_lock(m));
  std::future<void> locked = t2.post(op_lock(m));
  ASSERT_TRUE(is_blocked(locked));

  t1.run(op_unlock(m));
  finish(locked);
  ASSERT_FALSE(can_lock(m));

  t2.run(op_unlock(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, writer_waits_for_readers_and_goes_before_new_ones)
{
  mutex_ptr m = make_mutex();
  test_thread reader, writer, new_reader;

  reader.run(op_lock_shared(m));
  std::future<void> written = writer.post(op_lock(m));
  ASSERT_TRUE(wait_for_waiting_writer(m));
  ASSERT_TRUE(is_blocked(written));

  // the new reader has to let the waiting writer go first
  std::future<void> read = new_reader.post(op_lock_shared(m));
  ASSERT_TRUE(is_blocked(read));

  // recursion is not affected by the waiting writer, otherwise it would be a deadlock
  reader.run(op_lock_shared(m));
  reader.run(op_unlock_shared(m));
  ASSERT_TRUE(is_blocked(written));

  reader.run(op_unlock_shared(m));
  finish(written);
  ASSERT_TRUE(is_blocked(read));

  writer.run(op_unlock(m));
  finish(read);
  ASSERT_FALSE(can_lock(m));

  new_reader.run(op_unlock_shared(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, shared_inside_exclusive)
{
  mutex_ptr m = make_mutex();
  test_thread t;

  t.run(op_lock(m));
  t.run(op_lock_shared(m));
  t.run(op_unlock_shared(m));
  ASSERT_FALSE(can_lock_shared(m)); // still exclusive

  t.run(op_unlock(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, downgrade)
{
  mutex_ptr m = make_mutex();
  test_thread t;

  // exclusive ownership is released before the shared one that was acquired inside of it
  t.run(op_lock(m));
  t.run(op_lock_shared(m));
  t.run(op_unlock(m));
  ASSERT_TRUE(can_lock_shared(m));
  ASSERT_FALSE(can_lock(m)); // the thread is a reader now

  t.run(op_unlock_shared(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, upgrade)
{
  mutex_ptr m = make_mutex();
  test_thread t;

  t.run(op_lock_shared(m));
  t.run(op_lock(m));
  ASSERT_FALSE(can_lock_shared(m));

  // recursion of both kinds on top of the upgrade
  t.run(op_lock(m));
  t.run(op_lock_shared(m));
  t.run(op_unlock_shared(m));
  t.run(op_unlock(m));
  ASSERT_FALSE(can_lock_shared(m));

  // the thread gets back to the shared ownership it had before the upgrade
  t.run(op_unlock(m));
  ASSERT_TRUE(can_lock_shared(m));
  ASSERT_FALSE(can_lock(m));

  t.run(op_unlock_shared(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, upgrade_waits_for_other_readers)
{
  mutex_ptr m = make_mutex();
  test_thread t1, t2, writer;

  t1.run(op_lock_shared(m));
  t2.run(op_lock_shared(m));

  // a regular writer comes first, but it can't get in until both readers leave
  std::future<void> written = writer.post(op_lock(m));
  ASSERT_TRUE(wait_for_waiting_writer(m));

  std::future<void> upgraded = t1.post(op_lock(m));
  ASSERT_TRUE(is_blocked(upgraded));

  // the other reader is not affected by the pending upgrade until it releases the mutex
  t2.run(op_lock_shared(m));
  t2.run(op_unlock_shared(m));
  ASSERT_TRUE(is_blocked(upgraded));

  // the upgrade is atomic: the writer doesn't get in between
  t2.run(op_unlock_shared(m));
  finish(upgraded);
  ASSERT_TRUE(is_blocked(written));

  t1.run(op_unlock(m));
  ASSERT_TRUE(is_blocked(written)); // t1 is a reader again

  t1.run(op_unlock_shared(m));
  finish(written);
  writer.run(op_unlock(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, upgrade_while_writer_is_waiting)
{
  mutex_ptr m = make_mutex();
  test_thread reader, writer;

  reader.run(op_lock_shared(m));
  std::future<void> written = writer.post(op_lock(m));
  ASSERT_TRUE(wait_for_waiting_writer(m));

  // the writer waits for the reader, so the reader must not wait for the writer
  reader.run(op_lock(m));
  ASSERT_TRUE(is_blocked(written));

  reader.run(op_unlock(m));
  reader.run(op_unlock_shared(m));
  finish(written);
  writer.run(op_unlock(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, concurrent_upgrades_dont_deadlock)
{
  mutex_ptr m = make_mutex();
  test_thread t1, t2;

  t1.run(op_lock_shared(m));
  t2.run(op_lock_shared(m));

  std::future<void> upgraded = t1.post(op_lock(m));
  ASSERT_TRUE(wait_for_waiting_writer(m));
  ASSERT_TRUE(is_blocked(upgraded));

  // t1 waits for t2 to release the mutex, so t2 can't wait for t1: it's refused and keeps its shared ownership
  ASSERT_FALSE(t2.run(op_try_lock(m)));
  ASSERT_THROW(t2.run(op_lock(m)), std::runtime_error);
  ASSERT_TRUE(is_blocked(upgraded));

  t2.run(op_unlock_shared(m));
  finish(upgraded);
  ASSERT_FALSE(can_lock_shared(m));

  // nothing is left behind by the refused upgrade
  t1.run(op_unlock(m));
  t1.run(op_unlock_shared(m));
  ASSERT_TRUE(can_lock(m));
  t2.run(op_lock_shared(m));
  t2.run(op_lock(m));
  t2.run(op_unlock(m));
  t2.run(op_unlock_shared(m));
  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, try_lock)
{
  mutex_ptr m = make_mutex();
  test_thread t1, t2;

  // free mutex
  ASSERT_TRUE(t1.run(op_try_lock(m)));
  ASSERT_FALSE(t2.run(op_try_lock(m)));
  ASSERT_FALSE(t2.run(op_try_lock_shared(m)));
  ASSERT_TRUE(t1.run(op_try_lock_shared(m))); // shared inside exclusive
  t1.run(op_unlock_shared(m));
  t1.run(op_unlock(m));

  // upgrade is possible only for a single reader
  ASSERT_TRUE(t1.run(op_try_lock_shared(m)));
  ASSERT_TRUE(t2.run(op_try_lock_shared(m)));
  ASSERT_FALSE(t1.run(op_try_lock(m)));
  t2.run(op_unlock_shared(m));
  ASSERT_TRUE(t1.run(op_try_lock(m)));
  t1.run(op_unlock(m));
  t1.run(op_unlock_shared(m));

  // a failed try_lock leaves nothing behind
  ASSERT_TRUE(can_lock(m));
  ASSERT_TRUE(can_lock_shared(m));
}

TEST(shared_recursive_mutex, is_locked_shared_by_this_thread)
{
  mutex_ptr m = make_mutex();
  test_thread t1, t2;
  auto is_reader = [m]() { return m->is_locked_shared_by_this_thread(); };

  ASSERT_FALSE(t1.run(is_reader));

  // shared ownership belongs to the thread that acquired it
  t1.run(op_lock_shared(m));
  ASSERT_TRUE(t1.run(is_reader));
  ASSERT_FALSE(t2.run(is_reader));

  // recursion: the thread is a reader until the last unlock_shared()
  t1.run(op_lock_shared(m));
  t1.run(op_unlock_shared(m));
  ASSERT_TRUE(t1.run(is_reader));
  t1.run(op_unlock_shared(m));
  ASSERT_FALSE(t1.run(is_reader));

  // exclusive ownership alone doesn't count, shared ownership inside of it does, also after a downgrade
  t1.run(op_lock(m));
  ASSERT_FALSE(t1.run(is_reader));
  t1.run(op_lock_shared(m));
  ASSERT_TRUE(t1.run(is_reader));
  t1.run(op_unlock(m));
  ASSERT_TRUE(t1.run(is_reader));
  t1.run(op_unlock_shared(m));
  ASSERT_FALSE(t1.run(is_reader));

  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, unlock_without_ownership)
{
  mutex_ptr m = make_mutex();
  test_thread t1, t2;

  ASSERT_THROW(t1.run(op_unlock(m)), std::runtime_error);
  ASSERT_THROW(t1.run(op_unlock_shared(m)), std::runtime_error);

  // a reader can release neither exclusive ownership nor shared ownership of another thread
  t1.run(op_lock_shared(m));
  ASSERT_THROW(t1.run(op_unlock(m)), std::runtime_error);
  ASSERT_THROW(t2.run(op_unlock_shared(m)), std::runtime_error);
  ASSERT_TRUE(can_lock_shared(m));
  ASSERT_FALSE(can_lock(m));
  t1.run(op_unlock_shared(m));

  // the same about a writer
  t1.run(op_lock(m));
  ASSERT_THROW(t1.run(op_unlock_shared(m)), std::runtime_error);
  ASSERT_THROW(t2.run(op_unlock(m)), std::runtime_error);
  ASSERT_FALSE(can_lock_shared(m));
  t1.run(op_unlock(m));

  ASSERT_TRUE(can_lock(m));
}

TEST(shared_recursive_mutex, lock_guards)
{
  struct context
  {
    epee::shared_recursive_mutex rw_lock;
    epee::shared_membership<epee::shared_recursive_mutex> read_lock;

    context()
      : read_lock(rw_lock)
    {
    }
  };
  std::shared_ptr<context> c = std::make_shared<context>();
  mutex_ptr m(c, &c->rw_lock);
  test_thread t;

  // returns the number of the first failed check, 0 if all of them are passed
  int failed_check = t.run([c, m]()
  {
    {
      // the way it's used by blockchain_storage: write, then read
      CRITICAL_REGION_LOCAL(c->rw_lock);
      CRITICAL_REGION_LOCAL1(c->read_lock);
      if (can_lock_shared(m))
        return 1;
    }
    if (!can_lock(m))
      return 2;

    {
      // read, then write
      CRITICAL_REGION_LOCAL(c->read_lock);
      if (!can_lock_shared(m) || can_lock(m))
        return 3;
      CRITICAL_REGION_LOCAL1(c->rw_lock);
      if (can_lock_shared(m))
        return 4;
    }
    if (!can_lock(m))
      return 5;

    {
      // standard guards
      std::shared_lock<epee::shared_recursive_mutex> shared(c->rw_lock);
      std::unique_lock<epee::shared_recursive_mutex> exclusive(c->rw_lock);
      if (can_lock_shared(m))
        return 6;
      exclusive.unlock();
      if (!can_lock_shared(m) || can_lock(m))
        return 7;
      std::unique_lock<epee::shared_recursive_mutex> exclusive_again(c->rw_lock, std::try_to_lock);
      std::shared_lock<epee::shared_recursive_mutex> shared_again(c->rw_lock, std::try_to_lock);
      if (!exclusive_again.owns_lock() || !shared_again.owns_lock())
        return 8;
    }
    if (!can_lock(m))
      return 9;

    return 0;
  });
  ASSERT_EQ(0, failed_check);
}

namespace
{
  struct stress_context
  {
    epee::shared_recursive_mutex mutex;
    // protected by the mutex, a == b unless a writer is in the middle of an update
    uint64_t a = 0;
    uint64_t b = 0;

    std::atomic<uint64_t> writes{0};
    std::atomic<uint64_t> upgrades{0};
    std::atomic<uint64_t> refused_upgrades{0};
    std::atomic<bool> failed{false};
    std::mutex error_lock;
    std::string error;

    void check(bool condition, const char* message)
    {
      if (condition)
        return;
      std::lock_guard<std::mutex> lk(error_lock);
      if (!failed.exchange(true))
        error = message;
    }

    // to be called with exclusive ownership only
    void write()
    {
      check(a == b, "a writer sees a half-done update of another writer");
      ++a;
      std::this_thread::yield();
      ++b;
      ++writes;
    }

    // to be called with any ownership
    void read()
    {
      const uint64_t value = a;
      check(value == b, "a reader sees a half-done update");
      std::this_thread::yield();
      check(a == value && b == value, "data has been changed while a reader had the mutex");
    }
  };

  void run_stress_scenarios(std::shared_ptr<stress_context> c, size_t thread_index, size_t iterations, bool can_upgrade)
  {
    uint64_t random = 0x9E3779B97F4A7C15ull * (thread_index + 1);
    for (size_t i = 0; i != iterations && !c->failed; ++i)
    {
      random ^= random << 13;
      random ^= random >> 7;
      random ^= random << 17;
      switch (random % 7)
      {
      case 0: // reader with recursion
        c->mutex.lock_shared();
        c->read();
        c->mutex.lock_shared();
        c->read();
        c->mutex.unlock_shared();
        c->mutex.unlock_shared();
        break;

      case 1: // writer with recursion and a reader inside
        c->mutex.lock();
        c->write();
        c->mutex.lock();
        c->mutex.lock_shared();
        c->write();
        c->read();
        c->mutex.unlock_shared();
        c->mutex.unlock();
        c->mutex.unlock();
        break;

      case 2: // downgrade
        c->mutex.lock();
        c->write();
        c->mutex.lock_shared();
        c->mutex.unlock();
        c->read();
        c->mutex.unlock_shared();
        break;

      case 3: // upgrade
        if (!can_upgrade)
          break;
        c->mutex.lock_shared();
        {
          const uint64_t value = c->a;
          bool upgraded = true;
          try
          {
            c->mutex.lock();
          }
          catch (const std::runtime_error&)
          {
            upgraded = false;
            ++c->refused_upgrades;
          }
          if (upgraded)
          {
            ++c->upgrades;
            c->check(c->a == value && c->b == value, "data has been changed during an upgrade");
            c->write();
            c->mutex.unlock();
            c->read();
          }
        }
        c->mutex.unlock_shared();
        break;

      case 4: // try_lock
        if (c->mutex.try_lock())
        {
          c->write();
          c->mutex.unlock();
        }
        break;

      case 5: // try_lock_shared, then try to upgrade
        if (c->mutex.try_lock_shared())
        {
          c->read();
          if (c->mutex.try_lock())
          {
            c->write();
            c->mutex.unlock();
          }
          c->read();
          c->mutex.unlock_shared();
        }
        break;

      case 6: // plain reader
        c->mutex.lock_shared();
        c->read();
        c->mutex.unlock_shared();
        break;
      }
    }
  }

  void stress_thread(std::shared_ptr<stress_context> c, size_t thread_index, size_t iterations, bool can_upgrade)
  {
    try
    {
      run_stress_scenarios(c, thread_index, iterations, can_upgrade);
    }
    catch (const std::exception& e)
    {
      // only lock() of an upgrading thread may throw, and that is handled in place
      c->check(false, e.what());
    }
  }

  // runs threads_count threads, the first upgraders_count of them upgrade shared ownership to exclusive with lock()
  std::shared_ptr<stress_context> run_stress_test(size_t threads_count, size_t upgraders_count, size_t iterations)
  {
    std::shared_ptr<stress_context> c = std::make_shared<stress_context>();
    std::vector<std::thread> threads;
    std::vector<std::future<void>> finished;
    for (size_t i = 0; i != threads_count; ++i)
    {
      std::packaged_task<void()> task(std::bind(&stress_thread, c, i, iterations, i < upgraders_count));
      finished.push_back(task.get_future());
      threads.emplace_back(std::move(task));
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(5);
    for (size_t i = 0; i != threads_count; ++i)
    {
      if (finished[i].wait_until(deadline) == std::future_status::ready)
      {
        threads[i].join();
      }
      else
      {
        c->check(false, "deadlock: a thread is still waiting for the mutex");
        threads[i].detach();
      }
    }

    c->check(c->a == c->b && c->a == c->writes, "some updates have been lost");
    return c;
  }
}

TEST(shared_recursive_mutex, stress_test)
{
  // Two threads upgrade with lock(), so some of the upgrades are refused, but nothing must break or hang.
  // Every refused upgrade is logged as an error, that's why this test is shorter than the next one.
  std::shared_ptr<stress_context> c = run_stress_test(8, 2, 5000);
  ASSERT_FALSE(c->failed.load()) << c->error;
  ASSERT_TRUE(c->upgrades.load() != 0);
  std::cout << "writes: " << c->writes.load() << ", upgrades: " << c->upgrades.load() << ", refused upgrades: " << c->refused_upgrades.load() << std::endl;
}

TEST(shared_recursive_mutex, stress_test_single_upgrader)
{
  // The way the mutex is used with DB: only one thread at a time (the one that has an open write transaction)
  // acquires exclusive ownership with lock() while it may already have the shared one. Such an upgrade is never refused.
  std::shared_ptr<stress_context> c = run_stress_test(8, 1, 20000);
  ASSERT_FALSE(c->failed.load()) << c->error;
  ASSERT_TRUE(c->upgrades.load() != 0);
  ASSERT_TRUE(c->refused_upgrades.load() == 0);
  std::cout << "writes: " << c->writes.load() << ", upgrades: " << c->upgrades.load() << std::endl;
}
