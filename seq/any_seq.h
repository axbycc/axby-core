#pragma once

#include <span>
#include <typeindex>

#include "metaprogramming/metaprogramming.h"

namespace axby {

template <typename U>
concept is_span_like = requires(U&& u) {
    { to_span(u) };
};

template <bool IsConst>
struct AnySeqImpl {
    using Self = AnySeqImpl<IsConst>;
    using ConstSelf = AnySeqImpl<false>;
    using NonConstSelf = AnySeqImpl<true>;
    using maybe_const_byte = add_or_remove_const_t<std::byte, IsConst>;

    AnySeqImpl() = default;

    template <class U>
        requires(std::same_as<std::remove_cvref_t<U>, ConstSelf> ||
                 std::same_as<std::remove_cvref_t<U>, NonConstSelf>)
    AnySeqImpl(const U& u) {
        constexpr bool input_is_const = std::is_same_v<std::remove_cvref_t<U>, ConstSelf>;
        static_assert(IsConst || (input_is_const == IsConst), "Cannot init AnySeq from ConstAnySeq");
        span_ = u.span_;
    }

    template <class U>
        requires(!std::same_as<std::remove_cvref_t<U>, ConstSelf> &&
                 !std::same_as<std::remove_cvref_t<U>, NonConstSelf>) &&
                is_span_like<U>
    AnySeqImpl(U&& sequence) {
        if constexpr (IsConst) {
            span_ = std::as_bytes(to_span(sequence));
        } else {
            span_ = std::as_writable_bytes(to_span(sequence));
        }
        value_type_nocv_ = typeid(std::remove_cv_t<value_type_t<U>>);
        logical_size_ = span_.size() / sizeof(value_type_t<U>);
    }

    // constness matters here.
    template <typename T>
        requires(std::is_const_v<T> || !IsConst)
    std::span<T> get_span() const {
        CHECK(value_type_nocv_ == typeid(std::remove_cv_t<T>));
        return reinterpret_span<T>(span_);
    }

    // don't use const/volatile qualifiers with is_type eg
    // is_type<float> instead of is_type<const float>
    template <typename T>
        requires(std::is_same_v<T, std::remove_cv_t<T>>)
    bool is_type() const {
        return typeid(std::remove_cv_t<T>) == value_type_nocv_;
    }

    size_t num_bytes() const { return span_.size(); }
    size_t logical_size() const { return logical_size_; }

    auto as_bytes() const { return span_; }

    // returns the typeid, without cv qualifiers
    auto get_typeid() const { return value_type_nocv_; };

    bool empty() const { return span_.empty(); }

    size_t logical_size_ = 0;
    std::type_index value_type_nocv_ = typeid(void);
    std::span<maybe_const_byte> span_;
};

using AnySeq = AnySeqImpl</*IsConst=*/false>;
using ConstAnySeq = AnySeqImpl</*IsConst=*/true>;

}  // namespace axby
