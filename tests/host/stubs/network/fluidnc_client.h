#pragma once
enum { STATE_IDLE, STATE_DISCONNECTED, STATE_RUN, STATE_ALARM };
struct TestStatus { bool is_connected=false; int state=STATE_DISCONNECTED; };
inline TestStatus testStatus;
struct FluidNCClient { static const TestStatus &getStatus() { return testStatus; } };
