#include "primitives.h"

#include <cbor.h>

#include <cstdint>

#include "debug/check.h"
#include "macros.h"

namespace axby {

namespace serialization {

CborError cbor_encode(CborEncoder* encoder, bool value) {
    return cbor_encode_boolean(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, uint8_t value) {
    return cbor_encode_uint(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, uint16_t value) {
    return cbor_encode_uint(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, uint32_t value) {
    return cbor_encode_uint(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, uint64_t value) {
    return cbor_encode_uint(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, int8_t value) {
    return cbor_encode_int(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, int16_t value) {
    return cbor_encode_int(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, int32_t value) {
    return cbor_encode_int(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, int64_t value) {
    return cbor_encode_int(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, const std::string_view value) {
    // LOG(INFO) << "Encoding string " << value;
    return cbor_encode_text_string(encoder, value.data(), value.size());
}

CborError cbor_encode(CborEncoder* encoder, std::span<const uint8_t> bytes) {
    return cbor_encode_byte_string(encoder, bytes.data(), bytes.size());
}

CborError cbor_encode(CborEncoder* encoder, std::span<const std::byte> bytes) {
    return cbor_encode_byte_string(encoder, (const uint8_t*)bytes.data(),
                                   bytes.size());
}

CborError cbor_encode(CborEncoder* encoder, float value) {
    return cbor_encode_float(encoder, value);
}

CborError cbor_encode(CborEncoder* encoder, double value) {
    return cbor_encode_double(encoder, value);
}

// fwd declare so we can decode float via double
CborError cbor_decode(const CborValue* cbor_value, double& d);

CborError cbor_decode(const CborValue* cbor_value, float& f) {
    // make an attempt to decode double as float
    if (cbor_value_get_type(cbor_value) == CborDoubleType) {
        double d;
        CBOR_RETURN_IF_ERROR(cbor_decode(cbor_value, d));

        f = (float)d;  // todo: overflow check or warning
        return CborNoError;
    }

    return cbor_value_get_float(cbor_value, &f);
}

CborError cbor_decode(const CborValue* cbor_value, double& d) {
    return cbor_value_get_double(cbor_value, &d);
}

CborError cbor_decode(const CborValue* cbor_value, uint8_t& ui) {
    uint64_t temp;
    CBOR_RETURN_IF_ERROR(cbor_value_get_uint64(cbor_value, &temp));
    ui = (uint8_t)temp;
    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value, uint16_t& ui) {
    uint64_t temp;
    CBOR_RETURN_IF_ERROR(cbor_value_get_uint64(cbor_value, &temp));
    ui = (uint16_t)temp;
    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value, uint32_t& ui) {
    uint64_t temp;
    CBOR_RETURN_IF_ERROR(cbor_value_get_uint64(cbor_value, &temp));
    ui = (uint32_t)temp;
    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value, uint64_t& ui) {
    return cbor_value_get_uint64(cbor_value, &ui);
}

CborError cbor_decode(const CborValue* cbor_value, int8_t& ui) {
    int64_t temp;
    CBOR_RETURN_IF_ERROR(cbor_value_get_int64(cbor_value, &temp));
    ui = (int8_t)temp;
    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value, int16_t& ui) {
    int64_t temp;
    CBOR_RETURN_IF_ERROR(cbor_value_get_int64(cbor_value, &temp));
    ui = (int16_t)temp;
    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value, int32_t& ui) {
    int64_t temp;
    CBOR_RETURN_IF_ERROR(cbor_value_get_int64(cbor_value, &temp));
    ui = (int32_t)temp;
    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value, int64_t& ui) {
    return cbor_value_get_int64(cbor_value, &ui);
}

CborError cbor_decode(const CborValue* cbor_value, std::string& str) {
    size_t len = 0;

    CBOR_RETURN_IF_ERROR(cbor_value_get_string_length(cbor_value, &len));
    str.resize(len);

    char* data = str.data();  // need cpp20
    CBOR_RETURN_IF_ERROR(
        cbor_value_copy_text_string(cbor_value, data, &len, /*next=*/nullptr));

    // cpp11 strings are always null-terminated
    // so access to the [len] element is safe
    str[len] = 0;  // null terminate

    return CborNoError;
}

CborError cbor_get_span_extents(const CborValue* cbor_value,
                                const uint8_t** data,
                                size_t* size) {
    size_t length = 0;
    CBOR_RETURN_IF_ERROR(cbor_value_get_string_length(cbor_value, &length));

    CborValue tmp = *cbor_value;
    CBOR_RETURN_IF_ERROR(cbor_value_begin_string_iteration(&tmp));

    const uint8_t* str_start = nullptr;
    size_t chunk_size;
    CBOR_RETURN_IF_ERROR(cbor_value_get_byte_string_chunk(
        &tmp, &str_start, &chunk_size, nullptr));

    CHECK(chunk_size == length);

    *data = (const uint8_t*)str_start;
    *size = length;

    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value,
                      std::span<const std::byte>& bytes) {
    size_t length = 0;
    const uint8_t* data = nullptr;
    CBOR_RETURN_IF_ERROR(cbor_get_span_extents(cbor_value, &data, &length));
    bytes = std::span<const std::byte>((const std::byte*)data, length);
    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value,
                      std::span<const uint8_t>& bytes) {
    size_t length = 0;
    const uint8_t* data = nullptr;
    CBOR_RETURN_IF_ERROR(cbor_get_span_extents(cbor_value, &data, &length));
    bytes = std::span<const uint8_t>((const uint8_t*)data, length);
    return CborNoError;
}

CborError cbor_decode(const CborValue* cbor_value, std::string_view& bytes) {
    // set the span so that it points to the position of the byte
    // array within the parser buffer

    size_t length = 0;
    CBOR_RETURN_IF_ERROR(cbor_value_get_string_length(cbor_value, &length));

    CborValue tmp = *cbor_value;

    CBOR_RETURN_IF_ERROR(cbor_value_begin_string_iteration(&tmp));

    const char* str_start = nullptr;
    size_t chunk_size;
    CBOR_RETURN_IF_ERROR(cbor_value_get_text_string_chunk(
        &tmp, &str_start, &chunk_size, nullptr));

    CHECK(chunk_size == length);

    bytes = std::string_view((char*)str_start, length);
    return CborNoError;
}

}  // namespace serialization
}  // namespace axby
