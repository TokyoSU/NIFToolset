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
#include "NiLODNode.h"
#include "NiShadowGenerator.h"
#include "NiShadowRenderClick.h"

NiImplementRTTI(NiShadowRenderClick, NiViewRenderClick);

bool NiShadowRenderClick::ms_bDisableLODProcessing = true;

//------------------------------------------------------------------------------------------------
void NiShadowRenderClick::PerformRendering(unsigned int uiFrameID)
{
    EE_PUSH_GPU_MARKER_VA("ShadowRC(%s)", (const char*)GetName());

    EE_ASSERT(m_pkGenerator);

    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    pkRenderer->SetRenderShadowCasterBackfaces(
        m_pkGenerator->GetRenderBackfaces());
    pkRenderer->SetRenderShadowTechnique(
        m_pkGenerator->GetShadowTechnique());

    // If there are no views, return without rendering.
    if (m_kRenderViews.GetSize() == 0)
    {
        return;
    }

    // Get a pointer to the render list processor, using the default if none
    // has been specified.
    NiRenderListProcessor* pkProcessor = m_spProcessor;
    if (!pkProcessor)
        pkProcessor = ms_spDefaultProcessor;

    // Reset rendering statistics.
    m_uiNumObjectsDrawn = 0;
    m_fCullTime = m_fRenderTime = 0.0f;

    // Iterate over all render views.
    NiTListIterator kIter = m_kRenderViews.GetHeadPos();
    while (kIter)
    {
        NiRenderView* pkRenderView = m_kRenderViews.GetNext(kIter);
        EE_ASSERT(pkRenderView);
        EE_ASSERT(NiIsKindOf(Ni3DRenderView, pkRenderView));
        Ni3DRenderView* pk3DRenderView = (Ni3DRenderView*)pkRenderView;

        NiCullingProcess* pkShadowCullingProcess =
            pk3DRenderView->GetCullingProcess();
        pkShadowCullingProcess->SetLODCamera(
        NiShadowManager::GetSceneCamera());

        // Set up the renderer's camera data.
        pk3DRenderView->SetCameraData(m_kViewport);

        // Get set of potentially visible geometry.
        float fBeginTime = NiGetCurrentTimeInSec();
        const NiVisibleArray& kGeometryToProcess =
            pk3DRenderView->GetPVGeometry((efd::UInt32)NiRenderView::FORCE_PV_GEOMETRY_UPDATE);

        // Update rendering statistics.
        m_fCullTime += NiGetCurrentTimeInSec() - fBeginTime;
        m_uiNumObjectsDrawn += kGeometryToProcess.GetCount();

        // Resize processed geometry list, if necessary (grow only).
        EE_ASSERT(m_kProcessedGeometry.GetCount() == 0);
        if (m_kProcessedGeometry.GetAllocatedSize() < kGeometryToProcess.GetCount())
        {
            m_kProcessedGeometry.SetAllocatedSize(kGeometryToProcess.GetCount());
        }

        // Use render list processor to pre-process the array of geometry.
        fBeginTime = NiGetCurrentTimeInSec();
        pkProcessor->PreRenderProcessList(&kGeometryToProcess,
            m_kProcessedGeometry, m_pvProcessorData);

        // Immediately render geometry returned from render list processor.
        const unsigned int uiGeometryCount = m_kProcessedGeometry.GetCount();
        for (unsigned int ui = 0; ui < uiGeometryCount; ui++)
        {
            NiAVObject* pkAVObj = &m_kProcessedGeometry.GetAt(ui);
            EE_ASSERT(NiIsKindOf(NiRenderObject, pkAVObj));
            reinterpret_cast<NiRenderObject*>(pkAVObj)->
                RenderImmediate(pkRenderer);
        }

        // Use render list processor to post-process the array of geometry
        // that was just rendered.
        pkProcessor->PostRenderProcessList(m_kProcessedGeometry,
            m_pvProcessorData);

        // Update rendering statistics.
        m_fRenderTime += NiGetCurrentTimeInSec() - fBeginTime;

        // Ensure array is clear for next render view.
        m_kProcessedGeometry.RemoveAll();
    }

    SetBit(false, DIRTY);

    m_uiLastRenderedFrameID = uiFrameID;

    EE_POP_GPU_MARKER();
}

//------------------------------------------------------------------------------------------------
