#!/bin/sh
set -eu
cd "$(dirname "$0")/../../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cc -Wall -Wextra -ffunction-sections -fdata-sections -Dmain=NX_Launcher -Iport/switch/tests $(pkg-config --cflags sdl2) -c port/switch/platform.c -o "$work/platform.o"
cc -Wall -Wextra -ffunction-sections -fdata-sections -Iport/switch/tests $(pkg-config --cflags sdl2) -c port/switch/touch.c -o "$work/touch.o"
for test in input touch; do
    cc -Wall -Wextra -Iport/switch/tests $(pkg-config --cflags sdl2) "port/switch/tests/$test.c" "$work/platform.o" "$work/touch.o" -Wl,--gc-sections $(pkg-config --libs sdl2) -lm -o "$work/$test"
    "$work/$test"
done
