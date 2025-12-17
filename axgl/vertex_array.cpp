#include "vertex_array.h"

#include <glad/mx_shim.h>

#include "axgl/buffer.h"
#include "axgl/info.h"

namespace axby {
namespace gl {

void VertexArray::ensure_initted() {
    if (id == 0) {
        glGenVertexArrays(1, &id);
    }
}

void VertexArray::set_vertex_attribute_1d(int location,
                                          const Buffer& buffer,
                                          int logical_stride,
                                          int logical_offset) {
    set_vertex_attribute(1, location, buffer, logical_offset, logical_stride);
}

void VertexArray::set_vertex_attribute_2d(int location,
                                          const Buffer& buffer,
                                          int logical_stride,
                                          int logical_offset) {
    set_vertex_attribute(2, location, buffer, logical_offset, logical_stride);
}

void VertexArray::set_vertex_attribute_3d(int location,
                                          const Buffer& buffer,
                                          int logical_stride,
                                          int logical_offset) {
    set_vertex_attribute(3, location, buffer, logical_offset, logical_stride);
}

void VertexArray::set_vertex_attribute_4d(int location,
                                          const Buffer& buffer,
                                          int logical_stride,
                                          int logical_offset) {
    set_vertex_attribute(4, location, buffer, logical_offset, logical_stride);
}

void VertexArray::set_vertex_attribute(int dimension,
                                       int location,
                                       const Buffer& buffer,
                                       int logical_offset,
                                       int logical_stride) {
    ensure_initted();

    // bind attributes to vertex buffers
    CHECK_EQ(buffer.options.buffer_type, GL_ARRAY_BUFFER)
        << gl_buffertarget_tostring(buffer.options.buffer_type);

    /* Quote from https://learnopengl.com/Getting-started/Hello-Triangle :
         *
         * Each vertex attribute takes its data from memory managed by a VBO
         * and which VBO it takes its data from (you can have multiple VBOs)
         * is determined by the VBO currently bound to GL_ARRAY_BUFFER when
         * calling glVertexAttribPointer.
         *
    */

    // so first we bind the buffer, and then we bind the vertex
    // attribute pointer
    CHECK(buffer.id != 0);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.id);

    CHECK(id != 0);
    glBindVertexArray(id);

    const int byte_stride =
        logical_stride * gl_sizeof(buffer.options.data_type);
    const uint64_t byte_offset =
        logical_offset * gl_sizeof(buffer.options.data_type);

    if (is_integral_type(buffer.options.data_type) &&
        !buffer.options.normalized) {
        // user has requested an un-normalized integral attribute
        // we assume they want to use it inside a vertex shader as
        // an a "pure integer type." This type of attribute is useful
        // for implementing "which object was clicked" by rendering
        // the attribute to a corresponding integer texture
        // See:
        // https://stackoverflow.com/questions/57854484/passing-unsigned-int-input-attribute-to-vertex-shader
        // https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf#page=369&zoom=100,0,609
        // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
        glVertexAttribIPointer(location, dimension, buffer.options.data_type,
                               /*stride=*/byte_stride,
                               /*offset=*/(void*)byte_offset);
    } else {
        glVertexAttribPointer(location, dimension, buffer.options.data_type,
                              buffer.options.normalized, /*stride=*/byte_stride,
                              /*offset=*/(void*)byte_offset);
    }
    glEnableVertexAttribArray(location);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void VertexArray::set_element_array(const Buffer& buffer) {
    ensure_initted();
    glBindVertexArray(id);

    /* Quote from https://learnopengl.com/Getting-started/Hello-Triangle :
     * The last element buffer object that gets bound while a VAO is bound,
     * is stored as the VAO's element buffer object. Binding to a VAO then also
     * automatically binds that EBO.
     */
    CHECK(buffer.id != 0);
    CHECK(buffer.options.buffer_type == GL_ELEMENT_ARRAY_BUFFER);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.id);

    glBindVertexArray(0);

    ebo_data_type = buffer.options.data_type;
}

void VertexArray::enable_attrib(int location) {
    ensure_initted();
    glBindVertexArray(id);
    glDisableVertexAttribArray(location);
    glBindVertexArray(0);
}
void VertexArray::disable_attrib(int location) {
    ensure_initted();
    glBindVertexArray(id);
    glDisableVertexAttribArray(location);
    glBindVertexArray(0);
}

void VertexArray::set_divisor(int location, int divisor) {
    ensure_initted();
    glBindVertexArray(id);
    glVertexAttribDivisor(location, divisor);
    glBindVertexArray(0);
}

void VertexArray::set_default_float(int location, float f) {
    ensure_initted();
    glBindVertexArray(id);
    glVertexAttrib1f(location, f);
    glBindVertexArray(0);
}

void VertexArray::set_default_float3(int location,
                                     axby::Seq<const float> vals) {
    CHECK_GE(vals.size(), 3);
    ensure_initted();
    glBindVertexArray(id);
    glVertexAttrib3f(location, vals[0], vals[1], vals[2]);
    glBindVertexArray(0);
}

}  // namespace gl
}  // namespace axby
