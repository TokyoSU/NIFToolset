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

#include "NiShaderLibrary.h"
#include "NiShader.h"

//--------------------------------------------------------------------------------------------------
NiShaderLibrary::NiShaderLibrary(const char* pcName) :
    m_pcName(0)
{
    SetName(pcName);
}

//--------------------------------------------------------------------------------------------------
NiShaderLibrary::~NiShaderLibrary()
{
    NiFree(m_pcName);
    m_pcName = 0;
}

//--------------------------------------------------------------------------------------------------
const char* NiShaderLibrary::GetName()
{
    return m_pcName;
}

//--------------------------------------------------------------------------------------------------
void NiShaderLibrary::SetName(const char* pcName)
{
    NiFree(m_pcName);
    m_pcName = 0;

    if (pcName && pcName[0] != '\0')
    {
        size_t stLen = strlen(pcName) + 1;
        m_pcName = NiAlloc(char,stLen);
        EE_ASSERT(m_pcName);
        NiStrcpy(m_pcName, stLen, pcName);
    }
}

//--------------------------------------------------------------------------------------------------
NiShader* NiShaderLibrary::GetShader(NiRenderer*, const char*, unsigned int)
{
    // Default implementation returns 0
    return 0;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderLibrary::ReleaseShader(const char*, unsigned int)
{
    // Default implementation does nothing
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderLibrary::ReleaseShader(NiShader*)
{
    // Default implementation does nothing
    return false;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiShaderLibrary::GetNumSupportedMimeTypes() const
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
const char* NiShaderLibrary::GetSupportedMimeType(unsigned int uiIdx) const
{
    EE_UNUSED_ARG(uiIdx);

    EE_FAIL("Invalid index");
    return NULL;
}

//--------------------------------------------------------------------------------------------------