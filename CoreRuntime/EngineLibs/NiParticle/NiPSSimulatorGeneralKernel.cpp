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

#include "NiPSSimulatorGeneralKernel.h"
#include "NiPSKernelDefinitions.h"
#include <NiPSSimulatorKernelHelpers.h>
#include <NiPSFlagsHelpers.h>

//--------------------------------------------------------------------------------------------------
NiSPBeginKernelImpl(NiPSSimulatorGeneralKernel)
{
    // Get input streams.
    const NiPSSimulatorGeneralKernelStruct* pkIStruct =
        kWorkload.GetInput<NiPSSimulatorGeneralKernelStruct>(0);
    const NiPSKernelFloatKey* pkISizeKeys = kWorkload.GetInput<NiPSKernelFloatKey>(1);
    const NiPSKernelColorKey* pkIColorKeys = kWorkload.GetInput<NiPSKernelColorKey>(2);
    const NiPSKernelFloatKey* pkIRotationKeys = kWorkload.GetInput<NiPSKernelFloatKey>(3);
    const NiRGBA* pkIColors = kWorkload.GetInput<NiRGBA>(4);
    const NiUInt32* puiIFlags = kWorkload.GetInput<NiUInt32>(5);
    const float* pfIAges = kWorkload.GetInput<float>(6);
    const float* pfILifeSpans = kWorkload.GetInput<float>(7);
    const float* pfIRotAngles = kWorkload.GetInput<float>(8);
    const float* pfIRotSpeeds = kWorkload.GetInput<float>(9);

    // Get output streams.
    float* pfOSizes = kWorkload.GetOutput<float>(0);
    NiRGBA* pkOColors = kWorkload.GetOutput<NiRGBA>(1);
    float* pfORotAngles = kWorkload.GetOutput<float>(2);

    // Get block count.
    NiUInt32 uiBlockCount = kWorkload.GetBlockCount();

    // Simulate particles.
    for (NiUInt32 ui = 0; ui < uiBlockCount; ++ui)
    {
        NiUInt16 usGeneration = NiPSFlagsHelpers::GetGeneration(puiIFlags[ui]);

        // Update grow/shrink size.
        NiPSSimulatorKernelHelpers::UpdateParticleSize(
            pfOSizes[ui],
            usGeneration,
            pfIAges[ui],
            pfILifeSpans[ui],
            pkIStruct->m_fGrowTime,
            pkIStruct->m_fShrinkTime,
            pkIStruct->m_usGrowGeneration,
            pkIStruct->m_usShrinkGeneration,
            pkIStruct->m_ucNumSizeKeys, pkISizeKeys, pkIStruct->m_eSizeLoopBehavior);

        if (pkOColors)
        {
            if (pkIStruct->m_ucNumColorKeys)
            {
                // Update color animation.
                NiPSSimulatorKernelHelpers::UpdateParticleColor(
                    pkOColors[ui],
                    pkIColors[ui],
                    pfIAges[ui],
                    pfILifeSpans[ui],
                    pkIStruct->m_ucNumColorKeys, pkIColorKeys, pkIStruct->m_eColorLoopBehavior,
                    pkIStruct->m_ucBGRA);
            }
            else
            {
                pkOColors[ui] = pkIColors[ui];
            }
        }

        if (pfIRotAngles && pfIRotSpeeds)
        {
            // Update rotation angle.
            NiPSSimulatorKernelHelpers::UpdateParticleRotation(
                pfORotAngles[ui],
                pfIRotAngles[ui],
                pfIRotSpeeds[ui],
                pfIAges[ui],
                pfILifeSpans[ui],
                pkIStruct->m_ucNumRotationKeys, pkIRotationKeys,
                pkIStruct->m_eRotationLoopBehavior);
        }
    }
}
NiSPEndKernelImpl(NiPSSimulatorGeneralKernel)

//--------------------------------------------------------------------------------------------------
