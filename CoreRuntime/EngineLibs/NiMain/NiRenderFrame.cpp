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

#include "NiRenderFrame.h"
#include "NiRenderer.h"
#include <efd/Profiler.h>

NiFixedString NiRenderFrame::ms_kDefaultName;
//--------------------------------------------------------------------------------------------------
EE_PROFILER_CONTEXT_EXTERN(Renderer);
//--------------------------------------------------------------------------------------------------
void NiRenderFrame::Draw()
{
    EE_PROFILER_ZONE(Renderer, 0, "RenderFrame::Draw");
    EE_PUSH_GPU_MARKER_VA("RenderFrame(%s)", (const char*)GetName());

    // Pre-processing callback.
    if (m_pfnPreProcessingCallback)
    {
        if (!m_pfnPreProcessingCallback(this, m_pvPreProcessingCallbackData))
        {
            return;
        }
    }

    // Get renderer pointer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Open rendering frame.
    if (m_bExecuteBeginEndDisplayFrame)
        pkRenderer->BeginFrame();

    // Iterate over render steps.
    NiTListIterator kStepsIter = m_kRenderSteps.GetHeadPos();
    while (kStepsIter)
    {
        NiRenderStep* pkRenderStep = m_kRenderSteps.GetNext(kStepsIter);
        EE_ASSERT(pkRenderStep);

        // Execute the render step if it is active.
        if (pkRenderStep->GetActive())
        {
            pkRenderStep->Render();
        }
    }

    // Close render target group if one is open. This is to prevent each
    // render step from having to close its render target group during Render.
    // Thus, each render step does not close its render target group and only
    // opens it if it is not already open. The last open render target group
    // is closed here.
    if (pkRenderer->IsRenderTargetGroupActive())
    {
        pkRenderer->EndUsingRenderTargetGroup();
    }

    // Close rendering frame.
    if (m_bExecuteBeginEndDisplayFrame)
        pkRenderer->EndFrame();

    // Post-processing callback.
    if (m_pfnPostProcessingCallback)
    {
        if (!m_pfnPostProcessingCallback(this, m_pvPostProcessingCallbackData))
        {
            return;
        }
    }

    EE_POP_GPU_MARKER();
}

//--------------------------------------------------------------------------------------------------
void NiRenderFrame::Display()
{
    EE_PROFILER_ZONE(Renderer, 0, "RenderFrame::Display");

    if (!m_bExecuteBeginEndDisplayFrame)
        return;

    // Get renderer pointer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Display the frame.
    pkRenderer->DisplayFrame();
}

//--------------------------------------------------------------------------------------------------
void NiRenderFrame::ReleaseCaches()
{
    // Iterate over render steps.
    NiTListIterator kStepsIter = m_kRenderSteps.GetHeadPos();
    while (kStepsIter)
    {
        NiRenderStep* pkRenderStep = m_kRenderSteps.GetNext(kStepsIter);
        EE_ASSERT(pkRenderStep);

        pkRenderStep->ReleaseCaches();
    }
}

//--------------------------------------------------------------------------------------------------
