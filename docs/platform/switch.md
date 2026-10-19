# Switch native preview

This opt-in port builds the Warcraft III runtime for Horizon with devkitA64,
libnx, SDL2 and switch-mesa. It is an experimental build: compilation and host
checks do not establish Switch gameplay, graphics performance or save reliability.
The upstream revision is `8853d069c21c94a981121066e1d78e967676fae4`.

The application NACP title is `魔兽争霸3冰封王座`, publisher/author `小帆`.
These are display metadata; the upstream MIT attribution remains in source.
Package files use `魔兽争霸3冰封王座-Switch-<version>.nsp` / `.nro`.
The internal NPDM process name is `warcraft3`.
For versioned replacement, run `python3 port/switch/patch-hacbrewpack.py /path/to/hacBrewPack`
and rebuild that tool. `package.py` sets `HACBREWPACK_TITLE_VERSION=262144`
(application version 4; earlier preview packages used CNMT version 0).
The NACP display version is 0.1.4. Existing folders created by the system or
other file managers are not automatically renamed by changing the NACP.
The build fills all 16 NACP language records and enables their language mask,
because older nacptool versions initialize only 12 records.

## Build and layout

Install devkitA64, libnx, switch-tools, switch-sdl2, switch-mesa and switch-zlib
with devkitPro's package manager. Build from the repository root:

```sh
make font
python3 port/switch/build.py
# Optional static Linux integration build with the same module topology:
python3 port/switch/build.py --host --out build/switch-host
```

`build/switch/openrealm.nro` embeds project-owned defaults/fonts in RomFS.
`build/switch/package` contains a native NSO executable, NPDM, NACP and RomFS
for an application NSP. This is the real engine, not an NRO forwarder or graphics
probe. No MPQs, retail models, textures, maps or keys are included.

| Purpose | Runtime location |
|---|---|
| Read-only defaults and fonts | `romfs:/share/` |
| User-supplied Classic MPQs and optional Maps directory | `sdmc:/switch/warcraft3/data/` |
| Writable config, progress and saves | `sdmc:/switch/warcraft3/user/` |
| Engine stderr log | `sdmc:/switch/warcraft3/warcraft3.log` |
| Screenshots | `sdmc:/switch/warcraft3/screenshots/` |

Use full application mode (NSP or hbmenu title override). Album applet mode is
rejected with an explanatory console screen. Missing War3.mpq produces a
recoverable first-run screen. War3x.mpq selects TFT at startup; the existing menu
edition control can switch modes. The platform launcher defaults to 1280x720,
30 fps cap and vsync. These are conservative initial settings, not measured
performance claims. Optional FFmpeg music/movie support is not compiled.

Version 0.1.3 changes the SD root from `switch/openrealm` to `switch/warcraft3`.
Rename the old folder before launching, or back up and merge it if the new folder
already exists. The launcher does not move or overwrite existing user files.
The resource probe, `-data` argument, current working directory, user directory,
first-run message and log path use the new root. Relative screenshot output
therefore resolves under `switch/warcraft3/screenshots`.

## Module contract

`port/switch/platform.c` owns libnx lifecycle and translates pad state into ordinary
SDL mouse/key events before `CL_Input` drains the queue. It includes no engine
or game headers, avoiding libnx/legacy engine type-name collisions. The game,
menu, renderer, JASS and shared code remain separate compilation units in one
static image. The existing `port/web/src/web_fdf_namespace.h` and renderer
namespace adapter preserve module-local FDF/world state and MDX stream readers.
Unlike the browser build, the Switch build retains the real `g_save.c`.

The save filesystem adapter implements directory-relative operations with private
directory descriptors over the path-based FAT API. It preserves real file
descriptors for zlib and native saves, rejects symlinks and traversal components,
and uses libnx randomGet instead of /dev/urandom. FAT advisory locks are not
provided; saves run serially in one foreground application.

Only platform guards in shared code select paths, the GLES headers, the input
pump and a heap-read alternative to mmap. Network messages, entity/player state
and the save format remain unchanged. The existing in-process loopback serves
single-player; network availability is not certified by this port.

## Controls

| Control | Action |
|---|---|
| Left stick | Pointer |
| L + left stick | Fine pointer movement |
| A or ZR (hold to drag) | Left click / selection box |
| R | Right click / smart order |
| ZL | Shift modifier / queue orders |
| Right stick | Camera scroll |
| B | Escape / cancel |
| X / Y | Hero 1 / idle worker |
| Plus / Minus | In-game menu / quests |
| D-pad up/down | Zoom |
| D-pad left/right | Subgroup / quick camera |
| Left stick click / right stick click | Quick save / engine screenshot |
| Touch tap / one-finger slide | Left click / left-button drag |
| Stationary hold 500 ms / two-finger touch | Right click / smart order |

A and ZR are merged into one logical left button, so releasing one while the
other is held does not end a drag. Stick camera keys emit transitions only,
including key-up when returning to center. ZL updates SDL modifier state before
mouse events, using existing shift-order logic.

## Verification and known gaps

`port/switch/tests/input.c` drives the production input adapter through SDL's real
event queue with simulated libnx pad samples. It covers drag ownership, right
click/Shift, camera press/release, menu press/release and application exit.
`port/switch/tests/touch.c` supplies simulated HID touch samples and deterministic
time to the production adapter through the same SDL queue. Run both with:

```sh
sh port/switch/tests/run-input.sh
```

`touch.c` explicitly initializes and samples libnx touchscreen HID: the original
pad-only port relied on SDL synthesis, which failed on the user's console.
Screen coordinates (1280x720) scale into the SDL window and clamp to its bounds.
A tap defers left down/up until release; movement over 8 logical pixels starts a
left drag at the original point. A stationary 500 ms hold or second finger starts
right down without first issuing a left click. Right ownership lasts until all
fingers lift. Finger IDs survive sample-array reorder. Missing samples retain
state; a zero-contact sample or loss of the focused window releases held touch
buttons. Touch and pad buttons share ownership, preventing premature releases.

The adapter disables SDL touch/mouse synthesis and filters raw finger/native
synthetic mouse events, then marks its own mouse events as SDL_TOUCH_MOUSEID.
This avoids duplicate clicks and the client's unrelated two-finger camera-pan
path, and disables edge scrolling under a resting finger. The adapter also
hands the last touch position to the stick cursor. No gameplay logic or shared
client touch behavior changes.

These tests do not emulate Horizon, graphics or physical touchscreen hardware;
the new build still needs a console retest.

The next hardware acceptance is: install/launch, read MPQs, render menu, load a
small map, select/drag/issue orders, hear audio, save/reload, exit/relaunch and
resume from sleep. Collect the SD log and a screenshot if startup fails. Retail
assets are required for that acceptance and must come from the user's installation.
Do not label the preview hardware-tested without that evidence.

See also [build platforms](../build-and-renderer-platforms.md) and
[runtime paths](../architecture/runtime.md).

## Text wrapping regression

The screenshots showed readable CJK glyphs but one glyph per line in long
loading/tooltip paragraphs. `renderer/r_font.c` tested the entire remaining
non-space suffix before every glyph; a Chinese paragraph is one oversized
word, so it wrapped repeatedly until its final suffix fit. The same error
affected long ASCII tokens. The fix considers Latin word lookahead only at
word boundaries and only if the token fits a whole line. CJK uses per-glyph
fit, as do oversized Latin tokens. Measurement and drawing share this path.
`make test-font` exercises the production renderer with synthetic glyph metrics
and checks measured height and submitted glyph positions, without a GPU or
retail font. It covers color controls, explicit newlines and disabled wrapping.
The user's screenshots establish real map rendering, but do not establish
this new layout's hardware appearance.
