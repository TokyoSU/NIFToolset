// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#pragma once
#ifndef EE_GAMEAPPLICATION_H
#define EE_GAMEAPPLICATION_H

#include "ecrLibType.h"

#include <efd/UniversalTypes.h>
#include <efd/SmartPointer.h>
#include <efd/ServiceManager.h>
#include <ecr/RenderService.h>

#if defined(EE_PLATFORM_SERVICE_SDL3)
#include <SDL3/SDL.h>
#endif

// Forward declarations
namespace egf
{
    class EntityManager;
    class FlatModelManager;
}
namespace ecr
{
    class SceneGraphService;
    class SceneManager;
    class RenderSurface;
}
class NiRenderer;

namespace ecr
{

//--------------------------------------------------------------------------------------------------
/**
    Initialization parameters for GameApplication.

    All fields carry sensible defaults so the struct can be value-initialised:
    @code
        ecr::GameApplicationDesc desc;
        desc.pWindowTitle = "My Game";
        desc.uiWidth      = 1920;
        desc.uiHeight     = 1080;
    @endcode
*/
struct EE_ECR_ENTRY GameApplicationDesc
{
    /// Title bar string.
    const char*   pWindowTitle = "Game";

    /// Initial window width in pixels.
    efd::UInt32   uiWidth      = 1280;

    /// Initial window height in pixels.
    efd::UInt32   uiHeight     = 720;

    /// Renderer clear colour (RGBA, 0-1 range).
    efd::ColorA   bgColor        = efd::ColorA(0.0f, 0.0f, 0.0f, 1.0f);

    /// Start the window in fullscreen mode (false = windowed). Applies to Win32 and SDL3.
    bool bFullscreen     = false;
    /// Show the NiSettingsDialog on startup (false = use defaults/INI silently).
    bool bRendererDialog = false;

#if defined(EE_PLATFORM_SERVICE_SDL3)
    /// Allow the window to be resized by the user.
    bool bResizable    = true;
    /// Remove the window title bar and border.
    bool bBorderless   = false;
    /// Keep the window above all other windows.
    bool bAlwaysOnTop  = false;
    /// Request a high-DPI back buffer when available.
    bool bHighDPI      = false;
#endif
};

//--------------------------------------------------------------------------------------------------
/**
    High-level bootstrap facade for the Gamebryo/Emergent game framework.

    GameApplication handles the full engine lifecycle in one place:

      1. NiStaticDataManager::Init  (Gamebryo static data / memory / thread-locals)
      2. ServiceManager creation
      3. Window creation             (Win32PlatformService or SDL3PlatformService)
      4. Core service registration:
             Win32/SDL3PlatformService  → window + event pump
             MessageService             → pub/sub messaging
             FlatModelManager           → entity model definitions
             EntityManager              → entity lifecycle
             RenderService              → D3D9/10/11 renderer
             SceneGraphService          → .nif asset loading
      5. Main loop / per-tick dispatch
      6. Ordered teardown + NiStaticDataManager::Shutdown

    --- Blocking usage ---
    @code
        class MyApp : public ecr::GameApplication
        {
        protected:
            void OnInitComplete() override { LoadInitialScene(); }
            void OnPostTick()     override { UpdateCamera();     }
        };

        MyApp app;
        if (app.Initialize())
            app.Run();
    @endcode

    --- External loop integration ---
    @code
        ecr::GameApplication app;
        if (app.Initialize(desc))
        {
            while (app.Tick())
            {
                // interleave with your own per-frame work here
            }
        }
    @endcode

    Override the On* virtual methods to add custom services or per-frame logic without
    duplicating the bootstrap boilerplate.
*/
class EE_ECR_ENTRY GameApplication
{
public:
    GameApplication();
    virtual ~GameApplication();

    /**
        Initialize the engine, create the window, and register all core services.

        @param desc  Window/renderer configuration. All fields have defaults.
        @return true on success; false if any step failed (error is logged).
    */
    bool Initialize(const GameApplicationDesc& desc = {});

    /**
        Tick all services once (PreInit / Init / Tick / Shutdown as appropriate).

        Suitable for integrating the framework into an existing loop. Returns false once the
        ServiceManager has fully shut down.

        @return true while the framework is still running, false when done.
    */
    bool Tick();

    /**
        Blocking main loop.

        Delegates to ServiceManager::Run(), which calls RunOnce() in a tight loop until
        Shutdown() is requested.
    */
    void Run();

    /**
        Request framework shutdown.

        Safe to call at any time, including from within Tick() callbacks.
    */
    void Shutdown();

    /// Returns true if a shutdown has been requested.
    bool IsShutdownRequested() const;

    /// Returns true after Initialize() has returned true.
    bool IsInitialized() const;

    /// @name Service and renderer accessors
    //@{
    efd::ServiceManager*      GetServiceManager()    const;
    ecr::RenderService*       GetRenderService()     const;
    ecr::SceneGraphService*   GetSceneGraphService() const;
    ecr::SceneManager*        GetSceneManager()      const;
    egf::EntityManager*       GetEntityManager()     const;
    egf::FlatModelManager*    GetFlatModelManager()  const;

    /// Returns the underlying NiRenderer, or nullptr before the renderer is created.
    NiRenderer*               GetRenderer()          const;

    /// Returns the active RenderSurface, or nullptr before the renderer is created.
    ecr::RenderSurface*       GetActiveSurface()     const;
    //@}

protected:
    /**
        Called just before any service is registered.

        Override to prepend additional services that must initialize before the core set, or to
        configure the ServiceManager itself (e.g. SetSleepTime).
    */
    virtual void OnPreRegisterServices()  {}

    /**
        Called after all core services have been registered, before the first tick.

        Override to append extra services (physics, audio, input, networking, etc.).
    */
    virtual void OnPostRegisterServices() {}

    /**
        Called on the first tick where all services have completed OnInit.

        Override to perform post-init setup such as loading the initial scene, spawning entities,
        or configuring cameras.  Only fired when using Tick(); when using Run() the override
        will not be invoked (register a frame event or a custom service instead).
    */
    virtual void OnInitComplete()         {}

    /**
        Called at the start of each Tick(), before services are ticked.

        Not called from Run().
    */
    virtual void OnPreTick()              {}

    /**
        Called at the end of each Tick(), after services are ticked.

        Not called from Run().
    */
    virtual void OnPostTick()             {}

    /**
        Called once when the framework begins shutting down.

        Both Tick() and Run() paths call this before returning.
    */
    virtual void OnShutdown()             {}

private:
    bool RegisterCoreServices(const GameApplicationDesc& desc);
    void Cleanup();

    efd::SmartPointer<efd::ServiceManager> m_spServiceManager;

    bool m_bInitialized          = false;
    bool m_bNiInitialized        = false;
    bool m_bInitCompleteNotified = false;
    bool m_bShutdownNotified     = false;

#if defined(EE_PLATFORM_SERVICE_SDL3)
    SDL_Window* m_pSDLWindow = nullptr;
#endif
};

} // namespace ecr

#endif // EE_GAMEAPPLICATION_H
