#include "cmap.h"

#include <array>
#include <magic_enum.hpp>
#include <tinycolormap.hpp>
#include <utility>

#include "colors/colors.h"
#include "debug/log.h"

namespace axby {
namespace gl {

constexpr int CMAP_RESOLUTION = 1024;
std::array<Texture, uint8_t(Cmap::_NUM_CMAPS)> CMAP_TEXTURES = {};

tinycolormap::ColormapType as_tinycolormaptype(Cmap cmap) {
    switch (cmap) {
        case Cmap::heat:
            return tinycolormap::ColormapType::Heat;
        case Cmap::hsv:
            return tinycolormap::ColormapType::HSV;
        case Cmap::parula:
            return tinycolormap::ColormapType::Parula;
        case Cmap::viridis:
            return tinycolormap::ColormapType::Viridis;
        case Cmap::plasma:
            return tinycolormap::ColormapType::Plasma;
        case Cmap::jet:
            return tinycolormap::ColormapType::Jet;
        case Cmap::gray:
            return tinycolormap::ColormapType::Gray;
        default:
            LOG(FATAL) << "Need to implement cmap "
                       << magic_enum::enum_name(cmap);
    }

    // std::unreachable(); // unreachable not supported by gcc
    return tinycolormap::ColormapType::Gray;
}

const Texture& get_cmap_texture(Cmap cmap) {
    const auto tinycolormaptype = as_tinycolormaptype(cmap);
    Texture& texture = CMAP_TEXTURES[static_cast<uint8_t>(cmap)];
    if (texture.id == 0) {
        // lazy initialize the texture
        std::array<uint8_t, 3 * CMAP_RESOLUTION> data;

        texture = Texture(TextureOptions().set_data_type<uint8_t>().set_rgb());

        for (int i = 0; i < CMAP_RESOLUTION; ++i) {
            double value = double(i) / CMAP_RESOLUTION;
            auto rgb = tinycolormap::GetColor(value, tinycolormaptype);
            data[3 * i] = rgb.ri();
            data[3 * i + 1] = rgb.gi();
            data[3 * i + 2] = rgb.bi();
        }
        texture.upload(/*width=*/CMAP_RESOLUTION, /*height=*/1, data);
    }
    return texture;
}

const colors::RGB get_cmap_value(Cmap cmap, float value) {
    const auto tinycolormaptype = as_tinycolormaptype(cmap);
    const auto tinycolormapcolor =
        tinycolormap::GetColor(value, tinycolormaptype);
    colors::RGBf resultf{(float)tinycolormapcolor.r(),
                         (float)tinycolormapcolor.g(),
                         (float)tinycolormapcolor.b()};
    return to_uint8(resultf);
};

}  // namespace gl
}  // namespace axby
