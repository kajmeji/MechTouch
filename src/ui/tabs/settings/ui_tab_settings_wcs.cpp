#include "ui/ui_language.h"
#include "ui/ui_keyboard.h"
#include "ui/tabs/settings/ui_tab_settings_wcs.h"
#include "ui/ui_page_style.h"
#include "ui/wcs_config.h"
#include "ui/machine_config.h"
#include "ui/fonts/fontawesome_icons_20.h"
#include "config.h"
#include <Arduino.h>

// Static member initialization
lv_obj_t *UITabSettingsWCS::name_inputs[6] = {nullptr};
lv_obj_t *UITabSettingsWCS::lock_buttons[6] = {nullptr};
lv_obj_t *UITabSettingsWCS::keyboard = nullptr;
lv_obj_t *UITabSettingsWCS::parent_tab = nullptr;
lv_obj_t *UITabSettingsWCS::status_label = nullptr;

// Textarea focused event handler - show keyboard
static void textarea_focused_event_handler(lv_event_t *e) {
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
    UITabSettingsWCS::showKeyboard(ta);
}

// Show keyboard
void UITabSettingsWCS::showKeyboard(lv_obj_t *ta) {
    if (!keyboard) {
        keyboard = UIKeyboard::create(lv_scr_act());
        lv_obj_set_size(keyboard, SCREEN_WIDTH, 220);
        lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        UILanguage::setFont(keyboard, &lv_font_montserrat_20, 0);
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
        lv_obj_add_event_cb(keyboard, [](lv_event_t *e) { UITabSettingsWCS::hideKeyboard(); }, LV_EVENT_READY, nullptr);
        lv_obj_add_event_cb(keyboard, [](lv_event_t *e) { UITabSettingsWCS::hideKeyboard(); }, LV_EVENT_CANCEL, nullptr);
        if (parent_tab) {
            lv_obj_add_event_cb(parent_tab, [](lv_event_t *e) { UITabSettingsWCS::hideKeyboard(); }, LV_EVENT_CLICKED, nullptr);
        }
    }
    
    // Enable scrolling on parent tab and add extra padding at bottom for keyboard
    if (parent_tab) {
        lv_obj_add_flag(parent_tab, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_bottom(parent_tab, 240, 0);
    }
    
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    
    // Scroll the parent tab to position the focused textarea just above keyboard
    if (parent_tab && ta) {
        lv_coord_t ta_y = lv_obj_get_y(ta);
        lv_obj_t *parent = lv_obj_get_parent(ta);
        
        while (parent && parent != parent_tab) {
            ta_y += lv_obj_get_y(parent);
            parent = lv_obj_get_parent(parent);
        }
        
        lv_coord_t visible_height = 200;
        lv_coord_t ta_height = lv_obj_get_height(ta);
        lv_coord_t target_position = visible_height - ta_height - 10;
        
        lv_coord_t scroll_y = ta_y - target_position;
        if (scroll_y < 0) scroll_y = 0;
        
        lv_obj_scroll_to_y(parent_tab, scroll_y, LV_ANIM_ON);
    }
}

// Hide keyboard
void UITabSettingsWCS::hideKeyboard() {
    if (keyboard) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        
        if (parent_tab) {
            lv_obj_clear_flag(parent_tab, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_bottom(parent_tab, 10, 0);
            lv_obj_scroll_to_y(parent_tab, 0, LV_ANIM_ON);
        }
    }
}

// Save button event handler
void UITabSettingsWCS::btn_save_event_handler(lv_event_t *e) {
    int machine_index = MachineConfigManager::getSelectedMachineIndex();
    
    // Save all name and lock values
    for (int i = 0; i < 6; i++) {
        const char* name = lv_textarea_get_text(name_inputs[i]);
        bool locked = lv_obj_has_state(lock_buttons[i], LV_STATE_CHECKED);
        
        WCSConfig::saveWCSName(machine_index, i, name);
        WCSConfig::saveWCSLock(machine_index, i, locked);
    }
    
    // Show confirmation message
    if (status_label) {
        UILanguage::setText(status_label, "WCS settings saved");
        lv_obj_set_style_text_color(status_label, UIPageStyle::UI_SUCCESS, 0);
    }
    
    Serial.println("[WCS Settings] All settings saved");
}

// Reset to defaults button event handler
void UITabSettingsWCS::btn_reset_event_handler(lv_event_t *e) {
    // Reset to default values (empty names, unlocked)
    for (int i = 0; i < 6; i++) {
        lv_textarea_set_text(name_inputs[i], "");
        lv_obj_clear_state(lock_buttons[i], LV_STATE_CHECKED);
        // Update button label to show unlock icon
        lv_obj_t *label = lv_obj_get_child(lock_buttons[i], 0);
        if (label) {
            UILanguage::setText(label, FA_ICON_UNLOCK);
        }
    }
    
    // Show confirmation message
    if (status_label) {
        UILanguage::setText(status_label, "Reset to defaults (not saved)");
        lv_obj_set_style_text_color(status_label, UIPageStyle::UI_INFO, 0);
    }
    
    Serial.println("[WCS Settings] Reset to defaults");
}

// Lock button click handler - update icon based on state
// Note: LVGL automatically toggles LV_STATE_CHECKED due to LV_OBJ_FLAG_CHECKABLE
static void lock_button_event_handler(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    
    // Check state AFTER LVGL's automatic toggle (triggered by CHECKABLE flag)
    if (label) {
        if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {
            UILanguage::setText(label, FA_ICON_LOCK);
        } else {
            UILanguage::setText(label, FA_ICON_UNLOCK);
        }
    }
}

void UITabSettingsWCS::create(lv_obj_t *parent) {
    parent_tab = parent;
    
    // Title with 20px top margin
    lv_obj_t *title = lv_label_create(parent);
    UILanguage::setText(title, "WORK COORDINATE SYSTEMS");
    UILanguage::setFont(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, UIPageStyle::TEXT_DISABLED, 0);
    lv_obj_set_pos(title, 20, 20);
    
    // Load current WCS configuration
    int machine_index = MachineConfigManager::getSelectedMachineIndex();
    char names[6][32];
    bool locks[6];
    WCSConfig::loadWCSConfig(machine_index, names, locks);
    
    const char* wcs_labels[] = {"G54", "G55", "G56", "G57", "G58", "G59"};
    
    // Two-column layout: G54-G56 (left), G57-G59 (right)
    // Dimensions match probe/jog tabs: labels aligned at y+12, fields 45px height, 50px row spacing
    int left_x = 20;       // Left column X
    int right_x = 350;     // Right column X (moved 30px closer)
    int y_start = 60;      // Starting Y position (after title)
    int field_height = 45; // Match probe tab textarea height
    int row_spacing = 50;  // Match probe/jog settings
    int name_width = 200;  // Increased by 10px
    
    for (int i = 0; i < 6; i++) {
        // Determine position: left column (0-2) or right column (3-5)
        int col_x = (i < 3) ? left_x : right_x;
        int row = (i < 3) ? i : (i - 3);
        int y_pos = y_start + (row * row_spacing);
        
        // WCS label (G54-G59) - aligned with text area content at y+12
        lv_obj_t *lbl_wcs = lv_label_create(parent);
        UILanguage::setText(lbl_wcs, wcs_labels[i]);
        UILanguage::setFont(lbl_wcs, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(lbl_wcs, UIPageStyle::POS_MODAL, 0);
        lv_obj_set_pos(lbl_wcs, col_x, y_pos + 12);  // Align with text area content
        
        // Name input text area - 180px wide to fit both columns
        name_inputs[i] = UIPageStyle::textarea(parent);
        lv_textarea_set_one_line(name_inputs[i], true);
        UILanguage::setPlaceholder(name_inputs[i], "Custom name");
        lv_textarea_set_text(name_inputs[i], names[i]);
        lv_obj_set_size(name_inputs[i], name_width, field_height);
        lv_obj_set_pos(name_inputs[i], col_x + 45, y_pos);
        UILanguage::setFont(name_inputs[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_pad_left(name_inputs[i], 5, 0);  // Left padding to align text to left
        lv_obj_set_style_pad_right(name_inputs[i], 2, 0);  // Minimal right padding
        lv_obj_clear_flag(name_inputs[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(name_inputs[i], textarea_focused_event_handler, LV_EVENT_FOCUSED, nullptr);
        
        // Lock/unlock toggle button (45x45px with icon)
        lock_buttons[i] = UIPageStyle::button(parent);
        lv_obj_set_size(lock_buttons[i], field_height, field_height);
        lv_obj_set_pos(lock_buttons[i], col_x + 255, y_pos);  // 45 (label) + 200 (field) + 10 (gap) = 255
        UIPageStyle::background(lock_buttons[i], UIPageStyle::BG_BUTTON, LV_PART_MAIN | LV_STATE_DEFAULT);
        UIPageStyle::background(lock_buttons[i], UIPageStyle::ACCENT_PRIMARY, LV_STATE_CHECKED);
        lv_obj_add_flag(lock_buttons[i], LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(lock_buttons[i], lock_button_event_handler, LV_EVENT_CLICKED, nullptr);
        
        // Set initial lock state and icon
        if (locks[i]) {
            lv_obj_add_state(lock_buttons[i], LV_STATE_CHECKED);
        }
        
        // Lock/unlock icon label
        lv_obj_t *lbl_icon = lv_label_create(lock_buttons[i]);
        UILanguage::setText(lbl_icon, locks[i] ? FA_ICON_LOCK : FA_ICON_UNLOCK);
        UILanguage::setFont(lbl_icon, &fontawesome_icons_20, 0);
        lv_obj_center(lbl_icon);
    }
    
    // === Action Buttons (positioned at bottom like probe/jog tabs) ===
    // Save button (y=280 matches probe/jog positioning)
    lv_obj_t *btn_save = UIPageStyle::button(parent);
    lv_obj_set_size(btn_save, 180, 50);
    lv_obj_set_pos(btn_save, 20, 280);
    UIPageStyle::background(btn_save, UIPageStyle::BTN_PLAY, LV_PART_MAIN);
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    UILanguage::setText(lbl_save, "Save Settings");
    UILanguage::setFont(lbl_save, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_save);
    lv_obj_add_event_cb(btn_save, btn_save_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Reset to defaults button (200px gap from Save button)
    lv_obj_t *btn_reset = UIPageStyle::button(parent);
    lv_obj_set_size(btn_reset, 180, 50);
    lv_obj_set_pos(btn_reset, 220, 280);
    UIPageStyle::background(btn_reset, UIPageStyle::BG_BUTTON, LV_PART_MAIN);
    lv_obj_t *lbl_reset = lv_label_create(btn_reset);
    UILanguage::setText(lbl_reset, "Reset Defaults");
    UILanguage::setFont(lbl_reset, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_reset);
    lv_obj_add_event_cb(btn_reset, btn_reset_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Status label (positioned below buttons)
    status_label = lv_label_create(parent);
    UILanguage::setText(status_label, "");
    UILanguage::setFont(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label, UIPageStyle::UI_INFO, 0);
    lv_obj_set_pos(status_label, 20, 335);
}
