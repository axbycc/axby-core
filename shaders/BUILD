load("//tools:embed_data.bzl", "embed_data")

package(default_visibility = ["//visibility:public"])

embed_data(
    name = "colored_vertices_glsl",
    files = {
        "colored_vertices.vert": "colored_vertices_vs",
        "colored_vertices.frag": "colored_vertices_fs",
    },
    readable = True,
)

cc_library(
    name = "colored_vertices",
    srcs = ["colored_vertices.cpp"],
    hdrs = ["colored_vertices.h"],
    textual_hdrs = [":colored_vertices_glsl"],
    deps = [
        "//axgl:program",
        "//colors",
        "//debug:log",
        "//seq",
    ],
)

embed_data(
    name = "texture_glsl",
    files = {
        "texture.vert": "texture_vs",
        "texture.frag": "texture_fs",
    },
    readable = True,
)

cc_library(
    name = "texture",
    srcs = ["texture.cpp"],
    hdrs = ["texture.h"],
    textual_hdrs = [":texture_glsl"],
    deps = [
        "//axgl:cmap",
        "//axgl:program",
        "//colors",
        "//debug:log",
        "//seq",
    ],
)

embed_data(
    name = "cmapped_texture_glsl",
    files = {
        # note we re-use the texture.vert from texture_glsl
        "texture.vert": "cmapped_texture_vs",
        "cmapped_texture.frag": "cmapped_texture_fs",
    },
    readable = True,
)

cc_library(
    name = "cmapped_texture",
    srcs = ["cmapped_texture.cpp"],
    hdrs = ["cmapped_texture.h"],
    textual_hdrs = [":cmapped_texture_glsl"],
    deps = [
        "//axgl:cmap",
        "//axgl:program",
        "//colors",
        "//debug:log",
        "//seq",
    ],
)

embed_data(
    name = "vertex_ids_glsl",
    files = {
        "vertex_ids.vert": "vertex_ids_vs",
        "vertex_ids.frag": "vertex_ids_fs",
    },
    readable = True,
)

cc_library(
    name = "vertex_ids",
    srcs = ["vertex_ids.cpp"],
    hdrs = ["vertex_ids.h"],
    textual_hdrs = [":vertex_ids_glsl"],
    deps = [
        "//axgl:cmap",
        "//axgl:program",
        "//colors",
        "//debug:log",
        "//seq",
    ],
)

embed_data(
    name = "colored_vertex_ids_glsl",
    files = {
        "colored_vertex_ids.vert": "colored_vertex_ids_vs",
        "colored_vertex_ids.frag": "colored_vertex_ids_fs",
    },
    readable = True,
)

cc_library(
    name = "colored_vertex_ids",
    srcs = ["colored_vertex_ids.cpp"],
    hdrs = ["colored_vertex_ids.h"],
    textual_hdrs = [":colored_vertex_ids_glsl"],
    deps = [
        "//axgl:cmap",
        "//axgl:program",
        "//colors",
        "//debug:log",
        "//seq",
    ],
)

embed_data(
    name = "lines_glsl",
    files = {
        "lines.vert": "lines_vs",
        "lines.frag": "lines_fs",
    },
    readable = True,
)

cc_library(
    name = "lines",
    srcs = ["lines.cpp"],
    hdrs = ["lines.h"],
    textual_hdrs = [":lines_glsl"],
    deps = [
        "//axgl:program",
        "//debug:log",
    ],
)

embed_data(
    name = "debug_vertex_ids_glsl",
    files = {
        "debug_vertex_ids.vert": "debug_vertex_ids_vs",
        "debug_vertex_ids.frag": "debug_vertex_ids_fs",
    },
    readable = True,
)

cc_library(
    name = "debug_vertex_ids",
    srcs = ["debug_vertex_ids.cpp"],
    hdrs = ["debug_vertex_ids.h"],
    textual_hdrs = [":debug_vertex_ids_glsl"],
    deps = [
        "//axgl:frame_buffer",
        "//axgl:program",
        "//debug:log",
    ],
)

cc_binary(
    name = "scratch",
    srcs = ["scratch.cpp"],
    deps = [
        ":colored_vertex_ids",
        ":colored_vertices",
        ":lines",
        "//app:gui",
        "//app:main",
        "//axgl:gl",
        "//debug:check",
        "//debug:log",
        "//third_party/eigen",
        "//viewer",
    ],
)

cc_binary(
    name = "sanity",
    srcs = ["sanity.cpp"],
    deps = [
        "//app:gui",
        "//app:main",
        "//axgl:gl",
    ],
)

embed_data(
    name = "vertex_world_xyzs_glsl",
    files = {
        "vertex_world_xyzs.vert": "vertex_world_xyzs_vs",
        "vertex_world_xyzs.frag": "vertex_world_xyzs_fs",
    },
    readable = True,
)

cc_library(
    name = "vertex_world_xyzs",
    srcs = ["vertex_world_xyzs.cpp"],
    hdrs = ["vertex_world_xyzs.h"],
    textual_hdrs = [":vertex_world_xyzs_glsl"],
    deps = [
        "//axgl:frame_buffer",
        "//axgl:program",
        "//debug:log",
    ],
)

embed_data(
    name = "material_mesh_glsl",
    files = {
        "material_mesh.vert": "material_mesh_vs",
        "material_mesh.frag": "material_mesh_fs",
    },
    readable = True,
)

cc_library(
    name = "material_mesh",
    srcs = ["material_mesh.cpp"],
    hdrs = ["material_mesh.h"],
    textual_hdrs = [":material_mesh_glsl"],
    deps = [
        "//axgl:cmap",
        "//axgl:program",
        "//colors",
        "//debug:log",
        "//seq",
        "//viewer:mesh",
    ],
)
