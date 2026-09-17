// ---------------------------------------------------------------------------
// Protography — parametric housing
//
// A little brick camera for the XIAO ESP32S3 Sense + camera board stack.
// Geometry axis convention (mm):
//   +X  depth  : front (lens) wall at x=0 ... back wall
//   +Y  width  : -Y wall carries the USB-C and microSD cutouts
//   +Z  up     : shutter button on the top face
// The board stack stands vertically, camera board facing the front wall;
// the battery lies flat behind it.
//
// Parts (set `part` or use -D on the command line):
//   0 shell      main body        — print front face DOWN
//   1 lid        top plate        — print flat
//   2 cap        shutter cap      — print flat
//   3 coupon     calibration test — print FIRST, check every cutout, adjust
//   4 assembly   everything laid out for preview
// (numeric so -D part=0 works from Windows shells without quoting)
//
// Every dimension you may need to tweak is below. The coupon exists so you
// never have to guess: print it, test-fit, caliper, adjust, then print the
// real parts.
// ---------------------------------------------------------------------------

/* ---------------- boards (VERIFY WITH CALIPERS via coupon) ------------- */
stack_h = 9.4;      // total stack thickness: camera board + B2B + XIAO
stack_w = 21.0;     // board edge running vertically once installed
stack_d = 17.8;     // board edge running along the enclosure width (Y)

board_bot = 3.5;    // camera board lower edge above the interior floor
lens_off_w = 10.5;  // lens center, measured up from the camera board lower edge
lens_y   = 0;       // lens lateral offset from the interior width center
lens_d   = 11.6;    // through-hole for the lens barrel (focus ring travel!)

usb_w = 10.4;  usb_h = 4.4;   usb_x = 9.0;   // USB-C cutout (center, on -Y wall)
sd_w  = 15.5;  sd_h  = 3.6;   sd_x  = 2.6;   // microSD cutout (center, on -Y wall)
led_d = 3.2;      // LED glow window diameter on the +Y wall
led_x = 4.5;      // LED window position along the front wall (from x=wall)
led_z = 16;       // LED window height above the interior floor

/* ---------------- battery ------------------------------------------------ */
bat_l = 40;  bat_w = 30;  bat_t = 7.5;        // e.g. 803040 class LiPo
wire_gap = 6;                                 // slack between stack and battery

/* ---------------- shell -------------------------------------------------- */
wall    = 2.4;
floor_t = 2.4;
lid_t   = 2.2;
clear   = 0.5;      // general clearance around parts
snap_f  = 0.25;     // lid friction-fit tolerance
rr      = 6;        // outer corner radius
$fn     = 72;

/* ---------------- shutter ------------------------------------------------ */
btn_hole = 7.6;     // hole in the top face (over the tactile button)
btn_x    = 14;      // button position on the top face
cap_d    = 11.6;    // shutter cap: press-fit sleeve + dome
cap_grip = 1.2;     // sleeve wall the hole grips

/* ---------------- derived interior --------------------------------------- */
in_l = stack_h + clear + wire_gap + bat_l + 2*clear;   // interior depth (X)
in_w = max(bat_w + 2*clear, 24);                        // interior width (Y)
in_h = max(stack_w + 2*clear, bat_t + 2) + 4;           // interior height (Z)

out_l = in_l + wall + wall;
out_w = in_w + 2*wall;
out_h = in_h + floor_t + lid_t;

echo(str("outer size: ", out_l, " x ", out_w, " x ", out_h, " mm"));

/* ---------------- helpers ------------------------------------------------ */
module rbox(size, r) {           // box with rounded vertical edges
    hull() {
        translate([r, r, 0])           cylinder(r=r, h=size[2]);
        translate([size[0]-r, r, 0])   cylinder(r=r, h=size[2]);
        translate([r, size[1]-r, 0])   cylinder(r=r, h=size[2]);
        translate([size[0]-r, size[1]-r, 0]) cylinder(r=r, h=size[2]);
    }
}

/* stack pocket: camera board edge clips + rear stop for the XIAO.
   Local frame: origin at interior floor, front-bottom corner of the cavity. */
module stack_retention() {
    clip_t = 1.6;
    board_cy = in_w/2 + lens_y;                 // board center line in Y
    // two prongs gripping the camera board's lower and upper edges
    for (z = [board_bot - clip_t, board_bot + stack_w])
        translate([0.6, board_cy - (stack_d + 1.6)/2, z])
            cube([clip_t, stack_d + 1.6, clip_t]);
    // rear stop behind the XIAO
    translate([stack_h + clear + 0.8, board_cy - bat_w/2, 0])
        cube([2, bat_w, stack_w - 2]);
}

/* ---------------- parts --------------------------------------------------- */

module shell() difference() {
    union() {
        rbox([out_l, out_w, out_h - lid_t], rr);
        // board stack retention: edge clips + rear stop, grown from the floor
        translate([wall, wall, floor_t]) stack_retention();
    }
    // cavity, open at the back
    translate([wall, wall, floor_t]) rbox([in_l, in_w, in_h + 1], rr - wall/2);
    // battery floor recess (keeps the cell from sliding)
    translate([wall + stack_h + clear + wire_gap, wall + (in_w - bat_w)/2, floor_t])
        cube([bat_l + 2*clear, bat_w, 0.6]);

    // ---- front: lens window + recessed bezel ----
    lz = floor_t + board_bot + lens_off_w;      // lens center height
    translate([-1, wall + in_w/2 + lens_y, lz])
        rotate([0, 90, 0]) cylinder(d=lens_d, h=wall + 2);
    translate([-0.8, wall + in_w/2 + lens_y, lz])
        rotate([0, 90, 0]) cylinder(d=lens_d + 11, h=1.0);

    // ---- -Y wall: USB-C and microSD (rounded-end slots) ----
    translate([wall + usb_x, -1, floor_t + 8.6])
        rotate([-90, 0, 0]) hull() {
            cylinder(d=usb_h, h=wall + 2);
            translate([usb_w - usb_h, 0, 0]) cylinder(d=usb_h, h=wall + 2);
        }
    translate([wall + sd_x, -1, floor_t + 2.2])
        rotate([-90, 0, 0]) hull() {
            cylinder(d=sd_h, h=wall + 2);
            translate([sd_w - sd_h, 0, 0]) cylinder(d=sd_h, h=wall + 2);
        }

    // ---- +Y wall: LED glow window (drill from the INNER face outward) ----
    translate([wall + led_x, wall + in_w - 1, floor_t + led_z])
        rotate([-90, 0, 0]) cylinder(d=led_d, h=wall + 2);

    // ---- top rim: antenna cable exit under the lid ----
    translate([out_l - wall - 1, wall + in_w/2 - 6, out_h - lid_t - 1])
        cube([wall + 2, 12, 1.6]);
}

module lid() {
    difference() {
        rbox([out_l, out_w, lid_t], rr);
        translate([1.2, 1.2, -0.01]) rbox([out_l - 2.4, out_w - 2.4, lid_t + 0.5], rr - 1.2);
        // shutter hole (the lid is the top face; the button lives in the
        // cavity under it, the cap plugs in from above)
        translate([wall + btn_x, wall + in_w/2, -1]) cylinder(d=btn_hole, h=lid_t + 2);
        // antenna cable exit, aligned with the shell rim notch
        translate([out_l - wall - 1, wall + in_w/2 - 6, -1]) cube([wall + 2, 12, lid_t + 2]);
        // engraving: raised on the inside = debossed on the outside when
        // printed flat; flip sign depth to taste
        translate([out_l/2, out_w/2 - 7, lid_t - 0.6])
            linear_extrude(1.0)
                text("PROTOGRAPHY", size=4.6, halign="center",
                     font="Liberation Sans:style=Bold", spacing=1.15);
        translate([out_l/2, out_w/2 + 6, lid_t - 0.6])
            linear_extrude(1.0)
                text("P. graphica · habitat: pockets", size=2.6, halign="center",
                     font="Liberation Sans:style=Italic");
    }
    // friction rim
    translate([wall + snap_f, wall + snap_f, lid_t])
        difference() {
            rbox([in_l - 2*snap_f, in_w - 2*snap_f, 2.6], rr - wall/2);
            translate([1.8, 1.8, -0.5]) rbox([in_l - 2*snap_f - 3.6, in_w - 2*snap_f - 3.6, 4], rr - wall);
        }
}

module cap() {
    // sleeve plugs into the top-face hole; dome is the button you feel
    sleeve_h = 3.2;
    difference() {
        union() {
            cylinder(d=btn_hole - 0.5, h=sleeve_h);
            // dome: upper hemisphere sitting on the sleeve
            translate([0, 0, sleeve_h])
                difference() {
                    sphere(d=cap_d);
                    translate([-cap_d, -cap_d, -cap_d - 0.01]) cube(2*cap_d);
                }
            translate([0, 0, sleeve_h - 0.01])
                cylinder(d1=btn_hole - 0.5, d2=cap_d*0.72, h=1.2);
        }
        // hollow the underside so it presses on the plunger rim, not center
        translate([0, 0, sleeve_h - 1.0]) cylinder(d=btn_hole - 2.6, h=6);
    }
}

module coupon() {
    // Calibration plate: print this first, test every cutout on the real
    // assembled stack, adjust the parameters above, then print the parts.
    difference() {
        rbox([70, 34, 2.4], 3);
        // lens hole
        translate([12, 17, -1]) cylinder(d=lens_d, h=5);
        // shutter hole + cap fit
        translate([30, 24, -1]) cylinder(d=btn_hole, h=5);
        // USB slot
        translate([42, 9, -1]) rotate([-90, 0, 0]) hull() {
            translate([0, 0, -0.1]) cylinder(d=usb_h, h=5);
            translate([usb_w - usb_h, 0, -0.1]) cylinder(d=usb_h, h=5);
        }
        // SD slot
        translate([42, 25, -1]) rotate([-90, 0, 0]) hull() {
            translate([0, 0, -0.1]) cylinder(d=sd_h, h=5);
            translate([sd_w - sd_h, 0, -0.1]) cylinder(d=sd_h, h=5);
        }
        // wall-thickness gauge: steps of 1.8 / 2.4 / 3.0
        for (i = [0:2])
            translate([56 + i*4.2, 6, -1]) cylinder(d=2.0 + i*0.2, h=5);
        translate([6, 4, -0.01]) linear_extrude(2.5)
            text("calibrate me", size=3.2, font="Liberation Sans:style=Italic");
    }
}

module assembly() {
    shell();
    translate([0, out_w + 8, 0]) lid();
    translate([out_l + 14, out_w/2, 0]) cap();
    translate([0, out_w + 8 + out_w + 6, 0]) coupon();
}

part = 4;
if (part == 0) shell();
else if (part == 1) lid();
else if (part == 2) cap();
else if (part == 3) coupon();
else assembly();
