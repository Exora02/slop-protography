#include "dng.h"

#include <string.h>

#include "log.h"

static const char* TAG = "dng";

// ---- little-endian TIFF/DNG helpers ----
static void w16(uint8_t* p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }
static void w32(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

enum TagType : uint16_t { TT_BYTE = 1, TT_ASCII = 2, TT_SHORT = 3, TT_LONG = 4, TT_RATIONAL = 5 };

static uint32_t tag_type_size(uint16_t t) {
    switch (t) {
        case TT_BYTE:
        case TT_ASCII: return 1;
        case TT_SHORT: return 2;
        default: return 4;
    }
}

// Entry with inlined value (<= 4 bytes) or an offset into the extra area.
static void entry(uint8_t* p, uint16_t tag, uint16_t type, uint32_t count, uint32_t value_or_off,
                  bool is_offset, const uint8_t* small = nullptr) {
    w16(p + 0, tag);
    w16(p + 2, type);
    w32(p + 4, count);
    if (is_offset) {
        w32(p + 8, value_or_off);
    } else if (small != nullptr) {
        memset(p + 8, 0, 4);
        memcpy(p + 8, small, count * tag_type_size(type));
    } else {
        w32(p + 8, value_or_off);
    }
}

size_t dng_total_size(const PhotoMeta* m) {
    if (!m || !m->rlen) return 0;
    size_t payload;
    if (m->raw_bpp >= 1.9f) {
        payload = (size_t)m->rw * m->rh * 2;
    } else if (m->raw_bpp > 1.1f) {
        payload = (size_t)m->rw * m->rh * 2;   // 10-bit packed expands to 16-bit
    } else {
        payload = (size_t)m->rw * m->rh;       // RAW8 passthrough
    }
    return 512 + payload;
}

bool dng_begin(const PhotoMeta* m, const uint8_t* raw, size_t raw_len, DngCtx& ctx) {
    if (!m || !raw || !m->rlen || !m->rw || !m->rh) return false;
    memset(&ctx, 0, sizeof(ctx));
    ctx.meta = m;
    ctx.raw = raw;
    ctx.raw_len = raw_len;

    const uint16_t w = m->rw, h = m->rh;
    const bool is_raw8 = m->raw_bpp <= 1.1f;
    const uint32_t white = is_raw8 ? 255 : DNG_RAW16_WHITE_LEVEL;
    const uint16_t bits = is_raw8 ? 8 : 16;
    ctx.payload_len = is_raw8 ? (size_t)w * h : (size_t)w * h * 2;

    uint8_t* b = ctx.hdr;
    memset(b, 0, sizeof(ctx.hdr));

    // TIFF header (little-endian), IFD0 at offset 8.
    b[0] = 'I'; b[1] = 'I';
    w16(b + 2, 42);
    w32(b + 4, 8);

    const size_t entry_count = 23;
    const size_t ifd_off = 8;
    const size_t extra_off = ifd_off + 2 + entry_count * 12 + 4;   // 8 + 2 + 276 + 4 = 290
    const uint32_t payload_off = 512;

    size_t extra_pos = extra_off;
    auto place = [&](const uint8_t* data, uint32_t len) -> uint32_t {
        uint32_t at = (uint32_t)extra_pos;
        memcpy(b + extra_pos, data, len);
        extra_pos += len;
        if (extra_pos & 1) {
            b[extra_pos] = 0;
            extra_pos++;
        }
        return at;
    };

    // --- extra-data blobs ---
    static const char make_str[] = "Protography";
    static const char model_str[] = "OV5640";
    static const char unique_str[] = "Protography OV5640";
    uint8_t cfa_dim[4];
    w16(cfa_dim, 2); w16(cfa_dim + 2, 2);
    uint8_t cfa_pat[4] = DNG_CFA_PATTERN;
    uint8_t dng_ver[4] = {1, 4, 0, 0};
    uint8_t dng_bw_ver[4] = {1, 1, 0, 0};
    uint8_t plane_color[3] = {0, 1, 2};   // R, G, B
    // sRGB D65 color matrix, rationals over 10000
    static const uint32_t cmat[18] = {4124, 10000, 3576, 10000, 1805, 10000,
                                      2126, 10000, 7152, 10000, 722, 10000,
                                      193, 10000, 1192, 10000, 9505, 10000};
    uint8_t cmat_bytes[sizeof(cmat)];
    for (size_t i = 0; i < 9; i++) {
        w32(cmat_bytes + i * 8, cmat[i * 2]);
        w32(cmat_bytes + i * 8 + 4, cmat[i * 2 + 1]);
    }

    uint32_t off_make = place((const uint8_t*)make_str, sizeof(make_str));
    uint32_t off_model = place((const uint8_t*)model_str, sizeof(model_str));
    uint32_t off_unique = place((const uint8_t*)unique_str, sizeof(unique_str));
    uint32_t off_cfadim = place(cfa_dim, 4);
    uint32_t off_cfapat = place(cfa_pat, 4);
    uint32_t off_dngver = place(dng_ver, 4);
    uint32_t off_dngbw = place(dng_bw_ver, 4);
    uint32_t off_plane = place(plane_color, 3);
    uint32_t off_cmat = place(cmat_bytes, sizeof(cmat_bytes));

    if (extra_pos > payload_off) {
        LOGE(TAG, "DNG header overflow (%u > 512)", (unsigned)extra_pos);
        return false;
    }

    // --- IFD0 entries, strictly ascending tag order ---
    size_t p = ifd_off;
    w16(b + p, entry_count); p += 2;
    entry(b + p, 254, TT_LONG, 1, 0, false); p += 12;                    // NewSubFileType
    entry(b + p, 256, TT_LONG, 1, w, false); p += 12;                    // ImageWidth
    entry(b + p, 257, TT_LONG, 1, h, false); p += 12;                    // ImageLength
    entry(b + p, 258, TT_SHORT, 1, bits, false); p += 12;                // BitsPerSample
    entry(b + p, 259, TT_SHORT, 1, 1, false); p += 12;                    // Compression: none
    entry(b + p, 262, TT_SHORT, 1, 32803, false); p += 12;               // Photometric: CFA
    entry(b + p, 271, TT_ASCII, sizeof(make_str), off_make, true); p += 12;
    entry(b + p, 272, TT_ASCII, sizeof(model_str), off_model, true); p += 12;
    entry(b + p, 273, TT_LONG, 1, payload_off, false); p += 12;          // StripOffsets
    entry(b + p, 277, TT_SHORT, 1, 1, false); p += 12;                   // SamplesPerPixel
    entry(b + p, 278, TT_LONG, 1, h, false); p += 12;                    // RowsPerStrip
    entry(b + p, 279, TT_LONG, 1, (uint32_t)ctx.payload_len, false); p += 12;  // StripByteCounts
    entry(b + p, 284, TT_SHORT, 1, 1, false); p += 12;                   // PlanarConfiguration
    entry(b + p, 33421, TT_SHORT, 2, off_cfadim, true); p += 12;         // CFARepeatPatternDim
    entry(b + p, 33422, TT_BYTE, 4, off_cfapat, true); p += 12;          // CFAPattern
    entry(b + p, 50706, TT_BYTE, 4, off_dngver, true); p += 12;          // DNGVersion
    entry(b + p, 50707, TT_BYTE, 4, off_dngbw, true); p += 12;           // DNGBackwardVersion
    entry(b + p, 50708, TT_ASCII, sizeof(unique_str), off_unique, true); p += 12;
    entry(b + p, 50710, TT_BYTE, 3, off_plane, true); p += 12;           // CFAPlaneColor
    entry(b + p, 50713, TT_LONG, 1, DNG_BLACK_LEVEL, false); p += 12;    // BlackLevel
    entry(b + p, 50717, TT_LONG, 1, white, false); p += 12;              // WhiteLevel
    entry(b + p, 50721, TT_RATIONAL, 9, off_cmat, true); p += 12;        // ColorMatrix1
    entry(b + p, 50778, TT_SHORT, 1, 21, false); p += 12;                // CalibrationIlluminant1: D65
    w32(b + p, 0);                                                       // no next IFD

    ctx.hdr_sent = 0;
    LOGI(TAG, "dng %ux%u payload %u bpp=%.2f", w, h, (unsigned)ctx.payload_len, (double)m->raw_bpp);
    return true;
}

size_t dng_next(DngCtx& ctx, uint8_t* buf, size_t bufcap) {
    // 1) header
    if (ctx.hdr_sent < 512) {
        size_t n = 512 - ctx.hdr_sent;
        if (n > bufcap) n = bufcap;
        memcpy(buf, ctx.hdr + ctx.hdr_sent, n);
        ctx.hdr_sent += n;
        return n;
    }

    // 2) payload
    if (ctx.sent >= ctx.payload_len || !ctx.raw) return 0;

    const PhotoMeta* m = ctx.meta;

    if (m->raw_bpp >= 1.9f || m->raw_bpp <= 1.1f) {
        // 16-bit container or RAW8: straight passthrough.
        size_t n = ctx.payload_len - ctx.sent;
        if (n > bufcap) n = bufcap;
        memcpy(buf, ctx.raw + ctx.sent, n);
        ctx.sent += n;
        return n;
    }

    // 10-bit packed: 5 bytes -> 4 right-justified 16-bit samples.
    // Track how many complete 5-byte groups we've consumed.
    size_t groups_done = ctx.sent / 8;           // 8 bytes out per group
    size_t out_off = 0;
    while (out_off + 8 <= bufcap) {
        size_t gpos = groups_done * 5;
        if (gpos + 5 > ctx.raw_len) break;
        const uint8_t* s = ctx.raw + gpos;
        uint16_t v[4];
        v[0] = (uint16_t)(((uint16_t)s[0] << 2) | (s[1] >> 6));
        v[1] = (uint16_t)((((uint16_t)s[1] & 0x3F) << 4) | (s[2] >> 4));
        v[2] = (uint16_t)((((uint16_t)s[2] & 0x0F) << 6) | (s[3] >> 2));
        v[3] = (uint16_t)((((uint16_t)s[3] & 0x03) << 8) | s[4]);
        for (int i = 0; i < 4; i++) {
            w16(buf + out_off + i * 2, v[i] << 6);   // left-justify 10 bits in 16
        }
        out_off += 8;
        groups_done++;
        if ((groups_done * 8) >= ctx.payload_len) break;
    }
    ctx.sent = groups_done * 8;
    return out_off;
}
