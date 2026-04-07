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

#include "NiPSSMConfiguration.h"
#include "Ni3DRenderView.h"

//------------------------------------------------------------------------------------------------
NiPSSMConfiguration::NiPSSMConfiguration() :
    NiRefObject(),
    m_uFlags(0),
    m_pkSliceViewports(NULL),
    m_pkCasterList(NULL),
    m_pfSliceDistances(NULL),
    m_pfPackedMatrices(NULL),
    m_pfPackedDistances(NULL),
    m_pfPackedViewports(NULL),
    m_pfPackedTransitionMatrix(NULL),
    m_fSliceLambda(0.0f),
    m_fSliceTransitionSize(0.0f),
    m_fSliceTransitionScale(0.0f),
    m_uiExtraSplitSpacing(0),
    m_ucNumSlices(0)
{
    // We must call this function explicitly, so that memory is allocated for
    // m_pfSliceDistances and m_pkSliceViewports as well as the packed shader
    // constant values
    SetNumSlices(4);
    SetSliceTransitionEnabled(GetSliceTransitionEnabled());

    SetSceneDependentFrustumsEnabled(false);
    SetSubTexelOffsetEnabled(true);
    SetBorderTestingEnabled(true);

    SetSliceLambda(0.5f);
    SetSliceTransitionSize(300.0f);
    SetSliceTransitionNoiseScale(0.05f);
    SetCameraDistanceScaleFactor(4.0f);
}

//------------------------------------------------------------------------------------------------
NiPSSMConfiguration::NiPSSMConfiguration(const NiPSSMConfiguration& kCopy) :
    NiRefObject(),
    m_uFlags(kCopy.m_uFlags),
    m_pkSliceViewports(NULL),
    m_pkCasterList(NULL),
    m_pfSliceDistances(NULL),
    m_pfPackedMatrices(NULL),
    m_pfPackedDistances(NULL),
    m_pfPackedViewports(NULL),
    m_pfPackedTransitionMatrix(NULL),
    m_fSliceLambda(kCopy.m_fSliceLambda),
    m_fSliceTransitionSize(kCopy.m_fSliceTransitionSize),
    m_fSliceTransitionScale(kCopy.m_fSliceTransitionScale),
    m_fCameraDistScaleFactor(kCopy.m_fCameraDistScaleFactor),
    m_uiExtraSplitSpacing(0),
    m_ucNumSlices(0)
{
    // We must call this function explicitly, so that memory is allocated for
    // m_pfSliceDistances and m_pkSliceViewports as well as the packed shader
    // constant values
    SetNumSlices(kCopy.GetNumSlices());
    SetSliceTransitionEnabled(kCopy.GetSliceTransitionEnabled());
}

//------------------------------------------------------------------------------------------------
NiPSSMConfiguration::~NiPSSMConfiguration()
{
    if (m_pfSliceDistances != NULL)
        NiFree(m_pfSliceDistances);

    if (m_pkSliceViewports != NULL)
        NiFree(m_pkSliceViewports);

    if (m_pfPackedMatrices != NULL)
        NiFree(m_pfPackedMatrices);

    if (m_pfPackedDistances != NULL)
        NiFree(m_pfPackedDistances);

    if (m_pfPackedViewports != NULL)
        NiFree(m_pfPackedViewports);

    if (m_pfPackedTransitionMatrix != NULL)
        NiFree(m_pfPackedTransitionMatrix);

    if (m_pkCasterList != NULL)
        NiDelete m_pkCasterList;
}

//------------------------------------------------------------------------------------------------
void NiPSSMConfiguration::SetNumSlices(NiUInt8 ucNumSlices)
{
    EE_ASSERT(ucNumSlices <= MAX_SLICES);

    m_pfSliceDistances = (float*)NiRealloc(m_pfSliceDistances,
        (ucNumSlices + 1) * sizeof(float));
    m_pkSliceViewports = (NiRect<float>*)NiRealloc(m_pkSliceViewports,
        ucNumSlices * sizeof(NiRect<float>));

    m_pfPackedMatrices = (float*)NiRealloc(m_pfPackedMatrices,
        ucNumSlices * 16 * sizeof(float));
    m_pfPackedViewports = (float*)NiRealloc(m_pfPackedViewports,
        ucNumSlices * 4 * sizeof(float));

    NiUInt32 uiNumFloat4 = ucNumSlices / 4;
    if (ucNumSlices % 4)
        ++uiNumFloat4;
    m_pfPackedDistances = (float*)NiRealloc(m_pfPackedDistances,
        uiNumFloat4 * 4 * sizeof(float));

    m_ucNumSlices = ucNumSlices;

    SetRebuildFrustums();
}

//------------------------------------------------------------------------------------------------
void NiPSSMConfiguration::SetSliceTransitionEnabled(bool bEnabled)
{
    SetBit(bEnabled, SLICETRANSITIONS);

    if (!m_pfPackedTransitionMatrix)
    {
        m_pfPackedTransitionMatrix = NiAlloc(float, 16);
        for (NiUInt32 ui = 0; ui < 16; ++ui)
            m_pfPackedTransitionMatrix[ui] = 0.0f;
    }
}

//------------------------------------------------------------------------------------------------
void NiPSSMConfiguration::UpdateShaderConstantData(
    NiTPointerList<NiRenderViewPtr>& kRenderViews)
{
    NiTListIterator kPos = kRenderViews.GetHeadPos();
    float* pfMatrixData = m_pfPackedMatrices;
    float* pfViewportData = m_pfPackedViewports;

    for (NiUInt32 uiSlice = 0; uiSlice < GetNumSlices(); ++uiSlice)
    {
        Ni3DRenderView* pkShadowView = (Ni3DRenderView*)(kRenderViews.GetNext(kPos).data());
        EE_ASSERT(pkShadowView);

        // Viewport
        const NiRect<float>& kViewport = GetPSSMSliceViewport((NiUInt8)uiSlice);
        {
            if (GetBorderTestingEnabled())
            {
                pfViewportData[0] = 1.0f / kViewport.GetWidth();
                pfViewportData[1] = 1.0f / kViewport.GetHeight();
                pfViewportData[2] = -kViewport.m_left;
                pfViewportData[3] = -(1.0f - kViewport.m_top);
            }
            else
            {
                pfViewportData[0] = 1.0f;
                pfViewportData[1] = 1.0f;
                pfViewportData[2] = 0.0f;
                pfViewportData[3] = 0.0f;
            }
        }

        // Split Matrix
        {
            float fScaleX = kViewport.GetWidth();
            float fScaleY = kViewport.GetHeight();

            float fOffsetX = -1.0f + (kViewport.m_right + kViewport.m_left);
            float fOffsetY = -1.0f + (kViewport.m_top + kViewport.m_bottom);

            const float* aafWorldToSM =
                pkShadowView->GetCamera()->GetWorldToCameraMatrix();

            // Row 1
            pfMatrixData[0] = aafWorldToSM[0] * fScaleX;
            pfMatrixData[1] = aafWorldToSM[1] * fScaleX;
            pfMatrixData[2] = aafWorldToSM[2] * fScaleX;
            pfMatrixData[3] = aafWorldToSM[3] * fScaleX + fOffsetX;
            // Row 2
            pfMatrixData[4] = aafWorldToSM[4] * fScaleY;
            pfMatrixData[5] = aafWorldToSM[5] * fScaleY;
            pfMatrixData[6] = aafWorldToSM[6] * fScaleY;
            pfMatrixData[7] = aafWorldToSM[7] * fScaleY + fOffsetY;
            // Row 3
            pfMatrixData[8] = aafWorldToSM[8];
            pfMatrixData[9] = aafWorldToSM[9];
            pfMatrixData[10] = aafWorldToSM[10];
            pfMatrixData[11] = aafWorldToSM[11];
            // Row 4
            pfMatrixData[12] = aafWorldToSM[12];
            pfMatrixData[13] = aafWorldToSM[13];
            pfMatrixData[14] = aafWorldToSM[14];
            pfMatrixData[15] = aafWorldToSM[15];
        }

        pfMatrixData += 16;
        pfViewportData += 4;
    }

    // Split distances - we only want each slices 'max' distance.
    {
        EE_ASSERT(GetNumSlices() > 0);

        // Don't copy the last slice, as we want to over-ride its value with MAX_INT
        NiUInt32 uiSlicesToCopy = GetNumSlices() - 1;
        NiMemcpy(m_pfPackedDistances, uiSlicesToCopy * sizeof(float), 
            m_pfSliceDistances + 1, uiSlicesToCopy * sizeof(float));

        NiUInt32 uiNumFloat4 = uiSlicesToCopy / 4;
        if (uiSlicesToCopy % 4)
            ++uiNumFloat4;
        NiUInt32 uiNumFloats = uiNumFloat4 * 4;
        const float fMaxDist = FLT_MAX;
        for (NiUInt32 ui = uiSlicesToCopy; ui < uiNumFloats; ++ui)
        {
            m_pfPackedDistances[ui] = fMaxDist;
        }
    }

    // Transition matrix
    if (GetSliceTransitionEnabled())
    {
        float fScale = GetSliceTransitionNoiseScale();

        const float* pfWorldToTC =
            GetSliceTransitionCamera()->GetWorldToCameraMatrix();

        float* pfData = m_pfPackedTransitionMatrix;

        // We can skip columns 2 and 3, since they will be discarded in the
        // the matrix multiplication in the pixel shader:
        // res = mul(WorldPos, TransitionMatrix).xy

        // Row 1
        pfData[0] = pfWorldToTC[0] * fScale;
        pfData[1] = pfWorldToTC[1] * fScale;
        // Row 2
        pfData[4] = pfWorldToTC[4] * fScale;
        pfData[5] = pfWorldToTC[5] * fScale;
        // Row 3
        pfData[8] = pfWorldToTC[8] * fScale;
        pfData[9] = pfWorldToTC[9] * fScale;
        // Row 4
        pfData[12] = pfWorldToTC[12] * fScale;
        pfData[13] = pfWorldToTC[13] * fScale;
    }
}

//------------------------------------------------------------------------------------------------
