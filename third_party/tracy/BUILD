package(default_visibility = ["//visibility:public"])

cc_library(
    name = "tracy_enabled",
    textual_hdrs = glob([
        "**/*.cpp",
    ], exclude=["TracyClient.cpp"]),
    srcs = ["TracyClient.cpp"] + glob([
        "**/*.hpp",
        "**/*.h",
    ]),
    linkstatic=False,
    copts = select({
        "@platforms//os:linux": [
            "-Wno-error=unused-result",
            "-Wno-error=unused-function",
        ],
        "//conditions:default": [],
    }),
    hdrs = glob([
        "**/*.hpp",
        "**/*.h",
    ]),
    strip_include_prefix="/third_party/tracy/",
    defines=[
        "TRACY_ENABLE"
    ],
)

# same as the tracy_enabled, except without the TRACY_ENABLE define
cc_library(
    name = "tracy_disabled",
    textual_hdrs = glob([
        "**/*.cpp",
    ], exclude=["TracyClient.cpp"]),
    srcs = ["TracyClient.cpp"] + glob([
        "**/*.hpp",
        "**/*.h",
    ]),
    linkstatic=False,
    copts = select({
        "@platforms//os:linux": [
            "-Wno-error=unused-result",
            "-Wno-error=unused-function",
        ],
        "//conditions:default": [],
    }),
    hdrs = glob([
        "**/*.hpp",
        "**/*.h",
    ]),
    strip_include_prefix="/third_party/tracy/",
)

cc_library(
    name = "tracy",
    deps = [":tracy_disabled"])
