#include "app/timing.h"

#include <chrono>
#include <thread>

namespace axby {

const auto _PROCESS_START_TIME_ = std::chrono::steady_clock::now();

void sleep_ms(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void sleep_us(uint32_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

uint64_t get_process_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - _PROCESS_START_TIME_)
        .count();
}

uint64_t get_system_time_ms() {
    using namespace std::chrono;
    // Get the current time point from the system clock
    auto now = system_clock::now();
    // Convert the time point to duration since the epoch, then to milliseconds
    auto duration = now.time_since_epoch();
    auto millis = duration_cast<milliseconds>(duration).count();
    // Cast the duration to uint64_t
    return static_cast<uint64_t>(millis);
}

uint64_t get_process_time_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now() - _PROCESS_START_TIME_)
        .count();
}

FrequencyCalculator::FrequencyCalculator(double momentum) {
    momentum_ = momentum;
    prev_count_us_ = get_process_time_us();
}

void FrequencyCalculator::count(uint64_t cnt) {
    // turn count into frequency
    const uint64_t now_us = get_process_time_us();
    const uint64_t elapsed_us = now_us - prev_count_us_;
    const double elapsed_sec = double(elapsed_us) * 1e-6;

    elapsed_sec_ += elapsed_sec;
    count_ += double(cnt);
    update_frequency();

    prev_count_us_ = now_us;
}

void FrequencyCalculator::update_frequency() {
    if (elapsed_sec_ > 0.03) {
        double current_freq = count_ / elapsed_sec_;
        frequency_ = frequency_ * momentum_ + current_freq * (1.0 - momentum_);
        count_ = 0;
        elapsed_sec_ = 0;
    }
}

double FrequencyCalculator::get_frequency() {
    update_frequency();
    return frequency_;
}

void FrequencyCalculator::reset() {
    elapsed_sec_ = 0;
    count_ = 0;
    prev_count_us_ = get_process_time_us();
}

ActionPeriod::ActionPeriod(double period) : period_(period) {};

double ActionPeriod::get_sec_elapsed() {
    return double(get_process_time_ms() - last_triggered_ms_) / 1000.0;
}

double ActionPeriod::get_period() { return period_; };

bool ActionPeriod::should_act() {
    const uint64_t phase_ms = 1000 * phase_;
    const uint64_t period_ms = 1000 * period_;
    const uint64_t current_time_ms = get_process_time_ms();
    const uint64_t current_idx = (current_time_ms + phase_ms) / period_ms;
    const uint64_t last_idx = (last_triggered_ms_ + phase_ms) / period_ms;

    if (current_idx > last_idx) {
        last_triggered_ms_ = current_time_ms;
        return true;
    }

    return false;
}

void ActionPeriod::set_phase(double seconds) { phase_ = seconds; }

Stopwatch::Stopwatch() { press(); }

double Stopwatch::press() {
    const uint64_t current_timestamp_us = get_process_time_us();
    const double dt = double(current_timestamp_us - last_press_us_) / 1000000.0;
    last_press_us_ = current_timestamp_us;
    return dt;
}

double Stopwatch::get_sec_since_press() const {
    const int current_timestamp_us = get_process_time_us();
    return double(current_timestamp_us - last_press_us_) / 1000000.0;
}

double FpsThrottler::fps() { return fps_.get_frequency(); }

void FpsThrottler::wait_frame() {
    fps_.count();

    double current_fps = fps_.get_frequency();
    if (current_fps < target_fps_) {
        sleep_sec_ *= 0.9;
    } else {
        sleep_sec_ = 1.1 * (sleep_sec_ + 0.001);
    }
    if (sleep_sec_) {
        sleep_ms(sleep_sec_ * 1000);
    }
};

}  // namespace axby
