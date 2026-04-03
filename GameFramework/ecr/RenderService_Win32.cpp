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

#if defined(NI_RENDERER_DX9)
#   include <NiDX9Renderer.h>
#   if defined(NI_BUILD_RENDERER_SETUP)
#   include <NiDX9RendererSetup.h>
#   endif
#elif defined(NI_RENDERER_DX10)
#   if defined(NI_BUILD_RENDERER_SETUP)
#   include <NiD3D10RendererSetup.h>
#   endif
#elif defined(NI_RENDERER_DX11)
#   if defined(NI_BUILD_RENDERER_SETUP)
#   include <D3D11RendererSetup.h>
#   endif
#endif

#include <efd/IConfigManager.h>
#include <efd/SystemDesc.h>

#include <NiRendererSettings.h>
#include <NiSettingsDialog.h>
#include <NiBaseRendererSetup.h>

using namespace egf;
using namespace efd;
using namespace ecr;

//------------------------------------------------------------------------------------------------
void RenderService::InternalDestructor()
{
#if defined(NI_RENDERER_DX9)
    NiDX9Renderer* pDX9Renderer = NiDynamicCast(NiDX9Renderer, m_spRenderer);

    if (pDX9Renderer != NULL)
    {
        pDX9Renderer->RemoveLostDeviceNotificationFunc(&RenderService::OnDeviceLost);
        pDX9Renderer->RemoveResetNotificationFunc(&RenderService::OnDeviceReset);
    }
#endif
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

    // Force the renderer ID to match the compile-time selection so that
    // NiBaseRendererSetup::CreateRenderer finds the registered setup even
    // when the settings dialog is suppressed (default INI value is DX9).
#if defined(NI_RENDERER_DX11)
    settings.m_eRendererID = efd::SystemDesc::RENDERER_D3D11;
#elif defined(NI_RENDERER_DX10)
    settings.m_eRendererID = efd::SystemDesc::RENDERER_D3D10;
#else
    settings.m_eRendererID = efd::SystemDesc::RENDERER_DX9;
#endif

    #if defined(EE_PLATFORM_SERVICE_SDL3)
    if (settings.m_bRendererDialog && (pWin32 || pSDL3))
    {
        NiSettingsDialog dialog(&settings);
        const HINSTANCE hInstance = pWin32 ? pWin32->GetInstanceRef() : pSDL3->GetInstanceRef();
        if (dialog.InitDialog(hInstance) &&
            dialog.ShowDialog(m_parentHandle, (NiAcceleratorRef)m_parentHandle))
        {
            if (settings.m_bSaveSettings && validFile)
                settings.SaveSettings(settingsFile);
        }
    }
#else
    if (settings.m_bRendererDialog && pWin32)
    {
        NiSettingsDialog dialog(&settings);
        if (dialog.InitDialog(pWin32->GetInstanceRef()) &&
            dialog.ShowDialog(m_parentHandle, (NiAcceleratorRef)m_parentHandle))
        {
            if (settings.m_bSaveSettings && validFile)
                settings.SaveSettings(settingsFile);
        }
    }
#endif

    if (m_parentHandle && settings.m_uiScreenHeight && settings.m_uiScreenWidth)
    {
        // Resize it to new resolution
        RECT rect =
        {
            0,
            0,
            settings.m_uiScreenWidth,
            settings.m_uiScreenHeight
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

    m_spRenderer = NiBaseRendererSetup::CreateRenderer(
        &settings,
        m_parentHandle,
        m_parentHandle);

    #if defined(NI_RENDERER_DX9)
    if (settings.m_eRendererID == efd::SystemDesc::RENDERER_DX9)
    {
        NiRenderer* pkRenderer = m_spRenderer;
        NiDX9Renderer* pDX9Renderer = NiVerifyStaticCast(NiDX9Renderer, pkRenderer);

        if (pDX9Renderer != NULL)
        {
            pDX9Renderer->AddLostDeviceNotificationFunc(&RenderService::OnDeviceLost, this);
            pDX9Renderer->AddResetNotificationFunc(&RenderService::OnDeviceReset, this);
        }
    }
#endif

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
        // Do nothing; the device has already recreated itself and its swap chain.
    }
    else
    {
        #if defined(NI_RENDERER_DX9)
        if (m_spRenderer->GetRendererID() == efd::SystemDesc::RENDERER_DX9)
        {
            NiRenderer* pkRenderer = m_spRenderer;
            NiDX9Renderer* pDX9Renderer = NiVerifyStaticCast(NiDX9Renderer, pkRenderer);
            // If we don't have a valid device, don't try to recreate the render target group.
            if (!pDX9Renderer->LostDeviceRestore())
                return false;
        }
#endif

        // Swap chain.
        if (!m_spRenderer->RecreateWindowRenderTargetGroup(pSurface->GetWindowRef()))
        {
            return false;
        }
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
