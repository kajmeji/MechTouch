#include "ui/ui_language.h"
#include "ui/ui_keyboard.h"
#include "ui/ui_page_style.h"
#include <cstring>
namespace {
const char * const lower[]={
"q","w","e","r","t","y","u","i","o","p",LV_SYMBOL_BACKSPACE,"\n",
"a","s","d","f","g","h","j","k","l",";","'","\n",
"Shift","z","x","c","v","b","n","m",",",".","/","\n",
"Cancel","123",LV_SYMBOL_LEFT,"Space",LV_SYMBOL_RIGHT,"-","_","Enter","Done",""};
const char * const upper[]={
"Q","W","E","R","T","Y","U","I","O","P",LV_SYMBOL_BACKSPACE,"\n",
"A","S","D","F","G","H","J","K","L",";","'","\n",
"Shift","Z","X","C","V","B","N","M",",",".","/","\n",
"Cancel","123",LV_SYMBOL_LEFT,"Space",LV_SYMBOL_RIGHT,"-","_","Enter","Done",""};
const char * const symbols[]={
"1","2","3","4","5","6","7","8","9","0",LV_SYMBOL_BACKSPACE,"\n",
"@","#","$","%","&","*","(",")","[","]","<",">","\n",
"+","=","!","?",";",":","\\","|","{","}","\"","^","~","\n",
"Cancel","ABC","\u0060",",",".","Space","-","_","/","Enter","Done",""};
const char * const numbers[]={
"7","8","9",LV_SYMBOL_BACKSPACE,"Clear","\n",
"4","5","6",LV_SYMBOL_LEFT,LV_SYMBOL_RIGHT,"\n",
"1","2","3","-","+","\n",
"Cancel",".","0","Done",""};
const char * const lower_tr[]={
"q","w","e","r","t","y","u","ı","o","p","ğ","ü",LV_SYMBOL_BACKSPACE,"\n",
"a","s","d","f","g","h","j","k","l","ş","i","\n",
"Shift","z","x","c","v","b","n","m","ö","ç",".","/","\n",
"Cancel","123",LV_SYMBOL_LEFT,"Space",LV_SYMBOL_RIGHT,"-","_","Enter","Done",""};
const char * const upper_tr[]={
"Q","W","E","R","T","Y","U","I","O","P","Ğ","Ü",LV_SYMBOL_BACKSPACE,"\n",
"A","S","D","F","G","H","J","K","L","Ş","İ","\n",
"Shift","Z","X","C","V","B","N","M","Ö","Ç",".","/","\n",
"Cancel","123",LV_SYMBOL_LEFT,"Space",LV_SYMBOL_RIGHT,"-","_","Enter","Done",""};
const char *localized_maps[4][64];
bool keyIs(const char *key, const char *action) {
 return !strcmp(key,UILanguage::translate(action));
}
lv_buttonmatrix_ctrl_t lc[64],uc[64],sc[64],nc[64];
void install(lv_obj_t *kb,lv_keyboard_mode_t mode,const char * const *map,lv_buttonmatrix_ctrl_t *ctrl,unsigned slot){
 for(unsigned i=0;i<64;i++){localized_maps[slot][i]=UILanguage::translate(map[i]);if(!map[i][0])break;}
 unsigned n=0;
 for(unsigned i=0;map[i][0];i++){
  if(!strcmp(map[i],"\n"))continue;
  int width=!strcmp(map[i],"Space")?3:(!strcmp(map[i],"Done")||!strcmp(map[i],"Cancel")?2:1);
  ctrl[n++]=(lv_buttonmatrix_ctrl_t)(width|LV_BUTTONMATRIX_CTRL_CLICK_TRIG|(!strcmp(map[i],"Done")?LV_BUTTONMATRIX_CTRL_CHECKED:0));
 }
 lv_keyboard_set_map(kb,mode,localized_maps[slot],ctrl);
}
void pressed(lv_event_t *e){
 auto *kb=(lv_obj_t*)lv_event_get_target(e);auto *ta=lv_keyboard_get_textarea(kb);
 auto id=lv_buttonmatrix_get_selected_button(kb);if(id==LV_BUTTONMATRIX_BUTTON_NONE)return;
 const char *key=lv_buttonmatrix_get_button_text(kb,id);if(!key)return;
 // Event handlers may delete the keyboard. Do not access it after dispatch.
 if(keyIs(key,"Done")){lv_obj_send_event(kb,LV_EVENT_READY,nullptr);return;}
 if(keyIs(key,"Cancel")){lv_obj_send_event(kb,LV_EVENT_CANCEL,nullptr);return;}
 if(keyIs(key,"Shift")){
  lv_keyboard_set_mode(kb,lv_keyboard_get_mode(kb)==LV_KEYBOARD_MODE_TEXT_UPPER?LV_KEYBOARD_MODE_TEXT_LOWER:LV_KEYBOARD_MODE_TEXT_UPPER);return;
 }
 if(!strcmp(key,"123")){lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_SPECIAL);return;}
 if(!strcmp(key,"ABC")){lv_keyboard_set_mode(kb,LV_KEYBOARD_MODE_TEXT_LOWER);return;}
 if(!ta)return;
 if(keyIs(key,"Enter")){
  if(lv_textarea_get_one_line(ta))lv_obj_send_event(kb,LV_EVENT_READY,nullptr);
  else lv_textarea_add_char(ta,'\n');
  return;
 }
 if(!strcmp(key,LV_SYMBOL_BACKSPACE))lv_textarea_delete_char(ta);
 else if(!strcmp(key,LV_SYMBOL_LEFT))lv_textarea_cursor_left(ta);
 else if(!strcmp(key,LV_SYMBOL_RIGHT))lv_textarea_cursor_right(ta);
 else if(keyIs(key,"Clear"))lv_textarea_set_text(ta,"");
 else lv_textarea_add_text(ta,keyIs(key,"Space")?" ":key);
}
}
lv_obj_t *UIKeyboard::create(lv_obj_t *parent){
 auto *kb=lv_keyboard_create(parent);
 install(kb,LV_KEYBOARD_MODE_TEXT_LOWER,UILanguage::current()==UILanguage::Language::Turkish?lower_tr:lower,lc,0);
 install(kb,LV_KEYBOARD_MODE_TEXT_UPPER,UILanguage::current()==UILanguage::Language::Turkish?upper_tr:upper,uc,1);
 install(kb,LV_KEYBOARD_MODE_SPECIAL,symbols,sc,2);
 install(kb,LV_KEYBOARD_MODE_NUMBER,numbers,nc,3);
 lv_obj_remove_event_cb(kb,lv_keyboard_def_event_cb);
 lv_obj_add_event_cb(kb,pressed,LV_EVENT_VALUE_CHANGED,nullptr);
 lv_obj_set_size(kb,800,220);lv_obj_align(kb,LV_ALIGN_BOTTOM_MID,0,0);
 lv_obj_set_style_bg_color(kb,UIPageStyle::ACCENT_PRIMARY,0);
 lv_obj_set_style_bg_opa(kb,LV_OPA_COVER,0);lv_obj_set_style_border_width(kb,0,0);
 lv_obj_set_style_radius(kb,0,0);lv_obj_set_style_shadow_width(kb,0,0);
 lv_obj_set_style_pad_all(kb,6,0);lv_obj_set_style_pad_row(kb,6,0);lv_obj_set_style_pad_column(kb,6,0);
 lv_obj_set_style_bg_color(kb,UIPageStyle::BG_DARK,LV_PART_ITEMS);
 lv_obj_set_style_bg_opa(kb,LV_OPA_COVER,LV_PART_ITEMS);
 lv_obj_set_style_text_color(kb,UIPageStyle::ACCENT_PRIMARY,LV_PART_ITEMS);
 UILanguage::setFont(kb,&lv_font_montserrat_20,LV_PART_ITEMS);
 lv_obj_set_style_radius(kb,4,LV_PART_ITEMS);lv_obj_set_style_shadow_width(kb,0,LV_PART_ITEMS);
 lv_obj_set_style_bg_color(kb,UIPageStyle::LIME,LV_PART_ITEMS|LV_STATE_CHECKED);
 lv_obj_set_style_bg_color(kb,UIPageStyle::LIME,LV_PART_ITEMS|LV_STATE_PRESSED);
 lv_obj_set_style_text_color(kb,UIPageStyle::ACCENT_PRIMARY,LV_PART_ITEMS|LV_STATE_CHECKED);
 lv_obj_set_style_text_color(kb,UIPageStyle::ACCENT_PRIMARY,LV_PART_ITEMS|LV_STATE_PRESSED);
 lv_keyboard_set_popovers(kb,false);
 lv_obj_clear_flag(kb,LV_OBJ_FLAG_CLICK_FOCUSABLE);
 return kb;
}
