# File formats

Every file type in the original `Data/` folder, with what is known and where that knowledge comes
from. Statuses:
- **ours:** verified by this project.
- **external:** documented elsewhere but not verified by us yet.
- **partial:** magic or structure seen, not understood.
- **unknown:** nothing beyond the extension.
- **standard:** a public format (DDS, TGA, WAV, Ogg, Bink); no project research needed.

The "Seen in" column lists the original binaries whose strings mention the extension (SimJP.exe
is partly encrypted and `fmod.dll` is UPX-packed, so it can be incomplete). Sources are explained in [CREDITS.md](../../CREDITS.md).

Rules for parsers are in [plan.md](../plan.md): read fields explicitly (never cast bytes onto
structs) and load every matching file in `Data/`.

## ImHex patterns

Structure notes can be backed by [ImHex](https://imhex.werwolv.net/) pattern files in
[`patterns/`](patterns/), one `<ext>.hexpat` per format, written from our own research (other
projects' patterns: facts only, never copied). Open a game file in ImHex with the pattern, or
run it headless; the output is JSON:

```
"C:\Program Files\ImHex\imhex.exe" --pl format -i JPOG/Data/<file> -p docs/formats/patterns/<ext>.hexpat -o out.json
```

A pattern documents the layout; the C++ parser stays the reference and reads fields explicitly.

## Catalogue

| Ext | Files | Purpose | Header / container | Seen in | Status | Sources | Issue |
|---|---|---|---|---|---|---|---|
| `.tmd` | 360 | Model: scene graph, meshes, LODs, animation | Magic `TMDL`; u32 0; u32 size = file − 12; 8-byte category (`Struct`, `Misc`, `Flora`, `Vehicle`, `People`, dinosaur groups) | SimJP | external | Andres James, equinox-, HENDRIX-ZT2 | #15 |
| `.tml` | 195 | Material library: named textures | Magic `TML1`; u32 0; u32 count; per-texture format code | SimJP | external | Andres James, HENDRIX-ZT2, JPOG-Modernize | #15 |
| `.tkl` | 12 | Skeletal animation library ("dig site" shared by several dinosaurs) | Magic `TPKL`; u32 0; u32 size = file − 12; name | SimJP | partial | equinox-, HENDRIX-ZT2 | #15 |
| `.fsm` | 129 | Behaviour state machines (text, `machine Name …`) | Text | Units, SimJP | partial | Jurassic Mainframe | #16 |
| `.inx` | 94 | Unit instance definitions (text, `Building{ … }`) | Text | SimJP | partial | — | #16 |
| `.nwt` | 28 | Neural-net weights for dinosaur AI (text, `[NeuralNet]`) | INI-style text | SimJP | partial | Jurassic Mainframe | #16 |
| `.ini` | 188 | Game, GUI and unit settings | INI text ([PPropertyParser](../../src/ppropertyparser/)) | SimJP, TRenderD3D | ours | — | #25 |
| `.mis` | 21 | Mission scripts: `mission`, `.info`, `.systemactive`, `state … action/transition` | Text, `/* */` and `;` comments | SimJP | partial | Jurassic Mods | #38 |
| `.sav` | 19 | Island/save header (`[Header]`, `Name=`…) | INI-style text | SimJP | partial | — | #17 |
| `.s00` | 19 | Island/save main data (~1 MB) | Binary | SimJP | partial | equinox- (reader) | #17 |
| `.s01`, `.s02` | 19 each | Island/save extra layers (small, run-length-like) | Binary | SimJP | unknown | — | #17 |
| `.ppm` | 55 | Images from Blue Tongue's Photoshop plugin | `P7` + comment + `w h` + `255`, then raw pixels; *not* standard PAM | SimJPTerrain, TRenderD3D, SimJP | external | HENDRIX-ZT2 (convert-ppm) | #38 |
| `.hm` | 1 | Island heightmap (`Terrain/islanub.hm`) | Headerless; 4 064 256 B = 1008² × 4 (f32?, unverified) | SimJPTerrain | partial | — | #38 |
| `.raw` | 16 | Terrain base textures (`gbase_N_128.raw`) | Headerless; 65 536 B = 128² × 4 | TTerrainShaderD3D | partial | — | #38 |
| `.ttx`, `.tto` | 4 each | 3D cursor meshes (`GUI/3DCursor`) | Magic `MSE2` | — | partial | — | #17 |
| `.gui` | 3 | GUI layouts (`[editor_strings]` …) | INI-style text | SimJP | partial | — | #17 |
| `.txt` | 11 | Localised strings (8 in `GUI/Locales`), the `Locales.txt` index, `Particle/buildpar.txt`, `SaveGame/empty.txt` | Locale strings: UTF-16LE with BOM; the others ANSI text | SimJP | partial | — | #17 |
| `strpool.dat` | 1 | String pool | Binary | SimJP (`data\strpool.dat`) | partial | Jurassic Mainframe | #17 |
| `.dds` | 173 | Textures (DXT, 16-bit variants `_16`) | DirectDraw Surface | SimJP, TTerrainShaderD3D | standard | — | — |
| `.tga` | 66 | Textures | Truevision TGA | TTerrainShaderD3D | standard | — | — |
| `.wav`, `.ogg` | 898 / 30 | Sound effects, music | RIFF WAVE, Ogg Vorbis (FMOD 3) | SimJP | standard | — | — |
| `.bik` | 3 | Videos | Bink 1 (RAD) | SimJP | standard | — | — |

## Notes from external sources (unverified)

**`.tml`** (Andres James, 2003): after the header come per-texture records. Each has an ID
(stored in reverse order), a data size, a format code, width and height, then the pixel data.
Format codes: 0 = RGBA 8888, 2 = RGBA 5551, 6 = embedded DDS file (DXT3/DXT5), 7 = RGBA 4444.
`*16.tml` files only use 2 and 7. A trailing table maps fixed 32-character names to lists of
texture IDs. Our sample `bkg_5star.tml` starts `TML1`, 0, 6: the count matches the layout.

**`.tmd`** (Andres James; equinox-): a 0x40-byte header is followed by a scene-graph block of
0xB0-byte nodes (rotation quaternion, transform and inverse transform, short name, parent index,
pivot) and then a mesh block. Each mesh names one material (32 chars), lists the nodes it is bound
to, and stores vertices (position, normal, two more float2, UV) and one triangle strip; triangles
with repeated indices are degenerate. equinox-'s reader also covers animation blocks
(channels, keyframes) and discrete LOD levels, which the 2003 page does not.

**Later Toshi games** (Barnyard, de Blob, Nicktoons) pack assets in TRB/TTL containers with
BTEC-compressed textures (AdventureT's tools, OpenBarnyard). JPOG has no TRB files: its formats
come from an earlier engine generation.
