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
#include <NiDefaultClickRenderStep.h>
#include <NiViewRenderClick.h>
#include <Ni3DRenderView.h>
#include <NiAlphaSortProcessor.h>

#if WIN32
#include <Windows.h>
#endif

class NIAPPLICATION_ENTRY NiApplication
{
public:
    typedef bool (*InitCallback)(NiApplication* pApp, void* pUserData);
    typedef void (*ShutdownCallback)(NiApplication* pApp, void* pUserData);
    typedef void (*UpdateCallback)(NiApplication* pApp, float fDeltaTime, void* pUserData);
    typedef void (*RenderCallback)(NiApplication* pApp, void* pUserData);
    typedef bool (*EventCallback)(NiApplication* pApp, const SDL_Event& kEvent, void* pUserData);
	typedef void (*ResizeCallback)(NiApplication* pApp, unsigned int uiWidth, unsigned int uiHeight, void* pUserData);

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

        // Vertical field of view in degrees, near and far clip planes.
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
        bool m_bVSync              = true;

        // Optional override for precompiled bgfx shader binaries. The path
        // points at the directory containing dx11/, glsl/, spirv/, ... .
        // nullptr uses the build/install-time discovery paths.
        const char* m_pszBgfxShaderRoot = nullptr;

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

    };

    NiApplication();
    ~NiApplication();
    NiApplication(const NiApplication&)            = delete;
    NiApplication& operator=(const NiApplication&) = delete;

    bool Initialize(const Settings& kSettings = Settings{});
    int  Run(double targetFps = 60.0);
    void Quit();

    void SetInitCallback    (InitCallback     pfn, void* pUserData = nullptr);
    void SetShutdownCallback(ShutdownCallback pfn, void* pUserData = nullptr);
    void SetUpdateCallback  (UpdateCallback   pfn, void* pUserData = nullptr);
    void SetRenderCallback  (RenderCallback   pfn, void* pUserData = nullptr);
    void SetEventCallback   (EventCallback    pfn, void* pUserData = nullptr);
	void SetResizeCallback  (ResizeCallback   pfn, void* pUserData = nullptr);

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
#if WIN32
    HWND                   GetHandle() const;
#else
    void*                  GetHandle() const;
#endif
    void SetShadowsActive(bool bActive);

    // Returns the Ni3DRenderView used by the main render clicks so that
    // CScene (or any other owner) can attach / detach its scene root.
    Ni3DRenderView*            GetMainRenderView()  const;
    Ni3DRenderView*            GetWaterRenderView() const;

    // Returns the render step driving the main + water passes.
    NiDefaultClickRenderStep*  GetRenderStep()      const;

    // Draws only opaque main-scene geometry.
    // Call this after BeginScene() and before DrawWaterPass().
    void DrawMainOpaquePass();

    // Draws only the alpha-sorted (transparent) main-scene geometry.
    // Call this AFTER DrawWaterPass() so that particles/foliage etc.
    // composite correctly on top of water.
    void DrawMainAlphaPass();

    // Convenience: draws opaques + alpha in one call (no water pass).
    // Equivalent to DrawMainOpaquePass() + DrawMainAlphaPass().
    void DrawMainPass();

    // Executes the water render click against the currently open render
    // target group.  Call this AFTER your opaque scene draw (RenderState)
    // so that the water pass depth-tests against written depth values.
    // Does nothing if no water scene has been set via SetWaterScene().
    void DrawWaterPass();

    /// <summary>
    /// Begins a rendering scene with optional render target and clear flags.
	/// NOTE: This must be called before any rendering is done for the scene, and EndScene() must be called after all rendering is done for the scene, and before EndFrame() is called.
    /// </summary>
    /// <param name="pRenderTargetGroup">The render target group to render to, or nullptr to use the default render target.</param>
    /// <param name="clearFlags">Flags specifying which buffers to clear before rendering.</param>
    /// <returns>true if the scene was successfully begun; otherwise, false.</returns>
    bool BeginScene(NiRenderTargetGroup* pRenderTargetGroup = nullptr, NiRenderer::ClearFlags clearFlags = NiRenderer::ClearFlags::CLEAR_ALL);
	// Ends the current scene. This must be called after all rendering is done for the scene, and before EndFrame() is called.
    void EndScene();
    // Begins a new frame. This must be called before any rendering is done, and EndFrame() must be called after all rendering is done for the frame.
	void BeginFrame();
	/// <summary>
	/// Marks the end of the current frame.
	/// </summary>
	void EndFrame();
    /// <summary>
    /// Presents the rendered frame to the screen.
    /// </summary>
    void Present();

private:
    bool  CreateSDLWindow(const Settings& kSettings);
    bool  CreateRenderer(const Settings& kSettings);
    void  ApplyShaderDefaults(const Settings& kSettings);
    void  CreateRenderPipeline();
    void  DestroyAll();
    bool  DispatchEvent(const SDL_Event& kEvent);
    float ComputeDeltaTime();

    SDL_Window*                     m_pWindow = nullptr;
    NiPointer<NiRenderer>           m_spRenderer;
    NiPointer<NiCamera>             m_spCamera;
	NiPointer<NiAlphaAccumulator>          m_spAlphaAccum;
	NiPointer<NiMeshCullingProcess>        m_spCuller;
	NiPointer<NiDefaultClickRenderStep>    m_spRenderStep;
	NiPointer<Ni3DRenderView>              m_spMainView;       // shared by both main clicks
	NiPointer<NiViewRenderClick>           m_spMainOpaqueClick;
	NiPointer<NiViewRenderClick>           m_spMainAlphaClick;
	NiPointer<NiViewRenderClick>           m_spWaterClick;
	NiPointer<Ni3DRenderView>              m_spWaterView;
	NiVisibleArray                         m_kVisibleSet;
#if WIN32
	HWND m_hWnd = nullptr;
#else
	void* m_hWnd = nullptr;
#endif

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
	ResizeCallback   m_pfnResize         = nullptr;
	void*            m_pResizeUserData   = nullptr;

    bool m_bInitialized  = false;
    bool m_bQuit         = false;
    bool m_bShadowsEnabled = false;
    bool m_bVSync = true;

    Uint64 m_uiPerfFreq = 0;
    Uint64 m_uiLastTick = 0;
    float  m_fLastDelta = 0.0f;
	float  m_fTime      = 0.0f;
};

#endif // NIAPPLICATION_H

