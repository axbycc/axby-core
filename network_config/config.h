#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include "pimpl/pimpl.h"

namespace axby {
namespace network_config {

struct Endpoint {
    std::string protocol;
    std::string ip;  
    uint16_t port = 0;
};

struct SystemConfig {
    std::string bind;
    std::string connect;
    Endpoint kissnet;
};

class Config {
   public:
    Config();
    Config(std::string_view config_name);
    Config(const char* config_filename) : Config(std::string_view(config_filename)) {};
    SystemConfig get(std::string_view system) const;
    axby::Pimpl impl_;
};


}  // namespace network_config
}
