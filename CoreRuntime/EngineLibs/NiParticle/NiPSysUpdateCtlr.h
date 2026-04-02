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
#ifndef NIPSYSUPDATECTLR_H
#define NIPSYSUPDATECTLR_H

#include <NiParticleLibType.h>
#include <NiTimeController.h>

/// @cond DEPRECATED_CLASS

/**
    This class is deprecated.

    It only exists to support loading old NIF files. It has been replaced by
    NiPSParticleSystem.
*/
class NIPARTICLE_ENTRY NiPSysUpdateCtlr : public NiTimeController
{
    NiDeclareRTTI;
    NiDeclareStream;

public:
    // *** begin Emergent internal use only ***
    virtual void Update(float fTime);
    virtual void SetTarget(NiObjectNET* pkTarget);
    // *** end Emergent internal use only ***

protected:
    // For streaming only.
    NiPSysUpdateCtlr();

    virtual bool TargetIsRequiredType() const;
};

NiSmartPointer(NiPSysUpdateCtlr);

/// @endcond

#endif // #ifndef NIPSYSUPDATECTLR_H
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
