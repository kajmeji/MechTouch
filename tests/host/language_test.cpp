#include "ui/ui_language.h"
#include "core/display_tuning.h"
#include <Preferences.h>
#include <cassert>
#include <cstring>
#include <iostream>
int main() {
    using namespace UILanguage;
    testPrefs.clear(); init();
    assert(current()==Language::English);
    assert(!strcmp(translate("Save Settings"), "Save Settings"));
    // Saving the preference does not change the running UI before re-init.
    testPrefs["ui_lang"]=1;
    assert(current()==Language::English);
    init(); assert(current()==Language::Turkish);
    assert(!strcmp(translate("Save Settings"), "Ayarları Kaydet"));
    assert(!strcmp(translate("Shift"), "Büyük"));
    const char *command="G91 G0 X10";
    assert(translate(command)==command);
    lv_obj_t label;
    setText(&label,"\xef\x80\x93 Save Settings");
    assert(label.text=="\xef\x80\x93 Ayarları Kaydet");
    unsigned writes=label.writes;
    setText(&label,"\xef\x80\x93 Save Settings"); assert(label.writes==writes);
    setText(&label,"Version: 1.0.5"); assert(label.text=="Sürüm: 1.0.5");
    setText(&label,"• All macros\n• UI preferences");
    assert(label.text=="• Tüm makrolar\n• Arayüz tercihleri");
    setTextFmt(&label,"%s Delete File?","ICON"); assert(label.text=="ICON Dosya Silinsin mi?");
    char text[256];
    format(text,sizeof(text),"Running: %s","English-file.nc");
    assert(!strcmp(text,"Çalışıyor: English-file.nc"));
    format(text,sizeof(text),"%d bytes / %d bytes (%d%%)",100,200,50);
    assert(!strcmp(text,"100 bayt / 200 bayt (50%)"));
    setOptions(&label,"Disabled\n5 min\n10 min\n15 min\n30 min\n60 min\n90 min");
    assert(label.text=="Kapalı\n5 dk\n10 dk\n15 dk\n30 dk\n60 dk\n90 dk");
    setPlaceholder(&label,"Enter command..."); assert(label.text=="Komut girin...");
    addTab(&label,"Settings"); assert(label.text=="Ayarlar");
    char guarded[]={'a','b','c'};
    render("Settings",guarded+1,1);assert(guarded[0]=='a'&&guarded[1]==0&&guarded[2]=='c');
    testPrefs["ui_lang"]=0;init();setText(&label,"Save Settings");assert(label.text=="Save Settings");
    testPrefs["ui_lang"]=255;init();assert(current()==Language::English);
    assert(basicBacklightPWM(0)==0&&basicBacklightPWM(50)==127);
    assert(basicBacklightPWM(100)==191&&basicBacklightPWM(255)==191);
    for(unsigned i=1;i<=100;i++) assert(basicBacklightPWM(i)>=basicBacklightPWM(i-1));
    std::cout << "PASS: language persistence, Turkish UTF-8, icons, multiline, formats, unchanged labels, brightness curve\n";
}
