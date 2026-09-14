#pragma once

#include <Arduino.h>

// "Circuit-bent" capture: instead of corrupting bytes after the fact, abuse
// the sensor's analog pipeline — manual gain/exposure far outside what auto
// control would choose, corrections (denoise, bad-pixel, lens shading)
// switched off, white balance forced to wrong modes. Everything stays within
// register-safe ranges, so it can't damage anything — it just looks wrong
// in interesting, deterministic, seedable ways.

// Randomise the sensor into a bent state. `seed` 0 = random.
// Returns the seed actually used.
uint32_t bend_apply(uint32_t seed);

// Restore the clean base pipeline (user settings + safe defaults).
void bend_restore();
