#pragma once

#include <array>
#include <atomic>
#include <functional>  // std::hash
#include <mutex>
#include <thread>

#include "debug/check.h"
#include "debug/log.h"

namespace axby {

// only thread safe for single producer, single consumer read is
// blocking when queue is empty when stop() is called, any blocked
// reads unblock, and those reads and subsequent ones return
// nullptr/false
template <typename T, int size = 4>
struct RingBuffer {
    std::array<T, size> data;
    std::atomic<int> head = {0};
    std::atomic<int> tail = {0};
    std::atomic<bool> stopped = {false};
    std::atomic<uint64_t> update_counter = {0};

    void stop() {
        stopped = true;
        update_counter += 1;
        update_counter.notify_one();
    }
    bool full() const { return (tail + 1) % size == head; }
    bool empty() const { return head == tail; }

    int num_slots_filled() const {
        int result = tail - head;
        if (result < 0) {
            result += size;
        }
        return result;
    }

    T* begin_write() {
        if (full()) return nullptr;
        return &data[tail];
    }

    void end_write(T* t) {
        CHECK(!full());
        CHECK(&data[tail] == t);
        tail = (tail + 1) % size;

        update_counter.fetch_add(1, std::memory_order_release);
        update_counter.notify_one();
    }

    bool write(const T& t) {
        T* t_in = begin_write();
        if (!t_in) return false;

        *t_in = t;
        end_write(t_in);
        return true;
    }

    bool move_write(T&& in) {
        if (T* t = begin_write()) {
            *t = std::move(in);
            end_write(t);
            return true;
        }
        return false;
    }

    void block_until_stopped_or_nonempty() {
        uint64_t update_counter_old = update_counter;
        while (!stopped && empty()) {
            update_counter.wait(update_counter_old);
            update_counter_old = update_counter;
        }
        CHECK(stopped || !empty());
    }

    T* begin_read(bool blocking) {
        if (blocking) block_until_stopped_or_nonempty();
        if (stopped) return nullptr;
        if (empty()) return nullptr;
        return &data[head];
    }

    T* begin_read_latest(bool blocking) {
        if (blocking) block_until_stopped_or_nonempty();        
        if (empty()) return nullptr;
        if (stopped) return nullptr;
        head = (tail + size - 1) % size;
        return &data[head];
    }

    void end_read(T* t) {
        CHECK(&data[head] == t);
        CHECK(!empty());
        head = (head + 1) % size;
    }

    bool read(T* t_out, bool blocking) {
        CHECK_NOTNULL(t_out);
        T* t = begin_read(blocking);
        if (!t) return false;

        *t_out = *t;

        end_read(t);
        return true;
    }

    bool move_read(T& out, bool blocking) {
        if (T* maybe_t = begin_read(blocking)) {
            out = std::move(*maybe_t);
            end_read(maybe_t);
            return true;
        }
        return false;
    }

    bool read_latest(T* t_out, bool blocking) {
        CHECK_NOTNULL(t_out);
        T* t = begin_read_latest(blocking);
        if (!t) return false;

        *t_out = *t;

        end_read(t);
        return true;
    }

    T* peek_front() {
        if (empty()) return nullptr;
        return &data[head];
    }

    T* peek_back() {
        if (empty()) return nullptr;
        int back_idx = (tail + size - 1) % size;
        return &data[back_idx];
    }

    void clear() {
        int tail_value = tail;
        head = tail_value;
    }

    ~RingBuffer() { stop(); }
};

template <int size = 4>
struct Racer {
    std::atomic<int> head = {0};
    std::atomic<int> tail = {0};
    std::atomic<uint64_t> update_counter = {0};

    bool try_advance_tail() {
        // tail is about to collide with the head so do nothing
        if ((tail + 1) % size == head) return false;

        // advance the tail
        tail = (tail + 1) % size;

        // notify the head advancer if it is blocked
        update_counter.fetch_add(1, std::memory_order_release);
        update_counter.notify_one();
        return true;
    }

    void try_advance_head() {
        uint64_t old_counter = update_counter;
        int orig_head = head;
        int orig_tail = tail;
        bool orig_empty = orig_head == orig_tail;

        if (orig_empty) {
            // head is all caught up to tail. wait for tail to advance
            update_counter.wait(old_counter, std::memory_order_acquire);
        }

        int new_counter = update_counter;
        int new_head = head;
        int new_tail = tail;
        int new_empty = new_head == new_tail;
        CHECK(!new_empty)
            << old_counter << ", " << orig_head << ", " << orig_tail << ", "
            << new_counter << ", " << new_head << ", " << new_tail << ", ";

        head = (head + 1) % size;
    }
};

}  // namespace axby
