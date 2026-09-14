# Effects guide

Four capture modes, all seedable for reproducibility. Modes are cycled with a
double-press of the shutter button or picked in the phone UI.

## HQ — the clean base render

5MP (2592×1944) JPEG off the sensor's own compression engine. JPEG quality
is adjustable (6–20, lower = better) in settings. The glitch slider applies
here too — intensity 1–2 on HQ is a nice "analog dust" touch.

## RAW — pristine data

Captures raw Bayer data straight off the sensor, stores the packed bytes,
and serves them wrapped in an Adobe DNG at download time. A quick VGA JPEG
preview is stored alongside so the gallery still shows something. No
glitching happens in RAW mode — the whole point is a pristine negative you
can develop on the phone/desktop (Lightroom, Darktable, RawTherapeee all
open DNG).

### RAW caveats (verify on hardware)

The esp32-camera driver's RAW output packing is under-documented and has
changed between versions. The firmware records the observed bytes-per-pixel
of each capture and the DNG writer adapts (8-bit / packed 10-bit / 16-bit
container). Two knobs exist in `firmware/src/dng.h` if your first files look
wrong in a raw developer:

- **Swapped red/blue channels** → change `DNG_CFA_PATTERN` from BGGR
  (`{2,1,1,0}`) to RGGB (`{0,1,1,2}`).
- **Blown highlights / crushed blacks** → `DNG_RAW16_WHITE_LEVEL` between
  65472 (10-bit left-justified) and 1023 (right-justified).

Also expect RAW capture to be slower (~1.5–2 s: sensor re-init twice) and
capped at UXGA 1600×1200 by RAM — not the full 5MP. A full-res 5MP raw
(>10 MB) simply doesn't fit the XIAO's 8 MB PSRAM alongside everything else.

## MOSH — datamosh for stills

Classic datamoshing is a video trick (dropping I-frames, letting motion
vectors run wild). For stills, the equivalent is corrupting the
entropy-coded scan of a JPEG: decoders resynchronize on the damaged Huffman
stream and you get the same family of artifacts — smears, tears, color
bleeds, macroblock stutters — without any codec on the device.

The engine (`firmware/src/glitch.cpp`) applies `3 + intensity × 2` seeded
operations, only inside the scan data (headers and EOI stay intact so the
file remains decodable):

| op        | artifacts                                            |
|-----------|------------------------------------------------------|
| scramble  | hard smears, corrupted macroblocks                   |
| clone     | repeated/stuttered regions                           |
| xor mask  | color shifts, block noise                            |
| sort      | frequency ramps (sorted byte runs)                   |
| bitrotate | shimmering horizontal banding                        |

Same seed + intensity = same result. Seed 0 = roll a new seed per shot
(which is itself recorded in the photo metadata and shown in the gallery).

## BEND — circuit bending the sensor

Circuit bending a toy camera means poking its analog guts until the output
goes wrong in interesting ways. Here the "pokes" are register writes through
the camera driver, randomized from a seed (`firmware/src/bend.cpp`):

- manual gain (30–120) and exposure far outside auto-control ranges,
- amplifier noise with digital denoise/bad-pixel/lens-correction switched
  off,
- white balance forced to wrong color-temperature presets,
- occasional on-sensor special effects (negative/solarize family),
- exaggerated saturation/contrast/sharpness.

Everything stays within datasheet-safe register ranges — nothing can be
damaged; it just looks beautifully wrong. After the shot, the clean pipeline
is restored. BEND also moshes the result (default intensity 5) — set the
glitch slider to 0 before shooting for pure sensor abuse.

## Ideas parked for v2

- pixel-sort and channel-shift effects in the browser (the live canvas makes
  a great preview surface) with "export" compositing on the phone
- exposure bracketing in HQ
- animated GIF export of a seeded mosh sequence
- deterministic "director's cut": replay a whole session's seeds
