#include "rgbd.h"

#include <Eigen/Dense>

#include "absl/strings/str_format.h"
#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "gtest/gtest.h"
#include "math/camera.h"
#include "random/linear_congruential_generator.h"
#include "seq/seq.h"
#include "wrappers/eigen.h"

using namespace axby;

TEST(xy_from_depth, identity) {
    Eigen::Matrix4f hm_image_camera = Id4f();
    Eigen::Matrix4f hm_camera_image = hm_image_camera.inverse();

    // Make test data
    LinearCongruentialGenerator lcg;
    const float x = lcg.generate() * 10;
    const float y = lcg.generate() * 10;
    const float z = std::abs(lcg.generate() * 10);

    Eigen::Vector4f point;
    point << x, y, z, 1;
    Eigen::Vector4f image_point = hm_image_camera * point;
    image_point /= image_point(3);

    CHECK_GT(x, 0) << "malformed test";
    CHECK_GT(y, 0) << "malformed test";

    float ix = image_point(0);
    float iy = image_point(1);

    auto out_xy = xy_from_depth(hm_camera_image, ix, iy, z);
    const float out_x = out_xy[0];
    const float out_y = out_xy[1];

    EXPECT_NEAR(out_x, x, 1e-3);
    EXPECT_NEAR(out_y, y, 1e-3);
}

TEST(xy_from_depth, random_intrinsics) {
    const int width = 1024;
    const int height = 1024;
    LinearCongruentialGenerator lcg;
    const float fx = lcg.generate() * float(width) / 2;
    const float fy = lcg.generate() * float(height) / 2;
    const float cx = 10 * lcg.generate() + float(width) / 2;
    const float cy = 10 * lcg.generate() + float(height) / 2;

    Eigen::Matrix4f hm_image_camera;
    make_camera_matrix(/*camera_intrinsics*/ {{fx, fy, cx, cy}},
                       hm_image_camera);
    Eigen::Matrix4f hm_camera_image = hm_image_camera.inverse();

    const float x = lcg.generate() * 10;
    const float y = lcg.generate() * 10;
    const float z = std::abs(lcg.generate() * 10);

    Eigen::Vector4f point;
    point << x, y, z, 1;
    Eigen::Vector4f image_point = hm_image_camera * point;
    image_point /= image_point(3);

    CHECK_GT(x, 0) << "malformed test";
    CHECK_GT(y, 0) << "malformed test";

    float ix = image_point(0);
    float iy = image_point(1);

    auto out_xy = xy_from_depth(hm_camera_image, ix, iy, z);
    const float out_x = out_xy[0];
    const float out_y = out_xy[1];

    EXPECT_NEAR(out_x, x, 1e-3);
    EXPECT_NEAR(out_y, y, 1e-3);
}

TEST(xy_from_depth, random_intrinsics_integer_truncate) {
    const int width = 1024;
    const int height = 1024;
    LinearCongruentialGenerator lcg;
    const float fx = lcg.generate() * float(width) / 2;
    const float fy = lcg.generate() * float(height) / 2;
    const float cx = 10 * lcg.generate() + float(width) / 2;
    const float cy = 10 * lcg.generate() + float(height) / 2;

    Eigen::Matrix4f hm_image_camera;
    make_camera_matrix(/*camera_intrinsics*/ {{fx, fy, cx, cy}},
                       hm_image_camera);
    Eigen::Matrix4f hm_camera_image = hm_image_camera.inverse();

    const float x = lcg.generate() * 10;
    const float y = lcg.generate() * 10;
    const float z = std::abs(lcg.generate() * 10);

    Eigen::Vector4f point;
    point << x, y, z, 1;
    Eigen::Vector4f image_point = hm_image_camera * point;
    image_point /= image_point(3);

    CHECK_GT(x, 0) << "malformed test";
    CHECK_GT(y, 0) << "malformed test";

    int ix = image_point(0);
    int iy = image_point(1);

    auto out_xy = xy_from_depth(hm_camera_image, ix, iy, z);
    const float out_x = out_xy[0];
    const float out_y = out_xy[1];

    EXPECT_NEAR(out_x, x, 0.05);
    EXPECT_NEAR(out_y, y, 0.05);
}

TEST(make_xyzs_from_depth_image, random_intrinsics) {
    int width = 512;
    int height = 512;
    std::vector<uint16_t> depth_image(width * height, 0);
    float depth_scale = 0.001;

    LinearCongruentialGenerator lcg;
    const float fx = lcg.generate() * float(width) / 2;
    const float fy = lcg.generate() * float(height) / 2;
    const float cx = 10 * lcg.generate() + float(width) / 2;
    const float cy = 10 * lcg.generate() + float(height) / 2;

    Eigen::Matrix4f hm_image_camera;
    make_camera_matrix(/*camera_intrinsics*/ {{fx, fy, cx, cy}},
                       hm_image_camera);
    Eigen::Matrix4f hm_camera_image = hm_image_camera.inverse();

    constexpr int num_test_xyzs = 20;
    std::vector<float> xyzs_in;
    std::vector<float> ixys_in;
    for (int i = 0; i < num_test_xyzs; ++i) {
        // Make test data
        const float ix = int(lcg.generate() * width) + 0.5f;
        const float iy = int(lcg.generate() * height) + 0.5f;
        const float iz = int((lcg.generate() * 5.0f + 1.0f) / depth_scale);

        const float z = iz * depth_scale;
        const auto xy = xy_from_depth(hm_camera_image, ix, iy, z);
        const float x = xy[0];
        const float y = xy[1];

        // LOG(INFO) << "image " << absl::StrFormat("%.2f, %.2f, %.2f", ix, iy, iz)
        // << " => world " << absl::StrFormat("%.2f, %.2f, %.2f", x, y, z);

        xyzs_in.push_back(x);
        xyzs_in.push_back(y);
        xyzs_in.push_back(z);

        ixys_in.push_back(ix);
        ixys_in.push_back(iy);

        const int flat_idx = int(iy) * width + int(ix);
        depth_image[flat_idx] = iz;
    }

    FastResizableVector<float> xyzs_out;
    DepthImageInfo info{.width = width,
                        .height = height,
                        .depth_scale = depth_scale,
                        .depth_image = depth_image,
                        .hm_image_camera = hm_image_camera};

    make_xyzs_from_depth_image(info, XyzsFromDepthOptions{.remove_zeros = true},
                               xyzs_out);
    EXPECT_EQ(xyzs_out.size(), xyzs_in.size());

    // check that every input point used to generate the depth image
    // is present in the pointcloud
    int num_matches = 0;
    const int num_valid_xyzs = xyzs_out.size() / 3;
    for (int i = 0; i < num_valid_xyzs; ++i) {
        Eigen::Vector3f point_i(&xyzs_in[3 * i]);
        for (int j = 0; j < num_valid_xyzs; ++j) {
            Eigen::Vector3f point_j(&xyzs_out[3 * j]);
            float ij_dist = (point_i - point_j).norm();
            if (ij_dist < 0.05) {
                num_matches += 1;
                break;
            }
        }
    }
    EXPECT_EQ(num_matches, num_valid_xyzs);
}

TEST(make_xyzs_from_depth_image, random_homog) {
    int width = 512;
    int height = 512;
    std::vector<uint16_t> depth_image(width * height, 0);
    float depth_scale = 0.001;

    LinearCongruentialGenerator lcg;
    Eigen::Matrix4f hm_image_camera;
    for (int i = 0; i < 16; ++i) {
        hm_image_camera.data()[i] = lcg.generate();
    }
    Eigen::Matrix4f hm_camera_image = hm_image_camera.inverse();

    constexpr int num_test_xyzs = 20;
    std::vector<float> xyzs_in;
    std::vector<float> ixys_in;
    for (int i = 0; i < num_test_xyzs; ++i) {
        // Make test data
        const float ix = int(lcg.generate() * width) + 0.5f;
        const float iy = int(lcg.generate() * height) + 0.5f;
        const float iz = int((lcg.generate() * 5.0f + 1.0f) / depth_scale);

        const float z = iz * depth_scale;
        const auto xy = xy_from_depth(hm_camera_image, ix, iy, z);
        const float x = xy[0];
        const float y = xy[1];

        // LOG(INFO) << "image " << absl::StrFormat("%.2f, %.2f, %.2f", ix, iy, iz)
        // << " => world " << absl::StrFormat("%.2f, %.2f, %.2f", x, y, z);

        xyzs_in.push_back(x);
        xyzs_in.push_back(y);
        xyzs_in.push_back(z);

        ixys_in.push_back(ix);
        ixys_in.push_back(iy);

        const int flat_idx = int(iy) * width + int(ix);
        depth_image[flat_idx] = iz;
    }

    FastResizableVector<float> xyzs_out;
    DepthImageInfo info{.width = width,
                        .height = height,
                        .depth_scale = depth_scale,
                        .depth_image = depth_image,
                        .hm_image_camera = hm_image_camera};

    make_xyzs_from_depth_image(info, XyzsFromDepthOptions{.remove_zeros = true},
                               xyzs_out);
    EXPECT_EQ(xyzs_out.size(), xyzs_in.size());

    // check that every input point used to generate the depth image
    // is present in the pointcloud
    int num_matches = 0;
    const int num_valid_xyzs = xyzs_out.size() / 3;
    for (int i = 0; i < num_valid_xyzs; ++i) {
        Eigen::Vector3f point_i(&xyzs_in[3 * i]);
        for (int j = 0; j < num_valid_xyzs; ++j) {
            Eigen::Vector3f point_j(&xyzs_out[3 * j]);
            float ij_dist = (point_i - point_j).norm();
            if (ij_dist < 0.05) {
                num_matches += 1;
                break;
            }
        }
    }
    EXPECT_EQ(num_matches, num_valid_xyzs);
}

TEST(make_xyzs_from_depth_image_coordinates, random_intrinsics) {
    int width = 512;
    int height = 512;
    std::vector<uint16_t> depth_image(width * height, 0);
    float depth_scale = 0.001;

    LinearCongruentialGenerator lcg;
    const float fx = lcg.generate() * float(width) / 2;
    const float fy = lcg.generate() * float(height) / 2;
    const float cx = 10 * lcg.generate() + float(width) / 2;
    const float cy = 10 * lcg.generate() + float(height) / 2;

    Eigen::Matrix4f hm_image_camera;
    make_camera_matrix(/*camera_intrinsics*/ {{fx, fy, cx, cy}},
                       hm_image_camera);
    Eigen::Matrix4f hm_camera_image = hm_image_camera.inverse();

    constexpr int num_test_xyzs = 20;
    std::vector<float> xyzs_in;
    std::vector<float> ixys_in;
    for (int i = 0; i < num_test_xyzs; ++i) {
        // Make test data
        const float ix = int(lcg.generate() * width) + 0.5f;
        const float iy = int(lcg.generate() * height) + 0.5f;
        const float iz = int((lcg.generate() * 5.0f + 1.0f) / depth_scale);

        const float z = iz * depth_scale;
        const auto xy = xy_from_depth(hm_camera_image, ix, iy, z);
        const float x = xy[0];
        const float y = xy[1];

        // LOG(INFO) << "image " << absl::StrFormat("%.2f, %.2f, %.2f", ix, iy, iz)
        // << " => world " << absl::StrFormat("%.2f, %.2f, %.2f", x, y, z);

        xyzs_in.push_back(x);
        xyzs_in.push_back(y);
        xyzs_in.push_back(z);

        ixys_in.push_back(ix);
        ixys_in.push_back(iy);

        const int flat_idx = int(iy) * width + int(ix);
        depth_image[flat_idx] = iz;
    }

    FastResizableVector<float> xyzs_out;
    DepthImageInfo info{.width = width,
                        .height = height,
                        .depth_scale = depth_scale,
                        .depth_image = depth_image,
                        .hm_image_camera = hm_image_camera};

    make_xyzs_from_depth_image_coordinates(info, ixys_in, xyzs_out);
    EXPECT_EQ(xyzs_out.size(), xyzs_in.size());
    EXPECT_EQ(xyzs_out.size() / 3, num_test_xyzs);

    for (int i = 0; i < num_test_xyzs; ++i) {
        Eigen::Vector3f point_i(&xyzs_in[3 * i]);
        Eigen::Vector3f point_j(&xyzs_out[3 * i]);
        float ij_dist = (point_i - point_j).norm();
        EXPECT_LT(ij_dist, 1e-4);
    }
}

TEST(make_rgbs_from_xyzs, id) {
    const int width = 100;
    const int height = 50;

    LinearCongruentialGenerator lcg;
    Eigen::Matrix4f hm_rgbimage_rgbcamera;
    for (int i = 0; i < 16; ++i) {
        hm_rgbimage_rgbcamera.data()[i] = lcg.generate();
    }
    Eigen::Matrix4f hm_rgbcamera_rgbimage = hm_rgbimage_rgbcamera.inverse();

    const int num_test_points = 10;

    std::vector<uint8_t> rgbs;
    std::vector<float> ixys;
    std::vector<float> xyzs;
    for (int i = 0; i < num_test_points; ++i) {
        const float ix = int(lcg.generate() * width) + 0.5;
        const float iy = int(lcg.generate() * height) + 0.5;
        ixys.push_back(ix);
        ixys.push_back(iy);

        const float z = lcg.generate() * 5.0f + 1.0f;
        auto xy = xy_from_depth(hm_rgbcamera_rgbimage, ix, iy, z);
        const float x = xy[0];
        const float y = xy[1];
        xyzs.push_back(x);
        xyzs.push_back(y);
        xyzs.push_back(z);

        uint8_t r = int(lcg.generate() * 256);
        uint8_t g = int(lcg.generate() * 256);
        uint8_t b = int(lcg.generate() * 256);
        rgbs.push_back(r);
        rgbs.push_back(g);
        rgbs.push_back(b);
    }

    std::vector<uint8_t> rgb_image(width * height * 3, 0);
    for (int i = 0; i < num_test_points; i++) {
        const int ix = ixys[2 * i];
        const int iy = ixys[2 * i + 1];
        const uint8_t r = rgbs[3 * i];
        const uint8_t g = rgbs[3 * i + 1];
        const uint8_t b = rgbs[3 * i + 2];
        const int flat_idx = 3 * (iy * width + ix);
        rgb_image[flat_idx] = r;
        rgb_image[flat_idx + 1] = g;
        rgb_image[flat_idx + 2] = b;
    }

    RgbImageInfo info;
    info.width = width;
    info.height = height;
    info.rgb_image = rgb_image;
    info.hm_image_camera = hm_rgbimage_rgbcamera;

    FastResizableVector<uint8_t> rgbs_out;
    make_rgbs_from_xyzs(info, Id4f(), xyzs, rgbs_out);

    EXPECT_EQ(rgbs_out.size(), rgbs.size());
    for (int i = 0; i < rgbs_out.size(); ++i) {
        EXPECT_EQ(rgbs_out[i], rgbs[i]);
    }
}
