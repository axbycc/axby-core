#include "pubsub.h"

#include <mutex>
#include <optional>
#include <thread>
#include <zmq.hpp>

#include "absl/strings/match.h"
#include "app/process_id.h"
#include "app/pubsub_recorder.h"
#include "app/timing.h"
#include "concurrency/ring_buffer.h"
#include "concurrency/single_item.h"
#include "debug/check.h"
#include "debug/log.h"
#include "stop_all.h"

namespace axby {
namespace pubsub {

constexpr bool debug_subscriber = false;
constexpr bool debug_publisher = false;

std::mutex recorder_mutex_;
std::mutex recorder_buffer_mutex_;
SubscriberBuffer recorder_buffer_;
std::optional<Recorder> recorder_;
std::atomic<bool> is_recording_{false};

std::optional<zmq::context_t> zmq_ctx_;
void ensure_ctx_initted() {
    if (!zmq_ctx_) {
        zmq_ctx_.emplace(/*num_threads=*/4);
    }
}

struct PublisherRequest {
    // if topic is nonempty, publish thread will issue publisher_socket_.send();
    std::string topic;
    uint16_t message_version = 0;
    uint16_t flags = 0;
    MessageFrames frames;

    // if bind_address is nonempty, then publish thread will issue publisher_socket_.bind();
    std::string bind_address;
};
std::mutex publish_requests_mutex_;
RingBuffer<PublisherRequest, 20> publisher_requests_;
std::thread publisher_thread_;
void run_publisher_thread() {
    CHECK(zmq_ctx_);

    try {
        zmq::socket_t publisher_socket{*zmq_ctx_, zmq::socket_type::pub};

        uint64_t sequence_id = 0;
        while (!should_stop_all()) {
            PublisherRequest request;
            if (!publisher_requests_.move_read(request, /*blocking=*/true)) {
                // publish queue was stopped
                return;
            }

            if (!request.bind_address.empty()) {
                CHECK(request.topic.empty())
                    << "bind address mutually exclusive with topic";
                LOG_IF(INFO, debug_publisher)
                    << "Publisher socket binding " << request.bind_address;
                publisher_socket.bind(request.bind_address);
            }

            if (!request.topic.empty()) {
                // send topic
                LOG_IF(INFO, debug_publisher)
                    << "Publishing on topic " << request.topic;

                // send header
                MessageHeader header{
                    .sender_process_id = get_process_id(),
                    .sender_sequence_id = sequence_id++,
                    .sender_process_time_us = get_process_time_us(),
                    .protocol_version = 0,
                    .message_version = request.message_version,
                    .flags = request.flags};

                publisher_socket.send(
                    zmq::message_t(request.topic),
                    zmq::send_flags::dontwait | zmq::send_flags::sndmore);

                publisher_socket.send(
                    zmq::const_buffer(&header, sizeof(header)),
                    zmq::send_flags::dontwait | zmq::send_flags::sndmore);

                // send other frames
                for (int i = 0; i < request.frames.frames.size(); ++i) {
                    zmq::send_flags send_flags = zmq::send_flags::dontwait;
                    if (i + 1 != request.frames.frames.size()) {
                        send_flags = send_flags | zmq::send_flags::sndmore;
                    }
                    zmq::message_t frame_copy;
                    frame_copy.copy(request.frames.frames[i]);
                    publisher_socket.send(std::move(frame_copy),
                                          send_flags);
                }

                if (is_recording_) {
                    Message message;
                    message.header = header;
                    message.topic = request.topic;
                    for (auto& frame : request.frames.frames) {
                        zmq::message_t frame_copy;
                        frame_copy.copy(frame);
                        message.frames.push_back(std::move(frame_copy));
                    }

                    std::lock_guard<std::mutex> lock { recorder_buffer_mutex_};
                    if (!recorder_buffer_.move_write(std::move(message))) {
                        LOG(WARNING) << "recorder buffer is full";
                    }
                }

            }
        }
    } catch (const zmq::error_t& e) {
        return;
    }
}

struct SubscriberRequest {
    // note we must use optional for the subscribe topic type, since
    // the empty string is a valid subscription.
    std::optional<std::string> subscribe_topic;

    std::string connect_address;

    // if subscribe_topic is nonempty, subscribe_buffer or subscribe
    // item should point to a valid buffer
    SubscriberBuffer* subscribe_buffer = nullptr;
    SubscriberItem* subscribe_item = nullptr;
};

RingBuffer<SubscriberRequest, 20> subscriber_requests_;
std::mutex subscriber_requests_mutex_;
std::thread subscriber_thread_;
struct SubscriberOutput {
    SubscriberBuffer* buffer = nullptr;
    SubscriberItem* item = nullptr;
};

std::thread recorder_thread_;
void run_recorder_thread() {
    FrequencyCalculator bytes_per_sec_calculator;
    while (!should_stop_all()) {
        Message message;
        if (!recorder_buffer_.move_read(message, /*blocking=*/true)) break;

        {
            std::lock_guard<std::mutex> lock{recorder_mutex_};
            if (!recorder_) {
                // the recorder has not been initialized yet. drop this message
                continue;
            } else {
                recorder_->append(message);
            }
        }

        for (const auto& frame : message.frames) {
            bytes_per_sec_calculator.count(frame.size());
        }
        const double bytes_per_sec = bytes_per_sec_calculator.get_frequency();
        const double mb_per_sec = bytes_per_sec / 1e6;
        const double gb_per_min = 60 * mb_per_sec / 1e3;
        LOG_EVERY_T(INFO, 5) << "Recording at " << mb_per_sec << "MB/s, "
                             << gb_per_min << "GB/minute";
    }

    // flush
}

void run_subscriber_thread() {
    try {
        std::vector<std::pair<std::string, SubscriberOutput>>
            subscriber_outputs;

        zmq::socket_t subscriber_socket{*zmq_ctx_, zmq::socket_type::sub};
        subscriber_socket.set(zmq::sockopt::rcvtimeo, 1000);

        while (!should_stop_all()) {
            SubscriberRequest request;
            if (subscriber_requests_.move_read(request,
                                               /*blocking=*/false)) {
                if (request.subscribe_topic) {
                    LOG_IF(INFO, debug_subscriber)
                        << "Subscribing to topic \"" << *request.subscribe_topic
                        << "\"";
                    subscriber_socket.set(zmq::sockopt::subscribe,
                                          *request.subscribe_topic);

                    SubscriberOutput subscriber_output{
                        .buffer = request.subscribe_buffer,
                        .item = request.subscribe_item};

                    subscriber_outputs.push_back(
                        {std::move(*request.subscribe_topic),
                         subscriber_output});
                }

                if (!request.connect_address.empty()) {
                    LOG_IF(INFO, debug_subscriber)
                        << "Subscriber socket connecting to "
                        << request.connect_address;
                    subscriber_socket.connect(request.connect_address);
                }
            }

            zmq::message_t topic_message;
            if (subscriber_socket.recv(topic_message)) {
                // read out the rest of the message.
                // expect the first message is a topic.
                topic_message.to_string_view();
                LOG_IF(INFO, debug_subscriber)
                    << "Received message on topic \""
                    << topic_message.to_string_view() << "\"";
                CHECK(topic_message.more())
                    << "message on " << topic_message.to_string_view()
                    << "missing header";

                zmq::message_t header_message;
                CHECK(subscriber_socket.recv(header_message));

                std::vector<zmq::message_t> frames;
                if (header_message.more()) {
                    bool have_next_frame = false;
                    zmq::message_t frame;
                    do {
                        CHECK(subscriber_socket.recv(frame));
                        have_next_frame = frame.more();
                        frames.push_back(std::move(frame));
                    } while (have_next_frame);
                }

                std::string_view topic = topic_message.to_string_view();

                // todo: if we ever create MessageHeaderV2, replace this check with CHECK_GT.
                // make sure MessageHeaderV2 has MessageHeader fields as a prefix
                CHECK_EQ(header_message.size(), sizeof(MessageHeader));
                MessageHeader header;
                std::memcpy(/*dst=*/&header, /*src=*/header_message.data(),
                            sizeof(MessageHeader));
                // todo: for MessgeHeaderV2, copy additional bytes of the header
                // maybe use std::variant

                auto MakeMessage = [&]() {
                    Message message;
                    message.header = header;
                    message.topic = topic;
                    for (auto& frame : frames) {
                        zmq::message_t frame_copy;
                        frame_copy.copy(frame);
                        message.frames.push_back(std::move(frame_copy));
                    }
                    return message;
                };

                // route the message to the correct output buffers by topic prefix
                for (auto& [topic_prefix, output] : subscriber_outputs) {
                    if (topic.starts_with(topic_prefix)) {
                        if (output.buffer) {
                            if (!output.buffer->move_write(MakeMessage())) {
                                // this subscriber buffer is full. log warning?
                                LOG(WARNING) << "subscriber buffer for topic "
                                             << topic << " is full";
                            }
                        }
                        if (output.item) {
                            output.item->move_write(MakeMessage());
                        }
                    }
                }

                if (is_recording_) {
                    std::lock_guard<std::mutex> lock { recorder_buffer_mutex_};
                    if (!recorder_buffer_.move_write(MakeMessage())) {
                        LOG(WARNING) << "recorder buffer is full";
                    };
                }
            }
        }
    } catch (const zmq::error_t& e) {
        // zmq context shutting down
        return;
    }
}

void enable_recording(std::string_view recording_dir,
                      std::string_view recording_filename) {
    std::lock_guard<std::mutex> lock{recorder_mutex_};
    recorder_.emplace(recording_dir, recording_filename);
    is_recording_ = true;
}

void disable_recording() {
    std::lock_guard<std::mutex> lock{recorder_mutex_};
    recorder_ = std::nullopt;
    is_recording_ = false;
}

void init() {
    ensure_ctx_initted();
    if (!publisher_thread_.joinable()) {
        publisher_thread_ = std::thread{run_publisher_thread};
    }
    if (!subscriber_thread_.joinable()) {
        subscriber_thread_ = std::thread{run_subscriber_thread};
    }
    if (!recorder_thread_.joinable()) {
        recorder_thread_ = std::thread{run_recorder_thread};
    }

    // add default socket connection for in-processes
    bind("inproc://pubsub");
    connect("inproc://pubsub");
}

void bind(std::string_view connection_string) {
    CHECK(publisher_thread_.joinable()) << "you forgot to init";

    PublisherRequest request;
    request.bind_address = connection_string;

    // publish requests must be locked on the writer side, since we may
    // have writers from multiple threads
    std::lock_guard<std::mutex> lock{publish_requests_mutex_};
    CHECK(publisher_requests_.move_write(std::move(request)))
        << "publish queue was full";
}

void publish_topic_only(std::string_view topic) {
    MessageFrames empty;
    publish_frames(topic, 0, std::move(empty));
}

void publish_frames(std::string_view topic,
                    uint16_t message_version,
                    MessageFrames&& frames,
                    uint16_t flags) {
    CHECK(publisher_thread_.joinable()) << "you forgot to init";

    PublisherRequest request;
    request.topic = topic;
    request.message_version = message_version;
    request.flags = flags;
    request.frames = std::move(frames);

    // publish queue must be locked on the writer side, since we may
    // have writers from multiple threads
    std::lock_guard<std::mutex> lock{publish_requests_mutex_};
    CHECK(publisher_requests_.move_write(std::move(request)))
        << "publish queue was full";
}

void connect(std::string_view connection) {
    CHECK(subscriber_thread_.joinable()) << "you forgot to init";

    SubscriberRequest request;
    request.connect_address = connection;

    // subscriber queue must be locked on the writer side, since we may
    // have writers from multiple threads
    std::lock_guard<std::mutex> lock{subscriber_requests_mutex_};
    subscriber_requests_.move_write(std::move(request));
}

void subscribe(std::string_view topic, SubscriberBuffer* buffer) {
    CHECK(subscriber_thread_.joinable()) << "you forgot to init";

    SubscriberRequest request;
    request.subscribe_topic = topic;
    request.subscribe_buffer = buffer;

    // subscriber queue must be locked on the writer side, since we may
    // have writers from multiple threads
    std::lock_guard<std::mutex> lock{subscriber_requests_mutex_};
    subscriber_requests_.move_write(std::move(request));
}

void subscribe_latest(std::string_view topic, SubscriberItem* item) {
    CHECK(subscriber_thread_.joinable()) << "you forgot to init";

    SubscriberRequest request;
    request.subscribe_topic = topic;
    request.subscribe_item = item;

    // subscriber queue must be locked on the writer side, since we may
    // have writers from multiple threads
    std::lock_guard<std::mutex> lock{subscriber_requests_mutex_};
    subscriber_requests_.move_write(std::move(request));
}

void cleanup() {
    stop_all();

    CHECK(zmq_ctx_) << "cleanup without init";
    zmq_ctx_->shutdown();  // unblock sockets

    publisher_requests_.stop();
    publisher_thread_.join();

    subscriber_requests_.stop();
    subscriber_thread_.join();

    recorder_buffer_.stop();
    recorder_thread_.join();
}

}  // namespace pubsub
}  // namespace axby
