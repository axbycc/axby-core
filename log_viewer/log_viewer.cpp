#include <imgui.h>
#include <imoverlayable.h>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "app/flag.h"
#include "app/gui.h"
#include "app/main.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "concurrency/ring_buffer.h"
#include "concurrency/single_item.h"
#include "debug/check.h"
#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "network_config/config.h"
#include "realsense_streaming/client.h"
#include "realsense_streaming/pointcloud_job.h"
#include "seekbar.h"
#include "serialization/make_serializable.hpp"
#include "serialization/serialization.h"
#include "time_sync/time_sync.h"
#include "viewer/viewer.h"
#include "wrappers/duckdb.h"
#include "wrappers/imgui.h"

APP_FLAG(std::string, log_path, "", "path to log file");

using namespace axby;
namespace rss = realsense_streaming;

void open_log(const char* log_path, duckdb_database& db, bool read_only) {
    duckdb_config db_config;
    duckdb_create_config(&db_config);
    if (read_only) {
        duckdb_set_config(db_config, "access_mode", "READ_ONLY");
    }
    duckdb_set_config(db_config, "memory_limit", "500MB");
    duckdb_set_config(db_config, "threads", "1");
    char* open_error = nullptr;
    if (duckdb_open_ext(log_path, &db, db_config, &open_error) !=
        DuckDBSuccess) {
        LOG(FATAL) << open_error;
    };
    duckdb_free(open_error);
    duckdb_destroy_config(&db_config);
}

std::pair<uint64_t, uint64_t> get_time_bounds_us(duckdb_connection con) {
    const char* sql = R"SQL_(
select min(this_process_time_us), max(this_process_time_us) from log
)SQL_";

    DuckDbPreparedStatement prepared_statement(con, sql);
    prepared_statement.execute();

    auto& result = prepared_statement.result();
    while (result.fetch_chunk()) {
        idx_t num_rows = result.get_num_rows();
        CHECK_EQ(num_rows, 1);
        auto min_time_us = result.get_column<uint64_t>(0).row(0);
        auto max_time_us = result.get_column<uint64_t>(1).row(0);
        return {min_time_us, max_time_us};
    }

    LOG(FATAL) << "No records in log";
    return {0, 0};
}

std::pair<uint64_t, uint64_t> get_topic_time_bounds_us(duckdb_connection con,
                                                       std::string_view topic) {
    const char* sql = R"SQL_(
select min(this_process_time_us), max(this_process_time_us) from log
where topic = $topic
)SQL_";

    DuckDbPreparedStatement prepared_statement(con, sql);
    prepared_statement.bind_param_string("topic", topic);
    prepared_statement.execute();

    auto& result = prepared_statement.result();
    while (result.fetch_chunk()) {
        idx_t num_rows = result.get_num_rows();
        CHECK_EQ(num_rows, 1);
        auto min_time_us = result.get_column<uint64_t>(0).row(0);
        auto max_time_us = result.get_column<uint64_t>(1).row(0);
        return {min_time_us, max_time_us};
    }

    LOG(FATAL) << "No records in log";
    return {0, 0};
}

std::vector<std::string> get_topics_starting_with(
    duckdb_connection con, std::string_view topic_prefix) {
    const char* sql = R"SQL_(
select distinct topic from log
where topic like $topic_prefix || '%'
order by topic asc
)SQL_";

    DuckDbPreparedStatement prepared_statement(con, sql);
    prepared_statement.bind_param_string("topic_prefix", topic_prefix);
    prepared_statement.execute();

    std::vector<std::string> topics;

    auto& result = prepared_statement.result();
    while (result.fetch_chunk()) {
        idx_t num_rows = result.get_num_rows();
        auto topics_column = result.get_column<duckdb_string_t>(0);
        for (int row = 0; row < num_rows; ++row) {
            topics.push_back(duckdb_string_to_string(topics_column.row(row)));
        }
    }

    return topics;
}

std::optional<uint64_t> find_keyframe_message_id(duckdb_connection con,
                                                 std::string_view topic,
                                                 uint64_t timestamp_us) {
    const char* video_packet_get_closest_keyframe_sql = R"SQL_(
select message_id from log
where
    (topic = $topic) and
    (this_process_time_us <= $time_us) and
    ($time_us + 10000000 >= this_process_time_us) and -- limit the search to recent packets
    (flags = 1) -- we use this condition to mark keyframes
order by message_id desc limit 1
)SQL_";

    DuckDbPreparedStatement prepared_statement(
        con, video_packet_get_closest_keyframe_sql);
    prepared_statement.bind_param_string("topic", topic);
    prepared_statement.bind_param_uint64("time_us", timestamp_us);
    prepared_statement.execute();

    auto& result = prepared_statement.result();
    result.fetch_chunk();
    if (!result.is_done()) {
        uint64_t keyframe_message_id = result.get_column<uint64_t>(0).row(0);
        return keyframe_message_id;
    }
    return std::nullopt;
}

std::atomic<uint64_t> _frame_load_timestamp_ms{0};
std::atomic<uint64_t> _playback_timestamp_ms{0};

struct VideoPacket {
    uint64_t timestamp_ms = 0;
    uint64_t message_id = 0;
    rss::StreamMeta stream_meta;
    FastResizableVector<std::byte> data;
};

using VideoPacketBuffer = RingBuffer<VideoPacket, 256>;

FastResizableVector<std::span<const std::byte>> unpack_frames(
    std::string_view packed_frames) {
    FastResizableVector<std::span<const std::byte>> frames;
    serialization::deserialize_cbor(frames, packed_frames);
    return frames;
}

FastResizableVector<std::span<const std::byte>> unpack_frames(
    const duckdb_string_t& s) {
    return unpack_frames(duckdb_string_to_string_view(s));
}

std::vector<std::pair<std::string, uint64_t>> find_keyframe_message_ids(
    duckdb_connection con, uint64_t before_time_us) {
    const char* find_realsense_keyframes_sql = R"SQL_(
SELECT 
    topic,
    MAX(message_id)
FROM log
WHERE flags = 1 -- this flag is used to indicate keyframe
    AND (topic LIKE 'realsense/depth/%' OR topic LIKE 'realsense/color/%')
    AND this_process_time_us + 2e6 > $time_us
    AND this_process_time_us < $time_us
GROUP BY topic
)SQL_";

    DuckDbPreparedStatement prepared_statement(con,
                                               find_realsense_keyframes_sql);
    prepared_statement.bind_param_uint64("time_us", before_time_us * 1e3);
    prepared_statement.execute();

    std::vector<std::pair<std::string, uint64_t>> keyframe_message_ids;

    auto& result = prepared_statement.result();
    while (result.fetch_chunk()) {
        auto topics = result.get_column<duckdb_string_t>(0);
        auto message_ids = result.get_column<uint64_t>(1);
        for (int row = 0; row < result.get_num_rows(); ++row) {
            auto topic = duckdb_string_to_string_view(topics.row(row));
            auto message_id = message_ids.row(row);
            keyframe_message_ids.push_back({std::string(topic), message_id});
        }
    }

    return keyframe_message_ids;
}

void run_playback_thread(std::string log_path) {
    duckdb_database db;
    open_log(log_path.c_str(), db, /*read_only=*/true);

    const auto PublishResults = [](DuckDbResult& result) {
        while (result.fetch_chunk()) {
            auto sender_process_ids = result.get_column<uint64_t>(0);
            auto sender_sequence_ids = result.get_column<uint64_t>(1);
            auto sender_process_times_us = result.get_column<uint16_t>(2);
            auto message_versions = result.get_column<uint16_t>(3);
            auto flagss = result.get_column<uint16_t>(4);
            auto framess = result.get_column<duckdb_string_t>(6);
            auto topics = result.get_column<duckdb_string_t>(7);

            for (int row = 0; row < result.get_num_rows(); ++row) {
                const auto unpacked_frames = unpack_frames(framess.row(row));
                pubsub::MessageHeader header{
                    .sender_process_id = sender_process_ids.row(row),
                    .sender_sequence_id = sender_sequence_ids.row(row),
                    .sender_process_time_us = sender_process_times_us.row(row),
                    .message_version = message_versions.row(row),
                    .flags = flagss.row(row)};
                pubsub::MessageFrames frames;
                for (const auto& frame : unpacked_frames) {
                    frames.add_bytes(frame);
                }
                std::string_view topic =
                    duckdb_string_to_string_view(topics.row(row));
                pubsub::publish_frames_with_manual_header(topic, header,
                                                          std::move(frames));

                // // DEBUG
                // if (absl::StrContains(topic, "color/246322304238")) {
                //     auto seq_id = seq_bit_cast<uint64_t>(unpacked_frames[1]);
                //     LOG(INFO) << "Publishing " << topic << ": "
                //               << header.sender_sequence_id << " / " << seq_id;
                // }
            }
        }
    };

    uint64_t playback_timestamp_ms = 0;
    while (!should_stop_all()) {
        uint64_t new_playback_timestamp_ms = _playback_timestamp_ms;
        if (new_playback_timestamp_ms == playback_timestamp_ms) {
            // playback is paused. wait until playback starts again.
            sleep_ms(100);
        }

        bool need_initialize = false;
        if (new_playback_timestamp_ms < playback_timestamp_ms) {
            // jump backwards in time
            need_initialize = true;
        }
        if (new_playback_timestamp_ms > playback_timestamp_ms + 1000) {
            // large jump forward in time
            need_initialize = true;
        }
        if (need_initialize) {
            LOG(INFO) << "Running initialize";
            pubsub::publisher_requests_clear();
            // need to do a backward-search for keyframes
            const std::vector<std::pair<std::string, uint64_t>>
                keyframe_message_ids = find_keyframe_message_ids(
                    DuckDbConnection(db), new_playback_timestamp_ms);

            // for each keyframe, publish segment from keyframe up
            // until the current playback time
            for (const auto& [topic, message_id] : keyframe_message_ids) {
                const char* retreive_messages_sql = R"SQL_(
select
sender_process_id, sender_sequence_id, sender_process_time_us, message_version, flags,
this_process_time_us, frames, topic
from log
where message_id >= $message_id and this_process_time_us < $time_us
and topic = $topic
order by this_process_time_us asc
)SQL_";
                DuckDbConnection con(db);
                DuckDbPreparedStatement prepared_statement(
                    con, retreive_messages_sql);
                prepared_statement.bind_param_uint64(
                    "time_us", new_playback_timestamp_ms * 1e3);
                prepared_statement.bind_param_uint64("message_id", message_id);
                prepared_statement.bind_param_string("topic", topic);
                prepared_statement.execute();

                PublishResults(prepared_statement.result());
            }

            playback_timestamp_ms = new_playback_timestamp_ms - 1;
        }

        const char* retreive_messages_sql = R"SQL_(
select
sender_process_id, sender_sequence_id, sender_process_time_us, message_version, flags,
this_process_time_us, frames, topic
from log
where this_process_time_us > $min_time_us and this_process_time_us <= $max_time_us
order by this_process_time_us asc
)SQL_";

        DuckDbConnection con(db);
        DuckDbPreparedStatement prepared_statement(con, retreive_messages_sql);
        prepared_statement.bind_param_uint64("min_time_us",
                                             playback_timestamp_ms * 1e3);
        prepared_statement.bind_param_uint64("max_time_us",
                                             new_playback_timestamp_ms * 1e3);
        prepared_statement.execute();
        PublishResults(prepared_statement.result());

        playback_timestamp_ms = new_playback_timestamp_ms;
    }

    duckdb_close(&db);
}

struct Context {
    std::string log_path;
    duckdb_database db;
    Seekbar seekbar;

    std::set<std::string> realsense_serials;
    absl::flat_hash_map<std::string, rss::client::RealsenseState>
        serial_to_realsense_state;
    std::string selected_serial;

    float point_size = 1.0f;
};

void make_gui(Context& ctx) {
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    // --- Top Pane ---
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, -1));
    ImGui::Begin("TopPane", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

    if (ctx.realsense_serials.empty()) {
        ImGui::Text("Waiting for data.");
    } else {
        make_seekbar(ctx.seekbar);

        if (ctx.selected_serial.empty()) {
            ctx.selected_serial = *ctx.realsense_serials.begin();
        }

        if (ImGui::BeginTable("Display Table", ctx.realsense_serials.size(),
                              ImGuiTableFlags_SizingFixedFit)) {
            for (const auto& serial : ctx.realsense_serials) {
                ImGui::TableNextColumn();
                if (ImGui::RadioButton(serial.c_str(),
                                       (ctx.selected_serial == serial))) {
                    ctx.selected_serial = serial;
                };

                std::string color_viewer_key =
                    absl::StrFormat("color_%s", serial);
                if (viewer::have_image(color_viewer_key)) {
                    const auto image_handle =
                        viewer::get_image(color_viewer_key);
                    ImOverlayable::image((void*)image_handle.texture,
                                         (float)image_handle.width,
                                         (float)image_handle.height, 150);
                }

                std::string depth_viewer_key =
                    absl::StrFormat("depth_%s", serial);
                if (viewer::have_image(depth_viewer_key)) {
                    axby::viewer::set_cmap(depth_viewer_key, "heat", 0.001, 6.0,
                                           /*cmap_scale=*/0.001,
                                           /*cmap_invert=*/true);
                    const auto depth_image_handle =
                        viewer::get_image(depth_viewer_key);
                    ImOverlayable::image((void*)depth_image_handle.texture,
                                         (float)depth_image_handle.width,
                                         (float)depth_image_handle.height, 150);
                }
            }

            ImGui::EndTable();
        }
    }

    auto top_pane_bottom = imgui_get_window_bottom();
    ImGui::End();  // End top pane

    // --- Left Pane ---
    const float left_pane_width = 650.0f;
    ImGui::SetNextWindowPos(ImVec2(0, top_pane_bottom));
    ImGui::SetNextWindowSize(
        ImVec2(left_pane_width, displaySize.y - top_pane_bottom));
    ImGui::Begin("LeftPane", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
    ImGui::SliderFloat("Point Size", &ctx.point_size, 0.1f, 10.0f);
    ImGui::End();

    if (!ctx.selected_serial.empty()) {
        if (ImGui::Begin("Realsense Details")) {
            ImGui::Text("Serial: %s", ctx.selected_serial.c_str());
            const auto& state =
                ctx.serial_to_realsense_state.at(ctx.selected_serial);
            ImGui::Text("Device Name: %s",
                        state.color.stream_meta.device_name.c_str());

            const auto window_width = ImGui::GetWindowWidth();

            std::string color_viewer_key =
                absl::StrFormat("color_%s", ctx.selected_serial);
            if (viewer::have_image(color_viewer_key)) {
                const auto image_handle = viewer::get_image(color_viewer_key);
                ImOverlayable::image(
                    (void*)image_handle.texture, (float)image_handle.width,
                    (float)image_handle.height, 0.9 * window_width);
            }

            std::string depth_viewer_key =
                absl::StrFormat("depth_%s", ctx.selected_serial);
            // draw the depth image
            if (viewer::have_image(depth_viewer_key)) {
                axby::viewer::set_cmap(depth_viewer_key, "heat", 0.001, 6.0,
                                       /*cmap_scale=*/0.001,
                                       /*cmap_invert=*/true);

                const auto depth_image_handle =
                    viewer::get_image(depth_viewer_key);
                ImOverlayable::image((void*)depth_image_handle.texture,
                                     (float)depth_image_handle.width,
                                     (float)depth_image_handle.height,
                                     0.9 * window_width);
            }
            ImGui::End();
        }
    }
}

int main(int argc, char* argv[]) {
    __APP_MAIN_INIT__;

    APP_UNPACK_FLAG(log_path);

    LOG(INFO) << "Opening " << log_path;

    Context ctx;
    ctx.log_path = log_path;
    open_log(log_path.c_str(), ctx.db, /*read_only=*/true);

    {
        DuckDbConnection con(ctx.db);
        auto [min_time_us, max_time_us] = get_time_bounds_us(con);
        LOG(INFO) << "Time bounds " << min_time_us << ", " << max_time_us;
        ctx.seekbar.min_timestamp_ms = min_time_us / 1e3;
        ctx.seekbar.max_timestamp_ms = max_time_us / 1e3;
    }

    // load pubsub
    network_config::Config playback_config{"playback"};
    pubsub::init();
    time_sync::init(playback_config);
    rss::client::init(playback_config);

    {
        DuckDbConnection con(ctx.db);
        std::vector<std::string> realsense_topics =
            get_topics_starting_with(con, "realsense/");
        for (const auto& topic : realsense_topics) {
            std::vector<std::string_view> parts = absl::StrSplit(topic, '/');
            CHECK_GE(parts.size(), 3);  // "realsense/depth/serial/idx"
            std::string_view serial_number = parts[2];
            ctx.realsense_serials.emplace(serial_number);
            ctx.serial_to_realsense_state[serial_number] = {.serial_number =
                                                                serial_number};
        }
    }

    std::thread playback_thread{[=]() { run_playback_thread(log_path); }};

    gui_init("Log Viewer");
    viewer::init();
    viewer::enable_auto_orbit();

    auto thread_pool = std::make_shared<SimpleThreadPool>(/*num_threads=*/1);
    realsense_streaming::PointCloudJob pointcloud_job{thread_pool};
    FastResizableVector<float> pointcloud_xyzs;
    FastResizableVector<uint8_t> pointcloud_rgbs;

    while (!gui_wants_quit()) {
        gui_loop_begin();
        viewer::new_frame(ImGui::GetIO());

        handle_playback_control(ctx.seekbar);
        update_playing(ctx.seekbar);

        _frame_load_timestamp_ms = ctx.seekbar.current_timestamp_ms;
        _playback_timestamp_ms = ctx.seekbar.current_timestamp_ms;

        for (auto& [serial, state] : ctx.serial_to_realsense_state) {
            const auto did_update = rss::client::update_realsense_state(state);
            if (did_update.color) {
                viewer::update_image(absl::StrFormat("color_%s", serial),
                                     state.color.stream_meta.intrinsics.width,
                                     state.color.stream_meta.intrinsics.height,
                                     state.color.data);
            }
            if (did_update.depth) {
                viewer::update_image(absl::StrFormat("depth_%s", serial),
                                     state.depth.stream_meta.intrinsics.width,
                                     state.depth.stream_meta.intrinsics.height,
                                     state.depth.data);
            }

            if ((did_update.depth || did_update.color) &&
                (serial == ctx.selected_serial)) {
                if (pointcloud_job.is_none() &&
                    (!state.color.data.empty() && !state.depth.data.empty())) {
                    pointcloud_job.start(state.color, state.depth);
                }
                if (pointcloud_job.is_complete()) {
                    pointcloud_job.read_results(pointcloud_xyzs,
                                                pointcloud_rgbs);
                    viewer::update_points("pointcloud", pointcloud_xyzs);
                    viewer::update_point_colors("pointcloud", pointcloud_rgbs);
                }
            }
        }

        if (pointcloud_job.is_none() && !ctx.selected_serial.empty()) {
            auto& state = ctx.serial_to_realsense_state.at(ctx.selected_serial);
            const bool have_data =
                !state.color.data.empty() && !state.depth.data.empty();
            const bool have_new_device =
                pointcloud_job.last_serial_number() != ctx.selected_serial;
            const bool have_new_depth =
                pointcloud_job.last_depth_sequence_id() !=
                state.depth.sequence_id;
            const bool have_new_color =
                pointcloud_job.last_color_sequence_id() !=
                state.color.sequence_id;
            if (have_data &&
                (have_new_device || (have_new_depth && have_new_color))) {
                pointcloud_job.start(state.color, state.depth);
            }
        }

        if (pointcloud_job.is_complete()) {
            pointcloud_job.read_results(pointcloud_xyzs, pointcloud_rgbs);
            viewer::update_points("pointcloud", pointcloud_xyzs);
            viewer::update_point_colors("pointcloud", pointcloud_rgbs);
        }

        make_gui(ctx);

        if (viewer::have_points("pointcloud")) {
            viewer::draw_points("pointcloud", ctx.point_size);
        }

        gui_loop_end();
    }
    gui_cleanup();

    stop_all();

    playback_thread.join();

    rss::client::cleanup();
    time_sync::cleanup();
    pubsub::cleanup();
    // for (auto& thread : frame_loading_threads) {
    //     if (thread.joinable()) thread.join();
    // }

    duckdb_close(&ctx.db);

    return 0;
}
