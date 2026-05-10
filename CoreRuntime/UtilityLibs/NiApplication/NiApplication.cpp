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
#include <NiPortal/NiPortalSDM.h>
#include <NiMath.h>
#include <NiBillboardNode.h>
#include <NiParticleSystem.h>
#include <NiPSParticleSystem.h>
#include <NiMeshHWInstance.h>
#include <NiShadowGenerator.h>
#include <NiDX9Renderer.h>
#include <NiD3D10Renderer.h>
#include <ecrD3D11Renderer/D3D11Renderer.h>
#include <NiNode.h>
#include <NiDefaultClickRenderStep.h>
#include <NiViewRenderClick.h>
#include <Ni3DRenderView.h>
#include <NiAlphaSortProcessor.h>

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
    const float fAspect = static_cast<float>(kSettings.m_uiWidth) / static_cast<float>(kSettings.m_uiHeight);
    const float fSlope = std::tan(NiDegToRad(kSettings.m_fFov) * 0.5f);
    float fNear = kSettings.m_fNear;
    const float fTop = fNear * fSlope;
    const float fRight = fTop * fAspect;

    NiFrustum kFrustum(-fRight, fRight, fTop, -fTop, fNear, kSettings.m_fFar, false);
    NiRect<float> kViewport(0.0f, 1.0f, 1.0f, 0.0f);
    m_spCamera->SetViewFrustum(kFrustum);
    m_spCamera->SetViewPort(kViewport);
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
        NiShadowManager::SetActiveShadowClickGenerator("NiDefaultShadowClickGenerator");
        m_bShadowsEnabled = true;
    }

    CreateRenderPipeline();

    m_uiPerfFreq = SDL_GetPerformanceFrequency();
    m_uiLastTick = SDL_GetPerformanceCounter();

    if (m_pfnInit && !m_pfnInit(this, m_pInitUserData))
    {
        DestroyAll();
        return false;
    }

    // Not init like other SDMs.
    NiParticleSDM::Init();
    NiPortalSDM::Init();

    m_bInitialized = true;
    return true;
}

int NiApplication::Run(double targetFps)
{
    if (!m_bInitialized)
        return -1;

    m_bQuit = false;

    const double kTargetFrameSeconds = 1.0 / targetFps;
    const Uint64 uiPerfFreq = (m_uiPerfFreq > 0) ? m_uiPerfFreq : SDL_GetPerformanceFrequency();
    const double fPerfFreq = static_cast<double>(uiPerfFreq);

    while (!m_bQuit)
    {
        const Uint64 uiFrameStart = SDL_GetPerformanceCounter();

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
        if (m_pfnRender)
            m_pfnRender(this, m_pRenderUserData);

        const Uint64 uiFrameEnd = SDL_GetPerformanceCounter();
        const double fElapsedSeconds = static_cast<double>(uiFrameEnd - uiFrameStart) / fPerfFreq;
        if (fElapsedSeconds < kTargetFrameSeconds)
        {
            const double fRemainingSeconds = kTargetFrameSeconds - fElapsedSeconds;
            const Uint32 uiSleepMs = static_cast<Uint32>(fRemainingSeconds * 1000.0);
            if (uiSleepMs > 1)
                SDL_Delay(uiSleepMs - 1);

            while (true)
            {
                const Uint64 uiNow = SDL_GetPerformanceCounter();
                const double fTotalSeconds = static_cast<double>(uiNow - uiFrameStart) / fPerfFreq;
                if (fTotalSeconds >= kTargetFrameSeconds)
                    break;
                SDL_Delay(0);
            }
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

void NiApplication::SetResizeCallback(ResizeCallback pfn, void* pUserData)
{ m_pfnResize = pfn; m_pResizeUserData = pUserData; }

SDL_Window*             NiApplication::GetWindow()              const { return m_pWindow; }
NiRenderer*             NiApplication::GetRenderer()            const { return m_spRenderer; }
NiCamera*               NiApplication::GetCamera()              const { return m_spCamera; }
unsigned int            NiApplication::GetWidth()               const { return m_uiWidth; }
unsigned int            NiApplication::GetHeight()              const { return m_uiHeight; }
float                   NiApplication::GetDeltaTime()           const { return m_fLastDelta; }
float                   NiApplication::GetTime()                const { return m_fTime; }
NiAlphaAccumulator*     NiApplication::GetAlphaAccumulator()    const { return m_spAlphaAccum; }
NiMeshCullingProcess*   NiApplication::GetCullingProcess()      const { return m_spCuller; }
bool                    NiApplication::GetShadowsEnabled()      const { return m_bShadowsEnabled; }

#if WIN32
HWND NiApplication::GetHandle() const
#else
void* NiApplication::GetHandle() const
#endif
{
	return m_hWnd;
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

//--------------------------------------------------------------------------------------------------
void NiApplication::CreateRenderPipeline()
{
    // --- Main click: renders the primary scene (terrain, statics, NPCs). ---
    // No scene is attached here; the caller populates scenes via the render
    // view returned by GetMainRenderView (or directly through BeginScene).
    // A Ni3DRenderView with no scenes attached simply produces no geometry.
    NiPointer<Ni3DRenderView> spMainView = NiNew Ni3DRenderView(m_spCamera, m_spCuller);

    m_spMainClick = NiNew NiViewRenderClick();
    m_spMainClick->AppendRenderView(spMainView);
    m_spMainClick->SetClearAllBuffers(false);  // caller opens + clears the RTG via BeginScene
    m_spMainClick->SetProcessor(NiNew NiAlphaSortProcessor());

    // --- Water click: renders water/lava on top, reusing the depth buffer. ---
    m_spWaterView = NiNew Ni3DRenderView(m_spCamera, m_spCuller);

    m_spWaterClick = NiNew NiViewRenderClick();
    m_spWaterClick->AppendRenderView(m_spWaterView);
    m_spWaterClick->SetClearColorBuffers(false);   // keep colour from main pass
    m_spWaterClick->SetClearDepthBuffer(false);    // depth-test water against main pass
    m_spWaterClick->SetClearStencilBuffer(false);
    m_spWaterClick->SetProcessor(NiNew NiAlphaSortProcessor());
    m_spWaterClick->SetActive(false);              // inactive until a scene is set

    // --- Render step: ordered sequence of clicks. ---
    m_spRenderStep = NiNew NiDefaultClickRenderStep();
    m_spRenderStep->AppendRenderClick(m_spMainClick);
    m_spRenderStep->AppendRenderClick(m_spWaterClick);
}

//--------------------------------------------------------------------------------------------------
void NiApplication::SetWaterScene(NiAVObject* pkScene)
{
    if (!m_spWaterView || !m_spWaterClick)
        return;

    m_spWaterView->RemoveAllScenes();
    if (pkScene)
    {
        m_spWaterView->AppendScene(pkScene);
        m_spWaterClick->SetActive(true);
    }
    else
    {
        m_spWaterClick->SetActive(false);
    }
}

//--------------------------------------------------------------------------------------------------
void NiApplication::ClearWaterScene()
{
    if (!m_spWaterView || !m_spWaterClick)
        return;

    m_spWaterView->RemoveAllScenes();
    m_spWaterClick->SetActive(false);
}

//--------------------------------------------------------------------------------------------------
void NiApplication::DrawMainPass()
{
    if (!m_spMainClick || !m_spRenderer)
        return;

    // Route to the currently open RTG so the click stays in the
    // "same RTG" branch (ClearBuffer only, no Begin/End).
    NiRenderTargetGroup* pkCurrent = const_cast<NiRenderTargetGroup*>(
        m_spRenderer->GetCurrentRenderTargetGroup());
    m_spMainClick->SetRenderTargetGroup(pkCurrent);

    m_spMainClick->Render(m_spRenderer->GetFrameID());

    m_spMainClick->SetRenderTargetGroup(nullptr);
}

//--------------------------------------------------------------------------------------------------
void NiApplication::DrawWaterPass()
{
    if (!m_spWaterClick || !m_spWaterClick->GetActive())
        return;
    if (!m_spRenderer)
        return;

    // The water click must render into the same RTG that is currently open
    // so that it depth-tests against what was already drawn by RenderState().
    // We route it to the current open group so NiRenderClick::Render() stays
    // in the "same RTG" branch (ClearBuffer only, no Begin/End).
    NiRenderTargetGroup* pkCurrent = const_cast<NiRenderTargetGroup*>(m_spRenderer->GetCurrentRenderTargetGroup());
    m_spWaterClick->SetRenderTargetGroup(pkCurrent);

    m_spWaterClick->Render(m_spRenderer->GetFrameID());

    // Reset to nullptr so the click reverts to the default RTG next call.
    m_spWaterClick->SetRenderTargetGroup(nullptr);
}

//--------------------------------------------------------------------------------------------------
Ni3DRenderView* NiApplication::GetMainRenderView() const
{
    if (!m_spMainClick)
        return nullptr;
    NiRenderView* pkView = m_spMainClick->GetRenderViews().GetHead();
    return pkView ? NiDynamicCast(Ni3DRenderView, pkView) : nullptr;
}

//--------------------------------------------------------------------------------------------------
NiDefaultClickRenderStep* NiApplication::GetRenderStep() const
{
    return m_spRenderStep;
}

//--------------------------------------------------------------------------------------------------
bool NiApplication::BeginScene(NiRenderTargetGroup* pRenderTargetGroup, NiRenderer::ClearFlags clearFlags)
{
	if (!m_spRenderer)
		return false;

	// Open the render target group so that subsequent direct rendering calls
	// (e.g. NiDrawScene, NiAlphaAccumulator) have a valid target bound.
	// If the caller already opened it (non-null current group) we skip to
	// avoid a double-open.
	if (!m_spRenderer->GetCurrentRenderTargetGroup())
	{
		if (pRenderTargetGroup)
			m_spRenderer->BeginUsingRenderTargetGroup(pRenderTargetGroup, clearFlags);
		else
			m_spRenderer->BeginUsingDefaultRenderTargetGroup(clearFlags);
	}

	// Shadow passes run before the main scene and manage their own RTG.
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

	return true;
}

void NiApplication::EndScene()
{
    // Close the render target group that BeginScene conditionally opened.
    if (m_spRenderer && m_spRenderer->GetCurrentRenderTargetGroup())
        m_spRenderer->EndUsingRenderTargetGroup();
}

void NiApplication::BeginFrame()
{
    if (m_spRenderer)
        m_spRenderer->BeginFrame();
}

void NiApplication::EndFrame()
{
    if (m_spRenderer)
        m_spRenderer->EndFrame();
}

void NiApplication::Present()
{
    if (m_spRenderer)
		m_spRenderer->DisplayFrame();
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

    HWND hWnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(m_pWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!hWnd)
    {
        SDL_Log("Failed to retrieve HWND from SDL window: %s", SDL_GetError());
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
        return false;
	}

	m_hWnd = hWnd;
    return true;
}

bool NiApplication::CreateRenderer(const Settings& kSettings)
{
    switch (kSettings.m_eRendererID)
    {
    case efd::SystemDesc::RENDERER_DX9:
    {
        unsigned int uiFlags = kSettings.m_uiDX9Flags;
        if (kSettings.m_bFullscreen)
            uiFlags |= NiDX9Renderer::USE_FULLSCREEN;

        NiDX9Renderer* pRenderer = NiDX9Renderer::Create(
            kSettings.m_uiWidth, kSettings.m_uiHeight,
            uiFlags,
            reinterpret_cast<NiWindowRef>(m_hWnd),
            reinterpret_cast<NiWindowRef>(m_hWnd),
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
    }

    case efd::SystemDesc::RENDERER_D3D10:
    {
        NiD3D10Renderer::CreationParameters kParams(m_hWnd);
        kParams.m_eDriverType                   = kSettings.m_eDriverType10;
        kParams.m_uiCreateFlags                 = kSettings.m_uiDX10CreateFlags;
        kParams.m_bCreateDepthStencilBuffer     = kSettings.m_bCreateDepthBuffer10;
        kParams.m_eDepthStencilFormat           = kSettings.m_eDepthFormat10;
        kParams.m_kSwapChain.Windowed           = !kSettings.m_bFullscreen;
        kParams.m_kSwapChain.BufferCount        = kSettings.m_uiBackBufferCount;
        kParams.m_kSwapChain.SampleDesc.Count   = kSettings.m_uiSampleCount;
        kParams.m_kSwapChain.SampleDesc.Quality = kSettings.m_uiSampleQuality;
        kParams.m_kSwapChain.BufferDesc.Width   = kSettings.m_uiWidth;
        kParams.m_kSwapChain.BufferDesc.Height  = kSettings.m_uiHeight;
        kParams.m_kSwapChain.BufferDesc.Format  = kSettings.m_eSwapChainBuffer10;

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

        UINT uiViewportCount = 0;
        ID3D10Device* deviceContext = spD3D10->GetD3D10Device();
        deviceContext->RSGetViewports(&uiViewportCount, nullptr);
        if (uiViewportCount == 0)
        {
            D3D10_VIEWPORT kViewport = {};
            kViewport.TopLeftX = 0.0f;
            kViewport.TopLeftY = 0.0f;
            kViewport.Width = static_cast<float>(kSettings.m_uiWidth);
            kViewport.Height = static_cast<float>(kSettings.m_uiHeight);
            kViewport.MinDepth = 0.0f;
            kViewport.MaxDepth = 1.0f;
            deviceContext->RSSetViewports(1, &kViewport);
        }

        m_spRenderer->SetBackgroundColor(m_kClearColor);
        return true;
    }

    case efd::SystemDesc::RENDERER_D3D11:
    {
        ecr::D3D11Renderer::CreationParameters kParams(m_hWnd);
        kParams.m_driverType                    = kSettings.m_eDriverType11;
        kParams.m_createFlags                   = kSettings.m_uiDX11CreateFlags;
        kParams.m_createDepthStencilBuffer      = kSettings.m_bCreateDepthBuffer11;
        kParams.m_depthStencilFormat            = kSettings.m_eDepthFormat11;
        kParams.m_swapChain.Windowed            = !kSettings.m_bFullscreen;
        kParams.m_swapChain.SampleDesc.Count    = kSettings.m_uiSampleCount;
        kParams.m_swapChain.SampleDesc.Quality  = kSettings.m_uiSampleQuality;
        kParams.m_swapChain.BufferCount         = kSettings.m_uiBackBufferCount;
        kParams.m_swapChain.BufferDesc.Width    = kSettings.m_uiWidth;
        kParams.m_swapChain.BufferDesc.Height   = kSettings.m_uiHeight;
        kParams.m_swapChain.BufferDesc.Format   = kSettings.m_eSwapChainBuffer11;

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

        UINT uiViewportCount = 0;
        ID3D11DeviceContext* deviceContext = spD3D11->GetCurrentD3D11DeviceContext();
        deviceContext->RSGetViewports(&uiViewportCount, nullptr);
        if (uiViewportCount == 0)
        {
            D3D11_VIEWPORT kViewport = {};
            kViewport.TopLeftX = 0.0f;
            kViewport.TopLeftY = 0.0f;
            kViewport.Width = static_cast<float>(kSettings.m_uiWidth);
            kViewport.Height = static_cast<float>(kSettings.m_uiHeight);
            kViewport.MinDepth = 0.0f;
            kViewport.MaxDepth = 1.0f;
            deviceContext->RSSetViewports(1, &kViewport);
        }

        m_spRenderer->SetBackgroundColor(m_kClearColor);
        return true;
    }

    default:
        SDL_Log("Unsupported renderer ID: %u", static_cast<unsigned int>(kSettings.m_eRendererID));
        return false;
    }
}

void NiApplication::DestroyAll()
{
    if (!m_bInitialized)
        return;

    if (m_pfnShutdown)
        m_pfnShutdown(this, m_pShutdownUserData);

    m_spCamera = nullptr;
    if (m_bShadowsEnabled)
    {
        NiShadowManager::SetActive(false);
        NiShadowManager::Shutdown();
        m_bShadowsEnabled = false;
    }

    m_spCuller     = nullptr;
    m_spAlphaAccum = nullptr;

    // Release the render pipeline before the renderer and NiShutdown,
    // so that render-click and render-view destructors run while the
    // Ni allocator and renderer are still alive.
    m_spWaterView  = nullptr;
    m_spWaterClick = nullptr;
    m_spMainClick  = nullptr;
    m_spRenderStep = nullptr;

    m_spRenderer   = nullptr;

    if (m_pWindow)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
		m_hWnd = nullptr;
    }

    NiParticleSDM::Shutdown();
    NiPortalSDM::Shutdown();
    NiShutdown(true);

    SDL_Quit();
    m_bInitialized = false;
}

bool NiApplication::DispatchEvent(const SDL_Event& kEvent)
{
	// Call the user event callback, if set, before processing the event internally. If the callback returns false, skip internal processing of the event.
    if (m_pfnEvent) {
        m_pfnEvent(this, kEvent, m_pEventUserData);
    }

	// Handle window events and quit event. For window resize events, also update the renderer viewport size if applicable.
    switch (kEvent.type)
    {
    case SDL_EVENT_QUIT:
        return false;

    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    {
        m_uiWidth = static_cast<unsigned int>(kEvent.window.data1);
        m_uiHeight = static_cast<unsigned int>(kEvent.window.data2);
#if defined(NI_RENDERER_DX10)
        // Resize the renderer's swap chain / back buffer with the new window size.
        // This is necessary for proper rendering after a window resize, especially in fullscreen mode where the swap chain is tied to the window size.
        if (m_spRenderer) {
            auto* pRenderer = NiDynamicCast(NiD3D10Renderer, m_spRenderer);
            pRenderer->ResizeBuffers(m_uiWidth, m_uiHeight, m_hWnd);

            D3D10_VIEWPORT kViewport = {};
            kViewport.TopLeftX = 0.0f;
            kViewport.TopLeftY = 0.0f;
            kViewport.Width = static_cast<float>(m_uiWidth);
            kViewport.Height = static_cast<float>(m_uiHeight);
            kViewport.MinDepth = 0.0f;
            kViewport.MaxDepth = 1.0f;
            pRenderer->GetD3D10Device()->RSSetViewports(1, &kViewport);
        }
#elif defined(NI_RENDERER_DX11)
        // Resize the renderer's swap chain / back buffer with the new window size.
        // This is necessary for proper rendering after a window resize, especially in fullscreen mode where the swap chain is tied to the window size.
        if (m_spRenderer) {
            auto* pD3D11Renderer = NiDynamicCast(ecr::D3D11Renderer, m_spRenderer);
            pD3D11Renderer->ResizeBuffers(m_uiWidth, m_uiHeight, m_hWnd);

            D3D11_VIEWPORT kViewport = {};
            kViewport.TopLeftX = 0.0f;
            kViewport.TopLeftY = 0.0f;
            kViewport.Width = static_cast<float>(m_uiWidth);
            kViewport.Height = static_cast<float>(m_uiHeight);
            kViewport.MinDepth = 0.0f;
            kViewport.MaxDepth = 1.0f;
            pD3D11Renderer->GetCurrentD3D11DeviceContext()->RSSetViewports(1, &kViewport);
        }
#endif
        if (m_pfnResize)
			m_pfnResize(this, m_uiWidth, m_uiHeight, m_pResizeUserData);
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
    float fDelta = (m_uiPerfFreq > 0) ? static_cast<float>(uiNow - m_uiLastTick) / static_cast<float>(m_uiPerfFreq) : 0.0f;
    fDelta = std::min(fDelta, 0.1f);
    m_uiLastTick = uiNow;
    m_fLastDelta = fDelta;
	m_fTime += fDelta;
    return fDelta;
}

namespace
{
    bool IsUnsupportedShadowCasterType(NiAVObject* pkObject)
    {
        return pkObject &&
            (NiDynamicCast(NiBillboardNode, pkObject) ||
             NiDynamicCast(NiParticleSystem, pkObject) ||
             NiDynamicCast(NiPSParticleSystem, pkObject) ||
             NiDynamicCast(NiMeshHWInstance, pkObject));
    }

    void ExcludeUnsupportedShadowCastersRecursive(
        NiAVObject* pkObject,
        NiShadowGenerator* pkShadowGen)
    {
        if (!pkObject || !pkShadowGen)
            return;

        if (IsUnsupportedShadowCasterType(pkObject))
        {
            NiNode* pkExclude = NiDynamicCast(NiNode, pkObject);
            if (!pkExclude)
                pkExclude = pkObject->GetParent();

            if (pkExclude)
                pkShadowGen->AttachUnaffectedCasterNode(pkExclude);
            return;
        }

        NiNode* pkNode = NiDynamicCast(NiNode, pkObject);
        if (!pkNode)
            return;

        for (unsigned int ui = 0; ui < pkNode->GetArrayCount(); ++ui)
        {
            NiAVObject* pkChild = pkNode->GetAt(ui);
            if (pkChild)
                ExcludeUnsupportedShadowCastersRecursive(pkChild, pkShadowGen);
        }
    }

    void ExcludeUnsupportedShadowCasters(
        NiNode* pkScene,
        NiShadowGenerator* pkShadowGen)
    {
        if (!pkScene || !pkShadowGen)
            return;

        ExcludeUnsupportedShadowCastersRecursive(pkScene, pkShadowGen);
        pkShadowGen->SetRenderViewsDirty(true);
    }
}
