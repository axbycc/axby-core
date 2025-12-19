#include "viewer/viewer.h"

#include <imgui.h>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_format.h"
#include "app/flag.h"
#include "app/gui.h"
#include "app/main.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "network_config/config.h"
#include "realsense_streaming/client.h"
#include "realsense_streaming/messages.h"
#include "realsense_streaming/pointcloud_job.h"
#include "simple_thread_pool.h"
#include "third_party/imgui/imgui.h"
#include "third_party/simple_thread_pool/simple_thread_pool.h"
#include "time_sync/time_sync.h"
#include "wrappers/imgui.h"

APP_FLAG(std::string, config_name, "local", "network config name");

using namespace axby;
namespace rss = realsense_streaming;

absl::flat_hash_map<rss::SerialNumber, rss::client::RealsenseState>
    _serial_to_realsense_state;
std::set<std::string> _serials;
std::string _selected_serial;
float _point_size = 1.0f;

void make_gui() {
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    // --- Top Pane ---
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, -1));
    ImGui::Begin("TopPane", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

    if (_serials.empty()) {
        ImGui::Text("Waiting for data.");
    } else {
        if (_selected_serial.empty()) {
            _selected_serial = *_serials.begin();
        }

        static bool recording = false;
        if (!recording) {
            if (ImGui::Button("Start Recording")) {
                recording = true;
                pubsub::enable_recording();
            }
        } else {
            if (ImGui::Button("Stop Recording")) {
                recording = false;
                pubsub::disable_recording();
            }
        }

        ImGui::SliderFloat("Point Size", &_point_size, 0.1f, 10.0f);

        if (ImGui::BeginTable("Display Table", _serials.size(),
                              ImGuiTableFlags_SizingFixedFit)) {
            for (const auto& serial : _serials) {
                ImGui::TableNextColumn();
                if (ImGui::RadioButton(serial.c_str(),
                                       (_selected_serial == serial))) {
                    _selected_serial = serial;
                };
                // ImGui::Text("Serial: %s", serial.c_str());
                // const auto& state = _serial_to_realsense_state.at(serial);
                // ImGui::Text("Device Name: %s",
                //             state.color.stream_meta.device_name.c_str());

                std::string color_viewer_key =
                    absl::StrFormat("color_%s", serial);
                if (viewer::have_image(color_viewer_key)) {
                    const auto image_handle =
                        viewer::get_image(color_viewer_key);
                    imgui_image((void*)image_handle.texture,
                                (float)image_handle.width,
                                (float)image_handle.height, 150);
                }

                std::string depth_viewer_key =
                    absl::StrFormat("depth_%s", serial);
                // draw the depth image
                if (viewer::have_image(depth_viewer_key)) {
                    axby::viewer::set_cmap(depth_viewer_key, "heat", 0.001, 6.0,
                                           /*cmap_scale=*/0.001,
                                           /*cmap_invert=*/true);

                    const auto depth_image_handle =
                        viewer::get_image(depth_viewer_key);
                    imgui_image((void*)depth_image_handle.texture,
                                (float)depth_image_handle.width,
                                (float)depth_image_handle.height, 150);
                }
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();

    if (!_selected_serial.empty()) {
        if (ImGui::Begin("Detail")) {
            ImGui::Text("Serial: %s", _selected_serial.c_str());
            const auto& state = _serial_to_realsense_state.at(_selected_serial);
            ImGui::Text("Device Name: %s",
                        state.color.stream_meta.device_name.c_str());

            const auto window_width = ImGui::GetWindowWidth();

            std::string color_viewer_key =
                absl::StrFormat("color_%s", _selected_serial);
            if (viewer::have_image(color_viewer_key)) {
                const auto image_handle = viewer::get_image(color_viewer_key);
                imgui_image((void*)image_handle.texture,
                            (float)image_handle.width,
                            (float)image_handle.height, 0.9 * window_width);
            }

            std::string depth_viewer_key =
                absl::StrFormat("depth_%s", _selected_serial);
            // draw the depth image
            if (viewer::have_image(depth_viewer_key)) {
                axby::viewer::set_cmap(depth_viewer_key, "heat", 0.001, 6.0,
                                       /*cmap_scale=*/0.001,
                                       /*cmap_invert=*/true);

                const auto depth_image_handle =
                    viewer::get_image(depth_viewer_key);
                imgui_image((void*)depth_image_handle.texture,
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

    APP_UNPACK_FLAG(config_name);

    pubsub::init();

    network_config::Config network_config{config_name};
    time_sync::init(network_config);
    rss::client::init(network_config);

    gui_init("Realsense Stream Viewer");
    viewer::init();
    viewer::enable_auto_orbit();

    ActionPeriod refresh_items_period{1.0};

    auto thread_pool = std::make_shared<SimpleThreadPool>(/*num_threads=*/1);
    realsense_streaming::PointCloudJob pointcloud_job{thread_pool};
    FastResizableVector<float> pointcloud_xyzs;
    FastResizableVector<uint8_t> pointcloud_rgbs;

    while (!gui_wants_quit() && !should_stop_all()) {
        if (refresh_items_period.should_act()) {
            rss::client::update_realsense_list(_serial_to_realsense_state);
            for (const auto& [serial, _] : _serial_to_realsense_state) {
                _serials.insert(std::string(serial));
            }
        }

        for (auto& [serial, state] : _serial_to_realsense_state) {
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
                (serial == _selected_serial)) {
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

        gui_loop_begin();
        viewer::new_frame(ImGui::GetIO());

        if (viewer::have_points("pointcloud")) {
            viewer::draw_points("pointcloud", _point_size);
        }

        make_gui();

        gui_loop_end();
    }

    stop_all();

    gui_cleanup();

    rss::client::cleanup();
    time_sync::cleanup();
    pubsub::cleanup();

    return 0;
}
