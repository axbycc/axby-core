#pragma once

#include "axgl/program.h"
#include "axgl/frame_buffer.h"
#include "seq/seq.h"

namespace axby {
namespace shaders {

void draw_vertex_ids(const gl::FrameBufferInfo& frame_buffer,
                     const gl::ProgramDrawInfo& draw_info,
                     float point_size,
                     Seq<const float> mvp,
                     uint32_t group_id);

}  // namespace shaders

}  // namespace axby
