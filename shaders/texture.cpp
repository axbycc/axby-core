#include "axgl/texture.h"

#include <glad/gl.h>

#include "axgl/program.h"
#include "shaders/texture_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
GLuint _dummy_vao = 0;
gl::Program _program;

void ensure_program_initted() {
    if (_program.id == 0) {
        _program = gl::Program(gl::ProgramSource(texture_vs, texture_fs),
                               "texture");
        _program.set_texture_unit("input_texture", 0);

        // when we issue the draw call, some vertex array object needs
        // to be bound. we don't read any data from it, since the
        // vertex shader itself defines a quad for itself.
        glGenVertexArrays(1, &_dummy_vao);
    }
}
}  // namespace

void draw_texture(const gl::FrameBufferInfo& frame_buffer,
                  GLuint texture_id,
                  Seq<const float> mvp) {
    ensure_program_initted();

    _program.set_mat4("mvp", mvp);

    gl::ProgramDrawInfo draw_info;
    draw_info.draw_mode = GL_TRIANGLES;
    draw_info.num_items = 6;
    draw_info.vertex_array.id = _dummy_vao;
    draw_info.textures[0] = texture_id;

    _program.draw(frame_buffer, draw_info);
}

}  // namespace shaders
}  // namespace axby
