#pragma once

#include <librealsense2/h/rs_sensor.h>
#include <librealsense2/h/rs_types.h>
#include <vpx/vp8cx.h>
#include <vpx/vpx_encoder.h>

#include <librealsense2/rs.hpp>
#include <map>
#include <vector>
#include <zdepth.hpp>

#include "app/timing.h"
#include "messages.h"

namespace axby {
namespace realsense_streaming {
inline StreamType rs_to_stream_type(rs2_stream type) {
    if (type == RS2_STREAM_DEPTH) return StreamType::DEPTH;
    if (type == RS2_STREAM_COLOR) return StreamType::COLOR;
    if (type == RS2_STREAM_ACCEL) return StreamType::ACCEL;
    if (type == RS2_STREAM_GYRO) return StreamType::GYRO;
    return StreamType::INVALID;  // todo
};
inline rs2_stream rs_from_stream_type(StreamType type) {
    if (type == StreamType::DEPTH) return RS2_STREAM_DEPTH;
    if (type == StreamType::COLOR) return RS2_STREAM_COLOR;
    if (type == StreamType::ACCEL) return RS2_STREAM_ACCEL;
    if (type == StreamType::GYRO) return RS2_STREAM_GYRO;
    return RS2_STREAM_ANY;  // todo
};

inline StreamFormat rs_to_stream_format(rs2_format format) {
    if (format == RS2_FORMAT_Z16) return StreamFormat::Z16;
    if (format == RS2_FORMAT_RGB8) return StreamFormat::RGB8;
    if (format == RS2_FORMAT_MOTION_XYZ32F) return StreamFormat::MOTION_XYZ32F;
    return StreamFormat::INVALID;  // todo
};

inline rs2_format rs_from_stream_format(StreamFormat format) {
    if (format == StreamFormat::Z16) return RS2_FORMAT_Z16;
    if (format == StreamFormat::RGB8) return RS2_FORMAT_RGB8;
    if (format == StreamFormat::MOTION_XYZ32F) return RS2_FORMAT_MOTION_XYZ32F;
    return RS2_FORMAT_ANY;  // todo
};

// RAII wrapper that opens a sensor on construction and closes a
// sensor on destruction
struct OpenSensor {
    OpenSensor(rs2::sensor sensor,
               const std::vector<rs2::stream_profile>& profiles);
    OpenSensor(const OpenSensor&) = delete;

    template <typename T>
    void start(T t) {
        CHECK(sensor_);
        CHECK(!started_);
        sensor_.start(t);
        started_ = true;
    }

    void stop();
    OpenSensor(OpenSensor&& other) noexcept;
    ~OpenSensor();

   private:
    bool started_ = false;
    rs2::sensor sensor_;
};

struct DeviceConfiguration {
    rs2::device device;
    std::vector<rs2::sensor> sensors;

    rs2::stream_profile color_profile;
    uint16_t color_sensor_idx = 0;
    StreamMeta color_stream_meta;

    float depth_scale = 0;
    rs2::stream_profile depth_profile;
    uint16_t depth_sensor_idx = 0;
    StreamMeta depth_stream_meta;

    rs2::stream_profile gyro_profile;
    uint16_t gyro_sensor_idx = 0;
    StreamMeta gyro_stream_meta;

    rs2::stream_profile accel_profile;
    uint16_t accel_sensor_idx = 0;
    StreamMeta accel_stream_meta;

    // organize profiles as sensor => list of profiles
    std::map<uint16_t, std::vector<rs2::stream_profile>>
    make_sensor_idx_to_profiles() const;
};

struct DesiredSettings {
    int color_width = 640;
    int color_height = 480;
    int color_fps = 30;

    int depth_width = 640;
    int depth_height = 480;
    int depth_fps = 30;

    int gyro_fps = 200;
    int accel_fps = 200;
};

std::vector<DeviceConfiguration> get_device_configurations(
    const DesiredSettings& desired);

int get_profile_uid_from_frame(const rs2_frame* f);

struct ColorEncoder {
    ColorEncoder() = default;
    ColorEncoder(unsigned int width,
                 unsigned int height,
                 int fps,
                 unsigned int bitrate,
                 bool lossless = false);

    uint64_t sequence_id = 0;

    std::shared_ptr<vpx_codec_ctx_t> encoder = nullptr;
    std::shared_ptr<vpx_image_t> buffer = nullptr;

    ActionPeriod keyframe_period{2.0};  // seconds
};

struct DepthEncoder {
    DepthEncoder() = default;

    uint64_t sequence_id = 0;

    zdepth::DepthCompressor encoder;
    std::vector<uint8_t> buffer;
    ActionPeriod keyframe_period{2.0};  // seconds
};

}  // namespace realsense_streaming
}  // namespace axby
