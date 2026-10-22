#pragma once
#include <cstdint>

// Reduced upper backlight range for the Basic TN panel. Keep the lower
// half unchanged; reduce excessive light output at the top of the slider.
#ifndef BASIC_BACKLIGHT_MAX_PWM
#define BASIC_BACKLIGHT_MAX_PWM 191
#endif
static_assert(BASIC_BACKLIGHT_MAX_PWM >= 127 && BASIC_BACKLIGHT_MAX_PWM <= 255,
              "Basic backlight maximum must be between 127 and 255");
inline uint8_t basicBacklightPWM(uint8_t percent) {
    if (percent > 100) percent = 100;
    return percent <= 50 ? percent * 255U / 100U
                         : 127U + (percent - 50U) * (BASIC_BACKLIGHT_MAX_PWM - 127U) / 50U;
}
