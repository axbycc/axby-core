load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("//:config.bzl", "py3_libs")

refresh_compile_commands(
    name = "refresh_compile_commands",
    targets = {
        "//...": "",
    },
)

# BEGIN PYTHON DEPENDENCY MANAGEMENT ==============================================

# Trigger by running `bazel run pip_patching_requirements.update`
# This should only need to be run once on Windows and once on Linux
# and then the resulting pip_patching_requirements.txt and
# pip_patching_requirements_windows.txt should be checked into the repo
#
# This populates the @pip_patching//pip:pkg and
# @pip_patching//pip_tools:pkg targets, which are needed by the
# pip_monkeypatch library, which provides a faster version of
# pip-compile than the one from bazel's rules_python.
compile_pip_requirements(
    name = "pip_patching_requirements",
    src = "pip_patching_requirements.in",
    requirements_txt = "pip_patching_requirements.txt",
    requirements_windows = "pip_patching_requirements_windows.txt",
)

py_library(
    name = "pip_monkeypatch",
    srcs = ["pip_monkeypatch.py"],
    deps = [
        "@pip_patching//pip:pkg",
        "@pip_patching//pip_tools:pkg",
    ],
)

# This updates the python requirements.txt file for pip
# dependencies. The input is is requirements.in in this directory.
py_binary(
    name = "update_requirements",
    srcs = ["update_requirements.py"],
    args = [
        "--verbose",
        "--allow-unsafe",
        "--emit-index-url",
        "--extra-index-url",
        "https://download.pytorch.org/whl/cu124",
        "--trusted-host",
        "download.pytorch.org",
    ],
    data = [
        "requirements.in",
    ],
    deps = [
        # newer version of pip with v2 http caching,
        "@pip_patching//pip:pkg",
        "@pip_patching//pip_tools:pkg",
        ":pip_monkeypatch",
    ],
)
# END PYTHON DEPENDENCY MANAGEMENT =============================================

# BEGIN libpython fix for Windows ==============================================
py3_libs(
    name = "current_py3_stable_libs",
    stable_api = True,
)

py3_libs(
    name = "current_py3_unstable_libs",
    stable_api = False,
)

cc_library(
    name = "current_libpython_stable",
    srcs = [":current_py3_stable_libs"],
    visibility = ["//visibility:public"],
    deps = [
        "@rules_python//python/cc:current_py_cc_headers",
    ],
)

cc_library(
    name = "current_libpython_unstable",
    srcs = [":current_py3_unstable_libs"],
    visibility = ["//visibility:public"],
    deps = [
        "@rules_python//python/cc:current_py_cc_headers",
    ],
)
# END libpython fix ============================================================
