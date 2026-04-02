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
#include "NiColor.h"
#include "NiStream.h"

EE_COMPILETIME_ASSERT(sizeof(efd::Color) == sizeof(NiColor));
EE_COMPILETIME_ASSERT(sizeof(efd::Color) == 3*sizeof(float));

EE_COMPILETIME_ASSERT(sizeof(efd::ColorA) == sizeof(NiColorA));
EE_COMPILETIME_ASSERT(sizeof(efd::ColorA) == 4*sizeof(float));

EE_COMPILETIME_ASSERT(sizeof(efd::ColorA_UInt8) == sizeof(NiRGBA));
EE_COMPILETIME_ASSERT(sizeof(efd::ColorA_UInt8) == 4);

#ifndef __SPU__

//--------------------------------------------------------------------------------------------------
void NiColor::LoadBinary(NiStream& stream)
{
    NiStreamLoadBinary(stream,r);
    NiStreamLoadBinary(stream,g);
    NiStreamLoadBinary(stream,b);
}

//--------------------------------------------------------------------------------------------------
void NiColor::SaveBinary(NiStream& stream)
{
    NiStreamSaveBinary(stream,r);
    NiStreamSaveBinary(stream,g);
    NiStreamSaveBinary(stream,b);
}

//--------------------------------------------------------------------------------------------------
void NiColor::LoadBinary(NiStream& stream, NiColor* pkValues, unsigned int uiNumValues)
{
    NiStreamLoadBinary(stream, (float*)pkValues, uiNumValues*3);
}

//--------------------------------------------------------------------------------------------------
void NiColor::SaveBinary(NiStream& stream, NiColor* pkValues, unsigned int uiNumValues)
{
    NiStreamSaveBinary(stream, (float*)pkValues, uiNumValues*3);
}

//--------------------------------------------------------------------------------------------------
char* NiColor::GetViewerString(const char* pPrefix) const
{
    size_t stLen = strlen(pPrefix) + 28;
    char* pString = NiAlloc(char, stLen);
    NiSprintf(pString, stLen, "%s = (%5.3f,%5.3f,%5.3f)", pPrefix, r, g, b);
    return pString;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void NiColorA::LoadBinary(NiStream& stream)
{
    NiStreamLoadBinary(stream,r);
    NiStreamLoadBinary(stream,g);
    NiStreamLoadBinary(stream,b);
    NiStreamLoadBinary(stream,a);
}

//--------------------------------------------------------------------------------------------------
void NiColorA::SaveBinary(NiStream& stream)
{
    NiStreamSaveBinary(stream,r);
    NiStreamSaveBinary(stream,g);
    NiStreamSaveBinary(stream,b);
    NiStreamSaveBinary(stream,a);
}

//--------------------------------------------------------------------------------------------------
void NiColorA::LoadBinary(NiStream& stream, NiColorA* pkValues, unsigned int uiNumValues)
{
    NiStreamLoadBinary(stream, (float*)pkValues, uiNumValues*4);
}

//--------------------------------------------------------------------------------------------------
void NiColorA::SaveBinary(NiStream& stream, NiColorA* pkValues, unsigned int uiNumValues)
{
    NiStreamSaveBinary(stream, (float*)pkValues, uiNumValues*4);
}

//--------------------------------------------------------------------------------------------------
char* NiColorA::GetViewerString(const char* pPrefix) const
{
    size_t stLen = strlen(pPrefix) + 36;
    char* pString = NiAlloc(char, stLen);
    NiSprintf(pString, stLen, "%s = (%5.3f,%5.3f,%5.3f,%5.3f)",
        pPrefix, r, g, b, a);
    return pString;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void NiRGBA::LoadBinary(NiStream& kStream)
{
    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 4, 0, 11))
    {
        NiUInt32 uiColor;
        NiStreamLoadBinary(kStream, uiColor);

#if defined(_XENON) || defined(_PS3)
        // Byte swapping was performed upon load. Swizzle the bits so that
        // the colors are properly stored.
        r() = static_cast<NiUInt8>(uiColor);
        g() = static_cast<NiUInt8>(uiColor >> 8);
        b() = static_cast<NiUInt8>(uiColor >> 16);
        a() = static_cast<NiUInt8>(uiColor >> 24);
#else
        m_uiColor = uiColor;
#endif
    }
    else
    {
        // The color values must be streamed separately to avoid endian
        // swapping issues.
        NiStreamLoadBinary(kStream, r());
        NiStreamLoadBinary(kStream, g());
        NiStreamLoadBinary(kStream, b());
        NiStreamLoadBinary(kStream, a());
    }
}

//--------------------------------------------------------------------------------------------------
void NiRGBA::SaveBinary(NiStream& kStream)
{
    // The color values must be streamed separately to avoid endian swapping
    // issues.
    NiStreamSaveBinary(kStream, r());
    NiStreamSaveBinary(kStream, g());
    NiStreamSaveBinary(kStream, b());
    NiStreamSaveBinary(kStream, a());
}

//--------------------------------------------------------------------------------------------------
void NiRGBA::LoadBinary(NiStream& stream, NiRGBA* pkValues, unsigned int uiNumValues)
{
    NiStreamLoadBinary(stream, (NiUInt8*)pkValues, uiNumValues*4);
}

//--------------------------------------------------------------------------------------------------
void NiRGBA::SaveBinary(NiStream& stream, NiRGBA* pkValues, unsigned int uiNumValues)
{
    NiStreamSaveBinary(stream, (NiUInt8*)pkValues, uiNumValues*4);
}

//--------------------------------------------------------------------------------------------------
char* NiRGBA::GetViewerString(const char* pcPrefix) const
{
    size_t stLen = strlen(pcPrefix) + 24;
    char* pcString = NiAlloc(char, stLen);
    NiSprintf(pcString, stLen, "%s = (%3d, %3d, %3d, %3d)",
        pcPrefix, r(), g(), b(), a());
    return pcString;
}

//--------------------------------------------------------------------------------------------------

#endif
