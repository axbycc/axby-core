#include "debug/check.h"
#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "seq/seq.h"
#include "shaders/colored_vertex_ids_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
gl::Program _program;

void ensure_program_initted() {
    if (_program.id == 0) {
        _program = gl::Program(
            gl::ProgramSource(colored_vertex_ids_vs, colored_vertex_ids_fs),
            "colored_vertex_ids");
    }
}

}  // namespace

void draw_colored_vertex_ids(const gl::FrameBufferInfo& frame_buffer,
                                    const gl::ProgramDrawInfo& draw_info,
                                    Seq<const float> mvp,
                                    uint32_t group_id) {
    ensure_program_initted();
    _program.set_mat4("mvp", mvp);
    _program.set_uint("group_id", group_id);
    _program.draw(frame_buffer, draw_info);
}

}  // namespace shaders
}  // namespace axby
