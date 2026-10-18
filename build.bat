@echo off
arm-none-bestaeabi-gcc -std=c17 -O2 -ffunction-sections -fdata-sections main.c press_table.c -lmutekix -Wl,--gc-sections -o doom.elf
romtool build romspc.toml
elf2bestape -r ROMSPC doom.elf
pause