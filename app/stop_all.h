#pragma once

#include <functional>

namespace axby {

void stop_all();

bool should_stop_all();

// register a callback to be called when stop_all() is called. the
// callback will be issued from whatever thread calls stop_all(),
// which could be any thread. so any objects accessed by the callback
// must be thread safe.
void on_stop_all(std::function<void()> callback);

}
