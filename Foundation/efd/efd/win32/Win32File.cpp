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

#include <efd/MemoryDefines.h>
#include <efd/Utilities.h>
#include <efd/PathUtils.h>
#include <efd/Win32/Win32File.h>

#include <share.h>

namespace efd
{

EE_ImplementDerivedBinaryStream(Win32File, FileRead, FileWrite);

//--------------------------------------------------------------------------------------------------
Win32File::Win32File(const char* pcName, OpenMode eMode,
                     unsigned int uiBufferSize, bool flushOnWrite)
    : m_bFlushOnWrite(flushOnWrite)
{
    SetEndianSwap(false);

    m_eMode = eMode;
    const char *pcMode;

    switch (m_eMode)
    {
    case READ_ONLY:
        pcMode = "rb";
        break;
    case WRITE_ONLY:
        pcMode = "wb";
        break;
    case APPEND_ONLY:
        pcMode = "ab";
        break;

    case READ_ONLY_TEXT:
        pcMode = "r";
        break;
    case WRITE_ONLY_TEXT:
        pcMode = "w";
        break;
    case APPEND_ONLY_TEXT:
        pcMode = "a";
        break;

    default:
        EE_FAIL_MESSAGE(("Invalid OpenMode %d", m_eMode));
        pcMode = "ab";
        break;
    }

    // Check for environment variables.
    char acFileName[EE_MAX_PATH];
    DWORD uiWrite = ::ExpandEnvironmentStringsA(pcName, acFileName, EE_MAX_PATH);

    // Make sure we have a valid file name size.
    if (uiWrite >= EE_MAX_PATH)
    {
        EE_FAIL("Filename too large for buffer.");
        return;
    }

    // Make sure we have a terminator.
    acFileName[uiWrite] = '\0';

    // open the file as a share and allow other processes to open it read-only.
    m_pFile = _fsopen(acFileName, pcMode, _SH_DENYWR);
    m_bGood = (m_pFile != NULL);

    m_uiBufferAllocSize = uiBufferSize;
    m_uiPos = m_uiBufferReadSize = 0;

    if (m_bGood && uiBufferSize > 0)
    {
        m_pBuffer = EE_ALLOC(char, m_uiBufferAllocSize);
        EE_ASSERT(m_pBuffer != NULL);
    }
    else
    {
        m_pBuffer = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
Win32File::~Win32File()
{
    if (m_bGood && m_pFile)
    {
        Flush();
        fclose(m_pFile);
    }

    EE_FREE(m_pBuffer);
}

//--------------------------------------------------------------------------------------------------
bool Win32File::Seek(int iOffset, SeekOrigin iWhence)
{
    EE_ASSERT(iWhence == SO_BEGIN || iWhence == SO_CURRENT || iWhence == SO_END);
    EE_ASSERT(m_eMode != APPEND_ONLY && m_eMode != APPEND_ONLY_TEXT);

    if (m_bGood)
    {
#if defined(EE_ASSERTS_ARE_ENABLED)
        unsigned int uiNewPos = (int)m_uiAbsoluteCurrentPos + iOffset;
#endif
        if (iWhence == SO_CURRENT)
        {
            // If we can accomplish the Seek by adjusting m_uiPos, do so.

            int iNewPos = (int) m_uiPos + iOffset;
            if (iNewPos >= 0 && iNewPos < (int) m_uiBufferReadSize)
            {
                m_uiPos = iNewPos;
                m_uiAbsoluteCurrentPos = (int)m_uiAbsoluteCurrentPos + iOffset;
                return true;
            }

            // User's notion of current file position is different from
            // actual file position because of bufferring implemented by
            // this class. Make appropriate adjustment to offset.

            if (File::READ_ONLY == m_eMode)
                iOffset -= (m_uiBufferReadSize - m_uiPos);
        }

        Flush();

        m_bGood = (fseek(m_pFile, iOffset, iWhence) == 0);
        if (m_bGood)
        {
            m_uiAbsoluteCurrentPos = ftell(m_pFile);
#if defined(EE_ASSERTS_ARE_ENABLED)
            if (iWhence == SO_CURRENT)
            {
                EE_ASSERT(uiNewPos == m_uiAbsoluteCurrentPos);
            }
            else if (iWhence == SO_BEGIN)
            {
                EE_ASSERT((int)m_uiAbsoluteCurrentPos == iOffset);
            }
#endif
        }
    }
    return m_bGood;
}

//--------------------------------------------------------------------------------------------------
unsigned int Win32File::FileRead(void* pBuffer, unsigned int uiBytes)
{
    EE_ASSERT(m_eMode == READ_ONLY);

    if (m_bGood)
    {
        unsigned int uiAvailBufferBytes, uiRead;

        uiRead = 0;
        uiAvailBufferBytes = m_uiBufferReadSize - m_uiPos;
        if (uiBytes > uiAvailBufferBytes)
        {
            if (uiAvailBufferBytes > 0)
            {
                Memcpy(pBuffer, &m_pBuffer[m_uiPos], uiAvailBufferBytes);
                pBuffer = &(((char *) pBuffer)[uiAvailBufferBytes]);
                uiBytes -= uiAvailBufferBytes;
                uiRead = uiAvailBufferBytes;
            }
            Flush();

            if (uiBytes > m_uiBufferAllocSize)
            {
                return uiRead + DiskRead(pBuffer, uiBytes);
            }
            else
            {
                m_uiBufferReadSize = DiskRead(m_pBuffer, m_uiBufferAllocSize);
                if (m_uiBufferReadSize < uiBytes)
                {
                    uiBytes = m_uiBufferReadSize;
                }
            }
        }

        Memcpy(pBuffer, &m_pBuffer[m_uiPos], uiBytes);
        m_uiPos += uiBytes;
        return uiRead + uiBytes;
    }
    else
    {
        return 0;
    }
}

//--------------------------------------------------------------------------------------------------
unsigned int Win32File::FileWrite(const void *pBuffer, unsigned int uiBytes)
{
    EE_ASSERT(m_eMode != READ_ONLY);
    EE_ASSERT(uiBytes != 0);

    if (m_bGood)
    {
        unsigned int uiWrite = 0;

        if (m_bFlushOnWrite)
        {
            uiWrite = DiskWrite(pBuffer, uiBytes);
            Flush();
            return uiWrite;
        }

        unsigned int uiAvailBufferBytes;

        uiAvailBufferBytes = m_uiBufferAllocSize - m_uiPos;
        if (uiBytes > uiAvailBufferBytes)
        {
            if (uiAvailBufferBytes > 0)
            {
                Memcpy(&m_pBuffer[m_uiPos], pBuffer, uiAvailBufferBytes);
                pBuffer = &(((char *) pBuffer)[uiAvailBufferBytes]);
                uiBytes -= uiAvailBufferBytes;
                uiWrite = uiAvailBufferBytes;
                m_uiPos = m_uiBufferAllocSize;
            }

            if (!Flush())
                return 0;

            if (uiBytes >= m_uiBufferAllocSize)
            {
                return uiWrite + DiskWrite(pBuffer, uiBytes);
            }
        }

        Memcpy(&m_pBuffer[m_uiPos], pBuffer, uiBytes);
        m_uiPos += uiBytes;
        return uiWrite + uiBytes;
    }
    else
    {
        return 0;
    }
}

//--------------------------------------------------------------------------------------------------
unsigned int Win32File::DiskWrite(const void* pBuffer, unsigned int uiBytes)
{
    return static_cast<unsigned int>(
        fwrite(pBuffer, 1, (size_t)uiBytes, m_pFile));
}

//--------------------------------------------------------------------------------------------------
unsigned int Win32File::DiskRead(void* pBuffer, unsigned int uiBytes)
{
    return static_cast<unsigned int>(
        fread(pBuffer, 1, (size_t)uiBytes, m_pFile));
}

//--------------------------------------------------------------------------------------------------
bool Win32File::Flush()
{
    EE_ASSERT(m_bGood);

    if (m_eMode == READ_ONLY)
    {
        m_uiBufferReadSize = 0;
    }
    else
    {
        if (m_uiPos > 0)
        {
            if (DiskWrite(m_pBuffer, m_uiPos) != m_uiPos)
            {
                m_bGood = false;
                return false;
            }
        }
        // Force a flush.
        fflush(m_pFile);
    }

    m_uiPos = 0;
    return true;
}


//--------------------------------------------------------------------------------------------------
unsigned int Win32File::GetFileSize() const
{
    int iCurrent = ftell(m_pFile);
    if (iCurrent < 0)
        return 0;
    fseek(m_pFile, 0, SEEK_END);
    int iSize = ftell(m_pFile);
    fseek(m_pFile, iCurrent, SEEK_SET);
    if (iSize < 0)
        return 0;
    return (unsigned int)iSize;
}

} // end namespace efd

