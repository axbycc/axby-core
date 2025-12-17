#include "axgl/program.h"
#include "axgl/frame_buffer.h"
#include "shaders/vertex_ids_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
gl::Program _program;

void ensure_program_initted() {
    if (_program.id == 0) {
        _program = gl::Program(gl::ProgramSource(vertex_ids_vs, vertex_ids_fs),
                               "vertex_ids");
    }
}

}  // namespace

void draw_vertex_ids(const gl::FrameBufferInfo& frame_buffer,
                     const gl::ProgramDrawInfo& draw_info,
                     float point_size,
                     Seq<const float> mvp,
                     uint32_t group_id) {
    ensure_program_initted();
    _program.set_mat4("mvp", mvp);
    _program.set_uint("group_id", group_id);
    _program.set_float("point_size", point_size);
    _program.draw(frame_buffer, draw_info);
}

}  // namespace shaders
}  // namespace axby
