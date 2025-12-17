#pragma once

#include <array>
#include <span>

namespace axby {

// fx, fy, ppx, ppy
using CameraIntrinsics = std::array<float, 4>;

// sometimes we might get camera parameters represented as a struct
// with named fields fx, fy, ppx, ppy. this helper function converts
// them to contiguous array so that they can be more easily interfaced
// with camera_project()
template <typename T>
CameraIntrinsics make_camera_intrinsics(T&& t) {
    return {t.fx, t.fy, t.ppx, t.ppy};
}

// todo: make this one private? can call convert_hm_image_camera instead
void make_camera_matrix(
    const CameraIntrinsics& intrinsics,  // fx, fy, ppx, ppy
    std::span<float> out /* 3x3 or 4x4 matrix column major */);

// inputs/outputs expected as column major
void camera_matrix_4x4_to_3x3(std::span<const float> in, std::span<float> out);
void camera_matrix_3x3_to_4x4(std::span<const float> in, std::span<float> out);

// the homogenous transform from camera space to image space
// (hm_image_camera) can be presented in 3 different ways
//
//    - length 4: fx, fy, ppx, ppy
//    - length 3*3: column major (3,3) matrix
//    - length 4*4: column major (4,4) matrix

// this function converts any of these types to either the 3*3 or 4*4
// form, depending on the size of out
void convert_hm_image_camera(std::span<const float> hm_image_camera,
                             std::span<float> out);

// convert camera projection matrix into ndc projection matrix where
// ndc coordinates are range (-1,1). hm_image_camera can be (3,3) or
// (4,4) column major. output buffer can be (3,3) or (4,4)
void camera_matrix_to_ndc_matrix(std::span<const float> hm_image_camera,
                                 const int width, const int height,
                                 std::span<float> out);

// convert ndc projection matrix into camera projection matrix where
// ndc coordinates are range (-1,1). hm_image_camera can be (3,3) or
// (4,4) column major. output buffer can be (3,3) or (4,4)
void ndc_matrix_to_camera_matrix(std::span<const float> ndc_image_camera,
                                 const int width, const int height,
                                 std::span<float> out);

void camera_project_many(
    std::span<const float>
        hm_image_camera,  // any valid input to convert_hm_camera
    std::span<const float> tx_camera_body,  // 4x4 homogenous or empty
    std::span<const float> pts_body,        /* size 3*N */
    std::span<float> pts_out /* size 2*N */);

void camera_project(
    std::span<const float>
        hm_image_camera,  // any valid input to convert_hm_camera
    std::span<const float> tx_camera_body,  // 4x4 homogenous or empty
    std::span<const float> pt_body,         /* size 3, or homogenous 4 */
    std::span<float> out /* size 2 or 3 */);

template <typename T>
void camera_project(
    std::span<const float>
        hm_image_camera,  // any valid input to convert_hm_camera
    std::span<const float> tx_camera_body,  // 4x4 homogenous or empty
    std::span<const float> pt_body,         /* size 3, or homogenous 4 */
    T& out /* size 2 or 3 */) {
    camera_project(hm_image_camera, tx_camera_body, pt_body, to_span(out));
};

}
