#include "camera.h"

#include <Eigen/Dense>

#include "wrappers/eigen.h"
#include "debug/log.h"
#include "seq/seq.h"

namespace axby {

void make_camera_matrix(const CameraIntrinsics& intrinsics,
                        std::span<float> out) {
    const auto [fx, fy, ppx, ppy] = intrinsics;
    if (out.size() == 3 * 3) {
        Eigen::Map<Eigen::Matrix3f> out_wrap(out.data());
        out_wrap <<
            // clang-format off
	    fx, 0, ppx,
	    0, fy, ppy,
	    0, 0, 1;
        // clang-format on
    } else if (out.size() == 4 * 4) {
	float near = 0.001f;
	float far = 100.0f;
	float alpha = 1.0f/(1.0f - near/far);

        Eigen::Map<Eigen::Matrix4f> out_wrap(out.data());
        out_wrap <<
            // clang-format off
	    fx, 0,  ppx, 0,
	    0, fy,  ppy, 0,
	    0, 0, alpha, -near*alpha, // near and far plane related params
	    0, 0,     1, 0;
        // clang-format on
    } else {
        LOG(FATAL) << "invalid output size " << out.size();
    }
};

void camera_matrix_4x4_to_3x3(std::span<const float> in, std::span<float> out) {
    CHECK(in.size() == 4 * 4);
    CHECK(out.size() == 3 * 3);

    CM_Matrix4f in_matrix(in.data());
    M_Matrix3f out_matrix(out.data());

    out_matrix = in_matrix.block<3, 3>(0, 0);
};

void camera_matrix_3x3_to_4x4(std::span<const float> in, std::span<float> out) {
    CHECK(in.size() == 3 * 3);
    CHECK(out.size() == 4 * 4);

    CM_Matrix3f in_matrix(in.data());
    M_Matrix4f out_matrix(out.data());

    out_matrix.setZero();
    out_matrix.block<3, 3>(0, 0) = in_matrix;

    // near and far plane related parameters
    out_matrix(3, 2) = 1;
    out_matrix(2, 3) = -0.0001;
};

void convert_hm_image_camera(std::span<const float> shm_image_camera,
                             std::span<float> out) {
    if (shm_image_camera.size() == 4) {
        const float fx = shm_image_camera[0];
        const float fy = shm_image_camera[1];
        const float ppx = shm_image_camera[2];
        const float ppy = shm_image_camera[3];
        make_camera_matrix({fx, fy, ppx, ppy}, out);
    } else if (shm_image_camera.size() == 3 * 3) {
        if (out.size() == 3 * 3) {
            axby::seq_copy(shm_image_camera, out);
        } else if (out.size() == 4 * 4) {
            camera_matrix_3x3_to_4x4(shm_image_camera, out);
        } else {
            LOG(FATAL) << "unsupported out size " << out.size();
        }
    } else if (shm_image_camera.size() == 4 * 4) {
        if (out.size() == 4 * 4) {
            axby::seq_copy(shm_image_camera, out);
        } else if (out.size() == 3 * 3) {
            camera_matrix_4x4_to_3x3(shm_image_camera, out);
        } else {
            LOG(FATAL) << "unsupported out size " << out.size();
        }
    }
};

void camera_project_impl(const Eigen::Matrix4f& hm_image_camera,
                         const Eigen::Matrix4f& tx_camera_body,
                         std::span<const float> span_pt_body,
                         std::span<float> out) {
    CHECK(span_pt_body.size() == 3 || span_pt_body.size() == 4);
    CHECK(out.size() == 2 || out.size() == 3);

    Eigen::Vector4f pt_body;
    pt_body << span_pt_body[0], span_pt_body[1], span_pt_body[2], 1;

    Eigen::Vector4f result = hm_image_camera * tx_camera_body * pt_body;
    result /= result(3);
    out[0] = result(0);
    out[1] = result(1);
    if (out.size() == 3) {
        out[2] = result(2);
    }
}

void camera_project(std::span<const float> span_hm_image_camera,
                    std::span<const float> span_tx_camera_body,
                    std::span<const float> span_pt_body, std::span<float> out) {
    CHECK(span_tx_camera_body.size() == 4 * 4 ||
          span_tx_camera_body.size() == 0);

    Eigen::Matrix4f tx_camera_body = Id4f();
    if (!span_tx_camera_body.empty()) {
        tx_camera_body = CM_Matrix4f(span_tx_camera_body.data());
    }

    Eigen::Matrix4f hm_image_camera;
    convert_hm_image_camera(span_hm_image_camera, hm_image_camera);
    camera_project_impl(hm_image_camera, tx_camera_body, span_pt_body, out);
};

void camera_project_many(
    std::span<const float> span_hm_image_camera,
    std::span<const float> span_tx_camera_body,  // 4x4 homogenous or empty
    std::span<const float> pts_body,             /* size 3*N */
    std::span<float> pts_out /* size 2*N */) {
    CHECK(span_tx_camera_body.size() == 4 * 4 ||
          span_tx_camera_body.size() == 0);
    CHECK(pts_body.size() % 3 == 0);
    CHECK(pts_out.size() % 2 == 0);

    const int num_points = pts_body.size() / 3;
    const int num_points_alt = pts_out.size() / 2;
    CHECK(num_points == num_points_alt);

    Eigen::Matrix4f tx_camera_body = Id4f();
    if (!span_tx_camera_body.empty()) {
        tx_camera_body = CM_Matrix4f(span_tx_camera_body.data());
    }

    Eigen::Matrix4f hm_image_camera;
    convert_hm_image_camera(span_hm_image_camera, to_span(hm_image_camera));

    for (int i = 0; i < num_points; ++i) {
        camera_project_impl(hm_image_camera, tx_camera_body,
                            pts_body.subspan(3 * i, 3),
                            pts_out.subspan(2 * i, 2));
    }
};

void camera_matrix_to_ndc_matrix(std::span<const float> hm_image_camera,
                                 const int width, const int height,
                                 std::span<float> ndc_image_camera) {
    CHECK(width > 0);
    CHECK(height > 0);
    CHECK(ndc_image_camera.size() == 3 * 3 || ndc_image_camera.size() == 4 * 4);

    /*
      The map that takes (x,y,z,1) => (2x/w - 1, 2y/w - 1, z, 1)
      ⎡2/w,   0,  0, -1⎤
      ⎢0,   2/h,  0, -1⎥
      ⎢0,     0,  1,  0⎥
      ⎣0,     0,  0,  1⎦

      The map that takes (x,y,1) => (2x/w - 1, 2y/w - 1, 1)
      ⎡2/w,   0, -1⎤
      ⎢0,   2/h, -1⎥
      ⎣0,     0,  1⎦

      We manually premultiply one of these matrices depending on
      whether hm_image_camera is 3x3 or 4x4
    */

    // conversion from 3x3 <=> 4x4 requires temporary
    const bool need_dimension_conversion =
        (ndc_image_camera.size() != hm_image_camera.size());

    if (hm_image_camera.size() == 4 * 4) {
        std::array<float, 4 * 4> temp44;
        CM_Matrix4f in(hm_image_camera.data());

        // if we need dimension conversion, write the results into a
        // temporary 4x4 buffer. otherwise directly write to the
        // output buffer.
        M_Matrix4f out(need_dimension_conversion ? temp44.data()
                                                 : ndc_image_camera.data());

        // manual "sparse matrix multiplication"
        out.row(0) = 2.0f / width * in.row(0) - in.row(3);
        out.row(1) = 2.0f / height * in.row(1) - in.row(3);
        out.row(2) = in.row(2);
        out.row(3) = in.row(3);

        if (need_dimension_conversion) {
            // apply dimension conversion
            camera_matrix_4x4_to_3x3(temp44, ndc_image_camera);
        }
    } else if (hm_image_camera.size() == 3 * 3) {
        // same as the previous case, except for (3,3) input matrix
        std::array<float, 3 * 3> temp33;

        CM_Matrix3f in(hm_image_camera.data());
        M_Matrix3f out(need_dimension_conversion ? temp33.data()
                                                 : ndc_image_camera.data());

        // manual "sparse matrix multiplication"
        out.row(0) = 2.0f / width * in.row(0) - in.row(2);
        out.row(1) = 2.0f / height * in.row(1) - in.row(2);
        out.row(2) = in.row(2);

        if (need_dimension_conversion) {
            camera_matrix_3x3_to_4x4(temp33, ndc_image_camera);
        }
    } else {
        LOG(FATAL) << "Unsupported matrix size " << hm_image_camera.size();
    }
}

void ndc_matrix_to_camera_matrix(std::span<const float> ndc_image_camera,
                                 const int width, const int height,
                                 std::span<float> hm_image_camera) {
    CHECK(width > 0);
    CHECK(height > 0);
    CHECK(ndc_image_camera.size() == 3 * 3 || ndc_image_camera.size() == 4 * 4);

    /*
      The map that takes (x,y,z,1) => (x*w/2 + w/2, y*h/2 + h/2, z, 1)
      ⎡w/2,   0,  0, w/2⎤
      ⎢0,   h/2,  0, h/2⎥
      ⎢0,     0,  1,   0⎥
      ⎣0,     0,  0,   1⎦

      The map that takes (x,y,1) => (x*w/2 + w/2, y*h/2 + h/2, 1)
      ⎡w/2,   0, w/2⎤
      ⎢0,   h/2, h/2⎥
      ⎣0,     0,   1⎦

      We manually premultiply one of these matrices depending on
      whether hm_image_camera is 3x3 or 4x4
    */

    // conversion from 3x3 <=> 4x4 requires temporary
    const bool need_dimension_conversion =
        (ndc_image_camera.size() != hm_image_camera.size());

    if (ndc_image_camera.size() == 4 * 4) {
        std::array<float, 4 * 4> temp44;
        CM_Matrix4f in(ndc_image_camera.data());

        // if we need dimension conversion, write the results into a
        // temporary 4x4 buffer. otherwise directly write to the
        // output buffer.
        M_Matrix4f out(need_dimension_conversion ? temp44.data()
                                                 : hm_image_camera.data());

        // manual "sparse matrix multiplication"
        out.row(0) = width / 2.0f * in.row(0) + width/2.0f * in.row(3);
        out.row(1) = height / 2.0f * in.row(1) + height/2.0f * in.row(3);
        out.row(2) = in.row(2);
        out.row(3) = in.row(3);

        if (need_dimension_conversion) {
            // apply dimension conversion
            camera_matrix_4x4_to_3x3(temp44, hm_image_camera);
        }
    } else if (ndc_image_camera.size() == 3 * 3) {
        // same as the previous case, except for (3,3) input matrix
        std::array<float, 3 * 3> temp33;

        CM_Matrix3f in(ndc_image_camera.data());
        M_Matrix3f out(need_dimension_conversion ? temp33.data()
                                                 : hm_image_camera.data());

        // manual "sparse matrix multiplication"
        out.row(0) = width / 2.0f * in.row(0) + width/2.0f * in.row(2);
        out.row(1) = height / 2.0f * in.row(1) + height/2.0f * in.row(2);
        out.row(2) = in.row(2);

        if (need_dimension_conversion) {
            camera_matrix_3x3_to_4x4(temp33, hm_image_camera);
        }
    } else {
        LOG(FATAL) << "Unsupported matrix size " << ndc_image_camera.size();
    }
}

}
