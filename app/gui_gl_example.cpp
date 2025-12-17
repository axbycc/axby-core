#include <glad/gl.h>

#include <Eigen/Dense>
#include <iostream>

#include "app/gui.h"
#include "app/main.h"
#include "axgl/buffer.h"
#include "axgl/program.h"
#include "axgl/vertex_array.h"
#include "shaders/colored_vertices.h"

static const char* kVertexShader = R"(
#version 430 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

void main() {
    vUV = aUV;
    // gl_Position = vec4(aPos, 0.0, 1.0);
    gl_Position = vec4(0.1, 0.2, 0.0, 1.0);
    gl_PointSize = 10.0;  // make points large enough to see
}
)";

static const char* kFragmentShader = R"(
#version 430 core
out vec4 FragColor;
in vec2 vUV;

void main() {
    // Orange rectangle
    FragColor = vec4(1.0, 0.5, 0.0, 1.0);
}
)";

// Utility to check shader errors
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
    }
    return s;
}

static const float quadVertices[] = {
    // pos      // uv
    -0.3f, -0.8f, 0.f, 0.f, 0.5f, -0.6f, 0.9f, 0.f, 1.f,   0.9f,  1.f, 1.f,

    -0.7f, -0.9f, 0.f, 0.f, 1.f,  1.f,   1.f,  1.f, -0.7f, 0.75f, 0.f, 1.f};

void gl_setup(GLuint& prog, GLuint& vao) {
    // Build program
    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    // Cleanup shaders after linking
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Create VAO/VBO
    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
}

void alt_gl_setup(axby::gl::Program& program,
                  axby::gl::VertexArray& varray,
                  axby::gl::Buffer& buffer) {
    using namespace axby;
    program =
        axby::gl::Program(gl::ProgramSource(kVertexShader, kFragmentShader));

    buffer = axby::gl::Buffer(
        gl::BufferOptions().set_data_type<float>().set_static_draw());
    buffer.upload(quadVertices);
}

int main(int argc, char** argv) {
    __APP_MAIN_INIT__;

    axby::gui_init("GL Example");

    axby::gl::Program program;
    axby::gl::Buffer buffer;
    axby::gl::VertexArray vertex_array;
    alt_gl_setup(program, vertex_array, buffer);

    // Main loop
    while (!axby::gui_wants_quit()) {
        axby::gui_loop_begin();

        vertex_array.set_vertex_attribute_2d(0, buffer, /*logical_stride=*/4);
        vertex_array.set_vertex_attribute_2d(1, buffer, /*logical_stride=*/4,
                                             /*logical_offset=*/2);
        vertex_array.set_default_float(2, 1.0f);

        Eigen::Matrix4f mvp = Eigen::Matrix4f::Identity();
        auto [width, height] = axby::gui_window_size();

        axby::gl::FrameBuffer default_frame_buffer;
        default_frame_buffer.width = width;
        default_frame_buffer.height = height;

        axby::gl::ProgramDrawInfo draw_info;
        draw_info.vertex_array = vertex_array;
        draw_info.num_items = 6;
        draw_info.draw_mode = GL_POINTS;
            
        axby::shaders::draw_colored_vertices(default_frame_buffer, draw_info, mvp);

        axby::gui_loop_end();
    }

    axby::gui_cleanup();

    return 0;
}
