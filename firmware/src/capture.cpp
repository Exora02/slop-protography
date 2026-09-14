#include "capture.h"

#include <ArduinoJson.h>
#include <esp_camera.h>

#include "bend.h"
#include "camera_ctl.h"
#include "glitch.h"
#include "log.h"
#include "ws_live.h"

static const char* TAG = "cap";

static bool s_busy = false;

bool capture_busy() { return s_busy; }

static void broadcast_photo(const PhotoMeta& m) {
    JsonDocument doc;
    JsonObject ev = doc.to<JsonObject>();
    ev["event"] = "photo";
    ev["id"] = m.id;
    ev["mode"] = m.mode;
    ev["mode_name"] = cap_mode_name(m.mode);
    ev["jw"] = m.jw;
    ev["jh"] = m.jh;
    ev["jlen"] = m.jlen;
    ev["rw"] = m.rw;
    ev["rh"] = m.rh;
    ev["rlen"] = m.rlen;
    ev["seed"] = m.seed;
    ev["intensity"] = m.intensity;
    ev["ts"] = m.ts_ms;
    ev["boot"] = m.boot_seq;
    ws_broadcast_json(doc);
}

uint32_t capture_photo(const CaptureRequest& req_in, const char** why) {
    if (why) *why = "";
    if (s_busy) {
        if (why) *why = "busy";
        return 0;
    }
    if (!cam_ok()) {
        if (why) *why = "camera unavailable";
        return 0;
    }
    s_busy = true;

    CaptureRequest req = req_in;
    if (req.intensity < 0) req.intensity = g_settings.glitch_intensity;
    uint32_t seed = req.seed;

    PhotoMeta meta = {};
    meta.mode = (uint8_t)req.mode;
    meta.intensity = (uint8_t)req.intensity;
    meta.ts_ms = millis();
    meta.boot_seq = store_boot_seq();

    uint8_t* jpeg = nullptr;
    uint8_t* raw = nullptr;

    if (req.mode == MODE_RAW) {
        // RAW capture: full sensor Bayer via a format reinit, plus a quick
        // VGA JPEG preview so the gallery still shows something.
        float bpp = 0;
        if (!cam_capture_raw(raw, meta.rlen, meta.rw, meta.rh, bpp)) {
            s_busy = false;
            if (why) *why = "raw capture failed";
            return 0;
        }
        meta.raw_bpp = bpp;

        camera_fb_t* fb = cam_grab_jpeg(FRAMESIZE_VGA, 1, 3000);
        if (fb) {
            jpeg = (uint8_t*)heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM);
            if (jpeg) {
                memcpy(jpeg, fb->buf, fb->len);
                meta.jlen = fb->len;
                meta.jw = fb->width;
                meta.jh = fb->height;
            }
            esp_camera_fb_return(fb);
        }
        if (!jpeg) {
            heap_caps_free(raw);
            raw = nullptr;
            s_busy = false;
            if (why) *why = "raw preview failed";
            return 0;
        }
        // RAW stays pristine: no glitch passes on the preview or the Bayer.
        meta.seed = 0;
    } else {
        // JPEG-based modes: capture at the still framesize.
        if (req.mode == MODE_BEND) {
            seed = bend_apply(seed ? seed : g_settings.bend_seed);
        }

        camera_fb_t* fb = cam_grab_jpeg(FRAMESIZE_5MP, 1, 5000);
        if (!fb) {
            bend_restore();
            s_busy = false;
            if (why) *why = "frame timeout";
            return 0;
        }

        jpeg = (uint8_t*)heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM);
        if (!jpeg) {
            esp_camera_fb_return(fb);
            bend_restore();
            s_busy = false;
            if (why) *why = "out of memory";
            return 0;
        }
        memcpy(jpeg, fb->buf, fb->len);
        meta.jlen = fb->len;
        meta.jw = fb->width;
        meta.jh = fb->height;
        esp_camera_fb_return(fb);

        if (req.mode == MODE_BEND) bend_restore();

        if (req.mode == MODE_MOSH || req.mode == MODE_BEND) {
            if (req.intensity <= 0) req.intensity = 5;   // effect modes imply corruption
            meta.seed = glitch_jpeg(jpeg, meta.jlen, seed, (uint8_t)req.intensity);
        } else {
            meta.seed = glitch_jpeg(jpeg, meta.jlen, seed, (uint8_t)req.intensity);
        }
        meta.intensity = (uint8_t)req.intensity;
    }

    if (!store_add(meta, jpeg, raw)) {
        if (jpeg) heap_caps_free(jpeg);
        if (raw) heap_caps_free(raw);
        s_busy = false;
        if (why) *why = "storage full";
        return 0;
    }

    s_busy = false;
    broadcast_photo(meta);
    return meta.id;
}

camera_fb_t* capture_grab_live() {
    framesize_t fs = kLiveSizes[g_settings.live_framesize];
    return cam_grab_jpeg(fs, 0, 1500);
}
