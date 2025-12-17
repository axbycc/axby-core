#pragma once

#include <iostream>
#include <memory>
#include <typeindex>

/* Usage:

// In the .h file
class YourClass {
  YourClass();
  Pimpl pimpl;
};

// In the .cpp file
// the concrete implementation MUST inherit from Impl!
class YourClassImpl : public Impl {
  ...
};

YourClass::YourClass() {
  pimpl.emplace<YourClassImpl>(...);

  // retrieve the actual implementation
  auto& impl = pimpl.as<YourClassImpl>();
  // do stuff with impl
}

*/

namespace axby {

class Impl {
   public:
    virtual ~Impl() = default;
};

// lightweight check function to prevent abseil dependency
#define __PIMPL_CHECK__(condition)                                         \
    do {                                                                   \
        if (!(condition)) {                                                \
            std::cerr << "CHECK failed: " #condition << " at " << __FILE__ \
                      << ":" << __LINE__ << std::endl;                     \
            std::abort();                                                  \
        }                                                                  \
    } while (0)

struct Pimpl {
    std::unique_ptr<Impl> impl;
    std::type_index type = typeid(void);

    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        impl = std::make_unique<T>(std::forward<Args>(args)...);
        type = typeid(T);

        return as<T>();
    }

    template <typename T>
    T& as() const {
        __PIMPL_CHECK__(type == typeid(T) ||
                        type == typeid(std::remove_const_t<T>));
        return reinterpret_cast<T&>(*impl);
    }
};
}  // namespace axby
