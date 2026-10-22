#include "ui/ui_language.h"
#include "ui/ui_alerts.h"
#include "ui/tabs/settings/ui_tab_settings_backup.h"
#include "ui/ui_page_style.h"
#include "ui/settings_manager.h"
#include "config.h"
#include <Preferences.h>

// Global references for UI elements
static lv_obj_t *status_label = NULL;

// Forward declarations for event handlers
static void btn_export_event_handler(lv_event_t *e);
static void btn_clear_event_handler(lv_event_t *e);

void UITabSettingsBackup::create(lv_obj_t *tab) {
    // Set dark background
    UIPageStyle::background(tab, UIPageStyle::BG_MEDIUM, LV_PART_MAIN);
    
    // Disable scrolling for fixed layout
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    
    // === Backup & Restore Section ===
    lv_obj_t *section_title = lv_label_create(tab);
    UILanguage::setText(section_title, "BACKUP & RESTORE");
    UILanguage::setFont(section_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(section_title, UIPageStyle::TEXT_DISABLED, 0);
    lv_obj_set_pos(section_title, 20, 20);
    
    // Export settings button
    lv_obj_t *btn_export = UIPageStyle::button(tab);
    lv_obj_set_size(btn_export, 180, 50);
    lv_obj_set_pos(btn_export, 20, 60);
    UIPageStyle::background(btn_export, UIPageStyle::ACCENT_SECONDARY, LV_PART_MAIN);
    lv_obj_t *lbl_export = lv_label_create(btn_export);
    UILanguage::setText(lbl_export, LV_SYMBOL_DOWNLOAD " Export");
    UILanguage::setFont(lbl_export, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_export);
    lv_obj_add_event_cb(btn_export, btn_export_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Clear settings button
    lv_obj_t *btn_clear = UIPageStyle::button(tab);
    lv_obj_set_size(btn_clear, 180, 50);
    lv_obj_set_pos(btn_clear, 220, 60);
    UIPageStyle::background(btn_clear, UIPageStyle::STATE_ALARM, LV_PART_MAIN);
    lv_obj_t *lbl_clear = lv_label_create(btn_clear);
    UILanguage::setText(lbl_clear, LV_SYMBOL_TRASH " Clear All");
    UILanguage::setFont(lbl_clear, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_clear);
    lv_obj_add_event_cb(btn_clear, btn_clear_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Description for backup section
    lv_obj_t *backup_desc_label = lv_label_create(tab);
    UILanguage::setText(backup_desc_label, "Export saves a backup file to the Display SD card.\nClear All erases all settings and restarts the device.");
    UILanguage::setFont(backup_desc_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(backup_desc_label, UIPageStyle::TEXT_DISABLED, 0);
    lv_obj_set_pos(backup_desc_label, 20, 125);
    
    // Info section
    lv_obj_t *info_title = lv_label_create(tab);
    UILanguage::setText(info_title, "EXPORTED SETTINGS");
    UILanguage::setFont(info_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(info_title, UIPageStyle::TEXT_DISABLED, 0);
    lv_obj_set_pos(info_title, 20, 180);
    
    // What gets exported
    lv_obj_t *export_list = lv_label_create(tab);
    UILanguage::setText(export_list, 
        "The following items are saved to the backup file:\n"
        "• Machine configurations (name, connection, hostname/IP, port)\n"
        "• WiFi network names (passwords NOT included)\n"
        "• Jog settings (XY/Z feed rates)\n"
        "• Probe settings (feed rate, distances, thickness)\n"
        "• Macros\n"
        "• Power management settings\n"
        "• UI preferences");
    UILanguage::setFont(export_list, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(export_list, UIPageStyle::TEXT_LIGHT, 0);
    lv_obj_set_pos(export_list, 20, 215);
    lv_obj_set_width(export_list, 760);
    
    // Status label
    status_label = lv_label_create(tab);
    UILanguage::setText(status_label, "");
    UILanguage::setFont(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label, UIPageStyle::UI_INFO, 0);
    lv_obj_set_pos(status_label, 20, 335);
}

// Export settings button event handler
static void btn_export_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[SettingsBackup] Export button clicked");
        
        if (status_label != NULL) {
            UILanguage::setText(status_label, "Exporting...");
            lv_obj_set_style_text_color(status_label, UIPageStyle::UI_INFO, 0);
        }
        
        // Force LVGL update to show status
        lv_task_handler();
        
        // Attempt export
        if (SettingsManager::exportSettings()) {
            Serial.println("[SettingsBackup] Export successful");
            
            // Create modal backdrop
            lv_obj_t *backdrop = lv_obj_create(lv_layer_top());
            lv_obj_set_size(backdrop, SCREEN_WIDTH, SCREEN_HEIGHT);
            UIAlerts::background(backdrop, lv_color_hex(0x103f38), 0);
    lv_obj_set_style_pad_all(backdrop, 0, 0);
    lv_obj_set_style_radius(backdrop, 0, 0);
    lv_obj_set_pos(backdrop, 0, 0);
            lv_obj_set_style_bg_opa(backdrop, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(backdrop, 0, 0);
            lv_obj_clear_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_center(backdrop);
            
            // Show success dialog with important information
            lv_obj_t *dialog = lv_obj_create(backdrop);
            lv_obj_set_size(dialog, 650, 300);
            lv_obj_center(dialog);
            UIAlerts::background(dialog, UIAlerts::BG_MEDIUM, 0);
            lv_obj_set_style_border_width(dialog, 3, 0);
            lv_obj_set_style_border_color(dialog, UIAlerts::UI_SUCCESS, 0);
            lv_obj_set_style_pad_all(dialog, 20, 0);
            lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
            
            // Title
            lv_obj_t *title = lv_label_create(dialog);
            UILanguage::setText(title, LV_SYMBOL_OK " Export Successful");
            UILanguage::setFont(title, &lv_font_montserrat_22, 0);
            lv_obj_set_style_text_color(title, UIAlerts::UI_SUCCESS, 0);
            lv_obj_set_pos(title, 0, 0);
            
            // Success message with file location
            lv_obj_t *success_msg = lv_label_create(dialog);
            UILanguage::setText(success_msg, "Settings have been exported to:\n/fluidtouch_settings.json\n\nFile location: Display SD card root");
            UILanguage::setFont(success_msg, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(success_msg, UIAlerts::TEXT_LIGHT, 0);
            lv_obj_set_style_text_align(success_msg, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_pos(success_msg, 0, 40);
            lv_obj_set_width(success_msg, 610);
            
            // Warning about WiFi passwords
            lv_obj_t *warning = lv_label_create(dialog);
            UILanguage::setText(warning, LV_SYMBOL_WARNING " WiFi passwords are NOT included\nin the backup file for security reasons.");
            UILanguage::setFont(warning, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(warning, UIAlerts::UI_WARNING, 0);
            lv_obj_set_style_text_align(warning, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_pos(warning, 0, 145);
            lv_obj_set_width(warning, 610);
            
            // OK button
            lv_obj_t *btn_ok = UIAlerts::button(dialog);
            lv_obj_set_size(btn_ok, 200, 50);
            lv_obj_set_pos(btn_ok, 225, 210);
            UIAlerts::background(btn_ok, UIAlerts::BTN_PLAY, 0);
            lv_obj_t *lbl_ok = lv_label_create(btn_ok);
            UILanguage::setText(lbl_ok, "OK");
            UILanguage::setFont(lbl_ok, &lv_font_montserrat_18, 0);
            lv_obj_center(lbl_ok);
            lv_obj_add_event_cb(btn_ok, [](lv_event_t *e) {
                if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                    lv_obj_del((lv_obj_t*)lv_event_get_user_data(e));
                }
            }, LV_EVENT_CLICKED, backdrop);
            
            if (status_label != NULL) {
                UILanguage::setText(status_label, "");
            }
        } else {
            Serial.println("[SettingsBackup] Export failed");
            if (status_label != NULL) {
                UILanguage::setText(status_label, "Export failed! Check SD card.");
                lv_obj_set_style_text_color(status_label, UIPageStyle::STATE_ALARM, 0);
            }
        }
    }
}

// Clear settings button event handler
static void btn_clear_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[SettingsBackup] Clear All button clicked - showing confirmation");
        
        // Create modal backdrop
        lv_obj_t *backdrop = lv_obj_create(lv_layer_top());
        lv_obj_set_size(backdrop, SCREEN_WIDTH, SCREEN_HEIGHT);
        UIAlerts::background(backdrop, lv_color_hex(0x103f38), 0);
    lv_obj_set_style_pad_all(backdrop, 0, 0);
    lv_obj_set_style_radius(backdrop, 0, 0);
    lv_obj_set_pos(backdrop, 0, 0);
        lv_obj_set_style_bg_opa(backdrop, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(backdrop, 0, 0);
        lv_obj_clear_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_center(backdrop);
        
        // Show confirmation dialog
        lv_obj_t *dialog = lv_obj_create(backdrop);
        lv_obj_set_size(dialog, 600, 350);
        lv_obj_center(dialog);
        UIAlerts::background(dialog, UIAlerts::BG_MEDIUM, 0);
        lv_obj_set_style_border_width(dialog, 3, 0);
        lv_obj_set_style_border_color(dialog, UIAlerts::STATE_ALARM, 0);
        lv_obj_set_style_pad_all(dialog, 20, 0);
        lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
        
        // Title
        lv_obj_t *title = lv_label_create(dialog);
        UILanguage::setText(title, LV_SYMBOL_WARNING " Clear All Settings?");
        UILanguage::setFont(title, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(title, UIAlerts::STATE_ALARM, 0);
        lv_obj_set_pos(title, 0, 0);
        
        // Warning message
        lv_obj_t *warning_msg = lv_label_create(dialog);
        UILanguage::setText(warning_msg, "This will permanently delete:");
        UILanguage::setFont(warning_msg, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(warning_msg, UIAlerts::TEXT_LIGHT, 0);
        lv_obj_set_pos(warning_msg, 0, 40);
        
        // List of items to be deleted
        lv_obj_t *delete_list = lv_label_create(dialog);
        UILanguage::setText(delete_list, 
            "• All machine configurations\n"
            "• WiFi credentials\n"
            "• Jog and probe settings\n"
            "• All macros\n"
            "• Power management settings\n"
            "• UI preferences");
        UILanguage::setFont(delete_list, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(delete_list, UIAlerts::TEXT_LIGHT, 0);
        lv_obj_set_pos(delete_list, 20, 75);
        
        // Continue message
        lv_obj_t *continue_msg = lv_label_create(dialog);
        UILanguage::setText(continue_msg, "The device will restart after clearing.");
        UILanguage::setFont(continue_msg, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(continue_msg, UIAlerts::UI_WARNING, 0);
        lv_obj_set_pos(continue_msg, 0, 210);
        
        // Button container for horizontal layout
        lv_obj_t *btn_container = lv_obj_create(dialog);
        lv_obj_set_size(btn_container, 560, 60);
        lv_obj_set_pos(btn_container, 0, 250);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);
        lv_obj_set_style_pad_all(btn_container, 0, 0);
        lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // Clear & Restart button (left)
        lv_obj_t *btn_clear_confirm = UIAlerts::button(btn_container);
        lv_obj_set_size(btn_clear_confirm, 240, 50);
        UIAlerts::background(btn_clear_confirm, UIAlerts::STATE_ALARM, 0);
        lv_obj_t *lbl_clear_confirm = lv_label_create(btn_clear_confirm);
        UILanguage::setText(lbl_clear_confirm, LV_SYMBOL_TRASH " Clear & Restart");
        UILanguage::setFont(lbl_clear_confirm, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl_clear_confirm);
        lv_obj_add_event_cb(btn_clear_confirm, [](lv_event_t *e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                Serial.println("[SettingsBackup] Clear confirmed - clearing all settings");
                
                // Show clearing message
                lv_obj_t *backdrop = (lv_obj_t*)lv_event_get_user_data(e);
                lv_obj_clean(backdrop);
                
                lv_obj_t *clearing_label = lv_label_create(backdrop);
                UILanguage::setText(clearing_label, "Clearing settings...");
                UILanguage::setFont(clearing_label, &lv_font_montserrat_32, 0);
                lv_obj_set_style_text_color(clearing_label, lv_color_hex(0xf5eee4), 0);
                lv_obj_set_style_text_align(clearing_label, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_center(clearing_label);
                
                // Force UI update
                lv_task_handler();
                
                // Clear all settings
                SettingsManager::clearAllSettings();
                
                delay(1000);
                
                // Show restart message
                UILanguage::setText(clearing_label, "Restarting...");
                lv_task_handler();
                delay(1000);
                
                // Restart ESP32
                ESP.restart();
            }
        }, LV_EVENT_CLICKED, backdrop);
        
        // Cancel button (right)
        lv_obj_t *btn_cancel = UIAlerts::button(btn_container);
        lv_obj_set_size(btn_cancel, 240, 50);
        UIAlerts::background(btn_cancel, UIAlerts::BG_BUTTON, 0);
        lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
        UILanguage::setText(lbl_cancel, "Cancel");
        UILanguage::setFont(lbl_cancel, &lv_font_montserrat_18, 0);
        lv_obj_center(lbl_cancel);
        lv_obj_add_event_cb(btn_cancel, [](lv_event_t *e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                lv_obj_del((lv_obj_t*)lv_event_get_user_data(e));
            }
        }, LV_EVENT_CLICKED, backdrop);
    }
}
