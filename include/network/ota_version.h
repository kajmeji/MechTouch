#pragma once
namespace OTAVersion {
inline bool parse(const char *v, unsigned *parts) {
    if (!v) return false;
    for (int i = 0; i < 3; ++i) {
        if (*v < '0' || *v > '9') return false;
        unsigned n = 0;
        do { n = n * 10 + (*v++ - '0'); if (n > 65535) return false; } while (*v >= '0' && *v <= '9');
        parts[i] = n;
        if (i < 2 && *v++ != '.') return false;
    }
    return *v == 0;
}
inline bool newer(const char *candidate, const char *current) {
    unsigned a[3], b[3];
    if (!parse(candidate, a) || !parse(current, b)) return false;
    for (int i = 0; i < 3; ++i) if (a[i] != b[i]) return a[i] > b[i];
    return false;
}
}
