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

#ifndef NITERRAINFILE_H
#define NITERRAINFILE_H

#include "NiTerrainLibType.h"
#include "NiFileVersionRegistry.h"
#include "NiTerrainFileVersion4.h"

/// Typedef the latest terrain file format to a standardized type
typedef NiITerrainFileVersion4 NiTerrainFile;

/// Typedef a file format registry for this file type
typedef NiFileVersionRegistry<NiTerrainFile> NiTerrainFileFormatRegistry;

/// Typedef a smart pointer for the terrain file format
typedef efd::SmartPointer<NiTerrainFile> NiTerrainFilePtr;

/// Typedef a smart pointer for the format registry
typedef efd::SmartPointer<NiTerrainFileFormatRegistry> NiTerrainFileFormatRegistryPtr;

#endif // NITERRAINFILE_H
