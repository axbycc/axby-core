#pragma once

#include <cmath>
#include <limits>

namespace axby {

template <typename T>
inline T clip(T min, T max, T val) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

// puts value into range (0,1) based on lb and ub
template <typename T>
inline T interp(const T lb, const T ub, const T value) {
    T result = (value - lb) / (ub - lb);
    if (result < 0) return 0;
    if (result > 1) return 1;
    return result;
}

template <typename T>
bool allfinite(const T& ts) {
    for (const auto t : ts) {
        if (!std::isfinite(t)) return false;
    }
    return true;
}

template <typename T>
double absmax(const T& ts) {
    double result = std::numeric_limits<double>::lowest();
    for (const auto t : ts) {
        result = std::max(std::abs((double)t), result);
    }
    return result;
}

}  // namespace axby
