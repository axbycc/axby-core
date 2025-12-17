package(default_visibility = ["//visibility:public"])

cc_library(
    name = "implot",
    srcs = [
        "implot.cpp",
        "implot_demo.cpp",
        "implot_items.cpp",
        "implot.h",
        "implot_internal.h"
    ],
    hdrs = [
        "implot.h",
    ],
    deps = [
        "//third_party/imgui:imgui_base",
    ],
    strip_include_prefix = "//third_party/implot",
)
