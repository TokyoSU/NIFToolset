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

#ifndef NITERRAINFILEVERSION3_H
#define NITERRAINFILEVERSION3_H

#include <efdXML/tinyxml.h>
#include <NiBoxBV.h>

#include "NiTerrainLibType.h"
#include "NiTerrainFileInterface.h"
#include "NiTerrainFileVersion4.h"

/** 
    The file format interface for a file format implementing storage for version 3 files.
    This interface marks the point at which surface references were made to store assetID's and
    their last known relative paths in the XML along with the package's iteration count.
*/
class NITERRAIN_ENTRY NiITerrainFileVersion3 : public NiTerrainFileInterface
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    /// The version of the reader interface that this class presents.
    static const FileVersion ms_InterfaceVersion = FileVersion(3);
    /// Typedef to copy the file identifier used in version 4 to this class
    typedef NiITerrainFileVersion4::FileIdentifier FileIdentifier;

    /// @see NiFileInterface
    virtual NiFileInterface* AdaptToNextVersion();

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
        @param uiSurfaceCount the total number of surfaces in the terrain's surface index
        @return true when reading was successful
    */
    virtual bool ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
        efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
        float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
        float& fLowDetailSpecularIntensity, efd::UInt32& uiSurfaceCount);

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
        @param uiSurfaceCount the total number of surfaces in the terrain's surface index
    */
    virtual void WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
        efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
        float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
        float fLowDetailSpecularIntensity, efd::UInt32 uiSurfaceCount);

    /**
        Read the data related to a particular surface index

        @param uiSurfaceIndex the index of the surface we are interested in
        @param pkPackageRef A pointer to an reference object to receive the package details
        @param kSurfaceID A unique surface ID inside the given package
        @param uiIteration The iteration of the package when the terrain was saved
        @return True when successful
    */
    virtual bool ReadSurface(NiUInt32 uiSurfaceIndex, NiTerrainAssetReference* pkPackageRef, 
        NiFixedString& kSurfaceID, efd::UInt32& uiIteration);

    /**
        Write the data related to a particular surface index

        @param uiSurfaceIndex the index of the surface we are interested in
        @param pkPackageRef A pointer to a reference object that holds the package details
        @param kSurfaceID A unique surface ID inside the given package
        @param uiIteration The iteration of the package being referenced
    */
    virtual void WriteSurface(NiUInt32 uiSurfaceIndex, NiTerrainAssetReference* pkPackageRef, 
        NiFixedString kSurfaceID, efd::UInt32 uiIteration);

protected:

    /// Constructor
    NiITerrainFileVersion3();
};

/**
    This class is used to adapt a NiITerrainFileVersion3 interface object to present a
    NiITerrainFileVersion4 interface to the user.
*/
class NiITerrainFileVersion3To4Adapter : public NiITerrainFileVersion4
{
public:
    
    /// Constructor
    NiITerrainFileVersion3To4Adapter(NiITerrainFileVersion3* pBaseVersion);

    /// @see NiITerrainFileVersion4
    /// @{
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);
    virtual void Close();
    inline FileVersion GetFileVersion() const;
    virtual FileVersion GetInterfaceVersion() const;
    inline bool IsReady() const;
    virtual bool IsWritable() const;
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

private:

    /// The class implementing the previous interface
    efd::SmartPointer<NiITerrainFileVersion3> m_spPrevVersion;
};

/** 
    The class is used to iterate over and write terrain files using the NiITerrainFileVersion3 
    interface. 
 */
class NITERRAIN_ENTRY NiTerrainFileVersion3 : public NiITerrainFileVersion3
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    /// Constructor
    NiTerrainFileVersion3();
    /// Destructor
    virtual ~NiTerrainFileVersion3();

    /**
        Open function to open the file at the given identifier for access using the given 
        access mode. 

        @param kID The file identifier of the file the caller wishes to open
        @param eAccessMode The type of access to the file required (read/write)
        @return WRONG_VERSION if the file is not the correct version. SUCCESS if the file
        can be opened, FAIL if the file cannot be read/written.
    */
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);

    /// @name NewInterface
    /// @see NiITerrainFileVersion3
    /// @{
    virtual void Close();
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths);
    virtual bool ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
        efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
        float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
        float& fLowDetailSpecularIntensity);
    virtual void WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
        efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
        float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
        float fLowDetailSpecularIntensity, efd::UInt32 uiSurfaceCount);
    virtual bool ReadSurface(NiUInt32 uiSurfaceIndex, NiTerrainAssetReference* pkPackageRef, 
        NiFixedString& kSurfaceID, efd::UInt32& uiIteration);
    virtual void WriteSurface(NiUInt32 uiSurfaceIndex, NiTerrainAssetReference* pkPackageRef, 
        NiFixedString kSurfaceID, efd::UInt32 uiIteration);
    /// @}

protected:

    /**
        Return true if the file identified by the identifier is the correct version for reading
        by this file reader.

        @param kID The file to attempt to read
    */
    virtual bool DetectFileVersion(FileIdentifier kID);

    /**
        Initialize the file reading process. Returns false if the initialization did
        not succeed.
    */
    virtual bool Initialize();

    /**
        The original interface for ReadConfiguration
    */
    virtual bool ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
        efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
        float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
        float& fLowDetailSpecularIntensity, efd::UInt32& uiSurfaceCount);

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

    
    /**
        A structure to store the information for each surface reference inside.        
    */
    struct SurfaceReference: public NiMemObject
    {
        /// Constructor
        SurfaceReference();

        /// Is this reference valid
        bool m_bValid;
        /// The assetID of the package being referenced
        efd::utf8string m_kPackageAssetID;
        /// The relative path of the package being referenced
        efd::utf8string m_kPackageRelativePath;
        /// The name of the surface being referenced
        efd::utf8string m_kSurfaceName;
        /// The iteration of the package when the terrain was last saved
        efd::UInt32 m_uiIteration;
    };

    /// Filename to use for the terrain config file
    static const char* ms_pcTerrainConfigFile;

    /// The file object to access the file through
    efd::TiXmlDocument m_kFile;

    /// Typedef to store a map of surface references for specific positions
    typedef efd::map<efd::SInt32, SurfaceReference> SurfaceReferenceMap;
    /// The map of surface references
    SurfaceReferenceMap m_kSurfaceReferences;

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
    /// The archive path in which the terrain file is stored
    efd::utf8string m_kTerrainArchive;
};

#include "NiTerrainFileVersion3.inl"

#endif // NITERRAINFILEVERSION3_H
