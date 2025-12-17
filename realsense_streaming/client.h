#pragma once

#include <limits>
#include <span>
#include <string_view>
#include <vector>
#include <zmq.hpp>

#include "absl/container/flat_hash_map.h"
#include "concurrency/single_item.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "network_config/config.h"
#include "messages.h"

namespace axby {
namespace realsense_streaming {
namespace client {

struct DepthData {
    std::string topic;
    uint64_t process_id = 0;
    uint64_t creation_timestamp_us = 0;

    uint64_t sequence_id = 0;
    StreamMeta stream_meta;
    FastResizableVector<uint16_t> data;
};

struct ColorData {
    std::string topic;
    uint64_t process_id = 0;
    uint64_t creation_timestamp_us = 0;    

    uint64_t sequence_id = 0;
    StreamMeta stream_meta;
    FastResizableVector<uint8_t> data;
};

struct MotionData {
    std::string topic;
    uint64_t process_id = 0;

    uint64_t sequence_id = 0;
    StreamMeta stream_meta;
    FastResizableVector<uint64_t> timestamps_us;
    FastResizableVector<float> xyzs;
};

struct RealsenseState {
    SerialNumber serial_number;
    DepthData depth;
    ColorData color;
    MotionData accel;
    MotionData gyro;
};

struct RealsenseStateDidUpdate {
    bool depth = false;
    bool color = false;
    bool accel = false;
    bool gyro = false;
};

void init(const network_config::Config& network_config);
void update_realsense_list(absl::flat_hash_map<SerialNumber, RealsenseState>&
                           serial_to_realsense_state);
RealsenseStateDidUpdate update_realsense_state(RealsenseState& state);
void cleanup();

}  // namespace client
}  // namespace realsense_streaming
}  // namespace axby
