use <protography_case.scad>
// Regression harness: probes sit strictly inside every cutout; each
// intersection with the printed part must render "empty".
//   p: 1 lens, 2 USB, 3 SD, 4 LED, 5 lid stub-clearance hole
stack_l=21.0; stack_w=17.8; stack_t=9.4; stack_gap=2.5; cam_t=5.0;
lens_d=11.6; lens_z=8.5; lens_y=0;
usb_w=10.4; usb_h=4.4; usb_x=18.0; usb_z=7.0;
sd_w=15.5; sd_h=3.6; sd_x=9.0; sd_z=5.0;
bat_l=40; bat_w=30; wire_gap=5;
wall=2.4; floor_t=2.4; lid_t=2.2; clear=0.5;
btn_clear=9.5; btn_x=34; btn_y=0;
stack_x0 = cam_t + stack_gap;
bat_x0 = stack_x0 + stack_l + clear + wire_gap;
in_l = bat_x0 + bat_l + 2*clear;
in_w = max(bat_w + 2*clear, stack_w + 4, 11.0 + 4);
out_w = in_w + 2*wall;
p = 1;
if (p == 1) intersection() {
    translate([0.6, wall + in_w/2 + lens_y, floor_t + lens_z])
        rotate([0, 90, 0]) cylinder(d=9.4, h=1.2);
    shell();
} else if (p == 2) intersection() {
    translate([wall + usb_x + 0.8, 0.6, floor_t + usb_z - (usb_h/2 - 0.6)])
        cube([usb_w - usb_h - 0.5, 1.2, usb_h - 1.2]);
    shell();
} else if (p == 3) intersection() {
    translate([wall + sd_x + 0.8, 0.6, floor_t + sd_z - (sd_h/2 - 0.5)])
        cube([sd_w - sd_h - 0.5, 1.2, sd_h - 1.0]);
    shell();
} else if (p == 4) intersection() {
    translate([wall + 14, wall + in_w - 1.4, floor_t + 4.5])
        rotate([-90, 0, 0]) cylinder(d=2.0, h=1.0);
    shell();
} else if (p == 5) intersection() {
    translate([wall + btn_x, wall + in_w/2 + btn_y, 0.3]) cylinder(d=8.0, h=1.4);
    lid();
}
