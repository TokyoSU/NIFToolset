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
#ifndef NIPSYSVOLUMEEMITTER_H
#define NIPSYSVOLUMEEMITTER_H

#include "NiPSysEmitter.h"

class NiAVObject;

/// @cond DEPRECATED_CLASS

/**
    This class is deprecated.

    It only exists to support loading old NIF files. It has been replaced by
    NiPSVolumeEmitter.
*/
class NIPARTICLE_ENTRY NiPSysVolumeEmitter : public NiPSysEmitter
{
    NiDeclareRTTI;
    NiDeclareAbstractStream;

public:
    // *** begin Emergent internal use only ***
    NiAVObject* GetEmitterObj() const;
    // *** end Emergent internal use only ***

protected:
    // For streaming only.
    NiPSysVolumeEmitter();

    NiAVObject* m_pkEmitterObj;
};

NiSmartPointer(NiPSysVolumeEmitter);

/// @endcond

#include "NiPSysVolumeEmitter.inl"

#endif // #ifndef NIPSYSVOLUMEEMITTER_H
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
