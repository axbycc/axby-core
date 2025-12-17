#include <glad/gl.h>

#include "axgl/frame_buffer.h"

namespace axby {
namespace shaders {

void draw_debug_vertex_ids(const gl::FrameBufferInfo& frame_buffer,
                           GLuint vertex_ids_texture);

}  // namespace shaders
}  // namespace axby
