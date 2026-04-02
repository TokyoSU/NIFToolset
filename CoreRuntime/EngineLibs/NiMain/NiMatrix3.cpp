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

#include "NiMatrix3.h"
#include "NiStream.h"

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
void NiMatrix3::LoadBinary (NiStream& stream)
{
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
            NiStreamLoadBinary(stream,m_pEntry[row][col]);
    }
}

//--------------------------------------------------------------------------------------------------
void NiMatrix3::SaveBinary (NiStream& stream)
{
    Snap();
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
            NiStreamSaveBinary(stream,m_pEntry[row][col]);
    }
}

//--------------------------------------------------------------------------------------------------
char* NiMatrix3::GetViewerString (const char* pPrefix) const
{
    size_t stLen = strlen(pPrefix) + 128;
    char* pString = NiAlloc(char, stLen);
    NiSprintf(pString, stLen, "%s = ((%g,%g,%g),(%g,%g,%g),(%g,%g,%g))",
        pPrefix,
        m_pEntry[0][0], m_pEntry[0][1], m_pEntry[0][2],
        m_pEntry[1][0],m_pEntry[1][1],m_pEntry[1][2],
        m_pEntry[2][0],m_pEntry[2][1],m_pEntry[2][2]);
    return pString;
}

//--------------------------------------------------------------------------------------------------
