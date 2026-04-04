#include "NiApplication.h"
#include <NiStaticDataManager.h>
#include <NiAlphaAccumulator.h>
#include <NiShadowManager.h>
#include <NiMeshCullingProcess.h>
#include <NiDrawSceneUtility.h>
#include <NiRenderClick.h>
#include <NiMaterial.h>
#include <NiFragmentMaterial.h>
#include <NiDevImageConverter.h>
#include <NiParticleSDM.h>
#include <NiMath.h>

NiApplication::NiApplication() : m_kVisibleSet(1024, 1024) {

}

NiApplication::~NiApplication()
{
    DestroyAll();
}

//--------------------------------------------------------------------------------------------------
void NiApplication::ApplyShaderDefaults(const Settings& kSettings)
{
    // Set the shader cache working directory (where compiled .fxl / cache
    // files are read from and written to). Must be called after NiInit()
    // so NiMaterial's SDM has already zeroed ms_acDefaultWorkingDirectory.
    if (kSettings.m_pszShaderCacheFolder)
        NiMaterial::SetDefaultWorkingDirectory(kSettings.m_pszShaderCacheFolder);
	NiImageConverter::SetImageConverter(NiNew NiDevImageConverter); // Allow loading more image formats.

    // NiFragmentMaterial statics — these are read inside the
    // NiFragmentMaterial constructor and by NiRenderer::SetDefaultProgramCache,
    // so they must be set before any material or renderer is created.
    NiFragmentMaterial::SetDefaultAutoSaveProgramCache(
        kSettings.m_bShaderCacheAutoSave);
    NiFragmentMaterial::SetDefaultWriteDebugProgramData(
        kSettings.m_bShaderCacheWriteDebugData);
    NiFragmentMaterial::SetDefaultLoadProgramCacheOnCreation(
        kSettings.m_bShaderCacheLoadOnCreation);
    NiFragmentMaterial::SetDefaultLockProgramCache(
        kSettings.m_bShaderCacheLocked);
    NiFragmentMaterial::SetDefaultAutoCreateProgramCache(
        kSettings.m_bShaderCacheAutoCreate);
    NiFragmentMaterial::SetDefaultCreateReplacementShaders(
        kSettings.m_bShaderCacheReplacementShaders);
}

//--------------------------------------------------------------------------------------------------
bool NiApplication::Initialize(const Settings& kSettings)
{
    if (m_bInitialized)
        return false;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    NiInit(nullptr, true);
    ApplyShaderDefaults(kSettings);

    if (!CreateSDLWindow(kSettings))
    {
        NiShutdown(true);
        SDL_Quit();
        return false;
    }

    m_uiWidth     = kSettings.m_uiWidth;
    m_uiHeight    = kSettings.m_uiHeight;
    m_kClearColor = kSettings.m_kClearColor;

    if (!CreateRenderer(kSettings))
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
        NiShutdown(true);
        SDL_Quit();
        return false;
    }

    m_spCamera = NiNew NiCamera();
    const float fAspect = static_cast<float>(kSettings.m_uiWidth) /
                          static_cast<float>(kSettings.m_uiHeight);
    const float fSlope  = std::tan(NiDegToRad(kSettings.m_fFov) * 0.5f);

    // NiCamera::SetViewFrustum silently clamps m_fNear to
    // max(m_fFar / m_fMaxFarNearRatio, m_fMinNearPlaneDist).
    // Probe the camera with the requested near/far so it resolves
    // the actual near plane, then recompute half-extents from that value.
    NiFrustum kProbe(0.0f, 0.0f, 0.0f, 0.0f,
                     kSettings.m_fNear, kSettings.m_fFar, false);
    m_spCamera->SetViewFrustum(kProbe);
    const float fActualNear = m_spCamera->GetViewFrustum().m_fNear;

    const float fTop   = fActualNear * fSlope;
    const float fRight = fTop * fAspect;
    NiFrustum kFrustum(-fRight, fRight, fTop, -fTop,
                       fActualNear, kSettings.m_fFar, false);
    NiRect<float> kViewport(0.0f, 1.0f, 1.0f, 0.0f);
    m_spCamera->SetViewFrustum(kFrustum);
    m_spCamera->SetViewPort(kViewport);
	m_spCloner = NiNew NiCloningProcess();
    m_spCuller = NiNew NiMeshCullingProcess(&m_kVisibleSet, nullptr);

    if (kSettings.m_bAlphaSorting)
    {
        m_spAlphaAccum = NiNew NiAlphaAccumulator();
        m_spAlphaAccum->SetObserveNoSortHint(kSettings.m_bObserveNoSortHint);
        m_spAlphaAccum->SetSortByClosestPoint(kSettings.m_bSortByClosestPoint);
        m_spRenderer->SetSorter(m_spAlphaAccum);
    }

    if (kSettings.m_bShadows)
    {
        NiShadowManager::Initialize();
        NiShadowManager::SetActive(true);
        NiShadowManager::SetCullingProcess(m_spCuller);
        m_bShadowsEnabled = true;
    }

    m_uiPerfFreq = SDL_GetPerformanceFrequency();
    m_uiLastTick = SDL_GetPerformanceCounter();

    if (m_pfnInit && !m_pfnInit(this, m_pInitUserData))
    {
        DestroyAll();
        return false;
    }

    NiParticleSDM::Init(); // Not init like other SDMs.

    m_bInitialized = true;
    return true;
}

int NiApplication::Run()
{
    if (!m_bInitialized)
        return -1;

    m_bQuit = false;

    while (!m_bQuit)
    {
        SDL_Event kEvent;
        while (SDL_PollEvent(&kEvent))
        {
            if (!DispatchEvent(kEvent))
            {
                m_bQuit = true;
                break;
            }
        }

        if (m_bQuit)
            break;

        const float fDelta = ComputeDeltaTime();

        if (m_pfnUpdate)
            m_pfnUpdate(this, fDelta, m_pUpdateUserData);

        if (m_spRenderer && m_spRenderer->BeginFrame())
        {
            if (m_bShadowsEnabled && m_spCuller)
            {
                NiShadowManager::SetSceneCamera(m_spCamera);
                const NiTPointerList<NiRenderClick*>& kClicks = NiShadowManager::GenerateRenderClicks();
                const unsigned int uiFrameID = m_spRenderer->GetFrameID();
                NiTListIterator kIter = kClicks.GetHeadPos();
                while (kIter)
                {
                    NiRenderClick* pkClick = kClicks.GetNext(kIter);
                    if (pkClick && pkClick->GetActive())
                        pkClick->Render(uiFrameID);
                }
            }

            if (m_spRenderer->BeginUsingDefaultRenderTargetGroup(NiRenderer::CLEAR_ALL))
            {
                m_spRenderer->SetCameraData(m_spCamera);
                if (m_pfnRender)
                    m_pfnRender(this, m_pRenderUserData);
                m_spRenderer->EndUsingRenderTargetGroup();
            }
            m_spRenderer->EndFrame();
            m_spRenderer->DisplayFrame();
        }
    }

    return 0;
}

void NiApplication::Quit()
{
    m_bQuit = true;
}

void NiApplication::SetInitCallback(InitCallback pfn, void* pUserData)
{ m_pfnInit = pfn; m_pInitUserData = pUserData; }

void NiApplication::SetShutdownCallback(ShutdownCallback pfn, void* pUserData)
{ m_pfnShutdown = pfn; m_pShutdownUserData = pUserData; }

void NiApplication::SetUpdateCallback(UpdateCallback pfn, void* pUserData)
{ m_pfnUpdate = pfn; m_pUpdateUserData = pUserData; }

void NiApplication::SetRenderCallback(RenderCallback pfn, void* pUserData)
{ m_pfnRender = pfn; m_pRenderUserData = pUserData; }

void NiApplication::SetEventCallback(EventCallback pfn, void* pUserData)
{ m_pfnEvent = pfn; m_pEventUserData = pUserData; }

SDL_Window*  NiApplication::GetWindow()        const { return m_pWindow; }
NiRenderer*  NiApplication::GetRenderer()      const { return m_spRenderer; }
NiCamera*    NiApplication::GetCamera()        const { return m_spCamera; }
unsigned int NiApplication::GetWidth()         const { return m_uiWidth; }
unsigned int NiApplication::GetHeight()        const { return m_uiHeight; }
float        NiApplication::GetLastDeltaTime()    const { return m_fLastDelta; }
NiAlphaAccumulator*   NiApplication::GetAlphaAccumulator() const { return m_spAlphaAccum; }
NiMeshCullingProcess* NiApplication::GetCullingProcess()   const { return m_spCuller; }
NiCloningProcess* NiApplication::GetCloningProcess() const { return m_spCloner; }
bool         NiApplication::GetShadowsEnabled()   const { return m_bShadowsEnabled; }

void NiApplication::RenderScene()
{
    if (!m_spScene || !m_spCamera || !m_spCuller)
        return;
    NiDrawScene(m_spCamera, m_spScene, *m_spCuller);
}

void NiApplication::SetShadowsActive(bool bActive)
{
    if (bActive == m_bShadowsEnabled)
        return;
    if (bActive)
    {
        NiShadowManager::Initialize();
        NiShadowManager::SetActive(true);
        if (m_spCuller)
            NiShadowManager::SetCullingProcess(m_spCuller);
    }
    else
    {
        NiShadowManager::SetActive(false);
        NiShadowManager::Shutdown();
    }
    m_bShadowsEnabled = bActive;
}

bool NiApplication::CreateSDLWindow(const Settings& kSettings)
{
    SDL_WindowFlags uiFlags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (kSettings.m_bFullscreen)
        uiFlags |= SDL_WINDOW_FULLSCREEN;
    if (kSettings.m_bResizable)
        uiFlags |= SDL_WINDOW_RESIZABLE;

    m_pWindow = SDL_CreateWindow(
        kSettings.m_pszTitle,
        static_cast<int>(kSettings.m_uiWidth),
        static_cast<int>(kSettings.m_uiHeight),
        uiFlags);

    if (!m_pWindow)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool NiApplication::CreateRenderer(const Settings& kSettings)
{
    HWND hWnd = (HWND)SDL_GetPointerProperty(
        SDL_GetWindowProperties(m_pWindow),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
        nullptr);

    if (!hWnd)
    {
        SDL_Log("Failed to retrieve HWND from SDL window: %s", SDL_GetError());
        return false;
    }

#if defined(NI_RENDERER_DX9)
    unsigned int uiFlags = kSettings.m_uiDX9Flags;
    if (kSettings.m_bFullscreen)
        uiFlags |= NiDX9Renderer::USE_FULLSCREEN;

    NiDX9Renderer* pRenderer = NiDX9Renderer::Create(
        kSettings.m_uiWidth, kSettings.m_uiHeight,
        uiFlags,
        reinterpret_cast<NiWindowRef>(hWnd),
        reinterpret_cast<NiWindowRef>(hWnd),
        kSettings.m_uiAdapter,
        kSettings.m_eDeviceDesc,
        kSettings.m_eFBFormat,
        kSettings.m_eDSFormat,
        kSettings.m_ePresentInterval,
        kSettings.m_eSwapEffect,
        NiDX9Renderer::FBMODE_DEFAULT,
        kSettings.m_uiBackBufferCount,
        NiDX9Renderer::REFRESHRATE_DEFAULT);

    if (!pRenderer)
    {
        SDL_Log("NiDX9Renderer::Create failed.");
        return false;
    }

    m_spRenderer = pRenderer;
    if (!m_spRenderer) {
        SDL_Log("Failed to assign renderer to m_spRenderer.");
        return false;
    }

    m_spRenderer->SetBackgroundColor(m_kClearColor);
    return true;

#elif defined(NI_RENDERER_DX10)
    NiD3D10Renderer::CreationParameters kParams(hWnd);
    kParams.m_eDriverType                   = kSettings.m_eDriverType;
    kParams.m_uiCreateFlags                 = kSettings.m_uiDX10CreateFlags;
    kParams.m_bCreateDepthStencilBuffer     = kSettings.m_bCreateDepthBuffer;
    kParams.m_eDepthStencilFormat           = kSettings.m_eDepthFormat;
    kParams.m_kSwapChain.Windowed           = !kSettings.m_bFullscreen;
    kParams.m_kSwapChain.BufferCount        = kSettings.m_uiBackBufferCount;
    kParams.m_kSwapChain.SampleDesc.Count   = kSettings.m_uiSampleCount;
    kParams.m_kSwapChain.SampleDesc.Quality = kSettings.m_uiSampleQuality;
    kParams.m_kSwapChain.BufferDesc.Width   = kSettings.m_uiWidth;
    kParams.m_kSwapChain.BufferDesc.Height  = kSettings.m_uiHeight;
    kParams.m_kSwapChain.BufferDesc.Format  = kSettings.m_eSwapChainBuffer;

    NiPointer<NiD3D10Renderer> spD3D10;
    if (!NiD3D10Renderer::Create(kParams, spD3D10) || !spD3D10)
    {
        SDL_Log("NiD3D10Renderer::Create failed.");
        return false;
    }

    m_spRenderer = spD3D10;
    if (!m_spRenderer) {
        SDL_Log("Failed to assign renderer to m_spRenderer.");
        return false;
    }

    m_spRenderer->SetBackgroundColor(m_kClearColor);
    return true;

#elif defined(NI_RENDERER_DX11)
    ecr::D3D11Renderer::CreationParameters kParams(hWnd);
    kParams.m_driverType                    = kSettings.m_eDriverType;
    kParams.m_createFlags                   = kSettings.m_uiDX11CreateFlags;
    kParams.m_createDepthStencilBuffer      = kSettings.m_bCreateDepthBuffer;
    kParams.m_depthStencilFormat            = kSettings.m_eDepthFormat;
	kParams.m_swapChain.Windowed            = !kSettings.m_bFullscreen;
	kParams.m_swapChain.SampleDesc.Count    = kSettings.m_uiSampleCount;
	kParams.m_swapChain.SampleDesc.Quality  = kSettings.m_uiSampleQuality;
	kParams.m_swapChain.BufferCount         = kSettings.m_uiBackBufferCount;
    kParams.m_swapChain.BufferDesc.Width    = kSettings.m_uiWidth;
    kParams.m_swapChain.BufferDesc.Height   = kSettings.m_uiHeight;
	kParams.m_swapChain.BufferDesc.Format   = kSettings.m_eSwapChainBuffer;

    ecr::D3D11RendererPtr spD3D11;
    if (!ecr::D3D11Renderer::Create(kParams, spD3D11) || !spD3D11)
    {
        SDL_Log("ecr::D3D11Renderer::Create failed.");
        return false;
    }

    m_spRenderer = spD3D11;
    if (!m_spRenderer) {
        SDL_Log("Failed to assign renderer to m_spRenderer.");
		return false;
    }

    m_spRenderer->SetBackgroundColor(m_kClearColor);
    return true;

#else
    (void)hWnd;
    SDL_Log("No DX renderer selected. Define NI_RENDERER_DX9, _DX10 or _DX11.");
    return false;
#endif
}

void NiApplication::DestroyAll()
{
    if (!m_bInitialized && !m_pWindow)
        return;

    if (m_pfnShutdown)
        m_pfnShutdown(this, m_pShutdownUserData);

    m_spScene    = nullptr;
    m_spCamera   = nullptr;

    if (m_bShadowsEnabled)
    {
        NiShadowManager::SetActive(false);
        NiShadowManager::Shutdown();
        m_bShadowsEnabled = false;
    }

    m_spCuller     = nullptr;
    m_spAlphaAccum = nullptr;
    m_spRenderer   = nullptr;

    if (m_pWindow)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }

    NiParticleSDM::Shutdown();
    NiShutdown(true);
    SDL_Quit();
    m_bInitialized = false;
}

bool NiApplication::DispatchEvent(const SDL_Event& kEvent)
{
    if (m_pfnEvent && m_pfnEvent(this, kEvent, m_pEventUserData))
        return true;

    switch (kEvent.type)
    {
    case SDL_EVENT_QUIT:
        return false;

    case SDL_EVENT_WINDOW_RESIZED:
        if (kEvent.window.windowID == SDL_GetWindowID(m_pWindow))
        {
            m_uiWidth  = static_cast<unsigned int>(kEvent.window.data1);
            m_uiHeight = static_cast<unsigned int>(kEvent.window.data2);
        }
        break;

    case SDL_EVENT_KEY_DOWN:
        if (kEvent.key.key == SDLK_ESCAPE)
            return false;
        break;

    default:
        break;
    }

    return true;
}

float NiApplication::ComputeDeltaTime()
{
    const Uint64 uiNow = SDL_GetPerformanceCounter();
    const float fDelta = (m_uiPerfFreq > 0)
        ? static_cast<float>(uiNow - m_uiLastTick) / static_cast<float>(m_uiPerfFreq)
        : 0.0f;
    m_uiLastTick = uiNow;
    m_fLastDelta = fDelta;
    return fDelta;
}

