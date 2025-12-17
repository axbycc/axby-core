#include "program.h"

#include <glad/mx_shim.h>

#include "absl/container/flat_hash_set.h"
#include "debug/check.h"
#include "debug/log.h"

namespace axby {
namespace gl {

Program::Program(ProgramSource src, const char* debug_name) {
    this->debug_name = debug_name;

    const char* vshader = src.vshader;
    const char* fshader = src.fshader;
    const char* gshader = src.gshader;

    CHECK(vshader != nullptr)
        << "vshader=" << (void*)vshader << ", fshader=" << (void*)fshader;
    CHECK(fshader != nullptr)
        << "vshader=" << (void*)vshader << ", fshader=" << (void*)fshader;

    char error[512];  // error message
    GLint success = GL_FALSE;

    id = glCreateProgram();
    const GLuint vid = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vid, 1, &vshader, nullptr);
    glCompileShader(vid);
    glGetShaderiv(vid, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        glGetShaderInfoLog(vid, 512, NULL, error);
        LOG(FATAL) << "\n" << vshader << "\n" << error;
    }

    glAttachShader(id, vid);
    glDeleteShader(vid);

    const GLuint fid = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fid, 1, &fshader, nullptr);
    glCompileShader(fid);
    glGetShaderiv(fid, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        glGetShaderInfoLog(fid, 512, NULL, error);
        LOG(FATAL) << "\n" << fshader << "\n" << error;
        LOG(FATAL) << error;
    }

    glAttachShader(id, fid);
    glDeleteShader(fid);

    if (gshader) {
        const GLuint gid = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gid, 1, &gshader, nullptr);
        glCompileShader(gid);
        glGetShaderiv(gid, GL_COMPILE_STATUS, &success);
        if (success != GL_TRUE) {
            glGetShaderInfoLog(gid, 512, NULL, error);
            LOG(FATAL) << "\n" << gshader << "\n" << error;
            LOG(FATAL) << error;
        }

        glAttachShader(id, gid);
        glDeleteShader(gid);
    }

    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        glGetProgramInfoLog(id, 512, NULL, error);
        LOG(FATAL) << "(" << debug_name << ") Link error " << error;
    }
}

// to prevent blasting opengl warnings on every frame, we keep track
// about which missing locations have been warned about
absl::flat_hash_set<std::pair<int, std::string>> PROGRAM_LOCATION_WARNINGS;

int Program::get_uniform_location(const char* name) {
    CHECK(id != 0) << "program was not initialized";
    const int location = glGetUniformLocation(id, name);

    if (!PROGRAM_LOCATION_WARNINGS.count(std::make_pair(id, name))) {
        LOG_IF(WARNING, location < 0)
            << "uniform " << name << " is invalid or unused for program named "
            << debug_name;
        PROGRAM_LOCATION_WARNINGS.insert(std::make_pair(id, name));
    }
    return location;
}

void Program::set_texture_unit(const char* name, int unit) {
    CHECK(id != 0) << "program was not initialized";
    const int location = get_uniform_location(name);
    if (location < 0) return;
    glUseProgram(id);
    glUniform1i(location, unit);
    glUseProgram(0);
}

void Program::set_int(const char* name, int value) {
    CHECK(id != 0) << "program was not initialized";

    const int location = get_uniform_location(name);
    if (location < 0) return;

    glUseProgram(id);
    glUniform1i(location, value);
    glUseProgram(0);
}

void Program::set_uint(const char* name, uint32_t value) {
    CHECK(id != 0) << "program was not initialized";

    const int location = get_uniform_location(name);
    if (location < 0) return;

    glUseProgram(id);
    glUniform1ui(location, value);
    glUseProgram(0);
}

void Program::set_float(const char* name, float value) {
    CHECK(id != 0) << "program was not initialized";

    const int location = get_uniform_location(name);
    if (location < 0) return;

    glUseProgram(id);
    glUniform1f(location, value);
    glUseProgram(0);
}

void Program::set_vec(const char* name, axby::Seq<const float> values) {
    CHECK(id != 0) << "program was not initialized";

    const int location = get_uniform_location(name);
    if (location < 0) return;

    glUseProgram(id);

    const int num_elements = values.size();
    CHECK(num_elements >= 1 && num_elements <= 4);

    if (num_elements == 1) {
        glUniform1f(location, values[0]);
    } else if (values.size() == 2) {
        glUniform2f(location, values[0], values[1]);
    } else if (values.size() == 3) {
        glUniform3f(location, values[0], values[1], values[2]);
    } else {
        glUniform4f(location, values[0], values[1], values[2], values[3]);
    }

    glUseProgram(0);
}

void Program::set_vec2(const char* name, float x, float y) {
    CHECK(id != 0) << "program was not initialized";

    const int location = get_uniform_location(name);
    if (location < 0) return;

    glUseProgram(id);
    glUniform2f(location, x, y);
    glUseProgram(0);
}

void Program::set_vec4(const char* name, float x, float y, float z, float w) {
    CHECK(id != 0) << "program was not initialized";

    const int location = get_uniform_location(name);
    if (location < 0) return;

    glUseProgram(id);
    glUniform4f(location, x, y, z, w);
    glUseProgram(0);
}

void Program::set_vec3(const char* name, float x, float y, float z) {
    CHECK(id != 0) << "program was not initialized";

    const int location = get_uniform_location(name);
    if (location < 0) return;

    glUseProgram(id);
    glUniform3f(location, x, y, z);
    glUseProgram(0);
}

void Program::set_mat4(const char* name,
                       axby::Seq<const float> values,
                       bool row_major) {
    CHECK(id != 0) << "program was not initialized";
    CHECK_EQ(values.size(), 4 * 4);

    const int location = get_uniform_location(name);
    if (location < 0) return;

    glUseProgram(id);
    glUniformMatrix4fv(location, /*count=*/1, row_major, values.data());
    glUseProgram(0);
}

void Program::draw(const FrameBufferInfo& frame_buffer,
                   const ProgramDrawInfo& draw_info) {
    CHECK(id != 0);

    // save previous viewport
    stash_prev_framebuffer();

    // begin: try to fix opengl's horrible global state
    glDisable(GL_SCISSOR_TEST);
    if (frame_buffer.have_depth) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    if (draw_info.want_blend) {
        CHECK(!frame_buffer.is_integral);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
    } else {
        glDisable(GL_BLEND);
    }
    glViewport(0, 0, frame_buffer.width, frame_buffer.height);
    // end: try to fix opengl's horrible global state

    for (int i = 0; i < draw_info.textures.size(); ++i) {
        const int texture_id = draw_info.textures[i];
        if (texture_id == 0) continue;
        const auto ACTIVE_TEXTURE = GL_TEXTURE0 + i;
        glActiveTexture(ACTIVE_TEXTURE);
        glBindTexture(GL_TEXTURE_2D, texture_id);
    }
    glBindVertexArray(draw_info.vertex_array.id);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
    glUseProgram(id);
    if (draw_info.num_vertices_per_instance > 0) {
        // instanced mode
        if (draw_info.vertex_array.ebo_data_type) {
            LOG(FATAL) << "draw elements instanced not supported yet";
        } else {
            glDrawArraysInstanced(draw_info.draw_mode, 0,
                                  draw_info.num_vertices_per_instance,
                                  draw_info.num_items);
        }
    } else {
        // non-instanced mode
        if (draw_info.vertex_array.ebo_data_type) {
            glDrawElements(draw_info.draw_mode, draw_info.num_items,
                           draw_info.vertex_array.ebo_data_type, 0);
        } else {
            glDrawArrays(draw_info.draw_mode, 0, draw_info.num_items);
        }
    }
    glBindVertexArray(0);
    glUseProgram(0);

    // restore viewport
    unstash_prev_framebuffer();
};

}  // namespace gl
}  // namespace axby
