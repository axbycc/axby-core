#include "app/flag.h"
#include "app/main.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "debug/log.h"

APP_FLAG(std::string, mode, "pub", "pub or sub");

int main(int argc, char *argv[]) {
    using namespace axby;
    __APP_MAIN_INIT__;

    APP_UNPACK_FLAG(mode);
    pubsub::init();

    pubsub::SubscriberBuffer subscriber_buffer;
    on_stop_all([&]() { subscriber_buffer.stop(); });

    if (mode == "pub") {
        LOG(INFO) << "Starting publisher";
        pubsub::bind("ipc:///tmp/example");
    } else {
        LOG(INFO) << "Starting subscriber";
        pubsub::connect("ipc:///tmp/example");
        pubsub::subscribe("", &subscriber_buffer);
    }

    while (!should_stop_all()) {
        if (mode == "pub") {
            pubsub::MessageFrames frames;
            frames.add_bytes("hello");
            frames.add_bytes(std::array<std::byte, 3>{std::byte{0}, std::byte{1},
                    std::byte{2}});
            pubsub::publish_frames("example_topic", 0, std::move(frames));
            sleep_ms(1000);
        }
        if (mode == "sub") {
            pubsub::Message message;
            if (subscriber_buffer.move_read(message, /*blocking=*/true)) {
                LOG(INFO) << "Got a message.";
                LOG(INFO) << "Version: " << message.header.message_version;
                LOG(INFO) << "Flags: " << message.header.flags;
                LOG(INFO) << "Sequence Id: " << message.header.sender_sequence_id;
                LOG(INFO) << "Process time (us): "
                          << message.header.sender_process_time_us;
                LOG(INFO) << "Frame 0 " << message.frames[0].to_string_view();
            }
        }
    }

    LOG(INFO) << "Cleaning up";
    pubsub::cleanup();

    return 0;
}
