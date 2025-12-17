#include "colored_vertices.h"

#include "axgl/program.h"
#include "axgl/vertex_array.h"
#include "colored_vertices.h"
#include "shaders/colored_vertices_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
gl::Program _program;
void ensure_program_initted() {
    if (_program.id == 0) {
        _program = gl::Program(
            gl::ProgramSource(colored_vertices_vs, colored_vertices_fs),
            "colored_vertices");
    }
}
}  // namespace

void draw_colored_vertices(const gl::FrameBufferInfo& frame_buffer,
                           const gl::ProgramDrawInfo& draw_info,
                           Seq<const float> mvp,
                           float point_size,
                           colors::RGBf tint_color,
                           float tint_amount) {
    ensure_program_initted();
    _program.set_float("point_size", point_size);
    _program.set_mat4("mvp", mvp);
    _program.set_vec3("tint_color", tint_color.red, tint_color.blue,
                      tint_color.green);
    _program.set_float("tint_amount", tint_amount);

    // copy out vertex array handle so we can set default attributes.
    // without the copy, we hit const issues
    gl::VertexArray vertex_array = draw_info.vertex_array;
    vertex_array.set_default_float3(1, {{1.0f, 0.0f, 0.0f}});
    vertex_array.set_default_float(2, 1.0f);

    _program.draw(frame_buffer, draw_info);
}

}  // namespace shaders
}  // namespace axby
