package(default_visibility = ["//visibility:public"])

cc_import(
    name = "duckdb_dll",
    interface_library = "win64/duckdb.lib",
    shared_library = "win64/duckdb.dll",)

cc_import(
    name = "duckdb_so",
    shared_library = "linux64/libduckdb.so",)

cc_library(
    name = "duckdb_windows",
    hdrs = glob(["win64/*.h", "win64/*.hpp"]),
    deps = [":duckdb_dll"],
    strip_include_prefix="//third_party/duckdb/win64",    
)

cc_library(
    name = "duckdb_linux",
    hdrs = glob(["linux64/*.h", "linux64/*.hpp"]),
    deps = [":duckdb_so"],
    strip_include_prefix="//third_party/duckdb/linux64",    
)

cc_library(
    name = "duckdb",
    deps = select({
        "@platforms//os:windows": [":duckdb_windows"],
        "@platforms//os:linux": [":duckdb_linux"],
    }),
)

