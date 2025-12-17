// clang-format off
#include <kissnet.hpp> // for some reason kissnet needs to be first
// clang-format on
#include "app/flag.h"
#include "app/main.h"
#include "app/timing.h"
#include "app/stop_all.h"
#include "debug/log.h"
#include "network_config/config.h"

APP_FLAG(std::string, config_name, "local", "network config name");

int main(int argc, char* argv[]) {
    __APP_MAIN_INIT__;

    APP_UNPACK_FLAG(config_name);

    axby::network_config::Config config{config_name};
    auto system_config = config.get("time_sync");

    LOG(INFO) << "Starting up the time server on " << system_config.kissnet.ip
              << ", port " << system_config.kissnet.port;
    kissnet::udp_socket socket(kissnet::endpoint(system_config.kissnet.ip,
                                                 system_config.kissnet.port));
    socket.bind();

    while (!axby::should_stop_all()) {
        uint64_t recv_msg = 0;
        kissnet::addr_collection client_addr;
        auto [recv_bytes, status] = socket.recv(
            (std::byte*)&recv_msg, sizeof(recv_msg), true, &client_addr);

        if (!status) continue;

        const uint64_t timestamp_us = axby::get_process_time_us();
        std::array<uint64_t, 2> out = {recv_msg, timestamp_us};
        auto [snd_bytes, snd_status] =
            socket.send((std::byte*)out.data(), sizeof(out), &client_addr);

        // LOG(INFO) << "Current time (sec) " << double(timestamp_us) / 1e6;
    }

    // there are ~3e13 microseconds in year the maximum uint64_t
    // integer is ~18e18 so a rollover will occur if the time server
    // runs for longer than 60,000 years continuously

    return 0;
}
