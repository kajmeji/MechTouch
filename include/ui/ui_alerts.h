#pragma once
#include <lvgl.h>
// Modal-only palette. Main pages retain their current theme until the next stage.
namespace UIAlerts {
void background(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector);
lv_obj_t *button(lv_obj_t *parent);
static constexpr lv_color_t BG_DARK = LV_COLOR_MAKE(0xf5,0xee,0xe4);
static constexpr lv_color_t BG_DARKER = LV_COLOR_MAKE(0xf5,0xee,0xe4);
static constexpr lv_color_t BG_MEDIUM = LV_COLOR_MAKE(0xf5,0xee,0xe4);
static constexpr lv_color_t BG_BUTTON = LV_COLOR_MAKE(0xed,0xe9,0xdf);
static constexpr lv_color_t BG_BLACK = LV_COLOR_MAKE(0xf5,0xee,0xe4);
static constexpr lv_color_t ACCENT_PRIMARY = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t ACCENT_SECONDARY = LV_COLOR_MAKE(0x28,0x5a,0x50);
static constexpr lv_color_t TEXT_LIGHT = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t TEXT_DARK = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t TEXT_MEDIUM = LV_COLOR_MAKE(0x49,0x69,0x5f);
static constexpr lv_color_t TEXT_DISABLED = LV_COLOR_MAKE(0x6b,0x80,0x78);
static constexpr lv_color_t BORDER_MEDIUM = LV_COLOR_MAKE(0xa3,0xb0,0xa7);
static constexpr lv_color_t BORDER_LIGHT = LV_COLOR_MAKE(0xa3,0xb0,0xa7);
static constexpr lv_color_t STATE_ALARM = LV_COLOR_MAKE(0xaf,0x4c,0x36);
static constexpr lv_color_t STATE_HOLD = LV_COLOR_MAKE(0xcd,0xb8,0x6d);
static constexpr lv_color_t BTN_PLAY = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t BTN_CONNECT = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t UI_WARNING = LV_COLOR_MAKE(0x94,0x67,0x25);
static constexpr lv_color_t UI_SUCCESS = LV_COLOR_MAKE(0x35,0x6b,0x39);
static constexpr lv_color_t UI_INFO = LV_COLOR_MAKE(0x28,0x5a,0x50);
static constexpr lv_color_t AXIS_X = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t AXIS_Y = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t AXIS_Z = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t POS_MODAL = LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t POS_WORK = LV_COLOR_MAKE(0x10,0x3f,0x38);
}
