#include <glad/gl.h>

#include <cstdint>
#include <cstdio>
#include <vector>

#include "app/gui.h"
#include "app/main.h"
#include "axgl/frame_buffer.h"
#include "axgl/program.h"
#include "axgl/texture.h"
#include "axgl/vertex_array.h"

// Assume you already have a valid OpenGL context and
// helper functions: compileShader, linkProgram, etc.

using namespace axby;
static const int kWidth = 10;
static const int kHeight = 10;

#include <algorithm>  // std::clamp
#include <cstdint>
#include <cstdio>
#include <vector>

// ---------- Utility: create RG32UI pick FBO ----------

gl::FrameBuffer create_pick_fbo() {
    gl::TextureOptions opts;
    opts.type = GL_UNSIGNED_INT;
    opts.format = GL_RG_INTEGER;
    opts.internal_format = GL_RG32UI;
    gl::FrameBuffer frame_buffer{opts, kWidth, kHeight};
    return frame_buffer;
}

GLuint create_pick_fbo(GLuint& out_tex) {
    gl::TextureOptions opts;
    opts.min_filter = GL_NEAREST;
    opts.mag_filter = GL_NEAREST;
    opts.type = GL_UNSIGNED_INT;
    opts.format = GL_RG_INTEGER;
    opts.internal_format = GL_RG32UI;

    gl::Texture tex{opts, kWidth, kHeight};
    out_tex = tex.id;

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           out_tex, 0);

    GLenum bufs[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, bufs);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "Pick FBO incomplete: 0x%X\n", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;
}

// ---------- Pick program: writes uvec2(group_id, point_id) ----------
gl::Program create_pick_program() {
    const char* vs_src = R"GLSL(
        #version 430 core

        layout(location = 0) in vec3 in_position;

        flat out uint v_point_id;
        flat out uint v_group_id;

        uniform uint u_group_id;

        void main() {
            // Force everything to the center of clip space
            gl_Position = vec4(in_position, 1.0);
            gl_PointSize = 1.0;  // we'll use fixed-size points

            v_point_id = uint(gl_VertexID);
            v_group_id = u_group_id;
        }
    )GLSL";

    const char* fs_src = R"GLSL(
        #version 430 core

        flat in uint v_point_id;
        flat in uint v_group_id;

        // Render into GL_RG32UI framebuffer
        layout(location = 0) out uvec2 out_id;

        void main() {
            out_id = uvec2(v_group_id, v_point_id);
        }
    )GLSL";

    gl::Program prog{{vs_src, fs_src}, "pick program"};

    return prog;
}
// ---------- Debug program: RG32UI usampler2D -> vec3 color ----------

gl::Program create_debug_program() {
    const char* vs_src =
        R"GLSL(

#version 430

const vec2 positions[6] = vec2[6](
    // triangle 1
    vec2(-0.5f, -0.5f), 
    vec2( 0.5f, -0.5f),
    vec2(-0.5f,  0.5f),
    // triangle 2
    vec2(-0.5f,  0.5f), 
    vec2( 0.5f, -0.5f),
    vec2( 0.5f,  0.5f)
);

out vec2 texcoord;

void main() {
    gl_PointSize = 10.0;  // make points large enough to see
    vec2 quadcoord = positions[gl_VertexID];
    gl_Position = vec4(quadcoord*2, 0.0f, 1.0f); // full screen
    texcoord = (quadcoord + 0.5); // (-0.5 to 0.5) => (0 to 1)
}

)GLSL";
    const char* fs_src = R"GLSL(
#version 430

in vec2 texcoord;

layout(binding = 0) uniform usampler2D vertex_ids_texture;
layout(location = 0) out vec3 out_color;

uint wang_hash(uint x) {
    x = (x ^ 61u) ^ (x >> 16u);
    x *= 9u;
    x = x ^ (x >> 4u);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15u);
    return x;
}

vec3 id_to_color(uint id) {
    uint h = wang_hash(id);
    vec3 c = vec3(
        float((h >>  0) & 255u) / 255.0,
        float((h >>  8) & 255u) / 255.0,
        float((h >> 16) & 255u) / 255.0
        );
    // Bias a bit toward brighter colors
    c = mix(c, vec3(1.0), 0.25);
    return c;
}


void main() {
    uvec2 ids = texture(vertex_ids_texture, texcoord).xy;
    uint group_id = ids.x;
    uint point_id = ids.y;
    vec3 group_color = id_to_color(group_id);
    vec3 point_color = id_to_color(point_id);
    vec3 rgb = mix(group_color, point_color, 0.1);
    out_color = rgb;
}


)GLSL";

    return gl::Program{gl::ProgramSource{vs_src, fs_src}};
}

// ---------- ASCII printers ----------

void PrintRGBAsciiGrayscale(const std::vector<float>& rgb,
                            int width,
                            int height) {
    auto idx = [&](int x, int y) { return (std::size_t(y) * width + x) * 3; };

    // Dark → light
    static const char* ramp = " .:-=+*#%@";
    const int ramp_len = 10;  // strlen(ramp)

    std::printf("ASCII grayscale grid from RGB debug texture:\n\n");

    for (int y = height - 1; y >= 0; --y) {
        std::printf("%2d: ", y);
        for (int x = 0; x < width; ++x) {
            std::size_t i = idx(x, y);

            float r = rgb[i + 0];
            float g = rgb[i + 1];
            float b = rgb[i + 2];

            // Clamp to [0,1] just in case
            r = std::clamp(r, 0.0f, 1.0f);
            g = std::clamp(g, 0.0f, 1.0f);
            b = std::clamp(b, 0.0f, 1.0f);

            // Perceptual luminance (rough BT.709)
            float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;

            int idx_char = static_cast<int>(lum * (ramp_len - 1) + 0.5f);
            idx_char = std::clamp(idx_char, 0, ramp_len - 1);

            char c = ramp[idx_char];
            std::printf("%c", c);
        }
        std::printf("\n");
    }
    std::printf("\n");
}

void PrintPickAscii(const std::vector<uint32_t>& buffer,
                    int width,
                    int height,
                    uint32_t expected_group) {
    std::printf("ASCII grid (X = group %u, ' ' = group 0):\n\n",
                expected_group);
    for (int y = height - 1; y >= 0; --y) {
        std::printf("%2d: ", y);
        for (int x = 0; x < width; ++x) {
            std::size_t idx = (std::size_t(y) * width + x) * 2;
            uint32_t group_id = buffer[idx + 0];
            char c = (group_id == expected_group) ? 'X' : ' ';
            std::printf("%c", c);
        }
        std::printf("\n");
    }
    std::printf("\n");
}

void PrintRGBAscii(const std::vector<float>& rgb, int width, int height) {
    auto idx = [&](int x, int y) { return (std::size_t(y) * width + x) * 3; };

    std::printf("ANSI color grid (each pixel as colored block):\n\n");
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            std::size_t i = idx(x, y);
            float r = rgb[i + 0];
            float g = rgb[i + 1];
            float b = rgb[i + 2];

            int R = int(std::clamp(r, 0.0f, 1.0f) * 255.0f);
            int G = int(std::clamp(g, 0.0f, 1.0f) * 255.0f);
            int B = int(std::clamp(b, 0.0f, 1.0f) * 255.0f);

            std::printf("\033[48;2;%d;%d;%dm  \033[0m", R, G, B);
        }
        std::printf("\n");
    }
    std::printf("\n");
}

// ---------- Main test ----------

void run_full_pick_test() {
    // Pick FBO / texture
    GLuint pick_tex = 0;
    GLuint pick_fbo = create_pick_fbo(pick_tex);

    gl::FrameBuffer debug_fbo{
        gl::TextureOptions().set_data_type<float>().set_rgb(), kWidth, kHeight};

    // Simple VAO/VBO with one point
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    float one_point[3] = {0.0f, 0.0f, 0.0f};
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(one_point), one_point, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)0);

    glBindVertexArray(0);

    // Programs
    gl::Program gl_pick_prog = create_pick_program();
    const uint32_t expected_group = 123u;
    gl_pick_prog.set_uint("u_group_id", expected_group);

    GLuint pick_prog = gl_pick_prog.id;
    gl::Program debug_program = create_debug_program();

    // ---------- Pass 1: render pick IDs into RG32UI ----------

    glBindFramebuffer(GL_FRAMEBUFFER, pick_fbo);
    glViewport(0, 0, kWidth, kHeight);

    GLuint clear_vals[2] = {0u, 0u};
    glClearBufferuiv(GL_COLOR, 0, clear_vals);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    glUseProgram(pick_prog);
    glBindVertexArray(vao);
    glPointSize(1.0f);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---------- Download pick IDs and print ASCII X grid ----------

    std::vector<uint32_t> pick_buffer(kWidth * kHeight * 2);

    glBindFramebuffer(GL_FRAMEBUFFER, pick_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, kWidth, kHeight, GL_RG_INTEGER, GL_UNSIGNED_INT,
                 pick_buffer.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    PrintPickAscii(pick_buffer, kWidth, kHeight, expected_group);

    // ---------- Pass 2: integer texture -> RGB debug texture ----------
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    debug_fbo.clear();
    gl::ProgramDrawInfo info;
    info.draw_mode = GL_TRIANGLES;
    info.num_items = 6;
    info.vertex_array.id = vao;
    info.textures[0] = pick_tex;
    debug_program.draw(debug_fbo, info);

    // ---------- Download RGB debug texture and ASCII-color print ----------

    std::vector<float> rgb_buffer(kWidth * kHeight * 3);
    debug_fbo.color.download(rgb_buffer);
    PrintRGBAscii(rgb_buffer, kWidth, kHeight);
}


int main(int argc, char* argv[]) {
    __APP_MAIN_INIT__;
    axby::gui_init("sanity");

    run_full_pick_test();

    axby::gui_cleanup();

    return 0;
}
