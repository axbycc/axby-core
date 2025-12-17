// unofficial type extensions

#include "Dense"

namespace Eigen {

using CMapMatrix3f = Map<const Eigen::Matrix3f>;
using CMapMatrix4f = Map<const Eigen::Matrix4f>;
using CMapVector3f = Map<const Eigen::Vector3f>;
using CMapVector4f = Map<const Eigen::Vector4f>;
using MapMatrix3f = Map<Eigen::Matrix3f>;
using MapMatrix4f = Map<Eigen::Matrix4f>;
using MapVector3f = Map<Eigen::Vector3f>;
using MapVector4f = Map<Eigen::Vector4f>;

}
