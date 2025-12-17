package(default_visibility = ["//visibility:public"])

cc_library(
    name = "to_sstring",
    hdrs = ["to_sstring.h"],
)

cc_library(
    name = "log",
    hdrs = ["log.h"],
    deps = [
        "@abseil-cpp//absl/log:log",
    ],
)

cc_library(
    name = "check",
    hdrs = ["check.h"],
    deps = [
        "@abseil-cpp//absl/log:check",
    ],
)

cc_library(
    name = "error_message_or",
    hdrs = ["error_message_or.h"],
    deps = [
        "@abseil-cpp//absl/log:check",
    ],
)
