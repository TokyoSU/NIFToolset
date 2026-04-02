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

#include <efd/Win32/Win32File.h>
#include <efd/efdLogIDs.h>
#include <efd/ILogger.h>
#include <efd/PathUtils.h>

namespace efd
{

//--------------------------------------------------------------------------------------------------
File* File::DefaultFileCreateFunc(const char *pcName, OpenMode eMode,
    unsigned int uiSize, bool flushOnWrite)
{
    File* pkFile = EE_NEW Win32File(pcName, eMode, uiSize, flushOnWrite);

    if (pkFile->IsGood())
    {
        return pkFile;
    }
    else
    {
        EE_DELETE pkFile;
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
bool File::DefaultFileAccessFunc(const char *pcName, OpenMode eMode)
{
    DWORD desiredAccess = 0;
    DWORD shareMode = 0;

    if (eMode == efd::File::READ_ONLY ||
        eMode == efd::File::READ_ONLY_TEXT ||
        eMode == efd::File::READ_ONLY_ENCRYPTED)
    {
        desiredAccess = GENERIC_READ;
        shareMode = FILE_SHARE_READ;
    }
    else
    {
        desiredAccess = GENERIC_READ | GENERIC_WRITE;
    }

    // Check for environment variables.
    char acFileName[EE_MAX_PATH];
    DWORD bufSize = ::ExpandEnvironmentStringsA(pcName, acFileName, EE_MAX_PATH);

    if (bufSize >= EE_MAX_PATH)
    {
        // Filename too large for buffer.
        return false;
    }

    // Make sure we have a terminator.
    acFileName[bufSize] = '\0';

    // Check security permissions
    HANDLE h = ::CreateFileA(
        acFileName, 
        desiredAccess, 
        shareMode, 
        NULL,               // Security attributes
        OPEN_EXISTING,      // Try to open only if the file exists, otherwise fail.
        FILE_ATTRIBUTE_NORMAL,
        NULL               // Template file
        );

    if (h == INVALID_HANDLE_VALUE)
    {
        // Check to see if file doesn't exist and we are wanting to create it.
        if (desiredAccess & GENERIC_WRITE &&
            (::GetLastError() == ERROR_FILE_NOT_FOUND ||
            ::GetLastError() == ERROR_PATH_NOT_FOUND))
        {
            return true;
        }

        // permission denied or file doesn't exist.
        return false;
    }

    ::CloseHandle(h);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool File::DefaultCreateDirectoryFunc(const char* pcDirName)
{
    bool bCreateDir = ::CreateDirectory(pcDirName, NULL) != 0;

#ifdef EE_EFD_CONFIG_DEBUG
    if (bCreateDir == false)
    {
        DWORD dwLastError = ::GetLastError();

        EE_LOG(efd::kFile, ILogger::kLVL3,
            ("Create Dir Failed:\n\tDirectory: '%s'", pcDirName));

        char acString[1024];
        EE_VERIFY(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
            dwLastError, 0, acString, 1024, NULL));
        EE_LOG(efd::kFile, ILogger::kLVL3,
            ("\tErrorCode %d\tTranslation: %s", dwLastError, acString));
    }
#endif

    return bCreateDir;
}

//--------------------------------------------------------------------------------------------------
bool File::DefaultDirectoryExistsFunc(const char* pcDirName)
{
   DWORD dwAttrib = GetFileAttributes(pcDirName);
   if (dwAttrib == -1)
       return false;

   return (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

//--------------------------------------------------------------------------------------------------

} // end namespace efd
