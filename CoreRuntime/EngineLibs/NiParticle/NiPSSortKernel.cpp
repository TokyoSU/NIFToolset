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

#include "NiPSSortKernel.h"
#include <NiUniversalTypes.h>
#include <NiMath.h>

using namespace NiPSSortKernelFunctions;

//--------------------------------------------------------------------------------------------------
NiSPBeginKernelImpl(NiPSSortKernel)
{
    // Indicate to a profiler what we are up to
#ifdef _XENON
    PIXBeginNamedEvent(D3DCOLOR_XRGB(128,255,128), "NiPSSortKernel");
#endif

    // Get input streams.
    const NiPSSortKernelStruct* pIStruct =
        kWorkload.GetInput<NiPSSortKernelStruct>(0);
    const NiPoint3* pPositions = kWorkload.GetInput<NiPoint3>(1);
    NiUInt32* pInputRemap = kWorkload.GetInput<NiUInt32>(2);

    // Get output streams.
    NiUInt32* pOutput = kWorkload.GetOutput<NiUInt32>(0);

    // Get block count.
    NiUInt32 uiBlockCount = kWorkload.GetBlockCount();

    // Visit each particle and insert it earlier in the remap list if it isn't a decreasing distance
    float lastDistance = GetDistance(
        pPositions[pInputRemap[0]], 
        pIStruct->m_camPos, 
        pIStruct->m_normal);
    pOutput[0] = pInputRemap[0];
    for (NiUInt32 ui = 1; ui < uiBlockCount; ui++)
    {
        pOutput[ui] = pInputRemap[ui];
        NiUInt32 currentIndex = pOutput[ui];
        float currentDistance = GetDistance(
            pPositions[currentIndex], 
            pIStruct->m_camPos, 
            pIStruct->m_normal);

        // early out if we know this has less distance that our most previous
        if (currentDistance < lastDistance)
        {
            lastDistance = currentDistance;
            continue;
        }

        // if this particle isn't in order, step backwards until we know where to insert it
        // as we step backwards, move the indices forwards to make room
        pOutput[ui] = pOutput[ui - 1];
        NiUInt32 backwardsCounter = ui - 1;
        while ((backwardsCounter > 0) && (currentDistance > GetDistance(
            pPositions[pOutput[backwardsCounter - 1]], pIStruct->m_camPos, pIStruct->m_normal)))
        {
            pOutput[backwardsCounter] = pOutput[backwardsCounter - 1];
            backwardsCounter--;
        }
        // at this point, backwardsCounter should point to the last entry with a greater distance
        // than our current particle
        pOutput[backwardsCounter] = currentIndex;
    }

    // Indicate to a profiler that we are done
#ifdef _XENON
    PIXEndNamedEvent();
#endif
}
NiSPEndKernelImpl(NiPSSortKernel)

//--------------------------------------------------------------------------------------------------
float NiPSSortKernelFunctions::GetDistance(
    const NiPoint3& point1,
    const NiPoint3& point2,
    const NiPoint3& normal)
{
    return NiAbs((point1 - point2).Dot(normal));
}

//--------------------------------------------------------------------------------------------------
