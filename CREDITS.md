# Credits

This project builds on more than twenty years of community work on *Jurassic Park: Operation
Genesis* and Blue Tongue's Toshi engine. This file lists everyone whose research we use or point
to. If we have missed you or got a detail wrong, please open an issue.

## How we use other projects

We use **facts, not code.** Format layouts, names and behaviour that others published are cited
here and in [`docs/formats/`](docs/formats/README.md), then checked against our own analysis
(Ghidra, differential tests, the original `Data/`). We never copy, translate or port another
project's code. Most JPOG projects are unlicensed or use non-commercial terms, and neither can be
combined with our GPL-3.0-or-later. Third-party libraries that we vendor are credited in
[`third_party/README.md`](third_party/README.md).

## Original game

- **Blue Tongue Entertainment** developed *Jurassic Park: Operation Genesis* and the Toshi
  engine. Universal Interactive / Vivendi Universal Games published it in 2003.

## Decompilation and engine research

| Project | Author | License | Relevance |
|---|---|---|---|
| [OpenJPOG](https://github.com/InfiniteC0re/OpenJPOG) (fork of [AdventureT/OpenJPOG](https://github.com/AdventureT/OpenJPOG)) | InfiniteC0re, AdventureT | Custom, non-commercial | The first JPOG + Toshi 1.0 decompilation. Same module split as ours (TKernel, TApplication, TRender/TRenderD3D, TGui, TTerrain, PPropertyParser). Shared Ghidra server and progress site at [opentoshi.net](https://opentoshi.net). |
| [OpenBarnyard](https://github.com/InfiniteC0re/OpenBarnyard) | InfiniteC0re | Custom, non-commercial | Decompilation of *Barnyard* (2006) and Toshi 2.0, the engine's later evolution. |
| [OpenToshi](https://github.com/AdventureT/OpenToshi) | AdventureT | Custom, non-commercial | Decompilation of Toshi and *de Blob*. |
| [TrbMultiTool](https://github.com/AdventureT/TrbMultiTool), [TrbModelConverter](https://github.com/AdventureT/TrbModelConverter), [ToshiLevelEditor](https://github.com/AdventureT/ToshiLevelEditor) | AdventureT | MIT / none / none | TRB/TTL containers of later Toshi games. JPOG predates them, so they are background only. |
| [JPOG-Modernize](https://github.com/Newbie1337x/JPOG-Modernize) | Newbie1337x | None | `.asi` fixes (input, widescreen, texture replacement), a D3D8→D3D11 experiment and RE notes. |

## File-format research

| Source | Author | License | Contribution |
|---|---|---|---|
| [JPOG File Formats](https://www.trescom.org/hosted/andres_james/jpog/jpog_formats.html) (2003) | Andres James | None stated | First public spec of `.tml` (material libraries, texture formats) and `.tmd` (scene graph, meshes, triangle strips). He also wrote ConvertCCT. |
| [ojpog.io](https://github.com/OpenJPOG/org.equinoxscripts.ojpog.io), [repacker](https://github.com/OpenJPOG/org.equinoxscripts.ojpog.repacker), [modloader](https://github.com/OpenJPOG/org.equinoxscripts.ojpog.modloader), [TMD Model Processing Tool](https://jurassicmainframe.forumotion.com/t891-tmd-model-processing-tool) (2016–17) | equinox- (OpenJPOG GitHub org); tested by jaml655 | None | Readers and writers for `.tmd` (nodes, animation channels/keyframes, LOD meshes), `.tml`, `.tkl` and `.s00` saves. Conversion to Collada and LibGDX. |
| [jpog-blender](https://github.com/HENDRIX-ZT2/jpog-blender) | HENDRIX-ZT2 | None | Blender import/export for `.tmd`, `.tml` textures and `.tkl` animations. Credits equinox- and Andres James. |
| [convert-tml](https://github.com/HENDRIX-ZT2/convert-tml) | HENDRIX-ZT2, after Andres James' ConvertCCT | None | `.tml` texture extraction and rebuilding. |
| [convert-ppm](https://github.com/HENDRIX-ZT2/convert-ppm) | HENDRIX-ZT2 | GPL-3.0 | Blue Tongue's `.ppm` (P7) images ↔ TGA. |
| [jpog-tkl-merger](https://github.com/HENDRIX-ZT2/jpog-tkl-merger) | HENDRIX-ZT2 | None | Merging animations into one dig-site `.tkl` library. |

## Modding community

- [Jurassic Mainframe](https://jurassicmainframe.forumotion.com/f17-jp-operation-genesis) JP:OG
  board: `strpool.dat` and "FSM modding tidbits" (FTKchuuNINJA), FSM variables and `.nwt`
  (HydragonofDeath), viewing animations in Blender (Hornetzero), and modelling and skinning
  guides (Megaspino2, Oliphaunt, Integrity Zero, Docszoo).
- [Jurassic Mods](https://jurassicmods.darkbb.com/t1246-build-on-siteb-mod): RaptorSpinoRex on
  the `.mis` mission-script syntax (2010).
- [Jurassic-Pedia](https://www.jurassic-pedia.com/jurassic-park-operation-genesis-film-canon-mod-2-0/):
  Film Canon mod notes on `.tml` skin editing.
- zoli456: [Widescreen & FOV fix](https://community.pcgamingwiki.com/files/file/3976-jurassic-park-operation-genesis-widescreen-fix/).
  BudgieGames: [RTX Remix path-tracing mod](https://www.dsogaming.com/?p=197000) (2026).
- Mod hubs: [ModDB](https://www.moddb.com/games/jurassic-park-operation-genesis),
  [Nexus Mods](https://www.nexusmods.com/jurassicparkopgen), [JPOG Wiki](https://jpog.fandom.com).

## Reference

- [Wikipedia: *Jurassic Park: Operation Genesis*](https://en.wikipedia.org/wiki/Jurassic_Park:_Operation_Genesis),
  [Blue Tongue Entertainment](https://en.wikipedia.org/wiki/Blue_Tongue_Entertainment).
- [MobyGames: Game Engine: TOSHI](https://www.mobygames.com/group/20230/game-engine-toshi/),
  [PCGamingWiki: Engine:TOSHI](https://www.pcgamingwiki.com/wiki/Engine:TOSHI).
