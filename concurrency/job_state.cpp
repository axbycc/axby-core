#include "job_state.h"

#include "debug/check.h"
#include "debug/log.h"

namespace axby {

bool JobState::is_none() const { return state == JOB_STATE_NONE; }
bool JobState::is_complete() const { return state == JOB_STATE_COMPLETE; }
bool JobState::is_started() const { return state == JOB_STATE_STARTED; }

// call only from main thread
void JobState::start() {
    CHECK(state == JOB_STATE_NONE);
    state = JOB_STATE_STARTED;
}

// call only from worker thread
void JobState::complete() {
    CHECK(state == JOB_STATE_STARTED);
    state = JOB_STATE_COMPLETE;
}

// call only from main thread
void JobState::reset() {
    CHECK(state == JOB_STATE_COMPLETE);
    state = JOB_STATE_NONE;
}

JobState::~JobState() {
    // JobState is meant to be a member variable of an object that
    // submits to a thread pool. The parent object must not be
    // destroyed while a job is in progress. Make sure the
    // JobState is the last member of the parent object.
    while (state == JOB_STATE_STARTED) {
        LOG_EVERY_T(INFO, 1.0) << "Waiting for jobs to finish";
    }
}
}  // namespace axby
