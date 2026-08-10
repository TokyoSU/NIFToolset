# bgfx renderer port status

This file tracks the renderer migration independently from the legacy Direct3D source that remains in the repository.

## Implemented

- [x] `BgfxRenderer` derives from `NiRenderer` and is installed as the active renderer.
- [x] SDL3 native-window integration through `NiApplication`.
- [x] bgfx initialize/reset/frame/shutdown lifecycle.
- [x] Default Gamebryo back-buffer/depth-buffer objects and render-target group.
- [x] Secondary native-window render-target groups.
- [x] Per-render-pass bgfx view allocation and framebuffer binding.
- [x] Camera, viewport, perspective, orthographic, and screen-space transforms.
- [x] Source 2D textures and cube maps.
- [x] DXT1/BC1, DXT3/BC2, DXT5/BC3 uploads plus conversion of unsupported source layouts to RGBA32.
- [x] Source pixel/palette revision tracking and mip-skip refresh.
- [x] Dynamic textures with lock/staging/upload.
- [x] Rendered 2D textures, rendered cube maps, and depth/stencil textures.
- [x] Render-target framebuffer assembly.
- [x] Offscreen blit and readback used by snapshot generation.
- [x] Common Gamebryo vertex POSITION/TEXCOORD/COLOR formats and 16/32-bit index streams.
- [x] Triangles, triangle strips, lines, line strips, and points.
- [x] World transforms.
- [x] Z test/write, alpha blend/test, culling, and stencil operations.
- [x] Texture clamp/filter modes and anisotropic sampler request.
- [x] Base-map UV-set selection and `NiTextureTransform`.
- [x] Base texture `REPLACE`, `DECAL`, and `MODULATE` apply modes.
- [x] `NiVertexColorProperty` source-mode handling.
- [x] Build-time shader compilation through the bgfx CMake package's `shaderc` target.
- [x] Runtime shader lookup for build trees and installed executable layouts.
- [x] Legacy D3D renderer targets removed from the active bgfx build dependency chain.

## Intentionally not advertised yet

The following `NiRenderer` capability bits stay disabled so Gamebryo does not select code paths that are not implemented:

- [ ] Hardware skinning.
- [ ] Hardware instancing.

Meshes therefore use the existing CPU/modifier path followed by transient bgfx submission for now.

## Remaining parity work

These are real renderer features that still need a bgfx-native implementation; they are not hidden behind a false capability flag.

- [ ] Full `NiStandardMaterial` lighting parity: ambient/directional/point/spot lights, emissive/specular/gloss, and vertex-lighting modes.
- [ ] Fog (`NiFogProperty`) and fog-map behavior.
- [ ] Multi-texture standard material stages: detail, decal, glow, bump/parallax, environment maps, projected lights/shadows, and related texture effects.
- [ ] Dedicated bgfx material/shader path for terrain, light-prepass, and other legacy `NiFragmentMaterial` implementations.
- [ ] Shadow-write material parity. `GetShadowWriteShader()` intentionally returns `nullptr` at present.
- [ ] Dedicated wireframe topology generation for `NiWireframeProperty`.
- [ ] Scaled `Ni2DBuffer::Copy`; bgfx blit currently handles same-size regions only.
- [ ] Default swap-chain screenshot capture. Current readback is implemented for texture-backed/offscreen render targets.
- [ ] GPU-resident mesh buffer caching. Current mesh submission deliberately uses transient buffers to establish correctness first.
- [ ] Hardware skinning and hardware instancing once stable GPU mesh data/vertex layouts exist.
- [ ] Broader backend coverage (Metal/WebGPU) if the rest of the Gamebryo platform layer is ported beyond Windows.

## Validation notes

The source tree is Windows/Gamebryo specific, so a definitive validation build should be performed with MSVC on Windows and the manifest dependencies installed through vcpkg. CMake is structured so `bgfx[tools]` supplies both the runtime library and shader compiler used by `NiBgfxShaders`.
