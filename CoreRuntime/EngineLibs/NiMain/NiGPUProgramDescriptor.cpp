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

#include "NiGPUProgramDescriptor.h"
#include <NiSystem.h>

//--------------------------------------------------------------------------------------------------
NiGPUProgramDescriptor::NiGPUProgramDescriptor()
{
    // Use 128 bits by default.
    m_uiIntCount = 4;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiMalloc(
        sizeof(unsigned int) * m_uiIntCount);

    // Clear bit array
    for (unsigned int ui = 0; ui < m_uiIntCount; ui++)
    {
        m_pkBitArray[ui] = 0;
    }
}

//--------------------------------------------------------------------------------------------------
NiGPUProgramDescriptor::NiGPUProgramDescriptor(unsigned int uiByteCount)
{
    m_uiIntCount = uiByteCount;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiMalloc(
        sizeof(unsigned int) * m_uiIntCount);

    // Clear bit array
    for (unsigned int ui = 0; ui < m_uiIntCount; ui++)
    {
        m_pkBitArray[ui] = 0;
    }
}

//--------------------------------------------------------------------------------------------------
NiGPUProgramDescriptor::NiGPUProgramDescriptor(
    const NiGPUProgramDescriptor& kOther) : m_kIdentifier(kOther.m_kIdentifier)
{
    m_uiIntCount = kOther.m_uiIntCount;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiMalloc(
        sizeof(unsigned int) * m_uiIntCount);

    for (unsigned int ui = 0; ui < m_uiIntCount; ui++)
    {
        m_pkBitArray[ui] = kOther.m_pkBitArray[ui];
    }
}

//--------------------------------------------------------------------------------------------------
NiGPUProgramDescriptor::~NiGPUProgramDescriptor()
{
    NiFree(m_pkBitArray);
}

//--------------------------------------------------------------------------------------------------
bool NiGPUProgramDescriptor::GenerateKey(char* pcValue,
    unsigned int uiMaxSize) const
{
    // Ensure there is enough room in the incoming buffer to store the key.
    EE_ASSERT((m_uiIntCount * 8) + (m_uiIntCount-1) + 1 < uiMaxSize);

    pcValue[0] = 0;

    char acTemp[10];
    for (NiUInt32 ui = 0; ui < m_uiIntCount; ui++)
    {
        if (ui == m_uiIntCount-1)
            NiSprintf(acTemp, 10, "%.8X", m_pkBitArray[ui]);
        else
            NiSprintf(acTemp, 10, "%.8X-", m_pkBitArray[ui]);

        NiStrcat(pcValue, uiMaxSize, acTemp);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiGPUProgramDescriptor::IsEqual(
    const NiGPUProgramDescriptor* pkOther) const
{
    if (m_kIdentifier != pkOther->m_kIdentifier)
        return false;

    for (unsigned int ui = 0; ui < m_uiIntCount; ui++)
    {
        if (pkOther->m_pkBitArray[ui] != m_pkBitArray[ui])
            return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
NiGPUProgramDescriptor& NiGPUProgramDescriptor::operator=(
    const NiGPUProgramDescriptor& kDescriptor)
{
    m_kIdentifier = kDescriptor.m_kIdentifier;

    m_uiIntCount = kDescriptor.m_uiIntCount;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiRealloc(m_pkBitArray,
        sizeof(unsigned int) * m_uiIntCount);

    for (unsigned int ui = 0; ui < m_uiIntCount; ui++)
        m_pkBitArray[ui] = kDescriptor.m_pkBitArray[ui];

    return *this;
}

//--------------------------------------------------------------------------------------------------
