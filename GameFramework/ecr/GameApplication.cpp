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
#include "ecrPCH.h"

#include <ecr/GameApplication.h>

// Gamebryo engine init/shutdown
#include <NiStaticDataManager.h>
#include <NiInitOptions.h>

// Foundation services
#include <efd/ServiceManager.h>
#include <efd/MessageService.h>
#include <efd/ConfigManager.h>
#include <efd/AssetLocatorService.h>
#include <efd/AssetFactoryManager.h>

// Entity services
#include <egf/EntityManager.h>
#include <egf/FlatModelManager.h>
#include <egf/GameTimeClock.h>

// Render services
#include <ecr/RenderService.h>
#include <ecr/SceneGraphService.h>
#include <ecr/SceneManager.h>
#include <ecr/RenderSurface.h>

// Platform service
#if defined(EE_PLATFORM_SERVICE_SDL3)
#   include <ecr/SDL3PlatformService.h>
#elif defined(EE_PLATFORM_WIN32)
#   include <efd/Win32/Win32PlatformService.h>
#endif

using namespace efd;
using namespace egf;
using namespace ecr;

//--------------------------------------------------------------------------------------------------
GameApplication::GameApplication() = default;

//--------------------------------------------------------------------------------------------------
GameApplication::~GameApplication()
{
    Cleanup();
}

//--------------------------------------------------------------------------------------------------
bool GameApplication::Initialize(const GameApplicationDesc& desc)
{
    EE_ASSERT(!m_bInitialized);
    NiInit(NiExternalNew NiInitOptions);
    m_bNiInitialized = true;

    m_spServiceManager = EE_NEW ServiceManager();
    m_spServiceManager->RegisterClock(EE_NEW GameTimeClock());
    OnPreRegisterServices();

    if (!RegisterCoreServices(desc))
    {
        Cleanup();
        return false;
    }

    OnPostRegisterServices();

    m_bInitialized = true;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool GameApplication::RegisterCoreServices(const GameApplicationDesc& desc)
{
    // --- Platform service ---

#if defined(EE_PLATFORM_SERVICE_SDL3)

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        EE_LOG(efd::kGamebryoGeneral0, ILogger::kERR0,
            ("GameApplication: SDL_Init failed: %s", SDL_GetError()));
        return false;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props,  SDL_PROP_WINDOW_CREATE_TITLE_STRING,              desc.pWindowTitle);
    SDL_SetNumberProperty(props,  SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,              static_cast<Sint64>(desc.uiWidth));
    SDL_SetNumberProperty(props,  SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,             static_cast<Sint64>(desc.uiHeight));
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,         desc.bResizable);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN,        desc.bFullscreen);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN,        desc.bBorderless);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_ALWAYS_ON_TOP_BOOLEAN,     desc.bAlwaysOnTop);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, desc.bHighDPI);
    m_pSDLWindow = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if (!m_pSDLWindow)
    {
        EE_LOG(efd::kGamebryoGeneral0, ILogger::kERR0,
            ("GameApplication: SDL_CreateWindow failed: %s", SDL_GetError()));
        SDL_Quit();
        return false;
    }

    m_spServiceManager->RegisterSystemService(EE_NEW SDL3PlatformService(m_pSDLWindow));

#elif defined(EE_PLATFORM_WIN32)

    auto* pWin32 = EE_NEW Win32PlatformService(
        GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOW);
    pWin32->SetWindowTitle(efd::utf8string(desc.pWindowTitle));
    pWin32->SetWindowClass(efd::utf8string(desc.pWindowTitle)); // required: empty class name crashes RegisterClass
    pWin32->SetWindowWidth(desc.uiWidth);
    pWin32->SetWindowHeight(desc.uiHeight);
    m_spServiceManager->RegisterSystemService(pWin32);

#endif

    // --- Configuration (required by AssetConfigService which AssetLocatorService registers) ---
    m_spServiceManager->RegisterSystemService(EE_NEW ConfigManager(efd::utf8string::NullString(), 0, nullptr));

    // --- Foundation messaging ---
    m_spServiceManager->RegisterSystemService(EE_NEW MessageService());

    // --- Asset location (required by AssetFactoryManager; auto-registers AssetConfigService) ---
    m_spServiceManager->RegisterSystemService(EE_NEW AssetLocatorService(m_spServiceManager));

    // --- Asset loading (SceneGraphService has a mandatory dependency on this) ---
    m_spServiceManager->RegisterSystemService(EE_NEW efd::AssetFactoryManager());

    // --- Entity infrastructure ---
    m_spServiceManager->RegisterSystemService(EE_NEW FlatModelManager());
    m_spServiceManager->RegisterSystemService(EE_NEW EntityManager());

    // --- Renderer (priority 1000) ---
    auto* pRenderService = EE_NEW RenderService();
    pRenderService->SetDefaultSurfaceBackgroundColor(desc.bgColor);
    pRenderService->SetFullscreen(desc.bFullscreen);
    pRenderService->SetRendererDialog(desc.bRendererDialog);
    m_spServiceManager->RegisterSystemService(pRenderService);

    // --- Scene graph / asset management (priority 1500) ---
    m_spServiceManager->RegisterSystemService(EE_NEW SceneGraphService());

    // --- Manual scene + actor manager (priority 1600) ---
    m_spServiceManager->RegisterSystemService(EE_NEW SceneManager());

    return true;
}

//--------------------------------------------------------------------------------------------------
bool GameApplication::Tick()
{
    EE_ASSERT(m_bInitialized);

    if (!m_spServiceManager)
        return false;

    const ServiceManager::ServiceState state = m_spServiceManager->GetCurrentState();

    if (state == ServiceManager::kSysServState_Complete)
        return false;

    OnPreTick();
    m_spServiceManager->RunOnce();
    OnPostTick();

    // Fire OnInitComplete once, after every service's OnInit() has completed and before
    // OnTick() begins. GetCurrentState() alone is not sufficient — the manager transitions
    // to kSysServState_Running immediately after PreInit, before any OnInit() has run.
    if (!m_bInitCompleteNotified && m_spServiceManager->AreAllServicesRunning())
    {
        m_bInitCompleteNotified = true;
        OnInitComplete();
    }

    // Fire OnShutdown once, as soon as a shutdown is requested.
    if (!m_bShutdownNotified && m_spServiceManager->GetShutdown())
    {
        m_bShutdownNotified = true;
        OnShutdown();
    }

    return m_spServiceManager->GetCurrentState() != ServiceManager::kSysServState_Complete;
}

//--------------------------------------------------------------------------------------------------
void GameApplication::Run()
{
    EE_ASSERT(m_bInitialized);

    if (!m_spServiceManager)
        return;

    m_spServiceManager->Run();

    if (!m_bShutdownNotified)
    {
        m_bShutdownNotified = true;
        OnShutdown();
    }
}

//--------------------------------------------------------------------------------------------------
void GameApplication::Shutdown()
{
    if (m_spServiceManager)
        m_spServiceManager->Shutdown();
}

//--------------------------------------------------------------------------------------------------
bool GameApplication::IsShutdownRequested() const
{
    return m_spServiceManager ? m_spServiceManager->GetShutdown() : true;
}

//--------------------------------------------------------------------------------------------------
bool GameApplication::IsInitialized() const
{
    return m_bInitialized;
}

//--------------------------------------------------------------------------------------------------
void GameApplication::Cleanup()
{
    // Destroy the service manager first — this calls OnShutdown on all services and releases
    // all Ni* smart pointers before the static data manager is torn down.
    m_spServiceManager = nullptr;

#if defined(EE_PLATFORM_SERVICE_SDL3)
    if (m_pSDLWindow)
    {
        SDL_DestroyWindow(m_pSDLWindow);
        m_pSDLWindow = nullptr;
    }
    SDL_Quit();
#endif

    // Shut down Gamebryo static data last.
    if (m_bNiInitialized)
    {
        auto* config = NiStaticDataManager::GetInitOptions();
        NiShutdown();
        if (config) {
			NiExternalDelete config;
        }
        m_bNiInitialized = false;
    }

    m_bInitialized          = false;
    m_bInitCompleteNotified = false;
    m_bShutdownNotified     = false;
}

//--------------------------------------------------------------------------------------------------
ServiceManager* GameApplication::GetServiceManager() const
{
    return m_spServiceManager;
}

//--------------------------------------------------------------------------------------------------
RenderService* GameApplication::GetRenderService() const
{
    return m_spServiceManager
        ? m_spServiceManager->GetSystemServiceAs<RenderService>()
        : nullptr;
}

//--------------------------------------------------------------------------------------------------
SceneGraphService* GameApplication::GetSceneGraphService() const
{
    return m_spServiceManager
        ? m_spServiceManager->GetSystemServiceAs<SceneGraphService>()
        : nullptr;
}

//--------------------------------------------------------------------------------------------------
SceneManager* GameApplication::GetSceneManager() const
{
    return m_spServiceManager
        ? m_spServiceManager->GetSystemServiceAs<SceneManager>()
        : nullptr;
}

//--------------------------------------------------------------------------------------------------
EntityManager* GameApplication::GetEntityManager() const
{
    return m_spServiceManager
        ? m_spServiceManager->GetSystemServiceAs<EntityManager>()
        : nullptr;
}

//--------------------------------------------------------------------------------------------------
FlatModelManager* GameApplication::GetFlatModelManager() const
{
    return m_spServiceManager
        ? m_spServiceManager->GetSystemServiceAs<FlatModelManager>()
        : nullptr;
}

//--------------------------------------------------------------------------------------------------
NiRenderer* GameApplication::GetRenderer() const
{
    auto* pRS = GetRenderService();
    return pRS ? pRS->GetRenderer() : nullptr;
}

//--------------------------------------------------------------------------------------------------
RenderSurface* GameApplication::GetActiveSurface() const
{
    auto* pRS = GetRenderService();
    return pRS ? pRS->GetActiveRenderSurface() : nullptr;
}
