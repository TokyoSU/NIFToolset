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
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiTerrainPCH.h"
#include "NiTerrainSurfacePackageFileVersion0.h"
#include "NiTerrainXMLHelpers.h"
#include <efd/ecrLogIDs.h>

static const char* XML_ELEMENT_PACKAGE = "Package";
static const char* XML_ELEMENT_SURFACE = "Surface";
static const char* XML_ELEMENT_METADATA = "MetaData";
static const char* XML_ATTRIBUTE_PACKAGENAME = "name";
static const char* XML_ATTRIBUTE_SURFACENAME = "name";
static const char* XML_ATTRIBUTE_UVSCALE = "UVScaleModifier";
static const char* XML_ATTRIBUTE_DIFFUSEMAP = "DiffuseMap";
static const char* XML_ATTRIBUTE_NORMALMAP = "NormalMap";
static const char* XML_ATTRIBUTE_PACKAGEVERSION = "version";

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiITerrainSurfacePackageFileVersion0, NiITerrainSurfacePackageFileVersion1, NiTypeMask::NiITerrainSurfacePackageFileVersion0);
NiImplementRTTI(NiTerrainSurfacePackageFileVersion0, NiITerrainSurfacePackageFileVersion0, NiTypeMask::NiTerrainSurfacePackageFileVersion0);
//--------------------------------------------------------------------------------------------------
NiITerrainSurfacePackageFileVersion0::NiITerrainSurfacePackageFileVersion0()
{
    m_kFileVersion = ms_InterfaceVersion;
    m_kInterfaceVersion = ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
NiFileInterface* NiITerrainSurfacePackageFileVersion0::AdaptToNextVersion()
{
    return EE_NEW NiITerrainSurfacePackageFileVersion0to1Adapter(this);
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFileVersion0::NiTerrainSurfacePackageFileVersion0()
    : m_bConfigurationValid(false)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFileVersion0::~NiTerrainSurfacePackageFileVersion0()
{
    // Release the collection of surface information
    efd::vector<SurfaceData*>::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        NiDelete(*kIter);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion0::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    kFilePaths.insert(m_kPackageFilename);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::DetectFileVersion(FileIdentifier kID)
{
    FileVersion eVersion = 0;

    // Attempt to open the file:
    efd::TiXmlDocument kFile(kID.m_kPackageFile.c_str());
    if (NiTerrainXMLHelpers::LoadXMLFile(&kFile))
    {
        // Reset to the initial tag
        efd::TiXmlElement* pkCurElement = kFile.FirstChildElement(XML_ELEMENT_PACKAGE);            
        if (!pkCurElement)
            return NULL;

        // Inspect the file version
        const char* pcVersion = pkCurElement->Attribute(XML_ATTRIBUTE_PACKAGEVERSION);
        if (pcVersion)
        {
            NiUInt32 uiFileVersion = 0;
            if (NiString(pcVersion).ToUInt(uiFileVersion))
                eVersion = uiFileVersion;
        }
    }

    return eVersion == 0;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFileVersion0::OpenErrorCode NiTerrainSurfacePackageFileVersion0::Open(
    FileIdentifier kID, efd::File::OpenMode eAccessMode)
{
    if (eAccessMode == efd::File::READ_ONLY && !DetectFileVersion(kID))
        return WRONG_VERSION;

    NiTerrainFileInterface::Open(kID.m_pkStoragePolicy, eAccessMode);
    m_kPackageFilename = (kID.m_kPackageFile);

    if (Initialize())
        return SUCCESS;
    else
        return FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize the base file
    bool bResult = NiITerrainSurfacePackageFileVersion0::Initialize();
    if (!bResult)
        return false;

    // Open the file
    m_kFile = efd::TiXmlDocument(m_kPackageFilename.c_str());
    if (!IsWritable() && !NiTerrainXMLHelpers::LoadXMLFile(&m_kFile))
        return false;

    // Initialize the variables:
    m_kSurfaces.clear();

    // If we are reading from the file, then begin setting up:
    if (!IsWritable())
    {
        // Reset to the initial tag
        efd::TiXmlElement* pkCurElement = m_kFile.FirstChildElement(XML_ELEMENT_PACKAGE);            
        if (!pkCurElement)
            return NULL;

        // Read the file version
        m_kFileVersion = 0;

        // Read the configuration data
        if (!ReadOldPackage(pkCurElement))
            return false;
    }
    else
    {
        /// This version does not support saving any more
        return false;
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion0::Close()
{
    NiITerrainSurfacePackageFileVersion0::Close();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::ReadOldPackage(const efd::TiXmlElement* pkRootElement)
{
    const char* pcPackageName = 0;

    // Extract the package name
    pcPackageName = pkRootElement->Attribute(XML_ATTRIBUTE_PACKAGENAME);
    if (pcPackageName == 0)
        pcPackageName = "";
    m_kPackageName = pcPackageName;
    m_bConfigurationValid = true;

    // Extract the iteration count
    m_uiIteration = 0;

    // Extract all the surfaces from the file
    const efd::TiXmlElement* pkCurElement = pkRootElement->FirstChildElement(XML_ELEMENT_SURFACE);
    if (pkCurElement)
    {
        // Loop through all surfaces in the list and collect the information
        while (pkCurElement)
        {
            SurfaceData kTempSurfaceData;
            if (ReadOldSurface(pkCurElement, kTempSurfaceData))
            {
                // Add the surface to the list
                SurfaceData* pkSurfaceData = NiNew SurfaceData();
                *pkSurfaceData = kTempSurfaceData;
                m_kSurfaces.push_back(pkSurfaceData);
            }

            // Move onto the next surface
            pkCurElement = pkCurElement->NextSiblingElement();
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::ReadOldSurface(const efd::TiXmlElement* pkSurfaceElement,
    SurfaceData& kTempSurfaceData)
{
    // Read the name of the surface:
    const char* pcName = pkSurfaceElement->Attribute(XML_ATTRIBUTE_SURFACENAME);
    if (!pcName)
        return false;
    kTempSurfaceData.m_kName = pcName;

    // Read the surface attributes:
    NiPoint2 kScaleModifier;
    NiTerrainXMLHelpers::ReadElement(pkSurfaceElement, XML_ATTRIBUTE_UVSCALE, kScaleModifier);
    kTempSurfaceData.m_fTextureTiling = kScaleModifier.x;
    
    // Read in the different textures
    const char* pucDiffuseMap = NULL;
    NiTerrainXMLHelpers::ReadElement(pkSurfaceElement, XML_ATTRIBUTE_DIFFUSEMAP, pucDiffuseMap);
    if (!pucDiffuseMap)
        pucDiffuseMap = "";
    kTempSurfaceData.m_akTextureSlots[NiSurface::SURFACE_MAP_DIFFUSE].m_kLastRelativePath = 
        pucDiffuseMap;

    const char* pucNormalMap = NULL;
    NiTerrainXMLHelpers::ReadElement(pkSurfaceElement, XML_ATTRIBUTE_NORMALMAP, pucNormalMap);
    if (!pucNormalMap)
        pucNormalMap = "";
    kTempSurfaceData.m_akTextureSlots[NiSurface::SURFACE_MAP_NORMAL].m_kLastRelativePath = 
        pucNormalMap;

    // Load in all the meta data for the surface
    const efd::TiXmlElement* pkMetaDataElement = 
        pkSurfaceElement->FirstChildElement(XML_ELEMENT_METADATA);
    if (pkMetaDataElement)
        kTempSurfaceData.m_kMetaData.Load(pkMetaDataElement);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::ReadPackageConfig(efd::utf8string& kPackageName,
    efd::UInt32& uiIteration)
{
    if (!m_bConfigurationValid)
        return false;

    kPackageName = m_kPackageName;
    uiIteration = m_uiIteration;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::ReadNumSurfaces(efd::UInt32& uiNumSurfaces)
{
    if (!m_bConfigurationValid)
        return false;
    
    uiNumSurfaces = m_kSurfaces.size();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::ReadSurfaceConfig(efd::UInt32 uiSurfaceIndex,
    efd::utf8string& kName, 
    efd::Float32& fTextureTiling,
    efd::Float32& fDetailTiling,
    efd::Float32& fRotation,
    efd::Float32& fParallaxStrength,
    efd::Float32& fDistributionMaskStrength,
    efd::Float32& fSpecularPower,
    efd::Float32& fSpecularIntensity,
    efd::UInt32& uiNumDecorationLayers)
{
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return false;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    kName = 
        pkSurfaceData->m_kName;
    fTextureTiling = 
        pkSurfaceData->m_fTextureTiling;
    fDetailTiling = 
        pkSurfaceData->m_fDetailTiling;
    fRotation = 
        pkSurfaceData->m_fRotation;
    fParallaxStrength = 
        pkSurfaceData->m_fParallaxStrength;
    fDistributionMaskStrength = 
        pkSurfaceData->m_fDistributionMaskStrength;
    fSpecularPower = 
        pkSurfaceData->m_fSpecularPower;
    fSpecularIntensity = 
        pkSurfaceData->m_fSpecularIntensity;
    uiNumDecorationLayers = 
        pkSurfaceData->m_uiNumDecorationLayers;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::ReadSurfaceSlot(efd::UInt32 uiSurfaceIndex,
    efd::UInt32 uiSlotID,
    NiTerrainAssetReference* pkReference)
{
    EE_ASSERT(pkReference);
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return false;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    if (uiSlotID >= NiSurface::NUM_SURFACE_MAPS)
        return false;

    efd::utf8string lastRelativePath = 
        pkSurfaceData->m_akTextureSlots[uiSlotID].m_kLastRelativePath;
    efd::utf8string assetID;
    if (!lastRelativePath.empty())
    {
        assetID = NiTerrainAssetReference::URN_FORCE_LOOKUP + lastRelativePath;
    }

    pkReference->SetReferringAssetLocation(m_kPackageFilename);
    pkReference->SetAssetID(assetID);
    pkReference->SetRelativeAssetLocation(lastRelativePath);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion0::ReadSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
    NiMetaData& kMetaData)
{
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return false;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    kMetaData = pkSurfaceData->m_kMetaData;

    return true;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFileVersion0::SurfaceData::SurfaceData()
    : m_kName()
    , m_fTextureTiling(1.0f)
    , m_fDetailTiling(1.0f)
    , m_fRotation(0.0f)
    , m_fParallaxStrength(0.0f)
    , m_fDistributionMaskStrength(0.0f)
    , m_fSpecularPower(0.0f)
    , m_fSpecularIntensity(0.0f)
    , m_uiNumDecorationLayers(0)
{
}

//--------------------------------------------------------------------------------------------------
NiITerrainSurfacePackageFileVersion0to1Adapter::NiITerrainSurfacePackageFileVersion0to1Adapter(
    NiITerrainSurfacePackageFileVersion0* pBaseVersion)
    : NiITerrainSurfacePackageFileVersion1()
    , m_spPrevVersion(pBaseVersion)
{
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::OpenErrorCode NiITerrainSurfacePackageFileVersion0to1Adapter::Open(
    FileIdentifier kID, efd::File::OpenMode eAccessMode)
{
    return m_spPrevVersion->Open(kID, eAccessMode);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion0to1Adapter::Close()
{
    return m_spPrevVersion->Close();
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::FileVersion NiITerrainSurfacePackageFileVersion0to1Adapter::GetFileVersion() const
{
    return m_spPrevVersion->GetFileVersion();
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::FileVersion NiITerrainSurfacePackageFileVersion0to1Adapter::GetInterfaceVersion() 
    const
{
    return ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion0to1Adapter::IsReady() const
{
    return m_spPrevVersion->IsReady();
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion0to1Adapter::IsWritable() const
{
    return m_spPrevVersion->IsWritable();
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion0to1Adapter::GetFilePaths(
    efd::set<efd::utf8string>& kFilePaths)
{
    return m_spPrevVersion->GetFilePaths(kFilePaths);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion0to1Adapter::Precache(efd::UInt32 uiDataFields)
{
    return m_spPrevVersion->Precache(uiDataFields);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion0to1Adapter::ReadPackageConfig(
    efd::utf8string& kPackageName, efd::UInt32& uiIteration)
{
    return m_spPrevVersion->ReadPackageConfig(kPackageName, uiIteration);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion0to1Adapter::WritePackageConfig(
    const efd::utf8string& kPackageName, efd::UInt32 uiIteration)
{
    return m_spPrevVersion->WritePackageConfig(kPackageName, uiIteration);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion0to1Adapter::ReadNumSurfaces(efd::UInt32& uiNumSurfaces)
{
    return m_spPrevVersion->ReadNumSurfaces(uiNumSurfaces);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion0to1Adapter::WriteNumSurfaces(
    const efd::UInt32& uiNumSurfaces)
{
    return m_spPrevVersion->WriteNumSurfaces(uiNumSurfaces);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion0to1Adapter::ReadSurfaceConfig(efd::UInt32 uiSurfaceIndex,
    efd::utf8string& kName, 
    efd::Float32& fTextureTiling,
    efd::Float32& fDetailTiling,
    efd::Float32& fRotation,
    efd::Float32& fParallaxStrength,
    efd::Float32& fDistributionMaskStrength,
    efd::Float32& fSpecularPower,
    efd::Float32& fSpecularIntensity,
    efd::UInt32& uiNumDecorationLayers)
{
    return m_spPrevVersion->ReadSurfaceConfig(uiSurfaceIndex, 
        kName, 
        fTextureTiling,
        fDetailTiling,
        fRotation,
        fParallaxStrength,
        fDistributionMaskStrength,
        fSpecularPower,
        fSpecularIntensity,
        uiNumDecorationLayers);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion0to1Adapter::WriteSurfaceConfig(efd::UInt32 uiSurfaceIndex,
    const efd::utf8string& kName, 
    efd::Float32 fTextureTiling,
    efd::Float32 fDetailTiling,
    efd::Float32 fRotation,
    efd::Float32 fParallaxStrength,
    efd::Float32 fDistributionMaskStrength,
    efd::Float32 fSpecularPower,
    efd::Float32 fSpecularIntensity,
    efd::UInt32 uiNumDecorationLayers)
{
    return m_spPrevVersion->WriteSurfaceConfig(uiSurfaceIndex, 
        kName, 
        fTextureTiling,
        fDetailTiling,
        fRotation,
        fParallaxStrength,
        fDistributionMaskStrength,
        fSpecularPower,
        fSpecularIntensity,
        uiNumDecorationLayers);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion0to1Adapter::ReadSurfaceSlot(efd::UInt32 uiSurfaceIndex,
    efd::UInt32 uiSlotID, NiTerrainAssetReference* pkReference)
{
    return m_spPrevVersion->ReadSurfaceSlot(uiSurfaceIndex, 
        uiSlotID, 
        pkReference);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion0to1Adapter::WriteSurfaceSlot(efd::UInt32 uiSurfaceIndex,
    efd::UInt32 uiSlotID, const NiTerrainAssetReference* pkReference)
{
    return m_spPrevVersion->WriteSurfaceSlot(uiSurfaceIndex, 
        uiSlotID, 
        pkReference);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion0to1Adapter::ReadSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
    NiMetaData& kMetaData)
{
    return m_spPrevVersion->ReadSurfaceMetadata(uiSurfaceIndex, 
        kMetaData);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion0to1Adapter::WriteSurfaceMetadata(
    efd::UInt32 uiSurfaceIndex, const NiMetaData& kMetaData)
{
    return m_spPrevVersion->WriteSurfaceMetadata(uiSurfaceIndex, 
        kMetaData);
}

//--------------------------------------------------------------------------------------------------