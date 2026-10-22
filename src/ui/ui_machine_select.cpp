#include "ui/ui_keyboard.h"
#include "ui/ui_machine_select.h"
#include "ui/ui_common.h"
#include "ui/ui_tabs.h"
#include "ui/ui_theme.h"
#include "core/power_manager.h"
#include "config.h"
#include <Preferences.h>
#include <WiFi.h>
extern "C" {
LV_FONT_DECLARE(ft_bold_28);
LV_FONT_DECLARE(ft_bold_20);
}


namespace {
const lv_color_t PETROL=lv_color_hex(0x103f38), CREAM=lv_color_hex(0xf5eee4);
const lv_color_t LIME=lv_color_hex(0xd6e980), LINE=lv_color_hex(0x9aa9a2);
const lv_color_t DANGER=lv_color_hex(0xaf4c36);
lv_obj_t *setup_title=nullptr, *setup_count=nullptr, *form_actions=nullptr, *form_error=nullptr;
lv_timer_t *initial_add_timer=nullptr;
String scanned_ssids[15];
int scanned_count=0;
void panel(lv_obj_t *o, lv_color_t bg=CREAM, int border=0) {
    lv_obj_set_style_bg_color(o,bg,0); lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(o,border,0); lv_obj_set_style_border_color(o,LINE,0);
    lv_obj_set_style_radius(o,border?5:0,0); lv_obj_set_style_shadow_width(o,0,0);
    lv_obj_set_style_pad_all(o,0,0); lv_obj_set_style_text_color(o,PETROL,0);
    lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);
}
lv_obj_t *label(lv_obj_t *p,const char *t,int x,int y,int w,const lv_font_t *f=&lv_font_montserrat_18) {
    auto *o=lv_label_create(p);lv_label_set_text(o,t);lv_obj_set_pos(o,x,y);
    lv_obj_set_width(o,w);lv_label_set_long_mode(o,LV_LABEL_LONG_DOT);
    if(f==&lv_font_montserrat_32)f=&ft_bold_28;
    else if(f==&lv_font_montserrat_22)f=&ft_bold_20;
    lv_obj_set_style_text_font(o,f,0);return o;
}
lv_obj_t *button(lv_obj_t *p,const char *t,int x,int y,int w,int h,lv_event_cb_t cb,void *data=nullptr,bool solid=false) {
    auto *o=lv_button_create(p);panel(o,solid?PETROL:CREAM,1);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_text_color(o,solid?CREAM:PETROL,0);
    lv_obj_set_style_bg_color(o,LIME,LV_STATE_PRESSED);lv_obj_set_style_text_color(o,PETROL,LV_STATE_PRESSED);
    lv_obj_set_style_opa(o,LV_OPA_40,LV_STATE_DISABLED);
    auto *l=lv_label_create(o);lv_label_set_text(l,t);lv_obj_set_style_text_font(l,&lv_font_montserrat_18,0);lv_obj_center(l);
    if(cb)lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,data);return o;
}
void header(lv_obj_t *p,const char *section) {
    auto *bar=lv_obj_create(p);panel(bar,PETROL);lv_obj_set_size(bar,800,40);
    lv_obj_set_style_text_color(bar,CREAM,0);
    label(bar,"MechTouch",16,4,290,&ft_bold_28);
    auto *l=label(bar,section,410,12,374,&lv_font_montserrat_16);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_RIGHT,0);
}
void dropdown_style(lv_obj_t *o) {
    panel(o,CREAM,1);lv_obj_set_style_pad_all(o,10,0);
    lv_obj_set_style_text_font(o,&lv_font_montserrat_18,0);
    lv_obj_add_event_cb(o,[](lv_event_t *e){
        auto *list=lv_dropdown_get_list((lv_obj_t*)lv_event_get_target(e));
        if(list){panel(list,CREAM,1);lv_obj_add_flag(list,LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_max_height(list,180,0);lv_obj_set_style_pad_all(list,8,0);
            lv_obj_set_style_bg_color(list,PETROL,LV_PART_SELECTED);
            lv_obj_set_style_text_color(list,CREAM,LV_PART_SELECTED);
            lv_obj_set_style_bg_color(list,PETROL,LV_PART_SELECTED|LV_STATE_CHECKED);
            lv_obj_set_style_text_color(list,CREAM,LV_PART_SELECTED|LV_STATE_CHECKED);}
    },LV_EVENT_READY,nullptr);
}
}

// Static member initialization
lv_obj_t *UIMachineSelect::screen = nullptr;
lv_display_t *UIMachineSelect::display = nullptr;
MachineConfig UIMachineSelect::machines[MAX_MACHINES];
bool UIMachineSelect::edit_mode = false;
lv_obj_t *UIMachineSelect::edit_mode_button = nullptr;
lv_obj_t *UIMachineSelect::list_container = nullptr;  // Machine list container
lv_obj_t *UIMachineSelect::machine_buttons[MAX_MACHINES] = {nullptr};
lv_obj_t *UIMachineSelect::edit_buttons[MAX_MACHINES] = {nullptr};
lv_obj_t *UIMachineSelect::move_up_buttons[MAX_MACHINES] = {nullptr};
lv_obj_t *UIMachineSelect::move_down_buttons[MAX_MACHINES] = {nullptr};
lv_obj_t *UIMachineSelect::delete_buttons[MAX_MACHINES] = {nullptr};
lv_obj_t *UIMachineSelect::add_button = nullptr;
lv_obj_t *UIMachineSelect::config_dialog = nullptr;
lv_obj_t *UIMachineSelect::dialog_content = nullptr;
lv_obj_t *UIMachineSelect::keyboard = nullptr;
int UIMachineSelect::editing_index = -1;
lv_obj_t *UIMachineSelect::ta_name = nullptr;
lv_obj_t *UIMachineSelect::ta_ssid = nullptr;
lv_obj_t *UIMachineSelect::ta_password = nullptr;
lv_obj_t *UIMachineSelect::ta_url = nullptr;
lv_obj_t *UIMachineSelect::ta_port = nullptr;
lv_obj_t *UIMachineSelect::dd_connection_type = nullptr;
lv_obj_t *UIMachineSelect::wifi_scan_dropdown = nullptr;
lv_obj_t *UIMachineSelect::wifi_scan_button = nullptr;
lv_obj_t *UIMachineSelect::wifi_scan_status = nullptr;
lv_timer_t *UIMachineSelect::wifi_scan_timer = nullptr;
lv_obj_t *UIMachineSelect::delete_dialog = nullptr;

int UIMachineSelect::deleting_index = -1;

void UIMachineSelect::show(lv_display_t *disp) {
    display=disp;edit_mode=false;MachineConfigManager::loadMachines(machines);
    screen=lv_obj_create(nullptr);panel(screen);header(screen,"MACHINE SELECTION");
    setup_title=label(screen,"Select Machine",18,56,460,&lv_font_montserrat_32);
    add_button=button(screen,LV_SYMBOL_PLUS " Add",534,54,116,48,onAddMachine);
    edit_mode_button=button(screen,LV_SYMBOL_EDIT " Edit",664,54,118,48,onEditModeToggle,nullptr,true);
    list_container=lv_obj_create(screen);panel(list_container);lv_obj_set_pos(list_container,18,120);lv_obj_set_size(list_container,764,304);
    setup_count=label(screen,"",18,447,450,&lv_font_montserrat_16);
    if(PowerManager::isEnabled())button(screen,LV_SYMBOL_POWER " Power",652,432,130,44,[](lv_event_t*){UICommon::showPowerOffConfirmDialog();});
    refreshMachineList();lv_scr_load(screen);
    if(!getConfiguredMachineCount()){
        initial_add_timer=lv_timer_create([](lv_timer_t *t){
            initial_add_timer=nullptr;if(screen&&!config_dialog)onAddMachine(nullptr);lv_timer_del(t);
        },100,nullptr);
    }
}

void UIMachineSelect::hide() {
    if(initial_add_timer){lv_timer_del(initial_add_timer);initial_add_timer=nullptr;}
    hideConfigDialog();hideDeleteConfirmDialog();
    if(screen){lv_obj_del(screen);screen=nullptr;}
    setup_title=setup_count=nullptr;
}

void UIMachineSelect::refreshMachineList() {
    lv_obj_clean(list_container);int count=getConfiguredMachineCount();
    lv_label_set_text(setup_title,edit_mode?"Manage Machines":"Select Machine");
    lv_label_set_text_fmt(setup_count,"%d saved machine%s",count,count==1?"":"s");
    if(edit_mode&&count<MAX_MACHINES)lv_obj_clear_flag(add_button,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(add_button,LV_OBJ_FLAG_HIDDEN);
    int n=0;
    for(int i=0;i<MAX_MACHINES;i++){
        machine_buttons[i]=edit_buttons[i]=move_up_buttons[i]=move_down_buttons[i]=delete_buttons[i]=nullptr;
        if(!machines[i].is_configured)continue;
        void *id=(void*)(intptr_t)i;char host[145];
        snprintf(host,sizeof(host),"%s:%u",machines[i].fluidnc_url,machines[i].websocket_port);
        if(edit_mode){
            auto *row=lv_obj_create(list_container);panel(row,CREAM,1);lv_obj_set_pos(row,0,n*76);lv_obj_set_size(row,764,70);
            label(row,machines[i].name,12,7,460,&lv_font_montserrat_22);
            label(row,host,12,40,454,&lv_font_montserrat_16);
            move_up_buttons[i]=button(row,LV_SYMBOL_UP,480,8,60,52,onMoveUpMachine,id);
            move_down_buttons[i]=button(row,LV_SYMBOL_DOWN,548,8,60,52,onMoveDownMachine,id);
            edit_buttons[i]=button(row,LV_SYMBOL_EDIT,616,8,60,52,onEditMachine,id);
            delete_buttons[i]=button(row,LV_SYMBOL_TRASH,684,8,60,52,onDeleteMachine,id);
            lv_obj_set_style_text_color(delete_buttons[i],DANGER,0);
            if(!n)lv_obj_add_state(move_up_buttons[i],LV_STATE_DISABLED);
            if(n==count-1)lv_obj_add_state(move_down_buttons[i],LV_STATE_DISABLED);
        }else{
            auto *card=lv_button_create(list_container);machine_buttons[i]=card;
            panel(card,n==0?PETROL:CREAM,1);lv_obj_set_pos(card,(n%2)*390,(n/2)*158);lv_obj_set_size(card,374,146);
            lv_obj_set_style_text_color(card,n==0?CREAM:PETROL,0);lv_obj_set_style_bg_color(card,LIME,LV_STATE_PRESSED);
            lv_obj_set_style_text_color(card,PETROL,LV_STATE_PRESSED);
            lv_obj_add_event_cb(card,onMachineSelected,LV_EVENT_CLICKED,id);
            label(card,machines[i].name,16,16,340,&ft_bold_28);
            char network[48];snprintf(network,sizeof(network),"%s %s",machines[i].connection_type==CONN_WIRELESS?LV_SYMBOL_WIFI:LV_SYMBOL_USB,
                machines[i].connection_type==CONN_WIRELESS?machines[i].ssid:"Wired");
            label(card,network,16,78,286,&lv_font_montserrat_18);
            label(card,host,16,110,300,&lv_font_montserrat_16);
            auto *arrow=label(card,LV_SYMBOL_RIGHT,322,95,34,&lv_font_montserrat_24);lv_obj_set_style_text_color(arrow,n==0?LIME:PETROL,0);
        }n++;
    }
    if(!count)label(list_container,"No saved machines. Tap Edit, then Add.",16,90,720,&lv_font_montserrat_22);
}

// Helper function: Count configured machines
int UIMachineSelect::getConfiguredMachineCount() {
    int count = 0;
    for (int i = 0; i < MAX_MACHINES; i++) {
        if (machines[i].is_configured) {
            count++;
        }
    }
    return count;
}

// Helper function: Swap two machines in the array
void UIMachineSelect::swapMachines(int index1, int index2) {
    if (index1 < 0 || index1 >= MAX_MACHINES || index2 < 0 || index2 >= MAX_MACHINES) {
        Serial.printf("UIMachineSelect::swapMachines: Invalid indices %d, %d\n", index1, index2);
        return;
    }
    
    if (!machines[index1].is_configured || !machines[index2].is_configured) {
        Serial.printf("UIMachineSelect::swapMachines: Cannot swap unconfigured machines\n");
        return;
    }
    
    Serial.printf("UIMachineSelect::swapMachines: Swapping %d <-> %d\n", index1, index2);
    
    // Swap the machines
    MachineConfig temp = machines[index1];
    machines[index1] = machines[index2];
    machines[index2] = temp;
    
    // Save to preferences
    MachineConfigManager::saveMachines(machines);
    
    // Refresh the display
    refreshMachineList();
}

void UIMachineSelect::onMoveUpMachine(lv_event_t *e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    
    // Find previous configured machine
    int prev_index = -1;
    for (int i = index - 1; i >= 0; i--) {
        if (machines[i].is_configured) {
            prev_index = i;
            break;
        }
    }
    
    if (prev_index >= 0) {
        Serial.printf("UIMachineSelect: Move up machine %d\n", index);
        swapMachines(index, prev_index);
    }
}

void UIMachineSelect::onMoveDownMachine(lv_event_t *e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    
    // Find next configured machine
    int next_index = -1;
    for (int i = index + 1; i < MAX_MACHINES; i++) {
        if (machines[i].is_configured) {
            next_index = i;
            break;
        }
    }
    
    if (next_index >= 0) {
        Serial.printf("UIMachineSelect: Move down machine %d\n", index);
        swapMachines(index, next_index);
    }
}

void UIMachineSelect::onEditModeToggle(lv_event_t *) {
    edit_mode=!edit_mode;
    lv_label_set_text(lv_obj_get_child(edit_mode_button,0),edit_mode?LV_SYMBOL_OK " Done":LV_SYMBOL_EDIT " Edit");
    refreshMachineList();
}

void UIMachineSelect::onMachineSelected(lv_event_t *e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    
    Serial.printf("UIMachineSelect: Machine selected: %s (index %d)\n", machines[index].name, index);
    
    // Check if WiFi password is set for wireless machines
    if (machines[index].connection_type == CONN_WIRELESS && strlen(machines[index].password) == 0) {
        Serial.println("UIMachineSelect: WiFi password not set!");
        
        // Create modal backdrop
        lv_obj_t *backdrop = lv_obj_create(lv_layer_top());
        lv_obj_set_size(backdrop, SCREEN_WIDTH, SCREEN_HEIGHT);
        lv_obj_set_style_bg_color(backdrop, PETROL, 0);
        lv_obj_set_style_bg_opa(backdrop, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(backdrop, 0, 0);
        lv_obj_set_style_radius(backdrop, 0, 0);
        lv_obj_set_style_border_width(backdrop, 0, 0);
        lv_obj_clear_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_center(backdrop);
        
        // Show warning dialog (consistent with System Options popup style)
        lv_obj_t *dialog = lv_obj_create(backdrop);
        lv_obj_set_size(dialog, 600, 300);
        lv_obj_center(dialog);
        lv_obj_set_style_bg_color(dialog, CREAM, 0);
        lv_obj_set_style_border_width(dialog, 3, 0);
        lv_obj_set_style_border_color(dialog, DANGER, 0);
        lv_obj_set_style_pad_all(dialog, 20, 0);
        lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
        
        // Title (positioned near top)
        lv_obj_t *title = lv_label_create(dialog);
        lv_label_set_text(title, LV_SYMBOL_WARNING " WiFi Password Required");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(title, DANGER, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
        
        // Message (centered in available space)
        lv_obj_t *message = lv_label_create(dialog);
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "Machine: %s\n\n"
            "This machine requires a WiFi password\n"
            "but none is configured.\n\n"
            "Please edit the machine configuration and set the\n"
            "WiFi password before attempting to connect.",
            machines[index].name);
        lv_label_set_text(message, msg);
        lv_obj_set_style_text_font(message, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(message, PETROL, 0);
        lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(message, 560);
        lv_obj_align(message, LV_ALIGN_TOP_MID, 0, 50);
        
        // Button container (positioned at bottom)
        lv_obj_t *btn_container = lv_obj_create(dialog);
        lv_obj_set_size(btn_container, 560, 60);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);
        lv_obj_set_style_pad_all(btn_container, 0, 0);
        lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // OK button (centered in button container)
        lv_obj_t *btn_ok = lv_button_create(btn_container);
        lv_obj_set_size(btn_ok, 160, 50);
        lv_obj_center(btn_ok);
        lv_obj_set_style_bg_color(btn_ok, PETROL, 0);
        lv_obj_t *lbl_ok = lv_label_create(btn_ok);
        lv_label_set_text(lbl_ok, "OK");
        lv_obj_set_style_text_font(lbl_ok, &lv_font_montserrat_18, 0);
        lv_obj_center(lbl_ok);
        lv_obj_add_event_cb(btn_ok, [](lv_event_t *e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                lv_obj_del((lv_obj_t*)lv_event_get_user_data(e));
            }
        }, LV_EVENT_CLICKED, backdrop);
        
        return;  // Don't proceed with machine selection
    }
    
    // Save selection index to preferences
    MachineConfigManager::setSelectedMachineIndex(index);
    
    // Create main UI (creates and loads new screen first)
    UICommon::createMainUI();
    
    // Now safe to delete the machine selection screen
    hide();
}

void UIMachineSelect::onEditMachine(lv_event_t *e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    Serial.printf("UIMachineSelect: Edit machine %d\n", index);
    showConfigDialog(index);
}

void UIMachineSelect::onAddMachine(lv_event_t *e) {
    // Find first available unconfigured slot
    int index = -1;
    for (int i = 0; i < MAX_MACHINES; i++) {
        if (!machines[i].is_configured) {
            index = i;
            break;
        }
    }
    
    if (index >= 0) {
        Serial.printf("UIMachineSelect: Add machine to slot %d\n", index);
        showConfigDialog(index);
    } else {
        Serial.println("UIMachineSelect: No available slots");
    }
}

void UIMachineSelect::onDeleteMachine(lv_event_t *e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    Serial.printf("UIMachineSelect: Delete machine %d - showing confirmation\n", index);
    showDeleteConfirmDialog(index);
}

void UIMachineSelect::onDeleteConfirm(lv_event_t *e) {
    Serial.printf("UIMachineSelect: Delete confirmed for machine %d\n", deleting_index);
    
    MachineConfigManager::deleteMachine(deleting_index);
    MachineConfigManager::loadMachines(machines);
    
    hideDeleteConfirmDialog();
    refreshMachineList();
}

void UIMachineSelect::onDeleteCancel(lv_event_t *e) {
    Serial.println("UIMachineSelect: Delete cancelled");
    hideDeleteConfirmDialog();
}

void UIMachineSelect::onConfigSave(lv_event_t *e) {
    Serial.println("UIMachineSelect: Save configuration");
    
    // Read values from dialog
    const char *name = lv_textarea_get_text(ta_name);
    const char *ssid = lv_textarea_get_text(ta_ssid);
    const char *password = lv_textarea_get_text(ta_password);
    const char *url = lv_textarea_get_text(ta_url);
    const char *port_str = lv_textarea_get_text(ta_port);
    uint16_t sel = lv_dropdown_get_selected(dd_connection_type);
    
    // Validate
    if (strlen(name) == 0) {
        lv_label_set_text(form_error,"Enter a machine name.");
        Serial.println("UIMachineSelect: Machine name required");
        return;
    }
    
    char *port_end=nullptr;long port_value=strtol(port_str,&port_end,10);
    if(!*url){lv_label_set_text(form_error,"Enter the FluidNC host name or IP address.");return;}
    if(!*port_str||*port_end||port_value<1||port_value>65535){
        lv_label_set_text(form_error,"Port must be between 1 and 65535.");return;
    }
    // Preserve per-machine jog, probe and axis preferences.
    MachineConfig config = machines[editing_index];
    strncpy(config.name, name, sizeof(config.name) - 1);
    config.connection_type = (sel == 0) ? CONN_WIRELESS : CONN_WIRED;
    strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
    strncpy(config.password, password, sizeof(config.password) - 1);
    strncpy(config.fluidnc_url, url, sizeof(config.fluidnc_url) - 1);
    config.websocket_port = atoi(port_str);
    if (config.websocket_port == 0) config.websocket_port = 81;
    config.is_configured = true;
    
    // Save
    if(!MachineConfigManager::saveMachine(editing_index, config)){
        lv_label_set_text(form_error,"Save failed. Please try again.");return;
    }
    MachineConfigManager::loadMachines(machines);
    
    hideConfigDialog();
    refreshMachineList();
}

void UIMachineSelect::onConfigCancel(lv_event_t *e) {
    Serial.println("UIMachineSelect: Cancel configuration");
    hideConfigDialog();
}

void UIMachineSelect::onConnectionTypeChanged(lv_event_t *e) {
    updateConnectionFields();
}

void UIMachineSelect::onWifiScan(lv_event_t *e)
{
    Serial.println("UIMachineSelect: Starting async WiFi scan");

    if (!wifi_scan_dropdown)
        return;

    lv_dropdown_set_options(
        wifi_scan_dropdown,
        "Scanning..."
    );

    lv_obj_add_state(
        wifi_scan_dropdown,
        LV_STATE_DISABLED
    );

    if (wifi_scan_button)
    {
        lv_obj_add_state(
            wifi_scan_button,
            LV_STATE_DISABLED
        );

        lv_obj_t *label =
            lv_obj_get_child(
                wifi_scan_button,
                0
            );

        if (label)
            lv_label_set_text(
                label,
                "Scanning..."
            );
    }

    WiFi.mode(WIFI_STA);

    WiFi.scanDelete();

    // ASYNC WiFi scan
    WiFi.scanNetworks(
        true,   // async
        true    // show hidden
    );

    // Poll result every 250 ms
    if (!wifi_scan_timer)
    {
        wifi_scan_timer =
            lv_timer_create(
                checkWifiScan,
                250,
                nullptr
            );
    }
}

void UIMachineSelect::checkWifiScan(lv_timer_t *timer) {
    if(!config_dialog||!wifi_scan_dropdown){lv_timer_del(timer);wifi_scan_timer=nullptr;WiFi.scanDelete();return;}
    int result=WiFi.scanComplete();if(result==WIFI_SCAN_RUNNING)return;
    scanned_count=0;String options;
    for(int pass=0;pass<result&&scanned_count<15;pass++){
        int best=-1,rssi=-1000;
        for(int i=0;i<result;i++){
            String ssid=WiFi.SSID(i);if(!ssid.length())continue;
            bool duplicate=false;
            for(int j=0;j<scanned_count;j++)if(scanned_ssids[j]==ssid)duplicate=true;
            if(!duplicate&&WiFi.RSSI(i)>rssi){best=i;rssi=WiFi.RSSI(i);}
        }
        if(best<0)break;
        scanned_ssids[scanned_count++]=WiFi.SSID(best);
        if(options.length())options+="\n";
        options+=WiFi.SSID(best);
        options+=(rssi>=-55?"  Excellent":rssi>=-65?"  Good":rssi>=-75?"  Fair":"  Weak");
    }
    if(!scanned_count)options=result==WIFI_SCAN_FAILED?"Scan failed":"No networks found";
    lv_dropdown_set_options(wifi_scan_dropdown,options.c_str());
    if(scanned_count)lv_obj_clear_state(wifi_scan_dropdown,LV_STATE_DISABLED);
    else lv_obj_add_state(wifi_scan_dropdown,LV_STATE_DISABLED);
    lv_obj_clear_state(wifi_scan_button,LV_STATE_DISABLED);
    lv_label_set_text(lv_obj_get_child(wifi_scan_button,0),LV_SYMBOL_REFRESH " Scan");
    WiFi.scanDelete();lv_timer_del(timer);wifi_scan_timer=nullptr;
    if(scanned_count)lv_dropdown_open(wifi_scan_dropdown);
}
void UIMachineSelect::onWifiSelected(lv_event_t *) {
    if(!wifi_scan_dropdown||!ta_ssid)return;
    int selected=lv_dropdown_get_selected(wifi_scan_dropdown);
    if(selected>=0&&selected<scanned_count)lv_textarea_set_text(ta_ssid,scanned_ssids[selected].c_str());
}

void UIMachineSelect::updateConnectionFields() {
    uint16_t sel = lv_dropdown_get_selected(dd_connection_type);
    bool is_wireless = (sel == 0);
    
    // Enable/disable wireless-specific fields
    if (is_wireless) {
        lv_obj_clear_state(ta_ssid, LV_STATE_DISABLED);
        lv_obj_clear_state(ta_password, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(ta_ssid, LV_STATE_DISABLED);
        lv_obj_add_state(ta_password, LV_STATE_DISABLED);
    }
}

void UIMachineSelect::showConfigDialog(int index) {
    if(config_dialog)hideConfigDialog();
    editing_index=index;bool is_new=!machines[index].is_configured;
    config_dialog=lv_obj_create(screen);panel(config_dialog);lv_obj_set_size(config_dialog,800,480);
    header(config_dialog,"MACHINE SETUP");
    label(config_dialog,is_new?"Add Machine":"Edit Machine",18,50,740,&lv_font_montserrat_32);
    dialog_content=lv_obj_create(config_dialog);panel(dialog_content);
    lv_obj_set_pos(dialog_content,18,100);lv_obj_set_size(dialog_content,764,302);
    lv_obj_set_scroll_dir(dialog_content,LV_DIR_VER);
    lv_obj_clear_flag(dialog_content,LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(dialog_content,LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(dialog_content,LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    auto field=[&](const char *name,int x,int y,int width,int maxlen,const char *text){
        label(dialog_content,name,x,y,width,&lv_font_montserrat_16);
        auto *o=lv_textarea_create(dialog_content);panel(o,CREAM,1);
        lv_obj_set_pos(o,x,y+22);lv_obj_set_size(o,width,46);lv_obj_set_style_pad_all(o,10,0);
        lv_obj_set_style_text_font(o,&lv_font_montserrat_18,0);lv_textarea_set_one_line(o,true);
        lv_textarea_set_max_length(o,maxlen);lv_textarea_set_text(o,text);
        lv_obj_set_style_border_color(o,PETROL,LV_STATE_FOCUSED);lv_obj_set_style_border_width(o,2,LV_STATE_FOCUSED);
        lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_add_event_cb(o,onTextareaFocused,LV_EVENT_FOCUSED,nullptr);
        lv_obj_add_event_cb(o,onTextareaFocused,LV_EVENT_CLICKED,nullptr);return o;
    };
    ta_name=field("Name",0,0,372,31,is_new?"":machines[index].name);
    label(dialog_content,"Connection",392,0,372,&lv_font_montserrat_16);
    dd_connection_type=lv_dropdown_create(dialog_content);dropdown_style(dd_connection_type);
    lv_obj_set_pos(dd_connection_type,392,22);lv_obj_set_size(dd_connection_type,372,46);
    lv_dropdown_set_options(dd_connection_type,"Wireless");lv_dropdown_set_selected(dd_connection_type,0);
    lv_dropdown_set_symbol(dd_connection_type,nullptr);lv_obj_clear_flag(dd_connection_type,LV_OBJ_FLAG_CLICKABLE);
    ta_ssid=field("WiFi SSID",0,78,252,32,is_new?"":machines[index].ssid);
    wifi_scan_button=button(dialog_content,LV_SYMBOL_REFRESH " Scan",262,100,110,46,[](lv_event_t *e){hideKeyboard();onWifiScan(e);});
    ta_password=field("Password",392,78,312,63,is_new?"":machines[index].password);
    lv_textarea_set_password_mode(ta_password,true);
    button(dialog_content,LV_SYMBOL_EYE_OPEN,714,100,50,46,[](lv_event_t*){
        lv_textarea_set_password_mode(ta_password,!lv_textarea_get_password_mode(ta_password));
    });
    wifi_scan_dropdown=lv_dropdown_create(dialog_content);dropdown_style(wifi_scan_dropdown);
    lv_obj_set_pos(wifi_scan_dropdown,0,156);lv_obj_set_size(wifi_scan_dropdown,372,42);
    lv_dropdown_set_options(wifi_scan_dropdown,"Press Scan");lv_obj_add_state(wifi_scan_dropdown,LV_STATE_DISABLED);
    lv_obj_add_event_cb(wifi_scan_dropdown,onWifiSelected,LV_EVENT_VALUE_CHANGED,nullptr);
    label(dialog_content,"Select a network or type its SSID.",392,166,370,&lv_font_montserrat_16);
    ta_url=field("FluidNC URL",0,208,372,127,is_new?"fluidnc.local":machines[index].fluidnc_url);
    char port[8];snprintf(port,sizeof(port),"%u",is_new?81:machines[index].websocket_port);
    ta_port=field("Port",392,208,372,5,port);lv_textarea_set_accepted_chars(ta_port,"0123456789");
    form_error=label(config_dialog,"",18,401,764,&lv_font_montserrat_16);lv_obj_set_style_text_color(form_error,DANGER,0);
    form_actions=lv_obj_create(config_dialog);panel(form_actions);lv_obj_set_pos(form_actions,18,426);lv_obj_set_size(form_actions,764,50);
    button(form_actions,"Cancel",0,0,372,50,onConfigCancel);
    button(form_actions,is_new?"Save Machine":"Save Changes",392,0,372,50,onConfigSave,nullptr,true);
    updateConnectionFields();
}

void UIMachineSelect::hideConfigDialog() {
    // Stop UI polling before its target widgets are destroyed.
    if (wifi_scan_timer) {
        lv_timer_del(wifi_scan_timer);
        wifi_scan_timer = nullptr;
    }
    WiFi.scanDelete();
    if (keyboard) {
        lv_keyboard_set_textarea(keyboard, nullptr);
        lv_obj_del(keyboard);
        keyboard = nullptr;
    }
    if (config_dialog) {
        lv_obj_del(config_dialog);
        config_dialog = nullptr;
    }
    dialog_content = nullptr;
    form_actions=form_error=nullptr;
    ta_name = ta_ssid = ta_password = ta_url = ta_port = nullptr;
    dd_connection_type = nullptr;
    wifi_scan_dropdown = wifi_scan_button = wifi_scan_status = nullptr;
    editing_index = -1;
}

void UIMachineSelect::showDeleteConfirmDialog(int index) {
    deleting_index=index;delete_dialog=lv_obj_create(screen);panel(delete_dialog,PETROL);lv_obj_set_size(delete_dialog,800,480);
    lv_obj_set_style_bg_opa(delete_dialog,LV_OPA_COVER,0);
    auto *box=lv_obj_create(delete_dialog);panel(box,CREAM,1);lv_obj_set_size(box,580,260);lv_obj_center(box);
    auto *t=label(box,"Delete Machine?",24,24,530,&ft_bold_28);lv_obj_set_style_text_color(t,DANGER,0);
    label(box,machines[index].name,24,77,530,&lv_font_montserrat_22);
    label(box,"This saved profile will be permanently removed.",24,119,530,&lv_font_montserrat_16);
    button(box,"Cancel",24,182,254,52,onDeleteCancel);
    auto *b=button(box,LV_SYMBOL_TRASH " Delete",300,182,254,52,onDeleteConfirm,nullptr,true);lv_obj_set_style_bg_color(b,DANGER,0);
}

void UIMachineSelect::hideDeleteConfirmDialog() {
    if (delete_dialog) {
        lv_obj_del(delete_dialog);
        delete_dialog = nullptr;
    }
    deleting_index = -1;
}

void UIMachineSelect::onTextareaFocused(lv_event_t *e) {
    lv_obj_t *ta = (lv_obj_t*)lv_event_get_target(e);
    showKeyboard(ta);
}


void UIMachineSelect::showKeyboard(lv_obj_t *ta) {
    if(!config_dialog||!dialog_content||!ta)return;
    if(keyboard&&!lv_obj_has_flag(keyboard,LV_OBJ_FLAG_HIDDEN)&&lv_keyboard_get_textarea(keyboard)==ta)return;
    if(!keyboard){
        keyboard=UIKeyboard::create(config_dialog);
        lv_obj_set_align(keyboard,LV_ALIGN_TOP_LEFT);
        lv_obj_set_size(keyboard,800,220);lv_obj_set_pos(keyboard,0,260);
        lv_obj_add_event_cb(keyboard,[](lv_event_t *){hideKeyboard();},LV_EVENT_READY,nullptr);
        lv_obj_add_event_cb(keyboard,[](lv_event_t *){hideKeyboard();},LV_EVENT_CANCEL,nullptr);
    }
    lv_obj_clear_flag(keyboard,LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_mode(keyboard,ta==ta_port?LV_KEYBOARD_MODE_NUMBER:LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(keyboard,ta);
    lv_obj_add_flag(form_actions,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(form_error,LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(dialog_content,154);lv_obj_add_flag(dialog_content,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_update_layout(dialog_content);
    lv_area_t a,v;lv_obj_get_coords(ta,&a);lv_obj_get_coords(dialog_content,&v);
    int delta=0;if(a.y2>v.y2-6)delta=a.y2-v.y2+6;else if(a.y1<v.y1+24)delta=a.y1-v.y1-24;
    if(delta){int target=lv_obj_get_scroll_y(dialog_content)+delta;lv_obj_scroll_to_y(dialog_content,target<0?0:target,LV_ANIM_OFF);}
}
void UIMachineSelect::hideKeyboard() {
    if(!keyboard||lv_obj_has_flag(keyboard,LV_OBJ_FLAG_HIDDEN))return;
    lv_keyboard_set_textarea(keyboard,nullptr);lv_obj_add_flag(keyboard,LV_OBJ_FLAG_HIDDEN);
    if(dialog_content){
        lv_obj_set_height(dialog_content,302);lv_obj_scroll_to_y(dialog_content,0,LV_ANIM_OFF);
        lv_obj_clear_flag(dialog_content,LV_OBJ_FLAG_SCROLLABLE);
    }
    if(form_actions)lv_obj_clear_flag(form_actions,LV_OBJ_FLAG_HIDDEN);
    if(form_error)lv_obj_clear_flag(form_error,LV_OBJ_FLAG_HIDDEN);
}
