#pragma once

#include <Eigen/Dense>

namespace Eigen {

// unofficial type extensions
// useful for SE3 math
using Vector6f = Matrix<float, 6, 1>;
using Matrix6f = Matrix<float, 6, 6>;
using Vector6d = Matrix<double, 6, 1>;
using Matrix6d = Matrix<double, 6, 6>;

using Vector7d = Matrix<double, 7, 1>;
using Vector7f = Matrix<float, 7, 1>;

template <typename T>
using Vector6 = Matrix<T, 6, 1>;

template <typename T>
using Matrix4 = Matrix<T, 4, 4>;

} // namespace Eigen

// mapping aliases
using CM_Matrix3f = Eigen::Map<const Eigen::Matrix3f>;
using CM_Matrix3d = Eigen::Map<const Eigen::Matrix3d>;
using CM_Matrix4f = Eigen::Map<const Eigen::Matrix4f>;
using CM_Matrix6f = Eigen::Map<const Eigen::Matrix6f>;
using CM_Matrix4d = Eigen::Map<const Eigen::Matrix4d>;
using CM_Vector2f = Eigen::Map<const Eigen::Vector2f>;
using CM_Vector3d = Eigen::Map<const Eigen::Vector3d>;
using CM_Vector3f = Eigen::Map<const Eigen::Vector3f>;
using CM_Vector4f = Eigen::Map<const Eigen::Vector4f>;
using CM_Vector6f = Eigen::Map<const Eigen::Vector6f>;
using CM_Vector6d = Eigen::Map<const Eigen::Vector6d>;
using CM_Vector7d = Eigen::Map<const Eigen::Vector7d>;
using CM_Vector7f = Eigen::Map<const Eigen::Vector7f>;
using M_Matrix3f = Eigen::Map<Eigen::Matrix3f>;
using M_Matrix3d = Eigen::Map<Eigen::Matrix3d>;
using M_Matrix4d = Eigen::Map<Eigen::Matrix4d>;
using M_Matrix6d = Eigen::Map<Eigen::Matrix6d>;
using M_Matrix4f = Eigen::Map<Eigen::Matrix4f>;
using M_Matrix6f = Eigen::Map<Eigen::Matrix6f>;
using M_Vector2f = Eigen::Map<Eigen::Vector2f>;
using M_Vector3d = Eigen::Map<Eigen::Vector3d>;
using M_Vector3f = Eigen::Map<Eigen::Vector3f>;
using M_Vector4f = Eigen::Map<Eigen::Vector4f>;
using M_Vector6d = Eigen::Map<Eigen::Vector6d>;
using M_Vector6f = Eigen::Map<Eigen::Vector6f>;
using M_Vector7d = Eigen::Map<Eigen::Vector7d>;
using M_Vector7f = Eigen::Map<Eigen::Vector7f>;

// row-major versions
using Matrix3fr = Eigen::Matrix<float, 3, 3, Eigen::RowMajor>;
using Matrix4fr = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;
using M_Matrix3fr = Eigen::Map<Matrix3fr>;
using M_Matrix4fr = Eigen::Map<Matrix4fr>;
using CM_Matrix3fr = Eigen::Map<const Matrix3fr>;
using CM_Matrix4fr = Eigen::Map<const Matrix4fr>;

inline Eigen::Matrix3f Id3f() { return Eigen::Matrix3f::Identity(); }
inline Eigen::Matrix4f Id4f() { return Eigen::Matrix4f::Identity(); }
inline Eigen::Matrix4d Id4d() { return Eigen::Matrix4d::Identity(); }
