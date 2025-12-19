#pragma once

#include <cstdint>
#include <optional>

namespace axby {

struct Seekbar {
    uint64_t min_timestamp_ms = 0;
    uint64_t max_timestamp_ms = 0;
    float playback_speed = 1.0f;

    // if this is set, playback will stop when this timestamp is hit
    std::optional<int> auto_playback_stop_ms = std::nullopt;

    // using int instead of uint64_t for imgui integration
    int current_timestamp_ms = 0;
    bool playing = false;
    std::optional<int> last_play_time_ms = std::nullopt;
};


void jump_playing_time(Seekbar& ctx, int time_ms);
void start_playing(Seekbar& ctx);
void stop_playing(Seekbar& ctx);
void update_playing(Seekbar& ctx);
void make_seekbar(Seekbar& ctx);

// imgui keyboard integration for spacebar, left, right jumping
void handle_playback_control(Seekbar& ctx);


}  // namespace axby
