# Housing

A parametric 3D-printed enclosure, modeled around the **actual assembled
device** (camera head on its FPC flex, XIAO+Sense stack, direct-soldered
battery, SMD shutter switch). Source of truth:
[`hardware/housing/protography_case.scad`](../hardware/housing/protography_case.scad)
— every dimension is a named constant; regenerate the STLs with `export.bat`
(OpenSCAD at `C:\Program Files\OpenSCAD`).

**STLs ready for the slicer** are in
[`hardware/housing/stl/`](../hardware/housing/stl/) — no conversion needed,
that's already the slicer-native format:

| file | part |
|---|---|
| `protography_shell.stl` | body: **square camera window**, camera-head posts, lying-stack stops, switch pedestal, USB slot, LED window, battery bay |
| `protography_lid.stl` | top plate with the **compliant membrane shutter** (one connected solid: disc + paddle + stub), engraving, friction rim |
| `protography_memtest.stl` | standalone membrane — print first and feel the click |
| `protography_coupon.stl` | calibration plate — print first, caliper everything |

## The shutter (v2 redesign)

The photo of the build showed a **small SMD tactile on flying wires** —
nothing for a press-fit cap to bear against. So the shutter is now a fully
printed compliant mechanism, nothing to solder or glue:

- the SMD switch **wedges into a square pocket** on top of a printed
  pedestal (friction fit; trapped permanently once the lid is on);
- the lid carries a **membrane key**: Ø16 paddle on a 0.7 mm flexure web,
  with a Ø7 stub underneath that reaches through the lid and rests ~0.3 mm
  above the switch plunger — press the paddle, the web flexes, the plunger
  clicks;
- feel it first: print `protography_memtest.stl` (2 g of plastic, 15 min)
  — if you want it softer/thicker travel, tune `web_t` (0.6–1.0) and
  reprint just that.

## Layout

- **Camera head** (on the orange FPC) shows its square face through the
  square front window — corner posts stop it at the right depth, rails
  support its edges, a foam pad behind keeps it snug, and the FPC bends
  freely toward the main stack. Give the flex its natural bend radius —
  don't crease it.
- **XIAO + Sense stack lies flat** on floor stops, USB-C facing the slotted
  side wall.
- **microSD**: insert the card once before closing — there is deliberately
  no slot in the wall. (If that changes, `sd_w/sd_h/sd_x/sd_z` params can
  come back.)
- **Battery** defaults sized to the real cell (~25 x 38 x 5.5): overall the
  body is now a slim bar (~74 x 31 x 20 mm).
- **Battery** lies flat behind the stack, wires through the mid gap (tape
  the solder joints — they're the weak point of a direct-soldered pouch).
- **Antenna** along an interior wall (plastic is RF-transparent; keep it
  away from battery leads), cable out through the rim notch at the back.

## Print the coupon FIRST

| coupon feature | checks parameter | expected fit |
|---|---|---|
| Ø11.6 hole | `lens_d` | lens barrel rotates freely (AF travel) |
| Ø9.5 hole | `btn_clear` | stub slides, no scuffing |
| 10.4 × 4.4 slot | `usb_w/usb_h` | USB-C plugs through |
| 15.5 × 3.6 slot | `sd_w/sd_h` | card inserts and latches |
| 3 pins Ø2.0–2.4 | printer hole calibration | which size fits your machine |
| square pocket block | `sw_w` | SMD switch wedges in and stays |

Then caliper the real parts and adjust before printing the big pieces:

- camera head: `cam_w`, `cam_h`, `cam_t`
- stack thickness: `stack_t` (XIAO + B2B + Sense board)
- battery: `bat_l`, `bat_w`, `bat_t`
- slot positions along the wall: `usb_x`, `sd_x` and their heights
  `usb_z`, `sd_z`
- switch body: `sw_w`, `sw_t`, `sw_pl`

## Print settings

| | |
|---|---|
| material | PETG recommended (pocket heat, flex fatigue); PLA fine for drafts |
| layer height | 0.2 mm general; **0.12 mm for the lid and memtest** (the 0.7 web needs clean layers) |
| perimeters | 3 |
| infill | 15–20 % gyroid |
| supports | none — shell front-face down, lid and coupon flat, memtest flat |

## Assembly order

1. Camera head into the front pocket (lens through the window), FPC bent
   back toward the stack — a foam pad behind it keeps it snug.
2. Stack into the floor stops (USB/SD toward the slotted wall).
3. Battery in its bay, wires through the gap, joints taped.
4. SMD switch wedged into the pedestal pocket (test the click with a
   toothpick before closing).
5. Antenna placed, cable through the rim notch.
6. Lid on: align the membrane over the pedestal, press the friction rim
   home. Pry gently at a corner to open.

## Verification harness

`verify.scad` asserts every cutout (lens, USB, SD, LED, lid stub clearance)
is a true through-hole — run `openscad -D p=N -o tmp.stl verify.scad` for
N = 1..5 after changing parameters; each must report
`Current top level object is empty`. It exists because a drill that starts
inside the wall makes a very convincing-looking *blind pocket*.

## Sculpting your own

The STLs import cleanly into Blender / Tinkercad / Fusion — keep the
cutout positions and the pedestal/membrane alignment, go wild elsewhere.
