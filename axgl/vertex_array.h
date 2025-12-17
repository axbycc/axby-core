#pragma once

#include "buffer.h"
#include "seq/seq.h"

namespace axby {
namespace gl {

struct VertexArray {
    VertexArray() {};

    void enable_attrib(int location);
    void disable_attrib(int location);

    void set_vertex_attribute(int dimension,
                              int location,
                              const Buffer& buffer,
                              int logical_stride = 0,
                              int logical_offset = 0);
    void set_vertex_attribute_1d(int location,
                                 const Buffer& buffer,
                                 int logical_stride = 0,
                                 int logical_offset = 0);
    void set_vertex_attribute_2d(int location,
                                 const Buffer& buffer,
                                 int logical_stride = 0,
                                 int logical_offset = 0);
    void set_vertex_attribute_3d(int location,
                                 const Buffer& buffer,
                                 int logical_stride = 0,
                                 int logical_offset = 0);
    void set_vertex_attribute_4d(int location,
                                 const Buffer& buffer,
                                 int logical_stride = 0,
                                 int logical_offset = 0);

    // how many instances this vertex lasts over. used for
    // glDrawArraysInstanced
    void set_divisor(int location, int divisor); 

    void set_default_float(int location, float f);

    // passed in vals must have >= 3 elements. only first 3 elements
    // are used.
    void set_default_float3(int location, axby::Seq<const float> vals);

    void set_element_array(const Buffer& buffer);

    void ensure_initted();

    GLuint id = 0;

    // if set, ebo_data_type is used for glDrawElements
    GLenum ebo_data_type = 0; 
};

}  // namespace gl
}  // namespace axby
