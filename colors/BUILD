package(default_visibility=["//visibility:public"])

cc_library(
    name="colors",
    hdrs=[
        "colors.h",
    ],
    deps=[
        "//debug:check",
        "//debug:log",
        "//seq:seq",
    ],
)

cc_binary(
    name="colors_test",
    srcs=["colors_test.cpp"],
    deps=[
        ":colors",
        "//third_party/imgui:imgui_base",
        "@googletest//:gtest_main",
    ],
)
