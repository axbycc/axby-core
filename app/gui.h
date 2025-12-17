#pragma once

#include <stdint.h>
#include <utility>

namespace axby {
// see .cpp file for these flags
extern const uint32_t DEFAULT_SDL_INIT_FLAGS;
extern const uint32_t DEFAULT_SDL_WINDOW_FLAGS;

void* gui_window_ptr();

// gl_attributes is a span where the even indexes are attribute names
// and the odd indexes are the corresponding attributes
void gui_init(const char* window_name,
              int width = 0,
              int height = 0,
              bool use_imgui = true,
              uint32_t sdl_init_flags = DEFAULT_SDL_INIT_FLAGS,
              uint32_t sdl_window_flags = DEFAULT_SDL_WINDOW_FLAGS);

// every call of gui_loop_begin() must be matched by gui_loop_end()
void gui_loop_begin();

std::pair<int, int> gui_window_size();

// render everything to the screen
void gui_loop_end();

// call before returning from main
void gui_cleanup();

// returns true if the window close was ever triggered by the user
bool gui_wants_quit();

void set_dpi_scale_multiplier(float s);

}
