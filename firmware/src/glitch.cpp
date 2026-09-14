#include "glitch.h"

#include "log.h"

static const char* TAG = "glitch";

// xorshift32 — small, fast, good enough chaos for art.
static uint32_t xs32(uint32_t* s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}
static uint32_t rnd(uint32_t* s, uint32_t n) { return n ? xs32(s) % n : 0; }

// Find marker position; returns index of the 0xFF byte or (size_t)-1.
static size_t find_marker(const uint8_t* buf, size_t len, uint8_t marker) {
    for (size_t i = 0; i + 1 < len; i++) {
        if (buf[i] == 0xFF && buf[i + 1] == marker) return i;
    }
    return (size_t)-1;
}

static int cmp_u8(const void* a, const void* b) {
    return (int)*(const uint8_t*)a - (int)*(const uint8_t*)b;
}

uint32_t glitch_jpeg(uint8_t* buf, size_t len, uint32_t seed, uint8_t intensity) {
    if (!buf || len < 64 || intensity == 0) return seed;
    if (seed == 0) seed = (uint32_t)esp_random();
    uint32_t rng = seed ? seed : 0x9E3779B9;

    // Corrupt only the entropy-coded data: after the first SOS segment
    // header, before the trailing EOI.
    size_t sos = find_marker(buf, len, 0xDA);
    if (sos == (size_t)-1 || sos + 4 > len) {
        LOGW(TAG, "no SOS marker, leaving jpeg untouched");
        return seed;
    }
    size_t seg_len = (size_t)buf[sos + 2] << 8 | buf[sos + 3];
    size_t start = sos + 2 + seg_len;
    size_t end = len - 4;   // protect EOI (and one slack byte)
    if (start + 16 >= end) return seed;

    size_t span = end - start;
    int ops = 3 + intensity * 2;

    for (int op = 0; op < ops; op++) {
        size_t pos = start + rnd(&rng, span - 8);
        uint32_t r = xs32(&rng) % 100;
        uint32_t run = 16 + rnd(&rng, span / 8);
        if (pos + run > end) run = end - pos;

        if (r < 30) {
            // scramble: overwrite a run with chaos — hard smears and macroblocking
            for (size_t i = 0; i < run; i++) buf[pos + i] = (uint8_t)xs32(&rng);
        } else if (r < 50) {
            // clone: copy a block from elsewhere — repeats/stutters of detail
            size_t src = start + rnd(&rng, span - run);
            if (src != pos) memmove(buf + pos, buf + src, run);
        } else if (r < 70) {
            // xor: subtle-to-wild color and block corruption
            uint8_t mask = 1u << (1 + rnd(&rng, 7));
            for (size_t i = 0; i < run; i++) buf[pos + i] ^= mask;
        } else if (r < 85) {
            // sort: ascending runs turn image regions into frequency ramps
            qsort(buf + pos, run, 1, cmp_u8);
        } else {
            // bit-rotate: shimmery, "circuit-bent" horizontal banding
            uint8_t shift = 1 + rnd(&rng, 7);
            for (size_t i = 0; i < run; i++) {
                uint8_t b = buf[pos + i];
                buf[pos + i] = (uint8_t)((b << shift) | (b >> (8 - shift)));
            }
        }
    }
    LOGI(TAG, "moshed with seed %u intensity %u", seed, intensity);
    return seed;
}
