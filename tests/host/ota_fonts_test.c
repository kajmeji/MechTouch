// Exercise the actual pinned LVGL decompressor against original font pixels.
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "src/font/fmt_txt/lv_font_fmt_txt.c"

lv_global_t lv_global;
void *lv_malloc(size_t n) { void *p = malloc(n); assert(p); return p; }
void lv_free(void *p) { free(p); }
void *lv_malloc_zeroed(size_t n) { return calloc(1,n); }
void *lv_memcpy(void *d, const void *s, size_t n) { return memcpy(d,s,n); }
void lv_draw_buf_flush_cache(const lv_draw_buf_t *buf, const lv_area_t *area) { (void)buf; (void)area; }
void *lv_utils_bsearch(const void *key, const void *base, size_t n, size_t size,
                      int (*compare)(const void *, const void *)) { return bsearch(key,base,n,size,compare); }
uint32_t lv_draw_buf_width_to_stride(uint32_t w, lv_color_format_t cf) {
    assert(cf == LV_COLOR_FORMAT_A8); return w;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb"); assert(f);
    unsigned count = 0;
    uint32_t header[3];
    while(fread(header, sizeof(header), 1, f) == 1) {
        unsigned w=header[0], h=header[1], n=header[2];
        uint8_t *in=malloc(n), *out=malloc(w*h), *expected=malloc(w*h);
        assert(in && out && expected);
        assert(fread(in, 1, n, f)==n);
        assert(fread(expected, 1, w*h, f)==w*h);
        decompress(in, out, w, h, 4, true);
        for(unsigned i=0;i<w*h;i++) assert(out[i]==expected[i]);
        free(in); free(out); free(expected); count++;
    }
    assert(feof(f)); fclose(f);
    printf("LVGL decoded %u glyphs with identical pixels\n", count);
    return 0;
}
