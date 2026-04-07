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

#ifndef NITERRAINFILEVERSION0_H
#define NITERRAINFILEVERSION0_H

#include <NiBoxBV.h>

#include "NiTerrainLibType.h"
#include "NiTerrainFileVersion1.h"

/** 
    The class is used to iterate over and write terrain files.
    This version is used to read data from older versions of terrain file format, where the 
    surface information was previously stored in sector 0,0.
 */
class NITERRAIN_ENTRY NiTerrainFileVersion0 : public NiTerrainFileVersion1
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:
    /// The version of the reader interface that this class presents.
    static const FileVersion ms_InterfaceVersion = FileVersion(0);

    /// Constructor
    NiTerrainFileVersion0();
    
    /// Destructor.
    virtual ~NiTerrainFileVersion0();

    /**
        Open function to open the file at the given identifier for access using the given 
        access mode. 

        @param kID The file identifier of the file the caller wishes to open
        @param eAccessMode The type of access to the file required (read/write)
        @return WRONG_VERSION if the file is not the correct version. SUCCESS if the file
            can be opened, FAIL if the file cannot be read/written.
    */
    virtual OpenErrorCode Open(FileIdentifier kID, efd::File::OpenMode eAccessMode);

protected:

    /**
        Return true if the file identified by the identifier is the correct version for reading
        by this file reader.
    */
    virtual bool DetectFileVersion(FileIdentifier kID);

    /**
        Initialize the class by opening the file and reading the first couple
        of headers. 

        @return true if the class was successfully initialized.
    */
    virtual bool Initialize();

    /**
        Read the list of surfaces from a section of a DOM document.

        @param pkDocument Pointer to the XML document to read
        @return true when read is successful
    */
    bool ReadSurfaceIndex(const efd::TiXmlElement* pkDocument);
};

#endif // NITERRAINFILEVERSION0_H
