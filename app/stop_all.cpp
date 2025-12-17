#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

namespace axby {

std::atomic<bool> stop_{false};

std::mutex on_stop_all_mutex_;
std::vector<std::function<void()>> on_stop_all_;

void stop_all() {
    stop_.store(true, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock{on_stop_all_mutex_};
    for (auto& f : on_stop_all_) {
        f();
    }
    on_stop_all_.clear();
}

bool should_stop_all() { return stop_.load(std::memory_order_relaxed); }

void on_stop_all(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock{on_stop_all_mutex_};
    on_stop_all_.push_back(callback);
};

}  // namespace axby
