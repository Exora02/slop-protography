#include <Arduino.h>
#include <LittleFS.h>

#include "button.h"
#include "camera_ctl.h"
#include "capture.h"
#include "config.h"
#include "http_api.h"
#include "led.h"
#include "log.h"
#include "net.h"
#include "power.h"
#include "sdcard.h"
#include "settings.h"
#include "store.h"
#include "ws_live.h"

static const char* TAG = "main";

static Button s_shutter(BTN_PIN_SHUTTER);
static bool s_live_on = true;
static uint32_t s_last_frame_ms = 0;
static uint32_t s_last_status_ms = 0;

static void broadcast_hello() {
    JsonDocument doc;
    JsonObject ev = doc.to<JsonObject>();
    ev["event"] = "hello";
    ev["fw"] = PROTO_FW_VERSION;
    ev["name"] = net_ap_ssid();
    ev["cam_ok"] = cam_ok();
    ev["af_ok"] = af_ok();
    ev["url"] = net_url();
    ws_broadcast_json(doc);
}

static void broadcast_status() {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["event"] = "status";
    root["cam_ok"] = cam_ok();
    root["af"] = af_status_text();
    root["busy"] = capture_busy();
    root["uptime_ms"] = millis();
    root["psram_free"] = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    JsonObject store = root["store"].to<JsonObject>();
    store["used"] = store_used();
    store["budget"] = store_budget();
    store["count"] = store_count();
    JsonObject sd = root["sd"].to<JsonObject>();
    sd["ok"] = sd_ok();
    sd["free"] = sd_free_bytes();
    sd["count"] = sd_count();
    JsonObject live = root["live"].to<JsonObject>();
    live["on"] = s_live_on;
    live["clients"] = ws_client_count();
    settings_fill_json(root["settings"].to<JsonObject>());
    ws_broadcast_json(doc);
}

static void handle_ws_command(JsonVariant cmd) {
    power_note_activity();
    const char* c = cmd["cmd"] | "";
    if (strcmp(c, "hello") == 0) {
        broadcast_hello();
    } else if (strcmp(c, "live") == 0) {
        s_live_on = cmd["on"] | false;
        if (!cmd["fps"].isNull()) {
            int f = cmd["fps"];
            if (f >= 1 && f <= LIVE_MAX_FPS) g_settings.live_fps = f;
        }
        http_set_live_state(s_live_on);
        broadcast_status();
    } else if (strcmp(c, "capture") == 0) {
        CaptureRequest req = {};
        req.mode = (CapMode)g_settings.mode;
        req.seed = 0;
        req.intensity = -1;
        if (!cmd["mode"].isNull()) req.mode = (CapMode)(int)cmd["mode"];
        if (!cmd["seed"].isNull()) req.seed = cmd["seed"].as<uint32_t>();
        if (!cmd["intensity"].isNull()) req.intensity = (int)cmd["intensity"];
        const char* why = "";
        if (!capture_photo(req, &why) && strlen(why) > 0) {
            JsonDocument err;
            JsonObject o = err.to<JsonObject>();
            o["event"] = "error";
            o["msg"] = why;
            ws_broadcast_json(err);
        }
    } else if (strcmp(c, "focus") == 0) {
        cam_focus_trigger();
    } else if (strcmp(c, "status") == 0) {
        broadcast_status();
    } else if (strcmp(c, "sleep") == 0) {
        power_sleep_now();
    }
}

static void do_shutter() {
    CaptureRequest req = {};
    req.mode = (CapMode)g_settings.mode;
    req.seed = 0;
    req.intensity = -1;
    const char* why = "";
    uint32_t id = capture_photo(req, &why);
    led_flash(id ? LED_FLASH2 : LED_BLINK_FAST);
}

static void cycle_mode() {
    g_settings.mode = (g_settings.mode + 1) % 4;
    settings_save();
    broadcast_status();
}

void setup() {
    Serial.begin(115200);
    delay(150);
    LOGI(TAG, "Protography " PROTO_FW_VERSION " booting%s",
         power_woken_by_button() ? " (woke from sleep)" : "");

    led_begin();
    led_set(LED_BLINK_FAST);   // until Wi-Fi is up
    s_shutter.begin();

    settings_load();
    store_init();

    if (!LittleFS.begin(true)) {
        LOGE(TAG, "LittleFS mount failed (formatting didn't help?)");
    }

    // Persistent tier: mount the card if present. On the Sense expansion
    // board the card's chip-select shares GPIO21 with the user LED — when a
    // card is mounted we hand the pin over and the LED becomes a write
    // indicator (it flickers when art hits the card).
    bool card = sd_begin();
    if (card && SD_PIN_CS == LED_PIN) {
        led_suspend();
        LOGI(TAG, "LED handed over to SD chip-select (shared GPIO%d)", LED_PIN);
    }

    bool net_ok = net_begin();
    led_set(net_ok ? LED_STEADY : LED_BLINK_FAST);

    bool cam_up = cam_begin();
    if (!cam_up) LOGE(TAG, "camera init failed — UI runs in degraded mode");

    http_api_begin();
    ws_begin(handle_ws_command);

    power_note_activity();
    LOGI(TAG, "ready — join \"%s\", UI at %s or http://%s/",
         net_ap_ssid().c_str(), net_url().c_str(), net_ap_ip().toString().c_str());
}

void loop() {
    // An attached phone counts as activity — don't sleep under a viewer.
    if (ws_has_clients()) power_note_activity();

    // Input.
    switch (s_shutter.tick()) {
        case BTN_SHORT:
            power_note_activity();
            do_shutter();
            break;
        case BTN_DOUBLE:
            power_note_activity();
            cycle_mode();
            break;
        case BTN_LONG:
            power_note_activity();
            s_live_on = !s_live_on;
            http_set_live_state(s_live_on);
            broadcast_status();
            break;
        case BTN_VERYLONG:
            power_note_activity();
            power_sleep_now();
            break;
        default:
            break;
    }

    // Networking.
    net_dns_tick();
    http_api_loop();
    ws_loop();

    // Live view: paced JPEG pushes to any connected phone.
    if (s_live_on && ws_has_clients() && !capture_busy() && cam_ok()) {
        uint32_t interval = 1000u / (g_settings.live_fps ? g_settings.live_fps : 1);
        if (millis() - s_last_frame_ms >= interval) {
            s_last_frame_ms = millis();
            camera_fb_t* fb = capture_grab_live();
            if (fb) {
                ws_send_jpeg(fb->buf, fb->len);
                esp_camera_fb_return(fb);
            }
        }
        led_set(LED_BLINK_SLOW);
    } else {
        led_set(ws_has_clients() ? LED_HEARTBEAT : LED_STEADY);
    }

    // Periodic status push so the phone's indicators stay fresh.
    if (ws_has_clients() && millis() - s_last_status_ms > 5000) {
        s_last_status_ms = millis();
        broadcast_status();
    }

    led_tick();
    power_tick();
}
