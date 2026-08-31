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

#include "RenderService.h"

#include "efd/Win32/Win32PlatformService.h"
#if defined(EE_PLATFORM_SERVICE_SDL3)
#include <ecr/SDL3PlatformService.h>
#endif

#include <BgfxRenderer.h>

#include <efd/IConfigManager.h>
#include <efd/SystemDesc.h>

#include <NiRendererSettings.h>

using namespace egf;
using namespace efd;
using namespace ecr;

//------------------------------------------------------------------------------------------------
void RenderService::InternalDestructor()
{
    // bgfx owns device-loss/reset handling internally.
}

//------------------------------------------------------------------------------------------------
bool RenderService::CreateRenderer()
{
    // Resolve the platform service — prefer Win32PlatformService, fall back to SDL3PlatformService.
    Win32PlatformService* pWin32 =
        m_pServiceManager->GetSystemServiceAs<Win32PlatformService>();
#if defined(EE_PLATFORM_SERVICE_SDL3)
    SDL3PlatformService* pSDL3 =
        !pWin32 ? m_pServiceManager->GetSystemServiceAs<SDL3PlatformService>() : nullptr;
#endif

    if (!m_parentHandle)
    {
        if (pWin32)
            m_parentHandle = pWin32->GetWindowRef();
#if defined(EE_PLATFORM_SERVICE_SDL3)
        else if (pSDL3)
            m_parentHandle = pSDL3->GetWindowRef();
#endif
    }

    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();

    NiRendererSettings settings;
    char settingsFile[EE_MAX_PATH];
    bool validFile = efd::PathUtils::GetExecutableDirectory(settingsFile, sizeof(settingsFile));

    // Config manager/command line override settings file settings.
    if (validFile)
    {
        NiStrcat(settingsFile, sizeof(settingsFile), "AppSettings.ini");
        settings.LoadSettings(settingsFile);
    }
    settings.LoadFromConfigManager(pConfigManager);

    // GameApplicationDesc always wins over INI / config manager.
    settings.m_bFullscreen     = m_bFullscreen;
    settings.m_bRendererDialog = m_bRendererDialog;

    // The legacy renderer selection dialog only contains Direct3D setup
    // descriptors. bgfx selects the best supported backend automatically.
    settings.m_bRendererDialog = false;

    if (m_parentHandle && settings.m_uiScreenHeight && settings.m_uiScreenWidth)
    {
        // Resize it to new resolution
        RECT rect =
        {
            0,
            0,
            static_cast<LONG>(settings.m_uiScreenWidth),
            static_cast<LONG>(settings.m_uiScreenHeight)
        };

        // Read the style from the window handle
        WINDOWINFO windowInfo;
        windowInfo.cbSize = sizeof(windowInfo);

        EE_VERIFY(GetWindowInfo(m_parentHandle, &windowInfo));

        AdjustWindowRect(&rect,
            windowInfo.dwStyle,
            false);

        SetWindowPos(
            m_parentHandle,
            HWND_TOP,
            0, 0,
            rect.right-rect.left, rect.bottom-rect.top,
            SWP_NOMOVE);
    }

    RECT clientRect = {};
    if (!m_parentHandle || !GetClientRect(m_parentHandle, &clientRect))
        return false;

    // Use the actual post-resize client dimensions (DPI/window decorations may
    // make them differ from the requested settings values).
    const unsigned int width = static_cast<unsigned int>(
        std::max<LONG>(1, clientRect.right - clientRect.left));
    const unsigned int height = static_cast<unsigned int>(
        std::max<LONG>(1, clientRect.bottom - clientRect.top));

    m_spRenderer = BgfxRenderer::Create(m_parentHandle, width, height, settings.m_bVSync);

    return (m_spRenderer != NULL);
}

//------------------------------------------------------------------------------------------------
RenderSurfacePtr RenderService::CreateRenderSurface(NiWindowRef windowHandle)
{
    if (!windowHandle)
        windowHandle = m_parentHandle;

    if (windowHandle != m_parentHandle &&
        !m_spRenderer->CreateWindowRenderTargetGroup(windowHandle))
    {
        return nullptr;
    }


    // Create a new render surface entry for the back-buffer.
    RenderSurfacePtr spSurface = NiNew RenderSurface(windowHandle, this);

    if (windowHandle == m_parentHandle)
    {
        spSurface->GetSceneRenderClick()->SetRenderTargetGroup(
            m_spRenderer->GetDefaultRenderTargetGroup());
    }
    else
    {
        spSurface->GetSceneRenderClick()->SetRenderTargetGroup(
            m_spRenderer->GetWindowRenderTargetGroup(windowHandle));
    }

    // Initialise the camera frustum from the actual client area so the aspect ratio
    // matches the window and content at the world origin is visible by default.
    {
        RECT kRect = {};
        GetClientRect(windowHandle, &kRect);
        const auto uiW = static_cast<efd::UInt32>(kRect.right  - kRect.left);
        const auto uiH = static_cast<efd::UInt32>(kRect.bottom - kRect.top);
        spSurface->SetupDefaultCamera(uiW > 0 ? uiW : 1280u, uiH > 0 ? uiH : 720u);
    }

    if (!m_pActiveSurface)
        SetActiveRenderSurface(spSurface);

    return spSurface;
}

//------------------------------------------------------------------------------------------------
bool RenderService::DestroyRenderSurface(RenderSurface* pSurface)
{
    m_spRenderer->ReleaseWindowRenderTargetGroup(pSurface->GetWindowRef());

    return true;
}

//------------------------------------------------------------------------------------------------
bool RenderService::RecreateRenderSurface(RenderSurface* pSurface)
{
    EE_ASSERT(pSurface);

    // If the specified surface maps to the back-buffer of the device then
    // tell the NiRenderer to re-create the device. If the surface maps to a
    // swap chain, destroy the swap chain and re-create it.

    if (pSurface->GetRenderTargetGroup() == m_spRenderer->GetDefaultRenderTargetGroup())
    {
        RECT rect = {};
        if (!GetClientRect(pSurface->GetWindowRef(), &rect))
            return false;
        const unsigned int width = static_cast<unsigned int>(std::max<LONG>(1, rect.right - rect.left));
        const unsigned int height = static_cast<unsigned int>(std::max<LONG>(1, rect.bottom - rect.top));
        BgfxRenderer* renderer = static_cast<BgfxRenderer*>(m_spRenderer.data());
        if (!renderer->Resize(width, height, renderer->GetVSync()))
            return false;
    }
    else if (!m_spRenderer->RecreateWindowRenderTargetGroup(pSurface->GetWindowRef()))
    {
        return false;
    }

    // Make sure to fix the aspect ratio on the default camera if it's present.
    NiRenderTargetGroup* pRenderTarget = pSurface->GetRenderTargetGroup();
    EE_ASSERT(pRenderTarget);

    NiCamera* pCamera = pSurface->GetCamera();
    if (pCamera)
    {
        float width = (float)pRenderTarget->GetWidth(0);
        float height = (float)pRenderTarget->GetHeight(0);

        float aspectRatio = width / height;

        pCamera->AdjustAspectRatio(aspectRatio);
    }

    // Notify any callbacks that the render surface has been recreated.
    for (DelegateList::iterator i = m_renderServiceDelegates.begin();
         i != m_renderServiceDelegates.end(); ++i)
    {
        (*i)->OnSurfaceRecreated(this, pSurface);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
