load("@rules_cuda//cuda:defs.bzl", "cuda_binary", "cuda_library")

package(default_visibility = ["//visibility:public"])

# various small wrappers for to improve ergonomics of third party
# libraries

cc_library(
    name = "vpx",
    srcs = ["vpx.cpp"],
    hdrs = ["vpx.h"],
    deps = [
        "//debug:log",
        "@system_deps//:vpx",
    ],
)

cuda_library(
    name = "cuda",
    hdrs = ["cuda.h"],
    deps = [
        "//debug:check",
        "//debug:log",
    ],
)

cc_library(
    name = "eigen",
    hdrs = ["eigen.h"],
    deps = [
        "//third_party/eigen",
    ],
)

cc_library(
    name = "duckdb",
    srcs = ["duckdb.cpp"],
    hdrs = ["duckdb.h"],
    deps = [
        "//debug:log",
        "//seq",
        "//third_party/duckdb",
    ],
)

cc_library(
    name = "assimp",
    srcs = ["assimp.cpp"],
    hdrs = ["assimp.h"],
    deps = [
        "//viewer:mesh",
        "@system_deps//:assimp",
    ],
)

cc_library(
    name = "imgui",
    hdrs = ["imgui.h"],
    deps = [
        "//third_party/imgui:imgui_base",
        "@abseil-cpp//absl/strings:str_format",
    ],
)
