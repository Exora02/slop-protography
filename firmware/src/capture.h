#pragma once

#include <Arduino.h>
#include <esp_camera.h>

#include "settings.h"
#include "store.h"

// Capture orchestration shared by the shutter button, HTTP API and WS
// commands. Runs on the main loop task (single-threaded by design: the
// camera, effects and store are not touched from anywhere else).

struct CaptureRequest {
    CapMode  mode;       // defaults to settings mode when omitted
    uint32_t seed;       // 0 = settings/random
    int      intensity;  // -1 = settings
};

// Capture and store a photo. Returns the stored id, 0 on failure.
// `why` receives a short error string on failure.
uint32_t capture_photo(const CaptureRequest& req, const char** why);
bool     capture_busy();

// Grab a single JPEG frame for a live-view push or snapshot (not stored).
camera_fb_t* capture_grab_live();
