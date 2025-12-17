#include "viewer/viewer.h"

#include <imgui.h>
#include <imoverlayable.h>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_format.h"
#include "app/flag.h"
#include "app/gui.h"
#include "app/main.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "debug/log.h"
#include "network_config/config.h"
#include "realsense_streaming/client.h"
#include "realsense_streaming/messages.h"
#include "time_sync/time_sync.h"

APP_FLAG(std::string,
         output_log_path,
         /*default=*/"",
         "If nonempty, record to this log file.");

APP_FLAG(std::string, config_name, "local", "network config name");

using namespace axby;
namespace rss = realsense_streaming;

int main(int argc, char* argv[]) {
    __APP_MAIN_INIT__;

    APP_UNPACK_FLAG(output_log_path);
    APP_UNPACK_FLAG(config_name);

    const bool enable_logging = !output_log_path.empty();
    if (enable_logging) {
        LOG(FATAL) << "todo";
    }

    pubsub::init();

    network_config::Config network_config{config_name};
    time_sync::init(network_config);
    rss::client::init(network_config);

    gui_init("Realsense Stream Viewer", 1280, 720);
    viewer::init();

    ActionPeriod refresh_items_period{1.0};
    absl::flat_hash_map<rss::SerialNumber, rss::client::RealsenseState>
        serial_to_realsense_state;

    while (!gui_wants_quit() && !should_stop_all()) {
        if (refresh_items_period.should_act()) {
            rss::client::update_realsense_list(serial_to_realsense_state);
        }

        for (auto& [serial, state] : serial_to_realsense_state) {
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
        }

        gui_loop_begin();
        viewer::new_frame(ImGui::GetIO());

        if (ImGui::Begin("Recording")) {
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
            

            ImGui::End();
        }

        for (const auto& [serial, state] : serial_to_realsense_state) {
            if (ImGui::Begin(
                    absl::StrFormat("%s: %s",
                                    state.color.stream_meta.device_name, serial)
                        .c_str())) {
                const auto window_width = ImGui::GetWindowWidth();

                std::string depth_viewer_key =
                    absl::StrFormat("depth_%s", serial);
                std::string color_viewer_key =
                    absl::StrFormat("color_%s", serial);

                // draw the color image
                if (viewer::have_image(color_viewer_key)) {
                    const auto image_handle =
                        viewer::get_image(color_viewer_key);
                    ImOverlayable::image(
                        (void*)image_handle.texture, (float)image_handle.width,
                        (float)image_handle.height, window_width * 0.9);
                }

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
                                         window_width * 0.9);
                }
            }
        }

        gui_loop_end();
    }

    stop_all();

    gui_cleanup();

    rss::client::cleanup();
    time_sync::cleanup();
    pubsub::cleanup();

    return 0;
}
