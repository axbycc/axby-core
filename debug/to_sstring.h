#pragma once

#include <iomanip>
#include <sstream>

namespace axby {
// the function below is useful for turning
// structs which implement ostream operator
// into strings
template <typename T>
std::string to_sstring(const T& t) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(5) << t;
    return ss.str();
}
}  // namespace axby
