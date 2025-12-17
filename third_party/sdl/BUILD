package(default_visibility = ["//visibility:public"])

# on linux, try to use the system libraries
# installed by `sudo apt install libsdl2-dev`

cc_library(
    name = "sdl",
    deps = select({
        "@bazel_tools//src/conditions:windows": [":sdl_windows"],
        "//conditions:default": ["@system_deps//:sdl"],
    }),
)

cc_library(
    name = "sdl_windows",
    includes = [
        "include",
    ],
    hdrs = glob(["include/**/*"]),
    srcs = glob(["lib/win64/*.lib"]),
    strip_include_prefix = "/third_party/sdl/include",

    deps = [":sdl_dlls"] # triggers runtime copy of dll into executable dir
)

cc_library(
    name = "sdl_dlls",
    srcs = glob(["lib/win64/*.dll"]),
)


