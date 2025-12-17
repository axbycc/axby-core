package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mesh",
    hdrs = ["mesh.h"],
    deps = [
        "//colors",
        "//fast_resizable_vector",
    ],
)

cc_library(
    name = "viewer",
    srcs = ["viewer.cpp"],
    hdrs = ["viewer.h"],
    deps = [
        ":mesh",
        "//app:gui",
        "//axgl:gl",
        "//colors",
        "//fast_resizable_vector",
        "//math:camera",
        "//seq",
        "//shaders:cmapped_texture",
        "//shaders:colored_vertex_ids",
        "//shaders:colored_vertices",
        "//shaders:debug_vertex_ids",
        "//shaders:lines",
        "//shaders:material_mesh",
        "//shaders:texture",
        "//shaders:vertex_ids",
        "//shaders:vertex_world_xyzs",
        "//third_party/eigen",
        "//wrappers:eigen",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_binary(
    name = "viewer_mesh_demo",
    srcs = ["viewer_mesh_demo.cpp"],
    data = [
        "sphere.ply",
    ],
    deps = [
        ":viewer",
        "//app:gui",
        "//app:main",
        "//colors",
        "//debug:to_sstring",
        "//wrappers:assimp",
        "//wrappers:eigen",
    ],
)

cc_binary(
    name = "viewer_demo",
    srcs = ["viewer_demo.cpp"],
    deps = [
        ":viewer",
        "//app:gui",
        "//app:main",
        "//colors",
        "//debug:to_sstring",
        "//wrappers:eigen",
    ],
)
