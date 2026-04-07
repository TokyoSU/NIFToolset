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
#ifndef NITERRAINSECTORFILEVERSION7_H
#define NITERRAINSECTORFILEVERSION7_H

#include "NiTerrainLibType.h"
#include "NiTerrainFileInterface.h"
#include "NiTerrainPhysXTypes.h"
#include "NiTerrainCellLeaf.h"

#include <NiPixelData.h>

/**
    The file format interface for a file format implementing storage for version 7 sector files.
    This interface marks the point at which surface references were moved back into sector files
    away from terrain wide files to help combat synchronization issues when different users
    edit different sectors simultaneously.
*/
class NITERRAIN_ENTRY NiITerrainSectorFileVersion7 : public NiTerrainFileInterface
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:
    
    /// The version of the reader interface that this class presents.
    static const FileVersion ms_InterfaceVersion = FileVersion(7);

    /**
        The structure used to identify sector files used by the terrain system. 
    */
    struct FileIdentifier
    {
        /// The archive path storing all the terrain data
        efd::utf8string m_kArchivePath;
        /// The x coordinate of the sector to open
        efd::SInt32 m_iSectorX;
        /// The y coordinate of the sector to open
        efd::SInt32 m_iSectorY;
        /// The storage policy to apply to all files that are opened
        NiTerrainStoragePolicy* m_pkStoragePolicy;
    };
    
    /// Constructor
    NiITerrainSectorFileVersion7();

    /**
        Open function to open the file at the given identifier for access using the given 
        access mode. 

        @param kID The file identifier of the file the caller wishes to open
        @param eAccessMode The type of access to the file required (read/write)
        @return WRONG_VERSION if the file is not the correct version. SUCCESS if the file
        can be opened, FAIL if the file cannot be read/written.
    */ 
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode) = 0;

    /// Enum to identify different data items stored in the file
    struct DataField
    {
        enum Value
        {
            /// Configuration file block
            CONFIG              = 1 << 0,
            /// Heghtmap for the sector
            HEIGHTS             = 1 << 1,
            /// Normal data for the sector
            NORMALS             = 1 << 2,
            /// Tangent data for the sector
            TANGENTS            = 1 << 3,
            /// The sector's blend mask
            BLEND_MASK          = 1 << 4,
            /// The sector's low detail normal map
            LOWDETAIL_NORMALS   = 1 << 5,
            /// The sector's low detail diffuse map
            LOWDETAIL_DIFFUSE   = 1 << 6,
            /// The sector's bounds
            BOUNDS              = 1 << 7,
            /// The surface index map
            SURFACE_INDEXES     = 1 << 8,
            /// The terrain's material information
            PHYSXMATERIAL_DATA  = 1 << 9,
            /// The sector's PhysX data
            PHYSX_DATA          = 1 << 10
        };
    };

    /// Structure to store cell bounding information
    struct CellData
    {
        /// Spherical bound for the cell
        NiBound m_kBound;
        /// Box bounding volume for the cell
        NiBoxBV m_kBox;
    };

    /// Structure to store leaf surface information
    struct LeafData
    {
        /// Number of surfaces on this leaf
        efd::UInt32 m_uiNumSurfaces;
        /// Indexes of surfaces used on this leaf
        efd::UInt32 m_auiSurfaceIndex[NiTerrainCellLeaf::MAX_NUM_SURFACES];
    };

    /// Structure to store surface reference information
    struct SurfaceReference
    {
        /// The asset reference pointing to the package hosting the surface
        NiTerrainAssetReferencePtr m_spPackageReference;
        /// The iteration of the package when the sector was saved
        efd::UInt32 m_uiPackageIteration;
        /// The name of the surface being referenced
        efd::utf8string m_kSurfaceName;
        /// The index used to represent this surface in the file
        efd::SInt32 m_iSurfaceIndex;
    };
    /// Typedef of a map between surface index and the surface reference for that surface.
    typedef efd::map<efd::SInt32, SurfaceReference> SurfaceReferenceMap;

    /**
        Request certain data to be pre-cached. This function must be called with the data that
        will be required before any read operations may be performed. 

        @note Whilst a data field may be requested, it may not be available in this file. Use
        the IsDataReady function to determine if the data is available after calling precache.

        @param uiBeginLevel The first LOD level that the user wants data from
        @param uiEndLevel The last LOD level that the user wants data from
        @param uiData A set of flags denoting what data fields are required to be loaded.
        @return true if successful
    */
    virtual bool Precache(efd::UInt32 uiBeginLevel, efd::UInt32 uiEndLevel, efd::UInt32 uiData);

    /**
        Query the file to see if a certain data field is available to be read. In order for this
        function to return true, the relevant data field must have been pre-cached successfully in
        a previous call to Precache. 

        @param eDataField The data field to test for. 
        @return whether the requested data is ready
    */
    virtual bool IsDataReady(DataField::Value eDataField);

    /**
        Read the sector configuration from the file. 

        @param uiSectorWidthInVerts A reference to an integer that will receive the width of a 
        sector in verts according to this file.
        @param uiNumLOD A reference to an integer that will receive the number of LOD stored in 
        this sector.
        @return true if the data was successfully read
    */
    virtual bool ReadSectorConfig(efd::UInt32& uiSectorWidthInVerts, efd::UInt32& uiNumLOD);

    /**
        Write the sector configuration to the file

        @param uiSectorWidthInVerts The width of the sector in verts
        @param uiNumLOD The number of LOD stored in the sector
    */
    virtual void WriteSectorConfig(efd::UInt32 uiSectorWidthInVerts, efd::UInt32 uiNumLOD);

    /**
        Read the sector's height map from file into the supplied buffer.

        @param pusHeights The height map buffer to read the data into
        @param uiDataLength The number of elements stored in the buffer. 
        (Must be equal to sectorWidth * sectorWidth)
        @return true if the data was successfully read
    */
    virtual bool ReadHeights(efd::UInt16* pusHeights, efd::UInt32 uiDataLength);

    /**
        Write the sector's height map to the file from the supplied buffer.

        @param pusHeights The height map buffer
        @param uiDataLength The number of elements stored in the buffer.
        (Must be equal to sectorWidth * sectorWidth)
    */  
    virtual void WriteHeights(efd::UInt16* pusHeights, efd::UInt32 uiDataLength);

    /**
        Read the sector's normals from the file into the specified buffer. 
        The normals are compressed by not storing the Z value.
        (calculate Z based on what it needs to be to make the normal a unit vector)

        @param pkNormals The normal buffer
        @param uiDataLength the number of elements stored in the buffer
        @return true if the data was successfully read
    */
    virtual bool ReadNormals(efd::Point2* pkNormals, efd::UInt32 uiDataLength);

    /**
        Write the sector's normals to the file from the specified buffer
        The normals are compressed by not storing the Z value. 
        (calculate Z based on what it needs to be to make the normal a unit vector)

        @param pkNormals The normal buffer
        @param uiDataLength the number of elements stored in the buffer
    */
    virtual void WriteNormals(efd::Point2* pkNormals, efd::UInt32 uiDataLength);

    /**
        Read the sector's tangents from the file into the specified buffer. 
        The tangents are compressed by not storing the Y value (assumed 0).

        @param pkTangents The normal buffer
        @param uiDataLength the number of elements stored in the buffer
        @return true if the data was successfully read
    */
    virtual bool ReadTangents(efd::Point2* pkTangents, efd::UInt32 uiDataLength);

    /**
        Write the sector's tangents to the file from the specified buffer. 
        The tangents are compressed by not storing the Y value (assumed 0).

        @param pkTangents The tangent buffer
        @param uiDataLength the number of elements stored in the buffer
    */
    virtual void WriteTangents(efd::Point2* pkTangents, efd::UInt32 uiDataLength);

    /**
        Read the sector's blend mask from the file

        @param pkBlendMask the pointer to return the blend mask into.
        @return true if the data was successfully read.
    */
    virtual bool ReadBlendMask(NiPixelData*& pkBlendMask);

    /**
        Write the sector's blend mask to the file. 

        @param pkBlendMask The blend mask 
    */
    virtual void WriteBlendMask(NiPixelData* pkBlendMask);

    /**
        Read the sector's low detail diffuse map from the file

        @param pkLowDetailDiffuse the pointer to return the map into.
        @return true if the data was successfully read.
    */
    virtual bool ReadLowDetailDiffuseMap(NiPixelData*& pkLowDetailDiffuse);

    /**
        Write the sector's low detail diffuse map to the file. 

        @param pkLowDetailDiffuse The low detail diffuse map
    */
    virtual void WriteLowDetailDiffuseMap(NiPixelData* pkLowDetailDiffuse);

    /**
        Read the sector's low detail normal map from the file

        @param pkLowDetailNormal the pointer to return the map into.
        @return true if the data was successfully read.
    */
    virtual bool ReadLowDetailNormalMap(NiPixelData*& pkLowDetailNormal);

    /**
        Write the sector's low detail normal map to the file. 

        @param pkLowDetailNormal The low detail normal map
    */
    virtual void WriteLowDetailNormalMap(NiPixelData* pkLowDetailNormal);

    /**
        Read the sector's surface reference map. Use this map to decode
        all surface indexes stored in this file.

        @param kReferences The surface reference map mapping surface indexes to
        the relevant surface information
        @return true if the data was successfully read.
    */
    virtual bool ReadSurfaceReferences(SurfaceReferenceMap& kReferences);

    /**
        Write the sector's surface reference map to the file. 

        @param kReferences The surface reference map mapping surface indexes to
        the relevant surface information
    */
    virtual void WriteSurfaceReferences(const SurfaceReferenceMap& kReferences);

    /**
        Read the surface index of a range of leaf cells from the file. 

        @param uiCellRegionID The first leaf to read into the buffer
        @param uiNumCells The number of cells to read 
        @param pkLeafData The buffer to read the data into (must be allocated)
        @return true if the data was successfully read.
    */
    virtual bool ReadCellSurfaceIndex(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        LeafData* pkLeafData);

    /**
        Write the surface index of a range of leaf cells to the file. 

        @param uiCellRegionID The first leaf to write to the file
        @param uiNumCells The number of cells to write 
        @param pkLeafData The buffer to write the data from
    */
    virtual void WriteCellSurfaceIndex(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        LeafData* pkLeafData);

    /**
        Read the bounding information of a range of cells from the file. 

        @param uiCellRegionID The first cell to read into the buffer
        @param uiNumCells The number of cells to read 
        @param pkCellData The buffer to read the data into (must be allocated)
        @return true if the data was successfully read.
    */
    virtual bool ReadCellBoundData(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        CellData* pkCellData);

    /**
        Write the bounding information of a range of cells to the file. 

        @param uiCellRegionID The first cell to write to the file
        @param uiNumCells The number of cells to write 
        @param pkCellData The buffer to write the data from
    */
    virtual void WriteCellBoundData(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        CellData* pkCellData);

    /**
        Reads in the physx data for the sector.

        @param kMaterialMap The material data we are going to return
        @param pkSampleData The physx data for each sector samples that we are going to return. This
        parameter should not yet be allocated
        @return true if the data was successfully read.
    */
    virtual bool ReadTerrainSectorPhysXData(
        efd::map<efd::UInt32, NiPhysXMaterialMetaData>& kMaterialMap, 
        NiTerrainSectorPhysXData*& pkSampleData);

    /**
        Writes in the physx data for the sector.

        @param kMaterialMap The material data we are going to write
        @param pkSampleData The physx data for each sector samples that we are going to write.
        This should not be null.
    */
    virtual void WriteTerrainSectorPhysXData(
        efd::map<efd::UInt32, NiPhysXMaterialMetaData> kMaterialMap, 
        NiTerrainSectorPhysXData* pkSampleData);
};

/**
    This version of the sector file is the first to implement the new streaming interface. 
    Its main goal is to minimize the amount of data that is required to be stored on disk.
 */
class NITERRAIN_ENTRY NiTerrainSectorFileVersion7 : public NiITerrainSectorFileVersion7
{
public:

    /// Constructor
    NiTerrainSectorFileVersion7();
    /// Destructor.
    virtual ~NiTerrainSectorFileVersion7();

    /// @see NiTerrainSectorFile
    /// @{
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);
    virtual void Close();
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths);
    virtual bool Precache(efd::UInt32 uiBeginLevel, efd::UInt32 uiEndLevel, efd::UInt32 uiData);
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
    virtual bool ReadTerrainSectorPhysXData(
        efd::map<efd::UInt32, NiPhysXMaterialMetaData>& kMaterialMap, 
        NiTerrainSectorPhysXData*& pkSampleData);
    virtual void WriteTerrainSectorPhysXData(
        efd::map<efd::UInt32, NiPhysXMaterialMetaData> kMaterialMap, 
        NiTerrainSectorPhysXData* pkSampleData);
    /// @}
 
protected:

    /**
        Return true if the file identified by the identifier is the correct version for reading
        by this file reader.

        @param kID The file to attempt to read
    */
    virtual bool DetectFileVersion(FileIdentifier kID);

    /**
        Initialize this file reading parser. 
        @return true if successful
    */
    virtual bool Initialize();

    struct DataBlockType
    {
        enum VALUE
        {
            CONFIG             = 0,
            HEIGHTS            = 1,
            NORMALS            = 2,
            TANGENTS           = 3,
            BLEND_MASK         = 4,
            LOWDETAIL_NORMALS  = 5,
            LOWDETAIL_DIFFUSE  = 6,
            BOUNDS             = 7,
            SURFACE_INDEXES    = 8,
            PHYSXMATERIAL_DATA = 9,
            PHYSX_DATA         = 10,
            SURFACE_REFERENCES = 11,

            NUM_BLOCK_TYPES
        };
    };

    struct ImageCompressionMode
    {
        enum VALUE
        {
            NONE = 0,
        };
    };

    struct FileHeader
    {
        FileVersion m_kVersion;
        efd::UInt32 m_uiPresentData;
    };

    class DataBlock : public NiMemObject
    {
    public:
        DataBlockType::VALUE m_eBlockType;
        efd::UInt32 m_uiDataLength;
        
        virtual ~DataBlock();
        static bool ReadBlockHeader(efd::BinaryStream& kStream, DataBlock& kBlockHeader);
        virtual bool ReadBlockData(efd::BinaryStream& kStream);
        bool WriteBlockHeader(efd::BinaryStream& kStream);
        virtual bool WriteBlockData(efd::BinaryStream& kStream);
    };
    
    class ConfigDataBlock: public DataBlock
    {
    public:
        efd::UInt32 m_uiSectorWidthInVerts;
        efd::UInt32 m_uiNumLOD;
        
        virtual bool ReadBlockData(efd::BinaryStream& kStream);
        virtual bool WriteBlockData(efd::BinaryStream& kStream);
    };

    class SurfaceReferenceBlock: public DataBlock
    {
    public:
        SurfaceReferenceMap m_kSurfaceReferences;
        efd::utf8string m_kSaveLocation;

        efd::UInt32 CalculateDataLength();
        efd::UInt32 CalculateStringLength(const efd::utf8string& kString);
        virtual bool ReadBlockData(efd::BinaryStream& kStream);
        virtual bool WriteBlockData(efd::BinaryStream& kStream);
    };

    class ImageDataBlock : public DataBlock
    {
    public:
        efd::UInt32 m_uiWidth;
        efd::UInt32 m_uiHeight;
        efd::UInt16 m_usNumChannels;
        efd::UInt16 m_usBytesPerChannel;
        ImageCompressionMode::VALUE m_eCompressionMode;
        efd::UInt8* m_pucCompressedData;

        static const efd::UInt32 ms_uiStaticDataSize = 4 * sizeof(efd::UInt32);

        ~ImageDataBlock();
        virtual bool ReadBlockData(efd::BinaryStream& kStream);
        virtual bool WriteBlockData(efd::BinaryStream& kStream);

        virtual bool CompressFromStream(ImageCompressionMode::VALUE eCompressionMode,
            efd::UInt8* pucBuffer, efd::UInt32 uiBufferLength, 
            efd::UInt32 uiWidth, efd::UInt32 uiHeight, efd::UInt16 usNumChannels, 
            efd::UInt16 usBytesPerChannel);
        virtual bool CompressFromImageData(ImageCompressionMode::VALUE eCompressionMode, 
            NiPixelData* pkSource);
        virtual bool DecompressToImageData(NiPixelData*& pkSource);
        virtual bool DecompressToStream(efd::UInt8* pucBuffer, efd::UInt32 uiBufferLength);
    
    protected:
        virtual efd::UInt32 CalculateCompressionStride();
    };

    class BoundingDataBlock : public DataBlock
    {
    public:
        efd::UInt32 m_uiNumCells;
        efd::UInt32 m_uiStartCell;
        CellData* m_pkBoundingData;

        static const efd::UInt32 ms_uiCellDataSize = 20 * 4;

        ~BoundingDataBlock();
        virtual bool ReadBlockData(efd::BinaryStream& kStream);
        virtual bool WriteBlockData(efd::BinaryStream& kStream);
    };

    class SurfaceIndexBlock : public DataBlock
    {
    public:
        efd::UInt32 m_uiNumLeaves;
        efd::UInt32 m_uiStartLeaf;
        LeafData* m_pkSurfaceIndexData;

        static const efd::UInt32 ms_uiLeafDataSize = (2 + NiTerrainCellLeaf::MAX_NUM_SURFACES) * 4;

        ~SurfaceIndexBlock();
        virtual bool ReadBlockData(efd::BinaryStream& kStream);
        virtual bool WriteBlockData(efd::BinaryStream& kStream);
    };

    /**
        Class defining a physx material data block for the current sector
    */
    class PhysXMaterialDataBlock : public DataBlock
    {
    public:
        efd::map<efd::UInt32, NiPhysXMaterialMetaData> m_kMaterialData;

        static const efd::UInt32 ms_uiStaticDataSize = sizeof(efd::UInt32) + 3 * sizeof(float);

        ~PhysXMaterialDataBlock();
        virtual bool ReadBlockData(efd::BinaryStream& kStream);
        virtual bool WriteBlockData(efd::BinaryStream& kStream);
    };

    /**
        Class defining a physx data block for the current sector
    */
    class PhysXDataBlock : public DataBlock
    {
    public:
        NiTerrainSectorPhysXData* m_pkPhysXData;

        static const efd::UInt32 ms_uiStaticDataSize = sizeof(efd::UInt16) * 2 + sizeof(bool);

        ~PhysXDataBlock();
        virtual bool ReadBlockData(efd::BinaryStream& kStream);
        virtual bool WriteBlockData(efd::BinaryStream& kStream);
    };

    /**
        Function that reads the file header

        @param kStream stream to read from
        @param[out] kHeader data structure the file header is read to
        @return true
    */
    bool ReadFileHeader(efd::BinaryStream& kStream, FileHeader& kHeader);

    /**
        Function that reads the file into the appropriate data blocks.

        @param kStream stream to read from.
        @param uiBeginLevel the level of detail to start reading at.
        @param uiEndLevel the level of detail to stop redaing at.
        @param uiSelectedData data slected for reading.
        @return true
    */
    bool ReadDataBlocks(efd::BinaryStream& kStream, efd::UInt32 uiBeginLevel, 
        efd::UInt32 uiEndLevel, efd::UInt32 uiSelectedData);

    /**
        Writes all data to the file.

        @return true if successful
    */
    bool WriteFile();

    /**
        Writes the file's header

        @param[out] kStream the stream to write to
        @param kHeader the data structure to write
        @return true if successful
    */
    bool WriteFileHeader(efd::BinaryStream& kStream, FileHeader& kHeader);

    /**
        Writes the file's data content

        @param[out] kStream the stream to write to
        @return true if successful
    */
    bool WriteDataBlocks(efd::BinaryStream& kStream);

    /// Generate the filename to use for this sector file
    efd::utf8string GenerateFilename();

    // Filename generation variables
    static const char* ms_pcSectorFilename;

    // Data block pointers:
    DataBlock* m_apkDataBlocks[DataBlockType::NUM_BLOCK_TYPES];
    /// The terrain archive to use when generating filenames
    efd::utf8string m_kTerrainArchive;
    /// The sector's X coordinate
    efd::SInt32 m_iSectorX;
    /// The sector's Y coordinate
    efd::SInt32 m_iSectorY;
    /// The set of flags identifying what data has been cached
    efd::UInt32 m_eCachedData;
};

#endif // NITERRAINSECTORFILEVERSION7_H
