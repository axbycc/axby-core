#pragma once

#include "axgl/frame_buffer.h"
#include "seq/seq.h"

namespace axby {
namespace shaders {

void draw_texture(const gl::FrameBufferInfo& frame_buffer,
                  GLuint texture_id,
                  Seq<const float> mvp);

}  // namespace shaders
}  // namespace axby
