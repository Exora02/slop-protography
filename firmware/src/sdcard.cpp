#include "sdcard.h"

#include <SD.h>
#include <SPI.h>

#include <ArduinoJson.h>

#include "config.h"
#include "dng.h"
#include "log.h"

static const char* TAG = "sd";

static bool s_ok = false;
static PhotoMeta s_metas[SD_LIST_MAX];
static uint16_t s_count = 0;

static String jpg_path(uint32_t id) { return String("/P") + id + ".JPG"; }
static String dng_path(uint32_t id) { return String("/P") + id + ".DNG"; }
static String meta_path(uint32_t id) { return String("/P") + id + ".JSON"; }

// Keep the newest-first invariant: shift down and insert at the front.
static void insert_meta_sorted(const PhotoMeta& m) {
    if (s_count >= SD_LIST_MAX) return;   // oldest beyond the cap aren't listed
    uint16_t i = s_count;
    while (i > 0 && s_metas[i - 1].id < m.id) {
        s_metas[i] = s_metas[i - 1];
        i--;
    }
    s_metas[i] = m;
    s_count++;
}

static void parse_sidecar(File& f, PhotoMeta& m) {
    JsonDocument doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) return;
    m.id = doc["id"] | 0u;
    m.mode = doc["mode"] | 0u;
    m.jw = doc["jw"] | 0;
    m.jh = doc["jh"] | 0;
    m.jlen = doc["jlen"] | 0u;
    m.rw = doc["rw"] | 0;
    m.rh = doc["rh"] | 0;
    m.rlen = doc["rlen"] | 0u;
    m.seed = doc["seed"] | 0u;
    m.intensity = doc["intensity"] | 0u;
    m.ts_ms = doc["ts"] | 0u;
    m.boot_seq = doc["boot"] | 0;
}

bool sd_begin() {
    // Explicit pins: the expansion board routes SPI to D8/D9/D10 and CS to
    // GPIO21 (shared with the user LED — Seeed's design, not ours).
    SPI.begin(SD_PIN_SCK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS);
    if (!SD.begin(SD_PIN_CS, SPI, SD_SPI_HZ)) {
        LOGI(TAG, "no SD card — staying in amnesia mode (PSRAM only)");
        return false;
    }
    s_ok = true;
    s_count = 0;

    // Scan existing photos (sidecars only; files are opened on demand).
    File root = SD.open("/");
    if (root && root.isDirectory()) {
        File f;
        while ((f = root.openNextFile())) {
            String name = f.name();
            if (!name.endsWith(".JSON")) {
                f.close();
                continue;
            }
            PhotoMeta m = {};
            parse_sidecar(f, m);
            f.close();
            if (m.id) insert_meta_sorted(m);
        }
        root.close();
    }
    uint64_t freeB = sd_free_bytes();
    LOGI(TAG, "SD mounted: %u photo(s), %u MB free", s_count,
         (unsigned)(freeB / (1024u * 1024u)));
    return true;
}

bool sd_ok() { return s_ok; }

uint64_t sd_free_bytes() {
    if (!s_ok) return 0;
    return (uint64_t)SD.totalBytes() - SD.usedBytes();
}

uint16_t sd_count() { return s_count; }

const PhotoMeta* sd_meta(uint16_t index) {
    if (index >= s_count) return nullptr;
    return &s_metas[index];
}

bool sd_write_photo(const PhotoMeta& m, const uint8_t* jpeg, const uint8_t* raw) {
    if (!s_ok) return false;

    // 1) JPEG
    File fj = SD.open(jpg_path(m.id), FILE_WRITE);
    if (!fj) {
        LOGW(TAG, "cannot create %s", jpg_path(m.id).c_str());
        return false;
    }
    size_t written = fj.write(jpeg, m.jlen);
    fj.close();
    if (written != m.jlen) {
        LOGW(TAG, "jpeg truncated (%u/%u)", (unsigned)written, (unsigned)m.jlen);
        SD.remove(jpg_path(m.id));
        return false;
    }

    // 2) DNG via the streaming writer — never fully assembled in RAM.
    if (raw && m.rlen) {
        DngCtx ctx;
        if (dng_begin(&m, raw, m.rlen, ctx)) {
            File fd = SD.open(dng_path(m.id), FILE_WRITE);
            if (fd) {
                static uint8_t chunk[8192];
                size_t n;
                while ((n = dng_next(ctx, chunk, sizeof(chunk))) > 0) {
                    if (fd.write(chunk, n) != n) {
                        LOGW(TAG, "dng write truncated");
                        break;
                    }
                }
                fd.close();
            }
        }
    }

    // 3) Sidecar
    File fm = SD.open(meta_path(m.id), FILE_WRITE);
    if (fm) {
        JsonDocument doc;
        JsonObject o = doc.to<JsonObject>();
        o["id"] = m.id;
        o["mode"] = m.mode;
        o["jw"] = m.jw;
        o["jh"] = m.jh;
        o["jlen"] = m.jlen;
        o["rw"] = m.rw;
        o["rh"] = m.rh;
        o["rlen"] = m.rlen;
        o["seed"] = m.seed;
        o["intensity"] = m.intensity;
        o["ts"] = m.ts_ms;
        o["boot"] = m.boot_seq;
        serializeJson(doc, fm);
        fm.close();
    }

    insert_meta_sorted(m);
    LOGI(TAG, "mirrored photo %u to SD (%u KB)", m.id, (unsigned)(m.jlen / 1024));
    return true;
}

bool sd_has_photo(uint32_t id) {
    for (uint16_t i = 0; i < s_count; i++) {
        if (s_metas[i].id == id) return true;
    }
    return false;
}

bool sd_remove_photo(uint32_t id) {
    if (!s_ok) return false;
    SD.remove(jpg_path(id));
    SD.remove(dng_path(id));
    SD.remove(meta_path(id));
    for (uint16_t i = 0; i < s_count; i++) {
        if (s_metas[i].id == id) {
            memmove(&s_metas[i], &s_metas[i + 1], (s_count - i - 1) * sizeof(PhotoMeta));
            s_count--;
            return true;
        }
    }
    return true;   // files gone even if the sidecar wasn't indexed
}
