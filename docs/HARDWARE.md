# Hardware guide

The device is deliberately minimal: a battery, the XIAO ESP32S3 Sense, the
OV5640 camera board, and one button. This guide covers the bill of materials,
wiring, assembly order, and the practical gotchas that aren't obvious from
the product pages.

## Bill of materials

| part                                            | notes                                                       |
|-------------------------------------------------|-------------------------------------------------------------|
| Seeed Studio XIAO ESP32S3 **Sense**             | the *Sense* variant — it has the camera B2B connector + mic  |
| OV5640 camera board for XIAO ESP32S3 Sense      | Seeed sells the 5MP module with autofocus VCM (101-MDZ-LY0041). Get the Seeed one, not a generic OV5640 breakout — the connector and pinout must match |
| LiPo cell, 3.7 V, **with protection**           | 500–2000 mAh, JST or bare pads (see wiring)                 |
| Tactile button, 6×6 mm, through-hole            | the shutter                                                 |
| 100 kΩ resistor                                 | external pull-up on the button line (deep-sleep wake)       |
| (optional) 100 nF capacitor                     | across the button for hardware debounce — firmware already debounces, this is belt-and-braces |
| (optional) SPDT slide switch                    | battery on/off                                              |
| Wi-Fi antenna                                   | **ships in the XIAO box — don't lose it**, IPEX/U.FL connector |
| USB-C cable                                     | charging + flashing                                         |

## Wiring

The camera board plugs into the XIAO's B2B camera connector — no wiring. The
button is the only soldering:

```
 XIAO ESP32S3 (front edge)                shutter button
 ┌──────────────────────┐
 │  3V3  ───────────────┼──────────┐
 │                      │         [100kΩ]
 │  D1 (GPIO2) ─────────┼───┬──────┴───┐
 │                      │   │          │
 │  GND ────────────────┼───┴──────────┤ (button)
 └──────────────────────┘   (press → GPIO2 to GND)
```

- Button between **D1 (GPIO2)** and **GND**.
- The 100 kΩ pull-up goes from GPIO2 to 3V3. The firmware enables the
  internal pull-up at runtime, but internal pulls are not guaranteed to be
  retained in deep sleep on the S3 — the external resistor is what makes
  "press to wake" reliable.
- Why D1/GPIO2: all of GPIO10–18 and GPIO38–48 are used by the camera
  connector; GPIO0 is the boot button, GPIO3/45/46 are strapping pins.
  GPIO1–9, 43, 44 on the edge header are free. D1 is comfortably out of the
  way. (GPIO43/44 are the UART — leave them free for serial debugging.)

### Battery

- Solder the cell to the **BAT+ / BAT−** pads on the XIAO's bottom edge
  (respect polarity — check the silkscreen twice).
- The XIAO has an on-board charger: plug USB and it charges the cell. The
  charge current is fixed by the board (~350 mA), so keep the battery
  ≥ 700 mAh to stay under ~0.5C charging.
- Prefer a cell with a built-in protection circuit (PCM). Treat bare cells
  with respect: never puncture, never charge below 0 °C, tape the terminals
  while building.
- A slide switch in series with the battery works as a master off-switch;
  deep-sleep current is low enough (~tens of µA) that you can skip it and
  just let the idle timer sleep the device.

## Assembly order

1. **Bench-test first, mechanically later.** Seat the camera board on the
   XIAO, snap on the antenna, plug USB, flash the firmware, and confirm you
   get a live image on your phone before soldering anything.
2. Solder the button + pull-up to a scrap of perfboard or direct to wires;
   heat-shrink everything.
3. Solder the battery last (or add a switch so you can work on the device
   with the battery isolated).
4. Case it: a mint tin, a 3D-printed box, a film-canister… the art is yours.
   Leave a window for the LED (it's the status display) and let the antenna
   live outside metal enclosures.

## Gotchas checklist

- **Antenna**: the XIAO ESP32S3 needs its IPEX antenna snapped on *before*
  power-up for full range, and it should not be disconnected hot. An
  enclosure of metal = dead Wi-Fi.
- **Camera flex**: seat the OV5640 board flat and square on the connector;
  it's keyed but fragile. Avoid touching the lens; peel the protective film.
- **ESD**: touch grounded metal before handling the boards in dry weather.
  The camera lines run at 1.8 V logic levels — sensitive.
- **Strapping pins**: if you later add hardware, avoid GPIO0, 3, 45, 46 —
  they're read at boot and can brick boot-up wiring.
- **XCLK instability**: a handful of OV5640 units show noise lines at the
  default 20 MHz XCLK. If your HQ shots have horizontal banding, drop
  `cfg.xclk_freq_hz` in `firmware/src/camera_ctl.cpp` to 16 MHz and/or lower
  the gain ceiling — this matches reports on the Seeed forum.

## Power expectations (estimates until measured)

| state                       | current (typ.)      |
|-----------------------------|---------------------|
| streaming live view + Wi-Fi  | 120–180 mA          |
| idle, Wi-Fi up              | 40–80 mA            |
| deep sleep                  | 15–40 µA            |

With a 1000 mAh cell: several hours of continuous shooting/streaming, and
months of "shoot a few, sleep" usage. The firmware sleeps after a
configurable idle timeout (default 10 min; 0 = never) and wakes on the
shutter button.

## Physical controls (no screen, remember)

| gesture on the shutter button | action                                  |
|-------------------------------|-----------------------------------------|
| short press                   | take a photo (current mode)             |
| double press                  | cycle mode: HQ → RAW → MOSH → BEND      |
| long press (0.6 s)            | toggle the live stream                  |
| very long press (3 s)         | deep sleep now                          |

The LED tells you the state: steady = ready, soft double-pulse = phone
connected, slow blink = streaming, fast blink = error (e.g. camera init
failed), one double-flash = photo saved.
