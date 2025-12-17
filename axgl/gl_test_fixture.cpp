// clang-format off
#include "gl_test_fixture.h"

#include "debug/check.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <glad/gl.h>
#include <glad/mx_shim.h>

// clang-format on

struct Global {
    SDL_Window* sdl = nullptr;
    SDL_GLContext sdl_gl;
    GladGLContext glad_gl;
};

Global _global;

const char* gl_debug_type_to_string(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            return "Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "Deprecated Behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "Undefined Behavior";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "Portability Issue";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "Performance Warning";
        case GL_DEBUG_TYPE_MARKER:
            return "Marker";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "Push Group";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "Pop Group";
        case GL_DEBUG_TYPE_OTHER:
            return "Other";
        default:
            return "Unknown";
    }
}

void GLAPIENTRY gl_debug_callback(GLenum source,
                                  GLenum type,
                                  GLuint id,
                                  GLenum severity,
                                  GLsizei length,
                                  const GLchar* message,
                                  const void* userParam) {
    std::cout << "GL Debug [" << gl_debug_type_to_string(type) << "] "
              << message;
}
void create_window() {
    const int window_width = 500;
    const int window_height = 500;

    // GL attributes configuration
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    uint32_t window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
    SDL_Window* window = SDL_CreateWindow("gl_test.cpp", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, window_width,
                                          window_height, window_flags);

    SDL_GLContext sdl_gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, sdl_gl_context);

    GladGLContext glad_gl_context;
    CHECK(gladLoadGLContext(&glad_gl_context,
                            (GLADloadfunc)SDL_GL_GetProcAddress));

    gladThreadSetup(glad_gl_context);

    if (true) {
        // after loading the context, we can check and load extensions
        CHECK(SDL_GL_ExtensionSupported("GL_KHR_debug"))
            << "Debug extension not supported";

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(
            GL_DEBUG_OUTPUT_SYNCHRONOUS);  // Ensures messages are processed immediately

        glDebugMessageControl(GL_DONT_CARE,  // Allow all sources
                              GL_DONT_CARE,  // Enable error messages
                              GL_DONT_CARE,  // Allow all severities for errors
                              0, nullptr, GL_FALSE  // Enable them
        );

        glDebugMessageControl(GL_DONT_CARE,         // Allow all sources
                              GL_DEBUG_TYPE_ERROR,  // Enable error messages
                              GL_DONT_CARE,  // Allow all severities for errors
                              0, nullptr, GL_TRUE  // Enable them
        );

        glDebugMessageControl(
            GL_DONT_CARE,                      // Allow all sources
            GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,  // Enable error messages
            GL_DONT_CARE,        // Allow all severities for errors
            0, nullptr, GL_TRUE  // Enable them
        );

        glDebugMessageControl(
            GL_DONT_CARE,                       // Allow all sources
            GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR,  // Enable error messages
            GL_DONT_CARE,        // Allow all severities for errors
            0, nullptr, GL_TRUE  // Enable them
        );

        glDebugMessageControl(
            GL_DONT_CARE,               // Allow all sources
            GL_DEBUG_TYPE_PERFORMANCE,  // Enable performance warnings
            GL_DONT_CARE,  // Allow all severities for performance warnings
            0, nullptr, GL_TRUE  // Enable them
        );

        glDebugMessageCallback(gl_debug_callback, nullptr);
    }

    _global = {
        .sdl = window, .sdl_gl = sdl_gl_context, .glad_gl = glad_gl_context};
}

void destroy_window() {
    SDL_DestroyWindow(_global.sdl);
    SDL_Quit();
}

void GlTestFixture::SetUp() { create_window(); }
void GlTestFixture::TearDown() { destroy_window(); }
