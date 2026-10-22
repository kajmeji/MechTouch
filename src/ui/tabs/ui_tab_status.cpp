#include "ui/ui_language.h"
#include "ui/ui_alerts.h"
#include "ui/ui_keyboard.h"
#include "ui/tabs/ui_tab_status.h"
#include "ui/tabs/settings/ui_tab_settings_jog.h"
#include <initializer_list>
#include <cmath>
extern "C" { LV_FONT_DECLARE(ft_bold_28); LV_FONT_DECLARE(ft_bold_20); }
namespace StatusTheme {
using namespace UIAlerts;
static constexpr lv_color_t STATE_IDLE=LV_COLOR_MAKE(0x35,0x6b,0x39);
static constexpr lv_color_t STATE_RUN=LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t STATE_HOLD=LV_COLOR_MAKE(0x10,0x3f,0x38);
static constexpr lv_color_t BTN_ESTOP=LV_COLOR_MAKE(0xaf,0x4c,0x36);
static constexpr lv_color_t UI_SECONDARY=LV_COLOR_MAKE(0x49,0x69,0x5f);
static constexpr lv_color_t AXIS_A=LV_COLOR_MAKE(0x10,0x3f,0x38);
}
#include "ui/ui_common.h"
#include "ui/machine_config.h"
#include "ui/wcs_config.h"
#include "ui/fonts/fontawesome_icons_20.h"
#include "network/fluidnc_client.h"
#include <Arduino.h>
#include <Preferences.h>
#include <cstring>
#include <stdio.h>

namespace {
lv_obj_t *state_indicator=nullptr;
void setPositionTextIfChanged(lv_obj_t *obj, const char *text, lv_obj_t *active) {
    if(obj==active)return; // Live reporting must not replace unfinished input.
    if (strcmp(lv_textarea_get_text(obj), text) != 0) {
        lv_point_t size;lv_text_get_size(&size,text,&ft_bold_28,0,0,1000,LV_TEXT_FLAG_NONE);
        const lv_font_t *font=size.x>160?&ft_bold_20:&ft_bold_28;
        if(lv_obj_get_style_text_font(obj,LV_PART_MAIN)!=font)UILanguage::setFont(obj,font,0);
        lv_textarea_set_text(obj, text);
    }
}
void setBackgroundIfChanged(lv_obj_t *obj, lv_color_t color) {
    if (!lv_color_eq(lv_obj_get_style_bg_color(obj, LV_PART_MAIN), color)) {
        lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN);
    }
}
}

// Static member initialization
lv_obj_t *UITabStatus::lbl_message = nullptr;
lv_obj_t *UITabStatus::lbl_state = nullptr;
lv_obj_t *UITabStatus::btn_pause = nullptr;
lv_obj_t *UITabStatus::lbl_pause = nullptr;
lv_obj_t *UITabStatus::btn_stop = nullptr;
lv_obj_t *UITabStatus::btn_cancel_jog = nullptr;
lv_obj_t *UITabStatus::lbl_wpos_x = nullptr;
lv_obj_t *UITabStatus::lbl_wpos_y = nullptr;
lv_obj_t *UITabStatus::lbl_wpos_z = nullptr;
lv_obj_t *UITabStatus::lbl_wpos_a = nullptr;
lv_obj_t *UITabStatus::ind_limit_x = nullptr;
lv_obj_t *UITabStatus::ind_limit_y = nullptr;
lv_obj_t *UITabStatus::ind_limit_z = nullptr;
lv_obj_t *UITabStatus::ind_limit_a = nullptr;
lv_obj_t *UITabStatus::ind_probe = nullptr;
uint32_t UITabStatus::last_limit_trigger_x_ms = 0;
uint32_t UITabStatus::last_limit_trigger_y_ms = 0;
uint32_t UITabStatus::last_limit_trigger_z_ms = 0;
uint32_t UITabStatus::last_limit_trigger_a_ms = 0;
lv_obj_t *UITabStatus::lbl_mpos_x = nullptr;
lv_obj_t *UITabStatus::lbl_mpos_y = nullptr;
lv_obj_t *UITabStatus::lbl_mpos_z = nullptr;
lv_obj_t *UITabStatus::lbl_mpos_a = nullptr;
lv_obj_t *UITabStatus::keyboard = nullptr;
lv_obj_t *UITabStatus::active_textarea = nullptr;
char UITabStatus::original_value[32] = "";
lv_obj_t *UITabStatus::lbl_feed_value = nullptr;
lv_obj_t *UITabStatus::lbl_feed_override = nullptr;
lv_obj_t *UITabStatus::lbl_feed_units = nullptr;
lv_obj_t *UITabStatus::lbl_rapid_override = nullptr;
lv_obj_t *UITabStatus::lbl_spindle_value = nullptr;
lv_obj_t *UITabStatus::lbl_spindle_override = nullptr;
lv_obj_t *UITabStatus::lbl_spindle_units = nullptr;
lv_obj_t *UITabStatus::btn_modal_wcs = nullptr;
lv_obj_t *UITabStatus::lbl_modal_wcs_value = nullptr;
lv_obj_t *UITabStatus::lbl_modal_plane = nullptr;
lv_obj_t *UITabStatus::lbl_modal_dist = nullptr;
lv_obj_t *UITabStatus::lbl_modal_units = nullptr;
lv_obj_t *UITabStatus::lbl_modal_motion = nullptr;
lv_obj_t *UITabStatus::lbl_modal_feedrate = nullptr;
lv_obj_t *UITabStatus::lbl_modal_spindle = nullptr;
lv_obj_t *UITabStatus::lbl_modal_coolant = nullptr;
int UITabStatus::coolant_base_y = 0;
lv_obj_t *UITabStatus::lbl_modal_tool = nullptr;

// Cached values initialization (for delta checking)
float UITabStatus::last_wpos_x = -9999.0f;
float UITabStatus::last_wpos_y = -9999.0f;
float UITabStatus::last_wpos_z = -9999.0f;
float UITabStatus::last_wpos_a = -9999.0f;
float UITabStatus::last_mpos_x = -9999.0f;
float UITabStatus::last_mpos_y = -9999.0f;
float UITabStatus::last_mpos_z = -9999.0f;
float UITabStatus::last_mpos_a = -9999.0f;
float UITabStatus::last_feed_rate = -1.0f;
float UITabStatus::last_feed_override = -1.0f;
float UITabStatus::last_rapid_override = -1.0f;
float UITabStatus::last_spindle_speed = -1.0f;
float UITabStatus::last_spindle_override = -1.0f;
char UITabStatus::last_state[16] = "";
char UITabStatus::last_modal_wcs[8] = "";
char UITabStatus::last_modal_plane[8] = "";
char UITabStatus::last_modal_dist[8] = "";
char UITabStatus::last_modal_units[8] = "";
char UITabStatus::last_modal_motion[8] = "";
char UITabStatus::last_modal_feedrate[8] = "";
char UITabStatus::last_modal_spindle[8] = "";
char UITabStatus::last_modal_coolant[8] = "";
char UITabStatus::last_modal_tool[8] = "";

// WCS popup static members
lv_obj_t *UITabStatus::wcs_popup = nullptr;
lv_obj_t *UITabStatus::wcs_popup_content = nullptr;
lv_obj_t *UITabStatus::wcs_btn_set = nullptr;
lv_obj_t *UITabStatus::wcs_btn_cancel = nullptr;
lv_obj_t *UITabStatus::wcs_buttons[6] = {nullptr};  // Store button references
int UITabStatus::selected_wcs_index = -1;  // -1 = none selected
int UITabStatus::current_wcs_index = -1;  // -1 = none is current
char UITabStatus::wcs_offsets[10][64] = {{0}};  // G54-G59, G28, G30, G92, TLO

void UITabStatus::create(lv_obj_t *tab) {
    // Fixed 800 x 360 content area; header/footer are owned by UICommon/UITabs.
    lv_obj_set_style_pad_all(tab,10,0);lv_obj_set_style_border_width(tab,0,0);
    lv_obj_set_style_bg_color(tab,StatusTheme::BG_MEDIUM,0);lv_obj_set_style_bg_opa(tab,LV_OPA_COVER,0);
    lv_obj_clear_flag(tab,LV_OBJ_FLAG_SCROLLABLE);
    auto label=[&](const char *text,int x,int y,int w,const lv_font_t *font=&lv_font_montserrat_16){
        auto *l=lv_label_create(tab);UILanguage::setText(l,text);lv_obj_set_pos(l,x,y);lv_obj_set_width(l,w);
        UILanguage::setFont(l,font,0);lv_obj_set_style_text_color(l,StatusTheme::TEXT_LIGHT,0);
        lv_label_set_long_mode(l,LV_LABEL_LONG_DOT);return l;
    };
    auto line=[&](int x,int y,int w,int h){
        auto *o=lv_obj_create(tab);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
        lv_obj_set_style_bg_color(o,StatusTheme::BORDER_MEDIUM,0);lv_obj_set_style_border_width(o,0,0);
        lv_obj_set_style_radius(o,0,0);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);lv_obj_clear_flag(o,LV_OBJ_FLAG_CLICKABLE);
    };
    auto button=[&](const char*text,int x,int width,lv_color_t color,lv_event_cb_t cb){
        auto *b=UIAlerts::button(tab);lv_obj_set_pos(b,x,0);lv_obj_set_size(b,width,50);
        UIAlerts::background(b,color,0);lv_obj_set_style_pad_all(b,0,0);
        auto*l=lv_label_create(b);UILanguage::setText(l,text);UILanguage::setFont(l,&lv_font_montserrat_18,0);lv_obj_center(l);
        lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,nullptr);return b;
    };
    label("STATE",0,0,150);
    state_indicator=lv_obj_create(tab);lv_obj_set_pos(state_indicator,0,29);lv_obj_set_size(state_indicator,16,16);
    lv_obj_set_style_radius(state_indicator,8,0);lv_obj_set_style_border_width(state_indicator,0,0);
    lv_obj_set_style_bg_color(state_indicator,StatusTheme::STATE_ALARM,0);lv_obj_clear_flag(state_indicator,LV_OBJ_FLAG_SCROLLABLE);
    lbl_state=label("OFFLINE",25,22,175,&ft_bold_28);
    btn_pause=button(LV_SYMBOL_PAUSE " Pause",220,180,StatusTheme::STATE_HOLD,onPauseResumeClicked);
    lbl_pause=lv_obj_get_child(btn_pause,0);
    btn_stop=button(LV_SYMBOL_STOP " STOP",410,180,StatusTheme::BTN_ESTOP,onStopClicked);
    btn_cancel_jog=button(LV_SYMBOL_STOP " Cancel Jog",220,370,StatusTheme::UI_WARNING,onCancelJogClicked);
    for(auto *b:{btn_pause,btn_stop,btn_cancel_jog})lv_obj_add_flag(b,LV_OBJ_FLAG_HIDDEN);
    label("WCS",621,5,42);
    btn_modal_wcs=button("---",665,115,StatusTheme::BG_MEDIUM,onWCSButtonClicked);
    lbl_modal_wcs_value=lv_obj_get_child(btn_modal_wcs,0);UILanguage::setFont(lbl_modal_wcs_value,&ft_bold_28,0);
    line(0,60,780,1);line(440,75,1,265);line(613,75,1,265);
    label("PRB",18,74,45);label("WORK POSITION",70,74,178);label("MACHINE POSITION",258,74,180);
    auto indicator=[&](int x,int y){auto*o=lv_obj_create(tab);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,12,12);
        lv_obj_set_style_radius(o,6,0);lv_obj_set_style_bg_color(o,StatusTheme::BG_BUTTON,0);
        lv_obj_set_style_border_color(o,StatusTheme::BORDER_MEDIUM,0);lv_obj_set_style_border_width(o,1,0);
        lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);lv_obj_clear_flag(o,LV_OBJ_FLAG_CLICKABLE);return o;};
    ind_probe=indicator(0,77);
    auto field=[&](int x,int y,int h,const char*id){auto*o=lv_textarea_create(tab);
        lv_textarea_set_one_line(o,true);lv_textarea_set_max_length(o,10);lv_textarea_set_text(o,"----.---");
        lv_obj_set_pos(o,x,y);lv_obj_set_size(o,172,h);lv_obj_set_style_pad_all(o,4,0);
        if(h<44){lv_obj_set_style_pad_top(o,1,0);lv_obj_set_style_pad_bottom(o,1,0);}
        UILanguage::setFont(o,&ft_bold_28,0);lv_obj_set_style_text_color(o,StatusTheme::TEXT_LIGHT,0);
        lv_obj_set_style_bg_color(o,StatusTheme::BG_MEDIUM,0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
        lv_obj_set_style_border_width(o,1,0);lv_obj_set_style_border_color(o,StatusTheme::BORDER_MEDIUM,0);
        lv_obj_set_style_radius(o,5,0);lv_obj_set_style_border_width(o,2,LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(o,StatusTheme::ACCENT_PRIMARY,LV_STATE_FOCUSED);
        lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_user_data(o,(void*)id);
        lv_obj_add_event_cb(o,position_field_event_handler,LV_EVENT_ALL,nullptr);return o;};
    bool a_axis=UICommon::isAAxisEnabled();int rows=a_axis?4:3;int pitch=a_axis?42:54;int height=a_axis?39:46;
    const char*axis[]={"X","Y","Z","A"};const char*work[]={"WX","WY","WZ","WA"};const char*machine[]={"MX","MY","MZ","MA"};
    lv_obj_t **work_fields[]={&lbl_wpos_x,&lbl_wpos_y,&lbl_wpos_z,&lbl_wpos_a};
    lv_obj_t **machine_fields[]={&lbl_mpos_x,&lbl_mpos_y,&lbl_mpos_z,&lbl_mpos_a};
    lv_obj_t **limits[]={&ind_limit_x,&ind_limit_y,&ind_limit_z,&ind_limit_a};
    lbl_wpos_a=lbl_mpos_a=ind_limit_a=nullptr;
    for(int i=0;i<rows;i++){int y=98+i*pitch;
        label(axis[i],22,y+5,38,&ft_bold_28);*limits[i]=indicator(0,y+14);
        *work_fields[i]=field(70,y,height,work[i]);*machine_fields[i]=field(258,y,height,machine[i]);
    }
    label("MESSAGE",0,267,420);
    lbl_message=label("No messages.",0,289,430,&lv_font_montserrat_16);
    lv_obj_set_height(lbl_message,56);lv_label_set_long_mode(lbl_message,LV_LABEL_LONG_WRAP);
    lv_obj_set_style_pad_all(lbl_message,3,0);lv_obj_set_style_text_line_space(lbl_message,0,0);lv_obj_set_style_border_width(lbl_message,1,0);
    lv_obj_set_style_border_color(lbl_message,StatusTheme::BORDER_MEDIUM,0);lv_obj_set_style_radius(lbl_message,5,0);
    label("FEED RATE",455,74,150);
    lbl_feed_value=label("---",455,98,150,&ft_bold_28);
    lbl_feed_units=label("mm/min",455,132,76,&lv_font_montserrat_14);
    lbl_feed_override=label("---%",537,132,67,&lv_font_montserrat_14);
    lv_obj_set_style_text_align(lbl_feed_override,LV_TEXT_ALIGN_RIGHT,0);
    line(455,158,146,1);label("RAPID",455,169,150);
    lbl_rapid_override=label("---%",455,191,146,&ft_bold_28);
    line(455,229,146,1);label("SPINDLE",455,241,150);
    lbl_spindle_value=label("---",455,265,146,&ft_bold_28);
    lbl_spindle_units=label("RPM",455,305,75,&lv_font_montserrat_14);
    lbl_spindle_override=label("---%",537,305,67,&lv_font_montserrat_14);
    lv_obj_set_style_text_align(lbl_spindle_override,LV_TEXT_ALIGN_RIGHT,0);
    label("MODAL",628,74,152);
    const char*modal[]={"PLANE","DIST","UNITS","MOTION","FEED","SPINDLE","COOLANT","TOOL"};
    lv_obj_t**values[]={&lbl_modal_plane,&lbl_modal_dist,&lbl_modal_units,&lbl_modal_motion,&lbl_modal_feedrate,&lbl_modal_spindle,&lbl_modal_coolant,&lbl_modal_tool};
    for(int i=0;i<8;i++){int y=104+i*30;label(modal[i],628,y,90,&lv_font_montserrat_14);
        *values[i]=label("---",720,y-2,60,&lv_font_montserrat_18);lv_obj_set_style_text_align(*values[i],LV_TEXT_ALIGN_RIGHT,0);}
    coolant_base_y=282;
    last_wpos_x=last_wpos_y=last_wpos_z=last_wpos_a=-9999.0f;
    last_mpos_x=last_mpos_y=last_mpos_z=last_mpos_a=-9999.0f;
    last_feed_rate=last_feed_override=last_rapid_override=last_spindle_speed=last_spindle_override=-1;
    last_state[0]=last_modal_wcs[0]=last_modal_plane[0]=last_modal_dist[0]=last_modal_units[0]=last_modal_motion[0]=last_modal_feedrate[0]=last_modal_spindle[0]=last_modal_coolant[0]=last_modal_tool[0]='\0';
}

void UITabStatus::updateMessage(const char *message) {
    if (lbl_message && message) {
        if (strcmp(lv_label_get_text(lbl_message), message) == 0) return;
        lv_point_t text_size;lv_text_get_size(&text_size,message,&lv_font_montserrat_16,0,0,422,LV_TEXT_FLAG_NONE);
        const lv_font_t *message_font=text_size.y>48?&lv_font_montserrat_14:&lv_font_montserrat_16;
        if(lv_obj_get_style_text_font(lbl_message,LV_PART_MAIN)!=message_font)UILanguage::setFont(lbl_message,message_font,0);
        UILanguage::setText(lbl_message, message);
        
        // Color code based on message prefix or keywords
        if (strncmp(message, "ERROR", 5) == 0 || strstr(message, "error") != nullptr) {
            lv_obj_set_style_text_color(lbl_message, StatusTheme::STATE_ALARM, 0);
        } else if (strncmp(message, "WARN", 4) == 0 || strstr(message, "warning") != nullptr) {
            lv_obj_set_style_text_color(lbl_message, StatusTheme::UI_WARNING, 0);
        } else if (strncmp(message, "OK", 2) == 0 || strstr(message, "success") != nullptr) {
            lv_obj_set_style_text_color(lbl_message, StatusTheme::UI_SUCCESS, 0);
        } else {
            lv_obj_set_style_text_color(lbl_message, StatusTheme::UI_INFO, 0);
        }
    }
}

void UITabStatus::updateState(const char *state) {
    if (!lbl_state) return;
    
    // Only update if state changed
    if (strcmp(state, last_state) == 0) {
        return;
    }
    
    UILanguage::setText(lbl_state, state);
    strncpy(last_state, state, sizeof(last_state) - 1);
    last_state[sizeof(last_state) - 1] = '\0';
    
    // Update color based on state
    if (strcmp(state, "IDLE") == 0) {
        lv_obj_set_style_text_color(lbl_state, StatusTheme::STATE_IDLE, 0);
    } else if (strcmp(state, "RUN") == 0 || strcmp(state, "JOG") == 0) {
        lv_obj_set_style_text_color(lbl_state, StatusTheme::STATE_RUN, 0);
    } else if (strcmp(state, "ALARM") == 0 || strcmp(state, "OFFLINE") == 0) {
        lv_obj_set_style_text_color(lbl_state, StatusTheme::STATE_ALARM, 0);
    } else {
        lv_obj_set_style_text_color(lbl_state, StatusTheme::UI_WARNING, 0);
    }
    if(state_indicator) setBackgroundIfChanged(state_indicator,lv_obj_get_style_text_color(lbl_state,LV_PART_MAIN));
}

void UITabStatus::updateWorkPosition(float x, float y, float z, float a) {
    // Only update if values changed (avoid unnecessary redraws)
    if (x == last_wpos_x && y == last_wpos_y && z == last_wpos_z && a == last_wpos_a) {
        return;
    }

    char buf[20];

    if (lbl_wpos_x && x != last_wpos_x) {
        if (x <= -9999.0f) {
            setPositionTextIfChanged(lbl_wpos_x, "----.---", active_textarea);
        } else {
            UILanguage::format(buf, sizeof(buf), "%.3f", x);
            setPositionTextIfChanged(lbl_wpos_x, buf, active_textarea);
        }
        last_wpos_x = x;
    }

    if (lbl_wpos_y && y != last_wpos_y) {
        if (y <= -9999.0f) {
            setPositionTextIfChanged(lbl_wpos_y, "----.---", active_textarea);
        } else {
            UILanguage::format(buf, sizeof(buf), "%.3f", y);
            setPositionTextIfChanged(lbl_wpos_y, buf, active_textarea);
        }
        last_wpos_y = y;
    }

    if (lbl_wpos_z && z != last_wpos_z) {
        if (z <= -9999.0f) {
            setPositionTextIfChanged(lbl_wpos_z, "----.---", active_textarea);
        } else {
            UILanguage::format(buf, sizeof(buf), "%.3f", z);
            setPositionTextIfChanged(lbl_wpos_z, buf, active_textarea);
        }
        last_wpos_z = z;
    }

    // Update A-axis if enabled and value provided
    if (lbl_wpos_a && a != last_wpos_a && (a > -9999.0f || (x <= -9999.0f && y <= -9999.0f && z <= -9999.0f))) {
        if (a <= -9999.0f) {
            setPositionTextIfChanged(lbl_wpos_a, "----.---", active_textarea);
        } else {
            UILanguage::format(buf, sizeof(buf), "%.3f", a);
            setPositionTextIfChanged(lbl_wpos_a, buf, active_textarea);
        }
        last_wpos_a = a;
    }
}

void UITabStatus::updateMachinePosition(float x, float y, float z, float a) {
    // Only update if values changed (avoid unnecessary redraws)
    if (x == last_mpos_x && y == last_mpos_y && z == last_mpos_z && a == last_mpos_a) {
        return;
    }

    char buf[20];

    if (lbl_mpos_x && x != last_mpos_x) {
        if (x <= -9999.0f) {
            setPositionTextIfChanged(lbl_mpos_x, "----.---", active_textarea);
        } else {
            UILanguage::format(buf, sizeof(buf), "%.3f", x);
            setPositionTextIfChanged(lbl_mpos_x, buf, active_textarea);
        }
        last_mpos_x = x;
    }

    if (lbl_mpos_y && y != last_mpos_y) {
        if (y <= -9999.0f) {
            setPositionTextIfChanged(lbl_mpos_y, "----.---", active_textarea);
        } else {
            UILanguage::format(buf, sizeof(buf), "%.3f", y);
            setPositionTextIfChanged(lbl_mpos_y, buf, active_textarea);
        }
        last_mpos_y = y;
    }

    if (lbl_mpos_z && z != last_mpos_z) {
        if (z <= -9999.0f) {
            setPositionTextIfChanged(lbl_mpos_z, "----.---", active_textarea);
        } else {
            UILanguage::format(buf, sizeof(buf), "%.3f", z);
            setPositionTextIfChanged(lbl_mpos_z, buf, active_textarea);
        }
        last_mpos_z = z;
    }

    // Update A-axis if enabled and value provided
    if (lbl_mpos_a && a != last_mpos_a && (a > -9999.0f || (x <= -9999.0f && y <= -9999.0f && z <= -9999.0f))) {
        if (a <= -9999.0f) {
            setPositionTextIfChanged(lbl_mpos_a, "----.---", active_textarea);
        } else {
            UILanguage::format(buf, sizeof(buf), "%.3f", a);
            setPositionTextIfChanged(lbl_mpos_a, buf, active_textarea);
        }
        last_mpos_a = a;
    }
}

void UITabStatus::updateFeedRate(float rate, float override_pct) {
    // Only update if values changed
    if (rate == last_feed_rate && override_pct == last_feed_override) {
        return;
    }
    
    char buf[32];
    
    if (lbl_feed_value && rate != last_feed_rate) {
        if (rate <= -9999.0f) {
            UILanguage::setText(lbl_feed_value, "---");
        } else {
            UILanguage::format(buf, sizeof(buf), "%.0f", rate);
            UILanguage::setText(lbl_feed_value, buf);
        }
        last_feed_rate = rate;
    }
    
    if (lbl_feed_override && override_pct != last_feed_override) {
        if (override_pct <= -9999.0f) {
            UILanguage::setText(lbl_feed_override, "---%");
        } else {
            UILanguage::format(buf, sizeof(buf), "%.0f%%", override_pct);
            UILanguage::setText(lbl_feed_override, buf);
        }
        last_feed_override = override_pct;
    }
}

void UITabStatus::updateRapidOverride(float override_pct) {
    // Only update if value changed
    if (override_pct == last_rapid_override) {
        return;
    }
    
    if (lbl_rapid_override) {
        char buf[32];
        if (override_pct <= -9999.0f) {
            UILanguage::setText(lbl_rapid_override, "---%");
        } else {
            UILanguage::format(buf, sizeof(buf), "%.0f%%", override_pct);
            UILanguage::setText(lbl_rapid_override, buf);
        }
        last_rapid_override = override_pct;
    }
}

void UITabStatus::updateSpindle(float speed, float override_pct) {
    // Only update if values changed
    if (speed == last_spindle_speed && override_pct == last_spindle_override) {
        return;
    }
    
    char buf[32];
    
    if (lbl_spindle_value && speed != last_spindle_speed) {
        if (speed <= -9999.0f) {
            UILanguage::setText(lbl_spindle_value, "---");
        } else {
            UILanguage::format(buf, sizeof(buf), "%.0f", speed);
            UILanguage::setText(lbl_spindle_value, buf);
        }
        last_spindle_speed = speed;
    }
    
    if (lbl_spindle_override && override_pct != last_spindle_override) {
        if (override_pct <= -9999.0f) {
            UILanguage::setText(lbl_spindle_override, "---%");
        } else {
            UILanguage::format(buf, sizeof(buf), "%.0f%%", override_pct);
            UILanguage::setText(lbl_spindle_override, buf);
        }
        last_spindle_override = override_pct;
    }
}

void UITabStatus::updateModalStates(const char *wcs, const char *plane, const char *dist, 
                                    const char *units, const char *motion, const char *feedrate,
                                    const char *spindle, const char *coolant, const char *tool) {
    if (lbl_modal_wcs_value && strcmp(wcs, last_modal_wcs) != 0) {
        UILanguage::setText(lbl_modal_wcs_value, wcs);
        strncpy(last_modal_wcs, wcs, sizeof(last_modal_wcs) - 1);
        last_modal_wcs[sizeof(last_modal_wcs) - 1] = '\0';
    }
    
    if (lbl_modal_plane && strcmp(plane, last_modal_plane) != 0) {
        UILanguage::setText(lbl_modal_plane, plane);
        strncpy(last_modal_plane, plane, sizeof(last_modal_plane) - 1);
        last_modal_plane[sizeof(last_modal_plane) - 1] = '\0';
    }
    
    if (lbl_modal_dist && strcmp(dist, last_modal_dist) != 0) {
        UILanguage::setText(lbl_modal_dist, dist);
        strncpy(last_modal_dist, dist, sizeof(last_modal_dist) - 1);
        last_modal_dist[sizeof(last_modal_dist) - 1] = '\0';
    }
    
    if (lbl_modal_units && strcmp(units, last_modal_units) != 0) {
        UILanguage::setText(lbl_modal_units, units);
        strncpy(last_modal_units, units, sizeof(last_modal_units) - 1);
        last_modal_units[sizeof(last_modal_units) - 1] = '\0';
    }
    
    if (lbl_modal_motion && strcmp(motion, last_modal_motion) != 0) {
        UILanguage::setText(lbl_modal_motion, motion);
        strncpy(last_modal_motion, motion, sizeof(last_modal_motion) - 1);
        last_modal_motion[sizeof(last_modal_motion) - 1] = '\0';
    }
    
    if (lbl_modal_feedrate && strcmp(feedrate, last_modal_feedrate) != 0) {
        UILanguage::setText(lbl_modal_feedrate, feedrate);
        strncpy(last_modal_feedrate, feedrate, sizeof(last_modal_feedrate) - 1);
        last_modal_feedrate[sizeof(last_modal_feedrate) - 1] = '\0';
    }
    
    if (lbl_modal_spindle && strcmp(spindle, last_modal_spindle) != 0) {
        UILanguage::setText(lbl_modal_spindle, spindle);
        strncpy(last_modal_spindle, spindle, sizeof(last_modal_spindle) - 1);
        last_modal_spindle[sizeof(last_modal_spindle) - 1] = '\0';
    }
    
    if (lbl_modal_coolant && strcmp(coolant, last_modal_coolant) != 0) {
        bool bothActive = (strcmp(coolant, "M7 M8") == 0);
        UILanguage::setText(lbl_modal_coolant, coolant);
        // Use smaller font and slight y offset when both M7 and M8 are active
        UILanguage::setFont(lbl_modal_coolant,
            bothActive ? &lv_font_montserrat_16 : &lv_font_montserrat_20, 0);
        lv_obj_set_y(lbl_modal_coolant, coolant_base_y + (bothActive ? 2 : 0));
        strncpy(last_modal_coolant, coolant, sizeof(last_modal_coolant) - 1);
        last_modal_coolant[sizeof(last_modal_coolant) - 1] = '\0';
    }
    
    if (lbl_modal_tool && strcmp(tool, last_modal_tool) != 0) {
        UILanguage::setText(lbl_modal_tool, tool);
        strncpy(last_modal_tool, tool, sizeof(last_modal_tool) - 1);
        last_modal_tool[sizeof(last_modal_tool) - 1] = '\0';
    }
}

void UITabStatus::updateControlButtons(int machine_state) {
    if (!btn_pause || !btn_stop || !btn_cancel_jog) return;
    
    if (machine_state == STATE_JOG) {
        // Jogging - show cancel jog button, hide pause/stop
        lv_obj_clear_flag(btn_cancel_jog, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_stop, LV_OBJ_FLAG_HIDDEN);
    } else if (machine_state == STATE_RUN) {
        // Running job - show pause/stop buttons, hide cancel jog
        lv_obj_clear_flag(btn_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btn_stop, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_cancel_jog, LV_OBJ_FLAG_HIDDEN);
        
        // Update pause button appearance (same logic as Actions tab)
        if (lbl_pause) {
            setBackgroundIfChanged(btn_pause, StatusTheme::STATE_HOLD);
            if (strcmp(lv_label_get_text(lbl_pause), LV_SYMBOL_PAUSE " Pause") != 0) {
                UILanguage::setText(lbl_pause, LV_SYMBOL_PAUSE " Pause");
            }
        }
    } else if (machine_state == STATE_HOLD || machine_state == STATE_DOOR) {
        // Paused or Door Hold - show pause (as resume) and stop buttons
        lv_obj_clear_flag(btn_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btn_stop, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_cancel_jog, LV_OBJ_FLAG_HIDDEN);
        
        // Update pause button to show resume
        if (lbl_pause) {
            setBackgroundIfChanged(btn_pause, StatusTheme::BTN_PLAY);
            if (strcmp(lv_label_get_text(lbl_pause), LV_SYMBOL_PLAY " Resume") != 0) {
                UILanguage::setText(lbl_pause, LV_SYMBOL_PLAY " Resume");
            }
        }
    } else {
        // All other states - hide all control buttons
        lv_obj_add_flag(btn_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_stop, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_cancel_jog, LV_OBJ_FLAG_HIDDEN);
    }
}

void UITabStatus::updateLimitSwitches(bool x, bool y, bool z, bool a) {
    uint32_t now = millis();

    if (x) last_limit_trigger_x_ms = now;
    if (y) last_limit_trigger_y_ms = now;
    if (z) last_limit_trigger_z_ms = now;
    if (a) last_limit_trigger_a_ms = now;

    bool vis_x = x || (now - last_limit_trigger_x_ms < LIMIT_SWITCH_HOLD_MS);
    bool vis_y = y || (now - last_limit_trigger_y_ms < LIMIT_SWITCH_HOLD_MS);
    bool vis_z = z || (now - last_limit_trigger_z_ms < LIMIT_SWITCH_HOLD_MS);
    bool vis_a = a || (now - last_limit_trigger_a_ms < LIMIT_SWITCH_HOLD_MS);

    auto setIndicator = [](lv_obj_t *ind, bool triggered) {
        if (!ind) return;
        setBackgroundIfChanged(ind, triggered ? StatusTheme::BTN_PLAY : StatusTheme::BG_BUTTON);
    };
    setIndicator(ind_limit_x, vis_x);
    setIndicator(ind_limit_y, vis_y);
    setIndicator(ind_limit_z, vis_z);
    setIndicator(ind_limit_a, vis_a);
}

void UITabStatus::updateProbe(bool triggered) {
    if (!ind_probe) return;
    setBackgroundIfChanged(ind_probe, triggered ? StatusTheme::BTN_PLAY : StatusTheme::BG_BUTTON);
}

void UITabStatus::onPauseResumeClicked(lv_event_t *e) {
    if (!FluidNCClient::isConnected()) return;
    
    FluidNCStatus status = FluidNCClient::getStatus();
    
    if (status.state == STATE_HOLD || status.state == STATE_DOOR) {
        // Machine is paused or door hold - send Resume command
        Serial.println("[Status Tab] Sending Resume command (~)");
        FluidNCClient::sendCommand("~");
    } else {
        // Machine is running - send Pause command
        Serial.println("[Status Tab] Sending Pause command (!)");
        FluidNCClient::sendCommand("!");
    }
}

void UITabStatus::onStopClicked(lv_event_t *e) {
    if (!FluidNCClient::isConnected()) return;
    
    Serial.println("[Status Tab] STOP - Sending Door Hold (0x84)");
    char door_hold_cmd[2] = {0x84, 0x00};
    FluidNCClient::sendCommand(door_hold_cmd);
}

void UITabStatus::onCancelJogClicked(lv_event_t *e) {
    if (!FluidNCClient::isConnected()) return;
    
    Serial.println("[Status Tab] Cancelling jog (0x85)");
    char cancel_cmd[2] = {0x85, 0x00};
    FluidNCClient::sendCommand(cancel_cmd);
}

// Event handler for position field focus (show keyboard)
void UITabStatus::position_field_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *textarea = (lv_obj_t*)lv_event_get_target(e);
    
    if (code == LV_EVENT_FOCUSED || (code == LV_EVENT_CLICKED &&
        (!keyboard || lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN)))) {
        // Only allow position editing when machine is IDLE
        const FluidNCStatus& status = FluidNCClient::getStatus();
        if (status.state != STATE_IDLE || !FluidNCClient::isConnected()) {
            // Clear focus state from the textarea
            lv_obj_clear_state(textarea, LV_STATE_FOCUSED);
            return;
        }
        
        if(active_textarea && active_textarea!=textarea){
            lv_textarea_set_text(active_textarea,original_value);
            invalidatePositionCache();
        }
        active_textarea = textarea;
        
        // Store original value for cancel restoration
        strncpy(original_value, lv_textarea_get_text(textarea), sizeof(original_value) - 1);
        original_value[sizeof(original_value) - 1] = '\0';
        
        // Same numeric keyboard as other input pages; preserve this page's
        // right-side placement and existing position-command confirmation.
        if (!keyboard) {
            keyboard = UIKeyboard::create(lv_scr_act());
            lv_obj_set_align(keyboard, LV_ALIGN_TOP_LEFT);
            lv_obj_set_size(keyboard, 325, 340);
            lv_obj_set_pos(keyboard, 465, 70);
            UILanguage::setFont(keyboard, &lv_font_montserrat_16, LV_PART_ITEMS);
            lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
            lv_obj_add_event_cb(keyboard, keyboard_event_handler, LV_EVENT_READY, nullptr);
            lv_obj_add_event_cb(keyboard, keyboard_event_handler, LV_EVENT_CANCEL, nullptr);
        }
        lv_keyboard_set_textarea(keyboard, textarea);
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        
        Serial.printf("Position field focused: %s\n", (const char*)lv_obj_get_user_data(textarea));
    }
    
    if (code == LV_EVENT_DEFOCUSED) {
        // Textarea lost focus - check if we're switching to another textarea or clicking elsewhere
        if (keyboard && !lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_t *textarea = (lv_obj_t*)lv_event_get_target(e);
            
            // Check if one of the other textareas is getting focus
            // If so, don't close keyboard - let the FOCUSED event handler deal with it
            bool switching_fields = (lv_obj_has_state(lbl_wpos_x, LV_STATE_FOCUSED) ||
                                    lv_obj_has_state(lbl_wpos_y, LV_STATE_FOCUSED) ||
                                    lv_obj_has_state(lbl_wpos_z, LV_STATE_FOCUSED) ||
                                    (lbl_wpos_a && lv_obj_has_state(lbl_wpos_a, LV_STATE_FOCUSED)) ||
                                    lv_obj_has_state(lbl_mpos_x, LV_STATE_FOCUSED) ||
                                    lv_obj_has_state(lbl_mpos_y, LV_STATE_FOCUSED) ||
                                    lv_obj_has_state(lbl_mpos_z, LV_STATE_FOCUSED) ||
                                    (lbl_mpos_a && lv_obj_has_state(lbl_mpos_a, LV_STATE_FOCUSED)));
            
            // Only close keyboard if we're not switching to another position field
            if (!switching_fields) {
                if (active_textarea) {
                    lv_textarea_set_text(active_textarea, original_value);
                }
                lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
                active_textarea = nullptr;
            invalidatePositionCache();
                Serial.println("Keyboard closed by defocus");
            }
        }
    }
}

// Show validation error dialog
void UITabStatus::showValidationError(const char* message) {
    auto *backdrop=lv_obj_create(lv_layer_top());
    lv_obj_set_size(backdrop,800,480);lv_obj_set_pos(backdrop,0,0);
    UIAlerts::background(backdrop,lv_color_hex(0x103f38),0);
    lv_obj_set_style_bg_opa(backdrop,LV_OPA_COVER,0);
    lv_obj_set_style_pad_all(backdrop,0,0);lv_obj_set_style_border_width(backdrop,0,0);
    lv_obj_set_style_radius(backdrop,0,0);lv_obj_clear_flag(backdrop,LV_OBJ_FLAG_SCROLLABLE);
    auto *dialog=lv_obj_create(backdrop);lv_obj_set_size(dialog,560,250);lv_obj_center(dialog);
    UIAlerts::background(dialog,UIAlerts::BG_DARK,0);lv_obj_set_style_border_color(dialog,UIAlerts::STATE_ALARM,0);
    lv_obj_set_style_border_width(dialog,2,0);lv_obj_set_style_pad_all(dialog,22,0);
    lv_obj_clear_flag(dialog,LV_OBJ_FLAG_SCROLLABLE);
    auto *title=lv_label_create(dialog);UILanguage::setText(title,LV_SYMBOL_WARNING " Check value");
    UILanguage::setFont(title,&lv_font_montserrat_24,0);lv_obj_set_style_text_color(title,UIAlerts::STATE_ALARM,0);
    auto *msg=lv_label_create(dialog);UILanguage::setText(msg,message);lv_obj_set_pos(msg,0,48);
    lv_obj_set_size(msg,510,85);lv_label_set_long_mode(msg,LV_LABEL_LONG_WRAP);
    UILanguage::setFont(msg,&lv_font_montserrat_18,0);lv_obj_set_style_text_color(msg,UIAlerts::TEXT_LIGHT,0);
    auto *btn=UIAlerts::button(dialog);lv_obj_set_size(btn,180,48);lv_obj_align(btn,LV_ALIGN_BOTTOM_RIGHT,0,0);
    UIAlerts::background(btn,UIAlerts::BTN_CONNECT,0);
    auto *label=lv_label_create(btn);UILanguage::setText(label,"OK");lv_obj_center(label);
    lv_obj_add_event_cb(btn,[](lv_event_t *e){lv_obj_delete((lv_obj_t*)lv_event_get_user_data(e));},LV_EVENT_CLICKED,backdrop);
}

// Event handler for keyboard ready/cancel (send jog command or close)
void UITabStatus::keyboard_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
    
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        const char* action = code == LV_EVENT_READY ? "ok" :
                             code == LV_EVENT_CANCEL ? "cancel" : (const char*)lv_obj_get_user_data(btn);
        
        if (strcmp(action, "ok") == 0) {
            // The machine can change state while the numeric keyboard is open.
            if(active_textarea && (!FluidNCClient::isConnected() || FluidNCClient::getStatus().state!=STATE_IDLE)){
                showValidationError("Position entry requires a connected, IDLE machine.");
                return;
            }
            // User pressed OK - validate and send jog command
            if (active_textarea) {
                const char *axis_id = (const char*)lv_obj_get_user_data(active_textarea);
                const char *value_str = lv_textarea_get_text(active_textarea);
                
                // Validate input is a number
                char *endptr;
                float value = strtof(value_str, &endptr);
                if (endptr == value_str || *endptr != '\0') {
                    // Invalid number
                    showValidationError("Invalid number entered");
                    return;
                }
                
                char axis = axis_id[1];  // X, Y, Z, or A

                // Note: Position bounds validation would require checking if machine is homed
                // and querying $/axes/{axis}/max_travel_mm. For simplicity, we rely on
                // FluidNC's soft limits to reject invalid moves and provide feedback.

                char command[64];

                // Get appropriate feed rate based on axis
                int feed_rate;
                if (axis == 'Z') {
                    feed_rate = UITabSettingsJog::getDefaultZFeed();
                } else if (axis == 'A') {
                    feed_rate = UITabSettingsJog::getDefaultAFeed();
                } else {
                    feed_rate = UITabSettingsJog::getDefaultXYFeed();
                }
                
                // Determine if work or machine coordinates and which axis
                if (axis_id[0] == 'W') {
                    // Work coordinates - use $J jogging command with work coordinate system
                    UILanguage::format(command, sizeof(command), "$J=%c%.3f F%d\n", axis, value, feed_rate);
                    Serial.printf("Jogging to work position: %s", command);
                } else if (axis_id[0] == 'M') {
                    // Machine coordinates - use $J with G53 (move in machine coordinate system)
                    UILanguage::format(command, sizeof(command), "$J=G53 %c%.3f F%d\n", axis, value, feed_rate);
                    Serial.printf("Jogging to machine position: %s", command);
                }
                
                FluidNCClient::sendCommand(command);
            }
            
            // Hide keyboard
            if (keyboard) {
                lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            }
            active_textarea = nullptr;
            invalidatePositionCache();
            
        } else if (strcmp(action, "cancel") == 0) {
            // User cancelled - restore original value and refresh from FluidNC
            if (active_textarea) {
                lv_textarea_set_text(active_textarea, original_value);
                
                // Send status query to FluidNC to refresh current position
                FluidNCClient::sendCommand("?");
                Serial.println("Position edit cancelled, restored original value and requested status update");
            }
            
            if (keyboard) {
                lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            }
            active_textarea = nullptr;
            invalidatePositionCache();
        }
    }
}

// WCS Button Click Handler
void UITabStatus::onWCSButtonClicked(lv_event_t *e) {
    Serial.println("WCS button clicked, requesting coordinate offsets");
    
    // Set callback to capture the $# response
    FluidNCClient::setMessageCallback([](const char* message) {
        parseWCSOffsetsResponse(message);
    });
    
    // Request coordinate offsets from FluidNC
    // Popup will be shown automatically when G59 is parsed
    FluidNCClient::sendCommand("$#\n");
}

// Parse WCS offsets response from $# command
void UITabStatus::parseWCSOffsetsResponse(const char* response) {
    // FluidNC sends each coordinate system as a separate message
    // Example: [G54:90.000,0.000,0.000]
    
    // Map of WCS names to array indices
    const char* wcs_names[] = {"G54", "G55", "G56", "G57", "G58", "G59", "G28", "G30", "G92", "TLO"};
    
    // Check if this message contains a WCS offset
    if (response[0] != '[') return;
    
    // Find which WCS this is
    for (int i = 0; i < 10; i++) {
        char search[8];
        UILanguage::format(search, sizeof(search), "[%s:", wcs_names[i]);
        
        if (strncmp(response, search, strlen(search)) == 0) {
            // Found matching WCS - extract coordinates
            const char* start = response + strlen(search);
            const char* end = strchr(start, ']');
            
            if (end) {
                int len = end - start;
                if (len < 64) {
                    strncpy(wcs_offsets[i], start, len);
                    wcs_offsets[i][len] = '\0';
                    Serial.printf("Parsed %s: %s\n", wcs_names[i], wcs_offsets[i]);
                    
                    // Show popup after parsing G59 (last coordinate system we display)
                    if (i == 5) {  // G59 is at index 5
                        Serial.println("G59 parsed, showing WCS popup");
                        showWCSPopup();
                    }
                }
            }
            break;
        }
    }
}

// Show WCS Selection Popup
void UITabStatus::showWCSPopup() {
    if (wcs_popup) {
        lv_obj_del(wcs_popup);
    }
    
    // Create modal backdrop
    wcs_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(wcs_popup, 800, 480);
    lv_obj_set_pos(wcs_popup, 0, 0);
    UIAlerts::background(wcs_popup, lv_color_hex(0x103f38), 0);
    lv_obj_set_style_pad_all(wcs_popup, 0, 0);
    lv_obj_set_style_radius(wcs_popup, 0, 0);
    lv_obj_set_pos(wcs_popup, 0, 0);
    lv_obj_set_style_bg_opa(wcs_popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wcs_popup, 0, 0);
    lv_obj_clear_flag(wcs_popup, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create content panel
    wcs_popup_content = lv_obj_create(wcs_popup);
    lv_obj_set_size(wcs_popup_content, 700, 400);
    lv_obj_center(wcs_popup_content);
    UIAlerts::background(wcs_popup_content, UIAlerts::BG_MEDIUM, 0);
    lv_obj_set_style_border_color(wcs_popup_content, UIAlerts::ACCENT_SECONDARY, 0);
    lv_obj_set_style_border_width(wcs_popup_content, 3, 0);
    lv_obj_set_style_pad_all(wcs_popup_content, 15, 0);
    lv_obj_clear_flag(wcs_popup_content, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    lv_obj_t *title = lv_label_create(wcs_popup_content);
    UILanguage::setText(title, "SELECT WORK COORDINATE SYSTEM");
    UILanguage::setFont(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, UIAlerts::TEXT_DISABLED, 0);
    lv_obj_set_pos(title, 0, 0);
    
    // Create WCS buttons in 2 columns (no scrolling)
    const char* wcs_names[] = {"G54", "G55", "G56", "G57", "G58", "G59"};
    current_wcs_index = -1;  // Reset current WCS tracking
    
    // Load WCS configurations from Preferences
    char wcs_custom_names[6][32] = {{0}};  // Custom names for G54-G59
    bool wcs_locked[6] = {false};          // Lock status for G54-G59
    
    int machine_index = MachineConfigManager::getSelectedMachineIndex();
    if (machine_index >= 0) {
        WCSConfig::loadWCSConfig(machine_index, wcs_custom_names, wcs_locked);
    }
    
    for (int i = 0; i < 6; i++) {
        // Calculate position: column 0 (left) for G54-G56, column 1 (right) for G57-G59
        int col = i / 3;  // 0 for indices 0-2, 1 for indices 3-5
        int row = i % 3;  // Row within column (0, 1, 2)
        int x = col * 335;  // Left column at x=0, right column at x=335
        int y = 40 + (row * 85);  // Start at y=40, space buttons 85px apart
        
        lv_obj_t *btn = UIAlerts::button(wcs_popup_content);
        lv_obj_set_size(btn, 325, 75);
        lv_obj_set_pos(btn, x, y);
        UIAlerts::background(btn, UIAlerts::BG_BUTTON, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn, 8, LV_PART_MAIN);
        
        // Add transparent border by default to prevent layout shift when selected
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, UIAlerts::BG_BUTTON, LV_PART_MAIN);  // Match background
        
        // Check if this is the current WCS
        bool is_current = (strcmp(wcs_names[i], lv_label_get_text(lbl_modal_wcs_value)) == 0);
        if (is_current) {
            current_wcs_index = i;  // Store which button is current
            lv_obj_set_style_border_color(btn, UIAlerts::ACCENT_SECONDARY, LV_PART_MAIN);
        }
        
        // WCS code label (large, yellow)
        lv_obj_t *lbl_wcs_code = lv_label_create(btn);
        UILanguage::setText(lbl_wcs_code, wcs_names[i]);
        UILanguage::setFont(lbl_wcs_code, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(lbl_wcs_code, UIAlerts::POS_MODAL, 0);
        lv_obj_align(lbl_wcs_code, LV_ALIGN_TOP_LEFT, 0, 0);
        
        // Custom name label (if exists) - orange color, to the right of WCS code
        if (wcs_custom_names[i][0] != '\0') {
            lv_obj_t *lbl_custom = lv_label_create(btn);
            lv_label_set_text(lbl_custom, wcs_custom_names[i]);
            UILanguage::setFont(lbl_custom, &lv_font_montserrat_20, 0);
            lv_obj_set_style_text_color(lbl_custom, UIAlerts::POS_WORK, 0);
            lv_label_set_long_mode(lbl_custom, LV_LABEL_LONG_DOT);
            lv_obj_set_width(lbl_custom, wcs_locked[i] ? 215 : 245);  // Leave room for lock icon if present
            lv_obj_align(lbl_custom, LV_ALIGN_TOP_LEFT, 70, 3);  // To the right of WCS code, slight vertical adjustment
        }
        
        // Lock icon (bottom right) if locked
        if (wcs_locked[i]) {
            lv_obj_t *lbl_lock = lv_label_create(btn);
            UILanguage::setText(lbl_lock, FA_ICON_LOCK);
            UILanguage::setFont(lbl_lock, &fontawesome_icons_20, 0);
            lv_obj_set_style_text_color(lbl_lock, UIAlerts::ACCENT_SECONDARY, 0);
            lv_obj_align(lbl_lock, LV_ALIGN_BOTTOM_RIGHT, 0, -18);
        }
        
        // Coordinates label with axis colors (axis labels AND values colored)
        lv_obj_t *lbl_coords = lv_label_create(btn);
        char text[120];
        char* coords = wcs_offsets[i];
        
        // Parse the three coordinate values
        float x_val = 0, y_val = 0, z_val = 0;
        sscanf(coords, "%f,%f,%f", &x_val, &y_val, &z_val);
        
        // Format with axis color codes (LVGL recolor feature)
        // Extract RGB components from theme colors (convert from RGB565 to RGB888)
        uint32_t x_color = lv_color_to_u32(UIAlerts::AXIS_X);
        uint32_t y_color = lv_color_to_u32(UIAlerts::AXIS_Y);
        uint32_t z_color = lv_color_to_u32(UIAlerts::AXIS_Z);
        
        UILanguage::format(text, sizeof(text), 
                 "#%06X X: %.3f#  #%06X Y: %.3f#  #%06X Z: %.3f#",
                 x_color & 0xFFFFFF, x_val,
                 y_color & 0xFFFFFF, y_val,
                 z_color & 0xFFFFFF, z_val);
        
        UILanguage::setText(lbl_coords, text);
        UILanguage::setFont(lbl_coords, &lv_font_montserrat_16, 0);
        lv_label_set_recolor(lbl_coords, true);
        lv_obj_align(lbl_coords, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        
        // Store button reference for highlighting
        wcs_buttons[i] = btn;
        
        // Store WCS index in user data
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, onWCSSelected, LV_EVENT_CLICKED, nullptr);
    }
    
    // Set button (left side)
    wcs_btn_set = UIAlerts::button(wcs_popup_content);
    lv_obj_set_size(wcs_btn_set, 150, 50);
    lv_obj_set_pos(wcs_btn_set, 175, 305);
    UIAlerts::background(wcs_btn_set, UIAlerts::BTN_PLAY, LV_PART_MAIN);
    lv_obj_add_flag(wcs_btn_set, LV_OBJ_FLAG_HIDDEN);  // Hidden until WCS selected
    
    lv_obj_t *lbl_set = lv_label_create(wcs_btn_set);
    UILanguage::setText(lbl_set, "Set");
    UILanguage::setFont(lbl_set, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_set);
    
    lv_obj_add_event_cb(wcs_btn_set, onWCSSetClicked, LV_EVENT_CLICKED, nullptr);
    
    // Cancel button (right side)
    wcs_btn_cancel = UIAlerts::button(wcs_popup_content);
    lv_obj_set_size(wcs_btn_cancel, 150, 50);
    lv_obj_set_pos(wcs_btn_cancel, 375, 305);
    UIAlerts::background(wcs_btn_cancel, UIAlerts::BG_BUTTON, LV_PART_MAIN);
    
    lv_obj_t *lbl_cancel = lv_label_create(wcs_btn_cancel);
    UILanguage::setText(lbl_cancel, "Cancel");
    UILanguage::setFont(lbl_cancel, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_cancel);
    
    lv_obj_add_event_cb(wcs_btn_cancel, onWCSCancelClicked, LV_EVENT_CLICKED, nullptr);
    
    // Initialize selection state
    selected_wcs_index = -1;
}

// Hide WCS Popup
void UITabStatus::hideWCSPopup() {
    if (wcs_popup) {
        lv_obj_del(wcs_popup);
        wcs_popup = nullptr;
        wcs_popup_content = nullptr;
        wcs_btn_set = nullptr;
        wcs_btn_cancel = nullptr;
        for (int i = 0; i < 6; i++) {
            wcs_buttons[i] = nullptr;
        }
        selected_wcs_index = -1;
        current_wcs_index = -1;
    }
    
    // Clear message callback
    FluidNCClient::clearMessageCallback();
}

// WCS Selected Handler (highlights selection, doesn't execute)
void UITabStatus::onWCSSelected(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(btn);
    
    Serial.printf("WCS button clicked: index %d\n", index);
    
    // Clear previous selection highlighting
    if (selected_wcs_index >= 0 && selected_wcs_index < 6) {
        // If previous selection was the current WCS, restore its 2px teal border
        if (selected_wcs_index == current_wcs_index) {
            lv_obj_set_style_border_color(wcs_buttons[selected_wcs_index], UIAlerts::ACCENT_SECONDARY, LV_PART_MAIN);
            lv_obj_set_style_border_width(wcs_buttons[selected_wcs_index], 2, LV_PART_MAIN);
        } else {
            // Otherwise, restore transparent 2px border (matches background)
            lv_obj_set_style_border_color(wcs_buttons[selected_wcs_index], UIAlerts::BG_BUTTON, LV_PART_MAIN);
            lv_obj_set_style_border_width(wcs_buttons[selected_wcs_index], 2, LV_PART_MAIN);
        }
    }
    
    // Highlight new selection with blue 3px border
    selected_wcs_index = index;
    lv_obj_set_style_border_color(btn, UIAlerts::ACCENT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 3, LV_PART_MAIN);
    
    // Show Set button
    if (wcs_btn_set) {
        lv_obj_clear_flag(wcs_btn_set, LV_OBJ_FLAG_HIDDEN);
    }
}

// WCS Set Button Handler (executes the command)
void UITabStatus::onWCSSetClicked(lv_event_t *e) {
    if (selected_wcs_index < 0 || selected_wcs_index >= 6) {
        Serial.println("No WCS selected");
        return;
    }
    
    const char* wcs_names[] = {"G54", "G55", "G56", "G57", "G58", "G59"};
    const char* wcs = wcs_names[selected_wcs_index];
    
    Serial.printf("Setting WCS to: %s\n", wcs);
    
    // Send command to FluidNC to switch WCS
    char command[8];
    UILanguage::format(command, sizeof(command), "%s\n", wcs);
    FluidNCClient::sendCommand(command);
    
    // Close popup
    hideWCSPopup();
}

// WCS Cancel Button Handler
void UITabStatus::onWCSCancelClicked(lv_event_t *e) {
    hideWCSPopup();
}

void UITabStatus::hideKeyboard() {
    if(!active_textarea && (!keyboard || lv_obj_has_flag(keyboard,LV_OBJ_FLAG_HIDDEN)))return;
    if(active_textarea){lv_textarea_set_text(active_textarea,original_value);lv_obj_remove_state(active_textarea,LV_STATE_FOCUSED);}
    if(keyboard)lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);
    active_textarea=nullptr;
    invalidatePositionCache();
}

void UITabStatus::invalidatePositionCache() {
    // NaN forces the next report to repaint even OFFLINE's -9999 sentinel.
    last_wpos_x=last_wpos_y=last_wpos_z=last_wpos_a=NAN;
    last_mpos_x=last_mpos_y=last_mpos_z=last_mpos_a=NAN;
}
