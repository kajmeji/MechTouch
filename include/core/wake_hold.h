#pragma once
#include <cstdint>

// Requires a fresh, continuous press. A stalled input loop is not evidence
// that the user kept holding the screen throughout that interval.
class WakeHold {
public:
    void reset() { releaseRequired = true; tracking = false; }
    bool update(bool pressed, uint32_t now) {
        if (!pressed) {
            releaseRequired = false;
            tracking = false;
            return false;
        }
        if (releaseRequired) return false;
        if (!tracking || uint32_t(now - lastSample) > 150) {
            started = now;
            tracking = true;
        }
        lastSample = now;
        if (uint32_t(now - started) < 3000) return false;
        reset();
        return true;
    }
private:
    bool releaseRequired = true;
    bool tracking = false;
    uint32_t started = 0;
    uint32_t lastSample = 0;
};
