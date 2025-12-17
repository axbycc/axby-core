#include <glad/gl.h>

#include "axgl/program.h"
#include "debug/check.h"
#include "debug/log.h"
#include "shaders/cmapped_texture_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
GLuint _dummy_vao = 0;
gl::Program _program;

void ensure_program_initted() {
    if (_program.id == 0) {
        _program = gl::Program(
            gl::ProgramSource(cmapped_texture_vs, cmapped_texture_fs),
            "cmapped_texture");

        _program.set_texture_unit("input_texture", 0);
        _program.set_texture_unit("cmap_texture", 1);

        // when we issue the draw call, some vertex array object needs
        // to be bound. we don't read any data from it, since the
        // vertex shader itself defines a quad for itself.
        glGenVertexArrays(1, &_dummy_vao);
    }
}

}  // namespace

void draw_cmapped_texture(const gl::FrameBufferInfo& frame_buffer,
                          GLuint texture_id,
                          GLuint cmap_texture_id,
                          float cmap_min,
                          float cmap_max,
                          float cmap_scale,
                          bool cmap_invert) {
    ensure_program_initted();

    CHECK(texture_id > 0);
    CHECK(cmap_texture_id > 0);

    _program.set_float("cmap_min", cmap_min);
    _program.set_float("cmap_max", cmap_max);
    _program.set_float("cmap_scale", cmap_scale);
    _program.set_float("cmap_invert", cmap_invert);

    // full screen mvp
    const float mvp[16] = {
        // clang-format off
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
        // clang-format on
    };
    _program.set_mat4("mvp", mvp);

    gl::ProgramDrawInfo draw_info;
    draw_info.draw_mode = GL_TRIANGLES;
    draw_info.num_items = 6;
    draw_info.vertex_array.id = _dummy_vao;
    draw_info.textures[0] = texture_id;
    draw_info.textures[1] = cmap_texture_id;

    _program.draw(frame_buffer, draw_info);
}

}  // namespace shaders
}  // namespace axby
