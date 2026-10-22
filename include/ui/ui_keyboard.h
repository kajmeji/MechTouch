#pragma once
#include <lvgl.h>

// Shared English keyboard. Callers keep their existing READY/CANCEL semantics,
// input restrictions, textarea attachment and viewport positioning.
namespace UIKeyboard {
lv_obj_t *create(lv_obj_t *parent);
}
