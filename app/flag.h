#pragma once

#include "absl/flags/flag.h"
#include <cstdint>
#include <string>

// different flag library
#define APP_FLAG ABSL_FLAG
#define APP_GET_FLAG(name) absl::GetFlag(FLAGS_##name)
#define APP_UNPACK_FLAG(name) auto name = APP_GET_FLAG(name)
