#include "info.h"

#include "debug/check.h"
#include "debug/log.h"

namespace axby {
namespace gl {

uint8_t gl_format_to_num_channels(GLenum format) {
    switch (format) {
        case GL_DEPTH_COMPONENT:
            return 1;
        case GL_RGBA:
            return 4;
        case GL_RGB:
            return 3;
        case GL_RG:
            return 2;
        case GL_RED:
            return 1;
        case GL_RGBA_INTEGER:
            return 4;
        case GL_RGB_INTEGER:
            return 3;
        case GL_RG_INTEGER:
            return 2;
        case GL_RED_INTEGER:
            return 1;
    }

    LOG(FATAL) << "Unknown num channels of " << gl_format_tostring(format)
               << ", please add it to gl_format_to_num_channels()";
    return 0;
}

bool is_integral_format(GLenum format) {
    switch (format) {
        case GL_RED_INTEGER:
            return true;
        case GL_RG_INTEGER:
            return true;
        case GL_RGB_INTEGER:
            return true;
        case GL_RGBA_INTEGER:
            return true;
        case GL_RED:
            return false;
        case GL_RG:
            return false;
        case GL_RGB:
            return false;
        case GL_RGBA:
            return false;
    }

    LOG(FATAL) << "Uknown if " << gl_format_tostring(format)
               << " is integral,"
                  " please add it to gl_is_integral_format";

    return false;
}

bool is_integral_internal_format(GLenum internal_format) {
    switch (internal_format) {
        case GL_R8I: case GL_R8UI:
        case GL_R16I: case GL_R16UI:
        case GL_R32I: case GL_R32UI:
        case GL_RG8I: case GL_RG8UI:
        case GL_RG16I: case GL_RG16UI:
        case GL_RG32I: case GL_RG32UI:
        case GL_RGBA8I: case GL_RGBA8UI:
        case GL_RGBA16I: case GL_RGBA16UI:
        case GL_RGBA32I: case GL_RGBA32UI:
            return true;
        default:
            return false;
    }
}

const char* gl_texture_filter_tostring(GLenum filter)
{
    switch (filter)
    {
        case GL_NEAREST:                 return "GL_NEAREST";
        case GL_LINEAR:                  return "GL_LINEAR";

        case GL_NEAREST_MIPMAP_NEAREST:  return "GL_NEAREST_MIPMAP_NEAREST";
        case GL_LINEAR_MIPMAP_NEAREST:   return "GL_LINEAR_MIPMAP_NEAREST";
        case GL_NEAREST_MIPMAP_LINEAR:   return "GL_NEAREST_MIPMAP_LINEAR";
        case GL_LINEAR_MIPMAP_LINEAR:    return "GL_LINEAR_MIPMAP_LINEAR";

        default:
            return "UNKNOWN_GL_TEXTURE_FILTER";
    }
}


const char* gl_internal_format_tostring(GLenum fmt) {
    switch (fmt)
    {
        // --- Red channel ---
        case GL_R8:             return "GL_R8";
        case GL_R8_SNORM:       return "GL_R8_SNORM";
        case GL_R16:            return "GL_R16";
        case GL_R16_SNORM:      return "GL_R16_SNORM";
        case GL_R16F:           return "GL_R16F";
        case GL_R32F:           return "GL_R32F";
        case GL_R8I:            return "GL_R8I";
        case GL_R8UI:           return "GL_R8UI";
        case GL_R16I:           return "GL_R16I";
        case GL_R16UI:          return "GL_R16UI";
        case GL_R32I:           return "GL_R32I";
        case GL_R32UI:          return "GL_R32UI";

        // --- RG ---
        case GL_RG8:            return "GL_RG8";
        case GL_RG8_SNORM:      return "GL_RG8_SNORM";
        case GL_RG16:           return "GL_RG16";
        case GL_RG16_SNORM:     return "GL_RG16_SNORM";
        case GL_RG16F:          return "GL_RG16F";
        case GL_RG32F:          return "GL_RG32F";
        case GL_RG8I:           return "GL_RG8I";
        case GL_RG8UI:          return "GL_RG8UI";
        case GL_RG16I:          return "GL_RG16I";
        case GL_RG16UI:         return "GL_RG16UI";
        case GL_RG32I:          return "GL_RG32I";
        case GL_RG32UI:         return "GL_RG32UI";

        // --- RGB ---
        case GL_RGB8:           return "GL_RGB8";
        case GL_RGB8_SNORM:     return "GL_RGB8_SNORM";
        case GL_RGB16:          return "GL_RGB16";
        case GL_RGB16_SNORM:    return "GL_RGB16_SNORM";
        case GL_RGB16F:         return "GL_RGB16F";
        case GL_RGB32F:         return "GL_RGB32F";
        case GL_RGB8I:          return "GL_RGB8I";
        case GL_RGB8UI:         return "GL_RGB8UI";
        case GL_RGB16I:         return "GL_RGB16I";
        case GL_RGB16UI:        return "GL_RGB16UI";
        case GL_RGB32I:         return "GL_RGB32I";
        case GL_RGB32UI:        return "GL_RGB32UI";

        // --- RGBA ---
        case GL_RGBA8:          return "GL_RGBA8";
        case GL_RGBA8_SNORM:    return "GL_RGBA8_SNORM";
        case GL_RGBA16:         return "GL_RGBA16";
        case GL_RGBA16_SNORM:   return "GL_RGBA16_SNORM";
        case GL_RGBA16F:        return "GL_RGBA16F";
        case GL_RGBA32F:        return "GL_RGBA32F";
        case GL_RGBA8I:         return "GL_RGBA8I";
        case GL_RGBA8UI:        return "GL_RGBA8UI";
        case GL_RGBA16I:        return "GL_RGBA16I";
        case GL_RGBA16UI:       return "GL_RGBA16UI";
        case GL_RGBA32I:        return "GL_RGBA32I";
        case GL_RGBA32UI:       return "GL_RGBA32UI";

        // --- Depth / Stencil ---
        case GL_DEPTH_COMPONENT16:  return "GL_DEPTH_COMPONENT16";
        case GL_DEPTH_COMPONENT24:  return "GL_DEPTH_COMPONENT24";
        case GL_DEPTH_COMPONENT32:  return "GL_DEPTH_COMPONENT32";
        case GL_DEPTH_COMPONENT32F: return "GL_DEPTH_COMPONENT32F";
        case GL_DEPTH24_STENCIL8:   return "GL_DEPTH24_STENCIL8";
        case GL_DEPTH32F_STENCIL8:  return "GL_DEPTH32F_STENCIL8";

        default:
            return "UNKNOWN_GL_INTERNAL_FORMAT";
    }
}

const char* gl_format_tostring(GLenum format) {
    switch (format) {
        case GL_RGBA:
            return "GL_RGBA";
        case GL_RGBA16:
            return "GL_RGBA16";
        case GL_RGBA16F:
            return "GL_RGBA16F";
        case GL_RGB16F:
            return "GL_RGB16F";
        case GL_RGBA32F:
            return "GL_RGBA32F";
        case GL_RGB:
            return "GL_RGB";
        case GL_ALPHA:
            return "GL_ALPHA";
        case GL_DEPTH_COMPONENT:
            return "GL_DEPTH_COMPONENT";
        case GL_DEPTH_COMPONENT16:
            return "GL_DEPTH_COMPONENT16";
        case GL_DEPTH_COMPONENT24:
            return "GL_DEPTH_COMPONENT24";
        case GL_DEPTH_COMPONENT32:
            return "GL_DEPTH_COMPONENT32";
        case GL_DEPTH_COMPONENT32F:
            return "GL_DEPTH_COMPONENT32F";
        case GL_STENCIL_INDEX:
            return "GL_STENCIL_INDEX";
        case GL_RED:
            return "GL_RED";
        case GL_RG:
            return "GL_RG";
        case GL_RED_INTEGER:
            return "GL_RED_INTEGER";
        case GL_RG_INTEGER:
            return "GL_RG_INTEGER";
        case GL_RGB_INTEGER:
            return "GL_RGB_INTEGER";
        case GL_RGBA_INTEGER:
            return "GL_RGBA_INTEGER";
        case GL_DEPTH_STENCIL:
            return "GL_DEPTH_STENCIL";
        case GL_BGR:
            return "GL_BGR";
        case GL_BGRA:
            return "GL_BGRA";
        case GL_SRGB8_ALPHA8:
            return "GL_SRGB8_ALPHA8";
        case GL_SRGB:
            return "GL_SRGB";
        case GL_SRGB8:
            return "GL_SRGB8";
        case GL_COMPRESSED_RGB:
            return "GL_COMPRESSED_RGB";
        case GL_COMPRESSED_RGBA:
            return "GL_COMPRESSED_RGBA";
    }

    // Add more cases for other formats as needed
    LOG(FATAL) << "Unknown Format " << format
               << ", please add it to gl_format_tostring()";
    return "";
}

int gl_sizeof(GLenum type) {
    if (type == GL_BYTE) return 1;
    if (type == GL_UNSIGNED_BYTE) return 1;
    if (type == GL_SHORT) return 2;
    if (type == GL_UNSIGNED_SHORT) return 2;
    if (type == GL_INT) return 4;
    if (type == GL_UNSIGNED_INT) return 4;
    if (type == GL_FLOAT) return 4;
    if (type == GL_DOUBLE) return 8;

    LOG(FATAL) << "Unknown size of " << gl_datatype_tostring(type)
               << ", please add it to gl_sizeof() if it is a valid OpenGl type";
    return 0;
    // std::unreachable(); // unreachable not supported by gcc
}

bool is_integral_type(GLenum type) {
    if (type == GL_BYTE) return true;
    if (type == GL_UNSIGNED_BYTE) return true;
    if (type == GL_SHORT) return true;
    if (type == GL_UNSIGNED_SHORT) return true;
    if (type == GL_INT) return true;
    if (type == GL_UNSIGNED_INT) return true;
    if (type == GL_FLOAT) return false;
    if (type == GL_DOUBLE) return false;

    LOG(FATAL) << "Unknown type " << gl_datatype_tostring(type)
               << ", please add it to is_integral_type() if it is a "
                  "valid OpenGl type";

    return false;
    // std::unreachable(); // unreachable not supported by gcc
}

const char* gl_datatype_tostring(GLenum data_type) {
    switch (data_type) {
        case 0:
            return "0";
        case GL_FLOAT:
            return "GL_FLOAT";
        case GL_INT:
            return "GL_INT";
        case GL_UNSIGNED_INT:
            return "GL_UNSIGNED_INT";
        case GL_BYTE:
            return "GL_BYTE";
        case GL_UNSIGNED_BYTE:
            return "GL_UNSIGNED_BYTE";
        case GL_SHORT:
            return "GL_SHORT";
        case GL_UNSIGNED_SHORT:
            return "GL_UNSIGNED_SHORT";
        case GL_DOUBLE:
            return "GL_DOUBLE";
        case GL_FIXED:
            return "GL_FIXED";
            // Add more cases as needed for other GLenum values
    }

    LOG(FATAL) << "Unknown type (" << data_type
               << "), please add it to gl_datatype_tostring()";
    return "";
}

const char* gl_buffertarget_tostring(GLenum target) {
    switch (target) {
        case GL_ARRAY_BUFFER:
            return "GL_ARRAY_BUFFER";
        case GL_ELEMENT_ARRAY_BUFFER:
            return "GL_ELEMENT_ARRAY_BUFFER";
        case GL_PIXEL_PACK_BUFFER:
            return "GL_PIXEL_PACK_BUFFER";
        case GL_PIXEL_UNPACK_BUFFER:
            return "GL_PIXEL_UNPACK_BUFFER";
        case GL_UNIFORM_BUFFER:
            return "GL_UNIFORM_BUFFER";
    }

    LOG(FATAL) << "Unknown buffer target, please add it to "
                  "gl_buffertarget_tostring()";
    return "";
}

GLenum typeid_to_glenum(std::type_index type_id) {
    if (type_id == std::type_index(typeid(float))) {
        return GL_FLOAT;
    } else if (type_id == std::type_index(typeid(double))) {
        return GL_DOUBLE;
    } else if (type_id == std::type_index(typeid(int8_t))) {
        return GL_BYTE;
    } else if (type_id == std::type_index(typeid(uint8_t))) {
        return GL_UNSIGNED_BYTE;
    } else if (type_id == std::type_index(typeid(int16_t))) {
        return GL_SHORT;
    } else if (type_id == std::type_index(typeid(uint16_t))) {
        return GL_UNSIGNED_SHORT;
    } else if (type_id == std::type_index(typeid(int32_t))) {
        return GL_INT;
    } else if (type_id == std::type_index(typeid(uint32_t))) {
        return GL_UNSIGNED_INT;
    } else if (type_id == std::type_index(typeid(std::byte))) {
        return GL_BYTE;
    }
    LOG(FATAL) << "Unknown type " << type_id.name()
               << ", please add it to typeid_to_glenum()";
    return 0;
}

}  // namespace gl
}  // namespace axby
