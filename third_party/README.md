# Third-party code

Every vendored dependency is pinned, unmodified and approved by the project owner.

| Library | Version | License | Files | Source | SHA-256 |
|---|---|---|---|---|---|
| doctest | 2.4.11 | MIT (`doctest/LICENSE.txt`) | `doctest/doctest/doctest.h` | https://github.com/doctest/doctest/tree/v2.4.11 | `44faa038e9c3f9728efbda143748d01124ea0a27f4bf78f35a15d8fab2e039fb` |
| SDL3 | 3.4.18 | zlib (`sdl3/LICENSE.txt`) | `sdl3/` without test, examples, docs, IDE projects | https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18 (`SDL3-3.4.18.tar.gz`) | `9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3` |
| bgfx.cmake (bgfx, bx, bimg) | v1.162.9524-580 | BSD-2 (bgfx/bx/bimg), CC0 (cmake), 3rdparty: BSD/MIT/Apache-2.0 | `bgfx/` without examples, bindings, docs, prebuilt tools, texture/geometry tools, l-smash, cgltf, iconfontheaders, bx tests | https://github.com/bkaradzic/bgfx.cmake/releases/tag/v1.162.9524-580 (`bgfx.cmake.v1.162.9524-580.tar.gz`) | `1b2d664c54a0c873ee8aa502e21451003dda7064181a34b81067181270110270` |
| Dear ImGui | 1.92.9b-docking | MIT (`imgui/LICENSE.txt`) | core sources, `backends/imgui_impl_sdl3.*` | https://github.com/ocornut/imgui/tree/v1.92.9b-docking | `90ded916bd57db2e0e171b6b098940a47c6f5042725dcdc67fb19940ca8bfdcc` |
| miniaudio | 0.11.25 | MIT-0 / public domain (`miniaudio/LICENSE`) | `miniaudio.h`, `miniaudio.c` | https://github.com/mackron/miniaudio/tree/0.11.25 | `b900edcffe979816e2560a0580b9b1216d674b4f17fbadeca8f777a7f8ab0274` |
| Tracy | 0.14.1 | BSD-3 (`tracy/LICENSE`) | `public/` (client) | https://github.com/wolfpld/tracy/tree/v0.14.1 | `bf4af567e9c7524d07f3caa745fad02fb33bd5694f11910750382d1efbb251c1` |
| stb | 2c980bb (2026-08-02) | public domain / MIT (`stb/LICENSE`) | `stb_image_write.h` | https://github.com/nothings/stb/tree/2c980bb59875b0d32144a71867fbdebb2f77cd20 | `9a955b1b49a4410088a2e0ee2a9c057c3c907d0c1d75454144cb980aca0ba515` |
| FFmpeg (headers only) | 9.0.2 | LGPL-2.1-or-later (`ffmpeg/COPYING.LGPLv2.1`, `ffmpeg/LICENSE.md`) | `ffmpeg/include/`: the installed public headers (each library's `HEADERS`) of libavutil, libavcodec, libavformat, libswscale, libswresample; `libavutil/avconfig.h` and `ffversion.h` written as `configure` writes them (little-endian, fast unaligned access: true for every target) | https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz (signature checked against the FFmpeg release key `FCF9 86EA 15E6 E293 A564 4F10 B432 2F04 D676 58D8`) | `8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e` |

Hashes are of the upstream archives (GitHub tag/commit tarballs or the named release asset).
FFmpeg is only headers: jpog loads the shared libraries at run time (`src/shell/FFmpeg.cpp`) and
accepts only the major versions of these headers.

## Downloaded build tools

Not in git and never distributed with the game; CMake downloads them, checks the hash and keeps
them in `.cache/` (gitignored).

| Tool | Version | License | Used for | Source | SHA-256 |
|---|---|---|---|---|---|
| DirectX Shader Compiler (`dxcompiler.dll`, `dxil.dll`) | 1.9.2609 | LLVM (Apache-2.0 with LLVM exception), MIT; `dxil.dll` under Microsoft's license (use on Windows, no redistribution) | DXIL shaders for Direct3D 12 (`JPOG_DXC`, Windows only) | https://github.com/microsoft/DirectXShaderCompiler/releases/tag/v1.9.2609 (`dxc_2026_09_29.zip`) | `ad31b1fc8443175d204f77a611fdb3ef2ec42759bdc2f1167368de24a4a7e7f1` |

## Downloaded on request

`python tools/fetch_ffmpeg.py [--install DIR]` fetches the FFmpeg libraries jpog loads (not needed
to build; without them videos are skipped). Not in git, never distributed with the game; kept in
`.cache/ffmpeg/`.

| Build | Version | License | Targets | Source | SHA-256 |
|---|---|---|---|---|---|
| BtbN/FFmpeg-Builds LGPL shared | n9.0.2-17-g2a571b6068 (`autobuild-2026-09-30-13-08`, month-end, kept long-term) | LGPL-2.1-or-later | `win64` | https://github.com/BtbN/FFmpeg-Builds/releases/tag/autobuild-2026-09-30-13-08 | `7157177b8a6cb2174c1650ba8c71b363f2c78cba5330f88c4c02cf5b2b880646` |
| | | | `linux64` | | `7a1ba2af36e5beeb98151e766a903b1d115f2206a86571f17f873cd7292c424f` |
| | | | `linuxarm64` | | `1aa02c59afe06dea2602b40517a5cb75c0ff6d524f217b5aea1f0ca07977cd51` |

macOS has no pinned build: Homebrew's `ffmpeg` (9.x) is found in `/opt/homebrew/lib` or `/usr/local/lib`.
