#include <glad/gl.h>

#include "debug/check.h"
#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "shaders/lines_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
gl::Program _program;

void ensure_program_initted() {
    if (_program.id == 0) {
        _program =
            gl::Program(gl::ProgramSource(lines_vs, lines_fs), "lines_ids");
    }
}

}  // namespace

void draw_lines(const gl::FrameBufferInfo& frame_buffer,
                const gl::VertexArray& vertex_array,
                int num_lines,
                Seq<const float> mvp1,
                Seq<const float> mvp2) {
    ensure_program_initted();

    _program.set_mat4("mvp1", mvp1);
    _program.set_mat4("mvp2", mvp2);

    gl::ProgramDrawInfo draw_info;
    draw_info.vertex_array = vertex_array;
    draw_info.draw_mode = GL_LINES;
    draw_info.num_vertices_per_instance = 2;
    draw_info.num_items = num_lines;
    for (int location = 0; location <= 3; ++location) {
        draw_info.vertex_array.set_divisor(location, 1);
    }

    _program.draw(frame_buffer, draw_info);


    
}

}  // namespace shaders
}  // namespace axby
