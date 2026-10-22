#include "ui/ui_language.h"
#include "ui/ui_tabs.h"
#include "ui/ui_theme.h"
#include "ui/ui_alerts.h"
#include "ui/tabs/ui_tab_status.h"
#include "ui/tabs/ui_tab_control.h"
#include "ui/tabs/ui_tab_files.h"
#include "ui/tabs/ui_tab_macros.h"
#include "ui/tabs/ui_tab_terminal.h"
#include "ui/tabs/ui_tab_settings.h"
#include "network/fluidnc_client.h"

namespace {
lv_obj_t *navigation[6]={};
void showSelected(uint32_t selected){
    for(unsigned i=0;i<6;i++)if(navigation[i]){
        if(i==selected)lv_obj_add_state(navigation[i],LV_STATE_CHECKED);
        else lv_obj_remove_state(navigation[i],LV_STATE_CHECKED);
    }
}
}
// Static member initialization
lv_obj_t *UITabs::tabview = nullptr;
lv_obj_t *UITabs::tab_status = nullptr;
lv_obj_t *UITabs::tab_control = nullptr;
lv_obj_t *UITabs::tab_files = nullptr;
lv_obj_t *UITabs::tab_macros = nullptr;
lv_obj_t *UITabs::tab_terminal = nullptr;
lv_obj_t *UITabs::tab_settings = nullptr;

// Create main tabview and all tabs
void UITabs::createTabs() {
    // Create tabview
    tabview = lv_tabview_create(lv_screen_active());
    lv_obj_set_size(tabview, SCREEN_WIDTH, 360);
    lv_obj_align(tabview, LV_ALIGN_TOP_LEFT, 0, 36);  // Align to top-left, not bottom
    
    // Get the tab bar and set height
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);
    lv_tabview_set_tab_bar_size(tabview, 0);
    lv_obj_add_flag(tab_bar, LV_OBJ_FLAG_HIDDEN);
    
    // Remove padding from tabview content area
    lv_obj_set_style_pad_all(lv_tabview_get_content(tabview), 0, 0);
    
    // Disable scrolling on tab buttons and content
    lv_obj_clear_flag(tab_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);
    
    // Style tab buttons with larger font
    UILanguage::setFont(tab_bar, &lv_font_montserrat_20, 0);  // Direct to tab bar
    UILanguage::setFont(tabview, &lv_font_montserrat_20, LV_PART_ITEMS);
    UIAlerts::background(tabview, UITheme::BG_BUTTON, LV_PART_ITEMS);
    lv_obj_set_style_text_color(tabview, UITheme::TEXT_LIGHT, LV_PART_ITEMS);
    UIAlerts::background(tabview, UITheme::ACCENT_PRIMARY, (lv_state_t)(LV_PART_ITEMS | LV_STATE_CHECKED));
    lv_obj_set_style_text_color(tabview, lv_color_white(), (lv_state_t)(LV_PART_ITEMS | LV_STATE_CHECKED));
    
    // Add tabs
    tab_status = UILanguage::addTab(tabview, "Status");
    tab_control = UILanguage::addTab(tabview, "Control");
    tab_files = UILanguage::addTab(tabview, "Files");
    tab_macros = UILanguage::addTab(tabview, "Macros");
    tab_terminal = UILanguage::addTab(tabview, "Terminal");
    tab_settings = UILanguage::addTab(tabview, "Settings");
    
    // Disable scrolling only on tabs that don't need it
    lv_obj_clear_flag(tab_status, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tab_control, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tab_macros, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tab_files, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tab_terminal, LV_OBJ_FLAG_SCROLLABLE);
    // Settings tab may need scrolling, so leave it enabled
    
    // Create tab content
    createStatusTab(tab_status);
    createControlTab(tab_control);
    createFilesTab(tab_files);
    createMacrosTab(tab_macros);
    createTerminalTab(tab_terminal);
    createSettingsTab(tab_settings);
    
    // Independent footer preserves the six original tab indices and shortcuts.
    auto *footer=lv_obj_create(lv_screen_active());
    lv_obj_set_pos(footer,0,420);lv_obj_set_size(footer,800,60);
    lv_obj_set_style_pad_all(footer,4,0);lv_obj_set_style_border_width(footer,0,0);
    lv_obj_set_style_radius(footer,0,0);UIAlerts::background(footer,lv_color_hex(0x103f38),0);
    lv_obj_clear_flag(footer,LV_OBJ_FLAG_SCROLLABLE);
    const char *names[]={LV_SYMBOL_LIST "\nStatus",LV_SYMBOL_SETTINGS "\nControl",LV_SYMBOL_FILE "\nFiles",
        LV_SYMBOL_LOOP "\nMacros",LV_SYMBOL_KEYBOARD "\nTerminal",LV_SYMBOL_SETTINGS "\nSettings"};
    for(unsigned i=0;i<6;i++){
        auto *btn=lv_button_create(footer);navigation[i]=btn;
        lv_obj_set_pos(btn,i*132,0);lv_obj_set_size(btn,128,52);
        lv_obj_set_style_pad_all(btn,0,0);lv_obj_set_style_border_width(btn,0,0);
        UIAlerts::background(btn,lv_color_hex(0x103f38),0);
        lv_obj_set_style_text_color(btn,lv_color_hex(0xf5eee4),0);
        UIAlerts::background(btn,lv_color_hex(0xf5eee4),LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn,lv_color_hex(0x103f38),LV_STATE_CHECKED);
        auto *l=lv_label_create(btn);UILanguage::setText(l,names[i]);
        UILanguage::setFont(l,&lv_font_montserrat_14,0);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(l);
        lv_obj_add_event_cb(btn,[](lv_event_t *e){
            unsigned index=(unsigned)(uintptr_t)lv_event_get_user_data(e);
            lv_tabview_set_active(tabview,index,LV_ANIM_OFF);
            lv_obj_send_event(tabview,LV_EVENT_VALUE_CHANGED,nullptr);
        },LV_EVENT_CLICKED,(void*)(uintptr_t)i);
    }
    lv_obj_add_event_cb(tabview,tab_changed_event_cb,LV_EVENT_VALUE_CHANGED,nullptr);
    lv_tabview_set_active(tabview,0,LV_ANIM_OFF);showSelected(0);

}

// Tab change event handler
void UITabs::tab_changed_event_cb(lv_event_t *e) {
    lv_obj_t *tabview = (lv_obj_t*)lv_event_get_target(e);
    uint32_t active_tab = lv_tabview_get_tab_active(tabview);
    
    showSelected(active_tab);
    if(active_tab!=0)UITabStatus::hideKeyboard();

    // Tab indices: 0=Status, 1=Control, 2=Files, 3=Macros, 4=Terminal, 5=Settings
    if (active_tab == 2) {  // Files tab
        // Trigger initial load on first selection
        UITabFiles::refreshFileList();
    }
}

// Create Status tab content (delegated to UITabStatus module)
void UITabs::createStatusTab(lv_obj_t *tab) {
    UITabStatus::create(tab);
}

// Create Control tab content (delegated to UITabControl module)
void UITabs::createControlTab(lv_obj_t *tab) {
    UITabControl::create(tab);
}

// Create Files tab content (delegated to UITabFiles module)
void UITabs::createFilesTab(lv_obj_t *tab) {
    UITabFiles::create(tab);
}

// Create Macros tab content (delegated to UITabMacros module)
void UITabs::createMacrosTab(lv_obj_t *tab) {
    UITabMacros::create(tab);
}

// Create Terminal tab content (delegated to UITabTerminal module)
void UITabs::createTerminalTab(lv_obj_t *tab) {
    UITabTerminal::create(tab);

    // Register terminal callback to receive FluidNC messages (excluding status reports)
    FluidNCClient::setTerminalCallback([](const char* message) {
        UITabTerminal::appendMessage(message);
    });
}

// Create Settings tab content (delegated to UITabSettings module)
void UITabs::createSettingsTab(lv_obj_t *tab) {
    UITabSettings::create(tab);
}

// Public wrappers for settings functions
void UITabs::loadSettings() {
    // Settings are now loaded by the individual subtabs (General and Connection)
}

void UITabs::saveSettings() {
    // Settings are now saved by the individual subtabs (General and Connection)
}
