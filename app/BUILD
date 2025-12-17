load("//tools:embed_data.bzl", "embed_data")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "process_id",
    srcs = ["process_id.cpp"],
    hdrs = ["process_id.h"],
)

cc_library(
    name = "files",
    srcs = ["files.cpp"],
    hdrs = ["files.h"],
    linkopts = select(
        {
            "@platforms//os:windows": [
                "Shell32.lib",
            ],
            "//conditions:default": [],
        },
    ),
    deps = [
        "//debug:check",
        "//debug:log",
        "//seq",
        "@bazel_tools//tools/cpp/runfiles",
    ],
)

cc_library(
    name = "timing",
    srcs = ["timing.cpp"],
    hdrs = ["timing.h"],
)

cc_library(
    name = "stop_all",
    srcs = ["stop_all.cpp"],
    hdrs = ["stop_all.h"],
)

cc_library(
    name = "pubsub_message",
    hdrs = ["pubsub_message.h"],
    deps = [
        "//debug:check",
        "//serialization",
        "//serialization:make_serializable",
        "@system_deps//:zmq",
    ],
)

embed_data(
    name = "create_log_table_sql",
    files = {
        "create_log_table.sql": "create_log_table_sql",
    },
    readable = True,
)

cc_library(
    name = "pubsub_recorder",
    srcs = [
        "pubsub_recorder.cpp",
        ":create_log_table_sql",
    ],
    hdrs = ["pubsub_recorder.h"],
    deps = [
        ":process_id",
        ":pubsub_message",
        "//app:files",
        "//app:timing",
        "//wrappers:duckdb",
    ],
)

cc_library(
    name = "pubsub",
    srcs = [
        "pubsub.cpp",
    ],
    hdrs = [
        "pubsub.h",
        "pubsub_message.h",
    ],
    deps = [
        ":process_id",
        ":pubsub_message",
        ":pubsub_recorder",
        "//app:files",
        "//app:stop_all",
        "//app:timing",
        "//concurrency:ring_buffer",
        "//concurrency:single_item",
        "//debug:log",
        "//seq",
        "//serialization",
        "//serialization:make_serializable",
        "//wrappers:duckdb",
        "@abseil-cpp//absl/strings:strings",
        "@system_deps//:zmq",
    ],
)

cc_library(
    name = "main",
    srcs = ["main.cpp"],
    hdrs = ["main.h"],
    deps = [
        ":files",
        ":stop_all",
        "@abseil-cpp//absl/debugging:failure_signal_handler",
        "@abseil-cpp//absl/debugging:symbolize",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log:flags",
        "@abseil-cpp//absl/log:initialize",
    ],
)

cc_library(
    name = "gui",
    srcs = ["gui.cpp"],
    hdrs = ["gui.h"],
    deps = [
        ":stop_all",
        "//debug:check",
        "//debug:log",
        "//third_party/glad",
        "//third_party/imgui:imgui_base",
        "//third_party/imgui:imgui_sdl",
        "//third_party/implot",
        "//third_party/sdl",
    ],
)

cc_binary(
    name = "main_example",
    srcs = ["main_example.cpp"],
    deps = [
        ":flag",
        ":main",
        "//debug:log",
    ],
)

cc_binary(
    name = "gui_example",
    srcs = ["gui_example.cpp"],
    deps = [
        ":gui",
        ":main",
    ],
)

cc_binary(
    name = "gui_gl_example",
    srcs = ["gui_gl_example.cpp"],
    deps = [
        ":gui",
        ":main",
        "//axgl:gl",
        "//shaders:colored_vertices",
        "//third_party/eigen",
        "//third_party/glad",
    ],
)

cc_library(
    name = "flag",
    hdrs = ["flag.h"],
    deps = [
        "@abseil-cpp//absl/flags:flag",
    ],
)

cc_binary(
    name = "pubsub_example",
    srcs = ["pubsub_example.cpp"],
    deps = [
        ":flag",
        ":main",
        ":pubsub",
        ":stop_all",
        ":timing",
    ],
)
