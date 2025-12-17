#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "seq/seq.h"

namespace axby {
namespace shaders {

void draw_colored_vertex_ids(const gl::FrameBufferInfo& frame_buffer,
                             const gl::ProgramDrawInfo& draw_info,
                             Seq<const float> mvp,
                             uint32_t group_id);

}  // namespace shaders
}  // namespace axby
