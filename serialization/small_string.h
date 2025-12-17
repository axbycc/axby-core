#pragma once

#include <array>
#include <string_view>

#include "debug/check.h"

// SmallString does not allocate, so it is trivial to copy it via
// memcpy. The main use case is for inclusion of a string-like
// identifier into a parent struct which needs to be memcopyable.
// This is useful for sending a struct across a process or network
// boundary without having to chase the internal data pointer of
// std::string.  Of course the tradeoff is an upper bound on the size
// needs to be known beforehand.

template <size_t N>
struct SmallString {
    static_assert(N > 0, "SmallString size must be greater than 0.");

    std::array<char, N> buffer = {0};
    size_t length = 0;

    SmallString() : length(0) { buffer[0] = '\0'; }

    void init_from_string_view(std::string_view str) {
        length = str.size();
        CHECK_LE(str.size() + 1, N)
            << "small string not big enough to store " << str;
        std::memcpy(buffer.data(), str.data(), str.size());
        buffer[length] = '\0';  // null terminate
    }

    SmallString(std::string_view str) { init_from_string_view(str); }

    SmallString(const std::string& str) { init_from_string_view(str); }

    SmallString(const char* str) { init_from_string_view(str); }

    const char* c_str() const { return buffer.data(); }

    std::string str() const {
        return std::string(buffer.begin(), buffer.begin() + length);
    }

    // begin, end for range compat
    const char* begin() const { return buffer.begin(); }
    const char* end() const { return buffer.begin() + length; }

    size_t size() const { return length; }

    bool empty() const { return length == 0; }

    operator std::string_view() const {
        return std::string_view(buffer.data(), length);
    }

    bool operator==(const char* other) const {
        return std::string_view(*this) == other;
    }

    bool operator==(std::string_view other) const {
        return std::string_view(*this) == other;
    }

    bool operator==(const std::string& other) const {
        return std::string_view(*this) == other;
    }

    bool operator!=(const char* other) const {
        return std::string_view(*this) != other;
    }

    bool operator!=(std::string_view other) const {
        return std::string_view(*this) != other;
    }

    bool operator!=(const std::string& other) const {
        return std::string_view(*this) != other;
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const SmallString<N>& ss) {
        os << std::string_view(ss);
        return os;
    }
};

static_assert(std::is_constructible_v<std::string_view, SmallString<10>>);

namespace std {
template <size_t N>
struct hash<SmallString<N>> {
    std::size_t operator()(const SmallString<N>& s) const noexcept {
        std::string_view view = s;
        return std::hash<std::string_view>{}(view);
    }
};
}  // namespace std
