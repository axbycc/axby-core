#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "shaders/vertex_world_xyzs_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
gl::Program _program;

void ensure_program_initted() {
    if (_program.id == 0) {
        _program = gl::Program(
            gl::ProgramSource(vertex_world_xyzs_vs, vertex_world_xyzs_fs),
            "vertex_world_xyzs");
    }
}

}  // namespace

void draw_vertex_world_xyzs(const gl::FrameBufferInfo& frame_buffer,
                            const gl::ProgramDrawInfo& draw_info,
                            float point_size,
                            Seq<const float> mvp,
                            Seq<const float> tx_world_object) {
    ensure_program_initted();
    _program.set_mat4("mvp", mvp);
    _program.set_mat4("tx_world_object", tx_world_object);
    _program.set_float("point_size", point_size);
    _program.draw(frame_buffer, draw_info);
}

}  // namespace shaders
}  // namespace axby
