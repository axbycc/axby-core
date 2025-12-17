#pragma once

#include <glad/gl.h>

#include "info.h"
#include "seq/any_seq.h"

namespace axby {
namespace gl {

struct TextureOptions {
    int min_filter = GL_LINEAR;
    int mag_filter = GL_NEAREST;
    int max_level = 0;
    int wrap_s = GL_CLAMP_TO_BORDER;
    int wrap_t = GL_CLAMP_TO_BORDER;

    // https://stackoverflow.com/questions/34497195/difference-between-format-and-internalformat
    //
    // The internal format describes how the texture shall be stored
    // in the GPU. The format describes how the format of your pixel
    // data in client memory (together with the type parameter).  The
    // GL will convert your pixel data to the internal format.
    //
    // - user derhass

    // the 'format' specifies the R,G,B,A channel existence and
    // ordering the 'type' specifies the scalar type (uint, float) of
    // each channel that captures most cases, but are some packed
    // types that complicate the situation.
    int format = 0;
    GLenum type = 0;

    // gpu storage description
    int internal_format = 0;

    template <typename T>
    TextureOptions& set_data_type() {
        type = type_to_glenum<T>();
        return *this;
    }

    TextureOptions& set_rgba() {
        format = GL_RGBA;
        internal_format = GL_RGBA32F;
        return *this;
    }


    TextureOptions& set_rgb() {
        format = GL_RGB;
        internal_format = GL_RGB32F;
        return *this;
    }

    TextureOptions& set_rg() {
        format = GL_RG;
        internal_format = GL_RG32F;
        return *this;
    }

    TextureOptions& set_r() {
        format = GL_RED;
        internal_format = GL_R32F;
        return *this;
    }
};

struct Buffer;

struct Texture {
    TextureOptions options = {};
    int width = 0;
    int height = 0;
    uint8_t num_channels = 0;
    GLuint id = 0;

    Texture() = default;

    Texture(const TextureOptions& options, int width = 0, int height = 0);

    // write data from host to the OpenGL texture
    void upload(int width, int height, axby::ConstAnySeq data);

    // transfer data from pixel buffer to this texture
    // pixel_buffer.options.buffer_type must be GL_PIXEL_UNPACK_BUFFER
    void upload_pbo(int width, int height, const gl::Buffer& pixel_buffer);

    // copy data from OpenGL to host
    void download(axby::AnySeq) const;

    // postcondition: this->width and this->height match the input
    // width, height, and the texture storage is allocated (with
    // glTexImage2D).
    void ensure_size(int width, int height);
};

}  // namespace gl
}  // namespace axby
