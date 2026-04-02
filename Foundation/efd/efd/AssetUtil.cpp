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

#include <efd/AssetUtil.h>
#include <efd/File.h>
#include <efd/PathUtils.h>

using namespace efd;

void AssetUtil::PathParts(
    const utf8string& pathValue,
    utf8string& directory,
    utf8string& name,
    utf8string& extension)
{
    // set directory (using the canonical slashes)
    vector<utf8string> parts;
    pathValue.split("/", parts);
    int sz = parts.size();

    directory.clear();
    for (int x=0; x<(sz-1); x++)
    {
        directory = directory + "/" + parts[x];
    }

    // set the name and extension
    name = PathUtils::PathGetFileName(pathValue);
    name = PathUtils::PathRemoveFileExtension(name);

    extension = PathUtils::PathGetExtension(pathValue);
}


void AssetUtil::PathPartsGivenExtension(
    const utf8string& pathValue,
    const utf8string& extension,
    utf8string& directory,
    utf8string& name)
{
    // set directory (using the canonical slashes)
    vector<utf8string> parts;
    pathValue.split("/", parts);
    int sz = parts.size();

    directory.clear();
    for (int x=0; x<(sz-1); x++)
    {
        directory = directory + "/" + parts[x];
    }

    // set the name and extension
    EE_ASSERT(sz > 0);
    size_t stringLength = parts[sz-1].length();
    size_t extensionLength = extension.length() + 1;

    if (extensionLength == 1)
        name = parts[sz-1];
    else if (stringLength >= extensionLength)
        name = parts[sz-1].substr(0, stringLength - extensionLength);
    else
        name = "";
}