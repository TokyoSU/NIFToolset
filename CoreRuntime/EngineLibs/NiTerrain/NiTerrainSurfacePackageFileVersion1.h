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
#ifndef NITERRAINSURFACEPACKAGEFILEVERSION1_H
#define NITERRAINSURFACEPACKAGEFILEVERSION1_H

#include "NiTerrainLibType.h"
#include "NiTerrainFileInterface.h"
#include "NiMetaData.h"
#include "NiSurface.h"

#include <NiMemManager.h>
#include <NiRefObject.h>
#include <NiUniversalTypes.h>
#include <efd/utf8string.h>

/**
    This class is used to stream a surface package's data to/from disk. 
    The class provides basic versioning support to allow backwards compatibility with older
    package formats.

    This version was where this file was moved back to CoreRuntime and includes prebuilt 
    textures for it's surfaces.
*/
class NITERRAIN_ENTRY NiITerrainSurfacePackageFileVersion1 : public NiTerrainFileInterface
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:
    
    /// The version of the reader interface that this class presents.
    static const FileVersion ms_InterfaceVersion = FileVersion(1);

    /// The package file identifier
    struct FileIdentifier
    {
        /// The filename of the package to be opened
        efd::utf8string m_kPackageFile;
        /// The storage policy to use when opening files
        NiTerrainStoragePolicy* m_pkStoragePolicy;
    };

    /** 
        Surface package file iterator interface
    */
    NiITerrainSurfacePackageFileVersion1();

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
        A structure with a list of enumeration values describing discrete data fields that may be 
        loaded from the file.
    */
    struct DataField
    {
        /// The enumeration containing a list of data field type ID's
        enum Value
        {
            /// Indicates all package configuration information and surface/texture information.
            SURFACE_CONFIG = 1,

            /// Indicates the set of pre-compiled textures that have been saved into the format.
            COMPILED_TEXTURES = 2
        };
    };

    /**
        When reading from the file, the first operation to perform upon the class
        is a call to this Precache function to indicate what data should be loaded from disk.

        @param uiDataFields A bitmask combining values from the DataField enumeration.
    */
    virtual void Precache(efd::UInt32 uiDataFields);

    /**
        Read the name of the package from the file

        @param kPackageName The variable to store the package name in.
        @param uiIteration The iteration count of the last save.
        @return false If the package name could not be read.
    */
    virtual bool ReadPackageConfig(efd::utf8string& kPackageName, efd::UInt32& uiIteration);

    /**
        Write the name of the package to the file.

        @param kPackageName The name of the package.
        @param uiIteration The iteration count of the last save.
    */
    virtual void WritePackageConfig(const efd::utf8string& kPackageName, 
        efd::UInt32 uiIteration);

    /**
        Read the number of surfaces from the file.

        @param uiNumSurfaces The variable to store the number of surfaces in
        @return True if the number of surfaces could be determined.
    */
    virtual bool ReadNumSurfaces(efd::UInt32& uiNumSurfaces);

    /**
        Write the number of surfaces to the file

        @param uiNumSurfaces The number of surfaces to be stored in the file.
    */
    virtual void WriteNumSurfaces(const efd::UInt32& uiNumSurfaces);

    /**
        Read the configuration of a single surface from the package by it's index.

        @param uiSurfaceIndex The index of the surface the user is interested in
        @param kName The name of the surface
        @param fTextureTiling The tiling of the main texture
        @param fDetailTiling The tiling of the detail texture
        @param fRotation The rotation of the main texture
        @param fParallaxStrength The strength of the parallax mapping effect
        @param fDistributionMaskStrength The strength of the distribution mask effect
        @param fSpecularPower The power to use in specular calculations
        @param fSpecularIntensity The intensity to use in specular calculations
        @param uiNumDecorationLayers The number of decoration layers assigned to this surface

        @return True if the surface data could be read successfully
    */
    virtual bool ReadSurfaceConfig(efd::UInt32 uiSurfaceIndex,
        efd::utf8string& kName, 
        efd::Float32& fTextureTiling,
        efd::Float32& fDetailTiling,
        efd::Float32& fRotation,
        efd::Float32& fParallaxStrength,
        efd::Float32& fDistributionMaskStrength,
        efd::Float32& fSpecularPower,
        efd::Float32& fSpecularIntensity,
        efd::UInt32& uiNumDecorationLayers);

    /**
        Write the configuration of a single surface to the file by it's index.

        @param uiSurfaceIndex The index of the surface the user is interested in
        @param kName The name of the surface
        @param fTextureTiling The tiling of the main texture
        @param fDetailTiling The tiling of the detail texture
        @param fRotation The rotation of the main texture
        @param fParallaxStrength The strength of the parallax mapping effect
        @param fDistributionMaskStrength The strength of the distribution mask effect
        @param fSpecularPower The power to use in specular calculations
        @param fSpecularIntensity The intensity to use in specular calculations
        @param uiNumDecorationLayers The number of decoration layers assigned to this surface
    */
    virtual void WriteSurfaceConfig(efd::UInt32 uiSurfaceIndex,
        const efd::utf8string& kName, 
        efd::Float32 fTextureTiling,
        efd::Float32 fDetailTiling,
        efd::Float32 fRotation,
        efd::Float32 fParallaxStrength,
        efd::Float32 fDistributionMaskStrength,
        efd::Float32 fSpecularPower,
        efd::Float32 fSpecularIntensity,
        efd::UInt32 uiNumDecorationLayers);

    /**
        Read out the reference to the texture asset stored for a particular slot in the material 
        package.

        @param uiSurfaceIndex The index of the surface the user is interested in
        @param uiSlotID The ID of the texture slot the user is interested in
        @param pkReference An asset reference object to store the relevant data in

        @return True if the data could be successfully read from the file
    */
    virtual bool ReadSurfaceSlot(efd::UInt32 uiSurfaceIndex,
        efd::UInt32 uiSlotID,
        NiTerrainAssetReference* pkReference);

    /**
        Write the reference to a texture asset stored for a particular slot in the material 
        package.

        @param uiSurfaceIndex The index of the surface the user is interested in
        @param uiSlotID The ID of the texture slot the user is interested in
        @param pkReference An asset reference object to write the relevant data from
    */
    virtual void WriteSurfaceSlot(efd::UInt32 uiSurfaceIndex,
        efd::UInt32 uiSlotID,
        const NiTerrainAssetReference* pkReference);

    /**
        Read any metadata that has been assigned to a particular surface

        @param uiSurfaceIndex The index of the surface the user is interested in
        @param kMetaData A metadata object to store the requested information in

        @return True if the data could be read successfully
    */
    virtual bool ReadSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
        NiMetaData& kMetaData);

    /**
        Write a surface's metadata to the file. 

        @param uiSurfaceIndex The index of the surface the user is interested in
        @param kMetaData The metadata object to store inside the file for the surface.
    */
    virtual void WriteSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
        const NiMetaData& kMetaData);

    /**
        Read the compiled textures for a particular surface

        @param uiSurfaceIndex The index of the surface the user is interested in.
        @param aspTextures The texture data.
        @param uiNumTextures The number of textures.
        @return true if the read succeeded.
    */
    virtual bool ReadSurfaceCompiledTextures(efd::UInt32 uiSurfaceIndex,
        NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures);

    /**
        Read the compiled textures for a particular surface

        @param kSurfaceName The name of the surface the user is interested in.
        @param aspTextures The texture data.
        @param uiNumTextures The number of textures.
        @return true if the read succeeded.
    */
    virtual bool ReadSurfaceCompiledTextures(efd::utf8string kSurfaceName, 
        NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures);

    /**
        Write a surface's metadata to the file. 

        @param uiSurfaceIndex The index of the surface the user is interested in.
        @param aspTextures The texture data.
        @param uiNumTextures The number of textures.
    */
    virtual void WriteSurfaceCompiledTextures(efd::UInt32 uiSurfaceIndex,
        NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures);
};

/**
    This class is used to stream a surface package's data to/from disk. 
    The class provides basic versioning support to allow backwards compatibility with older
    package formats.
*/
class NITERRAIN_ENTRY NiTerrainSurfacePackageFileVersion1 : 
    public NiITerrainSurfacePackageFileVersion1
{
public:

    /// Constructor
    NiTerrainSurfacePackageFileVersion1();

    /// Destructor.
    virtual ~NiTerrainSurfacePackageFileVersion1();

    /// @see NiTerrainFileInterface
    /// @{
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths);
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);
    virtual void Close();
    /// @}

    /// @see NiTerrainSurfacePackageFile
    /// @{
    virtual void Precache(efd::UInt32 uiDataFields);
    virtual bool ReadPackageConfig(efd::utf8string& kPackageName, efd::UInt32& uiIteration);
    virtual bool ReadNumSurfaces(efd::UInt32& uiNumSurfaces);
    virtual bool ReadSurfaceConfig(efd::UInt32 uiSurfaceIndex,
        efd::utf8string& kName, 
        efd::Float32& fTextureTiling,
        efd::Float32& fDetailTiling,
        efd::Float32& fRotation,
        efd::Float32& fParallaxStrength,
        efd::Float32& fDistributionMaskStrength,
        efd::Float32& fSpecularPower,
        efd::Float32& fSpecularIntensity,
        efd::UInt32& uiNumDecorationLayers);
    virtual bool ReadSurfaceSlot(efd::UInt32 uiSurfaceIndex,
        efd::UInt32 uiSlotID,
        NiTerrainAssetReference* pkReference);
    virtual bool ReadSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
        NiMetaData& kMetaData);
    virtual bool ReadSurfaceCompiledTextures(efd::UInt32 uiSurfaceIndex,
        NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures);
    virtual bool ReadSurfaceCompiledTextures(efd::utf8string kSurfaceName, 
        NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures);

    virtual void WritePackageConfig(const efd::utf8string& kPackageName, efd::UInt32 uiIteration);
    virtual void WriteNumSurfaces(const efd::UInt32& uiNumSurfaces);
    virtual void WriteSurfaceConfig(efd::UInt32 uiSurfaceIndex,
        const efd::utf8string& kName, 
        efd::Float32 fTextureTiling,
        efd::Float32 fDetailTiling,
        efd::Float32 fRotation,
        efd::Float32 fParallaxStrength,
        efd::Float32 fDistributionMaskStrength,
        efd::Float32 fSpecularPower,
        efd::Float32 fSpecularIntensity,
        efd::UInt32 uiNumDecorationLayers);
    virtual void WriteSurfaceSlot(efd::UInt32 uiSurfaceIndex,
        efd::UInt32 uiSlotID,
        const NiTerrainAssetReference* pkReference);
    virtual void WriteSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
        const NiMetaData& kMetaData);
    virtual void WriteSurfaceCompiledTextures(efd::UInt32 uiSurfaceIndex,
        NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures);
    /// @}

protected:

    /**
        Attempt to detect what version of this file format this file is.

        @param kID The file to check the version of.
        @return True if the file can be read by this class.
    */
    virtual bool DetectFileVersion(FileIdentifier kID);

    // Typedef to help when saving the list of compiled textures
    typedef efd::map<efd::utf8string, efd::UInt32> StreamOffsetTable;

    /**
        A structure to store the data relevant to surface texture slots
    */
    struct TextureSlotData
    {
        /// The last relative path known for the texture file
        efd::utf8string m_kLastRelativePath;
        /// The assetID of the texture file
        efd::utf8string m_kAssetID;
    };

    /**
        A structure to store the all the data about a surface in the file
    */
    struct SurfaceData : public NiMemObject
    {
        SurfaceData();

        efd::utf8string m_kName;
        efd::Float32 m_fTextureTiling;
        efd::Float32 m_fDetailTiling;
        efd::Float32 m_fRotation;
        efd::Float32 m_fParallaxStrength;
        efd::Float32 m_fDistributionMaskStrength;
        efd::Float32 m_fSpecularPower;
        efd::Float32 m_fSpecularIntensity;
        efd::UInt32 m_uiNumDecorationLayers;
        TextureSlotData m_akTextureSlots[NiSurface::NUM_SURFACE_MAPS];
        NiMetaData m_kMetaData;
        NiTexturePtr m_aspTextures[NiSurface::NUM_SURFACE_TEXTURES];
    };

    /**
        Initialize the file reader. 

        @returns true if the initialization was successful
    */
    virtual bool Initialize();

    /**
        Read the package wide information from the file
    */
    bool ReadConfiguration(efd::TiXmlElement* pkRootElement);

    /**
        Read all the surfaces stored in the file ready to be retrieved
    */
    bool ReadSurfaces(efd::TiXmlElement* pkRootElement);

    /**
        Read a single surface in from the file
    */
    bool ReadSurface(const efd::TiXmlElement* pkRootElement, SurfaceData& kTempSurfaceData);

    /**
        Read a set of precompiled textures from the binary file
    */
    bool ReadCompiledTextures();

    /**
        Write the file header
    */
    bool WriteFileHeader();

    /**
        Write the package information to the file
    */
    bool WritePackage();

    /**
        Write the surface data to the file
    */
    bool WriteSurfaces();

    /**
        Write the set of precompiled textures to the binary file
    */
    bool WriteCompiledTextures();

    /**
        Write a compiled texture table at a certain point in the file
    */
    bool WriteCompiledTextureTable(efd::BinaryStream& kStream, StreamOffsetTable& kOffsetMap);

    /**
        Generate compiled texture filename
    */
    efd::utf8string GenerateCompiledTextureFilename();

    /// The file object to access the file through
    efd::TiXmlDocument m_kFile;
    /// Has the configuration data in this file been read from the file yet?
    bool m_bConfigurationValid;
    /// Iteration count
    efd::UInt32 m_uiIteration;
    /// The list of surfaces
    typedef efd::vector<SurfaceData*> SurfaceList;
    SurfaceList m_kSurfaces;
    /// A map of surface names to offsets for the compiled texture streams
    StreamOffsetTable m_kCompiledStreamOffsets;
    /// The package name
    efd::utf8string m_kPackageName;
    // Filename
    efd::utf8string m_kPackageFilename;
};

#endif // NITERRAINSURFACEPACKAGEFILEVERSION1_H
