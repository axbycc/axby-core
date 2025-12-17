#include "viewer.h"

#include <Eigen/Dense>
#include <cstdint>
#include <magic_enum.hpp>
#include <map>
#include <random>
#include <span>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_format.h"
#include "app/gui.h"
#include "axgl/buffer.h"
#include "axgl/cmap.h"
#include "axgl/frame_buffer.h"
#include "axgl/info.h"
#include "axgl/shapes.h"
#include "axgl/texture.h"
#include "axgl/vertex_array.h"
#include "colors/colors.h"
#include "debug/log.h"
#include "fast_resizable_vector/fast_resizable_vector.h"
#include "math/camera.h"
#include "seq/float_or_double_seq.h"
#include "shaders/cmapped_texture.h"
#include "shaders/colored_vertex_ids.h"
#include "shaders/colored_vertices.h"
#include "shaders/debug_vertex_ids.h"
#include "shaders/lines.h"
#include "shaders/material_mesh.h"
#include "shaders/texture.h"
#include "shaders/vertex_ids.h"
#include "shaders/vertex_world_xyzs.h"
#include "wrappers/eigen.h"

namespace axby {
namespace viewer {

inline uint32_t string_to_u32(std::string_view s) {
    // 32-bit FNV-1a
    uint32_t h = 2166136261u;  // FNV offset basis
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;  // FNV prime
    }
    return h;
}

ViewParams::ViewParams() {
    frame_buffer.id = 0;
    frame_buffer.width = 0;
    frame_buffer.height = 0;
    M_Matrix4f(ndc_image_camera).setIdentity();
    M_Matrix4f(tx_camera_world).setIdentity();
}

void ViewParams::get_ndc_image_world(
    FloatOrDoubleSeq ndc_image_world_out) const {
    Eigen::Matrix4f ndc_image_world =
        CM_Matrix4f(ndc_image_camera) * CM_Matrix4f(tx_camera_world);
    ndc_image_world_out.copy_from(ndc_image_world);
}

void ViewParams::get_ndc_image_object(
    ConstFloatOrDoubleSeq tx_world_object_in,
    FloatOrDoubleSeq ndc_image_object_out) const {
    Eigen::Matrix4f tx_world_object;
    tx_world_object_in.write_to(tx_world_object);

    Eigen::Matrix4f ndc_image_object = CM_Matrix4f(ndc_image_camera) *
                                       CM_Matrix4f(tx_camera_world) *
                                       tx_world_object;
    ndc_image_object_out.copy_from(ndc_image_object);
}

void ViewParams::get_ndc_camera_object(
    ConstFloatOrDoubleSeq tx_world_object_in,
    FloatOrDoubleSeq ndc_camera_object_out) const {
    Eigen::Matrix4f tx_world_object;
    tx_world_object_in.write_to(tx_world_object);

    Eigen::Matrix4f ndc_camera_object =
        CM_Matrix4f(tx_camera_world) * tx_world_object;
    ndc_camera_object_out.copy_from(ndc_camera_object);
}

ViewParams::ViewParams(gl::FrameBufferInfo frame_buffer,
                       ConstFloatOrDoubleSeq tx_camera_world_in,
                       ConstFloatOrDoubleSeq ndc_image_camera_in) {
    CHECK(tx_camera_world_in.size() == 4 * 4);
    CHECK(ndc_image_camera_in.size() == 4 * 4);

    this->frame_buffer = frame_buffer;
    tx_camera_world_in.write_to(tx_camera_world);
    ndc_image_camera_in.write_to(ndc_image_camera);
}

struct OrbitState {
    // the orbit state is used for mouse-based camera movement. the
    // camera should be placed relative to the tx_world_orbit pose,
    // translated by the cam_offset parameter

    // the camera starts here
    Eigen::Matrix4f tx_world_orbit;

    // xyz location of the orbit point in camera frame. the camera
    // translates this amount relative to the orbit frame.
    Eigen::Vector3f cam_offset;
};

ViewParams _view_params;
uint32_t _frame_buffer_id = 0;
uint32_t _frame_counter = 0;
bool _initted = false;
float _focal_scale = 1.0f;
OrbitState _orbit_state;

ViewParamsScope::ViewParamsScope(const ViewParams& view_params,
                                 uint32_t frame_buffer_id) {
    prev_view_params = _view_params;
    prev_frame_buffer_id = _frame_buffer_id;

    _view_params = view_params;
    _frame_buffer_id = frame_buffer_id;
};

ViewParamsScope::~ViewParamsScope() {
    _frame_buffer_id = prev_frame_buffer_id;
    _view_params = prev_view_params;
};

const float ROI_SCALE = 0.05f;  // ndc units, half-side length of ROI
Eigen::Vector2f _roi_center =
    Eigen::Vector2f::Zero();  // ROI center, updated inside new_frame()

const int PICK_BUFFER_WIDTH_PX = 30;
const int PICK_BUFFER_HEIGHT_PX = 30;
const float PICK_POINT_SIZE = 10;
gl::FrameBuffer _pick_buffer;
gl::FrameBuffer _pick_world_xyzs_buffer;
gl::FrameBuffer _debug_pick_buffer;
FastResizableVector<uint32_t> _pick_buffer_cpu;
FastResizableVector<float> _pick_world_xyzs_cpu;

PickInfo _pick_info;
bool PickInfo::id_matches(std::string_view id) const {
    return string_to_u32(id) == id_hash;
}

bool _auto_orbit = false;
void enable_auto_orbit() { _auto_orbit = true; }

void init() {
    CHECK(!_initted) << "double init";
    _initted = true;

    _orbit_state.tx_world_orbit.setIdentity();
    _orbit_state.cam_offset.setZero();
    _orbit_state.cam_offset(2) = -0.3;

    _view_params.frame_buffer.id = 0;
    _view_params.frame_buffer.have_depth = true;
    std::tie(_view_params.frame_buffer.width,
             _view_params.frame_buffer.height) = gui_window_size();
    M_Matrix4f(_view_params.tx_camera_world.data()).setIdentity();
    make_camera_matrix({1.0, 1.0, 0.0, 0.0}, _view_params.ndc_image_camera);

    gl::TextureOptions pick_buffer_options;
    pick_buffer_options.type = GL_UNSIGNED_INT;
    pick_buffer_options.format = GL_RG_INTEGER;
    pick_buffer_options.internal_format = GL_RG32UI;
    pick_buffer_options.min_filter = GL_NEAREST;
    pick_buffer_options.mag_filter = GL_NEAREST;
    _pick_buffer = {pick_buffer_options, PICK_BUFFER_WIDTH_PX,
                    PICK_BUFFER_HEIGHT_PX, /*with_depth=*/true};
    _pick_buffer_cpu.resize(PICK_BUFFER_WIDTH_PX * PICK_BUFFER_HEIGHT_PX * 2);

    gl::TextureOptions pick_world_position_buffer_options;
    pick_world_position_buffer_options.type = GL_FLOAT;
    pick_world_position_buffer_options.format = GL_RGB;
    pick_world_position_buffer_options.internal_format = GL_RGB32F;
    _pick_world_xyzs_buffer = {pick_world_position_buffer_options,
                               PICK_BUFFER_WIDTH_PX, PICK_BUFFER_HEIGHT_PX,
                               /*with_depth=*/true};
    _pick_world_xyzs_cpu.resize(PICK_BUFFER_WIDTH_PX * PICK_BUFFER_HEIGHT_PX *
                                3);

    gl::TextureOptions debug_pick_buffer_options;
    debug_pick_buffer_options.set_data_type<float>().set_rgb();
    _debug_pick_buffer = {debug_pick_buffer_options, PICK_BUFFER_WIDTH_PX,
                          PICK_BUFFER_HEIGHT_PX};
}

struct PointsData {
    gl::Buffer gl_buffer;
    uint32_t num_points = 0;
};

struct ColorsData {
    gl::Buffer gl_buffer;
    uint64_t num_colors = 0;
    bool have_alpha = false;
};

struct TintData {
    float tint_amount = 0.0f;
    colors::RGBf color = to_float(colors::red);
};

struct ImageData {
    int width = 0;
    int height = 0;
    gl::Texture gl_texture;
};

struct CmapData {
    gl::FrameBuffer frame_buffer;
    gl::Cmap cmap;
    float min = 0.0f;
    float max = 1.0f;
    float scale = 1.0f;
    bool invert = false;
};

struct MeshData {
    gl::VertexArray vertex_array;
    int num_items;  // 3 * num faces
    MeshMaterial material;
};

struct ModelPartData {
    std::vector<MeshData> meshes;
    Eigen::Matrix4f tx_model_modelpart;
};

absl::flat_hash_map<std::string, PointsData> _id_to_points;
absl::flat_hash_map<std::string, ColorsData> _id_to_colors;
absl::flat_hash_map<std::string, TintData> _id_to_tint;
absl::flat_hash_map<std::string, gl::VertexArray> _id_to_points_and_colors_vao;
absl::flat_hash_map<std::string, Eigen::Matrix4f> _id_to_tx_world_object;
absl::flat_hash_map<std::string, ImageData> _id_to_image_data;
absl::flat_hash_map<std::string, gl::FrameBuffer> _id_to_cmap_frame_buffer;
absl::flat_hash_map<std::string, CmapData> _id_to_cmap_data;
absl::flat_hash_map<std::string,
                    absl::flat_hash_map<std::string, ModelPartData>>
    _id_to_model;

ViewParams get_view_params() {
    CHECK(_initted);
    return _view_params;
}

void set_tx_camera_world(Seq<const float> tx_camera_world) {
    seq_copy(tx_camera_world, _view_params.tx_camera_world);
    _orbit_state.tx_world_orbit =
        CM_Matrix4f(_view_params.tx_camera_world.data()).inverse();
    _orbit_state.cam_offset.setZero();
};

void set_pt_world_orbit(Seq<const float> pt_world_orbit_data) {
    CHECK(pt_world_orbit_data.size() == 3);
    CM_Vector3f pt_world_orbit(pt_world_orbit_data);

    const Eigen::Vector3f pt_world_orbit_before =
        _orbit_state.tx_world_orbit.block<3, 1>(0, 3);

    const Eigen::Vector3f orbit_movement =
        pt_world_orbit - pt_world_orbit_before;

    _orbit_state.tx_world_orbit.block<3, 1>(0, 3) += orbit_movement;

    // we moved the orbit point, but then the camera should pan in the
    // opposite direction so that the screen does not jump
    _orbit_state.cam_offset +=
        _orbit_state.tx_world_orbit.block<3, 3>(0, 0).transpose() *
        orbit_movement;
}

void update_orbit_from_mouse(const IO& io, OrbitState& orbit_state) {
    // update the orbit's tx_world_orbit and the cam_offset based on user
    // mouse movement
    auto dx = io.MouseDelta[0];
    auto dy = io.MouseDelta[1];

    if (io.MouseWheel != 0 && io.MouseDown[1]) {
        // roll z
        const float roll_scale = 6e-2;
        const Eigen::Matrix3f drot_world_orbit =
            (Eigen::AngleAxisf(io.MouseWheel * roll_scale,
                               -Eigen::Vector3f::UnitZ()))
                .toRotationMatrix();
        orbit_state.tx_world_orbit.block<3, 3>(0, 0) *= drot_world_orbit;
    } else if (io.MouseWheel != 0) {
        // zooming
        orbit_state.cam_offset(2) += io.MouseWheel * 0.1;
    } else if (io.MouseDown[2] || (io.KeyCtrl && io.MouseDown[0])) {
        // panning. trans_scale is used so that when user is very
        // zoomed-in close to the orbit point, then a pan will
        // only move slightly.
        const float trans_scale =
            std::max(std::abs(_orbit_state.cam_offset(2)) * 1e-3, 0.001);
        orbit_state.cam_offset(0) += dx * trans_scale;
        orbit_state.cam_offset(1) += dy * trans_scale;
    } else if (io.MouseDown[1]) {
        // spin x, spin y
        const float rot_scale = 3e-3;
        const Eigen::Matrix3f drot_world_orbit =
            (Eigen::AngleAxisf(dx * rot_scale, Eigen::Vector3f::UnitY()) *
             Eigen::AngleAxisf(-dy * rot_scale, Eigen::Vector3f::UnitX()))
                .toRotationMatrix();

        orbit_state.tx_world_orbit.block<3, 3>(0, 0) *= drot_world_orbit;
    }
}

void update_view_params_from_orbit(const OrbitState& orbit_state) {
    // the camera pose starts at the orbit pose
    Eigen::Matrix4f tx_camera_world = orbit_state.tx_world_orbit.inverse();

    // then, it's offset in its own frame by the cam_offset
    tx_camera_world.block<3, 1>(0, 3) += orbit_state.cam_offset;

    // initialize the projection matrix accounting for screen aspect ratio
    const float width_to_height = float(_view_params.frame_buffer.width) /
                                  _view_params.frame_buffer.height;
    Eigen::Matrix4f ndc_image_camera;
    make_camera_matrix({1.0, width_to_height, 0.0, 0.0}, ndc_image_camera);
    ndc_image_camera.row(1) *= -1;  // left-handed coordinate system
    ndc_image_camera.topLeftCorner<2, 2>() *= _focal_scale;

    seq_copy(tx_camera_world, _view_params.tx_camera_world);
    seq_copy(ndc_image_camera, _view_params.ndc_image_camera);
}

IO _current_io;

void update_pick_info() {
    if (_current_io.WantCaptureMouse) {
        // mouse is being captured by imgui so there should be no
        // valid pick info
        _pick_info = {};
        return;
    }

    float closest_distance = std::numeric_limits<float>::infinity();
    uint32_t closest_group_id = 0;
    uint32_t closest_vertex_id = 0;
    uint32_t closest_w = 0;
    uint32_t closest_h = 0;
    for (int h = 0; h < PICK_BUFFER_HEIGHT_PX; ++h) {
        for (int w = 0; w < PICK_BUFFER_WIDTH_PX; ++w) {
            const int flat_idx = 2 * ((PICK_BUFFER_WIDTH_PX * h) + w);
            const uint32_t group_id = _pick_buffer_cpu[flat_idx + 0];
            const uint32_t vertex_id = _pick_buffer_cpu[flat_idx + 1];
            if (group_id != 0) {
                Eigen::Vector2f pos_ndc;
                pos_ndc << float(w) / PICK_BUFFER_WIDTH_PX,
                    float(h) / PICK_BUFFER_HEIGHT_PX;
                float distance = pos_ndc.norm();
                if (distance < closest_distance) {
                    closest_distance = distance;
                    closest_group_id = group_id;
                    closest_vertex_id = vertex_id;
                    closest_w = w;
                    closest_h = h;
                }
            }
        }
    }
    if (closest_group_id != 0) {
        _pick_info.id_hash = closest_group_id;
        _pick_info.index = closest_vertex_id;

        // lookup world position in the world position cpu buffer
        const int flat_idx =
            3 * ((PICK_BUFFER_WIDTH_PX * closest_h) + closest_w);
        const float x = _pick_world_xyzs_cpu[flat_idx];
        const float y = _pick_world_xyzs_cpu[flat_idx + 1];
        const float z = _pick_world_xyzs_cpu[flat_idx + 2];
        _pick_info.world_xyz = {x, y, z};

    } else {
        _pick_info.id_hash = 0;
        _pick_info.index = 0;
        _pick_info.world_xyz = {0, 0, 0};
    }
}

void new_frame(const IO& io) {
    CHECK(_initted);

    _current_io = io;

    if (_frame_counter != 0) {
        _debug_pick_buffer.clear();
        shaders::draw_debug_vertex_ids(_debug_pick_buffer,
                                       _pick_buffer.color.id);
        _pick_buffer.color.download(_pick_buffer_cpu);
        _pick_world_xyzs_buffer.color.download(_pick_world_xyzs_cpu);
        update_pick_info();

        if (_auto_orbit && (_pick_info.id_hash != 0) &&
            _current_io.MouseClicked[0]) {
            set_pt_world_orbit(_pick_info.world_xyz);
        }
    }

    _pick_buffer.clear_ui();
    _pick_world_xyzs_buffer.clear(1.234);

    std::tie(_view_params.frame_buffer.width,
             _view_params.frame_buffer.height) = gui_window_size();
    if (_view_params.frame_buffer.height == 0 ||
        _view_params.frame_buffer.width == 0) {
        // no screen, possibly minimized
        return;
    }
    _view_params.frame_buffer.id = 0;
    _view_params.frame_buffer.have_depth = true;

    if (!io.WantCaptureMouse) {
        update_orbit_from_mouse(io, _orbit_state);
    }

    // the roi center is the mouse position in NDC [-1,1]^2 coordinates
    _roi_center(0) =
        2 * ((io.MousePos[0] / _view_params.frame_buffer.width) - 0.5f);
    _roi_center(1) =
        -2 * ((io.MousePos[1] / _view_params.frame_buffer.height) - 0.5f);

    update_view_params_from_orbit(_orbit_state);

    _frame_counter += 1;
    if (_frame_counter == 0) {
        // we use _frame_counter == 0 as the uninitialized
        // condition. this works until the counter wraps around. we
        // probably would not see this in practice, but we can handle
        // it here for safety.
        _frame_counter = 1;
    }
}

void update_tx_world_object(std::string_view id,
                            ConstFloatOrDoubleSeq tx_world_object_in) {
    CHECK(_initted);
    tx_world_object_in.write_to(_id_to_tx_world_object[id]);
}

void update_points(std::string_view id, Seq<const float> points, bool dynamic) {
    CHECK(points.size() % 3 == 0);  // xyz,xyz,xyz,...
    auto& points_data = _id_to_points[id];
    auto& gl_buffer = points_data.gl_buffer;
    gl_buffer.options.set_data_type<float>();
    gl_buffer.options.dynamic = dynamic;
    gl_buffer.upload(points);
    points_data.num_points = points.size() / 3;
};

void update_pick_buffer(uint32_t id_hash,
                        const gl::ProgramDrawInfo& draw_info,
                        Seq<const float> original_mvp,
                        Seq<const float> tx_world_object) {
    // We want to render the vertex array into the
    // pick_frame_buffer. But we don't want to render the whole image,
    // only the ROI around _roi_center. So we need to alter the mvp (model-view-projection matrix).
    Eigen::Matrix4f mvp;
    seq_copy(/*src=*/original_mvp, /*dst=*/mvp);

    // mvp modifier will map the ROI to [-1,1]x[-1,1]
    Eigen::Matrix4f mvp_modifier = Eigen::Matrix4f::Identity();
    mvp_modifier.block<2, 1>(0, 3) << -_roi_center[0], -_roi_center[1];
    mvp_modifier.block<2, 4>(0, 0) *= 1.0f / ROI_SCALE;
    mvp = (mvp_modifier * mvp).eval();

    // render into the pick_render_buffer, using the the modified mvp
    // this will create an image of the vertex ids inside the ROI
    shaders::draw_vertex_ids(_pick_buffer, draw_info, PICK_POINT_SIZE, mvp,
                             id_hash);
    shaders::draw_vertex_world_xyzs(_pick_world_xyzs_buffer, draw_info,
                                    PICK_POINT_SIZE, mvp, tx_world_object);
}

bool have_points(std::string_view id) { return _id_to_points.count(id); }

void update_point_colors(std::string_view id,
                         ConstAnySeq colors,
                         bool dynamic) {
    CHECK(_initted);
    CHECK(_id_to_points.count(id));

    // validate colors is the correct length
    const auto num_points = _id_to_points.at(id).num_points;
    const size_t size_if_rgb = num_points * 3;
    const size_t size_if_rgba = num_points * 4;
    CHECK((colors.logical_size() == size_if_rgb) ||
          (colors.logical_size() == size_if_rgba))
        << colors.logical_size() << " vs " << size_if_rgb << " or "
        << size_if_rgba;

    auto& colors_data = _id_to_colors[id];
    auto& gl_buffer = colors_data.gl_buffer;
    gl_buffer.ensure_initted();
    gl_buffer.options.data_type = gl::typeid_to_glenum(colors.get_typeid());
    gl_buffer.options.dynamic = dynamic;
    gl_buffer.upload(colors);

    colors_data.num_colors = num_points;
    colors_data.have_alpha = (colors.logical_size() == size_if_rgba);
}

void get_tx_world_object(std::string_view id, FloatOrDoubleSeq out) {
    CHECK(_initted);
    if (!_id_to_tx_world_object.count(id)) {
        _id_to_tx_world_object[id].setIdentity();
    }
    out.copy_from(_id_to_tx_world_object[id]);
}

void draw_points(std::string_view id, float point_size) {
    CHECK(_initted);
    CHECK(_id_to_points.count(id));
    auto& points = _id_to_points[id];
    CHECK(points.gl_buffer.id != 0) << "No points uploaded for " << id;

    bool have_colors = false;
    bool have_alpha = false;
    if (_id_to_colors.count(id)) {
        auto& colors = _id_to_colors.at(id);
        CHECK(colors.gl_buffer.id != 0);
        CHECK_EQ(colors.num_colors, points.num_points);
        have_colors = true;
        have_alpha = colors.have_alpha;
    }

    auto& vao = _id_to_points_and_colors_vao[id];

    // in high-performance engines, we would want to avoid re-binding
    // vertex array attribute pointers if the parameters have not
    // changed. but we don't expect to be rendering thousands of
    // entities per frame, so we just incur the cost of re-binding on
    // every call. in exchange, we don't have to deal with state
    // tracking issues.

    const int xyz_location = 0;
    const int rgb_location = 1;
    const int alpha_location = 2;

    vao.set_vertex_attribute_3d(xyz_location, points.gl_buffer);

    if (have_colors) {
        auto& colors = _id_to_colors.at(id);
        vao.set_vertex_attribute_3d(rgb_location, colors.gl_buffer,
                                    have_alpha ? 4 : 3);
    } else {
        vao.set_default_float3(rgb_location, colors::to_float(colors::red));
    }

    if (have_colors && have_alpha) {
        auto& colors = _id_to_colors.at(id);
        vao.set_vertex_attribute_1d(alpha_location, colors.gl_buffer,
                                    /*logical_stride=*/4,
                                    /*logical_offset=*/3);
    } else {
        vao.set_default_float(alpha_location, 1.0f);
    }

    Eigen::Matrix4f tx_world_pointcloud;
    get_tx_world_object(id, tx_world_pointcloud);

    Eigen::Matrix4f ndc_image_object;  // mvp matrix
    _view_params.get_ndc_image_object(tx_world_pointcloud, ndc_image_object);

    const auto& tint_data = _id_to_tint[id];

    gl::ProgramDrawInfo draw_info;
    draw_info.vertex_array = vao;
    draw_info.num_items = points.num_points;
    draw_info.draw_mode = GL_POINTS;

    shaders::draw_colored_vertices(_view_params.frame_buffer, draw_info,
                                   ndc_image_object, point_size, tint_data.color,
                                   tint_data.tint_amount);

    update_pick_buffer(string_to_u32(id), draw_info, ndc_image_object,
                       tx_world_pointcloud);
}

void set_tint(std::string_view id, ConstAnySeq color, float tint_amount) {
    CHECK_GE(tint_amount, 0.0f);
    CHECK_LE(tint_amount, 1.0f);
    CHECK(color.logical_size() >= 3);

    auto& tint_data = _id_to_tint[id];
    tint_data.tint_amount = tint_amount;

    if (color.is_type<uint8_t>()) {
        auto color_u8 = color.get_span<const uint8_t>();
        colors::RGB converter{color_u8[0], color_u8[1], color_u8[2]};
        tint_data.color = to_float(converter);
    } else if (color.is_type<float>()) {
        auto color_f = color.get_span<const float>();
        tint_data.color = {color_f[0], color_f[1], color_f[2]};
    }
};

void unset_tint(std::string_view id) {
    auto& tint_data = _id_to_tint[id];
    tint_data.tint_amount = 0;
}

void update_image(std::string_view id,
                  int width,
                  int height,
                  ConstAnySeq data) {
    // figure out how many channels are in the image
    const int num_pixels = width * height;
    const int num_pixels_times_num_channels = data.logical_size();
    CHECK_EQ(num_pixels_times_num_channels % num_pixels, 0);
    const int num_channels = num_pixels_times_num_channels / num_pixels;

    // 1d (depth map) or rgb color
    CHECK((num_channels == 1) || (num_channels == 3)) << num_channels;

    // supported data types: float, uint8, uint16
    CHECK((data.is_type<float>() || data.is_type<uint8_t>() ||
           data.is_type<uint16_t>()));

    if (!_id_to_image_data.count(id)) {
        // need to initialize the texture
        auto& image_data = _id_to_image_data[id];

        gl::TextureOptions options;
        options.type = gl::typeid_to_glenum(data.get_typeid());

        if (num_channels == 1) {
            options.set_r();
        } else if (num_channels == 3) {
            options.set_rgb();
        } else {
            LOG(FATAL) << "Unsupported number of channels " << num_channels;
        }

        image_data.gl_texture = gl::Texture(options);
    }

    // upload the texture
    auto& image_data = _id_to_image_data[id];
    image_data.width = width;
    image_data.height = height;
    image_data.gl_texture.upload(width, height, data);
}

gl::Cmap get_cmap_from_string(std::string_view cmap_name, gl::Cmap fallback) {
    auto possible_cmap = magic_enum::enum_cast<gl::Cmap>(cmap_name);
    if (!possible_cmap) {
        LOG_EVERY_T(WARNING, 10)
            << "cmap " << cmap_name << " does not exist. using fallback";
        return fallback;
    } else if (*possible_cmap == gl::Cmap::_NUM_CMAPS) {
        LOG_EVERY_T(WARNING, 10)
            << "cmap " << cmap_name << " does not exist. using fallback";
        return fallback;
    } else {
        return *possible_cmap;
    }

    // unreachable
    return fallback;
}

void set_cmap(std::string_view id,
              std::string_view cmap,
              float cmap_min,
              float cmap_max,
              float cmap_scale,
              bool cmap_invert) {
    CHECK(_id_to_image_data.count(id)) << id;
    const ImageData& image_data = _id_to_image_data.at(id);
    const bool need_init_cmap_data = !_id_to_cmap_data.count(id);
    CmapData& cmap_data = _id_to_cmap_data[id];

    if (need_init_cmap_data) {
        // options for the color texture of the render buffer that
        // will hold the cmapped image
        gl::TextureOptions options;
        options.set_data_type<uint8_t>();
        options.set_rgb();
        cmap_data.frame_buffer =
            gl::FrameBuffer(options, image_data.width, image_data.height);
    }

    cmap_data.cmap = get_cmap_from_string(cmap, /*fallback=*/gl::Cmap::viridis);
    cmap_data.min = cmap_min;
    cmap_data.max = cmap_max;
    cmap_data.scale = cmap_scale;
    cmap_data.invert = cmap_invert;
}

ImageHandle get_debug_vertex_ids() {
    return {.texture = _debug_pick_buffer.color.id,
            .width = _debug_pick_buffer.width,
            .height = _debug_pick_buffer.height};
}

bool have_image(std::string_view id) { return _id_to_image_data.count(id); }

ImageHandle get_image(std::string_view id) {
    CHECK(_id_to_image_data.count(id)) << id;
    const auto& image_data = _id_to_image_data.at(id);

    if (_id_to_cmap_data.count(id)) {
        CmapData& cmap_data = _id_to_cmap_data.at(id);
        cmap_data.frame_buffer.set_size(image_data.width, image_data.height);

        float adjusted_scale = cmap_data.scale;
        if (image_data.gl_texture.options.type ==
            gl::type_to_glenum<uint16_t>()) {
            adjusted_scale *= 65535;  // undo OpenGL normalization
        }

        shaders::draw_cmapped_texture(
            cmap_data.frame_buffer, image_data.gl_texture.id,
            gl::get_cmap_texture(cmap_data.cmap).id, cmap_data.min,
            cmap_data.max, adjusted_scale, cmap_data.invert);

        return ImageHandle{.texture = cmap_data.frame_buffer.color.id,
                           .width = image_data.width,
                           .height = image_data.height};
    } else {
        return {.texture = image_data.gl_texture.id,
                .width = image_data.width,
                .height = image_data.height};
    }
}

void draw_coordinate_frame(ConstFloatOrDoubleSeq tx_world_frame_data,
                           float scale) {
    Eigen::Matrix4f tx_world_frame;
    tx_world_frame_data.write_to(tx_world_frame);

    tx_world_frame.topLeftCorner<3, 3>() *= scale;

    Eigen::Matrix4f ndc_image_frame;  // mvp matrix
    _view_params.get_ndc_image_object(tx_world_frame, ndc_image_frame);

    gl::ProgramDrawInfo draw_info = gl::get_coordinate_frame();
    shaders::draw_colored_vertices(_view_params.frame_buffer, draw_info,
                                   ndc_image_frame);
};

void draw_coordinate_frame(std::string_view id, float scale) {
    CHECK(_id_to_tx_world_object.count(id)) << id;

    Eigen::Matrix4f tx_world_object = _id_to_tx_world_object.at(id);
    // scale the transform by premultiplying the scale matrix
    //
    // clang-format off
    // [ R, t ] [ sI 0] 
    // [ 0, 1 ] [  0 1] =
    //
    // [ sR, t ]
    // [  0, 1 ]
    // clang-format on
    // tx_world_object.topLeftCorner<3, 3>() *= scale;

    auto& tint_data = _id_to_tint[id];

    Eigen::Matrix4f ndc_image_object;  // mvp matrix
    _view_params.get_ndc_image_object(tx_world_object, ndc_image_object);

    gl::ProgramDrawInfo draw_info = gl::get_coordinate_frame();
    shaders::draw_colored_vertices(_view_params.frame_buffer, draw_info,
                                   ndc_image_object, /*point_size=*/0.0f,
                                   tint_data.color, tint_data.tint_amount);
    update_pick_buffer(string_to_u32(id), draw_info, ndc_image_object,
                       tx_world_object);
};

void draw_cone(std::string_view id,
               float base_scale,
               float height_scale,
               ConstAnySeq color) {
    CHECK(_id_to_tx_world_object.count(id)) << id;

    Eigen::Matrix4f tx_world_object = _id_to_tx_world_object.at(id);

    // scale the transform by premultiplying the scale matrix.
    // with bs = base_scale, hs = height_scale
    // S = diag(bs, bs, hs)
    //
    // clang-format off
    // [ R, t ] [ S, 0] 
    // [ 0, 1 ] [ 0, 1]
    //
    // [ RS, t ]
    // [  0, 1 ]
    // clang-format on
    tx_world_object.col(0) *= base_scale;
    tx_world_object.col(1) *= base_scale;
    tx_world_object.col(2) *= height_scale;

    auto& tint_data = _id_to_tint[id];

    Eigen::Matrix4f ndc_image_object;  // mvp matrix
    _view_params.get_ndc_image_object(tx_world_object, ndc_image_object);

    gl::ProgramDrawInfo draw_info = gl::get_cone();
    colors::RGBAf colorf = colors::infer_rgbaf(color);
    draw_info.vertex_array.set_default_float3(1, drop_alpha(colorf));
    draw_info.vertex_array.set_default_float(2, colorf.alpha);

    shaders::draw_colored_vertices(_view_params.frame_buffer, gl::get_cone(),
                                   ndc_image_object, /*point_size=*/0.0f,
                                   tint_data.color, tint_data.tint_amount);

    update_pick_buffer(string_to_u32(id), draw_info, ndc_image_object,
                       tx_world_object);
};

void draw_square_cone(std::string_view id,
                      float base_scale,
                      float height_scale,
                      ConstAnySeq color) {
    CHECK(_id_to_tx_world_object.count(id)) << id;

    Eigen::Matrix4f tx_world_object = _id_to_tx_world_object.at(id);

    // scale the transform by premultiplying the scale matrix.
    // with bs = base_scale, hs = height_scale
    // S = diag(bs, bs, hs)
    //
    // clang-format off
    // [ R, t ] [ S, 0] 
    // [ 0, 1 ] [ 0, 1]
    //
    // [ RS, t ]
    // [  0, 1 ]
    // clang-format on
    tx_world_object.col(0) *= base_scale;
    tx_world_object.col(1) *= base_scale;
    tx_world_object.col(2) *= height_scale;

    auto& tint_data = _id_to_tint[id];

    Eigen::Matrix4f ndc_image_object;  // mvp matrix
    _view_params.get_ndc_image_object(tx_world_object, ndc_image_object);

    gl::ProgramDrawInfo draw_info = gl::get_square_cone();
    colors::RGBAf colorf = colors::infer_rgbaf(color);
    draw_info.vertex_array.set_default_float3(1, drop_alpha(colorf));
    draw_info.vertex_array.set_default_float(2, colorf.alpha);

    shaders::draw_colored_vertices(_view_params.frame_buffer, draw_info,
                                   ndc_image_object, /*point_size=*/0.0f,
                                   tint_data.color, tint_data.tint_amount);

    update_pick_buffer(string_to_u32(id), draw_info, ndc_image_object,
                       tx_world_object);
};

PickInfo get_pick_info() { return _pick_info; };

bool was_clicked(std::string_view id) {
    if (string_to_u32(id) != _pick_info.id_hash) return false;
    if (!_current_io.MouseClicked[0]) return false;

    // the thing was clicked
    return true;
}

bool was_hovered(std::string_view id) {
    if (string_to_u32(id) != _pick_info.id_hash) return false;
    for (bool is_down : _current_io.MouseDown) {
        if (is_down) return false;
    }

    // the thing was hovered
    return true;
}

void draw_lines(std::string_view id1,
                std::string_view id2,
                Seq<const float> points1,
                Seq<const float> points2,
                ConstAnySeq color_or_colors) {
    CHECK_EQ(points1.size(), points2.size());
    CHECK_EQ(points1.size() % 3, 0);

    const auto num_lines = points1.size() / 3;

    Eigen::Matrix4f tx_world_object1;
    Eigen::Matrix4f tx_world_object2;
    get_tx_world_object(id1, tx_world_object1);
    get_tx_world_object(id2, tx_world_object2);

    static gl::Buffer points1_buffer{
        gl::BufferOptions().set_data_type<float>()};
    static gl::Buffer points2_buffer{
        gl::BufferOptions().set_data_type<float>()};
    points1_buffer.upload(points1);
    points2_buffer.upload(points2);

    static gl::VertexArray vertex_array;

    vertex_array.set_vertex_attribute_3d(0, points1_buffer);
    vertex_array.set_vertex_attribute_3d(1, points2_buffer);

    if (color_or_colors.empty()) {
        // no color provided. use default color
        vertex_array.set_default_float3(2, to_float(colors::red));
        vertex_array.set_default_float(3, 1.0f);  // alpha
    } else if ((color_or_colors.logical_size() == 3) ||
               (color_or_colors.logical_size() == 4)) {
        // single color
        colors::RGBAf colorf = colors::infer_rgbaf(color_or_colors);
        vertex_array.set_default_float3(2, drop_alpha(colorf));
        vertex_array.set_default_float(3, colorf.alpha);

    } else {
        // per-line color
        const size_t size_if_rgb = num_lines * 3;
        const size_t size_if_rgba = num_lines * 4;
        CHECK((color_or_colors.logical_size() == size_if_rgb) ||
              (color_or_colors.logical_size() == size_if_rgba));

        static gl::Buffer colors_buffer;
        colors_buffer.options.dynamic = true;
        colors_buffer.options.data_type =
            gl::typeid_to_glenum(color_or_colors.get_typeid());
        colors_buffer.upload(color_or_colors);

        if (color_or_colors.logical_size() == size_if_rgba) {
            vertex_array.set_vertex_attribute_3d(2, colors_buffer, 4,
                                                 0);  // rgb
            vertex_array.set_vertex_attribute_1d(3, colors_buffer, 4,
                                                 3);  // alpha
        } else {
            vertex_array.set_vertex_attribute_3d(2, colors_buffer);  // rgb
            vertex_array.set_default_float(3, 1.0f);                 // alpha
        }
    }

    Eigen::Matrix4f ndc_image_object1;
    Eigen::Matrix4f ndc_image_object2;
    _view_params.get_ndc_image_object(tx_world_object1, ndc_image_object1);
    _view_params.get_ndc_image_object(tx_world_object2, ndc_image_object2);

    shaders::draw_lines(_view_params.frame_buffer, vertex_array, num_lines,
                        ndc_image_object1, ndc_image_object2);
}

void draw_camera_space_image_3d(std::string_view id,
                                ConstFloatOrDoubleSeq tx_camera_imagepanel_data,
                                float mm_per_pixel) {
    ImageHandle image = get_image(id);

    Eigen::Matrix4f tx_camera_imagepanel;
    tx_camera_imagepanel_data.write_to(tx_camera_imagepanel);
    CM_Matrix4f ndc_image_camera(_view_params.ndc_image_camera);
    Eigen::Matrix4f ndc_image_imagepanel =
        ndc_image_camera * tx_camera_imagepanel;
    ndc_image_imagepanel.col(0) *= image.width * mm_per_pixel * 1e-3;
    ndc_image_imagepanel.col(1) *= image.height * mm_per_pixel * 1e-3;

    shaders::draw_texture(_view_params.frame_buffer, image.texture,
                          ndc_image_imagepanel);
}

void draw_world_space_image_3d(std::string_view id,
                               ConstFloatOrDoubleSeq tx_world_imagepanel,
                               float mm_per_pixel) {
    ImageHandle image = get_image(id);

    Eigen::Matrix4f ndc_image_imagepanel;
    _view_params.get_ndc_image_object(tx_world_imagepanel,
                                      ndc_image_imagepanel);
    ndc_image_imagepanel.col(0) *= image.width * mm_per_pixel * 1e-3;
    ndc_image_imagepanel.col(1) *= image.height * mm_per_pixel * 1e-3;

    shaders::draw_texture(_view_params.frame_buffer, image.texture,
                          ndc_image_imagepanel);
};

void add_model_part(std::string_view id,
                    std::string_view part_id,
                    Seq<const Mesh> meshes) {
    auto& model = _id_to_model[id];
    CHECK_EQ(model.count(part_id), 0) << id << ", " << part_id;

    auto& part = model[part_id];
    M_Matrix4f(part.tx_model_modelpart).setIdentity();
    for (const auto& mesh : meshes) {
        gl::Buffer xyzs_buff{
            gl::BufferOptions().set_data_type<float>().set_static_draw()};
        gl::Buffer rgbs_buff{
            gl::BufferOptions().set_data_type<float>().set_static_draw()};
        gl::Buffer normals_buff{
            gl::BufferOptions().set_data_type<float>().set_static_draw()};
        gl::Buffer faces_buff{gl::BufferOptions()
                                  .set_data_type<uint32_t>()
                                  .set_static_draw()
                                  .set_element_array_buffer()};

        xyzs_buff.upload(mesh.xyzs);
        rgbs_buff.upload(mesh.rgbs);
        normals_buff.upload(mesh.normals);
        faces_buff.upload(mesh.faces);

        MeshData mesh_data;
        mesh_data.num_items = mesh.faces.size();
        mesh_data.material = mesh.material;
        mesh_data.vertex_array.set_vertex_attribute_3d(0, xyzs_buff);
        if (rgbs_buff.length != 0) {
            mesh_data.vertex_array.set_vertex_attribute_3d(1, rgbs_buff);
        } else {
            mesh_data.vertex_array.set_default_float3(1,
                                                      to_float(colors::white));
        }
        mesh_data.vertex_array.set_vertex_attribute_3d(2, normals_buff);
        mesh_data.vertex_array.set_element_array(faces_buff);

        part.meshes.push_back(mesh_data);
    }
}

void update_tx_model_modelpart(std::string_view id,
                               std::string_view part_id,
                               ConstFloatOrDoubleSeq tx_model_modelpart_data) {
    CHECK(_id_to_model.contains(id)) << id;
    CHECK(_id_to_model.at(id).contains(part_id)) << part_id;
    tx_model_modelpart_data.write_to(
        _id_to_model.at(id).at(part_id).tx_model_modelpart);
}

void add_model(std::string_view id, Seq<const Mesh> meshes) {
    add_model_part(id, "", meshes);
};

void get_tx_world_modelpart(std::string_view id,
                            std::string_view part_id,
                            FloatOrDoubleSeq tx_world_modelpart_out) {
    CHECK(_id_to_model.count(id)) << id;
    auto& model = _id_to_model.at(id);
    CHECK(model.count(part_id)) << part_id;

    Eigen::Matrix4f tx_world_model;
    get_tx_world_object(id, tx_world_model);

    const Eigen::Matrix4f tx_world_modelpart =
        tx_world_model * model.at(part_id).tx_model_modelpart;

    tx_world_modelpart_out.copy_from(tx_world_modelpart);
};

void draw_model(std::string_view id) {
    CHECK(_id_to_model.count(id)) << id;
    auto& model = _id_to_model.at(id);

    Eigen::Matrix4f tx_world_model;
    get_tx_world_object(id, tx_world_model);

    for (const auto& [part_id, part] : model) {
        const Eigen::Matrix4f tx_world_modelpart =
            tx_world_model * part.tx_model_modelpart;

        Eigen::Matrix4f ndc_camera_modelpart;
        _view_params.get_ndc_camera_object(tx_world_modelpart,
                                           ndc_camera_modelpart);

        for (const auto& mesh : part.meshes) {
            shaders::draw_material_mesh(_view_params.frame_buffer,
                                        _view_params.ndc_image_camera,
                                        ndc_camera_modelpart, mesh.num_items,
                                        mesh.vertex_array, mesh.material);
        }
    }
};

void set_model_material(std::string_view id, const MeshMaterial& material) {
    CHECK(_id_to_model.count(id)) << id;
    auto& model = _id_to_model.at(id);
    for (auto& [part_id, part] : model) {
        for (auto& mesh : part.meshes) {
            mesh.material = material;
        }
    }
};

}  // namespace viewer
}  // namespace axby
