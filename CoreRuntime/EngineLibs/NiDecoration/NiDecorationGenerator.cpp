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

#include "NiDecorationPCH.h"
#include "NiDecorationGenerator.h"
#include "NiDecorationMaterial.h"
#include "NiDecorationLayer.h"

#include <NiRenderer.h>

//------------------------------------------------------------------------------------------------

NiImplementRootRTTI(NiDecorationGenerator);
NiFixedString NiDecorationGenerator::DEFAULT_TEXTURING_PROPERTY_NAME = NULL;

//------------------------------------------------------------------------------------------------
NiDecorationGenerator::NiDecorationGenerator(bool bUseInstancing) :
    m_spTransformStreamManager(NULL),
    m_spNoiseTexture(NULL),
    m_pkRandom(NULL),
    m_fAnimationLoopTime(0.0f),
    m_fAnimationOffsetTime(0.0f),
    m_fBaseTextureSaturation(1.0f),
    m_bUseInstancing(bUseInstancing),
    m_bFallbackTransformsEnabled(true)
{
    // If called from the SDM::_Init chain, the renderer does not yet exist.
    if (!NiRenderer::GetRenderer())
        m_bUseInstancing = false;

    if (m_bUseInstancing && !(NiRenderer::GetRenderer()->GetFlags() & 
        NiRenderer::CAPS_HARDWAREINSTANCING) )
    {        
        NiOutputDebugString("The current hardware does not support hardware "
            "instancing. Use of hardware instancing will be disabled.");

        m_bUseInstancing = false;
    }

    m_pkRandom = NiNew NiRandomLCG(0);
}

//------------------------------------------------------------------------------------------------
NiDecorationGenerator::~NiDecorationGenerator()
{
    m_spNoiseTexture = NULL;
    NiDelete(m_pkRandom);
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::_SDMInit()
{
    DEFAULT_TEXTURING_PROPERTY_NAME = "__DECO_TEXURING_PROPERTY";
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::_SDMShutdown()
{
    DEFAULT_TEXTURING_PROPERTY_NAME = NULL;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationGenerator::GetUseInstancing() const
{
    return m_bUseInstancing;
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::SetUseInstancing(bool bUseInstancing)
{
    m_bUseInstancing = bUseInstancing;
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::UpdateShaderConstants(NiDecorationMeshInfo* pkBase, 
    const NiDecorationLayer* pkLayer)
{
    NiMesh* pkBaseMesh = pkBase->GetMesh();
    NiFloatExtraData* pkFloatData;

    float fMinRange = pkLayer->GetMinRange();
    float fMaxRange = pkLayer->GetMaxRange();
    float fFarFadeDistance = pkLayer->GetFarFadeDistance();
    float fNearFadeDistance = pkLayer->GetNearFadeDistance();
    float fWorldScale = pkLayer->GetWorldScale();

    // Calculate the fade radii, make sure the border cells cant pop in/out
    float fFarFadeStart = fWorldScale * (fMaxRange - fFarFadeDistance);
    float fFarInvisible = fWorldScale * fMaxRange;
    float fNearInvisible = 0.0f;
    float fNearFadeStart = 0.0f;

    if (fMinRange > 0.0f || fNearFadeDistance > 0.0f)
    {
        fNearInvisible = fWorldScale * fMinRange;
        fNearFadeStart = fWorldScale * (fMinRange + fNearFadeDistance);
    }

    // Outer Start fade value
    pkFloatData = (NiFloatExtraData*)pkBaseMesh->GetExtraData(
        NiDecorationMaterial::FADE_OUTERMINDISTSQR_SHADER_CONSTANT);
    if (!pkFloatData)
    {
        pkFloatData = NiNew NiFloatExtraData(NiSqr(fFarFadeStart));
        pkBaseMesh->AddExtraData(NiDecorationMaterial::FADE_OUTERMINDISTSQR_SHADER_CONSTANT, 
            pkFloatData);
    }
    else
    {
        pkFloatData->SetValue(NiSqr(fFarFadeStart));
    }

    // Outer End fade value
    pkFloatData = (NiFloatExtraData*)pkBaseMesh->GetExtraData(
        NiDecorationMaterial::FADE_OUTERMAXDISTSQR_SHADER_CONSTANT);
    if (!pkFloatData)
    {
        pkFloatData = NiNew NiFloatExtraData(NiSqr(fFarInvisible));
        pkBaseMesh->AddExtraData(NiDecorationMaterial::FADE_OUTERMAXDISTSQR_SHADER_CONSTANT,
			pkFloatData);
    }
    else
    {
        pkFloatData->SetValue(NiSqr(fFarInvisible));
    }

	// Inner Start fade value
	pkFloatData = (NiFloatExtraData*)pkBaseMesh->GetExtraData(
		NiDecorationMaterial::FADE_INNERMINDISTSQR_SHADER_CONSTANT);
	if (!pkFloatData)
	{
		pkFloatData = NiNew NiFloatExtraData(NiSqr(fNearInvisible));
		pkBaseMesh->AddExtraData(NiDecorationMaterial::FADE_INNERMINDISTSQR_SHADER_CONSTANT,
			pkFloatData);
	}
	else
	{
		pkFloatData->SetValue(NiSqr(fNearInvisible));
	}

	// Inner End fade value
	pkFloatData = (NiFloatExtraData*)pkBaseMesh->GetExtraData(
		NiDecorationMaterial::FADE_INNERMAXDISTSQR_SHADER_CONSTANT);
	if (!pkFloatData)
	{
		pkFloatData = NiNew NiFloatExtraData(NiSqr(fNearFadeStart));
		pkBaseMesh->AddExtraData(NiDecorationMaterial::FADE_INNERMAXDISTSQR_SHADER_CONSTANT,
			pkFloatData);
	}
	else
	{
		pkFloatData->SetValue(NiSqr(fNearFadeStart));
	}

    // Diffuse Over-saturation
    pkFloatData = (NiFloatExtraData*)pkBaseMesh->GetExtraData(
        NiDecorationMaterial::DIFFUSE_SATURATION_MULTIPLIER_SHADER_CONSTANT);
    if (!pkFloatData)
    {
        pkFloatData = NiNew NiFloatExtraData(GetBaseTextureSaturation());
        pkBaseMesh->AddExtraData(
            NiDecorationMaterial::DIFFUSE_SATURATION_MULTIPLIER_SHADER_CONSTANT,
            pkFloatData);
    }
    else
    {
        pkFloatData->SetValue(GetBaseTextureSaturation());
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::UpdatePropertyData(NiDecorationMeshInfo* pkBase)
{
    NiMesh* pkBaseMesh = pkBase->GetMesh();
    EE_ASSERT(pkBaseMesh);

    NiTexturingProperty* pkTexturingProperty = 
        (NiTexturingProperty*)pkBaseMesh->GetProperty(NiProperty::TEXTURING);

    // No texturing property created?
    if (!pkTexturingProperty)
        return;

    // Has the user created their own texturing property?
    if (pkTexturingProperty->GetName() != DEFAULT_TEXTURING_PROPERTY_NAME)
        return;

    if (m_spNoiseTexture == NULL)
    {
        // Need to create a noise texture for the screen door fading
        m_spNoiseTexture = NiNoiseTexture::Create(NiNoiseTexture::NT_RAND, 8, 0);
    }

    m_spNoiseTexture->SetUseMipmapping(true);

    NiTexturingProperty::ShaderMap* pkMap = 
        NiNew NiTexturingProperty::ShaderMap(m_spNoiseTexture, 0, 
        NiTexturingProperty::WRAP_S_WRAP_T, 
        NiTexturingProperty::FILTER_TRILERP);

    pkTexturingProperty->SetShaderMap(0, pkMap);
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::SetAnimationLoopTime(float fTime)
{
    m_fAnimationLoopTime = fTime;
}

//------------------------------------------------------------------------------------------------
float NiDecorationGenerator::GetAnimationLoopTime() const
{
    return m_fAnimationLoopTime;
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::SetAnimationOffsetTime(float fTime)
{
    m_fAnimationOffsetTime = fTime;
}

//------------------------------------------------------------------------------------------------
float NiDecorationGenerator::GetAnimationOffsetTime() const
{
    return m_fAnimationOffsetTime;
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::SetBaseTextureSaturation(float fSaturation)
{
    m_fBaseTextureSaturation = fSaturation;
}

//------------------------------------------------------------------------------------------------
float NiDecorationGenerator::GetBaseTextureSaturation() const
{
    return m_fBaseTextureSaturation;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationGenerator::GenerateTransforms(
    const FunctorList& kFunctorSet,
    NiUInt32 auiCellCount[2], NiDecorationCell* pkCell, NiTransform* pkTransforms, 
    NiUInt32 uiTransformCount, NiUInt32 uiSeed, const NiPoint2& kRange, NiUInt32 uiFieldIndex,
    const NiTransform& kWorldTransform)
{
    NiTransform kTransform = kWorldTransform;
    
    // When not instancing, the world transform is already applied to the local space points
    if (!m_bUseInstancing)
        kTransform.MakeIdentity();
    
    if (kFunctorSet.size())
    {
        bool bFunctorSucceeded = false;

        for (FunctorList::const_iterator iter = kFunctorSet.begin();
            iter != kFunctorSet.end();
            iter++)
        {
            NiDecorationFunctorBase* pkFunctor = *iter;

            // Reset the random seed
            m_pkRandom->SetSeed(uiSeed);

            bFunctorSucceeded |= pkFunctor->GenerateTransforms(
                auiCellCount, pkCell, pkTransforms, uiTransformCount, kRange, uiFieldIndex, 
                kWorldTransform, m_pkRandom);
        }

        return bFunctorSucceeded;
    }
    else if (GetFallbackTransformsEnabled())
    {
        return GenerateTransformsSimple(pkCell, pkTransforms, uiTransformCount, uiSeed, kRange, 
            kTransform);
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool NiDecorationGenerator::GenerateTransformsSimple(NiDecorationCell* pkCell, 
    NiTransform* pkTransforms, NiUInt32 uiTransformCount, NiUInt32 uiSeed, 
    const NiPoint2& kRange, const NiTransform& kWorldTransform)
{
    // Reset the random seed
    m_pkRandom->SetSeed(uiSeed);

    NiPoint2 kFactor = kRange * 0.5f;
    NiTransform* pkTransform = 0;
    for (NiUInt32 ui = 0; ui < uiTransformCount; ++ui)
    {
        pkTransform = &pkTransforms[ui];
        pkTransform->m_Translate = NiPoint3(
            m_pkRandom->GetNextSymmetricUnit() * kFactor.x,
            m_pkRandom->GetNextSymmetricUnit() * kFactor.y,
            0.0f) + pkCell->m_kLocalTransform.m_Translate;
        pkTransform->m_fScale = 1.0f + m_pkRandom->GetNextSymmetricUnit() * 0.2f;
        pkTransform->m_Rotate.MakeZRotation(m_pkRandom->GetNextUnit() * NI_TWO_PI);

        *pkTransform = kWorldTransform * (*pkTransform);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
NiDecorationTransformManager* NiDecorationGenerator::GetTransformManager() const
{
    return m_spTransformStreamManager;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationGenerator::GetFallbackTransformsEnabled() const
{
    return m_bFallbackTransformsEnabled;
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::SetFallbackTransformsEnabled(bool bFallbacksEnabled)
{
    m_bFallbackTransformsEnabled = bFallbacksEnabled;
}

//------------------------------------------------------------------------------------------------
NiUInt32 NiDecorationGenerator::GenerateInvalidTransforms(NiTransform* pkTransforms, 
    NiUInt32 uiTransformCount)
{
    // Is this stream already invalidated?
    if (uiTransformCount == 0 || pkTransforms[0].m_fScale == 0.0f)
        return 0;

    for (NiUInt32 ui = 0; ui < uiTransformCount; ++ui)
    {
        pkTransforms[ui].m_fScale = 0.0f;
    }

    return uiTransformCount;
}

//------------------------------------------------------------------------------------------------
void NiDecorationGenerator::ProcessChangedCells()
{
    NiDecorationTransformManager* pkManager = m_spTransformStreamManager;
    NiUInt32 uiInstancesPerCell = pkManager->GetInstancesPerCell();

    // Process the cell release and creation requests
    pkManager->ProcessRequests();

    // Update instance counts and stream offsets
    const NiDecorationTransformManager::RegionIDList& kPools = pkManager->GetInstanceRegions();

    for (NiDecorationTransformManager::RegionIDList::const_iterator poolIter = kPools.begin();
        poolIter != kPools.end();
        poolIter++)
    {
        NiDecorationMeshInfo* pkMeshInfo = *poolIter;
        NiUInt32 uiCount = pkManager->GetVisibleCellCount(pkMeshInfo) * uiInstancesPerCell;

        SetActiveInstanceCount(pkMeshInfo, uiCount);
    }

    // Upload changed cells
    const NiDecorationTransformManager::CellSet& kChangedCells = pkManager->GetChangedCells();

    NiTransform* pkTransformStream = pkManager->LockStream(NiDataStream::LOCK_READ);

    for (NiDecorationTransformManager::CellSet::const_iterator cellIter = kChangedCells.begin();
        cellIter != kChangedCells.end();
        cellIter++)
    {
        NiDecorationCell* pkCell = *cellIter;
        NiDecorationMeshInfo* pkBase = pkCell->m_pkRegionID;

        // The cell may have been released since the last upload step.
        if (pkBase == NULL)
            continue;

        // Work out the cell offset for the cells owning instance pool
        NiUInt32 uiFirstCellIndex;
        if (!pkManager->GetFirstCellIndex(pkCell->m_pkRegionID, uiFirstCellIndex))
        {
            // The pool in question has no visible cells, nothing to upload.
            continue;
        }

        // Work out the offset, in instances, from the beginning of the stream to this cell
        NiUInt32 uiOffset = (pkCell->m_uiIndexInRegion + uiFirstCellIndex);
        EE_ASSERT(uiOffset < pkManager->GetNumMaxCells());

        SetTransforms(pkBase, 
            pkTransformStream + uiOffset * uiInstancesPerCell, 
            1, 
            uiOffset);
    }

    pkManager->MarkAllCellsUploaded();

    pkManager->Unlock(NiDataStream::LOCK_READ);
}

//------------------------------------------------------------------------------------------------
