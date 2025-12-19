#include "pointcloud_job.h"
#include "debug/log.h"
#include "math/camera.h"
#include "math/spatial.h"
#include "realsense_streaming/client.h"
#include "rgbd/rgbd.h"
#include "wrappers/eigen.h"

namespace axby {
namespace realsense_streaming {

PointCloudJob::PointCloudJob(std::shared_ptr<SimpleThreadPool> thread_pool)
    : thread_pool_(std::move(thread_pool)) {}

SerialNumber PointCloudJob::last_serial_number() const {
    return color_.stream_meta.id.serial_number;
};

uint64_t PointCloudJob::last_color_sequence_id() const {
    return color_.sequence_id;
};
uint64_t PointCloudJob::last_depth_sequence_id() const {
    return depth_.sequence_id;
};

void PointCloudJob::start(const client::ColorData& color,
                          const client::DepthData& depth) {
    color_ = color;
    depth_ = depth;

    CHECK(job_state_.is_none());
    job_state_.start();

    thread_pool_->Push([this]() {
        std::array<float, 16> depth_camera_matrix;
        make_camera_matrix(
            make_camera_intrinsics(depth_.stream_meta.intrinsics),
            depth_camera_matrix);
        DepthImageInfo depth_info{
            .width = depth_.stream_meta.intrinsics.width,
            .height = depth_.stream_meta.intrinsics.height,
            .depth_scale = depth_.stream_meta.depth_scale,
            .depth_image = depth_.data,
            .hm_image_camera = depth_camera_matrix};

        std::array<float, 16> rgb_camera_matrix;
        make_camera_matrix(
            make_camera_intrinsics(color_.stream_meta.intrinsics),
            rgb_camera_matrix);
        RgbImageInfo rgb_info{.width = color_.stream_meta.intrinsics.width,
                              .height = color_.stream_meta.intrinsics.height,
                              .rgb_image = color_.data,
                              .hm_image_camera = rgb_camera_matrix};

        CM_Matrix4f tx_device_depth(depth_.stream_meta.extrinsics.data());
        CM_Matrix4f tx_device_rgb(color_.stream_meta.extrinsics.data());
        const Eigen::Matrix4f tx_rgb_depth =
            tx_device_rgb.inverse() * tx_device_depth;

        RgbdInfo info{
            .depth = depth_info, .rgb = rgb_info, .tx_rgb_depth = tx_rgb_depth};

        make_xyzs_and_rgbs_from_rgbd(info, {.remove_zeros = true},
                                     xyzs_rgbcloud_, rgbs_rgbcloud_);

        CHECK_EQ(xyzs_rgbcloud_.size() % 3, 0);        
        CHECK_EQ(rgbs_rgbcloud_.size(), xyzs_rgbcloud_.size());

        // convert to the rgb frame
        const int num_points = xyzs_rgbcloud_.size() / 3;
        for (int i = 0; i < num_points; ++i) {
            M_Vector3f xyz(&xyzs_rgbcloud_[3 * i]);
            tx_apply(tx_rgb_depth, xyz, xyz);
        }

        job_state_.complete();
    });
}

bool PointCloudJob::read_results(
    FastResizableVector<float>& xyzs_rgbcloud_out,
    FastResizableVector<uint8_t>& rgbs_rgbcloud_out) {
    CHECK(!job_state_.is_none());
    if (!job_state_.is_complete()) return false;

    std::swap(xyzs_rgbcloud_out, xyzs_rgbcloud_);
    std::swap(rgbs_rgbcloud_out, rgbs_rgbcloud_);
    job_state_.reset();

    return true;
};

}  // namespace realsense_streaming
}  // namespace axby
