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

#ifndef NITERRAINFILEINTERFACE_H
#define NITERRAINFILEINTERFACE_H

#include "NiTerrainLibType.h"
#include "NiTerrainStoragePolicy.h"
#include "NiFileVersionRegistry.h"

/**
    This class is used as a base class for all NiTerrain file format interfaces. 
    Each file format interface must derive from this interface and successfully implement it.
*/
class NITERRAIN_ENTRY NiTerrainFileInterface : public NiFileInterface
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    /**
        Constructor
        @param kVersion The version of the top level interface being constructed.
    */
    NiTerrainFileInterface(FileVersion kVersion);

    /// Destructor
    virtual ~NiTerrainFileInterface();

    /**
        Open the file. Every file that is attempting to be opened should have this function
        called upon it by the subclass. 

        @param pkStoragePolicy The storage policy being used for accessing files
        @param eAccessMode The access mode to use in loading this file
    */
    void Open(NiTerrainStoragePolicy* pkStoragePolicy, efd::File::OpenMode eAccessMode);
    
    /**
        Close the file. Every file that has had 'open' called upon it, must have 'Close' called
        upon it before destruction.

        This will notify the storage policy listeners that the file has been closed
    */
    virtual void Close();
    
    /// @return The file version of the underlying data
    virtual FileVersion GetFileVersion() const;
    
    /// @return The version number of the interface presented
    virtual FileVersion GetInterfaceVersion() const;
    
    /// @return True if this file is ready to be read/written
    virtual bool IsReady() const;
    
    /// @return True if this file is opened for write
    virtual bool IsWritable() const;
    
    /**
        Populate a list of all the file paths that this file interface uses to store it's data.

        @param kFilePaths The set of file paths used
    */
    virtual void GetFilePaths(efd::set<efd::utf8string>& kFilePaths) = 0;

protected:

    /**
        Initialize the file.
        This will notify the storage policy listeners that the file has been opened

        @return True if initialization was successful.
    */
    virtual bool Initialize();

    /// The storage policy to be used when interacting with files
    NiTerrainStoragePolicy* m_pkStoragePolicy;
    /// The access mode to use when opening files
    efd::File::OpenMode m_eAccessMode;
    /// The version of the file
    FileVersion m_kFileVersion;
    /// The version of the interface
    FileVersion m_kInterfaceVersion;
    /// A flag to determine if the file is open or not
    bool m_bOpen;
    /// A flag to determine the success/failure of any file operations
    NiTerrainStoragePolicy::IOSuccessCode::Value m_eSuccess;
    /// A flag to detect if the implementor of this interface is an adapter or a file reader
    bool m_bIsImplementation;
};

#include "NiTerrainFileInterface.inl"

#endif // NITERRAINFILEINTERFACE_H
