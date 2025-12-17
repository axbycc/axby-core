package(default_visibility = ["//visibility:public"])

cc_library(
    name = "job_state",
    srcs = ["job_state.cpp"],
    hdrs = ["job_state.h"],
    deps = [
        "//debug:check",
        "//debug:log",
    ],
)

cc_library(
    name = "single_item",
    hdrs = ["single_item.h"],
    deps = ["//debug:check"],
)

cc_library(
    name = "ring_buffer",
    hdrs = ["ring_buffer.h"],
    deps = ["//debug:check"],
)

cc_binary(
    name = "ring_buffer_test",
    srcs = ["ring_buffer_test.cpp"],
    deps = [
        ":ring_buffer",
        "//app:timing",
        "//debug:log",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "single_item_test",
    srcs = ["single_item_test.cpp"],
    deps = [
        ":single_item",
        "//app:timing",
        "//debug:check",
        "//debug:log",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "ring_buffer_stress_test",
    srcs = ["ring_buffer_stress_test.cpp"],
    deps = [
        ":ring_buffer",
        "//app:stop_all",
        "//app:main",
        "//app:timing",
        "//random:linear_congruential_generator",
        "//debug:log",
    ],
)
