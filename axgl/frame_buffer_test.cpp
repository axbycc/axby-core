/*
  
  gtest interacts poorly with frame buffer creation for unknown
  reasons, even though it works fine for the other gl tests. the
  following test file crashes. so we can't use gtest to test frame
  buffers.

// clang-format off
#include <glad/mx_shim.h>
#include "gtest/gtest.h"
// clang-format on

TEST(GlTestFixture, some_test) {
GLuint fbo;
glGenFramebuffers(1, &fbo); // crashes here!
EXPECT_TRUE(true);
}

*/

#define TEST_INSIDE_WINDOW_CONTEXT(func)                                                  \
    do {                                                                 \
        create_window();                                                 \
        bool result = (func());                                          \
        LOG(INFO) << #func << " result: " << (result ? "PASS" : "FAIL"); \
        if (!result) _all_passed = false;                                \
        destroy_window();                                                \
    } while (0)

#include "axgl/frame_buffer.h"

#include <glad/mx_shim.h>

#include "app/main.h"
#include "debug/check.h"
#include "debug/log.h"
#include "gl_test_fixture.h"

// using namespace axby;

bool frame_buffer_sanity0() {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    return true;
}

bool frame_buffer_sanity1() {
    const int fb_width = 10;
    const int fb_height = 3;
    const int num_channels = 3;

    // Create framebuffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create color texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (num_channels == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, /*internal_format=*/GL_RGBA8, fb_width,
                     fb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, /*internal_format=*/GL_RGB8, fb_width,
                     fb_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture, 0);

    // Check framebuffer status
    CHECK_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

    // Clear to red
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    std::vector<unsigned char> pixels(fb_width * fb_height * num_channels);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);  // For 1-byte aligned data (e.g., RGB)
    if (num_channels == 4) {
        glReadPixels(0, 0, fb_width, fb_height, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixels.data());
    } else {
        glReadPixels(0, 0, fb_width, fb_height, GL_RGB, GL_UNSIGNED_BYTE,
                     pixels.data());
    }

    CHECK_EQ(pixels.size() % num_channels, 0);
    for (int i = 0; i < pixels.size() / num_channels; ++i) {
        // LOG(INFO) << "Pixel " << i;
        for (int j = 0; j < num_channels; ++j) {
            const uint8_t val = pixels[i * num_channels + j];
            if (j == 0) {
                // red channel
                if (val != 255) return false;
            } else if (j == 3) {
                // alpha channel
                if (val != 255) return false;
            } else {
                if (val != 0) return false;
            }
        }
    }

    // Cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &fbo);

    return true;
}

bool frame_buffer_clear_red() {
    const int fb_width = 10;
    const int fb_height = 3;
    const int num_channels = 3;

    // Create framebuffer
    axby::gl::TextureOptions options;
    options.set_data_type<uint8_t>();
    options.set_rgb();

    axby::gl::FrameBuffer frame_buffer(options, fb_width, fb_height);

    // Clear to red
    frame_buffer.clear(1.0f, 0.0f, 0.0f, 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
    std::vector<unsigned char> pixels(fb_width * fb_height * num_channels);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);  // For 1-byte aligned data (e.g., RGB)
    if (num_channels == 4) {
        glReadPixels(0, 0, fb_width, fb_height, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixels.data());
    } else {
        glReadPixels(0, 0, fb_width, fb_height, GL_RGB, GL_UNSIGNED_BYTE,
                     pixels.data());
    }

    CHECK_EQ(pixels.size() % num_channels, 0);
    for (int i = 0; i < pixels.size() / num_channels; ++i) {
        // LOG(INFO) << "Pixel " << i;
        for (int j = 0; j < num_channels; ++j) {
            const uint8_t val = pixels[i * num_channels + j];
            if (j == 0) {
                // red channel
                if (val != 255) return false;
            } else if (j == 3) {
                // alpha channel
                if (val != 255) return false;
            } else {
                if (val != 0) return false;
            }
        }
    }

    return true;
}

bool frame_buffer_clear_blue() {
    const int fb_width = 10;
    const int fb_height = 3;
    const int num_channels = 3;

    // Create framebuffer
    axby::gl::TextureOptions options;
    options.set_data_type<uint8_t>();
    options.set_rgb();

    axby::gl::FrameBuffer frame_buffer(options, fb_width, fb_height);

    // Clear to blue
    frame_buffer.clear(0.0f, 0.0f, 1.0f, 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
    std::vector<unsigned char> pixels(fb_width * fb_height * num_channels);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);  // For 1-byte aligned data (e.g., RGB)
    if (num_channels == 4) {
        glReadPixels(0, 0, fb_width, fb_height, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixels.data());
    } else {
        glReadPixels(0, 0, fb_width, fb_height, GL_RGB, GL_UNSIGNED_BYTE,
                     pixels.data());
    }

    CHECK_EQ(pixels.size() % num_channels, 0);
    for (int i = 0; i < pixels.size() / num_channels; ++i) {
        // LOG(INFO) << "Pixel " << i;
        for (int j = 0; j < num_channels; ++j) {
            const uint8_t val = pixels[i * num_channels + j];
            if (j == 2) {
                // blue channel
                if (val != 255) return false;
            } else if (j == 3) {
                // alpha channel
                if (val != 255) return false;
            } else {
                if (val != 0) return false;
            }
        }
    }

    return true;
}

bool frame_buffer_rg_ui() {
    // passes as long as no crash
    axby::gl::TextureOptions pick_buffer_options;
    pick_buffer_options.set_data_type<uint32_t>();
    pick_buffer_options.format = GL_RG_INTEGER;
    pick_buffer_options.internal_format = GL_RG32UI;
    axby::gl::FrameBuffer pick_buffer = {pick_buffer_options, 50, 50};
    return pick_buffer.id > 0;
}

bool _all_passed = true;

int main(int argc, char *argv[]) {
    __APP_MAIN_INIT__;

    TEST_INSIDE_WINDOW_CONTEXT(frame_buffer_sanity0);
    TEST_INSIDE_WINDOW_CONTEXT(frame_buffer_sanity1);
    TEST_INSIDE_WINDOW_CONTEXT(frame_buffer_clear_red);
    TEST_INSIDE_WINDOW_CONTEXT(frame_buffer_clear_blue);
    TEST_INSIDE_WINDOW_CONTEXT(frame_buffer_rg_ui);

    LOG(INFO) << "Final: " << (_all_passed ? "PASS" : "FAIL");

    return 0;
}
