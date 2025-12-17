#include "client.h"

#include <yuv_rgb.h>

#include <array>
#include <atomic>
#include <future>
#include <magic_enum.hpp>
#include <mutex>
#include <thread>
#include <vector>
#include <zdepth.hpp>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "concurrency/single_item.h"
#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "messages.h"
#include "network_config/config.h"

#include "seq/seq.h"
#include "time_sync/time_sync.h"
#include "wrappers/vpx.h"
#include "absl/strings/match.h"

namespace axby {
namespace realsense_streaming {
namespace client {

constexpr auto INVALID_SEQUENCE_ID = std::numeric_limits<uint64_t>::max();

const bool receive_depth = true;
const bool receive_color = true;
const bool receive_accel = true;
const bool receive_gyro = true;

bool _initted = false;

pubsub::SubscriberBuffer _depth_buffer;
pubsub::SubscriberBuffer _color_buffer;
pubsub::SubscriberBuffer _motion_buffer;

std::jthread _depth_thread;
std::jthread _color_thread;
std::jthread _motion_thread;

std::mutex _depth_items_mutex;
std::mutex _color_items_mutex;
std::mutex _accel_items_mutex;
std::mutex _gyro_items_mutex;
absl::flat_hash_map<SerialNumber, std::shared_ptr<SingleItem<DepthData>>>
    _serial_to_depth_item;
absl::flat_hash_map<SerialNumber, std::shared_ptr<SingleItem<ColorData>>>
    _serial_to_color_item;
absl::flat_hash_map<SerialNumber, std::shared_ptr<SingleItem<MotionData>>>
    _serial_to_accel_item;
absl::flat_hash_map<SerialNumber, std::shared_ptr<SingleItem<MotionData>>>
    _serial_to_gyro_item;

std::mutex _serial_numbers_mutex;
absl::flat_hash_set<SerialNumber> _serial_numbers;

void run_depth_thread() {
    struct DepthProcessingContext {
        zdepth::DepthCompressor decompressor;
        uint64_t last_sequence_id = INVALID_SEQUENCE_ID;
        bool need_keyframe = true;
        std::shared_ptr<SingleItem<DepthData>> output_item = nullptr;
    };

    absl::flat_hash_map<std::string, DepthProcessingContext> serial_to_context;
    FastResizableVector<uint16_t> depth_out;

    while (!should_stop_all()) {
        pubsub::Message message;
        if (!_depth_buffer.move_read(message, /*blocking=*/true)) return;
        CHECK_EQ(message.header.message_version, 0) << "Unsupported version";

        const bool is_keyframe = message.header.flags > 0;
        const auto creation_us = message.get_simple<uint64_t>(0);
        const auto sequence_id = message.get_simple<uint64_t>(1);
        const auto stream_meta = message.get_simple<StreamMeta>(2);
        const auto& packet = message.frames[3];

        if (!serial_to_context.count(stream_meta.id.serial_number)) {
            auto& context = serial_to_context[stream_meta.id.serial_number];
            context.output_item = std::make_shared<SingleItem<DepthData>>();

            {
                std::lock_guard<std::mutex> lock{_depth_items_mutex};
                CHECK(
                    !_serial_to_depth_item.count(stream_meta.id.serial_number));
                _serial_to_depth_item[stream_meta.id.serial_number] =
                    context.output_item;
            }
            {
                std::lock_guard<std::mutex> lock{_serial_numbers_mutex};
                _serial_numbers.insert(stream_meta.id.serial_number);
            }
        }

        auto& context = serial_to_context.at(stream_meta.id.serial_number);

        if (context.last_sequence_id != INVALID_SEQUENCE_ID) {
            if (context.last_sequence_id + 1 != sequence_id) {
                LOG(INFO) << stream_meta.id << " frame drop, sequence id "
                          << sequence_id << " and last squence id "
                          << context.last_sequence_id;
                context.need_keyframe = true;
                context.last_sequence_id = INVALID_SEQUENCE_ID;
                context.decompressor = {};  // re-init
            }
        }

        if (context.need_keyframe && !is_keyframe) {
            LOG_EVERY_T(INFO, 1) << stream_meta.id << " waiting for keyframe";
            // we have not gotten a keyframe yet
            // cannot ingest this packet
            continue;
        }

        if (context.need_keyframe) {
            CHECK(is_keyframe);
            CHECK(zdepth::IsKeyFrame((const uint8_t*)packet.data(),
                                     packet.size()));
        }

        context.need_keyframe = false;
        context.last_sequence_id = sequence_id;

        depth_out.resize(stream_meta.intrinsics.width *
                         stream_meta.intrinsics.height);
        auto result = context.decompressor.Decompress(
            (uint8_t*)packet.data(), packet.size(),
            stream_meta.intrinsics.width, stream_meta.intrinsics.height,
            depth_out);

        if (result != zdepth::DepthResult::Success) {
            // possible frame drop?
            context.need_keyframe = true;
            context.decompressor = {};  // re-init
            context.last_sequence_id = INVALID_SEQUENCE_ID;
            LOG(WARNING) << "Could not decode depth frame because "
                         << magic_enum::enum_name(result)
                         << ". Waiting for next keyframe";
            continue;
        }

        context.output_item->write_func([&](DepthData& depth) {
            depth.topic = message.topic;
            depth.process_id = message.header.sender_process_id;
            depth.creation_timestamp_us = creation_us;
            depth.stream_meta = stream_meta;
            depth.sequence_id = sequence_id;
            std::swap(depth.data, depth_out);
        });
    }
}

void run_color_thread() {
    struct ColorProcessingContext {
        std::shared_ptr<vpx_codec_ctx> decoder;
        uint64_t last_sequence_id = INVALID_SEQUENCE_ID;
        bool need_keyframe = true;
        std::shared_ptr<SingleItem<ColorData>> output_item = nullptr;
    };

    absl::flat_hash_map<std::string, ColorProcessingContext> serial_to_context;
    FastResizableVector<uint8_t> color_out;

    while (!should_stop_all()) {
        pubsub::Message message;
        if (!_color_buffer.move_read(message, /*blocking=*/true)) return;
        CHECK_EQ(message.header.message_version, 0) << "Unsupported version";

        const bool is_keyframe = message.header.flags > 0;
        const auto creation_us = message.get_simple<uint64_t>(0);
        const auto sequence_id = message.get_simple<uint64_t>(1);
        const auto stream_meta = message.get_simple<StreamMeta>(2);
        const auto& packet = message.frames[3];

        if (!serial_to_context.count(stream_meta.id.serial_number)) {
            auto& context = serial_to_context[stream_meta.id.serial_number];
            context.output_item = std::make_shared<SingleItem<ColorData>>();
            context.decoder = init_vpx_decoder();

            {
                std::lock_guard<std::mutex> lock{_color_items_mutex};
                _serial_to_color_item[stream_meta.id.serial_number] =
                    context.output_item;
            }
            {
                std::lock_guard<std::mutex> lock{_serial_numbers_mutex};
                _serial_numbers.insert(stream_meta.id.serial_number);
            }
        }

        auto& context = serial_to_context.at(stream_meta.id.serial_number);

        if (context.last_sequence_id != INVALID_SEQUENCE_ID) {
            if (context.last_sequence_id + 1 != sequence_id) {
                LOG(INFO) << stream_meta.id << " frame drop, sequence id "
                          << sequence_id << " and last squence id "
                          << context.last_sequence_id;

                context.need_keyframe = true;
                context.last_sequence_id = INVALID_SEQUENCE_ID;
                context.decoder = init_vpx_decoder();
            }
        }

        if (context.need_keyframe && !is_keyframe) {
            LOG_EVERY_T(INFO, 1) << stream_meta.id << " waiting for keyframe";
            // we have not gotten a keyframe yet
            // cannot ingest this packet
            continue;
        }

        context.need_keyframe = false;
        context.last_sequence_id = sequence_id;

        // decode
        const vpx_codec_err_t err = vpx_codec_decode(
            context.decoder.get(), (const uint8_t*)packet.data(), packet.size(),
            /*user_priv=*/nullptr,
            /*deadline=*/0);

        CHECK(err == VPX_CODEC_OK) << vpx_codec_error(context.decoder.get());

        vpx_image_t* img = nullptr;
        vpx_codec_iter_t iter = nullptr;
        while ((img = vpx_codec_get_frame(context.decoder.get(), &iter)) !=
               nullptr) {
            // check frame size
            const int width = img->d_w;
            const int height = img->d_h;
            CHECK(img->fmt == VPX_IMG_FMT_I420);

            // const int y_stride = img->stride[VPX_PLANE_Y];
            // const int u_stride = img->stride[VPX_PLANE_U];
            // const int v_stride = img->stride[VPX_PLANE_V];

            color_out.resize(3 * width * height);
            yuv420_rgb24_sseu(width, height, img->planes[VPX_PLANE_Y],
                              img->planes[VPX_PLANE_U],
                              img->planes[VPX_PLANE_V],
                              img->stride[VPX_PLANE_Y],
                              img->stride[VPX_PLANE_U] /* = v stride */,
                              color_out.data(), 3 * width, YCBCR_601);

            context.output_item->write_func([&](ColorData& color) {
                color.topic = message.topic;
                color.process_id = message.header.sender_process_id;
                color.creation_timestamp_us = creation_us;
                color.stream_meta = stream_meta;
                color.sequence_id = sequence_id;
                std::swap(color_out, color.data);
            });
        }
    }
}

void run_motion_thread() {
    struct MotionProcessingContext {
        uint64_t last_sequence_id = INVALID_SEQUENCE_ID;
        std::shared_ptr<SingleItem<MotionData>> output_gyro_item = nullptr;
        std::shared_ptr<SingleItem<MotionData>> output_accel_item = nullptr;
        FastResizableVector<float> xyzs;
        FastResizableVector<uint64_t> timestamps_us;
    };

    absl::flat_hash_map<std::string, MotionProcessingContext> serial_to_context;

    while (!should_stop_all()) {
        pubsub::Message message;
        if (!_motion_buffer.move_read(message, /*blocking=*/true)) return;
        CHECK_EQ(message.header.message_version, 0) << "Unsupported version";

        const auto sequence_id = message.get_simple<uint64_t>(0);
        const auto stream_meta = message.get_simple<StreamMeta>(1);
        const auto& timestamps_us_bytes = message.frames[2];
        const auto& xyz_bytes = message.frames[3];

        if (!serial_to_context.count(stream_meta.id.serial_number)) {
            auto& context = serial_to_context[stream_meta.id.serial_number];
            context.output_gyro_item =
                std::make_shared<SingleItem<MotionData>>();
            context.output_accel_item =
                std::make_shared<SingleItem<MotionData>>();

            {
                std::lock_guard<std::mutex> lock{_serial_numbers_mutex};
                _serial_numbers.insert(stream_meta.id.serial_number);
            }
            {
                std::lock_guard<std::mutex> lock{_accel_items_mutex};
                _serial_to_accel_item[stream_meta.id.serial_number] =
                    context.output_accel_item;
            }
            {
                std::lock_guard<std::mutex> lock{_gyro_items_mutex};
                _serial_to_gyro_item[stream_meta.id.serial_number] =
                    context.output_gyro_item;
            }
        }

        auto& context = serial_to_context.at(stream_meta.id.serial_number);

        const size_t num_timestamps_us =
            timestamps_us_bytes.size() / sizeof(uint64_t);
        const size_t num_xyzs = xyz_bytes.size() / (3 * sizeof(float));
        CHECK_EQ(num_timestamps_us, num_xyzs);

        context.xyzs.resize(3 * num_xyzs);
        context.timestamps_us.resize(num_timestamps_us);

        std::memcpy(/*dst=*/context.timestamps_us.data(),
                    /*src=*/timestamps_us_bytes.data(),
                    timestamps_us_bytes.size());

        std::memcpy(/*dst=*/context.xyzs.data(),
                    /*src=*/xyz_bytes.data(), xyz_bytes.size());

        const auto& output_item = stream_meta.is_accel()
                                      ? context.output_accel_item
                                      : context.output_gyro_item;

        output_item->write_func([&](MotionData& motion) {
            motion.topic = message.topic;
            motion.process_id = message.header.sender_process_id;
            motion.sequence_id = sequence_id;
            motion.stream_meta = stream_meta;
            std::swap(motion.xyzs, context.xyzs);
            std::swap(motion.timestamps_us, context.timestamps_us);
        });
    }
}

void update_realsense_list(
    absl::flat_hash_map<SerialNumber, RealsenseState>& realsense_list) {
    std::lock_guard<std::mutex> lock{_serial_numbers_mutex};
    for (auto& serial : _serial_numbers) {
        if (!realsense_list.count(serial)) {
            realsense_list[serial] = {.serial_number = serial};
        }
    }
};

RealsenseStateDidUpdate update_realsense_state(RealsenseState& state) {
    std::lock_guard<std::mutex> lock0{_color_items_mutex};
    std::lock_guard<std::mutex> lock1{_depth_items_mutex};
    std::lock_guard<std::mutex> lock2{_accel_items_mutex};
    std::lock_guard<std::mutex> lock3{_gyro_items_mutex};

    RealsenseStateDidUpdate did_update;

    if (_serial_to_color_item.count(state.serial_number)) {
        auto item = _serial_to_color_item.at(state.serial_number);
        did_update.color = item->swap_read(state.color, /*blocking=*/false);
    }
    if (_serial_to_depth_item.count(state.serial_number)) {
        auto item = _serial_to_depth_item.at(state.serial_number);
        did_update.depth = item->swap_read(state.depth, /*blocking=*/false);
    }
    if (_serial_to_accel_item.count(state.serial_number)) {
        auto item = _serial_to_accel_item.at(state.serial_number);
        did_update.accel = item->swap_read(state.accel, /*blocking=*/false);
    }
    if (_serial_to_gyro_item.count(state.serial_number)) {
        auto item = _serial_to_gyro_item.at(state.serial_number);
        did_update.gyro = item->swap_read(state.gyro, /*blocking=*/false);
    }

    return did_update;
}

void init(const network_config::Config& network_config) {
    CHECK(!_initted);

    auto system_config = network_config.get("realsense");
    CHECK(!system_config.connect.empty());

    pubsub::connect(system_config.connect);

    pubsub::subscribe("realsense/color/", &_color_buffer);
    pubsub::subscribe("realsense/depth/", &_depth_buffer);
    pubsub::subscribe("realsense/gyro/", &_motion_buffer);
    pubsub::subscribe("realsense/accel/", &_motion_buffer);

    _depth_thread = std::jthread{run_depth_thread};
    _color_thread = std::jthread{run_color_thread};
    _motion_thread = std::jthread{run_motion_thread};
    _initted = true;
}

void cleanup() {
    CHECK(_initted);
    stop_all();

    _depth_buffer.stop();
    _color_buffer.stop();
    _motion_buffer.stop();

    if (_depth_thread.joinable()) _depth_thread.join();
    if (_color_thread.joinable()) _color_thread.join();
    if (_motion_thread.joinable()) _motion_thread.join();

    for (auto& [_, item] : _serial_to_depth_item) {
        item->stop();
    }
    for (auto& [_, item] : _serial_to_color_item) {
        item->stop();
    }
    for (auto& [_, item] : _serial_to_gyro_item) {
        item->stop();
    }
    for (auto& [_, item] : _serial_to_accel_item) {
        item->stop();
    }
}

}  // namespace client
}  // namespace realsense_streaming
}  // namespace axby
