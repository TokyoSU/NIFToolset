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

#pragma once
#ifndef NIPSSORTKERNEL_H
#define NIPSSORTKERNEL_H

#include <NiParticleLibType.h>
#include <NiSPKernelMacros.h>
#include <NiPSKernelDefinitions.h>

// Particle sorting is n^2 and requires access to the entire simulation position buffer so it
// cannot be split into multiple workloads. To keep performance to a reasonable level, we have
// a static cap on the number of particles that can be sorted. If this cap is modified, it 
// must stay small enough such that the position input stream can still fit within the PS3's 
// max input chunk size of 16384 - 128. This value was chosen because profiling revealed that 
// sorting this number of particles is approximately as expensive as quad generation for the 
// same number of particles
#define MAX_SORTED_PARTICLES 350

/// A structure containing data required by the NiPSSortKernel
struct NiPSSortKernelStruct
{
    /// Camera position in model space
    NiPoint3 m_camPos;

    /// Normal vector in model space.
    NiPoint3 m_normal;
};

/**
    Populates an index array that lists particle indices from furthest to nearest

    Sorting is performed relative to a camera where the sort distance is the component of the 
    camera's offset from the particle relative to the particle's normal
*/
NiSPDeclareKernelLib(NiPSSortKernel, NIPARTICLE_ENTRY)

namespace NiPSSortKernelFunctions
{
/// Get the distance between two points along a given axis
inline float GetDistance(
    const NiPoint3& point1,
    const NiPoint3& point2,
    const NiPoint3& normal);
} // namespace NiPSSortKernelFunctions

#endif  // #ifndef NIPSFACINGQUADGENERATORKERNEL_H
