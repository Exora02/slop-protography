#pragma once

#include <Arduino.h>

#include "store.h"

// Persistent storage tier: microSD on the Sense expansion board (FAT32).
//
// Photos are still captured into the PSRAM arena first (fast, always
// available); when a card is mounted, every stored photo is mirrored to it
// as three files at the card root:
//   /P<id>.JPG   the JPEG
//   /P<id>.DNG   the DNG, written via the streaming dng writer (if raw)
//   /P<id>.JSON  metadata sidecar (dims, mode, seed, ...)
// The gallery serves from RAM when hot and from SD otherwise.

bool     sd_begin();      // mount; returns false gracefully (no card)
bool     sd_ok();
uint64_t sd_free_bytes();
uint16_t sd_count();
const PhotoMeta* sd_meta(uint16_t index);   // newest first, 0..count-1

// Mirror one photo to the card. Best-effort: logs and returns false on
// failure, the photo stays perfectly usable from RAM.
bool     sd_write_photo(const PhotoMeta& m, const uint8_t* jpeg, const uint8_t* raw);
bool     sd_has_photo(uint32_t id);
bool     sd_remove_photo(uint32_t id);
