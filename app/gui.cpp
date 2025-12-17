#include "app/gui.h"

#include <glad/gl.h>  // Initialize with gladLoadGL()

// clang-format off
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <SDL.h>
#include <implot.h>
// clang-format on

#include "debug/check.h"
#include "debug/log.h"
#include "app/stop_all.h"

namespace axby {

const bool USE_GL_DEBUG_EXTENSION = true;

// SDL context
int _status = 0;
SDL_Window* _window = nullptr;
SDL_GLContext _sdl_gl_context;
bool _gui_wants_quit = false;
bool _using_imgui = false;
float _dpi_scale_multiplier = 1.4f;

void set_dpi_scale_multiplier(float s) {
    CHECK_GT(s, 0);
    _dpi_scale_multiplier = s;
}

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
    LOG_EVERY_T(INFO, 1.0) << "GL Debug [" << gl_debug_type_to_string(type)
                           << "] " << message;
}

void* gui_window_ptr() { return (void*)_window; };

void create_gl_context() {
    CHECK_NOTNULL(_window) << "No window. Did you call gui_init?";

    _sdl_gl_context = SDL_GL_CreateContext(_window);
    SDL_GL_MakeCurrent(_window, _sdl_gl_context);

    // retreive function pointers with Glad
    GladGLContext glad_gl_context;
    CHECK(gladLoadGLContext(&glad_gl_context,
                            (GLADloadfunc)SDL_GL_GetProcAddress));
    gladThreadSetup(glad_gl_context);

    if (USE_GL_DEBUG_EXTENSION) {
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

    glEnable(GL_PROGRAM_POINT_SIZE);
}

float GetDPIScale(SDL_Window* window) {
    int drawable_w, drawable_h;
    int window_w, window_h;
    SDL_GL_GetDrawableSize(window, &drawable_w, &drawable_h);
    SDL_GetWindowSize(window, &window_w, &window_h);
    float base_dpi_scale = (float)drawable_w / (float)window_w;
    return base_dpi_scale * _dpi_scale_multiplier;
}

const extern uint32_t DEFAULT_SDL_INIT_FLAGS = SDL_INIT_VIDEO | SDL_INIT_TIMER;
const extern uint32_t DEFAULT_SDL_WINDOW_FLAGS =
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

int sdl_init(const char* title,
             int width,
             int height,
             uint32_t sdl_init_flags,
             uint32_t sdl_window_flags) {
    CHECK(_window == nullptr) << "double init?";

    CHECK(SDL_Init(sdl_init_flags) == 0) << SDL_GetError();

    const bool should_maximize = width == 0 || height == 0;
    if (should_maximize) {
        // set some default dimension
        width = 1920;
        height = 1280;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    if (should_maximize) {
        sdl_window_flags = sdl_window_flags | SDL_WINDOW_MAXIMIZED;
    }
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(sdl_window_flags);
    _window =
        SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         width, height, window_flags);

    create_gl_context();

    SDL_GL_SetSwapInterval(0);  // disable vsync

    return 0;
}

void UpdateImGuiScaling(SDL_Window* window) {
    return;
    LOG_EVERY_T(INFO, 1.0) << "Updating imgui scaling!";
    static float last_scale = 1.0f;
    float scale = GetDPIScale(window);

    if (fabs(scale - last_scale) < 0.01f) return;

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->Clear();
    ImFontConfig cfg;
    cfg.SizePixels = 16.0f * scale;
    io.Fonts->AddFontDefault(&cfg);
    io.Fonts->Build();

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();  // reset before scaling
    style.ScaleAllSizes(scale);

    last_scale = scale;
}

void gui_init(const char* window_name,
              int width,
              int height,
              bool use_imgui,
              uint32_t sdl_init_flags,
              uint32_t sdl_window_flags) {
    sdl_init(window_name, width, height, sdl_init_flags, sdl_window_flags);

    _using_imgui = use_imgui;
    if (_using_imgui) {
        // init imgui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer bindings
        ImGui_ImplSDL2_InitForOpenGL(_window, _sdl_gl_context);
        ImGui_ImplOpenGL3_Init(/*glsl_version=*/nullptr);

        UpdateImGuiScaling(_window);
    }
};

std::pair<int, int> gui_window_size() {
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    return std::make_pair(w, h);
}

bool gui_wants_quit() { return _gui_wants_quit || should_stop_all(); }

void gui_loop_begin() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (_using_imgui) ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            _gui_wants_quit = true;
        }
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(_window)) {
            _gui_wants_quit = true;
        }
        if ((event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) &&
            _using_imgui) {
            UpdateImGuiScaling(_window);
        }
    }

    int w = 0;
    int h = 0;
    SDL_GetWindowSize(_window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1, 0.0, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (_using_imgui) {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(_window);
        ImGui::NewFrame();
    }
}

void gui_loop_end() {
    if (_using_imgui) {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    SDL_GL_SwapWindow(_window);
}

void gui_cleanup() {
    // Cleanup
    if (_using_imgui) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    SDL_GL_DeleteContext(_sdl_gl_context);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

}  // namespace axby
