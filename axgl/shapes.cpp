#include <string_view>

#include "debug/log.h"
#include "axgl/info.h"
#include "program.h"
#include "vertex_array.h"
#include "colors/colors.h"

namespace axby {
namespace gl {

const VertexArray& get_quad() {
    static VertexArray quad;

    if (quad.id == 0) {
        const float vertices[] = {
            0.5f,  0.5f,  0.0f,  // top right
            0.5f,  -0.5f, 0.0f,  // bottom right
            -0.5f, -0.5f, 0.0f,  // bottom left
            -0.5f, 0.5f,  0.0f,  // top left
        };
        const uint8_t indices[] = {
            0, 1, 3,  // first Triangle
            1, 2, 3   // second Triangle
        };

        Buffer vbo =
            Buffer(BufferOptions().set_data_type<float>().set_static_draw());
        vbo.upload(vertices);
        Buffer ebo = Buffer(BufferOptions()
                                .set_data_type<uint8_t>()
                                .set_element_array_buffer());
        ebo.upload(indices);

        quad.set_vertex_attribute_3d(0, vbo);
        quad.set_element_array(ebo);
    }

    return quad;
}

const ProgramDrawInfo& get_coordinate_frame() {
    static ProgramDrawInfo draw_info;
    if (draw_info.vertex_array.id == 0) {
        VertexArray coordinate_frame;
        const std::array<float, 3 * 2 * 3> coordinate_frame_vertices = {
            // clang-format off
            // x axis
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            // y axis
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            // z axis
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
            // clang-format on
        };
        const std::array<uint8_t, 3 * 2 * 3> coordinate_frame_colors = {
            // clang-format off
            // x = red
            255, 0, 0,
            255, 0, 0,
            // y = green
            0, 255, 0,
            0, 255, 0,
            // z = blue
            0, 0, 255,
            0, 0, 255,
            // clang-format on
        };

        Buffer vbo_xyz(
            BufferOptions().set_data_type<float>().set_static_draw());
        vbo_xyz.upload(coordinate_frame_vertices);

        Buffer vbo_rgb(BufferOptions()
                           .set_data_type<uint8_t>()
                           .set_static_draw());
        vbo_rgb.upload(coordinate_frame_colors);

        coordinate_frame.set_vertex_attribute_3d(0, vbo_xyz);
        coordinate_frame.set_vertex_attribute_3d(1, vbo_rgb);

        coordinate_frame.set_default_float(2, 1.0f); // alpha

        draw_info.draw_mode = GL_LINES;
        draw_info.vertex_array = coordinate_frame;
        draw_info.num_items = 6;  // 3 axes, two vertices per axis
    }
    

    return draw_info;
}

const ProgramDrawInfo& get_square_cone() {
    static ProgramDrawInfo draw_info;

    if (draw_info.vertex_array.id == 0) {
        VertexArray cone;
        // square pyramid cone, pointing to the origin along the z axis
        const std::array<float, 3 * 5> cone_vertices = {
            // clang-format off
            // square base
            -0.5f, -0.5f, -1.0f, //0
            -0.5f, 0.5f, -1.0f, //1
            0.5f, 0.5f, -1.0f, //2
            0.5f, -0.5f, -1.0f, //3
            // cone tip
            0.0f, 0.0f, 0.0f, //4
            // clang-format on
        };
        const std::array<uint8_t, 3 * 6> cone_indices = {
            // clang-format off
            0, 1, 3, // base triangle 1
            1, 2, 3, // base triangle 2
            0, 1, 4,
            1, 2, 4,
            1, 3, 4,
            3, 0, 4,
            // clang-format on
        };

        Buffer vbo(BufferOptions().set_data_type<float>().set_static_draw());
        vbo.upload(cone_vertices);

        Buffer ebo(BufferOptions()
                       .set_data_type<uint8_t>()
                       .set_static_draw()
                       .set_element_array_buffer());
        ebo.upload(cone_indices);

        cone.set_vertex_attribute_3d(0, vbo);
        cone.set_element_array(ebo);

        draw_info.vertex_array = cone;
        draw_info.draw_mode = GL_TRIANGLES;
        draw_info.num_items = ebo.length;  // num triangle indices
    }

    return draw_info;
}

const ProgramDrawInfo& get_cone() {
    static ProgramDrawInfo draw_info;

    if (draw_info.vertex_array.id == 0) {
        constexpr int num_sides = 20;
        VertexArray cone;
        // square pyramid cone, pointing to the origin along the z axis
        std::vector<float> cone_vertices = {};
        for (int i = 0; i < num_sides; ++i) {
            const float angle = float(2*M_PI*i)/num_sides;
            cone_vertices.push_back(0.5f*std::cos(angle));
            cone_vertices.push_back(0.5f*std::sin(angle));
            cone_vertices.push_back(-1.0f);
        }

        cone_vertices.push_back(0);
        cone_vertices.push_back(0);
        cone_vertices.push_back(-1);

        cone_vertices.push_back(0);
        cone_vertices.push_back(0);
        cone_vertices.push_back(0);

        std::vector<uint8_t> cone_indices;
        for (int i = 0; i < num_sides; ++i) {
            // tilted slab from base side to cone tip
            cone_indices.push_back(i);
            cone_indices.push_back((i+1) % num_sides);
            cone_indices.push_back(num_sides);

            // flat slab from base side to base center
            cone_indices.push_back(i);
            cone_indices.push_back((i+1) % num_sides);
            cone_indices.push_back(num_sides+1);
        }

        // = {
        //     // clang-format off
        //     0, 1, 3, // base triangle 1
        //     1, 2, 3, // base triangle 2
        //     0, 1, 4,
        //     1, 2, 4,
        //     1, 3, 4,
        //     3, 0, 4,
        //     // clang-format on
        // };

        Buffer vbo(BufferOptions().set_data_type<float>().set_static_draw());
        vbo.upload(cone_vertices);

        Buffer ebo(BufferOptions()
                       .set_data_type<uint8_t>()
                       .set_static_draw()
                       .set_element_array_buffer());
        ebo.upload(cone_indices);

        cone.set_vertex_attribute_3d(0, vbo);
        cone.set_element_array(ebo);

        draw_info.vertex_array = cone;
        draw_info.draw_mode = GL_TRIANGLES;
        draw_info.num_items = ebo.length;  // num triangle indices
    }

    return draw_info;
}

}  // namespace gl
}  // namespace axby
