#pragma once

#include <optional>

#include "fast_resizable_vector/fast_resizable_vector.h"
#include "seq/seq.h"

namespace axby {

struct DepthImageInfo {
    int width = 0;
    int height = 0;
    float depth_scale = 1.0f;
    Seq<const uint16_t> depth_image;
    Seq<const float> hm_image_camera;  // 4x4 homog
};

struct RgbImageInfo {
    int width = 0;
    int height = 0;
    Seq<const uint8_t> rgb_image;
    Seq<const float> hm_image_camera;  // 4x4 homog
};

struct RgbdInfo {
    DepthImageInfo depth;
    RgbImageInfo rgb;
    Seq<const float> tx_rgb_depth;
};

struct XyzsFromDepthOptions {
    bool remove_zeros = false;
    std::optional<float> min_depth = std::nullopt;
    std::optional<float> max_depth = std::nullopt;
};

// Given:
//   invP  - inverse projection (or inverse MVP) matrix P^{-1}
//   ix,iy - image coordinates (in the same normalized space P uses)
//   z     - known depth in the 3D space of [alpha, beta, gamma]
//
// Returns:
//   (x, y)
//
// This is derived from the system:
//   P^{-1} [ix, iy, lambda, 1]^T = tau [alpha, beta, gamma, 1]^T
// with gamma = z known.
std::array<float, 2> xy_from_depth(Seq<const float> invP_data,
                                   float ix,
                                   float iy,
                                   float z);

// fills xyz_out with 3d xyzs corresponding to the de-projected
// pointcloud in the depth sensor frame
void make_xyzs_from_depth_image(const DepthImageInfo& info,
                                const XyzsFromDepthOptions& options,
                                FastResizableVector<float>& xyzs_out);

// same as the above but restricted to `image_coordinates`
// for invalid xyzs, (0,0,0) is output
void make_xyzs_from_depth_image_coordinates(
    const DepthImageInfo& info,
    Seq<const float> image_coordinates,
    FastResizableVector<float>& xyzs_out);

// tx_rgb_xyzs is the transform that takes the xyz points to the frame
// of the rgb camera. rgbs are (0,0,0) when out of bounds
void make_rgbs_from_xyzs(const RgbImageInfo& info,
                         Seq<const float> tx_rgb_xyzs,
                         Seq<const float> xyzs,
                         FastResizableVector<uint8_t>& out);

void make_xyzs_and_rgbs_from_rgbd(const RgbdInfo& info,
                                  const XyzsFromDepthOptions& options,
                                  FastResizableVector<float>& xyzs,
                                  FastResizableVector<uint8_t>& rgbs);

}  // namespace axby
