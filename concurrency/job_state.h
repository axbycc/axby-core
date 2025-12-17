#include <atomic>
#include <cstdint>

namespace axby {

// JobState is meant to be a member variable of an object that submits
// to a thread pool. The parent object must not be destroyed while a
// job is in progress. Make sure the JobState is the last member of
// the parent object will ensure this, as the JobState destructor
// busy-waits until the state is none or complete.

struct JobState {
    static constexpr uint8_t JOB_STATE_NONE = 0;
    static constexpr uint8_t JOB_STATE_STARTED = 1;
    static constexpr uint8_t JOB_STATE_COMPLETE = 2;
    std::atomic<uint8_t> state{JOB_STATE_NONE};

    bool is_none() const;
    bool is_complete() const;
    bool is_started() const;

    // call only from main thread, mark job as started
    void start();

    // call only from worker thread, mark job as completed
    void complete();

    // call only from main thread after is_complete() true
    void reset();

    ~JobState();
};

}  // namespace axby
