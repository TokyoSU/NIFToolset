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

#include "NiTerrainPCH.h"
#include <efd/File.h>

#include "NiTerrainSectorFileDeveloper.h"
#include "NiTerrainXMLHelpers.h"
#include "NiTerrainUtils.h"

//--------------------------------------------------------------------------------------------------
const char* NiTerrainSectorFileDeveloper::ms_pcFolder = "\\ToolData\\";
const char* NiTerrainSectorFileDeveloper::ms_pcHeightsFile = "Heights.uint16.raw";
const char* NiTerrainSectorFileDeveloper::ms_pcNormalsFile = "Normals.float2.raw";
const char* NiTerrainSectorFileDeveloper::ms_pcTangentsFile = "Tangents.float2.raw";
const char* NiTerrainSectorFileDeveloper::ms_pcLowDetailNormalMapFile = "LD_NormalMap.rgb24.tga";
const char* NiTerrainSectorFileDeveloper::ms_pcLowDetailDiffuseMapFile = "LD_DiffuseMap.rgba32.tga";
const char* NiTerrainSectorFileDeveloper::ms_pcSectorDataFile = "Config.xml";
const char* NiTerrainSectorFileDeveloper::ms_pcBoundsFile = "Bounds.xml";
const char* NiTerrainSectorFileDeveloper::ms_pcSurfaceReferenceFile = "SurfaceReferences.xml";
const char* NiTerrainSectorFileDeveloper::ms_pcSurfaceIndexFile = "SurfaceIndex.xml";
const char* NiTerrainSectorFileDeveloper::ms_pcBlendMaskFile = "BlendMasks.rgba32.tga";

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileDeveloper::NiTerrainSectorFileDeveloper()
    : NiITerrainSectorFileVersion7()
    , m_iSectorX(0)
    , m_iSectorY(0)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileDeveloper::~NiTerrainSectorFileDeveloper()
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFile::FileVersion NiTerrainSectorFileDeveloper::GetFileVersion() const
{
    return ms_kFileVersion;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFile::FileVersion NiTerrainSectorFileDeveloper::GetCurrentVersion()
{
    return ms_kFileVersion;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    kFilePaths.insert((const char*)GenerateHeightsFilename());
    kFilePaths.insert((const char*)GenerateNormalsFilename());
    kFilePaths.insert((const char*)GenerateTangentsFilename());
    kFilePaths.insert((const char*)GenerateLowDetailDiffuseMapFilename());
    kFilePaths.insert((const char*)GenerateLowDetailNormalMapFilename());
    kFilePaths.insert((const char*)GenerateSectorDataFilename());
    kFilePaths.insert((const char*)GenerateBoundsFilename());
    kFilePaths.insert((const char*)GenerateSurfaceReferenceFilename());
    kFilePaths.insert((const char*)GenerateSurfaceIndexFilename());
    kFilePaths.insert((const char*)GenerateBlendMaskFilename());
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::DetectFileVersion(FileIdentifier kID)
{
    FileVersion eVersion = 0;

    // Attempt to access the file
    m_kTerrainArchive = (kID.m_kArchivePath);
    m_iSectorX = (kID.m_iSectorX);
    m_iSectorY = (kID.m_iSectorY);
    efd::TiXmlDocument kFile(GenerateSectorDataFilename());
    if (!NiTerrainXMLHelpers::LoadXMLFile(&kFile))
        return false;
    
    // Attempt to read version
    efd::TiXmlElement* pkSectorElement = kFile.FirstChildElement("Sector");
    if (!pkSectorElement)
        return false;

    // Attempt to parse the data
    if (!NiString(pkSectorElement->Attribute("Version")).ToUInt(eVersion))
        return false;

    return eVersion == ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileDeveloper::OpenErrorCode NiTerrainSectorFileDeveloper::Open(
    FileIdentifier kID, efd::File::OpenMode eAccessMode)
{
    if (eAccessMode == efd::File::READ_ONLY && !DetectFileVersion(kID))
        return WRONG_VERSION;

    NiTerrainFileInterface::Open(kID.m_pkStoragePolicy, eAccessMode);
    m_kTerrainArchive = (kID.m_kArchivePath);
    m_iSectorX = (kID.m_iSectorX);
    m_iSectorY = (kID.m_iSectorY);

    if (Initialize())
        return SUCCESS;
    else
        return FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize the file
    bool bResult = NiTerrainSectorFile::Initialize();
    if (!bResult)
        return false;
    
    // Check that we have the correct type of access to the files we need
    bool bAccess = true;
    if (!IsWritable())
    {
        // Must at least have access to the heightmap of this sector
        bAccess &= efd::File::Access(GenerateHeightsFilename(), m_eAccessMode);
    }
    else
    {
        bAccess &= efd::File::CreateDirectoryRecursive((m_kTerrainArchive + ms_pcFolder).c_str());
    }
    bResult &= bAccess;
    
    // Return false if we have failed
    if (!bResult)
        return false;

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::Precache(efd::UInt32 uiBeginLevel, efd::UInt32 uiEndLevel, 
    efd::UInt32 eData)
{
    // Attempt to open each of the data fields to determine if they are available
    if (eData & DataField::CONFIG && 
        !efd::File::Access(GenerateSectorDataFilename(), m_eAccessMode))
        eData ^= DataField::CONFIG;
    if (eData & DataField::HEIGHTS && 
        !efd::File::Access(GenerateHeightsFilename(), m_eAccessMode))
        eData ^= DataField::HEIGHTS;
    if (eData & DataField::NORMALS && 
        !efd::File::Access(GenerateNormalsFilename(), m_eAccessMode))
        eData ^= DataField::NORMALS;
    if (eData & DataField::TANGENTS && 
        !efd::File::Access(GenerateTangentsFilename(), m_eAccessMode))
        eData ^= DataField::TANGENTS;
    if (eData & DataField::BLEND_MASK && 
        !efd::File::Access(GenerateBlendMaskFilename(), m_eAccessMode))
        eData ^= DataField::BLEND_MASK;
    if (eData & DataField::LOWDETAIL_NORMALS && 
        !efd::File::Access(GenerateLowDetailNormalMapFilename(), m_eAccessMode))
        eData ^= DataField::LOWDETAIL_NORMALS;
    if (eData & DataField::LOWDETAIL_DIFFUSE && 
        !efd::File::Access(GenerateLowDetailDiffuseMapFilename(), m_eAccessMode))
        eData ^= DataField::LOWDETAIL_DIFFUSE;
    if (eData & DataField::BOUNDS && 
        !efd::File::Access(GenerateBoundsFilename(), m_eAccessMode))
        eData ^= DataField::BOUNDS;
    if (eData & DataField::SURFACE_INDEXES && (
        !efd::File::Access(GenerateSurfaceReferenceFilename(), m_eAccessMode) || 
        !efd::File::Access(GenerateSurfaceIndexFilename(), m_eAccessMode)))
        eData ^= DataField::SURFACE_INDEXES;

    bool bResult = NiTerrainSectorFile::Precache(uiBeginLevel, uiEndLevel, eData);
    m_eCachedData = eData;
    return bResult;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::IsDataReady(DataField::Value eDataField)
{
    return (m_eCachedData & eDataField) != 0;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadSectorConfig(efd::UInt32& uiSectorWidthInVerts, 
    efd::UInt32& uiNumLOD)
{
    bool bSuccess = true;
    if (!IsDataReady(DataField::CONFIG))
        return false;

    // Attempt to load the file
    efd::TiXmlDocument kFile(GenerateSectorDataFilename());
    if (!NiTerrainXMLHelpers::LoadXMLFile(&kFile))
        return false;

    // Parse the sector config data
    efd::TiXmlElement* pkSectorElement = kFile.FirstChildElement("Sector");
    if (!pkSectorElement)
        return false;

    // Attempt to parse the data
    efd::UInt32 uiWidth;
    efd::UInt32 uiLOD;
    bSuccess &= NiString(pkSectorElement->Attribute("WidthInVerts")).ToUInt(uiWidth);
    bSuccess &= NiString(pkSectorElement->Attribute("NumLOD")).ToUInt(uiLOD);

    if (bSuccess)
    {
        uiSectorWidthInVerts = uiWidth;
        uiNumLOD = uiLOD;
    }
    return bSuccess;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteSectorConfig(efd::UInt32 uiSectorWidthInVerts, 
    efd::UInt32 uiNumLOD)
{
    EE_ASSERT(IsReady() && IsWritable());

    // Generate an XML document to save
    efd::TiXmlDocument kFile(GenerateSectorDataFilename());

    // Open the main tag and write version header
    NiTerrainXMLHelpers::WriteXMLHeader(&kFile);
    efd::TiXmlElement* pkSectorElement = NiTerrainXMLHelpers::CreateElement("Sector", NULL);
    kFile.LinkEndChild(pkSectorElement);
    pkSectorElement->SetAttribute("Version", ms_InterfaceVersion);

    // Output the sector configuration
    pkSectorElement->SetAttribute("WidthInVerts", uiSectorWidthInVerts);
    pkSectorElement->SetAttribute("NumLOD", uiNumLOD);

    // Save the file
    if (!kFile.SaveFile())
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadHeights(efd::UInt16* pusHeights, efd::UInt32 uiDataLength)
{
#ifndef EE_ASSERTS_ARE_ENABLED
    EE_UNUSED_ARG(uiDataLength);
#endif

    if (!IsDataReady(DataField::HEIGHTS))
        return false;

    bool bSuccess = true;
    EE_ASSERT(pusHeights);
    EE_ASSERT(IsReady() && !IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateHeightsFilename(), m_eAccessMode);
    if (!pkFile)
        return false;

    // Calculate the dimensions of the height map (reading a 16bit RAW file)
    efd::UInt32 uiStride = sizeof(efd::UInt16);
    efd::UInt32 uiFileLength = pkFile->GetFileSize();
    efd::UInt32 uiSectorWidth = efd::UInt32(NiSqrt(float(uiFileLength) / uiStride));
    
    EE_ASSERT(uiSectorWidth * uiSectorWidth * uiStride == uiFileLength);
    EE_ASSERT(NiIsPowerOf2(uiSectorWidth - 1));
    EE_ASSERT(uiDataLength == uiSectorWidth * uiSectorWidth);

    // Read in lines
    efd::UInt32 uiComponentSize = sizeof(efd::UInt16);
    efd::UInt32 uiLineWidth = uiSectorWidth * uiStride;
    for (efd::UInt32 uiY = 0; uiY < uiSectorWidth; ++uiY)
    {
        efd::UInt16* pusScanLine = &pusHeights[(uiSectorWidth - uiY - 1) * uiSectorWidth];
        efd::UInt32 uiNumChars = pkFile->BinaryRead(pusScanLine, uiLineWidth, &uiComponentSize);

        if (!uiNumChars || uiNumChars != uiLineWidth)
        {
            bSuccess = false;
            EE_FAIL("Unexpected end of file");
            break;
        }
    }

    EE_DELETE pkFile;
    return bSuccess;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadNormals(efd::Point2* pkNormals, efd::UInt32 uiDataLength)
{
#ifndef EE_ASSERTS_ARE_ENABLED
    EE_UNUSED_ARG(uiDataLength);
#endif

    if (!IsDataReady(DataField::NORMALS))
        return false;

    bool bSuccess = true;
    EE_ASSERT(pkNormals);
    EE_ASSERT(IsReady() && !IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateNormalsFilename(), m_eAccessMode);
    if (!pkFile)
        return false;

    // Calculate the dimensions of the height map (reading a 16bit RAW file)
    efd::UInt32 uiStride = sizeof(float) * 2;
    efd::UInt32 uiFileLength = pkFile->GetFileSize();
    efd::UInt32 uiSectorWidth = efd::UInt32(NiSqrt(float(uiFileLength) / uiStride));

    EE_ASSERT(uiSectorWidth * uiSectorWidth * uiStride == uiFileLength);
    EE_ASSERT(NiIsPowerOf2(uiSectorWidth - 1));
    EE_ASSERT(uiDataLength == uiSectorWidth * uiSectorWidth);

    // Read in lines
    efd::UInt32 uiComponentSize = sizeof(float);
    efd::UInt32 uiLineWidth = uiSectorWidth * uiStride;
    for (efd::UInt32 uiY = 0; uiY < uiSectorWidth; ++uiY)
    {
        efd::Point2* pkScanLine = &pkNormals[(uiSectorWidth - uiY - 1) * uiSectorWidth];
        efd::UInt32 uiNumChars = pkFile->BinaryRead(pkScanLine, uiLineWidth, &uiComponentSize);

        if (!uiNumChars || uiNumChars != uiLineWidth)
        {
            bSuccess = false;
            EE_FAIL("Unexpected end of file");
            break;
        }
    }

    EE_DELETE pkFile;
    return bSuccess;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadTangents(efd::Point2* pkTangents, efd::UInt32 uiDataLength)
{
#ifndef EE_ASSERTS_ARE_ENABLED
    EE_UNUSED_ARG(uiDataLength);
#endif

    if (!IsDataReady(DataField::TANGENTS))
        return false;

    bool bSuccess = true;
    EE_ASSERT(pkTangents);
    EE_ASSERT(IsReady() && !IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateTangentsFilename(), m_eAccessMode);
    if (!pkFile)
        return false;

    // Calculate the dimensions of the height map (reading a 16bit RAW file)
    efd::UInt32 uiStride = sizeof(float) * 2;
    efd::UInt32 uiFileLength = pkFile->GetFileSize();
    efd::UInt32 uiSectorWidth = efd::UInt32(NiSqrt(float(uiFileLength) / uiStride));

    EE_ASSERT(uiSectorWidth * uiSectorWidth * uiStride == uiFileLength);
    EE_ASSERT(NiIsPowerOf2(uiSectorWidth - 1));
    EE_ASSERT(uiDataLength == uiSectorWidth * uiSectorWidth);

    // Read in lines
    efd::UInt32 uiComponentSize = sizeof(float);
    efd::UInt32 uiLineWidth = uiSectorWidth * uiStride;
    for (efd::UInt32 uiY = 0; uiY < uiSectorWidth; ++uiY)
    {
        efd::Point2* pkScanLine = &pkTangents[(uiSectorWidth - uiY - 1) * uiSectorWidth];
        efd::UInt32 uiNumChars = pkFile->BinaryRead(pkScanLine, uiLineWidth, &uiComponentSize);

        if (!uiNumChars || uiNumChars != uiLineWidth)
        {
            bSuccess = false;
            EE_FAIL("Unexpected end of file");
            break;
        }
    }

    EE_DELETE pkFile;
    return bSuccess;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadLowDetailDiffuseMap(NiPixelData*& pkLowDetailDiffuse)
{
    if (!IsDataReady(DataField::LOWDETAIL_DIFFUSE))
        return false;

    EE_ASSERT(pkLowDetailDiffuse == NULL);
    EE_ASSERT(IsReady() && !IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateLowDetailDiffuseMapFilename(), m_eAccessMode);
    if (!pkFile)
        return false;

    // Attempt to read the file
    NiTGAReader kImageReader;
    pkLowDetailDiffuse = kImageReader.ReadFile(*pkFile, NULL);

    // Close the file
    EE_DELETE pkFile;
    return pkLowDetailDiffuse != NULL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadLowDetailNormalMap(NiPixelData*& pkLowDetailNormal)
{
    if (!IsDataReady(DataField::LOWDETAIL_NORMALS))
        return false;

    EE_ASSERT(pkLowDetailNormal == NULL);
    EE_ASSERT(IsReady() && !IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateLowDetailNormalMapFilename(), m_eAccessMode);
    if (!pkFile)
        return false;

    // Attempt to read the file
    NiTGAReader kImageReader; 
    pkLowDetailNormal = kImageReader.ReadFile(*pkFile, NULL);

    // Close the file
    EE_DELETE pkFile;
    return pkLowDetailNormal != NULL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadBlendMask(NiPixelData*& pkBlendMask)
{
    if (!IsDataReady(DataField::BLEND_MASK))
        return false;

    EE_ASSERT(pkBlendMask == NULL);
    EE_ASSERT(IsReady() && !IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateBlendMaskFilename(), m_eAccessMode);
    if (!pkFile)
        return false;

    // Attempt to read the file
    NiTGAReader kImageReader; 
    pkBlendMask = kImageReader.ReadFile(*pkFile, NULL);

    // Close the file
    EE_DELETE pkFile;
    return pkBlendMask != NULL;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteHeights(efd::UInt16* pusHeights, efd::UInt32 uiDataLength)
{
    EE_ASSERT(pusHeights);
    EE_ASSERT(IsReady() && IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateHeightsFilename(), m_eAccessMode);
    if (!pkFile)
    {
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
        return;
    }

    // Write the contents of this height map out to a raw file
    efd::UInt32 uiSectorWidth = efd::UInt32(NiSqrt(float(uiDataLength)));
    efd::UInt32 uiComponentSize = sizeof(efd::UInt16);
    
    // Write out the buffer in flipped Y format (standard image format)
    for (efd::UInt32 uiY = 0; uiY < uiSectorWidth; ++uiY)
    {
        efd::UInt16* pusScanLine = &pusHeights[(uiSectorWidth - uiY - 1) * uiSectorWidth];
        pkFile->BinaryWrite(pusScanLine, uiSectorWidth * sizeof(efd::UInt16), &uiComponentSize);    
    }

    // Cleanup
    EE_DELETE pkFile;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteNormals(efd::Point2* pkNormals, efd::UInt32 uiDataLength)
{
    EE_ASSERT(pkNormals);
    EE_ASSERT(IsReady() && IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateNormalsFilename(), m_eAccessMode);
    if (!pkFile)
    {
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
        return;
    }

    // Write the contents of this height map out to a raw file
    efd::UInt32 uiSectorWidth = efd::UInt32(NiSqrt(float(uiDataLength)));
    efd::UInt32 uiComponentSize = sizeof(float);

    // Write out the buffer in flipped Y format (standard image format)
    for (efd::UInt32 uiY = 0; uiY < uiSectorWidth; ++uiY)
    {
        efd::Point2* pkScanLine = &pkNormals[(uiSectorWidth - uiY - 1) * uiSectorWidth];
        pkFile->BinaryWrite(pkScanLine, uiSectorWidth * 2 * sizeof(float), &uiComponentSize);    
    }

    // Cleanup
    EE_DELETE pkFile;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteTangents(efd::Point2* pkTangents, efd::UInt32 uiDataLength)
{
    EE_ASSERT(pkTangents);
    EE_ASSERT(IsReady() && IsWritable());

    // Open the file
    efd::File* pkFile = efd::File::GetFile(GenerateTangentsFilename(), m_eAccessMode);
    if (!pkFile)
    {
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
        return;
    }

    // Write the contents of this height map out to a raw file
    efd::UInt32 uiSectorWidth = efd::UInt32(NiSqrt(float(uiDataLength)));
    efd::UInt32 uiComponentSize = sizeof(float);

    // Write out the buffer in flipped Y format (standard image format)
    for (efd::UInt32 uiY = 0; uiY < uiSectorWidth; ++uiY)
    {
        efd::Point2* pkScanLine = &pkTangents[(uiSectorWidth - uiY - 1) * uiSectorWidth];
        pkFile->BinaryWrite(pkScanLine, uiSectorWidth * 2 * sizeof(float), &uiComponentSize);    
    }

    // Cleanup
    EE_DELETE pkFile;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteLowDetailNormalMap(NiPixelData* pkLowDetailNormal)
{
    EE_ASSERT(IsReady() && IsWritable());
    if (!NiTerrainUtils::WriteTexture(GenerateLowDetailNormalMapFilename(), pkLowDetailNormal))
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteLowDetailDiffuseMap(NiPixelData* pkLowDetailDiffuse)
{
    EE_ASSERT(IsReady() && IsWritable());
    if (!NiTerrainUtils::WriteTexture(GenerateLowDetailDiffuseMapFilename(), pkLowDetailDiffuse))
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteBlendMask(NiPixelData* pkBlendMask)
{
    EE_ASSERT(IsReady() && IsWritable());
    if (!NiTerrainUtils::WriteTexture(GenerateBlendMaskFilename(), pkBlendMask))
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadSurfaceReferences(SurfaceReferenceMap& kReferences)
{
    if (!IsDataReady(DataField::SURFACE_INDEXES))
        return false;

    EE_ASSERT(IsReady() && !IsWritable());

    // Attempt to load the file
    efd::TiXmlDocument kFile(GenerateSurfaceReferenceFilename());
    if (!NiTerrainXMLHelpers::LoadXMLFile(&kFile))
        return false;

    // Parse the sector config data
    efd::TiXmlElement* pkReferencesElement = kFile.FirstChildElement("SurfaceReferences");
    if (!pkReferencesElement)
        return false;

    // Parse the cell information
    efd::TiXmlElement* pkRefElement = NULL;
    for (pkRefElement = pkReferencesElement->FirstChildElement("Surface");
        pkRefElement;
        pkRefElement = pkRefElement->NextSiblingElement())
    {
        efd::SInt32 iSurfaceIndex;
        if (!NiString(pkRefElement->Attribute("SurfaceIndex")).ToInt(iSurfaceIndex))
            continue;

        efd::UInt32 uiPackageIteration;
        if (!NiString(pkRefElement->Attribute("PackageIteration")).ToUInt(uiPackageIteration))
            continue;

        if (!pkRefElement->Attribute("SurfaceName") || 
            !pkRefElement->Attribute("PackageAsset") || 
            !pkRefElement->Attribute("LaskKnownLocation"))
            continue;
            
        efd::utf8string kSurfaceName = pkRefElement->Attribute("SurfaceName");
        efd::utf8string kPackageAsset = pkRefElement->Attribute("PackageAsset");
        efd::utf8string kLastLocation = pkRefElement->Attribute("LaskKnownLocation");

        SurfaceReference kRef;
        kRef.m_spPackageReference = NiNew NiTerrainAssetReference();

        kRef.m_iSurfaceIndex = iSurfaceIndex;
        kRef.m_uiPackageIteration = uiPackageIteration;
        kRef.m_kSurfaceName = kSurfaceName;
        kRef.m_spPackageReference->SetAssetID(kPackageAsset);
        kRef.m_spPackageReference->SetRelativeAssetLocation(kLastLocation);
        kRef.m_spPackageReference->SetReferringAssetLocation(
            (const char*)GenerateSurfaceReferenceFilename());

        kReferences[iSurfaceIndex] = kRef;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteSurfaceReferences(const SurfaceReferenceMap& kReferences)
{
    EE_ASSERT(IsReady() && IsWritable());

    // Generate an XML document to save
    efd::TiXmlDocument kFile(GenerateSurfaceReferenceFilename());

    // Open the main tag and write version header
    NiTerrainXMLHelpers::WriteXMLHeader(&kFile);
    efd::TiXmlElement* pkReferencesElement = 
        NiTerrainXMLHelpers::CreateElement("SurfaceReferences", NULL);
    kFile.LinkEndChild(pkReferencesElement);

    // Output the surface information
    SurfaceReferenceMap::const_iterator kIter;
    for (kIter = kReferences.begin(); kIter != kReferences.end(); ++kIter)
    {
        const SurfaceReference& kRef = kIter->second;
        kRef.m_spPackageReference->SetReferringAssetLocation(
            (const char*)GenerateSurfaceReferenceFilename());

        efd::TiXmlElement* pkRefElement = 
            NiTerrainXMLHelpers::CreateElement("Surface", pkReferencesElement);

        pkRefElement->SetAttribute("SurfaceIndex", kRef.m_iSurfaceIndex);
        pkRefElement->SetAttribute("PackageIteration", kRef.m_uiPackageIteration);
        pkRefElement->SetAttribute("SurfaceName", kRef.m_kSurfaceName.c_str());
        pkRefElement->SetAttribute("PackageAsset", 
            kRef.m_spPackageReference->GetAssetID().c_str());
        pkRefElement->SetAttribute("LaskKnownLocation", 
            kRef.m_spPackageReference->GetLastRelativeLocation().c_str());
    }

    if (!kFile.SaveFile())
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadCellSurfaceIndex(efd::UInt32 uiCellRegionID, 
    efd::UInt32 uiNumCells, LeafData* pkCellData)
{
    if (!IsDataReady(DataField::SURFACE_INDEXES))
        return false;
 
    EE_ASSERT(IsReady() && !IsWritable());
    EE_ASSERT(pkCellData);

    // Attempt to load the file
    efd::TiXmlDocument kFile(GenerateSurfaceIndexFilename());
    if (!NiTerrainXMLHelpers::LoadXMLFile(&kFile))
        return false;

    // Parse the sector config data
    efd::TiXmlElement* pkIndexElement = kFile.FirstChildElement("SurfaceIndex");
    if (!pkIndexElement)
        return false;

    // Parse the cell information
    efd::TiXmlElement* pkCellElement = NULL;
    for (pkCellElement = pkIndexElement->FirstChildElement("Cell");
        pkCellElement;
        pkCellElement = pkCellElement->NextSiblingElement())
    {
        // Fetch this cell's region id
        efd::UInt32 uiRegionID;
        if (!NiString(pkCellElement->Attribute("LeafID")).ToUInt(uiRegionID))
            continue;

        // Figure out the index into the buffer
        efd::SInt32 iIndex = uiRegionID - uiCellRegionID;
        if (iIndex < 0 || iIndex >= efd::SInt32(uiNumCells))
            continue;

        efd::UInt32& uiNumSurfaces = pkCellData[iIndex].m_uiNumSurfaces;
        efd::UInt32* puiSurfaceIndex = pkCellData[iIndex].m_auiSurfaceIndex;

        // Read the number of surfaces
        NiString(pkCellElement->Attribute("NumSurfaces")).ToUInt(uiNumSurfaces);
        for (efd::UInt32 uiSlot = 0; uiSlot < NiTerrainCellLeaf::MAX_NUM_SURFACES; ++uiSlot)
        {
            efd::SInt32 uiSurfaceID = 0;
            if (uiSlot < uiNumSurfaces)
            {
                NiTerrainXMLHelpers::ReadElement(pkCellElement, "Surface", uiSurfaceID);
            }
            puiSurfaceIndex[uiSlot] = uiSurfaceID;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteCellSurfaceIndex(efd::UInt32 uiLeafID, 
    efd::UInt32 uiNumCells, LeafData* pkCellData)
{
    EE_ASSERT(uiLeafID == 0); // This format does not support saving partial data at once
    EE_ASSERT(IsReady() && IsWritable());
    EE_ASSERT(pkCellData);

    // Generate an XML document to save
    efd::TiXmlDocument kFile(GenerateSurfaceIndexFilename());

    // Open the main tag and write version header
    NiTerrainXMLHelpers::WriteXMLHeader(&kFile);
    efd::TiXmlElement* pkIndexElement = NiTerrainXMLHelpers::CreateElement("SurfaceIndex", NULL);
    kFile.LinkEndChild(pkIndexElement);

    // Output the cell information
    for (efd::UInt32 uiIndex = uiLeafID; uiIndex < uiNumCells; ++uiIndex)
    {
        // Create an element for this cell
        efd::TiXmlElement* pkCellElement = 
            NiTerrainXMLHelpers::CreateElement("Cell", pkIndexElement);
        pkCellElement->SetAttribute("LeafID", uiIndex);

        // Grab references to the relevant values
        efd::UInt32& uiNumSurfaces = pkCellData[uiIndex].m_uiNumSurfaces;
        efd::UInt32* puiSurfaceIndex = pkCellData[uiIndex].m_auiSurfaceIndex;

        // Output the surface index
        pkCellElement->SetAttribute("NumSurfaces", uiNumSurfaces);
        for (efd::UInt32 uiSlot = 0; uiSlot < uiNumSurfaces; ++uiSlot)
        {
            NiTerrainXMLHelpers::WriteElement(pkCellElement, "Surface", 
                efd::SInt32(puiSurfaceIndex[uiSlot]));
        }
    }

    if (!kFile.SaveFile())
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileDeveloper::ReadCellBoundData(efd::UInt32 uiCellRegionID, 
    efd::UInt32 uiNumCells, CellData* pkCellData)
{
    if (!IsDataReady(DataField::BOUNDS))
        return false;

    EE_ASSERT(IsReady() && !IsWritable());
    EE_ASSERT(pkCellData);
    
    // Attempt to load the file
    efd::TiXmlDocument kFile(GenerateBoundsFilename());
    if (!NiTerrainXMLHelpers::LoadXMLFile(&kFile))
        return false;

    // Parse the sector config data
    efd::TiXmlElement* pkBoundsElement = kFile.FirstChildElement("Bounds");
    if (!pkBoundsElement)
        return false;

    // Parse the cell information
    efd::TiXmlElement* pkCellElement = NULL;
    for (pkCellElement = pkBoundsElement->FirstChildElement("Cell");
        pkCellElement;
        pkCellElement = pkCellElement->NextSiblingElement())
    {
        // Fetch this cell's region id
        efd::UInt32 uiRegionID;
        if (!NiString(pkCellElement->Attribute("CellRegionID")).ToUInt(uiRegionID))
            continue;

        // Figure out the index into the buffer
        efd::SInt32 iIndex = uiRegionID - uiCellRegionID;
        if (iIndex < 0 || iIndex >= efd::SInt32(uiNumCells))
            continue;

        // Reference the relevant bound objects
        NiBound& kBound = pkCellData[iIndex].m_kBound;
        NiBoxBV& kBox = pkCellData[iIndex].m_kBox;

        // Read bound information
        NiPoint3 kValue;
        float fValue;
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoundCenter", kValue);
        kBound.SetCenter(kValue);
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoundRadius", fValue);
        kBound.SetRadius(fValue);
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoxCenter", kValue);
        kBox.SetCenter(kValue);
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoxAxisX", kValue);
        kBox.SetAxis(0, kValue);
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoxAxisY", kValue);
        kBox.SetAxis(1, kValue);
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoxAxisZ", kValue);
        kBox.SetAxis(2, kValue);
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoxExtentX", fValue);
        kBox.SetExtent(0, fValue);
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoxExtentY", fValue);
        kBox.SetExtent(0, fValue);
        NiTerrainXMLHelpers::ReadElement(pkCellElement, "BoxExtentZ", fValue);
        kBox.SetExtent(0, fValue);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileDeveloper::WriteCellBoundData(efd::UInt32 uiCellRegionID, 
    efd::UInt32 uiNumCells, CellData* pkCellData)
{
    EE_ASSERT(uiCellRegionID == 0); // This format does not support saving partial data at once
    EE_ASSERT(IsReady() && IsWritable());
    EE_ASSERT(pkCellData);

    // Generate an XML document to save this bound data in
    efd::TiXmlDocument kFile(GenerateBoundsFilename());

    // Open the main tag and write version header
    NiTerrainXMLHelpers::WriteXMLHeader(&kFile);
    efd::TiXmlElement* pkBoundsElement = NiTerrainXMLHelpers::CreateElement("Bounds", NULL);
    kFile.LinkEndChild(pkBoundsElement);

    // Output the cell information
    for (efd::UInt32 uiIndex = uiCellRegionID; uiIndex < uiNumCells; ++uiIndex)
    {
        NiBound& kBound = pkCellData[uiIndex].m_kBound;
        NiBoxBV& kBox = pkCellData[uiIndex].m_kBox;

        // Create an element for this cell
        efd::TiXmlElement* pkCellElement = 
            NiTerrainXMLHelpers::CreateElement("Cell", pkBoundsElement);
        pkCellElement->SetAttribute("CellRegionID", uiIndex);

        // Output bound information
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoundCenter", kBound.GetCenter());
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoundRadius", kBound.GetRadius());
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoxCenter", kBox.GetCenter());
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoxAxisX", kBox.GetAxis(0));
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoxAxisY", kBox.GetAxis(1));
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoxAxisZ", kBox.GetAxis(2));
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoxExtentX", kBox.GetExtent(0));
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoxExtentY", kBox.GetExtent(1));
        NiTerrainXMLHelpers::WriteElement(pkCellElement, "BoxExtentZ", kBox.GetExtent(2));
    }

    if (!kFile.SaveFile())
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;
}

//--------------------------------------------------------------------------------------------------
