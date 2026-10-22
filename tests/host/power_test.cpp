#include "core/power_manager.h"
#include "core/wake_hold.h"
#include "network/fluidnc_client.h"
#include "ui/upload_manager.h"
#include <Preferences.h>
#include <cassert>
#include <cstdio>
uint32_t testMillis=0;
using PM=PowerManager;
static void touchAt(uint32_t t,bool pressed) { testMillis=t; assert(!PM::handleTouch(pressed)); }
int main() {
    DisplayDriver display;
    testPrefs["pm_norm_bri"]=100;
    PM::init(&display);
    assert(display.brightness==50 && testPrefs["pm_bri_v2"]==1);
    PM::setNormalBrightness(100); PM::saveSettings(); PM::init(&display);
    assert(display.brightness==100); // migration must not run twice
    PM::setNormalBrightness(50);
    PM::setDimTimeout(0); PM::setSleepTimeout(10);
    testMillis=10000; PM::update(STATE_IDLE);
    assert(PM::getCurrentState()==PM::SCREEN_OFF); // dim disabled
    touchAt(10010,true); assert(display.brightness==50);
    touchAt(10040,true); // wake touch consumed through release
    touchAt(10070,false); testMillis=10100; assert(PM::handleTouch(true));
    PM::enterStandby(); assert(display.brightness==0 && display.display.timer.paused);
    for(uint32_t t=10110;t<14000;t+=30) touchAt(t,true); // held on entry cannot wake
    assert(PM::getCurrentState()==PM::STANDBY);
    touchAt(14000,false);
    for(uint32_t t=14010;t<17010;t+=30) touchAt(t,true);
    assert(PM::getCurrentState()==PM::STANDBY);
    touchAt(17010,true);
    assert(PM::getCurrentState()==PM::FULL_BRIGHTNESS && !display.display.timer.paused);
    touchAt(17040,true); touchAt(17070,false);
    assert(PM::handleTouch(true));
    // A live machine alarm must restore the screen even with automatic PM disabled.
    PM::setEnabled(false); PM::enterStandby();
    testStatus={true,STATE_ALARM}; PM::update(STATE_ALARM);
    assert(display.brightness==50 && !display.display.timer.paused);
    PM::enterStandby(); assert(PM::getCurrentState()==PM::FULL_BRIGHTNESS);
    testStatus={false,STATE_DISCONNECTED}; testUploading=true;
    PM::enterStandby(); assert(PM::getCurrentState()==PM::FULL_BRIGHTNESS);
    testUploading=false;
    // Continuous user input must prevent inactivity sleep.
    PM::setEnabled(true);
    for(uint32_t t=20000;t<80000;t+=100) { testMillis=t; PM::handleTouch(true); PM::update(STATE_IDLE); }
    assert(PM::getCurrentState()==PM::FULL_BRIGHTNESS);
    WakeHold hold;
    hold.update(false,0);
    for(uint32_t t=10;t<2900;t+=30) assert(!hold.update(true,t));
    assert(!hold.update(false,2900));
    for(uint32_t t=3000;t<6000;t+=30) assert(!hold.update(true,t));
    assert(hold.update(true,6000));
    // A long scheduling stall must start a fresh 3-second observation window.
    hold.update(false,7000);
    for(uint32_t t=7010;t<9900;t+=30) assert(!hold.update(true,t));
    assert(!hold.update(true,11000));
    // millis() rollover.
    hold.reset(); uint32_t start=UINT32_MAX-1000; hold.update(false,start-1);
    for(uint32_t elapsed=0;elapsed<3000;elapsed+=30) assert(!hold.update(true,start+elapsed));
    assert(hold.update(true,start+3000));
    std::puts("PASS: migration, dim-disabled sleep, hold/release suppression, alarm wake, upload guard, inactivity, interrupted hold, stalls and rollover");
}
