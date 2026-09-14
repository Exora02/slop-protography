#include "camera_ctl.h"

#include <Arduino.h>

#include "log.h"
#include "settings.h"

static const char* TAG = "cam";

const framesize_t kLiveSizes[4] = {
    FRAMESIZE_QVGA,   // 320x240
    FRAMESIZE_VGA,    // 640x480
    FRAMESIZE_SVGA,   // 800x600
    FRAMESIZE_HD,     // 1280x720
};

static bool s_cam_ok = false;
static bool s_af_ok = false;
static framesize_t s_cur_fs = FRAMESIZE_QVGA;

static camera_config_t base_config() {
    camera_config_t cfg = {};
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer = LEDC_TIMER_0;
    cfg.pin_pwdn = CAM_PIN_PWDN;
    cfg.pin_reset = CAM_PIN_RESET;
    cfg.pin_xclk = CAM_PIN_XCLK;
    cfg.pin_sccb_sda = CAM_PIN_SIOD;
    cfg.pin_sccb_scl = CAM_PIN_SIOC;
    cfg.pin_d7 = CAM_PIN_Y9;
    cfg.pin_d6 = CAM_PIN_Y8;
    cfg.pin_d5 = CAM_PIN_Y7;
    cfg.pin_d4 = CAM_PIN_Y6;
    cfg.pin_d3 = CAM_PIN_Y5;
    cfg.pin_d2 = CAM_PIN_Y4;
    cfg.pin_d1 = CAM_PIN_Y3;
    cfg.pin_d0 = CAM_PIN_Y2;
    cfg.pin_vsync = CAM_PIN_VSYNC;
    cfg.pin_href = CAM_PIN_HREF;
    cfg.pin_pclk = CAM_PIN_PCLK;
    cfg.xclk_freq_hz = 20000000;
    cfg.pixel_format = PIXFORMAT_JPEG;
    // 5MP is the OV5640's full array. The frame buffer is allocated for this
    // worst case so smaller framesizes can be switched to without reinit.
    cfg.frame_size = FRAMESIZE_5MP;
    cfg.jpeg_quality = g_settings.jpeg_quality;
    cfg.fb_count = 1;
    cfg.fb_location = CAMERA_FB_IN_PSRAM;
    cfg.grab_mode = CAMERA_GRAB_LATEST;
    return cfg;
}

static bool init_camera(const camera_config_t& cfg) {
    esp_err_t err = esp_camera_init((camera_config_t*)&cfg);
    if (err != ESP_OK) {
        LOGE(TAG, "esp_camera_init failed: 0x%x", err);
        return false;
    }
    return true;
}

static bool init_af() {
    sensor_t* s = esp_camera_sensor_get();
    if (!s || !s->af_init) {
        // Driver built without CONFIG_CAMERA_AF_SUPPORT or sensor without VCM.
        return false;
    }
    // Loads the OV5640 AF firmware over I2C (~0.5 s), then enables
    // continuous autofocus.
    if (s->af_init(s, 3000) != 0) {
        LOGW(TAG, "AF firmware load failed, staying fixed-focus");
        return false;
    }
    if (s->af_set_mode && s->af_set_mode(s, 0) != 0) {  // 0 = continuous
        LOGW(TAG, "continuous AF enable failed");
    }
    return true;
}

void cam_apply_base() {
    sensor_t* s = esp_camera_sensor_get();
    if (!s) return;
    s->set_quality(s, g_settings.jpeg_quality);
    s->set_hmirror(s, g_settings.hmirror);
    s->set_vflip(s, g_settings.vflip);
    // Defaults that favour a clean base render; bend mode overrides these
    // temporarily during capture.
    s->set_whitebal(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_exposure_ctrl(s, 1);
    s->set_awb_gain(s, 1);
    s->set_denoise(s, 1);
    s->set_lenc(s, 1);
    s->set_bpc(s, 1);
    s->set_wpc(s, 1);
    s->set_special_effect(s, 0);
    s->set_saturation(s, 0);
    s->set_contrast(s, 0);
    s->set_sharpness(s, 0);
}

bool cam_begin() {
    camera_config_t cfg = base_config();
    if (!init_camera(cfg)) {
        s_cam_ok = false;
        return false;
    }
    s_cam_ok = true;
    s_af_ok = init_af();
    cam_apply_base();
    LOGI(TAG, "camera up (af=%d)", s_af_ok);
    return true;
}

bool cam_ok() { return s_cam_ok; }
bool af_ok() { return s_af_ok; }

const char* af_status_text() {
    sensor_t* s = esp_camera_sensor_get();
    if (!s || !s->af_get_status) return "n/a";
    uint8_t raw = 0;
    bool focused = false, busy = false;
    if (s->af_get_status(s, &raw, &focused, &busy) != 0) return "n/a";
    if (busy) return "focusing";
    if (focused) return "focused";
    return "idle";
}

bool cam_focus_trigger() {
    sensor_t* s = esp_camera_sensor_get();
    if (!s || !s->af_trigger || !s_af_ok) return false;
    return s->af_trigger(s) == 0;
}

bool cam_set_framesize(framesize_t fs) {
    if (!s_cam_ok) return false;
    if (fs == s_cur_fs) return true;
    sensor_t* s = esp_camera_sensor_get();
    if (!s || !s->set_framesize || s->set_framesize(s, fs) != 0) {
        LOGW(TAG, "set_framesize(%d) failed", (int)fs);
        return false;
    }
    s_cur_fs = fs;
    return true;
}

camera_fb_t* cam_grab_jpeg(framesize_t fs, int settle, uint32_t timeout_ms) {
    if (!s_cam_ok) return nullptr;
    if (!cam_set_framesize(fs)) return nullptr;
    camera_fb_t* fb = nullptr;
    for (int i = 0; i <= settle; i++) {
        // Discard frames captured right after a mode switch — the sensor
        // may still be streaming the previous framesize.
        fb = esp_camera_fb_get();
        if (!fb) {
            LOGW(TAG, "fb_get timeout");
            return nullptr;
        }
        if (i < settle) esp_camera_fb_return(fb);
    }
    return fb;
}

bool cam_capture_raw(uint8_t*& out_buf, uint32_t& out_len, uint16_t& w, uint16_t& h,
                     float& fb_bytes_pp) {
    out_buf = nullptr;
    out_len = 0;
    if (!s_cam_ok) return false;

    // Candidate RAW sizes, largest first. The RAW frame buffer lives in PSRAM
    // together with the stored photos, so pick the largest that fits with
    // headroom. All OV5640 native 4:3 sizes.
    const struct { framesize_t fs; uint16_t w, h; } cand[] = {
        {FRAMESIZE_UXGA, 1600, 1200},
        {FRAMESIZE_SXGA, 1280, 1024},
        {FRAMESIZE_HD, 1280, 720},
        {FRAMESIZE_SVGA, 800, 600},
    };

    // 2 bytes/pixel is the worst case for RAW (10-bit in a 16-bit container).
    const size_t need_margin = 512u * 1024u;
    for (auto& c : cand) {
        size_t worst = (size_t)c.w * c.h * 2 + need_margin;
        if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < worst) continue;

        esp_camera_deinit();
        camera_config_t cfg = base_config();
        cfg.pixel_format = PIXFORMAT_RAW;
        cfg.frame_size = c.fs;
        cfg.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        if (!init_camera(cfg)) continue;

        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            LOGE(TAG, "raw fb_get failed at %dx%d", c.w, c.h);
            esp_camera_deinit();
            cam_recover();
            return false;
        }

        out_buf = (uint8_t*)heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM);
        if (out_buf) {
            memcpy(out_buf, fb->buf, fb->len);
            out_len = (uint32_t)fb->len;
            w = fb->width;
            h = fb->height;
            fb_bytes_pp = (float)fb->len / ((float)fb->width * fb->height);
            LOGI(TAG, "raw %ux%u len=%u (%.2f bpp)", w, h, (unsigned)fb->len,
                 (double)fb_bytes_pp);
        }
        esp_camera_fb_return(fb);
        esp_camera_deinit();
        cam_recover();
        return out_buf != nullptr;
    }

    LOGE(TAG, "no raw size fits in free PSRAM");
    cam_recover();
    return false;
}

// Reinit back to the normal JPEG pipeline after a RAW excursion.
bool cam_recover() {
    camera_config_t cfg = base_config();
    if (!init_camera(cfg)) {
        s_cam_ok = false;
        return false;
    }
    s_af_ok = init_af();
    cam_apply_base();
    s_cur_fs = FRAMESIZE_QVGA;  // force re-set on next grab
    return true;
}
