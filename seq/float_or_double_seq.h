#pragma once

#include <cstddef>

#include "debug/check.h"
#include "metaprogramming/metaprogramming.h"
#include "seq/any_seq.h"
#include "seq/seq.h"

namespace axby {
template <bool IsConst>
struct FloatOrDoubleSeqImpl {
    using ConstSelf = FloatOrDoubleSeqImpl<true>;
    using NonConstSelf = FloatOrDoubleSeqImpl<false>;
    using Self = FloatOrDoubleSeqImpl<IsConst>;

    using F = add_or_remove_const_t<float, IsConst>;
    using D = add_or_remove_const_t<double, IsConst>;

    using FloatSpan = std::span<F>;
    using DoubleSpan = std::span<D>;

    template <typename U>
        requires(
            // the first two lines here guard against instatiation of
            // malformed templates when the input is a ConstSelf or
            // NonConstSelf, which cannot be converted to span.
            !std::is_same_v<std::remove_cvref_t<U>, ConstSelf> &&
            !std::is_same_v<std::remove_cvref_t<U>, NonConstSelf> &&
            (std::is_constructible_v<Seq<F>, U> ||
             std::is_constructible_v<Seq<D>, U>))
    FloatOrDoubleSeqImpl(U&& seq) : seq_(seq) {}

    // self-constructor
    template <typename U>
        requires(std::is_same_v<std::remove_cvref_t<U>, NonConstSelf> ||
                 std::is_same_v<std::remove_cvref_t<U>, ConstSelf>)
    FloatOrDoubleSeqImpl(U&& fds) {
        constexpr bool input_is_const =
            std::is_same_v<std::remove_cvref_t<U>, ConstSelf>;
        static_assert(
            IsConst || (IsConst == input_is_const),
            "cannot init a FloatOrDoubleSeq from a ConstFloatOrDoubleSeq");

        seq_ = fds.seq_;
    }

    bool is_float() const { return seq_.template is_type<float>(); }
    bool is_double() const { return seq_.template is_type<double>(); }

    size_t size() const { return seq_.logical_size(); }

    FloatSpan floats() const {
        CHECK(is_float());
        return seq_.template get_span<F>();
    }

    DoubleSpan doubles() const {
        CHECK(is_double());
        return seq_.template get_span<D>();
    }

    double operator[](size_t i) const {
        CHECK(i < size());
        if (is_float()) {
            return floats()[i];
        } else {
            return doubles()[i];
        }
    }

    void copy_from(FloatOrDoubleSeqImpl</*IsConst=*/true> other)
        requires(!IsConst)
    {
        CHECK(other.size() == size());

        if (other.is_float() && is_float()) {
            seq_copy_bytes(other.floats(), /*dst=*/floats());
            return;
        }

        if (other.is_double() && is_double()) {
            seq_copy_bytes(other.doubles(), /*dst=*/doubles());
            return;
        }

        // mis-match floats/doubles
        if (is_float()) {
            seq_copy(/*src=*/other.doubles(), /*dst=*/floats());
            return;
        }

        if (is_double()) {
            seq_copy(/*src=*/other.floats(), /*dst=*/doubles());
            return;
        }
    }

    void write_to(FloatOrDoubleSeqImpl</*IsConst=*/false> dst) const {
        CHECK(dst.size() == size());

        if (dst.is_float() && is_float()) {
            seq_copy_bytes(/*src=*/floats(), /*dst=*/dst.floats());
            return;
        }

        if (dst.is_double() && is_double()) {
            seq_copy_bytes(/*src=*/doubles(), /*dst=*/dst.doubles());
            return;
        }

        // mis-match floats/doubles
        if (dst.is_float()) {
            seq_copy(/*src=*/doubles(), /*dst=*/dst.floats());
            return;
        }

        if (dst.is_double()) {
            seq_copy(/*src=*/floats(), /*dst=*/dst.doubles());
            return;
        }
    }

    AnySeqImpl<IsConst> seq_;

    std::string to_string() {
        if (is_float()) {
            return seq_to_string(floats());
        } else {
            return seq_to_string(doubles());
        }
    }
};

using ConstFloatOrDoubleSeq = FloatOrDoubleSeqImpl</*IsConst=*/true>;
using FloatOrDoubleSeq = FloatOrDoubleSeqImpl</*IsConst=*/false>;

}  // namespace axby
