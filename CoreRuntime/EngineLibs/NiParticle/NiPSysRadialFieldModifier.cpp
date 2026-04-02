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

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING

#include "NiPSysRadialFieldModifier.h"
#include <NiStream.h>

NiImplementRTTI(NiPSysRadialFieldModifier, NiPSysFieldModifier);

//--------------------------------------------------------------------------------------------------
NiPSysRadialFieldModifier::NiPSysRadialFieldModifier() : m_fRadialType(0.0f)
{
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSysRadialFieldModifier);

//--------------------------------------------------------------------------------------------------
void NiPSysRadialFieldModifier::LoadBinary(NiStream& kStream)
{
    NiPSysFieldModifier::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_fRadialType);
}

//--------------------------------------------------------------------------------------------------
void NiPSysRadialFieldModifier::LinkObject(NiStream& kStream)
{
    NiPSysFieldModifier::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSysRadialFieldModifier::RegisterStreamables(NiStream& kStream)
{
    return NiPSysFieldModifier::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSysRadialFieldModifier::SaveBinary(NiStream& kStream)
{
    NiPSysFieldModifier::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_fRadialType);
}

//--------------------------------------------------------------------------------------------------
bool NiPSysRadialFieldModifier::IsEqual(NiObject* pkObject)
{
    if (!NiPSysFieldModifier::IsEqual(pkObject))
    {
        return false;
    }

    NiPSysRadialFieldModifier* pkDest = (NiPSysRadialFieldModifier*) pkObject;

    if (pkDest->m_fRadialType != m_fRadialType)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
