package(default_visibility = ["//visibility:public"])

# Glad, MX version
# https://gen.glad.sh/#generator=c&api=gl%3D4.6&profile=gl%3Dcore%2Cgles1%3Dcommon&extensions=GL_ARB_direct_state_access%2CGL_KHR_debug&options=ALIAS%2CLOADER%2CMX

# we use the MX version because it gives us the option to do multithreaded rendering.

cc_library(
    name = "khr_platform",
    # include this subdirectory as a system directory for gcc, this
    # uses -isystem. this is required for dependencies to include
    # mx/KHR/khr_platform.h with "#include <KHR/khr_platform.h>"
    # this is only needed because glad/gl.h uses this form of include
    includes = ["mx/"],

    # bazel will not populate the build directory with the actual
    # header files on linux unless you include them as sources
    srcs = ["mx/KHR/khrplatform.h"]
)

# this packs in a utility library allows for binding the glad context
# to global thread-local function variables so that conumers of OpenGL
# functions do not have to access them through the glad context
# struct.
cc_library(
    name = "glad",
    srcs = [
        "mx/glad/mx_shim.cpp",
        "mx/glad/gl_copy.c"
    ],
    hdrs = [
        "mx/glad/mx_shim.h",
        "mx/glad/gl.h",
        "mx/glad/gl_copy.h",
    ],
    includes = [
        "mx/"
    ],
    deps = [":khr_platform"],
    strip_include_prefix="//third_party/glad/mx"
)
