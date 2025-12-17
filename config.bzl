load("@bazel_skylib//rules:copy_file.bzl", "copy_file")
load("@nanobind_bazel//:build_defs.bzl", nanobind_extension_orig="nanobind_extension")

def _py3_libs(ctx):
    toolchain = ctx.toolchains["@bazel_tools//tools/python:toolchain_type"]
    if not hasattr(toolchain, "py3_runtime"):
        fail("Python toolchain field 'py3_runtime' is missing")
    if not toolchain.py3_runtime:
        fail("Python toolchain missing py3_runtime")

    py3_runtime = toolchain.py3_runtime
    major_version = py3_runtime.interpreter_version_info.major
    minor_version = py3_runtime.interpreter_version_info.minor
        
    py3_filenames = ["libpython{}.{}.dylib".format(major_version, minor_version),
                     "libpython{}.{}.so".format(major_version, minor_version),
                     "libpython{}.{}.so.1.0".format(major_version, minor_version)]

    is_windows = py3_runtime.interpreter.extension == "exe"
    if is_windows:
        if ctx.attr.stable_api:
            py3_filenames.append("python{}.lib".format(major_version))
            py3_filenames.append("python{}.dll".format(major_version))
        else:
            py3_filenames.append("python{}{}.lib".format(major_version, minor_version))
            py3_filenames.append("python{}{}.dll".format(major_version, minor_version))

    files = py3_runtime.files
    filelist = files.to_list()
    py3_libs = []
    for file in filelist:
        if file.basename in py3_filenames:
            py3_libs.append(file)

    return [DefaultInfo(
        files = depset(py3_libs),
    )]

# Get the appropriate libpython binaries for the current Python
# toolchain. This rule is used inside a cc_library rule in the root
# BUILD file. This fixes the current bazel_rules libpython
# implementation on Windows which is currently bugged.
# https://github.com/bazelbuild/rules_python/issues/1823
py3_libs = rule(
    implementation = _py3_libs,
    attrs = {
        "stable_api": attr.bool(
            default=False
        )
    },
    toolchains = [
        str(Label("@bazel_tools//tools/python:toolchain_type")),
    ],    
)

def _py_embedded_libs_impl(ctx):
    deps = ctx.attr.deps
    toolchain = ctx.toolchains["@bazel_tools//tools/python:toolchain_type"]
    py3_runtime = toolchain.py3_runtime

    # addresses that need to be added to python sys.path
    all_imports = []        
    for lib in deps:
        all_imports.append(lib[PyInfo].imports)
    imports_txt = "\n".join(depset(transitive = all_imports).to_list())
    imports_file = ctx.actions.declare_file(ctx.attr.name + ".imports")
    ctx.actions.write(imports_file, imports_txt)

    python_home_txt = str(py3_runtime.interpreter.dirname)
    if python_home_txt.endswith("/bin"):
        # prefix/bin => prefix
        python_home_txt = python_home_txt[: -len("/bin")]
    if python_home_txt.startswith("external/"):
        # fix for bazel8, the runfiles location will not be prefixed by "external/"
        python_home_txt = python_home_txt[len("external/"):]
    
    python_home_file = ctx.actions.declare_file(ctx.attr.name + ".python_home")
    ctx.actions.write(python_home_file, python_home_txt)
    
    py3_runfiles = ctx.runfiles(files = py3_runtime.files.to_list())
    dep_runfiles = [py3_runfiles]
    for lib in deps:
        lib_runfiles = ctx.runfiles(files = lib[PyInfo].transitive_sources.to_list())
        dep_runfiles.append(lib_runfiles)
        dep_runfiles.append(lib[DefaultInfo].default_runfiles)

    runfiles = ctx.runfiles().merge_all(dep_runfiles)

    return [DefaultInfo(files=depset([imports_file, python_home_file]),
                        runfiles=runfiles
                        )]

# collect paths to all files of a python library and generate .imports file
py_embedded_libs = rule(
    implementation = _py_embedded_libs_impl,
    attrs = {
        "deps": attr.label_list(
            providers = [PyInfo], 
        ),
    },
    toolchains = [
        str(Label("@bazel_tools//tools/python:toolchain_type")),
    ],        
)


def _collect_dlls_from_cc_info(ctx, cc_info):
    # Get all the linker inputs
    dll_files = []

    # Iterate over all linker inputs from the CcLinkingContext
    for linker_input in cc_info.linking_context.linker_inputs.to_list():
        # Each linker_input contains libraries
        for library in linker_input.libraries:
            # Filter out only .dll files which are binary sources
            if library.dynamic_library and library.dynamic_library.basename.endswith(".dll") \
               and library.dynamic_library.is_source:
                dll_files.append(library.dynamic_library)

    return dll_files

def _collect_dlls_impl(ctx):
    # Initialize an empty depset to collect all .dll files
    dll_files = depset()

    # Iterate over the deps attribute to collect .dll files from each dep
    for dep in ctx.attr.deps:
        if CcInfo in dep:
             dll_files = depset(transitive = [dll_files], direct = _collect_dlls_from_cc_info(ctx, dep[CcInfo]))

    dll_symlinks = []
    for dll in dll_files.to_list():
        # dll_symlink_path = "{}/{}".format(ctx.label.name + "_dlls", dll.basename)
        dll_symlink_path = dll.basename
        dll_symlink = ctx.actions.declare_file(dll_symlink_path)
        dll_symlinks.append(dll_symlink)
        
        ctx.actions.symlink(
            output = dll_symlink,
            target_file = dll,
        )
    
    dll_files = depset(transitive = [dll_files], direct = dll_symlinks)

    # Return the directory containing the symlinked .dll files as output
    return [DefaultInfo(files = dll_files)]    

# this rule looks into the cc dependencies and symlinks all dlls into
# the output directory
collect_dlls = rule(
    implementation = _collect_dlls_impl,
    attrs = {
        "deps": attr.label_list(providers = [CcInfo], allow_files=True),
    },
)

# wrapper around the original nanobind_extension which fixes dll behavior on windows
def nanobind_extension(
    name,
    copts = [],
    features = [],
    linkopts = [],
    tags = [],
    deps = [],
    **kwargs):

    nanobind_extension_orig(
        name + "_orig",
        copts = copts,
        features = features,
        linkopts = linkopts,
        tags = tags,
        deps = deps,
        **kwargs)

    # Windows fix: The extension may need runtime dlls to be in the
    # same folder, eg assimp, if the extension depends on a dll. The
    # normal nanobind_extension rule does not account for this, leading
    # to ImportError
    collect_dlls(name = name + "_dlls", deps=deps)

    copy_file(
        name = name + "_orig_rename",
        src = name + "_orig.so",
        out = name + ".so",
        testonly = kwargs.get("testonly"),
        visibility = kwargs.get("visibility"),
    )

    copy_file(
        name = name + "_pyd_rename",
        src = name + "_orig.so",
        out = name + ".pyd",
        testonly = kwargs.get("testonly"),
        visibility = kwargs.get("visibility"),
    )
    
    native.filegroup(
        name = name,
        srcs = select({
            "@platforms//os:windows": [name + ".pyd"],
            "//conditions:default": [name + ".so"],
        }),
        data = [name + "_dlls"],
    )    
