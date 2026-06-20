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
#include "NiTerrainFileVersion2.h"
#include "NiTerrainXMLHelpers.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainFileVersion2, NiTerrainFileVersion3, NiTypeMask::NiTerrainFileVersion2);
//--------------------------------------------------------------------------------------------------
const char* NiTerrainFileVersion2::ms_pcTerrainConfigFile = "\\root.terrain";
//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion2::DetectFileVersion(FileIdentifier kID)
{
    FileVersion eVersion = 0;

    // Attempt to access the file
    NiString kSectorPath;
    kSectorPath.Format("%s%s", kID.m_kArchivePath.c_str(), ms_pcTerrainConfigFile);

    // Attempt to open the file:
    efd::TiXmlDocument kFile(kSectorPath);
    if (NiTerrainXMLHelpers::LoadXMLFile(&kFile))
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
                eVersion = uiFileVersion;
        }
    }

    return eVersion == 2;
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion2::NiTerrainFileVersion2()
    : m_bConfigurationValid(false)
{
    m_kFileVersion = ms_InterfaceVersion;
    m_kInterfaceVersion = ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion2::~NiTerrainFileVersion2()
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion2::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    kFilePaths.insert((const char*)GenerateTerrainConfigFilename());
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion2::OpenErrorCode NiTerrainFileVersion2::Open(FileIdentifier kID, 
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
bool NiTerrainFileVersion2::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize the base file
    bool bResult = NiITerrainFileVersion3::Initialize();
    if (!bResult)
        return false;

    // Open the file
    m_kFile = efd::TiXmlDocument(GenerateTerrainConfigFilename());
    if (!IsWritable() && !NiTerrainXMLHelpers::LoadXMLFile(&m_kFile))
        return false;

    // Initialize the variables:
    m_kSurfaceNameArray.RemoveAll();
    m_kSurfacePackageArray.RemoveAll();

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
        
        // Read the surface index
        if (!ReadSurfaceIndex(pkCurElement))
            return false;

        // Read the configuration data
        if (!ReadConfiguration(pkCurElement))
            return false;
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion2::ReadConfiguration(efd::TiXmlElement* pkRootElement)
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
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "NumSurfaces", m_uiSurfaceCount);

    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "LowDetailSpecularPower", 
        m_fLowDetailSpecularPower);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "LowDetailSpecularIntensity", 
        m_fLowDetailSpecularIntensity);

    m_bConfigurationValid = true;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion2::ReadSurfaceIndex(efd::TiXmlElement* pkDocument)
{
    const efd::TiXmlElement* pkCurElement = pkDocument->FirstChildElement("SurfaceList");
    pkCurElement = pkCurElement->FirstChildElement("Surface");
    if(pkCurElement)
    {
        const char* pcPackage = 0;
        const char* pcName = 0;
        const char* pcLayerNum = 0;

        // Loop through all surfaces in the list and collect the information
        while (pkCurElement)
        {
            pcPackage = pkCurElement->Attribute("package");
            if (pcPackage == 0)
                pcPackage = "";

            pcName = pkCurElement->Attribute("name");
            if (pcName == 0)
                pcName = "";

            pcLayerNum = pkCurElement->Attribute("position");
            if (pcLayerNum == 0)
                pcLayerNum = "";

            // Convert the layer number into an index
            NiUInt32 uiSurfaceIndex = 0;
            if (NiString(pcLayerNum).ToUInt(uiSurfaceIndex))
            {
                // Add this surface to the list
                m_kSurfacePackageArray.SetAtGrow(uiSurfaceIndex, pcPackage);
                m_kSurfaceNameArray.SetAtGrow(uiSurfaceIndex, pcName);
            }

            // Move onto the next surface
            pkCurElement = pkCurElement->NextSiblingElement();
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion2::WriteFileHeader()
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
bool NiTerrainFileVersion2::WriteConfiguration()
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
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "NumSurfaces", m_uiSurfaceCount);

    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "LowDetailSpecularPower", 
        m_fLowDetailSpecularPower);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "LowDetailSpecularIntensity", 
        m_fLowDetailSpecularIntensity);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion2::WriteSurfaceIndex()
{
    if (!IsReady() || !IsWritable())
        return false;

    efd::TiXmlElement* pkRootElement = m_kFile.FirstChildElement("Terrain");
    efd::TiXmlElement* pkSurfaceListElement = NiTerrainXMLHelpers::CreateElement(
        "SurfaceList", pkRootElement);

    // Write each surface into the file
    NiUInt32 uiNumSurfaces = m_kSurfacePackageArray.GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiNumSurfaces; ++uiIndex)
    {   
        // Check that this entry exists:
        if (!m_kSurfaceNameArray[uiIndex].Exists())
            continue;

        efd::TiXmlElement* pkSurfaceElement = NiTerrainXMLHelpers::CreateElement(
            "Surface", pkSurfaceListElement);
        
        // Write this surface's data to the file
        pkSurfaceElement->SetAttribute("package", m_kSurfacePackageArray[uiIndex]);
        pkSurfaceElement->SetAttribute("name", m_kSurfaceNameArray[uiIndex]);
        pkSurfaceElement->SetAttribute("position", uiIndex);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion2::Close()
{
    bool bResult = true;

    if (IsWritable())
    {
        bResult &= WriteFileHeader();
        bResult &= WriteConfiguration();
        bResult &= WriteSurfaceIndex();
        bResult &= m_kFile.SaveFile();
    }

    if (!bResult)
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    NiITerrainFileVersion3::Close();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion2::ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
    efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
    float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
    float& fLowDetailSpecularIntensity, efd::UInt32& uiSurfaceCount)
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
    uiSurfaceCount = m_uiSurfaceCount;
    fLowDetailSpecularPower = m_fLowDetailSpecularPower;
    fLowDetailSpecularIntensity = m_fLowDetailSpecularIntensity;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion2::WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
    efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
    float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
    float fLowDetailSpecularIntensity, efd::UInt32 uiSurfaceCount)
{
    m_bConfigurationValid = true;

    m_uiSectorSize = uiSectorSize;
    m_uiNumLOD = uiNumLOD;
    m_uiMaskSize = uiMaskSize;
    m_uiLowDetailSize = uiLowDetailSize;
    m_fMinElevation = fMinElevation;
    m_fMaxElevation = fMaxElevation;
    m_fVertexSpacing = fVertexSpacing;
    m_uiSurfaceCount = uiSurfaceCount;
    m_fLowDetailSpecularPower = fLowDetailSpecularPower;
    m_fLowDetailSpecularIntensity = fLowDetailSpecularIntensity;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion2::ReadSurface(NiUInt32 uiSurfaceIndex, 
    NiTerrainAssetReference* pkPackageRef, NiFixedString& kSurfaceID, efd::UInt32& uiIteration)
{
    NiFixedString kPackageID;
    bool bResult = ReadSurface(uiSurfaceIndex, kPackageID, kSurfaceID);

    if (bResult)
    {
        pkPackageRef->SetAssetID(NiTerrainAssetReference::URN_FORCE_LOOKUP + 
            efd::utf8string((const char*)kPackageID));
        pkPackageRef->SetRelativeAssetLocation((const char*)kPackageID);
        pkPackageRef->SetReferringAssetLocation((const char*)GenerateTerrainConfigFilename());

        uiIteration = 0;
    }

    return bResult;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion2::ReadSurface(NiUInt32 uiSurfaceIndex, NiFixedString& kPackageID, 
    NiFixedString& kSurfaceID)
{   
    if (uiSurfaceIndex >= m_kSurfacePackageArray.GetSize())
        return false;

    if (!m_kSurfaceNameArray[uiSurfaceIndex].Exists())
        return false;

    kPackageID = m_kSurfacePackageArray[uiSurfaceIndex];
    kSurfaceID = m_kSurfaceNameArray[uiSurfaceIndex];
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion2::WriteSurface(NiUInt32 uiSurfaceIndex, NiFixedString kPackageID, 
    NiFixedString kSurfaceID)
{
    m_kSurfacePackageArray.SetAtGrow(uiSurfaceIndex, kPackageID);
    m_kSurfaceNameArray.SetAtGrow(uiSurfaceIndex, kSurfaceID);
}

//--------------------------------------------------------------------------------------------------
