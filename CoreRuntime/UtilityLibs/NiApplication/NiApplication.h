#pragma once
#ifndef NIAPPLICATION_H
#define NIAPPLICATION_H

#include "NiApplicationLibType.h"

#include <SDL3/SDL.h>
#include <NiRenderer.h>
#include <NiCamera.h>
#include <NiNode.h>
#include <NiColor.h>
#include <NiFrustum.h>
#include <NiAlphaAccumulator.h>
#include <NiMeshCullingProcess.h>
#include <NiCullingProcess.h>
#include <NiCloningProcess.h>

#if defined(NI_RENDERER_DX9)
    #include <NiDX9Renderer.h>
#elif defined(NI_RENDERER_DX10)
    #include <NiD3D10Renderer.h>
#elif defined(NI_RENDERER_DX11)
    #include <ecrD3D11Renderer/D3D11Renderer.h>
#endif

class NIAPPLICATION_ENTRY NiApplication
{
public:
    typedef bool (*InitCallback)(NiApplication* pApp, void* pUserData);
    typedef void (*ShutdownCallback)(NiApplication* pApp, void* pUserData);
    typedef void (*UpdateCallback)(NiApplication* pApp, float fDeltaTime, void* pUserData);
    typedef void (*RenderCallback)(NiApplication* pApp, void* pUserData);
    typedef bool (*EventCallback)(NiApplication* pApp, const SDL_Event& kEvent, void* pUserData);

    struct Settings
    {
        // ---------------------------------------------------------------
        // Window / viewport
        // ---------------------------------------------------------------
        unsigned int m_uiWidth     = 1280;
        unsigned int m_uiHeight    = 720;
        unsigned int m_uiBackBufferCount = 1;
        unsigned int m_uiSampleCount = 1;
		unsigned int m_uiSampleQuality = 0;
        const char*  m_pszTitle    = "NiApplication";
        bool         m_bFullscreen = false;
        bool         m_bResizable  = false;

        NiColorA m_kClearColor     = NiColorA(0.0f, 0.0f, 0.0f, 1.0f);

        // Vertical field of view in radians, near and far clip planes.
        // The frustum half-extents are derived at initialisation from
        // these values and the window aspect ratio (m_uiWidth / m_uiHeight).
        float m_fFov  = 45.0f;
        float m_fNear = 0.01f;
        float m_fFar  = 1000.0f;

        // ---------------------------------------------------------------
        // Rendering features
        // ---------------------------------------------------------------
        bool m_bAlphaSorting       = true;
        bool m_bObserveNoSortHint  = true;
        bool m_bSortByClosestPoint = false;
        bool m_bShadows            = false;

        // ---------------------------------------------------------------
        // Shader / material cache
        //
        // These are applied to NiMaterial / NiFragmentMaterial statics
        // after NiInit() but before the renderer is created, so that any
        // NiFragmentMaterial constructed during renderer initialisation
        // picks up the correct values.
        //
        // m_pszShaderCacheFolder — directory where compiled shader
        //   programs (.fxl / cache files) are read from and written to.
        //   Relative paths are resolved from the working directory.
        //   Pass nullptr to leave the engine default ("") unchanged.
        // ---------------------------------------------------------------
        const char* m_pszShaderCacheFolder          = "ShaderCache";
        bool        m_bShaderCacheAutoSave           = true;
        bool        m_bShaderCacheWriteDebugData     = false;
        bool        m_bShaderCacheLoadOnCreation     = true;
        bool        m_bShaderCacheLocked             = false;
        bool        m_bShaderCacheAutoCreate         = true;
        bool        m_bShaderCacheReplacementShaders = true;

        // ---------------------------------------------------------------
        // Renderer-specific
        // ---------------------------------------------------------------
#if defined(NI_RENDERER_DX9)
        NiDX9Renderer::DeviceDesc           m_eDeviceDesc       = NiDX9Renderer::DEVDESC_PURE;
        NiDX9Renderer::FrameBufferFormat    m_eFBFormat         = NiDX9Renderer::FBFMT_UNKNOWN;
        NiDX9Renderer::DepthStencilFormat   m_eDSFormat         = NiDX9Renderer::DSFMT_D24S8;
        NiDX9Renderer::PresentationInterval m_ePresentInterval  = NiDX9Renderer::PRESENT_INTERVAL_ONE;
        NiDX9Renderer::SwapEffect           m_eSwapEffect       = NiDX9Renderer::SWAPEFFECT_DEFAULT;
        unsigned int                        m_uiDX9Flags        = NiDX9Renderer::USE_NOFLAGS;
        unsigned int                        m_uiAdapter         = D3DADAPTER_DEFAULT;
#elif defined(NI_RENDERER_DX10)
        NiD3D10Renderer::DriverType m_eDriverType           = NiD3D10Renderer::DRIVER_HARDWARE;
        unsigned int                m_uiDX10CreateFlags     = 0;
        bool                        m_bCreateDepthBuffer    = true;
        DXGI_FORMAT                 m_eDepthFormat          = DXGI_FORMAT_UNKNOWN;
        DXGI_FORMAT                 m_eSwapChainBuffer      = DXGI_FORMAT_R8G8B8A8_UNORM;
#elif defined(NI_RENDERER_DX11)
        ecr::D3D11Renderer::DriverType m_eDriverType        = ecr::D3D11Renderer::DRIVER_TYPE_HARDWARE;
        unsigned int                   m_uiDX11CreateFlags  = 0;
        bool                           m_bCreateDepthBuffer = true;
        DXGI_FORMAT                    m_eDepthFormat       = DXGI_FORMAT_D24_UNORM_S8_UINT;
		DXGI_FORMAT                    m_eSwapChainBuffer   = DXGI_FORMAT_R8G8B8A8_UNORM;
#endif
    };

    NiApplication();
    ~NiApplication();
    NiApplication(const NiApplication&)            = delete;
    NiApplication& operator=(const NiApplication&) = delete;

    bool Initialize(const Settings& kSettings = Settings{});
    int  Run();
    void Quit();

    void SetInitCallback    (InitCallback     pfn, void* pUserData = nullptr);
    void SetShutdownCallback(ShutdownCallback pfn, void* pUserData = nullptr);
    void SetUpdateCallback  (UpdateCallback   pfn, void* pUserData = nullptr);
    void SetRenderCallback  (RenderCallback   pfn, void* pUserData = nullptr);
    void SetEventCallback   (EventCallback    pfn, void* pUserData = nullptr);

    SDL_Window*            GetWindow()           const;
    NiRenderer*            GetRenderer()         const;
    NiCamera*              GetCamera()           const;
    unsigned int           GetWidth()            const;
    unsigned int           GetHeight()           const;
    float                  GetDeltaTime()        const;
    float 				   GetTime()             const;
    NiAlphaAccumulator*    GetAlphaAccumulator() const;
    NiMeshCullingProcess*  GetCullingProcess()   const;
    bool                   GetShadowsEnabled()   const;

    void RenderScene();
    void SetShadowsActive(bool bActive);

	// Render the scene to the specified render target group, or to the main back buffer if pRenderTargetGroup is null.
    bool BeginScene(NiRenderTargetGroup* pRenderTargetGroup = nullptr, NiRenderer::ClearFlags clearFlags = NiRenderer::ClearFlags::CLEAR_ALL);
	// End the scene. If BeginScene() return true then this must be called before calling Present().
    void EndScene();
    void Present();

private:
    bool  CreateSDLWindow(const Settings& kSettings);
    bool  CreateRenderer (const Settings& kSettings);
    void  ApplyShaderDefaults(const Settings& kSettings);
    void  DestroyAll();
    bool  DispatchEvent(const SDL_Event& kEvent);
    float ComputeDeltaTime();

    SDL_Window* m_pWindow = nullptr;
    NiPointer<NiRenderer>           m_spRenderer;
    NiPointer<NiCamera>             m_spCamera;
    NiPointer<NiNode>               m_spScene;
    NiPointer<NiAlphaAccumulator>   m_spAlphaAccum;
    NiPointer<NiMeshCullingProcess> m_spCuller;
	NiVisibleArray m_kVisibleSet;

    unsigned int m_uiWidth     = 0;
    unsigned int m_uiHeight    = 0;
    NiColorA     m_kClearColor;

    InitCallback     m_pfnInit           = nullptr;
    void*            m_pInitUserData     = nullptr;
    ShutdownCallback m_pfnShutdown       = nullptr;
    void*            m_pShutdownUserData = nullptr;
    UpdateCallback   m_pfnUpdate         = nullptr;
    void*            m_pUpdateUserData   = nullptr;
    RenderCallback   m_pfnRender         = nullptr;
    void*            m_pRenderUserData   = nullptr;
    EventCallback    m_pfnEvent          = nullptr;
    void*            m_pEventUserData    = nullptr;

    bool m_bInitialized  = false;
    bool m_bQuit         = false;
    bool m_bShadowsEnabled = false;

    Uint64 m_uiPerfFreq = 0;
    Uint64 m_uiLastTick = 0;
    float  m_fLastDelta = 0.0f;
	float  m_fTime      = 0.0f;
};

#endif // NIAPPLICATION_H

