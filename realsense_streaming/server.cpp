#include <librealsense2/h/rs_option.h>
#include <librealsense2/h/rs_sensor.h>
#include <librealsense2/h/rs_types.h>
#include <yuv_rgb.h>

#include <librealsense2/hpp/rs_frame.hpp>
#include <librealsense2/hpp/rs_pipeline.hpp>
#include <librealsense2/rs.hpp>
#include <magic_enum.hpp>
#include <map>
#include <mutex>
#include <optional>
#include <thread>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_format.h"
#include "app/flag.h"
#include "app/main.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "concurrency/ring_buffer.h"
#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "math/spatial.h"
#include "network_config/config.h"
#include "time_sync/time_sync.h"
#include "util.h"
#include "wrappers/vpx.h"

APP_FLAG(bool, verbose, false, "verbose mode");
APP_FLAG(int,
         color_bitrate,
         72,
         "kbps target for the color video stream compression");
APP_FLAG(std::string, color_size, "small", "small=640x480, large=1280x720");
APP_FLAG(int,
         color_fps,
         30,
         "each resolution supports different fps, 30 is common");
APP_FLAG(std::string, depth_size, "small", "small=640x480, large=1280x720");
APP_FLAG(int,
         depth_fps,
         30,
         "each resolution supports different fps, 30 is common");

APP_FLAG(std::string, config_name, "local", "network config name.");

using namespace axby;
using namespace realsense_streaming;

std::mutex report_mutex_;
bool verbose_ = false;
FrequencyCalculator bandwidth_report_;

std::string get_topic(const StreamId& stream_id) {
    if (stream_id.type == StreamType::COLOR) {
        return absl::StrFormat("realsense/color/%s/%d",
                               std::string(stream_id.serial_number),
                               stream_id.index);
    }
    if (stream_id.type == StreamType::DEPTH) {
        return absl::StrFormat("realsense/depth/%s/%d",
                               std::string(stream_id.serial_number),
                               stream_id.index);
    }
    if (stream_id.type == StreamType::GYRO) {
        return absl::StrFormat("realsense/gyro/%s/%d",
                               std::string(stream_id.serial_number),
                               stream_id.index);
    }
    if (stream_id.type == StreamType::ACCEL) {
        return absl::StrFormat("realsense/accel/%s/%d",
                               std::string(stream_id.serial_number),
                               stream_id.index);
    }
    LOG(FATAL) << "Unhandled stream type "
               << magic_enum::enum_name(stream_id.type);
    return "";
    // unreachable not supported by gcc11
    // std::unreachable();
}

struct FrameData {
    int uid = 0;
    uint64_t creation_timestamp_us = 0;
    rs2::frame frame;
};

struct MotionEncoder {
    uint64_t sequence_id = 0;
    FastResizableVector<uint64_t> timestamps_us;
    FastResizableVector<float> xyzs;
};

std::mutex _color_ring_buffer_mutex;
RingBuffer<FrameData, 10> _color_ring_buffer;
void process_color_frames(
    const absl::flat_hash_map<int, StreamMeta>& uid_to_stream_meta,
    const absl::flat_hash_map<int, std::string>& uid_to_topic,
    absl::flat_hash_map<int, ColorEncoder>& uid_to_color_encoder,
    absl::flat_hash_map<int, FrequencyCalculator>& uid_to_fps_report) {
    while (!should_stop_all()) {
        FrameData frame_data;
        if (!_color_ring_buffer.move_read(frame_data, /*blocking=*/true))
            return;

        const auto& frame = frame_data.frame;
        const int uid = frame_data.uid;
        const auto creation_timestamp_us = frame_data.creation_timestamp_us;

        const StreamMeta& stream_meta = uid_to_stream_meta.at(uid);
        const auto& topic = uid_to_topic.at(uid);
        ColorEncoder& encoder = uid_to_color_encoder.at(uid);

        // unpack encoder variables
        auto& encoder_image_buffer = encoder.buffer;
        auto& video_encoder = encoder.encoder;
        const auto sequence_id = encoder.sequence_id++;
        auto& keyframe_period = encoder.keyframe_period;

        // encode this packet
        const auto width = stream_meta.intrinsics.width;
        const auto height = stream_meta.intrinsics.height;
        CHECK(encoder_image_buffer->d_w == width);
        CHECK(encoder_image_buffer->d_h == height);
        CHECK(encoder_image_buffer->fmt == VPX_IMG_FMT_I420);

        const uint8_t* rs_data = (const uint8_t*)frame.get_data();
        const size_t rs_data_size = frame.get_data_size();

        if (stream_meta.format == StreamFormat::RGB8) {
            auto video_frame = frame.as<rs2::video_frame>();
            const int actual_width = video_frame.get_width();
            const int actual_height = video_frame.get_height();
            CHECK(actual_width == width)
                << "expected " << width << "and got " << actual_width;
            CHECK(actual_height == height)
                << "expected " << height << " and got " << actual_height;

            CHECK(rs_data_size == width * height * 3)
                << "rs_data_size was " << rs_data_size << " and expected "
                << width * height * 3;  // 3 bytes per pixel
            // convert rgb8 into I420
            rgb24_yuv420_sseu(
                width, height, rs_data,
                /*rgb_stride=*/3 * width,
                encoder_image_buffer->planes[VPX_PLANE_Y],
                encoder_image_buffer->planes[VPX_PLANE_U],
                encoder_image_buffer->planes[VPX_PLANE_V],
                encoder_image_buffer->stride[VPX_PLANE_Y],
                /*uv_stride=*/
                encoder_image_buffer
                    ->stride[VPX_PLANE_U], /*u stride == v_stride*/
                YCBCR_601);
        } else {
            LOG(FATAL) << "Unsupported image format "
                       << magic_enum::enum_name(stream_meta.format);
            // todo: support realsense YUV by direct copying
            // into vpx frame?
        }

        vpx_codec_iter_t iter = nullptr;
        const vpx_codec_cx_pkt_t* pkt = nullptr;

        // copied from google udpsample project
        const unsigned int RECOVERY_FLAGS[] = {
            0,                   //   NORMAL,
            VPX_EFLAG_FORCE_KF,  //   KEY,
            VP8_EFLAG_FORCE_GF | VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_REF_LAST |
                VP8_EFLAG_NO_REF_ARF,  //   GOLD = 2,
            VP8_EFLAG_FORCE_ARF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_REF_LAST |
                VP8_EFLAG_NO_REF_GF  //   ALTREF = 3
        };
        // const int NORMAL = 0;
        const int KEY = 1;
        // const int GOLD = 2;
        // const int ALTREF = 3;

        int flags = 0;
        const bool using_keyframe = keyframe_period.should_act();
        if (using_keyframe) {
            flags = RECOVERY_FLAGS[KEY];
            LOG_IF(INFO, verbose_)
                << "Sending color keyframe for " << stream_meta.id;
        }

        // Quality arguments
        // - VPX_DL_REALTIME
        // - VPX_DL_GOOD_QUALITY
        // - VPX_DL_BEST_QUALITY
        const vpx_codec_err_t res =
            vpx_codec_encode(video_encoder.get(), encoder_image_buffer.get(),
                             /*pts=*/sequence_id,
                             /*duration=*/1,
                             /*flags=*/flags, VPX_DL_REALTIME);

        CHECK(res == VPX_CODEC_OK) << vpx_codec_error(video_encoder.get());

        while ((pkt = vpx_codec_get_cx_data(video_encoder.get(), &iter)) !=
               nullptr) {
            if (pkt->kind != VPX_CODEC_CX_FRAME_PKT) {
                // the other packet kinds are irrelevant
                // VPX_CODEC_CX_FRAME_PKT: Compressed video
                // VPX_CODEC_STATS_PKT: Two-pass statistics
                // VPX_CODEC_FPMB_STATS_PKT: first pass mb
                // statistics VPX_CODEC_PSNR_PKT: PSNR statistics
                // VPX_CODEC_CUSTOM_PKT = 256:  Algorithm extensions
                continue;
            }

            // found the first VPX_CODEC_CX_FRAME_PKT
            break;
        }

        // we break out of the previous loop since it seems like in realtime
        // mode, every frame corresponds to exactly one packet. if the CHECK
        // below triggers, it means the assumption is violated and we update the
        // code to keep a vector of serialized messages
        {
            const vpx_codec_cx_pkt_t* next_pkt = nullptr;
            next_pkt = vpx_codec_get_cx_data(video_encoder.get(), &iter);
            CHECK(next_pkt == nullptr);
        }

        pubsub::MessageFrames message_frames;
        message_frames.add_simple(creation_timestamp_us);
        message_frames.add_simple(sequence_id);
        message_frames.add_simple(stream_meta);
        message_frames.add_bytes(pkt);
        const size_t frame_size = message_frames.size();

        pubsub::publish_frames(topic, 0, std::move(message_frames),
                               using_keyframe);

        std::lock_guard<std::mutex> lock(report_mutex_);
        uid_to_fps_report.at(frame_data.uid).count();
        bandwidth_report_.count(frame_size);
    }
}

std::mutex _depth_ring_buffer_mutex;
RingBuffer<FrameData, 10> _depth_ring_buffer;
void process_depth_frames(
    const absl::flat_hash_map<int, StreamMeta>& uid_to_stream_meta,
    const absl::flat_hash_map<int, std::string>& uid_to_topic,
    absl::flat_hash_map<int, DepthEncoder>& uid_to_depth_encoder,
    absl::flat_hash_map<int, FrequencyCalculator>& uid_to_fps_report) {
    while (!should_stop_all()) {
        FrameData frame_data;
        if (!_depth_ring_buffer.move_read(frame_data, /*blocking=*/true))
            return;

        const StreamMeta& stream_meta = uid_to_stream_meta.at(frame_data.uid);
        const auto& topic = uid_to_topic.at(frame_data.uid);
        const auto creation_timestamp_us = frame_data.creation_timestamp_us;
        auto& encoder = uid_to_depth_encoder.at(frame_data.uid);
        auto& depth_encoder = encoder.encoder;
        const auto sequence_id = encoder.sequence_id++;
        auto& buffer = encoder.buffer;
        auto& keyframe_period = encoder.keyframe_period;

        bool request_keyframe = keyframe_period.should_act();
        const int width = stream_meta.intrinsics.width;
        const int height = stream_meta.intrinsics.height;

        const int expected_data_size = width * height * sizeof(uint16_t);
        LOG_IF(INFO, expected_data_size != frame_data.frame.get_data_size())
            << "Expected data size " << expected_data_size << ", actual "
            << frame_data.frame.get_data_size();

        depth_encoder.Compress(width, height,
                               (uint16_t*)frame_data.frame.get_data(), buffer,
                               request_keyframe);

        const bool have_keyframe =
            zdepth::IsKeyFrame(buffer.data(), buffer.size());
        if (request_keyframe) {
            // sanity check
            CHECK(have_keyframe);
        }

        pubsub::MessageFrames message_frames;
        message_frames.add_simple(creation_timestamp_us);
        message_frames.add_simple(sequence_id);
        message_frames.add_simple(stream_meta);
        message_frames.add_bytes(buffer);
        const size_t message_size = message_frames.size();

        pubsub::publish_frames(topic, 0, std::move(message_frames),
                               have_keyframe);

        std::lock_guard<std::mutex> lock(report_mutex_);
        uid_to_fps_report.at(frame_data.uid).count();
        bandwidth_report_.count(message_size);
    }
}

std::mutex _motion_ring_buffer_mutex;
RingBuffer<FrameData, 100> _motion_ring_buffer;
void process_motion_frames(
    const absl::flat_hash_map<int, std::string>& uid_to_topic,
    const absl::flat_hash_map<int, StreamMeta>& uid_to_stream_meta,
    absl::flat_hash_map<int, MotionEncoder>& uid_to_motion_encoder,
    absl::flat_hash_map<int, FrequencyCalculator>& uid_to_fps_report) {
    while (!should_stop_all()) {
        FrameData frame_data;
        if (!_motion_ring_buffer.move_read(frame_data, /*blocking=*/true))
            return;

        const auto& topic = uid_to_topic.at(frame_data.uid);
        const auto& stream_meta = uid_to_stream_meta.at(frame_data.uid);
        const auto creation_timestamp_us = frame_data.creation_timestamp_us;
        const rs2_vector motion_data =
            frame_data.frame.as<rs2::motion_frame>().get_motion_data();

        auto& motion_encoder = uid_to_motion_encoder.at(frame_data.uid);
        motion_encoder.timestamps_us.push_back(creation_timestamp_us);
        motion_encoder.xyzs.push_back(motion_data.x);
        motion_encoder.xyzs.push_back(motion_data.y);
        motion_encoder.xyzs.push_back(motion_data.z);

        size_t message_frames_size = 0;
        const auto first_timestamp_us = motion_encoder.timestamps_us.front();
        if (creation_timestamp_us > first_timestamp_us + 1e6 * 0.033) {
            pubsub::MessageFrames message_frames;
            message_frames.add_simple(motion_encoder.sequence_id++);
            message_frames.add_simple(stream_meta);
            message_frames.add_bytes(
                reinterpret_span<std::byte>(motion_encoder.timestamps_us));
            message_frames.add_bytes(
                reinterpret_span<std::byte>(motion_encoder.xyzs));
            message_frames_size = message_frames.size();
            pubsub::publish_frames(topic, 0, std::move(message_frames));

            motion_encoder.timestamps_us.clear();
            motion_encoder.xyzs.clear();
        }

        std::lock_guard<std::mutex> lock(report_mutex_);
        uid_to_fps_report.at(frame_data.uid).count();
        if (message_frames_size) {
            bandwidth_report_.count(message_frames_size);
        }
    }
}

int main(int argc, char* argv[]) {
    __APP_MAIN_INIT__;

    APP_UNPACK_FLAG(config_name);
    APP_UNPACK_FLAG(color_size);
    APP_UNPACK_FLAG(color_bitrate);
    APP_UNPACK_FLAG(color_fps);
    APP_UNPACK_FLAG(depth_size);
    APP_UNPACK_FLAG(depth_fps);
    APP_UNPACK_FLAG(verbose);

    verbose_ = verbose;

    pubsub::init();

    network_config::Config network_config{config_name};

    time_sync::init(network_config);

    CHECK(!network_config.get("realsense").bind.empty());    
    pubsub::bind(network_config.get("realsense").bind);

    DesiredSettings settings;
    settings.color_fps = color_fps;
    settings.depth_fps = depth_fps;
    if (color_size == "small") {
        settings.color_height = 480;
        settings.color_width = 640;
    } else if (color_size == "large") {
        settings.color_height = 720;
        settings.color_width = 1280;
    } else {
        LOG(FATAL) << "Unsupported color size " << color_size;
    }
    if (depth_size == "small") {
        settings.depth_height = 480;
        settings.depth_width = 640;
    } else if (depth_size == "large") {
        settings.depth_height = 720;
        settings.depth_width = 1280;
    } else {
        LOG(FATAL) << "Unsupported depth size " << depth_size;
    }

    auto configs = get_device_configurations(settings);

    // build a lookup of stream metas by stream profile uid this is
    // used in the frame callback to quickly associate a frame back to
    // originating stream_meta.
    absl::flat_hash_map<int, StreamMeta> uid_to_stream_meta;
    for (const DeviceConfiguration& config : configs) {
        uid_to_stream_meta[config.accel_profile.unique_id()] =
            config.accel_stream_meta;
        uid_to_stream_meta[config.color_profile.unique_id()] =
            config.color_stream_meta;
        uid_to_stream_meta[config.depth_profile.unique_id()] =
            config.depth_stream_meta;
        uid_to_stream_meta[config.gyro_profile.unique_id()] =
            config.gyro_stream_meta;
    }

    // create topics for each stream
    absl::flat_hash_map<int, std::string> uid_to_topic;
    for (const auto& [uid, stream_meta] : uid_to_stream_meta) {
        uid_to_topic[uid] = get_topic(stream_meta.id);
    }

    // initialize encoders for streams which need to be compressed
    absl::flat_hash_map<int, ColorEncoder> uid_to_color_encoder;
    absl::flat_hash_map<int, DepthEncoder> uid_to_depth_encoder;
    absl::flat_hash_map<int, MotionEncoder> uid_to_motion_encoder;
    int bitrate = color_bitrate * 1000;  // bits per sec
    for (auto& [uid, stream_meta] : uid_to_stream_meta) {
        if (stream_meta.is_color()) {
            CHECK(!uid_to_color_encoder.count(uid));
            uid_to_color_encoder[uid] = ColorEncoder(
                stream_meta.intrinsics.width, stream_meta.intrinsics.height,
                settings.color_fps, bitrate);
        }
        if (stream_meta.is_depth()) {
            CHECK(!uid_to_depth_encoder.count(uid));
            uid_to_depth_encoder[uid] = DepthEncoder();
        }
        if (stream_meta.is_accel() || stream_meta.is_gyro()) {
            CHECK(!uid_to_motion_encoder.count(uid));
            uid_to_motion_encoder[uid] = {};
        }
    }

    // set phases of the encoders so that they don't all make
    // keyframes at the same time
    const auto num_color_encoders = uid_to_color_encoder.size();
    int color_encoder_idx = 0;
    double color_encoder_base_phase = 0;
    for (auto& [uid, encoder] : uid_to_color_encoder) {
        const double phase = color_encoder_idx *
                             encoder.keyframe_period.get_period() /
                             num_color_encoders;
        if (color_encoder_idx == 1) {
            // save the base phase so we can use it during the depth
            // encoder phase calcuation
            color_encoder_base_phase = phase;
        }
        encoder.keyframe_period.set_phase(phase);
        ++color_encoder_idx;
    }
    const auto num_depth_encoders = uid_to_depth_encoder.size();
    int depth_encoder_idx = 0;
    for (auto& [uid, encoder] : uid_to_depth_encoder) {
        double phase = depth_encoder_idx *
                       encoder.keyframe_period.get_period() /
                       num_depth_encoders;
        // perturb the phase so it doesn't line up with the color encoder phase
        phase += color_encoder_base_phase / 2;
        encoder.keyframe_period.set_phase(phase);
        ++depth_encoder_idx;
    }

    std::vector<OpenSensor> open_sensors;
    for (const DeviceConfiguration& config : configs) {
// windows version of realsense2 needs to be updated and
// doesn't have get_description()
#ifndef _WIN32
        LOG_IF(INFO, verbose_)
            << "Opening device " << config.device.get_description();
#endif
        LOG_IF(INFO, verbose_)
            << "\tThere are " << config.sensors.size() << " sensors";
        const auto sensor_idx_to_profiles =
            config.make_sensor_idx_to_profiles();
        for (uint16_t sensor_idx = 0; sensor_idx < config.sensors.size();
             ++sensor_idx) {
            rs2::sensor sensor = config.sensors[sensor_idx];
            const auto& profiles = sensor_idx_to_profiles.at(sensor_idx);
            LOG_IF(INFO, verbose_)
                << "\tOpening sensor " << sensor.get_info(RS2_CAMERA_INFO_NAME)
                << " (" << profiles.size() << " profiles)";
            for (rs2::stream_profile profile : profiles) {
                LOG_IF(INFO, verbose_) << "\t\t" << profile.unique_id();
            }
            open_sensors.emplace_back(sensor, profiles);
        }
    }

    // start all sensors. the callback takes the frame and writes it
    // into one of three ring buffers. each ring buffer is polled by
    // its own thread.
    for (auto& open_sensor : open_sensors) {
        open_sensor.start([&uid_to_stream_meta](rs2::frame frame) {
            const int uid = get_profile_uid_from_frame(frame.get());
            const auto& stream_meta = uid_to_stream_meta.at(uid);
            const auto creation_timestamp_us = get_process_time_us();

            if (stream_meta.id.type == StreamType::COLOR) {
                std::lock_guard<std::mutex> lock{_color_ring_buffer_mutex};
                _color_ring_buffer.move_write(
                    {.uid = uid,
                     .creation_timestamp_us = creation_timestamp_us,
                     .frame = std::move(frame)});
            }
            if (stream_meta.id.type == StreamType::DEPTH) {
                std::lock_guard<std::mutex> lock{_depth_ring_buffer_mutex};
                _depth_ring_buffer.move_write(
                    {.uid = uid,
                     .creation_timestamp_us = creation_timestamp_us,
                     .frame = std::move(frame)});
            }
            if (stream_meta.id.type == StreamType::ACCEL ||
                stream_meta.id.type == StreamType::GYRO) {
                std::lock_guard<std::mutex> lock{_motion_ring_buffer_mutex};
                _motion_ring_buffer.move_write(
                    {.uid = uid,
                     .creation_timestamp_us = creation_timestamp_us,
                     .frame = std::move(frame)});
            }
        });
    }

    absl::flat_hash_map<int, FrequencyCalculator> uid_to_fps_report;
    for (const auto& [uid, _] : uid_to_stream_meta) {
        uid_to_fps_report[uid];
    }

    on_stop_all([&]() {
        _color_ring_buffer.stop();
        _depth_ring_buffer.stop();
        _motion_ring_buffer.stop();
    });

    // these threads constantly poll the ring buffers and then process
    // and publish the results
    std::thread process_color_frames_thread([&]() {
        process_color_frames(uid_to_stream_meta, uid_to_topic,
                             uid_to_color_encoder, uid_to_fps_report);
    });

    std::thread process_depth_frames_thread([&]() {
        process_depth_frames(uid_to_stream_meta, uid_to_topic,
                             uid_to_depth_encoder, uid_to_fps_report);
    });

    std::thread process_motion_frames_thread([&]() {
        process_motion_frames(uid_to_topic, uid_to_stream_meta,
                              uid_to_motion_encoder, uid_to_fps_report);
    });

    LOG(INFO) << "Press ctrl+c to exit...";
    ActionPeriod fps_report_period(5.0);
    while (!should_stop_all()) {
        // wake-up once in a while to check the stop_all flag and to
        // print fps statistics
        sleep_ms(250);

        if (fps_report_period.should_act()) {
            std::lock_guard<std::mutex> lock(report_mutex_);
            std::cout << "\r\n";
            std::cout << "Bandwidth " << bandwidth_report_.get_frequency() / 1e6
                      << "MB/sec \n";
            for (const auto& [uid, stream_meta] : uid_to_stream_meta) {
                if (!uid_to_fps_report.count(uid)) {
                    std::cout << stream_meta.id << "[ NONE ]\n";
                } else {
                    std::cout << stream_meta.id << "["
                              << uid_to_fps_report.at(uid).get_frequency()
                              << " fps ]\n, ";
                }
            }
        }
    }

    process_color_frames_thread.join();
    process_motion_frames_thread.join();
    process_depth_frames_thread.join();

    time_sync::cleanup();
    pubsub::cleanup();

    LOG(INFO) << "Running destructors";
}
