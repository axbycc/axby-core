#include "mx_shim.h"

// clang-format off
thread_local GladGLContext __CURRENT_GLAD_GL_CONTEXT__;
// clang-format on

void gladThreadSetup(const GladGLContext& context) {
    __CURRENT_GLAD_GL_CONTEXT__ = context;
};
