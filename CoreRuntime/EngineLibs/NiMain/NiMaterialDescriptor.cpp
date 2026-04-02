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

#include "NiMaterialDescriptor.h"
#include "NiMath.h"
#include <NiSystem.h>

//--------------------------------------------------------------------------------------------------
NiMaterialDescriptor::NiMaterialDescriptor(unsigned int uiDWORDCount)
{
    m_bExternalAlloc = false;
    m_uiDWORDCount = uiDWORDCount;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiMalloc(
        sizeof(unsigned int) * m_uiDWORDCount);

    // Clear bit array
    for (unsigned int ui = 0; ui < m_uiDWORDCount; ui++)
    {
        m_pkBitArray[ui] = 0;
    }
}

//--------------------------------------------------------------------------------------------------
NiMaterialDescriptor::NiMaterialDescriptor(unsigned int uiDWORDCount,
    unsigned int* pkBitArray)
{
    m_bExternalAlloc = true;
    m_uiDWORDCount = uiDWORDCount;

    // Allocate bit array
    m_pkBitArray = pkBitArray;

    // Clear bit array
    for (unsigned int ui = 0; ui < m_uiDWORDCount; ui++)
    {
        m_pkBitArray[ui] = 0;
    }
}

//--------------------------------------------------------------------------------------------------
NiMaterialDescriptor::NiMaterialDescriptor(
    NiMaterialDescriptor* pkMatDescriptor)
{
    m_kIdentifier = pkMatDescriptor->m_kIdentifier;
    m_uiDWORDCount = pkMatDescriptor->m_uiDWORDCount;

    m_bExternalAlloc = false;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiMalloc(
        sizeof(unsigned int) * m_uiDWORDCount);

    // Clear bit array
    for (unsigned int ui = 0; ui < m_uiDWORDCount; ui++)
    {
        m_pkBitArray[ui] = pkMatDescriptor->m_pkBitArray[ui];
    }
}

//--------------------------------------------------------------------------------------------------
NiMaterialDescriptor::NiMaterialDescriptor()
{
    m_bExternalAlloc = false;

    // Use 128 bits by default.
    m_uiDWORDCount = 4;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiMalloc(
        sizeof(unsigned int) * m_uiDWORDCount);

    // Clear bit array
    for (unsigned int ui = 0; ui < m_uiDWORDCount; ui++)
    {
        m_pkBitArray[ui] = 0;
    }
}

//--------------------------------------------------------------------------------------------------
NiMaterialDescriptor::~NiMaterialDescriptor()
{
    if (!m_bExternalAlloc)
        NiFree(m_pkBitArray);
}

//--------------------------------------------------------------------------------------------------
bool NiMaterialDescriptor::GenerateKey(char* pcValue,
    unsigned int uiMaxSize)
{
    EE_ASSERT((m_uiDWORDCount * 8) + (m_uiDWORDCount-1) + 1 < uiMaxSize);

    pcValue[0] = 0;

    char acTemp[10];
    for (NiUInt32 ui = 0; ui < m_uiDWORDCount; ui++)
    {
        if (ui == m_uiDWORDCount-1)
            NiSprintf(acTemp, 10, "%.8X", m_pkBitArray[ui]);
        else
            NiSprintf(acTemp, 10, "%.8X-", m_pkBitArray[ui]);

        NiStrcat(pcValue, uiMaxSize, acTemp);
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiMaterialDescriptor::CopyData(const NiMaterialDescriptor* pkOther)
{
    m_kIdentifier = pkOther->m_kIdentifier;

    for (unsigned int ui = 0; ui < m_uiDWORDCount; ui++)
    {
        m_pkBitArray[ui] = pkOther->m_pkBitArray[ui];
    }
}

//--------------------------------------------------------------------------------------------------
bool NiMaterialDescriptor::IsEqual(
    const NiMaterialDescriptor* pkOther) const
{
    if (m_kIdentifier != pkOther->m_kIdentifier)
        return false;

    int uiMinDWORDCount =
        NiMin((int)m_uiDWORDCount, (int)pkOther->m_uiDWORDCount);

    for (int ui = 0; ui < uiMinDWORDCount; ui++)
    {
        if (pkOther->m_pkBitArray[ui] != m_pkBitArray[ui])
            return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
NiMaterialDescriptor& NiMaterialDescriptor::operator=(
    const NiMaterialDescriptor& kDescriptor)
{
    m_kIdentifier = kDescriptor.m_kIdentifier;
    m_uiDWORDCount = kDescriptor.m_uiDWORDCount;
    m_bExternalAlloc = false;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiRealloc(m_pkBitArray,
        sizeof(unsigned int) * m_uiDWORDCount);

    for (unsigned int ui = 0; ui < m_uiDWORDCount; ui++)
        m_pkBitArray[ui] = kDescriptor.m_pkBitArray[ui];

    return *this;
}

//--------------------------------------------------------------------------------------------------
