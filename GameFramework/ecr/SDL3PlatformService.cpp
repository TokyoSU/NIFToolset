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

#if defined(EE_PLATFORM_SERVICE_SDL3)

#include <ecr/SDL3PlatformService.h>
#include <efd/ServiceManager.h>

using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(SDL3PlatformService);

//--------------------------------------------------------------------------------------------------
SDL3PlatformService::SDL3PlatformService(SDL_Window* pWindow)
    : m_pWindow(pWindow)
    , m_bQuitRequested(false)
{
    // Tick before RenderService (priority 1000) so the HWND is ready when the renderer is created.
    m_defaultPriority = 2000;
}

//--------------------------------------------------------------------------------------------------
SDL3PlatformService::~SDL3PlatformService() = default;

//--------------------------------------------------------------------------------------------------
void SDL3PlatformService::OnServiceRegistered(IAliasRegistrar* /*pAliasRegistrar*/)
{
    // No aliases needed: RenderService_Win32 queries SDL3PlatformService directly as a fallback
    // when Win32PlatformService is not registered.
}

//--------------------------------------------------------------------------------------------------
SyncResult SDL3PlatformService::OnPreInit(IDependencyRegistrar* /*pDependencyRegistrar*/)
{
    EE_ASSERT(m_pWindow != nullptr);
    return SyncResult_Success;
}

//--------------------------------------------------------------------------------------------------
AsyncResult SDL3PlatformService::OnInit()
{
    return AsyncResult_Complete;
}

//--------------------------------------------------------------------------------------------------
AsyncResult SDL3PlatformService::OnTick()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            m_bQuitRequested = true;
            m_pServiceManager->Shutdown();
            return AsyncResult_Complete;

        default:
            break;
        }
    }
    return AsyncResult_Pending;
}

//--------------------------------------------------------------------------------------------------
AsyncResult SDL3PlatformService::OnShutdown()
{
    return AsyncResult_Complete;
}

//--------------------------------------------------------------------------------------------------
const char* SDL3PlatformService::GetDisplayName() const
{
    return "SDL3PlatformService";
}

//--------------------------------------------------------------------------------------------------
#ifdef EE_PLATFORM_WIN32

HWND SDL3PlatformService::GetWindowRef() const
{
    return static_cast<HWND>(
        SDL_GetPointerProperty(
            SDL_GetWindowProperties(m_pWindow),
            SDL_PROP_WINDOW_WIN32_HWND_POINTER,
            nullptr));
}

HINSTANCE SDL3PlatformService::GetInstanceRef() const
{
    return ::GetModuleHandle(nullptr);
}

#endif // EE_PLATFORM_WIN32

#endif // EE_PLATFORM_SERVICE_SDL3
