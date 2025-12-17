#pragma once

#include <cbor.h>

#include <boost/pfr.hpp>
#include <cstdint>
#include <magic_enum.hpp>
#include <ranges>
#include <string_view>

#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "macros.h"
#include "metaprogramming/metaprogramming.h"
#include "primitives.h"
#include "seq/seq.h"
#include "serialization/serialization.h"

#define INSTANTIATE_SERIALIZE_AND_DESERIALIZE_CBOR_TEMPLATES(Type)           \
    template bool ::axby::serialization::serialize_cbor(                     \
        const Type&, std::byte*, size_t, size_t*, const char**);             \
    template bool ::axby::serialization::serialize_cbor(                     \
        const Type&, ::axby::FastResizableVector<std::byte>&, const char**); \
    template bool ::axby::serialization::deserialize_cbor(                   \
        Type&, const std::byte*, size_t, const char**);                      \
    template bool ::axby::serialization::deserialize_cbor(                   \
        Type&, ::axby::Seq<const std::byte>, const char**);

namespace axby {
namespace serialization {

template <class T>
concept has_resize = requires(T t, std::size_t n) {
    { t.resize(n) } -> std::same_as<void>;
};

template <class T>
concept is_enum = magic_enum::is_scoped_enum<T>::value ||
                  magic_enum::is_unscoped_enum<T>::value;

// hack for to make static_assert conditionally compile inside if
// constexpr blocks
template <typename>
inline constexpr bool always_false = false;

// ==================== Encoding functions =================================

template <typename T>
CborError cbor_encode(CborEncoder* encoder, const T& value) {
    // this template is only used when none of the primitives in
    // primitives.h match.  therefore we can assume T does not match
    // any of those cases. eg, T is not float, double, int, etc.

    constexpr bool debug = false;

    if constexpr (is_enum<T>) {
        // encode enum as a string
        const std::string_view enum_name = magic_enum::enum_name(value);
        return cbor_encode_text_string(encoder, enum_name.data(),
                                       enum_name.size());
    } else if constexpr (std::is_constructible_v<std::span<const std::byte>, T> ||
                       std::is_constructible_v<std::span<const uint8_t>, T>) {
        // special case: contiguous byte array
        return cbor_encode(encoder,
                           reinterpret_span<const std::byte>(value));
    } else if constexpr (std::is_constructible_v<std::string_view, T>) {
        // special case: string-like (contiguous chars)
        return cbor_encode(encoder, std::string_view(value));
    } else if constexpr (std::ranges::range<T>) {
        // general list of objects
        const auto& list_like = value;  // rename, for readability
        CborEncoder list_encoder;
        CBOR_RETURN_IF_ERROR(cbor_encoder_create_array(
                                 encoder, &list_encoder, list_like.size()));
        // LOG_IF(INFO, debug) << "CBOR ENCODE LIST LEN " << list_like.size();
        for (const auto& elem : list_like) {
            CBOR_RETURN_IF_ERROR(cbor_encode(&list_encoder, elem));
        }
        CBOR_RETURN_IF_ERROR(
            cbor_encoder_close_container_checked(encoder, &list_encoder));
        return CborNoError;
    } else if constexpr (std::is_class_v<T>) {
        // encode as dict[str,Object]
        LOG_IF(INFO, debug) << "CBOR ENCODE DICT";

        auto field_names = boost::pfr::names_as_array<T>();

        CborEncoder map_encoder;
        CBOR_RETURN_IF_ERROR(
            cbor_encoder_create_map(encoder, &map_encoder, field_names.size()));

        auto field_encoding_error = CborNoError;
        boost::pfr::for_each_field(value, [&](const auto& f, auto i) {
            // if a previous field failed to encode, don't encode any
            // more fields
            CBOR_RETURN_IF_ERROR(field_encoding_error);

            LOG_IF(INFO, debug) << "Encoding field ." << field_names[i]
                                << ", type " << typeid(decltype(f)).name();

            // encode key
            field_encoding_error = cbor_encode_text_string(
                &map_encoder, field_names[i].data(), field_names[i].size());
            LOG_IF(INFO, debug && field_encoding_error)
                << "Field name failure "
                << cbor_error_string(field_encoding_error);
            CBOR_RETURN_IF_ERROR(field_encoding_error);

            // encode value
            field_encoding_error = cbor_encode(&map_encoder, f);
            LOG_IF(INFO, debug && field_encoding_error)
                << "Value encoding failure "
                << cbor_error_string(field_encoding_error);

            return CborNoError;
        });

        CBOR_RETURN_IF_ERROR(field_encoding_error);
        CBOR_RETURN_IF_ERROR(
            cbor_encoder_close_container_checked(encoder, &map_encoder));

        LOG_IF(INFO, debug) << "CBOR ENCODE DICT DONE";
        return CborNoError;
    } else {
        // always_false prevents unconditional compilation of the
        // static_assert
        static_assert(always_false<T>,
                      "Don't know how to serialize this type T");
        return CborUnknownError;
    }
}

template <typename T>
size_t estimate_cbor_size(const T& value) {
    // this template is only used when none of the primitives in
    // primitives.h match.  therefore we can assume T does not match
    // any of those cases. eg, T is not float, double, int, etc.

    if constexpr (is_enum<T>) {
        // encode enum as a string
        const std::string_view enum_name = magic_enum::enum_name(value);
        return enum_name.size();
    } if constexpr (std::is_constructible_v<std::span<const std::byte>, T> ||
                    std::is_constructible_v<std::span<const uint8_t>, T>) {
            // special case: contiguous byte array
            return value.size();
    } else if constexpr (std::is_constructible_v<std::string_view, T>) {
        // special case: string-like (contiguous chars)
        return value.size();
    } else if constexpr (std::ranges::range<T>) {
        // general list of objects
        size_t estimated_list_size = 0;
        const auto& list_like = value;  // rename, for readability
        for (const auto& elem : list_like) {
            estimated_list_size += estimate_cbor_size(elem);
        }
        return estimated_list_size;
    } else if constexpr (std::is_class_v<T>) {
        // encode as dict[str,Object]
        size_t estimated_obj_size = 0;
        auto field_names = boost::pfr::names_as_array<T>();
        boost::pfr::for_each_field(value, [&](const auto& f, auto i) {
            estimated_obj_size += field_names[i].size();
            estimated_obj_size += estimate_cbor_size(f);
        });
        return estimated_obj_size;
    } else {
        return sizeof(T);
    }
}

template <typename T>
bool serialize_cbor(const T& item,
                    std::byte* buf,
                    size_t bufsize,
                    size_t* bufsize_actual,
                    const char** out_error_msg) {
    CborEncoder encoder;
    cbor_encoder_init(&encoder, (uint8_t*)buf, bufsize, 0);
    auto result = cbor_encode(&encoder, item);

    if (result != CborNoError) {
        if (out_error_msg) {
            *out_error_msg = cbor_error_string(result);
        }
        return false;
    }

    if (out_error_msg) {
        *out_error_msg = "";
    }

    *bufsize_actual = cbor_encoder_get_buffer_size(&encoder, (uint8_t*)buf);
    return true;
}

template <typename T>
bool serialize_cbor(const T& item,
                    FastResizableVector<std::byte>& buf,
                    const char** out_error_msg) {
    size_t estimated_cbor_size = 1.5 * estimate_cbor_size(item);
    buf.resize(estimated_cbor_size);
    size_t bufsize_actual = 0;
    const bool ok = serialize_cbor(item, buf.data(), buf.size(),
                                   &bufsize_actual, out_error_msg);
    if (ok) {
        buf.resize(bufsize_actual);
    } else {
        buf.clear();
    }
    return ok;
}

// ==================== Decoding functions =================================

template <typename T>
CborError cbor_decode(const CborValue* cbor_value, T& out) {
    // this template is only used when none of the primitives in
    // primitives.h match.  therefore we can assume T does not match
    // any of those cases. eg, T is not float, double, int, etc.

    constexpr bool debug = false;

    if constexpr (is_enum<T>) {
        // T is enum. It would have been serialized as a cbor string.
        std::string_view enum_str;
        CBOR_RETURN_IF_ERROR(cbor_decode(cbor_value, enum_str));
        auto enum_value = magic_enum::enum_cast<T>(enum_str);
        if (enum_value) {
            out = enum_value.value();
        } else {
            LOG(FATAL) << "Could not deserialize value " << enum_str
                       << " into to an enum";
        }
        return CborNoError;
    } else if constexpr (std::is_constructible_v<std::span<const std::byte>,
                                                 T> ||
                         std::is_constructible_v<std::span<const uint8_t>, T>) {
        // the above condition means that the destination field can be
        // thought of as a contiguous sequence of bytes. in that case
        // we fill that output field with the source bytes.
        //
        // however, if the destination field actually *is* a span, we
        // want it to set its pointer into the internal serialized
        // bytes. this case is handled by specific overload of
        // std::span types in the non-templated
        // primitives.cpp/primitives.h.
        //
        // todo: make this less brittle by moving the std::span
        // handlers from primitives to template
        //
        static_assert(!std::is_same_v<T, std::span<const std::byte>>);
        static_assert(!std::is_same_v<T, std::span<const uint8_t>>);

        // T is a contiguous sequence of bytes. It would have been
        // serialized as a bytearray. Get a pointer to the serialized
        // bytearray.
        std::span<const std::byte> bytearray;
        CBOR_RETURN_IF_ERROR(cbor_decode(cbor_value, bytearray));

        if constexpr (has_resize<T>) {
            // allocate memory to receive the serialized
            // bytearray, eg if T is std::vector<std::byte>
            out.resize(bytearray.size());
        }

        // check that either the resize worked, or the static size
        // expected by the receiving buffer matches the number of
        // serialized bytes.
        CHECK_EQ(out.size(), bytearray.size());

        axby::seq_copy(/*src=*/bytearray, /*dst=*/out);
        return CborNoError;
    } else if constexpr (std::is_constructible_v<T, std::string_view>) {
        // T is a string-like object. It would have been serialized as
        // a string. Get a view to the serialized string.
        //
        // Note the order of the arguments in is_constructible is
        // reversed. We want to assign into T from a string_view of
        // the serialized contents. We need to do this because strings
        // come with special semantics like small-string optimization
        // and null byte termination.
        //
        // This is different from the bytearray case, where we
        // basically memcpy the bytes.

        std::string_view strview;
        CBOR_RETURN_IF_ERROR(cbor_decode(cbor_value, strview));
        out = strview;

        return CborNoError;
    } else if constexpr (std::ranges::range<T>) {
        // T is a list of generic objects, and so the corresponding
        // cbor value needs to be a list cbor array.

        CHECK(cbor_value_is_array(cbor_value));

        using Elem = value_type_t<T>;
        constexpr bool is_integral = std::is_integral_v<Elem>;
        constexpr bool is_floating_point = std::is_floating_point_v<Elem>;
        constexpr bool is_fixed_size = is_integral || is_floating_point;

        size_t list_length;
        CBOR_RETURN_IF_ERROR(
            cbor_value_get_array_length(cbor_value, &list_length));

        if constexpr (has_resize<T>) {
            LOG_IF(INFO, debug) << "Resizing to " << list_length;
            out.resize(list_length);
        }
        // check that either the resize worked, or the static size
        // expected by the receiving buffer matches the number of
        // serialized items.
        CHECK(out.size() == list_length) << out.size() << " vs " << list_length;

        // enter the container and deserialize one by one
        CborValue value_it;
        CBOR_RETURN_IF_ERROR(cbor_value_enter_container(cbor_value, &value_it));

        // LOG_IF(INFO, debug) << "Fixed size? " << is_fixed_size;
        // LOG_IF(INFO, debug) << "Element type? " << typeid(Elem).name();

        for (size_t i = 0; i < list_length; ++i) {
            // LOG_IF(INFO, debug)
            //     << "Decoding element " << i << " of " << list_length;
            CBOR_RETURN_IF_ERROR(cbor_decode(&value_it, out[i]));

            if constexpr (is_fixed_size) {
                CBOR_RETURN_IF_ERROR(cbor_value_advance_fixed(&value_it));
            } else {
                CBOR_RETURN_IF_ERROR(cbor_value_advance(&value_it));
            }
        }

        return CborNoError;
    } else if constexpr (std::is_class_v<T>) {
        // T is a generic class. It should have been serialized as a
        // cbor map.
        CHECK(cbor_value_is_map(cbor_value));
        auto field_names = boost::pfr::names_as_array<T>();

        CborError field_decoding_error = CborNoError;
        boost::pfr::for_each_field(out, [&](auto& f, auto i) {
            LOG_IF(INFO, debug && field_decoding_error)
                << "\tBailing " << cbor_error_string(field_decoding_error);
            CBOR_RETURN_IF_ERROR(field_decoding_error);

            LOG_IF(INFO, debug) << "Decoding field " << field_names[i];
            CborValue element;
            field_decoding_error =
                cbor_value_map_find_value1(cbor_value, field_names[i].data(),
                                           field_names[i].size(), &element);
            LOG_IF(INFO, debug && field_decoding_error)
                << "\tMap find failed "
                << cbor_error_string(field_decoding_error);
            CBOR_RETURN_IF_ERROR(field_decoding_error);

            // if the key is not found, the tinycbor library does not
            // consider it an error, but instead returns a value with
            // CborInvalidType
            const bool not_found = !cbor_value_is_valid(&element);
            if (not_found) {
                field_decoding_error = CborUnknownError;
                LOG(ERROR) << "Struct field " << field_names[i]
                           << " was not present in cbor";
                // there is no CborKeyNotFound, so we use
                // CborUnkownError here
                field_decoding_error = CborUnknownError;
            }
            CBOR_RETURN_IF_ERROR(field_decoding_error);

            LOG_IF(INFO, debug) << "Found the element, recursing "
                                << typeid(decltype(f)).name();
            field_decoding_error = cbor_decode(&element, f);
            CBOR_RETURN_IF_ERROR(field_decoding_error);

            field_decoding_error = CborNoError;
            return CborNoError;
        });

        return CborNoError;
    } else {
        // always_false prevents unconditional compilation of the
        // static_assert
        static_assert(always_false<T>,
                      "Don't know how to deserialize this type T");
        return CborUnknownError;
    }
}

template <typename T>
bool deserialize_cbor(T& item,
                      const std::byte* buf,
                      size_t buf_size,
                      const char** out_error_msg) {
    CborParser decoder;
    CborValue val;

    {
        CborError error =
            cbor_parser_init((const uint8_t*)buf, buf_size, 0, &decoder, &val);
        CHECK(error == CborNoError);
    }

    CborError error = cbor_decode(&val, item);
    if (error != CborNoError) {
        if (out_error_msg) {
            *out_error_msg = cbor_error_string(error);
        }
        return false;
    }

    if (out_error_msg) {
        *out_error_msg = "";
    }

    return (error == CborNoError);
}

template <typename T>
bool deserialize_cbor(T& item,
                      Seq<const std::byte> buf,
                      const char** out_error_msg) {
    return deserialize_cbor(item, buf.data(), buf.size(), out_error_msg);
}

}  // namespace serialization
}  // namespace axby
