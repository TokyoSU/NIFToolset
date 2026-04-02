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

#include "NiTerrainPCH.h"

#include "NiLODNode.h"
#include "NiDynamicEffect.h"
#include "NiShadowGenerator.h"
#include "NiShadowRenderClick.h"
#include "NiTerrainShadowClickValidator.h"
#include "NiTerrainCullingProcess.h"

NiImplementRTTI(NiTerrainShadowClickValidator, NiShadowClickValidator);

//------------------------------------------------------------------------------------------------
NiShadowClickValidator* NiTerrainShadowClickValidator::CreateTerrainShadowClickValidator()
{
    return NiNew NiTerrainShadowClickValidator();
}

//------------------------------------------------------------------------------------------------
bool NiTerrainShadowClickValidator::ValidateClick(
    NiRenderClick* pkRenderClick,
    unsigned int uiFrameID)
{
    EE_UNUSED_ARG(uiFrameID);

    // Cast render click to an NiShadowRenderClick.
    NiShadowRenderClick* pkShadowClick = NiDynamicCast(NiShadowRenderClick,
        pkRenderClick);
    if (!pkShadowClick)
    {
        // If the render click is not a shadow click, allow rendering.
        return true;
    }
    else if (pkShadowClick->GetForceRender())
    {
        // If the shadow click is set to force render, allow rendering.
        pkShadowClick->MarkAsDirty();
        return true;
    }

    // If no camera or culling process have been set, allow rendering.
    if (!m_spCamera || !m_spCullingProcess)
    {
        pkShadowClick->MarkAsDirty();
        return true;
    }

    // Tell the culling process not to submit mesh modifiers. This allows
    // a culling pass to be performed without an associated rendering pass.
    bool bOldSubmitModifiers = m_spCullingProcess->GetSubmitModifiers();
    m_spCullingProcess->SetSubmitModifiers(false);

    m_kVisibleSet.RemoveAll();

    // Get shadow generator.
    NiShadowGenerator* pkGenerator = pkShadowClick->GetGenerator();
    EE_ASSERT(pkGenerator);

    NiTerrainCullingProcess* pkTerrainCullingProcess =
        NiDynamicCast(NiTerrainCullingProcess, m_spCullingProcess);

    // Since we don't intend on rendering the results of the following
    // culling pass, disable updating the terrain geometry.
    if (pkTerrainCullingProcess)
        pkTerrainCullingProcess->SetUpdateGeometry(false);

    NiDynamicEffect* pkDynEffect = pkGenerator->GetAssignedDynamicEffect();
    EE_ASSERT(pkDynEffect);

    m_spCullingProcess->SetCamera(m_spCamera);
    m_spCullingProcess->SetFrustum(m_spCamera->GetViewFrustum());
    m_spCullingProcess->SetVisibleSet(&m_kVisibleSet);

    // Loop over shadow receivers (affected nodes list), culling each to
    // determine its visibility. If any are visible, allow rendering.
    bool bRetVal = false;

    const NiNodeList& kReceivers = pkDynEffect->GetAffectedNodeList();
    NiTListIterator kIter = kReceivers.GetHeadPos();
    while (kIter)
    {
        NiNode* pkReceiver = kReceivers.GetNext(kIter);

        // Cull the shadow receiver against the camera using the culling
        // process.
        pkReceiver->Cull(*m_spCullingProcess);

        // Determine whether or not the receiver is visible. If so, return
        // immediately allowing rendering.
        if (m_kVisibleSet.GetCount() > 0)
        {
            // Mark the shadow click as dirty.
            pkShadowClick->MarkAsDirty();

            bRetVal = true;
            break;
        }
    }

    // Since we don't intend on rendering the results of the following
    // culling pass, disable updating the terrain geometry.
    if (pkTerrainCullingProcess)
        pkTerrainCullingProcess->SetUpdateGeometry(true);

    // Restore the previous state of the culling process.
    m_spCullingProcess->SetSubmitModifiers(bOldSubmitModifiers);

    return bRetVal;
}

//------------------------------------------------------------------------------------------------