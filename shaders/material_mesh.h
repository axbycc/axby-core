#pragma once

#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "viewer/mesh.h"

namespace axby {
namespace shaders {

void draw_material_mesh(const gl::FrameBufferInfo&,
                        Seq<const float> ndc_image_camera,
                        Seq<const float> tx_camera_object,
                        int num_items,
                        const gl::VertexArray& vertex_array,
                        const MeshMaterial& material);

}  // namespace shaders
}  // namespace axby
