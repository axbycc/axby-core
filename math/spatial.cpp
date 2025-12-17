#include "math/spatial.h"

#include <Eigen/Dense>
#include <cmath>

#include "debug/check.h"
#include "spatial_impl.hpp"
#include "wrappers/eigen.h"

namespace axby {

void direction_to_rotation(Seq<const float> direction,
                           Seq<float> rot_out,
                           uint8_t axis) {
    M_Matrix3f out(rot_out);
    out = Spatial<float>::direction_to_rotation(CM_Vector3f(direction), axis);
}

void rotation_to_vector(Seq<const float> rot, Seq<float> vec_out) {
    CHECK(rot.size() == 3 * 3);
    CHECK(vec_out.size() == 3);
    M_Vector3f out(vec_out);
    out = Spatial<float>::rotation_to_vector(CM_Matrix3f(rot));
}

void rotation_to_vector(Seq<const double> rot,
                        Seq<double> vec_out) {
    CHECK(rot.size() == 3 * 3);
    CHECK(vec_out.size() == 3);
    M_Vector3d out(vec_out);
    out = Spatial<double>::rotation_to_vector(CM_Matrix3d(rot));
}

void vector_to_rotation(Seq<const float> vec, Seq<float> rot_out) {
    CHECK(rot_out.size() == 3 * 3);
    CHECK(vec.size() == 3);
    M_Matrix3f out(rot_out);
    out = Spatial<float>::vector_to_rotation(CM_Vector3f(vec));
}

void rotation_difference_to_vector(Seq<const float> initial_rotation,
                                   Seq<const float> final_rotation,
                                   Seq<float> rot_out) {
    M_Vector3f out(rot_out);
    out = Spatial<float>::rotation_difference_to_vector(
        CM_Matrix3f(initial_rotation), CM_Matrix3f(final_rotation));
}

void lerp_rotation(Seq<const float> initial_rot,
                   Seq<const float> final_rot,
                   const float progress,
                   Seq<float> out_rot) {
    CHECK(progress >= 0);
    CHECK(progress <= 1);
    M_Matrix3f out(out_rot);
    out = Spatial<float>::lerp_rotation(CM_Matrix3f(initial_rot),
                                        CM_Matrix3f(final_rot), progress);
};

void lerp_rotation(Seq<const double> initial_rot,
                   Seq<const double> final_rot,
                   const double progress,
                   Seq<double> out_rot) {
    CHECK(progress >= 0);
    CHECK(progress <= 1);
    M_Matrix3d out(out_rot);
    out = Spatial<double>::lerp_rotation(CM_Matrix3d(initial_rot),
                                         CM_Matrix3d(final_rot), progress);
};

void lerp_tx(Seq<const double> initial_tx,
             Seq<const double> final_tx,
             const double progress,
             Seq<double> out_tx) {
    CHECK(progress >= 0);
    CHECK(progress <= 1);

    M_Matrix4d out(out_tx);
    out = Spatial<double>::lerp_tx(CM_Matrix4d(initial_tx),
                                   CM_Matrix4d(final_tx), progress);
};

void lerp_tx(Seq<const float> initial_tx,
             Seq<const float> final_tx,
             const float progress,
             Seq<float> out_tx) {
    CHECK(progress >= 0);
    CHECK(progress <= 1);

    M_Matrix4f out(out_tx);
    out = Spatial<float>::lerp_tx(CM_Matrix4f(initial_tx),
                                  CM_Matrix4f(final_tx), progress);
};

void tx_from_rot_trans(Seq<const float> rot,
                       Seq<const float> trans,
                       Seq<float> tx_out) {
    M_Matrix4f out(tx_out);
    out =
        Spatial<float>::tx_from_rot_trans(CM_Matrix3f(rot), CM_Vector3f(trans));
}

void tx_from_rot_trans(Seq<const double> rot,
                       Seq<const double> trans,
                       Seq<double> tx_out) {
    M_Matrix4d out(tx_out);
    out = Spatial<double>::tx_from_rot_trans(CM_Matrix3d(rot),
                                             CM_Vector3d(trans));
}

void tx_apply(Seq<const float> tx,
              Seq<const float> xyz,
              Seq<float> xyz_out) {
    M_Vector3f out(xyz_out);
    out = Spatial<float>::tx_apply(CM_Matrix4f(tx), CM_Vector3f(xyz));
}

void make_random_SE3(Seq<double> tx_out) {
    M_Matrix4d out(tx_out);
    out = Spatial<double>::make_random_SE3();
}

void make_random_SE3(Seq<float> tx_out) {
    M_Matrix4f out(tx_out);
    out = Spatial<float>::make_random_SE3();
}

void se3_exp(Seq<const float> se3, Seq<float> tx_out) {
    M_Matrix4f out(tx_out);
    out = Spatial<float>::se3_exp(CM_Vector6f(se3));
}

void se3_exp(Seq<const double> se3, Seq<double> tx_out) {
    M_Matrix4d out(tx_out);
    out = Spatial<double>::se3_exp(CM_Vector6d(se3));
}

void SE3_log(Seq<const double> SE3, Seq<double> se3_out) {
    M_Vector6d out(se3_out);
    out = Spatial<double>::SE3_log(CM_Matrix4d(SE3));
};

void SE3_log(Seq<const float> SE3, Seq<float> se3_out) {
    M_Vector6f out(se3_out);
    out = Spatial<float>::SE3_log(CM_Matrix4f(SE3));
};

void SE3_adj(Seq<const float> SE3, Seq<float> Adj) {
    M_Matrix6f out(Adj);
    out = Spatial<float>::SE3_adj(CM_Matrix4f(SE3));
}

void SE3_adj(Seq<const double> SE3, Seq<double> Adj) {
    M_Matrix6d out(Adj);
    out = Spatial<double>::SE3_adj(CM_Matrix4d(SE3));
}

SpatialDistance get_spatial_distance(Seq<const float> tx_a,
                                     Seq<const float> tx_b) {
    Eigen::Matrix4f inv_tx_b =
        Spatial<float>::SE3_inv(Eigen::Matrix4f(tx_b.data()));
    Eigen::Matrix4f tx_delta = CM_Matrix4f(tx_a) * inv_tx_b;

    SpatialDistance result;
    result.rot_distance = 3 - tx_delta.topLeftCorner<3, 3>().trace();
    result.trans_distance = tx_delta.topRightCorner<3, 1>().norm();
    return result;
};

}
