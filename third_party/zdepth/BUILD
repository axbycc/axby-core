package(default_visibility = ["//visibility:public"])

cc_library(
    name="zdepth",
    hdrs = glob(["include/*.hpp"]),
    srcs = glob(["include/*.hpp", "src/*.cpp"]),
    deps = ["//third_party/zstd:zstd"],
    strip_include_prefix = "/third_party/zdepth/include",
)
