#include "seekbar.h"

#include "app/timing.h"

#include <algorithm>
#include <imgui.h>

namespace axby {

void jump_playing_time(Seekbar& ctx, int time_ms) {
    ctx.current_timestamp_ms = time_ms;
    ctx.last_play_time_ms = std::nullopt;
}

void start_playing(Seekbar& ctx) {
    // CHECK(!ctx.playing);
    ctx.playing = true;
    ctx.last_play_time_ms = std::nullopt;
}

void stop_playing(Seekbar& ctx) {
    // CHECK(ctx.playing);
    ctx.playing = false;
    ctx.last_play_time_ms = std::nullopt;
}

void update_playing(Seekbar& ctx) {
    if (!ctx.playing) return;

    // advance play time
    uint64_t current_time_ms = get_process_time_ms();
    if (ctx.last_play_time_ms.has_value()) {
        uint64_t elapsed_time_ms = current_time_ms - *ctx.last_play_time_ms;
        ctx.current_timestamp_ms += elapsed_time_ms * ctx.playback_speed;
    }
    ctx.last_play_time_ms = current_time_ms;

    if (ctx.current_timestamp_ms > ctx.max_timestamp_ms) {
        // prevent playing past the end of the log
        ctx.current_timestamp_ms = ctx.max_timestamp_ms;
        stop_playing(ctx);
    }
    if (ctx.auto_playback_stop_ms.has_value() &&
        (ctx.current_timestamp_ms >= *ctx.auto_playback_stop_ms)) {
        // stop playing if there is an auto-stop set
        ctx.current_timestamp_ms = *ctx.auto_playback_stop_ms;
        stop_playing(ctx);
        ctx.auto_playback_stop_ms = std::nullopt;
    }
}

void make_seekbar(Seekbar& ctx) {
    if (ctx.current_timestamp_ms < ctx.min_timestamp_ms) {
        ctx.current_timestamp_ms = ctx.min_timestamp_ms;
    }

    uint64_t seconds = ctx.current_timestamp_ms / 1000;
    uint64_t minutes = seconds / 60;
    float seconds_frac =
        (seconds % 60) + float(ctx.current_timestamp_ms % 1000) / 1000.0f;
    ImGui::Text("Display Time %lu:%05.2f", minutes, seconds_frac);

    if (ctx.playing) {
        if (ImGui::Button("Pause")) {
            stop_playing(ctx);
        }
    } else {
        if (ImGui::Button("Play ")) {
            start_playing(ctx);
        }
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::GetWindowWidth() - 40);
    const bool user_seeked =
        ImGui::SliderInt("", &ctx.current_timestamp_ms, ctx.min_timestamp_ms,
                         ctx.max_timestamp_ms);
    ImGui::PopItemWidth();

    if (user_seeked) {
        jump_playing_time(ctx, ctx.current_timestamp_ms);
    }
}

void handle_playback_control(Seekbar& ctx) {
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        // keyboard controls for updating seek state
        // space: toggle playback
        // left: jump back
        // right: jump forward
        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            if (ctx.playing) {
                stop_playing(ctx);
            } else {
                start_playing(ctx);
            }
        }
        const uint64_t seek_jump_ms = 2000;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            uint64_t later_timestamp_ms = std::min<uint64_t>(
                ctx.max_timestamp_ms, ctx.current_timestamp_ms + seek_jump_ms);
            jump_playing_time(ctx, later_timestamp_ms);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            uint64_t earlier_timestamp_ms =
                ctx.current_timestamp_ms - seek_jump_ms;
            if (earlier_timestamp_ms > ctx.current_timestamp_ms) {
                earlier_timestamp_ms = 0;
            }
            jump_playing_time(ctx, earlier_timestamp_ms);
        }
    }
}

}  // axby
