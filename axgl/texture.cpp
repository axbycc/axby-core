#include "texture.h"

#include <glad/mx_shim.h>

#include "buffer.h"
#include "debug/check.h"
#include "info.h"
#include "seq/any_seq.h"

namespace axby {
namespace gl {

bool is_allowed_integer_texture_filter(GLenum filter) {
    if (filter == GL_NEAREST) return true;
    if (filter == GL_LINEAR_MIPMAP_LINEAR) return true;
    if (filter == GL_LINEAR_MIPMAP_NEAREST) return true;
    return false;
}

Texture::Texture(const TextureOptions& options, int width, int height) {
    CHECK(options.type != 0);

    // check configuration for integral textures which are very easy
    // to misconfigure
    if (is_integral_internal_format(options.internal_format)) {
        CHECK(is_allowed_integer_texture_filter(options.min_filter))
            << gl_internal_format_tostring(options.internal_format) << ": "
            << gl_texture_filter_tostring(options.min_filter);
        CHECK(is_allowed_integer_texture_filter(options.mag_filter))
            << gl_internal_format_tostring(options.internal_format) << ": "
            << gl_texture_filter_tostring(options.mag_filter);
        CHECK(is_integral_format(options.format))
            << gl_internal_format_tostring(options.format) << ": "
            << gl_format_tostring(options.format);
        CHECK(is_integral_type(options.type))
            << gl_internal_format_tostring(options.internal_format) << ": "
            << gl_datatype_tostring(options.type);
        // todo: this does not fully check the valid
        // configurations. could probably have ChatGPT write out a
        // fully fleshed out version
    }

    this->options = options;
    this->num_channels = gl_format_to_num_channels(options.format);

    if (id == 0) {
        glGenTextures(1, &id);
    }

    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, options.min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, options.mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, options.max_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, options.wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, options.wrap_t);

    ensure_size(width, height);
}

void Texture::ensure_size(int new_width, int new_height) {
    if (width != new_width || height != new_height) {
        width = new_width;
        height = new_height;

        // (re)allocate space for the texture
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D,
                     /*level=*/0,
                     /*internal_format=*/options.internal_format, width, height,
                     /*border=*/0,
                     /*format=*/options.format,
                     /*type=*/options.type,
                     /*data=*/nullptr);
    }
}

void Texture::upload_pbo(int new_width,
                         int new_height,
                         const gl::Buffer& pixel_buffer) {
    CHECK(id != 0) << "texture cannot be uploaded because was not initialized";
    CHECK(pixel_buffer.options.buffer_type == GL_PIXEL_UNPACK_BUFFER);
    CHECK(pixel_buffer.options.data_type == options.type);
    CHECK(pixel_buffer.length == new_width * new_height);

    ensure_size(new_width, new_height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_2D, id);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixel_buffer.id);
    glTexSubImage2D(GL_TEXTURE_2D,
                    /*level=*/0,
                    /*xoffset=*/0,
                    /*yoffset=*/0, width, height,
                    /*format=*/options.format,
                    /*type=*/options.type,
                    /*data=*/nullptr);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::download(axby::AnySeq data) const {
    CHECK(id != 0) << "texture cannot be uploaded because was not initialized";
    CHECK_EQ(data.logical_size(), width * height * num_channels);

    CHECK_EQ(typeid_to_glenum(data.get_typeid()), options.type);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, options.format, options.type,
                  data.as_bytes().data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::upload(int new_width, int new_height, axby::ConstAnySeq seq) {
    CHECK_EQ(typeid_to_glenum(seq.get_typeid()), options.type);
    CHECK_EQ(seq.logical_size(), new_width * new_height * num_channels);

    ensure_size(new_width, new_height);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D,
                    /*level=*/0,
                    /*xoffset=*/0,
                    /*yoffset=*/0, width, height,
                    /*format=*/options.format,
                    /*type=*/options.type,
                    /*data=*/seq.as_bytes().data());

    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace gl
}  // namespace axby
