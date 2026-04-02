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
#ifndef NITERRAINSECTORFILEVERSION5_H
#define NITERRAINSECTORFILEVERSION5_H

#include <NiBoxBV.h>
#include "NiTerrainLibType.h"
#include "NiTerrainSectorFileVersion6.h"

/**
    The file format interface for a file format implementing storage for version 5 sector files.
    This interface marks the point at which multisector many-materialed files could be 
    generated and edited.
*/
class NITERRAIN_ENTRY NiITerrainSectorFileVersion5 : public NiTerrainFileInterface
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    /// The version of the reader interface that this class presents.
    static const FileVersion ms_InterfaceVersion = FileVersion(5);
    
    /// The file identifier for this version of Terrain sector files
    struct FileIdentifier
    {
        /// The sector file to be opened.
        efd::utf8string m_kSectorFile;
        /// The storage policy to use when opening files
        NiTerrainStoragePolicy* m_pkStoragePolicy;
    };
    
    /// @see NiFileInterface
    virtual NiFileInterface* AdaptToNextVersion();
    
    /// Constructor
    NiITerrainSectorFileVersion5();

    /**
        Open function to open the file at the given identifier for access using the given 
        access mode. 

        @param kID The file identifier of the file the caller wishes to open
        @param eAccessMode The type of access to the file required (read/write)
        @return WRONG_VERSION if the file is not the correct version. SUCCESS if the file
        can be opened, FAIL if the file cannot be read/written.
    */
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode) = 0;

    /**
        Identifiers for each type of stream capable of being stored.

        Currently, element size is assumed to be sizeof(float) for
        endian swapping purposes.
    */
    enum StreamType
    {
        /// Invalid or unspecified stream.
        STREAM_INVALID = -1,

        /// Height stream.
        STREAM_HEIGHT = 0,

        /// Normal stream.
        STREAM_NORMAL = 1,

        /// Tangent stream.
        STREAM_TANGENT = 2,

        /// Morph Height stream.
        STREAM_MORPH_HEIGHT = 3,

        /// Morph Normal stream.
        STREAM_MORPH_NORMAL = 4,

        /// Morph Tangent stream.
        STREAM_MORPH_TANGENT = 5,

        /// The surface index stream
        STREAM_SURFACE_INDEX = 6,

        /// A blend mask stream
        STREAM_BLEND_MASK = 7,

        /// A low detail diffuse texture stream
        STREAM_LOW_DETAIL_DIFFUSE_TEXTURE = 8,

        /// A low detail normal texture stream
        STREAM_LOW_DETAIL_NORMAL_TEXTURE = 9,

        /// Maximum number of streams per block.
        STREAM_MAX_NUMBER
    };

    /// File header type for deciphering file information.
    struct FileHeader
    {
        /// Version of the file format used.
        FileVersion m_kVersion;

        /// Width of the block in vertices.
        NiUInt32 m_uiVertsPerBlock;

        /// Maximum number of lod levels present in the terrain hierarchy.
        NiUInt32 m_uiNumLOD;

        /// Load.
        void LoadBinary(efd::BinaryStream& kStream);

        /// Save.
        void SaveBinary(efd::BinaryStream& kStream);
    };

    /// Present Data Bitfield.
    union PresentData
    {
        /// Place holder for accessing the bitfield as a whole
        NiUInt32 m_uiBitField;
        struct Data
        {
            /// Indicates if height data is present.
            NiUInt32 m_bHeights : 1;

            /// Indicates if normal data is present.
            NiUInt32 m_bNormals : 1;

            /// Indicates if tangent data is present.
            NiUInt32 m_bTangents : 1;

            /// Indicates if morph height data is present.
            NiUInt32 m_bMorphHeights : 1;

            /// Indicates if morph normal data is present.
            NiUInt32 m_bMorphNormals : 1;

            /// Indicates if morph tangent data is present.
            NiUInt32 m_bMorphTangents : 1;

            /// Indicates we have a surface index stream
            NiUInt32 m_bSurfaceIndex : 1;

            /// Indicates we have a blend mask stream
            NiUInt32 m_bBlendMask : 1;

            /// Indicates we have a low detail texture stream
            NiUInt32 m_bLowDetailTexture : 1;
        };
    };

    /// Block Header type.
    struct BlockHeader
    {
        /// Length of the data stored in this block (including this header).
        NiUInt32 m_ulLength;
        /// Number of bytes from the start of this header to the first child.
        NiUInt32 m_ulChildOffset;
        /// Bitfield representing what data streams are present for this block.
        PresentData m_kPresentData;

        /// Bounding radius of this block.
        float m_fBoundRadius;
        /// Bounding center of this block.
        NiPoint3 m_kBoundCenter;

        /// Bounding Box Volume Center.
        NiPoint3 m_kVolumeCenter;
        /// Bounding Box Volume direction 1.
        NiPoint3 m_kVolumeDirection1;
        /// Bounding Box Volume direction 2.
        NiPoint3 m_kVolumeDirection2;
        /// Bounding Box Volume direction 3.
        NiPoint3 m_kVolumeDirection3;
        /// Bounding Box Volume extent 1.
        float m_kVolumeExtent1;
        /// Bounding Box Volume extent 2.
        float m_kVolumeExtent2;
        /// Bounding Box Volume extent 3.
        float m_kVolumeExtent3;

        /// Load.
        void LoadBinary(efd::BinaryStream& kStream);
        /// Save.
        void SaveBinary(efd::BinaryStream& kStream);
    };

    /// Data stream header.
    struct DataStreamHeader
    {
        /**
            Number of bytes stored in this stream including this header.

            Currently, element size is assumed to be sizeof(float) for
            endian swapping purposes.
        */
        NiUInt32 m_ulLength;
        /// The type of stream this data describes.
        StreamType m_kStreamType;
        /// The number of bytes per object stored in this stream.
        NiUInt32 m_uiObjectSize;

        /// Load.
        void LoadBinary(efd::BinaryStream& kStream);
        /// Save.
        void SaveBinary(efd::BinaryStream& kStream);
    };

    /**
        @return the number of LOD stored in this file.
    */
    virtual NiUInt32 GetNumLOD() const = 0;

    /**
        Set the number of vertices per block in this sector.

        @note This function must be called BEFORE WriteFileHeader().
        @note Can only be called on a writable file

        @param uiVertsPerBlock the number of vertices per block in this sector.
    */
    virtual void SetBlockWidthInVerts(NiUInt32 uiVertsPerBlock) = 0;
    
    /**
        Set the number of lod levels contained in this sector.
        @param uiNumLods Number of lod levels contained in the sector.

        @note This function must be called BEFORE WriteFileHeader().
        @note Can only be called on a writable file.
    */
    virtual void SetNumLOD(NiUInt32 uiNumLods) = 0;

    /**
        Write the file header.

        @note This function must only be called once, and called BEFORE
        any calls to WriteBlock().
        @note Can only be called on a writable file.
    */
    virtual void WriteFileHeader() = 0;

    /**
        Write the current block's data to the file.

        @note This function must be called for each block to be written to the
        file, all stream data, bounding data, etc must be set before
        this function is called. By calling this function all current block
        data is reset and must be set again before the next call to
        WriteBlock().
        @note Can only be called on a writable file.
    */
    virtual void WriteBlock() = 0;

    /**
        Set the bounding information of the current block.

        @param kBound the bounding sphere around the block.
        @param kVolume the bounding box volume of this block.

        @note Can only be called on a writable file.
    */
    virtual void SetBlockBounds(NiBound kBound, NiBoxBV kVolume) = 0;

    /**
        Set the data for a particular stream of information in the current
        block.

        @note Can only be called on a writable file.

        @param kStreamID the ID of the stream to set the data for.
        @param uiObjectSize the size in bytes of the data for each vertex.
        @param pvData pointer to the data stream.
        @param uiDataLength the total length in bytes of the stream.
    */
    virtual void SetStreamData(StreamType kStreamID, NiUInt32 uiObjectSize,
        void* pvData, NiUInt32 uiDataLength) = 0;

    /**
        @return the number of vertices per block in this file
    */
    virtual NiUInt32 GetBlockWidthInVerts() const = 0;

    /**
        Read in the next block of information from the file
        @note Can only be called on a file opened for read access

        @return True if the next block was successfully read.
    */
    virtual bool NextBlock() = 0;

    /**
        Jump to the [iChildID]th child of the current block

        @note Can only be called on a file opened for read access
        @note Can be useful for jumping to a specific level of detail region
        of the file

        @param iChildID the ID of the child to jump to
        @return True if the block was successfully read, false otherwise.
    */
    virtual bool PushBlock(int iChildID) = 0;

    /**
        Jump to the parent of the current block

        @note Can only be called on a file opened for read access

        @note Using this function after reading a LOD region of the file
        (i.e. sequentially reading more than the 4 children of the parent
        block) will cause any further GetBlockID calls to return
        incorrect information

        @return True if the block was successfully read.
    */
    virtual bool PopBlock() = 0;

    /**
        Get the data for a particular stream of information in the current
        block.

        @note Can only be called on a readable file

        @param kStreamID The ID of the stream to set the data for.
        @param[out] uiDataLength A reference to a variable to place the length of
        the stream in.

        @return A pointer to the data stored in the stream.
    */
    virtual void* GetStreamData(StreamType kStreamID, NiUInt32& uiDataLength) = 0;

    /**
        @return the bounding sphere of the current block.
    */
    virtual NiBound GetBlockBound() const = 0;

    /**
        @return the bounding box volume of the current block.
    */
    virtual NiBoxBV GetBlockBoundVolume() const = 0;
};

/**
    This class is used to adapt a NiITerrainSectorFileVersion5 interface object to present a
    NiITerrainSectorFileVersion6 interface to the user.
*/
class NiITerrainSectorFileVersion5To6Adapter : public NiITerrainSectorFileVersion6
{
public:

    /// Constructor
    NiITerrainSectorFileVersion5To6Adapter(NiITerrainSectorFileVersion5* pBaseVersion);

    /// @name Backwards compatibility helper functions
    /// @{
    void GenerateMinMaxElevation();
    void GenerateSectorSpaceMap(efd::map<efd::UInt32, NiIndex>& kSectorSpaceMap, 
        efd::UInt32 uiLeafID, efd::UInt32 uiSubDivisions, NiIndex kOffset);
    void DetectTextureSizes();

    efd::UInt32 GetNumLOD();
    efd::UInt32 GetBlockWidthInVerts();
    efd::UInt32 GetBlendMaskSize();
    efd::UInt32 GetLowDetailTextureSize();
    void GetMinMaxElevation(float& fMin, float& fMax);
    /// @}

    /// @see NiITerrainSectorFileVersion6
    /// @{
    virtual OpenErrorCode Open(FileIdentifier kIdentifier, efd::File::OpenMode eAccessMode);
    virtual void Close();
    virtual FileVersion GetFileVersion() const;
    virtual FileVersion GetInterfaceVersion() const;
    virtual bool IsReady() const;
    virtual bool IsWritable() const;
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths);
    virtual bool Precache(efd::UInt32 uiBeginLevel, efd::UInt32 uiEndLevel, efd::UInt32 uiData);
    virtual bool ReadSectorConfig(efd::UInt32& uiSectorWidthInVerts, efd::UInt32& uiNumLOD);
    virtual bool ReadHeights(efd::UInt16* pusHeights, efd::UInt32 uiDataLength);
    virtual bool ReadBlendMask(NiPixelData*& pkBlendMask);
    virtual bool ReadLowDetailDiffuseMap(NiPixelData*& pkLowDetailDiffuse);
    virtual bool ReadCellSurfaceIndex(efd::UInt32 uiCellRegionID, efd::UInt32 uiNumCells, 
        LeafData* pkLeafData);
    virtual bool IsDataReady(DataField::Value eDataField);
    virtual void WriteSectorConfig(efd::UInt32 uiSectorWidthInVerts, efd::UInt32 uiNumLOD);
    virtual void WriteHeights(efd::UInt16* pusHeights, efd::UInt32 uiDataLength);
    virtual bool ReadNormals(efd::Point2* pkNormals, efd::UInt32 uiDataLength);
    virtual void WriteNormals(efd::Point2* pkNormals, efd::UInt32 uiDataLength);
    virtual bool ReadTangents(efd::Point2* pkTangents, efd::UInt32 uiDataLength);
    virtual void WriteTangents(efd::Point2* pkTangents, efd::UInt32 uiDataLength);
    virtual void WriteBlendMask(NiPixelData* pkBlendMask);
    virtual void WriteLowDetailDiffuseMap(NiPixelData* pkLowDetailDiffuse);
    virtual bool ReadLowDetailNormalMap(NiPixelData*& pkLowDetailNormal);
    virtual void WriteLowDetailNormalMap(NiPixelData* pkLowDetailNormal);
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

    /// The class implementing the previous interface
    efd::SmartPointer<NiITerrainSectorFileVersion5> m_spPrevVersion;
    /// The set of data has been cached in the file reader
    efd::UInt32 m_eCachedData;
    /// An array used to map into the new file format
    efd::map<efd::UInt32, NiIndex> m_kSectorSpaceMap;
    /// Value storing the highest point on the terrain
    float m_fMaxElevation;
    /// Value storing the lowest point on the terrain
    float m_fMinElevation;
    /// Detected size of the sectorwide blend mask
    efd::UInt32 m_uiBlendMaskSize;
    /// Detected size of the low detail texture
    efd::UInt32 m_uiLowDetailTextureSize;
};

/**
    The class is used to iterate over and write terrain sector files.
 */
class NITERRAIN_ENTRY NiTerrainSectorFileVersion5 : public NiITerrainSectorFileVersion5
{
public:

    /// Constructor
    NiTerrainSectorFileVersion5();
    /// Destructor
    virtual ~NiTerrainSectorFileVersion5();

    ///@see NiITerrainSectorFileVersion5
    /// @{
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);
    virtual NiUInt32 GetNumLOD() const;
    virtual bool IsReady() const;
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths);
    virtual void SetBlockWidthInVerts(NiUInt32 uiVertsPerBlock);
    virtual void SetNumLOD(NiUInt32 uiNumLods);
    virtual void WriteFileHeader();
    virtual void WriteBlock();
    virtual void SetBlockBounds(NiBound kBound, NiBoxBV kVolume);
    virtual void SetStreamData(StreamType kStreamID, NiUInt32 uiObjectSize,
        void* pvData, NiUInt32 uiDataLength);
    virtual NiUInt32 GetBlockWidthInVerts() const;
    virtual bool NextBlock();
    virtual bool PushBlock(int iChildID);
    virtual bool PopBlock();
    virtual void* GetStreamData(StreamType kStreamID, NiUInt32& uiDataLength);
    virtual NiBound GetBlockBound() const;
    virtual NiBoxBV GetBlockBoundVolume() const;
    /// @}
        
    /**
        Sector filename generation function for use in converting the constructor's interface

        @param pcTerrainArchive the path to a sector file 
        @param iSectorX the X index of the sector
        @param iSectorY the Y index of the sector
        @return the generated filename
    */
    static NiString GenerateSectorFileName(const char* pcTerrainArchive, NiInt32 iSectorX, 
        NiInt32 iSectorY);

    /**
        Set the data for the height stream of the current block. Assumes
        one float per vertex.

        See SetStreamData.

        @param pfData the data stream.
        @param uiDataLength the total length in bytes of the stream.
    */
    inline void SetHeightData(float* pfData, NiUInt32 uiDataLength);

    /**
        Set the data for the normal stream of the current block.

        Assumes two floats per vertex. (Z is assumed to be "1.0f" and the
        vector is then normalized during use).

        See SetStreamData.

        @param pfData The data stream
        @param uiDataLength The total length in bytes of the stream
    */
    inline void SetNormalData(float* pfData, NiUInt32 uiDataLength);

    /**
        Set the data for the tangent stream of the current block.

        Assumes two floats per vertex. (Z is assumed to be "1.0f" and the
        vector is then normalized during use)

        See SetStreamData.

        @param pfData the data stream
        @param uiDataLength the total length in bytes of the stream
    */
    inline void SetTangentData(float* pfData, NiUInt32 uiDataLength);

    /**
        Set the data for the morphing height stream of the current block.

        Assumes one float per vertex.

        See SetStreamData.

        @param pfData the data stream
        @param uiDataLength the total length in bytes of the stream
    */
    inline void SetMorphHeightData(float* pfData, NiUInt32 uiDataLength);

    /**
        Set the data for the morphing normal stream of the current block.

        Assumes two floats per vertex. (Z is assumed to be "1.0f" and the
        vector is then normalized during use)

        See SetStreamData.

        @param pfData the data stream
        @param uiDataLength the total length in bytes of the stream
    */
    inline void SetMorphNormalData(float* pfData, NiUInt32 uiDataLength);

    /**
        Set the data for the morphing tangent stream of the current block.

        Assumes two floats per vertex. (Z is assumed to be "1.0f" and the
        vector is then normalized during use)

        See SetStreamData.

        @param pfData The data stream.
        @param uiDataLength The total length in bytes of the stream.
    */
    inline void SetMorphTangentData(float* pfData, NiUInt32 uiDataLength);

    /**
        Set the data for the surface index stream of the current block. 
        
        Assumes one float per index. (one surface is only defined by one index)

        See SetStreamData.

        @param puiData The data stream.
        @param uiDataLength The total length in bytes of the stream.
    */
    inline void SetSurfaceIndexData(NiUInt32* puiData, NiUInt32 uiDataLength);

    /**
        Set the data for the blend mask stream of the current block. 
        The data stored here is to be in the same format that the blend mask
        pixels are stored in the pixel data (RGBA). 

        See SetStreamData.

        @param pucData The data stream.
        @param uiDataLength The total length in bytes of the stream.
        */
    inline void SetBlendMaskData(NiUInt8* pucData, NiUInt32 uiDataLength);

    /**
        Set the data for the low detail diffuse texture stream of the current 
        block. The data stored here is to be in RGBA format and would be the
        same pixel data given to an appropriate NiPixelData object. 

        See SetStreamData.

        @param pucData The data stream.
        @param uiDataLength The total length in bytes of the stream.
    */
    inline void SetLowDetailDiffuseData(NiUInt8* pucData, NiUInt32 uiDataLength);

    /**
        Set the data for the low detail normal texture stream of the current 
        block. The data stored here is to be in RGBA format and would be the
        same pixel data given to an appropriate NiPixelData object. 

        See SetStreamData.

        @param pucData The data stream.
        @param uiDataLength The total length in bytes of the stream.
    */
    inline void SetLowDetailNormalData(NiUInt8* pucData, NiUInt32 uiDataLength);
   
    /**
        Get the data for the height stream in the current
        block.

        See GetStreamData.

        @note Can only be called on a readable file.

        @param[out] uiDataLength A reference to a variable to place the length of
            the stream in.
        @return A pointer to the data stored in the stream.
    */
    inline float* GetHeightData(NiUInt32& uiDataLength);

    /**
        Get the data for the normal stream in the current block.

        See GetStreamData.

        @note Can only be called on a readable file.

        @param[out] uiDataLength A reference to a variable to place the length of
            the stream in.
        @return A pointer to the data stored in the stream.
    */
    inline float* GetNormalData(NiUInt32& uiDataLength);

    /**
        Get the data for the tangent stream in the current
        block.

        See GetStreamData.

        @note Can only be called on a readable file.

        @param[out] uiDataLength A reference to a variable to place the length of
            the stream in.
        @return A pointer to the data stored in the stream.
    */
    inline float* GetTangentData(NiUInt32& uiDataLength);

    /**
        Get the data for the morphing height stream in the current
        block.

        See GetStreamData.

        @note Can only be called on a readable file.

        @param[out] uiDataLength A reference to a variable to place the length of
            the stream in.
        @return A pointer to the data stored in the stream.
    */
    inline float* GetMorphHeightData(NiUInt32& uiDataLength);

    /**
        Get the data for the morphing normal stream in the current
        block.

        See GetStreamData.

        @note Can only be called on a readable file.

        @param[out] uiDataLength A reference to a variable to place the length of
            the stream in.
        @return A pointer to the data stored in the stream.
    */
    inline float* GetMorphNormalData(NiUInt32& uiDataLength);

    /**
        Get the data for the morphing tangent stream in the current
        block.

        See GetStreamData.

        @note Can only be called on a readable file.

        @param[out] uiDataLength A reference to a variable to place the length of
            the stream in.
        @return A pointer to the data stored in the stream.
    */
    inline float* GetMorphTangentData(NiUInt32& uiDataLength);

    /**
        Get the data for the surface index stream in the current
        block. 

        See GetStreamData.

        @note Can only be called on a readable file.
        @param uiDataLength is the amount of data to read in bytes
        @return A pointer to the data stored in the stream.
    */
    inline NiUInt32* GetSurfaceIndexData(NiUInt32& uiDataLength);

    /**
        Get the data for the blend mask. Each char is the greyscale
        value of the mask at that pixel position. Essentially the stream
        contains the exact pixel values from the relevant blend mask texture.

        See GetStreamData.

        @note Can only be called on a readable file.
        @param uiDataLength is the amount of data to read in bytes
        @return A pointer to the data stored in the stream.
    */
    inline NiUInt8* GetBlendMaskData(NiUInt32& uiDataLength);

    /**
        Get the data for the low detail diffuse texture. Essentially the stream
        contains the exact pixel values from the relevant low detail texture.

        See GetStreamData.

        @note Can only be called on a readable file.
        @param uiDataLength is the amount of data to read in bytes
        @return A pointer to the data stored in the stream.
    */
    inline NiUInt8* GetLowDetailDiffuseData(NiUInt32& uiDataLength);

    /**
        Get the data for the low detail normal texture. Essentially the stream
        contains the exact pixel values from the relevant low detail texture.

        See GetStreamData.

        @note Can only be called on a readable file.
        @param uiDataLength is the amount of data to read in bytes
        @return A pointer to the data stored in the stream.
    */
    inline NiUInt8* GetLowDetailNormalData(NiUInt32& uiDataLength);

    /**
        Return the number of blocks that were stored before the current 
        block

        See PopBlock

        @return the Current block ID
    */
    inline NiUInt32 GetBlockID() const;

    /**
        @return the level of detail region that this block resides in.
        (0 = Lowest detail)
    */
    inline NiUInt32 GetBlockLevel() const;

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

    // File data:
    /// The file object to access the file through
    efd::File *m_pkFile;
    /// The current position in the file
    NiUInt32 m_ulFilePosition;
    /// The header present at the beginning of this file
    FileHeader m_kFileHeader;
    /// The name of the file that was opened
    NiString m_kSectorFile;

    /// Iteration Stack/Queue - Stack in read mode, Queue in write mode
    NiTPointerList<NiUInt32> m_kPositionStack;

    // Current block data:
    /// The number of blocks stored before this block in the file
    NiInt32 m_iCurrentBlockID;
    /// The LOD region of the file that this block resides in
    NiInt32 m_iCurrentBlockLevel;
    /// The BlockID that this LOD region began
    NiInt32 m_iCurrentBlockLevelIndex;
    /// The current block's header
    BlockHeader m_kCurrentBlockHeader;

    /// Array of data stream headers for this block
    DataStreamHeader m_akStreamHeader[STREAM_MAX_NUMBER];

    /// Array of pointers to the data for each stream in this block
    void* m_apvStreamData[STREAM_MAX_NUMBER];
};

#include "NiTerrainSectorFileVersion5.inl"

#endif // NITERRAINSECTORFILE_H
