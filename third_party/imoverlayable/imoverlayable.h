#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <utility>  // std::pair

#include "absl/types/span.h"

struct ImGuiWindow;
struct ImDrawList;

namespace ImOverlayable {

struct Overlay {
    float data_width;
    float data_height;
    float display_width;
    float display_height;
    float corner_x;
    float corner_y;
    float scale;
    ImGuiWindow* window = nullptr;
    ImDrawList* window_draw_list = nullptr;

    void line(float x1,
              float y1,
              float x2,
              float y2,
              uint32_t color,
              float thickness);
    void circle(float x, float y, float r, uint32_t color, float thickness);
    void text(float x, float y, uint32_t color, const std::string_view text);
    void polyline(std::span<const float> polyline,
                  std::span<const uint32_t> colors,
                  float thickness);
    void line_list(std::span<const float> line_list,
                   std::span<const uint32_t> colors,
                   float thickness);

    std::pair<float, float> transform(float, float);
    std::pair<float, float> inv_transform(float, float);

    void _push_clip_rect();
    void _pop_clip_rect();
};

Overlay image(void* texture_id,
              float data_width,
              float data_height,
              float display_width = 0);
Overlay rectangle(float data_width, float data_height, float display_width = 0);

}  // namespace ImOverlayable
