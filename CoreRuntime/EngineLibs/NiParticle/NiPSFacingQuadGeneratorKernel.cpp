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
#include <NiParticlePCH.h>

#include "NiPSFacingQuadGeneratorKernel.h"
#include <NiUniversalTypes.h>
#include <NiMath.h>

//--------------------------------------------------------------------------------------------------
NiSPBeginKernelImpl(NiPSFacingQuadGeneratorKernel)
{
    // Get input streams.
    const NiPSFacingQuadGeneratorKernelStruct* pkIStruct =
        kWorkload.GetInput<NiPSFacingQuadGeneratorKernelStruct>(0);
    const NiPoint3* pkIPositions = kWorkload.GetInput<NiPoint3>(1);
    const float* pfIRadii = kWorkload.GetInput<float>(2);
    const float* pfISizes = kWorkload.GetInput<float>(3);
    const NiRGBA* pkIColors = kWorkload.GetInput<NiRGBA>(4);
    const float* pfIRotAngles = kWorkload.GetInput<float>(5);
    const NiUInt32* pRemap = kWorkload.GetInput<NiUInt32>(6);

    // Get output streams.
    NiPoint3* pkOVertices = kWorkload.GetOutput<NiPoint3>(0);
    NiRGBA* pkOColors = kWorkload.GetOutput<NiRGBA>(1);

    // Get block count.
    NiUInt32 uiBlockCount = kWorkload.GetBlockCount();

    // Generate quads.
    if (pfIRotAngles)
    {
        if (pkIColors)
        {
            for (NiUInt32 ui = 0; ui < uiBlockCount; ++ui)
            {
                NiUInt32 index = (pRemap != NULL) ? pRemap[ui] : ui;
                float fSize = pfIRadii[index] * pfISizes[index];

                // Compute the rotated top left and top right offset vectors.
                float fSinA, fCosA;
                NiSinCos(pfIRotAngles[index], fSinA, fCosA);
                float fC1 = fSize * (fCosA + fSinA);
                float fC2 = fSize * (fCosA - fSinA);

                NiPoint3 kV0 = fC1 * pkIStruct->m_kCamRight +
                    fC2 * pkIStruct->m_kCamUp;
                NiPoint3 kV1 = -fC2 * pkIStruct->m_kCamRight +
                    fC1 * pkIStruct->m_kCamUp;

                const NiPoint3& kPosition = pkIPositions[index];

                *pkOVertices = kPosition + kV1;
                *pkOColors = pkIColors[index];
                ++pkOVertices;
                ++pkOColors;

                *pkOVertices = kPosition - kV0;
                *pkOColors = pkIColors[index];
                ++pkOVertices;
                ++pkOColors;

                *pkOVertices = kPosition - kV1;
                *pkOColors = pkIColors[index];
                ++pkOVertices;
                ++pkOColors;

                *pkOVertices = kPosition + kV0;
                *pkOColors = pkIColors[index];
                ++pkOVertices;
                ++pkOColors;
            }
        }
        else
        {
            for (NiUInt32 ui = 0; ui < uiBlockCount; ++ui)
            {
                NiUInt32 index = (pRemap != NULL) ? pRemap[ui] : ui;
                float fSize = pfIRadii[index] * pfISizes[index];

                // Compute the rotated top left and top right offset vectors.
                float fSinA, fCosA;
                NiSinCos(pfIRotAngles[index], fSinA, fCosA);
                float fC1 = fSize * (fCosA + fSinA);
                float fC2 = fSize * (fCosA - fSinA);

                NiPoint3 kV0 = fC1 * pkIStruct->m_kCamRight +
                    fC2 * pkIStruct->m_kCamUp;
                NiPoint3 kV1 = -fC2 * pkIStruct->m_kCamRight +
                    fC1 * pkIStruct->m_kCamUp;

                const NiPoint3& kPosition = pkIPositions[index];

                *pkOVertices = kPosition + kV1;
                ++pkOVertices;

                *pkOVertices = kPosition - kV0;
                ++pkOVertices;

                *pkOVertices = kPosition - kV1;
                ++pkOVertices;

                *pkOVertices = kPosition + kV0;
                ++pkOVertices;
            }
        }
    }
    else
    {
        // Compute the top left corner offset vector.
        NiPoint3 kTopLeft = pkIStruct->m_kCamUp - pkIStruct->m_kCamRight;

        // Compute the top right corner offset vector.
        NiPoint3 kTopRight = pkIStruct->m_kCamUp + pkIStruct->m_kCamRight;

        if (pkIColors)
        {
            for (NiUInt32 ui = 0; ui < uiBlockCount; ++ui)
            {
                NiUInt32 index = (pRemap != NULL) ? pRemap[ui] : ui;
                float fSize = pfIRadii[index] * pfISizes[index];

                NiPoint3 kV0 = fSize * kTopRight;
                NiPoint3 kV1 = fSize * kTopLeft;

                const NiPoint3& kPosition = pkIPositions[index];

                NiPoint3 kVertex;

                *pkOVertices = kPosition + kV1;
                *pkOColors = pkIColors[index];
                ++pkOVertices;
                ++pkOColors;

                *pkOVertices = kPosition - kV0;
                *pkOColors = pkIColors[index];
                ++pkOVertices;
                ++pkOColors;

                *pkOVertices = kPosition - kV1;
                *pkOColors = pkIColors[index];
                ++pkOVertices;
                ++pkOColors;

                *pkOVertices = kPosition + kV0;
                *pkOColors = pkIColors[index];
                ++pkOVertices;
                ++pkOColors;
            }
        }
        else
        {
            for (NiUInt32 ui = 0; ui < uiBlockCount; ++ui)
            {
                NiUInt32 index = (pRemap != NULL) ? pRemap[ui] : ui;
                float fSize = pfIRadii[index] * pfISizes[index];

                NiPoint3 kV0 = fSize * kTopRight;
                NiPoint3 kV1 = fSize * kTopLeft;

                const NiPoint3& kPosition = pkIPositions[index];

                *pkOVertices = kPosition + kV1;
                ++pkOVertices;

                *pkOVertices = kPosition - kV0;
                ++pkOVertices;

                *pkOVertices = kPosition - kV1;
                ++pkOVertices;

                *pkOVertices = kPosition + kV0;
                ++pkOVertices;
            }
        }
    }
}
NiSPEndKernelImpl(NiPSFacingQuadGeneratorKernel)

//--------------------------------------------------------------------------------------------------
