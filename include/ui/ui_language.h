#pragma once
#include <lvgl.h>
#include <cstddef>

namespace UILanguage {
enum class Language : uint8_t { English = 0, Turkish = 1 };
void init();
Language current();
const char *translate(const char *text);
void setText(lv_obj_t *obj, const char *text);
void setTextFmt(lv_obj_t *obj, const char *format, ...);
void setPlaceholder(lv_obj_t *obj, const char *text);
void setOptions(lv_obj_t *obj, const char *text);
lv_obj_t *addTab(lv_obj_t *obj, const char *text);
int format(char *dst, size_t size, const char *format, ...);
const lv_font_t *font(const lv_font_t *base);
void setFont(lv_obj_t *obj, const lv_font_t *base, lv_style_selector_t selector);
void render(const char *text, char *output, size_t size);
}
