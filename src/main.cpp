#include "core/ui_perf.h"
#include <Arduino.h>
#include <lvgl.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include "network/ota_updater.h"
#include "core/display_driver.h"     // Display driver module
#include "core/touch_driver.h"       // Touch driver module
#include "core/power_manager.h"      // Power management module
#include "network/screenshot_server.h"  // Screenshot web server
#include "network/fluidnc_client.h"     // FluidNC WebSocket client
#include "ui/ui_theme.h"        // UI theme colors
#include "ui/ui_splash.h"       // Splash screen module
#include "ui/ui_machine_select.h" // Machine selection screen
#include "ui/ui_common.h"       // UI common components (status bar)
#include "ui/ui_tabs.h"         // UI tabs module
#include "ui/settings_manager.h" // Settings import/export/clear
#include "ui/tabs/ui_tab_status.h" // Status tab for updates
#include "ui/tabs/ui_tab_files.h" // Files tab for refresh check
#include "ui/tabs/ui_tab_macros.h" // Macros tab for progress updates
#include "ui/tabs/ui_tab_terminal.h" // Terminal tab for updates
#include "ui/tabs/settings/ui_tab_settings_about.h" // About tab for screenshot URL updates
#include "ui/tabs/control/ui_tab_control_actions.h" // Actions tab for pause button updates
#include "ui/tabs/control/ui_tab_control_override.h" // Override tab for updates
#include "ui/tabs/control/ui_tab_control_probe.h"   // Probe tab for probe indicator
#include "ui/machine_config.h"  // Machine configuration manager

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== MechTouch - LVGL 9 with LovyanGFX ===");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

    // Read-only OTA readiness report; does not alter partitions or flash.
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
    const esp_partition_t *otaData = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr);
    const bool hasNextSlot = running && next && running->address != next->address;
    Serial.printf("[OTA-CHECK] flash=%lu app=%lu running=%lu next=%lu otadata=%u\n",
        (unsigned long)ESP.getFlashChipSize(), (unsigned long)ESP.getSketchSize(),
        (unsigned long)(running ? running->size : 0),
        (unsigned long)(hasNextSlot ? next->size : 0), otaData ? 1U : 0U);

    // Initialize Display Driver
    Serial.println("Initializing display driver...");
    static DisplayDriver displayDriver;
    if (!displayDriver.init()) {
        Serial.println("ERROR: Failed to initialize display!");
        while (1) delay(1000);
    }
    Serial.println("Display driver initialized successfully");
    
    // Load and apply display rotation preference
    {
        Preferences prefs;
        prefs.begin(PREFS_SYSTEM_NAMESPACE, true);  // Read-only
        uint8_t display_rotation = prefs.getUChar("display_rot", 0);  // Default to 0 (normal)
        prefs.end();
        
        Serial.printf("Main: Loading display rotation: %d degrees\n", display_rotation * 90);
        displayDriver.setRotation(display_rotation);
    }

    // Initialize Touch Driver
    Serial.println("Initializing touch driver...");
    static TouchDriver touchDriver;
    if (!touchDriver.init(displayDriver.getLCD())) {
        Serial.println("ERROR: Failed to initialize touch!");
        while (1) delay(1000);
    }
    Serial.println("Touch driver initialized successfully");

    // Initialize Power Manager
    Serial.println("Initializing power manager...");
    PowerManager::init(&displayDriver);
    Serial.println("Power manager initialized successfully");

    // Store display driver reference for later use (screenshot server after WiFi connects)
    UICommon::setDisplayDriver(&displayDriver);

    // Initialize Screenshot Server (WiFi not connected yet, will initialize after machine selection)
    Serial.println("Screenshot server will initialize after WiFi connection...");

    // Initialize FluidNC Client
    Serial.println("Initializing FluidNC client...");
    FluidNCClient::init();

    // Check for auto-import (only if no machines configured)
    Serial.println("Checking for settings auto-import...");
    if (SettingsManager::autoImportOnBoot()) {
        Serial.println("Settings imported successfully! Restarting...");
        delay(2000);  // Give user time to see serial message
        ESP.restart();
    }

    // Show splash screen
    Serial.println("Showing splash screen...");
    UISplash::show(displayDriver.getDisplay());

    // Check if machine selection should be shown
    Preferences prefs;
    prefs.begin(PREFS_SYSTEM_NAMESPACE, true);  // Read-only
    bool show_machine_select = prefs.getBool("show_mach_sel", true);  // Default to true
    prefs.end();
    
    Serial.printf("Main: show_mach_sel preference = %d\n", show_machine_select);
    
    if (show_machine_select) {
        // Show machine selection screen
        Serial.println("Showing machine selection screen...");
        UIMachineSelect::show(displayDriver.getDisplay());
    } else {
        // Auto-load first configured machine
        Serial.println("Auto-loading first machine...");
        MachineConfig machines[MAX_MACHINES];
        MachineConfigManager::loadMachines(machines);
        
        // Find first configured machine
        int first_machine_index = -1;
        for (int i = 0; i < MAX_MACHINES; i++) {
            if (machines[i].is_configured) {
                first_machine_index = i;
                break;
            }
        }
        
        if (first_machine_index >= 0) {
            // Set as selected machine and initialize UI
            MachineConfigManager::setSelectedMachineIndex(first_machine_index);
            Serial.printf("Auto-selected machine: %s\n", machines[first_machine_index].name);
            
            // Initialize main UI directly
            UICommon::createMainUI();
        } else {
            // No machines configured, show selection screen anyway
            Serial.println("No machines configured, showing selection screen...");
            UIMachineSelect::show(displayDriver.getDisplay());
        }
    }
}

namespace {
uint32_t maxLvglUs = 0;
uint32_t maxNetworkUs = 0;
uint32_t maxScreenshotUs = 0;
uint32_t maxFilesUs = 0;
uint32_t maxLoopUs = 0;
UIWorkCounters slowestUIWork;
uint32_t slowestOtherUs = 0;
void recordMax(uint32_t &maximum, uint32_t started) {
    uint32_t elapsed = micros() - started;
    if (elapsed > maximum) maximum = elapsed;
}
void serviceUI() {
    // Both calls share this timestamp: elapsed time is never counted twice.
    static uint32_t lastTick = millis();
    uint32_t now = millis();
    lv_tick_inc(now - lastTick);
    lastTick = now;
    UIWorkCounters before = uiWorkCounters();
    uint32_t started = micros();
    lv_timer_handler();
    uint32_t elapsed = micros() - started;
    if (elapsed > maxLvglUs) {
        maxLvglUs = elapsed;
        const UIWorkCounters &after = uiWorkCounters();
        slowestUIWork.touch_us = after.touch_us - before.touch_us;
        slowestUIWork.swap_us = after.swap_us - before.swap_us;
        slowestUIWork.push_us = after.push_us - before.push_us;
        slowestUIWork.pixels = after.pixels - before.pixels;
        slowestUIWork.flushes = after.flushes - before.flushes;
        uint64_t accounted = slowestUIWork.touch_us + slowestUIWork.swap_us + slowestUIWork.push_us;
        slowestOtherUs = accounted < elapsed ? elapsed - (uint32_t)accounted : 0;
    }
}
}

void loop()
{
    const uint32_t loopStarted = micros();
    serviceUI();
    // Forward any Serial input to FluidNC (for debugging via Serial Monitor)
    static char serial_buf[128];
    static uint8_t serial_buf_pos = 0;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serial_buf_pos > 0) {
                serial_buf[serial_buf_pos++] = '\n';
                serial_buf[serial_buf_pos] = '\0';
                Serial.printf("[Serial->FluidNC] %s", serial_buf);
                FluidNCClient::sendCommand(serial_buf);
                serial_buf_pos = 0;
            }
        } else if (serial_buf_pos < sizeof(serial_buf) - 2) {
            serial_buf[serial_buf_pos++] = c;
        }
    }

    // Handle screenshot server web requests
    {
        uint32_t started = micros();
        ScreenshotServer::handleClient();
        recordMax(maxScreenshotUs, started);
    }
    
    // Handle FluidNC client WebSocket events
    {
        uint32_t started = micros();
        FluidNCClient::loop();
        recordMax(maxNetworkUs, started);
    }
    
    // Check for connection timeout (non-blocking)
    OTAUpdater::loop();
    UICommon::checkConnectionTimeout();
    
    // Check for pending file list refresh (from Files tab delete callback)
    {
        uint32_t started = micros();
        UITabFiles::checkPendingRefresh();
        recordMax(maxFilesUs, started);
    }
    
    // Update UI from FluidNC status (every 250ms)
    static uint32_t lastUIUpdate = 0;
    uint32_t currentMillis = millis();
    // FAST POSITION UPDATE - 20 Hz
    static uint32_t lastPositionUpdate = 0;

    bool position_machine_connected = FluidNCClient::isConnected();

    if (position_machine_connected &&
        currentMillis - lastPositionUpdate >= 50) {

        lastPositionUpdate = currentMillis;

        const FluidNCStatus& positionStatus =
            FluidNCClient::getStatus();

        // Top status bar coordinates
        UICommon::updateMachinePosition(
            positionStatus.mpos_x,
            positionStatus.mpos_y,
            positionStatus.mpos_z
        );

        UICommon::updateWorkPosition(
            positionStatus.wpos_x,
            positionStatus.wpos_y,
            positionStatus.wpos_z,
            positionStatus.wpos_a
        );

        // Status tab coordinates
        UITabStatus::updateWorkPosition(
            positionStatus.wpos_x,
            positionStatus.wpos_y,
            positionStatus.wpos_z,
            positionStatus.wpos_a
        );

        UITabStatus::updateMachinePosition(
            positionStatus.mpos_x,
            positionStatus.mpos_y,
            positionStatus.mpos_z,
            positionStatus.mpos_a
        );
    }
    if (currentMillis - lastUIUpdate >= 250) {
        lastUIUpdate = currentMillis;
        
        bool machine_connected = FluidNCClient::isConnected();
        bool wifi_connected = (WiFi.status() == WL_CONNECTED);
        
        // Update connection status symbols (always update, even if not connected)
        UICommon::updateConnectionStatus(machine_connected, wifi_connected);
        
        // Update About tab screenshot server URL (in case WiFi status changed)
        UITabSettingsAbout::update();
        
        // Only update other status info if machine is connected
        if (machine_connected) {
            const FluidNCStatus& status = FluidNCClient::getStatus();
        
            // Update status bar
            const char* state_str = "IDLE";
            switch (status.state) {
                case STATE_IDLE: state_str = "IDLE"; break;
                case STATE_RUN: state_str = "RUN"; break;
                case STATE_HOLD: state_str = "HOLD"; break;
                case STATE_JOG: state_str = "JOG"; break;
                case STATE_ALARM: state_str = "ALARM"; break;
                case STATE_DOOR: state_str = "DOOR"; break;
                case STATE_CHECK: state_str = "CHECK"; break;
                case STATE_HOME: state_str = "HOME"; break;
                case STATE_SLEEP: state_str = "SLEEP"; break;
                default: state_str = "DISCONNECTED"; break;
            }
            
            UICommon::updateMachineState(state_str);
            // Check for HOLD/ALARM state and show popups if needed
            UICommon::checkStatePopups(status.state, status.last_message);

            // Update Control Actions pause/resume button based on machine state
            UITabControlActions::updatePauseButton(status.state);

            // Update Status tab
            UITabStatus::updateState(state_str);
            UITabStatus::updateFeedRate(status.feed_rate, status.feed_override);
            UITabStatus::updateRapidOverride(status.rapid_override);
            UITabStatus::updateSpindle(status.spindle_speed, status.spindle_override);
            UITabStatus::updateModalStates(status.modal_wcs, status.modal_plane, status.modal_distance,
                                        status.modal_units, status.modal_motion, status.modal_feedrate,
                                        status.modal_spindle, status.modal_coolant, status.modal_tool);
            UITabStatus::updateMessage(status.last_message);
            UITabStatus::updateLimitSwitches(status.pin_limit_x, status.pin_limit_y, status.pin_limit_z, status.pin_limit_a);
            UITabControlActions::updateLimitSwitches(status.pin_limit_x, status.pin_limit_y, status.pin_limit_z, status.pin_limit_a);
            UITabStatus::updateProbe(status.pin_probe);
            UITabControlProbe::updateProbe(status.pin_probe);
            
            // Update file progress in status bar (UICommon) instead of status tab
            UICommon::updateFileProgress(status.is_sd_printing, status.sd_percent,
                                        status.sd_filename, status.sd_elapsed_ms);
            
            // Update control buttons visibility based on machine state
            UITabStatus::updateControlButtons(status.state);
            
            // Update Macros tab progress (only when a macro from that tab is running)
            static bool macro_print_started = false;  // Track if SD print actually started
            static unsigned long completion_display_start = 0;  // Track 100% display time
            static unsigned long macro_start_time = 0;  // Track when macro button was clicked
            static const unsigned long COMPLETION_DISPLAY_MS = 2000;  // Show 100% for 2 seconds
            static const unsigned long FAST_MACRO_TIMEOUT_MS = 1000;  // If no SD activity within 1s, assume macro completed
            bool is_macro_running = UITabMacros::isMacroRunning();
            
            // Detect when a new macro starts (transition from not running to running)
            static bool was_macro_running = false;
            if (is_macro_running && !was_macro_running) {
                // New macro just started
                macro_start_time = millis();
                macro_print_started = false;
                Serial.printf("[Main] New macro started, tracking start time\n");
            }
            was_macro_running = is_macro_running;
            
            if (status.is_sd_printing && status.sd_percent > 0 && is_macro_running) {
                macro_print_started = true;  // Mark that print has started
                completion_display_start = 0;  // Reset completion timer while printing
                Serial.printf("[Main] Showing progress: printing=%d, percent=%.1f, macro_running=%d\n", 
                    status.is_sd_printing, status.sd_percent, is_macro_running);
                // Use the stored macro name (not the SD filename)
                UITabMacros::updateProgress((int)status.sd_percent, UITabMacros::getRunningMacroName(), status.last_message);
                UITabMacros::showProgress();
            } else {
                // Check if macro completed (either we saw SD activity that stopped, or it was too fast)
                bool macro_completed = false;
                
                if (is_macro_running && macro_print_started && !status.is_sd_printing) {
                    // Normal case: SD print started and then stopped
                    macro_completed = true;
                    Serial.printf("[Main] Macro completed (normal)\n");
                } else if (is_macro_running && !macro_print_started && macro_start_time > 0 && 
                          (millis() - macro_start_time >= FAST_MACRO_TIMEOUT_MS)) {
                    // Fast macro case: never saw SD activity, but enough time passed
                    macro_completed = true;
                    Serial.printf("[Main] Macro completed (fast, no SD activity detected)\n");
                }
                
                if (macro_completed) {
                    // Macro completed - start 2-second display timer if not already started
                    if (completion_display_start == 0) {
                        completion_display_start = millis();
                        Serial.printf("[Main] Showing 100%% for 2 seconds\n");
                        // Show 100% with the macro name
                        UITabMacros::updateProgress(100, UITabMacros::getRunningMacroName(), "Complete");
                        UITabMacros::showProgress();
                    } else {
                        // Check if 2 seconds have elapsed
                        if (millis() - completion_display_start >= COMPLETION_DISPLAY_MS) {
                            Serial.printf("[Main] Completion display timeout, clearing macro\n");
                            UITabMacros::clearRunningMacro();
                            macro_print_started = false;
                            completion_display_start = 0;
                            macro_start_time = 0;
                            UITabMacros::hideProgress();
                        } else {
                            // Still within 2-second window, keep showing 100%
                            UITabMacros::showProgress();
                        }
                    }
                } else if (is_macro_running && !macro_print_started && macro_start_time > 0) {
                    // Macro is running but hasn't started SD print yet - keep showing progress
                    UITabMacros::showProgress();
                } else {
                    // No macro running, hide progress
                    UITabMacros::hideProgress();
                }
            }
            
            // Update Override tab
            UITabControlOverride::updateValues(status.feed_override, status.rapid_override, status.spindle_override);
            
            // Update power manager with current machine state
            PowerManager::update(status.state);
        } else {
            // Machine disconnected - show OFFLINE state and reset all values to dashes
            UICommon::updateMachineState("OFFLINE");
            UICommon::updateMachinePosition(-9999.0f, -9999.0f, -9999.0f);  // Triggers dash display
            UICommon::updateWorkPosition(-9999.0f, -9999.0f, -9999.0f, -9999.0f);     // Triggers dash display

            // Update Status tab with OFFLINE state and reset all values
            UITabStatus::updateState("OFFLINE");
            UITabStatus::updateWorkPosition(-9999.0f, -9999.0f, -9999.0f, -9999.0f);
            UITabStatus::updateMachinePosition(-9999.0f, -9999.0f, -9999.0f, -9999.0f);
            UITabStatus::updateFeedRate(-9999.0f, -9999.0f);  // Reset feed rate and override
            UITabStatus::updateRapidOverride(-9999.0f);        // Reset rapid override
            UITabStatus::updateSpindle(-9999.0f, -9999.0f);    // Reset spindle and override
            UITabStatus::updateModalStates("---", "---", "---", "---", "---", "---", "---", "---", "---");
            
            // Update power manager with OFFLINE state (treat as IDLE for power management)
            PowerManager::update(STATE_IDLE);
        }
    }
    

    
    // Update Terminal tab (batched UI updates every 100ms)
    UITabTerminal::update();
    
    serviceUI();
    // Yield to background tasks.
    delay(1);
    recordMax(maxLoopUs, loopStarted);

    // Status update every 5 seconds
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        lastUpdate = millis();
        // Maximum durations in this window, not end-to-end touch latency.
        // LVGL time includes event callbacks, such as Files refresh.
        Serial.printf("[PERF] max_ms loop=%.1f lvgl=%.1f network=%.1f screenshot=%.1f files=%.1f\n",
                      maxLoopUs / 1000.0f, maxLvglUs / 1000.0f,
                      maxNetworkUs / 1000.0f, maxScreenshotUs / 1000.0f,
                      maxFilesUs / 1000.0f);
        // Components of the SAME slowest LVGL call in this window.
        // "other" includes render, layout, event callbacks and scheduler waits.
        Serial.printf("[UI-SLOW] total=%.1f touch=%.1f swap=%.1f push=%.1f other=%.1f ms pixels=%lu flushes=%lu\n",
                      maxLvglUs / 1000.0f, slowestUIWork.touch_us / 1000.0,
                      slowestUIWork.swap_us / 1000.0, slowestUIWork.push_us / 1000.0,
                      slowestOtherUs / 1000.0f, (unsigned long)slowestUIWork.pixels,
                      (unsigned long)slowestUIWork.flushes);
        slowestUIWork = UIWorkCounters{};
        slowestOtherUs = 0;
        maxLoopUs = maxLvglUs = maxNetworkUs = maxScreenshotUs = maxFilesUs = 0;
        Serial.printf("[%lu] LVGL running, Free heap: %d, FluidNC: %s\n", 
                      millis()/1000, ESP.getFreeHeap(),
                      FluidNCClient::isConnected() ? "Connected" : "Disconnected");
    }
}
