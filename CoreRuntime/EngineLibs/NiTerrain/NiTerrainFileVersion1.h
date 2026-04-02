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

#ifndef NITERRAINFILEVERSION1_H
#define NITERRAINFILEVERSION1_H

#include <efdXML/tinyxml.h>
#include <NiBoxBV.h>

#include "NiTerrainLibType.h"
#include "NiTerrainFileVersion2.h"

/** 
    The class is used to iterate over and write terrain files.
    This version is the first version of the terrain file format, that simply stores
    the surface references for a particular terrain object.
 */
class NITERRAIN_ENTRY NiTerrainFileVersion1 : public NiTerrainFileVersion2
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    /// The version of the reader interface that this class presents.
    static const FileVersion ms_InterfaceVersion = FileVersion(1);

    /// Constructor
    NiTerrainFileVersion1();
    /// Destructor
    virtual ~NiTerrainFileVersion1();

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

    /// @see NiTerrainFileVersion2
    /// @{
    virtual bool ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
        efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
        float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
        float& fLowDetailSpecularIntensity, efd::UInt32& uiSurfaceCount);
    virtual bool ReadSurface(NiUInt32 uiSurfaceIndex, NiFixedString& kPackageID, 
        NiFixedString& kSurfaceID);
    /// @}

    /**
        Fetch the number of indexes used to store surfaces in the surface index
        @return the number of surfaces in the file
    */
    virtual NiUInt32 GetNumSurfaces() const;

    /**
        Fetch the surface package for a specified surface index
        @param uiSurfaceIndex the surface index we want the package for
        @return the package name
    */
    virtual NiFixedString GetSurfacePackage(NiUInt32 uiSurfaceIndex) const;

    /**
        Fetch the surface name for a specified surface index
        @param uiSurfaceIndex the surface index we want the surface name for
        @return the surface name
    */
    virtual NiFixedString GetSurfaceName(NiUInt32 uiSurfaceIndex) const;

    /**
        Set the surface package for a specified surface index
        Only available on a writable file.

        @param uiSurfaceIndex The surface index for the associated package name
        @param kPackage The name of the package this surface belongs to
    */
    virtual void SetSurfacePackage(NiUInt32 uiSurfaceIndex, NiFixedString kPackage);

    /**
        Set the surface name for a specified surface index
        Only available on a writable file.

        @param uiSurfaceIndex The surface index for the associated surface name
        @param kName The name of this surface
    */
    virtual void SetSurfaceName(NiUInt32 uiSurfaceIndex, NiFixedString kName);

protected:

    /**
        Return true if the file identified by the identifier is the correct version for reading
        by this file reader.
    */
    virtual bool DetectFileVersion(FileIdentifier kID);

    /**
        Generate the terrain configuration filename

        @return the generated file name
    */
    NiString GenerateTerrainConfigFilename();

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
    virtual bool WriteFileHeader();

    /**
        Write the surface index to the terrain file
        @return true when successful
    */
    virtual bool WriteSurfaceIndex();

    /**
        Read the list of surfaces from a section of a DOM document.
        @param pkRootElement xml element to read surface indices from
        @return true when successful
    */
    bool ReadSurfaceIndex(const efd::TiXmlElement* pkRootElement);

    // File data:
    /// The file object to access the file through
    efd::TiXmlDocument m_kFile;

    // Current File Data:
    /// The array of surface packages - per index
    NiTObjectArray<NiFixedString> m_kSurfacePackageArray;
    /// The array of surface names - per index
    NiTObjectArray<NiFixedString> m_kSurfaceNameArray;
};

#include "NiTerrainFileVersion1.inl"

#endif // NITERRAINFILEVERSION1_H
