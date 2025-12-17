#pragma once

#include <cstdint>
#include "seq/seq.h"

namespace axby {

// axis is 0 (x), 1 (y), or 2 (z). given a direction, returns a 3x3
// rotation matrix that transforms the axis to that
// direction. direction does not need to be normalized.
void direction_to_rotation(Seq<const float> direction,
                           Seq<float> rot_out,
                           uint8_t axis = 0);

// 3x3 col-major rotation matrix to length 3 vector, aka rodriguez
void rotation_to_vector(Seq<const float> rot, Seq<float> vec_out);
void rotation_to_vector(Seq<const double> rot, Seq<double> vec_out);

// length 3 vector to 3x3 col-major rotation matrix, aka inverse rodriguez
void vector_to_rotation(Seq<const float> vec, Seq<float> rot_out);

// lerp between two rotations. all xxx_rot are 3x3 col major. progress
// is in [0,1]. when progress=0, the out_rot is equal to initial_rot
// and when progress=1, out_rot is the final_rot
void lerp_rotation(Seq<const float> initial_rot,
                   Seq<const float> final_rot,
                   const float progress,
                   Seq<float> out_rot);

void lerp_rotation(Seq<const double> initial_rot,
                   Seq<const double> final_rot,
                   const double progress,
                   Seq<double> out_rot);

// lerp between two 4x4 col-major transform matrices. progress is in
// [0,1]. when progress=0, the out_tx is equal to initial_tx and
// when progress=1, out_rot is the final_tx
void lerp_tx(Seq<const double> initial_tx,
             Seq<const double> final_tx,
             const double progress,
             Seq<double> out_rot);

void lerp_tx(Seq<const float> initial_tx,
             Seq<const float> final_tx,
             const float progress,
             Seq<float> out_rot);

// same as rotation_to_vector(final_rotation *
// initial_rotation.transpose()) this gives the "displacement" in
// rotational space as a 3-vector
void rotation_difference_to_vector(Seq<const float> initial_rotation,
                                   Seq<const float> final_rotation,
                                   Seq<float> rot_out);

// rot is 3x3 col major
// trans is size 3
// out is 4x4 col major homogenous transform matrix
void tx_from_rot_trans(Seq<const float> rot,
                       Seq<const float> trans,
                       Seq<float> out);
void tx_from_rot_trans(Seq<const double> rot,
                       Seq<const double> trans,
                       Seq<double> out);

// tx is 4x4 col major
// xyz is 3
// out is 3
void tx_apply(Seq<const float> tx,
              Seq<const float> xyz,
              Seq<float> out);

// out is 4x4 col major
void make_random_SE3(Seq<float> out);
void make_random_SE3(Seq<double> out);

// se3 is length 6 (rot, trans), out is 4x4 col major
void se3_exp(Seq<const double> se3, Seq<double> tx_out);
void se3_exp(Seq<const float> se3, Seq<float> tx_out);

// SE3 is 4x4 col major, se3_out is 6
void SE3_log(Seq<const double> SE3, Seq<double> se3_out);
void SE3_log(Seq<const float> SE3, Seq<float> se3_out);

// SE3 is 4x4 col major, Adj is 6x6 col major
void SE3_adj(Seq<const double> SE3, Seq<double> Adj);
void SE3_adj(Seq<const float> SE3, Seq<float> Adj);

struct SpatialDistance {
    float trans_distance = 0;
    float rot_distance = 0;
};

SpatialDistance get_spatial_distance(Seq<const float> tx_a,
                                     Seq<const float> tx_b);

}  // namespace axby
