#pragma once

#include <span>
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "app/pubsub_message.h"
#include "wrappers/duckdb.h"

namespace axby {
namespace pubsub {

class Recorder {
public:
    Recorder(std::string_view log_dir, std::string_view log_name);
    void append(const pubsub::Message& message);
    ~Recorder();

private:
    DuckDbContext ctx_;
    duckdb_appender appender_;
    uint64_t message_id_ = 0;
    std::vector<std::span<const std::byte>> frame_spans_;
    FastResizableVector<std::byte> serialization_buf_;
};

}
}
