package(default_visibility = ["//visibility:public"])

cc_library(
    name = "pointcloud_job",
    srcs = ["pointcloud_job.cpp"],
    hdrs = ["pointcloud_job.h"],
    deps = [
        ":client",
        "//concurrency:job_state",
        "//math:camera",
        "//math:spatial",
        "//rgbd",
        "//third_party/simple_thread_pool",
    ],
)

cc_library(
    name = "messages",
    srcs = ["messages.cpp"],
    hdrs = ["messages.h"],
    deps = [
        "//fast_resizable_vector",
        "//serialization:small_string",
        "//third_party/magic_enum",
        "@system_deps//:realsense",
    ],
)

cc_library(
    name = "realsense_state",
    hdrs = ["realsense_state.h"],
    deps = [":messages"],
)

cc_library(
    name = "util",
    srcs = ["util.cpp"],
    hdrs = ["util.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":messages",
        "//app:timing",
        "//debug:log",
        "//math:spatial",
        "//third_party/zdepth",
        "//wrappers:vpx",
        "@system_deps//:realsense",
    ],
)

cc_library(
    name = "client",
    srcs = ["client.cpp"],
    hdrs = ["client.h"],
    deps = [
        ":messages",
        ":realsense_state",
        "//app:pubsub",
        "//app:stop_all",
        "//app:timing",
        "//concurrency:ring_buffer",
        "//concurrency:single_item",
        "//debug:check",
        "//debug:log",
        "//fast_resizable_vector",
        "//network_config:config",
        "//third_party/yuv2rgb",
        "//third_party/zdepth",
        "//time_sync",
        "//wrappers:vpx",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:strings",
        "@system_deps//:realsense",
    ],
)

cc_binary(
    name = "client_sanity",
    srcs = ["client_sanity.cpp"],
    deps = [
        ":client",
        "//app:flag",
        "//app:main",
        "//app:pubsub",
        "//app:stop_all",
        "//network_config:config",
        "//time_sync",
    ],
)

cc_binary(
    name = "viewer",
    srcs = ["viewer.cpp"],
    deps = [
        ":client",
        ":pointcloud_job",
        "//app:flag",
        "//app:gui",
        "//app:main",
        "//app:pubsub",
        "//debug:log",
        "//network_config:config",
        "//third_party/imgui:imgui_base",
        "//third_party/imoverlayable",
        "//time_sync",
        "//viewer",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_binary(
    name = "server",
    srcs = [
        "server.cpp",
    ],
    deps = [
        ":util",
        "//app:flag",
        "//app:main",
        "//app:pubsub",
        "//app:timing",
        "//concurrency:ring_buffer",
        "//debug:log",
        "//math:spatial",
        "//network_config:config",
        "//third_party/yuv2rgb",
        "//time_sync",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings:str_format",
        "@system_deps//:realsense",
    ],
)
