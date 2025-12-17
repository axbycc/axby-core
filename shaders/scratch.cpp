#include <glad/gl.h>
#include <imgui.h>

#include <Eigen/Dense>
#include <iostream>
#include <random>
#include <vector>

#include "app/gui.h"
#include "app/main.h"
#include "debug/check.h"
#include "debug/log.h"
#include "axgl/buffer.h"
#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "axgl/vertex_array.h"
#include "shaders/colored_vertex_ids.h"
#include "shaders/colored_vertices.h"
#include "shaders/debug_vertex_ids.h"
#include "shaders/lines.h"
#include "shaders/vertex_ids.h"
#include "viewer/viewer.h"

std::random_device _rd;
std::mt19937 _gen(_rd());
std::uniform_real_distribution<float> _dis(-1, 1);

#include <cmath>
#include <cstdio>
#include <cstdlib>

// Single-file example: instanced line rendering with N star-like lines.
// - Two endpoint buffers (p0 and p1).
// - N lines all intersect in the center like a star.
// - No model/view/projection matrix.
// - Lines are constant red.
// Assumes a valid OpenGL context already exists.
// Call run() from your render loop.

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace axby;

struct Vec3 {
    float x, y, z;
};

// Number of star rays
static const int NUM_LINES = 16;

// Globals
static GLuint gProgram = 0;
static GLuint gVAO = 0;
static bool gInitDone = false;

static gl::Program glProgram;
static gl::VertexArray glVertexArray;

//--------------------------------------------------------------------------------------
// Initialization: shaders + VAO/VBOs
//--------------------------------------------------------------------------------------

static void initLines() {
    if (gInitDone) return;

    const char* vsSrc = R"(

#version 430

uniform mat4 mvp1 = mat4(1.0); // model view projection of points 1
uniform mat4 mvp2 = mat4(1.0); // model view projection of points 2

layout(location = 0) in vec3 point1; 
layout(location = 1) in vec3 point2;
layout(location = 2) in vec3 color;
layout(location = 3) in float alpha;
 
out vec4 v_color;

void main()
{
    v_color = vec4(color, alpha);

    vec3 pos;
    mat4 mvp;    
    if (gl_VertexID == 0) {
        pos = point1;
        mvp = mvp1;
    } else {
        pos = vec3(0.5,0.5,0.5);
        pos = point2;
        mvp = mvp2;
    }

    // mvp = mat4(1.0); // debug

    gl_Position = mvp * vec4(pos, 1.0);
}

    )";

    const char* fsSrc = R"(
#version 430

in vec4 v_color;
out vec4 out_color;

void main()
{
    // out_color = v_color;
out_color = vec4(1, 0,0,1);
}

    )";

    glProgram = {{vsSrc, fsSrc}, "prog"};
    gProgram = glProgram.id;

    Vec3 endpoints0[NUM_LINES];
    Vec3 endpoints1[NUM_LINES];

    const float R = 0.9f;
    const float TWO_PI = 6.28318530717958647692f;

    for (int i = 0; i < NUM_LINES; ++i) {
        float t = (TWO_PI * i) / float(NUM_LINES);
        float cx = std::cos(t);
        float sy = std::sin(t);

        float x = R * cx;
        float y = R * sy;

        endpoints0[i] = {x, y, 0.0f};
        endpoints1[i] = {-x, -y, 0.0f};
    }

    std::vector<float> endpoints0_flat;
    std::vector<float> endpoints1_flat;
    for (int i = 0; i < NUM_LINES; ++i) {
        endpoints0_flat.push_back(endpoints0[i].x);
        endpoints0_flat.push_back(endpoints0[i].y);
        endpoints0_flat.push_back(endpoints0[i].z);
        endpoints1_flat.push_back(endpoints1[i].x);
        endpoints1_flat.push_back(endpoints1[i].y);
        endpoints1_flat.push_back(endpoints1[i].z);
    }

    gl::Buffer e0_buf{gl::BufferOptions().set_data_type<float>()};
    e0_buf.upload(endpoints0_flat);

    gl::Buffer e1_buf{gl::BufferOptions().set_data_type<float>()};
    e1_buf.upload(endpoints1_flat);

    glVertexArray.ensure_initted();
    glVertexArray.set_vertex_attribute_3d(0, e0_buf);
    glVertexArray.set_vertex_attribute_3d(1, e1_buf);
    glVertexArray.set_default_float3(2, to_float(colors::red));
    glVertexArray.set_default_float(3, 1.0f);

    for (int i = 0; i < 4; ++i) {
        glVertexArray.set_divisor(i, 1);
    }
    
    gVAO = glVertexArray.id;

    gInitDone = true;
}

//--------------------------------------------------------------------------------------
// Render: draws the star using instanced lines
//--------------------------------------------------------------------------------------

static void renderLines() {
    if (!gInitDone) return;

    gl::ProgramDrawInfo draw;
    draw.draw_mode = GL_LINES;
    draw.num_items = NUM_LINES;
    draw.num_vertices_per_instance = 2;
    draw.vertex_array = glVertexArray;

    gl::FrameBuffer fb;
    std::tie(fb.width, fb.height) = gui_window_size();
    glProgram.draw(fb, draw);
    
    // glUseProgram(gProgram);
    // glBindVertexArray(gVAO);
    // glLineWidth(2.0f);
    // const GLsizei vertexCount = 2;  // vertices per instance (one line)
    // const GLsizei instanceCount = NUM_LINES;  // number of star rays
    // glDrawArraysInstanced(GL_LINES, 0, vertexCount, instanceCount);
    // glBindVertexArray(0);
    // glUseProgram(0);
}

//--------------------------------------------------------------------------------------
// Public entry point: call this from your render loop
//--------------------------------------------------------------------------------------

void run() {
    if (!gInitDone) {
        initLines();
    }

    // Optional clear:
    // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderLines();
}

int main(int argc, char* argv[]) {
    __APP_MAIN_INIT__;

    // float endpoints0[6] = {
    //     -0.8f, -0.8f, 0.8f,  // line 0 start
    //     -0.8f, 0.8f,  0.6f,  // line 1 start
    // };

    // float endpoints1[6] = {
    //     0.8f, 0.8f,  0.8f,  // line 0 end
    //     0.8f, -0.8f, 0.9f,  // line 1 end
    // };

    axby::gui_init("shaders/scratch", 1024, 1024);
    axby::viewer::init();

    // Eigen::Matrix4f pose = Eigen::Matrix4f::Identity();
    // pose(2, 3) = 0.3;
    // axby::viewer::update_tx_world_object("lines", pose);

    while (!axby::gui_wants_quit()) {
        axby::gui_loop_begin();
        // axby::viewer::new_frame(ImGui::GetIO());
        // axby::viewer::draw_lines("lines", "lines", endpoints0, endpoints1,
        //                          axby::colors::blue);

        run();

        axby::gui_loop_end();
    }
    axby::gui_cleanup();

    return 0;
}
