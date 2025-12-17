package(default_visibility = ["//visibility:public"])

cc_library(
    name = "serialization",
    hdrs = [
        "serialization.h",
    ],
    deps = [
        "//debug:log",
        "//fast_resizable_vector",
    ],
)

cc_library(
    name = "make_serializable",
    srcs = [
        "macros.h",
        "make_serializable.hpp",
        "primitives.cpp",
        "primitives.h",
    ],
    deps = [
        ":serialization",
        "//seq",
        "//third_party/magic_enum",
        "//third_party/pfr",
        "//third_party/tinycbor",
        "//debug:log",
        "//metaprogramming",
        ":small_string",
    ],
)

cc_binary(
    name = "serialization_test",
    srcs = ["serialization_test.cpp"],
    deps = [
        ":make_serializable",
        ":serialization",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "small_string",
    hdrs = [
        "small_string.h",
    ],
    deps = ["//debug:check"],
)

cc_binary(
    name = "small_string_test",
    srcs = [
        "small_string_test.cpp",
    ],
    deps = [
        ":small_string",
        "@googletest//:gtest_main",
    ],
)
