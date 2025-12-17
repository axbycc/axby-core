package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "server",
    srcs = ["server.cpp"],
    deps = [
        "//app:flag",
        "//app:main",
        "//app:stop_all",
        "//app:timing",
        "//debug:log",
        "//network_config:config",
        "//third_party/kissnet",
    ],
)

cc_library(
    name = "time_sync",
    srcs = ["time_sync.cpp"],
    hdrs = ["time_sync.h"],
    deps = [
        "//app:flag",
        "//app:process_id",
        "//app:pubsub",
        "//app:stop_all",
        "//app:timing",
        "//concurrency:ring_buffer",
        "//debug:log",
        "//network_config:config",
        "//third_party/kissnet",
        "@abseil-cpp//absl/container:flat_hash_map",
    ],
)

cc_binary(
    name = "client_demo",
    srcs = ["client_demo.cpp"],
    deps = [
        ":time_sync",
        "//app:flag",
        "//app:main",
        "//app:process_id",
        "//debug:log",
        "//network_config:config",
        "@system_deps//:zmq",
    ],
)
