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

//--------------------------------------------------------------------------------------------------
template <class FileFormatInterface>
inline NiFileVersionRegistry<FileFormatInterface>::NiFileVersionRegistry()
{
}

//--------------------------------------------------------------------------------------------------
template <class FileFormatInterface>
inline NiFileVersionRegistry<FileFormatInterface>::~NiFileVersionRegistry()
{
}

//--------------------------------------------------------------------------------------------------
template <class FileFormatInterface>
inline FileFormatInterface* NiFileVersionRegistry<FileFormatInterface>::OpenFile(
    const typename FileFormatInterface::FileIdentifier& kFileIdentifier,
    efd::File::OpenMode eAccessMode)
{
    return Open<FileFormatInterface>(kFileIdentifier, eAccessMode);
}

//--------------------------------------------------------------------------------------------------
template <class FileFormatInterface>
inline FileFormatInterface* NiFileVersionRegistry<FileFormatInterface>::OpenFile(
    const typename FileFormatInterface::FileIdentifier& kFileIdentifier,
    bool bWriteAccess)
{
    efd::File::OpenMode eAccessMode = 
        (bWriteAccess) ? (efd::File::WRITE_ONLY) : (efd::File::READ_ONLY);
    return Open<FileFormatInterface>(kFileIdentifier, eAccessMode);
}

//--------------------------------------------------------------------------------------------------
template <class FileFormatInterface>
template <class InterfaceFormat>
inline InterfaceFormat* NiFileVersionRegistry<FileFormatInterface>::OpenLegacyFile(
    const typename InterfaceFormat::FileIdentifier& kFileIdentifier)
{
    return Open<InterfaceFormat>(kFileIdentifier, efd::File::READ_ONLY);
}

//--------------------------------------------------------------------------------------------------
template <class FileFormatInterface>
template <class Interface>
inline Interface* NiFileVersionRegistry<FileFormatInterface>::Open(
    const typename Interface::FileIdentifier& kFileIdentifier,
    efd::File::OpenMode eAccessMode)
{
    // Work out the target interface version
    typename FileFormatInterface::FileVersion kTargetVersion = Interface::ms_InterfaceVersion;
    
    // Look up the target version and begin searching for the file format from there
    typename FormatMap::iterator matchingFormat = m_formats.find(kTargetVersion);
    if (matchingFormat == m_formats.end())
        return NULL;

    NiFileInterface::OpenErrorCode ec = NiFileInterface::WRONG_VERSION;
    Interface* pkInterface = NULL;
    while (ec == NiFileInterface::WRONG_VERSION)
    {
        // Create the file parser
        NiFileInterface* pkFile = matchingFormat->second.m_createFunction();
        EE_ASSERT(pkFile->GetInterfaceVersion() == matchingFormat->first);
        
        // Adapt it up to the version being used
        while (pkFile && pkFile->GetInterfaceVersion() != kTargetVersion)
        {
            NiFileInterface* pkNextVersion = pkFile->AdaptToNextVersion();
            if (!pkNextVersion)
            {
                EE_DELETE pkFile;
                pkFile = NULL;
            }
            else
            {
                pkFile = pkNextVersion;
            }
        }

        pkInterface = NiDynamicCast(Interface, pkFile);
        if (pkInterface)
        {  
            // Attempt to initialize it  
            ec = pkInterface->Open(kFileIdentifier, eAccessMode);

            // Check if this parser is the wrong version for the file
            if (ec != NiFileInterface::WRONG_VERSION)
                break;

            // This isn't the right parser, so forget it
            pkInterface = NULL;
        }

        // Parser is the wrong version so destroy it and try another
        EE_DELETE pkFile;
        
        // If there are no more formats to try, then break
        if (matchingFormat == m_formats.begin())
            break;
        else
            --matchingFormat;
    }

    if (pkInterface && ec != NiFileInterface::SUCCESS)
    {
        // Failed to open the file properly
        pkInterface->Close();
        EE_DELETE pkInterface;
        pkInterface = NULL;
    }

    return pkInterface;
}

//--------------------------------------------------------------------------------------------------
template <class FileFormatInterface>
template <class FileFormat>
inline NiFileInterface* NiFileVersionRegistry<FileFormatInterface>::CreateFileFormatInstance()
{
    return EE_NEW FileFormat();
}

//--------------------------------------------------------------------------------------------------
template <class FileFormatInterface>
template <class FileFormat>
inline void NiFileVersionRegistry<FileFormatInterface>::AddFileFormat(
    const typename FileFormatInterface::FileVersion& kVersion)
{
    EE_ASSERT(m_formats.find(kVersion) == m_formats.end());
    FileFormatInfo& info = m_formats[kVersion] ;

    // Factory function which is able to instantiate the file format in question
    info.m_createFunction = &CreateFileFormatInstance<FileFormat>;
}

//--------------------------------------------------------------------------------------------------
