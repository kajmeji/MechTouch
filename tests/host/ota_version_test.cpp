#include "network/ota_version.h"
#include <cassert>
#include <initializer_list>
int main() {
    assert(OTAVersion::newer("1.0.6", "1.0.5"));
    assert(OTAVersion::newer("1.0.10", "1.0.9"));
    assert(OTAVersion::newer("2.0.0", "1.99.99"));
    assert(!OTAVersion::newer("1.0.5", "1.0.5"));
    assert(!OTAVersion::newer("1.0.4", "1.0.5"));
    unsigned p[3];
    for (auto invalid : {"", "1", "1.2", "v1.2.3", "1.2.3-rc1", "1.2.3x", "1.2.65536", "999999999999999999999.2.3", "-1.2.3"})
        assert(!OTAVersion::parse(invalid, p));
}
