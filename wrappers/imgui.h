#pragma once

#include <imgui.h>

#include "absl/strings/str_format.h"

inline void imgui_progress_bar(const float lb,
                               const float ub,
                               const float value,
                               const char* overlay = nullptr) {
    ImGui::ProgressBar((value - lb) / (ub - lb), ImVec2(-FLT_MIN, 0.0f),
                       overlay);
}

inline bool SliderFloat10x(const char* name,
                           float* value,
                           float low,
                           float high) {
    float value10x = *value * 10;
    bool result = ImGui::SliderFloat(absl::StrFormat("%s (10x)", name).c_str(),
                                     &value10x, low * 10, high * 10);
    *value = value10x / 10;
    return result;
};

float imgui_get_window_bottom() {
    ImVec2 window_size = ImGui::GetWindowSize();
    ImVec2 window_pos = ImGui::GetWindowPos();
    return window_pos.y + window_size.y;
}
