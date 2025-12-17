#pragma once

#include <glad/gl.h>

#include "texture.h"

namespace axby {
namespace gl {

void stash_prev_framebuffer();
void unstash_prev_framebuffer();

struct FrameBufferInfo {
    GLuint id = 0;
    int width = 0;
    int height = 0;

    bool have_depth = false;
    bool is_integral = false;
};

struct FrameBuffer : public FrameBufferInfo {
    GLuint depth_id = 0;
    Texture color = {};

    FrameBuffer();

    FrameBuffer(const TextureOptions& color_options,
                int width,
                int height,
                bool with_depth = false);

    // default texture options (RGB uint8) for color buffer
    FrameBuffer(int width, int height, bool with_depth = false);

    void set_size(int width, int height);

    void clear(float r = 0, float g = 0, float b = 0, float a = 0);

    // if the underlying color buffer is integral as in the case
    // of a pick buffer
    void clear_ui(uint32_t r = 0,
                  uint32_t g = 0,
                  uint32_t b = 0,
                  uint32_t a = 0);
};

}  // namespace gl
}  // namespace axby
