#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>

#include "axgl/frame_buffer.h"
#include "colors/colors.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "mesh.h"
#include "seq/any_seq.h"
#include "seq/float_or_double_seq.h"
#include "seq/seq.h"

namespace axby {
namespace viewer {

struct ViewParams {
    gl::FrameBufferInfo frame_buffer;

    // all matrices are square, col-major
    std::array<float, 4 * 4>
        tx_camera_world;  // transform from world to camera coordinates
    std::array<float, 4 * 4>
        ndc_image_camera;  // projection camera into NDC coordinates

    ViewParams();

    ViewParams(gl::FrameBufferInfo frame_buffer,
               ConstFloatOrDoubleSeq tx_camera_world_in,
               ConstFloatOrDoubleSeq ndc_image_camera_in);

    // product of previous two matrices
    void get_ndc_image_world(FloatOrDoubleSeq ndc_image_world_out) const;

    // product of the previous two matrices, plus the input tx_world_object
    void get_ndc_image_object(ConstFloatOrDoubleSeq tx_world_object_in,
                              FloatOrDoubleSeq ndc_image_object_out) const;

    void get_ndc_camera_object(ConstFloatOrDoubleSeq tx_world_object_in,
                               FloatOrDoubleSeq ndc_camera_object_out) const;
};

class ViewParamsScope {
    // RAII class which pushes and pops the current ViewParams and
    // frame buffer id used by the geometry viewer functions
   public:
    ViewParamsScope(const ViewParams& view_params, uint32_t frame_buffer_id);
    ~ViewParamsScope();

   private:
    ViewParams prev_view_params;
    uint32_t prev_frame_buffer_id;
};

struct PickInfo {
    uint32_t id_hash = 0;
    uint32_t index = 0;
    std::array<float, 3> world_xyz{};

    bool id_matches(std::string_view id) const;
};

// requires gl context initialized already
void init();

// should we automatically reset the orbit point to any point that is
// clicked
void enable_auto_orbit();

ViewParams get_view_params();

// set the orbit point of the viewer. any viewer rotations will be
// relative to this point
void set_pt_world_orbit(Seq<const float> pt_world_orbit);

void set_tx_camera_world(Seq<const float> tx_camera_world);

// wrapper around ImGui's ImGuiIO so that we don't have to take a
// dependency on ImGui. this is necessary to create python bindings for
// python programs which may link their own version of ImGui
struct IO {
    bool WantCaptureMouse = false;
    std::array<float, 2> MousePos = {};
    float MouseWheel = 0;
    std::array<bool, 3> MouseDown = {};
    std::array<bool, 3> MouseClicked = {};
    std::array<float, 2> MouseDelta = {};

    std::array<float, 2> DisplaySize;
    bool KeyCtrl = false;

    IO() = default;

    // templated constructor to convert from ImGuiIO without taking
    // the dependency <imgui.h>
    template <typename U>
    IO(const U& imgui_io) {
        WantCaptureMouse = imgui_io.WantCaptureMouse;
        MousePos = {imgui_io.MousePos.x, imgui_io.MousePos.y};
        MouseDelta = {imgui_io.MouseDelta.x, imgui_io.MouseDelta.y};
        MouseWheel = imgui_io.MouseWheel;
        MouseDown = {imgui_io.MouseDown[0], imgui_io.MouseDown[1],
                     imgui_io.MouseDown[2]};
        MouseClicked = {imgui_io.MouseClicked[0], imgui_io.MouseClicked[1],
                        imgui_io.MouseClicked[2]};
        DisplaySize = {imgui_io.DisplaySize[0], imgui_io.DisplaySize[1]};
        KeyCtrl = imgui_io.KeyCtrl;
    }
};

void new_frame(const IO& io);

/*
  picking functionality:
  if (was_clicked("my_thing")) {
      get_pick_info().world_xyz; // the world point that was clicked
      get_pick_info().index; // the index of the point that was clicked
  }

  if (was_hovered("my_thing")) {
      get_pick_info().world_xyz; // the world point that was hovered
      get_pick_info().index; // the index of the point that was hovered
  }

  you can also theoretically call
  get_pick_info().id_matches("my_thing") and if there is a match, use
  the mouse state to know if "my_thing" was hovered or clicked. but
  there are some details around handling interaction with imgui when
  it wants to capture the mouse.
 */
bool was_clicked(std::string_view id);
bool was_hovered(std::string_view id);
PickInfo get_pick_info();

void update_points(std::string_view id,
                   Seq<const float> points,
                   bool dynamic = true);

// must have called update_points before hand
void update_point_colors(std::string_view id,
                         ConstAnySeq colors,
                         bool dynamic = true);

void update_image(std::string_view id, int width, int height, ConstAnySeq data);

bool have_image(std::string_view id);

struct ImageHandle {
    // cast texture to void* for interfacing with ImGui::Image
    uint64_t texture = 0;
    int width = 0;
    int height = 0;
};

ImageHandle get_image(std::string_view id);

void set_tint(std::string_view id, ConstAnySeq color, float tint_amount);
void unset_tint(std::string_view id);

/*
  an image must exist for the id (created using update_image)
  valid cmaps (color maps) are heat, hsv, parula, viridis, plasma,
  jet, gray
 */
void set_cmap(std::string_view id,
              std::string_view cmap,
              float cmap_min = 0.0f,
              float cmap_max = 1.0f,
              float cmap_scale = 1.0f,
              bool cmap_invert = false);

ImageHandle get_debug_vertex_ids();

void update_tx_world_object(std::string_view id,
                            ConstFloatOrDoubleSeq tx_world_object);

void get_tx_world_object(std::string_view id, FloatOrDoubleSeq out);

bool have_points(std::string_view id);
void draw_points(std::string_view id, float point_size = 1.0f);

void draw_coordinate_frame(std::string_view id, float scale = 1.0f);
void draw_coordinate_frame(ConstFloatOrDoubleSeq tx_world_frame, float scale = 1.0f);

void draw_cone(std::string_view id,
               float base_scale,
               float height_scale,
               ConstAnySeq color);
void draw_square_cone(std::string_view id,
                      float base_scale,
                      float height_scale,
                      ConstAnySeq color);

// id1 and id2 specify the pose of points1 and points2 in world frame.
// length of points1 and points2 must be the same. if length of colors
// is 4 or 3, it will be applied to uniformly to all lines. Or else it
// must be (3 or 4)*number of points depending on if there is an alpha
// channel.
void draw_lines(std::string_view id1,
                std::string_view id2,
                Seq<const float> points1,
                Seq<const float> points2,
                ConstAnySeq color_or_colors);

void draw_camera_space_image_3d(std::string_view id,
                                ConstFloatOrDoubleSeq tx_camera_imagepanel,
                                float mm_per_pixel = 1);

void draw_world_space_image_3d(std::string_view id,
                               ConstFloatOrDoubleSeq tx_world_imagepanel,
                               float mm_per_pixel = 1);

/*
  A model is made out of parts. The typical case is a robot model,
  whose parts are its links.

  Each part is visualized with a list of meshes.
 */
void add_model_part(std::string_view id,
                    std::string_view part_id,
                    Seq<const Mesh> meshes);

// sets all meshes of all parts of the model to use a specific
// material (mainly useful for debugging shaders)
void set_model_material(std::string_view id, const MeshMaterial& material);

// convenience overload, when there is just one part
// the part_id is set as empty string
void add_model(std::string_view id, Seq<const Mesh> meshes);

void get_tx_world_modelpart(std::string_view id,
                            std::string_view part_id,
                            FloatOrDoubleSeq tx_world_modelpart);

// update the relative pose of the the model part wrt model. to update
// the pose of model as a whole just use update_tx_world_object
void update_tx_model_modelpart(std::string_view id,
                               std::string_view part_id,
                               ConstFloatOrDoubleSeq tx_model_modelpart);

void draw_model(std::string_view id);

}  // namespace viewer
}  // namespace axby
