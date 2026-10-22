# Basic panel: wake, display and memory changes

## Wake behavior

Settings > Power > Standby replaces the former automatic Deep Sleep mode. The existing `pm_deepsleep` preference and backup key remain compatible. The power menu's Sleep action uses the same standby state.

- Backlight and LVGL refresh stop; touch polling and the FluidNC connection continue.
- Release any existing touch, then hold continuously for 3 seconds. Releasing early restarts the hold. An input sampling gap above 150 ms also restarts it.
- The wake gesture is consumed until the finger is lifted, preventing accidental control activation.
- Dim/screen-off still wake with one consumed tap. Active machine states, including alarms, restore the screen automatically.
- Standby is refused during an upload or while a connected machine is not idle. Disabling dimming no longer prevents the separate screen-off timeout.

This is software standby, **not CPU deep sleep**. The project's GT911 configuration has `TOUCH_INT = -1`. The old implementation disabled every wake source before `esp_deep_sleep_start()`. ESP32-S3 deep sleep powers off the CPUs and most peripherals; an independent wake source would be needed to use it with this touchscreen. See [Espressif sleep modes](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/sleep_modes.html). Standby retains RAM, the touch controller and networking, so its power consumption is higher. No hardware power measurement has been performed.

## Display

Normal backlight defaults to 50%, including Basic startup. The previous 100% setting migrates once to 50%; later explicit settings, including 100%, are preserved. Brightness can still be adjusted in Settings > Power.

On Basic, the upper half of the slider now uses a compressed PWM curve: 50% gives 127/255 and 100% gives 191/255. The lower half stays unchanged. This reduces the excessive light output reported at maximum brightness without applying an expensive per-pixel color filter. `BASIC_BACKLIGHT_MAX_PWM` in `include/core/display_tuning.h` controls the upper limit. This is an initial software adjustment, not a measured panel calibration.

The supplied Mech Dynamix logo is embedded as a 300 x 47 RGB565 constant (28,200 bytes) and displayed at 600 x 94. It is centered with 100 px horizontal margins and an RGB `#f7f7f7` background matching the JPEG. Startup requires neither an SD card nor a JPEG decoder. Both About QR codes use black modules on white with a quiet zone.

## Language

Settings > General now has an ENG/TR selector. Selection alone changes no UI or saved preference. Save Settings persists `ui_lang` and automatically restarts after 700 ms if the language changed. General defaults select ENG but still require saving. Language is included in settings export/import.

Labels, tabs, dialogs, keyboard actions, placeholders and application-generated status/error messages use a compiled, sorted translation catalog; no runtime JSON parsing or translation heap is needed. Binary lookup returns constant strings, and unchanged labels avoid LVGL writes. Turkish keyboard rows and small fallback glyph sets supply Turkish characters. Machine names, SSIDs, file names, custom WCS names, G-code, and raw controller/terminal output retain their original content.

Translation source: `include/ui/i18n/tr.json`. Run `python scripts/generate_language.py` after edits; the generator verifies printf arguments and emits the catalog in `src/ui/ui_language.cpp`. `scripts/generate_turkish_fallback.py` reproduces the glyph asset with Pillow and Windows Arial; it is not needed for normal firmware builds.

LVGL's native RGB565 is passed through LovyanGFX's explicitly typed RGB565 interface and synchronously copied into the panel framebuffer. The input buffer is not manually byte-swapped. An exhaustive host test verifies channel preservation for all 65,536 values using the installed LovyanGFX color types. This does not validate physical panel calibration, viewing angle or color accuracy; those require checking the actual TN panel.

## Memory and responsiveness

- One 32,000-byte draw buffer replaces two. The RGB framebuffer already receives synchronous copies, so no asynchronous transfer uses the buffer after `flush_ready`.
- File rows are created in pages of 20, including access past the previous first-100-entry cutoff. Persistent path storage drops from 25,600 to 5,120 bytes; metadata is retained for navigation.
- File-list JSON assembly is limited to 64 KiB in PSRAM, with allocation failure and timeout handling. Parsed JSON also uses PSRAM. Oversized responses report an error instead of growing indefinitely. Very large directory metadata and SD enumeration remain proportional to directory size.
- Terminal history is bounded before appending to 8 KiB. Old content is removed in place; full-history substring copies are avoided. Hidden terminal labels do not re-layout on every update. Commands are limited to 255 characters.
- Screenshot transfer sends at most one partial BMP row per loop using a nonblocking socket write. The 768,000-byte snapshot is freed on completion, disconnect, or timeout (3 seconds without progress, 30 seconds total). A 2,400-byte row buffer is retained. HTTP request parsing and the initial framebuffer snapshot still take synchronous time.
- File upload runs in one worker task with an 8 KiB task stack and a 16 KiB PSRAM streaming buffer. A protected mailbox coalesces progress; all LVGL callbacks execute from the main loop. Worker creation failure is reported. Upload reserves SD use until completion and has bounded connection/response waits. The worker does not send motion commands or access LVGL.
- Serial forwarding is limited to 64 bytes per loop; incoming debug messages print at most 160 characters.
- `[MEM]` logs include internal free/minimum/largest block, free PSRAM, LVGL free/largest block/fragmentation and minimum remaining loop stack. `[PERF]` remains available to identify blocking work.

The changes address concrete allocation and scheduling problems. They do not establish that every possible freeze is eliminated.

## Validation

Host commands from the repository root (MinGW g++):

```powershell
g++ -std=c++17 -Wall -Wextra -Werror -I tests/host/stubs -I include tests/host/power_test.cpp src/core/power_manager.cpp -o tests/host/power_test.exe
./tests/host/power_test.exe
g++ -std=c++17 -Wall -Wextra -I .pio/libdeps/elecrow-crowpanel-7-basic/LovyanGFX/src tests/host/color_test.cpp -o tests/host/color_test.exe
./tests/host/color_test.exe
g++ -std=c++17 -Wall -Wextra -Werror -finput-charset=UTF-8 -fexec-charset=UTF-8 -I tests/host/language_stubs -I tests/host/stubs -I include tests/host/language_test.cpp src/ui/ui_language.cpp -o tests/host/language_test.exe
./tests/host/language_test.exe
```

The power test compiles the actual power manager against fake clock, preferences, display and machine interfaces. It covers migration persistence, dim-disabled screen-off, touch suppression, timed holds, release, sampling stalls, millis rollover, upload guard, continuous activity and alarm wake.

The language test compiles the actual language implementation with fake LVGL/Preferences interfaces. It checks saved preference activation, ENG fallback, UTF-8, symbol prefixes, multiline dialogs, printf arguments, placeholders/options, avoiding unchanged label writes and the monotonic backlight curve. These host tests do not validate actual UI layout or glyph rendering on the panel.

Basic firmware build: `pio run -e elecrow-crowpanel-7-basic`.

The build reports LovyanGFX LTO/ODR warnings and a deprecated network API warning in dependencies. These dependency warnings are not resolved by the UI changes.

Pending physical checks:

1. Repeat manual and timed standby/wake 20 times; short taps must not wake standby or operate hidden buttons. Check an alarm also wakes the screen.
2. Compare 50%, 75% and 100% backlight from the normal viewing angle. Check petrol, cream, primary RGB colors and a grayscale ramp at both supported rotations.
3. Browse folders with 0, 20, 21 and more than 100 entries, including long names; test previous/next, folder changes and play/delete paths.
4. Repeat screenshots and uploads while observing touch response and `[MEM]`/`[PERF]`. Disconnect the screenshot client mid-transfer and interrupt upload WiFi; verify memory returns and an upload failure appears.
5. Run for at least an hour with terminal traffic and repeated navigation; look for a falling minimum/largest heap block, LVGL fragmentation growth, or long loop stalls.
6. Select TR without saving: the running UI must remain English. Save, verify automatic restart, inspect all tabs/dialogs and type `ÇĞİÖŞÜçğıöşü`. Repeat TR to ENG. Check that machine/file/SSID names remain unchanged.
7. Check logo placement on startup and scan both About QR codes with a phone.

No firmware upload, panel measurement, network transfer integration test or long-duration device test was performed by the agent.
