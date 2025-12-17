/*
  MIT License

  Copyright (c) 2024 Mark Liu

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "absl/types/span.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <utility> // std::pair

#include "imoverlayable.h"
#include "debug/log.h"

namespace ImOverlayable {

Overlay image(void* texture_id, float data_width, float data_height,
              float display_width) {
    if (display_width == 0) {
        display_width = data_width;
    }
    float scale = display_width / data_width;
    float display_height = scale * data_height;
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();
    ImGui::Image(texture_id, ImVec2(display_width, display_height));
    return Overlay{data_width,
                   data_height,
                   display_width,
                   display_height,
                   screen_pos.x,
                   screen_pos.y,
                   scale,
                   ImGui::GetCurrentWindowRead(),
                   ImGui::GetWindowDrawList()};
}

Overlay rectangle(float data_width, float data_height, float display_width) {
    if (display_width == 0) {
        display_width = data_width;
    }
    float scale = display_width / data_width;
    float display_height = scale * data_height;
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("", ImVec2(display_width, display_height));
    return Overlay{data_width,
                   data_height,
                   display_width,
                   display_height,
                   screen_pos.x,
                   screen_pos.y,
                   scale,
                   ImGui::GetCurrentWindowRead(),
                   ImGui::GetWindowDrawList()};
}

std::pair<float, float> Overlay::transform(float x, float y) {
    float sx = scale * x + corner_x;
    float sy = scale * y + corner_y;
    return {sx, sy};
}

std::pair<float, float> Overlay::inv_transform(float sx, float sy) {
    float x = (sx - corner_x) / scale;
    float y = (sy - corner_y) / scale;
    return {x, y};
}

void Overlay::_push_clip_rect() {
    window_draw_list->PushClipRect(window->Pos, window->Pos + window->Size);
}

void Overlay::_pop_clip_rect() { window_draw_list->PopClipRect(); }

void Overlay::line(float x1, float y1, float x2, float y2, uint32_t color,
                   float thickness) {
    auto [sx1, sy1] = transform(x1, y1);
    auto [sx2, sy2] = transform(x2, y2);

    _push_clip_rect();
    window_draw_list->AddLine(ImVec2(sx1, sy1), ImVec2(sx2, sy2), color,
                              thickness);
    _pop_clip_rect();
}

void Overlay::circle(float x, float y, float r, uint32_t color,
                     float thickness) {

    auto [sx, sy] = transform(x, y);
    float sr = std::max(
        scale * r, 1.0f); // anything under 1.0f radius will not be displayed

    _push_clip_rect();
    window_draw_list->AddCircle(ImVec2(sx, sy), std::max(sr, 1.0f), color, 12,
                                thickness);
    _pop_clip_rect();
}

void Overlay::text(float x, float y, uint32_t color,
                   const std::string_view text) {
    auto [sx, sy] = transform(x, y);
    _push_clip_rect();
    window_draw_list->AddText(ImVec2(sx, sy), color, text.data());
    _pop_clip_rect();
}

void Overlay::polyline(std::span<const float> polyline_data,
                       std::span<const uint32_t> colors, float thickness) {
    for (int i = 0; i < polyline_data.size(); i += 2) {
        int ni = (i + 2) % polyline_data.size();
        line(polyline_data[i], polyline_data[i + 1], polyline_data[ni],
             polyline_data[ni + 1], colors[(i / 2) % colors.size()], thickness);
    }
}

void Overlay::line_list(std::span<const float> line_list_data,
                        std::span<const uint32_t> colors, float thickness) {
    for (int i = 0; i < line_list_data.size() / 2; ++i) {
        int p = 2 * i;
        int np = 2 * i + 1;
        line(line_list_data[p], line_list_data[p + 1], line_list_data[np],
             line_list_data[np + 1], colors[i % colors.size()], thickness);
    }
}

} // namespace ImOverlayable
