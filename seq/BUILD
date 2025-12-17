package(default_visibility=["//visibility:public"])

cc_library(
    name="seq",
    hdrs=[
        "any_seq.h",
        "float_or_double_seq.h",
        "seq.h",
    ],
    deps=[
        "//metaprogramming",
    ],
)

cc_binary(
    name="seq_test",
    srcs=["seq_test.cpp"],
    deps=[
        ":seq",
        "//debug:check",
        "//third_party/eigen",
        "@googletest//:gtest_main",
    ],
)
