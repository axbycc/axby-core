#pragma once
#include <array>
#include <atomic>

#include "debug/check.h"

namespace axby {

struct SlotIdxs {
    uint8_t read_head = 0;
    uint8_t write_head = 0;
    constexpr SlotIdxs(uint8_t r, uint8_t w) : read_head(r), write_head(w) {};
};

// thread safe for single producer single consumer. holds an internal
// array of three slots. at every instant, one slot is labelled R for
// read_head, W for write_head, or 0. The 0 slot functions to pass
// information between reader and writer threads. After a write
// succeeds, the W and 0 slots swap roles. Before a read occurs, the R
// and 0 slots swap roles.
template <typename T>
class SingleItem {
    static constexpr std::array<SlotIdxs, 6> TRANSITION_TABLE = {
        SlotIdxs(1, 2),  // 0: 0_R_W
        SlotIdxs(0, 2),  // 1: R_0_W
        SlotIdxs(0, 1),  // 2: R_W_0
        SlotIdxs(2, 1),  // 3: 0_W_R
        SlotIdxs(2, 0),  // 4: W_0_R
        SlotIdxs(1, 0)   // 5: W_R_0
    };

   public:
    template <typename Callable>
        requires std::same_as<std::invoke_result_t<Callable, T&>, void>
    void write_func(const Callable& C) {
        const auto write_slot = get_write_slot();
        C(slots_[write_slot]);
        have_unread_[write_slot] = true;
        finish_write();
    }

    void write(const T& item) {
        const auto write_slot = get_write_slot();
        slots_[write_slot] = item;
        have_unread_[write_slot] = true;
        finish_write();
    }

    void move_write(T&& item) {
        const auto write_slot = get_write_slot();
        slots_[write_slot] = std::move(item);
        have_unread_[write_slot] = true;
        finish_write();
    }

    void swap_write(T& item) {
        const auto write_slot = get_write_slot();
        std::swap(slots_[write_slot], item);
        have_unread_[write_slot] = true;
        finish_write();
    }

    bool read(T& item, bool blocking) {
        if (!begin_read(blocking)) return false;

        const auto read_slot = get_read_slot();
        item = slots_[read_slot];
        have_unread_[read_slot] = false;

        return true;
    }

    bool swap_read(T& item, bool blocking) {
        if (!begin_read(blocking)) return false;

        const auto read_slot = get_read_slot();
        std::swap(item, slots_[read_slot]);
        have_unread_[read_slot] = false;

        return true;
    }

    void stop() {
        stop_ = true;
        update_counter_ += 1;
        update_counter_.notify_one();
    }

    ~SingleItem() { stop(); }

   private:
    // advance the read slot and return whether the slot has an unread
    // item
    bool begin_read(bool blocking) {
        if (blocking) {
            uint64_t update_counter_old = update_counter_;
            while (true) {
                if (stop_) return false;
                
                begin_read_or_finish_write(/*is_write=*/false);
                const uint8_t read_slot = get_read_slot();
                bool have_unread = have_unread_[read_slot];
                if (have_unread) return true;

                update_counter_.wait(update_counter_old);
                update_counter_old = update_counter_;
            }
            return false; // unreachable
        } else {
            begin_read_or_finish_write(/*is_write=*/false);
            const uint8_t read_slot = get_read_slot();
            const bool have_unread = have_unread_[read_slot];
            return have_unread;
        }
    }

    void finish_write() {
        begin_read_or_finish_write(/*is_write=*/true);
        update_counter_ += 1;
        update_counter_.notify_one();
    }

    uint8_t get_read_slot() {
        const uint8_t state_idx = state_idx_;
        return TRANSITION_TABLE[state_idx].read_head;
    }
    uint8_t get_write_slot() {
        const uint8_t state_idx = state_idx_;
        return TRANSITION_TABLE[state_idx].write_head;
    }

    void begin_read_or_finish_write(bool is_write) {
        // when is_write:
        // state = (state % 2 == 1) ? ((state + 1) % 6) : ((state - 1) % 6)
        //
        // when !is_write (aka is_read):
        // state = (state % 2 == 0) ? ((state + 1) % 6) : ((state - 1) % 6)

        uint8_t expected = state_idx_.load(std::memory_order_relaxed);
        uint8_t desired;
        do {
            if (expected % 2 == uint8_t(is_write)) {
                desired = (expected + 1) % 6;
            } else {
                desired = (expected + 5) % 6;
            }
        } while (!state_idx_.compare_exchange_weak(
            expected, desired,
            std::memory_order_release,  // success memory order
            std::memory_order_relaxed   // failure memory order
            ));
    }

    std::atomic<bool> stop_{false};
    std::atomic<uint64_t> update_counter_{0};
    std::array<T, 3> slots_;
    std::array<bool, 3> have_unread_{false, false, false};
    std::atomic<uint8_t> state_idx_{0};  // index into transition table
};

}  // namespace axby
