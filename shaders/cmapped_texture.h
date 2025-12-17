#pragma once

#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "axgl/vertex_array.h"
#include "colors/colors.h"
#include "seq/seq.h"

namespace axby {
namespace shaders {

void draw_cmapped_texture(const gl::FrameBufferInfo& frame_buffer,
                          GLuint texture_id,
                          GLuint cmap_texture_id,
                          float cmap_min,
                          float cmap_max,
                          float cmap_scale,
                          bool cmap_invert);

}  // namespace shaders
}  // namespace axby
