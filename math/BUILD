package(default_visibility = ["//visibility:public"])

cc_library(
    name = "misc",
    hdrs = ["misc.h"],
)

cc_library(
    name = "spatial_impl",
    hdrs = ["spatial_impl.hpp"],
    deps = [
        "//third_party/eigen",
        "//wrappers:eigen",
    ],
)

cc_library(
    name = "spatial",
    srcs = ["spatial.cpp"],
    hdrs = ["spatial.h"],
    deps = [
        "spatial_impl",
        "//debug:check",
        "//seq",
        "//third_party/eigen",
        "//wrappers:eigen",
    ],
)

cc_library(
    name = "camera",
    srcs = ["camera.cpp"],
    hdrs = ["camera.h"],
    deps = [
        "//debug:log",
        "//seq",
        "//third_party/eigen",
        "//wrappers:eigen",
    ],
)

cc_library(
    name = "running_stat",
    hdrs = [
        "running_stat.h",
    ],
)
