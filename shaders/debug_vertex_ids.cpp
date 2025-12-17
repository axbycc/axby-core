#include <glad/gl.h>

#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "shaders/debug_vertex_ids_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
GLuint _dummy_vao = 0;
gl::Program _program;

void ensure_program_initted() {
    if (_program.id == 0) {
        _program = gl::Program(
            gl::ProgramSource(debug_vertex_ids_vs, debug_vertex_ids_fs),
            "debug_vertex_ids");
        _program.set_texture_unit("vertex_ids_texture", 0);

        // when we issue the draw call, some vertex array object needs
        // to be bound. we don't read any data from it, since the
        // vertex shader itself defines a quad for itself.
        glGenVertexArrays(1, &_dummy_vao);
    }
}
}  // namespace

void draw_debug_vertex_ids(const gl::FrameBufferInfo& frame_buffer,
                           GLuint vertex_ids_texture) {
    ensure_program_initted();

    gl::ProgramDrawInfo draw_info;
    draw_info.draw_mode = GL_TRIANGLES;
    draw_info.num_items = 6;
    draw_info.vertex_array.id = _dummy_vao;
    draw_info.textures[0] = vertex_ids_texture;

    _program.draw(frame_buffer, draw_info);
}

}  // namespace shaders
}  // namespace axby
