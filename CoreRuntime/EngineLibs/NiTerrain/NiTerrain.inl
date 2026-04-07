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

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::InToolMode()
{
    return ms_bInToolMode;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetInToolMode(bool bInToolMode)
{
    ms_bInToolMode = bInToolMode;
}

//------------------------------------------------------------------------------------------------
inline const NiFixedString& NiTerrain::GetArchivePath() const
{
    return m_kArchivePath;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetArchivePath(const NiFixedString& kArchivePath)
{
    if (m_kArchivePath != kArchivePath)
    {
        m_kArchivePath = kArchivePath;
        SetBit(true, PROP_STORAGE_FILENAME_CHANGED);
        SetBit(true, SETTING_CHANGED);
    }
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::HasShapeChangedLastUpdate()
{
    return m_bHasShapeChangedLastUpdate;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetShapeChangedLastUpdate(bool bChanged)
{
    m_bHasShapeChangedLastUpdate = bChanged;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetLODScale(float fScale)
{
    // See NiTerrainSectorData for derivation of minimum scale
    static const float fMinimumScale = 2.0f * NiSqrt(2.0);

    if (fScale >= 1.0f)
    {
        m_fLODScale = fScale * fMinimumScale;
        m_fLODShift = 0;
    }
    else
    {
        m_fLODScale = fMinimumScale;
        m_fLODShift = (fScale - 1) * NiPow(2,(float)GetNumLOD()) * m_fLODScale
            * NiSqrt(2.0f) * float(GetCellSize()) ;
    }

    SetBit(true, SETTING_CHANGED);
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::GetLODScale() const
{
    return m_fLODScale;
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::GetLODShift() const
{
    return m_fLODShift;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrain::GetLODMode() const
{
    return m_uiLODMode;
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::SetLODMode(NiUInt32 uiLODMode)
{
    if ((uiLODMode & (~NiTerrainSectorData::LOD_MORPH_ENABLE))
        < NiTerrainSectorData::NUM_LOD_MODES)
    {
        m_uiLODMode = uiLODMode;
        SetBit(true, SETTING_CHANGED);
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrain::GetCellSize() const
{
    return m_uiBlockSize;
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::SetCellSize(NiUInt32 uiBlockSize)
{
    if (uiBlockSize != m_uiBlockSize)
    {
        // Make sure it is a valid block size
        if (NiIsPowerOf2(uiBlockSize))
        {
            m_uiBlockSize = uiBlockSize;
            SetBit(true, PROP_BLOCKSIZE_CHANGED);
            SetBit(true, SETTING_CHANGED);
        }
        else
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrain::GetNumLOD() const
{
    return m_uiNumLOD;
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::SetNumLOD(NiUInt32 uiNumLOD)
{
    if (uiNumLOD != m_uiNumLOD)
    {
        m_uiNumLOD = uiNumLOD;
        SetBit(true, PROP_NUMLOD_CHANGED);
        SetBit(true, SETTING_CHANGED);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetMaxHeight(float fHeight)
{
    m_fMaxHeight = fHeight;
    SetBit(true, SETTING_CHANGED);
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::GetMaxHeight() const
{
    return m_fMaxHeight;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetMinHeight(float fHeight)
{
    m_fMinHeight = fHeight;
    SetBit(true, SETTING_CHANGED);
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetVertexSpacing(float fSpacing)
{
    EE_ASSERT(fSpacing > 0.0f);
    EE_ASSERT(fSpacing == 1.0f && "Terrain does not yet support vertex spacing.");
    m_fVertexSpacing = fSpacing;
    SetBit(true, SETTING_CHANGED);
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::GetMinHeight() const
{
    return m_fMinHeight;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrain::GetMaskSize() const
{
    return m_uiMaskSize;
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::GetVertexSpacing() const
{
    return m_fVertexSpacing;
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::CalcTerrainSpaceHeight(efd::UInt16 heightMapValue)
{
    float height = CalcWorldSpaceHeight(heightMapValue);
    height /= m_fVertexSpacing;
    return height;
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::CalcWorldSpaceHeight(efd::UInt16 heightMapValue)
{
    float height = float(heightMapValue / efd::UInt16(-1));
    height *= m_fMaxHeight - m_fMinHeight;
    height += m_fMinHeight;
    return height;
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::SetMaskSize(NiUInt32 uiMaskSize)
{
    if (uiMaskSize != m_uiMaskSize)
    {
        m_uiMaskSize = uiMaskSize;
        SetBit(true, SETTING_CHANGED);
    }
    return true;
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::GetSurfaceEntry(NiUInt32 uiSurfaceIndex, efd::utf8string& kPackageID, 
    efd::utf8string& kPackageLocation, efd::utf8string& kSurfaceID, efd::UInt32& uiPackageIteration)
{
    SurfaceReferenceMap::iterator kIter = m_kSurfaces.find(uiSurfaceIndex);
    if (kIter == m_kSurfaces.end())
        return false;

    SurfaceEntry* pkEntry = m_kSurfaces[uiSurfaceIndex];
    if (pkEntry)
    {
        // Find out the data from the original reference
        kPackageID = pkEntry->m_kPackageReference.GetAssetID();
        kPackageLocation = pkEntry->m_kPackageReference.GetRelativeAssetLocation();
        uiPackageIteration = pkEntry->m_uiIteration;
        kSurfaceID = pkEntry->m_kSurfaceName;

        // Figure out if this is a new surface that needs its new details saved out
        const NiSurface* pkSurface = pkEntry->m_pkSurface;
        if (pkSurface != NULL && 
            pkSurface != NiSurface::GetErrorSurface() && 
            pkSurface != NiSurface::GetLoadingSurface())
        {
            NiSurfacePackage* pkPackage = pkSurface->GetPackage();
            if (pkPackage)
            {
                kPackageID = pkPackage->GetAssetID();
                kPackageLocation = pkPackage->GetFilename();
                uiPackageIteration = pkPackage->GetIteration();
                kSurfaceID = pkSurface->GetName();
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::UpdateSurfaceEntry(NiUInt32 uiSurfaceIndex, 
    NiFixedString kPackageID, NiFixedString kSurfaceID, efd::UInt32 uiPackageIteration)
{    
    SurfaceEntry *pkEntry = NiNew SurfaceEntry();

    pkEntry->m_kPackageReference.SetAssetID((const char*)kPackageID);
    pkEntry->m_kSurfaceName = kSurfaceID;
    pkEntry->m_uiIteration = uiPackageIteration;
    
    ModifySurfaceEntry(uiSurfaceIndex, pkEntry);
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::ResolveSurface(NiUInt32 uiSurfaceIndex, NiSurface* pkSurface)
{
    LockSurface(true);

    SurfaceReferenceMap::iterator kIter = m_kSurfaces.find(uiSurfaceIndex);
    if (kIter != m_kSurfaces.end())
    {
        
        SurfaceEntry* pkEntry = m_kSurfaces[uiSurfaceIndex];
        if (pkEntry)
        {
            // Prepare the surface for use now (may involve loading textures)
            pkSurface->PrepareForUse();

            // Resolve the entry
            pkEntry->m_pkSurface = pkSurface;
        }

        // Mark the surface as changed
        NotifySurfaceChanged(pkSurface);

    }
    UnlockSurface(true);
}

//------------------------------------------------------------------------------------------------
inline efd::UInt32 NiTerrain::GetNumUnresolvedSurfaces()
{
    efd::UInt32 uiResult = 0;
    SurfaceReferenceMap::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        SurfaceEntry* pkEntry = kIter->second;
        if (pkEntry && pkEntry->m_pkSurface == 0)
        {
            uiResult++;
        }
    }
    
    return uiResult;
}

//------------------------------------------------------------------------------------------------
inline const NiSurface* NiTerrain::GetSurfaceAt(NiUInt32 uiIndex)
{
    SurfaceReferenceMap::iterator kIter = m_kSurfaces.find(uiIndex);
    if (kIter == m_kSurfaces.end())
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1, 
            ("NiTerrain: An unknown surface was referenced on a sector. The sector files are "
            "inconsistent with the terrain surface index."));
        return NiSurface::GetErrorSurface();
    }

    const NiSurface* pkResult = NULL;
    const SurfaceEntry* pkEntry = m_kSurfaces[uiIndex];
    if (pkEntry) // Fetch the surface entry
        pkResult = pkEntry->m_pkSurface;
    else // There is no surface data matching that entry
        pkResult = NiSurface::GetErrorSurface();

    if (!pkResult) // That surface hasn't been resolved yet
        pkResult = NiSurface::GetLoadingSurface();

    return pkResult;
}

//------------------------------------------------------------------------------------------------
inline NiInt32 NiTerrain::GetSurfaceIndex(const NiSurface* pkSurface)
{
    SurfaceReferenceMap::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        SurfaceEntry* pkEntry = kIter->second;
        if (pkEntry && pkEntry->m_pkSurface == pkSurface)
            return kIter->first;
    }

    // We didn't find anything
    return -1;
}

//------------------------------------------------------------------------------------------------
inline NiInt32 NiTerrain::GetSurfaceIndex(NiFixedString kPackageID, 
    NiFixedString kSurfaceID)
{
    SurfaceReferenceMap::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        SurfaceEntry* pkEntry = kIter->second;
        if (pkEntry && 
            pkEntry->m_kPackageReference.GetAssetID() == kPackageID && 
            pkEntry->m_kSurfaceName == kSurfaceID)
            return kIter->first;
    }

    // We didn't find anything
    return -1;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrain::GetNumSurfaces()
{
    if (m_kSurfaces.size())
        return m_kSurfaces.rbegin()->first + 1;
    return 0;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrain::GetLowDetailTextureSize() const
{
    return m_uiLowDetailTextureSize;
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::SetLowDetailTextureSize(NiUInt32 uiSize)
{
    if (uiSize != m_uiLowDetailTextureSize)
    {
        m_uiLowDetailTextureSize = uiSize;
        SetBit(true, SETTING_CHANGED);
    }
    return true;
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::GetLowDetailSpecularPower() const
{
    NiFloatsExtraData* pkSpecularData = NiDynamicCast(NiFloatsExtraData, 
        GetExtraData(NiTerrainCellShaderData::LOWDETAIL_SPECULAR_SHADER_CONSTANT));
    return pkSpecularData->GetValue(0);
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::SetLowDetailSpecularPower(float fPower)
{
    NiFloatsExtraData* pkSpecularData = NiDynamicCast(NiFloatsExtraData, 
        GetExtraData(NiTerrainCellShaderData::LOWDETAIL_SPECULAR_SHADER_CONSTANT));
    pkSpecularData->SetValue(0, fPower);
    return true;
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::GetLowDetailSpecularIntensity() const
{
    NiFloatsExtraData* pkSpecularData = NiDynamicCast(NiFloatsExtraData, 
        GetExtraData(NiTerrainCellShaderData::LOWDETAIL_SPECULAR_SHADER_CONSTANT));
    return pkSpecularData->GetValue(1);
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::SetLowDetailSpecularIntensity(float fIntensity)
{
    NiFloatsExtraData* pkSpecularData = NiDynamicCast(NiFloatsExtraData, 
        GetExtraData(NiTerrainCellShaderData::LOWDETAIL_SPECULAR_SHADER_CONSTANT));
    pkSpecularData->SetValue(1, fIntensity);
    return true;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrain::GetCalcSectorSize() const
{
    return (GetCellSize() << m_uiNumLOD) + 1;
}

//------------------------------------------------------------------------------------------------
inline const NiTMap<NiUInt32, NiTerrainSector*>& NiTerrain::GetLoadedSectors() 
    const
{
    return m_kSectors;
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::HasSettingChanged() const
{
    return GetBit(SETTING_CHANGED);
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::RemoveSector(
    NiTerrainSector* pkSector)
{
    NiTerrainSector::SectorID uiIndex = pkSector->GetSectorID();
    m_kSectors.RemoveAt(uiIndex);     

    // Remove this sector from the low detail scene
    if (m_pkPaintingData)
    {
        NiNode* pkSectorLowDetailScene = pkSector->GetLowDetailScene();
        if (pkSectorLowDetailScene)
            m_pkPaintingData->m_spLowDetailScene->DetachChild(
                pkSectorLowDetailScene);
    }

    // Detach this sector from the root node:
    DetachChild(pkSector);
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::AddSector(NiTerrainSector* pkSector)
{
    NiInt16 sIndexX, sIndexY;
    pkSector->GetSectorIndex(sIndexX, sIndexY);
    NiTerrainSector::SectorID uiIndex = pkSector->GetSectorID();
        
    // Check for a previous sector at this location
    NiTerrainSector* pkOldSector = 0;
    m_kSectors.GetAt(uiIndex, pkOldSector);
    if (pkOldSector)
        RemoveSector(pkOldSector);

    // Figure out the name of this sector:
    NiString kSectorName;
    kSectorName = "Sector_";
    kSectorName += NiString::FromInt(sIndexX);
    kSectorName += "_";
    kSectorName += NiString::FromInt(sIndexY);
    pkSector->SetName(NiFixedString(kSectorName));
   
    // Assign the current sector
    m_kSectors.SetAt(uiIndex, pkSector);    
    // Attach this sector to the root node:
    AttachChild(pkSector);
    // Push this terrain's properties to this sector
    UpdateSector(pkSector);
    // Push the terrain transform to this sector
    pkSector->UpdateWorldData();
}

//------------------------------------------------------------------------------------------------
inline NiTerrainSector* NiTerrain::GetSector(NiTerrainSector::SectorID kSectorID)
{
    NiTerrainSector* pkResult = NULL;
    m_kSectors.GetAt(kSectorID, pkResult);

    return pkResult;
}

//------------------------------------------------------------------------------------------------
inline const NiTerrainSector* NiTerrain::GetSector(NiTerrainSector::SectorID kSectorID) const
{
    NiTerrainSector* pkResult = NULL;
    m_kSectors.GetAt(kSectorID, pkResult);

    return pkResult;
}

//------------------------------------------------------------------------------------------------
inline NiTerrainSector* NiTerrain::GetSector(NiInt16 sSectorX, NiInt16 sSectorY)
{
    NiTerrainSector::SectorID uiIndex;
    NiTerrainSector::GenerateSectorID(sSectorX, sSectorY, uiIndex);

    return GetSector(uiIndex);
}

//------------------------------------------------------------------------------------------------
inline const NiTerrainSector* NiTerrain::GetSector(NiInt16 sSectorX, NiInt16 sSectorY) const
{
    NiTerrainSector::SectorID uiIndex;
    NiTerrainSector::GenerateSectorID(sSectorX, sSectorY, uiIndex);

    return GetSector(uiIndex);
}

//------------------------------------------------------------------------------------------------
inline NiTerrainSector* NiTerrain::CreateSector(NiInt16 sSectorX, NiInt16 sSectorY)
{
    NiTerrainSector* pkSector = NiNew NiTerrainSector(false);

    pkSector->SetSectorIndex(sSectorX, sSectorY);
    pkSector->SetIsDeformable(false);

    // Set the transformation of this sector
    float fSectorWidth = (float)(GetCellSize() << GetNumLOD());
    NiPoint3 kTranslation(sSectorX * fSectorWidth,sSectorY * fSectorWidth,0);
    pkSector->SetTranslate(kTranslation);

    // Tell this sector that it is a part of this terrain (causes this
    // terrain's data to be sent to it and is added to the list of sectors)
    pkSector->SetTerrain(this);

    // Make sure the sector creates paging data too
    pkSector->CreatePagingData();

    return pkSector;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::UseSectorList(const NiTPrimitiveSet<NiUInt32>&
    kSectorList)
{
    for (NiUInt32 uiIndex = 0; uiIndex < kSectorList.GetSize();
        ++uiIndex)
    {
        NiUInt32 uiValue = kSectorList.GetAt(uiIndex);
        NiInt16 sValueX, sValueY;
        NiTerrainSector::GenerateSectorIndex(uiValue, sValueX, sValueY);

        if (!GetSector(sValueX, sValueY))
        {
            CreateSector(sValueX, sValueY);
        }
    }
    if (kSectorList.GetSize() < m_kSectors.GetCount())
    {
        NiTerrainSector* pkSector = NULL;
        NiUInt32 uiIndex;
        NiTMapIterator kIterator = m_kSectors.GetFirstPos();
        while (kIterator)
        {
            m_kSectors.GetNext(kIterator, uiIndex, pkSector);
            if (kSectorList.Find(uiIndex) == -1)
            {
                RemoveSector(pkSector);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::UpdateSector(NiTerrainSector *pkSector)
{
    // Update all the sectors with the latest changes to the terrain
    // Archive file

    // Has the user changed the dimensions of the terrain?
    if (m_uiNumLOD != pkSector->GetSectorData()->GetNumLOD() ||
        m_uiBlockSize != pkSector->GetSectorData()->GetCellSize())
    {
        pkSector->SetFormat(m_uiBlockSize, m_uiNumLOD);
    }

    // LOD Scale
    pkSector->SetLODScale(GetLODScale(), GetLODShift());

    // LOD Mode
    pkSector->SetLODMode(GetLODMode());

    // Low Detail texture size
    pkSector->SetLowDetailTextureSize(m_uiLowDetailTextureSize);
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrain::GetNumLoadedSectors() const
{
    return m_kSectors.GetCount();
}

//------------------------------------------------------------------------------------------------
inline NiMaterial* NiTerrain::GetMaterial()
{
    EE_ASSERT(m_spTerrainMaterial);
    return m_spTerrainMaterial;
}

//------------------------------------------------------------------------------------------------
inline float NiTerrain::GetSurfaceMaskDensity()
{
    // Calculate the number of pixels per unit area of the terrain 
    // (one unit = distance between adjacent verts)
    // This density calculation takes into account the pixel duplication involved in the masks.

    // Subtract the pixels that are duplicated from the standard texture size
    // (2 pixels per high detail cell)
    float fDensity = float(m_uiMaskSize - (2 << m_uiNumLOD));
    // Divide by the width of the sector
    fDensity /= float(GetCalcSectorSize() - 1);

    // If fDensity is zero, the NumLODs are too large for the MaskSize
    EE_ASSERT(fDensity > 0.0f);
    return fDensity;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::EnforceValidBound()
{
    if (m_kWorldBound.GetRadius() == 0.0f)
        m_kWorldBound.SetRadius(1.0f);
}

//------------------------------------------------------------------------------------------------
inline NiTerrain::StoragePolicy* NiTerrain::GetStoragePolicy()
{
    if (!m_spStoragePolicy)
    {
        m_spStoragePolicy = EE_NEW StoragePolicy();
    }

    return m_spStoragePolicy;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetStoragePolicy(StoragePolicy* pkPolicy)
{
    // Check that all the callbacks are valid
    EE_ASSERT(pkPolicy->m_spSectorReadFormat);
    EE_ASSERT(pkPolicy->m_spSectorWriteFormat);
    EE_ASSERT(pkPolicy->m_spTerrainReadFormat);
    EE_ASSERT(pkPolicy->m_spTerrainWriteFormat);

    m_spStoragePolicy = pkPolicy;
}

//------------------------------------------------------------------------------------------------
inline NiTerrain::CustomDataPolicy* NiTerrain::GetCustomDataPolicy()
{
    return m_spCustomDataPolicy;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetCustomDataPolicy(CustomDataPolicy* pkPolicy)
{
    m_spCustomDataPolicy = pkPolicy;
}

//------------------------------------------------------------------------------------------------
inline NiTerrainStreamingManager* NiTerrain::GetStreamingManager()
{
    return &m_kStreamingManager;
}

//------------------------------------------------------------------------------------------------
inline NiTerrainDecalManager* NiTerrain::GetDecalManager()
{
    if (!m_spDecalManager)
    {
        // Configure the decal manager
        m_spDecalManager = EE_NEW NiTerrainDecalManager();
        AttachChild(m_spDecalManager);
    }
    
    return m_spDecalManager;
}

//------------------------------------------------------------------------------------------------
inline void NiTerrain::SetSurfaceLibrary(NiTerrainSurfaceLibrary* pkLibrary)
{
    // The surface library for a terrain may only be set once
    EE_ASSERT(m_spSurfaceLibrary == NULL);
    m_spSurfaceLibrary = pkLibrary;
}

//------------------------------------------------------------------------------------------------
inline NiTerrainSurfaceLibrary* NiTerrain::GetSurfaceLibrary()
{
    if (!m_spSurfaceLibrary)
    {
        m_spSurfaceLibrary = EE_NEW NiTerrainSurfaceLibrary();
        m_spSurfaceLibrary->GetStoragePolicy()->SetAssetResolver(
            m_spStoragePolicy->GetAssetResolver());
    }
    return m_spSurfaceLibrary;
}

//------------------------------------------------------------------------------------------------
inline efd::Float32 NiTerrain::GetCellRange(efd::SInt32 iCellLOD)
{
    const float fSquareRoot2 = NiSqrt(2.0f);
    efd::SInt32 iNumDivisions = GetNumLOD() - iCellLOD;

    efd::Float32 fRange = NiMax(0.0f, (float)(GetCellSize()) * 
        (float)(1 << iNumDivisions) * GetLODScale() * fSquareRoot2 + 
        GetLODShift());
    fRange = NiSqr(fRange);

    return fRange;
}

//------------------------------------------------------------------------------------------------
inline bool NiTerrain::IsCellInRange(efd::Float32 fMaxDistanceSqr, efd::SInt32 iCellLOD, 
    const efd::Point3& kCameraLocation, const efd::Point3& kCellCenter)
{
    // Calculate the distance the camera is away from the cell
    float fDist = 0;
    switch(GetLODMode() 
        & ~NiTerrainSectorData::LOD_MORPH_ENABLE)
    {   
    case NiTerrainSectorData::LOD_MODE_3D:
        {
            fDist = (kCameraLocation - kCellCenter).Length();
        }break;
    case NiTerrainSectorData::LOD_MODE_25D: 
        {
            if (NiSqr(kCameraLocation.z) > fMaxDistanceSqr)
                return false;
        }// Continues into default case
    case NiTerrainSectorData::LOD_MODE_2D:
    default:    
        {
            fDist = NiSqrt(NiSqr(kCameraLocation.x - kCellCenter.x) +
                NiSqr(kCameraLocation.y - kCellCenter.y));
        }
    }

    // Adjust the distance by the cell's width
    const float fSquareRoot2 = NiSqrt(2.0f);
    efd::SInt32 iNumDivisions = GetNumLOD() - iCellLOD;
    float fBlockWidth = float(GetCellSize() << iNumDivisions);
    fDist -= fSquareRoot2 * (fBlockWidth * 0.5f);

    // Perform the range test
    return (fMaxDistanceSqr < 0) || ((fDist * fDist) < fMaxDistanceSqr);
}

//------------------------------------------------------------------------------------------------
inline efd::SInt32 NiTerrain::GetSectorMaximumVisibleLOD(efd::SInt16 iSectorX, efd::SInt16 iSectorY, 
    const efd::Point3& kCameraPosition)
{
    efd::SInt32 iResult = -1;
    
    // Calculate the center of the sector
    float fSectorWidth = float(GetCalcSectorSize() - 1);
    NiPoint3 kSectorCenter(
        (float)iSectorX * fSectorWidth, 
        (float)iSectorY * fSectorWidth,
        0.0f);

    // Transform the camera position to world space
    NiTransform kTerrainTransform = GetWorldTransform();
    NiTransform kInverseTerrainTransform;
    kTerrainTransform.Invert(kInverseTerrainTransform);
    NiPoint3 kRefPoint = kInverseTerrainTransform * kCameraPosition;

    // Calculate the sector range
    float fSectorRange = fSectorWidth * NiSqrt(2.0f) / 2.0f;

    // Calculate the distance to this sector
    efd::Point3 kDirectionToCell = kSectorCenter - kRefPoint;
    efd::Float32 fDistance = kDirectionToCell.Unitize();
    fDistance -= fSectorRange;
    fDistance = efd::Max(fDistance, 0.0f);
    // Calculate the closest position
    efd::Point3 kCellPosition = kRefPoint + kDirectionToCell * fDistance;

    // Work out what LOD cells are in range at this distance:
    efd::SInt32 iNumLOD = GetNumLOD();
    for (iResult = 0; iResult <= iNumLOD; ++iResult)
    {
        // Check if this cell is within range
        efd::Float32 fRange = GetCellRange(iResult);
        if (!IsCellInRange(fRange, iResult, kRefPoint, kCellPosition))
        {
            iResult = iResult - 1;
            break;
        }
    }
    iResult = efd::Clamp(iResult, 0, iNumLOD);

    return iResult;
}

//------------------------------------------------------------------------------------------------
inline efd::Point3 NiTerrain::GetSectorCenter(NiTerrainSector::SectorID kSectorID)
{
    efd::Point3 kResult;

    // Calculate the index of this sector
    efd::SInt16 iSectorX, iSectorY;
    NiTerrainSector::GenerateSectorIndex(kSectorID, iSectorX, iSectorY);

    // Calculate the center of the sector
    float fSectorWidth = float(GetCalcSectorSize() - 1);
    NiPoint3 kSectorCenter(
        (float)iSectorX * fSectorWidth, 
        (float)iSectorY * fSectorWidth,
        0.0f);

    // Transform this to world space
    NiTransform kTerrainTransform = GetWorldTransform();
    kResult = kTerrainTransform * kSectorCenter;

    return kResult;
}

//------------------------------------------------------------------------------------------------
inline efd::Point2 NiTerrain::ConvertToSectorSpace(const efd::Point3& kPoint)
{
    efd::Point2 kResult;

    // Calculate some basic factors.
    float fSectorWidth = (float)GetCalcSectorSize() - 1.0f;

    // Convert the ref point to terrain space
    NiTransform kTerrainTransform = GetWorldTransform();
    NiTransform kInverseTerrainTransform;
    kTerrainTransform.Invert(kInverseTerrainTransform);
    NiPoint3 kRefPoint = kInverseTerrainTransform * kPoint;

    kResult = NiPoint2(kRefPoint.x / fSectorWidth, kRefPoint.y / fSectorWidth);
    return kResult;
}

//------------------------------------------------------------------------------------------------
inline NiTerrainSector::SectorID NiTerrain::GetNearestSector(const efd::Point3& kPoint)
{
    NiTerrainSector::SectorID kResultID;

    // Calculate some basic factors.
    float fSectorWidth = (float)GetCalcSectorSize() - 1.0f;

    // Convert the ref point to terrain space
    NiTransform kTerrainTransform = GetWorldTransform();
    NiTransform kInverseTerrainTransform;
    kTerrainTransform.Invert(kInverseTerrainTransform);
    NiPoint3 kRefPoint3 = kInverseTerrainTransform * kPoint;
    NiPoint2 kRefPoint = NiPoint2(kRefPoint3.x, kRefPoint3.y);

    NiPoint2 kCenterSectorID = NiPoint2(
        NiFloor(kRefPoint.x / fSectorWidth + 0.5f),
        NiFloor(kRefPoint.y / fSectorWidth + 0.5f));

    efd::SInt16 sectorX = efd::Int32ToInt16(efd::SInt32(kCenterSectorID.x));
    efd::SInt16 sectorY = efd::Int32ToInt16(efd::SInt32(kCenterSectorID.y));
    NiTerrainSector::GenerateSectorID(sectorX, sectorY, kResultID);

    return kResultID;
}

//------------------------------------------------------------------------------------------------