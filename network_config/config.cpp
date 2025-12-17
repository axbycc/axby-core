#include "config.h"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/numbers.h"
#include "app/files.h"
#include "debug/check.h"
#include "debug/log.h"
#include "metaprogramming/metaprogramming.h"

namespace axby {
namespace network_config {

using json = nlohmann::json;

inline Endpoint ParseEndpoint(std::string_view uri) {
    // Example accepted formats:
    //   tcp://127.0.0.1:7777
    //   udp://10.0.0.5:4444
    //
    // General format:
    //   <protocol>://<ip>:<port>

    const size_t sep_protocol = uri.find("://");
    CHECK(sep_protocol != std::string_view::npos) << "Missing '://'";
    CHECK(sep_protocol > 0) << "Empty protocol";

    std::string_view protocol = uri.substr(0, sep_protocol);
    uri.remove_prefix(sep_protocol + 3);  // strip "://"

    const size_t sep_port = uri.rfind(':');
    CHECK(sep_port != std::string_view::npos) << "Missing ':' before port";
    CHECK(sep_port > 0) << "Empty host/ip";

    std::string_view ip = uri.substr(0, sep_port);
    std::string_view port_str = uri.substr(sep_port + 1);

    CHECK(!port_str.empty()) << "Empty port";

    uint32_t port32 = 0;
    CHECK(absl::SimpleAtoi(port_str, &port32)) << "Non-numeric port";
    CHECK(port32 <= std::numeric_limits<uint16_t>::max())
        << "Port out of range: " << port32;

    Endpoint ep;
    ep.protocol = std::string(protocol);
    ep.ip = std::string(ip);
    ep.port = static_cast<uint16_t>(port32);
    return ep;
}

struct ConfigImpl : axby::Impl {
    ConfigImpl(json j0) : j(j0) {}
    json j;
};

Config::Config() {};

Config::Config(std::string_view config_name) {
    std::filesystem::path config_dir =
        axby::prepend_home_path(".network_config");
    std::filesystem::path config_path = config_dir / absl::StrFormat("%s.json", config_name);

    LOG(INFO) << "Loading network config from " << config_path;

    auto file_contents = axby::read_bytes_from_file(config_path.string());
    auto j = json::parse(reinterpret_span<const char>(file_contents));
    impl_.emplace<ConfigImpl>(j);
}

SystemConfig Config::get(std::string_view key) const {
    SystemConfig result;
    auto& system = impl_.as<ConfigImpl>().j[key];
    if (system.contains("bind")) {
        result.bind = system["bind"].get<std::string>();
    }
    if (system.contains("connect")) {
        result.connect = system["connect"].get<std::string>();
    }
    if (system.contains("kissnet")) {
        result.kissnet = ParseEndpoint(system["kissnet"].get<std::string>());
    }
    return result;
}

}  // namespace network_config
}  // namespace axby
