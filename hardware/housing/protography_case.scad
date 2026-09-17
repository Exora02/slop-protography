// ---------------------------------------------------------------------------
// Protography — parametric housing  (v3)
//
// Layout for the real assembled device:
//   - square camera window in the front wall sized to the OV5640 head (the
//     module face is visible from outside; interior posts stop it, foam
//     behind keeps it snug, FPC bends back freely);
//   - XIAO + Sense stack lies flat, USB-C slot on the -Y wall;
//   - microSD card is inserted ONCE before closing — no slot, by design;
//   - battery (25 x 38 x 5.5 class pouch) flat behind the stack;
//   - shutter: SMD switch wedged in a pedestal pocket + a compliant
//     membrane key that is ONE connected solid with the lid (disc flexure,
//     all unions volumetric — no floating pieces in the slicer).
//
// Axes (mm): +X front(window, x=0) -> back; +Y width (-Y wall: USB slot);
// +Z up (lid on top with the membrane shutter).
//
// Parts (-D part=N, numeric for Windows shells):
//   0 shell      main body        — print front face DOWN
//   1 lid        top plate w/ membrane shutter — print flat, 0.12 layers
//   2 memtest    standalone membrane click test — print first, feel it
//   3 coupon     calibration plate — print first, caliper everything
//   4 assembly   preview layout
// ---------------------------------------------------------------------------

/* ------------- camera head on flex (CALIPER-CHECK via coupon) ---------- */
cam_w = 11.0;      // camera head width (Y) — square module
cam_h = 11.0;      // camera head height (Z)
cam_t = 5.0;       // camera head depth (X)
winclr = 0.6;      // window clearance around the module
win_r = 1.5;       // rounded corner radius of the square window

/* ------------- main stack lying flat ------------------------------------ */
stack_l = 21.0;    // XIAO(+Sense) length along X
stack_w = 17.8;    // stack width along Y
stack_t = 9.4;     // stack total thickness (Z) — caliper!
stack_gap = 2.0;   // gap between camera pocket and stack front edge

usb_w = 10.4;  usb_h = 4.4;   usb_x = 16.0;  // USB-C slot (center, -Y wall)
usb_z = 7.0;                                // slot center above interior floor

/* ------------- battery (user's cell: ~1.4x XIAO wide, ~2x long) --------- */
bat_l = 38;  bat_w = 25;  bat_t = 5.5;
wire_gap = 4;                                // wires + slack mid-gap

/* ------------- compliant shutter ---------------------------------------- */
sw_w = 6.8;        // SMD switch body, square, drop-in pocket width
sw_t = 3.6;        // switch body height (above pocket floor)
sw_pl = 0.8;       // plunger travel height
sw_pre = 0.3;      // pre-load gap between stub and plunger at rest
btn_x = 30;        // shutter position from the front wall (interior)
btn_y = 0;         // lateral offset from interior width center
mem_d = 16;        // membrane disc diameter (flexure + paddle in one)
web_t = 0.7;       // disc thickness — the flexure (0.6–1.0 to tune feel)
paddle_t = 1.6;    // raised paddle thickness on top of the disc
stub_d = 7.0;      // stub that presses the plunger
stub_h = 2.2;      // stub reach below the lid underside

/* ------------- shell ----------------------------------------------------- */
wall    = 2.4;
floor_t = 2.4;
lid_t   = 2.2;
clear   = 0.5;
snap_f  = 0.25;
rr      = 6;
$fn     = 72;

/* LED glow window (+Y wall) — glow leaks from under the lying stack */
led_d = 3.2;  led_x = 12;  led_z = 4.5;

/* ------------- derived --------------------------------------------------- */
stack_x0 = cam_t + stack_gap;
bat_x0 = stack_x0 + stack_l + clear + wire_gap;
in_l = bat_x0 + bat_l + 2*clear;
in_w = max(bat_w + 2*clear, stack_w + 4, cam_w + 4);
in_h = max(cam_h + 2.5, stack_t + 3.5, bat_t + 2.5) + 2;
mem_hole = mem_d - 2;      // lid hole under the disc: stub travel + flex

out_l = in_l + wall + wall;
out_w = in_w + 2*wall;
out_h = in_h + floor_t + lid_t;

echo(str("outer size: ", out_l, " x ", out_w, " x ", out_h, " mm"));

module rbox(size, r) {
    hull() {
        translate([r, r, 0])                cylinder(r=r, h=size[2]);
        translate([size[0]-r, r, 0])        cylinder(r=r, h=size[2]);
        translate([r, size[1]-r, 0])        cylinder(r=r, h=size[2]);
        translate([size[0]-r, size[1]-r, 0]) cylinder(r=r, h=size[2]);
    }
}

function plunger_z() = out_h - lid_t - stub_h - sw_pre;
function pedestal_h() = plunger_z() - sw_pl - sw_t - floor_t;

module shell() difference() {
    union() {
        rbox([out_l, out_w, out_h - lid_t], rr);
        translate([wall, wall, floor_t]) interior_features();
    }
    translate([wall, wall, floor_t]) rbox([in_l, in_w, in_h + 1], rr - wall/2);
    translate([wall + bat_x0, wall + (in_w - bat_w)/2, floor_t])
        cube([bat_l + 2*clear, bat_w, 0.6]);            // battery recess

    // ---- front: SQUARE camera window + recessed frame ----
    translate([-1, wall + in_w/2, floor_t + in_h/2])
        rotate([0, 90, 0]) linear_extrude(height = wall + 2)
            offset(r = win_r) square([cam_h + winclr, cam_w + winclr], center = true);
    translate([-0.8, wall + in_w/2, floor_t + in_h/2])
        rotate([0, 90, 0]) linear_extrude(height = 1.0)
            offset(r = win_r + 3) square([cam_h + winclr + 6, cam_w + winclr + 6], center = true);

    // ---- -Y wall: USB-C slot (rounded ends) ----
    translate([wall + usb_x, -1, floor_t + usb_z])
        rotate([-90, 0, 0]) hull() {
            cylinder(d=usb_h, h=wall + 2);
            translate([usb_w - usb_h, 0, 0]) cylinder(d=usb_h, h=wall + 2);
        }

    // ---- +Y wall: LED glow window (drilled from the INNER face outward) ----
    translate([wall + led_x, wall + in_w - 1, floor_t + led_z])
        rotate([-90, 0, 0]) cylinder(d=led_d, h=wall + 2);

    // ---- top rim: antenna cable exit under the lid ----
    translate([out_l - wall - 1, wall + in_w/2 - 6, out_h - lid_t - 1])
        cube([wall + 2, 12, 1.6]);
}

// Interior furniture, grown from the cavity floor (local frame).
module interior_features() {
    cy = in_w/2;
    // camera head: corner posts stop the module at the right depth
    for (y = [cy - cam_w/2 - 1.2, cy + cam_w/2 + 0.4])
        translate([cam_t + 0.6, y, in_h/2 - cam_h/2 - 1])
            cube([1.6, 0.8, cam_h + 2]);
    // rails above/below the module for side support
    rail_t = 1.8;
    for (z = [in_h/2 - cam_h/2 - rail_t, in_h/2 + cam_h/2])
        translate([0.4, cy - cam_w/2 - 1, z])
            cube([cam_t + 1.0, cam_w + 2, rail_t]);

    // lying stack: front stop, side rails, rear stop
    translate([stack_x0 - 2, cy - stack_w/2 - 1, 0])
        cube([2, stack_w + 2, 2.5]);
    for (y = [cy - stack_w/2 - 1.6, cy + stack_w/2 + 0.6])
        translate([stack_x0, y, 0])
            cube([stack_l + 1, 1.0, 2.5]);
    translate([stack_x0 + stack_l + 1, cy - stack_w/2 - 1, 0])
        cube([2, stack_w + 2, 2.5]);

    // SMD switch pedestal with drop-in pocket (trapped once lid is on)
    translate([btn_x - 5.5, cy + btn_y - 5.5, 0])
        difference() {
            cube([11, 11, pedestal_h()]);
            translate([0.6, 0.6, pedestal_h() - sw_t])
                cube([sw_w, sw_w, sw_t + 1]);
        }
}

module lid() {
    difference() {
        rbox([out_l, out_w, lid_t], rr);
        translate([1.2, 1.2, -0.01]) rbox([out_l - 2.4, out_w - 2.4, lid_t + 0.5], rr - 1.2);
        // membrane travel hole (stub clearance + disc flex area)
        translate([wall + btn_x, wall + in_w/2 + btn_y, -1])
            cylinder(d=mem_hole, h=lid_t + 2);
        // antenna cable exit, aligned with the shell rim notch
        translate([out_l - wall - 1, wall + in_w/2 - 6, -1])
            cube([wall + 2, 12, lid_t + 2]);
        // engraving
        translate([out_l/2, out_w/2 - 7, lid_t - 0.6])
            linear_extrude(1.0)
                text("PROTOGRAPHY", size=4.6, halign="center",
                     font="Liberation Sans:style=Bold", spacing=1.15);
        translate([out_l/2, out_w/2 + 6, lid_t - 0.6])
            linear_extrude(1.0)
                text("P. graphica · habitat: pockets", size=2.6, halign="center",
                     font="Liberation Sans:style=Italic");
    }
}

// The compliant shutter — ONE connected solid, every union volumetric:
// disc overlaps the plate rim (0.4 mm), paddle overlaps the disc, stub
// overlaps the disc from below. Nothing floats; slicers see one body.
module membrane(bx, by, plate_top) {
    translate([bx, by, plate_top - 0.4]) cylinder(d=mem_d, h=web_t + 0.4);
    translate([bx, by, plate_top - 0.4 + web_t]) cylinder(d=mem_d - 1.6, h=paddle_t);
    translate([bx, by, plate_top - stub_h]) cylinder(d=stub_d, h=stub_h + 0.3);
}

module lid_full() {
    lid();
    membrane(wall + btn_x, wall + in_w/2 + btn_y, lid_t);
    // friction rim, overlapping the plate by 1 mm so it's one body too
    translate([wall + snap_f, wall + snap_f, lid_t - 1.0])
        difference() {
            rbox([in_l - 2*snap_f, in_w - 2*snap_f, 3.6], rr - wall/2);
            translate([1.8, 1.8, -0.5]) rbox([in_l - 2*snap_f - 3.6, in_w - 2*snap_f - 3.6, 5], rr - wall);
        }
}

// Standalone click test: same disc construction on its own plate.
module memtest() {
    difference() {
        cylinder(d=mem_d + 14, h=2.2);
        cylinder(d=mem_hole, h=5);                    // travel + flex room
    }
    membrane(0, 0, 2.2);
    translate([0, 0, 2.2 + web_t + paddle_t + 1.6]) linear_extrude(0.8)
        text("click", size=4, halign="center", font="Liberation Sans:style=Italic");
}

module coupon() {
    difference() {
        rbox([70, 34, 2.4], 3);
        // square camera window test
        translate([12, 17, -1]) linear_extrude(5)
            offset(r=win_r) square([cam_h + winclr, cam_w + winclr], center=true);
        // membrane travel hole
        translate([30, 24, -1]) cylinder(d=mem_hole, h=5);
        // USB slot
        translate([42, 9, -1]) rotate([-90, 0, 0]) hull() {
            translate([0, 0, -0.1]) cylinder(d=usb_h, h=5);
            translate([usb_w - usb_h, 0, -0.1]) cylinder(d=usb_h, h=5);
        }
        for (i = [0:2])                                // printer hole gauge
            translate([56 + i*4.2, 6, -1]) cylinder(d=2.0 + i*0.2, h=5);
        translate([6, 4, -0.01]) linear_extrude(2.5)
            text("calibrate me", size=3.2, font="Liberation Sans:style=Italic");
    }
    // switch pocket sample
    translate([58, 24, 0]) difference() {
        cube([11, 11, 6]);
        translate([(11 - sw_w)/2, (11 - sw_w)/2, 2.4]) cube([sw_w, sw_w, 6]);
    }
}

module assembly() {
    shell();
    translate([0, out_w + 10, lid_t + web_t + paddle_t]) rotate([180, 0, 0]) lid_full();
    translate([out_l + 20, out_w/2, 0]) memtest();
    translate([0, out_w + 10 + out_w + 10, 0]) coupon();
}

part = 4;
if (part == 0) shell();
else if (part == 1) lid_full();
else if (part == 2) memtest();
else if (part == 3) coupon();
else assembly();
