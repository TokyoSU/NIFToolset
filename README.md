# NIFToolset

NIFToolset is a Gamebryo-oriented runtime/tooling tree. The active renderer in this revision is **bgfx**, with **SDL3** used by `NiApplication` for window creation and events.

## Dependencies

The project uses a vcpkg manifest (`vcpkg.json`). The renderer depends on `bgfx[tools]`; the tools feature supplies `shaderc`, which CMake uses to build the renderer shaders.

On Windows, set `VCPKG_ROOT` or pass the vcpkg toolchain explicitly before configuring:

```bat
set VCPKG_ROOT=C:\path\to\vcpkg
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

bgfx is the only renderer backend in this tree. The legacy DX9/D3D10/D3D11 renderer modules and their compile-time selection macros have been removed.

## bgfx shaders

`CoreRuntime/EngineLibs/NiBgfxRenderer/Shaders` contains the canonical shader sources. CMake compiles backend-specific binaries with bgfx `shaderc` and makes `NiBgfxRenderer` depend on the shader target.

Windows builds generate shader sets for:

- Direct3D 11 / Direct3D 12 (`dx11`, Shader Model 5.0)
- OpenGL (`glsl`)
- Vulkan (`spirv`)

At runtime `BgfxRenderer` searches, in order:

1. `NiApplication::Settings::m_pszBgfxShaderRoot`, when explicitly set;
2. the CMake build shader directory;
3. `Shaders/bgfx` next to the executable (installed layout);
4. `Shaders/bgfx` relative to the current working directory.

## NiApplication

`NiApplication` creates an SDL3 window and a `BgfxRenderer`. A minimal setup is:

```cpp
NiApplication::Settings settings;
settings.m_bFullscreen = false;
settings.m_kClearColor = NiColorA(0.0f, 0.0f, 0.0f, 1.0f);
settings.m_pszTitle = "Your game application name";
settings.m_uiWidth = 1920;
settings.m_uiHeight = 1080;
settings.m_fNear = 0.01f;
settings.m_fFar = 10000.0f;
settings.m_fFov = 45.0f;              // Degrees.
settings.m_bAlphaSorting = true;
settings.m_bShadows = false;          // Shadow-material parity is still incomplete.
settings.m_bResizable = true;
settings.m_bVSync = true;
settings.m_pszBgfxShaderRoot = nullptr; // Use automatic shader discovery.

m_pApp = new NiApplication();
m_pApp->SetInitCallback(&CApplication::OnInit, this);
m_pApp->SetShutdownCallback(&CApplication::OnShutdown, this);
m_pApp->SetEventCallback(&CApplication::OnSDLEvent, this);
m_pApp->SetUpdateCallback(&CApplication::OnUpdate, this);
m_pApp->SetRenderCallback(&CApplication::OnRender, this);
// m_pApp->SetResizeCallback(&CApplication::OnResize, this);

if (m_pApp->Initialize(settings))
{
    m_pCamera = m_pApp->GetCamera();
    m_pApp->Run();
}

bool CApplication::OnInit(NiApplication* pApp, void* pUserData)
{
    CApplication* pThis = static_cast<CApplication*>(pUserData);
    return true;
}

void CApplication::OnShutdown(NiApplication* pApp, void* pUserData)
{
    CApplication* pThis = static_cast<CApplication*>(pUserData);
}

void CApplication::OnUpdate(NiApplication* pApp, float fDeltaTime, void* pUserData)
{
    CApplication* pThis = static_cast<CApplication*>(pUserData);
}

void CApplication::OnRender(NiApplication* pApp, void* pUserData)
{
    CApplication* pThis = static_cast<CApplication*>(pUserData);

    pApp->BeginFrame();
    pApp->BeginScene();
    // Draw here.
    pApp->EndScene();
    pApp->EndFrame();
    pApp->Present();
}

bool CApplication::OnSDLEvent(
    NiApplication* pApp, const SDL_Event& kEvent, void* pUserData)
{
    CApplication* pThis = static_cast<CApplication*>(pUserData);
    return false;
}
```

The older `m_eDriverType`, DXGI back-buffer/depth format settings, and D3D shader-cache renderer fields are no longer renderer settings for `NiApplication`.

## bgfx renderer status

The current backend implements the Gamebryo-facing `NiRenderer` lifecycle rather than only wrapping `bgfx::init()`/`bgfx::frame()`. Implemented paths include:

- default and secondary-window render-target groups;
- resize/reset and presentation;
- camera/view/projection setup and screen-space camera setup;
- source textures, source cube maps, dynamic textures, rendered textures/cube maps, and depth/stencil buffers;
- offscreen render-target framebuffer creation;
- mip skipping and source texture revision refresh;
- transient mesh submission for common POSITION, TEXCOORD, COLOR, and 16/32-bit INDEX streams;
- triangle, triangle-strip, line, line-strip, and point primitives;
- world transforms;
- depth test/write, alpha blending/testing, stencil state, and face culling;
- texture wrap/filter selection, base-map UV set selection, and `NiTextureTransform`;
- `APPLY_REPLACE`, `APPLY_DECAL`, and `APPLY_MODULATE` base-texture behavior;
- `NiVertexColorProperty` source-mode handling;
- offscreen texture blits and rendered-texture screenshot/readback.

See [`BGFX_PORT_STATUS.md`](BGFX_PORT_STATUS.md) for the remaining parity work and deliberately unadvertised capabilities.

## NiAudio

NiMilesAudio was removed and replaced with the BASS-based `NiBASSAudio` implementation. Miles used proprietary middleware for which this repository does not contain a complete buildable source distribution.

## Supported asset formats

Images currently handled by the tool/runtime image path include BMP, TGA, SGI, and DDS. Gamebryo asset paths include KFM, NIF, and KF. Terrain data continues to use the repository's sector/archive format.
