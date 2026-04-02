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

#include "NiEnvironmentPCH.h"

#include "NiSkyRenderView.h"
#include "NiRenderer.h"

//---------------------------------------------------------------------------
NiImplementRTTI(NiSkyRenderView, Ni3DRenderView);

//---------------------------------------------------------------------------
NiSkyRenderView::NiSkyRenderView(NiEnvironment* pkEnvironment,
    NiCamera* pkCamera, NiCullingProcess* pkCullingProcess, 
    bool bAlwaysUseCameraViewport):
    Ni3DRenderView(pkCamera, pkCullingProcess, bAlwaysUseCameraViewport),
    m_spEnvironment(pkEnvironment)
{
}

//---------------------------------------------------------------------------
void NiSkyRenderView::SetCameraData(const NiRect<float>& kViewport)
{
    // Overide the camera's translation to center the position in the sky.
    NiPoint3 kOrigTranslate = m_spCamera->GetWorldTranslate();
    m_spCamera->SetWorldTranslate(NiPoint3::ZERO);
    Ni3DRenderView::SetCameraData(kViewport);
    m_spCamera->SetWorldTranslate(kOrigTranslate);
}

//---------------------------------------------------------------------------
bool NiSkyRenderView::CallbackClickConfigureFog(NiRenderClick* pkCurrentRenderClick,
    void* pvCallbackData)
{
    // Extract useful pointers from the callback data
    NiSkyRenderView* pkSkyView = NiStaticCast(NiSkyRenderView, pvCallbackData);
    NiEnvironment* pkEnvironment = pkSkyView->m_spEnvironment;

    EE_ASSERT(pkEnvironment);
    EE_ASSERT(pkEnvironment->GetSun());
    EE_ASSERT(pkEnvironment->GetAtmosphere());

    // Update the fog color based on the atmosphere, or just make it white
    NiFogProperty* pkFog = pkEnvironment->GetFogProperty();
    if (!pkFog)
        return true;

    NiColor kFogColor = pkFog->GetFogColor();
    if (pkEnvironment->GetAutoCalcFogColor())
    {
        // Based on the direction the camera is facing (horizon in that 
        // direction)
        NiMatrix3 kCurrentRot = pkEnvironment->GetSun()->GetRotate();
        NiMatrix3 kCameraRot = pkSkyView->m_spCamera->GetRotate();
        NiPoint3 kSunDirection = kCurrentRot * NiPoint3::UNIT_X;
        NiPoint3 kCameraDirection = kCameraRot * NiPoint3::UNIT_X;

        // Project the camera direction onto the ground
        kCameraDirection.z = 0;
        NiPoint3::UnitizeVector(kCameraDirection);

        // Calculate atmosphere fog color in that direction
        kFogColor = pkEnvironment->GetAtmosphere()->
            GetSkyColorInDirection(kCameraDirection,
            kSunDirection, false);

        // Set the fog color
        pkFog->SetFogColor(kFogColor);
    }

    // Set the background color
    if (pkEnvironment->GetAutoSetBackgroundColor())
    {
        NiColorA kBackgroundColor;
        kBackgroundColor.r = kFogColor.r;
        kBackgroundColor.g = kFogColor.g;
        kBackgroundColor.b = kFogColor.b;
        kBackgroundColor.a = 1.0f;
        pkCurrentRenderClick->SetBackgroundColor(kBackgroundColor);
        pkCurrentRenderClick->SetUseRendererBackgroundColor(false);
    }
    else
    {
        pkCurrentRenderClick->SetUseRendererBackgroundColor(true);
    }

    return true;
}

//---------------------------------------------------------------------------