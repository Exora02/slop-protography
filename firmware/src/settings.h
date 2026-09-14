#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// Capture modes — shared by firmware, web UI and docs. Keep in sync with
// web/app.js MODES and docs/EFFECTS.md.
enum CapMode : uint8_t {
    MODE_HQ = 0,    // clean high-quality JPEG
    MODE_RAW = 1,   // raw Bayer capture stored as packed bytes + JPEG preview, served as DNG
    MODE_MOSH = 2,  // JPEG byte-glitch (datamosh-style artifacts)
    MODE_BEND = 3,  // circuit-bent sensor abuse before capture (+ optional mosh after)
};

inline const char* cap_mode_name(uint8_t m) {
    switch (m) {
        case MODE_HQ:   return "hq";
        case MODE_RAW:  return "raw";
        case MODE_MOSH: return "mosh";
        case MODE_BEND: return "bend";
        default:        return "?";
    }
}

// Runtime-mutable settings, persisted in NVS.
struct Settings {
    uint8_t  mode;             // default capture mode (CapMode)
    uint8_t  jpeg_quality;     // 8 (best) .. 20 (worst), sensor JPEG quant
    uint8_t  live_framesize;   // index into the live-size table (see camera_ctl)
    uint8_t  live_fps;         // 1..LIVE_MAX_FPS
    uint8_t  glitch_intensity; // 0..10, 0 = clean
    uint32_t glitch_seed;      // 0 = random per shot
    uint32_t bend_seed;        // 0 = random per shot
    bool     hmirror;
    bool     vflip;
    uint16_t sleep_min;        // idle minutes before deep sleep, 0 = never
    char     ap_name[25];      // empty = "Protography-XXXX" from MAC
    char     ap_pass[65];      // empty = open network
};

extern Settings g_settings;

void settings_load();
void settings_save();
void settings_apply_json(JsonVariant obj);   // merge partial settings from JSON
void settings_fill_json(JsonObject out);
