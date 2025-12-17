#pragma once

#include <glad/gl.h>

#include <type_traits>
#include <typeindex>

namespace axby {
namespace gl {

GLenum typeid_to_glenum(std::type_index t);

template <typename T>
GLenum type_to_glenum() {
    return typeid_to_glenum(typeid(std::remove_cv_t<T>));
}

bool is_integral_format(GLenum format);
bool is_integral_internal_format(GLenum format);
bool is_integral_type(GLenum type);
const char* gl_texture_filter_tostring(GLenum filter);
const char* gl_buffertarget_tostring(GLenum buffer_type);
const char* gl_datatype_tostring(GLenum data_type);
const char* gl_format_tostring(GLenum format);
const char* gl_internal_format_tostring(GLenum fmt);
int gl_sizeof(GLenum type);
uint8_t gl_format_to_num_channels(GLenum format);

}  // namespace gl
}  // namespace axby
