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
#include "NiMainPCH.h"

#include <NiRTLib.h>
#include "NiMatrix3.h"
#include "NiQuaternion.h"
#include "NiStream.h"

EE_COMPILETIME_ASSERT(sizeof(efd::Quaternion) == sizeof(NiQuaternion));
EE_COMPILETIME_ASSERT(sizeof(NiQuaternion) == 4*sizeof(float));

#ifndef __SPU__

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
void NiQuaternion::LoadBinary(NiStream& stream)
{
    NiStreamLoadBinary(stream,m_fW);
    NiStreamLoadBinary(stream,m_fX);
    NiStreamLoadBinary(stream,m_fY);
    NiStreamLoadBinary(stream,m_fZ);
}

//--------------------------------------------------------------------------------------------------
void NiQuaternion::SaveBinary(NiStream& stream)
{
    Snap();
    NiStreamSaveBinary(stream,m_fW);
    NiStreamSaveBinary(stream,m_fX);
    NiStreamSaveBinary(stream,m_fY);
    NiStreamSaveBinary(stream,m_fZ);
}

//--------------------------------------------------------------------------------------------------
char* NiQuaternion::GetViewerString(const char* pcPrefix) const
{
    size_t stLen = strlen(pcPrefix) + 128;
    char* pcString = NiAlloc(char, stLen);
    NiSprintf(pcString, stLen, "%s: (w=%g,x=%g,y=%g,z=%g)", pcPrefix,
        m_fW, m_fX, m_fY, m_fZ);
    return pcString;
}

//--------------------------------------------------------------------------------------------------
void NiQuaternion::LoadBinary(
    NiStream& stream,
    NiQuaternion* pkValues,
    unsigned int uiNumValues)
{
    NiStreamLoadBinary(stream, (float*)pkValues, uiNumValues*4);
}

//--------------------------------------------------------------------------------------------------
void NiQuaternion::SaveBinary(
    NiStream& stream,
    NiQuaternion* pkValues,
    unsigned int uiNumValues)
{
    NiStreamSaveBinary(stream, (float*)pkValues, uiNumValues*4);
}

//--------------------------------------------------------------------------------------------------
#endif

