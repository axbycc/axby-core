#include <thread>

#include "random/linear_congruential_generator.h"
#include "app/main.h"
#include "app/timing.h"
#include "app/stop_all.h"
#include "debug/log.h"
#include "ring_buffer.h"

using namespace axby;

struct MyObj {
    float a;
    float b;
};

RingBuffer<MyObj, 10> _ring_buffer;

float _total = 0;
void run_head_loop() {
    while (!should_stop_all()) {
        MyObj o;
        if (!_ring_buffer.move_read(o, /*blocking=*/true)) return;
        _total += o.a;
        _total += o.b;
    }
};

void run_tail_loop() {
    LinearCongruentialGenerator lcg;
    while (!should_stop_all()) {
        MyObj o;
        o.a = lcg.generate();
        o.b = lcg.generate();
        _ring_buffer.write(o);
    }
};

int main(int argc, char *argv[]) {
    __APP_MAIN_INIT__;

    std::thread head_thread {run_head_loop};
    std::thread tail_thread {run_tail_loop};

    while (!should_stop_all()) {
        sleep_ms(1000);
    }

    _ring_buffer.stop();
    head_thread.join();
    tail_thread.join();

    LOG(INFO) << "Total " << _total;

    return 0;
}
