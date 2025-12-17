#include "concurrency/ring_buffer.h"

#include <thread>

#include "app/timing.h"
#include "debug/log.h"
#include "gtest/gtest.h"

using namespace axby;

TEST(RingBuffer, blocking_empty) {
    RingBuffer<int, 4> ring_buffer;

    bool out_value_valid = false;
    int out_value = 100;
    uint64_t blocked_ms = 0;
    std::thread t{[&]() {
        const uint64_t start_time_ms = get_process_time_ms();
        out_value_valid = ring_buffer.read(&out_value, /*blocking=*/true);
        const uint64_t end_time_ms = get_process_time_ms();
        blocked_ms = end_time_ms - start_time_ms;
    }};
    const int sleep_time_ms = 10;
    sleep_ms(sleep_time_ms);
    ring_buffer.write(123);
    t.join();

    EXPECT_NEAR(blocked_ms, sleep_time_ms, 10);
    EXPECT_EQ(out_value, 123);
    EXPECT_EQ(out_value_valid, true);
}

TEST(RingBuffer, nonblocking_empty) {
    RingBuffer<int, 4> ring_buffer;

    bool out_value_valid = true;
    int out_value = 100;
    uint64_t blocked_ms = 0;
    std::thread t{[&]() {
        const uint64_t start_time_ms = get_process_time_ms();
        out_value_valid = ring_buffer.read(&out_value, /*blocking=*/false);
        const uint64_t end_time_ms = get_process_time_ms();
        blocked_ms = end_time_ms - start_time_ms;
    }};
    const int sleep_time_ms = 10;
    sleep_ms(sleep_time_ms);
    ring_buffer.write(123);
    t.join();

    EXPECT_NEAR(blocked_ms, 0, 10);
    EXPECT_EQ(out_value, 100);
    EXPECT_EQ(out_value_valid, false);
}

TEST(RingBuffer, no_block_when_nonempty) {
    RingBuffer<int, 4> ring_buffer;
    std::vector<int> in_values{123, 456, 789};

    for (int i : in_values) {
        ring_buffer.write(i);
    }

    std::vector<int> out_values;
    std::vector<int> out_values_valid;

    std::thread t{[&]() {
        int out = 0;
        bool out_valid = false;
        out_valid = ring_buffer.read(&out, /*blocking=*/true);
        out_values.push_back(out);
        out_values_valid.push_back(out_valid);
    }};
    t.join();

    for (int i = 0; i < out_values.size(); ++i) {
        EXPECT_EQ(out_values[i], in_values[i]);
    }
}

TEST(RingBuffer, unblocks_on_stop) {
    RingBuffer<int, 4> ring_buffer;

    bool out_value_valid = true;
    int out_value = 100;
    std::thread t{[&]() {
        out_value_valid = ring_buffer.read(&out_value, /*blocking=*/true);
    }};

    ring_buffer.stop();
    t.join();

    EXPECT_FALSE(out_value_valid);
    EXPECT_EQ(out_value, 100);
}
