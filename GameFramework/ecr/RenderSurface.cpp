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
#include "ecrPCH.h"

#include "RenderSurface.h"
#include "RenderService.h"
#include "NiRenderFrame.h"
#include "RenderSurfaceStep.h"
#include <NiMath.h>

using namespace ecr;

//--------------------------------------------------------------------------------------------------
RenderSurface::RenderSurface(efd::WindowRef windowHandle, RenderService* pService) :
    m_windowRef(windowHandle), m_pRenderService(pService)
{
    m_spCamera = NiNew NiCamera(); // Dummy camera

    m_spSceneRenderClick = NiNew NiViewRenderClick;
    m_spSceneRenderClick->SetName("SceneGraphClick");

    NiDefaultClickRenderStep* pClickRenderStep = NiNew RenderSurfaceStep();
    m_spRenderStep = pClickRenderStep;
    pClickRenderStep->AppendRenderClick(m_spSceneRenderClick);

    // Create a new NiRenderFrame, but disable the usage of BeginFrame(), EndFrame()
    // and DisplayFrame() since these will be manually called in the RenderContext.
    m_spRenderFrame = NiNew NiRenderFrame(false);
    m_spRenderFrame->AppendRenderStep(m_spRenderStep);
}
//--------------------------------------------------------------------------------------------------
RenderSurface::~RenderSurface()
{
    m_spCamera = 0;
    m_spRenderFrame = 0;
    m_spRenderStep = 0;
    m_spSceneRenderClick = 0;
}
//--------------------------------------------------------------------------------------------------
void RenderSurface::SetMainRenderClick(NiViewRenderClick* pRenderClick)
{
    EE_ASSERT(pRenderClick);

    NiDefaultClickRenderStep* pClickRenderStep =
        NiDynamicCast(NiDefaultClickRenderStep, m_spRenderStep);

    if (pClickRenderStep)
    {
        // Copy necessary information from the previous main render click
        // This is information that other main scene render clicks likely wouldn't change.
        if (m_spSceneRenderClick)
        {
            pRenderClick->SetRenderTargetGroup(
                m_spSceneRenderClick->GetRenderTargetGroup());

            NiColorA backgroundColor;
            m_spSceneRenderClick->GetBackgroundColor(backgroundColor);
            pRenderClick->SetBackgroundColor(backgroundColor);

            bool bClearColorBuffers = m_spSceneRenderClick->GetClearColorBuffers();
            bool bClearDepthBuffer = m_spSceneRenderClick->GetClearDepthBuffer();
            bool bClearStencilBuffer = m_spSceneRenderClick->GetClearStencilBuffer();
            pRenderClick->SetClearAllBuffers(false); // Clear the existing values
            pRenderClick->SetClearColorBuffers(bClearColorBuffers);
            pRenderClick->SetClearDepthBuffer(bClearDepthBuffer);
            pRenderClick->SetClearStencilBuffer(bClearStencilBuffer);

            const NiTPointerList<NiRenderViewPtr>& views = m_spSceneRenderClick->GetRenderViews();
            NiTListIterator viewItr = views.GetHeadPos();
            while (viewItr)
            {
                pRenderClick->AppendRenderView(views.Get(viewItr));
                viewItr = views.GetNextPos(viewItr);
            }
        }

        pRenderClick->SetName("SceneGraphClick");

        // Swap out the old render click for the new one
        NiTPointerList<NiRenderClickPtr>& clickList = pClickRenderStep->GetRenderClickList();
        NiTListIterator itr = pClickRenderStep->GetRenderClickPosByName("SceneGraphClick");
        clickList.InsertAfter(itr, (NiRenderClickPtr)pRenderClick);
        clickList.RemovePos(itr);

        m_spSceneRenderClick = pRenderClick;
    }
}
//--------------------------------------------------------------------------------------------------
void RenderSurface::Draw(efd::Float32 currentTime)
{
    EE_UNUSED_ARG(currentTime);

    m_pRenderService->RaisePreDrawEvent(this);

    m_spRenderFrame->Draw();
    m_spRenderFrame->Display();

    m_pRenderService->RaisePostDrawEvent(this);
}
//--------------------------------------------------------------------------------------------------
NiRenderStep* RenderSurface::GetRenderStep()
{
    return m_spRenderStep;
}
//--------------------------------------------------------------------------------------------------
void RenderSurface::SetupDefaultCamera(
    efd::UInt32 uiWidth, efd::UInt32 uiHeight,
    float fFovDegrees, float fNear, float fFar)
{
    if (!m_spCamera)
        return;

    const float fAspect = (uiHeight > 0)
        ? static_cast<float>(uiWidth) / static_cast<float>(uiHeight)
        : (16.0f / 9.0f);

    // Half-height at the near plane, derived from the vertical FOV.
    const float fHalfFovRad = fFovDegrees * (NI_PI / 360.0f);
    const float fTop        =  1.0f * tanf(fHalfFovRad);
    const float fRight      =  fTop  * fAspect;

    m_spCamera->SetViewFrustum(
        NiFrustum(-fRight, fRight, fTop, -fTop, fNear, fFar, false));

    // Default position: 10 units back on -X, 3 up on Z, looking at the world origin.
    // Override with GetCamera() or a camera controller (e.g. GFCameraController).
    const NiPoint3 kPos   (-10.0f, 0.0f, 3.0f);
    const NiPoint3 kTarget(  0.0f, 0.0f, 0.0f);

    NiPoint3 kDir = kTarget - kPos;
    kDir.Unitize();

    static const NiPoint3 kWorldUp(0.0f, 0.0f, 1.0f);
    NiPoint3 kRight = kDir.UnitCross(kWorldUp);

    // Degenerate fallback — camera pointing straight up or down.
    if (kRight.SqrLength() < 0.5f)
        kRight = kDir.UnitCross(NiPoint3(0.0f, 1.0f, 0.0f));

    const NiPoint3 kUp = kRight.UnitCross(kDir);

    m_spCamera->SetTranslate(kPos);
    m_spCamera->SetRotate(NiMatrix3(kDir, kUp, kRight));
    m_spCamera->Update(0.0f);
}
//--------------------------------------------------------------------------------------------------
