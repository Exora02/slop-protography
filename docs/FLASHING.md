# Flashing & first boot

## One-time setup

```bash
pip install platformio        # or install the PlatformIO IDE extension in VS Code
```

> **Windows note**: espressif's tool installer refuses to run from MSys /
> Git Bash shells. If `pio run` fails with `MSys/Mingw is not supported`,
> run it from cmd/PowerShell, or clear the variable first:
> `cmd /c "set MSYSTEM= && pio run"`. (CI on Linux is unaffected.)

## Build & flash

With the XIAO on USB-C (its boot button enters the ROM bootloader if the
port doesn't enumerate the DFU device automatically):

```bash
cd firmware
pio run                     # compile
pio run -t upload           # flash the firmware
pio run -t uploadfs         # flash the web app (LittleFS)
pio device monitor          # 115200 — boot log prints the SSID + IP
```

First build downloads the toolchain and framework (~600 MB).

## First boot checklist

1. Serial log shows `camera up (af=1)` — if `camera init failed`, reseat the
   camera board and check the log's camera section.
2. On the phone, join `Protography-XXXX` (open). The captive portal should
   open the UI; if not, browse to `http://192.168.4.1` (Android) or
   `http://protography.local` (iOS).
3. You should see the live view within a second or two; the LED breathes
   slow while streaming.
4. Take one photo in each mode; check the gallery; download a JPG and (for
   the RAW shot) a DNG.
5. Long-press the shutter to sleep; short-press to wake.

## Updating just the web app

After editing `firmware/data/`: `pio run -t uploadfs` — no firmware reflash
needed. Hard-refresh the phone page to bust the cache.

## Factory reset

Settings live in NVS; photos in RAM are lost on reboot/sleep by design. To
wipe settings: `pio run -t erase` then reflash firmware + fs.
