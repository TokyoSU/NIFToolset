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
#ifndef EE_SDL3PLATFORMSERVICE_H
#define EE_SDL3PLATFORMSERVICE_H

#if defined(EE_PLATFORM_SERVICE_SDL3)

#include "ecrLibType.h"

#include <efd/ISystemService.h>
#include <efd/efdSystemServiceIDs.h>
#include <efd/IServiceDetailRegister.h>

#include <SDL3/SDL.h>

#ifdef EE_PLATFORM_WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

namespace efd
{

/**
    SDL3PlatformService provides an SDL3-based platform service to drive the framework event
    pump and expose the native window/instance handles needed by RenderService.

    Register this service in place of Win32PlatformService when SDL3 manages the window.
    It must tick before RenderService (default priority 2000 > 1000) so the window handle
    is available when the renderer is created.

    Each call to OnTick drains the SDL event queue. A SDL_EVENT_QUIT or
    SDL_EVENT_WINDOW_CLOSE_REQUESTED will call ServiceManager::Shutdown and stop ticking.
*/
class EE_ECR_ENTRY SDL3PlatformService : public ISystemService
{
    /// @cond EMERGENT_INTERNAL
    EE_DECLARE_CLASS1(SDL3PlatformService, efd::kCLASSID_SDL3PlatformService, ISystemService);
    EE_DECLARE_CONCRETE_REFCOUNT;
    /// @endcond

public:
    /**
        Constructs the service with an existing SDL3 window.

        @param pWindow The SDL3 window that backs this service. Must remain valid for the
            lifetime of the service.
    */
    explicit SDL3PlatformService(SDL_Window* pWindow);

    /// Destructor.
    virtual ~SDL3PlatformService();

    /// Returns the SDL3 window supplied at construction.
    inline SDL_Window* GetSDLWindow() const;

    /// Returns true once a quit event has been received and Shutdown has been requested.
    inline bool IsQuitRequested() const;

#ifdef EE_PLATFORM_WIN32
    /**
        Returns the Win32 HWND embedded in the SDL3 window.

        The value is retrieved each call so it is safe to call at any time after construction.
    */
    HWND GetWindowRef() const;

    /// Returns the process HINSTANCE via GetModuleHandle(nullptr).
    HINSTANCE GetInstanceRef() const;
#endif // EE_PLATFORM_WIN32

    /// @name ISystemService overrides
    //@{
    virtual void OnServiceRegistered(IAliasRegistrar* pAliasRegistrar) override;
    virtual SyncResult OnPreInit(IDependencyRegistrar* pDependencyRegistrar) override;
    virtual AsyncResult OnInit() override;

    /**
        Drains the SDL event queue.

        Returns AsyncResult_Complete (and calls ServiceManager::Shutdown) when a quit event
        is received; AsyncResult_Pending otherwise.
    */
    virtual AsyncResult OnTick() override;
    virtual AsyncResult OnShutdown() override;
    virtual const char* GetDisplayName() const override;
    //@}

private:
    SDL_Window* m_pWindow;
    bool        m_bQuitRequested;
};

//--------------------------------------------------------------------------------------------------
inline SDL_Window* SDL3PlatformService::GetSDLWindow() const
{
    return m_pWindow;
}

//--------------------------------------------------------------------------------------------------
inline bool SDL3PlatformService::IsQuitRequested() const
{
    return m_bQuitRequested;
}

} // namespace efd

#endif // EE_PLATFORM_SERVICE_SDL3

#endif // EE_SDL3PLATFORMSERVICE_H