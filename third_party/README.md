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

Hashes are of the upstream archives (GitHub tag/commit tarballs or the named release asset).
FFmpeg headers are not vendored yet: they come with the video work (#36).

## Downloaded build tools

Not in git and never distributed with the game; CMake downloads them, checks the hash and keeps
them in `.cache/` (gitignored).

| Tool | Version | License | Used for | Source | SHA-256 |
|---|---|---|---|---|---|
| DirectX Shader Compiler (`dxcompiler.dll`, `dxil.dll`) | 1.9.2609 | LLVM (Apache-2.0 with LLVM exception), MIT; `dxil.dll` under Microsoft's license (use on Windows, no redistribution) | DXIL shaders for Direct3D 12 (`JPOG_DXC`, Windows only) | https://github.com/microsoft/DirectXShaderCompiler/releases/tag/v1.9.2609 (`dxc_2026_09_29.zip`) | `ad31b1fc8443175d204f77a611fdb3ef2ec42759bdc2f1167368de24a4a7e7f1` |
