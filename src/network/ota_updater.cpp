#include "network/ota_updater.h"
#include "network/ota_version.h"
#include "network/ota_ca.h"
#include "network/fluidnc_client.h"
#include "ui/tabs/ui_tab_macros.h"
#include "ui/upload_manager.h"
#include "core/power_manager.h"
#include "ui/ui_alerts.h"
#include "config.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include <atomic>
#include <time.h>
#include <lvgl.h>
#include <algorithm>

namespace {
#if defined(HARDWARE_BASIC)
#define MECHTOUCH_OTA_BOARD "basic"
#elif defined(HARDWARE_ADVANCE_V12)
#define MECHTOUCH_OTA_BOARD "advance-v1.2"
#else
#define MECHTOUCH_OTA_BOARD "advance-v1.3"
#endif
constexpr const char *board = MECHTOUCH_OTA_BOARD;
// Kept in the binary through the serial reference; release packaging verifies it.
constexpr const char *releaseIdentity = "MechTouch-OTA:" FLUIDTOUCH_VERSION ":" MECHTOUCH_OTA_BOARD;
constexpr const char *base = "https://github.com/kajmeji/MechTouch/releases/";
enum State { Ready, Checking, Available, Current, Downloading, Success, Failed };
std::atomic<State> state{Ready};
std::atomic<int> progress{0};
std::atomic<bool> permitted{false};
String version, url, digest;
size_t firmwareSize = 0;
char errorText[160] = {};
char downloadError[160] = {};
lv_obj_t *overlay = nullptr, *message = nullptr, *bar = nullptr, *install = nullptr, *closeButton = nullptr;
bool automaticChecked = false;
uint32_t restartAt = 0, pollAt = 0;

bool idle() {
    const auto &s = FluidNCClient::getStatus();
    return FluidNCClient::isConnected() && s.state == STATE_IDLE &&
        s.last_update_ms && millis() - s.last_update_ms < 3000 &&
        !s.is_sd_printing && !UITabMacros::isMacroRunning() && !UploadManager::isUploading() && s.spindle_speed == 0;
}
void fail(const char *reason) {
    snprintf(errorText, sizeof(errorText), "%s", reason);
    state.store(Failed);
}
bool trusted(const String &u) {
    return u.startsWith(base) || u.startsWith("https://release-assets.githubusercontent.com/") ||
           u.startsWith("https://objects.githubusercontent.com/");
}
esp_err_t redirectEvent(esp_http_client_event_t *e) {
    if (e->event_id == HTTP_EVENT_ON_HEADER && e->user_data && e->header_key &&
        strcasecmp(e->header_key, "Location") == 0)
        *static_cast<String *>(e->user_data) = e->header_value;
    return ESP_OK;
}
// Follow only HTTPS redirects to GitHub asset hosts; authenticate every TLS connection.
esp_http_client_handle_t openDownload(String target, int &length) {
    snprintf(downloadError, sizeof(downloadError), "Update redirect limit or untrusted download host.");
    for (int i = 0; i < 5 && trusted(target); ++i) {
        String location;
        esp_http_client_config_t cfg = {};
        cfg.url = target.c_str(); cfg.cert_pem = otaRootCertificates;
        // GitHub signed asset URLs exceed ESP-IDF's default 512-byte TX buffer.
        // The request line (including its query string) must fit in one buffer.
        cfg.buffer_size_tx = 4096;
        cfg.timeout_ms = 12000; cfg.disable_auto_redirect = true;
        cfg.event_handler = redirectEvent; cfg.user_data = &location;
        auto h = esp_http_client_init(&cfg);
        if (!h) {
            snprintf(downloadError, sizeof(downloadError), "Not enough memory for HTTPS update request.");
            return nullptr;
        }
        esp_http_client_set_header(h, "User-Agent", "MechTouch/" FLUIDTOUCH_VERSION);
        esp_http_client_set_header(h, "Accept-Encoding", "identity");
        esp_err_t result = esp_http_client_open(h, 0);
        if (result != ESP_OK) {
            int tlsError = 0, tlsFlags = 0;
            esp_http_client_get_and_clear_last_tls_error(h, &tlsError, &tlsFlags);
            Serial.printf("[OTA-HTTP] hop=%d url_bytes=%u open=%s tls=0x%x flags=0x%x heap=%u\n",
                          i, static_cast<unsigned>(target.length()), esp_err_to_name(result),
                          tlsError, tlsFlags, ESP.getFreeHeap());
            snprintf(downloadError, sizeof(downloadError), "HTTPS request failed: %s. See Serial Monitor.", esp_err_to_name(result));
            esp_http_client_cleanup(h); return nullptr;
        }
        length = esp_http_client_fetch_headers(h);
        int status = esp_http_client_get_status_code(h);
        Serial.printf("[OTA-HTTP] hop=%d url_bytes=%u status=%d length=%d redirect_bytes=%u\n",
                      i, static_cast<unsigned>(target.length()), status, length,
                      static_cast<unsigned>(location.length()));
        // No event callback may retain the stack-local redirect destination.
        esp_http_client_set_user_data(h, nullptr);
        if (status == 200 && length > 0) return h;
        esp_http_client_cleanup(h);
        if (status < 300 || status >= 400 || location.isEmpty()) {
            snprintf(downloadError, sizeof(downloadError), "Update server: HTTP %d, length %d. See Serial Monitor.", status, length);
            return nullptr;
        }
        target = location;
    }
    return nullptr;
}
bool clockReady() {
    if (time(nullptr) > 1700000000) return true;
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
    for (int i = 0; i < 100; ++i) {
        if (time(nullptr) > 1700000000) return true;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return false;
}
void checkWorker(void *) {
    Serial.println(releaseIdentity);
    if (!clockReady()) { fail("Cannot sync clock. Check internet access."); vTaskDelete(nullptr); return; }
    int length = 0;
    auto h = openDownload(String(base) + "latest/download/ota-" + board + ".json", length);
    if (!h) { fail(downloadError); vTaskDelete(nullptr); return; }
    char data[2049]; int got = 0;
    if (length <= 2048) {
        while (got < length) {
            int n = esp_http_client_read(h, data + got, length - got);
            if (n <= 0) break;
            got += n;
        }
    }
    esp_http_client_cleanup(h);
    JsonDocument doc;
    if (got != length || got == 0 || deserializeJson(doc, data, got)) { fail("Invalid release manifest."); }
    else {
        version = doc["version"].as<String>(); url = doc["url"].as<String>(); digest = doc["sha256"].as<String>();
        firmwareSize = doc["size"] | 0U;
        unsigned parts[3]; bool hashValid = digest.length() == 64;
        for (char c : digest) if (!isxdigit(static_cast<unsigned char>(c))) hashValid = false;
        digest.toLowerCase();
        String expected = String(base) + "download/v" + version + "/mechtouch-" + board + "-" + version + ".bin";
        if (doc["schema"].as<int>() != 1 || doc["board"].as<String>() != board ||
            !OTAVersion::parse(version.c_str(), parts) || !hashValid || !firmwareSize || url != expected)
            fail("Release metadata does not match this panel.");
        else state.store(OTAVersion::newer(version.c_str(), FLUIDTOUCH_VERSION) ? Available : Current);
    }
    vTaskDelete(nullptr);
}
void installWorker(void *) {
    const auto *running = esp_ota_get_running_partition();
    const auto *next = esp_ota_get_next_update_partition(nullptr);
    if (!next || !running || next->address == running->address || firmwareSize > next->size ||
        !esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr)) {
        fail("OTA space unavailable. Install the OTA build once via USB."); vTaskDelete(nullptr); return;
    }
    int length = 0;
    auto h = openDownload(url, length);
    if (!h) { fail(downloadError); vTaskDelete(nullptr); return; }
    if (length != static_cast<int>(firmwareSize)) {
        if (h) esp_http_client_cleanup(h);
        fail("Firmware download failed or size mismatch."); vTaskDelete(nullptr); return;
    }
    esp_ota_handle_t handle = 0;
    if (!permitted.load() || esp_ota_begin(next, OTA_WITH_SEQUENTIAL_WRITES, &handle) != ESP_OK) {
        esp_http_client_cleanup(h); fail("Machine not idle or flash unavailable."); vTaskDelete(nullptr); return;
    }
    mbedtls_sha256_context sha; mbedtls_sha256_init(&sha); mbedtls_sha256_starts(&sha, 0);
    uint8_t buffer[4096]; size_t received = 0; bool ok = true;
    while (received < firmwareSize && ok) {
        int n = esp_http_client_read(h, reinterpret_cast<char *>(buffer),
                                    std::min(sizeof(buffer), firmwareSize - received));
        ok = n > 0 && permitted.load() && (received != 0 || buffer[0] == 0xe9);
        if (ok) ok = esp_ota_write(handle, buffer, n) == ESP_OK && mbedtls_sha256_update(&sha, buffer, n) == 0;
        if (ok) { received += n; progress.store(received * 100 / firmwareSize); }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    esp_http_client_cleanup(h);
    uint8_t hash[32]; mbedtls_sha256_finish(&sha, hash); mbedtls_sha256_free(&sha);
    char hex[65]; for (int i = 0; i < 32; ++i) snprintf(hex + 2*i, 3, "%02x", hash[i]);
    if (!ok || received != firmwareSize || digest != hex || !permitted.load()) {
        esp_ota_abort(handle); fail("Update cancelled: connection, machine state or checksum changed.");
    } else if (esp_ota_end(handle) != ESP_OK) fail("Firmware image validation failed.");
    else if (!permitted.load() || esp_ota_set_boot_partition(next) != ESP_OK) fail("Cannot activate update.");
    else state.store(Success);
    vTaskDelete(nullptr);
}
void launch(TaskFunction_t worker, State nextState) {
    state.store(nextState);
    if (xTaskCreate(worker, "ota", 12288, nullptr, 1, nullptr) != pdPASS) fail("Not enough memory for update.");
}
lv_obj_t *label(lv_obj_t *parent, const char *text, int y) {
    auto *l = lv_label_create(parent); lv_label_set_text(l, text);
    lv_obj_set_pos(l, 8, y); lv_obj_set_width(l, 590);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0); return l;
}
void show() {
    if (overlay) return;
    overlay = lv_obj_create(lv_layer_top()); lv_obj_set_size(overlay, 800, 480);
    lv_obj_set_style_bg_color(overlay, UIAlerts::ACCENT_PRIMARY, 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_90, 0); lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    auto *panel = lv_obj_create(overlay); lv_obj_set_size(panel, 660, 330); lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, UIAlerts::BG_DARK, 0);
    lv_obj_set_style_text_color(panel, UIAlerts::TEXT_LIGHT, 0); lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    label(panel, "Software Update", 5);
    message = label(panel, "Checking GitHub releases...", 55);
    lv_obj_set_style_text_font(message, &lv_font_montserrat_18, 0);
    bar = lv_bar_create(panel); lv_obj_set_size(bar, 590, 18); lv_obj_set_pos(bar, 8, 210);
    lv_obj_set_style_bg_color(bar, UIAlerts::ACCENT_PRIMARY, LV_PART_INDICATOR);
    closeButton = lv_button_create(panel); lv_obj_set_size(closeButton, 170, 45); lv_obj_set_pos(closeButton, 195, 250);
    auto *l = lv_label_create(closeButton); lv_label_set_text(l, "Later / Close"); lv_obj_center(l);
    lv_obj_add_event_cb(closeButton, [](lv_event_t *) {
        if (OTAUpdater::installing()) return;
        lv_obj_delete(overlay); overlay = nullptr;
    }, LV_EVENT_CLICKED, nullptr);
    install = lv_button_create(panel); lv_obj_set_size(install, 220, 45); lv_obj_set_pos(install, 380, 250);
    lv_obj_set_style_bg_color(closeButton, UIAlerts::ACCENT_PRIMARY, 0);
    lv_obj_set_style_bg_color(install, UIAlerts::ACCENT_PRIMARY, 0);
    l = lv_label_create(install); lv_label_set_text(l, "Install & Restart"); lv_obj_center(l);
    lv_obj_add_event_cb(install, [](lv_event_t *) {
        if (state.load() == Available && idle()) { permitted.store(true); progress.store(0); launch(installWorker, Downloading); }
    }, LV_EVENT_CLICKED, nullptr);
}
}
void OTAUpdater::check() {
    State s = state.load();
    if (s == Downloading || s == Checking || s == Success) { show(); return; }
    show();
    if (WiFi.status() != WL_CONNECTED) { fail("Connect to WiFi with internet access first."); return; }
    launch(checkWorker, Checking);
}
bool OTAUpdater::installing() { return state.load() == Downloading || state.load() == Success; }
void OTAUpdater::loop() {
    State s = state.load();
    if (s == Downloading || s == Success) PowerManager::onUserActivity();
    if (s == Downloading || s == Available || s == Success) {
        if (millis() - pollAt > 500) { pollAt = millis(); FluidNCClient::requestStatusReport(); }
    }
    // Latch cancellation: a later Idle report cannot resume a cancelled installation.
    if (s == Downloading && !idle()) permitted.store(false);
    if (!automaticChecked && WiFi.status() == WL_CONNECTED && millis() > 15000 && s == Ready) {
        automaticChecked = true; launch(checkWorker, Checking);
    }
    static State previous = Ready;
    if (s == Available && previous != Available && idle()) show();
    if (s != Available || idle()) previous = s;
    if (s == Success && !restartAt) restartAt = millis();
    if (restartAt && millis() - restartAt > 1500 && idle()) ESP.restart();
    if (!overlay) return;
    static uint32_t paintedAt = 0;
    if (millis() - paintedAt < 150) return;
    paintedAt = millis();
    if (s == Available && idle()) lv_obj_remove_state(install, LV_STATE_DISABLED);
    else lv_obj_add_state(install, LV_STATE_DISABLED);
    if (s == Downloading || s == Success) lv_obj_add_state(closeButton, LV_STATE_DISABLED);
    else lv_obj_remove_state(closeButton, LV_STATE_DISABLED);
    switch (s) {
        case Checking: lv_label_set_text(message, "Checking GitHub releases..."); break;
        case Available: lv_label_set_text_fmt(message, "New version available\nCurrent: %s    New: %s\n\nInstall only when the machine is idle.\nThe panel will restart after the update.%s", FLUIDTOUCH_VERSION, version.c_str(), idle() ? "" : "\nWaiting for connected, idle machine..."); break;
        case Current: lv_label_set_text(message, "Your software is up to date."); break;
        case Downloading: lv_label_set_text_fmt(message, "Downloading firmware... %d%%\n\nDo not turn off the panel.\nDownload > Verify > Install > Restart", progress.load()); break;
        case Success: lv_label_set_text(message, "Firmware verified. Restarting when machine is idle..."); break;
        case Failed: lv_label_set_text(message, errorText); break;
        default: break;
    }
    lv_bar_set_value(bar, progress.load(), LV_ANIM_OFF);
}
