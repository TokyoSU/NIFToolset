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
#include "efdPCH.h"

#include <efd/BinaryStream.h>
#include <efd/BinaryLoadSave.h>
#include <efd/Endian.h>

namespace efd
{

bool BinaryStream::ms_bEndianMatchHint = false;

//--------------------------------------------------------------------------------------------------
BinaryStream::BinaryStream()
{
    // Set read and write function pointers as explicitly invalid.
    // Derived classes MUST assign before reading/writing.
    m_pfnRead = 0;
    m_pfnWrite = 0;
    m_uiAbsoluteCurrentPos = 0;
}

//--------------------------------------------------------------------------------------------------
BinaryStream::~BinaryStream()
{
}

//--------------------------------------------------------------------------------------------------
unsigned int BinaryStream::GetLine(char* pBuffer, unsigned int uiMaxBytes)
{
    unsigned int uiBytesRead = 0;
    unsigned int i = 0;

    EE_ASSERT(uiMaxBytes > 0);

    while (i + 1 < uiMaxBytes)
    {
        char c;
        unsigned int uiRead = Read(&c, 1);
        uiBytesRead += uiRead;

        if (uiRead != 1 || c == '\n')
            break;

        if (c != '\r')
            pBuffer[i++] = c;
    }

    pBuffer[i] = 0;

    return uiBytesRead;
}

//--------------------------------------------------------------------------------------------------
unsigned int BinaryStream::PutS(const char* pBuffer)
{
    unsigned int i = 0;

    while (*pBuffer != 0)
    {
        if (Write(pBuffer++, 1) == 1)
        {
            i++;
        }
        else
        {
            break;
        }
    }

    return i;
}

//--------------------------------------------------------------------------------------------------
bool BinaryStream::GetEndianMatchHint()
{
    return ms_bEndianMatchHint;
}

//--------------------------------------------------------------------------------------------------
void BinaryStream::SetEndianMatchHint(bool bForceMatch)
{
    ms_bEndianMatchHint = bForceMatch;
}

//--------------------------------------------------------------------------------------------------
void BinaryStream::DoByteSwap(void* pvData,
    unsigned int uiTotalBytes, unsigned int* puiComponentSizes,
    unsigned int uiNumComponents)
{
    // short circuit for byte transfer
    if (uiNumComponents == 1 && *puiComponentSizes == 1)
        return;

    char* pDummy = (char*)(pvData);
    unsigned int uiSize = 0;
    while (uiSize < uiTotalBytes)
    {
        for (unsigned int j = 0; j < uiNumComponents; j++)
        {
            unsigned int uiCompSize = puiComponentSizes[j];
            switch (uiCompSize)
            {
            case 2:
                Endian::Swap16((char*)pDummy, 1);
                break;
            case 4:
                Endian::Swap32((char*)pDummy, 1);
                break;
            case 8:
                Endian::Swap64((char*)pDummy, 1);
                break;
            case 1:
                // Don't need to swap a single byte.
                break;
            default:
                EE_FAIL("Endian swapping invalid size.");
                break;
            }
            pDummy += uiCompSize;
            uiSize += uiCompSize;
        }
    }
    EE_ASSERT(uiSize == uiTotalBytes);
}

//--------------------------------------------------------------------------------------------------
unsigned int BinaryStream::GetPosition() const
{
    return m_uiAbsoluteCurrentPos;
}

//--------------------------------------------------------------------------------------------------
void BinaryStream::WriteCString(const char* pcString)
{
    size_t uiLength = (pcString ? strlen(pcString) : 0);
    StreamSaveBinary(*this, uiLength);

    if (uiLength)
    {
        Write(pcString, (unsigned int)uiLength);
    }
}

//--------------------------------------------------------------------------------------------------
char* BinaryStream::ReadCString()
{
    char* pcString = NULL;

    unsigned int uiLength;
    StreamLoadBinary(*this, uiLength);
    if (uiLength > 0)
    {
        pcString = EE_ALLOC(char, uiLength + 1);
        EE_ASSERT(pcString);
        Read(pcString, uiLength);

        pcString[uiLength] = 0;
    }

    return pcString;
}

//--------------------------------------------------------------------------------------------------
unsigned int BinaryStream::Read(void *pvBuffer, unsigned int uiBytes)
{
    unsigned int uiSize = 1;
    unsigned int uiBytesRead = BinaryRead(pvBuffer, uiBytes, &uiSize, 1);
    return uiBytesRead;
}

//--------------------------------------------------------------------------------------------------
unsigned int BinaryStream::Write(const void *pvBuffer, unsigned int uiBytes)
{
    unsigned int uiSize = 1;
    unsigned int uiBytesWritten = BinaryWrite(pvBuffer, uiBytes, &uiSize, 1);
    return uiBytesWritten;
}

} // end namespace efd
