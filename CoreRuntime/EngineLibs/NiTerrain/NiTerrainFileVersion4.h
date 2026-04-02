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

#ifndef NITERRAINFILEVERSION4_H
#define NITERRAINFILEVERSION4_H

#include <efdXML/tinyxml.h>
#include <NiBoxBV.h>

#include "NiTerrainLibType.h"
#include "NiTerrainFileInterface.h"

/** 
    The file format interface for a file format implementing storage for version 4 files.
    This interface marks the point at which surface references were moved back to sector files 
    to alleviate issues with many users editing different sectors.
*/
class NITERRAIN_ENTRY NiITerrainFileVersion4 : public NiTerrainFileInterface
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    /// The version of the reader interface that this class presents.
    static const FileVersion ms_InterfaceVersion = FileVersion(4);

    /// The file identifier used to reference terrain files
    struct FileIdentifier
    {
        /// The archive path of the terrain
        efd::utf8string m_kArchivePath;
        /// The storage policy to use when opening files
        NiTerrainStoragePolicy* m_pkStoragePolicy;
    };
    
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
        Read the configuration of the terrain

        @param uiSectorSize the size of the sector in verts
        @param uiNumLOD the number of LOD on the stored terrain
        @param uiMaskSize the size of blend masks per sector
        @param uiLowDetailSize the size of low detail diffuse textures per sector
        @param fMinElevation the minimum elevation of the terrain
        @param fMaxElevation the maximum elevation of the terrain
        @param fVertexSpacing the spacing between verts of the terrain (XY scaling)
        @param fLowDetailSpecularPower the specular power of the low detail texture
        @param fLowDetailSpecularIntensity the specular intensity of the low detail texture
        @return true when reading was successful
    */
    virtual bool ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
        efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
        float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
        float& fLowDetailSpecularIntensity);

    /**
        Write the configuration of the terrain

        @param uiSectorSize the size of the sector in verts
        @param uiNumLOD the number of LOD on the stored terrain
        @param uiMaskSize the size of blend masks per sector
        @param uiLowDetailSize the size of low detail diffuse textures per sector
        @param fMinElevation the minimum elevation of the terrain
        @param fMaxElevation the maximum elevation of the terrain
        @param fVertexSpacing the spacing between verts of the terrain (XY scaling)
        @param fLowDetailSpecularPower the specular power of the low detail texture
        @param fLowDetailSpecularIntensity the specular intensity of the low detail texture
    */
    virtual void WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
        efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
        float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
        float fLowDetailSpecularIntensity);

protected:

    /// Constructor
    NiITerrainFileVersion4();
};

/** 
    The class is used to iterate over and write terrain sector files.
 */
class NITERRAIN_ENTRY NiTerrainFileVersion4 : public NiITerrainFileVersion4
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    /// Constructor
    NiTerrainFileVersion4();
    
    /// Destructor.
    virtual ~NiTerrainFileVersion4();

    /// @see NiITerrainFileVersion4
    /// @{
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);
    virtual void Close();
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths);
    virtual bool ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
        efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
        float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
        float& fLowDetailSpecularIntensity);
    virtual void WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
        efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
        float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
        float fLowDetailSpecularIntensity);
    /// @}

protected:

    /**
        Detect the version of the file in a particular terrain archive

        @param kID The file to check the file version of.
        @return True if this class can read this file.
    */
    virtual bool DetectFileVersion(FileIdentifier kID);

    /**
        Initialize the class by opening the file and reading the first couple
        of headers. 

        @return true if the class was successfully initialized.
    */
    virtual bool Initialize();

    /**
        Write the file header to the terrain file
        @return true when successful
    */
    bool WriteFileHeader();
    
    /**
        Write the configuration out to file
        @return true when successful
    */
    bool WriteConfiguration();

    /**
        Read the configuration from the file
        @param pkRootElement the Xml element to read the configuration from
        @return true when successful
    */
    bool ReadConfiguration(efd::TiXmlElement* pkRootElement);

    /**
        Generate the terrain config file's filename
        @return the generated file name
    */
    inline NiString GenerateTerrainConfigFilename();

    /// Filename to use for the terrain config file
    static const char* ms_pcTerrainConfigFile;

    // File data:
    efd::utf8string m_kTerrainArchive;
    /// The file object to access the file through
    efd::TiXmlDocument m_kFile;
    /// The sector size in verts
    efd::UInt32 m_uiSectorSize;
    /// The number of LOD per sector
    efd::UInt32 m_uiNumLOD;
    /// The size of a blend mask per sector
    efd::UInt32 m_uiMaskSize;
    /// The size of a low detail diffuse texture per sector
    efd::UInt32 m_uiLowDetailSize;
    /// The minimum elevation of a terrain
    efd::Float32 m_fMinElevation;
    /// The maximum elevation of a terrain
    efd::Float32 m_fMaxElevation;
    /// The spacing between vertices of the terrain
    efd::Float32 m_fVertexSpacing;
    /// The low detail specular power
    efd::Float32 m_fLowDetailSpecularPower;
    /// The low detail specular intensity
    efd::Float32 m_fLowDetailSpecularIntensity;
    /// Flag to signal that the configuration is valid
    bool m_bConfigurationValid;
};

#include "NiTerrainFileVersion4.inl"

#endif // NITERRAINFILEVERSION4_H
