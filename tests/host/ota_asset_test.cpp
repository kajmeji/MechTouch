#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
extern "C" {
uint32_t lv_rle_decompress(const uint8_t *, uint32_t, uint8_t *, uint32_t, uint8_t);
void *lv_memcpy(void *dst, const void *src, size_t n) { return std::memcpy(dst, src, n); }
void *lv_memset(void *dst, uint8_t value, size_t n) { return std::memset(dst, value, n); }
}
int main(int argc, char **argv) {
    assert(argc == 3);
    std::ifstream a(argv[1], std::ios::binary), b(argv[2], std::ios::binary);
    assert(a && b);
    std::vector<uint8_t> packed((std::istreambuf_iterator<char>(a)), {});
    std::vector<uint8_t> expected((std::istreambuf_iterator<char>(b)), {}), actual(expected.size());
    assert(lv_rle_decompress(packed.data(), packed.size(), actual.data(), actual.size(), 2) == expected.size());
    assert(actual == expected);
}
