#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "debug/check.h"

// =====================================================
// Misc concepts
template <typename T>
concept has_data_method = requires(T container) { container.data(); };
template <typename T>
concept has_size_method = requires(T container) { container.size(); };
template <class T>
concept has_resize_method = requires(T t, std::size_t n) {
    { t.resize(n) } -> std::same_as<void>;
};

template <typename T>
using data_pointer_t = decltype(std::declval<T&>().data());

// this helper allows us to enable memory copy between signed and
// unsigned char buffers, which we usually just view as a buffer of
// bytes
template <typename T>
inline constexpr bool is_byte_like =
    std::is_same_v<std::remove_cv_t<T>, std::byte> ||
    std::is_same_v<std::remove_cv_t<T>, uint8_t> ||
    std::is_same_v<std::remove_cv_t<T>, char> ||
    std::is_same_v<std::remove_cv_t<T>, signed char> ||
    std::is_same_v<std::remove_cv_t<T>, unsigned char>;

// =====================================================
// Begin value_type. Figuring out what type a containter contains
// eg: value_type_t<std::vector<float>> = float
// eg: value_type_t<double[]> = double
// eg: value_type_t<std::span<const int>> = const int
// Primary template
template <typename T, typename Enable = void>
struct value_type;
// Specialization for types with a member type `value_type`
template <typename T>
struct value_type<T, std::void_t<typename T::value_type>> {
    using type = typename T::value_type;
};
// Specialization for C arrays
template <typename T, std::size_t N>
struct value_type<T[N]> {
    using type = T;
};
// Specialization for pointers (to cover decayed arrays)
template <typename T>
struct value_type<T*> {
    using type = T;
};
// Specialization for span: in c++, the span::value_type strips
// constness. our version puts it back so it's consistent.
template <typename T>
struct value_type<std::span<T>> {
    using type = std::span<T>::element_type;
};

// specialization for buffers with data(): (const) void*
template <typename T>
concept void_data_buffer =
    has_data_method<T> && has_size_method<T> &&
    std::is_pointer_v<data_pointer_t<T>> &&
    std::same_as<std::remove_cv_t<std::remove_pointer_t<data_pointer_t<T>>>,
                 void>;
template <void_data_buffer T>
struct value_type<T, void> {
   private:
    using ptr = data_pointer_t<T>;
    using pointee = std::remove_pointer_t<ptr>;

   public:
    using type = std::
        conditional_t<std::is_const_v<pointee>, const std::byte, std::byte>;
};

// Helper alias template
template <typename T>
using value_type_t = typename value_type<std::decay_t<T>>::type;

template <typename T>
using value_type_nocv_t = std::remove_cv_t<value_type_t<T>>;

template <typename T, bool IsConst>
using add_or_remove_const_t =
    std::conditional_t<IsConst, std::add_const_t<T>, std::remove_const_t<T>>;

// ==========================================================================
// Begin to_span and friends for converting many different contiguous sequences to span.
// unlike the std::span constructor, this function also works for Eigen::Matrix

template <typename T>
auto to_span(T&& seq) {
    if constexpr (has_data_method<T> && has_size_method<T>) {
        // the seq has a .data() and .size() method, so we can
        // construct the span using the span(data, size) constructor.
        using Elem = typename std::remove_pointer<decltype(seq.data())>::type;
        if constexpr (std::is_void_v<Elem>) {
            // the the .data() returns a void*, so interpret the
            // object as a buffer of bytes
            if constexpr (std::is_const_v<Elem>) {
                return std::span((const std::byte*)seq.data(), seq.size());
            } else {
                return std::span((std::byte*)seq.data(), seq.size());
            }
        } else {
            // the normal case, Elem is not void
            return std::span((Elem*)seq.data(), seq.size());
        }
    } else {
        // fallback to regular std::span constructor, for things like
        // T=float[12]
        return std::span(std::forward<T>(seq));
    }
}

template <typename T>
auto to_cspan(const T& seq) {
    auto orig_span = to_span(seq);
    using V = value_type_t<decltype(orig_span)>;
    return std::span<const V>(orig_span.data(), orig_span.size());
}

template <typename TNew, typename T>
std::span<TNew> reinterpret_span(T& old_collection) {
    // WARNING: Undefined behavior unless:
    //   - TNew is std::byte/char/unsigned char (byte view), or
    //   - old_collection actually contains TNew objects (true round trip T -> bytes -> T).
    // Using this to reinterpret one unrelated type as another is not supported.

    using TOld = value_type_t<decltype(old_collection)>;
    static constexpr bool TNew_is_const = std::is_const_v<TNew>;
    static constexpr bool TOld_is_const = std::is_const_v<TOld>;
    static_assert(TNew_is_const || (TNew_is_const == TOld_is_const),
                  "Cannot reinterpret const as non-const");

    // calling to_span to give us .size() and .data(), because because
    // sometimes old_collection does not have these (eg float[12])
    //
    // ability to call to_span() also implies old_collection is contiguous
    auto old = to_span(old_collection);
    const auto num_old_bytes = old.size() * sizeof(TOld);
    CHECK(num_old_bytes % sizeof(TNew) == 0)
        << "num old bytes=" << num_old_bytes
        << " and sizeof(TNew)=" << sizeof(TNew);

    return std::span<TNew>((TNew*)old.data(), num_old_bytes / sizeof(TNew));
}
