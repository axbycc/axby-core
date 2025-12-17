package(default_visibility = ["//visibility:public"])
load("@rules_cuda//cuda:defs.bzl", "cuda_library", "cuda_objects")

# headers must be separated out into a cc_library because of
# a deficiency in cuda_rules, which doesn't implement a strip_prefix
# attribute for cuda_library
cc_library(
    name = "popsift_headers",
    srcs = [],
    hdrs = glob(["popsift/**/*.h", "popsift/**/*.hpp"]),
    strip_include_prefix = "//third_party/popsift",
    copts = ["-diag-suppress", "20044-D"]
)

cuda_library(
    name = "popsift_cuda",
    deps = [
        ":popsift_headers",
    ],
    srcs = glob(["popsift/**/*.cu"]),

    host_linkopts = select({
        # Fixes LNK4098: defaultlib 'LIBCMT' conflicts with other libs
        "@platforms//os:windows": ["/NODEFAULTLIB:LIBCMT"],
        "@platforms//os:linux": [],
    }), 
    # host_copts = select({
    #     "//:dbg_windows_mode": ["/MDd",],
    #     "//:opt_windows_mode": ["/MD",],
    #     "//conditions:default": ["???todo???"],
    # }),
    rdc = True,
)

cc_library(
    name = "popsift",
    srcs = glob(["popsift/**/*.cpp"]),
    linkstatic=False,
    deps = [
        ":popsift_cuda",
    ]
)
    


