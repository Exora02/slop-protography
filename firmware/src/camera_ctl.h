#pragma once

#include <esp_camera.h>

#include "config.h"

// Camera lifecycle + frame grabbing for the OV5640 on the XIAO ESP32S3 Sense.
//
// The camera is initialised once at the largest still size we ever use (5MP
// JPEG) so the frame buffer is sized for the worst case, then the live view
// switches framesizes freely through sensor->set_framesize(). RAW capture
// needs a different pixel format, which requires a full deinit/init cycle
// (sensor constraint, not a driver choice).

// Sizes offered for the live view (index stored in settings.live_framesize).
// Keep in sync with web/app.js LIVE_SIZES.
extern const framesize_t kLiveSizes[4];

bool cam_begin();                       // init in JPEG mode at max still size
bool cam_ok();                          // camera initialised successfully
bool af_ok();                           // autofocus firmware loaded
const char* af_status_text();           // human-readable AF state for status JSON
bool cam_focus_trigger();               // single-shot AF (blocks up to ~2 s)
void cam_apply_base();                  // apply user settings to the sensor
bool cam_set_framesize(framesize_t fs); // switch JPEG framesize (no reinit)

// Grab a JPEG frame at `fs`, discarding up to `settle` transitional frames
// after the switch. Returns a frame that MUST be released with cam_return(),
// or nullptr on timeout.
camera_fb_t* cam_grab_jpeg(framesize_t fs, int settle = 1, uint32_t timeout_ms = 4000);

// RAW capture: reinit in RAW at the largest size that fits, grab, then
// reinit back to JPEG + re-apply base settings. Output buffer is ps_malloc'd
// and owned by the caller on success.
// Returns the frame dimensions in *w/*h and the packed raw bytes.
// `fb_bytes_pp` reports the driver's bytes-per-pixel (1, 1.25 or 2) so the
// DNG writer knows how to interpret the data.
bool cam_capture_raw(uint8_t*& out_buf, uint32_t& out_len, uint16_t& w, uint16_t& h,
                     float& fb_bytes_pp);

// Reinit back to the normal JPEG pipeline after a RAW excursion.
bool cam_recover();
