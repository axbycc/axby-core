#include "realsense_streaming/util.h"

#include <librealsense2/h/rs_option.h>
#include <librealsense2/h/rs_sensor.h>
#include <librealsense2/h/rs_types.h>

#include <librealsense2/hpp/rs_frame.hpp>
#include <librealsense2/hpp/rs_pipeline.hpp>
#include <librealsense2/rs.hpp>

#include "math/spatial.h"
#include "debug/log.h"
#include "wrappers/vpx.h"

namespace axby {
namespace realsense_streaming {

OpenSensor::OpenSensor(rs2::sensor sensor,
                       const std::vector<rs2::stream_profile>& profiles) {
    sensor_ = sensor;
    CHECK(sensor_);
    LOG(INFO) << "Opening sensor " << sensor_.get_info(RS2_CAMERA_INFO_NAME)
              << ", " << sensor_.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
    sensor_.open(profiles);
}

void OpenSensor::stop() {
    CHECK(started_);
    CHECK(sensor_);
    sensor_.stop();
}

OpenSensor::OpenSensor(OpenSensor&& other) noexcept {
    sensor_ = std::move(other.sensor_);
    started_ = other.started_;
    other.sensor_ = rs2::sensor();
    other.started_ = false;
}

OpenSensor::~OpenSensor() {
    if (sensor_) {
        LOG(INFO) << "Closing sensor " << sensor_.get_info(RS2_CAMERA_INFO_NAME)
                  << ", " << sensor_.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
        if (started_) {
            sensor_.stop();
        }
        sensor_.close();
    }
}

std::map<uint16_t, std::vector<rs2::stream_profile>>
DeviceConfiguration::make_sensor_idx_to_profiles() const {
    std::map<uint16_t, std::vector<rs2::stream_profile>> sensor_idx_to_profiles;
    sensor_idx_to_profiles[accel_sensor_idx].push_back(accel_profile);
    sensor_idx_to_profiles[gyro_sensor_idx].push_back(gyro_profile);
    sensor_idx_to_profiles[color_sensor_idx].push_back(color_profile);
    sensor_idx_to_profiles[depth_sensor_idx].push_back(depth_profile);
    return sensor_idx_to_profiles;
}

std::vector<DeviceConfiguration> get_device_configurations(
    const DesiredSettings& desired) {
    rs2::context rs_context;
    rs2::device_list devices = rs_context.query_devices();

    std::vector<DeviceConfiguration> out;

    for (rs2::device device : devices) {
        // eg Intel Realsense D455
        LOG(INFO) << "Configuring device "
                  << device.get_info(RS2_CAMERA_INFO_NAME);

        const std::string device_name = device.get_info(RS2_CAMERA_INFO_NAME);
        const std::string device_serial_number =
            device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);

        DeviceConfiguration config;
        config.device = device;

        // walk through the sensors of the device, trying to fill the
        // above profiles with ones that match the desired options
        config.sensors = device.query_sensors();
        CHECK(config.sensors.size() < std::numeric_limits<uint16_t>::max());
        for (uint16_t sensor_idx = 0; sensor_idx < config.sensors.size();
             ++sensor_idx) {
            rs2::sensor sensor = config.sensors[sensor_idx];
            // the sensor name is one of (Stereo Module, RGB Camera, or Motion
            // Module) LOG(INFO) << "\tSensor " <<
            // sensor.get_info(RS2_CAMERA_INFO_NAME);

            for (rs2::stream_profile stream_profile :
                 sensor.get_stream_profiles()) {
                const bool is_color_candidate =
                    (stream_profile.format() == RS2_FORMAT_RGB8) &&
                    (stream_profile.stream_type() == RS2_STREAM_COLOR);

                const bool is_depth_candidate =
                    (stream_profile.format() == RS2_FORMAT_Z16) &&
                    (stream_profile.stream_type() == RS2_STREAM_DEPTH);

                const bool is_accel_candidate =
                    (stream_profile.format() == RS2_FORMAT_MOTION_XYZ32F) &&
                    (stream_profile.stream_type() == RS2_STREAM_ACCEL);

                const bool is_gyro_candidate =
                    (stream_profile.format() == RS2_FORMAT_MOTION_XYZ32F) &&
                    (stream_profile.stream_type() == RS2_STREAM_GYRO);

                if (is_color_candidate) {
                    if (config.color_profile)
                        continue;  // already have a color profile

                    rs2::video_stream_profile vs_profile =
                        stream_profile.as<rs2::video_stream_profile>();
                    CHECK(vs_profile);

                    if (vs_profile.width() != desired.color_width) continue;
                    if (vs_profile.height() != desired.color_height) continue;
                    if (vs_profile.fps() != desired.color_fps) continue;

                    // this profile matches user requirements. select it as the
                    // color profile
                    config.color_profile = vs_profile;
                    config.color_sensor_idx = sensor_idx;

                    // collect all stream information into the stream meta
                    StreamMeta& stream_meta = config.color_stream_meta;
                    stream_meta.id.index = stream_profile.stream_index();
                    stream_meta.id.type =
                        rs_to_stream_type(stream_profile.stream_type());
                    stream_meta.id.serial_number = device_serial_number;
                    stream_meta.device_name = device_name;
                    stream_meta.format =
                        rs_to_stream_format(stream_profile.format());
                    stream_meta.fps = stream_profile.fps();
                    const auto intrinsics = vs_profile.get_intrinsics();
                    stream_meta.intrinsics = {
                        .width = intrinsics.width,
                        .height = intrinsics.height,
                        .ppx = intrinsics.ppx,
                        .ppy = intrinsics.ppy,
                        .fx = intrinsics.fx,
                        .fy = intrinsics.fy,
                    };
                }

                if (is_depth_candidate) {
                    if (config.depth_profile)
                        continue;  // already have a depth profile

                    rs2::video_stream_profile d_profile =
                        stream_profile.as<rs2::video_stream_profile>();
                    CHECK(d_profile);

                    if (d_profile.width() != desired.depth_width) continue;
                    if (d_profile.height() != desired.depth_height) continue;
                    if (d_profile.fps() != desired.depth_fps) continue;

                    // this profile matches user requirements. select it as
                    // depth profile
                    config.depth_profile = d_profile;
                    config.depth_sensor_idx = sensor_idx;

                    // collect all stream information into the stream meta
                    StreamMeta& stream_meta = config.depth_stream_meta;
                    stream_meta.id.index = stream_profile.stream_index();
                    stream_meta.id.type =
                        rs_to_stream_type(stream_profile.stream_type());
                    stream_meta.id.serial_number = device_serial_number;
                    stream_meta.device_name = device_name;
                    stream_meta.format =
                        rs_to_stream_format(stream_profile.format());
                    stream_meta.fps = stream_profile.fps();
                    const auto intrinsics = d_profile.get_intrinsics();
                    stream_meta.intrinsics = {
                        .width = intrinsics.width,
                        .height = intrinsics.height,
                        .ppx = intrinsics.ppx,
                        .ppy = intrinsics.ppy,
                        .fx = intrinsics.fx,
                        .fy = intrinsics.fy,
                    };

                    // additionally pull out the depth scale
                    auto depth_sensor = sensor.as<rs2::depth_sensor>();
                    CHECK(depth_sensor);  // the profile was for a depth stream,
                                          // but it wasn't a depth sensor?
                    stream_meta.depth_scale = depth_sensor.get_depth_scale();
                }

                if (is_accel_candidate) {
                    if (config.accel_profile)
                        continue;  // already have accel profile

                    auto m_profile =
                        stream_profile.as<rs2::motion_stream_profile>();
                    CHECK(m_profile);

                    if (m_profile.fps() != desired.accel_fps) continue;

                    // this profile matches user requirements. select it as
                    // accel profile
                    config.accel_profile = m_profile;
                    config.accel_sensor_idx = sensor_idx;

                    // collect all stream information into the stream meta
                    StreamMeta& stream_meta = config.accel_stream_meta;
                    stream_meta.id.index = stream_profile.stream_index();
                    stream_meta.id.type =
                        rs_to_stream_type(stream_profile.stream_type());
                    stream_meta.id.serial_number = device_serial_number;
                    stream_meta.device_name = device_name;
                    stream_meta.format =
                        rs_to_stream_format(stream_profile.format());
                    stream_meta.fps = stream_profile.fps();
                }

                if (is_gyro_candidate) {
                    if (config.gyro_profile)
                        continue;  // already have gyro profile

                    auto m_profile =
                        stream_profile.as<rs2::motion_stream_profile>();
                    CHECK(m_profile);

                    if (m_profile.fps() != desired.gyro_fps) continue;

                    // this profile matches user requirements. select it as
                    // accel profile
                    config.gyro_profile = m_profile;
                    config.gyro_sensor_idx = sensor_idx;

                    // collect all stream information into the stream meta
                    StreamMeta& stream_meta = config.gyro_stream_meta;
                    stream_meta.id.index = stream_profile.stream_index();
                    stream_meta.id.type =
                        rs_to_stream_type(stream_profile.stream_type());
                    stream_meta.id.serial_number = device_serial_number;
                    stream_meta.device_name = device_name;
                    stream_meta.format =
                        rs_to_stream_format(stream_profile.format());
                    stream_meta.fps = stream_profile.fps();
                }
            }
        }

        LOG_IF(INFO, !config.color_profile) << "Could not find color profile";
        LOG_IF(INFO, !config.depth_profile) << "Could not find depth profile";
        LOG_IF(INFO, !config.accel_profile) << "Could not find accel profile";
        LOG_IF(INFO, !config.gyro_profile) << "Could not find gyro profile";

        if (!(config.color_profile && config.depth_profile &&
              config.accel_profile && config.gyro_profile)) {
            // this device was not fully configured
            LOG(INFO) << "Device could not be properly configured";
            continue;
        }

        // using color stream as the base profile, get transforms from
        // other streams to the color stream and write into the
        // .extrinsics field as 4x4 homogenous transform, col major
        const rs2_extrinsics rs_color_extrinsics =
            config.color_profile.get_extrinsics_to(config.color_profile);
        const rs2_extrinsics rs_depth_extrinsics =
            config.depth_profile.get_extrinsics_to(config.color_profile);
        const rs2_extrinsics rs_accel_extrinsics =
            config.accel_profile.get_extrinsics_to(config.accel_profile);
        const rs2_extrinsics rs_gyro_extrinsics =
            config.gyro_profile.get_extrinsics_to(config.gyro_profile);

        tx_from_rot_trans(rs_color_extrinsics.rotation,
                          rs_color_extrinsics.translation,
                          /*out=*/config.color_stream_meta.extrinsics);
        tx_from_rot_trans(rs_depth_extrinsics.rotation,
                          rs_depth_extrinsics.translation,
                          /*out=*/config.depth_stream_meta.extrinsics);
        tx_from_rot_trans(rs_accel_extrinsics.rotation,
                          rs_accel_extrinsics.translation,
                          /*out-*/ config.accel_stream_meta.extrinsics);
        tx_from_rot_trans(rs_gyro_extrinsics.rotation,
                          rs_gyro_extrinsics.translation,
                          /*out-*/ config.gyro_stream_meta.extrinsics);

        out.push_back(config);
    }

    return out;
}

int get_profile_uid_from_frame(const rs2_frame* f) {
    rs2_error* error0 = nullptr;
    const rs2_stream_profile* profile =
        rs2_get_frame_stream_profile(f, &error0);
    if (error0) {
        LOG(FATAL) << "Error " << rs2_get_error_message(error0);
        rs2_free_error(error0);
    }
    rs2_stream stream_type;
    rs2_format format;
    int index;
    int uid;
    int fps;
    rs2_error* error1 = nullptr;
    rs2_get_stream_profile_data(profile, &stream_type, &format, &index, &uid,
                                &fps, &error1);
    if (error0) {
        LOG(FATAL) << "Error " << rs2_get_error_message(error1);
        rs2_free_error(error1);
    }
    return uid;
}

ColorEncoder::ColorEncoder(unsigned int width, unsigned int height, int fps,
                           unsigned int bitrate, bool lossless) {
    encoder =
        init_vpx_encoder(/*profile=*/0, width, height, fps, bitrate, lossless);
    buffer = init_vpx_img(VPX_IMG_FMT_I420, width, height, /*align=*/0);
};

}  // namespace realsense_streaming
}
