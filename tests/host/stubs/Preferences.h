#pragma once
#include <cstdint>
#include <map>
#include <string>
inline std::map<std::string, uint32_t> testPrefs;
struct Preferences {
    void begin(const char *, bool) {}
    void end() {}
    uint32_t getUInt(const char *key, uint32_t fallback) { auto i=testPrefs.find(key); return i==testPrefs.end()?fallback:i->second; }
    uint8_t getUChar(const char *key, uint8_t fallback) { return getUInt(key,fallback); }
    bool getBool(const char *key, bool fallback) { return getUInt(key,fallback); }
    void putUInt(const char *key,uint32_t v) { testPrefs[key]=v; }
    void putUChar(const char *key,uint8_t v) { putUInt(key,v); }
    void putBool(const char *key,bool v) { putUInt(key,v); }
};
