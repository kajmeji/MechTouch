#include "ui/ui_power_dialog.h"
extern "C" { LV_FONT_DECLARE(ft_bold_28); }
namespace {
lv_obj_t *root=nullptr;
void surface(lv_obj_t *o,uint32_t color,int border=0){
 lv_obj_set_style_bg_color(o,lv_color_hex(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
 lv_obj_set_style_border_width(o,border,0);lv_obj_set_style_border_color(o,lv_color_hex(0xa3b0a7),0);
 lv_obj_set_style_radius(o,border?8:0,0);lv_obj_set_style_pad_all(o,0,0);
 lv_obj_set_style_shadow_width(o,0,0);lv_obj_set_style_text_color(o,lv_color_hex(0x103f38),0);
 lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);
}
lv_obj_t *label(lv_obj_t *p,const char *s,int x,int y,int w,const lv_font_t *font){
 auto *l=lv_label_create(p);lv_label_set_text(l,s);lv_obj_set_pos(l,x,y);lv_obj_set_width(l,w);
 lv_obj_set_style_text_font(l,font,0);return l;
}
lv_obj_t *create(){
 root=lv_obj_create(lv_layer_top());surface(root,0x103f38);lv_obj_set_size(root,800,480);
 lv_obj_add_event_cb(root,[](lv_event_t *e){if(lv_event_get_target(e)==root)root=nullptr;},LV_EVENT_DELETE,nullptr);
 auto *brand=label(root,"MechTouch",20,14,300,&ft_bold_28);lv_obj_set_style_text_color(brand,lv_color_hex(0xf5eee4),0);
 auto *tag=label(root,"POWER",620,22,160,&lv_font_montserrat_16);lv_obj_set_style_text_color(tag,lv_color_hex(0xf5eee4),0);
 lv_obj_set_style_text_align(tag,LV_TEXT_ALIGN_RIGHT,0);
 auto *card=lv_obj_create(root);surface(card,0xf5eee4,1);lv_obj_set_pos(card,80,102);lv_obj_set_size(card,640,280);
 return card;
}
void button(lv_obj_t *card,const char *name,int x,lv_event_cb_t cb,bool primary){
 auto *b=lv_button_create(card);surface(b,primary?0xaf4c36:0xf5eee4,1);
 lv_obj_set_pos(b,x,196);lv_obj_set_size(b,278,54);
 lv_obj_set_style_text_color(b,lv_color_hex(primary?0xf5eee4:0x103f38),0);
 lv_obj_set_style_bg_color(b,lv_color_hex(0xd6e980),LV_STATE_PRESSED);
 lv_obj_set_style_text_color(b,lv_color_hex(0x103f38),LV_STATE_PRESSED);
 auto *l=lv_label_create(b);lv_label_set_text(l,name);lv_obj_set_style_text_font(l,&lv_font_montserrat_18,0);lv_obj_center(l);
 lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,nullptr);
}
}
void UIPowerDialog::hide(){if(root)lv_obj_delete(root);}
void UIPowerDialog::show(lv_event_cb_t confirm){
 hide();auto *card=create();
 auto *icon=label(card,LV_SYMBOL_POWER,26,29,42,&lv_font_montserrat_24);lv_obj_set_style_text_color(icon,lv_color_hex(0xaf4c36),0);
 label(card,"Power off the controller?",78,24,535,&ft_bold_28);
 label(card,"Press the reset button to wake the controller.",26,101,586,&lv_font_montserrat_18);
 button(card,"Cancel",26,[](lv_event_t*){hide();},false);
 button(card,LV_SYMBOL_POWER " Power Off",334,confirm,true);
}
void UIPowerDialog::showBusy(){
 hide();auto *card=create();
 label(card,"Powering off...",26,30,586,&ft_bold_28);
 label(card,"Please wait.",26,102,586,&lv_font_montserrat_18);
}
