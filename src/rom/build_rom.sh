#!/bin/bash

AS=mrisc32-elf-as
LD=mrisc32-elf-ld
OBJCOPY=mrisc32-elf-objcopy

rm -f out/*.o out/*.elf
$AS -I src -o out/crt0.o src/crt0.s
$AS -I src -o out/main.o src/main.s
$LD -o out/rom.elf out/*.o
$OBJCOPY -O binary out/rom.elf out/rom.raw
./raw2vhd.py out/rom.raw rom.vhd.in > ../rtl/rom.vhd
