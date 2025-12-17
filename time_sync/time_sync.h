#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "network_config/config.h"

namespace axby {
namespace time_sync {

struct Options {
    double window_duration_sec;
    int blast_size = 20;

    static Options Default(const std::string& server_ip = "127.0.0.1",
                           uint16_t port = 7777) {
        return {
            .window_duration_sec = 1.25,
            .blast_size = 5,
        };
    }
};

void init(const network_config::Config& config,
          const Options& options = Options::Default());

// used for playback
void start_without_time_server();

uint64_t estimate_time_server_timestamp_us();
uint64_t estimate_time_server_timestamp_ms();

// estimate the time server timestamp using another process' local
// process time. this is only accurate if the other process' local
// process time is close to its current local process time because of
// clock drift.
uint64_t estimate_time_server_timestamp_us(uint64_t process_id,
                                           uint64_t process_time_us);

uint64_t estimate_time_server_timestamp_ms(uint64_t process_id,
                                           uint64_t process_time_ms);

int64_t estimate_offset_us();
int64_t estimate_offset_ms();

void cleanup();

}  // namespace time_sync
}  // namespace axby
