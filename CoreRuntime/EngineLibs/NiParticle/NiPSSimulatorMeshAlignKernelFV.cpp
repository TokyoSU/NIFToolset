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

#include "NiPSSimulatorMeshAlignKernelFV.h"
#include <NiPSSimulatorMeshAlignKernelStruct.h>
#include <NiPSSimulatorKernelHelpers.h>
#include <NiUniversalTypes.h>
#include <NiMath.h>

//--------------------------------------------------------------------------------------------------
NiSPBeginKernelImpl(NiPSSimulatorMeshAlignKernelFV)
{
    // Get input streams.
    const NiPSSimulatorMeshAlignKernelStruct* pkIStruct =
        kWorkload.GetInput<NiPSSimulatorMeshAlignKernelStruct>(0);
    const NiPSKernelQuaternionKey* pkIRotationKeys = kWorkload.GetInput<NiPSKernelQuaternionKey>(1);
    const NiPoint3* pkIPositions = kWorkload.GetInput<NiPoint3>(2);
    const NiPoint3* pkIVelocities = kWorkload.GetInput<NiPoint3>(3);
    const float* pfIRadii = kWorkload.GetInput<float>(4);
    const float* pfISizes = kWorkload.GetInput<float>(5);
    const float* pfIRotAngles = kWorkload.GetInput<float>(6);
    const NiPoint3* pfIRotAxes = kWorkload.GetInput<NiPoint3>(7);
    const float* pfIAges = kWorkload.GetInput<float>(8);
    const float* pfILifeSpans = kWorkload.GetInput<float>(9);

    // Get output streams.
    NiPoint3* pkORotations = kWorkload.GetOutput<NiPoint3>(0);
    float* pkOScales = kWorkload.GetOutput<float>(1);

    // Get block count.
    NiUInt32 uiBlockCount = kWorkload.GetBlockCount();

    const NiPoint3* pkIUp;
    if (pkIStruct->m_bUpUsePosition)
    {
        pkIUp = pkIPositions;
    }
    else
    {
        pkIUp = pkIVelocities;
    }

    if (pfIRotAngles)
    {
        EE_ASSERT(pfIRotAxes);
        for (NiUInt32 ui = 0; ui < uiBlockCount; ++ui)
        {
            NiPoint3 kUpIn = pkIStruct->m_kUpTransform.m_Rotate * pkIUp[ui] +
                pkIStruct->m_kUpTransform.m_Translate;

            NiPSSimulatorKernelHelpers::UpdateMeshAlignmentVVR(
                pkORotations[0], pkORotations[1], pkORotations[2], pkOScales[ui],
                pkIStruct->m_kNormal, kUpIn, pkIVelocities[ui], pfIRadii[ui], pfISizes[ui],
                pfIRotAngles[ui], pfIRotAxes[ui], pfIAges[ui], pfILifeSpans[ui],
                pkIStruct->m_ucNumRotationKeys, pkIRotationKeys,
                pkIStruct->m_eRotationLoopBehavior,
                pkIStruct->m_fScaleAmount, pkIStruct->m_fScaleRest, pkIStruct->m_fScaleLimit);
            pkORotations += 3;
        }
    }
    else
    {
        for (NiUInt32 ui = 0; ui < uiBlockCount; ++ui)
        {
            NiPoint3 kUpIn = pkIStruct->m_kUpTransform.m_Rotate * pkIUp[ui] +
                pkIStruct->m_kUpTransform.m_Translate;

            NiPSSimulatorKernelHelpers::UpdateMeshAlignmentVV(
                pkORotations[0], pkORotations[1], pkORotations[2], pkOScales[ui],
                pkIStruct->m_kNormal, kUpIn, pkIVelocities[ui], pfIRadii[ui], pfISizes[ui],
                pkIStruct->m_fScaleAmount, pkIStruct->m_fScaleRest, pkIStruct->m_fScaleLimit);
            pkORotations += 3;
        }
    }
}
NiSPEndKernelImpl(NiPSSimulatorMeshAlignKernelFV)

//--------------------------------------------------------------------------------------------------
