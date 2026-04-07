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

#include "NiPoint2.h"
#include "NiStream.h"

EE_COMPILETIME_ASSERT(sizeof(efd::Point2) == sizeof(NiPoint2));
EE_COMPILETIME_ASSERT(sizeof(efd::Point2) == 2*sizeof(float));

//--------------------------------------------------------------------------------------------------
void NiPoint2::LoadBinary(NiStream& stream)
{
    NiStreamLoadBinary(stream,x);
    NiStreamLoadBinary(stream,y);
}

//--------------------------------------------------------------------------------------------------
void NiPoint2::SaveBinary(NiStream& stream) const
{
    NiStreamSaveBinary(stream,x);
    NiStreamSaveBinary(stream,y);
}

//--------------------------------------------------------------------------------------------------
char* NiPoint2::GetViewerString(const char* pPrefix) const
{
    size_t stLen = strlen(pPrefix) + 36;
    char* pString = NiAlloc(char, stLen);
    NiSprintf(pString, stLen, "%s = (%g,%g)", pPrefix, x, y);
    return pString;
}

//--------------------------------------------------------------------------------------------------
void NiPoint2::LoadBinary(
    NiStream& stream,
    NiPoint2* pkValues,
    unsigned int uiNumValues)
{
    NiStreamLoadBinary(stream, (float*)pkValues, uiNumValues*2);
}

//--------------------------------------------------------------------------------------------------
void NiPoint2::SaveBinary(
    NiStream& stream,
    NiPoint2* pkValues,
    unsigned int uiNumValues)
{
    NiStreamSaveBinary(stream, (float*)pkValues, uiNumValues*2);
}

//--------------------------------------------------------------------------------------------------
