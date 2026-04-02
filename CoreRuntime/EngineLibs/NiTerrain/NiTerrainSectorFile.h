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
#ifndef NITERRAINSECTORFILE_H
#define NITERRAINSECTORFILE_H

#include "NiTerrainLibType.h"
#include "NiTerrainSectorFileVersion7.h"

/// Typedef the latest terrain sector file format to a standardized type
typedef NiITerrainSectorFileVersion7 NiTerrainSectorFile;

/// Typedef a file format registry for this file type
typedef NiFileVersionRegistry<NiTerrainSectorFile> NiTerrainSectorFileFormatRegistry;

/// Typedef a smart pointer for the terrain sector file format
typedef efd::SmartPointer<NiTerrainSectorFile> NiTerrainSectorFilePtr;

/// Typedef a smart pointer for the format registry
typedef efd::SmartPointer<NiTerrainSectorFileFormatRegistry> NiTerrainSectorFileFormatRegistryPtr;

#endif // NITERRAINSECTORFILE_H
