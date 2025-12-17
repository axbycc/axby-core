#include "frame_buffer.h"

#include <glad/mx_shim.h>
#include "debug/log.h"
#include "info.h"

namespace axby {
namespace gl {

GLint prev_viewport[4] = {0};
GLint prev_framebuffer_id = -1;

void stash_prev_framebuffer() {
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_framebuffer_id);
    glGetIntegerv(GL_VIEWPORT, prev_viewport);
}

void unstash_prev_framebuffer() {
    CHECK(prev_framebuffer_id >= 0) << "double unstash?";
    glBindFramebuffer(GL_FRAMEBUFFER, prev_framebuffer_id);
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2],
               prev_viewport[3]);
    prev_framebuffer_id = -1;
}

FrameBuffer::FrameBuffer() {
    // default framebuffer. you still need to set the width, height
    // before rendering
    id = 0;
    have_depth = true;
}

FrameBuffer::FrameBuffer(int width, int height, bool with_depth)
    : FrameBuffer(
          TextureOptions().set_data_type<uint8_t>().set_rgb(), width, height) {}

void FrameBuffer::set_size(int new_width, int new_height) {
    color.ensure_size(new_width, new_height);
    width = new_width;
    height = new_height;
}

FrameBuffer::FrameBuffer(const TextureOptions& options,
                         int width,
                         int height,
                         bool with_depth) {
    glGenFramebuffers(1, &id);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    color = Texture(options);

    if (gl::is_integral_internal_format(color.options.internal_format)) {
        is_integral = true;
    }

    // this calls down to color.ensure_size, as well as setting
    // this->width, this->height fields
    set_size(width, height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           color.id, 0);

    if (with_depth) {
        glGenRenderbuffers(1, &depth_id);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width,
                              height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depth_id);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glEnable(GL_DEPTH_TEST);
        have_depth = true;
    }

    CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::clear(float r, float g, float b, float a) {
    CHECK(!is_integral_internal_format(color.options.internal_format))
        << "you called clear() instead of clear_ui() when format is "
        << gl_internal_format_tostring(color.options.internal_format);

    stash_prev_framebuffer();

    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glViewport(0, 0, color.width, color.height);
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    unstash_prev_framebuffer();
}

void FrameBuffer::clear_ui(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    CHECK(is_integral_internal_format(color.options.internal_format))
        << "you called clear_ui() instead of clear() when format is "
        << gl_internal_format_tostring(color.options.internal_format);

    stash_prev_framebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glViewport(0, 0, color.width, color.height);

    std::array<uint32_t, 4> clear_colors{r, g, b, a};
    glClearBufferuiv(GL_COLOR, 0, clear_colors.data());
    glClear(GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    unstash_prev_framebuffer();
}

}  // namespace gl
}  // namespace axby
