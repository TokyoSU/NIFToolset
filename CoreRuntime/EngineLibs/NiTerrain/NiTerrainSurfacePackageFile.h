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

#pragma once
#ifndef NITERRAINSURFACEPACKAGEFILE_H
#define NITERRAINSURFACEPACKAGEFILE_H

#include "NiTerrainSurfacePackageFileVersion1.h"

/// Typedef the latest terrain surface package file format to a standardized type
typedef NiITerrainSurfacePackageFileVersion1 NiTerrainSurfacePackageFile;

/// Typedef a file format registry for this file type
typedef NiFileVersionRegistry<NiTerrainSurfacePackageFile> NiTerrainSurfacePackageFileFormatRegistry;

/// Typedef a smart pointer for the terrain surface package file format
typedef efd::SmartPointer<NiTerrainSurfacePackageFile> NiTerrainSurfacePackageFilePtr;

/// Typedef a smart pointer for the format registry
typedef efd::SmartPointer<NiTerrainSurfacePackageFileFormatRegistry> 
    NiTerrainSurfacePackageFileFormatRegistryPtr;

#endif // NITERRAINSURFACEPACKAGEFILE_H
