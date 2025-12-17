package(default_visibility = ["//visibility:public"])

cc_library(
    name = "tinycbor",
    srcs = glob(["src/*.c", "src/*.h"], exclude=["src/open_memstream.c"]),
    hdrs = ["src/cbor.h", "src/tinycbor-version.h"],
    strip_include_prefix = "//third_party/tinycbor/src",
)

