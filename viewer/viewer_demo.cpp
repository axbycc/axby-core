#include <imgui.h>

#include <Eigen/Dense>
#include <limits>
#include <random>
#include <vector>

#include "app/gui.h"
#include "app/main.h"
#include "colors/colors.h"
#include "debug/log.h"
#include "debug/to_sstring.h"
#include "seq/seq.h"
#include "viewer/viewer.h"
#include "wrappers/eigen.h"

int main(int argc, char *argv[]) {
    __APP_MAIN_INIT__;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> dis_n11(-1, 1);
    std::uniform_real_distribution<float> dis_01(0, 1);
    std::uniform_int_distribution<unsigned int> disint(0, 255);

    // create random pointcloud
    std::vector<float> points;
    std::vector<axby::colors::RGBAf> points_colors;
    std::vector<uint8_t> points_rgbs;
    const int num_points = 40;
    points.reserve(3 * num_points);
    points_rgbs.reserve(3 * num_points);
    for (int i = 0; i < 3 * num_points; ++i) {
        points.push_back(dis_n11(gen));
    }
    for (int i = 0; i < 3 * num_points; ++i) {
        points_rgbs.push_back(disint(gen));
    }

    // create random image
    int width = 100;
    int height = 60;
    std::vector<uint8_t> image_pixels;
    std::vector<float> image_pixels_f;
    for (int i = 0; i < width * height * 3; ++i) {
        image_pixels.push_back(disint(gen));
        image_pixels_f.push_back(dis_01(gen));
    }

    // create random "depth image" for cmapping
    std::vector<float> depth_pixels_f;
    std::vector<uint16_t> depth_pixels_u16;
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            float w_frac = float(w) / width;
            float h_frac = float(h) / height;
            // float val = w_frac * h_frac;
            float val = std::abs(std::cos(5 * w_frac * 3.14) *
                                 std::cos(5 * h_frac * 3.14));
            depth_pixels_f.push_back(val);
            depth_pixels_u16.push_back(val * 1000);
        }
    }

    // create random lines
    const uint32_t num_lines = 10;
    std::vector<uint32_t> line_idxs1;
    std::vector<uint32_t> line_idxs2;
    for (uint32_t i = 0; i < num_lines; ++i) {
        uint32_t idx1 = dis_01(gen) * num_points;
        uint32_t idx2 = dis_01(gen) * num_points;
        CHECK_LT(idx1, num_points);
        CHECK_LT(idx2, num_points);
        line_idxs1.push_back(idx1);
        line_idxs2.push_back(idx2);

        LOG(INFO) << "Connecting " << idx1 << ", " << idx2;
    }

    std::vector<float> line_points1;
    std::vector<float> line_points2;
    LOG(INFO) << "Constructing endpoints 1";
    for (uint32_t i = 0; i < num_lines; ++i) {
        uint32_t idx = line_idxs1[i];
        for (int j = 0; j < 3; ++j) {
            float v = points[3 * idx + j];
            line_points1.push_back(v);
        }
    }
    LOG(INFO) << "Constructing endpoints 2";
    for (uint32_t i = 0; i < num_lines; ++i) {
        uint32_t idx = line_idxs2[i];
        for (int j = 0; j < 3; ++j) {
            float v = points[3 * idx + j];
            line_points2.push_back(v);
        }
    }

    LOG(INFO) << "Initting gui.";
    axby::gui_init("Viewer Demo", 1024, 1024);

    LOG(INFO) << "Initting viewer.";
    axby::viewer::init();
    axby::viewer::enable_auto_orbit();

    Eigen::Matrix4f tx_shift;
    tx_shift.setIdentity();
    tx_shift(1, 3) = 0.75;
    tx_shift(2, 3) = 4.5;

    axby::viewer::update_points("pointcloud", points);
    axby::viewer::update_point_colors("pointcloud", points_rgbs);

    axby::viewer::update_image("image", width, height, image_pixels_f);

    bool use_depth_u16 = true;
    float cmap_min = 0;
    float cmap_max = 1;
    float cmap_scale = use_depth_u16 ? 0.001 : 1;

    if (use_depth_u16) {
        axby::viewer::update_image("depth", width, height, depth_pixels_u16);
    } else {
        axby::viewer::update_image("depth", width, height, depth_pixels_f);
    }

    float tint_amount = 0;
    axby::colors::RGBf tint_color = to_float(axby::colors::cornsilk);

    while (!axby::gui_wants_quit()) {
        axby::gui_loop_begin();
        axby::viewer::new_frame(ImGui::GetIO());

        if (ImGui::Begin("Controls")) {
            ImGui::SliderFloat("tx y", &tx_shift(1, 3), -2, 2);
            ImGui::SliderFloat("tx z", &tx_shift(2, 3), 0, 10);
            ImGui::SliderFloat("tint", &tint_amount, 0, 1);
            ImGui::Text("tx_shift\n%s", axby::to_sstring(tx_shift).c_str());

            ImGui::SliderFloat("cmap min", &cmap_min, 0, 1);
            ImGui::SliderFloat("cmap max", &cmap_max, 0, 1);
            ImGui::SliderFloat("cmap scale", &cmap_scale, 0,
                               use_depth_u16 ? 0.005 : 0.002);

            if (cmap_min > cmap_max) {
                std::swap(cmap_min, cmap_max);
            }

            axby::viewer::ViewParams vp = axby::viewer::get_view_params();
            Eigen::Matrix4f ndc_image_object;
            vp.get_ndc_image_object(tx_shift, ndc_image_object);
            ImGui::Text("ndc_image_object\n%s",
                        axby::to_sstring(ndc_image_object).c_str());

            ImGui::End();
        }
        axby::viewer::set_cmap("depth", "heat", cmap_min, cmap_max, cmap_scale);

        static bool draw_coordinate_frame = false;
        static bool draw_cone = false;
        static bool draw_screen_image = false;
        static bool draw_world_image = false;
        if (ImGui::Begin("Options")) {
            ImGui::Checkbox("Draw Coordinate Frame", &draw_coordinate_frame);
            ImGui::Checkbox("Draw Cone", &draw_cone);
            ImGui::Checkbox("Draw Screen Image", &draw_screen_image);
            ImGui::Checkbox("Draw World Image", &draw_world_image);
        }

        axby::viewer::update_tx_world_object("pointcloud", tx_shift);
        axby::viewer::set_tint("pointcloud", tint_color, tint_amount);
        axby::viewer::draw_points("pointcloud");

        if (draw_screen_image) {
            Eigen::Matrix4f tx_camera_imagepanel = Id4f();
            tx_camera_imagepanel(2, 3) = 1.2;
            axby::viewer::draw_camera_space_image_3d(
                "depth", tx_camera_imagepanel, /*mm_per_px=*/1);
        }
        if (draw_world_image) {
            Eigen::Matrix4f tx_world_imagepanel = Id4f();
            tx_world_imagepanel(2, 3) = 1.2;
            axby::viewer::draw_world_space_image_3d(
                "depth", tx_world_imagepanel, /*mm_per_px=*/1);
        }
        if (draw_coordinate_frame) {
            axby::viewer::draw_coordinate_frame("pointcloud", 0.5);
        }
        if (draw_cone) {
            axby::viewer::draw_cone(
                "pointcloud", 1.0f, 0.5f,
                add_alpha(to_float(axby::colors::blue), 0.5));
        }

        if (ImGui::Begin("Cmap Display")) {
            auto depth_handle = axby::viewer::get_image("depth");
            ImGui::Image((void *)depth_handle.texture,
                         {(float)width * 3, (float)height * 3});
            ImGui::End();
        }
        if (ImGui::Begin("Image Display")) {
            auto image_handle = axby::viewer::get_image("image");
            ImGui::Image((void *)image_handle.texture,
                         {(float)width * 3, (float)height * 3});

            ImGui::End();
        }
        if (ImGui::Begin("Pick Debug")) {
            auto image_handle = axby::viewer::get_debug_vertex_ids();
            ImGui::Image((void *)image_handle.texture,
                         {(float)100, (float)100});

            constexpr uint32_t invalid_point_idx =
                std::numeric_limits<uint32_t>::max();
            static uint32_t clicked_point_idx = invalid_point_idx;
            static int num_clicks = 0;
            ImGui::Text("Num clicks %d", num_clicks);
            ImGui::Text("Last click: %d", clicked_point_idx);
            if (axby::viewer::was_clicked("pointcloud")) {
                auto info = axby::viewer::get_pick_info();
                clicked_point_idx = info.index;
                LOG(INFO) << "World space click "
                          << axby::seq_to_string(info.world_xyz);
                // this is automatic when auto orbit is on
                // axby::viewer::set_pt_world_orbit(info.world_xyz);
                num_clicks += 1;
            }

            uint32_t hovered_point_idx = invalid_point_idx;
            if (axby::viewer::was_hovered("pointcloud")) {
                auto info = axby::viewer::get_pick_info();
                hovered_point_idx = info.index;
                ImGui::Text("Pointcloud is being hovered at %u!",
                            hovered_point_idx);
            }

            axby::viewer::draw_lines("pointcloud", "pointcloud", line_points1,
                                     line_points2, axby::colors::whitesmoke);

            ImGui::ShowDemoWindow();

            ImGui::End();
        }
        axby::gui_loop_end();
    }
    axby::gui_cleanup();

    return 0;
}
