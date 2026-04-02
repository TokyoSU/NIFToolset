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
#include "NiCubeMapRenderStep.h"

#include <NiCamera.h>
#include <NiRenderer.h>
#include <NiRenderTargetGroup.h>
#include <NiDrawSceneUtility.h>

//---------------------------------------------------------------------------
NiImplementRTTI(NiCubeMapRenderStep, NiRenderStep);

//---------------------------------------------------------------------------
NiCubeMapRenderStep* NiCubeMapRenderStep::Create(NiRenderedCubeMap* pkMap, 
    NiRenderer* pkRenderer, NiNode* pkScene, NiAVObject* pkReference)
{
    // Make sure all the required elements are present
    if (!(pkMap && pkRenderer && pkScene && pkReference))
        return NULL;

    // Create the cubemap render step
    NiCubeMapRenderStep* pkThis = NiNew NiCubeMapRenderStep(pkScene);

    // Create an appropriate frustum for the camera used to render the maps
    pkThis->m_spCamera = NiNew NiCamera;
    NiFrustum kFr = pkThis->m_spCamera->GetViewFrustum();
    kFr.m_fLeft = -1.0f;
    kFr.m_fRight = 1.0f;
    kFr.m_fBottom = -1.0f;
    kFr.m_fTop = 1.0f;
    kFr.m_fNear = 1.0f;
    kFr.m_fFar = 10000.0f;
    pkThis->m_spCamera->SetViewFrustum(kFr);

    // Assign the renderer
    pkThis->m_spRenderer = pkRenderer;

    // Assign the texture to render to
    pkThis->m_spRenderedTexture = pkMap;

    // Generate the render targets for each face
    NiRenderTargetGroup::CubeMapSetupHelper kHelper(pkMap);
    for (unsigned int ui = 0; ui < NiRenderedCubeMap::FACE_NUM; ui++)
    {
        pkThis->m_aspRenderTargetGroups[ui] = 
            kHelper.m_aspRenderTargetGroups[ui];
        EE_ASSERT(pkThis->m_aspRenderTargetGroups[ui]);
    }

    // Assign the reference object
    pkThis->m_spReference = pkReference;

    // Initialize the number of faces to update per frame
    pkThis->m_uiCamerasPerUpdate = 6;
    pkThis->m_uiLastUpdatedCamera = 5;

    return pkThis;
}

//---------------------------------------------------------------------------
NiCubeMapRenderStep::NiCubeMapRenderStep(NiNode* pkScene)
    :
    m_spScene(pkScene),
    m_kVisible(1024, 1024),
    m_kCuller(&m_kVisible, NULL),
    m_uiNumObjectsDrawn(0),
    m_fCullTime(0.0f),
    m_fRenderTime(0.0f)
{
    SetSwapLeftRight(true);
    SetUseRendererBackgroundColor(true);
    SetPersistBackgroundColorToRenderer(false);
}

//---------------------------------------------------------------------------
NiCubeMapRenderStep::~NiCubeMapRenderStep()
{
}

//---------------------------------------------------------------------------
void NiCubeMapRenderStep::PerformRendering()
{
    // Setup the camera to render from the reference point
    m_spCamera->SetTranslate(m_spReference->GetWorldTranslate());

    // Reset rendering statistics.
    m_uiNumObjectsDrawn = 0;
    m_fCullTime = m_fRenderTime = 0.0f;

    // Organize the left/right swap in the renderer 
    // (skybox Vs Reflection generation)
    bool bOrigLeftRightSwap = m_spRenderer->GetLeftRightSwap();
    m_spRenderer->SetLeftRightSwap(GetSwapLeftRight());

    // Configure the renderer background
    NiColorA kOriginalBGColor;
    m_spRenderer->GetBackgroundColor(kOriginalBGColor);
    if (!GetUseRendererBackgroundColor())
    {
        m_spRenderer->SetBackgroundColor(m_kBackgroundColor);
    }

    // Render the requested faces in this update
    unsigned int i;
    for (i = 0; i < m_uiCamerasPerUpdate; i++)
    {
        // Select the face to render
        m_uiLastUpdatedCamera++;
        if (m_uiLastUpdatedCamera >= NiRenderedCubeMap::FACE_NUM)
            m_uiLastUpdatedCamera = 0;

        // Orient the camera
        NiMatrix3 kMat;
        switch((NiRenderedCubeMap::FaceID)m_uiLastUpdatedCamera)
        {
            case NiRenderedCubeMap::FACE_POS_X:
                kMat.SetRow(0, 1.0f, 0.0f, 0.0f);
                kMat.SetRow(1, 0.0f, 1.0f, 0.0f);
                kMat.SetRow(2, 0.0f, 0.0f, 1.0f);
                break;
            case NiRenderedCubeMap::FACE_NEG_X:
                kMat.SetRow(0, -1.0f, 0.0f, 0.0f);
                kMat.SetRow(1, 0.0f, 1.0f, 0.0f);
                kMat.SetRow(2, 0.0f, 0.0f, -1.0f);
                break;
            case NiRenderedCubeMap::FACE_POS_Y:
                kMat.SetRow(0, 0.0f, 0.0f, -1.0f);
                kMat.SetRow(1, 1.0f, 0.0f, 0.0f);
                kMat.SetRow(2, 0.0f, -1.0f, 0.0f);
                break;
            case NiRenderedCubeMap::FACE_NEG_Y:
                kMat.SetRow(0, 0.0f, 0.0f, -1.0f);
                kMat.SetRow(1, -1.0f, 0.0f, 0.0f);
                kMat.SetRow(2, 0.0f, 1.0f, 0.0f);
                break;
            case NiRenderedCubeMap::FACE_POS_Z:
                kMat.SetRow(0, 0.0f, 0.0f, -1.0f);
                kMat.SetRow(1, 0.0f, 1.0f, 0.0f);
                kMat.SetRow(2, 1.0f, 0.0f, 0.0f);
                break;
            case NiRenderedCubeMap::FACE_NEG_Z:
                kMat.SetRow(0, 0.0f, 0.0f, 1.0f);
                kMat.SetRow(1, 0.0f, 1.0f, 0.0f);
                kMat.SetRow(2, -1.0f, 0.0f, 0.0f);
                break;
            case NiRenderedCubeMap::FACE_NUM:
                break;
        }
        m_spCamera->SetRotate(kMat);
        m_spCamera->Update(0.0f);

        // Set the correct cube face and render target
        m_spRenderedTexture->SetCurrentCubeFace(
            (NiRenderedCubeMap::FaceID)m_uiLastUpdatedCamera);
        m_spRenderer->BeginUsingRenderTargetGroup(
            m_aspRenderTargetGroups[m_uiLastUpdatedCamera],
            NiRenderer::CLEAR_ALL);

        // Render the face
        NiVisibleArray* pkVisibleSet = m_kCuller.GetVisibleSet();
        if (pkVisibleSet)
        {
            // Cull scene.
            float fBeginTime = NiGetCurrentTimeInSec();
            NiCullScene(m_spCamera, m_spScene, m_kCuller, *pkVisibleSet);
            m_fCullTime += NiGetCurrentTimeInSec() - fBeginTime;

            // Render scene.
            fBeginTime = NiGetCurrentTimeInSec();
            NiDrawVisibleArray(m_spCamera, *pkVisibleSet);
            m_fRenderTime += NiGetCurrentTimeInSec() - fBeginTime;
            m_uiNumObjectsDrawn += pkVisibleSet->GetCount();
        }

        m_spRenderer->EndUsingRenderTargetGroup();
    }

    // Restore the renderer background
    if (!GetUseRendererBackgroundColor() && 
        !GetPersistBackgroundColorToRenderer())
    {
        m_spRenderer->SetBackgroundColor(kOriginalBGColor);
    }

    // Restore the original left/right orientation
    m_spRenderer->SetLeftRightSwap(bOrigLeftRightSwap);
}

//---------------------------------------------------------------------------
NiPixelData* NiCubeMapRenderStep::TakeScreenshot(
    NiRenderedCubeMap::FaceID uiFaceID)
{
    EE_ASSERT(uiFaceID < NiRenderedCubeMap::FACE_NUM);

    return m_spRenderer->TakeScreenShot(NULL, 
        m_aspRenderTargetGroups[uiFaceID]);
}

//---------------------------------------------------------------------------
unsigned int NiCubeMapRenderStep::GetNumObjectsDrawn() const
{
    return m_uiNumObjectsDrawn;
}

//---------------------------------------------------------------------------
float NiCubeMapRenderStep::GetCullTime() const
{
    return m_fCullTime;
}

//---------------------------------------------------------------------------
float NiCubeMapRenderStep::GetRenderTime() const
{
    return m_fRenderTime;
}

//---------------------------------------------------------------------------
bool NiCubeMapRenderStep::SetOutputRenderTargetGroup(
    NiRenderTargetGroup*)
{
    // This render step does not have an output render target group and thus
    // does not allow it to be set.
    return false;
}

//---------------------------------------------------------------------------
NiRenderTargetGroup* NiCubeMapRenderStep::GetOutputRenderTargetGroup()
{
    // This render step does not have an output render target group and thus
    // does not allow it to be retrieved.
    return NULL;
}

//---------------------------------------------------------------------------
