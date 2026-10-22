#pragma once
#include <ArduinoJson.h>
#include <esp_heap_caps.h>

// Large file-list documents must not consume the internal heap needed by
// networking and display work. ArduinoJson reports NoMemory on allocation failure.
class PsramJsonAllocator : public ArduinoJson::Allocator {
public:
    void *allocate(size_t size) override {
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    void deallocate(void *ptr) override { heap_caps_free(ptr); }
    void *reallocate(void *ptr, size_t size) override {
        return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
};
