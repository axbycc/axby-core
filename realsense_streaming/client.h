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
#include "realsense_state.h"

namespace axby {
namespace realsense_streaming {
namespace client {


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
