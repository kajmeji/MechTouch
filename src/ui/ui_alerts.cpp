#include "ui/ui_language.h"
#include "ui/ui_alerts.h"
lv_obj_t *UIAlerts::button(lv_obj_t *parent){
 auto *b=lv_button_create(parent);background(b,BTN_CONNECT,0);return b;
}
void UIAlerts::background(lv_obj_t *o,lv_color_t c,lv_style_selector_t selector){
 lv_obj_set_style_bg_color(o,c,selector);
 if(selector==LV_PART_MAIN){
  lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
  lv_obj_set_style_radius(o,6,0);lv_obj_set_style_shadow_width(o,0,0);
  lv_obj_set_style_border_color(o,BORDER_MEDIUM,0);
  auto n=lv_color_to_u32(c);
  unsigned brightness=(((n>>16)&255)*299+((n>>8)&255)*587+(n&255)*114)/1000;
  lv_obj_set_style_text_color(o,lv_color_hex(brightness<145?0xf5eee4:0x103f38),0);
  if(lv_obj_check_type(o,&lv_button_class)){
   lv_obj_set_style_bg_color(o,lv_color_hex(0xd6e980),LV_STATE_PRESSED);
   lv_obj_set_style_text_color(o,TEXT_LIGHT,LV_STATE_PRESSED);
   lv_obj_set_style_border_width(o,1,0);
  }
 }
}
