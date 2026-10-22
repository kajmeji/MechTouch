#include "ui/ui_alerts.h"
#include "ui/ui_common.h"
extern "C" { LV_FONT_DECLARE(ft_bold_28); LV_FONT_DECLARE(ft_bold_20); }
#include "ui/ui_power_dialog.h"
#include "ui/ui_connection_dialog.h"
#include "ui/ui_tabs.h"
#include "ui/ui_theme.h"
#include "ui/machine_config.h"
#include "ui/ui_machine_select.h"
#include "network/fluidnc_client.h"
#include "core/display_driver.h"
#include "core/power_manager.h"
#include "network/screenshot_server.h"
#include "config.h"
#include <Preferences.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_sleep.h>

// Static member initialization
lv_display_t *UICommon::display = nullptr;
DisplayDriver *UICommon::display_driver = nullptr;
lv_obj_t *UICommon::status_bar = nullptr;
lv_obj_t *UICommon::status_bar_left_area = nullptr;
lv_obj_t *UICommon::status_bar_right_area = nullptr;
lv_obj_t *UICommon::machine_select_dialog = nullptr;
lv_obj_t *UICommon::connecting_popup = nullptr;
lv_obj_t *UICommon::connection_error_dialog = nullptr;
lv_obj_t *UICommon::hold_popup = nullptr;
lv_obj_t *UICommon::hold_popup_msg_label = nullptr;
lv_obj_t *UICommon::alarm_popup = nullptr;
lv_obj_t *UICommon::alarm_popup_msg_label = nullptr;
int UICommon::last_popup_state = -1;
bool UICommon::hold_popup_dismissed = false;
bool UICommon::alarm_popup_dismissed = false;
lv_obj_t *UICommon::lbl_modal_states = nullptr;
lv_obj_t *UICommon::lbl_status = nullptr;

// Connection status labels
lv_obj_t *UICommon::lbl_machine_symbol = nullptr;
lv_obj_t *UICommon::lbl_machine_name = nullptr;
lv_obj_t *UICommon::lbl_wifi_symbol = nullptr;
lv_obj_t *UICommon::lbl_wifi_name = nullptr;

// Work Position labels
lv_obj_t *UICommon::lbl_wpos_label = nullptr;
lv_obj_t *UICommon::lbl_wpos_x = nullptr;
lv_obj_t *UICommon::lbl_wpos_y = nullptr;
lv_obj_t *UICommon::lbl_wpos_z = nullptr;
lv_obj_t *UICommon::lbl_wpos_a = nullptr;

// Machine Position labels
lv_obj_t *UICommon::lbl_mpos_label = nullptr;
lv_obj_t *UICommon::lbl_mpos_x = nullptr;
lv_obj_t *UICommon::lbl_mpos_y = nullptr;
lv_obj_t *UICommon::lbl_mpos_z = nullptr;

// Job progress display
lv_obj_t *UICommon::lbl_file_progress_container = nullptr;
lv_obj_t *UICommon::lbl_filename = nullptr;
lv_obj_t *UICommon::bar_progress = nullptr;
lv_obj_t *UICommon::lbl_percent = nullptr;
lv_obj_t *UICommon::lbl_elapsed_time = nullptr;
lv_obj_t *UICommon::lbl_elapsed_unit = nullptr;
lv_obj_t *UICommon::lbl_estimated_time = nullptr;
lv_obj_t *UICommon::lbl_estimated_unit = nullptr;

// Cached values for delta checking
float UICommon::last_wpos_x = -9999.0f;
float UICommon::last_wpos_y = -9999.0f;
float UICommon::last_wpos_z = -9999.0f;
float UICommon::last_wpos_a = -9999.0f;
float UICommon::last_mpos_x = -9999.0f;
float UICommon::last_mpos_y = -9999.0f;
float UICommon::last_mpos_z = -9999.0f;

// Cached system preferences (loaded once at startup)
bool UICommon::enable_a_axis = false;

static char last_machine_state[16] = "";  // Cached state to avoid unnecessary updates
static bool last_machine_connected = false;  // Cached connection status
static bool last_wifi_connected = false;     // Cached WiFi status
static bool last_auto_reporting = false;     // Cached auto-reporting status

// File progress cached values for delta checking
static char last_filename[128] = "";
static float last_percent = -1.0f;
static uint32_t last_elapsed_sec = 0xFFFFFFFF;  // Use seconds for comparison
static uint32_t last_estimated_sec = 0xFFFFFFFF;

// Connection timeout tracking
static uint32_t connection_timeout_start = 0;
static bool connection_timeout_active = false;
static bool connection_error_shown = false;
static bool ever_connected_successfully = false;  // Track if we've connected at least once

// Event handler for status bar left area click (go to Status tab)
static void status_bar_left_click_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        // Switch to Status tab (index 0) without animation
        lv_obj_t *tabview = UITabs::getTabview();
        if (tabview) {
            lv_tabview_set_active(tabview, 0, LV_ANIM_OFF);
            lv_obj_send_event(tabview, LV_EVENT_VALUE_CHANGED, nullptr);
        }
    }
}

// Event handler for status bar right area click (machine selection/connection status)
static void status_bar_right_click_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        // Check if both WiFi and WebSocket are connected
        bool wifi_connected = (WiFi.status() == WL_CONNECTED);
        bool ws_connected = FluidNCClient::isConnected();
        
        if (wifi_connected && ws_connected) {
            // Both connected - show restart confirmation dialog
            UICommon::showMachineSelectConfirmDialog();
        } else {
            // One or both disconnected - show connection error dialog
            MachineConfig config;
            if (MachineConfigManager::getSelectedMachine(config)) {
                char error_msg[300];
                if (!wifi_connected && !ws_connected) {
                    snprintf(error_msg, sizeof(error_msg), 
                            "WiFi and machine are disconnected.\n\n%s\n\nClick Connect to reconnect.",
                            config.name);
                } else if (!wifi_connected) {
                    snprintf(error_msg, sizeof(error_msg), 
                            "WiFi is disconnected.\n\n%s\n\nClick Connect to reconnect.",
                            config.name);
                } else {
                    snprintf(error_msg, sizeof(error_msg), 
                            "Machine is disconnected.\n\n%s\nURL: %s:%d\n\nClick Connect to reconnect.",
                            config.name, config.fluidnc_url, config.websocket_port);
                }
                UICommon::showConnectionErrorDialog("Connection Lost", error_msg);
            }
        }
    }
}

// Event handler for confirming machine selection change
static void on_machine_select_confirm(lv_event_t *e) {
    Serial.println("UICommon: Restarting to change machine...");
    UICommon::hideMachineSelectConfirmDialog();
    
    // Show restart message
    lv_obj_t *restart_label = lv_label_create(lv_screen_active());
    lv_label_set_text(restart_label, "Restarting...");
    lv_obj_set_style_text_font(restart_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(restart_label, UITheme::UI_INFO, 0);
    lv_obj_center(restart_label);
    
    // Force display update
    lv_timer_handler();
    delay(500);
    
    // Restart the ESP32
    ESP.restart();
}

// Event handler for power off
static void on_power_off_confirm(lv_event_t *e) {
    Serial.println("UICommon: Powering off device...");
    UICommon::hideMachineSelectConfirmDialog();
    
    // Save clean shutdown flag in separate namespace to avoid corrupting machine configs
    Preferences prefs;
    prefs.begin(PREFS_SYSTEM_NAMESPACE, false);
    prefs.putBool("clean_shutdown", true);
    prefs.end();
    
    UIPowerDialog::showBusy();

    // Force display update
    lv_timer_handler();
    delay(1000);
    
    // Disconnect from FluidNC gracefully
    FluidNCClient::disconnect();
    delay(100);
    
    // Power down display and peripherals (Elecrow-specific)
    DisplayDriver *display_driver = UICommon::getDisplayDriver();
    if (display_driver) {
        display_driver->powerDown();
    }
    delay(100);
    
    // Disconnect WiFi cleanly
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Disable Bluetooth (reduces power even if not used)
    btStop();
    
    // Disable all wakeup sources except reset button
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    
    // Disable RTC peripherals to save power
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    // Note: ESP_PD_DOMAIN_RTC_SLOW_MEM and RTC_FAST_MEM removed in IDF 5.3+
    // RTC memory domains are now controlled automatically
    
    // Disable unused GPIO power domains
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    
    // Isolate GPIO pins to prevent current leakage
    gpio_deep_sleep_hold_en();
    
    // Enter deep sleep (only reset button can wake)
    Serial.println("Entering deep sleep with maximum power savings...");
    Serial.flush();
    delay(100);
    
    esp_deep_sleep_start();
}

// Event handler for canceling machine selection change
static void on_machine_select_cancel(lv_event_t *e) {
    Serial.println("UICommon: Machine selection cancelled");
    UICommon::hideMachineSelectConfirmDialog();
}

void UICommon::init(lv_display_t *disp) {
    display = disp;
}

void UICommon::setDisplayDriver(DisplayDriver* driver) {
    display_driver = driver;
}

void UICommon::createMainUI() {
    Serial.println("UICommon: Creating main UI");

    // Load machine-specific preferences now that the machine has been selected
    loadSystemPreferences();
    
    // Get selected machine configuration
    MachineConfig config;
    if (!MachineConfigManager::getSelectedMachine(config)) {
        Serial.println("UICommon: ERROR - No machine selected!");
        return;
    }
    
    // Create main screen first
    lv_obj_t *main_screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(main_screen, UITheme::BG_DARKER, LV_PART_MAIN);
    
    // Load the new screen immediately to show user something is happening
    lv_scr_load(main_screen);
    
    // Show connecting popup BEFORE creating UI (faster response)
    // This gives immediate visual feedback while UI is being built
    if (config.connection_type == CONN_WIRELESS && strlen(config.ssid) > 0) {
        showConnectingPopup(config.name, config.ssid);
        lv_refr_now(nullptr);  // Force immediate display update
    } else if (config.connection_type == CONN_WIRED) {
        showConnectingPopup(config.name, nullptr);
        lv_refr_now(nullptr);  // Force immediate display update
    }
    
    // Create status bar
    createStatusBar();
    
    // Create all tabs
    UITabs::createTabs();
    
    // Initialize WiFi connection using machine-specific credentials
    if (config.connection_type == CONN_WIRELESS) {
        if (strlen(config.ssid) > 0) {
            Serial.println("\n=== WiFi Connection ===");
            Serial.printf("Connecting to WiFi: %s\n", config.ssid);
            
            WiFi.mode(WIFI_STA);
            WiFi.setAutoReconnect(false);  // Disable auto-reconnect - user must manually reconnect
            WiFi.begin(config.ssid, config.password);
            
            // Wait for connection with timeout (10 seconds)
            int timeout = 20;
            while (WiFi.status() != WL_CONNECTED && timeout > 0) {
                delay(500);
                Serial.print(".");
                timeout--;
                lv_timer_handler();  // Keep UI responsive
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nWiFi connected!");
                Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
                
                // Initialize mDNS client stack to enable resolving .local hostnames (like fluidnc.local)
                // Note: MDNS.begin() is required on ESP32 to enable mDNS client queries, not just advertising
                if (MDNS.begin("mechtouch")) {
                    Serial.println("mDNS client initialized - can now resolve .local hostnames");
                    delay(1000);  // Give mDNS time to fully initialize before attempting queries
                } else {
                    Serial.println("Warning: mDNS client failed to start (.local hostname resolution will not work)");
                }
                
                // Initialize screenshot server now that WiFi is connected
                if (display_driver) {
                    Serial.println("Initializing screenshot server with WiFi connection...");
                    ScreenshotServer::init(display_driver);
                    if (ScreenshotServer::isConnected()) {
                        Serial.println("Screenshot server available at: http://" + ScreenshotServer::getIPAddress());
                    }
                }
                
                // Update WiFi status in status bar
                if (lbl_wifi_name) {
                    lv_label_set_text(lbl_wifi_name, config.ssid);
                }
                if (lbl_wifi_symbol) {
                    lv_obj_set_style_text_color(lbl_wifi_symbol, UITheme::STATE_IDLE, 0);
                }
                
                // Update popup to show machine connection (WiFi is now connected)
                hideConnectingPopup();
                showConnectingPopup(config.name, nullptr);  // nullptr = no WiFi name (already connected)
            } else {
                Serial.println("\nWiFi connection failed!");
                
                // Build error message
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), 
                        "Could not connect to network:\n%s\n\nCheck WiFi settings and try again.",
                        config.ssid);
                
                // Show error dialog
                showConnectionErrorDialog("WiFi Connection Failed", error_msg);
                
                // Disable connection timeout monitoring since we're not attempting machine connection
                connection_timeout_active = false;
                
                // Do NOT attempt machine connection if WiFi failed
                return;
            }
        } else {
            Serial.println("UICommon: Warning - Wireless connection selected but no SSID configured");
            // Popup already shown at start
            // Disable connection timeout monitoring since we're not attempting machine connection
            connection_timeout_active = false;
            return;  // Don't attempt machine connection without WiFi config
        }
    } else {
        Serial.println("UICommon: Wired connection selected, skipping WiFi initialization");
        // Popup already shown at start
    }
    
    // Connect to FluidNC using selected machine
    // Only reached if WiFi connected successfully (or wired connection)
    Serial.printf("UICommon: Connecting to FluidNC at %s:%d\n", 
                 config.fluidnc_url, config.websocket_port);
    FluidNCClient::connect(config);
    
    // Start connection timeout monitoring (10 seconds)
    connection_timeout_start = millis();
    connection_timeout_active = true;
    connection_error_shown = false;
    
    Serial.println("UICommon: Main UI created");
}

void UICommon::createStatusBar() {
    // Header and context strip occupy the same 60px budget as the former footer.
    status_bar=lv_obj_create(lv_screen_active());lv_obj_set_size(status_bar,800,420);
    lv_obj_set_style_bg_opa(status_bar,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(status_bar,0,0);
    lv_obj_set_style_pad_all(status_bar,0,0);lv_obj_clear_flag(status_bar,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(status_bar,LV_OBJ_FLAG_CLICKABLE);
    auto box=[&](int x,int y,int w,int h,uint32_t color){
        auto *o=lv_obj_create(status_bar);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
        UIAlerts::background(o,lv_color_hex(color),0);lv_obj_set_style_border_width(o,0,0);
        lv_obj_set_style_pad_all(o,0,0);lv_obj_set_style_radius(o,0,0);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
    };
    auto label=[](lv_obj_t *p,const char *text,int x,int y,int w,const lv_font_t *font){
        auto *l=lv_label_create(p);lv_label_set_text(l,text);lv_obj_set_pos(l,x,y);lv_obj_set_width(l,w);
        lv_obj_set_style_text_font(l,font,0);lv_label_set_long_mode(l,LV_LABEL_LONG_DOT);return l;
    };
    auto *head=box(0,0,800,36,0x103f38);lv_obj_set_style_text_color(head,lv_color_hex(0xf5eee4),0);
    label(head,"MechTouch",16,2,225,&ft_bold_28);
    status_bar_right_area=box(260,0,540,36,0x103f38);
    lv_obj_set_style_text_color(status_bar_right_area,lv_color_hex(0xf5eee4),0);
    lv_obj_add_event_cb(status_bar_right_area,status_bar_right_click_handler,LV_EVENT_CLICKED,nullptr);
    MachineConfig config;bool has=MachineConfigManager::getSelectedMachine(config);
    lbl_machine_symbol=label(status_bar_right_area,LV_SYMBOL_WIFI,4,10,25,&lv_font_montserrat_16);
    lbl_machine_name=label(status_bar_right_area,has?config.name:"No Machine",34,7,258,&ft_bold_20);
    lbl_wifi_name=label(status_bar_right_area,has?config.ssid:"Not Connected",312,10,190,&lv_font_montserrat_14);
    lv_obj_set_style_text_align(lbl_wifi_name,LV_TEXT_ALIGN_RIGHT,0);
    lbl_wifi_symbol=label(status_bar_right_area,LV_SYMBOL_WIFI,510,10,25,&lv_font_montserrat_16);
    lbl_modal_states=nullptr;
    status_bar_left_area=box(0,396,800,24,0xede9df);
    lv_obj_add_event_cb(status_bar_left_area,status_bar_left_click_handler,LV_EVENT_CLICKED,nullptr);
    lbl_status=label(status_bar_left_area,"OFFLINE",8,4,88,&lv_font_montserrat_14);
    lbl_wpos_label=label(status_bar_left_area,"W",100,4,20,&lv_font_montserrat_14);
    lbl_wpos_x=label(status_bar_left_area,"X ----.---",122,4,100,&lv_font_montserrat_14);
    lbl_wpos_y=label(status_bar_left_area,"Y ----.---",226,4,100,&lv_font_montserrat_14);
    lbl_wpos_z=label(status_bar_left_area,"Z ----.---",330,4,100,&lv_font_montserrat_14);
    lbl_wpos_a=nullptr;lbl_mpos_label=lbl_mpos_x=lbl_mpos_y=lbl_mpos_z=nullptr;
    if(isAAxisEnabled())lbl_wpos_a=label(status_bar_left_area,"A ----.---",434,4,130,&lv_font_montserrat_14);
    else{
        lbl_mpos_label=label(status_bar_left_area,"M",450,4,20,&lv_font_montserrat_14);
        lbl_mpos_x=label(status_bar_left_area,"X ----.---",472,4,100,&lv_font_montserrat_14);
        lbl_mpos_y=label(status_bar_left_area,"Y ----.---",576,4,100,&lv_font_montserrat_14);
        lbl_mpos_z=label(status_bar_left_area,"Z ----.---",680,4,112,&lv_font_montserrat_14);
    }
    lbl_file_progress_container=box(0,396,800,24,0xede9df);
    lv_obj_add_flag(lbl_file_progress_container,LV_OBJ_FLAG_HIDDEN);
    lbl_filename=label(lbl_file_progress_container,"",8,4,176,&lv_font_montserrat_14);
    bar_progress=lv_bar_create(lbl_file_progress_container);
    lv_obj_set_style_bg_color(bar_progress,lv_color_hex(0xa3b0a7),LV_PART_MAIN);lv_obj_set_style_bg_color(bar_progress,lv_color_hex(0x103f38),LV_PART_INDICATOR);lv_obj_set_pos(bar_progress,192,8);lv_obj_set_size(bar_progress,100,8);
    lbl_percent=label(lbl_file_progress_container,"0%",300,4,55,&lv_font_montserrat_14);
    label(lbl_file_progress_container,"Elapsed",360,4,58,&lv_font_montserrat_14);
    lbl_elapsed_time=label(lbl_file_progress_container,"0:00",422,4,46,&lv_font_montserrat_14);
    lbl_elapsed_unit=label(lbl_file_progress_container,"min:sec",470,4,65,&lv_font_montserrat_14);
    label(lbl_file_progress_container,"Estimated",550,4,78,&lv_font_montserrat_14);
    lbl_estimated_time=label(lbl_file_progress_container,"0:00",632,4,46,&lv_font_montserrat_14);
    lbl_estimated_unit=label(lbl_file_progress_container,"min:sec",682,4,65,&lv_font_montserrat_14);
    // Fresh widgets must receive first live values even after rebuilding the UI.
    last_wpos_x=last_wpos_y=last_wpos_z=last_wpos_a=-9999.0f;
    last_mpos_x=last_mpos_y=last_mpos_z=-9999.0f;
    last_machine_state[0]='\0';
}

void UICommon::updateModalStates(const char *text) {
    // Check if status bar exists and label is valid
    if (status_bar && lbl_modal_states) {
        lv_label_set_text(lbl_modal_states, text);
    }
}

namespace {
void setCoordinateLabelIfChanged(lv_obj_t *obj, const char *text) {
    if (strcmp(lv_label_get_text(obj), text) != 0) {
        lv_label_set_text(obj, text);
    }
}
}

void UICommon::updateMachinePosition(float x, float y, float z) {
    // Don't update machine position when A-axis is enabled (MPos is hidden)
    if (isAAxisEnabled()) {
        return;
    }

    // Only update if values changed (avoid unnecessary redraws)
    if (x == last_mpos_x && y == last_mpos_y && z == last_mpos_z) {
        return;
    }

    char buf[16];
    
    if (status_bar && lbl_mpos_x && x != last_mpos_x) {
        if (x <= -9999.0f) {
            setCoordinateLabelIfChanged(lbl_mpos_x, "X ----.---");
        } else {
            snprintf(buf, sizeof(buf), "X %04.3f", x);
            setCoordinateLabelIfChanged(lbl_mpos_x, buf);
        }
        last_mpos_x = x;
    }
    if (status_bar && lbl_mpos_y && y != last_mpos_y) {
        if (y <= -9999.0f) {
            setCoordinateLabelIfChanged(lbl_mpos_y, "Y ----.---");
        } else {
            snprintf(buf, sizeof(buf), "Y %04.3f", y);
            setCoordinateLabelIfChanged(lbl_mpos_y, buf);
        }
        last_mpos_y = y;
    }
    if (status_bar && lbl_mpos_z && z != last_mpos_z) {
        if (z <= -9999.0f) {
            setCoordinateLabelIfChanged(lbl_mpos_z, "Z ----.---");
        } else {
            snprintf(buf, sizeof(buf), "Z %04.3f", z);
            setCoordinateLabelIfChanged(lbl_mpos_z, buf);
        }
        last_mpos_z = z;
    }
}

void UICommon::updateWorkPosition(float x, float y, float z, float a) {
    // Only update if values changed (avoid unnecessary redraws)
    if (x == last_wpos_x && y == last_wpos_y && z == last_wpos_z && a == last_wpos_a) {
        return;
    }

    char buf[16];

    if (status_bar && lbl_wpos_x && x != last_wpos_x) {
        if (x <= -9999.0f) {
            setCoordinateLabelIfChanged(lbl_wpos_x, "X ----.---");
        } else {
            snprintf(buf, sizeof(buf), "X %04.3f", x);
            setCoordinateLabelIfChanged(lbl_wpos_x, buf);
        }
        last_wpos_x = x;
    }
    if (status_bar && lbl_wpos_y && y != last_wpos_y) {
        if (y <= -9999.0f) {
            setCoordinateLabelIfChanged(lbl_wpos_y, "Y ----.---");
        } else {
            snprintf(buf, sizeof(buf), "Y %04.3f", y);
            setCoordinateLabelIfChanged(lbl_wpos_y, buf);
        }
        last_wpos_y = y;
    }
    if (status_bar && lbl_wpos_z && z != last_wpos_z) {
        if (z <= -9999.0f) {
            setCoordinateLabelIfChanged(lbl_wpos_z, "Z ----.---");
        } else {
            snprintf(buf, sizeof(buf), "Z %04.3f", z);
            setCoordinateLabelIfChanged(lbl_wpos_z, buf);
        }
        last_wpos_z = z;
    }
    // Update A-axis if enabled and value changed
    if (status_bar && lbl_wpos_a && a != last_wpos_a &&
        (a > -9999.0f || (x <= -9999.0f && y <= -9999.0f && z <= -9999.0f))) {
        if(a <= -9999.0f)snprintf(buf,sizeof(buf),"A ----.---");
        else snprintf(buf, sizeof(buf), "A %04.3f", a);
        setCoordinateLabelIfChanged(lbl_wpos_a, buf);
        last_wpos_a = a;
    }
}

void UICommon::updateMachineState(const char *state) {
    if (status_bar && lbl_status) {
        // Only update if state actually changed
        if (strcmp(state, last_machine_state) == 0) {
            return;  // No change, skip update
        }
        
        lv_label_set_text(lbl_status, state);
        
        // Update cached state
        strncpy(last_machine_state, state, sizeof(last_machine_state) - 1);
        last_machine_state[sizeof(last_machine_state) - 1] = '\0';
        
        // Color code the status (state is already uppercase)
        if (strcmp(state, "IDLE") == 0) {
            lv_obj_set_style_text_color(lbl_status, UIAlerts::UI_SUCCESS, 0);
        } else if (strcmp(state, "RUN") == 0 || strcmp(state, "JOG") == 0) {
            lv_obj_set_style_text_color(lbl_status, UIAlerts::ACCENT_PRIMARY, 0);
        } else if (strcmp(state, "ALARM") == 0 || strcmp(state, "OFFLINE") == 0) {
            lv_obj_set_style_text_color(lbl_status, UIAlerts::STATE_ALARM, 0);
        } else {
            lv_obj_set_style_text_color(lbl_status, UIAlerts::UI_WARNING, 0);
        }
    }
}

void UICommon::updateConnectionStatus(bool machine_connected, bool wifi_connected) {
    bool auto_reporting = FluidNCClient::isAutoReporting();
    
    // Update machine symbol color with three states:
    // - Red (STATE_ALARM): Disconnected
    // - Orange (UI_WARNING): Connected with fallback polling (degraded)
    // - Green (STATE_IDLE): Connected with auto-reporting (optimal)
    if (lbl_machine_symbol && (machine_connected != last_machine_connected || auto_reporting != last_auto_reporting)) {
        lv_color_t color;
        if (!machine_connected) {
            color = lv_color_hex(0xf3aa8e);  // Red: Disconnected
        } else if (!auto_reporting) {
            color = lv_color_hex(0xe4ca84);   // Orange: Fallback polling (100ms requests)
        } else {
            color = lv_color_hex(0xd6e980);   // Green: Auto-reporting (50ms requested)
        }
        lv_obj_set_style_text_color(lbl_machine_symbol, color, 0);
        last_machine_connected = machine_connected;
        last_auto_reporting = auto_reporting;
    }
    
    // Update WiFi symbol color
    if (lbl_wifi_symbol && wifi_connected != last_wifi_connected) {
        lv_obj_set_style_text_color(lbl_wifi_symbol, 
            wifi_connected ? lv_color_hex(0xd6e980) : lv_color_hex(0xf3aa8e), 0);
        last_wifi_connected = wifi_connected;
    }
}

void UICommon::showMachineSelectConfirmDialog() {
    // Create modal background
    machine_select_dialog = lv_obj_create(lv_layer_top());
    lv_obj_set_size(machine_select_dialog, LV_PCT(100), LV_PCT(100));
    UIAlerts::background(machine_select_dialog, lv_color_hex(0x103f38), 0);
    lv_obj_set_style_pad_all(machine_select_dialog, 0, 0);
    lv_obj_set_style_radius(machine_select_dialog, 0, 0);
    lv_obj_set_pos(machine_select_dialog, 0, 0);
    lv_obj_set_style_bg_opa(machine_select_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(machine_select_dialog, 0, 0);
    lv_obj_clear_flag(machine_select_dialog, LV_OBJ_FLAG_SCROLLABLE);
    
    // Dialog content box (same size as HOLD/ALARM popups)
    lv_obj_t *content = lv_obj_create(machine_select_dialog);
    lv_obj_set_size(content, 600, 300);
    lv_obj_center(content);
    UIAlerts::background(content, UIAlerts::BG_MEDIUM, 0);
    lv_obj_set_style_border_color(content, UIAlerts::ACCENT_PRIMARY, 0);
    lv_obj_set_style_border_width(content, 3, 0);
    lv_obj_set_style_pad_all(content, 20, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title (positioned near top)
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " System Options");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, UIAlerts::ACCENT_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    // Check if power management is enabled
    bool power_mgmt_enabled = PowerManager::isEnabled();
    
    // Message (centered vertically in available space)
    lv_obj_t *msg_label = lv_label_create(content);
    if (power_mgmt_enabled) {
        lv_label_set_text(msg_label, "Restart to change machines,\nor power off for battery operation.\n\nNote: Press reset button to power on\nafter using power off.");
    } else {
        lv_label_set_text(msg_label, "Restart to change machines.");
    }
    lv_obj_set_style_text_font(msg_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(msg_label, UIAlerts::TEXT_LIGHT, 0);
    lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(msg_label, 560);
    lv_obj_align(msg_label, LV_ALIGN_TOP_MID, 0, 50);
    
    // Button container (positioned at bottom)
    lv_obj_t *btn_container = lv_obj_create(content);
    lv_obj_set_size(btn_container, 560, 60);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_pad_all(btn_container, 0, 0);
    lv_obj_set_style_pad_gap(btn_container, 10, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Restart button (adjust size based on number of buttons)
    lv_obj_t *restart_btn = UIAlerts::button(btn_container);
    lv_obj_set_size(restart_btn, power_mgmt_enabled ? 165 : 250, 50);
    UIAlerts::background(restart_btn, UIAlerts::ACCENT_PRIMARY, 0);
    lv_obj_add_event_cb(restart_btn, on_machine_select_confirm, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t *restart_label = lv_label_create(restart_btn);
    lv_label_set_text(restart_label, LV_SYMBOL_REFRESH " Restart");
    lv_obj_set_style_text_font(restart_label, &lv_font_montserrat_18, 0);
    lv_obj_center(restart_label);
    
    // Power off button (only show if power management is enabled)
    if (power_mgmt_enabled) {
        lv_obj_t *poweroff_btn = UIAlerts::button(btn_container);
        lv_obj_set_size(poweroff_btn, 165, 50);
        UIAlerts::background(poweroff_btn, UIAlerts::STATE_ALARM, 0);
        lv_obj_add_event_cb(poweroff_btn, on_power_off_confirm, LV_EVENT_CLICKED, nullptr);
        
        lv_obj_t *poweroff_label = lv_label_create(poweroff_btn);
        lv_label_set_text(poweroff_label, LV_SYMBOL_POWER " Power Off");
        lv_obj_set_style_text_font(poweroff_label, &lv_font_montserrat_18, 0);
        lv_obj_center(poweroff_label);
    }
    
    // Cancel button (adjust size based on number of buttons)
    lv_obj_t *cancel_btn = UIAlerts::button(btn_container);
    lv_obj_set_size(cancel_btn, power_mgmt_enabled ? 165 : 250, 50);
    UIAlerts::background(cancel_btn, UIAlerts::BG_BUTTON, 0);
    lv_obj_add_event_cb(cancel_btn, on_machine_select_cancel, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_set_style_text_font(cancel_label, &lv_font_montserrat_18, 0);
    lv_obj_center(cancel_label);
}

void UICommon::showPowerOffConfirmDialog() {
    UIPowerDialog::show([](lv_event_t *e) {
        UIPowerDialog::hide();
        on_power_off_confirm(e);
    });
}

void UICommon::hideMachineSelectConfirmDialog() {
    if (machine_select_dialog) {
        lv_obj_del(machine_select_dialog);
        machine_select_dialog = nullptr;
    }
}

void UICommon::showConnectingPopup(const char *machine_name, const char *ssid) {
    hideConnectingPopup();
    hideConnectionErrorDialog();
    connecting_popup = UIConnectionDialog::connecting(machine_name, ssid);
}

void UICommon::hideConnectingPopup() {
    if (connecting_popup) {
        lv_obj_del(connecting_popup);
        connecting_popup = nullptr;
        Serial.println("UICommon: Connecting popup hidden");
    }
}

// Event handlers for connection error dialog
static void on_connection_error_close(lv_event_t *e) {
    Serial.println("UICommon: Connection error dialog closed");
    UICommon::hideConnectionErrorDialog();
}

static void on_connection_error_connect(lv_event_t *e) {
    Serial.println("UICommon: Reconnecting WiFi and WebSocket...");
    UICommon::hideConnectionErrorDialog();
    
    // Get machine config
    MachineConfig config;
    if (!MachineConfigManager::getSelectedMachine(config)) {
        Serial.println("UICommon: Error - No machine selected!");
        return;
    }
    
    // Show connecting popup
    UICommon::showConnectingPopup(config.name, config.ssid);
    lv_refr_now(nullptr);  // Force immediate display update
    
    // Disconnect existing connections
    WiFi.disconnect();
    FluidNCClient::disconnect();
    
    // Small delay to ensure clean disconnect
    delay(100);
    lv_timer_handler();
    
    // Reconnect WiFi
    if (config.connection_type == CONN_WIRELESS && strlen(config.ssid) > 0) {
        Serial.printf("UICommon: Reconnecting to WiFi: %s\n", config.ssid);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(false);
        WiFi.begin(config.ssid, config.password);
        
        // Wait for WiFi connection with timeout (10 seconds)
        int timeout = 20;
        while (WiFi.status() != WL_CONNECTED && timeout > 0) {
            delay(500);
            Serial.print(".");
            timeout--;
            lv_timer_handler();
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi reconnected!");
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
            
            // Update status bar WiFi indicator
            UICommon::updateConnectionStatus(false, true);
            
            // Update connecting popup to show machine connection
            UICommon::hideConnectingPopup();
            UICommon::showConnectingPopup(config.name, nullptr);
            
            // Reconnect to FluidNC
            Serial.printf("UICommon: Reconnecting to FluidNC at %s:%d\n", 
                         config.fluidnc_url, config.websocket_port);
            FluidNCClient::connect(config);
            
            // Restart connection timeout monitoring (10 seconds)
            connection_timeout_start = millis();
            connection_timeout_active = true;
            connection_error_shown = false;
        } else {
            Serial.println("\nWiFi reconnection failed!");
            UICommon::hideConnectingPopup();
            UICommon::showConnectionErrorDialog("WiFi Connection Failed", 
                "Could not reconnect to WiFi.\n\nCheck network settings and try again.");
        }
    }
}

static void on_connection_error_restart(lv_event_t *e) {
    Serial.println("UICommon: Restarting to change machine...");
    UICommon::hideConnectionErrorDialog();
    
    UIConnectionDialog::restarting();
    
    // Force display update
    lv_timer_handler();
    delay(500);
    
    // Restart the ESP32
    ESP.restart();
}

void UICommon::showConnectionErrorDialog(const char *title, const char *message) {
    hideConnectingPopup();
    FluidNCClient::stopReconnectionAttempts();
    hideConnectionErrorDialog();
    connection_error_dialog = UIConnectionDialog::error(title, message,
        on_connection_error_connect, on_connection_error_restart, on_connection_error_close);
    Serial.printf("UICommon: Connection error dialog shown - %s: %s\n", title, message);
}

void UICommon::hideConnectionErrorDialog() {
    if (connection_error_dialog) {
        lv_obj_del(connection_error_dialog);
        connection_error_dialog = nullptr;
        Serial.println("UICommon: Connection error dialog hidden");
    }
}

void UICommon::checkConnectionTimeout() {
    // If not monitoring timeout, nothing to do
    if (!connection_timeout_active) {
        return;
    }
    
    // If already connected, deactivate timeout and hide popups
    if (FluidNCClient::isConnected()) {
        connection_timeout_active = false;
        connection_error_shown = false;
        ever_connected_successfully = true;  // Mark that we've connected successfully
        hideConnectingPopup();  // Hide connecting popup when connected
        hideConnectionErrorDialog();
        return;
    }
    
    // Check if timeout exceeded (10 seconds)
    uint32_t elapsed = millis() - connection_timeout_start;
    if (elapsed >= 10000 && !connection_error_shown && !ever_connected_successfully) {
        // Only show error if we've never connected successfully (prevents popup on brief disconnects after initial connection)
        connection_error_shown = true;
        
        // Get machine config for error message
        MachineConfig config;
        if (MachineConfigManager::getSelectedMachine(config)) {
            // Build error message
            char error_msg[300];
            snprintf(error_msg, sizeof(error_msg), 
                    "Could not connect to machine:\n%s\n\nURL: %s:%d\n\nCheck that the machine is powered on\nand network connection is available.",
                    config.name, config.fluidnc_url, config.websocket_port);
            
            // Show error dialog
            showConnectionErrorDialog("Machine Connection Failed", error_msg);
            
            Serial.println("UICommon: Machine connection timeout - showing error dialog");
        }
    }
}

// HOLD popup - shown when machine enters HOLD state
void UICommon::showHoldPopup(const char *message) {
    if (hold_popup) return; // Already showing
    
    // Create modal backdrop
    hold_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(hold_popup, SCREEN_WIDTH, SCREEN_HEIGHT);
    UIAlerts::background(hold_popup, lv_color_hex(0x103f38), 0);
    lv_obj_set_style_pad_all(hold_popup, 0, 0);
    lv_obj_set_style_radius(hold_popup, 0, 0);
    lv_obj_set_pos(hold_popup, 0, 0);
    lv_obj_set_style_bg_opa(hold_popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hold_popup, 0, 0);
    lv_obj_clear_flag(hold_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(hold_popup);
    
    // Create dialog box
    lv_obj_t *dialog = lv_obj_create(hold_popup);
    lv_obj_set_size(dialog, 600, 300);
    UIAlerts::background(dialog, UIAlerts::BG_DARK, 0);
    lv_obj_set_style_border_color(dialog, UIAlerts::STATE_HOLD, 0);
    lv_obj_set_style_border_width(dialog, 3, 0);
    lv_obj_set_style_pad_all(dialog, 20, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(dialog);
    
    // State label (large, colored)
    lv_obj_t *state_label = lv_label_create(dialog);
    lv_label_set_text(state_label, LV_SYMBOL_PAUSE " Machine on hold");
    lv_obj_set_style_text_font(state_label, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(state_label, UIAlerts::UI_WARNING, 0);
    lv_obj_align(state_label, LV_ALIGN_TOP_MID, 0, 0);
    
    // Message label
    hold_popup_msg_label = lv_label_create(dialog);
    lv_label_set_text(hold_popup_msg_label, message && strlen(message) > 0 ? message : "Machine paused");
    lv_obj_set_style_text_font(hold_popup_msg_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hold_popup_msg_label, UIAlerts::TEXT_LIGHT, 0);
    lv_obj_set_size(hold_popup_msg_label, 520, 112);
    lv_label_set_long_mode(hold_popup_msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(hold_popup_msg_label, LV_ALIGN_TOP_MID, 0, 60);
    
    // Button container
    lv_obj_t *btn_container = lv_obj_create(dialog);
    lv_obj_set_size(btn_container, 560, 60);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_pad_all(btn_container, 0, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Resume button
    lv_obj_t *resume_btn = UIAlerts::button(btn_container);
    lv_obj_set_size(resume_btn, 250, 50);
    UIAlerts::background(resume_btn, UIAlerts::BTN_PLAY, 0);
    lv_obj_add_event_cb(resume_btn, [](lv_event_t *e) {
        FluidNCClient::sendCommand("~"); // Send cycle start (resume)
        // Clear the last message so it doesn't persist after resuming
        FluidNCClient::clearLastMessage();
        if (hold_popup_msg_label) lv_label_set_text(hold_popup_msg_label, "");
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t *resume_label = lv_label_create(resume_btn);
    lv_label_set_text(resume_label, LV_SYMBOL_PLAY " Resume");
    lv_obj_set_style_text_font(resume_label, &lv_font_montserrat_18, 0);
    lv_obj_center(resume_label);
    
    // Close button
    lv_obj_t *close_btn = UIAlerts::button(btn_container);
    lv_obj_set_size(close_btn, 250, 50);
    UIAlerts::background(close_btn, UIAlerts::BG_MEDIUM, 0);
    lv_obj_add_event_cb(close_btn, [](lv_event_t *e) {
        UICommon::hold_popup_dismissed = true;  // Mark as dismissed
        UICommon::hideHoldPopup();
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE " Close");
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_18, 0);
    lv_obj_center(close_label);
    
    Serial.println("UICommon: HOLD popup shown");
}

void UICommon::hideHoldPopup() {
    if (hold_popup) {
        lv_obj_del(hold_popup);
        hold_popup = nullptr;
        hold_popup_msg_label = nullptr;
        Serial.println("UICommon: HOLD popup hidden");
    }
}

// ALARM popup - shown when machine enters ALARM state
void UICommon::showAlarmPopup(const char *message) {
    if (alarm_popup) return; // Already showing
    
    // Create modal backdrop
    alarm_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(alarm_popup, SCREEN_WIDTH, SCREEN_HEIGHT);
    UIAlerts::background(alarm_popup, lv_color_hex(0x103f38), 0);
    lv_obj_set_style_pad_all(alarm_popup, 0, 0);
    lv_obj_set_style_radius(alarm_popup, 0, 0);
    lv_obj_set_pos(alarm_popup, 0, 0);
    lv_obj_set_style_bg_opa(alarm_popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(alarm_popup, 0, 0);
    lv_obj_clear_flag(alarm_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(alarm_popup);
    
    // Create dialog box
    lv_obj_t *dialog = lv_obj_create(alarm_popup);
    lv_obj_set_size(dialog, 600, 300);
    UIAlerts::background(dialog, UIAlerts::BG_DARK, 0);
    lv_obj_set_style_border_color(dialog, UIAlerts::STATE_ALARM, 0);
    lv_obj_set_style_border_width(dialog, 3, 0);
    lv_obj_set_style_pad_all(dialog, 20, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(dialog);
    
    // State label (large, colored)
    lv_obj_t *state_label = lv_label_create(dialog);
    lv_label_set_text(state_label, LV_SYMBOL_WARNING " Machine alarm");
    lv_obj_set_style_text_font(state_label, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(state_label, UIAlerts::STATE_ALARM, 0);
    lv_obj_align(state_label, LV_ALIGN_TOP_MID, 0, 0);
    
    // Message label
    alarm_popup_msg_label = lv_label_create(dialog);
    lv_label_set_text(alarm_popup_msg_label, message && strlen(message) > 0 ? message : "Alarm condition detected");
    lv_obj_set_style_text_font(alarm_popup_msg_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(alarm_popup_msg_label, UIAlerts::TEXT_LIGHT, 0);
    lv_obj_set_size(alarm_popup_msg_label, 520, 112);
    lv_label_set_long_mode(alarm_popup_msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(alarm_popup_msg_label, LV_ALIGN_TOP_MID, 0, 60);
    
    // Button container
    lv_obj_t *btn_container = lv_obj_create(dialog);
    lv_obj_set_size(btn_container, 560, 60);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_pad_all(btn_container, 0, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Clear Alarm button
    lv_obj_t *clear_btn = UIAlerts::button(btn_container);
    lv_obj_set_size(clear_btn, 250, 50);
    UIAlerts::background(clear_btn, UIAlerts::UI_WARNING, 0);
    lv_obj_add_event_cb(clear_btn, [](lv_event_t *e) {
        // Send soft reset, then unlock
        FluidNCClient::sendCommand("\x18"); // Ctrl-X (soft reset)
        delay(100);
        FluidNCClient::sendCommand("$X\n");   // Unlock
        // Clear the last message so it doesn't persist after clearing the alarm
        FluidNCClient::clearLastMessage();
        if (alarm_popup_msg_label) lv_label_set_text(alarm_popup_msg_label, "");
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, LV_SYMBOL_REFRESH " Clear Alarm");
    lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_18, 0);
    lv_obj_center(clear_label);
    
    // Close button
    lv_obj_t *close_btn = UIAlerts::button(btn_container);
    lv_obj_set_size(close_btn, 250, 50);
    UIAlerts::background(close_btn, UIAlerts::BG_MEDIUM, 0);
    lv_obj_add_event_cb(close_btn, [](lv_event_t *e) {
        UICommon::alarm_popup_dismissed = true;  // Mark as dismissed
        UICommon::hideAlarmPopup();
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE " Close");
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_18, 0);
    lv_obj_center(close_label);
    
    Serial.println("UICommon: ALARM popup shown");
}

void UICommon::hideAlarmPopup() {
    if (alarm_popup) {
        lv_obj_del(alarm_popup);
        alarm_popup = nullptr;
        alarm_popup_msg_label = nullptr;
        Serial.println("UICommon: ALARM popup hidden");
    }
}

// Check current state and manage popups accordingly
void UICommon::checkStatePopups(int current_state, const char *last_message) {
    // If state changed from previous, reset dismissal flags and hide any existing popups
    if (current_state != last_popup_state && last_popup_state != -1) {
        // State changed - reset dismissal flags
        if (current_state != STATE_HOLD) {
            hold_popup_dismissed = false;
        }
        if (current_state != STATE_ALARM) {
            alarm_popup_dismissed = false;
        }
        
        // Hide any existing popups
        hideHoldPopup();
        hideAlarmPopup();
    }
    
    // Show appropriate popup for current state (only if not dismissed)
    if (current_state == STATE_HOLD && !hold_popup_dismissed) {
        if (!hold_popup) {
            showHoldPopup(last_message);
        } else if (hold_popup_msg_label && last_message) {
            lv_label_set_text(hold_popup_msg_label, last_message);
        }
    } else if (current_state == STATE_ALARM && !alarm_popup_dismissed) {
        if (!alarm_popup) {
            showAlarmPopup(last_message);
        } else if (alarm_popup_msg_label && last_message) {
            lv_label_set_text(alarm_popup_msg_label, last_message);
        }
    }
    
    // Update last state
    last_popup_state = current_state;
}

void UICommon::updateFileProgress(bool is_printing, float percent, const char *filename, uint32_t elapsed_ms) {
    if (!lbl_file_progress_container) return;
    
    if (is_printing) {
        // Show job progress, hide normal status/position display
        lv_obj_clear_flag(lbl_file_progress_container, LV_OBJ_FLAG_HIDDEN);
        if (lbl_status) lv_obj_add_flag(lbl_status, LV_OBJ_FLAG_HIDDEN);
        if (lbl_wpos_label) lv_obj_add_flag(lbl_wpos_label, LV_OBJ_FLAG_HIDDEN);
        if (lbl_wpos_x) lv_obj_add_flag(lbl_wpos_x, LV_OBJ_FLAG_HIDDEN);
        if (lbl_wpos_y) lv_obj_add_flag(lbl_wpos_y, LV_OBJ_FLAG_HIDDEN);
        if (lbl_wpos_z) lv_obj_add_flag(lbl_wpos_z, LV_OBJ_FLAG_HIDDEN);
        if (lbl_mpos_label) lv_obj_add_flag(lbl_mpos_label, LV_OBJ_FLAG_HIDDEN);
        if (lbl_mpos_x) lv_obj_add_flag(lbl_mpos_x, LV_OBJ_FLAG_HIDDEN);
        if (lbl_mpos_y) lv_obj_add_flag(lbl_mpos_y, LV_OBJ_FLAG_HIDDEN);
        if (lbl_mpos_z) lv_obj_add_flag(lbl_mpos_z, LV_OBJ_FLAG_HIDDEN);
        
        // Update filename (delta check)
        if (lbl_filename && filename && strcmp(filename, last_filename) != 0) {
            lv_label_set_text(lbl_filename, filename);
            strncpy(last_filename, filename, sizeof(last_filename) - 1);
            last_filename[sizeof(last_filename) - 1] = '\0';
        }
        
        // Update progress bar and percentage (delta check with 0.1% tolerance)
        if (fabsf(percent - last_percent) >= 0.1f) {
            if (bar_progress) {
                lv_bar_set_value(bar_progress, (int)percent, LV_ANIM_OFF);
            }
            if (lbl_percent) {
                char percent_text[8];
                snprintf(percent_text, sizeof(percent_text), "%.1f%%", percent);
                lv_label_set_text(lbl_percent, percent_text);
            }
            last_percent = percent;
        }
        
        // Update elapsed time (delta check - only update if changed by 1+ second)
        uint32_t elapsed_sec = elapsed_ms / 1000;
        if (elapsed_sec != last_elapsed_sec) {
            if (lbl_elapsed_time) {
                uint32_t hours = elapsed_sec / 3600;
                uint32_t minutes = (elapsed_sec % 3600) / 60;
                
                char time_text[16];
                if (hours > 0) {
                    snprintf(time_text, sizeof(time_text), "%d:%02d", hours, minutes);
                    if (lbl_elapsed_unit) lv_label_set_text(lbl_elapsed_unit, "hr:min");
                } else {
                    snprintf(time_text, sizeof(time_text), "%d:%02d", minutes, (int)(elapsed_sec % 60));
                    if (lbl_elapsed_unit) lv_label_set_text(lbl_elapsed_unit, "min:sec");
                }
                lv_label_set_text(lbl_elapsed_time, time_text);
            }
            last_elapsed_sec = elapsed_sec;
        }
        
        // Calculate and update estimated time (delta check - only update if changed by 1+ second)
        if (lbl_estimated_time && percent > 0.1f) {
            uint32_t estimated_total_sec = (uint32_t)((elapsed_sec / percent) * 100.0f);
            
            if (estimated_total_sec != last_estimated_sec) {
                uint32_t est_hours = estimated_total_sec / 3600;
                uint32_t est_minutes = (estimated_total_sec % 3600) / 60;
                
                char est_text[20];
                if (est_hours > 0) {
                    snprintf(est_text, sizeof(est_text), "%d:%02d", est_hours, est_minutes);
                    if (lbl_estimated_unit) lv_label_set_text(lbl_estimated_unit, "hr:min");
                } else {
                    snprintf(est_text, sizeof(est_text), "%d:%02d", est_minutes, (int)(estimated_total_sec % 60));
                    if (lbl_estimated_unit) lv_label_set_text(lbl_estimated_unit, "min:sec");
                }
                lv_label_set_text(lbl_estimated_time, est_text);
                last_estimated_sec = estimated_total_sec;
            }
        }
    } else {
        // Hide job progress, show normal status/position display
        lv_obj_add_flag(lbl_file_progress_container, LV_OBJ_FLAG_HIDDEN);
        if (lbl_status) lv_obj_clear_flag(lbl_status, LV_OBJ_FLAG_HIDDEN);
        if (lbl_wpos_label) lv_obj_clear_flag(lbl_wpos_label, LV_OBJ_FLAG_HIDDEN);
        if (lbl_wpos_x) lv_obj_clear_flag(lbl_wpos_x, LV_OBJ_FLAG_HIDDEN);
        if (lbl_wpos_y) lv_obj_clear_flag(lbl_wpos_y, LV_OBJ_FLAG_HIDDEN);
        if (lbl_wpos_z) lv_obj_clear_flag(lbl_wpos_z, LV_OBJ_FLAG_HIDDEN);
        if (lbl_mpos_label) lv_obj_clear_flag(lbl_mpos_label, LV_OBJ_FLAG_HIDDEN);
        if (lbl_mpos_x) lv_obj_clear_flag(lbl_mpos_x, LV_OBJ_FLAG_HIDDEN);
        if (lbl_mpos_y) lv_obj_clear_flag(lbl_mpos_y, LV_OBJ_FLAG_HIDDEN);
        if (lbl_mpos_z) lv_obj_clear_flag(lbl_mpos_z, LV_OBJ_FLAG_HIDDEN);
        
        // Reset cached values when not printing
        last_filename[0] = '\0';
        last_percent = -1.0f;
        last_elapsed_sec = 0xFFFFFFFF;
        last_estimated_sec = 0xFFFFFFFF;
    }
}

void UICommon::showWCSLockDialog(const char *wcs_code, const char *wcs_name, void (*continue_callback)(lv_event_t*)) {
    // Create modal backdrop
    lv_obj_t *backdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_size(backdrop, LV_PCT(100), LV_PCT(100));
    UIAlerts::background(backdrop, lv_color_hex(0x103f38), 0);
    lv_obj_set_style_pad_all(backdrop, 0, 0);
    lv_obj_set_style_radius(backdrop, 0, 0);
    lv_obj_set_pos(backdrop, 0, 0);
    lv_obj_set_style_bg_opa(backdrop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(backdrop, 0, 0);
    lv_obj_clear_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create dialog container
    lv_obj_t *dialog = lv_obj_create(backdrop);
    lv_obj_set_size(dialog, 600, 300);
    lv_obj_center(dialog);
    UIAlerts::background(dialog, UIAlerts::BG_DARKER, 0);
    lv_obj_set_style_border_color(dialog, UIAlerts::UI_WARNING, 0);
    lv_obj_set_style_border_width(dialog, 3, 0);
    lv_obj_set_style_pad_all(dialog, 20, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title with warning icon and WCS code
    lv_obj_t *title = lv_label_create(dialog);
    char title_text[80];
    if (wcs_name && strlen(wcs_name) > 0) {
        snprintf(title_text, sizeof(title_text), LV_SYMBOL_WARNING " %s (%s) is Locked", wcs_code, wcs_name);
    } else {
        snprintf(title_text, sizeof(title_text), LV_SYMBOL_WARNING " %s is Locked", wcs_code);
    }
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, UIAlerts::UI_WARNING, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title, 560);
    lv_obj_set_pos(title, 0, 0);
    
    // Warning message with centered text and more spacing from title
    lv_obj_t *message = lv_label_create(dialog);
    lv_label_set_text(message, "Modifying this position may affect fixtures or setups.\n\nAre you sure you want to continue?");
    lv_obj_set_style_text_font(message, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(message, UIAlerts::TEXT_LIGHT, 0);
    lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(message, 560);
    lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(message, 0, 75);
    
    // Cancel button (left)
    lv_obj_t *btn_cancel = UIAlerts::button(dialog);
    lv_obj_set_size(btn_cancel, 220, 50);
    lv_obj_set_pos(btn_cancel, 40, 200);
    UIAlerts::background(btn_cancel, UIAlerts::BG_MEDIUM, 0);
    
    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, "Cancel");
    lv_obj_set_style_text_font(lbl_cancel, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_cancel);
    
    // Close on Cancel
    lv_obj_add_event_cb(btn_cancel, [](lv_event_t *e) {
        lv_obj_t *backdrop = (lv_obj_t*)lv_event_get_user_data(e);
        lv_obj_delete(backdrop);
    }, LV_EVENT_CLICKED, backdrop);
    
    // Continue button (right) with warning icon
    lv_obj_t *btn_continue = UIAlerts::button(dialog);
    lv_obj_set_size(btn_continue, 220, 50);
    lv_obj_set_pos(btn_continue, 280, 200);
    UIAlerts::background(btn_continue, UIAlerts::UI_WARNING, 0);
    
    lv_obj_t *lbl_continue = lv_label_create(btn_continue);
    lv_label_set_text(lbl_continue, LV_SYMBOL_WARNING " Continue");
    lv_obj_set_style_text_font(lbl_continue, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_continue);
    
    // Store callback and backdrop in user data
    struct CallbackData {
        void (*callback)(lv_event_t*);
        lv_obj_t *backdrop;
    };
    
    CallbackData *data = (CallbackData*)malloc(sizeof(CallbackData));
    data->callback = continue_callback;
    data->backdrop = backdrop;
    
    lv_obj_add_event_cb(btn_continue, [](lv_event_t *e) {
        CallbackData *data = (CallbackData*)lv_event_get_user_data(e);
        
        // Call the continue callback
        if (data->callback) {
            data->callback(e);
        }
        
        // Close dialog
        lv_obj_delete(data->backdrop);
        free(data);
    }, LV_EVENT_CLICKED, data);
}

// Load system preferences once at startup
void UICommon::loadSystemPreferences() {
    // Load enable_a_axis from the selected machine's config (it's machine-specific)
    enable_a_axis = false;
    MachineConfig config;
    if (MachineConfigManager::getSelectedMachine(config)) {
        enable_a_axis = config.enable_a_axis;
    }

    Serial.printf("UICommon: Loaded system preferences - enable_a_axis=%d\n", enable_a_axis);
}

// Get cached A-axis enabled preference
bool UICommon::isAAxisEnabled() {
    return enable_a_axis;
}

// Set cached A-axis enabled preference (call after saving to preferences)
void UICommon::setAAxisEnabled(bool enabled) {
    enable_a_axis = enabled;
    Serial.printf("UICommon: Updated enable_a_axis to %d\n", enabled);
}
