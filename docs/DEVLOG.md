# Devlog

Working notes, decisions and dead ends, newest last.

## 2026-09-14 — project bootstrap

**Goal.** Screenless artistic camera: XIAO ESP32S3 Sense + OV5640 + shutter
button + battery. Phone is the viewfinder/gallery. Wants: clean base render,
true RAW, datamosh/circuit-bent effects, everything git-tracked and
documented.

**Research findings that shaped the design:**

- esp32-camera (upstream, v2.1.x era) supports the OV5640 up to 5MP, has a
  `PIXFORMAT_RAW` path in the OV5640 driver, and — new since roughly 2025 —
  a sensor-agnostic autofocus API in `sensor_t` (`af_init` downloads the VCM
  firmware over I²C, `af_set_mode(0)` = continuous, `af_trigger()` =
  single-shot). It's compiled in via `CONFIG_CAMERA_AF_SUPPORT`.
- Seeed's OV5640 camera board for the XIAO Sense exists and carries a VCM
  (101-MDZ-LY0041). Seeed forum threads from 2025 show AF was long broken
  from Arduino-land; the esp32-camera-native AF API is the better base than
  the old third-party "OV5640 autofocus" libraries those threads tried.
- Official PlatformIO is frozen at Arduino core 2.x; the pioarduino fork
  tracks core 3.x / IDF 5.x → we use its platform zip URL.
- RAW packing in the driver is under-documented (the HAL computes buffer
  sizes per-format in `ll_cam.c`, out of sight) → the DNG writer
  auto-detects 8-bit / packed-10 / 16-bit from the observed frame length,
  and the header knobs are #defines to flip once real hardware answers.

**Architecture** (see ARCHITECTURE.md): PlatformIO + Arduino core 3.x;
device runs an open AP + captive portal; web app from LittleFS; WebSocket
(port 81) pushes binary JPEG frames for live view (iOS Safari doesn't do
MJPEG-over-HTTP, and blob-URL `<img>` updates work everywhere); HTTP API
for capture/gallery/settings; DNGs stream out chunked. Photos live in a
5 MB PSRAM arena with LRU eviction + pinning during downloads.

**Effects:** two engines, both free of codecs — (1) seeded JPEG byte
corruption inside the entropy scan (scramble/clone/xor/sort/bitrotate ops),
(2) "bend" = randomized sensor register abuse within datasheet-safe ranges,
restored after each shot. RAW mode stays pristine.

**Toolchain fight (Windows):** the pioarduino platform installs its
toolchains through espressif's `idf_tools.py`, which (a) needs its Python
deps in PlatformIO's private venv — a broken `requests` there cost an hour —
and (b) refuses to run under MSys/Git Bash (`MSYSTEM` env var). Documented
the cmd.exe workaround in FLASHING.md and made CI (Linux) the canonical
clean build.

**Partition table gotcha:** the first successful build silently used the
`XIAO_ESP32S3` board *variant's* own `partitions-8mb.csv` (OTA layout, FAT
partition) instead of ours — priority order is `build.partitions <
variant < source`, and our file had the same name. Renamed ours to
`protography-8mb.csv` (6 MB app + ~2 MB LittleFS) so it wins. Caught by
decoding the generated `partitions.bin` — worth remembering that the build
report's "maximum_size" is the tell.

**State at end of day:** firmware compiles (details below), web UI written,
docs written. Nothing tested on hardware — the camera module hasn't been
wired yet. Open questions tracked in ARCHITECTURE.md "Known limitations".

**Next:** assemble hardware, run the FLASHING.md checklist, then:
1. verify AF actually focuses (log `af_status_text` transitions),
2. shoot a RAW, check the DNG in a real converter, flip the `dng.h` knobs
   if channels/levels look wrong,
3. tune glitch op mix on real images,
4. measure real current draw vs the estimates in HARDWARE.md.
