#include "http_api.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <SD.h>
#include <uri/UriRegex.h>

#include "camera_ctl.h"
#include "capture.h"
#include "config.h"
#include "dng.h"
#include "log.h"
#include "net.h"
#include "power.h"
#include "sdcard.h"
#include "settings.h"
#include "store.h"
#include "ws_live.h"

static const char* TAG = "http";

static WebServer s_server(HTTP_PORT);
static bool s_live_enabled = true;   // pushed by main via http_set_live_state

void http_set_live_state(bool on) { s_live_enabled = on; }

static void send_status_json() {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["event"] = "status";
    root["fw"] = PROTO_FW_VERSION;
    root["name"] = net_ap_ssid();
    root["cam_ok"] = cam_ok();
    root["af"] = af_status_text();
    root["af_ok"] = af_ok();
    root["busy"] = capture_busy();
    root["uptime_ms"] = millis();
    root["psram_free"] = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    root["psram_total"] = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    JsonObject store = root["store"].to<JsonObject>();
    store["used"] = store_used();
    store["budget"] = store_budget();
    store["count"] = store_count();
    JsonObject sd = root["sd"].to<JsonObject>();
    sd["ok"] = sd_ok();
    sd["free"] = sd_free_bytes();
    sd["count"] = sd_count();
    JsonObject live = root["live"].to<JsonObject>();
    live["on"] = s_live_enabled;
    live["clients"] = ws_client_count();
    settings_fill_json(root["settings"].to<JsonObject>());
    String out;
    serializeJson(doc, out);
    s_server.send(200, "application/json", out);
}

static void photo_meta_json(const PhotoMeta* m, JsonObject o, bool in_ram) {
    o["id"] = m->id;
    o["mode"] = m->mode;
    o["mode_name"] = cap_mode_name(m->mode);
    o["jw"] = m->jw;
    o["jh"] = m->jh;
    o["jlen"] = m->jlen;
    o["rw"] = m->rw;
    o["rh"] = m->rh;
    o["rlen"] = m->rlen;
    o["seed"] = m->seed;
    o["intensity"] = m->intensity;
    o["ts"] = m->ts_ms;
    o["boot"] = m->boot_seq;
    o["in_ram"] = in_ram;
    o["sd"] = sd_has_photo(m->id);
}

static void send_photos_json() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    // Hot tier first (newest first), then card-only photos.
    for (uint16_t i = 0; i < store_count(); i++) {
        const PhotoMeta* m = store_meta(i);
        if (!m) continue;
        photo_meta_json(m, arr.add<JsonObject>(), true);
    }
    for (uint16_t i = 0; i < sd_count(); i++) {
        const PhotoMeta* m = sd_meta(i);
        if (!m || store_find(m->id)) continue;   // already listed from RAM
        photo_meta_json(m, arr.add<JsonObject>(), false);
    }
    String out;
    serializeJson(doc, out);
    s_server.send(200, "application/json", out);
}

static void handle_capture() {
    CaptureRequest req = {};
    req.mode = (CapMode)g_settings.mode;
    req.seed = 0;
    req.intensity = -1;
    if (s_server.hasArg("plain")) {
        JsonDocument doc;
        if (deserializeJson(doc, s_server.arg("plain")) == DeserializationError::Ok) {
            JsonVariant v = doc.as<JsonVariant>();
            if (!v["mode"].isNull()) req.mode = (CapMode)(int)v["mode"];
            if (!v["seed"].isNull()) req.seed = v["seed"].as<uint32_t>();
            if (!v["intensity"].isNull()) req.intensity = (int)v["intensity"];
        }
    }
    const char* why = "";
    uint32_t id = capture_photo(req, &why);
    if (!id) {
        String err = String("{\"error\":\"") + why + "\"}";
        s_server.send(409, "application/json", err);
        return;
    }
    const PhotoMeta* m = store_find(id);
    if (!m) {
        s_server.send(500, "application/json", "{\"error\":\"lost\"}");
        return;
    }
    JsonDocument doc;
    JsonObject o = doc.to<JsonObject>();
    o["id"] = m->id;
    o["mode"] = m->mode;
    o["mode_name"] = cap_mode_name(m->mode);
    o["jw"] = m->jw;
    o["jh"] = m->jh;
    o["jlen"] = m->jlen;
    o["rw"] = m->rw;
    o["rh"] = m->rh;
    o["rlen"] = m->rlen;
    o["seed"] = m->seed;
    o["intensity"] = m->intensity;
    String out;
    serializeJson(doc, out);
    s_server.send(200, "application/json", out);
}

static bool parse_photo_id(const String& path, const char* suffix, uint32_t* id) {
    // path like /api/photo/123.jpg or /api/photo/123.dng
    int start = strlen("/api/photo/");
    String sub = path.substring(start);
    int dot = sub.indexOf('.');
    if (dot < 0) return false;
    if (suffix && sub.substring(dot) != suffix) return false;
    *id = (uint32_t)sub.substring(0, dot).toInt();
    return *id != 0;
}

static void handle_photo_jpeg() {
    uint32_t id;
    if (!parse_photo_id(s_server.uri(), ".jpg", &id)) {
        s_server.send(400, "text/plain", "bad id");
        return;
    }
    uint32_t len = 0;
    const uint8_t* p = store_jpeg_ptr(id, &len);
    if (p) {
        store_pin(id);
        String cd = String("inline; filename=\"protography_") + id + ".jpg\"";
        s_server.sendHeader("Content-Disposition", cd);
        // send_P takes (type, buffer, length) and does not treat content as a
        // C string — required for binary JPEG payloads.
        s_server.send_P(200, "image/jpeg", (const char*)p, len);
        store_unpin(id);
        return;
    }
    // Cold tier: stream straight off the card.
    if (sd_ok()) {
        File f = SD.open(String("/P") + id + ".JPG");
        if (f && !f.isDirectory()) {
            String cd = String("inline; filename=\"protography_") + id + ".jpg\"";
            s_server.sendHeader("Content-Disposition", cd);
            s_server.streamFile(f, "image/jpeg");
            f.close();
            return;
        }
    }
    s_server.send(404, "text/plain", "no such photo");
}

static void handle_photo_dng() {
    uint32_t id;
    if (!parse_photo_id(s_server.uri(), ".dng", &id)) {
        s_server.send(400, "text/plain", "bad id");
        return;
    }
    const PhotoMeta* m = store_find(id);
    uint32_t rlen = 0;
    const uint8_t* raw = m ? store_raw_ptr(id, &rlen) : nullptr;

    // Cold tier: the DNG was materialized onto the card at capture time.
    if (!raw && sd_ok()) {
        File f = SD.open(String("/P") + id + ".DNG");
        if (f && !f.isDirectory()) {
            String cd = String("attachment; filename=\"protography_") + id + ".dng\"";
            s_server.sendHeader("Content-Disposition", cd);
            s_server.streamFile(f, "image/x-adobe-dng");
            f.close();
            return;
        }
    }
    if (!m || !m->rlen || !raw) {
        s_server.send(404, "text/plain", "no raw for this photo");
        return;
    }

    store_pin(id);
    DngCtx ctx;
    if (!dng_begin(m, raw, rlen, ctx)) {
        store_unpin(id);
        s_server.send(500, "text/plain", "dng init failed");
        return;
    }
    size_t total = 512 + ctx.payload_len;
    String cd = String("attachment; filename=\"protography_") + id + ".dng\"";
    s_server.sendHeader("Content-Disposition", cd);
    s_server.setContentLength(total);
    s_server.send(200, "image/x-adobe-dng", "");
    static uint8_t chunk[8192];
    size_t n;
    while ((n = dng_next(ctx, chunk, sizeof(chunk))) > 0) {
        size_t sent = 0;
        while (sent < n) {
            size_t w = s_server.client().write((const char*)chunk + sent, n - sent);
            if (w == 0) {
                LOGW(TAG, "client dropped mid-dng");
                store_unpin(id);
                return;
            }
            sent += w;
        }
        // Yield so the WebSocket/LED keep breathing during long downloads.
        yield();
    }
    store_unpin(id);
}

static void handle_photo_delete() {
    uint32_t id;
    if (!parse_photo_id(s_server.uri(), nullptr, &id)) {
        s_server.send(400, "text/plain", "bad id");
        return;
    }
    bool in_ram = store_remove(id);
    bool on_sd = sd_ok() && sd_remove_photo(id);
    if (in_ram || on_sd) {
        s_server.send(200, "application/json", "{\"ok\":true}");
    } else {
        s_server.send(404, "text/plain", "no such photo");
    }
}

static void handle_settings_post() {
    if (s_server.hasArg("plain")) {
        JsonDocument doc;
        if (deserializeJson(doc, s_server.arg("plain")) == DeserializationError::Ok) {
            settings_apply_json(doc.as<JsonVariant>());
            cam_apply_base();
            JsonDocument out;
            settings_fill_json(out.to<JsonObject>());
            String s;
            serializeJson(out, s);
            s_server.send(200, "application/json", s);
            return;
        }
    }
    s_server.send(400, "text/plain", "bad json");
}

static void handle_snapshot() {
    camera_fb_t* fb = capture_grab_live();
    if (!fb) {
        s_server.send(503, "text/plain", "camera busy or unavailable");
        return;
    }
    s_server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
}

static void handle_reboot() {
    s_server.send(200, "application/json", "{\"ok\":true}");
    delay(150);   // let the response flush
    ESP.restart();
}

static void handle_captive() {
    // Phones probing captive-portal URLs get bounced to our index page.
    String host = s_server.hostHeader();
    String ip = net_ap_ip().toString();
    if (host != ip && host != String(MDNS_HOSTNAME) + ".local" && host.indexOf("protography") < 0) {
        String loc = String("http://") + ip + "/";
        s_server.sendHeader("Location", loc, true);
        s_server.send(302, "text/plain", "");
        return;
    }
    s_server.send(404, "text/plain", "not found");
}

bool http_api_begin() {
    s_server.on("/api/status", HTTP_GET, send_status_json);
    s_server.on("/api/photos", HTTP_GET, send_photos_json);
    s_server.on("/api/capture", HTTP_POST, handle_capture);
    s_server.on("/api/settings", HTTP_POST, handle_settings_post);
    s_server.on("/api/snapshot.jpg", HTTP_GET, handle_snapshot);
    s_server.on("/api/reboot", HTTP_POST, handle_reboot);
    s_server.on(UriRegex("^/api/photo/[0-9]+\\.jpg$"), HTTP_GET, handle_photo_jpeg);
    s_server.on(UriRegex("^/api/photo/[0-9]+\\.dng$"), HTTP_GET, handle_photo_dng);
    s_server.on(UriRegex("^/api/photo/[0-9]+$"), HTTP_DELETE, handle_photo_delete);
    // ESP32 WebServer's serveStatic() returns void, so the index page gets an
    // explicit route; everything else (css/js) falls through to serveStatic.
    s_server.on("/", HTTP_GET, []() {
        File f = LittleFS.open("/index.html", "r");
        if (!f) {
            s_server.send(404, "text/plain",
                          "web ui missing — flash it with: pio run -t uploadfs");
            return;
        }
        s_server.streamFile(f, "text/html");
        f.close();
    });
    s_server.serveStatic("/", LittleFS, "/");
    s_server.onNotFound(handle_captive);
    s_server.begin();
    LOGI(TAG, "http server on :%u", HTTP_PORT);
    return true;
}

void http_api_loop() { s_server.handleClient(); }
