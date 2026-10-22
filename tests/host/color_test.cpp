#include <lgfx/v1/misc/colortype.hpp>
#include <cassert>
#include <cstdio>
int main() {
    // Validate the library's actual typed RGB565 conversion against the old
    // byte-swapped transport across the entire 16-bit color space.
    for(unsigned raw=0;raw<=65535;++raw) {
        lgfx::rgb565_t input(static_cast<uint16_t>(raw));
        lgfx::swap565_t transport;
        transport=input;
        assert(transport.raw == static_cast<uint16_t>((raw >> 8) | (raw << 8)));
        assert(transport.R8()==input.R8());
        assert(transport.G8()==input.G8());
        assert(transport.B8()==input.B8());
    }
    std::puts("PASS: all 65,536 RGB565 colors preserve their channels in the typed transport");
}
