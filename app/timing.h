#pragma once

#include <cstdint>

namespace axby {

uint64_t get_process_time_ms();  // process alive time
uint64_t get_process_time_us();  // process alive time
uint64_t get_system_time_ms();   // unix time

void sleep_ms(uint32_t ms);
void sleep_us(uint32_t us);

// utility for subtracting two timestamps to get a duration since we
// use unsigned timestamps, the difference may be negative
inline int64_t safe_minus(uint64_t a, uint64_t b) {
    int64_t result = 0;
    uint64_t temp;
    if (a > b) {
        temp = a - b;
        result = temp;
    } else {
        temp = b - a;
        result = temp;
        result *= -1;
    }
    return result;
}

// usually returns a - b, but returns 0 when b > a
inline uint64_t clipped_minus(uint64_t a, uint64_t b) {
    if (b > a) return 0;
    return a - b;
}

class FrequencyCalculator {
    // exponential moving average for stuff like FPS estimate
   public:
    // momentum between 0 and 1. higher means more biased towards
    // previous measurements
    FrequencyCalculator(double momentum = 0.6);
    void count(uint64_t cnt = 1);
    double get_frequency();  // count per sec
    void reset();

   private:
    void update_frequency();
    double count_ = 0;
    double elapsed_sec_ = 0;
    double frequency_ = 0;
    double momentum_ = 0.1;
    uint64_t prev_count_us_ = 0;
};

class FpsThrottler {
   public:
    FpsThrottler(double target_fps) : target_fps_(target_fps) {};

    // blocks thread, attempting to keep target frame rate
    void wait_frame();
    double fps();

    FrequencyCalculator fps_;
    double target_fps_;
    double sleep_sec_ = 0;
};

class ActionPeriod {
   public:
    ActionPeriod(double seconds);
    bool should_act();
    double get_sec_elapsed();
    double get_period();

    // optional. this is intended to prevent action periods started at
    // the same time from all triggering around the same time, which
    // causes bursty workloads (e.g. triggering keyframe encoding for
    // different video encoding threads)
    void set_phase(double seconds);

   private:
    double period_ = 0;
    double phase_ = 0;
    uint64_t last_triggered_ms_ = 0;
};

class Stopwatch {
   public:
    Stopwatch();

    // returns seconds since last press and resets the last press time
    double press();

    // same as press() but does not reset the last press time
    double get_sec_since_press() const;

   private:
    uint64_t last_press_us_ = 0;
};

}  // namespace axby
