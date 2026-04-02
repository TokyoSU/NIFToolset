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
#include "NiEnvironment.h"

#include <NiShaderFactory.h>
#include <NiSingleShaderMaterial.h>
#include <NiStencilProperty.h>
#include <NiZBufferProperty.h>
#include "NiSkyRenderView.h"

//---------------------------------------------------------------------------
NiImplementRTTI(NiEnvironment, NiNode);

//---------------------------------------------------------------------------
NiEnvironment::NiEnvironment():
    m_fSunAzimuthAngle(0.0f),
    m_fSunElevationAngle(0.0f),
    m_bAutoCalcFogColor(true),
    m_bAutoSetBackgroundColor(true),
    m_bUseSunAnglesRotation(true),
    m_bSunSettingsChanged(false)
{  
    SetAppCulled(true); 
    m_spSkyRoot = NiNew NiNode();
    AttachChild(m_spSkyRoot);
    m_kName = "Environment";
}

//---------------------------------------------------------------------------
NiEnvironment::~NiEnvironment()
{    
}

//---------------------------------------------------------------------------
void NiEnvironment::DoUpdate(NiUpdateProcess& kUpdate)
{       
    EE_UNUSED_ARG(kUpdate);
    
    if (m_bSunSettingsChanged && m_bUseSunAnglesRotation)
    {
        m_bSunSettingsChanged = false;
        NiMatrix3 kElevationRot;
        NiMatrix3 kAzimuthRot;

        kElevationRot.MakeYRotation(m_fSunElevationAngle);
        kAzimuthRot.MakeZRotation(m_fSunAzimuthAngle);

        m_spSun->SetRotate(kAzimuthRot * kElevationRot);
        m_spSun->UpdateEffects();
        m_spSun->UpdateProperties();
        m_spSun->Update(kUpdate);
    }
}

//---------------------------------------------------------------------------
void NiEnvironment::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{        
    DoUpdate(kUpdate);
    NiNode::UpdateDownwardPass(kUpdate); 
}

//---------------------------------------------------------------------------
void NiEnvironment::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{    
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    }

    NiNode::UpdateSelectedDownwardPass(kUpdate);
}

//---------------------------------------------------------------------------
void NiEnvironment::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    } 
    
    NiNode::UpdateRigidDownwardPass(kUpdate);
}

//---------------------------------------------------------------------------
NiAtmosphere* NiEnvironment::CreateAtmosphere()
{
    SetAtmosphere(NiNew NiAtmosphere());
    return GetAtmosphere();
}

//---------------------------------------------------------------------------
void NiEnvironment::SetAtmosphere(NiAtmosphere* pkAtmosphere)
{
    m_spAtmosphere = pkAtmosphere;
    m_spAtmosphere->SetName(m_kName);

    AttachChild(pkAtmosphere);
}

//---------------------------------------------------------------------------
NiAtmosphere* NiEnvironment::GetAtmosphere()
{
    return m_spAtmosphere;
}

//---------------------------------------------------------------------------
NiSkyDome* NiEnvironment::CreateSkyDome()
{
    NiSkyDome* pkSkyDome = NiNew NiSkyDome();

    pkSkyDome->SetAtmosphere(m_spAtmosphere);
    pkSkyDome->SetName(m_kName);

    SetSky(pkSkyDome);

    return pkSkyDome;
}

//---------------------------------------------------------------------------
void NiEnvironment::SetSky(NiSky* pkSky)
{
    m_spSky = pkSky;
    
    m_spSkyRoot->AttachChild(pkSky);
}

//---------------------------------------------------------------------------
NiSky* NiEnvironment::GetSky()
{
    return m_spSky;
}

//---------------------------------------------------------------------------
NiDirectionalLight* NiEnvironment::CreateSun()
{
    SetSun(NiNew NiDirectionalLight());
    return GetSun();
}

//---------------------------------------------------------------------------
void NiEnvironment::SetSun(NiDirectionalLight* pkSun)
{
    if (m_spSun)
    {
        m_spSkyRoot->DetachChild(m_spSun);
    }

    m_spSun = pkSun;

    m_spSkyRoot->AttachChild(pkSun);

    if (m_spSky)
    {
        m_spSky->SetSun(pkSun);
    }
}

//---------------------------------------------------------------------------
NiDirectionalLight* NiEnvironment::GetSun()
{
    return m_spSun;
}

//---------------------------------------------------------------------------
NiFogProperty* NiEnvironment::CreateFogProperty()
{
    NiFogProperty* pkFogProperty = NiNew NiFogProperty();
    SetFogProperty(pkFogProperty);
    return GetFogProperty();
}

//---------------------------------------------------------------------------
void NiEnvironment::SetFogProperty(NiFogProperty* pkFogProperty)
{
    RemoveProperty(NiFogProperty::GetType());
    m_spFogProperty = pkFogProperty;
    AttachProperty(pkFogProperty);
}

//---------------------------------------------------------------------------
NiFogProperty* NiEnvironment::GetFogProperty()
{
    if (!m_spFogProperty)
    {
        m_spFogProperty = NiStaticCast(NiFogProperty, 
            GetProperty(NiFogProperty::GetType()));
    }
    return m_spFogProperty;
}

//---------------------------------------------------------------------------
NiRenderClick* NiEnvironment::CreateSkyRenderClick(NiCamera* pkCamera)
{
    // Create the necessary render view to render the sky
    Ni3DRenderView* pkSkyRenderView = NiNew NiSkyRenderView(this, pkCamera);
    pkSkyRenderView->AppendScene(m_spSkyRoot);
    
    // Create a renderclick
    NiViewRenderClick* pkSkyRenderClick = NiNew NiViewRenderClick();
    pkSkyRenderClick->AppendRenderView(pkSkyRenderView);
    pkSkyRenderClick->SetClearAllBuffers(true);
    
    // Configure the render click to automatically adjust the fog/background
    pkSkyRenderClick->SetUseRendererBackgroundColor(false);
    pkSkyRenderClick->SetPreProcessingCallbackFunc(
        &NiSkyRenderView::CallbackClickConfigureFog, (void*)pkSkyRenderView);

    return pkSkyRenderClick;
}

//---------------------------------------------------------------------------
NiCubeMapRenderStep* NiEnvironment::CreateSkyBoxCubeMapRenderStep(
    NiRenderedCubeMap* pkDestinationCubeMap, NiRenderer* pkRenderer)
{
    // Create the render step
    NiCubeMapRenderStep* pkRenderStep = NiCubeMapRenderStep::Create(
        pkDestinationCubeMap, pkRenderer, m_spSkyRoot, m_spSkyRoot);
    EE_ASSERT(pkRenderStep);

    // Assign the appropriate preprocessing callback to render the environment
    pkRenderStep->SetPreProcessingCallbackFunc(&CallbackBakeRenderStep, this);
    
    return pkRenderStep;
}

//---------------------------------------------------------------------------
bool NiEnvironment::CallbackBakeRenderStep(NiRenderStep* pkCurrentStep,
    void* pvCallbackData)
{
    // Extract useful pointers from the callback data
    NiEnvironment* pkEnvironment = NiStaticCast(NiEnvironment, pvCallbackData);
    NiCubeMapRenderStep* pkCubeStep = NiDynamicCast(NiCubeMapRenderStep, 
        pkCurrentStep);
    EE_ASSERT(pkCubeStep);
    EE_ASSERT(pkEnvironment);

    // Fetch the environment fog color:
    NiFogProperty* pkFog = pkEnvironment->GetFogProperty();
    NiColor kFogColor = pkFog->GetFogColor();

    // Assign this as the background color of the render step
    if (pkEnvironment->GetAutoSetBackgroundColor())
    {
        NiColorA kBackgroundColor;
        kBackgroundColor.r = kFogColor.r;
        kBackgroundColor.g = kFogColor.g;
        kBackgroundColor.b = kFogColor.b;
        kBackgroundColor.a = 1.0f;
        pkCubeStep->SetBackgroundColor(kBackgroundColor);
        pkCubeStep->SetUseRendererBackgroundColor(false);
    }
    else
    {
        pkCubeStep->SetUseRendererBackgroundColor(true);
    }

    return true;
}

//---------------------------------------------------------------------------
