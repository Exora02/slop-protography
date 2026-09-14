# Protography

A tiny artistic camera built on the **Seeed XIAO ESP32S3 Sense** with the
**OV5640** 5MP camera module and a single shutter button. No screen — your
phone is the viewfinder, gallery and darkroom.

- **Live view + shutter** from any phone browser (the camera runs its own
  Wi-Fi network and serves a web app — nothing to install, works offline).
- **HQ mode** — clean 5MP JPEG.
- **RAW mode** — raw Bayer capture, downloadable as `.dng`.
- **MOSH mode** — in-camera JPEG corruption (datamosh-style smears, tears,
  macroblocks), seedable and reproducible.
- **BEND mode** — circuit-bent capture: sensor gain/exposure chaos,
  corrections off, wrong white balance, then moshed.

```
shoot        phone ──Wi-Fi──► camera ──DVP──► OV5640
store        photos live in PSRAM until you pull them
pull         phone browser: gallery → save JPG / save DNG
```

**Status:** firmware + web UI written and compiling; hardware not assembled
yet, so anything marked *verify-on-hardware* in the docs is untested against
a live sensor (RAW/DNG packing and the AF callbacks in particular).

## Repo layout

| path                | what                                                    |
|---------------------|---------------------------------------------------------|
| `firmware/`         | PlatformIO project (Arduino core 3.x / ESP-IDF 5.x)     |
| `firmware/src/`     | camera control, effects, DNG writer, store, web servers |
| `firmware/data/`    | the phone web app (LittleFS, uploaded with `uploadfs`)  |
| `docs/`             | hardware build guide, architecture, effects, devlog      |
| `.github/workflows/`| CI that builds the firmware on push                      |

## Quick start (when the hardware is ready)

```bash
pip install platformio                 # or the VS Code extension
cd firmware
pio run                                # build
pio run -t upload                      # flash firmware (USB-C)
pio run -t uploadfs                    # flash the web app
pio device monitor                     # 115200 baud, shows the SSID
```

Then on your phone: join the `Protography-XXXX` Wi-Fi (open network; the
captive portal should pop the UI — otherwise open `http://192.168.4.1` or
`http://protography.local`), and shoot. Details in
[docs/FLASHING.md](docs/FLASHING.md).

## What it builds on

- [espressif/esp32-camera](https://github.com/espressif/esp32-camera) — OV5640
  driver incl. RAW pixformat and the voice-coil autofocus API
- [pioarduino/platform-espressif32](https://github.com/pioarduino/platform-espressif32) —
  PlatformIO platform with Arduino core 3.x (official PlatformIO is frozen on 2.x)
- [ArduinoJson](https://arduinojson.org/), [WebSockets](https://github.com/Links2004/arduinoWebSockets),
  LittleFS, ESPmDNS — the usual suspects

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for how it all fits
together, [docs/EFFECTS.md](docs/EFFECTS.md) for the art knobs, and
[docs/HARDWARE.md](docs/HARDWARE.md) for the build.

## License

MIT — see [LICENSE](LICENSE).
