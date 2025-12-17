#include "buffer.h"

#include <glad/mx_shim.h>

#include "axgl/info.h"
#include "seq/any_seq.h"
#include "seq/seq.h"

namespace axby {
namespace gl {

void Buffer::ensure_initted() {
    if (id == 0) {
        glGenBuffers(1, &id);
    }
}

void Buffer::download(axby::AnySeq data) {
    CHECK(id != 0) << "buffer not initted";
    CHECK_EQ(data.logical_size(), length);
    CHECK_EQ(typeid_to_glenum(data.get_typeid()), options.data_type);
    CHECK_EQ(length * gl_sizeof(options.data_type), num_bytes_uploaded)
        << "data_type out of sync, maybe type changed after upload";
    glBindBuffer(options.buffer_type, id);
    glGetBufferSubData(options.buffer_type, 0, data.num_bytes(),
                       (void*)data.as_bytes().data());
}

void Buffer::resize(size_t new_length) {
    if (new_length != length) {
        glBindBuffer(options.buffer_type, id);
        glBufferData(options.buffer_type,
                     new_length * gl_sizeof(options.data_type), 0,
                     options.dynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW);
        glBindBuffer(options.buffer_type, 0);
        length = new_length;
    }
}

void Buffer::upload_bytes(axby::Seq<const std::byte> bytes) {
    CHECK(options.data_type != 0) << "buffer options data type not initialized";
    ensure_initted();
    glBindBuffer(options.buffer_type, id);
    glBufferData(options.buffer_type, bytes.size(), (void*)bytes.data(),
                 options.dynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW);
    glBindBuffer(options.buffer_type, 0);

    CHECK_EQ(bytes.size() % gl::gl_sizeof(options.data_type), 0);
    length = bytes.size() / gl::gl_sizeof(options.data_type);
    num_bytes_uploaded = bytes.size();
}

void Buffer::upload(axby::ConstAnySeq data) {
    CHECK(options.data_type != 0) << "buffer options data type not initialized";
    ensure_initted();
    CHECK_EQ(typeid_to_glenum(data.get_typeid()), options.data_type);

    glBindBuffer(options.buffer_type, id);

    GLenum usage = options.dynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW;
    glBufferData(options.buffer_type, data.num_bytes(),
                 (void*)data.as_bytes().data(), usage);
    glBindBuffer(options.buffer_type, 0);

    length = data.logical_size();
    num_bytes_uploaded = data.num_bytes();
}

}  // namespace gl
}  // namespace axby
