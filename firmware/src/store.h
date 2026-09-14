#pragma once

#include <Arduino.h>

#include "config.h"

// Metadata for one captured photo. Timestamps are milliseconds since boot,
// qualified by a persistent boot counter (no RTC on this build).
struct PhotoMeta {
    uint32_t id;
    uint8_t  mode;         // CapMode
    uint16_t jw, jh;       // JPEG dimensions
    uint32_t jlen;         // JPEG bytes
    uint16_t rw, rh;       // RAW dimensions (0 if no raw)
    uint32_t rlen;         // RAW bytes as captured (packed)
    float    raw_bpp;      // driver bytes-per-pixel for the raw buffer
    uint32_t seed;         // glitch/bend seed used
    uint8_t  intensity;    // glitch intensity used
    uint32_t ts_ms;        // millis() at capture
    uint16_t boot_seq;     // boot counter at capture
};

// PSRAM-backed photo arena with oldest-first eviction.
//
// add() takes ownership of the jpeg/raw buffers (ps_malloc'd) on success;
// on failure the caller keeps ownership and must free them.
bool     store_init();
bool     store_add(PhotoMeta& m, uint8_t* jpeg, uint8_t* raw);
uint16_t store_count();
const PhotoMeta* store_meta(uint16_t index);          // 0..count-1, newest first
const PhotoMeta* store_find(uint32_t id);
const uint8_t*   store_jpeg_ptr(uint32_t id, uint32_t* len);
const uint8_t*   store_raw_ptr(uint32_t id, uint32_t* len);
bool     store_remove(uint32_t id);
void     store_clear();
size_t   store_used();
size_t   store_budget();
uint16_t store_boot_seq();

// Pinning: while a photo is pinned it is never evicted (removal still
// allowed). Used while streaming a DNG/JPEG out so pointers stay valid.
bool     store_pin(uint32_t id);
void     store_unpin(uint32_t id);
