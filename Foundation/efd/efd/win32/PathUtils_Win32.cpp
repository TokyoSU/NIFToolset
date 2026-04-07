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

#include "efdPCH.h"

#include <efd/PathUtils.h>

using namespace efd;

//--------------------------------------------------------------------------------------------------
const char* PathUtils::StripAbsoluteBase(const char* pcAbsolutePath)
{
    EE_ASSERT(pcAbsolutePath && !PathUtils::IsRelativePath(pcAbsolutePath));

    // Absolute base is one of the following:
    if (Strnicmp(pcAbsolutePath, "\\\\", 2) == 0)
    {
        // [1] Double-slash + network path + slash
        pcAbsolutePath = strchr(pcAbsolutePath+2, EE_PATH_DELIMITER_CHAR);
        EE_ASSERT(pcAbsolutePath);
        pcAbsolutePath++;
    }
    else if (Strnicmp(pcAbsolutePath, "\\", 1) == 0)
    {
        // [2] a single "\" can be followed by a file name.
        pcAbsolutePath++;
    }
    else
    {
        // [3] A drive letter followed by ":" followed [optionally] by a slash
        EE_ASSERT(strlen(pcAbsolutePath) >= 2);
        char c1stChar = static_cast<char>(toupper(pcAbsolutePath[0]));
        char c2ndChar = pcAbsolutePath[1];
        if (c2ndChar == ':' && (c1stChar >= 'A' && c1stChar <= 'Z'))
        {
            pcAbsolutePath += 2;
            if (pcAbsolutePath[0] == EE_PATH_DELIMITER_CHAR)
                pcAbsolutePath++;
        }
        else
        {
            EE_ASSERT(!"PathUtils::StripAbsoluteBase error.");
        }
    }

    return pcAbsolutePath;

}


//--------------------------------------------------------------------------------------------------
bool PathUtils::IsRelativePath(const efd::utf8string& i_strPath)
{
    return IsRelativePath(i_strPath.c_str());
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::IsRelativePath(const efd::Char* pCStr)
{
    // An absolute path is one that begins with either of the following:
    // [1] a forward or backward slash
    // [2] A drive letter followed by ":"

    size_t stLen = strlen(pCStr);
    if (stLen < 2)
    {
        // the smallest absolute path is slash followed by letter, so
        // this must be a relative path.
        return true;
    }

    // check for case 1
    char c1stChar = pCStr[0];
    if (c1stChar == '\\' || c1stChar == '/')
    {
        // test for case 1 indicates this is an absolute path
        return false;
    }

    // check for case 2
    c1stChar = static_cast<char>(toupper(c1stChar));
    char c2ndChar = pCStr[1];
    if (c2ndChar == ':' && (c1stChar >= 'A' && c1stChar <= 'Z'))
    {
        // test for case 2 indicates this is an absolute path
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::GetWorkingDirectory()
{
    efd::Char buffer[ EE_MAX_PATH ];
    return efd::Getcwd(buffer, EE_ARRAYSIZEOF(buffer));
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::GetWorkingDirectory(efd::Char* pcPath, size_t stDestSize)
{
    return (efd::Getcwd(pcPath, (int)stDestSize) != NULL);
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::GetExecutableDirectory(efd::Char* pcPath, size_t stDestSize)
{
    size_t stWrittenChars = GetModuleFileName(GetModuleHandle(NULL), pcPath,
        (DWORD)stDestSize);

    if (stWrittenChars != NULL && stWrittenChars != stDestSize)
    {
        char* pcLastDirSlash = strrchr(pcPath, '\\');
        if (pcLastDirSlash == NULL)
            pcLastDirSlash = strrchr(pcPath, '/');

        if (pcLastDirSlash)
        {
            pcLastDirSlash[1] = '\0';
            Standardize(pcPath);
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::GetDefaultLogDirectory(efd::Char* pcPath, size_t stDestSize)
{
    return GetExecutableDirectory(pcPath, stDestSize);
}

//--------------------------------------------------------------------------------------------------
bool PathUtils::PathContainsDrive(const utf8string& i_strPath)
{
    if (i_strPath.length() >= 2)
    {
        if (i_strPath[1] == ':')
        {
            char firstCharacter = i_strPath[0].ToAscii();
            if ((firstCharacter >= 'a' && firstCharacter <= 'z') ||
                (firstCharacter >= 'A' && firstCharacter <= 'Z'))
            {
                return true;
            }
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::IsUNCPath(const utf8string& i_strPath)
{
    // A UNC style path has the form \\server\share.  For our purposes any path that starts
    // with two slashes will be considered UNC-style.
    if (i_strPath.length() >= 2)
    {
        if (IsPathSeperator(i_strPath[0]) && IsPathSeperator(i_strPath[1]))
        {
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
efd::Bool PathUtils::GetLastModifed(const efd::utf8string& path,
                                    efd::PathUtils::FileTimestamp* modified)
{
    efd::Bool retval = false;

    modified->HiBits = (UInt32)0;
    modified->LoBits = (UInt32)0;

    // Convert our UTF8 path to a WCHAR
    WCHAR widePath[MAX_PATH];
    utf8string path_native = PathUtils::PathMakeNative (path);

    int count = MultiByteToWideChar(
        CP_UTF8,
        0,
        path_native.c_str(),
        (int)path_native.length(),
        widePath,
        MAX_PATH);
    widePath[count] = 0;

    HANDLE h = CreateFileW(
        widePath,             //file to open
        GENERIC_READ,         //open for reading
        FILE_SHARE_READ,      //share for reading
        NULL,                 //default security
        OPEN_EXISTING,        //existing file only
        FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
        NULL);                //no attribute template

    if (h != INVALID_HANDLE_VALUE)
    {
        FILETIME ftWrite;
        if (GetFileTime(h, NULL, NULL, &ftWrite))
        {
            modified->HiBits = ftWrite.dwHighDateTime;
            modified->LoBits = ftWrite.dwLowDateTime;
            retval = true;
        }
        CloseHandle (h);
    }

    return retval;
}