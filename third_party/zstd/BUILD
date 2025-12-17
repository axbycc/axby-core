package(default_visibility = ["//visibility:public"])

cc_library(
    name="zstd_src_headers",
    hdrs=glob(["src/*.h"]),
    strip_include_prefix = "/third_party/zstd/src")

cc_library(
    name="zstd",
    hdrs = glob(["include/*.h"]),
    srcs = glob(["include/*.h", "src/*.c", "src/*.h"]),
    deps = [":zstd_src_headers"],
    strip_include_prefix = "/third_party/zstd/include",
)

