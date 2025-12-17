#pragma once

#include <Eigen/Dense>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

template <typename T>
struct Spatial {
    using Matrix3 = Eigen::Matrix<T, 3, 3>;
    using Matrix4 = Eigen::Matrix<T, 4, 4>;
    using Matrix6 = Eigen::Matrix<T, 6, 6>;
    using Vector3 = Eigen::Matrix<T, 3, 1>;
    using Vector6 = Eigen::Matrix<T, 6, 1>;

    static constexpr double kTol = 1e-4;  // tolerance for almost equal

    static bool almostEquals(const T a, const T b) {
        return std::abs(a - b) < kTol;
    }

    static Vector3 rotation_to_vector(const Matrix3& rot) {
        Eigen::AngleAxis<T> angle_axis(rot);
        Vector3 rot_vector = angle_axis.angle() * angle_axis.axis();
        return rot_vector;
    }

    static Vector3 rotation_difference_to_vector(const Matrix3& initial_rot,
                                                 const Matrix3& final_rot) {
        Matrix3 delta_rot_mat = final_rot * initial_rot.transpose();
        Vector3 delta_rot_vec = rotation_to_vector(delta_rot_mat);
        return delta_rot_vec;
    }

    static Matrix3 lerp_rotation(const Matrix3& initial_rot,
                                 const Matrix3& final_rot,
                                 const T progress) {
        Matrix3 delta_rot_mat = final_rot * initial_rot.transpose();
        Vector3 delta_rot_vec = rotation_to_vector(delta_rot_mat);
        return vector_to_rotation(delta_rot_vec * progress) * initial_rot;
    }

    static Matrix4 lerp_tx(const Matrix4& initial_tx,
                           const Matrix4& final_tx,
                           const T progress) {
        Matrix3 initial_rot = initial_tx.template topLeftCorner<3, 3>();
        Matrix3 final_rot = final_tx.template topLeftCorner<3, 3>();
        Matrix3 out_rot = lerp_rotation(initial_rot, final_rot, progress);

        Vector3 initial_trans = initial_tx.template topRightCorner<3, 1>();
        Vector3 final_trans = final_tx.template topRightCorner<3, 1>();
        Vector3 out_trans =
            (1.0 - progress) * initial_trans + progress * final_trans;

        Matrix4 out;
        out.setIdentity();
        out.template topLeftCorner<3, 3>() = out_rot;
        out.template topRightCorner<3, 1>() = out_trans;
        return out;
    }

    static Matrix3 vector_to_rotation(const Vector3& vec) {
        T length = vec.norm();
        if (length < 1e-8) {
            return Matrix3::Identity();
        }
        const Vector3 axis = vec / length;
        Matrix3 result = Eigen::AngleAxis<T>(length, axis).toRotationMatrix();
        return result;
    }

    static Matrix3 direction_to_rotation(const Vector3& direction,
                                         uint8_t axis) {
        Vector3 rot_z = direction.normalized();
        Vector3 rot_x = rot_z.cross(Vector3::UnitX()).normalized();
        Vector3 rot_y = rot_z.cross(rot_x);
        Matrix3 rotation;
        rotation.col((axis + 1) % 3) = rot_x;
        rotation.col((axis + 2) % 3) = rot_y;
        rotation.col(axis) = rot_z;
        return rotation;
    }

    static Matrix4 tx_from_rot_trans(const Matrix3& rot, const Vector3& trans) {
        Matrix4 out;
        out.setIdentity();
        out.template block<3, 3>(0, 0) = rot;
        out.template block<3, 1>(0, 3) = trans;
        return out;
    }

    static Vector3 tx_apply(const Matrix4& tx, const Vector3& xyz) {
        return tx.template block<3, 3>(0, 0) * xyz +
               tx.template block<3, 1>(0, 3);
    }

    static Matrix4 SE3_inv(const Matrix4& SE3) {
        Matrix4 result = Matrix4::Identity();
        result.template topLeftCorner<3, 3>() =
            SE3.template topLeftCorner<3, 3>().transpose();
        result.template topRightCorner<3, 1>() =
            -result.template topLeftCorner<3, 3>() *
            SE3.template topRightCorner<3, 1>();
        return result;
    }

    static Matrix6 SE3_adj(const Matrix3& rot, const Vector3& trans) {
        Matrix3 t_hat;
        t_hat << T(0), -trans.z(), trans.y(), trans.z(), T(0), -trans.x(),
            -trans.y(), trans.x(), T(0);

        Matrix6 result;
        result.template topLeftCorner<3, 3>() = rot;
        result.template topRightCorner<3, 3>() = Matrix3::Zero();
        result.template bottomLeftCorner<3, 3>() = t_hat * rot;
        result.template bottomRightCorner<3, 3>() = rot;
        return result;
    }

    static Matrix6 SE3_adj(const Matrix4& SE3) {
        Matrix3 R = SE3.template topLeftCorner<3, 3>();
        Vector3 t = SE3.template topRightCorner<3, 1>();
        return SE3_adj(R, t);
    }

    static Matrix4 se3_exp(const Vector6& se3) {
        Matrix4 result = Matrix4::Identity();
        Vector3 omega = se3.template head<3>();
        Vector3 v = se3.template tail<3>();
        T theta_squared = omega.squaredNorm();
        Matrix3 omega_hat;
        omega_hat << 0, -omega.z(), omega.y(), omega.z(), 0, -omega.x(),
            -omega.y(), omega.x(), 0;

        T A, B, C;
        if (theta_squared < 1e-8) {
            A = 1.0 - theta_squared / 6.0;
            B = 0.5 - theta_squared / 24.0;
            C = 1.0 / 6.0 - theta_squared / 120.0;
        } else {
            T theta = std::sqrt(theta_squared);
            T stheta = std::sin(theta);
            T ctheta = std::cos(theta);
            A = stheta / theta;
            B = (1 - ctheta) / theta_squared;
            C = (1 - A) / theta_squared;
        }

        Matrix3 omega_hat2 = omega_hat * omega_hat;
        result.template topLeftCorner<3, 3>() += A * omega_hat + B * omega_hat2;
        result.template topRightCorner<3, 1>() =
            (B * omega_hat + C * omega_hat2) * v + v;

        return result;
    }

    // x * cotan(x) for x
    // accurate only in region (-pi/2, pi/2) via taylor series
    // https://www.wolframalpha.com/input/?i=taylor+expand+x+cotan%28x%29+around+0
    static T x_cotx(T x) {
        constexpr T c2 = -1.0 / 3;
        constexpr T c4 = -1.0 / 45;
        constexpr T c6 = -2.0 / 945;
        constexpr T c8 = -1.0 / 4725;
        constexpr T c10 = -2.0 / 93555;
        const T x2 = x * x;
        const T x4 = x2 * x2;
        const T x6 = x4 * x2;
        const T x8 = x4 * x4;
        const T x10 = x8 * x2;
        return 1.0 + c2 * x2 + c4 * x4 + c6 * x6 + c8 * x8 + c10 * x10;
    }

    static void SO3_log(const Matrix3& rotation_matrix,
                        T* theta,
                        Matrix3* omega_hat,
                        bool* was_identity = nullptr) {
        // Check for edge case: identity
        // A rotation matrix is identity iff all diagonal entries are 1.0
        bool is_identity = true;
        for (int i = 0; i < 3; ++i) {
            if (!almostEquals(1.0f, rotation_matrix(i, i))) {
                is_identity = false;
                break;
            }
        }
        if (is_identity) {
            *theta = 0;
            Vector3 omega_vector;
            omega_vector << 1, 0, 0;
            (*omega_hat) = so3_vec_to_mat(omega_vector);

            if (was_identity != nullptr) {
                *was_identity = true;
            }

            return;
        }

        // Check for edge case: rotation of k*PI
        const T trace = rotation_matrix.trace();
        if (almostEquals(-1.0f, trace)) {
            *theta = M_PI;
            Vector3 omega_vector;
            const T r33 = rotation_matrix(2, 2);
            const T r22 = rotation_matrix(1, 1);
            const T r11 = rotation_matrix(0, 0);
            if (!almostEquals(1.0f + r33, 0.0f)) {
                (omega_vector)(0) = rotation_matrix(0, 2);
                (omega_vector)(1) = rotation_matrix(1, 2);
                (omega_vector)(2) = 1 + rotation_matrix(2, 2);
                (omega_vector) /= std::sqrt(2 * (1 + r33));
            } else if (!almostEquals(1.0f + r22, 0.0f)) {
                (omega_vector)(0) = rotation_matrix(0, 1);
                (omega_vector)(1) = 1 + rotation_matrix(1, 1);
                (omega_vector)(2) = rotation_matrix(2, 1);
                (omega_vector) /= std::sqrt(2 * (1 + r22));
            } else {
                // 1 + r11 != 0
                (omega_vector)(0) = 1 + rotation_matrix(0, 0);
                (omega_vector)(1) = rotation_matrix(1, 0);
                (omega_vector)(2) = rotation_matrix(2, 0);
                (omega_vector) /= std::sqrt(2 * (1 + r11));
            }
            (*omega_hat) = so3_vec_to_mat(omega_vector);
            return;
        }

        // Normal case

        // htmo means Half of Trace Minus One
        const T htmo = 0.5f * (trace - 1);

	CHECK(std::abs(htmo) <= 1);
	
        *theta = std::acos(htmo);  // todo: investigate faster approx to acos

        const T sin_acos_htmo = std::sqrt(1.0 - htmo * htmo);
        *omega_hat = 0.5f / sin_acos_htmo *
                     (rotation_matrix - rotation_matrix.transpose());

	CHECK(omega_hat->allFinite()) << "htmo " << htmo << ", sin_acos_htmo " << sin_acos_htmo << "\ninput\n" << rotation_matrix;
    }

    // Returns a twist
    static Vector6 SE3_log(const Matrix4& SE3_element) {
        T theta;
        Matrix3 omega;
        bool was_identity = false;
        SO3_log(SE3_element.template block<3, 3>(0, 0), &theta, &omega,
                &was_identity);

        // conversion from omega matrix to omega vector, then multiplication by
        // theta
        Vector3 omega_theta;
        omega_theta << theta * omega(2, 1), -theta * omega(2, 0),
            theta * omega(1, 0);

        const Vector3 p = SE3_element.template block<3, 1>(0, 3);

        const Vector3 omega_p = omega * p;
        const Vector3 v_theta = p - 0.5f * theta * omega_p +
                                (1.0f - x_cotx(theta / 2)) * omega * omega_p;

        Vector6 result;
        result.template head<3>() = omega_theta;
        result.template tail<3>() = v_theta;

        CHECK(result.allFinite())
            << "Src\n"
            << SE3_element << "\nResult " << result.transpose() << "\ntheta "
            << theta << "\nomega\n"
            << omega << "\nwas identity? " << was_identity;
        return result;
    }

    static Matrix3 so3_vec_to_mat(const Vector3& vec) {
        const double w1 = vec(0);
        const double w2 = vec(1);
        const double w3 = vec(2);

        Matrix3 m;
        // clang-format off
	m <<
	    0,  -w3, w2,
	    w3,  0,  -w1,
	    -w2, w1, 0;
        // clang-format on

        return m;
    }

    static Vector3 so3_mat_to_vec(const Matrix3& mat) {
        Vector3 result;
        result << mat(2, 1), mat(0, 2), mat(1, 0);
        return result;
    }

    static Matrix4 se3_vec_to_mat(const Vector6& vec) {
        Matrix4 result;
        result.template block<3, 3>(0, 0) =
            so3_vec_to_mat(vec.template head<3>());
        result.template block<3, 1>(0, 3) = vec.template tail<3>();
        result.template row(3).template setZero();
        return result;
    }

    static Vector6 se3_mat_to_vec(const Matrix4& mat) {
        Vector6 result;
        result.template tail<3>() = mat.template block<3, 1>(0, 3);
        result.template head<3>() =
            so3_mat_to_vec(mat.template block<3, 3>(0, 0).template eval());
        return result;
    }

    static Matrix4 make_random_SE3() {
        Matrix4 tx;

        Matrix3 rotation;
        rotation = Matrix3::Random();
        const T det = rotation.determinant();
        if (det == 0) {
            // this will never happen in practice
            rotation.setIdentity();
        } else {
            if (det < 0) {
                rotation.row(0) *= -1;
            }
            Eigen::JacobiSVD<Matrix3> svd(
                rotation, Eigen::ComputeFullU | Eigen::ComputeFullV);
            rotation = svd.matrixU() * svd.matrixV().transpose();
        }

        // generate random translation vector
        Vector3 translation = Vector3::Random();

        // construct SE3 transformation matrix
        tx.template block<3, 3>(0, 0) = rotation;
        tx.template block<3, 1>(0, 3) = translation;
        tx.template block<1, 4>(3, 0) << 0, 0, 0,
            1;  // bottom row for homogeneous coordinates
        return tx;
    }
};
