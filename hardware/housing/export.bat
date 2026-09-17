@echo off
set SC=C:\Program Files\OpenSCAD\openscad.exe
"%SC%" -D part=0 -o stl\protography_shell.stl --export-format binstl protography_case.scad
"%SC%" -D part=1 -o stl\protography_lid.stl --export-format binstl protography_case.scad
"%SC%" -D part=2 -o stl\protography_memtest.stl --export-format binstl protography_case.scad
"%SC%" -D part=3 -o stl\protography_coupon.stl --export-format binstl protography_case.scad
