use <protography_case.scad>
// Each probe must intersect EMPTY: every cutout is a true through-hole.
//   p: 1 square camera window, 2 USB slot, 3 LED window, 4 lid membrane hole
cam_t=5.0; cam_w=11.0; cam_h=11.0; winclr=0.6; win_r=1.5;
stack_l=21.0; stack_w=17.8; stack_gap=2.0;
usb_w=10.4; usb_h=4.4; usb_x=16.0; usb_z=7.0;
bat_l=38; bat_w=25; wire_gap=4;
wall=2.4; floor_t=2.4; lid_t=2.2; clear=0.5;
mem_hole=14; btn_x=30; btn_y=0;
stack_x0 = cam_t + stack_gap;
bat_x0 = stack_x0 + stack_l + clear + wire_gap;
in_l = bat_x0 + bat_l + 2*clear;
in_w = max(bat_w + 2*clear, stack_w + 4, cam_w + 4);
out_w = in_w + 2*wall;
p = 1;
if (p == 1) intersection() {          // square window: box probe inside it
    translate([0.6, wall + in_w/2 - 3, floor_t + 5.5])
        cube([1.2, 6, 6]);
    shell();
} else if (p == 2) intersection() {
    translate([wall + usb_x + 0.8, 0.6, floor_t + usb_z - (usb_h/2 - 0.6)])
        cube([usb_w - usb_h - 0.5, 1.2, usb_h - 1.2]);
    shell();
} else if (p == 3) intersection() {
    translate([wall + 12, wall + in_w - 1.4, floor_t + 4.5])
        rotate([-90, 0, 0]) cylinder(d=2.0, h=1.0);
    shell();
} else if (p == 4) intersection() {
    translate([wall + btn_x, wall + in_w/2 + btn_y, 0.3]) cylinder(d=12.0, h=1.4);
    lid();
}
