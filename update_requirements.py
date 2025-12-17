import sys
import os
import importlib.metadata

import pip
import pip._internal


# BEGIN MONKEYPATCH
import pip_monkeypatch
from pip._internal.operations import prepare
prepare.RequirementPreparer = pip_monkeypatch.RequirementPreparer
# END MONKEYPATCH

from piptools.scripts.compile import cli

def get_requirements_out_name(plat):
    if plat == "win32":
        return "requirements_windows.txt"
    return "requirements.txt"
    
if __name__ == "__main__":

    if not "BUILD_WORKSPACE_DIRECTORY" in os.environ:
        print("Not running from bazel launcher so I don't know where the project root is.")
        exit(-1)

    project_root = os.environ["BUILD_WORKSPACE_DIRECTORY"]

    requirements_out = os.path.join(project_root, get_requirements_out_name(sys.platform))
    requirements_in = os.path.join(project_root, "requirements.in")

    cache_path = os.path.join(project_root, "_pip_download_cache")
    pip_monkeypatch.MONKEYPATCH_ARGS["DOWNLOAD_DIR"] = cache_path

    base_args = [requirements_in, "-o", requirements_out, "--cache-dir", cache_path]
    other_args = sys.argv[1:]
    pip_compile_args = base_args + other_args

    piptools_version = importlib.metadata.version('pip-tools')
    print(f"Running pip-tools {piptools_version}")
    print(f"pip-compile {' '.join(pip_compile_args)}")
    cli(pip_compile_args)
