def _is_windows(repo_ctx):
    return repo_ctx.os.name.lower().startswith("windows")


windows_system_build = \
r"""
package(default_visibility = ["//visibility:public"])

# stub so that hedron compile commands can succeed since it wants to
# run aquery even for linux-only targets
cc_library(
  name = "drake",
)
cc_library(
  name = "glog",
)
cc_library(
  name = "gflags",
)
cc_library(
  name = "suitesparse",
)
cc_library(
  name = "atlas",
)



cc_library(
  name = "cudnn",
  includes = ["d/system_libs/CUDNN/v9.0/include"],
  srcs = glob([
      "d/system_libs/CUDNN/v9.0/bin/*.dll",
      "d/system_libs/CUDNN/v9.0/lib/x64/*.lib"
    ],)
)

# ================= onnxruntime ==============================
# git clone official onnx runtime repo https://github.com/microsoft/onnxruntime
# make sure cuda is installed
# make sure tensorrt is installed
# build with the following command, changing paths to nvidia related deps if necessary
# build.bat --use_cuda --cudnn_home "C:\Program Files\NVIDIA\CUDNN\v9.0\include" --cuda_home "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4" --config RelWithDebInfo --build_shared_lib --parallel --compile_no_warning_as_error --skip_submodule_sync --use_tensorrt --tensorrt_home "C:\Program Files\NVIDIA GPU Computing Toolkit\TensorRT-10.8.0.43"
# then `cmake --instal . --config=RelWithDebInfo --prefix="D:\onnxruntime_install"`

cc_library(
  name = "tensorrt",
  srcs = glob([
    "d/system_libs/TensorRT/lib/*.dll",
    # "d/system_libs/TensorRT/lib/*.lib",
  ]),
)

cc_import(
  name = "ort_tensorrt_ep",
  shared_library = "d/system_libs/onnxruntime/lib/onnxruntime_providers_tensorrt.dll",
)

cc_import(
  name = "ort_cuda_ep",
  shared_library = "d/system_libs/onnxruntime/lib/onnxruntime_providers_cuda.dll",
)

cc_library(
  name = "onnxruntime",
  includes = ["d/system_libs/onnxruntime/include/onnxruntime"],
  srcs = glob([
    "d/system_libs/onnxruntime/bin/onnxruntime.dll",
    "d/system_libs/onnxruntime/bin/onnxruntime_providers_shared.dll",
    "d/system_libs/onnxruntime/lib/onnxruntime.lib",
    "d/system_libs/onnxruntime/lib/onnxruntime_providers_shared.lib",
  ]),
  deps = [
    ":ort_tensorrt_ep",
    ":ort_cuda_ep",
    ":tensorrt",
  ],
)
"""

linux_system_build = \
"""
package(default_visibility = ["//visibility:public"])

# download the source from opencv website
# extract and do the regular cmake thing
#   mkdir build
#   cmake ..
#   make -j6
#   sudo make install
cc_library(
  name = "opencv",
  hdrs = glob([
    "usr/local/include/opencv4/**/*.h",
    "usr/local/include/opencv4/**/*.hpp",
  ]),
  srcs = glob([
    "usr/local/lib/libopencv*"
  ]),
  strip_include_prefix="usr/local/include/opencv4",
)

cc_library(
  name = "cudnn_hdrs",
  hdrs = glob(["usr/include/cudnn*.h"]),
  strip_include_prefix="usr/include",
)

cc_library(
  name = "cudnn",
  hdrs = [":cudnn_hdrs"],
  srcs = [
    "usr/lib/x86_64-linux-gnu/libcudnn.so.9"
  ]
)

# ================= onnxruntime ==============================
# git clone official onnx runtime repo https://github.com/microsoft/onnxruntime
# make sure cuda is installed
# make sure tensorrt is installed
# build with the following command, changing paths to nvidia related deps if necessary
# ./build.sh --config RelWithDebInfo --build_shared_lib --parallel --compile_no_warning_as_error --skip_submodule_sync --use_cuda --cuda_home /usr/local/cuda-12.5 --cudnn_home /usr/lib/x86_64-linux-gnu --skip_tests --use_tensorrt --tensorrt_home /usr/lib/x86_64-linux-gnu
# then `sudo make install` in the build directory
# then
#   echo "/usr/local/onnxruntime/lib" | sudo tee /etc/ld.so.conf.d/onnxruntime.conf
#   sudo ldconfig
#
# we can't use the usual method of including the .so inside the srcs
# since we want to avoid linking against onnxruntime_providers_cuda
# due to https://github.com/microsoft/onnxruntime/issues/16146
 
cc_library(
  name = "onnxruntime",
  hdrs = glob([
    "usr/local/onnxruntime/include/**/*.h",
    "usr/local/onnxruntime/include/**/*.hpp"
  ]),

  # usually we would use strip_include_prefix
  # but onnxruntime makes use of relative includes
  # which makes strip_include_prefix unworkable
  includes = [
    "usr/local/onnxruntime/include/onnxruntime",
  ],

  srcs = [
    "usr/local/onnxruntime/lib/libonnxruntime.so",
    "usr/local/onnxruntime/lib/libonnxruntime_providers_shared.so",
  ],
)
# ================ end onnxruntime ============================

# sudo apt-get install libgoogle-glog-dev 
cc_library(
  name = "glog",
  hdrs = glob(["usr/include/glog/*.h"]),
  srcs = glob(["usr/lib/x86_64-linux-gnu/libglog.so*"]),
  strip_include_prefix="usr/include", 
)

# sudo apt-get install libgflags-dev
cc_library(
  name = "gflags",
  hdrs = glob(["usr/include/gflags/*.h"]),

  # here the shared lib needed to use gflags from python extension
  # because the .a file is not compiled with -fPIC
  srcs = ["usr/lib/x86_64-linux-gnu/libgflags.so"], 

  strip_include_prefix="usr/include",
)

# sudo apt-get install libatlas-base-dev
cc_library(
  name = "atlas",
  # hdrs = glob(["usr/include/atlas/*.h"]),
  srcs = glob(["usr/lib/x86_64-linux-gnu/atlas/*.so"]) + [
    "usr/lib/x86_64-linux-gnu/libatlas.so",
    "usr/lib/x86_64-linux-gnu/libcblas.so",
    "usr/lib/x86_64-linux-gnu/libf77blas.so",
    "usr/lib/x86_64-linux-gnu/liblapack_atlas.so",
  ],
  strip_include_prefix="usr/include",
)

# sudo apt-get install libsuitesparse-dev
cc_library(
  name = "suitesparse",
  hdrs = glob(["usr/include/suitesparse/*.h"]),
  srcs = [
    "usr/lib/x86_64-linux-gnu/libamd.so",
    "usr/lib/x86_64-linux-gnu/libbtf.so",
    "usr/lib/x86_64-linux-gnu/libcamd.so",
    "usr/lib/x86_64-linux-gnu/libccolamd.so",
    "usr/lib/x86_64-linux-gnu/libcholmod.so",
    "usr/lib/x86_64-linux-gnu/libcolamd.so",
    "usr/lib/x86_64-linux-gnu/libcxsparse.so",
    "usr/lib/x86_64-linux-gnu/libklu.so",
    "usr/lib/x86_64-linux-gnu/libldl.so",
    "usr/lib/x86_64-linux-gnu/libmongoose.so",
    "usr/lib/x86_64-linux-gnu/librbio.so",
    "usr/lib/x86_64-linux-gnu/libsliplu.so",
    "usr/lib/x86_64-linux-gnu/libspqr.so",
    "usr/lib/x86_64-linux-gnu/libsuitesparseconfig.so",
    "usr/lib/x86_64-linux-gnu/libumfpack.so",
  ],
  strip_include_prefix="usr/include",
)


# apt install libvpx-dev
cc_library(
  name = "vpx",
  hdrs = glob(["usr/include/vpx/*.h"]),
  srcs = ["usr/lib/x86_64-linux-gnu/libvpx.a"],
  strip_include_prefix="usr/include",
)

# apt install libzmq3-dev
cc_library(
  name = "zmq",
  hdrs = ["usr/include/zmq.h", "usr/include/zmq_utils.h"],
  srcs = ["usr/lib/x86_64-linux-gnu/libzmq.so"],
  strip_include_prefix="usr/include"
)

# manual install from source to avoid xioctl(UVCIOC_CTRL_QUERY) error
# see https://github.com/IntelRealSense/librealsense/issues/2850
# due to unsupported kernel with the official distribution
# https://github.com/IntelRealSense/librealsense/blob/v2.56.3/doc/installation.md
cc_library(
  name = "realsense",
  hdrs = glob(["usr/local/include/librealsense2/**/*.h", "usr/local/include/librealsense2/**/*.hpp"]),
  srcs = ["usr/local/lib/librealsense2.so"],
  strip_include_prefix="usr/local/include",
)

# apt install libsdl2-dev
cc_library(
  name = "sdl",
  hdrs = glob(["usr/include/SDL2/*.h"]),
  srcs = ["usr/lib/x86_64-linux-gnu/libSDL2.so"],
  strip_include_prefix="usr/include/SDL2"
)

# copy force dimension libdhd.so, libdrd.so, etc into /usr/local/lib
# copy dhdc.h drdc.h into /usr/local/include/force_dimension
cc_library(
  name = "force_dimension",
  hdrs = glob(["usr/local/include/force_dimension/*.h"]),
  srcs = [
    # the examples from force dimension only link drd
    # and linking both looks like it causes double free
    # "usr/local/lib/libdhd.so",
    "usr/local/lib/libdrd.so"
  ],
  strip_include_prefix="usr/local/include/force_dimension"
)

# follow apt-install instructions https://drake.mit.edu/apt.html#stable-releases
cc_library(
  name = "drake",
  hdrs = glob(["opt/drake/**/*.h", "opt/drake/**/*.hpp"]),
  srcs = glob([
    "opt/drake/lib/*.so",
    "opt/drake/lib/libmosek64.so.10.1",
    "opt/drake/lib/libtbb.so.12",
  ]),
  strip_include_prefix="opt/drake/include",
)

# apt install libassimp-dev
cc_library(
  name = "assimp",
  hdrs = glob(["usr/include/assimp/**/*.h", "usr/include/assimp/**/*.hpp",]),
  srcs = ["usr/lib/x86_64-linux-gnu/libassimp.so"]
)
"""

def _system_deps_impl(repository_ctx):
    if _is_windows(repository_ctx):
        repository_ctx.symlink("C:/", "c")
        repository_ctx.symlink("D:/", "d")
        repository_ctx.file("BUILD", windows_system_build, executable=False)
    else:
        repository_ctx.symlink("/usr", "usr")
        repository_ctx.symlink("/opt", "opt")
        repository_ctx.file("BUILD", linux_system_build, executable=False)

system_deps = repository_rule(
    implementation = _system_deps_impl,
)


def _init(module_ctx):
    # instantiate the system deps repo
    system_deps(name = "system_deps")
    
make_system_deps = module_extension(implementation = _init)
