#include "ui/ui_language.h"
#include "ui/tabs/ui_tab_settings.h"
#include "ui/tabs/settings/ui_tab_settings_general.h"
#include "ui/tabs/settings/ui_tab_settings_backup.h"
#include "ui/tabs/settings/ui_tab_settings_power.h"
#include "ui/tabs/settings/ui_tab_settings_jog.h"
#include "ui/tabs/settings/ui_tab_settings_probe.h"
#include "ui/tabs/settings/ui_tab_settings_wcs.h"
#include "ui/tabs/settings/ui_tab_settings_about.h"
#include "ui/ui_page_style.h"
#include "config.h"

void UITabSettings::create(lv_obj_t *tab) {
    // Remove padding from the tab itself
    lv_obj_set_style_pad_all(tab, 0, 0);
    
    // Create main content container
    lv_obj_t *content = lv_obj_create(tab);
    lv_obj_set_size(content, lv_pct(100), lv_pct(100));
    UIPageStyle::background(content, UIPageStyle::BG_MEDIUM, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(content, 0, 0);

    // Create vertical tabview on the left side
    lv_obj_t *sub_tabview = lv_tabview_create(content);
    lv_obj_set_size(sub_tabview, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(sub_tabview, 0, 0);
    lv_tabview_set_tab_bar_position(sub_tabview, LV_DIR_LEFT);
    lv_tabview_set_tab_bar_size(sub_tabview, 120);
    
    // Disable swiping between tabs by disabling scrolling on the content
    lv_obj_t *sub_content = lv_tabview_get_content(sub_tabview);
    lv_obj_clear_flag(sub_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(sub_content, 0, 0);
    
    // Style the sub-tabview
    UIPageStyle::background(sub_tabview, UIPageStyle::BG_MEDIUM, 0);
    lv_obj_set_style_pad_all(sub_tabview, 0, 0);  // Remove padding from tabview
    
    // Get tab bar and remove all padding/spacing
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(sub_tabview);
    lv_obj_set_style_pad_all(tab_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tab_bar, 0, LV_PART_ITEMS);
    lv_obj_set_style_pad_row(tab_bar, 0, 0);  // Remove vertical spacing between buttons
    lv_obj_set_style_pad_column(tab_bar, 0, 0);  // Remove horizontal spacing
    UILanguage::setFont(tab_bar, &lv_font_montserrat_18, 0);  // Font for subtabs
    UIPageStyle::background(tab_bar, UIPageStyle::BG_DARKER, 0);  // Darker background for tab bar

    // Also ensure the content area has no padding
    lv_obj_set_style_pad_all(lv_tabview_get_content(sub_tabview), 0, 0);
    
    // Style tab buttons (inactive) - different colors than main tabs
    UIPageStyle::background(sub_tabview, UIPageStyle::BG_MEDIUM, LV_PART_ITEMS);
    lv_obj_set_style_text_color(sub_tabview, UIPageStyle::TEXT_MEDIUM, LV_PART_ITEMS);
    lv_obj_set_style_border_width(sub_tabview, 0, LV_PART_ITEMS);
    UILanguage::setFont(sub_tabview, &lv_font_montserrat_18, LV_PART_ITEMS);
    
    // Style tab buttons (active/checked) - use a different accent color
    UIPageStyle::background(sub_tabview, UIPageStyle::ACCENT_SECONDARY, (lv_state_t)(LV_PART_ITEMS | LV_STATE_CHECKED));
    lv_obj_set_style_text_color(sub_tabview, lv_color_white(), (lv_state_t)(LV_PART_ITEMS | LV_STATE_CHECKED));

    // Add sub-tabs
    lv_obj_t *general_tab = UILanguage::addTab(sub_tabview, "General");
    lv_obj_t *backup_tab = UILanguage::addTab(sub_tabview, "Backup");
    lv_obj_t *power_tab = UILanguage::addTab(sub_tabview, "Power");
    lv_obj_t *jog_tab = UILanguage::addTab(sub_tabview, "Jog");
    lv_obj_t *probe_tab = UILanguage::addTab(sub_tabview, "Probe");
    lv_obj_t *wcs_tab = UILanguage::addTab(sub_tabview, "WCS");
    lv_obj_t *about_tab = UILanguage::addTab(sub_tabview, "About");
    
    // Disable scrolling on sub-tabs
    lv_obj_clear_flag(general_tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(backup_tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(power_tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(jog_tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(probe_tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(wcs_tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(about_tab, LV_OBJ_FLAG_SCROLLABLE);
    
    // Set background for sub-tabs
    UIPageStyle::background(general_tab, UIPageStyle::BG_MEDIUM, 0);
    UIPageStyle::background(backup_tab, UIPageStyle::BG_MEDIUM, 0);
    UIPageStyle::background(power_tab, UIPageStyle::BG_MEDIUM, 0);
    UIPageStyle::background(jog_tab, UIPageStyle::BG_MEDIUM, 0);
    UIPageStyle::background(probe_tab, UIPageStyle::BG_MEDIUM, 0);
    UIPageStyle::background(wcs_tab, UIPageStyle::BG_MEDIUM, 0);
    UIPageStyle::background(about_tab, UIPageStyle::BG_MEDIUM, 0);
    
    // Add 5px padding to all sub-tabs
    lv_obj_set_style_pad_all(general_tab, 5, 0);
    lv_obj_set_style_pad_all(backup_tab, 5, 0);
    lv_obj_set_style_pad_all(power_tab, 5, 0);
    lv_obj_set_style_pad_all(jog_tab, 5, 0);
    lv_obj_set_style_pad_all(probe_tab, 5, 0);
    lv_obj_set_style_pad_all(wcs_tab, 5, 0);
    lv_obj_set_style_pad_all(about_tab, 5, 0);

    UIPageStyle::subTabs(sub_tabview);

    
    // Add tab change event handler to close keyboards when switching tabs
    lv_obj_add_event_cb(sub_tabview, [](lv_event_t *e) {
        // Close all keyboards when tab changes
        UITabSettingsJog::hideKeyboard();
        UITabSettingsProbe::hideKeyboard();
        UITabSettingsWCS::hideKeyboard();
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    
    // Delegate creation to subtab modules
    createGeneralTab(general_tab);
    createBackupTab(backup_tab);
    createPowerTab(power_tab);
    createJogTab(jog_tab);
    createProbeTab(probe_tab);
    createWCSTab(wcs_tab);
    createAboutTab(about_tab);
}

void UITabSettings::createGeneralTab(lv_obj_t *tab) {
    UITabSettingsGeneral::create(tab);
}

void UITabSettings::createBackupTab(lv_obj_t *tab) {
    UITabSettingsBackup::create(tab);
}

void UITabSettings::createPowerTab(lv_obj_t *tab) {
    UITabSettingsPower::create(tab);
}

void UITabSettings::createJogTab(lv_obj_t *tab) {
    UITabSettingsJog::create(tab);
}

void UITabSettings::createProbeTab(lv_obj_t *tab) {
    UITabSettingsProbe::create(tab);
}

void UITabSettings::createWCSTab(lv_obj_t *tab) {
    UITabSettingsWCS::create(tab);
}

void UITabSettings::createAboutTab(lv_obj_t *tab) {
    UITabSettingsAbout::create(tab);
}
