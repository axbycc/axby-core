#pragma once

/*
 To register a type that can be used with these functions, create a
 xxx.cpp file ("xxx" can be whatever you like), include
 "make_serializable.hpp" and use the
 INSTANTIATE_SERIALIZE_AND_DESERIALIZE_CBOR_TEMPLATES macro in the
 global namespace. When compiling the xxx.cpp, link with the
 //serialization:make_serializable dependency.

 Then in any other compilation unit, you can just call these functions
 as long as you link with your xxx object file
*/

#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "seq/seq.h"
#include <string_view>

namespace axby {
namespace serialization {

template <typename T>
bool serialize_cbor(const T& item,
                    std::byte* buf,
                    size_t buf_size,
                    size_t* bufsize_actual,
                    const char** error_out = nullptr);

template <typename T>
bool serialize_cbor(const T& item,
                    FastResizableVector<std::byte>& buf,
                    const char** out_error_msg = nullptr);

template <typename T>
bool deserialize_cbor(T& item,
                      const std::byte* buf,
                      size_t buf_size,
                      const char** error_out = nullptr);

template <typename T>
bool deserialize_cbor(T& item,
                      Seq<const std::byte> buf,
                      const char** out_error_msg = nullptr);

template <typename T>
bool deserialize_cbor(T& item,
                      std::string_view buf,
                      const char** out_error_msg = nullptr) {
    return deserialize_cbor<T>(item, reinterpret_span<const std::byte>(buf), out_error_msg);
};

}  // namespace serialization
}  // namespace axby
