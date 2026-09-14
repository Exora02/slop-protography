#include "settings.h"

#include <Preferences.h>

#include "log.h"

static const char* TAG = "settings";

Settings g_settings;

static void settings_defaults(Settings& s) {
    s.mode = MODE_HQ;
    s.jpeg_quality = 10;
    s.live_framesize = 1;   // index 1 = VGA in the live-size table
    s.live_fps = LIVE_DEFAULT_FPS;
    s.glitch_intensity = 0;
    s.glitch_seed = 0;
    s.bend_seed = 0;
    s.hmirror = false;
    s.vflip = false;
    s.sleep_min = SLEEP_DEFAULT_MIN;
    s.ap_name[0] = '\0';
    s.ap_pass[0] = '\0';
}

void settings_load() {
    settings_defaults(g_settings);

    Preferences prefs;
    prefs.begin("protography", true);
    g_settings.mode = prefs.getUChar("mode", g_settings.mode);
    g_settings.jpeg_quality = prefs.getUChar("jq", g_settings.jpeg_quality);
    g_settings.live_framesize = prefs.getUChar("lfs", g_settings.live_framesize);
    g_settings.live_fps = prefs.getUChar("lfps", g_settings.live_fps);
    g_settings.glitch_intensity = prefs.getUChar("gi", g_settings.glitch_intensity);
    g_settings.glitch_seed = prefs.getULong("gseed", g_settings.glitch_seed);
    g_settings.bend_seed = prefs.getULong("bseed", g_settings.bend_seed);
    g_settings.hmirror = prefs.getBool("hmirror", g_settings.hmirror);
    g_settings.vflip = prefs.getBool("vflip", g_settings.vflip);
    g_settings.sleep_min = prefs.getUShort("sleep", g_settings.sleep_min);
    prefs.getString("apname", g_settings.ap_name, sizeof(g_settings.ap_name));
    prefs.getString("appass", g_settings.ap_pass, sizeof(g_settings.ap_pass));
    prefs.end();

    // Clamp anything corrupt from a previous firmware flash.
    if (g_settings.mode > MODE_BEND) g_settings.mode = MODE_HQ;
    if (g_settings.jpeg_quality < 6) g_settings.jpeg_quality = 6;
    if (g_settings.jpeg_quality > 20) g_settings.jpeg_quality = 20;
    if (g_settings.live_fps < 1) g_settings.live_fps = 1;
    if (g_settings.live_fps > LIVE_MAX_FPS) g_settings.live_fps = LIVE_MAX_FPS;
}

void settings_save() {
    Preferences prefs;
    prefs.begin("protography", false);
    prefs.putUChar("mode", g_settings.mode);
    prefs.putUChar("jq", g_settings.jpeg_quality);
    prefs.putUChar("lfs", g_settings.live_framesize);
    prefs.putUChar("lfps", g_settings.live_fps);
    prefs.putUChar("gi", g_settings.glitch_intensity);
    prefs.putULong("gseed", g_settings.glitch_seed);
    prefs.putULong("bseed", g_settings.bend_seed);
    prefs.putBool("hmirror", g_settings.hmirror);
    prefs.putBool("vflip", g_settings.vflip);
    prefs.putUShort("sleep", g_settings.sleep_min);
    prefs.putString("apname", g_settings.ap_name);
    prefs.putString("appass", g_settings.ap_pass);
    prefs.end();
}

void settings_apply_json(JsonVariant obj) {
    Settings& s = g_settings;
    if (!obj["mode"].isNull()) {
        int m = obj["mode"];
        if (m >= MODE_HQ && m <= MODE_BEND) s.mode = m;
    }
    if (!obj["jpeg_quality"].isNull()) {
        int q = obj["jpeg_quality"];
        if (q >= 6 && q <= 20) s.jpeg_quality = q;
    }
    if (!obj["live_fps"].isNull()) {
        int f = obj["live_fps"];
        if (f >= 1 && f <= LIVE_MAX_FPS) s.live_fps = f;
    }
    if (!obj["live_framesize"].isNull()) {
        int f = obj["live_framesize"];
        if (f >= 0 && f <= 3) s.live_framesize = f;   // live-size table has 4 entries
    }
    if (!obj["glitch_intensity"].isNull()) {
        int i = obj["glitch_intensity"];
        if (i >= 0 && i <= 10) s.glitch_intensity = i;
    }
    if (!obj["glitch_seed"].isNull()) s.glitch_seed = obj["glitch_seed"].as<uint32_t>();
    if (!obj["bend_seed"].isNull()) s.bend_seed = obj["bend_seed"].as<uint32_t>();
    if (!obj["hmirror"].isNull()) s.hmirror = obj["hmirror"];
    if (!obj["vflip"].isNull()) s.vflip = obj["vflip"];
    if (!obj["sleep_min"].isNull()) {
        int t = obj["sleep_min"];
        if (t >= 0 && t <= 240) s.sleep_min = t;
    }
    if (!obj["ap_name"].isNull()) strlcpy(s.ap_name, obj["ap_name"] | "", sizeof(s.ap_name));
    if (!obj["ap_pass"].isNull()) strlcpy(s.ap_pass, obj["ap_pass"] | "", sizeof(s.ap_pass));
    settings_save();
}

void settings_fill_json(JsonObject out) {
    Settings& s = g_settings;
    out["mode"] = s.mode;
    out["mode_name"] = cap_mode_name(s.mode);
    out["jpeg_quality"] = s.jpeg_quality;
    out["live_framesize"] = s.live_framesize;
    out["live_fps"] = s.live_fps;
    out["glitch_intensity"] = s.glitch_intensity;
    out["glitch_seed"] = s.glitch_seed;
    out["bend_seed"] = s.bend_seed;
    out["hmirror"] = s.hmirror;
    out["vflip"] = s.vflip;
    out["sleep_min"] = s.sleep_min;
    out["ap_name"] = s.ap_name;
}
