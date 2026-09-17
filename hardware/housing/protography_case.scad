// ---------------------------------------------------------------------------
// Protography — parametric housing  (v2: flex-camera + SMD button layout)
//
// Layout for the real assembled device (see docs/HOUSING.md):
//   - the OV5640 camera HEAD rides a short FPC flex: it sits alone in a
//     pocket right behind the front lens window;
//   - the XIAO + Sense board stack lies FLAT in the cavity, USB-C and the
//     microSD slot facing the -Y wall;
//   - the battery lies flat behind the stack (wires through the mid gap);
//   - the shutter is an SMD tactile switch wedged in a pedestal pocket;
//     the lid carries a compliant membrane key that presses its plunger.
//
// Axes (mm): +X front(lens, x=0) -> back; +Y width (-Y wall: USB + SD
// slots); +Z up (lid on top, membrane shutter on the lid).
//
// Parts (-D part=N, numeric for Windows shells):
//   0 shell      main body        — print front face DOWN
//   1 lid        top plate w/ membrane shutter — print flat, 0.12 layers
//   2 memtest    standalone membrane click test — print first, feel it
//   3 coupon     calibration plate — print first, caliper everything
//   4 assembly   preview layout
// ---------------------------------------------------------------------------

/* ------------- camera head on flex (CALIPER-CHECK via coupon) ---------- */
lens_d = 11.6;     // window for the lens barrel (AF focus-ring travel)
lens_z = 8.5;      // lens center height above the interior floor
lens_y = 0;        // lateral offset from the interior width center
cam_w = 11.0;      // camera head width (Y) + clearance
cam_h = 11.0;      // camera head height (Z) + clearance
cam_t = 5.0;       // camera head depth (X)

/* ------------- main stack lying flat ------------------------------------ */
stack_l = 21.0;    // XIAO(+Sense) length along X
stack_w = 17.8;    // stack width along Y
stack_t = 9.4;     // stack total thickness (Z) — caliper!
stack_gap = 2.5;   // gap between camera pocket and stack front edge

usb_w = 10.4;  usb_h = 4.4;   usb_x = 18.0;  // USB-C slot (center, -Y wall)
usb_z = 7.0;                                // slot center above interior floor
sd_w  = 15.5;  sd_h  = 3.6;   sd_x  = 9.0;   // microSD slot (center, -Y wall)
sd_z  = 5.0;

/* ------------- battery --------------------------------------------------- */
bat_l = 40;  bat_w = 30;  bat_t = 7.5;       // pouch cell, caliper!
wire_gap = 5;                               // wires + slack between stack and cell

/* ------------- compliant shutter ---------------------------------------- */
sw_w = 6.8;        // SMD switch body, square, drop-in pocket width
sw_t = 3.6;        // switch body height (above pocket floor)
sw_pl = 0.8;       // plunger travel height
sw_pre = 0.3;      // pre-load gap between stub and plunger at rest
btn_x = 34;        // shutter position from the front wall (interior)
btn_y = 0;         // lateral offset from interior width center
mem_d = 16;        // membrane paddle diameter
web_t = 0.7;       // flexure web thickness (≈3 layers @0.2, or 5-6 @0.12)
paddle_t = 1.6;    // paddle thickness
stub_d = 7.0;      // stub under the paddle that presses the plunger
stub_h = 2.2;      // stub reach below the lid underside
btn_clear = 9.5;   // clearance hole through the lid (stub passes)

/* ------------- shell ----------------------------------------------------- */
wall    = 2.4;
floor_t = 2.4;
lid_t   = 2.2;
clear   = 0.5;
snap_f  = 0.25;
rr      = 6;
$fn     = 72;

/* LED glow window (+Y wall) — glow leaks from under the lying stack */
led_d = 3.2;  led_x = 14;  led_z = 4.5;

/* ------------- derived --------------------------------------------------- */
stack_x0 = cam_t + stack_gap;                       // stack front edge (X)
bat_x0 = stack_x0 + stack_l + clear + wire_gap;     // battery front edge
in_l = bat_x0 + bat_l + 2*clear;                    // interior depth (X)
in_w = max(bat_w + 2*clear, stack_w + 4, cam_w + 4);// interior width (Y)
in_h = max(cam_h + 2.5, stack_t + 3.5, bat_t + 2.5) + 2; // interior height (Z)

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

// z of the switch plunger tip once wedged in its pedestal pocket
function plunger_z() = out_h - lid_t - stub_h - sw_pre;
// pedestal height so the pocket floor puts the plunger at that z
function pedestal_h() = plunger_z() - sw_pl - sw_t - floor_t;

module shell() difference() {
    union() {
        rbox([out_l, out_w, out_h - lid_t], rr);
        translate([wall, wall, floor_t]) interior_features();
    }
    // cavity, open at the top (the lid closes it)
    translate([wall, wall, floor_t]) rbox([in_l, in_w, in_h + 1], rr - wall/2);
    // battery floor recess
    translate([wall + bat_x0, wall + (in_w - bat_w)/2, floor_t])
        cube([bat_l + 2*clear, bat_w, 0.6]);

    // ---- front: lens window + recessed bezel ----
    translate([-1, wall + in_w/2 + lens_y, floor_t + lens_z])
        rotate([0, 90, 0]) cylinder(d=lens_d, h=wall + 2);
    translate([-0.8, wall + in_w/2 + lens_y, floor_t + lens_z])
        rotate([0, 90, 0]) cylinder(d=lens_d + 11, h=1.0);

    // ---- -Y wall: USB-C and microSD slots (rounded ends) ----
    translate([wall + usb_x, -1, floor_t + usb_z])
        rotate([-90, 0, 0]) hull() {
            cylinder(d=usb_h, h=wall + 2);
            translate([usb_w - usb_h, 0, 0]) cylinder(d=usb_h, h=wall + 2);
        }
    translate([wall + sd_x, -1, floor_t + sd_z])
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

// Interior furniture, grown from the cavity floor (local frame).
module interior_features() {
    // camera head pocket: rails above/below the head, corner posts behind
    rail_t = 1.8;
    cy = in_w/2 + lens_y;
    for (z = [lens_z - cam_h/2 - rail_t, lens_z + cam_h/2])
        translate([0.4, cy - cam_w/2 - 1, z])
            cube([cam_t + 1.6, cam_w + 2, rail_t]);
    for (y = [cy - cam_w/2 - 1.2, cy + cam_w/2 + 0.4])
        translate([cam_t + 0.8, y, lens_z - cam_h/2 - 1])
            cube([1.6, 0.8, cam_h + 2]);

    // lying stack: front stop, side rails, rear stop
    translate([stack_x0 - 2, cy - stack_w/2 - 1, 0])
        cube([2, stack_w + 2, 2.5]);
    for (y = [cy - stack_w/2 - 1.6, cy + stack_w/2 + 0.6])
        translate([stack_x0, y, 0])
            cube([stack_l + 1, 1.0, 2.5]);
    translate([stack_x0 + stack_l + 1, cy - stack_w/2 - 1, 0])
        cube([2, stack_w + 2, 2.5]);

    // SMD switch pedestal with drop-in pocket (switch trapped once lid is on)
    translate([btn_x - 5.5, cy + btn_y - 5.5, 0])
        difference() {
            cube([11, 11, pedestal_h()]);
            translate([0.6, 0.6, pedestal_h() - sw_t])
                cube([sw_w + 1.2 - 1.2, sw_w + 1.2 - 1.2, sw_t + 1]);
            // note: pocket is (sw_w)x(sw_w) after the 0.6 inset on x/y
        }
}

module lid() difference() {
    rbox([out_l, out_w, lid_t], rr);
    translate([1.2, 1.2, -0.01]) rbox([out_l - 2.4, out_w - 2.4, lid_t + 0.5], rr - 1.2);
    // shutter clearance hole (the membrane's stub reaches through it)
    translate([wall + btn_x, wall + in_w/2 + btn_y, -1]) cylinder(d=btn_clear, h=lid_t + 2);
    // antenna cable exit, aligned with the shell rim notch
    translate([out_l - wall - 1, wall + in_w/2 - 6, -1]) cube([wall + 2, 12, lid_t + 2]);
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
module lid_additions() {
    // compliant membrane shutter: web annulus + paddle + underside stub
    bx = wall + btn_x; by = wall + in_w/2 + btn_y;
    difference() {
        translate([bx, by, lid_t]) cylinder(d=mem_d + 2.6, h=web_t);
        translate([bx, by, lid_t - 1]) cylinder(d=mem_d - 2.2, h=web_t + 2);
    }
    translate([bx, by, lid_t + web_t]) cylinder(d=mem_d, h=paddle_t);
    translate([bx, by, lid_t - stub_h]) cylinder(d=stub_d, h=stub_h);
    // finger ring on the paddle
    translate([bx, by, lid_t + web_t]) difference() {
        cylinder(d=mem_d - 1.5, h=paddle_t);
        cylinder(d=mem_d - 4.5, h=paddle_t + 1);
    }
    // friction rim
    translate([wall + snap_f, wall + snap_f, lid_t])
        difference() {
            rbox([in_l - 2*snap_f, in_w - 2*snap_f, 2.6], rr - wall/2);
            translate([1.8, 1.8, -0.5]) rbox([in_l - 2*snap_f - 3.6, in_w - 2*snap_f - 3.6, 4], rr - wall);
        }
}
module lid_full() { lid(); lid_additions(); }

// Standalone click test: print this alone to feel the membrane before
// committing the whole lid (try 0.2 and 0.12 layer heights).
module memtest() {
    difference() {
        cylinder(d=mem_d + 14, h=2.2);
        cylinder(d=mem_d - 2, h=5);          // travel room under the web
    }
    difference() {
        cylinder(d=mem_d + 2.6, h=2.2 + web_t);
        translate([0, 0, 2.2 - 1]) cylinder(d=mem_d - 2.2, h=web_t + 2);
    }
    translate([0, 0, 2.2 + web_t]) cylinder(d=mem_d, h=paddle_t);
    translate([0, 0, 2.2 + web_t]) difference() {
        cylinder(d=mem_d - 1.5, h=paddle_t);
        cylinder(d=mem_d - 4.5, h=paddle_t + 1);
    }
    translate([0, 0, 2.2 + web_t + paddle_t + 1.6]) linear_extrude(0.8)
        text("click", size=4, halign="center", font="Liberation Sans:style=Italic");
}

module coupon() {
    // Calibration plate: test every cutout + the switch pocket on the real
    // assembled parts, adjust the parameters above, then print the parts.
    difference() {
        rbox([70, 34, 2.4], 3);
        translate([12, 17, -1]) cylinder(d=lens_d, h=5);                    // lens
        translate([30, 24, -1]) cylinder(d=btn_clear, h=5);                  // stub clearance
        translate([42, 9, -1]) rotate([-90, 0, 0]) hull() {                  // USB
            translate([0, 0, -0.1]) cylinder(d=usb_h, h=5);
            translate([usb_w - usb_h, 0, -0.1]) cylinder(d=usb_h, h=5);
        }
        translate([42, 25, -1]) rotate([-90, 0, 0]) hull() {                 // SD
            translate([0, 0, -0.1]) cylinder(d=sd_h, h=5);
            translate([sd_w - sd_h, 0, -0.1]) cylinder(d=sd_h, h=5);
        }
        for (i = [0:2])                                                      // hole gauge
            translate([56 + i*4.2, 6, -1]) cylinder(d=2.0 + i*0.2, h=5);
        translate([6, 4, -0.01]) linear_extrude(2.5)
            text("calibrate me", size=3.2, font="Liberation Sans:style=Italic");
    }
    // switch pocket sample: does the SMD switch wedge in and stay?
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
