# Architecture

## Decisions and why

| decision | choice | why |
|---|---|---|
| toolchain | PlatformIO + [pioarduino] platform (Arduino core 3.x, IDF 5.x) | official PlatformIO is frozen at core 2.x; Seeed's own OV5640 guidance targets core 3.x, and esp32-camera's OV5640 AF + RAW support wants the newer stack |
| camera driver | bundled `esp32-camera` (v2.x) | upstream now supports the OV5640 up to 5MP, `PIXFORMAT_RAW`, and a sensor-agnostic AF API (`af_init` / `af_trigger` …) |
| phone app | web app served from the device (LittleFS) | nothing to install, iOS + Android, works with no internet; WebSocket live view because iOS Safari doesn't render MJPEG over HTTP |
| storage | PSRAM arena (photos live in RAM) | no SD card in the BOM; 8 MB PSRAM holds a useful roll; everything is pulled to the phone before shutdown |
| effects | in-camera: JPEG byte-glitch + sensor abuse | both are computationally free and produce *real* artifacts — no JPEG codec needed on-device |
| threading | everything on the main loop task | the camera, Wi-Fi servers and store are single-owner; no locks, fewer races. Live view is paced inside the loop |

[pioarduino]: https://github.com/pioarduino/platform-espressif32

## Module map

```
main.cpp ──────── boot, loop, WS command dispatch, button actions
 ├─ settings      NVS-persisted user settings (mode, quality, fx, sleep…)
 ├─ camera_ctl    init/reinit, framesize switching, JPEG grabs, RAW capture
 │                 (RAW needs a deinit/init cycle — different pixformat)
 ├─ capture       one function: request (mode, seed, intensity) → stored photo
 │   ├─ bend      "circuit-bent" sensor presets (apply → shoot → restore)
 │   ├─ glitch    seeded JPEG byte corruption (datamosh)
 │   └─ store     PSRAM arena, metadata, LRU eviction, pinning
 ├─ dng           streaming TIFF/DNG writer for stored raw captures
 ├─ net           AP + captive DNS + mDNS
 ├─ http_api      REST endpoints, JPEG download, chunked DNG download
 ├─ ws_live       WebSocket server: binary JPEG frames (live view) + JSON events
 ├─ button        debounce, short/double/long/very-long detection
 ├─ led           status patterns (the only "screen")
 └─ power         idle timer → deep sleep, GPIO wake on the shutter
```

## Capture pipeline

```
HQ    : set_framesize(5MP) → grab JPEG → copy to PSRAM → glitch? → store
RAW   : deinit → init(PIXFORMAT_RAW, largest fitting size) → grab → copy
        → deinit → init(JPEG) → re-apply settings + AF firmware → grab VGA
        preview → store (raw bytes + preview). DNG is generated at download
        time from the packed raw bytes.
MOSH  : HQ pipeline + glitch_jpeg(seed, intensity)
BEND  : bend_apply(seed) → HQ pipeline → bend_restore() → glitch on top
```

The OV5640 has an on-sensor JPEG engine — for HQ/MOSH/BEND the bytes coming
out of the DMA buffer are already JPEG, which is what makes in-camera
byte-glitching free. The RAW excursion re-inits the sensor because pixel
format can't be switched at runtime; that's why RAW shots take ~1.5–2 s.

## Memory budget (8 MB PSRAM)

| consumer                        | typical     |
|---------------------------------|-------------|
| camera frame buffer (5MP JPEG)  | ~1 MB       |
| RAW frame buffer (UXGA, 2 bpp)  | ~3.8 MB (transient, only during RAW capture) |
| photo arena (hot tier)          | ≤ 5 MB cap  |
| live JPEG + WebSocket tx buffer | ~100 KB     |

Eviction is oldest-first and skips photos currently being streamed out
(pinned). A RAW capture picks the largest of UXGA/SXGA/HD/SVGA that fits in
free PSRAM with headroom.

## Storage tiers

Photos are captured into the PSRAM arena (hot tier) and, when a FAT32 card
is mounted, mirrored to it (cold tier) as three files per photo at the card
root: `/P<id>.JPG`, `/P<id>.DNG` (materialized at capture time through the
streaming DNG writer) and `/P<id>.JSON` (metadata sidecar). The gallery
merges both tiers (RAM entries first, card-only entries after, deduplicated
by id); JPEG/DNG downloads serve from RAM when hot and stream off the card
otherwise; deletes hit both tiers.

Two hardware quirks worth knowing (details in HARDWARE.md): the expansion
board's SD chip-select shares GPIO21 with the user LED — with a card
mounted, the firmware suspends the LED module and the pin flickers on card
writes — and photo ids embed a persistent boot counter, so filenames stay
unique across reboots (wrapping after 65 536 boots, which is a you-problem
for the year 2200 or so).

## Wire protocol

HTTP (port 80):

| endpoint                    | method | purpose                          |
|-----------------------------|--------|----------------------------------|
| `/`                         | GET    | web app (LittleFS)               |
| `/api/status`               | GET    | firmware/camera/store/settings   |
| `/api/photos`               | GET    | photo list (newest first)        |
| `/api/capture`              | POST   | `{mode, seed, intensity}`        |
| `/api/photo/<id>.jpg`       | GET    | JPEG download (RAM, else SD)     |
| `/api/photo/<id>.dng`       | GET    | DNG download (RAM, else SD)      |
| `/api/photo/<id>`           | DELETE | remove photo (both tiers)        |
| `/api/settings`             | POST   | partial settings update          |
| `/api/snapshot.jpg`         | GET    | one live frame (no store)        |
| `/api/reboot`               | POST   | restart                          |

WebSocket (port 81):

- binary frames: `0x01` + JPEG bytes = live view frame
- text frames (JSON):
  - client → device: `{"cmd":"live","on":bool,"fps":n}`,
    `{"cmd":"capture","mode":m,"seed":s,"intensity":i}`, `{"cmd":"focus"}`,
    `{"cmd":"status"}`, `{"cmd":"hello"}`, `{"cmd":"sleep"}`
  - device → client: `event: hello|status|photo|error`

The web app mirrors this protocol exactly — see `firmware/data/app.js`.

## Modes shared everywhere

`0 = HQ, 1 = RAW, 2 = MOSH, 3 = BEND` — kept in sync between
`firmware/src/settings.h`, `firmware/data/app.js` and `docs/EFFECTS.md`.

## Known limitations / roadmap

- RAW/DNG packing is driver-version dependent — verify on hardware (see
  EFFECTS.md "RAW caveats"); the writer auto-detects 8-bit / packed-10 /
  16-bit and the knobs are in `dng.h`.
- Switching to RAW reloads AF firmware each time (~0.5 s); a future
  `esp_camera_reconfigure()` path could avoid it.
- No clock: photo timestamps are millis + boot counter; first STA sync to
  NTP would give wall-clock EXIF.
- No auth on the AP by default (open network) — set a password in settings.
- SD writes happen synchronously in the capture path (up to ~1–2 s for a
  5MP JPEG + DNG); a background write queue would hide that latency.
- The SD gallery listing caps at the newest 128 card photos (RAM for the
  index; files beyond the cap are still downloadable by id).
