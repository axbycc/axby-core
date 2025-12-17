#pragma once

#include <array>
#include <cstdint>
#include <iostream>

#include "debug/check.h"
#include "debug/log.h"
#include "seq/seq.h"
#include "seq/any_seq.h"

namespace axby {
namespace colors {

struct RGBf {
    float red = 0;
    float green = 0;
    float blue = 0;

    float& operator[](size_t i) {
        CHECK_LT(i, 4);
        if (i == 0) return red;
        if (i == 1) return green;
        if (i == 2) return blue;

        // unreachable
        return red;
    }

    const float& operator[](size_t i) const {
        return ((RGBf*)(this))->operator[](i);
    }

    // span iterator compatibility
    using value_type = float;
    float* data() { return &red; }
    const float* data() const { return &red; }
    size_t size() const { return 3; }
};

struct RGBAf {
    float red = 0;
    float green = 0;
    float blue = 0;
    float alpha = 1.0f;

    float& operator[](size_t i) {
        CHECK_LT(i, 4);
        if (i == 0) return red;
        if (i == 1) return green;
        if (i == 2) return blue;
        if (i == 3) return alpha;

        // unreachable
        return red;
    }

    const float& operator[](size_t i) const {
        return ((RGBAf*)(this))->operator[](i);
    }

    // span iterator compatibility
    using value_type = float;
    float* data() { return &red; }
    const float* data() const { return &red; }
    size_t size() const { return 4; }
};

struct RGBA {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint8_t alpha = 255;

    uint8_t& operator[](size_t i) {
        CHECK_LT(i, 4);
        if (i == 0) return red;
        if (i == 1) return green;
        if (i == 2) return blue;
        if (i == 3) return alpha;

        // unreachable
        return red;
    }

    const uint8_t& operator[](size_t i) const {
        return ((RGBA*)(this))->operator[](i);
    }

    // span iterator compatibility
    using value_type = uint8_t;
    uint8_t* data() { return &red; }
    const uint8_t* data() const { return &red; }
    size_t size() const { return 4; }
};

struct RGB {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;

    uint8_t& operator[](size_t i) {
        CHECK_LT(i, 3);
        if (i == 0) return red;
        if (i == 1) return green;
        if (i == 2) return blue;

        // unreachable
        return red;
    }

    const uint8_t& operator[](size_t i) const {
        return ((RGBA*)(this))->operator[](i);
    }

    // span iterator compatibility
    using value_type = uint8_t;
    uint8_t* data() { return &red; }
    const uint8_t* data() const { return &red; }
    size_t size() const { return 3; }
};

inline RGBAf to_float(RGBA rgba) {
    return {rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f,
            rgba[3] / 255.0f};
}

inline RGBf to_float(RGB rgb) {
    return {rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f};
}

inline RGBA to_uint8(RGBAf rgba) {
    return {uint8_t(rgba[0] * 255), uint8_t(rgba[1] * 255),
            uint8_t(rgba[2] * 255), uint8_t(rgba[3] * 255)};
}

inline RGB to_uint8(RGBf rgb) {
    return {uint8_t(rgb[0] * 255), uint8_t(rgb[1] * 255),
            uint8_t(rgb[2] * 255)};
}

inline RGB drop_alpha(RGBA rgba) { return {rgba[0], rgba[1], rgba[2]}; }
inline RGBf drop_alpha(RGBAf rgba) { return {rgba[0], rgba[1], rgba[2]}; }

inline RGBA add_alpha(RGB rgb, uint8_t alpha) {
    return {rgb[0], rgb[1], rgb[2], alpha};
}

inline RGBAf add_alpha(RGBf rgb, float alpha) {
    return {rgb[0], rgb[1], rgb[2], alpha};
}

inline RGBAf make_rgbaf(Seq<const float> src) {
    RGBAf dst;
    CHECK(src.size() == 4);
    seq_copy(src, dst);
    return dst;
}

inline RGBf make_rgbf(Seq<const float> src) {
    RGBf dst;
    CHECK(src.size() == 3);
    seq_copy(src, dst);
    return dst;
}


inline RGBA make_rgba(Seq<const uint8_t> src) {
    RGBA dst;
    CHECK(src.size() == 4);
    seq_copy(src, dst);
    return dst;
}

inline RGB make_rgb(Seq<const uint8_t> src) {
    RGB dst;
    CHECK(src.size() == 3);
    seq_copy(src, dst);
    return dst;
}


inline RGBAf infer_rgbaf(ConstAnySeq channels) {
    CHECK((channels.logical_size() == 3) || (channels.logical_size() == 4)) << channels.logical_size();
    if (channels.logical_size() == 3) {
        if (channels.is_type<float>()) {
            auto data = channels.get_span<const float>();
            return add_alpha(make_rgbf(data), 1.0f);
        } else if (channels.is_type<uint8_t>()) {
            auto data = channels.get_span<const uint8_t>();
            return to_float(add_alpha(make_rgb(data), 255));
        } else {
            LOG(FATAL) << "Unsupported color channel type " << channels.get_typeid().name();
        }
    } else {
        if (channels.is_type<float>()) {
            auto data = channels.get_span<const float>();
            return make_rgbaf(data);
        } else if (channels.is_type<uint8_t>()) {
            auto data = channels.get_span<const uint8_t>();
            return to_float(make_rgba(data));
        } else {
            LOG(FATAL) << "Unsupported color channel type " << channels.get_typeid().name();
        }
    }

    // unreachable
    return {0.0f, 0.0f, 0.0f, 0.0f};
}

inline uint32_t to_imgui(RGBA rgba) {
    // bit layout (matching imgui default convention)
    // a, b, g, r
    uint32_t result = 0;
    result |= static_cast<uint32_t>(rgba.red) << 0;
    result |= static_cast<uint32_t>(rgba.green) << 8;
    result |= static_cast<uint32_t>(rgba.blue) << 16;
    result |= static_cast<uint32_t>(rgba.alpha) << 24;
    return result;
}

inline uint32_t to_imgui(RGB rgb) { return to_imgui(add_alpha(rgb, 255)); }

inline std::ostream& operator<<(std::ostream& os, const RGBA& color) {
    os << "RGBA" << ::axby::seq_to_string(color);
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const RGBAf& color) {
    os << "RGBAf" << ::axby::seq_to_string(color);
    return os;
}

constexpr RGB aliceblue  {240, 248, 255};
constexpr RGB antiquewhite  {250, 235, 215};
constexpr RGB aqua  {0, 255, 255};
constexpr RGB aquamarine  {127, 255, 212};
constexpr RGB azure  {240, 255, 255};
constexpr RGB beige  {245, 245, 220};
constexpr RGB bisque  {255, 228, 196};
constexpr RGB black  {0, 0, 0};
constexpr RGB blanchedalmond  {255, 235, 205};
constexpr RGB blue  {0, 0, 255};
constexpr RGB blueviolet  {138, 43, 226};
constexpr RGB brown  {165, 42, 42};
constexpr RGB burlywood  {222, 184, 135};
constexpr RGB cadetblue  {95, 158, 160};
constexpr RGB chartreuse  {127, 255, 0};
constexpr RGB chocolate  {210, 105, 30};
constexpr RGB coral  {255, 127, 80};
constexpr RGB cornflowerblue  {100, 149, 237};
constexpr RGB cornsilk  {255, 248, 220};
constexpr RGB crimson  {220, 20, 60};
constexpr RGB cyan  {0, 255, 255};
constexpr RGB darkblue  {0, 0, 139};
constexpr RGB darkcyan  {0, 139, 139};
constexpr RGB darkgoldenrod  {184, 134, 11};
constexpr RGB darkgray  {169, 169, 169};
constexpr RGB darkgreen  {0, 100, 0};
constexpr RGB darkgrey  {169, 169, 169};
constexpr RGB darkkhaki  {189, 183, 107};
constexpr RGB darkmagenta  {139, 0, 139};
constexpr RGB darkolivegreen  {85, 107, 47};
constexpr RGB darkorange  {255, 140, 0};
constexpr RGB darkorchid  {153, 50, 204};
constexpr RGB darkred  {139, 0, 0};
constexpr RGB darksalmon  {233, 150, 122};
constexpr RGB darkseagreen  {143, 188, 143};
constexpr RGB darkslateblue  {72, 61, 139};
constexpr RGB darkslategray  {47, 79, 79};
constexpr RGB darkslategrey  {47, 79, 79};
constexpr RGB darkturquoise  {0, 206, 209};
constexpr RGB darkviolet  {148, 0, 211};
constexpr RGB deeppink  {255, 20, 147};
constexpr RGB deepskyblue  {0, 191, 255};
constexpr RGB dimgray  {105, 105, 105};
constexpr RGB dimgrey  {105, 105, 105};
constexpr RGB dodgerblue  {30, 144, 255};
constexpr RGB firebrick  {178, 34, 34};
constexpr RGB floralwhite  {255, 250, 240};
constexpr RGB forestgreen  {34, 139, 34};
constexpr RGB fuchsia  {255, 0, 255};
constexpr RGB gainsboro  {220, 220, 220};
constexpr RGB ghostwhite  {248, 248, 255};
constexpr RGB gold  {255, 215, 0};
constexpr RGB goldenrod  {218, 165, 32};
constexpr RGB gray  {128, 128, 128};
constexpr RGB green  {0, 128, 0};
constexpr RGB greenyellow  {173, 255, 47};
constexpr RGB grey  {128, 128, 128};
constexpr RGB honeydew  {240, 255, 240};
constexpr RGB hotpink  {255, 105, 180};
constexpr RGB indianred  {205, 92, 92};
constexpr RGB indigo  {75, 0, 130};
constexpr RGB ivory  {255, 255, 240};
constexpr RGB khaki  {240, 230, 140};
constexpr RGB lavender  {230, 230, 250};
constexpr RGB lavenderblush  {255, 240, 245};
constexpr RGB lawngreen  {124, 252, 0};
constexpr RGB lemonchiffon  {255, 250, 205};
constexpr RGB lightblue  {173, 216, 230};
constexpr RGB lightcoral  {240, 128, 128};
constexpr RGB lightcyan  {224, 255, 255};
constexpr RGB lightgoldenrodyellow  {250, 250, 210};
constexpr RGB lightgray  {211, 211, 211};
constexpr RGB lightgreen  {144, 238, 144};
constexpr RGB lightgrey  {211, 211, 211};
constexpr RGB lightpink  {255, 182, 193};
constexpr RGB lightsalmon  {255, 160, 122};
constexpr RGB lightseagreen  {32, 178, 170};
constexpr RGB lightskyblue  {135, 206, 250};
constexpr RGB lightslategray  {119, 136, 153};
constexpr RGB lightslategrey  {119, 136, 153};
constexpr RGB lightsteelblue  {176, 196, 222};
constexpr RGB lightyellow  {255, 255, 224};
constexpr RGB lime  {0, 255, 0};
constexpr RGB limegreen  {50, 205, 50};
constexpr RGB linen  {250, 240, 230};
constexpr RGB magenta  {255, 0, 255};
constexpr RGB maroon  {128, 0, 0};
constexpr RGB mediumaquamarine  {102, 205, 170};
constexpr RGB mediumblue  {0, 0, 205};
constexpr RGB mediumorchid  {186, 85, 211};
constexpr RGB mediumpurple  {147, 112, 219};
constexpr RGB mediumseagreen  {60, 179, 113};
constexpr RGB mediumslateblue  {123, 104, 238};
constexpr RGB mediumspringgreen  {0, 250, 154};
constexpr RGB mediumturquoise  {72, 209, 204};
constexpr RGB mediumvioletred  {199, 21, 133};
constexpr RGB midnightblue  {25, 25, 112};
constexpr RGB mintcream  {245, 255, 250};
constexpr RGB mistyrose  {255, 228, 225};
constexpr RGB moccasin  {255, 228, 181};
constexpr RGB navajowhite  {255, 222, 173};
constexpr RGB navy  {0, 0, 128};
constexpr RGB oldlace  {253, 245, 230};
constexpr RGB olive  {128, 128, 0};
constexpr RGB olivedrab  {107, 142, 35};
constexpr RGB orange  {255, 165, 0};
constexpr RGB orangered  {255, 69, 0};
constexpr RGB orchid  {218, 112, 214};
constexpr RGB palegoldenrod  {238, 232, 170};
constexpr RGB palegreen  {152, 251, 152};
constexpr RGB paleturquoise  {175, 238, 238};
constexpr RGB palevioletred  {219, 112, 147};
constexpr RGB papayawhip  {255, 239, 213};
constexpr RGB peachpuff  {255, 218, 185};
constexpr RGB peru  {205, 133, 63};
constexpr RGB pink  {255, 192, 203};
constexpr RGB plum  {221, 160, 221};
constexpr RGB powderblue  {176, 224, 230};
constexpr RGB purple  {128, 0, 128};
constexpr RGB rebeccapurple  {102, 51, 153};
constexpr RGB red  {255, 0, 0};
constexpr RGB rosybrown  {188, 143, 143};
constexpr RGB royalblue  {65, 105, 225};
constexpr RGB saddlebrown  {139, 69, 19};
constexpr RGB salmon  {250, 128, 114};
constexpr RGB sandybrown  {244, 164, 96};
constexpr RGB seagreen  {46, 139, 87};
constexpr RGB seashell  {255, 245, 238};
constexpr RGB sienna  {160, 82, 45};
constexpr RGB silver  {192, 192, 192};
constexpr RGB skyblue  {135, 206, 235};
constexpr RGB slateblue  {106, 90, 205};
constexpr RGB slategray  {112, 128, 144};
constexpr RGB slategrey  {112, 128, 144};
constexpr RGB snow  {255, 250, 250};
constexpr RGB springgreen  {0, 255, 127};
constexpr RGB steelblue  {70, 130, 180};
constexpr RGB tan  {210, 180, 140};
constexpr RGB teal  {0, 128, 128};
constexpr RGB thistle  {216, 191, 216};
constexpr RGB tomato  {255, 99, 71};
constexpr RGB turquoise  {64, 224, 208};
constexpr RGB violet  {238, 130, 238};
constexpr RGB wheat  {245, 222, 179};
constexpr RGB white  {255, 255, 255};
constexpr RGB whitesmoke  {245, 245, 245};
constexpr RGB yellow  {255, 255, 0};
constexpr RGB yellowgreen             {154, 205, 50};
}  // namespace colors
}  // namespace axby
