#pragma once

#include <vector>
#include <string>
#include "debug/check.h"
#include "serialization/serialization.h"
#include "serialization/make_serializable.hpp"
#include <zmq.hpp>

extern "C" {
struct AXBY_PUBSUB_MessageHeader {
    uint64_t sender_process_id = 0;
    uint64_t sender_sequence_id = 0;
    uint64_t sender_process_time_us = 0;  // overflow at 584 thousand years
    uint16_t protocol_version = 0;
    uint16_t message_version = 0;
    uint16_t flags = 0;
};
}

namespace axby {
namespace pubsub {

using MessageHeader = AXBY_PUBSUB_MessageHeader;
struct Message {
    std::string topic;
    MessageHeader header;
    std::vector<zmq::message_t> frames;

    template <typename T>
        requires(std::is_trivially_copyable_v<T>)
    T get_simple(size_t frame_idx) {
        CHECK_LT(frame_idx, frames.size());
        CHECK_EQ(frames[frame_idx].size(), sizeof(T));
        return seq_bit_cast<T>(frames[frame_idx]);
    }

    template <typename T, size_t N>
    std::array<T, N> get_array(size_t frame_idx) {
        return get_simple<std::array<T, N>>(frame_idx);
    }

    template <typename T>
    T get_cbor(size_t frame_idx) {
        CHECK_LT(frame_idx, frames.size());

        T t;
        serialization::deserialize_cbor(t, frames[frame_idx]);
        return t;
    }
};

}
}
