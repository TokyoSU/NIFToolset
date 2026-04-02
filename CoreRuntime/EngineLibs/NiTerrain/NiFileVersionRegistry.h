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
#ifndef NIFILEVERSIONREGISTRY_H
#define NIFILEVERSIONREGISTRY_H

#include <efd/UniversalTypes.h>
#include <NiRefObject.h>
#include <NiRTTI.h>

#include "NiTerrainLibType.h"

/**
    This class defines a base file interface to be implemented by all file format interfaces
    to be used with the NiFileVersionRegistry class. It provides a basic interface to allow
    adaptation from old versions to new and querying of the interfaces themselves. 

    Note. Every derived class must provide a "FileIdentifier" structure, and a function matching
    the signature: 
       OpenErrorCode Open(FileIdentifier kFileID, efd::File::AccessMode eAccessMode);
    This function will attempt to open the file for a particular type of access, and return
    success/failure or a WrongVersion if the file was not the correct version for this particular
    reader.
*/
class NITERRAIN_ENTRY NiFileInterface : public NiRefObject
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRootRTTI(NiFileInterface);
    /// @endcond

public:
    
    /// Typedef to define a type to use for all FileVersion numbers in the file versioning system.
    typedef efd::UInt32 FileVersion;

    /// An enumeration to define the success/fail of an Open command on the NiFileInterface. 
    enum OpenErrorCode
    {
        SUCCESS,
        FAIL,
        WRONG_VERSION
    };
    
    /**
        Adapt this file interface to the next version in the chain. If no adaptation is possible, 
        then this function will return NULL. The returned value should be a NiFileInterface object
        that implements the next version interface and may be dynamically cast to that next version.
    */
    virtual NiFileInterface* AdaptToNextVersion();

    /**
        Get the version number of the latest interface that this object currently presents.
    */
    virtual FileVersion GetInterfaceVersion() const = 0;
};

/**
    This class defines a file version registry, where reader classes for each version of a file type
    may be registered such that each version may be loaded and interacted with through the same
    interface. Each version provides it's own adaptation classes to wrap their own interface, and 
    present it through the newer interface. Each new interface is then adapted to the later 
    interface until all file versions may be accessed through a single interface.

    Note this class is used to read data from a single file type, but from multiple versions of 
    that type. 
*/
template <class FileFormatInterface>
class NiFileVersionRegistry : public efd::MemObject
{
    /// @cond EMERGENT_INTERNAL
    EE_DECLARE_CONCRETE_REFCOUNT;
    /// @endcond
    
public:
    /// Typedef to retrieve the latest file format interface that this file registry attempts 
    /// to produce by default. 
    typedef FileFormatInterface CurrentVersionInterface;

    /// Constructor
    NiFileVersionRegistry();
    /// Destructor
    virtual ~NiFileVersionRegistry();

    /**
        Creates a 'FileFormatInterface' object to be used to access the data stored in
        the given identifier, or returns NULL if the identifier points to an unknown file format.

        @param kIdentifier The identifier structure for the file being opened.
        @param eAccessMode the mode to use in opening the file
        @return An interface object to read the file, or NULL if it could not be opened.
    */
    inline FileFormatInterface* OpenFile(
        const typename FileFormatInterface::FileIdentifier& kIdentifier,
        efd::File::OpenMode eAccessMode);

    /**
        Creates a 'FileFormatInterface' object to be used to access the data stored in
        the given identifier, or returns NULL if the identifier points to an unknown file format.

        @param kIdentifier The identifier structure for the file being opened.
        @param bWriteAccess True if the file is to be opened for write access.
        @return An interface object to read the file, or NULL if it could not be opened.
    */
    inline FileFormatInterface* OpenFile(
        const typename FileFormatInterface::FileIdentifier& kIdentifier,
        bool bWriteAccess);

    /**
        Create a file format interface in the format defined by 'InterfaceFormat'. Only file 
        versions lower than the InterfaceFormat can be read via this method. Legacy formats
        are only ever opened in READ_ONLY mode.

        @param kIdentifier The identifier structure for the file being opened.
        @return An interface object to read the file, or NULL if it could not be opened.
    */
    template <class InterfaceFormat>
    inline InterfaceFormat* OpenLegacyFile(
        const typename InterfaceFormat::FileIdentifier& kIdentifier);

    /**
        Register a new file format version reader. 
    */
    template <class FileFormat>
    inline void AddFileFormat(const typename FileFormatInterface::FileVersion& kVersion);

protected:

    /// Structure definition to store the relevant data required for each file reader.
    struct FileFormatInfo
    {
        /// Typedef for the format object creation function required for the factory
        typedef NiFileInterface* (*CreateFormatInstance)();
        /// A create function to create an instance of a particular format reader.
        CreateFormatInstance m_createFunction;
    };

    /// Generalized open function used by the two public open functions.
    template <class Interface>
    inline Interface* Open(const typename Interface::FileIdentifier& identifier,
        efd::File::OpenMode mode);

    // Templated function used to create a file format that matches the version of the given 
    // resource type.
    template <class FileFormat>
    static inline NiFileInterface* CreateFileFormatInstance();

    /// Typedef to store a map of version numbers to file format information structures 
    typedef efd::map<typename FileFormatInterface::FileVersion, FileFormatInfo> FormatMap;
    /// Map of known file version numbers to creation functions
    FormatMap m_formats;
};

#include "NiFileVersionRegistry.inl"

#endif // NIFILEVERSIONREGISTRY_H
