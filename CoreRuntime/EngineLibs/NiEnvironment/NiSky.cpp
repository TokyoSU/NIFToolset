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
#include "NiSky.h"
#include "NiShadowGenerator.h"

//---------------------------------------------------------------------------
NiImplementRTTI(NiSky, NiNode);
//---------------------------------------------------------------------------
NiSky::NiSky():
    m_pkSun(0)
{

}
//---------------------------------------------------------------------------
NiSky::~NiSky()
{
}

//---------------------------------------------------------------------------
void NiSky::SetAtmosphere(NiAtmosphere* pkAtmosphere)
{
    if (m_spGeometry)
    {
        if (m_spAtmosphere)
            m_spAtmosphere->DetachExtraData(m_spGeometry);

        if (pkAtmosphere)
            pkAtmosphere->AttachExtraData(m_spGeometry);
    }

    m_spAtmosphere = pkAtmosphere;
}

//---------------------------------------------------------------------------
void NiSky::SetSun(NiDirectionalLight* pkSun)
{
    if (m_pkSun)
    {
        m_pkSun->DetachAffectedNode(this);
    }

    m_pkSun = pkSun;

    if (m_pkSun)
    {
        m_pkSun->AttachAffectedNode(this);

        // Stop the sky from casting shadows
        NiShadowGenerator* pGenerator = m_pkSun->GetShadowGenerator();
        if (pGenerator)
            pGenerator->AttachUnaffectedCasterNode(this);

        UpdateEffects();
    }
}

//---------------------------------------------------------------------------
void NiSky::LoadDefaultConfiguration()
{
    // Fetch an instance of the sky material:
    m_spSkyMaterial = NiSkyMaterial::Create();
    EE_ASSERT(m_spSkyMaterial);

    // Define the default method of calculating atmosphere
    m_eAtmosphericCalcMode = AtmosphericCalcMode::GPU_VS;

    // Empty the stages
    for (NiUInt32 ui = 0; ui< NUM_BLEND_STAGES; ++ui)
    {
        m_aspBlendStages[ui] = NULL;
    }
}

//---------------------------------------------------------------------------
NiSkyBlendStage* NiSky::GetBlendStage(NiUInt32 uiStage)
{
    EE_ASSERT(uiStage < NUM_BLEND_STAGES);

    return m_aspBlendStages[uiStage];
}

//---------------------------------------------------------------------------
void NiSky::InsertBlendStage(NiUInt32 uiStage, NiSkyBlendStage* pkStage)
{
    NiUInt32 uiMaxStageIndex = NUM_BLEND_STAGES - 1;

    // Are we able to shuffle stages?
    EE_ASSERT(m_aspBlendStages[uiMaxStageIndex] == NULL);

    for (NiUInt32 ui = uiMaxStageIndex; ui > uiStage; --ui)
    {
        m_aspBlendStages[ui] = m_aspBlendStages[ui - 1];
    }

    SetBlendStage(uiStage, pkStage);
}

//---------------------------------------------------------------------------
void NiSky::SetBlendStage(NiUInt32 uiStage, NiSkyBlendStage* pkStage)
{
    EE_ASSERT(uiStage < NUM_BLEND_STAGES);

    m_aspBlendStages[uiStage] = pkStage;

    m_bBlendStagesChanged = true;
}

//---------------------------------------------------------------------------
void NiSky::UpdateBlendStages(NiAVObject* pkObject, bool bForceUpdate)
{
    // Update all the blend stages on this object for the NiSkyMaterial
    for (NiUInt32 ui = 0; ui < NUM_BLEND_STAGES; ++ui)
    {
        if (m_aspBlendStages[ui] && 
            m_aspBlendStages[ui]->GetEnabled())
        {
            if (m_aspBlendStages[ui]->HasPropertyChanged() || bForceUpdate)
            {
                m_aspBlendStages[ui]->ConfigureObject(pkObject, ui, 
                    m_spSkyMaterial);
            }
        }
        else
        {
            m_spSkyMaterial->DisableBlendStage(pkObject, ui);
        }
    }
}

//---------------------------------------------------------------------------
void NiSky::DoUpdate(NiUpdateProcess& kUpdate)
{       
    EE_UNUSED_ARG(kUpdate);

    // Update the dome geometry if required
    if (m_bGeometrySettingsChanged)
    {
        UpdateGeometry();
        m_bGeometrySettingsChanged = false;
    }

    // Update any changed blend stages
    if (m_spGeometry)
    {
        UpdateBlendStages(m_spGeometry, m_bBlendStagesChanged);
        m_bBlendStagesChanged = false;
    }

    // Update any changing shader constants
    UpdateShaderConstants(m_spGeometry);
}

//---------------------------------------------------------------------------
void NiSky::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{        
    DoUpdate(kUpdate);
    NiNode::UpdateDownwardPass(kUpdate); 
}

//---------------------------------------------------------------------------
void NiSky::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{    
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    }

    NiNode::UpdateSelectedDownwardPass(kUpdate);
}

//---------------------------------------------------------------------------
void NiSky::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    } 

    NiNode::UpdateRigidDownwardPass(kUpdate);
}

//---------------------------------------------------------------------------
void NiSky::AttachExtraData(NiAVObject* pkObject)
{
    // Attach the atmospheric extra data to this object
    if (m_spAtmosphere)
        m_spAtmosphere->AttachExtraData(pkObject);

    UpdateBlendStages(pkObject, true);
}

//---------------------------------------------------------------------------
void NiSky::UpdateShaderConstants(NiAVObject* pkObject)
{
    // Attach the extra data that specifies the atmosphere calc mode
    m_spSkyMaterial->SetAtmosphericCalcMode(pkObject,
        m_eAtmosphericCalcMode);
}

//---------------------------------------------------------------------------
