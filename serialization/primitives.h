#pragma once

#include <cbor.h>

#include <cstdint>
#include <span>
#include <string_view>

namespace axby {

namespace serialization {

// encoding functions
CborError cbor_encode(CborEncoder* encoder, bool value);

CborError cbor_encode(CborEncoder* encoder, uint8_t value);

CborError cbor_encode(CborEncoder* encoder, uint16_t value);

CborError cbor_encode(CborEncoder* encoder, uint32_t value);

CborError cbor_encode(CborEncoder* encoder, uint64_t value);

CborError cbor_encode(CborEncoder* encoder, int8_t value);

CborError cbor_encode(CborEncoder* encoder, int16_t value);

CborError cbor_encode(CborEncoder* encoder, int32_t value);

CborError cbor_encode(CborEncoder* encoder, int64_t value);

CborError cbor_encode(CborEncoder* encoder, const std::string_view value);

CborError cbor_encode(CborEncoder* encoder, std::span<const std::byte> bytes);

CborError cbor_encode(CborEncoder* encoder,
                      std::span<uint8_t> bytes); /* deprecate! */

CborError cbor_encode(CborEncoder* encoder, float value);

CborError cbor_encode(CborEncoder* encoder, double value);

// decoding functions
CborError cbor_decode(const CborValue* cbor_value, float& f);

CborError cbor_decode(const CborValue* cbor_value, double& d);

CborError cbor_decode(const CborValue* cbor_value, uint8_t& ui);

CborError cbor_decode(const CborValue* cbor_value, uint16_t& ui);

CborError cbor_decode(const CborValue* cbor_value, uint32_t& ui);

CborError cbor_decode(const CborValue* cbor_value, uint64_t& ui);

CborError cbor_decode(const CborValue* cbor_value, int8_t& ui);

CborError cbor_decode(const CborValue* cbor_value, int16_t& ui);

CborError cbor_decode(const CborValue* cbor_value, int32_t& ui);

CborError cbor_decode(const CborValue* cbor_value, int64_t& ui);

CborError cbor_decode(const CborValue* cbor_value, std::string& str);

CborError cbor_decode(const CborValue* cbor_value, std::string_view& str);

CborError cbor_decode(const CborValue* cbor_value,
                      std::span<const std::byte>& bytes);

CborError cbor_decode(const CborValue* cbor_value,
                      std::span<const uint8_t>& bytes);

// get cbor string or byte_string extents as a pointer and size this
// only works the data is continguous, which in turn happens when the
// data is not too big.
CborError cbor_get_span_extents(const CborValue* cbor_value,
                                const uint8_t** data,
                                size_t* size);

}  // namespace serialization
}  // namespace axby
