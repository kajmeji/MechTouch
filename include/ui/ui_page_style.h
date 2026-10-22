#include "ui/ui_language.h"
#pragma once
#include "ui/ui_alerts.h"

// Shared styling for the migrated pages and their input dialogs.
extern "C" {
LV_FONT_DECLARE(ft_bold_28);
LV_FONT_DECLARE(ft_bold_20);
}
namespace UIPageStyle {
using namespace UIAlerts;
static constexpr lv_color_t BTN_DISCONNECT = STATE_ALARM;
static constexpr lv_color_t BTN_ESTOP = STATE_ALARM;
static constexpr lv_color_t LIME = LV_COLOR_MAKE(0xd6,0xe9,0x80);
static constexpr lv_color_t ACCENT_PRIMARY_PRESSED = ACCENT_SECONDARY;
static constexpr lv_color_t AXIS_A = ACCENT_PRIMARY;
static constexpr lv_color_t AXIS_XY = ACCENT_PRIMARY;
static constexpr lv_color_t JOYSTICK_BG = BG_BUTTON;
static constexpr lv_color_t JOYSTICK_BORDER = BORDER_MEDIUM;
static constexpr lv_color_t JOYSTICK_LINE = BORDER_MEDIUM;
static constexpr lv_color_t JOYSTICK_XY = ACCENT_PRIMARY;
static constexpr lv_color_t JOYSTICK_Z = ACCENT_PRIMARY;
inline void background(lv_obj_t *obj, lv_color_t color, lv_style_selector_t part) {
    UIAlerts::background(obj, color, part);
    if (lv_obj_check_type(obj, &lv_button_class)) {
        auto rgb = lv_color_to_u32(color);
        unsigned brightness = (((rgb >> 16) & 255) * 299 + ((rgb >> 8) & 255) * 587 + (rgb & 255) * 114) / 1000;
        lv_obj_set_style_text_color(obj, brightness < 145 ? BG_DARK : TEXT_LIGHT, part);
    }
}
inline lv_obj_t *button(lv_obj_t *parent) { return UIAlerts::button(parent); }
inline void title(lv_obj_t *parent, const char *text, int x, int y) {
    auto *label = lv_label_create(parent);
    UILanguage::setText(label, text);
    UILanguage::setFont(label, &ft_bold_28, 0);
    lv_obj_set_style_text_color(label, TEXT_LIGHT, 0);
    lv_obj_set_pos(label, x, y);
}
inline void dropdown(lv_obj_t *obj) {
    background(obj, BG_BUTTON, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    auto *list = lv_dropdown_get_list(obj);
    if (list) {
        background(list, BG_DARK, 0);
        lv_obj_set_style_border_width(list, 1, 0);
        lv_obj_set_style_bg_color(list, ACCENT_PRIMARY, LV_PART_SELECTED);
        lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SELECTED);
        lv_obj_set_style_text_color(list, BG_DARK, LV_PART_SELECTED);
        lv_obj_set_style_bg_color(list, ACCENT_PRIMARY, static_cast<lv_style_selector_t>(LV_PART_SELECTED) | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(list, BG_DARK, static_cast<lv_style_selector_t>(LV_PART_SELECTED) | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(list, ACCENT_SECONDARY, static_cast<lv_style_selector_t>(LV_PART_SELECTED) | LV_STATE_PRESSED);
        lv_obj_set_style_text_color(list, BG_DARK, static_cast<lv_style_selector_t>(LV_PART_SELECTED) | LV_STATE_PRESSED);
    }
}
inline void dropdownEvent(lv_event_t *e) {
    dropdown(static_cast<lv_obj_t *>(lv_event_get_target(e)));
}
inline lv_obj_t *createDropdown(lv_obj_t *parent) {
    auto *obj = lv_dropdown_create(parent);
    dropdown(obj);
    lv_obj_add_event_cb(obj, dropdownEvent, LV_EVENT_READY, nullptr);
    return obj;
}
inline lv_obj_t *textarea(lv_obj_t *parent) {
    auto *obj = lv_textarea_create(parent);
    background(obj, BG_BUTTON, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, ACCENT_PRIMARY, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(obj, BG_DARK, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(obj, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(obj, TEXT_DISABLED, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_bg_color(obj, LIME, LV_PART_SELECTED);
    lv_obj_set_style_text_color(obj, TEXT_LIGHT, LV_PART_SELECTED);
    lv_obj_set_style_border_color(obj, ACCENT_PRIMARY, LV_PART_CURSOR);
    return obj;
}
inline lv_obj_t *toggle(lv_obj_t *parent) {
    auto *obj = lv_switch_create(parent);
    lv_obj_set_style_bg_color(obj, BORDER_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, ACCENT_PRIMARY, static_cast<lv_style_selector_t>(LV_PART_INDICATOR) | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(obj, BG_DARK, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_KNOB);
    return obj;
}
inline void subTabs(lv_obj_t *tabview) {
    auto *bar = lv_tabview_get_tab_bar(tabview);
    background(bar, ACCENT_PRIMARY, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    for (uint32_t i = 0; i < lv_obj_get_child_count(bar); ++i) {
        auto *btn = lv_obj_get_child(bar, i);
        background(btn, ACCENT_PRIMARY, 0);
        background(btn, LIME, LV_STATE_CHECKED);
        background(btn, ACCENT_SECONDARY, LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 4, LV_STATE_CHECKED);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_RIGHT, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, ACCENT_PRIMARY, LV_STATE_CHECKED);
        UILanguage::setFont(btn, &ft_bold_20, 0);
    }
}
inline void colorStripe(lv_obj_t *button, lv_color_t color) {
    background(button, BG_DARK, 0);
    lv_obj_set_style_pad_all(button, 10, 0);
    auto *stripe = lv_obj_create(button);
    lv_obj_set_size(stripe, 5, LV_PCT(100));
    lv_obj_align(stripe, LV_ALIGN_LEFT_MID, -8, 0);
    lv_obj_set_style_bg_color(stripe, color, 0);
    lv_obj_set_style_border_width(stripe, 0, 0);
    lv_obj_set_style_radius(stripe, 2, 0);
    lv_obj_clear_flag(stripe, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE);
}
}
