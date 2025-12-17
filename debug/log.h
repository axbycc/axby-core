#pragma once

#ifndef LOG
#include "absl/log/log.h"
#endif

// convenience package to include absl's
// log.h and check.h under one header

#ifndef LOG_EVERY_T
// conversion from glog to absl
#define LOG_EVERY_T LOG_EVERY_N_SEC
#endif

