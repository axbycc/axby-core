#pragma once

#include "colors/colors.h"
#include "texture.h"

namespace axby {
namespace gl {

enum class Cmap : uint8_t {
    heat = 0,
    hsv = 1,
    parula = 2,
    viridis = 3,
    plasma = 4,
    jet = 5,
    gray = 6,
    _NUM_CMAPS = 7
};  // namespace cmap

const Texture& get_cmap_texture(Cmap cmap);
const colors::RGB get_cmap_value(Cmap cmap, float value);

}  // namespace gl
}  // namespace axby
