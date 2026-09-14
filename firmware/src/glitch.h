#pragma once

#include <Arduino.h>

// JPEG byte-glitch engine ("datamosh for stills").
//
// Corrupts bytes inside the entropy-coded scan of a JPEG. Decoders resync on
// the damaged Huffman stream, which produces the classic smear/tear/bleed
// artifacts without needing a JPEG decoder or encoder on the device.
// Deterministic for a given (seed, intensity) so shots are reproducible.

// Applies `intensity` (1..10) seeded corruption passes to buf.
// Only touches bytes after the first scan header (SOS) and keeps the EOI
// marker intact so the file stays decodable.
// Returns the seed actually used (helpful when the caller passed 0 = random).
uint32_t glitch_jpeg(uint8_t* buf, size_t len, uint32_t seed, uint8_t intensity);
