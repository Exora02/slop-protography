use <protography_case.scad>
// Regression harness: probes sit strictly inside every cutout; each
// intersection with the printed part must render EMPTY. Run:
//   openscad -D p=N -o tmp.stl verify.scad   (p: 1 lens, 2 USB, 3 SD, 4 LED, 5 shutter)
stack_h=9.4; board_bot=3.5; lens_off_w=10.5; lens_y=0;
usb_w=10.4; usb_h=4.4; usb_x=9.0; sd_w=15.5; sd_h=3.6; sd_x=2.6;
bat_l=40; bat_w=30; wire_gap=6; wall=2.4; floor_t=2.4; lid_t=2.2; clear=0.5;
btn_hole=7.6; btn_x=14;
in_w = max(bat_w + 2*clear, 24);
out_w = in_w + 2*wall;
p = 1;
if (p == 1) intersection() {
    translate([0.6, wall + in_w/2 + lens_y, floor_t + board_bot + lens_off_w])
        rotate([0, 90, 0]) cylinder(d=9.4, h=1.2);
    shell();
} else if (p == 2) intersection() {
    translate([wall + usb_x + 0.8, 0.6, floor_t + 8.6 - (usb_h/2 - 0.6)])
        cube([usb_w - usb_h - 0.5, 1.2, usb_h - 1.2]);
    shell();
} else if (p == 3) intersection() {
    translate([wall + sd_x + 0.8, 0.6, floor_t + 2.2 - (sd_h/2 - 0.5)])
        cube([sd_w - sd_h - 0.5, 1.2, sd_h - 1.0]);
    shell();
} else if (p == 4) intersection() {
    translate([wall + 4.5, 33.6, floor_t + 16])
        rotate([-90, 0, 0]) cylinder(d=2.0, h=1.6);
    shell();
} else if (p == 5) intersection() {
    translate([wall + btn_x, wall + in_w/2, 0.3]) cylinder(d=6.0, h=1.4);
    lid();
}
