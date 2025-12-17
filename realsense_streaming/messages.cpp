#include "messages.h"

#include <magic_enum.hpp>

namespace axby {
namespace realsense_streaming {
std::ostream& operator<<(std::ostream& os, const StreamMeta& streamMeta) {
    os << "Stream ID: " << streamMeta.id << ", "
       << "Name: " << streamMeta.device_name << ", "
       << "Format: " << magic_enum::enum_name(streamMeta.format) << ", "
       << "FPS: " << streamMeta.fps;

    if (streamMeta.id.type == StreamType::DEPTH ||
        streamMeta.id.type == StreamType::COLOR) {
        os << "\n\tIntrinsics: " << streamMeta.intrinsics;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const StreamId& streamId) {
    os << "(Serial: " << streamId.serial_number << ", "
       << "Index: " << streamId.index << ", "
       << "Type: " << magic_enum::enum_name(streamId.type) << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Intrinsics& c) {
    os << "[ width: " << c.width << ", "
       << "height: " << c.height << ", "
       << "ppx: " << c.ppx << ", "
       << "ppy: " << c.ppy << ", "
       << "fx: " << c.fx << ", "
       << "fy: " << c.fy << " ]";
    return os;
}
}  // namespace realsense_streaming
}  // namespace axby
