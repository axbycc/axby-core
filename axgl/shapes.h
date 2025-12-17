#pragma once

#include "vertex_array.h"
#include "program.h"

namespace axby {
namespace gl {

const VertexArray& get_quad();
const ProgramDrawInfo& get_coordinate_frame();
const ProgramDrawInfo& get_cone();
const ProgramDrawInfo& get_square_cone();

}  // namespace gl
}  // namespace axby
