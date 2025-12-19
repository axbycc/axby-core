#include "messages.h"

namespace axby {
namespace realsense_streaming {
namespace client {

struct DepthData {
    std::string topic;
    uint64_t process_id = 0;
    uint64_t creation_timestamp_us = 0;

    uint64_t sequence_id = 0;
    StreamMeta stream_meta;
    FastResizableVector<uint16_t> data;
};

struct ColorData {
    std::string topic;
    uint64_t process_id = 0;
    uint64_t creation_timestamp_us = 0;    

    uint64_t sequence_id = 0;
    StreamMeta stream_meta;
    FastResizableVector<uint8_t> data;
};

struct MotionData {
    std::string topic;
    uint64_t process_id = 0;

    uint64_t sequence_id = 0;
    StreamMeta stream_meta;
    FastResizableVector<uint64_t> timestamps_us;
    FastResizableVector<float> xyzs;
};

struct RealsenseState {
    SerialNumber serial_number;
    DepthData depth;
    ColorData color;
    MotionData accel;
    MotionData gyro;
};

}
}
}
