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
#include "NiTerrainFileVersion4.h"
#include "NiTerrainXMLHelpers.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiITerrainFileVersion4, NiTerrainFileInterface);
NiImplementRTTI(NiTerrainFileVersion4, NiITerrainFileVersion4);
//--------------------------------------------------------------------------------------------------
NiITerrainFileVersion4::NiITerrainFileVersion4()
    : NiTerrainFileInterface(ms_InterfaceVersion)
{
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainFileVersion4::ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
    efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
    float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
    float& fLowDetailSpecularIntensity)
{
    EE_UNUSED_ARG(uiSectorSize);
    EE_UNUSED_ARG(uiNumLOD);
    EE_UNUSED_ARG(uiMaskSize);
    EE_UNUSED_ARG(uiLowDetailSize);
    EE_UNUSED_ARG(fMinElevation);
    EE_UNUSED_ARG(fMaxElevation);
    EE_UNUSED_ARG(fVertexSpacing);
    EE_UNUSED_ARG(fLowDetailSpecularPower);
    EE_UNUSED_ARG(fLowDetailSpecularIntensity);

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainFileVersion4::WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
    efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
    float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
    float fLowDetailSpecularIntensity)
{
    EE_UNUSED_ARG(uiSectorSize);
    EE_UNUSED_ARG(uiNumLOD);
    EE_UNUSED_ARG(uiMaskSize);
    EE_UNUSED_ARG(uiLowDetailSize);
    EE_UNUSED_ARG(fMinElevation);
    EE_UNUSED_ARG(fMaxElevation);
    EE_UNUSED_ARG(fVertexSpacing);
    EE_UNUSED_ARG(fLowDetailSpecularPower);
    EE_UNUSED_ARG(fLowDetailSpecularIntensity);
}

//--------------------------------------------------------------------------------------------------
const char* NiTerrainFileVersion4::ms_pcTerrainConfigFile = "\\root.terrain";
//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion4::DetectFileVersion(FileIdentifier kID)
{
    // Attempt to access the file
    efd::utf8string kFilePath = kID.m_kArchivePath + ms_pcTerrainConfigFile;

    // Attempt to open the file:
    efd::TiXmlDocument kFile(kFilePath.c_str());
    if (!NiTerrainXMLHelpers::LoadXMLFile(&kFile))
    {
        return false;
    }
    else
    {
        // Reset to the initial tag
        efd::TiXmlElement* pkCurElement = kFile.FirstChildElement("Terrain");            
        if (!pkCurElement)
            return false;

        // Inspect the file version
        const char* pcVersion = pkCurElement->Attribute("Version");
        if (pcVersion)
        {
            NiUInt32 uiFileVersion = 0;
            if (NiString(pcVersion).ToUInt(uiFileVersion))
                return uiFileVersion == ms_InterfaceVersion;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion4::NiTerrainFileVersion4()
    : m_bConfigurationValid(false)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion4::OpenErrorCode NiTerrainFileVersion4::Open(FileIdentifier kID, 
    efd::File::OpenMode eAccessMode)
{
    if (eAccessMode == efd::File::READ_ONLY && !DetectFileVersion(kID))
        return WRONG_VERSION;

    NiTerrainFileInterface::Open(kID.m_pkStoragePolicy, eAccessMode);
    m_kTerrainArchive = (kID.m_kArchivePath);

    if (Initialize())
        return SUCCESS;
    else
        return FAIL;
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion4::~NiTerrainFileVersion4()
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion4::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    kFilePaths.insert((const char*)GenerateTerrainConfigFilename());
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion4::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize the base file
    bool bResult = NiITerrainFileVersion4::Initialize();
    if (!bResult)
        return false;

    // Open the file
    m_kFile = efd::TiXmlDocument(GenerateTerrainConfigFilename());
    if (!IsWritable() && !NiTerrainXMLHelpers::LoadXMLFile(&m_kFile))
        return false;

    // If we are reading from the file, then begin setting up:
    if (!IsWritable())
    {
        // Reset to the initial tag
        efd::TiXmlElement* pkCurElement = m_kFile.FirstChildElement("Terrain");            
        if (!pkCurElement)
            return NULL;

        // Read the file version
        m_kFileVersion = 0;
        if (!NiString(pkCurElement->Attribute("Version")).ToUInt(m_kFileVersion))
            return false;
        
        // Read the configuration data
        if (!ReadConfiguration(pkCurElement))
            return false;
    }
    else
    {
        // Must at have access to the file to write
        if (!efd::File::Access(GenerateTerrainConfigFilename(), efd::File::WRITE_ONLY))
            return false;
        m_kFileVersion = ms_InterfaceVersion;
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion4::ReadConfiguration(efd::TiXmlElement* pkRootElement)
{
    efd::TiXmlElement* pkConfigElement = pkRootElement->FirstChildElement("Configuration");
    if (!pkConfigElement)
        return false;

    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "SectorSize", m_uiSectorSize);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "NumLOD", m_uiNumLOD);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "MaskSize", m_uiMaskSize);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "LowDetailTextureSize", m_uiLowDetailSize);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "MinElevation", m_fMinElevation);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "MaxElevation", m_fMaxElevation);
    if (!NiTerrainXMLHelpers::ReadElement(pkConfigElement, "VertexSpacing", m_fVertexSpacing))
    {
        // Default value for vertex spacing
        m_fVertexSpacing = 1.0f;
    }

    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "LowDetailSpecularPower", 
        m_fLowDetailSpecularPower);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "LowDetailSpecularIntensity", 
        m_fLowDetailSpecularIntensity);

    m_bConfigurationValid = true;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion4::Close()
{
    bool bResult = true;

    // Write the file
    if (IsWritable())
    {
        bResult &= WriteFileHeader();
        bResult &= WriteConfiguration();

        bResult &= m_kFile.SaveFile();
    }

    if (!bResult)
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    NiITerrainFileVersion4::Close();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion4::WriteFileHeader()
{
    if (!IsReady() || !IsWritable())
        return false;

    NiTerrainXMLHelpers::WriteXMLHeader(&m_kFile);
    efd::TiXmlElement* pkTerrainElement = NiTerrainXMLHelpers::CreateElement("Terrain", NULL);
    pkTerrainElement->SetAttribute("Version", m_kFileVersion);
    m_kFile.LinkEndChild(pkTerrainElement);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion4::WriteConfiguration()
{
    if (!IsReady() || !IsWritable() || !m_bConfigurationValid)
        return false;

    efd::TiXmlElement* pkRootElement = m_kFile.FirstChildElement("Terrain");
    efd::TiXmlElement* pkConfigElement = NiTerrainXMLHelpers::CreateElement(
        "Configuration", pkRootElement);

    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "SectorSize", m_uiSectorSize);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "NumLOD", m_uiNumLOD);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "MaskSize", m_uiMaskSize);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "LowDetailTextureSize", m_uiLowDetailSize);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "MinElevation", m_fMinElevation);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "MaxElevation", m_fMaxElevation);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "VertexSpacing", m_fVertexSpacing);

    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "LowDetailSpecularPower", 
        m_fLowDetailSpecularPower);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "LowDetailSpecularIntensity", 
        m_fLowDetailSpecularIntensity);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion4::ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
    efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
    float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
    float& fLowDetailSpecularIntensity)
{
    if (!m_bConfigurationValid)
        return false;

    uiSectorSize = m_uiSectorSize;
    uiNumLOD = m_uiNumLOD;
    uiMaskSize = m_uiMaskSize;
    uiLowDetailSize = m_uiLowDetailSize;
    fMinElevation = m_fMinElevation;
    fMaxElevation = m_fMaxElevation;
    fVertexSpacing = m_fVertexSpacing;
    fLowDetailSpecularPower = m_fLowDetailSpecularPower;
    fLowDetailSpecularIntensity = m_fLowDetailSpecularIntensity;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion4::WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
    efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
    float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
    float fLowDetailSpecularIntensity)
{
    m_bConfigurationValid = true;

    m_uiSectorSize = uiSectorSize;
    m_uiNumLOD = uiNumLOD;
    m_uiMaskSize = uiMaskSize;
    m_uiLowDetailSize = uiLowDetailSize;
    m_fMinElevation = fMinElevation;
    m_fMaxElevation = fMaxElevation;
    m_fVertexSpacing = fVertexSpacing;
    m_fLowDetailSpecularPower = fLowDetailSpecularPower;
    m_fLowDetailSpecularIntensity = fLowDetailSpecularIntensity;
}

//--------------------------------------------------------------------------------------------------
