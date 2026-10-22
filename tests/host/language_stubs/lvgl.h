#pragma once
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <string>
struct lv_obj_t { std::string text; unsigned writes = 0; };
struct lv_font_t {};
using lv_style_selector_t = uint32_t;
#define LV_FONT_DECLARE(name) extern const lv_font_t name
inline const char *lv_label_get_text(lv_obj_t *o) { return o->text.c_str(); }
inline void lv_label_set_text(lv_obj_t *o, const char *s) { if(s)o->text=s; ++o->writes; }
inline void lv_label_set_text_vfmt(lv_obj_t *o,const char *s,va_list args) {
    char buf[4096]; vsnprintf(buf,sizeof(buf),s,args); lv_label_set_text(o,buf);
}
inline void lv_textarea_set_placeholder_text(lv_obj_t *o,const char *s) { o->text=s; }
inline void lv_dropdown_set_options(lv_obj_t *o,const char *s) { o->text=s; }
inline lv_obj_t *lv_tabview_add_tab(lv_obj_t *o,const char *s) { o->text=s; return o; }
inline void lv_obj_set_style_text_font(lv_obj_t *, const lv_font_t *, lv_style_selector_t) {}
