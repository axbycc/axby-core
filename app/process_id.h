#pragma once

#include <cstdint>

namespace axby {

uint64_t get_process_id();

// usually process id is set randomly on startup, but for playback we
// may want to force a process id for some systems (eg time_sync) to
// work properly
void force_process_id(uint64_t forced_process_id);

}  // axby
 
