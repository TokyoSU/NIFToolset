// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

// Precompiled Header
#include "NiSystemPCH.h"

#include "NiFile.h"
#include "NiBinaryLoadSave.h"
#include "NiRTLib.h"
#include "NiSystem.h"

NiFile::FILECREATEFUNC NiFile::ms_pfnFileCreateFunc = 
    NiFile::DefaultFileCreateFunc;

NiFile::FILEACCESSFUNC NiFile::ms_pfnFileAccessFunc =
    NiFile::DefaultFileAccessFunc;
    
NiFile::CREATEDIRFUNC NiFile::ms_pfnCreateDirFunc =
    NiFile::DefaultCreateDirectoryFunc;

NiFile::DIREXISTSFUNC NiFile::ms_pfnDirExistsFunc =
    NiFile::DefaultDirectoryExistsFunc;

NiImplementDerivedBinaryStream(NiFile, FileRead, FileWrite);

//---------------------------------------------------------------------------
NiFile* NiFile::GetFile(const char *pcName, OpenMode eMode,
    unsigned int uiSize)
{
    return ms_pfnFileCreateFunc(pcName, eMode, uiSize);
}

//---------------------------------------------------------------------------
NiFile* NiFile::DefaultFileCreateFunc(const char *pcName, OpenMode eMode,
    unsigned int uiSize)
{
    return NiNew NiFile(pcName, eMode, uiSize);
}

//---------------------------------------------------------------------------
void NiFile::SetFileCreateFunc(FILECREATEFUNC pfnFunc)
{
    ms_pfnFileCreateFunc = (pfnFunc == NULL) ? DefaultFileCreateFunc : pfnFunc;
}

//---------------------------------------------------------------------------
bool NiFile::Access(const char *pcName, OpenMode eMode)
{
    return ms_pfnFileAccessFunc(pcName, eMode);
}

//---------------------------------------------------------------------------
bool NiFile::DefaultFileAccessFunc(const char *pcName, OpenMode eMode)
{
    NiFile kFile(pcName, eMode, 0);
    return kFile.m_bGood;
}

//---------------------------------------------------------------------------
void NiFile::SetFileAccessFunc(FILEACCESSFUNC pfnFunc)
{
    ms_pfnFileAccessFunc = (pfnFunc == NULL) ? DefaultFileAccessFunc : pfnFunc;
}

//---------------------------------------------------------------------------
NiFile::operator bool() const
{
    return m_bGood;
}
//---------------------------------------------------------------------------
void NiFile::Seek(int iNumBytes)
{
    Seek(iNumBytes, ms_iSeekCur);
}
//---------------------------------------------------------------------------
bool NiFile::CreateDirectory(const char* pcDirName)
{
    return ms_pfnCreateDirFunc(pcDirName);
}
//---------------------------------------------------------------------------
void NiFile::SetCreateDirectoryFunc(CREATEDIRFUNC pfnFunc)
{
    ms_pfnCreateDirFunc = (pfnFunc == NULL) ? DefaultCreateDirectoryFunc : 
        pfnFunc;
}
//---------------------------------------------------------------------------
bool NiFile::DirectoryExists(const char* pcDirName)
{
    return ms_pfnDirExistsFunc(pcDirName);
}
//---------------------------------------------------------------------------
void NiFile::SetDirectoryExistsFunc(DIREXISTSFUNC pfnFunc)
{
    ms_pfnDirExistsFunc = (pfnFunc == NULL) ? DefaultDirectoryExistsFunc : 
        pfnFunc;
}
//---------------------------------------------------------------------------
bool NiFile::CreateDirectoryRecursive(const char* pcFullPath)
{
    if (DirectoryExists(pcFullPath))
        return true;

    if (strlen(pcFullPath) > NI_MAX_PATH)
        return false;

    char acFullPathCopy[NI_MAX_PATH];
    NiStrcpy(acFullPathCopy, NI_MAX_PATH, pcFullPath);
    NiPath::Standardize(acFullPathCopy);

    unsigned int uiStart = 0;

    // Check for drive start path
    if (acFullPathCopy[uiStart + 1] == ':')
    {
        uiStart += 2;
    }
  
    // Consume the leading slash characters
    while (uiStart < NI_MAX_PATH && 
        (acFullPathCopy[uiStart] == '\\' || acFullPathCopy[uiStart] == '/'))
    {
        uiStart++;
    }

    // Search through the string buffer for any '\\' or '/' and 
    // make sure that the directory exists. If not, create it.
    bool bDealtWithNetworkPath = false;
    for (unsigned int ui = uiStart; ui < NI_MAX_PATH; ui++)
    {
        char cCurChar = acFullPathCopy[ui];
        if ((cCurChar == '/' || cCurChar == '\\')
#ifdef _PS3
            // handle paths such as /app_home/c:/foo/bar 
            // We will skip this code block if the current and last char
            // are ":/" or "//"
            // that will save us from attempting /app_home/c:
            //
            && !(ui == 0 || acFullPathCopy[ui-1] == ':' || 
            acFullPathCopy[ui-1] == '/' || acFullPathCopy[ui+1] == '/')
#endif
            )
        {
            acFullPathCopy[ui] = '\0';

            if (uiStart == 2 && bDealtWithNetworkPath == false &&
                acFullPathCopy[0] == '\\' && acFullPathCopy[1] == '\\')
            {
                // A network path such as "\\CPU1" would fail the 
                // DirectoryExists test below since no directory is
                // technically being specified. That case is detected by the
                // condition above and the DirectoryExists test is skipped 
                // because it is a network path only.
                bDealtWithNetworkPath = true;
            }
            else if (!DirectoryExists(acFullPathCopy))
            {
                if (!CreateDirectory(acFullPathCopy))
                    return false;
                NIASSERT(DirectoryExists(acFullPathCopy));
            }
            acFullPathCopy[ui] = cCurChar;
        }
    }

    // Assume that the last characters of the array may define a directory as 
    // well even though the string was not necessarily
    // terminated with a seperator.
    if (!DirectoryExists(acFullPathCopy))
    {
        if (!CreateDirectory(acFullPathCopy))
            return false;
        NIASSERT(DirectoryExists(acFullPathCopy));
    }

    NIASSERT(DirectoryExists(pcFullPath));
    return true;
}
//---------------------------------------------------------------------------

const int NiFile::ms_iSeekSet = SEEK_SET;
const int NiFile::ms_iSeekCur = SEEK_CUR;
const int NiFile::ms_iSeekEnd = SEEK_END;

//---------------------------------------------------------------------------
NiFile::NiFile()
{
    m_pBuffer = NULL;
    m_pFile = NULL;
}
//---------------------------------------------------------------------------
NiFile::NiFile(const char* pcName, OpenMode eMode, unsigned int uiBufferSize)
{
    NIASSERT(eMode == READ_ONLY || eMode == WRITE_ONLY ||
        eMode == APPEND_ONLY);

    SetEndianSwap(false);

    m_eMode = eMode;
    const char* pcMode;

    if (m_eMode == READ_ONLY)
    {
        pcMode = "rb";
    }
    else
    {
        pcMode = (m_eMode == WRITE_ONLY) ? "wb" : "ab";
    }

    // Check for environment variables.
    char acFileName[NI_MAX_PATH];
    NiUInt32 uiWrite = 0;
    size_t stLength = strlen(pcName);
    for (size_t st = 0; st < stLength; st++)
    {
        // Make sure we have a valid file name size.
        if (uiWrite >= NI_MAX_PATH)
        {
            NIASSERT(0 && "Filename too large for buffer.");
            break;
        }

        // Check for environment variables.
        if (pcName[st] == '%')
        {
            size_t stStart = st + 1;
            size_t stEnd = strcspn(&pcName[stStart], "%") + 1;

            // Get the environement variable.
            char acEnvVariable[128];
            NiStrncpy(acEnvVariable, sizeof(acEnvVariable),
                &pcName[stStart], stEnd - stStart);

            // Get the enviroment variable value.
            char acEnvValue[MAX_PATH];
            size_t stEnvValueLength;
            NiGetenv(&stEnvValueLength, acEnvValue, sizeof(acEnvValue),
                acEnvVariable);
            if (stEnvValueLength)
            {
                // Set the terminator.
                acFileName[uiWrite] = '\0';
                NiStrcat(acFileName, NI_MAX_PATH, acEnvValue);
                uiWrite += (NiUInt32)(stEnvValueLength - 1);

                // Pass up the %
                st = stEnd;
            }
            else
            {
                // Just copy the character if we can not find the enviroment 
                // variable.
                acFileName[uiWrite] = pcName[st];
                uiWrite++;
            }
        }
        else
        {
            // Just copy the character if we are not an enviroment variable.
            acFileName[uiWrite] = pcName[st];
            uiWrite++;
        }
    }

    // Make sure we have a terminator.
    acFileName[uiWrite] = '\0';

#if _MSC_VER >= 1400
    m_bGood = (fopen_s(&m_pFile, acFileName, pcMode) == 0 && m_pFile != NULL);
#else //#if _MSC_VER >= 1400
    m_pFile = fopen(acFileName, pcMode);
    m_bGood = (m_pFile != NULL);
#endif //#if _MSC_VER >= 1400

    m_uiBufferAllocSize = uiBufferSize;
    m_uiPos = m_uiBufferReadSize = 0;

    if (m_bGood && uiBufferSize > 0)
    {
        m_pBuffer = NiAlloc(char, m_uiBufferAllocSize);
        NIASSERT(m_pBuffer != NULL);
    }
    else
    {
        m_pBuffer = NULL;
    }
}

//---------------------------------------------------------------------------
NiFile::~NiFile()
{
    if (m_bGood && m_pFile)
    {
        Flush();
        fclose(m_pFile);
    }

    NiFree(m_pBuffer);
}

//---------------------------------------------------------------------------
void NiFile::Seek(int iOffset, int iWhence)
{
    NIASSERT(iWhence == ms_iSeekSet || iWhence == ms_iSeekCur ||
        iWhence == ms_iSeekEnd);
    NIASSERT(m_eMode != APPEND_ONLY);

    if (m_bGood)
    {
#ifdef NIDEBUG
        unsigned int uiNewPos = (int)m_uiAbsoluteCurrentPos + iOffset;
#endif
        if (iWhence == ms_iSeekCur)
        {
            // If we can accomplish the Seek by adjusting m_uiPos, do so.

            int iNewPos = (int)m_uiPos + iOffset;
            if (iNewPos >= 0 && iNewPos < (int)m_uiBufferReadSize)
            {
                m_uiPos = iNewPos;
                m_uiAbsoluteCurrentPos = (int)m_uiAbsoluteCurrentPos +
                    iOffset;
                return;
            }

            // User's notion of current file position is different from
            // actual file position because of bufferring implemented by
            // this class. Make appropriate adjustment to offset.

            if (NiFile::READ_ONLY == m_eMode)
                iOffset -= (m_uiBufferReadSize - m_uiPos);
        }

        Flush();

        m_bGood = (fseek(m_pFile, iOffset, iWhence) == 0);
        if (m_bGood)
        {
            m_uiAbsoluteCurrentPos = ftell(m_pFile);
#ifdef NIDEBUG
            if (iWhence == ms_iSeekCur)
            {
                NIASSERT(uiNewPos == m_uiAbsoluteCurrentPos);
            }
            else if (iWhence == ms_iSeekSet)
            {
                NIASSERT((int)m_uiAbsoluteCurrentPos == iOffset);
            }
#endif
        }
    }
}

//---------------------------------------------------------------------------
unsigned int NiFile::FileRead(void* pBuffer, unsigned int uiBytes)
{
    NIASSERT(m_eMode == READ_ONLY);

    if (m_bGood)
    {
        unsigned int uiAvailBufferBytes, uiRead;

        uiRead = 0;
        uiAvailBufferBytes = m_uiBufferReadSize - m_uiPos;
        if (uiBytes > uiAvailBufferBytes)
        {
            if (uiAvailBufferBytes > 0)
            {
                NiMemcpy(pBuffer, &m_pBuffer[m_uiPos], uiAvailBufferBytes);
                pBuffer = &(((char*)pBuffer)[uiAvailBufferBytes]);
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

        NiMemcpy(pBuffer, &m_pBuffer[m_uiPos], uiBytes);
        m_uiPos += uiBytes;
        return uiRead + uiBytes;
    }
    else
    {
        return 0;
    }
}

//---------------------------------------------------------------------------
unsigned int NiFile::FileWrite(const void* pBuffer, unsigned int uiBytes)
{
    NIASSERT(m_eMode != READ_ONLY);
    NIASSERT(uiBytes != 0);

    if (m_bGood)
    {
        unsigned int uiAvailBufferBytes, uiWrite;

        uiWrite = 0;
        uiAvailBufferBytes = m_uiBufferAllocSize - m_uiPos;
        if (uiBytes > uiAvailBufferBytes)
        {
            if (uiAvailBufferBytes > 0)
            {
                NiMemcpy(&m_pBuffer[m_uiPos], pBuffer, uiAvailBufferBytes);
                pBuffer = &(((char*)pBuffer)[uiAvailBufferBytes]);
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

        NiMemcpy(&m_pBuffer[m_uiPos], pBuffer, uiBytes);
        m_uiPos += uiBytes;
        return uiWrite + uiBytes;
    }
    else
    {
        return 0;
    }
}

//---------------------------------------------------------------------------
unsigned int NiFile::DiskWrite(const void* pBuffer, unsigned int uiBytes)
{
    return static_cast<unsigned int>(
        fwrite(pBuffer, 1, (size_t)uiBytes, m_pFile));
}

//---------------------------------------------------------------------------
unsigned int NiFile::DiskRead(void* pBuffer, unsigned int uiBytes)
{
    return static_cast<unsigned int>(
        fread(pBuffer, 1, (size_t)uiBytes, m_pFile));
}

//---------------------------------------------------------------------------
bool NiFile::Flush()
{
    NIASSERT(m_bGood);

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
    }

    m_uiPos = 0;
    return true;
}

//---------------------------------------------------------------------------
unsigned int NiFile::GetFileSize() const
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
//---------------------------------------------------------------------------
bool NiFile::DefaultCreateDirectoryFunc(const char* pcDirName)
{
    bool bCreateDir = ::CreateDirectoryA(pcDirName, NULL) != 0;

#ifdef NIDEBUG
    if (bCreateDir == false)
    {
        NiOutputDebugString("Create Dir Failed:\n");
        NiOutputDebugString("\tDirectory: \"");
        NiOutputDebugString(pcDirName);
        NiOutputDebugString("\"\n");

        char acString[1024];
        DWORD dwLastError = ::GetLastError();
        NiSprintf(acString, 1024, "\tErrorCode %d\n\t"
            "Translation: ", dwLastError);
        NiOutputDebugString(acString);

        dwLastError = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
            dwLastError, 0, acString, 1024, NULL);
        NIASSERT(dwLastError != 0);
        NiOutputDebugString(acString);
        NiOutputDebugString("\n");
    }
#endif

    return bCreateDir;
}
//---------------------------------------------------------------------------
bool NiFile::DefaultDirectoryExistsFunc(const char* pcDirName)
{
    DWORD dwAttrib = GetFileAttributes(pcDirName);
    if (dwAttrib == -1)
        return false;

    return (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
//---------------------------------------------------------------------------
