#pragma once
#include <cstdint>
struct lv_timer_t { bool paused=false; };
struct lv_display_t { lv_timer_t timer; };
inline lv_timer_t *lv_display_get_refr_timer(lv_display_t *d) { return &d->timer; }
inline void lv_timer_pause(lv_timer_t *t) { t->paused=true; }
inline void lv_timer_resume(lv_timer_t *t) { t->paused=false; }
inline void *lv_screen_active() { return nullptr; }
inline void lv_obj_invalidate(void *) {}
struct DisplayDriver {
    uint8_t brightness=0;
    lv_display_t display;
    void setBacklight(uint8_t b) { brightness=b; }
    void setBacklightOff() { brightness=0; }
    lv_display_t *getDisplay() { return &display; }
};
