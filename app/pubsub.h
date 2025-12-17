#pragma once

#include <string_view>
#include <vector>
#include <zmq.hpp>

#include "app/pubsub_message.h"
#include "concurrency/ring_buffer.h"
#include "concurrency/single_item.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "seq/seq.h"
#include "serialization/make_serializable.hpp"
#include "serialization/serialization.h"

namespace axby {
namespace pubsub {

struct MessageFrames {
    /* this convenience class builds multi-part zmq messages.
       otherwise we would have to do something like:

       zmq::message_t part1(part1_arguments);
       zmq::message_t part2(part2_arguments);
       ...
       frames.push_back(std::move(part1));
       frames.push_back(std::move(part2));
       ...

       and also the constructor for zmq::message_t does not bind to as
       many arguments as Seq. so we would sometimes have to call

       zmq::message_t partx((void*)my_thing.data(), my_thing.size());
       
     */

    template <typename T>
        requires(std::is_trivially_copyable_v<T>)
    void add_simple(const T& t) {
        frames.emplace_back(&t, sizeof(T));
    }

    template <typename T>
    void add_cbor(const T& t) {
        auto* serialized = new FastResizableVector<std::byte>;
        const auto Deleter = [](void* data, void* hint) {
            delete static_cast<FastResizableVector<std::byte>*>(hint);
        };
        serialization::serialize_cbor(t, *serialized);
        zmq::message_t msg{serialized->data(), serialized->size(), Deleter,
                           serialized};
        add_message(std::move(msg));
    }

    template <size_t N, typename T>
    void add_array(T&& ts) {
        add_simple(seq_to_array<N>(ts));
    }

    void add_bytes(Seq<const int8_t> bytes) {
        frames.emplace_back(bytes.as_bytes());
    }
    void add_bytes(Seq<const uint8_t> bytes) {
        frames.emplace_back(bytes.as_bytes());
    }
    void add_bytes(Seq<const std::byte> bytes) {
        frames.emplace_back(bytes.as_bytes());
    }
    void add_bytes(Seq<const char> bytes) {
        frames.emplace_back(bytes.as_bytes());
    }
    void add_message(zmq::message_t&& msg) {
        frames.emplace_back(std::move(msg));
    }
    size_t size() const {
        size_t result = 0;
        for (const auto& frame : frames) {
            result += frame.size();
        }
        return result;
    }
    std::vector<zmq::message_t> frames;
};

// call from main thread
void init();
void cleanup();

// publisher side, thread safe
void bind(std::string_view connection_string);

void publish_frames(std::string_view topic,
                    uint16_t message_version,
                    MessageFrames&& frames,
                    uint16_t flags = 0);

void publish_topic_only(std::string_view topic);

template <typename T>
void publish_simple(std::string_view topic,
                    uint16_t message_version,
                    const T& object,
                    uint16_t flags = 0) {
    MessageFrames frames;
    frames.add_simple(object);
    publish_frames(topic, message_version, std::move(frames), flags);
};

template <typename T>
void publish_cbor(std::string_view topic,
                  uint16_t message_version,
                  const T& object,
                  uint16_t flags = 0) {
    MessageFrames frames;
    frames.add_cbor(object);
    publish_frames(topic, message_version, std::move(frames), flags);
};

// subscriber side, thread safe
using SubscriberBuffer = RingBuffer<Message, 120>;
using SubscriberItem = SingleItem<Message>;

void enable_recording(std::string_view log_dir = "",
                      std::string_view log_name = "");
void disable_recording();
void connect(std::string_view connection_string);
void subscribe(std::string_view topic, SubscriberBuffer* subscriber_buffer);
void subscribe_latest(std::string_view topic, SubscriberItem* subscriber_item);

}  // namespace pubsub
}  // namespace axby
