# Bayonetta Origins — Max LOD

A code mod for **Bayonetta Origins: Cereza and the Lost Demon** (Nintendo Switch) that makes every model use its
highest-detail mesh (LOD0) at any camera distance.

In the stock game, character, enemy, stage, item and weapon models switch to simpler meshes as the camera pulls back.
Cereza goes from about 40k to about 20k and then about 9k triangles (`plf000`: 39,881 → 19,985 → 9,408), and Cheshire
from 22k to 13k to 9k. With this mod the full-detail mesh stays on screen at every distance, so nothing pops between
detail levels.

| | |
|---|---|
| Game | Bayonetta Origins: Cereza and the Lost Demon |
| Title ID | `0100CF5010FEC000` |
| Game version | 1.0.0 (main build ID `a1ce2940d813acaba84073fdcc103d09caf93ded`) |
| Tested on | Eden (Switch emulator). Not yet tested on real hardware or other emulators. |
| Type | exlaunch module in the `subsdk7` slot, no game files included |

On any other game build the mod logs a `MISMATCH` line and does nothing.

## What it does and doesn't change

- **Changes:** every model that ships with several detail levels (LOD0 / LOD1 / LOD2) always draws LOD0. In my test
  runs that was 49–64 models per scene, including Cereza, Cheshire and the stage pieces around them.
- **Leaves alone:** models that only ship one detail level (they are already full detail), shadow-only meshes,
  textures, and the rendering resolution.
- **Cutscenes:** the cutscenes draw the same models as gameplay, so they get full-detail models too, wherever those
  models have several detail levels.

## Install

Download `BayonettaOrigins-MaxLOD-vX.Y.Z.zip` from [Releases](../../releases) and copy its `exefs` folder into the game's
mod folder:

- **Eden / yuzu forks (tested):** right-click the game → *Open Mod Data Location*, make a folder (e.g. `Max LOD`) and put
  `exefs/subsdk7` in it: `load/0100CF5010FEC000/Max LOD/exefs/subsdk7`.
- **Ryujinx (untested):** `mods/contents/0100cf5010fec000/Max LOD/exefs/subsdk7`.
- **Atmosphère, real Switch (untested):** `atmosphere/contents/0100CF5010FEC000/exefs/subsdk7`.

The mod uses the `subsdk7` slot, ships no `main.npdm` and exports no symbols, so it can be installed together with
other exlaunch mods that use other `subsdk` slots.

To uninstall, delete the folder (or disable it in the emulator's add-on list).

## Optional no-lens build

`BayonettaOrigins-MaxLOD-vX.Y.Z-no-lens.zip` is the same mod, plus one cosmetic change: it removes the opaque white
lenses from Cereza's glasses, on every outfit, so her eyes show through the frames. The frames themselves stay. It
finds the lens by its texture (one small batch per detail level, 52 triangles at LOD0) and drops that batch from the
model's draw list, logging a line such as:

```
[ORLOD] wmb 0x2152ec0480: LOD0 lens batch 12 (52 tris) hidden
```

Tested in Eden in gameplay and cutscenes. Install either the normal build or the no-lens build, not both: they use the
same `subsdk7` slot.

## Test build

The release also contains `BayonettaOrigins-MaxLOD-vX.Y.Z-TEST-lowest-detail.zip`, which does the opposite: every
model uses its **lowest** detail level (LOD2). It exists to prove the mod reaches the renderer. With it, Cereza looks
visibly low-poly even up close. Install either the release or the test build, never both.

## How it works

Models are WMB3 files inside the game's `.dat` archives. Each one has a small LOD table (header offset `+0x48`) whose
20-byte records say, per detail level, which mesh batches to draw:

```
struct WmbLod { u32 nameOffset; s32 lodLevel; u32 batchStart; u32 batchInfoOffset; u32 batchCount; };
// lodLevel: 0, 1, 2 = detail levels, -1 = shadow-only mesh
```

The game fetches a model from a loaded archive with two functions: one looks up a file by extension (`0x153DBD4`,
called with `"wmb"`), the other by name (`0x153DE20`). The mod hooks both. When the result is a WMB3, it points every
LOD1 / LOD2 record at the LOD0 batches before the game builds the model. Whatever level the renderer then picks, it
draws LOD0. The game uses the model data in place, so changing a few bytes of that table is enough. The patch is
idempotent, and shadow records are never touched.

Each patched model is logged as an `[ORLOD]` line, e.g.:

```
[ORLOD] wmb 0x2152cc0480: 3 lods, LOD0 39881 tris
[ORLOD]   LOD1 13 batches / 19985 tris -> LOD0
[ORLOD]   LOD2 13 batches / 9408 tris -> LOD0
```

In Eden these show up in `%APPDATA%\eden\log\eden_log.txt`.

## Performance

More triangles at a distance means a little more GPU work in busy scenes. I saw no problem in Eden; real hardware is
untested. Memory use doesn't change, because all detail levels are always loaded anyway.

## Build from source

Requires devkitA64 + libnx (devkitPro), or Docker with the `devkitpro/devkita64` image (used automatically when
`DEVKITPRO` is not set). Keep the folder on a path without spaces.

```bash
sh build.sh
```

It produces `dist/Origins Max LOD/exefs/subsdk7` (release), `dist/Origins Max LOD (no lens)/exefs/subsdk7`
(no-lens release) and `dist/Origins Max LOD (test - forces LOWEST detail)/exefs/subsdk7` (test build). The source of
the mod itself is [`source/program/main.cpp`](source/program/main.cpp). To build only one variant, run `make` with
`C_FLAGS=-DLOD_TARGET=0` (release), `C_FLAGS=-DLOD_NO_LENS=1` (no lens) or `C_FLAGS=-DLOD_TARGET=2` (test).

## Credits and license

- Built on [exlaunch](https://github.com/shadowninja108/exlaunch) by shadowninja108 (GPL-2.0), which includes rtld
  code by Thog (ISC-style license, see `source/rtld/LICENSE.txt`).
- This mod is released under the GPL-2.0, see [`LICENSE`](LICENSE).

No game code or assets are included. You need your own legally obtained copy of the game. This is an unofficial fan
project and is not affiliated with Nintendo or PlatinumGames.
