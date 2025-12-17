#pragma once

#include <array>
#include <ostream>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

#include "fast_resizable_vector/fast_resizable_vector.h"
#include "serialization/small_string.h"

namespace axby {
namespace realsense_streaming {

enum class StreamType : uint8_t {
    INVALID,
    DEPTH,
    COLOR,
    ACCEL,
    GYRO,
};

enum class StreamFormat : uint8_t {
    INVALID,
    Z16,
    RGB8,
    MOTION_XYZ32F,
};

using SerialNumber = SmallString<24>;
using DeviceName = SmallString<24>;

struct StreamId {
    SerialNumber serial_number;
    StreamType type = StreamType::INVALID;
    int index = 0;

    bool is_depth() const { return type == StreamType::DEPTH; }
    bool is_color() const { return type == StreamType::COLOR; }
    bool is_gyro() const { return type == StreamType::GYRO; }
    bool is_accel() const { return type == StreamType::ACCEL; }
};

inline bool operator==(const StreamId& a, const StreamId& b) {
    return std::tuple(a.serial_number, a.type, a.index) ==
           std::tuple(b.serial_number, b.type, b.index);
}

template <typename H>
H AbslHashValue(H h, const StreamId& m) {
    return H::combine(std::move(h), std::string_view(m.serial_number), m.type,
                      m.index);
};

struct Intrinsics {
    int width = 0;
    int height = 0;
    float ppx = 0;
    float ppy = 0;
    float fx = 0;
    float fy = 0;
};

inline bool operator==(const Intrinsics& a, const Intrinsics& b) {
    return std::tuple(a.width, a.height, a.ppx, a.ppy, a.fx, a.fy) ==
           std::tuple(b.width, b.height, b.ppx, b.ppy, b.fx, b.fy);
}

struct StreamMeta {
    StreamId id;
    DeviceName device_name;
    StreamFormat format = StreamFormat::INVALID;
    uint16_t fps = 0;
    Intrinsics intrinsics;
    std::array<float, 16> extrinsics = {0};  // tx_device_sensor, 4x4 transform
                                             // matrix, column major
    float depth_scale = 0;

    bool is_depth() const { return id.is_depth(); }
    bool is_color() const { return id.is_color(); }
    bool is_gyro() const { return id.is_gyro(); }
    bool is_accel() const { return id.is_accel(); }
};

inline bool operator==(const StreamMeta& a, const StreamMeta& b) {
    if (a.id != b.id) return false;
    if (a.device_name != b.device_name) return false;
    if (a.fps != b.fps) return false;
    if (a.intrinsics != b.intrinsics) return false;
    if (a.extrinsics != b.extrinsics) return false;
    if (a.depth_scale != b.depth_scale) return false;
    return true;
}

std::ostream& operator<<(std::ostream& os, const StreamMeta& streamMeta);

std::ostream& operator<<(std::ostream& os, const StreamId& streamId);

std::ostream& operator<<(std::ostream& os, const Intrinsics& intrinsics);

}  // namespace realsense_streaming
}  // namespace axby
