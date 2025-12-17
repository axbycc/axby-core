#pragma once
#include <cstdint>

#include "colors/colors.h"
#include "fast_resizable_vector/fast_resizable_vector.h"

namespace axby {

struct MeshMaterial {
    colors::RGBf diffuse = to_float(colors::white);
    colors::RGBf specular = to_float(colors::white);
    colors::RGBf ambient = to_float(colors::purple);
    colors::RGBf emissive = to_float(colors::white);
    float opacity = 1.0;
    float specular_exponent = 15;
};

struct Mesh {
    FastResizableVector<float> xyzs;
    FastResizableVector<float> rgbs;
    FastResizableVector<float> normals;
    FastResizableVector<uint32_t> faces;
    MeshMaterial material;
};

}  // namespace axby
