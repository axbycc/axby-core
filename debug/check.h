#pragma once

#ifndef CHECK
#include "absl/log/check.h"
#endif

#ifndef CHECK_NOTNULL
#define CHECK_NOTNULL(item) CHECK(item != nullptr)
#endif
