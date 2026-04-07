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

#include "NiMeshPCH.h"
#include "NiDebugVisualizationClick.h"

#include <efd/BitUtils.h>
#include <NiImmediateModeMacro.h>
#include <NiNode.h>
#include <NiCamera.h>
#include <NiAmbientLight.h>
#include <NiDirectionalLight.h>
#include <NiSpotLight.h>

using namespace efd;

NiImplementRTTI(NiDebugVisualizationClick, NiRenderClick);

//--------------------------------------------------------------------------------------------------
NiDebugVisualizationClick::NiDebugVisualizationClick(NiUInt8 uiMask) :
    m_fCullTime(0.0f),
    m_fRenderTime(0.0f),
    m_uiNumObjectsDrawn(0),
    m_fScreenMultiplier(0.05f),
    m_uiSettingsMask(uiMask)
{
    m_kAdapter.SetNumMaxVertices(1024);
}

//--------------------------------------------------------------------------------------------------
float NiDebugVisualizationClick::GetScreenMultiplier() const
{
    return m_fScreenMultiplier;
}

//--------------------------------------------------------------------------------------------------
void NiDebugVisualizationClick::SetScreenMultiplier(float fScreenMultiplier)
{
    m_fScreenMultiplier = fScreenMultiplier;
}

//---------------------------------------------------------------------------
bool NiDebugVisualizationClick::GetViewPointLights() const
{
    return (m_uiSettingsMask & POINT_LIGHT_MASK) != 0;
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetViewPointLights(bool bNewValue)
{
    m_uiSettingsMask = BitUtils::SetBitsOnOrOff<UInt8>(m_uiSettingsMask,
        POINT_LIGHT_MASK, bNewValue);
}

//---------------------------------------------------------------------------
bool NiDebugVisualizationClick::GetViewDirectionalLights() const
{
    return (m_uiSettingsMask & DIRECTIONAL_LIGHT_MASK) != 0;
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetViewDirectionalLights(bool bNewValue)
{
    m_uiSettingsMask = BitUtils::SetBitsOnOrOff<UInt8>(m_uiSettingsMask,
        DIRECTIONAL_LIGHT_MASK, bNewValue);
}

//---------------------------------------------------------------------------
bool NiDebugVisualizationClick::GetViewSpotLights() const
{
    return (m_uiSettingsMask & SPOT_LIGHT_MASK) != 0;
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetViewSpotLights(bool bNewValue)
{
    m_uiSettingsMask = BitUtils::SetBitsOnOrOff<UInt8>(m_uiSettingsMask,
        SPOT_LIGHT_MASK, bNewValue);
}

//---------------------------------------------------------------------------
bool NiDebugVisualizationClick::GetViewCameras() const
{
    return (m_uiSettingsMask & CAMERA_MASK) != 0;
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetViewCameras(bool bNewValue)
{
    m_uiSettingsMask = BitUtils::SetBitsOnOrOff<UInt8>(m_uiSettingsMask,
        CAMERA_MASK, bNewValue);
}

//---------------------------------------------------------------------------
bool NiDebugVisualizationClick::GetViewBounds() const
{
    return (m_uiSettingsMask & BOUNDS_MASK) != 0;
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetViewBounds(bool bNewValue)
{
    m_uiSettingsMask = BitUtils::SetBitsOnOrOff<UInt8>(m_uiSettingsMask,
        BOUNDS_MASK, bNewValue);
}

//---------------------------------------------------------------------------
bool NiDebugVisualizationClick::GetViewBones() const
{
    return (m_uiSettingsMask & BONE_MASK) != 0;
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetViewBones(bool bNewValue)
{
    m_uiSettingsMask = BitUtils::SetBitsOnOrOff<UInt8>(m_uiSettingsMask,
        BONE_MASK, bNewValue);
}

//---------------------------------------------------------------------------
bool NiDebugVisualizationClick::GetViewNodes() const
{
    return (m_uiSettingsMask & NODE_MASK) != 0;
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetViewNodes(bool bNewValue)
{
    m_uiSettingsMask = BitUtils::SetBitsOnOrOff<UInt8>(m_uiSettingsMask,
        NODE_MASK, bNewValue);
}

//---------------------------------------------------------------------------
bool NiDebugVisualizationClick::GetViewConnections() const
{
    return (m_uiSettingsMask & CONNECTION_MASK) != 0;
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetViewConnections(bool bNewValue)
{
    m_uiSettingsMask = BitUtils::SetBitsOnOrOff<UInt8>(m_uiSettingsMask,
        CONNECTION_MASK, bNewValue);
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::ClearRoots()
{
    m_kSceneRoots.RemoveAll();

    ClearScene(ALL_MASK);
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::AddRoot(NiAVObject* pkRoot)
{
    m_kSceneRoots.Add(pkRoot);
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::RemoveRoot(NiAVObject* pkRoot)
{
    int iIdx = m_kSceneRoots.Find(pkRoot);
    if (iIdx != -1)
    {
        m_kSceneRoots.RemoveAt(iIdx);
    }
}

//---------------------------------------------------------------------------
void NiDebugVisualizationClick::SetCamera(NiCamera* pkCamera)
{
    m_kAdapter.SetCurrentCamera(pkCamera);
}

//--------------------------------------------------------------------------------------------------
const NiCamera* NiDebugVisualizationClick::GetCamera() const
{
    return m_kAdapter.GetCurrentCamera();
}

//--------------------------------------------------------------------------------------------------
unsigned int NiDebugVisualizationClick::GetNumObjectsDrawn() const
{
    return m_uiNumObjectsDrawn;
}

//--------------------------------------------------------------------------------------------------
float NiDebugVisualizationClick::GetCullTime() const
{
    return m_fCullTime;
}

//--------------------------------------------------------------------------------------------------
float NiDebugVisualizationClick::GetRenderTime() const
{
    return m_fRenderTime;
}

//--------------------------------------------------------------------------------------------------
void NiDebugVisualizationClick::ClearScene(unsigned int uiClearMask)
{
    if (uiClearMask & CAMERA_MASK)
    {
        m_kCameras.RemoveAll();
    }

    if (uiClearMask & BOUNDS_MASK)
    {
        m_kBounds.RemoveAll();
    }

    if ((uiClearMask & LIGHT_MASK) == LIGHT_MASK)
    {
        m_kLights.RemoveAll();
    }
    else if (uiClearMask & LIGHT_MASK)
    {
        for (unsigned int i = 0; i < m_kLights.GetSize(); i++)
        {
            NiLight* pkLight = m_kLights.GetAt(i);
            if (NiIsKindOf(NiSpotLight, pkLight))
            {
                if (uiClearMask & SPOT_LIGHT_MASK)
                    m_kLights.RemoveAt(i);
            }
            else if (NiIsKindOf(NiPointLight, pkLight))
            {
                if (uiClearMask & POINT_LIGHT_MASK)
                    m_kLights.RemoveAt(i);
            }
            else if (NiIsKindOf(NiDirectionalLight, pkLight))
            {
                if (uiClearMask & DIRECTIONAL_LIGHT_MASK)
                    m_kLights.RemoveAt(i);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiDebugVisualizationClick::ProcessScene()
{
    //Clear previous process results
    ClearScene();

    //Process each root that has been registered
    unsigned int uiNumRoots = m_kSceneRoots.GetSize();
    for (unsigned int i = 0; i < uiNumRoots; i++)
    {
        NiAVObject* pkRoot = m_kSceneRoots.GetAt(i);
        if (pkRoot)
        {
            PerformProcessing(pkRoot);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiDebugVisualizationClick::PerformProcessing(NiAVObject* pkObject)
{
    if (!pkObject)
    {
        return;
    }

    {
        m_kBounds.Add(pkObject);
    }
    
    if (pkObject->IsNode())
    {
        NiNode* pkNode = NiVerifyStaticCast(NiNode, pkObject);
        for (unsigned int i = 0; i < pkNode->GetArrayCount(); i++)
            PerformProcessing(pkNode->GetAt(i));
    }
    else if (NiIsKindOf(NiCamera, pkObject))
    {
        m_kCameras.Add((NiCamera*)pkObject);
    }
    else if (NiIsKindOf(NiLight, pkObject))
    {
        if (NiIsKindOf(NiSpotLight, pkObject))
        {
            m_kLights.Add((NiLight*)pkObject);
        }
        else if (NiIsKindOf(NiPointLight, pkObject))
        {
            m_kLights.Add((NiLight*)pkObject);
        }
        else if (NiIsKindOf(NiDirectionalLight, pkObject))
        {
            m_kLights.Add((NiLight*)pkObject);
        }
    }
}

//--------------------------------------------------------------------------------------------------
 void NiDebugVisualizationClick::PerformRendering(unsigned int uiFrameID)
{
    EE_UNUSED_ARG(uiFrameID);

    m_fRenderTime = m_fCullTime = 0.0f;
    m_uiNumObjectsDrawn = 0;

    if (!m_kAdapter.GetCurrentCamera())
        return;

    float fBeginTime = NiGetCurrentTimeInSec();

    {
        NiImmediateModeMacro kMacro(m_kAdapter);

        // Colors
        const NiColorA kCameraColor(0.1098f, 0.3490f, 0.6941f, 1.0f);
        const NiColorA kPointLightColor(1.0f, 0.9216f, 0.0275f, 1.0f);
        const NiColorA kDirectionalLightColor(1.0f, 0.9216f, 0.0275f, 1.0f);
        const NiColorA kSpotLightColor(1.0f, 0.9216f, 0.0275f, 1.0f);
        //const NiColorA kBoundingSphereColor(1.0f, 0.316f, 0.1f, 1.0f);

        // Draw cameras
        if (GetViewCameras())
        {
            m_kAdapter.SetCurrentColor(kCameraColor);
            unsigned int uiNumCameras = m_kCameras.GetSize();
            for (unsigned int i = 0; i < uiNumCameras; i++)
            {
                NiCamera* pkCamera = m_kCameras.GetAt(i);
                if (pkCamera->GetAppCulled() ||
                    pkCamera == m_kAdapter.GetCurrentCamera())
                {
                    continue;
                }

                float fScreenScale = m_fScreenMultiplier *
                    kMacro.GetScreenScaleFactor(pkCamera);

                kMacro.WireCamera(pkCamera, fScreenScale);
                kMacro.WireFrustum(pkCamera);
            }
        }


        // Draw bounds
        if (GetViewBounds())
        {
            const float fOver255 = 1.0f / 255.0f;

            unsigned int uiNumBounds = m_kBounds.GetSize();
            for (unsigned int i = 0; i < uiNumBounds; i++)
            {
                NiAVObject* pkObj = m_kBounds.GetAt(i);
                if (pkObj->GetAppCulled())
                    continue;

                const NiBound& kBound = pkObj->GetWorldBound();

                // Colorize the bounds according to the node pointer
                // This generates stable 'random' colors cheaply.
                unsigned int uc = (unsigned int)((size_t)pkObj & 0xFFFFFFFF);
                m_kAdapter.SetCurrentColor(NiColorA(
                    fOver255 * ((uc >> 10) & 255),
                    fOver255 * ((uc >> 2) & 255),
                    fOver255 * ((uc >> 18) & 255), 1.0f));

                kMacro.WireBounds(kBound);
            }
        }

        // Draw lights
        {
            unsigned int uiNumLights = m_kLights.GetSize();
            for (unsigned int i = 0; i < uiNumLights; i++)
            {
                NiLight* pkLight = m_kLights.GetAt(i);
                if (pkLight->GetAppCulled())
                    continue;

                float fScreenScale = m_fScreenMultiplier *
                    kMacro.GetScreenScaleFactor(pkLight);

                // NOTE: check NiSpotLight before NiPointLight
                if (NiIsKindOf(NiSpotLight, pkLight) && GetViewSpotLights())
                {
                    m_kAdapter.SetCurrentColor(kSpotLightColor);
                    kMacro.WireSpotLight(NiVerifyStaticCast(
                        NiSpotLight, pkLight), fScreenScale);
                }
                else if (NiIsKindOf(NiPointLight, pkLight) && GetViewPointLights())
                {
                    m_kAdapter.SetCurrentColor(kPointLightColor);
                    kMacro.WirePointLight(NiVerifyStaticCast(
                        NiPointLight, pkLight), fScreenScale);
                }
                else if (NiIsKindOf(NiDirectionalLight, pkLight)
                    && GetViewDirectionalLights())
                {
                    m_kAdapter.SetCurrentColor(kDirectionalLightColor);
                    kMacro.WireDirectionalLight(NiVerifyStaticCast(
                        NiDirectionalLight, pkLight), fScreenScale);
                }
            }
        }

        // Draw hierarchy
        if (GetViewBones() || GetViewNodes() || GetViewConnections())
        {
            unsigned int uiNumRoots = m_kSceneRoots.GetSize();
            for (unsigned int i = 0; i < uiNumRoots; i++)
            {
                NiAVObject* pkRoot = m_kSceneRoots.GetAt(i);
                if (pkRoot)
                {
                    kMacro.WireHierarchy(pkRoot, m_fScreenMultiplier, GetViewBones(),
                        GetViewNodes(), GetViewConnections());
                }
            }
        }
        // Let the NiImmediateModeMacro fall out of local scope...
    }

    // Resize adapter...
    unsigned int uiUsedVerts = m_kAdapter.GetNumVerticesUsed();
    unsigned int uiUsedIndices = m_kAdapter.GetNumIndicesUsed();
    if (uiUsedVerts > m_kAdapter.GetVertexBufferSize())
        m_kAdapter.SetNumMaxVertices(uiUsedVerts);
    if (uiUsedIndices > m_kAdapter.GetIndexBufferSize())
        m_kAdapter.SetNumMaxIndices(uiUsedIndices);

    m_uiNumObjectsDrawn = (uiUsedVerts + uiUsedIndices > 0 ? 1 : 0);
    m_fRenderTime = NiGetCurrentTimeInSec() - fBeginTime;
}

//--------------------------------------------------------------------------------------------------
inline void NiDebugVisualizationClick::ExpandFrustumNearFar(NiCamera* pkCamera,
    NiFrustum& kFrust, const NiPoint3& kCenter, float fRadius)
{
    float fDistToCenter = (kCenter - pkCamera->GetWorldTranslate()).Dot(
        pkCamera->GetWorldDirection());

    kFrust.m_fNear = NiMin(kFrust.m_fNear, fDistToCenter - fRadius);
    kFrust.m_fFar = NiMax(kFrust.m_fFar, fDistToCenter + fRadius);
}

//--------------------------------------------------------------------------------------------------
void NiDebugVisualizationClick::ExtendCameraNearAndFar(NiCamera* pkCamera) const
{
    NiFrustum kFrust = pkCamera->GetViewFrustum();

    // Fit cameras
    {
        unsigned int uiNumCameras = m_kCameras.GetSize();
        for (unsigned int i = 0; i < uiNumCameras; i++)
        {
            NiCamera* pkDebugCamera = m_kCameras.GetAt(i);
            if (pkDebugCamera->GetAppCulled() ||
                pkDebugCamera == pkCamera)
            {
                continue;
            }

            float fScreenScale = m_fScreenMultiplier *
                NiImmediateModeMacro::GetScreenScaleFactor(pkDebugCamera,
                &m_kAdapter);

            NiPoint3 kCenter = pkDebugCamera->GetWorldTranslate();
            ExpandFrustumNearFar(pkCamera, kFrust, kCenter, fScreenScale);

            // Fit debug camera frustum - just check the four far plane points
            NiPoint3 akPoint[4];
            const NiTransform& kTrans = pkDebugCamera->GetWorldTransform();
            const NiFrustum& kDebugFrust = pkDebugCamera->GetViewFrustum();
            if (kFrust.m_bOrtho)
            {
                NiPoint3 kViewTL = kTrans.m_Rotate * NiPoint3(0.0f,
                    kDebugFrust.m_fTop, kDebugFrust.m_fLeft);
                NiPoint3 kViewTR = kTrans.m_Rotate * NiPoint3(0.0f,
                    kDebugFrust.m_fTop, kDebugFrust.m_fRight);
                NiPoint3 kViewBL = kTrans.m_Rotate * NiPoint3(0.0f,
                    kDebugFrust.m_fBottom, kDebugFrust.m_fLeft);
                NiPoint3 kViewBR = kTrans.m_Rotate * NiPoint3(0.0f,
                    kDebugFrust.m_fBottom, kDebugFrust.m_fRight);

                NiPoint3 kFar = pkDebugCamera->GetWorldDirection() *
                    kDebugFrust.m_fFar;
                akPoint[0] = kTrans.m_Translate + kViewBL + kFar;
                akPoint[1] = kTrans.m_Translate + kViewTL + kFar;
                akPoint[2] = kTrans.m_Translate + kViewTR + kFar;
                akPoint[3] = kTrans.m_Translate + kViewBR + kFar;
            }
            else
            {
                NiPoint3 kViewTL = kTrans.m_Rotate * NiPoint3(1.0f,
                    kDebugFrust.m_fTop, kDebugFrust.m_fLeft);
                NiPoint3 kViewTR = kTrans.m_Rotate * NiPoint3(1.0f,
                    kDebugFrust.m_fTop, kDebugFrust.m_fRight);
                NiPoint3 kViewBL = kTrans.m_Rotate * NiPoint3(1.0f,
                    kDebugFrust.m_fBottom, kDebugFrust.m_fLeft);
                NiPoint3 kViewBR = kTrans.m_Rotate * NiPoint3(1.0f,
                    kDebugFrust.m_fBottom, kDebugFrust.m_fRight);

                akPoint[0] = kTrans.m_Translate + kViewBL * kDebugFrust.m_fFar;
                akPoint[1] = kTrans.m_Translate + kViewTL * kDebugFrust.m_fFar;
                akPoint[2] = kTrans.m_Translate + kViewTR * kDebugFrust.m_fFar;
                akPoint[3] = kTrans.m_Translate + kViewBR * kDebugFrust.m_fFar;
            }

            ExpandFrustumNearFar(pkCamera, kFrust, akPoint[0], 0.0f);
            ExpandFrustumNearFar(pkCamera, kFrust, akPoint[1], 0.0f);
            ExpandFrustumNearFar(pkCamera, kFrust, akPoint[2], 0.0f);
            ExpandFrustumNearFar(pkCamera, kFrust, akPoint[3], 0.0f);
        }
    }

    // Fit lights
    {
        unsigned int uiNumLights = m_kLights.GetSize();
        for (unsigned int i = 0; i < uiNumLights; i++)
        {
            NiLight* pkLight = m_kLights.GetAt(i);
            if (pkLight->GetAppCulled())
                continue;

            float fScreenScale = m_fScreenMultiplier *
                NiImmediateModeMacro::GetScreenScaleFactor(pkCamera,
                &m_kAdapter);

            NiPoint3 kCenter = pkLight->GetWorldTranslate();
            ExpandFrustumNearFar(pkCamera, kFrust, kCenter, fScreenScale);
        }
    }

    // Fit bounds
    {
        unsigned int uiNumBounds = m_kBounds.GetSize();
        for (unsigned int i = 0; i < uiNumBounds; i++)
        {
            NiAVObject* pkObj = m_kBounds.GetAt(i);
            if (pkObj->GetAppCulled())
                continue;

            const NiBound& kBound = pkObj->GetWorldBound();

            ExpandFrustumNearFar(pkCamera, kFrust, kBound.GetCenter(),
                kBound.GetRadius());
        }
    }

    pkCamera->SetViewFrustum(kFrust);
}

//--------------------------------------------------------------------------------------------------
