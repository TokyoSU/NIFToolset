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

// Precompiled Header
#include "NiMainPCH.h"

#include "NiShadowMaterialSwapProcessor.h"
#include "NiRenderer.h"
#include "NiRenderObject.h"

NiImplementRTTI(NiShadowMaterialSwapProcessor, NiMaterialSwapProcessor);

//--------------------------------------------------------------------------------------------------
void NiShadowMaterialSwapProcessor::PreRenderProcessList(
    const NiVisibleArray* pkInput, NiVisibleArray& kOutput, void*)
{
    // If the input array pointer is null, do nothing.
    if (!pkInput)
    {
        return;
    }

    const unsigned int uiInputCount = pkInput->GetCount();

    // If the material pointer is null, defer rendering of all objects.
    if (!m_spMaterial)
    {
        for (unsigned int ui = 0; ui < uiInputCount; ui++)
        {
            kOutput.Add(pkInput->GetAt(ui));
        }
        return;
    }

    // Get renderer pointer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Clear the old material cache
    m_kOldMaterials.RemoveAll();

    // Iterate over input array, swapping material and rendering.
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);

        // Backup current active material.
        m_kOldMaterials.Add(kMesh.GetActiveMaterial());

        // Apply and set active material to new material.
        kMesh.ApplyAndSetActiveMaterial(m_spMaterial,
            m_uiMaterialExtraData);

        if (!pkRenderer->PrecacheShader(&kMesh))
            continue;

        // Add the geometry objects to the output array.
        kOutput.Add(kMesh);
    }

}

//--------------------------------------------------------------------------------------------------
void NiShadowMaterialSwapProcessor::PostRenderProcessList(
    NiVisibleArray& kPreviousOutput, void* pvExtraData)
{
    EE_UNUSED_ARG(pvExtraData);

    const unsigned int uiPreviousOutputCount = kPreviousOutput.GetCount();

    for (unsigned int ui = 0; ui < uiPreviousOutputCount; ui++)
    {
        NiRenderObject& kMesh = kPreviousOutput.GetAt(ui);

        kMesh.SetActiveMaterial(m_kOldMaterials.GetAt(ui));
    }

}

//--------------------------------------------------------------------------------------------------
