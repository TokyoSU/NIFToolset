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
#include "NiPoint3.h"
#include "NiStream.h"

EE_COMPILETIME_ASSERT(sizeof(efd::Point3) == sizeof(NiPoint3));
EE_COMPILETIME_ASSERT(sizeof(efd::Point3) == 3*sizeof(float));

#ifndef __SPU__

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
void NiPoint3::LoadBinary(NiStream& stream)
{
    NiStreamLoadBinary(stream,x);
    NiStreamLoadBinary(stream,y);
    NiStreamLoadBinary(stream,z);
}

//--------------------------------------------------------------------------------------------------
void NiPoint3::SaveBinary(NiStream& stream) const
{
    NiStreamSaveBinary(stream,x);
    NiStreamSaveBinary(stream,y);
    NiStreamSaveBinary(stream,z);
}

//--------------------------------------------------------------------------------------------------
char* NiPoint3::GetViewerString(const char* pPrefix) const
{
    size_t stLen = strlen(pPrefix) + 64;
    char* pString = NiAlloc(char, stLen);
    NiSprintf(pString, stLen, "%s = (%g,%g,%g)", pPrefix, x, y, z);
    return pString;
}

//--------------------------------------------------------------------------------------------------
void NiPoint3::LoadBinary(NiStream& stream, NiPoint3* pkValues,
                                 unsigned int uiNumValues)
{
    NiStreamLoadBinary(stream, (float*)pkValues, uiNumValues*3);
}

//--------------------------------------------------------------------------------------------------
void NiPoint3::SaveBinary(NiStream& stream, NiPoint3* pkValues,
                                 unsigned int uiNumValues)
{
    NiStreamSaveBinary(stream, (float*)pkValues, uiNumValues*3);
}

//--------------------------------------------------------------------------------------------------
#endif
