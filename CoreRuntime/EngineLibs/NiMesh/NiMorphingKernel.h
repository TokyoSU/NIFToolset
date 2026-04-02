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
#ifndef NIMORPHINGKERNEL_H
#define NIMORPHINGKERNEL_H

#include <NiSPKernelMacros.h>

/**
    This count is used to size constant sized arrays inside the kernel.
    It is the maximum number of active (non-zero-weight) targets on
    any given frame on some platforms. Other platforms may have a lower
    limit on active targets.
*/
#define MAX_MORPH_TARGETS 128

/**
    The NiMorphingKernel is a Floodgate kernel class used
    to perform morphing operations.

    It is used by the NiMorphMeshModifier class and should never
    need to be used directly by an application.
*/
NiSPDeclareKernel(NiMorphingKernel)

#endif

