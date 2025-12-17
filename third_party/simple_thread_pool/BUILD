package(default_visibility = ["//visibility:public"])

cc_library(
    name = "simple_thread_pool",
    hdrs = ["simple_thread_pool.h"],
    srcs = ["simple_thread_pool.cpp"],
    strip_include_prefix="//third_party/simple_thread_pool",
)

cc_binary(
    name = "thread_example",
    srcs = ["thread_pool_example.cpp"],
    deps = [":simple_thread_pool"],
)
