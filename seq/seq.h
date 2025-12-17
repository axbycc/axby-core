#pragma once

#include <concepts>
#include <cstring>  // for std::memcpy
#include <iomanip>
#include <span>

#include "debug/check.h"
#include "metaprogramming/metaprogramming.h"

namespace axby {

template <typename T>
struct Seq {
    using value_type = T;
    using value_type_nocv = std::remove_cv_t<T>;
    // Seq models sequences backed by contiguous memory. It's like span,
    // but tries to construct itself liberally from any type that works
    // with to_span.
    //
    // For example, my_func(std::span<uint8_t> data) will not be callable
    // from a zmq message unless we write my_func(to_span(zmq_message)),
    // but my_other_func(Seq<uint8_t> data) will readily accept a zmq
    // message.
    //
    // Use this for function parameter types to make them accept a
    // wide range of arguments.

    template <class U>
        requires requires(U&& u) {
            // to_span(u) must exist and be convertible to std::span<T>
            { to_span(u) } -> std::convertible_to<std::span<T>>;
        }
    Seq(U&& sequence) : span_(to_span(sequence)) {}

    Seq() = default;

    explicit(!std::is_const_v<T>) Seq(std::initializer_list<T> il)
        : span_(il.begin(), il.size()) {}

    size_t size() const { return span_.size(); };
    auto data() { return span_.data(); };
    auto data() const { return span_.data(); };
    auto begin() { return span_.begin(); }
    auto begin() const { return span_.begin(); }
    auto end() { return span_.end(); }
    auto end() const { return span_.end(); }

    const T& operator[](size_t t) const { return span_[t]; }
    T& operator[](size_t t) { return span_[t]; }

    // overriding operator=() breaks copy constructor and provides a
    // footgun for accidentally copying large buffers
    //
    // void operator=(Seq<const T> other) {
    //     CHECK(other.size() == this->size());
    //     for (size_t i = 0; i < this->size(); ++i) {
    //         (*this)[i] = other[i];
    //     }
    // }

    // there is not a definition of operator==() that is most
    // reasonable. on the one hand, we can compare element-wise and on
    // the other hand, we can compare if two Seq point to the same
    // data. for the former, use seq_equals()
    //
    // bool operator==(Seq<const T> other) const {
    //     if (other.size() != this->size()) return false;
    //     for (size_t i = 0; i < this->size(); ++i) {
    //         if ((*this)[i] != other[i]) {
    //             return false;
    //         }
    //     }
    //     return true;
    // }

    std::span<const std::byte> as_bytes() const { return std::as_bytes(span_); }

    bool empty() const { return span_.empty(); }

    std::span<T> span_;
};

template <typename Ts>
auto to_seq(Ts&& ts) {
    return Seq<value_type_t<Ts>>(ts);
}

template <typename Ts>
auto to_cseq(Ts&& ts) {
    return Seq<const value_type_t<Ts>>(ts);
}

template <typename TSeq, typename USeq>
bool seq_equals(TSeq&& tseq, USeq&& useq) {
    auto ts = to_cseq(tseq);
    auto us = to_cseq(useq);

    if (ts.size() != us.size()) return false;

    for (size_t i = 0; i < ts.size(); ++i) {
        if (ts[i] != us[i]) {
            return false;
        }
    }

    return true;
}

template <typename TSrc, typename TDst>
void seq_copy(TSrc&& src, TDst&& dst) {
    static_assert(!std::is_const_v<value_type_t<decltype(dst)>>,
                  "dst cannot be const");

    auto d = to_seq(dst);
    auto s = to_cseq(src);
    CHECK_EQ(d.size(), s.size());

    for (size_t i = 0; i < s.size(); ++i) {
        d[i] = s[i];
    }
}

template <typename TSeq, typename USeq>
void seq_copy_bytes(TSeq&& src, USeq&& dst) {
    auto d = to_seq(dst);
    auto s = to_cseq(src);

    using dElem = value_type_t<decltype(dst)>;
    using sElem = value_type_t<decltype(src)>;
    using dElem_nocv = std::remove_cv_t<dElem>;
    using sElem_nocv = std::remove_cv_t<sElem>;

    static_assert(!std::is_const_v<dElem>, "dst cannot be const");

    static_assert(
        (std::is_same_v<dElem_nocv, sElem_nocv> || is_byte_like<dElem_nocv>));

    CHECK_EQ(d.as_bytes().size(), s.as_bytes().size());

    std::memcpy(/*dst=*/(void*)d.as_bytes().data(),
                /*src=*/(const void*)s.as_bytes().data(), s.as_bytes().size());
}

template <size_t N, typename TSeq>
    // requires(std::is_same_v<value_type_nocv_t<TSeq>, float> ||
    //          std::is_same_v<value_type_nocv_t<TSeq>, double>)
auto seq_to_array(TSeq&& tseq) {
    using T = value_type_nocv_t<TSeq>;
    Seq<const T> wrapper{tseq};

    CHECK(wrapper.size() == N);

    std::array<T, N> result;
    for (size_t i = 0; i < N; ++i) {
        result[i] = wrapper[i];
    }

    return result;
}

template <typename TSeq>
std::string seq_to_string(TSeq&& tseq, size_t max_items = 10) {
    auto items = to_cseq(tseq);

    std::stringstream ss;

    using Elem = value_type_nocv_t<decltype(items)>;

    if (std::is_same_v<double, Elem> || std::is_same_v<float, Elem>) {
        ss << std::setprecision(6) << std::setw(7) << std::setfill('.')
           << std::fixed;
    }
    size_t output_size = std::min(max_items, items.size());
    for (int i = 0; i < output_size; ++i) {
        if constexpr (is_byte_like<Elem>) {
            // cast to int, otherwise stringstream interprets a byte
            // as a character code and will print a blank to the
            // screen for many different byte values
            ss << int(items[i]);
        } else {
            ss << items[i];
        }
        if (i + 1 < output_size) {
            ss << ", ";
        }
    }
    if (output_size < items.size()) {
        ss << " ... (" << items.size() - output_size << " more)";
    }
    return ss.str();
}

template <typename TSrc, typename TIdx, typename Tout>
void seq_take(const TSrc& src,
              const TIdx& indices,
              Tout& out,
              size_t stride = 1) {
    auto src_seq = to_cseq(src);
    auto indices_seq = to_cseq(indices);
    auto out_seq = to_seq(out);

    CHECK_EQ(out_seq.size(), stride * indices_seq.size());

    size_t out_group_idx = 0;
    for (const auto src_group_idx : indices_seq) {
        const size_t src_idx = stride * src_group_idx;
        const size_t out_idx = stride * out_group_idx;

        CHECK(src_idx < src_seq.size());
        CHECK(out_idx < out_seq.size());

        for (int i = 0; i < stride; ++i) {
            out_seq[out_idx + i] = src_seq[src_idx + i];
        }

        out_group_idx++;
    }
}

template <typename T>
requires(std::is_trivially_copyable_v<T>)
T seq_bit_cast(Seq<const std::byte> bytes) {
    CHECK_EQ(bytes.size(), sizeof(T));
    // the compiler will probably(?) elide this temp array
    std::array<std::byte, sizeof(T)> temp;
    std::memcpy(/*dst=*/temp.data(), /*src=*/bytes.data(),
                sizeof(T));
    // the compiler will apply return value optimization
    return std::bit_cast<T>(temp);
}

}  // namespace axby
