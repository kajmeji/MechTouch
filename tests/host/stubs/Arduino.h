#pragma once
#include <cstdint>
extern uint32_t testMillis;
inline uint32_t millis() { return testMillis; }
struct TestSerial { template<class... T> void printf(const char *, T...) {} void println(const char *) {} };
inline TestSerial Serial;
