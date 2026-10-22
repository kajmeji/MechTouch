#pragma once
#include <lvgl.h>
namespace UIPowerDialog {
void show(lv_event_cb_t confirm);
void hide();
void showBusy();
}
