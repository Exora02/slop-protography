# Housing

A parametric 3D-printed enclosure for the assembled camera. Source of truth
is [`hardware/housing/protography_case.scad`](../hardware/housing/protography_case.scad)
— every dimension is a named constant at the top of the file, and the
printable STLs in `hardware/housing/stl/` are generated from it (rerun
`export.bat` after any change; Windows users: OpenSCAD installs to
`C:\Program Files\OpenSCAD`).

The shape: a small brick camera (≈ 60 × 36 × 31 mm with the default
battery). The board stack stands vertically behind the front wall — camera
board facing forward, lens looking through a Ø11.6 mm window with a
recessed bezel — and the battery lies flat behind it. The lid is the top
face and carries the shutter hole; USB-C and microSD slots are on the same
side wall; a Ø3.2 window on the opposite wall lets the status LED glow out.

## Print the coupon FIRST

Board measurements came from Seeed's wiki, not from calipers on your parts.
Before printing the real shell, print the **coupon** and check it against
your assembled device:

| coupon feature | checks parameter | expected fit |
|---|---|---|
| Ø11.6 hole | `lens_d` | lens barrel rotates freely (AF travel) |
| Ø7.6 hole | `btn_hole` + cap press-fit | cap grips, dome proud |
| 10.4 × 4.4 slot | `usb_w/usb_h` | USB-C plugs through |
| 15.5 × 3.6 slot | `sd_w/sd_h` | card inserts and latches |
| 3 pins Ø2.0–2.4 | hole-size calibration | which pin your printer's holes fit best |

Adjust the constants, re-export, then print the parts. Also caliper-check:
total stack thickness (`stack_h` — camera board + B2B + XIAO), lens center
offset from the camera board's lower edge (`lens_off_w`), your battery
(`bat_l/bat_w/bat_t`), and where the slots actually land on your boards
(`usb_x`, `sd_x` — slot center distance from the front wall, and their
heights in the shell source).

## Print settings

| | |
|---|---|
| material | PETG recommended (handles pocket heat better than PLA); PLA fine for prototypes |
| layer height | 0.2 mm (0.12 for the cap's dome) |
| perimeters | 3 |
| infill | 15–20 %, gyroid |
| supports | none needed — print the shell **front face down**, lid and cap flat |
| orientation detail | the lens bezel recess lands on the plate side, so the visible front gets the smoothest surface |

## Parts

| file | part |
|---|---|
| `protography_shell.stl` | body: lens window, USB/SD slots, LED window, board clips, battery bay, antenna notch |
| `protography_lid.stl` | top plate: shutter hole, antenna exit, engraving; friction rim underneath |
| `protography_shutter_cap.stl` | dome cap — presses through the lid onto the tactile plunger |
| `protography_coupon.stl` | calibration plate — print first, recycle after |

## Assembly

1. Seat the board stack in the front pocket: camera board under the two
   edge clips (a strip of foam tape behind the XIAO keeps it snug), lens in
   the window.
2. Lay the battery in its floor recess; route wires through the wire gap.
3. Tape or glue the tactile button so its plunger sits just under the
   shutter hole (sticking it to the lid's underside also works — give the
   wires slack).
4. Press the cap into the hole from above.
5. Route the antenna along the interior wall (plastic is transparent to
   2.4 GHz; keep it away from the battery leads if you can), cable out
   through the rim notch.
6. Press the lid on. To open later: pry gently at a corner.

## Verification harness

`verify.scad` next to the model asserts that every cutout is actually cut
through (it exists because a drill that starts 1 mm inside the wall makes a
very convincing-looking *blind pocket* — ask the LED window). Run
`openscad -D p=N -o tmp.stl verify.scad` for N = 1..5 after changing any
parameter; every run must print `Current top level object is empty`.

## Sculpting your own

The STLs import cleanly into Blender / Tinkercad / Fusion if you want to
sculpt an organic shell over the functional bits — keep the cutout
positions, go wild elsewhere.
