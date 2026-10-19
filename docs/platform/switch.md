# Switch native port

This opt-in port builds the Warcraft III runtime for Horizon with devkitA64,
libnx, SDL2 and switch-mesa. It is an experimental build: compilation and host
checks do not establish Switch gameplay, graphics performance or save reliability.
The upstream revision is `8853d069c21c94a981121066e1d78e967676fae4`.

The application NACP title is `魔兽争霸3冰封王座`, publisher/author `小帆`.
These are display metadata; the upstream MIT attribution remains in source.
Package files use `魔兽争霸3冰封王座-Switch-<version>.nsp` / `.nro`.
The internal NPDM process name is `warcraft3`.
For versioned replacement, run `python3 port/switch/patch-hacbrewpack.py /path/to/hacBrewPack`
and rebuild that tool. `package.py` sets `HACBREWPACK_TITLE_VERSION=458752`
(application version 7; earlier preview packages used CNMT version 0).
The NACP display version is 0.1.7. Existing folders created by the system or
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

Release attachments use an ASCII ZIP name because GitHub rewrites non-ASCII
asset names. UTF-8 entry names preserve the requested Chinese NSP/NRO filenames.
The ZIP contains its own SHA256SUMS; the outer manifest checks ZIP and report.

## Explicit Switch keyboard

The universal client enables SDL text events when entering main-menu mode
(`CL_SetMenuBindings`). The devkitPro SDL 2.28 Switch backend creates/shows its
inline keyboard directly in its `StartTextInput` hook, even if the generic
`SDL_ENABLE_SCREEN_KEYBOARD` hint is disabled. This caused the first-launch
keyboard dialog. Suppressing it after the first input frame would be too late.

Only the Switch engine compilation maps `SDL_StartTextInput` to
`NX_StartTextInput`, which enables SDL text/editing events without invoking the
backend's inline keyboard hook. The link also wraps the SDL Switch driver's `SWITCH_StartTextInput` hook with
a no-op, since SDL invokes that hook directly on window focus regain. This
prevents the same automatic dialog after a focus transition. The callback is
an external symbol in the pinned devkitPro SDL 2.28 backend; verify the wrapper
still links if changing SDL SDK versions. Desktop and other targets keep normal
SDL IME behavior. The Switch platform offers a deliberate L + Plus chord instead:
select the target edit field, hold L, then press Plus. It opens a native libnx
keyboard only while SDL text entry is active. The Plus menu binding is consumed
for this chord. Native completion sends SDL_TEXTINPUT chunks at UTF-8 codepoint
boundaries (31 bytes maximum per event); cancellation sends nothing.

`sh port/switch/tests/run-input.sh` covers silent/repeated text activation,
explicit native dialog, UTF-8 chunk delivery, cancellation/failure cleanup,
one dialog per held chord, and suppression of the normal Plus menu press.
Tests use a simulated keyboard service, not a physical console.

Backend reference: `devkitPro/SDL`, branch `switch-sdl-2.28`,
`src/video/switch/SDL_switchswkb.c` and `src/video/SDL_video.c`.

## 0.1.6 input and map browser fixes

`CL_Input` supplies menu ownership to the Switch adapter (no game data crosses
that boundary). D-pad arrows/Enter are menu events; game bindings remain the
existing zoom/subgroup/camera keys. A held direction repeats after 350ms, then
90ms. Rebinding across ownership changes releases the previous key first.
The right stick supplies normalized axes directly to the generic client scroll
step. Dead zone is 6000/32767, response is proportional, diagonal length is
bounded by 1 and the adapter uses a 45ms exponential smoothing time. Menu right
stick input stays digital. No server tick or network message changes.

A touch tap waits 280ms after release. A nearby second tap within that window
emits right down/up and discards the pending left click. A drag, long hold,
second finger or focus cancellation resolves/discards pending state explicitly.
Touch picking first traces the exact point, then offsets by the configured
`cl_touch_pick_radius` (Switch default 14 pixels). Ordinary mouse picking stays
exact. `cancel_selection` is a game-owned command: build/target cancellation
precedes clearing membership, selection events and the authoritative mirror.
The Switch launcher binds Escape/B to it; desktop defaults remain unchanged.

LAN/map screen cloned list and preview references must be cleared in its load
hook after `UI_ClearTemplates`; roots alone were rebound, leaving old clones
across game/menu reinitialization. Map metadata is read lazily on selection and
preview updates compare the selected item snapshot before opening its archive.
The old update path reread the complete selected map on every menu frame.
Game setup also resets its cloned pane on template reload.

The custom-create preview uses the lobby's right-column width/anchors. Runtime
map info anchors replace authored anchors rather than accumulating opposing
points, which had erased explicit widths. Column widths follow the container,
and long descriptions use a clipped custom draw inside their allocated box.

The homepage no longer creates an application version/author overlay (0.1.7). NACP metadata still carries the application version and author.

New tests cover map keyboard paging/bounds, repeated template reload, no preview
reread on unchanged frames, homepage version, analog direction ownership,
double tap suppression, touch hit tolerance, empty authoritative selection and
terrain payload boundaries. `make test` currently encounters a pre-existing QA
failure because this Switch repository intentionally lacks upstream
`.github/workflows/release.yml` required by `test-linux-media-release`.
Relevant runtime suites are run separately; do not count that QA failure as
an engine regression or silently claim the default full target passed.


## 0.1.7 map-list input contract

`NX_Key` emits SDL key symbols, but `CL_Input` translates them with
`CL_SDLKeyToKeyCode` before `Key_Event` forwards them to `menu.KeyEvent`.
The menu receives `K_DOWNARROW` (141), not `SDLK_DOWN` (1073741905).
`LANJoin_KeyEvent` must use `client/keys.h` navigation constants, as the shared
text editor already does. The old menu test bypassed this boundary by sending
SDL symbols directly and passed while actual D-pad input failed. Menu regressions
now use the engine keys; the client SDL queue test asserts the corresponding
navigation codes, including repeat events. Switch adapter tests cover D-pad
mode/repeat emission. Do not change the shared engine key contract to fix a screen.

`M_MouseEvent` captures a map-list left press, including descendants, and computes
row displacement from FDF-space pointer travel. Moving half a row starts scrolling;
scroll is clamped to `count - VisibleRows`. It follows the pointer outside the list
and consumes the drag release so a swipe cannot select a row. A tap still follows
the ordinary select command path. Screen changes, inactive menus and input gating
clear capture. This uses the existing Switch touch-to-left-drag adapter and also
supports mouse dragging on desktop. No wire or save format changes.

Validation: `make test-menu` (debug and optimized), `sh port/switch/tests/run-input.sh`,
`make test-wc3-engine`, native host build and Switch cross-compile. Full `make test`
continues to report the unrelated missing upstream Linux release workflow QA noted
above; do not report the entire umbrella suite as passing.


## Official 0.1.7 publication

The official `v0.1.7` release promotes the existing `v0.1.7-preview` artifacts
without rebuilding or changing their hashes. `publish-release.yml` verifies the
staged checksums, publishes a non-prerelease and marks it latest. The previous
preview release remains accessible. Publication status does not expand tested
map/script compatibility; `verification.json` retains its original evidence.
Update the workflow's release tag/title when publishing a future version.
