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
#ifndef NIPSYSROTATIONMODIFIER_H
#define NIPSYSROTATIONMODIFIER_H

#include "NiPSysModifier.h"
#include <NiPoint3.h>

/// @cond DEPRECATED_CLASS

/**
    This class is deprecated.

    It only exists to support loading old NIF files. It has been replaced by
    NiPSSimulatorGeneralStep and NiPSEmitter.
*/
class NIPARTICLE_ENTRY NiPSysRotationModifier : public NiPSysModifier
{
    NiDeclareRTTI;
    NiDeclareStream;

public:
    // *** begin Emergent internal use only ***
    float GetInitialRotSpeed() const;
    float GetInitialRotSpeedVar() const;
    bool GetRandomRotSpeedSign() const;
    float GetInitialRotAngle() const;
    float GetInitialRotAngleVar() const;
    bool GetRandomInitialAxis() const;
    const NiPoint3& GetInitialAxis() const;
    virtual void PostLinkObject(NiStream& kStream);
    // *** end Emergent internal use only ***

protected:
    // For streaming only.
    NiPSysRotationModifier();

    float m_fInitialRotSpeed;
    float m_fInitialRotSpeedVar;
    float m_fInitialRotAngle;
    float m_fInitialRotAngleVar;
    NiPoint3 m_kInitialAxis;
    bool m_bRandomInitialAxis;
    bool m_bRandomRotSpeedSign;
};

NiSmartPointer(NiPSysRotationModifier);

/// @endcond

#include "NiPSysRotationModifier.inl"

#endif // #ifndef NIPSYSROTATIONMODIFIER_H
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
