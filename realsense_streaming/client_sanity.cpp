#include "app/flag.h"
#include "app/main.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "network_config/config.h"
#include "realsense_streaming/client.h"
#include "time_sync/time_sync.h"

using namespace axby;
namespace rss = realsense_streaming;

APP_FLAG(std::string, config_name, "local", "network config name");
APP_FLAG(bool, recording, false, "generate a recording");

int main(int argc, char* argv[]) {
    __APP_MAIN_INIT__;

    APP_UNPACK_FLAG(config_name);
    APP_UNPACK_FLAG(recording);

    LOG(INFO) << "Recording? " << recording;

    network_config::Config network_config{config_name};

    pubsub::init();
    if (recording) {
        pubsub::enable_recording();
    }

    time_sync::init(network_config);
    rss::client::init(network_config);

    ActionPeriod refresh_items_period{1.0};

    absl::flat_hash_map<rss::SerialNumber, rss::client::RealsenseState>
        serial_to_realsense_state;

    FrequencyCalculator depth_fps;
    FrequencyCalculator color_fps;
    FrequencyCalculator accel_fps;
    FrequencyCalculator gyro_fps;

    while (!should_stop_all()) {
        if (refresh_items_period.should_act()) {
            rss::client::update_realsense_list(serial_to_realsense_state);
        }

        for (auto& [serial, state] : serial_to_realsense_state) {
            const auto did_update = rss::client::update_realsense_state(state);
            if (did_update.accel) accel_fps.count();
            if (did_update.color) color_fps.count();
            if (did_update.depth) depth_fps.count();
            if (did_update.gyro) gyro_fps.count();
        }

        LOG_EVERY_T(INFO, 1.0) << "\ncolor " << color_fps.get_frequency()
                               << "\ndepth " << depth_fps.get_frequency()
                               << "\naccel " << accel_fps.get_frequency()
                               << "\ngyro " << gyro_fps.get_frequency();
    }

    rss::client::cleanup();
    time_sync::cleanup();
    pubsub::cleanup();

    return 0;
}
