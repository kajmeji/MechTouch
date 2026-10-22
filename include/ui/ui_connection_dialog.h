#pragma once
#include <lvgl.h>

// View construction only. UICommon owns connection state and event handlers.
namespace UIConnectionDialog {
lv_obj_t *connecting(const char *machine, const char *ssid);
lv_obj_t *error(const char *title, const char *message,
                lv_event_cb_t retry, lv_event_cb_t restart, lv_event_cb_t close);
lv_obj_t *restarting();
}
