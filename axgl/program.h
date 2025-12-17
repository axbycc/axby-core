#pragma once

#include <glad/gl.h>

#include "frame_buffer.h"
#include "seq/seq.h"
#include "texture.h"
#include "vertex_array.h"

namespace axby {
namespace gl {

struct ProgramSource {
    const char* vshader = nullptr;
    const char* fshader = nullptr;
    const char* gshader = nullptr;
};

struct ProgramDrawInfo {
    VertexArray vertex_array;
    int num_items = 0;

    // non-zero triggers glDrawArraysInstanced
    int num_vertices_per_instance = 0; 

    GLenum draw_mode = GL_TRIANGLES;

    // texture_ids[i] should be set to the texture id that is mapped
    // to texture unit i. these textures are bound before the draw
    // call. entries that have id=0 are skipped over.
    std::array<GLuint, 4> textures = {};

    bool want_blend = false;

    ProgramDrawInfo() = default;
};

struct Program {
    Program() {};
    Program(ProgramSource src, const char* debug_name = "noname");

    int get_uniform_location(const char* name);

    // setting uniforms
    void set_texture_unit(const char* name, int unit);
    void set_float(const char* name, float);
    void set_int(const char* name, int);
    void set_uint(const char* name, uint32_t);
    void set_mat4(const char* name,
                  const axby::Seq<const float> values,
                  bool row_major = false);
    void set_vec4(const char* name, float x, float y, float z, float w);
    void set_vec3(const char* name, float x, float y, float z);
    void set_vec2(const char* name, float x, float y);
    void set_vec(const char* name, axby::Seq<const float> values);

    void draw(const FrameBufferInfo& frame_buffer,
              const ProgramDrawInfo& draw_info);

    GLuint id = 0;

    const char* debug_name = "noname";
};

}  // namespace gl
}  // namespace axby
