#pragma once

#include <Arduino.h>
#include "store.h"

// Minimal streaming Adobe DNG writer for stored RAW captures.
//
// The esp32-camera driver returns OV5640 raw Bayer data with an
// under-documented packing (10-bit packed, or 10-bit in a 16-bit container,
// depending on driver version). The capture path records the observed
// bytes-per-pixel in PhotoMeta.raw_bpp and this writer adapts:
//   * bpp <= 1.1  -> RAW8, passed through (8-bit CFA)
//   * bpp < 1.9   -> 10-bit packed (5 bytes -> 4 pixels), unpacked to 16-bit
//   * bpp >= 1.9  -> 16-bit container, passed through
//
// The DNG is generated in chunks so a multi-megabyte file streams straight
// out over HTTP without ever being fully assembled in RAM.

// ---- Verify-on-hardware knobs (see docs/EFFECTS.md, "RAW caveats") ----
// Bayer quadrant of the OV5640 as seen by the driver. 0=R 1=G 2=B.
// Default BGGR; if raw files come out with swapped red/blue, try RGGB.
#define DNG_CFA_PATTERN {2, 1, 1, 0}
// White level of a 16-bit sample. The packed-10 path below left-justifies
// (<<6), matching 65472; if the 16-bit-container passthrough turns out to be
// right-justified, use 1023.
#define DNG_RAW16_WHITE_LEVEL 65472
#define DNG_BLACK_LEVEL 64

struct DngCtx {
    const PhotoMeta* meta;
    const uint8_t* raw;     // pinned raw bytes from the store
    size_t   raw_len;
    uint8_t  hdr[512];
    size_t   hdr_sent;
    size_t   payload_len;   // total output bytes after the header
    size_t   sent;          // payload bytes emitted so far
};

// Total size the generated DNG will have (512-byte header area + payload).
size_t dng_total_size(const PhotoMeta* m);

// Begin a streaming conversion. `raw` must stay pinned (store_pin) for the
// lifetime of the context. Returns false if the meta has no raw data.
bool dng_begin(const PhotoMeta* m, const uint8_t* raw, size_t raw_len, DngCtx& ctx);

// Emit the next chunk into buf (up to bufcap). Returns bytes written; 0 when
// the stream is complete.
size_t dng_next(DngCtx& ctx, uint8_t* buf, size_t bufcap);
