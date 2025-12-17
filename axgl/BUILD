package(default_visibility = ["//visibility:public"])

cc_library(
    name = "info",
    srcs = ["info.cpp"],
    hdrs = ["info.h"],
    deps = [
        "//debug:check",
        "//debug:log",
        "//third_party/glad",
    ],
)

cc_library(
    name = "texture",
    srcs = ["texture.cpp"],
    hdrs = ["texture.h"],
    deps = [
        ":buffer",
        ":info",
        "//debug:check",
        "//debug:log",
        "//third_party/glad",
        "//third_party/magic_enum",
    ],
)

cc_library(
    name = "buffer",
    srcs = ["buffer.cpp"],
    hdrs = ["buffer.h"],
    deps = [
        ":info",
        "//debug:check",
        "//debug:log",
        "//seq",
        "//third_party/glad",
    ],
)

cc_library(
    name = "frame_buffer",
    srcs = ["frame_buffer.cpp"],
    hdrs = ["frame_buffer.h"],
    deps = [
        ":info",
        ":texture",
        "//debug:check",
        "//debug:log",
        "//seq",
        "//third_party/glad",
    ],
)

cc_library(
    name = "program",
    srcs = ["program.cpp"],
    hdrs = ["program.h"],
    deps = [
        ":frame_buffer",
        ":info",
        ":texture",
        ":vertex_array",
        "//debug:check",
        "//debug:log",
        "//seq",
        "//third_party/glad",
        "@abseil-cpp//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "vertex_array",
    srcs = ["vertex_array.cpp"],
    hdrs = ["vertex_array.h"],
    deps = [
        ":buffer",
        ":info",
        "//debug:check",
        "//debug:log",
        "//seq",
        "//third_party/glad",
        "@abseil-cpp//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "cmap",
    srcs = [
        "cmap.cpp",
        "cmap.h",
    ],
    hdrs = ["cmap.h"],
    deps = [
        ":texture",
        "//colors",
        "//debug:log",
        "//third_party/tinycolormap",
    ],
)

cc_library(
    name = "gl_test_fixture",
    srcs = ["gl_test_fixture.cpp"],
    hdrs = ["gl_test_fixture.h"],
    deps = [
        "//debug:check",
        "//third_party/glad",
        "@googletest//:gtest",
        "@system_deps//:sdl",
    ],
)

cc_binary(
    name = "buffer_test",
    srcs = ["buffer_test.cpp"],
    deps = [
        ":buffer",
        ":gl_test_fixture",
        "//debug:check",
        "//seq",
        "//third_party/glad",
        "@googletest//:gtest_main",
        "@system_deps//:sdl",
    ],
)

cc_binary(
    name = "info_test",
    srcs = ["info_test.cpp"],
    deps = [
        ":info",
        "//debug:check",
        "//third_party/glad",
        "@googletest//:gtest_main",
        "@system_deps//:sdl",
    ],
)

cc_binary(
    name = "texture_test",
    srcs = ["texture_test.cpp"],
    deps = [
        ":gl_test_fixture",
        ":texture",
        "//debug:check",
        "//seq",
        "//third_party/glad",
        "@googletest//:gtest_main",
        "@system_deps//:sdl",
    ],
)

cc_library(
    name = "gl",
    deps = [
        ":cmap",
        ":frame_buffer",
        ":info",
        ":program",
        ":shapes",
        ":texture",
        ":vertex_array",
    ],
)

cc_binary(
    name = "frame_buffer_test",
    srcs = ["frame_buffer_test.cpp"],
    deps = [
        ":frame_buffer",
        ":gl_test_fixture",
        "//app:main",
        "//debug:check",
        "//debug:log",
        "//seq",
        "//third_party/glad",
        "@system_deps//:sdl",
    ],
)

cc_library(
    name = "shapes",
    srcs = ["shapes.cpp"],
    hdrs = ["shapes.h"],
    deps = [
        ":buffer",
        ":program",
        ":vertex_array",
        "//colors",
    ],
)
