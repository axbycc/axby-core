#include "material_mesh.h"

#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "debug/check.h"
#include "shaders/material_mesh_glsl.h"

namespace axby {
namespace shaders {

namespace {
// internal linkage
gl::Program _program;

void ensure_program_initted() {
    if (_program.id == 0) {
        _program =
            gl::Program(gl::ProgramSource(material_mesh_vs, material_mesh_fs),
                        "material_mesh");
    }
}

}  // namespace

void draw_material_mesh(const gl::FrameBufferInfo& frame_buffer,
                        Seq<const float> ndc_image_camera,
                        Seq<const float> tx_camera_object,
                        int num_items,
                        const gl::VertexArray& vertex_array,
                        const MeshMaterial& material) {
    ensure_program_initted();

    gl::ProgramDrawInfo draw_info;
    draw_info.num_items = num_items;
    draw_info.vertex_array = vertex_array;
    draw_info.draw_mode = GL_TRIANGLES;

    _program.set_mat4("ndc_image_camera", ndc_image_camera);
    _program.set_mat4("tx_camera_object", tx_camera_object);
    _program.set_vec("material.diffuse", material.diffuse);
    _program.set_vec("material.specular", material.specular);
    _program.set_vec("material.ambient", material.ambient);
    _program.set_vec("material.emissive", material.emissive);
    _program.set_float("material.opacity", material.opacity);
    _program.set_float("material.specular_exponent",
                       material.specular_exponent);

    _program.draw(frame_buffer, draw_info);
}

}  // namespace shaders
}  // namespace axby
