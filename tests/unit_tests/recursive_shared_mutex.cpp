#include <gtest/gtest.h>

#include <atomic>
#include <random>
#include <stdexcept>
#include <vector>

#include <boost/thread.hpp>

#include "common/recursive_shared_mutex.h"

TEST(recursive_shared_mutex, single_thread_write_recursion)
{
    tools::recursive_shared_mutex lock;
    lock.lock();
    lock.lock();
    lock.unlock();
    lock.unlock();
}

TEST(recursive_shared_mutex, single_thread_read_recursion)
{
    tools::recursive_shared_mutex lock;
    lock.lock_shared();
    lock.lock_shared();
    lock.unlock_shared();
    lock.unlock_shared();
}

TEST(recursive_shared_mutex, write_implies_read)
{
    tools::recursive_shared_mutex lock;
    lock.lock();
    lock.lock_shared();
    lock.unlock_shared();
    lock.unlock();
}

TEST(recursive_shared_mutex, try_lock_uncontended)
{
    tools::recursive_shared_mutex lock;
    ASSERT_TRUE(lock.try_lock());
    lock.unlock();

    ASSERT_TRUE(lock.try_lock_shared());
    lock.unlock_shared();
}

TEST(recursive_shared_mutex, upgrade_shared_to_exclusive_throws)
{
    tools::recursive_shared_mutex lock;
    lock.lock_shared();
    EXPECT_THROW(lock.lock(), std::runtime_error);
    lock.unlock_shared();
}

TEST(recursive_shared_mutex, try_upgrade_shared_to_exclusive_throws)
{
    tools::recursive_shared_mutex lock;
    lock.lock_shared();
    EXPECT_THROW(lock.try_lock(), std::runtime_error);
    lock.unlock_shared();
}

TEST(recursive_shared_mutex, stray_unlock_throws)
{
    tools::recursive_shared_mutex lock;
    EXPECT_THROW(lock.unlock(), std::runtime_error);
}

TEST(recursive_shared_mutex, stray_unlock_shared_throws)
{
    tools::recursive_shared_mutex lock;
    EXPECT_THROW(lock.unlock_shared(), std::runtime_error);
}

TEST(recursive_shared_mutex, unlock_while_holding_shared_only_throws)
{
    tools::recursive_shared_mutex lock;
    lock.lock_shared();
    EXPECT_THROW(lock.unlock(), std::runtime_error);
    lock.unlock_shared();
}

TEST(recursive_shared_mutex, unlock_shared_while_holding_exclusive_throws)
{
    tools::recursive_shared_mutex lock;
    lock.lock();
    EXPECT_THROW(lock.unlock_shared(), std::runtime_error);
    lock.unlock();
}

TEST(recursive_shared_mutex, unlock_shared_nested_under_exclusive_does_not_throw)
{
    tools::recursive_shared_mutex lock;
    lock.lock();
    lock.lock_shared();
    lock.unlock_shared(); // not the outermost release, so this must not be mistaken for a mismatch
    lock.unlock();
}

TEST(recursive_shared_mutex, misuse_does_not_poison_subsequent_use)
{
    tools::recursive_shared_mutex lock;
    EXPECT_THROW(lock.unlock(), std::runtime_error);

    // a prior stray unlock() must not leave the thread's counter corrupted
    lock.lock();
    lock.unlock();

    lock.lock_shared();
    lock.unlock_shared();
}

TEST(recursive_shared_mutex, concurrent_readers_do_not_block_each_other)
{
    tools::recursive_shared_mutex lock;
    lock.lock_shared();

    std::atomic<bool> acquired(false);
    boost::thread other([&lock, &acquired]()
    {
        lock.lock_shared();
        acquired = true;
        lock.unlock_shared();
    });

    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    EXPECT_TRUE(acquired.load());

    lock.unlock_shared();
    other.join();
}

namespace
{
    constexpr int reading_cycles_min = 2;
    constexpr int reading_cycles_max = 10;
    constexpr int reading_step_duration_min = 1;
    constexpr int reading_step_duration_max = 4;

    constexpr int writing_cycles_min = 2;
    constexpr int writing_cycles_max = 10;
    constexpr int writing_step_duration_min = 1;
    constexpr int writing_step_duration_max = 4;

    void reader(tools::recursive_shared_mutex &lock);
    void writer(tools::recursive_shared_mutex &lock);

    void reader(tools::recursive_shared_mutex &lock)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<int> cycles_dist(reading_cycles_min, reading_cycles_max);
        std::uniform_int_distribution<int> duration_dist(reading_step_duration_min, reading_step_duration_max);
        const int reading_cycles = cycles_dist(rng);

        lock.lock_shared();
        for (int i = 0; i < reading_cycles; ++i)
            boost::this_thread::sleep_for(boost::chrono::milliseconds(duration_dist(rng)));

        // a shared holder must never recurse into writer(): shared -> exclusive upgrade is
        // unsupported and is now a hard failure, not silent mutual-exclusion corruption
        const bool recurse = std::uniform_int_distribution<int>(0, 3)(rng) == 0; // ~25%
        if (recurse)
            reader(lock);
        lock.unlock_shared();
    }

    void writer(tools::recursive_shared_mutex &lock)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<int> cycles_dist(writing_cycles_min, writing_cycles_max);
        std::uniform_int_distribution<int> duration_dist(writing_step_duration_min, writing_step_duration_max);
        const int writing_cycles = cycles_dist(rng);

        lock.lock();
        for (int i = 0; i < writing_cycles; ++i)
            boost::this_thread::sleep_for(boost::chrono::milliseconds(duration_dist(rng)));

        const bool recurse = std::uniform_int_distribution<int>(0, 3)(rng) == 0; // ~25%
        if (recurse)
        {
            if (std::uniform_int_distribution<int>(0, 1)(rng))
                writer(lock);
            else
                reader(lock);
        }
        lock.unlock();
    }

    void run_deadlock_stress_test(size_t num_threads)
    {
        tools::recursive_shared_mutex lock;
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<int> which_dist(0, 1);

        std::vector<boost::thread> threads;
        threads.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            if (which_dist(rng))
                threads.emplace_back(reader, std::ref(lock));
            else
                threads.emplace_back(writer, std::ref(lock));
        }

        for (auto &thread : threads)
            thread.join();
    }
} // namespace

TEST(recursive_shared_mutex, deadlock_stress_4_threads)
{
    run_deadlock_stress_test(4);
}

TEST(recursive_shared_mutex, deadlock_stress_16_threads)
{
    run_deadlock_stress_test(16);
}

TEST(recursive_shared_mutex, deadlock_stress_64_threads)
{
    run_deadlock_stress_test(64);
}
