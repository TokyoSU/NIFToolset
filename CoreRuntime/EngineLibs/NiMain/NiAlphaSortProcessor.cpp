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

#include "NiAlphaSortProcessor.h"
#include "NiRenderer.h"

NiImplementRTTI(NiAlphaSortProcessor, NiBackToFrontSortProcessor);

//--------------------------------------------------------------------------------------------------
void NiAlphaSortProcessor::PreRenderProcessList(const NiVisibleArray* pkInput,
    NiVisibleArray& kOutput, void*)
{
    // If the input array pointer is null, do nothing.
    if (!pkInput)
    {
        return;
    }

    // Get renderer pointer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Get camera data from renderer.
    NiPoint3 kWorldLoc, kWorldDir, kWorldUp, kWorldRight;
    NiFrustum kFrustum;
    NiRect<float> kViewport;
    pkRenderer->GetCameraData(kWorldLoc, kWorldDir, kWorldUp, kWorldRight,
        kFrustum, kViewport);

    const unsigned int uiInputCount = pkInput->GetCount();

    // Initialize size of object depths array.
    if (m_uiAllocatedDepths < uiInputCount)
    {
        NiFree(m_pfDepths);
        m_pfDepths = NiAlloc(float, uiInputCount);
        m_uiAllocatedDepths = uiInputCount;
    }

    // Iterate over input geometry array.
    unsigned int uiDepthIndex = 0;
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);

        if (IsTransparent(kMesh))
        {
            // Add geometry to output array.
            kOutput.Add(kMesh);

            // Compute and store depth from camera.
            m_pfDepths[uiDepthIndex++] = ComputeDepth(kMesh, kWorldDir);
        }
        else
        {
            // Render opaque geometry immediately.
            NiAVObject* pkAVObj = &kMesh;
            if (NiIsKindOf(NiRenderObject, pkAVObj))
            {
                kMesh.RenderImmediate(pkRenderer);
            }
        }
    }

    // Sort output array by depth.
    SortObjectsByDepth(kOutput, 0, kOutput.GetCount() - 1);

    // Immediately render transparent objects now that they've been sorted.  Note that the array
    // is organized such that the "back-most" object appears at the end of the array.
    const unsigned int uiGeometryCount = kOutput.GetCount();
    for (unsigned int ui = uiGeometryCount; ui > 0; ui--)
    {
        NiAVObject* pkAVObj = &kOutput.GetAt(ui - 1);
        EE_ASSERT(NiIsKindOf(NiRenderObject, pkAVObj));
        reinterpret_cast<NiRenderObject*>(pkAVObj)->RenderImmediate(pkRenderer);
    }

    // Ensure array is clear for next render view.
    kOutput.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
