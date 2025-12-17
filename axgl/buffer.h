#pragma once

#include <glad/gl.h>

#include "info.h"
#include "seq/any_seq.h"
#include "seq/seq.h"

namespace axby {
namespace gl {

struct BufferOptions {
    GLenum buffer_type =
        GL_ARRAY_BUFFER;  // GL_ARRAY_BUFFER or GL_ELEMENT_BUFFER or GL_PIXEL_UNPACK_BUFFER

    // In OpenGL, the scalar data type (float, int, etc) is not
    // actually a property of the buffer. OpenGL treats buffers as
    // binary blobs, and does not record what those bytes represent.
    //
    // Interpretation of those bytes is deferred to call sites which
    // use the buffer.
    //
    //   - glVertexAttribPointer specifies GL_FLOAT, GL_INT, etc.
    //   - glDrawElements specifies GL_UNSIGNED_BYTE/SHORT/INT.
    //   - TexBuffer operations specify an internal format such as GL_R32F.
    //
    // In practice, is is often useful to know that the buffer is
    // meant to house a certain type, like floats. This enables:
    //
    //    - runtime sanity checks
    //    - automatic selection of the correct GLenum in API calls
    //    - automatic calculation of byte strides and offsets given logical strides and offsets
    //
    // So we use the field below to associate a scalar data type to
    // each buffer we create.
    GLenum data_type = 0;  // eg GL_INT, GL_FLOAT, etc

    // The 'normalized' field is only relevant for integral
    // 'data_type'. When true, then the integral values of the buffer
    // should logically be considered as values between [0,1] for
    // unsigned and [-1,1] for signed types during binding to
    // VertexArray. In the majority of cases, integer data is used to
    // represent colors, eg RGB channels, so we make normalized=true
    // by default. An example exotic use case is assigning integral
    // "id" to vertices. In that case, the buffer must be marked as
    // normalized=false.
    bool normalized = true;

    bool dynamic = false;  // controls GL_STREAM_DRAW vs GL_STATIC_DRAW

    template <typename T>
    BufferOptions& set_data_type() {
        data_type = type_to_glenum<T>();
        return *this;
    }

    BufferOptions& set_array_buffer() {
        buffer_type = GL_ARRAY_BUFFER;
        return *this;
    }

    BufferOptions& set_element_array_buffer() {
        buffer_type = GL_ELEMENT_ARRAY_BUFFER;
        return *this;
    }

    BufferOptions& set_pixel_unpack_buffer() {
        buffer_type = GL_PIXEL_UNPACK_BUFFER;
        return *this;
    }

    BufferOptions& set_stream_draw() {
        dynamic = true;
        return *this;
    }

    BufferOptions& set_unnormalized() {
        normalized = false;
        return *this;
    }

    BufferOptions& set_static_draw() {
        dynamic = false;
        return *this;
    }
};

inline std::ostream& operator<<(std::ostream& os,
                                const BufferOptions& options) {
    os << "Buffer Type: " << gl_buffertarget_tostring(options.buffer_type)
       << ", Data Type: " << gl_datatype_tostring(options.data_type) << ", "
       << (options.dynamic ? "Dynamic" : "Static");
    return os;
}

struct Buffer {
    GLuint id = 0;
    BufferOptions options = {};

    // number of logical elements that have been uploaded
    size_t length = 0;

    // actual number of bytes that were uploaded
    size_t num_bytes_uploaded = 0;

    Buffer() {}
    Buffer(const BufferOptions& o) : options{o} {}

    void upload(axby::ConstAnySeq data);                  // type checked
    void upload_bytes(axby::Seq<const std::byte> bytes);  // unchecked
    void download(axby::AnySeq data);
    void resize(size_t length);

    void ensure_initted();
};

}  // namespace gl
}  // namespace axby
