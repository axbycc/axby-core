#pragma once

// initialize common utilities that need argv / argc
// - abseil flags
// - abseil log
// - runfiles
void app_main_init(int argc, char *argv[]);

#ifndef __APP_MAIN_INIT__
#define __APP_MAIN_INIT__ app_main_init(argc, argv)
#endif
