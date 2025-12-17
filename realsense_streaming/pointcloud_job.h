#pragma once

#include "concurrency/job_state.h"
#include "client.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "simple_thread_pool.h"

namespace axby {
namespace realsense_streaming {

class PointCloudJob {
   public:
    PointCloudJob(std::shared_ptr<SimpleThreadPool> thread_pool);

    // job must have been started
    // returns true if the job was complete and the result was copied out
    // if return value was true, automatically resets job state
    bool read_results(FastResizableVector<float>& xyzs_rgbcloud_out,
                      FastResizableVector<uint8_t>& rgbs_rgbcloud_out);

    void start(const client::ColorData& color, const client::DepthData& depth);

    uint64_t last_color_sequence_id() const;
    uint64_t last_depth_sequence_id() const;

    bool is_started() { return job_state_.is_started(); }
    bool is_complete() { return job_state_.is_complete(); }
    bool is_none() { return job_state_.is_none(); }

   private:
    client::ColorData color_;
    client::DepthData depth_;
    FastResizableVector<float> xyzs_rgbcloud_;
    FastResizableVector<uint8_t> rgbs_rgbcloud_;
    std::shared_ptr<SimpleThreadPool> thread_pool_;

    // listed last, destructed first
    JobState job_state_;
};

}  // namespace realsense_streaming
}  // namespace axby
