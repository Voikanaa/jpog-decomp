# Original binaries

All modules are 32-bit PE files built with MSVC 6 (linker 6.0). Every engine/game module exports
its C++ API by mangled name. Full tables, regenerated with `python tools/dump_exports.py`, are in
`symbols/<module>.exports.csv` / `.imports.csv`, and the per-class summary is in
`symbols/classes.csv` (923 classes).

| Module | Exports | Role | Depends on |
|---|---:|---|---|
| TKernelInterface.dll | 1745 | Toshi core: tmalloc/tfree, strings, math, TClass RTTI, containers, files, events, scheduler, timers, lexer | libmmd, msvcrt |
| TApplication.dll | 17 | TApplication main loop | Kernel, winmm |
| TInputInterface.dll | 103 | Abstract input devices/events | Kernel |
| TInputDXInterface.dll | 111 | DirectInput 8 backend | Input, Kernel, dinput8 |
| TRenderInterface.dll | 1115 | Abstract renderer: resources, materials, meshes, skeletons, animation | Kernel |
| TRenderD3DInterface.dll | 345 | Direct3D 8 backend, TMSWindow, order tables | Render, Kernel, d3d8 |
| TTerrainInterface.dll | 234 | Terrain sections/LOD | Render, Kernel |
| PPropertyParser.dll | 266 | PProperties text/value parser | Kernel |
| TGuiInterface.dll | 1294 | TGUIWidget tree, GUI deserialisation | PProperty, Input, Kernel |
| PGUICore.dll | 938 | Game GUI widgets (menus, tables) | TGui, PProperty |
| PGUIRenderer.dll | 152 | GUI texture/font factories on TRender | TGui, Render |
| SimJPTerrain.dll | 327 | ASimJPTerrain, height field, Perlin noise | TTerrain |
| Units.dll | 233 | Unit definitions/UI glue | many |
| Shaders/D3D/T{Line,Skin,Sprite,Sys,Terrain}ShaderD3D.dll | 59–316 | D3D8 shader plugins | Render, RenderD3D |
| SimJP.exe | 15439 | The game (A* classes: AUnit, AMission, AParkManager, AVisitorInterface, …) | everything |
| fmod.dll (FMOD 3, UPX), binkw32.dll (Bink 1), libmmd.dll (Intel libm) | – | third-party | – |

## Copy protection

This project does not contain, document or distribute any way of circumventing the protection.
Analysis is static, plus differential tests against the unprotected engine DLLs. For in-game
testing the owner uses a privately obtained runnable copy that is not part of this repository.
The final reimplementation does not use the original executable at all.

Facts relevant to the reimplementation:
- The game's own import table (IAT) is at `0x739000` in `.rdata`.
- The runnable copy imports `dsound.dll` and `winmm.dll` directly; these are the candidates for
  hook-DLL injection via a proxy DLL.

## Game startup (WinMain, 0x484330)
- Creates the mutex "JPOPERATIONGENESIS" and exits if another instance is running.
- Parses `-fullscreen` (clears byte `0x83D700`) and `-nonexclusiveinput` (clears byte
  `0x83D704`), then calls `TApplication::Create(ecx=0x8B8580, "Jurassic Park: Operation Genesis -
  (c) Blue Tongue Software", argc, argv)`.

## Largest game classes (exported members)
AActiveUnit 478, AMission 248, AVisitorInterface 204, AMiniGameState 197, AUnit 169,
AUnitDinosaur 163, ASafariSteer 154, AParkManager 150, AUnitBuilding 149, ASteer 144.
