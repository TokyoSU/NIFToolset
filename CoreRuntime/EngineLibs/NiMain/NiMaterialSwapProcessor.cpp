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

#include "NiMaterialSwapProcessor.h"
#include "NiRenderer.h"
#include "NiRenderObject.h"

NiImplementRTTI(NiMaterialSwapProcessor, NiRenderListProcessor, NiTypeMask::NiMaterialSwapProcessor);

//--------------------------------------------------------------------------------------------------
void NiMaterialSwapProcessor::PreRenderProcessList(
    const NiVisibleArray* pkInput, NiVisibleArray& kOutput, void* pvExtraData)
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

    // Swap materials
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);
        kMesh.SwapMaterial(m_spMaterial, m_uiMaterialExtraData);
    }

    NiShaderSortProcessor::PreRenderProcessList(pkInput, kOutput, pvExtraData);

    // Restore materials
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);
        kMesh.RestoreSwapMaterial();
    }
}

//--------------------------------------------------------------------------------------------------
