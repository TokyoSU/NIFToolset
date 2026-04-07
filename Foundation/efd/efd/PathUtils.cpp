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
#include <algorithm>

using namespace efd;

namespace efd
{
    // Given an absolute path this splits it into a "base" and "remainder".
    // Examples:
    //  "\\server\share" => { "\\server", "share" }
    //  "c:\directory" => { "c:", "directory" }
    //  "c:directory" => { "c:", "directory" }
    //  "\directory" => { "\", "directory" }
    //---------------------------------------------------------------------------------------------
    efd::utf8string SplitAbsoluteBase(efd::utf8string& io_strPath)
    {
        efd::utf8string basePath;

        // Case 1: "\\server\share"
        if (PathUtils::IsUNCPath(io_strPath))
        {
            size_t offset = io_strPath.find_first_of("\\/", 2);
            EE_ASSERT(offset > 2);

            // We want to include the found character so advance the offset by one:
            ++offset;

            basePath = io_strPath.substr(0, offset);
            io_strPath = io_strPath.substr(offset, static_cast<size_t>(-1));
        }

        // Case 2: "\whatever"
        else if (!io_strPath.empty() && PathUtils::IsPathSeperator(io_strPath[0]))
        {
            basePath = PathUtils::GetNativePathSeperator();
            io_strPath = io_strPath.substr(1, static_cast<size_t>(-1));
        }

        // Case 3: "c:whatever"
        else if (PathUtils::PathContainsDrive(io_strPath))
        {
            size_t offset = io_strPath.find_first_of(':');
            EE_ASSERT(offset > 0);

            // We want to include the found character so advance the offset by one:
            ++offset;

            bool addSlashToBase = true;
            if (io_strPath.length() > offset + 1)
            {
                if (PathUtils::IsPathSeperator(io_strPath[ offset]))
                {
                    addSlashToBase = false;
                    ++offset;
                }
            }

            basePath = io_strPath.substr(0, offset);
            if (addSlashToBase)
            {
                basePath.append(PathUtils::GetNativePathSeperator());
            }

            io_strPath = io_strPath.substr(offset, static_cast<size_t>(-1));
        }

        return basePath;
    }


//--------------------------------------------------------------------------------------------------
    size_t FindFirstDifference(const efd::utf8string& i_strOne,
        const efd::utf8string& i_strTwo)
    {
        int length = EE_STL_NAMESPACE::min(i_strOne.length(), i_strTwo.length());

        for (int i=0; i<length; ++i)
        {
            if (i_strOne[i] != i_strTwo[i])
            {
                return i;
            }
        }

        // We return -1 when the strings are completely identical, but if one string is longer
        // than the other then the null character for the shorter string is treated as the first
        // difference.
        if (i_strOne.length() == i_strTwo.length())
        {
            return static_cast<size_t>(-1);
        }

        return length;
    }

} // end namespace efd;


//--------------------------------------------------------------------------------------------------
bool PathUtils::IsPathSeperator(const utf8char_t& i_ch)
{
    // All path separators are ASCII and anything non-ASCII will be converted to a '?' character
    // which is not a path separator so this should simplify things:
    char ch = i_ch.ToAscii();
    return ch == GetNativePathSeperator() || ch == GetNonNativePathSeperator();
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::IsExtensionSeperator(const utf8char_t& i_ch)
{
    // All separators are ASCII and anything non-ASCII will be converted to a '?' character
    // which is not a separator so this should simplify things:
    char ch = i_ch.ToAscii();
    return ch == '.';
}

//--------------------------------------------------------------------------------------------------
bool PathUtils::IsAbsolutePath(const efd::utf8string& i_strPath)
{
    return IsAbsolutePath(i_strPath.c_str());
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::IsAbsolutePath(const efd::Char* pCStr)
{
    if (IsRelativePath(pCStr))
        return false;

    // Strip off absolute base
    pCStr = StripAbsoluteBase(pCStr);

    // After absolute base, are there any occurrences of dotdots?
    const char* pcRelative = strstr(pCStr, "..");
    if (pcRelative)
        return false;

    return true;
}


//--------------------------------------------------------------------------------------------------
EE_EFD_ENTRY bool PathUtils::IsStandarizedAbsolutePath(const efd::utf8string& i_strPath)
{
    return IsStandarizedAbsolutePath(i_strPath.c_str());
}


//--------------------------------------------------------------------------------------------------
EE_EFD_ENTRY bool PathUtils::IsStandarizedAbsolutePath(const efd::Char* pCStr)
{
    if (!IsAbsolutePath(pCStr))
        return false;

    EE_ASSERT(strlen(pCStr) + 1 < EE_MAX_PATH);
    char acTestStandardize[EE_MAX_PATH];
    Strcpy(acTestStandardize, EE_MAX_PATH, pCStr);
    bool bStandardized = PathUtils::Standardize(acTestStandardize);

    return (!bStandardized);
}


//--------------------------------------------------------------------------------------------------
utf8string PathUtils::PathMakeNative(const utf8string& i_strPath)
{
    utf8string result = i_strPath;

    efd::Char find[2] = { GetNonNativePathSeperator(), '\0' };
    efd::Char replace[2] = { GetNativePathSeperator(), '\0' };

    result.replace_substr(find, replace);

    return result;
}


//--------------------------------------------------------------------------------------------------
utf8string PathUtils::PathMakeNonNative(const utf8string& i_strPath)
{
    utf8string result = i_strPath;

    efd::Char find[2] = { GetNativePathSeperator(), '\0' };
    efd::Char replace[2] = { GetNonNativePathSeperator(), '\0' };

    result.replace_substr(find, replace);

    return result;
}


//--------------------------------------------------------------------------------------------------
utf8string PathUtils::PathCombine(const utf8string& i_strRoot, const utf8string& i_strMore)
{
    utf8string result = i_strRoot;

    // We treat empty strings like they have a trailing slash so as not to add a slash that
    // would imply "root dir" instead of "current dir".  This means:
    //      "" + "filename" -> "filename"
    //      "/" + "filename" -> "/filename"
    //      "" + "/filename" -> "/filename"

    bool hasTrailingSlash = result.empty() || IsPathSeperator(result[ result.length() - 1 ]);
    bool hasLeadingSlash = i_strMore.empty() || IsPathSeperator(i_strMore[0]);

    // Avoiding adding a slash if the more string already starts with one.
    if (!hasTrailingSlash && !hasLeadingSlash)
    {
        // We know the strings aren't empty so no special case empty string handling is needed
        EE_ASSERT(!result.empty());
        EE_ASSERT(!i_strMore.empty());

        result += GetNativePathSeperator();
    }
    else if (hasTrailingSlash && hasLeadingSlash)
    {
        // If both strings have a slash then we need to remove one of the slashes, unless we
        // only think it has a slash because the string is really empty.
        if (!result.empty() && !i_strMore.empty())
        {
            result = result.substr(0, result.length() - 1);
        }
    }

    result += i_strMore;
    return result;
}

//--------------------------------------------------------------------------------------------------
utf8string PathUtils::PathGetExtension(const efd::utf8string& i_strPath)
{
    utf8string result;
    size_t offset = i_strPath.find_last_of(".\\/");
    if (utf8string::npos != offset && i_strPath[offset] == '.')
    {
        // If we found an extension separator after the last path separator then the
        // result is everything after the separator:
        result = i_strPath.substr(offset + 1);
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
utf8string PathUtils::PathAddExtension(const utf8string& i_strRoot, const utf8string& i_strExt)
{
    utf8string result = i_strRoot;

    bool hasLeadingDot = !i_strExt.empty() && IsExtensionSeperator(i_strExt[ 0 ]);

    if (!hasLeadingDot)
    {
        result += '.';
    }

    result += i_strExt;
    return result;
}




//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::PathRemoveFileExtension(const efd::utf8string& i_strPath)
{
    utf8string result;
    size_t offset = i_strPath.find_last_of(".\\/");
    if (utf8string::npos != offset && i_strPath[offset] == '.')
    {
        // If we found an extension separator after the last path seperator then the
        // result is everything up to that seperator:
        result = i_strPath.substr(0, offset);
    }
    else
    {
        result = i_strPath;
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::PathRemoveFileName(const efd::utf8string& i_strPath)
{
    // If no path seperator is found we will return an empty string
    utf8string result;
    size_t offset = i_strPath.find_last_of("\\/");
    if (utf8string::npos != offset)
    {
        // If we found a path separator then the result is everything up to that separator:
        result = i_strPath.substr(0, offset);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::PathGetFileName(const efd::utf8string& i_strPath)
{
    utf8string result;
    size_t offset = i_strPath.find_last_of("\\/");
    if (utf8string::npos != offset)
    {
        // If we found a path separator then the result is everything after that separator:
        result = i_strPath.substr(offset+1);
    }
    else
    {
        result = i_strPath;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::PathRemoveSlashSlash(const efd::utf8string& i_strPath)
{
    efd::Char find[] = { GetNativePathSeperator(), GetNativePathSeperator(), '\0' };
    efd::Char replace[] = { GetNativePathSeperator(), '\0' };

    bool isUNCPath = IsUNCPath(i_strPath);

    efd::utf8string result = i_strPath;
    result.replace_substr(find, replace);

    if (isUNCPath)
    {
        // its easiest to blindly replace all \\ with \ and then re-add the leading \ if we
        // started with a UNC path:
        result = GetNativePathSeperator() + result;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::PathRemoveSlashDotSlash(const efd::utf8string& i_strPath)
{
    efd::Char find[] = { GetNativePathSeperator(), '.', GetNativePathSeperator(), '\0' };
    efd::Char replace[] = { GetNativePathSeperator(), '\0' };

    // Convert to native slashes so we can handle cases like:
    //      "foo\./bar" => "foo\bar"
    efd::utf8string result = PathMakeNative(i_strPath);

    // Use the overlapping replace sub-string in order to handle cases like:
    //      "foo\.\.\bar" => "foo\bar"
    result.ol_replace_substr(find, replace);

    return result;
}


//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::PathRemoveDotDots(const efd::utf8string& i_strPath)
{
    efd::utf8string result;

    if (IsUNCPath(i_strPath))
    {
        efd::Char slashslash[] = { GetNativePathSeperator(), GetNativePathSeperator(), '\0' };
        result = slashslash;
    }
    else if (IsPathSeperator(i_strPath[0]))
    {
        result = GetNativePathSeperator();
    }

    efd::vector<efd::utf8string> components;

    // If there aren't at least two components then there's nothing for us to do.  The shortest
    // path we can simplify is "something/.." => "".
    if (i_strPath.split("\\/", components) >= 2)
    {
        efd::vector< efd::utf8string >::iterator iterPrevious = components.begin();
        efd::vector< efd::utf8string >::iterator iterCurrent = components.begin();
        ++iterCurrent;

        while (iterCurrent != components.end())
        {
            bool currIsParentDir = (*iterCurrent == "..");
            bool prevIsRealDir = (*iterPrevious != "..");

            if (currIsParentDir && prevIsRealDir)
            {
                // When erasing the head I cannot use --iter so treat this special:
                if (iterPrevious == components.begin())
                {
                    components.erase(iterCurrent);
                    components.erase(iterPrevious);

                    iterPrevious = components.begin();
                    if (iterPrevious == components.end())
                    {
                        break;
                    }
                }
                else
                {
                    // remove the previous and the current from the vector
                    efd::vector<efd::utf8string>::iterator iterToRemove = iterPrevious;
                    --iterPrevious;

                    components.erase(iterCurrent);
                    components.erase(iterToRemove);
                }

                iterCurrent = iterPrevious;
                ++iterCurrent;
            }
            else
            {
                iterPrevious = iterCurrent;
                ++iterCurrent;
            }
        }
    }

    // Now we just stick the remaining pieces together again:
    for (efd::vector< efd::utf8string >::iterator iter = components.begin();
          iter != components.end();
          ++iter)
    {
        result = PathCombine(result, *iter);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::ConvertToRelative(const efd::utf8string& i_strAbsolutePath,
                                   efd::utf8string& o_result)
{
    return ConvertToRelative(i_strAbsolutePath, GetWorkingDirectory(), o_result);
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::ConvertToRelative(const efd::utf8string& i_strAbsolutePath,
                                  const efd::utf8string& i_strRelativeToHere,
                                  efd::utf8string& o_result)
{
    char relative[EE_MAX_PATH];

    size_t s = ConvertToRelative(
        relative, 
        EE_MAX_PATH, 
        i_strAbsolutePath.c_str(), 
        i_strRelativeToHere.c_str());

    bool rval = (s != 0);

    // The original utf8string implementation of this function did not prepend a "./" to the 
    // result but the char* version does. Remove this "./" to maintain backward compatibility.
    // If we have a result it is guaranteed to have this value prepended to it.
    if (rval)
    {
        o_result = &relative[strlen("." EE_PATH_DELIMITER_STR)];
    }
    else
    {
        o_result = "";
    }

    return rval;
}


//--------------------------------------------------------------------------------------------------
bool PathUtils::Standardize(char* pcPath)
{
    bool bModified = false;
    if (!pcPath)
        return bModified;

    size_t stLen = strlen(pcPath);
    bool bLastCharWasSlash = false;

    unsigned int uiWrite = 0;
    for (unsigned int uiRead = 0; uiRead < stLen; uiRead++)
    {
        char c = pcPath[uiRead];

        //  convert NI_PATH_DELIMITER_INCORRECT_CHAR to NI_PATH_DELIMITER_CHAR
        if (c == EE_PATH_DELIMITER_INCORRECT_CHAR)
        {
            bModified = true;
            c = EE_PATH_DELIMITER_CHAR;
        }

        bool bThisCharIsSlash = (c == EE_PATH_DELIMITER_CHAR);

        // Drop duplicate slashes
        if (bThisCharIsSlash && bLastCharWasSlash)
        {
            // Unless this is a network resource name, e.g. "\\share"
            // which is true only in first two characters of string.
            if (uiRead > 1)
            {
                bModified = true;
                continue; // skip this char
            }
        }

        bLastCharWasSlash = bThisCharIsSlash;

        // write char
        pcPath[uiWrite++] = c;
    }

    // Terminate (string may have shrunk)
    pcPath[uiWrite] = 0;
    return bModified;
}


//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::ConvertToAbsolute(const efd::utf8string& i_strRelativePath)
{
    return ConvertToAbsolute(i_strRelativePath, GetWorkingDirectory());
}


//--------------------------------------------------------------------------------------------------
efd::utf8string PathUtils::ConvertToAbsolute(
    const efd::utf8string& i_strRelativePath,
    const efd::utf8string& i_strRelativeToHere)
{
    if (IsAbsolutePath(i_strRelativePath))
        return i_strRelativePath;

    EE_ASSERT(IsAbsolutePath(i_strRelativeToHere));

    efd::utf8string result = PathCombine(i_strRelativeToHere, i_strRelativePath);
    result = PathRemoveSlashDotSlash(result);
    return PathRemoveDotDots(result);
}


//--------------------------------------------------------------------------------------------------
char* PathUtils::StripAbsoluteBase(char* pcAbsolutePath)
{
    return const_cast<char*>(
        StripAbsoluteBase(const_cast<const char*>(pcAbsolutePath)));
}


//--------------------------------------------------------------------------------------------------
size_t PathUtils::ConvertToRelative(
    efd::Char* pcRelativePath,
    size_t stRelBytes,
    const efd::Char* pcAbsolutePath,
    const efd::Char* pcRelativeToHere)
{
    EE_ASSERT(Strlen(pcAbsolutePath) + 1 < EE_MAX_PATH);
    EE_ASSERT(Strlen(pcRelativeToHere) + 1 < EE_MAX_PATH);

    // This function takes pcAbsolutePath and converts it to an relative path.
    // The relative path is stored in pcRelativePath which is assumed to be
    // allocated with stRelBytes. The function returns the number of bytes
    // written.
    //
    // The function fails if:
    // - acRelativePath is too small (stRelBytes) to hold acAbsolutePath
    // - acAbsolutePath is larger than NI_MAX_PATH
    // - acAbsolutePath and acRelativeToHere do not have a common root

    EE_ASSERT(pcAbsolutePath && !IsRelativePath(pcAbsolutePath));
    EE_ASSERT(pcRelativeToHere && !IsRelativePath(pcRelativeToHere));

    size_t stAbsBytes = Strlen(pcAbsolutePath);
    if (stRelBytes <= stAbsBytes)
        return 0;
    if (stAbsBytes >= EE_MAX_PATH)
        return 0;

    // First, remove dotdots and standardize paths...
    char acAbsolutePath[EE_MAX_PATH];
    char acRelativeToHere[EE_MAX_PATH];
    Strcpy(acAbsolutePath, EE_MAX_PATH, pcAbsolutePath);
    Strcpy(acRelativeToHere, EE_MAX_PATH, pcRelativeToHere);

    // acRelativeToHere is a directory. If there is no ending delimiter,
    // one is added. This is done for the case where acRelativeToHere IS
    // the absolute base and we want to ensure that our comparison of
    // dtRoot1Size against dtRoot2Size holds true.
    size_t stEndChar = Strlen(acRelativeToHere) - 1;
    EE_ASSERT(stEndChar < EE_MAX_PATH);
    if (acRelativeToHere[stEndChar] != EE_PATH_DELIMITER_CHAR)
    {
        Strcat(acRelativeToHere, EE_MAX_PATH, EE_PATH_DELIMITER_STR);
    }

    PathUtils::RemoveDotDots(acAbsolutePath);
    PathUtils::RemoveDotDots(acRelativeToHere);

    // From here on out, only the local copies are used.

    // search for a common root
    const char* pcAbsoluteNoRoot = StripAbsoluteBase(acAbsolutePath);
    const char* pcRelativeNoRoot = StripAbsoluteBase(acRelativeToHere);
    ptrdiff_t dtRoot1Size = pcAbsoluteNoRoot - acAbsolutePath;
    ptrdiff_t dtRoot2Size = pcRelativeNoRoot - acRelativeToHere;

    if (dtRoot1Size != dtRoot2Size)
        return 0;

    if (Strnicmp(acAbsolutePath, acRelativeToHere, dtRoot1Size) != 0)
        return 0;

    // common root found, now construct the relative path
    // always start with "./"
    // acSubString contains the common portion in both
    // absolute & relative strings
    char acSubString[EE_MAX_PATH];
    Strcpy(pcRelativePath, stRelBytes, "." EE_PATH_DELIMITER_STR);
    Strcpy(acSubString, EE_MAX_PATH, pcRelativeNoRoot);

    // Remove trailing delimiter if there is one
    size_t stLenOfCommon = Strlen(acSubString);
    if (stLenOfCommon >= 1 &&
        acSubString[stLenOfCommon-1] == EE_PATH_DELIMITER_CHAR)
    {
        acSubString[stLenOfCommon-1] = '\0';
    }

    for (; ;)
    {
        stLenOfCommon = Strlen(acSubString);
        // Does pcAbsoluteNoRoot start with acSubString?  If so, done.
        if (Strnicmp(pcAbsoluteNoRoot, acSubString, stLenOfCommon) == 0)
            break;

        // append a ../ to relative path
        Strcat(pcRelativePath, stRelBytes, ".." EE_PATH_DELIMITER_STR);

        // Remove the trailing delimiter before looking for the next subdirectory.
        if (stLenOfCommon >= 1 &&
            acSubString[stLenOfCommon-1] == EE_PATH_DELIMITER_CHAR)
        {
            acSubString[stLenOfCommon-1] = '\0';
        }

        // remove this subdir from acSubString
        char* pcEndOfNextSubDir = strrchr(acSubString, EE_PATH_DELIMITER_CHAR);
        if (pcEndOfNextSubDir)
        {
            *(pcEndOfNextSubDir+1) = '\0';
        }
        else
        {
            // no more subdirs to remove, done.
            acSubString[0] = '\0';
            break;
        }
    }

    // append the remaining path and filename
    stLenOfCommon = Strlen(acSubString);
    if (pcAbsoluteNoRoot[stLenOfCommon] == EE_PATH_DELIMITER_CHAR)
    {
        // skip leading slash if there is one
        stLenOfCommon++;
    }
    Strcat(pcRelativePath, stRelBytes, &pcAbsoluteNoRoot[stLenOfCommon]);

    return Strlen(pcRelativePath);
}


//--------------------------------------------------------------------------------------------------
size_t PathUtils::ConvertToAbsolute(
    efd::Char* pcPath,
    size_t stPathBytes,
    const efd::Char* pcRelativeToHere)
{
    // This ConvertToAbsolute modifies pcPath in place.
    const size_t stMaximumPathBufferSize = EE_MAX_PATH * 2 + 2;
    char acAbsPath[stMaximumPathBufferSize];
    size_t uiAbsBytes = ConvertToAbsolute(acAbsPath, stMaximumPathBufferSize,
        pcPath, pcRelativeToHere);

    if (uiAbsBytes < stPathBytes)
    {
        Strcpy(pcPath, stPathBytes, acAbsPath);
        return uiAbsBytes;
    }
    else
    {
        pcPath[0] = 0;
        return 0;
    }

}


//--------------------------------------------------------------------------------------------------
size_t PathUtils::ConvertToAbsolute(efd::Char* pcPath, size_t stPathBytes)
{
    char acCWD[EE_MAX_PATH];
    if (!GetWorkingDirectory(acCWD, EE_MAX_PATH))
        return 0;
    return ConvertToAbsolute(pcPath, stPathBytes, acCWD);
}


//--------------------------------------------------------------------------------------------------
size_t PathUtils::ConvertToAbsolute(
    efd::Char* pcAbsolutePath,
    size_t stAbsBytes,
    const efd::Char* pcRelativePath,
    const efd::Char* pcRelativeToHere)
{
    // This function takes pcRelativePath and converts it to an absolute path
    // by concatenating it with pcRelativeToHere and removing dotdots.

    // The resulting absolute path is stored in pcAbsolutePath, which is
    // assumed to have been allocated with size stAbsBytes.
    // The function returns the number of bytes written.

    EE_ASSERT(pcAbsolutePath && pcRelativePath);
    EE_ASSERT(IsRelativePath(pcRelativePath));
    EE_ASSERT(pcAbsolutePath != pcRelativePath); // destination cannot be source

    // If pcRelativeToHere is null or an empty string, use the current working directory.
    if (!pcRelativeToHere)
    {
        pcRelativeToHere = "";
    }
    size_t stLenRelativeToHere = Strlen(pcRelativeToHere);
    if (stLenRelativeToHere == 0)
    {
        char acCWD[EE_MAX_PATH];
        if (!GetWorkingDirectory(acCWD, EE_MAX_PATH))
        {
            if (stAbsBytes > 0)
                pcAbsolutePath[0] = 0;
            return 0;
        }
        EE_ASSERT(Strlen(acCWD) != 0);
        return ConvertToAbsolute(pcAbsolutePath, stAbsBytes, pcRelativePath,
            acCWD);
    }

    size_t stLenRelativePath = Strlen(pcRelativePath);

    // Concatenate a delimiter if necessary
    bool bInsertDelimiter =
        (pcRelativeToHere[stLenRelativeToHere-1] != EE_PATH_DELIMITER_CHAR);

    size_t stRequiredSize = 1 // null termination
        + stLenRelativeToHere
        + stLenRelativePath
        + ((bInsertDelimiter) ? 1 : 0);

    // Fail if not enough storage
    if (stAbsBytes < stRequiredSize)
    {
        if (stAbsBytes > 0)
            pcAbsolutePath[0] = 0;
        return 0;
    }

    // Store pcRelativeToHere into pcAbsolutePath
    Strcpy(pcAbsolutePath, stAbsBytes, pcRelativeToHere);

    // Concatenate a delimiter if necessary
    if (bInsertDelimiter)
        Strcat(pcAbsolutePath, stAbsBytes, EE_PATH_DELIMITER_STR);

    // Concatenate pcRelativePath
    Strcat(pcAbsolutePath, stAbsBytes, pcRelativePath);

    RemoveDotDots(pcAbsolutePath);
    return strlen(pcAbsolutePath);
}


//--------------------------------------------------------------------------------------------------
void PathUtils::RemoveDotDots(efd::Char* pcPath)
{
    // Ensure that path is standardized first
    PathUtils::Standardize(pcPath);

    RemoveSlashDotSlash(pcPath);

    // This function collapses any "../" paths not found at the start
    // of the string.  i.e. "../../foo/quux/../bar" -> "../../foo/bar".
    // It assumes that all slashes are backslashes and that there are no
    // sequential backslashes.

    char* pcPtr;
    if (IsRelativePath(pcPath))
    {
        // Find the first non-dot/slash character in case the string
        // has a ../ path at the beginning.  This will be the first directory.
        pcPtr = pcPath;
        while (pcPtr && (*pcPtr == '.' || *pcPtr == EE_PATH_DELIMITER_CHAR))
        {
            pcPtr++;
        }
    }
    else
    {
        pcPtr = StripAbsoluteBase(pcPath);
    }

    // If the string consists of only dots and slashes, then nothing to do.
    if (!pcPtr)
        return;

    // pointer to past the end of the string
    char* pcEndPlusOne = pcPtr + Strlen(pcPtr) + 1;
    const char acSlashDotDot[] = EE_PATH_DELIMITER_STR "..";

    char* pcNextDir = strstr(pcPtr, acSlashDotDot);
    while (pcNextDir)
    {
        // Found a /.. in the string

        // Because we're going to remove the /.., we can zero
        // out the beginning as a temporary end of string.
        *pcNextDir = 0;

        // Find the directory before the temporary end.
        char* pcLastDir = strrchr(pcPtr, EE_PATH_DELIMITER_CHAR);

        // Advance pcNextDir past the /..
        if (pcLastDir)
        {
            // If we found a slash, then pcLastDir points to a slash
            // advance pcNextDir forward to point to after the
            // /.., which will be a slash or a /0.
            pcNextDir += 3;
        }
        else
        {
            // If we didn't find a slash, then pcLastDir needs to be
            // set to the start of the string.  In this case,
            // pcNextDir will be: (1) "/../dir", (2) "/../0" or
            // (3) "/../0".
            if (pcNextDir + 3)
            {
                // Case 1, Case 2
                pcNextDir += 4;
            }
            else
            {
                // Case 3
                pcNextDir += 3;
            }
            pcLastDir = pcPtr;
        }

        // Now move everything at pcNextDir to the end of the string
        // backwards onto pcLastDir.
        for (int i = 0; i < pcEndPlusOne - pcNextDir; i++)
        {
            pcLastDir[i] = pcNextDir[i];
        }

        // Find the first non-dot/slash character in case the string
        // has a ../ path at the beginning. This will be the first directory.
        while (pcPtr && (*pcPtr == '.' || *pcPtr == EE_PATH_DELIMITER_CHAR))
        {
            ++pcPtr;
        }

        pcNextDir = strstr(pcPtr, acSlashDotDot);
    }
}


//--------------------------------------------------------------------------------------------------
void PathUtils::RemoveSlashDotSlash(efd::Char* pcPath)
{
    // Remove any "/./" that remain in pcPath
    const char acSlashDotSlash[] = EE_PATH_DELIMITER_STR "." EE_PATH_DELIMITER_STR;

    char* pcNext = strstr(pcPath, acSlashDotSlash);
    while (pcNext)
    {
        size_t uiLen = Strlen(pcNext);
        const char* pcAfterDotSlash = pcNext + 2;
        Strcpy(pcNext, uiLen, pcAfterDotSlash);
        pcNext = strstr(pcPath, acSlashDotSlash);
    }
}


//--------------------------------------------------------------------------------------------------
void PathUtils::ReplaceInvalidFilenameCharacters(efd::Char* pcFilename, efd::Char cReplacement)
{
    EE_ASSERT(pcFilename);

    size_t stLen = Strlen(pcFilename);
    for (unsigned int uiChar = 0; uiChar < stLen; uiChar++)
    {
        // Detect illegal filename characters. This will detect '/' and '\\'
        // characters, so this function should not be used on full paths, only
        // filenames.
        switch (pcFilename[uiChar])
        {
        case '/':
        case '\\':
        case ':':
        case '*':
        case '?':
        case '\"':
        case '<':
        case '>':
        case '|':
            // Replace invalid character with replacement character.
            pcFilename[uiChar] = cReplacement;
            break;
        }
    }
}
