#pragma once

#include "axgl/frame_buffer.h"
#include "axgl/vertex_array.h"
#include "seq/seq.h"

namespace axby {
namespace shaders {

void draw_lines(const gl::FrameBufferInfo& frame_buffer,
                const gl::VertexArray& vertex_array,
                int num_lines,
                Seq<const float> mvp1,
                Seq<const float> mvp2);

}  // namespace shaders
}  // namespace axby
