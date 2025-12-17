#include <chrono>
#include <thread>
#include <zmq.hpp>

#include "app/flag.h"
#include "app/main.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "debug/log.h"
#include "network_config/config.h"
#include "time_sync.h"

APP_FLAG(std::string, config_name, "local", "network config name");

APP_FLAG(double,
         window_duration,
         1.25,
         "sliding window duration for historical measurements, seconds");

APP_FLAG(uint16_t,
         blast_size,
         20,
         "Number of packets to blast at one time in the time_sync send thread");

using namespace axby;
int main(int argc, char* argv[]) {
    __APP_MAIN_INIT__;

    APP_UNPACK_FLAG(window_duration);
    APP_UNPACK_FLAG(blast_size);
    APP_UNPACK_FLAG(config_name);

    network_config::Config config{config_name};

    pubsub::init();
    time_sync::init(config,
                    time_sync::Options{.window_duration_sec = window_duration,
                                       .blast_size = blast_size});

    while (!should_stop_all()) {
        const uint64_t remote_ts_ms =
            time_sync::estimate_time_server_timestamp_ms();
        const double remote_ts_sec = double(remote_ts_ms % 10000) / 1000;
        LOG(INFO) << "remote ts (sec) " << remote_ts_sec << ", offset (ms) "
                  << time_sync::estimate_offset_ms();
        sleep_ms(1000);
    }

    time_sync::cleanup();
    pubsub::cleanup();

    return 0;
}
