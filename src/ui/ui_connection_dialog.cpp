#include "ui/ui_connection_dialog.h"
#include <cstring>
extern "C" {
LV_FONT_DECLARE(ft_bold_28);
LV_FONT_DECLARE(ft_bold_20);
}

namespace {
const lv_color_t petrol=lv_color_hex(0x103f38), cream=lv_color_hex(0xf5eee4);
const lv_color_t lime=lv_color_hex(0xd6e980), muted=lv_color_hex(0x6b8078);
const lv_color_t line=lv_color_hex(0xa3b0a7), danger=lv_color_hex(0xaf4c36);
void surface(lv_obj_t *o,lv_color_t color,int border=0) {
    lv_obj_set_style_bg_color(o,color,0);
    lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(o,border,0);
    lv_obj_set_style_border_color(o,line,0);
    lv_obj_set_style_radius(o,border?8:0,0);
    lv_obj_set_style_pad_all(o,0,0);
    lv_obj_set_style_shadow_width(o,0,0);
    lv_obj_set_style_text_color(o,petrol,0);
    lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);
}
lv_obj_t *text(lv_obj_t *p,const char *s,int x,int y,int w,const lv_font_t *font) {
    auto *o=lv_label_create(p);lv_label_set_text(o,s?s:"");
    lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);
    lv_label_set_long_mode(o,LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(o,font,0);
    return o;
}
lv_obj_t *shell(lv_obj_t **card,int h) {
    auto *root=lv_obj_create(lv_layer_top());surface(root,petrol);
    lv_obj_set_pos(root,0,0);lv_obj_set_size(root,800,480);
    auto *brand=text(root,"MechTouch",20,14,300,&ft_bold_28);
    lv_obj_set_style_text_color(brand,cream,0);
    auto *section=text(root,"CONNECTION",500,22,278,&lv_font_montserrat_16);
    lv_obj_set_style_text_color(section,cream,0);
    lv_obj_set_style_text_align(section,LV_TEXT_ALIGN_RIGHT,0);
    *card=lv_obj_create(root);surface(*card,cream,1);
    lv_obj_set_size(*card,704,h);lv_obj_set_pos(*card,48,74);
    return root;
}
void action(lv_obj_t *p,const char *name,int x,lv_event_cb_t cb,bool primary=false) {
    auto *b=lv_button_create(p);surface(b,primary?petrol:cream,1);
    lv_obj_set_size(b,204,52);lv_obj_set_pos(b,x,276);
    lv_obj_set_style_text_color(b,primary?cream:petrol,0);
    lv_obj_set_style_bg_color(b,lime,LV_STATE_PRESSED);
    lv_obj_set_style_text_color(b,petrol,LV_STATE_PRESSED);
    auto *l=lv_label_create(b);lv_label_set_text(l,name);
    lv_obj_set_style_text_font(l,&lv_font_montserrat_18,0);lv_obj_center(l);
    if(cb)lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,nullptr);
}
void step(lv_obj_t *p,const char *name,int x,bool active) {
    auto *b=lv_obj_create(p);surface(b,active?petrol:cream,1);
    lv_obj_set_pos(b,x,216);lv_obj_set_size(b,316,54);
    lv_obj_set_style_text_color(b,active?cream:muted,0);
    auto *l=lv_label_create(b);lv_label_set_text(l,name);
    lv_obj_set_style_text_font(l,&ft_bold_20,0);lv_obj_center(l);
}
}

lv_obj_t *UIConnectionDialog::connecting(const char *machine,const char *ssid) {
    const bool wifi=ssid&&ssid[0];
    lv_obj_t *card;auto *root=shell(&card,322);
    auto *icon=text(card,wifi?LV_SYMBOL_WIFI:LV_SYMBOL_SETTINGS,26,28,44,&lv_font_montserrat_24);
    lv_obj_set_style_text_color(icon,petrol,0);
    text(card,wifi?"Connecting to WiFi...":"Connecting to machine...",84,22,590,&ft_bold_28);
    text(card,machine&&machine[0]?machine:"Selected machine",26,84,650,&ft_bold_28);
    auto *detail=text(card,wifi?ssid:"Waiting for a response from FluidNC.",26,130,650,&lv_font_montserrat_18);
    lv_obj_set_style_text_color(detail,muted,0);
    step(card,"1  WiFi",26,wifi);step(card,"2  Machine",360,!wifi);
    auto *hint=text(root,"Please wait while the connection is established.",48,424,704,&lv_font_montserrat_16);
    lv_obj_set_style_text_color(hint,cream,0);
    lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
    return root;
}

lv_obj_t *UIConnectionDialog::error(const char *title,const char *message,
                                    lv_event_cb_t retry,lv_event_cb_t restart,lv_event_cb_t close) {
    lv_obj_t *card;auto *root=shell(&card,350);
    auto *icon=text(card,LV_SYMBOL_WARNING,26,27,42,&lv_font_montserrat_24);
    lv_obj_set_style_text_color(icon,danger,0);
    auto *heading=text(card,title&&title[0]?title:"Connection Failed",80,22,598,&ft_bold_28);
    lv_obj_set_style_text_color(heading,danger,0);
    text(card,"Check the connection, then try again.",26,65,650,&lv_font_montserrat_18);
    auto *body=lv_obj_create(card);surface(body,cream);
    lv_obj_set_pos(body,26,110);lv_obj_set_size(body,650,145);
    lv_obj_add_flag(body,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(body,LV_DIR_VER);
    lv_obj_clear_flag(body,LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(body,LV_OBJ_FLAG_SCROLL_ELASTIC);
    auto *details=text(body,message,0,0,632,&lv_font_montserrat_16);
    lv_label_set_long_mode(details,LV_LABEL_LONG_WRAP);
    action(card,LV_SYMBOL_REFRESH " Retry",26,retry,true);
    action(card,LV_SYMBOL_POWER " Restart",250,restart);
    action(card,"Close",474,close);
    auto *hint=text(root,"Restart returns to machine selection.",48,447,704,&lv_font_montserrat_16);
    lv_obj_set_style_text_color(hint,cream,0);lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
    return root;
}

lv_obj_t *UIConnectionDialog::restarting() {
    lv_obj_t *card;auto *root=shell(&card,250);
    text(card,"Restarting...",26,35,650,&ft_bold_28);
    text(card,"Returning to machine selection.",26,100,650,&lv_font_montserrat_18);
    return root;
}
