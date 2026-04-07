// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2007 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiTerrainPCH.h"

#include "NiTerrainDecorationFunctor.h"

#include <NiRandomLCG.h>
#include "NiTerrain.h"

NiFixedString NiTerrainDecorationFunctor::FUNCTOR_NAME = NULL;
NiFixedString NiTerrainDecorationFunctor::COLOR_EFFECT_NAME = NULL;

//---------------------------------------------------------------------------
void NiTerrainDecorationFunctor::_SDMInit()
{
    FUNCTOR_NAME = "Terrain";
    COLOR_EFFECT_NAME = "Color map effect";
}
//---------------------------------------------------------------------------
void NiTerrainDecorationFunctor::_SDMShutdown()
{
    FUNCTOR_NAME = NULL;
    COLOR_EFFECT_NAME = NULL;
}
//---------------------------------------------------------------------------
bool NiTerrainDecorationFunctor::IsValidTargetType(NiObject* pkTarget)
{
    return NiIsKindOf(target_type, pkTarget);
}
//---------------------------------------------------------------------------
void NiTerrainDecorationFunctor::InitializeExtraData(
    NiTPrimitiveArray<NiExtraData*>& kExtraData)
{
    kExtraData.SetSize(EDK_COUNT);

    // Terrain material meta data key
    kExtraData.SetAt(EDK_MATERIAL_META_DATA, NiNew NiStringExtraData(NULL));
    kExtraData[EDK_MATERIAL_META_DATA]->SetName("MetaDataKey");

    // Opacity threshold
    kExtraData.SetAt(EDK_OPACITY_THRESHOLD, NiNew NiFloatExtraData(0.85f));
    kExtraData[EDK_OPACITY_THRESHOLD]->SetName("MetaDataOpacityThreshold");

    // Scale
    kExtraData.SetAt(EDK_SCALE, NiNew NiFloatExtraData(1.0f));
    kExtraData[EDK_SCALE]->SetName("InstanceScale");

    // Scale Variation
    kExtraData.SetAt(EDK_SCALE_VARIATION, NiNew NiFloatExtraData(0.3f));
    kExtraData[EDK_SCALE_VARIATION]->SetName("InstanceScaleVariation");

    // Quota Multiplier
    kExtraData.SetAt(EDK_QUOTA_MULTIPLIER, NiNew NiFloatExtraData(0.94f));
    kExtraData[EDK_QUOTA_MULTIPLIER]->SetName("QuotaMultiplier");

    // Take color from the terrain
    kExtraData.SetAt(EDK_TERRAIN_COLOR, NiNew NiBooleanExtraData(true));
    kExtraData[EDK_TERRAIN_COLOR]->SetName("UseTerrainSurfaceColor");

    // Take normals from the terrain
    kExtraData.SetAt(EDK_TERRAIN_NORMALS, NiNew NiBooleanExtraData(true));
    kExtraData[EDK_TERRAIN_NORMALS]->SetName("UseTerrainNormals");
}
//---------------------------------------------------------------------------
bool NiTerrainDecorationFunctor::Validate(NiTerrain* pkTerrain,
    NiTPrimitiveArray<NiExtraData*>& kExtraData, const NiUInt32 auiCellCount[2],
    const NiPoint2& kCellRange,
    const NiTransform& kLayerWorldTransform)
{
    EE_UNUSED_ARG(kExtraData);
    EE_UNUSED_ARG(kLayerWorldTransform);
    EE_UNUSED_ARG(kCellRange);

    EE_ASSERT(pkTerrain);

    // Number of blocks must be a factor of the number of cells
    NiUInt32 uiBlocksPerSectorSide = (pkTerrain->GetCalcSectorSize() - 1) / 
        pkTerrain->GetCellSize();
    NiIndex kNumSectors = NiIndex(1, 1);

    // There needs to be at least one sector
    if (kNumSectors.x == 0 || kNumSectors.y == 0)
        return false;

    // Is there an integral number of decoration cells per terrain cell?
    if (auiCellCount[0] % (kNumSectors.x * uiBlocksPerSectorSide) != 0 || 
        auiCellCount[1] % (kNumSectors.y * uiBlocksPerSectorSide) != 0)
    {
        return false;
    }

    if (!NiIsPowerOf2(auiCellCount[0] / kNumSectors.x) ||
        !NiIsPowerOf2(auiCellCount[1] / kNumSectors.y))
    {
        return false;
    }

    return true;
}
//--------------------------------------------------------------------------------------------------
void NiTerrainDecorationFunctor::ConfigureMesh(const NiTPrimitiveArray<NiExtraData*>& kExtraData,
    const NiUInt32 auiCellCount[2], const NiPoint2& kCellRange, NiUInt32 uiFieldIndex, 
    const NiTransform& kLayerWorldTransform, NiTerrain* pkTerrain, NiAVObject* pkBase) 
{
    EE_UNUSED_ARG(kExtraData);
    EE_UNUSED_ARG(auiCellCount);
    EE_UNUSED_ARG(kCellRange);
    EE_UNUSED_ARG(kLayerWorldTransform);

    // Does the mesh have a texturing property?
    NiTexturingProperty* pkMeshProperty = NiDynamicCast(NiTexturingProperty, 
        pkBase->GetProperty(NiProperty::TEXTURING));
    if (!pkMeshProperty)
        return;

    // Terrain color effect
    NiNode* pkLayer = NiVerifyStaticCast(NiNode, pkBase->GetParent());
    if (!pkLayer)
        return;

    NiAVObject* pkEffectObject = pkLayer->GetObjectByName(COLOR_EFFECT_NAME);

    // Enable using the terrains color?
    NiBooleanExtraData* pkUseTerrainColor = NiVerifyStaticCast(NiBooleanExtraData, 
        kExtraData[EDK_TERRAIN_COLOR]);
    if (pkUseTerrainColor->GetValue())
    {    
        // Find the terrains low detail map and apply to the base mesh
        NiTerrainSector* pkSector;
        if (pkTerrain->GetLoadedSectors().GetAt(uiFieldIndex, pkSector))
        {
            NiTexture* pkLowDetailDiffuse = pkSector->GetTexture(
                NiTerrainCell::TextureType::LOWDETAIL_DIFFUSE);
            if (pkLowDetailDiffuse != NULL)
            {
                NiTextureEffect* pkEffect = NULL;
                if (!pkEffectObject)
                {
                    pkEffect = EE_NEW NiTextureEffect();
                    pkEffect->SetName(COLOR_EFFECT_NAME);
                    pkLayer->AttachChild(pkEffect);
                    pkLayer->AttachEffect(pkEffect);
                    pkLayer->UpdateEffects();
                }
                else
                {
                    pkEffect = NiDynamicCast(NiTextureEffect, pkEffectObject);
                    if (!pkEffect)
                        return;
                }

                // Reconfigure the effect to use a WORLD_PARALLEL projection
                float fSectorSize = 
                    (float)(pkTerrain->GetCalcSectorSize() - 1) * pkTerrain->GetWorldScale();

                // How big are the pixels, in sector space?
                float fLowDetailTextureSize = (float)pkTerrain->GetLowDetailTextureSize();
                if (fLowDetailTextureSize <= 2.0f)
                    fLowDetailTextureSize = 4.0f;
                float fScale = fLowDetailTextureSize / (fLowDetailTextureSize - 2.0f); 
                fSectorSize *= fScale;
                float fInverseSize = 1.0f / (fSectorSize);

                efd::Matrix3 kProjMatrix;
                kProjMatrix.SetRow(0, efd::Point3(0.0f, 0.0f, fInverseSize));
                kProjMatrix.SetRow(1, efd::Point3(0.0f, fInverseSize, 0.0f));
                kProjMatrix.SetRow(2, efd::Point3(0.0f, 0.0f, 0.0f));
                pkEffect->SetModelProjectionMatrix(kProjMatrix);
                pkEffect->SetModelProjectionTranslation(efd::Point3(0.5f, 0.5f, 0.0f));

                // Configure the orientation of the effect (it projects down it's positive X axis)
                const efd::Float32 f90Degrees = NI_PI / 2.0f;
                efd::Matrix3 kRotMatrix;
                kRotMatrix.FromEulerAnglesXYZ(2.0f * f90Degrees, -f90Degrees, 0.0f);
                pkEffect->SetRotate(kRotMatrix);
                
                // Set the texture
                pkEffect->SetEffectTexture(pkLowDetailDiffuse);
                pkEffect->SetTextureClamp(NiTexturingProperty::CLAMP_S_CLAMP_T);

                // Finally update the effect
                pkEffect->Update(0.0f);
                pkEffect->UpdateEffects();
                pkEffect->Update(0.0f);
            }
        }
    }
    else
    {
        // Detach the effect
        pkLayer->DetachEffect(NiDynamicCast(NiTextureEffect, pkEffectObject));
        pkLayer->DetachChild(pkEffectObject);
        pkLayer->UpdateEffects();
    }

    NiMesh* pkBaseMesh = NiDynamicCast(NiMesh, pkBase);
    if (pkBaseMesh != NULL)
        pkBaseMesh->SetMaterialNeedsUpdate(true);
}

//---------------------------------------------------------------------------
bool NiTerrainDecorationFunctor::GenerateTransforms(NiTerrain* pkTerrain,
    const NiTPrimitiveArray<NiExtraData*>& kExtraData,
    NiTransform* pkTransforms, NiUInt32 uiTransformCount,
    const NiUInt32 auiCellCount[2], const NiUInt32 auiCellIndex[2],
    const NiPoint3& kCellCenter, const NiPoint2& kRange, NiUInt32 uiFieldIndex,
    const NiTransform& kWorldTransform,
    NiRandomLCG* pkRandom)
{
    EE_UNUSED_ARG(kRange);

    // Throughout this function, we use (where possible) the following naming
    // convention:
    // cell = decoration cell
    // block = terrain cell leaf

    // Width and breadth, in sectors, of the area covered by the cells
    NiIndex kNumSectors = NiIndex(1, 1);
    NiTerrainSector* pkSector;
    if (!pkTerrain->GetLoadedSectors().GetAt(uiFieldIndex, pkSector))
        return false;

    // Cell count must be a power of two when used in terrains
    EE_ASSERT(NiIsPowerOf2(auiCellCount[0] / kNumSectors.x) && 
        NiIsPowerOf2(auiCellCount[1] / kNumSectors.y));

    // Work out which sector we are in, within the surrounding sectors. (0,0) is
    // the bottom left of all surrounding sectors.
    NiIndex kCellsPerSectorSide = NiIndex(
        auiCellCount[0] / kNumSectors.x,
        auiCellCount[1] / kNumSectors.y);
    EE_ASSERT(kCellsPerSectorSide.x > 0 && kCellsPerSectorSide.y > 0);

    // Cell index, within our sector
    NiIndex kLocalCellIndex = NiIndex(
        auiCellIndex[0] % kCellsPerSectorSide.x,
        auiCellIndex[1] % kCellsPerSectorSide.y);

    // Calculate some variables that will be used in the generation, and place
    // them in this info struct.
    GeneratorInfo kGenInfo;

    // Number of voxels along a terrain cell's side
    kGenInfo.m_uiBlockSize = pkTerrain->GetCellSize();

    NiUInt32 uiBlocksPerSectorSide = 1 << pkTerrain->GetNumLOD();

    kGenInfo.m_kCellsPerBlock.x = 
        auiCellCount[0] / (uiBlocksPerSectorSide * kNumSectors.x);
    kGenInfo.m_kCellsPerBlock.y = 
        auiCellCount[1] / (uiBlocksPerSectorSide * kNumSectors.y);

    // The index offset within our leaf    
    div_t dtDiv;
    dtDiv = div((int)kLocalCellIndex.x, kGenInfo.m_kCellsPerBlock.x);
    kGenInfo.m_kLeafIndex.x = dtDiv.quot;//uiCellX / uiCellsPerBlockX;
    kGenInfo.m_kCellOffset.x = dtDiv.rem;//uiCellX % uiCellsPerBlockX;
    dtDiv = div((int)kLocalCellIndex.y, kGenInfo.m_kCellsPerBlock.y);
    kGenInfo.m_kLeafIndex.y = dtDiv.quot;//uiCellY / uiCellsPerBlockY;
    kGenInfo.m_kCellOffset.y = dtDiv.rem;//uiCellY % uiCellsPerBlockY;

    EE_ASSERT(kGenInfo.m_kCellOffset.x < kGenInfo.m_kCellsPerBlock.x &&
        kGenInfo.m_kCellOffset.y < kGenInfo.m_kCellsPerBlock.y);

    // Number of terrain block voxels per cell
    kGenInfo.m_kSizePerCell = NiIndex(
        (kGenInfo.m_uiBlockSize * uiBlocksPerSectorSide * kNumSectors.x) / auiCellCount[0],
        (kGenInfo.m_uiBlockSize * uiBlocksPerSectorSide * kNumSectors.y) / auiCellCount[1]);

    // Work out the exact location and size of the cell within the terrain block
    kGenInfo.m_kBottomLeft.x = 
        kGenInfo.m_kCellOffset.x * kGenInfo.m_kSizePerCell.x;
    kGenInfo.m_kBottomLeft.y = 
        kGenInfo.m_kCellOffset.y * kGenInfo.m_kSizePerCell.y;
    kGenInfo.m_kTopRight.x = 
        kGenInfo.m_kBottomLeft.x + kGenInfo.m_kSizePerCell.x;
    kGenInfo.m_kTopRight.y = 
        kGenInfo.m_kBottomLeft.y + kGenInfo.m_kSizePerCell.y;

    // The bottom left corner of our cell, in XY world space.
    kGenInfo.m_kWorldCellCorner = kCellCenter - NiPoint3(
        (pkTerrain->GetCellSize() / kGenInfo.m_kCellsPerBlock.x) * ((float)kGenInfo.m_kCellOffset.x + 0.5f) * pkTerrain->GetWorldScale(),
        (pkTerrain->GetCellSize() / kGenInfo.m_kCellsPerBlock.y) * ((float)kGenInfo.m_kCellOffset.y + 0.5f) * pkTerrain->GetWorldScale(),
        0.0f);

    // Get how much of the sector is loaded.
    // IMPORTANT NOTE: this assumes that the functor is being called from within
    // the main Update tree thread, not within a multithreaded or floodgate
    // scenario. If the functor is being used in a multithreaded environment,
    // we MUST lock all reads to terrain values and heights.
    NiInt32 iHighestLoadedLOD = pkSector->GetSectorData()->GetHighestLoadedLOD();

    // Is the sector loaded to maximum detail?
    if (iHighestLoadedLOD < NiInt32(pkTerrain->GetNumLOD()))
        return false;

    NiTerrainCell* pkCell = pkSector->GetCell(
        kGenInfo.m_kLeafIndex.x + kGenInfo.m_kLeafIndex.y * 
        (1 << pkTerrain->GetNumLOD() /* blocks per sector side */) + 
        pkSector->GetCellOffset(pkTerrain->GetNumLOD()));

    NiTerrainCellLeaf* pkCellLeaf = NiDynamicCast(NiTerrainCellLeaf, pkCell);
    EE_ASSERT(pkCellLeaf);
    if (!pkCellLeaf)
        return false;

    kGenInfo.m_pkCellLeaf = pkCellLeaf;

    // TODO: lweaver Can't access normals while lighting is being generated. How do we know what we 
    // can access?

    // Acquire locks to the appropriate streams
    NiTerrainStreamLocks kLocks;
    kLocks.GetPositionIterator(pkCellLeaf, NiDataStream::LOCK_READ, 
        kGenInfo.m_kPositions);
    kLocks.GetNormalIterator(pkCellLeaf, NiDataStream::LOCK_READ, 
        kGenInfo.m_kNormals);
    if (!kGenInfo.m_kPositions.Exists() || !kGenInfo.m_kNormals.Exists())
        return false;

    // The 'end' of the transforms array
    NiTransform* pkEnd = pkTransforms + uiTransformCount;

    // Transform currently being modified. Start at the beginning of the array
    NiTransform* pkTransformIter = pkTransforms;    

    // Vertex spacing is always the same as the terrains world scale, since distance is 1.0
    // in terrain local space.
    kGenInfo.m_fVertexSpacing = pkTerrain->GetWorldScale();

    // Retrieve base scale setting
    NiFloatExtraData* pkScale = (NiFloatExtraData*)kExtraData[EDK_SCALE];
    kGenInfo.m_fInstanceScale = pkScale->GetValue();

    // Retrieve scale variation setting - between 0.0 and 1.0
    NiFloatExtraData* pkScaleVariationData = (NiFloatExtraData*)kExtraData[EDK_SCALE_VARIATION];
    kGenInfo.m_fInstanceScaleVariation = NiMin(1.0f, NiMax(0.0f, 
        pkScaleVariationData->GetValue()));

    // Retrieve meta data settings from the extra data
    NiStringExtraData* pkMetaDataExtraData = (NiStringExtraData*)kExtraData[EDK_MATERIAL_META_DATA];
    const NiFixedString& kMetaDataKey = pkMetaDataExtraData->GetValue();

    // Use terrain normals?
    NiBooleanExtraData* pkTerrainNormals = (NiBooleanExtraData*)kExtraData[EDK_TERRAIN_NORMALS];
    kGenInfo.m_bUseTerrainNormals = pkTerrainNormals->GetValue();

    // Reduce the transform count to account for the randomness of instance 
    // placement. In general, the lower the value of uiTransformCount; the lower
    // the multiplier should be.
    // Reducing the transform count has no effect on the maximum number of 
    // placeable instances (it will add until it reaches pkEnd) - it simply
    // alter the probability of an instance being placed.
    NiFloatExtraData* pkQuotaMultiplier = (NiFloatExtraData*)
        kExtraData[EDK_QUOTA_MULTIPLIER];
    float fMultiplier = NiMin(1.0f, NiMax(0.0f, pkQuotaMultiplier->GetValue()));

    int uiPercentile = int((1.0f - fMultiplier) * 100.0f);
    EE_ASSERT(uiPercentile <= 100);
    int uiReservedInstances = (uiPercentile * uiTransformCount) / 100;
    uiTransformCount -= uiReservedInstances;

    if (kMetaDataKey && kMetaDataKey.GetLength() > 0)
    {
        NiFloatExtraData* pkOpacityThreshold = (NiFloatExtraData*)
            kExtraData[EDK_OPACITY_THRESHOLD];
        float fOpacityThreshold = 
            NiMin(NiMax(pkOpacityThreshold->GetValue(), 0.0f), 1.0f);

        GenerateTransformsUsingMeta(
            pkTransformIter, pkEnd, uiTransformCount, kWorldTransform, pkRandom, 
            kGenInfo, kMetaDataKey, fOpacityThreshold);
    }
    else
    {
        GenerateTransformsWithoutMeta(pkTransformIter, pkEnd, uiTransformCount,
            kWorldTransform, pkRandom, kGenInfo);
    }

    // Were any transforms added?
    if (pkTransformIter == pkTransforms)
        return false;

    // Invalidate any left over instances
    while (pkTransformIter != pkEnd)
    {
        // Since we cannot simply disable a single instance, set its scale to zero.
        pkTransformIter->m_fScale = 0.0f;
        pkTransformIter++;
    }

    return true;
}
//---------------------------------------------------------------------------
void NiTerrainDecorationFunctor::GenerateTransformsUsingMeta(
    NiTransform*& pkTransformIter, const NiTransform* pkTransformEnd, 
    NiUInt32 uiTransformCount, const NiTransform& kWorldTransform, 
    NiRandomLCG* pkRandom, const GeneratorInfo& kGenInfo, 
    const NiFixedString& kMetaDataKey, float fOpacityThreshold)
{
    EE_UNUSED_ARG(uiTransformCount);

    // Random scale to instances
    float fInstanceScaleMultiplier = kGenInfo.m_fInstanceScaleVariation * 2.0f * kGenInfo.m_fInstanceScale;
    float fInstanceScaleOffset = kGenInfo.m_fInstanceScale - kGenInfo.m_fInstanceScaleVariation * kGenInfo.m_fInstanceScale;

    NiIndex kVertBottomLeft, kVertTopRight;
    NiIndex kMaskBottomLeft, kMaskTopRight;

    NiIndex kCurrentVertSum;
    NiIndex kCurrentMaskSum;
    NiUInt32* puiActiveSumY;
    NiUInt32* puiActiveSumX;

    // Assuming that the constructor initializes the entries to zero.
    EE_ASSERT(kCurrentVertSum == NiIndex::ZERO);

    // Indices of the 'leading' vertex and mask points. [P_TOP, P_BOTTOM]
    NiIndex akVertIndex[2];
    NiIndex akMaskIndex[2];

    float afVertFactors[VOXELFACTOR_MAX];
    float afMaskFactors[VOXELFACTOR_MAX];

    float afPreviousHeight[2];
    float afCurrentHeight[2];
    NiPoint3 akPreviousNormals[2] = { NiPoint3::UNIT_Z, NiPoint3::UNIT_Z };
    NiPoint3 akCurrentNormals[2] = { NiPoint3::UNIT_Z, NiPoint3::UNIT_Z };
    float afCurrentMaskProb[2];
    float afPreviousMaskProb[2];

    // Build the meta data arrays
    NiTPrimitiveArray<NiUInt32> kMaskIndexArray;
    NiTPrimitiveArray<float> kMaskMetaValues;
    for (NiUInt32 ui = 0; ui < kGenInfo.m_pkCellLeaf->GetSurfaceCount(); ++ui)
    {
        const NiSurface* pkSurface = kGenInfo.m_pkCellLeaf->GetSurface(ui);
        NiMetaData kCurrentMeta = pkSurface->GetMetaData();

        NiMetaData::KeyType eKeyType;
        float fWeight;
        if (kCurrentMeta.HasKey(kMetaDataKey) &&
            kCurrentMeta.GetKeyType(kMetaDataKey, eKeyType))
        {
            if (eKeyType != NiMetaData::STRING)
            {

                NiInt32 iValue;
                float fValue;
                if (eKeyType == NiMetaData::INTEGER || 
                    eKeyType == NiMetaData::INTEGER_BLENDED)
                {
                    kCurrentMeta.Get(kMetaDataKey, iValue, fWeight);
                    fValue = (float)(iValue);
                }
                else
                {
                    kCurrentMeta.Get(kMetaDataKey, fValue, fWeight);
                }
                fValue = NiMin(1.0f, fValue);
                fValue = NiMax(0.0f, fValue);

                // This is not a catch all - if the value of a mask pixel 
                // is less than 255 and fValue ==  fOpacityThreshold then 
                // the probabilty will still come under the threshold.
                if (fValue >= fOpacityThreshold)
                {
                    kMaskIndexArray.Add(ui);
                    kMaskMetaValues.Add(fValue);
                }
            }
        }
    }

    // Make sure there are some surfaces that we are able to bind too
    if (kMaskIndexArray.GetSize() == 0)
        return;

    // Get the blend mask size.
    /// This value is NOT equal to the width in pixels; it is width - 1.
    /// This naming is consistent to that of the terrain, where size = width -1.
    NiUInt32 uiBlendMaskSize = 0;
    NiSourceTexture* pkBlendMask = NULL;
    const NiPixelData* pkBlendPixelData = NULL;

    const NiTextureRegion& blendMaskRegion = kGenInfo.m_pkCellLeaf->GetTextureRegion(
        NiTerrain::TextureType::BLEND_MASK);
    pkBlendMask = NiDynamicCast(NiSourceTexture, blendMaskRegion.GetTexture());

    if (pkBlendMask)
    {
        // Make sure the terrain cells texture region is square
        EE_ASSERT(blendMaskRegion.GetEndPixelIndex().x - blendMaskRegion.GetStartPixelIndex().x ==
            blendMaskRegion.GetEndPixelIndex().y - blendMaskRegion.GetStartPixelIndex().y);

        uiBlendMaskSize = 
            (blendMaskRegion.GetEndPixelIndex().x - blendMaskRegion.GetStartPixelIndex().x) - 1;

        pkBlendPixelData = pkBlendMask->GetSourcePixelData();
    }
    else
    {
        // Invalid blend mask; no surfaces on this cell. Skip all instances.
        return;
    }

    // Converts a coordinate from terrain cell region space to a memory offset from the beginning
    // of the blend mask texture. The offset will always point to the first component of a pixel.
    struct BlendMaskCoordinateConverter 
    {
        BlendMaskCoordinateConverter(const NiTextureRegion& maskRegion)
        {
            const NiPixelData* pkPixelData = 
                NiVerifyStaticCast(NiSourceTexture, maskRegion.GetTexture())->GetSourcePixelData();
            EE_ASSERT(pkPixelData->GetPixels());

            // Store some calculated values for later use
            m_startPixelIndexX = maskRegion.GetStartPixelIndex().x;
            m_endPixelIndexY = maskRegion.GetEndPixelIndex().y - 1;
            m_uiPixelSizeBytes = pkPixelData->GetPixelStride();
            m_uiRowSizeBytes = maskRegion.GetTexture()->GetWidth() * m_uiPixelSizeBytes;
        }

        inline NiUInt32 GetTexturePixelIndex(const NiIndex& kCoordinates)
        {
            return
                (m_endPixelIndexY - kCoordinates.y) * m_uiRowSizeBytes +
                (m_startPixelIndexX + kCoordinates.x) * m_uiPixelSizeBytes;

        }

        NiUInt32 m_startPixelIndexX;
        NiUInt32 m_endPixelIndexY;
        NiUInt32 m_uiRowSizeBytes;
        NiUInt32 m_uiPixelSizeBytes;

    } pixelCoordConverter = BlendMaskCoordinateConverter(blendMaskRegion);

    EE_ASSERT(uiBlendMaskSize >= 1 && kGenInfo.m_uiBlockSize > 1);
    NiIndex kWeightedSumMax(
        uiBlendMaskSize * kGenInfo.m_kTopRight.x,
        uiBlendMaskSize * kGenInfo.m_kTopRight.y);
    EE_ASSERT(kWeightedSumMax.x > 0 && kWeightedSumMax.y > 0);


    float fInvBlockSize = 1.0f / float(kGenInfo.m_uiBlockSize);
    float fInvMaskSize = 1.0f / float(uiBlendMaskSize);

    bool bForceInstance = false;

    NiPoint3 kAverageNormal;
    NiPoint3 kLookTangent;
    NiPoint3 kLookBiTangent;
    NiMatrix3 kRotMat;

    NiPoint2 kLocationOffset;

    // The instance quota dictates how many instances are allowed in the 
    // current quad.
    float fAvailQuota = 0.0f;
    float fQuotaUnit = 
        (float(uiTransformCount) * (kGenInfo.m_kCellsPerBlock.x * kGenInfo.m_kCellsPerBlock.y)) / 
        NiSqr(float(kGenInfo.m_uiBlockSize * uiBlendMaskSize));

    NiUInt32 uiLastActiveSum;
    puiActiveSumY = (&kCurrentVertSum.y);

    // Starting values, the mask may actually read from a value slightly outside
    // the cell, since we base ourselves on vertices.
    kCurrentVertSum.y = kGenInfo.m_kBottomLeft.y * uiBlendMaskSize;
    kCurrentMaskSum.y = (kCurrentVertSum.y / kGenInfo.m_uiBlockSize) * kGenInfo.m_uiBlockSize;

    // Make sure the initial 'current' values are zeroed out, so that
    // the correct initial values are demoted to 'previous' during the
    // first iteration
    afVertFactors[Y_TOP] = 0.0f;
    afMaskFactors[Y_TOP] = 0.0f;

    for (;;)
    {
        // Give a random quota to begin with on the row, to prevent 'lines' of
        // instances across the beginning of a cell
        fAvailQuota = pkRandom->GetNextUnit();

        // Make sure the initial 'current' values are zeroed out, so that
        // the correct initial values are demoted to 'previous' during the
        // first iteration
        afVertFactors[X_CURRENT] = 0.0f;
        afMaskFactors[X_CURRENT] = 0.0f;

        // Starting values, the mask may actually read from a value slightly 
        // outside the cell, since we base ourselves on vertices.
        kCurrentVertSum.x = kGenInfo.m_kBottomLeft.x * uiBlendMaskSize;
        kCurrentMaskSum.x = (kCurrentVertSum.x / kGenInfo.m_uiBlockSize) *
            kGenInfo.m_uiBlockSize;

        // Weighted increment
        uiLastActiveSum = *puiActiveSumY;
        if (kCurrentVertSum.y < kCurrentMaskSum.y)
            kCurrentVertSum.y += uiBlendMaskSize;
        else if (kCurrentVertSum.y > kCurrentMaskSum.y)
            kCurrentMaskSum.y += kGenInfo.m_uiBlockSize;
        else
        {
            // Increase by the other factor, since we are weighted sums
            kCurrentVertSum.y += uiBlendMaskSize;
            kCurrentMaskSum.y += kGenInfo.m_uiBlockSize;
        }

        // Active sum is now referring to rows
        // Get vertex and mask interpolation factors
        // Pointer to the active sum
        if (kCurrentVertSum.y < kCurrentMaskSum.y)
            puiActiveSumY = &kCurrentVertSum.y;
        else
            puiActiveSumY = &kCurrentMaskSum.y;

        // Loop condition
        if (*puiActiveSumY > kWeightedSumMax.y)
        {
            break;
        }

        // Work out the initial index values
        akVertIndex[P_TOP].y = kCurrentVertSum.y / uiBlendMaskSize;
        akVertIndex[P_BOTTOM].y = (kCurrentVertSum.y - uiBlendMaskSize) / 
            uiBlendMaskSize;
        akVertIndex[P_TOP].x = kCurrentVertSum.x / uiBlendMaskSize;
        akVertIndex[P_BOTTOM].x = akVertIndex[P_TOP].x;

        akMaskIndex[P_TOP].y = kCurrentMaskSum.y / kGenInfo.m_uiBlockSize;
        akMaskIndex[P_BOTTOM].y = (kCurrentMaskSum.y - kGenInfo.m_uiBlockSize) / 
            kGenInfo.m_uiBlockSize;
        akMaskIndex[P_TOP].x = kCurrentMaskSum.x / kGenInfo.m_uiBlockSize;
        akMaskIndex[P_BOTTOM].x = akMaskIndex[P_TOP].x;

        // Get 'previous' values, and put them in the afCurrentHeight 
        // (because they will be immediately swapped to previous).
        afCurrentHeight[P_TOP] = kGenInfo.m_pkCellLeaf->GetHeightAt(
            kGenInfo.m_kPositions, akVertIndex[P_TOP]);
        afCurrentHeight[P_BOTTOM] = kGenInfo.m_pkCellLeaf->GetHeightAt(
            kGenInfo.m_kPositions, akVertIndex[P_BOTTOM]);
        if (kGenInfo.m_bUseTerrainNormals)
        {
            kGenInfo.m_pkCellLeaf->GetNormalAt(kGenInfo.m_kNormals, 
                akCurrentNormals[P_TOP], akVertIndex[P_TOP]);
            kGenInfo.m_pkCellLeaf->GetNormalAt(kGenInfo.m_kNormals, 
                akCurrentNormals[P_BOTTOM], akVertIndex[P_BOTTOM]);
        }

        // Get initial mask values
        afCurrentMaskProb[P_TOP] = GetProbabilityAt(pkBlendPixelData,
            pixelCoordConverter.GetTexturePixelIndex(akMaskIndex[P_TOP]), 
            kMaskIndexArray, kMaskMetaValues);
        akMaskIndex[P_BOTTOM].x = kGenInfo.m_kBottomLeft.x;
        afCurrentMaskProb[P_BOTTOM] = GetProbabilityAt(pkBlendPixelData,
            pixelCoordConverter.GetTexturePixelIndex(akMaskIndex[P_BOTTOM]), 
            kMaskIndexArray, kMaskMetaValues);

        // Nominate that we need to set previous = current, then get updated
        // 'current' value.
        bool bFetchVert = true;
        bool bFetchMask = true;

        // Demote vertex and mask interpolation factors
        afVertFactors[Y_BOTTOM] = afVertFactors[Y_TOP];
        afMaskFactors[Y_BOTTOM] = afMaskFactors[Y_TOP];

        afVertFactors[Y_TOP] = fInvMaskSize * float(
            (uiBlendMaskSize - (kCurrentVertSum.y - *puiActiveSumY)) % 
            uiBlendMaskSize); 
        afMaskFactors[Y_TOP] = fInvBlockSize * float(
            (kGenInfo.m_uiBlockSize - (kCurrentMaskSum.y - *puiActiveSumY)) % 
            kGenInfo.m_uiBlockSize);

        kLocationOffset.y = kGenInfo.m_kWorldCellCorner.y + 
            float(akVertIndex[P_BOTTOM].y) * kGenInfo.m_fVertexSpacing;

        // Create instances for this row
        float fQuotaMultiplier = float(*puiActiveSumY - uiLastActiveSum);

        puiActiveSumX = &kCurrentVertSum.x;
        for (;;)
        {
            uiLastActiveSum = *puiActiveSumX;

            // Weighted increment
            if (kCurrentVertSum.x < kCurrentMaskSum.x)
                kCurrentVertSum.x += uiBlendMaskSize;
            else if (kCurrentVertSum.x > kCurrentMaskSum.x)
                kCurrentMaskSum.x += kGenInfo.m_uiBlockSize;
            else
            {
                // Increase by the other factor, since we are weighted sums
                kCurrentVertSum.x += uiBlendMaskSize;
                kCurrentMaskSum.x += kGenInfo.m_uiBlockSize;
            }

            if (kCurrentVertSum.x < kCurrentMaskSum.x)
                puiActiveSumX = &kCurrentVertSum.x;
            else
                puiActiveSumX = &kCurrentMaskSum.x;

            // Loop condition
            if (*puiActiveSumX > kWeightedSumMax.x)
            {
                break;
            }

            // We have entered a new quad, add 1 quads worth to the quota
            NiUInt32 uiDiff = *puiActiveSumX - uiLastActiveSum;
            fAvailQuota += float(uiDiff) * fQuotaMultiplier * fQuotaUnit;

            // Is there not enough quota to add an instance?
            if (fAvailQuota < 1.0f)
            {
                // Even though there isn't enough quota, see if we can add an 
                // instance anyway, to give a more random appearance
                // The penalty of course, is that the quota will actually go
                // negative once this instance is dealt with.

                if (fAvailQuota > 0.0f &&
                    pkRandom->GetNextUnit() < fQuotaUnit * fQuotaMultiplier)
                {
                    bForceInstance = true;
                }
            }

            // Translate the weighted sum into index values we can read.
            // We use a trick here, and store the previous value in bottom
            // over the span of the 'if' statement, to avoid a division.
            akVertIndex[P_TOP].x = kCurrentVertSum.x / uiBlendMaskSize;

            // Do we need to query the terrain for heights and mask data?
            if (akVertIndex[P_BOTTOM].x != akVertIndex[P_TOP].x)
            {
                bFetchVert = true;
            }
            akVertIndex[P_BOTTOM].x = akVertIndex[P_TOP].x;

            // We need the 'trailing' x index, not the leading so subtract 1.
            kLocationOffset.x = kGenInfo.m_kWorldCellCorner.x + 
                float(akVertIndex[P_BOTTOM].x - 1) * kGenInfo.m_fVertexSpacing;

            akMaskIndex[P_TOP].x = kCurrentMaskSum.x / kGenInfo.m_uiBlockSize;
            if (akMaskIndex[P_BOTTOM].x != akMaskIndex[P_TOP].x)
            {
                bFetchMask = true;
            }
            akMaskIndex[P_BOTTOM].x = akMaskIndex[P_TOP].x;

            // Get Heights and normals
            if (bFetchVert) 
            {
                // Demote values backwards
                afPreviousHeight[P_TOP] = afCurrentHeight[P_TOP];
                afPreviousHeight[P_BOTTOM] = afCurrentHeight[P_BOTTOM];
                akPreviousNormals[P_TOP] = akCurrentNormals[P_TOP];
                akPreviousNormals[P_BOTTOM] = akCurrentNormals[P_BOTTOM];

                // Get new 'current' values
                afCurrentHeight[P_TOP] = kGenInfo.m_pkCellLeaf->GetHeightAt(
                    kGenInfo.m_kPositions, akVertIndex[P_TOP]);
                afCurrentHeight[P_BOTTOM] = kGenInfo.m_pkCellLeaf->GetHeightAt(
                    kGenInfo.m_kPositions, akVertIndex[P_BOTTOM]);
                if (kGenInfo.m_bUseTerrainNormals)
                {
                    kGenInfo.m_pkCellLeaf->GetNormalAt(kGenInfo.m_kNormals, 
                        akCurrentNormals[P_TOP], akVertIndex[P_TOP]);
                    kGenInfo.m_pkCellLeaf->GetNormalAt(kGenInfo.m_kNormals, 
                        akCurrentNormals[P_BOTTOM], akVertIndex[P_BOTTOM]);
                }

                bFetchVert = false;
            }

            // Get mask values
            if (bFetchMask)
            {
                // Demote values backwards
                afPreviousMaskProb[P_TOP] = afCurrentMaskProb[P_TOP];
                afPreviousMaskProb[P_BOTTOM] = afCurrentMaskProb[P_BOTTOM];

                // Get new 'current' values
                afCurrentMaskProb[P_TOP] = GetProbabilityAt(pkBlendPixelData,
                    pixelCoordConverter.GetTexturePixelIndex(akMaskIndex[P_TOP]),
                    kMaskIndexArray, kMaskMetaValues);

                afCurrentMaskProb[P_BOTTOM] = GetProbabilityAt(pkBlendPixelData,
                    pixelCoordConverter.GetTexturePixelIndex(akMaskIndex[P_BOTTOM]),
                    kMaskIndexArray, kMaskMetaValues);

                bFetchMask = false;
            }

            // Demote vertex and mask interpolation factors
            afVertFactors[X_PREVIOUS] = afVertFactors[X_CURRENT];
            afMaskFactors[X_PREVIOUS] = afMaskFactors[X_CURRENT];

            afVertFactors[X_CURRENT] = fInvMaskSize * float(
                (uiBlendMaskSize - (kCurrentVertSum.x - *puiActiveSumX)) 
                % uiBlendMaskSize); 
            afMaskFactors[X_CURRENT] = fInvBlockSize * float(
                (kGenInfo.m_uiBlockSize - (kCurrentMaskSum.x - *puiActiveSumX))
                % kGenInfo.m_uiBlockSize); 

            // Get the normal. We dont interpolate normal values, to save CPU.
            kAverageNormal = (
                akPreviousNormals[P_BOTTOM] +
                akCurrentNormals[P_BOTTOM] +
                akPreviousNormals[P_TOP] +
                akCurrentNormals[P_TOP]);

            // Vertical normal?
            if (kAverageNormal.z == 4.0f)
            {
                kRotMat = NiMatrix3::IDENTITY;
            }
            else
            {
                kAverageNormal.Unitize();
                kLookTangent = kAverageNormal.Cross(NiPoint3::UNIT_Z);
                kLookTangent.Unitize();
                kLookBiTangent = kLookTangent.Cross(kAverageNormal);
                kRotMat = NiMatrix3(
                    kLookBiTangent, kLookTangent, kAverageNormal);
            }

            while (fAvailQuota >= 1.0f || bForceInstance)
            {
                // Note! All calls to random come before early exits due to
                // surface coverage probability, to retain position consistency
                // when instances become visible due to surface painting.
                bForceInstance = false;

                // Have we added as many transforms as we can?
                if (pkTransformIter == pkTransformEnd)
                    return;

                // Deduct from the available quota
                fAvailQuota -= 1.0f;

                // TODO: Use our mask values, with the factors, to get weighted
                // values. Bilerp?
                NiPoint2 kRandomInterp(pkRandom->GetNextUnit(), 
                    pkRandom->GetNextUnit());
                NiPoint2 kVortexPosition;

                // According to the surface mask, how probable is it that we 
                // will want to be drawn?
                float fProbability;
                FactoredBiLerp(kRandomInterp, afMaskFactors, 
                    afPreviousMaskProb, afCurrentMaskProb, fProbability,
                    kVortexPosition);

                // Is the instance invisible?
                if (fProbability < pkRandom->GetNextUnit() ||
                    fProbability < fOpacityThreshold)
                {
                    continue;
                }

                // Find our position via BiLerp.
                FactoredBiLerp(kRandomInterp, afVertFactors,
                    afPreviousHeight, afCurrentHeight, 
                    pkTransformIter->m_Translate.z, kVortexPosition);

                // Set the instance position
                pkTransformIter->m_Translate.x = kLocationOffset.x + 
                    kVortexPosition.x * kGenInfo.m_fVertexSpacing;
                pkTransformIter->m_Translate.y = kLocationOffset.y + 
                    kVortexPosition.y * kGenInfo.m_fVertexSpacing;
                pkTransformIter->m_Translate.z *= kGenInfo.m_fVertexSpacing;

                // Give random scale to all instances.
                pkTransformIter->m_fScale = fInstanceScaleOffset +
                    pkRandom->GetNextUnit() * fInstanceScaleMultiplier;

                // Expanded matrix multiplication of:
                // kRotMat X MakeZRotation(NI_TWO_PI * pkRandom->GetNextUnit())
                float sn, cs, nsn;
                NiSinCos(NI_TWO_PI * pkRandom->GetNextUnit(), sn, cs);
                nsn = -1.0f * sn;

                pkTransformIter->m_Rotate.SetEntry(0, 0, 
                    kRotMat.GetEntry(0, 0) * cs + 
                    kRotMat.GetEntry(0, 1) * nsn);
                pkTransformIter->m_Rotate.SetEntry(1, 0,  
                    kRotMat.GetEntry(1, 0) * cs + 
                    kRotMat.GetEntry(1, 1) * nsn);
                pkTransformIter->m_Rotate.SetEntry(2, 0,  
                    kRotMat.GetEntry(2, 0) * cs + 
                    kRotMat.GetEntry(2, 1) * nsn);

                pkTransformIter->m_Rotate.SetEntry(0, 1, 
                    kRotMat.GetEntry(0, 0) * sn + 
                    kRotMat.GetEntry(0, 1) * cs);
                pkTransformIter->m_Rotate.SetEntry(1, 1, 
                    kRotMat.GetEntry(1, 0) * sn + 
                    kRotMat.GetEntry(1, 1) * cs);
                pkTransformIter->m_Rotate.SetEntry(2, 1, 
                    kRotMat.GetEntry(2, 0) * sn + 
                    kRotMat.GetEntry(2, 1) * cs);

                pkTransformIter->m_Rotate.SetEntry(0, 2, 
                    kRotMat.GetEntry(0, 2));
                pkTransformIter->m_Rotate.SetEntry(1, 2, 
                    kRotMat.GetEntry(1, 2));
                pkTransformIter->m_Rotate.SetEntry(2, 2, 
                    kRotMat.GetEntry(2, 2));

                *pkTransformIter = kWorldTransform * (*pkTransformIter);

                // Go to the next transform
                pkTransformIter++;
            }
        }
    }
}
//---------------------------------------------------------------------------
void NiTerrainDecorationFunctor::GenerateTransformsWithoutMeta(
    NiTransform*& pkTransformIter, const NiTransform* pkTransformEnd, 
    NiUInt32 uiTransformCount, const NiTransform& kWorldTransform, 
    NiRandomLCG* pkRandom, const GeneratorInfo& kGenInfo)
{    
    // Random scale to instances
    float fInstanceScaleMultiplier = kGenInfo.m_fInstanceScaleVariation * 2.0f * kGenInfo.m_fInstanceScale;
    float fInstanceScaleOffset = kGenInfo.m_fInstanceScale - kGenInfo.m_fInstanceScaleVariation * kGenInfo.m_fInstanceScale;

    // Assuming that the constructor initializes the entries to zero.
    NiIndex kCurrentVertSum;
    EE_ASSERT(kCurrentVertSum == NiIndex::ZERO);

    // Indices of the 'leading' vertex. [P_TOP, P_BOTTOM]
    NiIndex akVertIndex[2];

    // When there is no blend mask, vertex factors should always be 0
    const float afVertFactors[] = {0.0f, 0.0f, 0.0f, 0.0f};

    // Value (height and normal) for current and previous points
    float afPreviousHeight[2];
    float afCurrentHeight[2];
    NiPoint3 akPreviousNormals[2] = { NiPoint3::UNIT_Z, NiPoint3::UNIT_Z };
    NiPoint3 akCurrentNormals[2] = { NiPoint3::UNIT_Z, NiPoint3::UNIT_Z };

    bool bForceInstance = false;
    NiPoint3 kAverageNormal;
    NiPoint3 kLookTangent;
    NiPoint3 kLookBiTangent;
    NiMatrix3 kRotMat;

    NiPoint2 kLocationOffset;

    // The instance quota dictates how many instances are allowed in the 
    // current quad.
    float fAvailQuota = 0.0f;
    float fQuotaUnit = float(uiTransformCount) / 
        float(kGenInfo.m_kSizePerCell.x * kGenInfo.m_kSizePerCell.y);

    // Starting values, the mask may actually read from a value slightly outside
    // the cell, since we base ourselves on vertices.
    kCurrentVertSum.y = kGenInfo.m_kBottomLeft.y;

    for (;;)
    {
        // Give a random quota to begin with on the row, to prevent 'lines' of
        // instances across the beginning of a cell
        fAvailQuota = pkRandom->GetNextUnit();

        // set up starting values
        kCurrentVertSum.x = kGenInfo.m_kBottomLeft.x;

        // Increment the vertex sum
        kCurrentVertSum.y++;

        // Loop condition
        if (kCurrentVertSum.y > kGenInfo.m_kTopRight.y)
        {
            break;
        }

        // Work out the initial index values
        akVertIndex[P_TOP].y = kCurrentVertSum.y;
        akVertIndex[P_BOTTOM].y = kCurrentVertSum.y - 1;
        akVertIndex[P_TOP].x = kCurrentVertSum.x;
        akVertIndex[P_BOTTOM].x = akVertIndex[P_TOP].x;

        // Get 'previous' values, and put them in the afCurrentHeight 
        // (because they will be immediately swapped to previous).
        afCurrentHeight[P_TOP] = kGenInfo.m_pkCellLeaf->GetHeightAt(
            kGenInfo.m_kPositions, akVertIndex[P_TOP]);
        afCurrentHeight[P_BOTTOM] = kGenInfo.m_pkCellLeaf->GetHeightAt(
            kGenInfo.m_kPositions, akVertIndex[P_BOTTOM]);
        if (kGenInfo.m_bUseTerrainNormals)
        {
            kGenInfo.m_pkCellLeaf->GetNormalAt(kGenInfo.m_kNormals, 
                akCurrentNormals[P_TOP], akVertIndex[P_TOP]);
            kGenInfo.m_pkCellLeaf->GetNormalAt(kGenInfo.m_kNormals, 
                akCurrentNormals[P_BOTTOM], akVertIndex[P_BOTTOM]);
        }

        // Work out the y location offset
        kLocationOffset.y = kGenInfo.m_kWorldCellCorner.y + 
            float(akVertIndex[P_BOTTOM].y) * kGenInfo.m_fVertexSpacing;

        for (;;)
        {
            // Increment vertex sum
            kCurrentVertSum.x++;

            // Loop condition
            if (kCurrentVertSum.x > kGenInfo.m_kTopRight.x)
            {
                break;
            }

            // We have entered a new quad, add 1 quads worth to the quota
            fAvailQuota += fQuotaUnit;

            // Is there not enough quota to add an instance?
            if (fAvailQuota < 1.0f)
            {
                // Even though there isn't enough quota, see if we can add an 
                // instance anyway, to give a more random appearance
                // The penalty of course, is that the quota will actually go
                // negative once this instance is dealt with.
                if (fAvailQuota > 0.0f)
                    bForceInstance = pkRandom->GetNextUnit() < fQuotaUnit;
            }

            // Update the vertex index
            akVertIndex[P_TOP].x = kCurrentVertSum.x;
            akVertIndex[P_BOTTOM].x = akVertIndex[P_TOP].x;

            // We need the 'trailing' x index, not the leading so subtract 1.
            kLocationOffset.x = kGenInfo.m_kWorldCellCorner.x + 
                float(akVertIndex[P_BOTTOM].x - 1) * kGenInfo.m_fVertexSpacing;

            // Get Heights and normals
            // Demote values backwards
            afPreviousHeight[P_TOP] = afCurrentHeight[P_TOP];
            afPreviousHeight[P_BOTTOM] = afCurrentHeight[P_BOTTOM];
            akPreviousNormals[P_TOP] = akCurrentNormals[P_TOP];
            akPreviousNormals[P_BOTTOM] = akCurrentNormals[P_BOTTOM];

            // Get new 'current' values
            afCurrentHeight[P_TOP] = kGenInfo.m_pkCellLeaf->GetHeightAt(
                kGenInfo.m_kPositions, akVertIndex[P_TOP]);
            afCurrentHeight[P_BOTTOM] = kGenInfo.m_pkCellLeaf->GetHeightAt(
                kGenInfo.m_kPositions, akVertIndex[P_BOTTOM]);   
            if (kGenInfo.m_bUseTerrainNormals)
            {
                kGenInfo.m_pkCellLeaf->GetNormalAt(kGenInfo.m_kNormals, 
                    akCurrentNormals[P_TOP], akVertIndex[P_TOP]);
                kGenInfo.m_pkCellLeaf->GetNormalAt(kGenInfo.m_kNormals, 
                    akCurrentNormals[P_BOTTOM], akVertIndex[P_BOTTOM]);
            }

            // Get the normal. We dont interpolate normal values, to save CPU.
            kAverageNormal = (
                akPreviousNormals[P_BOTTOM] +
                akCurrentNormals[P_BOTTOM] +
                akPreviousNormals[P_TOP] +
                akCurrentNormals[P_TOP]);

            // Calculate the rotation matrix from the normal
            if (kAverageNormal.z == 4.0f)
            {
                // Normal is vertical?
                kRotMat = NiMatrix3::IDENTITY;
            }
            else
            {
                kAverageNormal.Unitize();
                kLookTangent = kAverageNormal.Cross(NiPoint3::UNIT_Z);
                kLookTangent.Unitize();
                kLookBiTangent = kLookTangent.Cross(kAverageNormal);
                kRotMat = NiMatrix3(
                    kLookBiTangent, kLookTangent, kAverageNormal);
            }

            while (fAvailQuota >= 1.0f || bForceInstance)
            {
                bForceInstance = false;

                // Have we added as many transforms as we can?
                if (pkTransformIter == pkTransformEnd)
                    return;

                // Deduct from the available quota
                fAvailQuota -= 1.0f;

                // Get Random interpolaters
                NiPoint2 kRandomInterp(pkRandom->GetNextUnit(), 
                    pkRandom->GetNextUnit());
                NiPoint2 kVortexPosition;                

                // Find our position via BiLerp.
                FactoredBiLerp(kRandomInterp, afVertFactors,
                    afPreviousHeight, afCurrentHeight, 
                    pkTransformIter->m_Translate.z, kVortexPosition);

                // Modify position according to location offset
                pkTransformIter->m_Translate.x = kLocationOffset.x + 
                    kVortexPosition.x * kGenInfo.m_fVertexSpacing;
                pkTransformIter->m_Translate.y = kLocationOffset.y + 
                    kVortexPosition.y * kGenInfo.m_fVertexSpacing;
                pkTransformIter->m_Translate.z *= kGenInfo.m_fVertexSpacing;

                // Give random scale to all instances.
                pkTransformIter->m_fScale = fInstanceScaleOffset +
                    pkRandom->GetNextUnit() * fInstanceScaleMultiplier;

                // Expanded matrix multiplication of:
                // kRotMat X MakeZRotation(NI_TWO_PI * pkRandom->GetNextUnit())
                float sn, cs, nsn;
                NiSinCos(NI_TWO_PI * pkRandom->GetNextUnit(), sn, cs);
                nsn = -1.0f * sn;

                pkTransformIter->m_Rotate.SetEntry(0, 0, 
                    kRotMat.GetEntry(0, 0) * cs + 
                    kRotMat.GetEntry(0, 1) * nsn);
                pkTransformIter->m_Rotate.SetEntry(1, 0,  
                    kRotMat.GetEntry(1, 0) * cs + 
                    kRotMat.GetEntry(1, 1) * nsn);
                pkTransformIter->m_Rotate.SetEntry(2, 0,  
                    kRotMat.GetEntry(2, 0) * cs + 
                    kRotMat.GetEntry(2, 1) * nsn);

                pkTransformIter->m_Rotate.SetEntry(0, 1, 
                    kRotMat.GetEntry(0, 0) * sn + 
                    kRotMat.GetEntry(0, 1) * cs);
                pkTransformIter->m_Rotate.SetEntry(1, 1, 
                    kRotMat.GetEntry(1, 0) * sn + 
                    kRotMat.GetEntry(1, 1) * cs);
                pkTransformIter->m_Rotate.SetEntry(2, 1, 
                    kRotMat.GetEntry(2, 0) * sn + 
                    kRotMat.GetEntry(2, 1) * cs);

                pkTransformIter->m_Rotate.SetEntry(0, 2, 
                    kRotMat.GetEntry(0, 2));
                pkTransformIter->m_Rotate.SetEntry(1, 2, 
                    kRotMat.GetEntry(1, 2));
                pkTransformIter->m_Rotate.SetEntry(2, 2, 
                    kRotMat.GetEntry(2, 2));

                *pkTransformIter = kWorldTransform * (*pkTransformIter);

                // Go to the next transform
                pkTransformIter++;
            }
        }
    }

    return;
}
//---------------------------------------------------------------------------
float NiTerrainDecorationFunctor::GetProbabilityAt(
    const NiPixelData*& pkPixelData, const NiUInt32& uiPixelIndex,
    const NiTPrimitiveArray<NiUInt32>& kMaskIndexArray,
    const NiTPrimitiveArray<float>& kMaskMetaValues)
{    
    //float fBestValue = 0;
    float fSum = 0.0f;
    for (NiUInt32 ui = 0; ui < kMaskIndexArray.GetSize(); ++ui)
    {
        // Get the pixel value
        // NiUInt32 uiComponent = kMaskIndexArray[ui];
        EE_ASSERT(kMaskIndexArray[ui] < pkPixelData->GetPixelFormat().GetNumComponents());

        fSum += (float)pkPixelData->GetPixels()[uiPixelIndex + kMaskIndexArray[ui]] * 
            kMaskMetaValues[ui];
    }

    const float fInvDivisor = 1.0f / 255.0f;

    return fSum * fInvDivisor;
}
//---------------------------------------------------------------------------
