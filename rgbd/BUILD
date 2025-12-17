package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rgbd",
    hdrs = ["rgbd.h"],
    srcs = ["rgbd.cpp"],
    deps = [
        "//seq",
        "//debug:log",
        "//fast_resizable_vector",
        "//third_party/eigen",
        "//third_party/nanoflann",
        "//wrappers:eigen",
    ]
)

cc_binary(
    name = "rgbd_test",
    srcs = ["rgbd_test.cpp"],
    deps = [
        "//debug:log",
        "//math:camera",
        "//random:linear_congruential_generator",
        "//third_party/eigen",
        ":rgbd",
        "@googletest//:gtest_main",
    ],
)
