#ifndef FLUIDTOUCH_UI_PERF_H
#define FLUIDTOUCH_UI_PERF_H
#include <stdint.h>

// Main/LVGL thread only. Inline accessor shares one instance across .cpp files.
// Monotonic counters allow each handler call to take independent snapshots.
struct UIWorkCounters {
    uint64_t touch_us = 0;
    uint64_t swap_us = 0;
    uint64_t push_us = 0;
    uint64_t pixels = 0;
    uint64_t flushes = 0;
};
inline UIWorkCounters &uiWorkCounters() {
    static UIWorkCounters counters;
    return counters;
}
#endif
