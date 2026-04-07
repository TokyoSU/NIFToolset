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

#ifndef NITERRAINFILEVERSION2_H
#define NITERRAINFILEVERSION2_H

#include <efdXML/tinyxml.h>
#include <NiBoxBV.h>

#include "NiTerrainLibType.h"
#include "NiTerrainFileVersion3.h"

/** 
    The class is used to iterate over and write terrain files.
    This version of the format marks when the terrain configuration information was also stored
    in the config file, and its name was renamed to root.terrain.
 */
class NITERRAIN_ENTRY NiTerrainFileVersion2 : public NiITerrainFileVersion3
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    /// The version of the reader interface that this class presents.
    static const FileVersion ms_InterfaceVersion = FileVersion(2);

    /// Constructor
    NiTerrainFileVersion2();
    /// Destructor
    virtual ~NiTerrainFileVersion2();

    /**
        Open function to open the file at the given identifier for access using the given 
        access mode. 

        @param kID The file identifier of the file the caller wishes to open
        @param eAccessMode The type of access to the file required (read/write)
        @return WRONG_VERSION if the file is not the correct version. SUCCESS if the file
        can be opened, FAIL if the file cannot be read/written.
    */
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);

    /// @see NiTerrainFileInterface
    /// @{
    virtual void Close();
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths);
    /// @}

    /// @see NiITerrainFileVersion3
    /// @{
    virtual bool ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
        efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
        float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
        float& fLowDetailSpecularIntensity, efd::UInt32& uiSurfaceCount);
    virtual void WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
        efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
        float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
        float fLowDetailSpecularIntensity, efd::UInt32 uiSurfaceCount);
    virtual bool ReadSurface(NiUInt32 uiSurfaceIndex, NiTerrainAssetReference* pkPackageRef, 
        NiFixedString& kSurfaceID, efd::UInt32& uiIteration);
    /// @}

protected:
    
    /**
        Detect the version of the file in a particular terrain archive

        @param kID The file for which we want check the version of
        @return True if this class can read this file format
    */
    virtual bool DetectFileVersion(FileIdentifier kID);

    /**
        Initialize the class by opening the file and reading the first couple
        of headers. 

        @return true if the class was successfully initialized.
    */
    virtual bool Initialize();

    /// Deprecated read interface
    virtual bool ReadSurface(NiUInt32 uiSurfaceIndex, NiFixedString& kPackageID, 
        NiFixedString& kSurfaceID);

    /// Deprecated write interface
    virtual void WriteSurface(NiUInt32 uiSurfaceIndex, NiFixedString kPackageID, 
        NiFixedString kSurfaceID);

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
        Write the list of surfaces to a section of a DOM document.
        @return true when successful
    */
    bool WriteSurfaceIndex();

    /**
        Read the configuration from the file
        @param pkRootElement the Xml element to read the configuration from
        @return true when successful
    */
    bool ReadConfiguration(efd::TiXmlElement* pkRootElement);
    
    /**
        Read the list of surfaces from a section of a DOM document.
        @param pkRootElement the Xml element to read the surface index from
        @return true when successful
    */
    bool ReadSurfaceIndex(efd::TiXmlElement* pkRootElement);

    /**
        Generate the terrain config file's filename
        @return the generated file name
    */
    inline NiString GenerateTerrainConfigFilename();

    /// The file object to access the file through
    static const char* ms_pcTerrainConfigFile;

    // File data:
    /// The file object to access the file through
    efd::TiXmlDocument m_kFile;

    // Current File Data:
    /// The array of surface packages - per index
    NiTObjectArray<NiFixedString> m_kSurfacePackageArray;
    /// The array of surface names - per index
    NiTObjectArray<NiFixedString> m_kSurfaceNameArray;

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
    /// The number of surfaces on the terrain
    efd::UInt32 m_uiSurfaceCount;
    /// Flag to signal that the configuration is valid
    bool m_bConfigurationValid;
    /// The archive path of the terrain file being loaded
    efd::utf8string m_kTerrainArchive;
};

#include "NiTerrainFileVersion2.inl"

#endif // NITERRAINFILEVERSION2_H
