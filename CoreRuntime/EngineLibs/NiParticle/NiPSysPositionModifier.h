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
#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
#ifndef NIPSYSPOSITIONMODIFIER_H
#define NIPSYSPOSITIONMODIFIER_H

#include "NiPSysModifier.h"

/// @cond DEPRECATED_CLASS

/**
    This class is deprecated.

    It only exists to support loading old NIF files. It has been replaced by
    NiPSSimulatorFinalStep.
*/
class NIPARTICLE_ENTRY NiPSysPositionModifier : public NiPSysModifier
{
    NiDeclareRTTI;
    NiDeclareStream;

protected:
    // For streaming only.
    NiPSysPositionModifier();
};

NiSmartPointer(NiPSysPositionModifier);

/// @endcond

#endif // #ifndef NIPSYSPOSITIONMODIFIER_H
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
