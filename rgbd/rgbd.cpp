#include "rgbd.h"

#include <Eigen/Dense>
#include <cmath>
#include <stdexcept>

#include "absl/strings/str_format.h"
#include "debug/check.h"
#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "wrappers/eigen.h"

namespace axby {

std::array<float, 2> xy_from_depth(Seq<const float> invP_data,
                                   float ix,
                                   float iy,
                                   float z) {
    CM_Matrix4f invP(invP_data);

    const float m31 = invP(2, 0);
    const float m32 = invP(2, 1);
    const float m33 = invP(2, 2);
    const float m34 = invP(2, 3);
    const float a3 = m31 * ix + m32 * iy + m34;
    const float b3 = m33;
    const float m41 = invP(3, 0);
    const float m42 = invP(3, 1);
    const float m43 = invP(3, 2);
    const float m44 = invP(3, 3);
    const float a4 = m41 * ix + m42 * iy + m44;
    const float b4 = m43;
    const float denom = b3 - z * b4;
    CHECK_GT(std::fabs(denom), 1e-8f)
        << "singular system for lambda (denom ~ 0).";

    const float lambda = (z * a4 - a3) / denom;
    const float tau = a4 + b4 * lambda;
    CHECK_GT(std::fabs(tau), 1e-8f) << "tau is near zero.";

    const float m11 = invP(0, 0);
    const float m12 = invP(0, 1);
    const float m13 = invP(0, 2);
    const float m14 = invP(0, 3);
    const float a1 = m11 * ix + m12 * iy + m14;
    const float b1 = m13;
    const float alpha = (a1 + b1 * lambda) / tau;
    const float m21 = invP(1, 0);
    const float m22 = invP(1, 1);
    const float m23 = invP(1, 2);
    const float m24 = invP(1, 3);
    const float a2 = m21 * ix + m22 * iy + m24;
    const float b2 = m23;
    const float beta = (a2 + b2 * lambda) / tau;
    return {alpha, beta};
}

void make_xyzs_from_depth_image_coordinates(
    const DepthImageInfo& info,
    Seq<const float> image_coordinates,
    FastResizableVector<float>& xyzs_out) {
    xyzs_out.reserve(info.width * info.height);
    xyzs_out.clear();

    CM_Matrix4f hm_image_camera(info.hm_image_camera);
    Eigen::Matrix4f hm_camera_image = hm_image_camera.inverse();

    const auto PushZeros = [](FastResizableVector<float>& xyzs_out) {
        xyzs_out.push_back(0);
        xyzs_out.push_back(0);
        xyzs_out.push_back(0);
    };

    for (int i = 0; i < image_coordinates.size(); i += 2) {
        const float image_x = image_coordinates[i];
        const float image_y = image_coordinates[i + 1];
        if ((image_x < 0) || (image_y < 0) || (image_x > info.width) ||
            (image_y > info.height)) {
            // out of bounds
            PushZeros(xyzs_out);
            continue;
        }

        const int image_ix = int(image_x);
        const int image_iy = int(image_y);
        const int image_iflat = image_iy * info.width + image_ix;
        CHECK_LT(image_iflat, info.width * info.height);
        const uint16_t zi = info.depth_image[image_iflat];
        if (zi == 0) {
            PushZeros(xyzs_out);
            continue;
        }

        const float z = zi * info.depth_scale;
        const auto xy = xy_from_depth(hm_camera_image, image_x, image_y, z);
        xyzs_out.push_back(xy[0]);
        xyzs_out.push_back(xy[1]);
        xyzs_out.push_back(z);
    }
}

void make_xyzs_from_depth_image(const DepthImageInfo& info,
                                const XyzsFromDepthOptions& options,
                                FastResizableVector<float>& xyzs_out) {
    xyzs_out.reserve(info.width * info.height);
    xyzs_out.clear();

    CM_Matrix4f hm_image_camera(info.hm_image_camera);
    Eigen::Matrix4f hm_camera_image = hm_image_camera.inverse();

    for (int yidx = 0; yidx < info.height; ++yidx) {
        for (int xidx = 0; xidx < info.width; ++xidx) {
            const int idx = xidx + yidx * info.width;
            const uint16_t zi = info.depth_image[idx];

            if (zi == 0) {
                // missing data
                if (options.remove_zeros) {
                    continue;
                } else {
                    xyzs_out.push_back(0);
                    xyzs_out.push_back(0);
                    xyzs_out.push_back(0);
                }
            } else {
                const float z = zi * info.depth_scale;
                if (options.min_depth && (z < *options.min_depth)) continue;
                if (options.max_depth && (z > *options.max_depth)) continue;
                const auto xy =
                    xy_from_depth(hm_camera_image, xidx + 0.5f, yidx + 0.5f, z);
                xyzs_out.push_back(xy[0]);
                xyzs_out.push_back(xy[1]);
                xyzs_out.push_back(z);
            }
        }
    }
}

void make_rgbs_from_xyzs(const RgbImageInfo& info,
                         Seq<const float> tx_rgb_xyzs,
                         Seq<const float> xyzs,
                         FastResizableVector<uint8_t>& out) {
    CHECK_EQ(xyzs.size() % 3, 0);
    const size_t num_points = xyzs.size() / 3;

    out.clear();

    // homogenous transform fro depth frame into the rgb image
    Eigen::Matrix4f hm_rgbimage_depth =
        CM_Matrix4f(info.hm_image_camera) * CM_Matrix4f(tx_rgb_xyzs);

    const auto PushZero = [](FastResizableVector<uint8_t>& out) {
        out.push_back(0);
        out.push_back(0);
        out.push_back(0);
    };

    for (size_t i = 0; i < num_points; ++i) {
        Eigen::Vector4f depth_xyz1;
        depth_xyz1 << xyzs[3 * i], xyzs[3 * i + 1], xyzs[3 * i + 2], 1;

        Eigen::Vector4f rgbimage_xyzw = hm_rgbimage_depth * depth_xyz1;
        rgbimage_xyzw /= rgbimage_xyzw(3);  // de-homogenize

        // pixel coordinates in rgb image
        const int ix = int(rgbimage_xyzw(0));
        const int iy = int(rgbimage_xyzw(1));
        if ((ix < 0) || (ix >= info.width) || (iy < 0) || (iy >= info.height)) {
            // out of bounds
            PushZero(out);
            continue;
        }

        const int flat_idx = 3 * (iy * info.width + ix);
        CHECK_LT(flat_idx, info.rgb_image.size());

        const uint8_t r = info.rgb_image[flat_idx];
        const uint8_t g = info.rgb_image[flat_idx + 1];
        const uint8_t b = info.rgb_image[flat_idx + 2];

        out.push_back(r);
        out.push_back(g);
        out.push_back(b);
    }

    CHECK_EQ(out.size(), 3*num_points);
    CHECK_EQ(out.size(), xyzs.size());
}

void make_xyzs_and_rgbs_from_rgbd(const RgbdInfo& info,
                                  const XyzsFromDepthOptions& options,
                                  FastResizableVector<float>& xyzs,
                                  FastResizableVector<uint8_t>& rgbs) {
    make_xyzs_from_depth_image(info.depth, options, xyzs);
    make_rgbs_from_xyzs(info.rgb, info.tx_rgb_depth, xyzs, rgbs);
};

}  // namespace axby
