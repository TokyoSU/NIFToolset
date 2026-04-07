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

#include "NiMainPCH.h"

#include "NiPSSMShadowClickGenerator.h"
#include "NiShadowGenerator.h"
#include "NiSpotLight.h"
#include "NiPointLight.h"
#include "NiDirectionalLight.h"
#include "NiNode.h"
#include "NiLODNode.h"
#include "NiRenderedCubeMap.h"
#include "NiShadowTechnique.h"

NiFixedString NiPSSMShadowClickGenerator::ms_kPSSMName;

//------------------------------------------------------------------------------------------------
NiImplementRTTI(NiPSSMShadowClickGenerator, NiDefaultShadowClickGenerator);
NiShadowGenerator* NiPSSMShadowClickGenerator::ms_pkActiveGenerator = NULL;

//------------------------------------------------------------------------------------------------
NiPSSMShadowClickGenerator::NiPSSMShadowClickGenerator(
    const NiPSSMConfiguration& kDefaultConfiguration):
    NiDefaultShadowClickGenerator(ms_RTTI.GetName()),
    m_kDefaultConfiguration(kDefaultConfiguration),
    m_pkNoiseTextureMap(NULL)
{
}

//------------------------------------------------------------------------------------------------
NiPSSMShadowClickGenerator::~NiPSSMShadowClickGenerator()
{
    NiDelete m_pkNoiseTextureMap;
}

//------------------------------------------------------------------------------------------------
NiPSSMConfiguration* NiPSSMShadowClickGenerator::GetConfiguration(
    const NiShadowGenerator* pkGenerator, bool bCreateFromDefault)
{
    // Is there a bound configuration?
    NiPSSMConfigurationPtr spConfiguration = NULL;
    m_kConfigurationMap.GetAt(pkGenerator, spConfiguration);

    // If no bound config found, use the default.
    if (!spConfiguration && bCreateFromDefault)
    {
        EE_ASSERT(pkGenerator);
        if (!pkGenerator)
            return NULL;

        spConfiguration = NiNew NiPSSMConfiguration(m_kDefaultConfiguration);

        BindConfiguration(pkGenerator, spConfiguration);
    }

    return spConfiguration;
}

//------------------------------------------------------------------------------------------------
void NiPSSMShadowClickGenerator::BindConfiguration(
    const NiShadowGenerator* pkGenerator, NiPSSMConfiguration* pkConfig)
{
    if (pkConfig != NULL)
    {
        EE_ASSERT(pkConfig->GetIsBoundToGenerator() == false);
        if (pkConfig->GetIsBoundToGenerator())
            return;

        NiPSSMConfigurationPtr spConfiguration = pkConfig;
        m_kConfigurationMap.SetAt(pkGenerator, spConfiguration);
    }
    else
    {
        NiPSSMConfigurationPtr spConfiguration;
        if (m_kConfigurationMap.GetAt(pkGenerator, spConfiguration))
        {
            m_kConfigurationMap.RemoveAt(pkGenerator);
            spConfiguration->SetIsBoundToGenerator(false);
        }
    }
}

//------------------------------------------------------------------------------------------------
bool NiPSSMShadowClickGenerator::HandleDirectionalLight(
    NiShadowGenerator* pkGenerator,
    NiDirectionalLight* pkDirLight,
    bool bRegenerateViews,
    bool bUpdateShadowMaps)
{
    bool bUsingPSSM = LightSupportsPSSM(pkGenerator, pkDirLight);

    NiPSSMConfiguration* pkConfiguration = GetConfiguration(pkGenerator);
    //bUsingPSSM &= (pkConfiguration->GetNumSlices() > 1);

    if (!bUsingPSSM)
    {
        if (pkConfiguration->GetRebuildFrustums())
        {
            // Need to reset the camera's viewport
            NiShadowMap* pkShadowMap = ObtainAndAssignShadowMap(
                pkGenerator,
                0,
                NULL,
                NiStandardMaterial::LIGHT_DIR);

            if  (!pkShadowMap)
                return false;

            EE_ASSERT(pkShadowMap->GetTextureType() == NiShadowMap::TT_SINGLE);

            // Get the render click for the shadow map.
            NiShadowRenderClick* pkShadowClick =
                pkShadowMap->GetRenderClick();
            EE_ASSERT(pkShadowClick);

            // Ensure at least one render view exists.
            EE_ASSERT(pkShadowClick->GetRenderViews().GetSize() > 0);

            NiTListIterator kIter = pkShadowClick->GetRenderViews().GetHeadPos();
            Ni3DRenderView* pkShadowView = NiDynamicCast(Ni3DRenderView,
                pkShadowClick->GetRenderViews().GetNext(kIter));
            EE_ASSERT(pkShadowView);

            NiCamera* pkShadowCamera = pkShadowView->GetCamera();
            EE_ASSERT(pkShadowCamera);

            NiRect<float> kViewport;
            kViewport.m_bottom = 0.0;
            kViewport.m_left = 0.0;
            kViewport.m_top = 1.0;
            kViewport.m_right = 1.0;

            pkShadowCamera->SetViewPort(kViewport);

            // Remove excess render views
            while (kIter)
            {
                pkShadowView = NiDynamicCast(Ni3DRenderView,
                    pkShadowClick->GetRenderViews().GetNext(kIter));
                pkShadowClick->RemoveRenderView(pkShadowView);
            }

            pkConfiguration->SetRebuildFrustums(false);
        }

        return NiDefaultShadowClickGenerator::HandleDirectionalLight(
            pkGenerator, pkDirLight, bRegenerateViews, bUpdateShadowMaps);
    }
    else
    {
        return HandlePSSMDirectionalLight(
            pkGenerator, pkDirLight, bRegenerateViews, bUpdateShadowMaps,
            pkConfiguration);
    }
}

//------------------------------------------------------------------------------------------------
bool NiPSSMShadowClickGenerator::HandlePSSMDirectionalLight(
    NiShadowGenerator* pkGenerator, NiDirectionalLight* pkDirLight,
    bool bRegenerateViews, bool bUpdateShadowMaps,
    NiPSSMConfiguration* pkConfiguration)
{
    EE_ASSERT(pkDirLight);

    // We always regenerate the views in PSSM.
    EE_UNUSED_ARG(bRegenerateViews);

    // SETTINGS
    bool bCasterDependent = pkConfiguration->GetSceneDependentFrustumsEnabled();
    bool bReceiverDependent =
        pkConfiguration->GetSceneDependentFrustumsEnabled();
    bool bSubTexelOffset = pkConfiguration->GetSubTexelOffsetEnabled();
    bool bSliceTransitions = pkConfiguration->GetSliceTransitionEnabled();

    // What technique are we using?
    NiShadowTechnique* pkTechnique = pkGenerator->GetShadowTechnique();

    // Get a shadow map for the generator.
    NiShadowMap* pkShadowMap = ObtainAndAssignShadowMap(
        pkGenerator,
        0,
        NULL,
        NiStandardMaterial::LIGHT_DIR);
    if  (!pkShadowMap)
        return false;
    EE_ASSERT(pkShadowMap->GetTextureType() == NiShadowMap::TT_SINGLE);

    // Get pointer to the active scene camera
    NiCamera* pkSceneCamera = NiShadowManager::GetSceneCamera();
    EE_ASSERT(pkSceneCamera);

    // Get the render click for the shadow map.
    NiShadowRenderClick* pkShadowClick = pkShadowMap->GetRenderClick();
    EE_ASSERT(pkShadowClick);

    NiFrustum kViewFrustum;
    if (pkConfiguration->GetCustomSceneCameraFrustumEnabled())
        kViewFrustum = pkConfiguration->GetCustomSceneCameraFrustum();
    else
        kViewFrustum = pkSceneCamera->GetViewFrustum();

    if (pkConfiguration->GetCustomSceneCameraFarPlaneEnabled())
        kViewFrustum.m_fFar = pkConfiguration->GetCustomSceneCameraFarPlane();

    if (pkConfiguration->GetRebuildFrustums())
    {
        // Do we need to rebuild frustum and view-port info?
        CalculateSliceDistances(pkGenerator, kViewFrustum.m_fNear,
            kViewFrustum.m_fFar);

        // Has the user requested additional spacing between splits on the
        // shadowmap texture?
        NiUInt32 uiGapSize = pkConfiguration->GetExtraSplitSpacing();

        // Since PCF uses a 4x4 blur kernel, we need to make a gap between
        // the PSSM splits so that it doesn't blur in the neighbors.
        uiGapSize += pkTechnique->GetGuardBandSize();

        CalculateSliceViewports(pkGenerator,
            pkShadowMap->GetTexture()->GetWidth(),
            pkShadowMap->GetTexture()->GetHeight(),
            uiGapSize);

        pkConfiguration->SetRebuildFrustums(false);
    }

    //if (bRegenerateViews)
    {
        PopulateCasterListCache(pkGenerator, pkConfiguration);
    }

    // Get the list of render views for this click:
    NiTPointerList<NiRenderViewPtr>& kRenderViews =
        pkShadowClick->GetRenderViews();
    EE_ASSERT(kRenderViews.GetSize() > 0);
    NiTListIterator kRenderViewPos = kRenderViews.GetHeadPos();

    // Cycle through each render view (PSSM split) and correctly position the
    // shadow camera
    NiUInt8 ucNumSlices = pkConfiguration->GetNumSlices();
    for (NiUInt8 ucSplitIndex = 0; ucSplitIndex < ucNumSlices; ++ucSplitIndex)
    {
        // Get a pointer to the camera for this split
        Ni3DRenderView* pkShadowView = NULL;
        NiCamera* pkShadowCamera = NULL;
        if (kRenderViewPos)
        {
            // Get the render view from the render click.
            pkShadowView = NiDynamicCast(Ni3DRenderView,
                kRenderViews.GetNext(kRenderViewPos));
            pkShadowCamera = pkShadowView->GetCamera();
        }
        else
        {
            // Create a render view for this view split:
            pkShadowCamera = NiNew NiCamera;
            pkShadowView = NiNew Ni3DRenderView(
                pkShadowCamera, NiShadowManager::GetCullingProcess());

            kRenderViews.AddTail(pkShadowView);
        }
        EE_ASSERT(pkShadowView);
        EE_ASSERT(pkShadowCamera);

        NiFrustum kSliceFrustum;
        NiFrustum kShadowFrustum;
        NiPoint3 kShadowCameraPos;
        {
            // Calculate the view split frustum
            kSliceFrustum = kViewFrustum;
            kSliceFrustum.m_fNear = pkConfiguration->GetPSSMSliceMinDistance(
                ucSplitIndex);
            kSliceFrustum.m_fFar = pkConfiguration->GetPSSMSliceMaxDistance(
                ucSplitIndex);

            // Create a bound containing all of the split frustums points.
            // This could potentially be precomputed. We cannot use the provided
            // function in NiBound, since it is crucial that the bound is the
            // same size no matter what orientation the frustum is. The provided
            // function does not guarantee this.
            NiBound kBound;
            {
                // TODO lweaver 3.11.08 could this use a pre-computed bound,
                // which is then transformed? Issue: center of the bound does
                // not correlate to the pivot point of the frustum.
                NiPoint3 akPoints[8];
                CalculateFrustumPoints(kSliceFrustum,
                    pkSceneCamera->GetWorldTransform(), akPoints, 8);

                NiPoint3 kCenter = NiPoint3::ZERO;
                for (NiUInt32 ui = 0; ui < 8; ++ui)
                    kCenter += akPoints[ui];
                kCenter *= 0.125f;

                float fRadiusSqr = 0.0f;
                float fLenSqr;
                for (NiUInt32 ui = 0; ui < 8; ++ui)
                {
                    fLenSqr = (kCenter - akPoints[ui]).SqrLength();
                    if (fLenSqr > fRadiusSqr)
                        fRadiusSqr = fLenSqr;
                }

                kBound.SetCenterAndRadius(kCenter, NiSqrt(fRadiusSqr));
            }

            // Create the lights frustum to tightly fit the slice frustum bound
            float fFrustumWidth = kBound.GetRadius();
            float fCameraDistFromBoundCenter =
                fFrustumWidth * pkConfiguration->GetCameraDistanceScaleFactor();
            kShadowCameraPos = kBound.GetCenter() -
                (pkDirLight->GetWorldDirection() * fCameraDistFromBoundCenter);

            // TODO mbailey Remove magic number.
            float fNear = 0.1f;
            float fFar = fCameraDistFromBoundCenter + fFrustumWidth;

            kShadowFrustum = NiFrustum(-fFrustumWidth, fFrustumWidth,
                fFrustumWidth, -fFrustumWidth, fNear, fFar, true);
        }

        // Affected geometry. We cannot cache this between frames, since it is
        // culled to the shadowmap frustums, which change. Additionally, there will
        // be uniquely visible geometry in different splits.
        NiTPointerList<NiAVObjectPtr>& kCasterList = pkShadowView->GetScenes();

        NiAVObjectRawList kReceiverGeometry;
        {
            // Assign the current shadow camera frustum
            pkShadowCamera->SetViewFrustum(kShadowFrustum);

            // Calculate the view/projection transform
            pkShadowCamera->SetTranslate(kShadowCameraPos);
            pkShadowCamera->SetRotate(pkDirLight->GetWorldRotate());
            pkShadowCamera->SetScale(1.0f);

            // Assign this view it's appropriate atlas view port:
            pkShadowCamera->SetViewPort(pkConfiguration->GetPSSMSliceViewport(
                ucSplitIndex));
            pkShadowCamera->Update(0.0f);
        }

        // Clear and repopulate the render view's scene list if the current render
        // view's are flagged as dirty.
        if (bRegenerateViews)
        {
            kCasterList.RemoveAll();
            NiShadowVisitor::RawList* pkObjectList = pkConfiguration->GetCasterList();

            NiShadowVisitor::RawListIterator kIter = pkObjectList->GetFirstPos();
            while (kIter)
            {
                kCasterList.AddTail(pkObjectList->GetNext(kIter));
            }
        }

        // If scene dependent, we may wish to further shrink the lights frustum
        if (bCasterDependent)
        {
            // We will always need the casters to add them to the casters scene.
            // Additionally, the list must be recalculated each frame so that
            // it can be correctly culled.
            const NiVisibleArray& kVisArray =
                GetVisibleAffectedGeometryList(pkShadowView);

            NiBound kCasterBound;
            bool bFoundGeometry = FindGeometryBound(kVisArray, kCasterBound);

            // Is the radius smaller than our current frustum width? If not,
            // there is no benefit to using it.
            if (bFoundGeometry &&
                kCasterBound.GetRadius() > 0 &&
                kCasterBound.GetRadius() < kShadowFrustum.m_fRight)
            {
                float fFrustumWidth = kCasterBound.GetRadius();
                float fCameraDistFromCentroid = fFrustumWidth * 2.0f;
                kShadowCameraPos = kCasterBound.GetCenter() -
                    (pkDirLight->GetWorldDirection() * fCameraDistFromCentroid);

                kShadowFrustum.m_fLeft = -fFrustumWidth;
                kShadowFrustum.m_fRight = fFrustumWidth;
                kShadowFrustum.m_fTop = fFrustumWidth;
                kShadowFrustum.m_fBottom = -fFrustumWidth;

                // Far = fCameraDistFromCentroid + fFrustumWidth
                kShadowFrustum.m_fFar = fFrustumWidth * 3;
            }
        }

        // Re-Assign the shadow camera frustum with the modified scene dependent
        // values
        if (bCasterDependent || bReceiverDependent)
        {
            pkShadowCamera->SetViewFrustum(kShadowFrustum);

            // Calculate the view/projection transform
            pkShadowCamera->SetTranslate(kShadowCameraPos);
            pkShadowCamera->SetRotate(pkDirLight->GetWorldRotate());
            pkShadowCamera->SetScale(1.0f);

            // Assign this view it's appropriate atlas view port:
            pkShadowCamera->SetViewPort(pkConfiguration->GetPSSMSliceViewport(
                ucSplitIndex));
            pkShadowCamera->Update(0.0f);
        }

        if (bSliceTransitions)
        {
            NiCamera* pkSliceTransitionCamera =
                pkConfiguration->GetSliceTransitionCamera();

            pkSliceTransitionCamera->SetRotate(pkDirLight->GetWorldRotate());
            pkSliceTransitionCamera->SetScale(1.0f);
            pkSliceTransitionCamera->Update(0.0f);

            if (!m_pkNoiseTextureMap)
            {
                NiTexture* pkNoiseTexture = NiNoiseTexture::Create(
                    NiNoiseTexture::NT_RAND, 128, 0);
                EE_ASSERT(pkNoiseTexture);

                m_pkNoiseTextureMap = NiNew NiTexturingProperty::Map;
                m_pkNoiseTextureMap->SetTexture(pkNoiseTexture);
                m_pkNoiseTextureMap->SetClampMode(
                    NiTexturingProperty::WRAP_S_WRAP_T);
                m_pkNoiseTextureMap->SetFilterMode(
                    NiTexturingProperty::FILTER_NEAREST);
            }
        }

        if (bSubTexelOffset)
        {
            NiTransform kQuantizedTransform;

            // Quantize the shadow camera's transform according to the relative
            // size of the pixels on the corresponding shadow map.
            CalculateQuantizedTransform(pkShadowCamera, pkShadowMap,
                kQuantizedTransform);

            pkShadowCamera->SetLocalFromWorldTransform(kQuantizedTransform);
            pkShadowCamera->Update(0.0f);
        }
    }

    // Remove any excess render views that may be left over if we reduced the
    // slice count since the last frame.
    while (kRenderViewPos)
    {
        kRenderViews.RemovePos(kRenderViewPos);
    }

    // Assign the appropriate material to be used when drawing objects in these
    // views
    pkShadowClick->SetProcessor(NiShadowManager::GetMaterialSwapProcessor(
        NiStandardMaterial::LIGHT_DIR));

    if (bUpdateShadowMaps)
    {
        AddRenderClick(pkShadowClick);

        pkTechnique->PrepareShadowMap(pkShadowMap);
        pkTechnique->AppendPostProcessRenderClicks(pkShadowMap, this);
    }

    // Update the shader constant cache
    pkConfiguration->UpdateShaderConstantData(kRenderViews);

    pkGenerator->SetRenderViewsDirty(false);
    return true;
}

//------------------------------------------------------------------------------------------------
void NiPSSMShadowClickGenerator::CalculateQuantizedTransform(
    const NiCamera* pkShadowCamera, const NiShadowMap* pkShadowMap,
    NiTransform& kQuantizedTransform)
{
    /*
    Prevent shadow shimmer caused by sub-texel camera movement. This is
    only necessary when scene independent frustums are used.

    1. Project an arbitrary (constant) world space point onto the
    shadowmap.
    2. Round the projected 2D coordinate to the nearest pixel
    3. Project the rounded camera space coordinate back into world
    space
    4. Find the transformation difference between the original
    arbitrary point and the re-projected point
    5. Add that difference on to the position of the shadowmap
    camera
    */

    // TODO lweaver 3.11.08 this is still quite an expensive routine,
    // since it is impossible to simply add a post-projection transform
    // to a camera. We current need to work in world space, which is
    // slow and can potentially introduce floating point errors.

    //
    // Define some variables
    //

    // Arbitrary Translation (reference point)
    const NiPoint3& kRef = NiPoint3::ZERO;

    // Camera info
    const NiRect<float>& kViewPort = pkShadowCamera->GetViewPort();
    const NiFrustum& kFrustum = pkShadowCamera->GetViewFrustum();

    // Calculate the perpendicular distance of the arbitrary point from the
    // shadowmap plane
    NiPlane kPlane(pkShadowCamera->GetWorldDirection(),
        pkShadowCamera->GetWorldLocation());

    float fProjectionLength = kPlane.Distance(kRef);

    // Dimension of the shadowmap slice, in pixels
    float fSliceWidth = pkShadowMap->GetTexture()->GetWidth() *
        kViewPort.GetWidth();
    float fSliceHeight = pkShadowMap->GetTexture()->GetHeight() *
        kViewPort.GetHeight();

    // Project the reference point to screen space
    float x, y;
    pkShadowCamera->WorldPtToScreenPt(kRef, x, y);
    x = (x - kViewPort.m_left) / kViewPort.GetWidth();
    y = (y - kViewPort.m_bottom) / kViewPort.GetHeight();

    // Round the projected coordinates
    NiPoint3 kRounded(
        NiFloor(x * fSliceWidth) / fSliceWidth,
        NiFloor(y * fSliceHeight) / fSliceHeight,
        0.0f);

    // Re-project back to world space
    NiPoint3 kOrigin, kDir;
    pkShadowCamera->ViewPointToRay(
        kRounded.x * (kFrustum.m_fRight - kFrustum.m_fLeft) + kFrustum.m_fLeft,
        kRounded.y * (kFrustum.m_fTop - kFrustum.m_fBottom) + kFrustum.m_fBottom,
        kOrigin, kDir);
    NiPoint3 kProj = kDir * fProjectionLength + kOrigin;

    // Find the difference between the reference point and projected
    // point
    NiPoint3 kDiff = kProj - kRef;
    NiPoint3 kFinal = pkShadowCamera->GetWorldTranslate() - kDiff;

    // Add the difference to the camera transform
    kQuantizedTransform = pkShadowCamera->GetWorldTransform();
    kQuantizedTransform.m_Translate = kFinal;
}

//------------------------------------------------------------------------------------------------
bool NiPSSMShadowClickGenerator::FindGeometryBound(
    const NiVisibleArray& kVisibleArray,
    NiBound& kVisibleGeometryNiBound)
{
    if (kVisibleArray.GetCount() == 0)
        return false;

    // Start the bound with the first entry
    NiAVObject* pkRenderObject = (NiAVObject*)&kVisibleArray.GetAt(0);
    kVisibleGeometryNiBound = pkRenderObject->GetWorldBound();

    // For each slice, find casters that are in the lights frustum
    for (NiUInt32 ui = 1; ui < kVisibleArray.GetCount(); ui++)
    {
        pkRenderObject = (NiAVObject*)&kVisibleArray.GetAt(ui);

        kVisibleGeometryNiBound.Merge(&pkRenderObject->GetWorldBound());
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void NiPSSMShadowClickGenerator::CalculateFrustumPoints(
    const NiFrustum& kSliceFrustum, const NiTransform& kTransform,
    NiPoint3* akPoints, NiUInt32 uiPointCount) const
{
    EE_ASSERT(uiPointCount >= 8);
    EE_UNUSED_ARG(uiPointCount);

    NiPoint3 kCameraDir;
    kTransform.m_Rotate.GetCol(0, kCameraDir);

    // Note that scale is not applied anywhere in this code since the renderers do
    // not consider scale when constructing view matrices.
    if (kSliceFrustum.m_bOrtho)
    {
        // World-space viewport translation vectors.
        NiPoint3 kViewTL = kTransform.m_Rotate * NiPoint3(0.0f,
            kSliceFrustum.m_fTop, kSliceFrustum.m_fLeft);

        NiPoint3 kViewTR = kTransform.m_Rotate * NiPoint3(0.0f,
            kSliceFrustum.m_fTop, kSliceFrustum.m_fRight);

        NiPoint3 kViewBL = kTransform.m_Rotate * NiPoint3(0.0f,
            kSliceFrustum.m_fBottom, kSliceFrustum.m_fLeft);

        NiPoint3 kViewBR = kTransform.m_Rotate * NiPoint3(0.0f,
            kSliceFrustum.m_fBottom, kSliceFrustum.m_fRight);

        akPoints[0] = kTransform.m_Translate + kViewBL +
            kCameraDir * kSliceFrustum.m_fNear;
        akPoints[1] = kTransform.m_Translate + kViewTL +
            kCameraDir * kSliceFrustum.m_fNear;
        akPoints[2] = kTransform.m_Translate + kViewTR +
            kCameraDir * kSliceFrustum.m_fNear;
        akPoints[3] = kTransform.m_Translate + kViewBR +
            kCameraDir * kSliceFrustum.m_fNear;

        akPoints[4] = kTransform.m_Translate + kViewBL +
            kCameraDir * kSliceFrustum.m_fFar;
        akPoints[5] = kTransform.m_Translate + kViewTL +
            kCameraDir * kSliceFrustum.m_fFar;
        akPoints[6] = kTransform.m_Translate + kViewTR +
            kCameraDir * kSliceFrustum.m_fFar;
        akPoints[7] = kTransform.m_Translate + kViewBR +
            kCameraDir * kSliceFrustum.m_fFar;
    }
    else
    {
        // World-space viewport direction vectors.
        NiPoint3 kViewTL = kTransform.m_Rotate * NiPoint3(1.0f,
            kSliceFrustum.m_fTop, kSliceFrustum.m_fLeft);

        NiPoint3 kViewTR = kTransform.m_Rotate * NiPoint3(1.0f,
            kSliceFrustum.m_fTop, kSliceFrustum.m_fRight);

        NiPoint3 kViewBL = kTransform.m_Rotate * NiPoint3(1.0f,
            kSliceFrustum.m_fBottom, kSliceFrustum.m_fLeft);

        NiPoint3 kViewBR = kTransform.m_Rotate * NiPoint3(1.0f,
            kSliceFrustum.m_fBottom, kSliceFrustum.m_fRight);

        // Viewport direction vectors are in camera space. Multiplying by the distance to
        // the view planes has the effect of projecting them into a frustum in view space.
        akPoints[0] = kTransform.m_Translate + kViewBL * kSliceFrustum.m_fNear;
        akPoints[1] = kTransform.m_Translate + kViewTL * kSliceFrustum.m_fNear;
        akPoints[2] = kTransform.m_Translate + kViewTR * kSliceFrustum.m_fNear;
        akPoints[3] = kTransform.m_Translate + kViewBR * kSliceFrustum.m_fNear;

        akPoints[4] = kTransform.m_Translate + kViewBL * kSliceFrustum.m_fFar;
        akPoints[5] = kTransform.m_Translate + kViewTL * kSliceFrustum.m_fFar;
        akPoints[6] = kTransform.m_Translate + kViewTR * kSliceFrustum.m_fFar;
        akPoints[7] = kTransform.m_Translate + kViewBR * kSliceFrustum.m_fFar;
    }
}

//------------------------------------------------------------------------------------------------
void NiPSSMShadowClickGenerator::PopulateCasterListCache(
    NiShadowGenerator* pkGenerator, NiPSSMConfiguration* pkPSSMConfig)
{
    NiShadowVisitor::RawList* pkCasterList = pkPSSMConfig->GetCasterList();
    pkGenerator->GetCasterList(*pkCasterList);
}

//------------------------------------------------------------------------------------------------
const NiVisibleArray& NiPSSMShadowClickGenerator::GetVisibleAffectedGeometryList(
    Ni3DRenderView* pkRenderView)
{
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    NiCullingProcess* pkShadowCullingProcess =
        pkRenderView->GetCullingProcess();

    pkShadowCullingProcess->SetLODCamera(
        NiShadowManager::GetSceneCamera());

    // Normally we would disable executing mesh modfiers for this culling
    // pass. However since we are performing culling via the render view
    // the results will be cached in the render view allowing us to avoid
    // needing to perform another culling pass when we actually render
    // the geometry. Since we won't be performing another culling pass
    // while rendering we must execute mesh modifiers in this culling pass.
    const NiVisibleArray& kVisibleArray =
        pkRenderView->GetPVGeometry(pkRenderer->GetFrameID());

    return kVisibleArray;
}

//------------------------------------------------------------------------------------------------
void NiPSSMShadowClickGenerator::CalculateSliceDistances(
    NiShadowGenerator* pkGenerator, float fNearDistance, float fFarDistance)
{
    NiPSSMConfiguration* pkConfiguration = GetConfiguration(pkGenerator);
    // Ensure we have valid input
    EE_ASSERT(fNearDistance > 0.0f);
    EE_ASSERT(fFarDistance > fNearDistance);
    EE_ASSERT(pkConfiguration);

    NiUInt8 ucNumSlices = pkConfiguration->GetNumSlices();
    EE_ASSERT(ucNumSlices > 0 && ucNumSlices <= NiPSSMConfiguration::MAX_SLICES);

    // Calculate slice distances
    const float fFarDNear = fFarDistance / fNearDistance;
    const float fFarMNear = fFarDistance - fNearDistance;
    const float fNumSlicesInv = 1.0f / float(ucNumSlices);
    float fLamba = pkConfiguration->GetSliceLambda();
    for (NiUInt8 ucIndex = 1; ucIndex < ucNumSlices; ++ucIndex)
    {
        float fPercentDone = float(ucIndex) * fNumSlicesInv;
        float fLog = fNearDistance * NiPow(fFarDNear, fPercentDone);
        float fUniform = fFarMNear * fPercentDone + fNearDistance;

        pkConfiguration->SetPSSMSliceMinDistance(
            NiLerp(fLamba, fUniform, fLog), ucIndex);
    }

    // Set the boundary values:
    pkConfiguration->SetPSSMSliceMinDistance(fNearDistance, 0);
    pkConfiguration->SetPSSMSliceMaxDistance(fFarDistance, ucNumSlices - 1);
}

//------------------------------------------------------------------------------------------------
void NiPSSMShadowClickGenerator::CalculateSliceViewports(
    NiShadowGenerator* pkGenerator, NiUInt32 uiTextureWidth,
    NiUInt32 uiTextureHeight, NiUInt32 uiGapSize)
{
    NiPSSMConfiguration* pkConfig = GetConfiguration(pkGenerator);


    // We will work with floating points to avoid having to cast back and forth.
    float fNumSlices = float(pkConfig->GetNumSlices());

    // Work out the number of rows we have. Do this by comparing ourselves
    // the next highest 'square' number of slices.
    float fSqrt = NiSqrt(fNumSlices);
    float fCeil = -1.0f * NiFloor(-1.0f * fSqrt);
    float fRound = NiFloor(fSqrt + 0.5f);
    float fNumRows = NiMin(fRound, fCeil);

    // Height of each row, in viewport space
    float fRowHeight = 1.0f / fNumRows;

    // Number of cells, in total, left to add. This is used to work out how
    // many cells per row.
    float fRemainingCells = fNumSlices;

    // Start at the worst detail split, to ensure the 'closer' splits
    // get the cells with the most pixels
    NiUInt8 ucSplit = pkConfig->GetNumSlices();

    // We want to add a 1 pixel buffer between all cells
    float fCellHeight = fRowHeight - (float)uiGapSize / (float)uiTextureHeight;

    for (float fRow = 0; fRow < fNumRows; ++fRow)
    {
        // Ceiling operation
        float fNumCols = fRemainingCells / (fNumRows - fRow);
        fNumCols = -1.0f * NiFloor(-1.0f * fNumCols);

        if (fRow == fNumRows - 1.0f)
            fCellHeight = fRowHeight;

        // Width of each column on this row, in view-port space
        float fColWidth = 1.0f / fNumCols;
        float fCellWidth = fColWidth - (float)uiGapSize / (float)uiTextureWidth;
        for (float fCol = 0; fCol < fNumCols; ++fCol)
        {
            EE_ASSERT(ucSplit != 0);
            ucSplit--;

            NiRect<float>& kSliceViewport =
                pkConfig->GetPSSMSliceViewport(ucSplit);

            if (fCol == fNumCols - 1.0f)
                fCellWidth = fColWidth;

            kSliceViewport.m_left = fCol * fColWidth;
            kSliceViewport.m_right = kSliceViewport.m_left + fCellWidth;
            kSliceViewport.m_bottom = fRow * fRowHeight;
            kSliceViewport.m_top = kSliceViewport.m_bottom + fCellHeight;
        }


        fRemainingCells -= fNumCols;
    }

    // Make sure nothing went wrong with the maths; this should never be hit!
    EE_ASSERT(ucSplit == 0);
}

//------------------------------------------------------------------------------------------------
bool NiPSSMShadowClickGenerator::PrepareDirectionalLightShadowGenerator(
    NiShadowGenerator* pkGenerator)
{
    NiDefaultShadowClickGenerator::PrepareDirectionalLightShadowGenerator(
        pkGenerator);

    // Override callbacks
    pkGenerator->SetSMConstantRetrievalCallbackFunc(
        NiPSSMShadowClickGenerator::PSSMGetShaderConstantCallback);

    pkGenerator->SetTextureMapRetrievalCallbackFunc(
        NiPSSMShadowClickGenerator::PSSMGetTextureMapCallback);

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiPSSMShadowClickGenerator::PSSMGetShaderConstantCallback(
    void* pvData,
    unsigned int uiDataSize,
    NiShadowGenerator* pGenerator,
    NiRenderObject* pGeometry,
    unsigned int uiSMIndex,
    NiShaderConstantMap::ObjectMappings eMapping,
    const NiPropertyState* pState,
    const NiDynamicEffectState* pEffects,
    const NiTransform& world,
    const NiBound& worldBound,
    unsigned int uiPass)
{
    EE_ASSERT(pGenerator);

    NiPSSMShadowClickGenerator* pClickGenerator = NiDynamicCast(
        NiPSSMShadowClickGenerator,
        NiShadowManager::GetShadowClickGenerator(ms_kPSSMName));

    if (!pClickGenerator)
    {
        return NiShadowGenerator::DefaultGetShaderConstantCallback(
            pvData,
            uiDataSize,
            pGenerator,
            pGeometry,
            uiSMIndex,
            eMapping,
            pState,
            pEffects,
            world,
            worldBound,
            uiPass);
    }

    NiPSSMConfiguration* pConfiguration = pClickGenerator->GetConfiguration(pGenerator);

    switch (eMapping)
    {
    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_ATLASVIEWPORTS:
        {
            NiMemcpy(
                pvData,
                uiDataSize,
                pConfiguration->GetPackedSplitViewports(),
                pConfiguration->GetNumSlices() * sizeof(float) * 4);
        }
        return true;

    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_SPLITDISTANCES:
        {
            NiMemcpy(pvData, uiDataSize, pConfiguration->GetPackedSplitDistances(), uiDataSize);
        }
        return true;

    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_SPLITMATRICES:
        {
            NiMemcpy(
                pvData,
                uiDataSize,
                pConfiguration->GetPackedSplitMatrices(),
                pConfiguration->GetNumSlices() * sizeof(float) * 16);
        }
        return true;

    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_TRANSITIONMATRIX:
        {
            NiMemcpy(
                pvData,
                uiDataSize,
                pConfiguration->GetPackedTransitionMatrix(),
                sizeof(float) * 16);
        }
        return true;

    case NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_TRANSITIONSIZE:
        {
            float* pfData = (float*)pvData;

            pfData[0] = pConfiguration->GetSliceTransitionSize();
            pfData[1] = 0.0f;
            pfData[2] = 0.0f;
            pfData[3] = 0.0f;
        }
        return true;

    default:
        return NiShadowGenerator::DefaultGetShaderConstantCallback(
            pvData,
            uiDataSize,
            pGenerator,
            pGeometry,
            uiSMIndex,
            eMapping,
            pState,
            pEffects,
            world,
            worldBound,
            uiPass);
    }
}

//------------------------------------------------------------------------------------------------
NiTexturingProperty::Map* NiPSSMShadowClickGenerator::PSSMGetTextureMapCallback(
    NiShadowGenerator* pGenerator,
    NiStandardMaterial::TextureMap eMapType,
    NiRenderObject* pGeometry,
    unsigned int uiTMIndex)
{
    EE_UNUSED_ARG(pGenerator);
    EE_UNUSED_ARG(pGeometry);
    EE_UNUSED_ARG(uiTMIndex);

    EE_ASSERT(pGenerator);

    NiPSSMShadowClickGenerator* pClickGenerator = NiDynamicCast(
        NiPSSMShadowClickGenerator, NiShadowManager::GetShadowClickGenerator(
        "NiPSSMShadowClickGenerator"));

    if (eMapType == NiStandardMaterial::MAP_SHADOWMAP_NOISE_GREYSCALE)
    {
        return pClickGenerator->GetNoiseTextureMap();
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
void NiPSSMShadowClickGenerator::_SDMInit()
{
    ms_kPSSMName = "NiPSSMShadowClickGenerator";
}

//------------------------------------------------------------------------------------------------
void NiPSSMShadowClickGenerator::_SDMShutdown()
{
    ms_kPSSMName = NULL;
}

//------------------------------------------------------------------------------------------------
