#include "store.h"

#include <Preferences.h>

#include "log.h"

static const char* TAG = "store";

struct Slot {
    PhotoMeta meta;
    uint8_t*  jpeg;
    uint8_t*  raw;
    bool      pinned;
};

static Slot s_slots[PHOTO_MAX_COUNT];
static uint16_t s_count = 0;
static size_t s_used = 0;
static uint32_t s_next_id = 1;
static uint16_t s_boot_seq = 0;

uint16_t store_boot_seq() { return s_boot_seq; }

bool store_init() {
    memset(s_slots, 0, sizeof(s_slots));
    s_count = 0;
    s_used = 0;
    Preferences prefs;
    prefs.begin("protography", false);
    s_boot_seq = prefs.getUShort("boot", 0) + 1;
    prefs.putUShort("boot", s_boot_seq);
    s_next_id = ((uint32_t)s_boot_seq << 16) + 1;   // ids sortable across boots
    prefs.end();
    LOGI(TAG, "arena %u KB, boot seq %u", (unsigned)(PHOTO_ARENA_BYTES / 1024), s_boot_seq);
    return true;
}

static void free_slot(Slot& s) {
    if (s.jpeg) heap_caps_free(s.jpeg);
    if (s.raw) heap_caps_free(s.raw);
    s_used -= s.meta.jlen + s.meta.rlen;
    s = {};
}

static void evict_oldest() {
    // Slots are kept sorted newest-first; evict from the end, skipping photos
    // that are being streamed out (pinned).
    for (int i = s_count - 1; i >= 0; i--) {
        if (s_slots[i].pinned) continue;
        LOGI(TAG, "evict photo %u (%u KB)", s_slots[i].meta.id,
             (unsigned)((s_slots[i].meta.jlen + s_slots[i].meta.rlen) / 1024));
        free_slot(s_slots[i]);
        memmove(&s_slots[i], &s_slots[i + 1], (s_count - i - 1) * sizeof(Slot));
        s_count--;
        s_slots[s_count] = {};
        return;
    }
}

bool store_add(PhotoMeta& m, uint8_t* jpeg, uint8_t* raw) {
    size_t need = m.jlen + m.rlen;
    while ((s_used + need > store_budget() || s_count >= PHOTO_MAX_COUNT) && s_count > 0) {
        evict_oldest();
    }
    if (s_used + need > store_budget()) {
        LOGW(TAG, "capture too big for arena (%u KB)", (unsigned)(need / 1024));
        return false;
    }

    // New ids are monotonically increasing, so the newest always goes to the
    // front and the array stays sorted newest-first.
    m.id = s_next_id++;
    memmove(&s_slots[1], &s_slots[0], (PHOTO_MAX_COUNT - 1) * sizeof(Slot));
    s_slots[0].meta = m;
    s_slots[0].jpeg = jpeg;
    s_slots[0].raw = raw;
    s_used += need;
    s_count++;
    LOGI(TAG, "stored #%u mode=%u %uKB (+%uKB raw) — %u/%u KB used", m.id, m.mode,
         (unsigned)(m.jlen / 1024), (unsigned)(m.rlen / 1024), (unsigned)(s_used / 1024),
         (unsigned)(store_budget() / 1024));
    return true;
}

uint16_t store_count() { return s_count; }

const PhotoMeta* store_meta(uint16_t index) {
    if (index >= s_count) return nullptr;
    return &s_slots[index].meta;
}

const PhotoMeta* store_find(uint32_t id) {
    for (uint16_t i = 0; i < s_count; i++) {
        if (s_slots[i].meta.id == id) return &s_slots[i].meta;
    }
    return nullptr;
}

const uint8_t* store_jpeg_ptr(uint32_t id, uint32_t* len) {
    for (uint16_t i = 0; i < s_count; i++) {
        if (s_slots[i].meta.id == id) {
            if (len) *len = s_slots[i].meta.jlen;
            return s_slots[i].jpeg;
        }
    }
    return nullptr;
}

const uint8_t* store_raw_ptr(uint32_t id, uint32_t* len) {
    for (uint16_t i = 0; i < s_count; i++) {
        if (s_slots[i].meta.id == id && s_slots[i].raw) {
            if (len) *len = s_slots[i].meta.rlen;
            return s_slots[i].raw;
        }
    }
    return nullptr;
}

bool store_remove(uint32_t id) {
    for (uint16_t i = 0; i < s_count; i++) {
        if (s_slots[i].meta.id == id) {
            free_slot(s_slots[i]);
            memmove(&s_slots[i], &s_slots[i + 1], (s_count - i - 1) * sizeof(Slot));
            s_count--;
            s_slots[s_count] = {};
            return true;
        }
    }
    return false;
}

void store_clear() {
    for (uint16_t i = 0; i < s_count; i++) free_slot(s_slots[i]);
    s_count = 0;
}

size_t store_used() { return s_used; }
size_t store_budget() { return PHOTO_ARENA_BYTES; }

bool store_pin(uint32_t id) {
    for (uint16_t i = 0; i < s_count; i++) {
        if (s_slots[i].meta.id == id) {
            s_slots[i].pinned = true;
            return true;
        }
    }
    return false;
}

void store_unpin(uint32_t id) {
    for (uint16_t i = 0; i < s_count; i++) {
        if (s_slots[i].meta.id == id) s_slots[i].pinned = false;
    }
}
