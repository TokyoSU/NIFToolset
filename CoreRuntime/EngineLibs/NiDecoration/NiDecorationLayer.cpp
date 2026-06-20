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
#include "NiDecorationLayer.h"
#include "NiDecorationCell.h"

#include <NiInstancingUtilities.h>
#include <NiMesh.h>
#include <NiPropertyState.h>
#include <NiTexturingProperty.h>

#include "NiDecorationField.h"
#include "NiDecorationLayerTransformProcessor.h"

NiImplementRTTI(NiDecorationLayer, NiNode, NiTypeMask::NiDecorationLayer);

//------------------------------------------------------------------------------------------------
NiDecorationLayer::NiDecorationLayer(NiDecorationGenerator* pkGenerator) :
    m_kCellSpacing(NiPoint3::ZERO),
    m_kCellLocationOffset(NiPoint3::ZERO),
    m_kDimensions(NiPoint2::ZERO),
    m_fFarFadeDistance(0.0f),
	m_fNearFadeDistance(0.0f),
    m_fMinRange(0.0f),
    m_fMaxRange(0.0f),
    m_fMinRangeClamped(0.0f),
    m_fMinRangeClampedSqr(0.0f),
    m_fMaxRangeClamped(0.0f),
    m_fMaxRangeClampedSqr(0.0f),
    m_spGenerator(pkGenerator),
    m_spCamera(0),
    m_spBaseMesh(0),
    m_uiBaseSeed(0),
    m_uiMaxCellGenerationsPerCall(UINT_MAX),
    m_bCameraRedefined(false)
{
    m_auiNumCells[0] = 0;
    m_auiNumCells[1] = 0;
    m_kLastCameraTransform.MakeIdentity();
}

//------------------------------------------------------------------------------------------------
NiDecorationLayer::~NiDecorationLayer()
{
    RemoveFunctorsFromMesh();

    // Use the helper function to unset the base mesh to ensure that things are cleaned up properly
    SetBaseMesh(NULL);
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::Initialize(NiUInt32 uiNumCellsX, NiUInt32 uiNumCellsY,
    NiPoint2 kLayerWidth, float fMaxRange, float fMinRange,
	float fFarFadeDistance, float fNearFadeDistance)
{
    fFarFadeDistance = NiClamp(fFarFadeDistance, 0.0f, fMaxRange);
    fNearFadeDistance = NiClamp(fNearFadeDistance, 0.0f, fMaxRange - fFarFadeDistance);

    EE_ASSERT(kLayerWidth.x > 0.0f && kLayerWidth.y > 0.0f);
    EE_ASSERT(uiNumCellsX > 0 && uiNumCellsY > 0);

    /* Calculated values */
    m_auiNumCells[0] = uiNumCellsX;
    m_auiNumCells[1] = uiNumCellsY;
    m_kDimensions = kLayerWidth;
    m_kCellSpacing = NiPoint3(m_kDimensions.x / (float)uiNumCellsX,
        m_kDimensions.y / (float)uiNumCellsY, 0.0f);

    m_fMinRange = fMinRange;
    m_fFarFadeDistance = fFarFadeDistance;
    m_fNearFadeDistance = fNearFadeDistance;
    m_fMaxRange = fMaxRange;

    float fCellRadius = NiMax(m_kCellSpacing.x, m_kCellSpacing.y) * 0.5f * NiSqrt(2.0f);

    // Don't let the minimum distance become less than zero.
    m_fMinRangeClamped = NiMax(fMinRange - fCellRadius, 0.0f);
    m_fMinRangeClamped = 0.0f;
    m_fMinRangeClampedSqr = m_fMinRangeClamped * m_fMinRangeClamped;

    // Don't let the maximum distance become smaller than (radius of a cell bound + maxDistance)
    // for the purposes of in-range calculations
    m_fMaxRangeClamped = NiMax(fMaxRange, fMaxRange + fCellRadius);
    m_fMaxRangeClampedSqr = m_fMaxRangeClamped * m_fMaxRangeClamped;

    // Simplified: m_kDimensions * -0.5f + m_kCellSpacing * 0.5f
    m_kCellLocationOffset = NiPoint3(0.5f * (m_kCellSpacing.x - m_kDimensions.x),
        0.5f * (m_kCellSpacing.y - m_kDimensions.y), 0.0f);

    if (m_spGenerator)
    {
        NiDecorationTransformManager* pkTransformManager = m_spGenerator->GetTransformManager();

        // Create the base mesh
        NiUInt32 uiNumCells = uiNumCellsX * uiNumCellsY;
        if (uiNumCells != 0 && pkTransformManager->GetInstancesPerCell() != 0)
        {
            NiDecorationMeshInfo* pkMeshInfo = m_spGenerator->CreateBaseMesh(
                pkTransformManager->GetNumMaxCells(),
                pkTransformManager->GetInstancesPerCell());

            if (pkMeshInfo != NULL)
                m_spGenerator->UpdatePropertyData(pkMeshInfo);

            SetBaseMesh(pkMeshInfo);

            UpdateProperties();
            UpdateEffects();
        }
        else 
        {
            SetBaseMesh(NULL);
        }
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::SetBaseMesh(NiDecorationMeshInfo* pkBaseMesh)
{
    if (m_spBaseMesh != NULL)
    {
        DetachChild(m_spBaseMesh->GetMesh());

        NIASSERT(m_spGenerator != NULL);
        if (m_spGenerator->GetTransformManager())
        {
            ResetCells();

            // Process the cell free requests
            m_spGenerator->GetTransformManager()->ProcessRequests();

            m_spGenerator->GetTransformManager()->ReleaseInstanceRegion(m_spBaseMesh);
        }

        // Give the functor an opportunity to de-initialize the mesh
        RemoveFunctorsFromMesh();
    }

    m_spBaseMesh = pkBaseMesh;

    if (m_spBaseMesh != NULL)
    {
        AttachChild(m_spBaseMesh->GetMesh());

        NIASSERT(m_spGenerator != NULL);
        if (m_spGenerator->GetTransformManager())
        {
            m_spGenerator->GetTransformManager()->CreateInstanceRegion(
                m_spBaseMesh,
                NiNew NiDecorationLayerTransformProcessor(this));
        }

        // Give the functor an opportunity to initialize the mesh
        ApplyFunctorsToMesh();
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::ResetCells()
{
    // Force all cells to be recalculated
    m_bCameraRedefined = true;

    // Make sure our inverse transform is correct
    GetWorldTransform().Invert(m_kWorldTransformInv);

    // Make sure we have been initialized, so that we have a valid instance
    // pool ID before trying to remove cells
    if (GetNumCellsX() == 0 || GetNumCellsY() == 0)
        return;

    if (m_spGenerator && m_spBaseMesh)
    {
        m_spGenerator->GetTransformManager()->ClearRegion(m_spBaseMesh);
    }

    m_kVisibleCells.clear();
    m_pendingTransformCells.clear();
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{
    // Make sure our meshes bound is up to date
    UpdateWorldBound();

    NiNode::UpdateDownwardPass(kUpdate);
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{
    // Make sure our meshes bound is up to date
    UpdateWorldBound();

    NiNode::UpdateSelectedDownwardPass(kUpdate);
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    // Make sure our meshes bound is up to date
    UpdateWorldBound();

    NiNode::UpdateRigidDownwardPass(kUpdate);
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::UpdateWorldBound()
{
    NiCamera* pkCamera = GetCamera();
    NiBound kEstimateBound;

    if (pkCamera == NULL)
    {
        kEstimateBound.SetCenter(GetWorldTranslate());
        kEstimateBound.SetRadius(0.0f);
    }
    else
    {
        // Calculate an estimate bound, since the size of all cells will always
        // be the same.
        NiPoint3 kLocalCamera = (m_kWorldTransformInv * pkCamera->GetWorldTransform()).m_Translate;

        NiPoint3 kLocalTranslate(NiClamp(kLocalCamera.x, -m_kDimensions.x, m_kDimensions.x),
            NiClamp(kLocalCamera.y, -m_kDimensions.y, m_kDimensions.y), 0.0f);

        kEstimateBound.SetCenter(/*GetWorldTransform() * */kLocalTranslate);
        kEstimateBound.SetRadius(m_fMaxRange /* * GetWorldTransform().m_fScale*/);
    }

    if (m_spBaseMesh)
    {
        NiMesh* pkMesh = m_spBaseMesh->GetMesh();
        if (pkMesh != NULL)
            pkMesh->SetModelBound(kEstimateBound);

        // When in instanced mesh mode, the world bound isn't updated automatically from the
        // model space mesh, so we need to do it ourselves
        kEstimateBound.SetCenter(GetWorldTransform() * kEstimateBound.GetCenter());
        kEstimateBound.SetRadius(GetWorldScale() * kEstimateBound.GetRadius());
        pkMesh->SetWorldBound(kEstimateBound);
    }
    else if (GetChildCount() == 0)
        SetWorldBound(kEstimateBound);
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::UpdateShaderConstants()
{
    NiDecorationMeshInfo* pkMeshInfo= GetBaseMesh();

    if (pkMeshInfo != NULL && m_spGenerator != NULL)
        GetGenerator()->UpdateShaderConstants(GetBaseMesh(), this);
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::ApplyFunctorsToMesh()
{
    if (m_spBaseMesh != NULL)
    {
        // Get the field index
        NiUInt32 uiFieldIndex = 0;
        NiDecorationField* pkParent = NiDynamicCast(NiDecorationField, GetParent());
        if (pkParent)
            uiFieldIndex = pkParent->GetFieldIndex();

        NiPoint2 kCellRange = CalculateCellRange();

        // Configure the functors
        for (FunctorList::iterator iter = m_kFunctors.begin(); iter != m_kFunctors.end(); iter++)
        {
            NiDecorationFunctorBase* pkFunctor = *iter;
            pkFunctor->ConfigureMesh(m_auiNumCells, kCellRange, uiFieldIndex, GetWorldTransform(),
                m_spBaseMesh->GetMesh());
        }
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::RemoveFunctorsFromMesh()
{
    if (m_spBaseMesh != NULL)
    {
        // Get the field index
        NiUInt32 uiFieldIndex = 0;
        NiDecorationField* pkParent = NiDynamicCast(NiDecorationField, GetParent());
        if (pkParent)
            uiFieldIndex = pkParent->GetFieldIndex();

        NiPoint2 kCellRange = CalculateCellRange();

        // Configure the functors
        for (FunctorList::iterator iter = m_kFunctors.begin(); iter != m_kFunctors.end(); iter++)
        {
            // TODO: Do this in a function on the functor
            /*
            NiDecorationFunctorBase* pkFunctor = *iter;
            pkFunctor->DisconfigureMesh(m_auiNumCells, kCellRange, uiFieldIndex, GetWorldTransform(),
                m_spBaseMesh);
                */

            NiAVObject* pkEffectObject = GetObjectByName("Color map effect");

            NiTextureEffect* pkTextureEffect = NiDynamicCast(NiTextureEffect, pkEffectObject);
            DetachEffect(pkTextureEffect);
            DetachChild(pkEffectObject);
            UpdateEffects();
        }
    }

}

//------------------------------------------------------------------------------------------------
NiPoint2 NiDecorationLayer::CalculateCellRange() const
{
    return NiPoint2(
        GetDimensions().x / float(GetNumCellsX()),
        GetDimensions().y / float(GetNumCellsY()));
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::ReleaseInvisibleCells()
{
    if (!m_spBaseMesh)
        return;

    NiCamera* pkCamera = GetCamera();
    if (!pkCamera)
        return;

    // General local variables 
    NiPoint3 kLocation;
    float fDistSqr;
    
    NiDecorationTransformManager* pkTransformManager = GetGenerator()->GetTransformManager();
    if (!pkTransformManager)
        return;
    NiDecorationTransformManager::RegionID kPoolID = m_spBaseMesh;
    
    if ((m_kLastCameraTransform.m_Translate != pkCamera->GetWorldTransform().m_Translate) ||
        m_bCameraRedefined)
    {
        NiTransform kLocalCameraTransform = WorldToLocal(pkCamera->GetWorldTransform());

        // 2D distance from cells, since cells are mapped to 2d grid ignoring z
        kLocalCameraTransform.m_Translate.z = 0.0f;

        NiDecorationCell* pkCell;
        for (CellList::iterator cellIter = m_kVisibleCells.begin();
            cellIter != m_kVisibleCells.end();
            cellIter++)
        {
            pkCell = *cellIter;
            fDistSqr = (pkCell->m_kLocalTransform.m_Translate - kLocalCameraTransform.m_Translate).
                SqrLength();

            if (fDistSqr >= m_fMaxRangeClampedSqr || fDistSqr <= m_fMinRangeClampedSqr)
            {
                // Out of range, free it
                EE_ASSERT(pkCell->m_pkRegionID == kPoolID);
                pkTransformManager->ReleaseCell(kPoolID, pkCell);
                cellIter = m_kVisibleCells.erase(cellIter);
            }
        }

        // Cancel any pending cells that were left over from previous frames that are now too far 
        // away
        for (UIntPairList::iterator pendingCellIter = m_pendingTransformCells.begin();
            pendingCellIter != m_pendingTransformCells.end();
            pendingCellIter++)
        {
            UIntPair& kIndex = *pendingCellIter;

            // Are we still in camera range?
            kLocation = NiPoint3(m_kCellLocationOffset.x + m_kCellSpacing.x * (float)kIndex.m_x,
                m_kCellLocationOffset.y + m_kCellSpacing.y * (float)kIndex.m_y, 0.0f);

            fDistSqr = (kLocalCameraTransform.m_Translate - kLocation).SqrLength();
            
            if (fDistSqr < m_fMaxRangeClampedSqr && fDistSqr > m_fMinRangeClampedSqr)
                continue;

            // No, we must pop it!
            pendingCellIter = m_pendingTransformCells.erase(pendingCellIter);
        }
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationLayer::CreateVisibleCells()
{
    if (!m_spBaseMesh)
        return;

    NiCamera* pkCamera = GetCamera();
    if (!pkCamera)
        return;

    // General local variables 
    NiPoint3 kLocation;
    float fDistSqr;

    NiDecorationTransformManager* pkTransformManager = GetGenerator()->GetTransformManager();
    if (!pkTransformManager)
        return;
    NiDecorationTransformManager::RegionID kPoolID = m_spBaseMesh;
    
    if ((m_kLastCameraTransform.m_Translate != pkCamera->GetWorldTransform().m_Translate) ||
        m_bCameraRedefined)
    {
        NiTransform kLocalCameraTransform = WorldToLocal(pkCamera->GetWorldTransform());

        // 2D distance from cells, since cells are mapped to 2d grid ignoring z
        kLocalCameraTransform.m_Translate.z = 0.0f;

        // Potential cell locations
        // TODO: Take into account a min distance - avoid checking large number of
        //       cell positions in the middle of the range radius that are in the
        //       'min' distance range, when min is finally supported.
        NiPoint3 kShiftedPos = kLocalCameraTransform.m_Translate + NiPoint3(GetDimensions().x * 0.5f,
            GetDimensions().y * 0.5f, 0.0f);

        NiPoint3 kCellStartPosition = NiPoint3((kShiftedPos.x - m_fMaxRangeClamped) / m_kCellSpacing.x,
            (kShiftedPos.y - m_fMaxRangeClamped) / m_kCellSpacing.y, 0.0f);
        NiPoint3 kCellEndPosition = NiPoint3((kShiftedPos.x + m_fMaxRangeClamped) / m_kCellSpacing.x,
            (kShiftedPos.y + m_fMaxRangeClamped) / m_kCellSpacing.y, 0.0f);

        // It is the intention of the following lines to 'floor' the start and
        // end positions to form 2D array indices.
        NiUInt32 uiMinX = NiUInt32(NiClamp(kCellStartPosition.x, 0.0f, (float)GetNumCellsX() - 1.0f));
        NiUInt32 uiMinY = NiUInt32(NiClamp(kCellStartPosition.y, 0.0f, (float)GetNumCellsY() - 1.0f));
        NiUInt32 uiMaxX = NiUInt32(NiClamp(kCellEndPosition.x, 0.0f, (float)GetNumCellsX() - 1.0f));
        NiUInt32 uiMaxY = NiUInt32(NiClamp(kCellEndPosition.y, 0.0f, (float)GetNumCellsY() - 1.0f));

        // Find a list of cells we want to create
        for (NiUInt32 uiY = uiMinY; uiY <= uiMaxY; ++uiY)
        {
            for (NiUInt32 uiX = uiMinX; uiX <= uiMaxX; ++uiX)
            {
                kLocation = NiPoint3(m_kCellLocationOffset.x + m_kCellSpacing.x * (float)uiX,
                    m_kCellLocationOffset.y + m_kCellSpacing.y * (float)uiY, 0.0f);

                fDistSqr = (kLocalCameraTransform.m_Translate - kLocation).SqrLength();

                if (fDistSqr < m_fMaxRangeClampedSqr && fDistSqr > m_fMinRangeClampedSqr)
                {
                    NiPoint3 kDiff = (m_kLastLocalCameraTransform.m_Translate - kLocation);
                    fDistSqr = kDiff.SqrLength();

                    if (fDistSqr >= m_fMaxRangeClampedSqr || 
                        fDistSqr <= m_fMinRangeClampedSqr ||
                        m_bCameraRedefined)
                    {
                        // This was out of range last update, so it is not present
                        // in the m_kCells array.

                        UIntPair kIndexPair;
                        kIndexPair.m_x = uiX;
                        kIndexPair.m_y = uiY;
                        m_pendingTransformCells.push_back(kIndexPair);
                    }
                }
            }
        }

        // Store the current camera position for use next frame
        m_kLastCameraTransform = pkCamera->GetWorldTransform();
        m_kLastLocalCameraTransform = kLocalCameraTransform;
        m_bCameraRedefined = false;
    }

    // Create as many cells as we can per frame, storing the rest for later
    NiInt32 uiRemainingCellQuota = m_uiMaxCellGenerationsPerCall;

    for (UIntPairList::iterator newCellIter = m_pendingTransformCells.begin();
         newCellIter != m_pendingTransformCells.end();
         newCellIter++)
    {
        EE_ASSERT (uiRemainingCellQuota != 0);
        UIntPair& kIndex = *newCellIter;

        // Request the creation of this cell.
        pkTransformManager->RequestCells(kPoolID, kIndex.m_x, kIndex.m_y);

        newCellIter = m_pendingTransformCells.erase(newCellIter);

        // Have we done enough this frame?
        if (--uiRemainingCellQuota == 0)
            break;
    }
}

//------------------------------------------------------------------------------------------------
CellRequestGenerationResult NiDecorationLayer::HandleCellRequestResponse(
    NiDecorationCell* pkCell, 
    NiTransform* pkTransformStream,
    NiUInt32 uiTransformCount,
    bool bWriteInvalidTransformOnFail)
{
    // This arg is only used in an assertion
    EE_UNUSED_ARG(uiTransformCount);

    // Create the cells local transform
    pkCell->m_kLocalTransform.m_Rotate.MakeIdentity();
    pkCell->m_kLocalTransform.m_fScale = 1.0f;
    pkCell->m_kLocalTransform.m_Translate = NiPoint3(
        m_kCellLocationOffset.x + m_kCellSpacing.x * (float)pkCell->m_uiIndexX,
        m_kCellLocationOffset.y + m_kCellSpacing.y * (float)pkCell->m_uiIndexY,
        0.0f);

    // Cells world transform
    pkCell->m_kWorldTransform = pkCell->m_kLocalTransform * GetWorldTransform();

    // Generation properties
    NiUInt32 uiSeed = GetBaseSeed() + 
        pkCell->m_uiIndexX + 
        (pkCell->m_uiIndexY * GetNumCellsX());
    NiPoint2 kRange = CalculateCellRange();
    NiUInt32 uiInstancesPerCell = GetGenerator()->GetTransformManager()->GetInstancesPerCell();
    EE_ASSERT(uiInstancesPerCell <= uiTransformCount);

    // Get the field index
    NiUInt32 uiFieldIndex = 0;
    NiDecorationField* pkParent = NiDynamicCast(NiDecorationField, GetParent());
    if (pkParent)
        uiFieldIndex = pkParent->GetFieldIndex();

    // Calculate instance transforms
    bool bValidTransforms = m_spGenerator->GenerateTransforms(
        GetFunctorList(), 
        m_auiNumCells,
        pkCell,
        pkTransformStream,
        uiInstancesPerCell,
        uiSeed,
        kRange,
        uiFieldIndex,
        GetWorldTransform());

    if (bValidTransforms)
    {
        EE_ASSERT(pkCell->m_pkRegionID == m_spBaseMesh);
        m_kVisibleCells.push_back(pkCell);

        return GR_GENERATED_VALID_TRANSFORMS;
    }
    else if (bWriteInvalidTransformOnFail)
    {
        NiUInt32 uiGenerationCount = m_spGenerator->GenerateInvalidTransforms(pkTransformStream, 
            uiTransformCount);

        if (uiGenerationCount == 0)
            return GR_EXISTING_INVALID_TRANSFORMS;
        else
            return GR_GENERATED_INVALID_TRANSFORMS;
    }
    else
    {
        return GR_EXISTING_UNKNOWN_TRANSFORMS;
    }
}
//------------------------------------------------------------------------------------------------
