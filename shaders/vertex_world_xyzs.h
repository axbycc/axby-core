#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "seq/seq.h"

namespace axby {
namespace shaders {

void draw_vertex_world_xyzs(const gl::FrameBufferInfo& frame_buffer,
                            const gl::ProgramDrawInfo& draw_info,
                            float point_size,
                            Seq<const float> mvp,
                            Seq<const float> tx_world_object);

}  // namespace shaders
}  // namespace axby
