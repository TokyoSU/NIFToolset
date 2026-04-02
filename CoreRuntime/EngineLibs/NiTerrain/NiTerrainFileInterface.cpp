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

#include "NiTerrainPCH.h"
#include "NiTerrainFileInterface.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainFileInterface, NiFileInterface);
//--------------------------------------------------------------------------------------------------
NiTerrainFileInterface::NiTerrainFileInterface(FileVersion kVersion)
    : m_pkStoragePolicy(NULL)
    , m_eAccessMode(efd::File::READ_ONLY)
    , m_kFileVersion(kVersion)
    , m_kInterfaceVersion(kVersion)
    , m_bOpen(false)
    , m_eSuccess(NiTerrainStoragePolicy::IOSuccessCode::UNKNOWN)
    , m_bIsImplementation(false)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileInterface::~NiTerrainFileInterface()
{
    EE_ASSERT(m_bOpen == false);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileInterface::Open(NiTerrainStoragePolicy* pkStoragePolicy, 
      efd::File::OpenMode eAccessMode)
{
    m_pkStoragePolicy = (pkStoragePolicy);
    m_eAccessMode = (eAccessMode);
    m_bOpen = (false);
    m_eSuccess = (NiTerrainStoragePolicy::IOSuccessCode::UNKNOWN);

    // Since adapters never have their 'open' function called, this must be an implementation.
    m_bIsImplementation = (true);

    EE_ASSERT(pkStoragePolicy);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileInterface::Initialize()
{
    EE_ASSERT(m_bIsImplementation);
    EE_ASSERT(m_pkStoragePolicy);

    // Send off callbacks to storage policy about the file opening
    efd::set<efd::utf8string> kFilePaths;
    GetFilePaths(kFilePaths);
    efd::set<efd::utf8string>::iterator kIter;
    for (kIter = kFilePaths.begin(); kIter != kFilePaths.end(); ++kIter)
    {
        m_pkStoragePolicy->RaiseOpening(
            NiTerrainStoragePolicy::OpeningEventArgs(*kIter, IsWritable()));
    }

    // Mark the file as open
    m_bOpen = true;
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileInterface::Close()
{
    EE_ASSERT(m_bIsImplementation);
    EE_ASSERT(m_pkStoragePolicy);

    // Mark the file as closed
    m_bOpen = false;

    // Send off callbacks to storage policy about the file closing
    efd::set<efd::utf8string> kFilePaths;
    GetFilePaths(kFilePaths);
    efd::set<efd::utf8string>::iterator kIter;
    for (kIter = kFilePaths.begin(); kIter != kFilePaths.end(); ++kIter)
    {
        m_pkStoragePolicy->RaiseClosed(
            NiTerrainStoragePolicy::ClosedEventArgs(*kIter, m_eSuccess));
    }
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileInterface::FileVersion NiTerrainFileInterface::GetInterfaceVersion() const
{
    return m_kInterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
