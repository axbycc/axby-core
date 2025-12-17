#pragma once

#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "seq/seq.h"
#include "colors/colors.h"

namespace axby {
namespace shaders {

void draw_colored_vertices(const gl::FrameBufferInfo& frame_buffer,
                           const gl::ProgramDrawInfo& draw_info,
                           Seq<const float> mvp,
                           float point_size = 1.0f,
                           colors::RGBf tint_color = to_float(colors::red),
                           float tint_amount = 0.0f);


}  // namespace shaders
}  // namespace axby
