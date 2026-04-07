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
#ifndef NITERRAINSECTORFILEDEVELOPER_H
#define NITERRAINSECTORFILEDEVELOPER_H

#include "NiTerrainLibType.h"
#include "NiTerrainSectorFile.h"
#include "NiTerrainCellLeaf.h"

/**
    The class is used to iterate over and write terrain sector files.
 */
class NITERRAIN_ENTRY NiTerrainSectorFileDeveloper : public NiTerrainSectorFile
{
public:
    NiTerrainSectorFileDeveloper();

    /// Destructor.
    virtual ~NiTerrainSectorFileDeveloper();

    virtual FileVersion GetFileVersion() const;
    virtual FileVersion GetCurrentVersion();
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths);
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);
    virtual bool Precache(efd::UInt32 uiBeginLevel, efd::UInt32 uiEndLevel, efd::UInt32 eData);
    virtual bool IsDataReady(DataField::Value eDataField);
    virtual bool ReadSectorConfig(efd::UInt32& uiSectorWidthInVerts, efd::UInt32& uiNumLOD);
    virtual void WriteSectorConfig(efd::UInt32 uiSectorWidthInVerts, efd::UInt32 uiNumLOD);
    virtual bool ReadHeights(efd::UInt16* pusHeights, efd::UInt32 uiDataLength);
    virtual void WriteHeights(efd::UInt16* pusHeights, efd::UInt32 uiDataLength);
    virtual bool ReadNormals(efd::Point2* pkNormals, efd::UInt32 uiDataLength);
    virtual void WriteNormals(efd::Point2* pkNormals, efd::UInt32 uiDataLength);
    virtual bool ReadTangents(efd::Point2* pkTangents, efd::UInt32 uiDataLength);
    virtual void WriteTangents(efd::Point2* pkTangents, efd::UInt32 uiDataLength);
    virtual bool ReadBlendMask(NiPixelData*& pkBlendMask);
    virtual void WriteBlendMask(NiPixelData* pkBlendMask);
    virtual bool ReadLowDetailDiffuseMap(NiPixelData*& pkLowDetailDiffuse);
    virtual void WriteLowDetailDiffuseMap(NiPixelData* pkLowDetailDiffuse);
    virtual bool ReadLowDetailNormalMap(NiPixelData*& pkLowDetailNormal);
    virtual void WriteLowDetailNormalMap(NiPixelData* pkLowDetailNormal);
    virtual bool ReadSurfaceReferences(SurfaceReferenceMap& kReferences);
    virtual void WriteSurfaceReferences(const SurfaceReferenceMap& kReferences);
    virtual bool ReadCellSurfaceIndex(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        LeafData* pkLeafData);
    virtual void WriteCellSurfaceIndex(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        LeafData* pkLeafData);
    virtual bool ReadCellBoundData(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        CellData* pkCellData);
    virtual void WriteCellBoundData(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        CellData* pkCellData);

protected:

    /**
        Return true if the file identified by the identifier is the correct version for reading
        by this file reader.

        @param kID The file to attempt to read
    */
    virtual bool DetectFileVersion(FileIdentifier kID);

    /**
        Initialize the class by opening the file and reading the first couple
        of headers.

        @return true if the class was successfully initialized.
    */
    virtual bool Initialize();

    /// Filename generation functions
    NiString GenerateSectorPathPrefix();
    NiString GenerateHeightsFilename();
    NiString GenerateNormalsFilename();
    NiString GenerateTangentsFilename();
    NiString GenerateLowDetailDiffuseMapFilename();
    NiString GenerateLowDetailNormalMapFilename();
    NiString GenerateSectorDataFilename();
    NiString GenerateBoundsFilename();
    NiString GenerateSurfaceReferenceFilename();
    NiString GenerateSurfaceIndexFilename();
    NiString GenerateBlendMaskFilename();
    
    /// The file version this class is capable of reading/writing
    // V1: Basic data files stored on disk in a folder:
    static const FileVersion ms_kFileVersion = FileVersion(1); 

    /// Filenames used in this format
    static const char* ms_pcFolder;
    static const char* ms_pcHeightsFile;
    static const char* ms_pcNormalsFile;
    static const char* ms_pcTangentsFile;
    static const char* ms_pcLowDetailDiffuseMapFile;
    static const char* ms_pcLowDetailNormalMapFile;
    static const char* ms_pcSectorDataFile;
    static const char* ms_pcBoundsFile;
    static const char* ms_pcSurfaceReferenceFile;
    static const char* ms_pcSurfaceIndexFile;
    static const char* ms_pcBlendMaskFile;

    /// The terrain archive to use when generating filenames
    efd::utf8string m_kTerrainArchive;
    /// The sector's X coordinate
    efd::SInt32 m_iSectorX;
    /// The sector's Y coordinate
    efd::SInt32 m_iSectorY;
    /// The cached set of data
    efd::UInt32 m_eCachedData;
};

#include "NiTerrainSectorFileDeveloper.inl"

#endif // NITERRAINSECTORFILEDEVELOPER_H
