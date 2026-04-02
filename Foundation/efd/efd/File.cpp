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
#include <efd/File.h>

namespace efd
{

//--------------------------------------------------------------------------------------------------
File::FILECREATEFUNC File::ms_pfnFileCreateFunc = File::DefaultFileCreateFunc;
File::FILEACCESSFUNC File::ms_pfnFileAccessFunc = File::DefaultFileAccessFunc;
File::CREATEDIRFUNC File::ms_pfnCreateDirFunc = File::DefaultCreateDirectoryFunc;
File::DIREXISTSFUNC File::ms_pfnDirExistsFunc = File::DefaultDirectoryExistsFunc;
bool File::ms_EncryptionCheckEnabled = false;

//--------------------------------------------------------------------------------------------------
File* File::GetFile(const char *pcName, OpenMode eMode, unsigned int uiSize, bool flushOnWrite)
{
    return ms_pfnFileCreateFunc(pcName, eMode, uiSize, flushOnWrite);
}

//--------------------------------------------------------------------------------------------------
File::File()
{
}

//--------------------------------------------------------------------------------------------------
File::~File()
{
}

//--------------------------------------------------------------------------------------------------
void File::SetFileCreateFunc(FILECREATEFUNC pfnFunc)
{
    ms_pfnFileCreateFunc = (pfnFunc == NULL) ? DefaultFileCreateFunc : pfnFunc;
}

//--------------------------------------------------------------------------------------------------
bool File::Access(const char *pcName, OpenMode eMode)
{
    return ms_pfnFileAccessFunc(pcName, eMode);
}

//--------------------------------------------------------------------------------------------------
void File::SetFileAccessFunc(FILEACCESSFUNC pfnFunc)
{
    ms_pfnFileAccessFunc = (pfnFunc == NULL) ? DefaultFileAccessFunc : pfnFunc;
}

//--------------------------------------------------------------------------------------------------
bool File::CreateDirectory(const char* pcDirName)
{
    return ms_pfnCreateDirFunc(pcDirName);
}

//--------------------------------------------------------------------------------------------------
void File::SetCreateDirectoryFunc(CREATEDIRFUNC pfnFunc)
{
    ms_pfnCreateDirFunc = (pfnFunc == NULL) ? DefaultCreateDirectoryFunc :
        pfnFunc;
}

//--------------------------------------------------------------------------------------------------
bool File::DirectoryExists(const char* pcDirName)
{
    return ms_pfnDirExistsFunc(pcDirName);
}

//--------------------------------------------------------------------------------------------------
void File::SetDirectoryExistsFunc(DIREXISTSFUNC pfnFunc)
{
    ms_pfnDirExistsFunc = (pfnFunc == NULL) ? DefaultDirectoryExistsFunc :
        pfnFunc;
}

//--------------------------------------------------------------------------------------------------
bool File::CreateDirectoryRecursive(const char* pcFullPath)
{
    if (DirectoryExists(pcFullPath))
        return true;

    if (strlen(pcFullPath) > EE_MAX_PATH)
        return false;

    char acFullPathCopy[EE_MAX_PATH];

    efd::Strcpy(acFullPathCopy, EE_MAX_PATH, pcFullPath);
    efd::PathUtils::Standardize(acFullPathCopy);

    unsigned int uiStart = 0;

    // Check for drive start path
    if (acFullPathCopy[uiStart + 1] == ':')
    {
        uiStart += 2;
    }

    // Consume the leading slash characters
    while (uiStart < EE_MAX_PATH &&
        (acFullPathCopy[uiStart] == '\\' || acFullPathCopy[uiStart] == '/'))
    {
        uiStart++;
    }

    // Search through the string buffer for any '\\' or '/' and
    // make sure that the directory exists. If not, create it.
    bool bDealtWithNetworkPath = false;
    for (unsigned int ui = uiStart; ui < EE_MAX_PATH; ui++)
    {
        char cCurChar = acFullPathCopy[ui];
        if ((cCurChar == '/' || cCurChar == '\\')
#ifdef EE_PLATFORM_PS3
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
                EE_ASSERT(DirectoryExists(acFullPathCopy));
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
        EE_ASSERT(DirectoryExists(acFullPathCopy));
    }

    EE_ASSERT(DirectoryExists(pcFullPath));
    return true;
}

//--------------------------------------------------------------------------------------------------
bool File::Seek(int iNumBytes)
{
    return Seek(iNumBytes, SO_CURRENT);
}

//-------------------------------------------------------------------------------------------------
void File::SetEncryptionCheckEnabled(bool enabled)
{
    if (ms_EncryptionCheckEnabled && !enabled)
    {
        EE_FAIL("Once enabled, encryption may not be disabled");
        return;
    }
    ms_EncryptionCheckEnabled = enabled;
}

//-------------------------------------------------------------------------------------------------
bool File::GetEncryptionCheckEnabled()
{
    return ms_EncryptionCheckEnabled;
}

//-------------------------------------------------------------------------------------------------

} // end namespace efd
