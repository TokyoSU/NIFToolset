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
        unsigned int m_uiWidth     = 1280;
        unsigned int m_uiHeight    = 720;
        const char*  m_pszTitle    = "NiApplication";
        bool         m_bFullscreen = false;
        bool         m_bResizable  = false;
        NiColorA m_kClearColor = NiColorA(0.0f, 0.0f, 0.0f, 1.0f);
        float m_fFrustumLeft   = -0.5625f;
        float m_fFrustumRight  =  0.5625f;
        float m_fFrustumTop    =  0.5f;
        float m_fFrustumBottom = -0.5f;
        float m_fNearPlane     =  1.0f;
        float m_fFarPlane      =  10000.0f;
        bool  m_bAlphaSorting      = true;
        bool  m_bObserveNoSortHint = true;
        bool  m_bSortByClosestPoint = false;
        bool  m_bShadows           = false;
#if defined(NI_RENDERER_DX9)
        NiDX9Renderer::DeviceDesc           m_eDeviceDesc       = NiDX9Renderer::DEVDESC_PURE;
        NiDX9Renderer::FrameBufferFormat    m_eFBFormat         = NiDX9Renderer::FBFMT_UNKNOWN;
        NiDX9Renderer::DepthStencilFormat   m_eDSFormat         = NiDX9Renderer::DSFMT_D24S8;
        NiDX9Renderer::PresentationInterval m_ePresentInterval  = NiDX9Renderer::PRESENT_INTERVAL_ONE;
        NiDX9Renderer::SwapEffect           m_eSwapEffect       = NiDX9Renderer::SWAPEFFECT_DEFAULT;
        unsigned int                        m_uiDX9Flags        = NiDX9Renderer::USE_NOFLAGS;
        unsigned int                        m_uiAdapter         = D3DADAPTER_DEFAULT;
        unsigned int                        m_uiBackBufferCount = 1;
#elif defined(NI_RENDERER_DX10)
        NiD3D10Renderer::DriverType m_eDriverType        = NiD3D10Renderer::DRIVER_HARDWARE;
        unsigned int                m_uiDX10CreateFlags  = 0;
        bool                        m_bCreateDepthBuffer = true;
        DXGI_FORMAT                 m_eDepthFormat       = DXGI_FORMAT_UNKNOWN;
#elif defined(NI_RENDERER_DX11)
        ecr::D3D11Renderer::DriverType m_eDriverType        = ecr::D3D11Renderer::DRIVER_TYPE_HARDWARE;
        unsigned int                   m_uiDX11CreateFlags  = 0;
        bool                           m_bCreateDepthBuffer = true;
        DXGI_FORMAT                    m_eDepthFormat       = DXGI_FORMAT_UNKNOWN;
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

    SDL_Window*            GetWindow()            const;
    NiRenderer*            GetRenderer()          const;
    NiCamera*              GetCamera()            const;
    NiNode*                GetScene()             const;
    unsigned int           GetWidth()             const;
    unsigned int           GetHeight()            const;
    float                  GetLastDeltaTime()     const;
    NiAlphaAccumulator*    GetAlphaAccumulator()  const;
    NiMeshCullingProcess*  GetCullingProcess()    const;
    bool                   GetShadowsEnabled()    const;

    void RenderScene();
    void SetShadowsActive(bool bActive);

private:
    bool  CreateSDLWindow(const Settings& kSettings);
    bool  CreateRenderer (const Settings& kSettings);
    void  DestroyAll();
    bool  DispatchEvent  (const SDL_Event& kEvent);
    float ComputeDeltaTime();

    SDL_Window* m_pWindow = nullptr;
    NiPointer<NiRenderer>          m_spRenderer;
    NiPointer<NiCamera>            m_spCamera;
    NiPointer<NiNode>              m_spScene;
    NiPointer<NiAlphaAccumulator>  m_spAlphaAccum;
    NiPointer<NiMeshCullingProcess> m_spCuller;

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

    Uint64 m_uiPerfFreq = 0;
    Uint64 m_uiLastTick = 0;
    float  m_fLastDelta = 0.0f;

    bool m_bQuit           = false;
    bool m_bInitialized    = false;
    bool m_bShadowsEnabled = false;
};

#endif // NIAPPLICATION_H
