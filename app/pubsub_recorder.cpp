#include "pubsub_recorder.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#include "app/create_log_table_sql.h"
#include "app/files.h"
#include "app/timing.h"
#include "debug/check.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "process_id.h"
#include "wrappers/duckdb.h"

namespace axby {
namespace pubsub {

std::string generate_log_name() {
    static std::mutex mtx;
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm;
    {
        std::lock_guard<std::mutex> lock(mtx);
        tm = *std::localtime(&t);
    }
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".duckdb";
    return oss.str();
}

Recorder::Recorder(std::string_view log_dir, std::string_view log_name) {
    std::filesystem::path actual_log_dir =
        log_dir.empty() ? get_home_path() : log_dir;
    std::string actual_log_name =
        log_name.empty() ? generate_log_name() : std::string(log_name);
    std::filesystem::create_directories(actual_log_dir);
    std::filesystem::path final_path = actual_log_dir / actual_log_name;

    ctx_.init(std::string(final_path));
    CHECK(duckdb_query(ctx_.conn_, create_log_table_sql, nullptr) !=
          DuckDBError);

    {
        const char* metadata_entry_sql = R"SQL_(
insert into metadata
values ($this_process_id, $process_time_us, $unix_time_ms)
)SQL_";
        DuckDbPreparedStatement metadata_statement(ctx_.conn_,
                                                   metadata_entry_sql);
        metadata_statement.bind_param_uint64("this_process_id",
                                             get_process_id());
        metadata_statement.bind_param_uint64("process_time_us",
                                             get_process_time_us());
        metadata_statement.bind_param_uint64("unix_time_ms",
                                             get_system_time_ms());
        metadata_statement.execute();
    }

    CHECK(duckdb_appender_create(ctx_.conn_, nullptr, "log", &appender_) !=
          DuckDBError);
}

void Recorder::append(const pubsub::Message& message) {
    frame_spans_.clear();
    serialization_buf_.clear();

    // topic
    duckdb_append_varchar_length(appender_, message.topic.data(),
                                 message.topic.size());
    check_duckdb_appender_error(appender_);

    // header
    duckdb_append_uint64(appender_, message.header.sender_process_id);
    check_duckdb_appender_error(appender_);
    duckdb_append_uint64(appender_, message.header.sender_sequence_id);
    check_duckdb_appender_error(appender_);
    duckdb_append_uint64(appender_, message.header.sender_process_time_us);
    check_duckdb_appender_error(appender_);
    duckdb_append_uint16(appender_, message.header.protocol_version);
    check_duckdb_appender_error(appender_);
    duckdb_append_uint16(appender_, message.header.message_version);
    check_duckdb_appender_error(appender_);
    duckdb_append_uint16(appender_, message.header.flags);
    check_duckdb_appender_error(appender_);

    duckdb_append_uint64(appender_, get_process_time_us());
    check_duckdb_appender_error(appender_);
    duckdb_append_uint64(appender_, message_id_++);
    check_duckdb_appender_error(appender_);

    // frames
    for (auto& frame : message.frames) {
        frame_spans_.push_back(reinterpret_span<const std::byte>(frame));
    }
    CHECK(serialization::serialize_cbor(frame_spans_, serialization_buf_));
    duckdb_append_blob(appender_, (void*)serialization_buf_.data(),
                       serialization_buf_.size());
    check_duckdb_appender_error(appender_);
    duckdb_appender_end_row(appender_);
}

Recorder::~Recorder() { duckdb_appender_destroy(&appender_); }

}  // namespace pubsub
}  // namespace axby
