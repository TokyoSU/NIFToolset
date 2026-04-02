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
#include "NiMeshPCH.h"

#include "NiMesh2DRenderView.h"

NiImplementRTTI(NiMesh2DRenderView, NiRenderView);

//--------------------------------------------------------------------------------------------------
void NiMesh2DRenderView::SetCameraData(const NiRect<float>& kViewport)
{
    // Get renderer pointer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Set screen-space camera data using viewport.
    pkRenderer->SetScreenSpaceCameraData(&kViewport);
}

//--------------------------------------------------------------------------------------------------
void NiMesh2DRenderView::CalculatePVGeometry()
{
    EE_ASSERT(m_kCachedPVGeometry.GetCount() == 0);

    // Add all screen elements to the PV geometry array.
    NiTListIterator kIter = m_kScreenElements.GetHeadPos();
    while (kIter)
    {
        NiMeshScreenElements* pkScreenElement =
            m_kScreenElements.GetNext(kIter);
        EE_ASSERT(pkScreenElement);
        m_kCachedPVGeometry.Add(*pkScreenElement);
    }
}

//--------------------------------------------------------------------------------------------------
