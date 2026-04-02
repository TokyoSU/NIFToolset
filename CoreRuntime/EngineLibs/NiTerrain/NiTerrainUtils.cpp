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

#include <NiAVObject.h>
#include <NiCollisionUtils.h>
#include <NiFilename.h>
#include <NiSourceTexture.h>

#ifdef WIN32
#include <NiDX9Renderer.h>
#include <NiDX9Defines.h>
#include <NiDX9SourceTextureData.h>
#endif

#include <efd/Logger.h>
#include <efd/ecrLogIDs.h>

#include "NiTerrainUtils.h"
#include "NiTerrainCell.h"
#include "NiTerrainCellNode.h"
#include "NiTerrainCellLeaf.h"
#include "NiTerrainSector.h"
#include "NiTerrainStreamLocks.h"
#include "NiTerrain.h"

namespace NiTerrainUtils
{

//------------------------------------------------------------------------------------------------
bool TestRay2D(NiRay& kRay, NiTerrainCell* pkDataRoot)
{
    if (!pkDataRoot)
        return false;

    bool bCompareDistance = kRay.UseLength();
    NiTerrainSector* pkSector = pkDataRoot->GetContainingSector();
    NiTerrain* pkTerrain = pkSector->GetTerrain();
    NiTransform kTerrainTransform = pkSector->GetSectorData()->GetWorldTransform();

    // Work out the region over which this cell lives
    NiRect<efd::SInt32> kWorldSpaceRegion;

    NiIndex kBottomLeft;
    pkDataRoot->GetBottomLeftIndex(kBottomLeft);

    efd::UInt32 uiSpacing = 1 << pkDataRoot->GetNumSubDivisions();
    efd::UInt32 uiBlockWidth = pkDataRoot->GetCellSize();
    efd::UInt32 uiBlockWidthInVerts = pkDataRoot->GetWidthInVerts();
    efd::UInt32 uiCellSize = uiBlockWidth * (uiSpacing);
    efd::UInt32 uiSectorSize = pkTerrain->GetCalcSectorSize() - 1;    
    efd::SInt16 sSectorX, sSectorY;
    pkSector->GetSectorIndex(sSectorX, sSectorY);

    kWorldSpaceRegion.m_left = kBottomLeft.x + (efd::SInt32)sSectorX * uiSectorSize;
    kWorldSpaceRegion.m_bottom = kBottomLeft.y + (efd::SInt32)sSectorY * uiSectorSize;
    kWorldSpaceRegion.m_right = kWorldSpaceRegion.m_left + uiCellSize;
    kWorldSpaceRegion.m_top = kWorldSpaceRegion.m_bottom + uiCellSize;

    // retrieve the cell's heightmap
    HeightMapBuffer* pkHeightMap = EE_NEW HeightMapBuffer();
    pkSector->GetTerrain()->GetHeightMap(kWorldSpaceRegion, pkHeightMap);

    // Convert the ray's origin to cell index
    NiPoint3 kOrigin = kRay.GetOrigin();    
    NiPoint3 kLeafSpaceOrigin;
    float fHalfSectorSize = uiSectorSize / 2.0f;
    kLeafSpaceOrigin.x = efd::Floor(kOrigin.x) + fHalfSectorSize - kBottomLeft.x;
    kLeafSpaceOrigin.y = efd::Floor(kOrigin.y) + fHalfSectorSize - kBottomLeft.y;
    kLeafSpaceOrigin.x /= efd::Float32(uiSpacing);
    kLeafSpaceOrigin.y /= efd::Float32(uiSpacing);

    // If this is not the case then the given cell is not the one we are colliding with.
    if (kLeafSpaceOrigin.x < 0 || kLeafSpaceOrigin.x > uiBlockWidth ||
        kLeafSpaceOrigin.y < 0 || kLeafSpaceOrigin.y > uiBlockWidth)
    {
        EE_FAIL("NiTerrainUtils::Test2D MUST be given the cell the ray is colliding with!");
        return false;
    }

    NiIndex kMinPointIndex(NiUInt32(efd::Floor(kLeafSpaceOrigin.x)), 
        NiUInt32(efd::Floor(kLeafSpaceOrigin.y)));
    NiIndex kMaxPointIndex(NiUInt32(efd::Floor(kLeafSpaceOrigin.x + 1)), 
        NiUInt32(efd::Floor(kLeafSpaceOrigin.y + 1)));

    // Resolve the sample's tesselation
    NiUInt8 ucMask = NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ;
    NiTerrainStreamLocks kLocks;
    bool bTesselation = false;
    NiTStridedRandomAccessIterator<NiUInt16> kOriIndIter16;
    NiTStridedRandomAccessIterator<NiUInt32> kOriIndIter32;
    bool bUseIndex16 = false;

    if (pkSector->GetUsingShortIndexBuffer())
    {
        kLocks.GetIndexIterator(pkDataRoot, ucMask, kOriIndIter16);
        bUseIndex16 = true;
    }
    else
    {
        kLocks.GetIndexIterator(pkDataRoot, ucMask, kOriIndIter32);
    }

    NiUInt32 uiIndexValue[3] = {};
    NiUInt32 uiIndStreamIndex = (kMinPointIndex.y * uiBlockWidth + kMinPointIndex.x) * 6;
    if (bUseIndex16)
    {
        uiIndexValue[0] = kOriIndIter16[uiIndStreamIndex];
        uiIndexValue[1] = kOriIndIter16[uiIndStreamIndex + 1];
        uiIndexValue[2] = kOriIndIter16[uiIndStreamIndex + 2];
    }
    else
    {
        uiIndexValue[0] = kOriIndIter32[uiIndStreamIndex];
        uiIndexValue[1] = kOriIndIter32[uiIndStreamIndex + 1];
        uiIndexValue[2] = kOriIndIter32[uiIndStreamIndex + 2];
    }
    
    // case for tesselation = 
    //   _      _
    // |\ | or | /|
    // |_\|    |/_| 
    // true    false
    //
    // with index 0 being bottom right;
    if (uiIndexValue[0] + uiBlockWidthInVerts == uiIndexValue[2])
        bTesselation = true;
    else
        bTesselation = false;

    // We can now get the triangles and resolve the collision point and normal.
    efd::UInt16* pusData = pkHeightMap->GetBuffer();
    efd::UInt32 uiBufferIndex = (kMinPointIndex.x * uiSpacing) + 
        pkHeightMap->GetWidth() * (kMinPointIndex.y * uiSpacing);

    // Retrieve all points values should be in terrain space: 
    // k2 . . k3
    // k0 . . k1
    float fMaxElevation = pkTerrain->GetMaxHeight();
    float fMinElevation = pkTerrain->GetMinHeight();
    float fModifier = float((fMaxElevation - fMinElevation) / float(NiUInt16(-1)));

    // Convert the heightmap value into a valid height
    float fMinX = (float)((kMinPointIndex.x * uiSpacing) + kBottomLeft.x - fHalfSectorSize);
    float fMaxX = (float)((kMaxPointIndex.x * uiSpacing) + kBottomLeft.x - fHalfSectorSize);
    float fMinY = (float)((kMinPointIndex.y * uiSpacing) + kBottomLeft.y - fHalfSectorSize);
    float fMaxY = (float)((kMaxPointIndex.y * uiSpacing) + kBottomLeft.y - fHalfSectorSize);
    NiPoint3 k0 = NiPoint3(fMinX, fMinY, 
        pusData[uiBufferIndex] * fModifier + fMinElevation);
    NiPoint3 k1 = NiPoint3(fMaxX, fMinY, 
        pusData[uiBufferIndex + uiSpacing]* fModifier + fMinElevation);
    NiPoint3 k2 = NiPoint3(fMinX, fMaxY,
        pusData[uiBufferIndex + (pkHeightMap->GetWidth() * uiSpacing)] * fModifier + fMinElevation);
    NiPoint3 k3 = NiPoint3(fMaxX, fMaxY,
        pusData[uiBufferIndex + (pkHeightMap->GetWidth() * uiSpacing) + uiSpacing] * 
            fModifier + fMinElevation);

    EE_DELETE pkHeightMap;

    if (!bTesselation)
    {
        // Rotate the points
        NiPoint3 kTemp = k0;
        k0 = k2;
        k2 = k3;
        k3 = k1;
        k1 = kTemp;
    }
        
    /*
     * TRIANGLE 1: 012
    */
    NiPoint3 kNormal0 = (k1 - k0).UnitCross(k2 - k0);
    float fRayDotNormal = kRay.GetDirection().Dot(kNormal0);

    float fDistFromPlane;
    NiPoint3 kCollisionPoint;
    // Triangle facing right direction?
    if (fRayDotNormal < 0.0f)
    {
        // Workout the collisionpoint
        NiPoint3 kNewRay(
            kOrigin.x,
            kOrigin.y,
            NiMax(NiMax((k0).z, (k1).z), (k2).z) + 1.0f);

        fDistFromPlane = -1.0f * ((kNewRay - k0).Dot(kNormal0) / fRayDotNormal);
        kCollisionPoint = kNewRay + (fDistFromPlane * kRay.GetDirection());
                
        // Now see if point is on same side as all 3 points
        if ((k1 - k0).Cross
            (kCollisionPoint - k0).Dot(kNormal0) >= - 1e-05f &&
            (k2 - k1).Cross
            (kCollisionPoint - k1).Dot(kNormal0) >= - 1e-05f &&
            (k0 - k2).Cross
            (kCollisionPoint - k2).Dot(kNormal0) >= - 1e-05f)
        {
            if ((kCollisionPoint.z - kOrigin.z) * kRay.GetDirection().z < 0)
                return false;

            NiPoint3 kDistance = kOrigin - kCollisionPoint;
            if (!bCompareDistance || kDistance.Length() < (kRay.GetLength()))
            {
                kNormal0 = kTerrainTransform.m_Rotate.Inverse() * kNormal0;
                kNormal0.Unitize();
                kCollisionPoint = kTerrainTransform * kCollisionPoint;
                kRay.UseClosestIntersection(kCollisionPoint, kNormal0);
                kRay.SetCollidedLeaf(pkDataRoot);

                return true;
            }
        }
    }

    /*
     * TRIANGLE 2: 132
    */
    kNormal0 = (k3 - k1).UnitCross(k2 - k1);
    fRayDotNormal = kRay.GetDirection().Dot(kNormal0);

    // Triangle facing right direction?
    if (fRayDotNormal < 0.0f)
    {
        // Workout the collisionpoint
        NiPoint3 kNewRay(
            kOrigin.x,
            kOrigin.y,
            NiMax(NiMax((k3).z, (k1).z), (k2).z) + 1.0f);

        fDistFromPlane = -1.0f * ((kNewRay - k1).Dot(kNormal0) / fRayDotNormal);

        kCollisionPoint = kNewRay + (fDistFromPlane * kRay.GetDirection());
    

        // Now see if point is on same side as all 3 points
        if ((k3 - k1).Cross
            (kCollisionPoint - k1).Dot(kNormal0) >= - 1e-05f &&
            (k2 - k3).Cross
            (kCollisionPoint - k3).Dot(kNormal0) >= - 1e-05f &&
            (k1 - k2).Cross
            (kCollisionPoint - k2).Dot(kNormal0) >= - 1e-05f)
        {
            if ((kCollisionPoint.z - kOrigin.z) * kRay.GetDirection().z < 0)
                return false;

            NiPoint3 kDistance = kOrigin - kCollisionPoint;
            if (!bCompareDistance || kDistance.Length() < (kRay.GetLength()))
            {
                kNormal0 = kTerrainTransform.m_Rotate.Inverse() * kNormal0;
                kNormal0.Unitize();
                kCollisionPoint = kTerrainTransform * kCollisionPoint;
                kRay.UseClosestIntersection(kCollisionPoint, kNormal0);
                kRay.SetCollidedLeaf(pkDataRoot);
                return true;
            }
        }
    }

    // We get here in the cases where a triangle is facing in the same general direction as the ray.
    // This means we are attempting to collide with a triangle's back face.
    return false;
}

//------------------------------------------------------------------------------------------------
bool TestRay(NiRay& kRay, NiTerrainCell* pkDataRoot, 
    NiUInt32 uiDetailLOD)
{
    bool bIntersection = false;

    NiPoint3 kOrigin = kRay.GetOrigin();
    NiTransform kTerrainTransform =
        pkDataRoot->GetContainingSector()->GetSectorData()
        ->GetWorldTransform();

    if (!pkDataRoot)
    {
        return false;
    }

    NiInt32 iHighestLoD = pkDataRoot->GetContainingSector()->
        GetSectorData()->GetHighestLoadedLOD();
    if (iHighestLoD < 0)
        return false;
    
    if ((NiInt32)uiDetailLOD > iHighestLoD)
    {
        uiDetailLOD = NiUInt32(iHighestLoD);
    }

    NiUInt8 ucMask = NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ;
    NiTerrainStreamLocks kLocks;
    NiTerrainPositionRandomAccessIterator kPositionIter;
    NiTerrainNormalRandomAccessIterator kNormalIter;
    kLocks.GetPositionIterator(pkDataRoot, ucMask, 
        kPositionIter);
    kLocks.GetNormalIterator(pkDataRoot, ucMask, 
        kNormalIter);
    
    NiTerrainCellNode* pkRootNode = 
        NiDynamicCast(NiTerrainCellNode, pkDataRoot);
    // Collide with any of the data leaf's children?
    // Using optimization assumption: we will only ever have 0 or 4 children.
    if (pkRootNode && pkRootNode->GetChildAt(0) && 
        pkRootNode->GetChildAt(0)->GetLevel() <= uiDetailLOD) 
    {
        float afChildDistances[4] = {};
        float fCurDistance;
        NiTerrainCell *pkCurChild = NULL;

        // Figure out each child's distance from the RAY origin:
        for (int i = 0; i < 4; ++i)
        {
            pkCurChild = pkRootNode->GetChildAt((NiUInt16)i);
            afChildDistances[i] = (
                kOrigin - pkCurChild->GetLocalBound().GetCenter()).SqrLength();
        }

        // Loop through all the children in order to find the intersector:
        for (int iIteration = 0; iIteration < 4; ++iIteration)
        {
            // Find the closest child:
            fCurDistance = NI_INFINITY;
            NiUInt32 uiChosenIndex = 4;
            for (NiUInt32 ui = 0; ui<4; ++ui)
            {
                if (afChildDistances[ui] < fCurDistance)
                {
                    fCurDistance = afChildDistances[ui];
                    pkCurChild = pkRootNode->GetChildAt(ui);

                    uiChosenIndex = ui;
                }
            }

            if (uiChosenIndex == 4)
            {
                // Invalid bound!
                continue;
            }

            // Remove this child's distance from the array:
            afChildDistances[uiChosenIndex] = NI_INFINITY;

            const NiBound* pkBound = &pkCurChild->GetLocalBound();

            if (kRay.PassesThroughBound(*pkBound))
            {
                /*
                 * Passes through the bounding sphere, but what about the box?
                */

                if (!NiBoxBV::BoxTriTestIntersect(
                    0.0f, pkCurChild->GetLocalBoxBound(), NiPoint3::ZERO,
                        kOrigin, kOrigin,
                        kOrigin + (kRay.GetDirection() * 10000.0f),
                        NiPoint3::ZERO))
                {
                    continue;
                }

                if (TestRay(kRay, pkCurChild, uiDetailLOD))
                    return true;
            }
        }

        // No children contained matching geometry.
        return false;
    }

    bool bCompareDistance = kRay.UseLength();

    /*
     * Examine our 'collision squares'.
    */
    float fRayDotTriangle1Normal;
    float fRayDotTriangle2Normal;
    bool bTriangle1FacesRayCast;
    bool bTriangle2FacesRayCast;
    float fDistFromPlane;
    NiPoint3 kCollisionPoint(0.0f, 0.0f, 0.0f);

    NiPoint3 k0;
    NiPoint3 k1;
    NiPoint3 k2;
    NiPoint3 k3;

    NiPoint3 kNormal0;
    NiPoint3 kNormal1;
    NiPoint3 kNormal2;

    NiUInt32 uiSize = pkDataRoot->GetCellSize();
    for (NiUInt32 uiY = 0; uiY < uiSize; ++uiY)
    {
        for (NiUInt32 uiX = 0; uiX < uiSize; ++uiX)
        {
            // Retrieve the four points            
            pkDataRoot->GetVertexAt(kPositionIter, k0, NiIndex(uiX, uiY));
            pkDataRoot->GetVertexAt(kPositionIter, k1, NiIndex(uiX+1, uiY));
            pkDataRoot->GetVertexAt(kPositionIter, k2, NiIndex(uiX, uiY+1));
            pkDataRoot->GetVertexAt(kPositionIter, k3, NiIndex(uiX+1, uiY+1));

            // Identify the tesselation:
            // NOTE: This is done on the assumption indexes can not change which is correct in the 
            // current implementation.
            // case for tesselation = 
            //   _      _
            // |\ | or | /|
            // |_\|    |/_| 
            // true    false
            //
            bool bTesselation = (uiY % 2 != 0 || uiX % 2 != 0) && (uiY % 2 == 0 || uiX % 2 == 0);

            // Rotate the points to fit the tesselation
            if (!bTesselation)
            {
                // Rotate the points
                NiPoint3 kTemp = k0;
                k0 = k2;
                k2 = k3;
                k3 = k1;
                k1 = kTemp;
            }

            /*
             * TRIANGLE 1: 012
            */
            kNormal0 = (k1 - k0).UnitCross(k2 - k0);
            fRayDotTriangle1Normal = kRay.GetDirection().Dot(kNormal0);
            bTriangle1FacesRayCast = (fRayDotTriangle1Normal < 0.0f);
            // Triangle facing right direction?
            if (bTriangle1FacesRayCast)
            {
                // Find intersection point on the triangles plane.
                // Get functions are inlined.
                fDistFromPlane =
                    -1.0f * ((kOrigin - k0).Dot(kNormal0) / fRayDotTriangle1Normal);
                kCollisionPoint =
                    kOrigin + (fDistFromPlane * kRay.GetDirection());

                // Now see if point is on same side as all 3 triangles
                if (fDistFromPlane >= - 1e-05f &&
                    (k1 - k0).Cross
                    (kCollisionPoint - k0).Dot(kNormal0) >= - 1e-05f &&
                    (k2 - k1).Cross
                    (kCollisionPoint - k1).Dot(kNormal0) >= - 1e-05f &&
                    (k0 - k2).Cross
                    (kCollisionPoint - k2).Dot(kNormal0) >= - 1e-05f)
                {
                    NiPoint3 kDistance = kOrigin - kCollisionPoint;
                    if (!bCompareDistance || 
                        kDistance.Length() < kRay.GetLength())
                    {
                        kCollisionPoint = kTerrainTransform * kCollisionPoint;
                        kNormal0 = 
                            kTerrainTransform.m_Rotate.Inverse() * kNormal0;
                        kNormal0.Unitize();

                        bIntersection |= kRay.UseClosestIntersection(
                            kCollisionPoint, kNormal0);  
                    }
                }
            }

            /*
             * TRIANGLE 2: 132
            */
            
            kNormal1 = (k3 - k1).UnitCross(k2 - k1);
            fRayDotTriangle2Normal = kRay.GetDirection().Dot(kNormal1);
            bTriangle2FacesRayCast = (fRayDotTriangle2Normal < 0.0f);

            // Triangle facing right direction?
            if (bTriangle2FacesRayCast)
            {
                // Find intersection point on the triangles plane.
                // Get functions are inlined.
                fDistFromPlane =
                    -1.0f * ((kOrigin - k1).Dot(kNormal1) / fRayDotTriangle2Normal);
                kCollisionPoint =
                    kOrigin + (fDistFromPlane * kRay.GetDirection());

                // Now see if point is on same side as all 3 triangles
                if (fDistFromPlane >= - 1e-05f &&
                    (k3 - k1).Cross
                    (kCollisionPoint - k1).Dot(kNormal1) >= - 1e-05f &&
                    (k2 - k3).Cross
                    (kCollisionPoint - k3).Dot(kNormal1) >= - 1e-05f &&
                    (k1 - k2).Cross
                    (kCollisionPoint - k2).Dot(kNormal1) >= - 1e-05f)
                {
                    NiPoint3 kDistance = kOrigin - kCollisionPoint;
                    if (!bCompareDistance || 
                        kDistance.Length() < kRay.GetLength())
                    {
                        kCollisionPoint = kTerrainTransform * kCollisionPoint;
                        kNormal1 = kTerrainTransform.m_Rotate.Inverse() * 
                            kNormal1;
                        kNormal1.Unitize();
                        bIntersection |= kRay.UseClosestIntersection(
                            kCollisionPoint, kNormal1);
                    }
                }
            }

            // Check that this ray hasn't intersected perfectly with one
            // of the triangle's verts (May be missed due to floating point error)
            kOrigin = kRay.GetOrigin();
            // If the k0 vertex is at the same (x,y) as the
            // ray cast origin and it's triangle is facing
            // the right direction, check for intersection.
            if (kOrigin.x == k0.x && kOrigin.y == k0.y
                && bTriangle1FacesRayCast)
            {
                pkDataRoot->GetNormalAt(kNormalIter, kNormal2, NiIndex(uiX, uiY));
                bIntersection |= kRay.UseClosestIntersection(k0, kNormal2);
            }
            // If the k3 vertex is at the same (x,y) as the
            // ray cast origin and it's triangle is facing
            // the right direction, check for intersection.
            if (kOrigin.x == k3.x && kOrigin.y == k3.y
                && bTriangle2FacesRayCast)
            {
                pkDataRoot->GetNormalAt(kNormalIter, kNormal2, NiIndex(uiX+1, uiY+1));
                bIntersection |= kRay.UseClosestIntersection(k3, kNormal2);
            }
            // If the k1 or k2 vertex is at the same (x,y) as the
            // ray cast origin and either of it's triangles are facing
            // the right direction ,check for intersection.
            if (kOrigin.x == k1.x && kOrigin.y == k1.y &&
                (bTriangle1FacesRayCast || bTriangle2FacesRayCast))
            {
                pkDataRoot->GetNormalAt(kNormalIter, kNormal2, NiIndex(uiX+1, uiY));
                bIntersection |= kRay.UseClosestIntersection(k1, kNormal2);
            }
            if (kOrigin.x == k2.x && kOrigin.y == k2.y &&
                (bTriangle1FacesRayCast || bTriangle2FacesRayCast))
            {
                pkDataRoot->GetNormalAt(kNormalIter, kNormal2, NiIndex(uiX, uiY+1));
                bIntersection |= kRay.UseClosestIntersection(k2, kNormal2);
            }
        }
    }

    if (bIntersection)
        kRay.SetCollidedLeaf(pkDataRoot);

    return bIntersection;
}

//------------------------------------------------------------------------------------------------
bool TestBound(const NiBound& kVolume, NiTerrainCell* pkDataCell)
{
    float fDistDelta = (kVolume.GetCenter() -
        pkDataCell->GetLocalBound().GetCenter()).SqrLength();
    float fRadiusDelta = NiSqr(
        kVolume.GetRadius() + pkDataCell->GetLocalBound().GetRadius());

    if (fDistDelta < fRadiusDelta)
        return true;
    else
        return false;
}

//------------------------------------------------------------------------------------------------
bool TestBound2D(const NiBound& kVolume, NiTerrainCell* pkDataCell) 
{
    NiPoint2 fDiff(
        kVolume.GetCenter().x - pkDataCell->GetLocalBound().GetCenter().x,
        kVolume.GetCenter().y - pkDataCell->GetLocalBound().GetCenter().y);

    float fRadiusDiff = NiSqr(kVolume.GetRadius() +
        pkDataCell->GetLocalBound().GetRadius());

    if (NiSqr(fDiff.x) + NiSqr(fDiff.y) < fRadiusDiff)
        return true;
    else
        return false;
}

//------------------------------------------------------------------------------------------------
bool GetSurfaceOpacity(const NiSurface* pkSurface, const NiPoint3& kLocation,
    NiUInt8& ucValue, NiUInt32 uiDetailLevel, bool bSearchLowerDetail,
    const NiTerrainCell* pkDataCell)
{
    EE_UNUSED_ARG(pkSurface);
    EE_UNUSED_ARG(kLocation);
    EE_UNUSED_ARG(ucValue);
    EE_UNUSED_ARG(uiDetailLevel);
    EE_UNUSED_ARG(bSearchLowerDetail);
    EE_UNUSED_ARG(pkDataCell);

    ucValue = 0;

    NiTerrainCellLeaf* pkLeaf = 
        NiDynamicCast(NiTerrainCellLeaf, pkDataCell);
    if (!pkLeaf)
        return false;

    NiTransform kTerrainTransform = pkDataCell->GetContainingSector()
        ->GetSectorData()->GetWorldTransform();

    NiUInt32 uiComponent;
    if (!pkLeaf->GetSurfacePriority(pkSurface, uiComponent))
        return false;

    const NiPoint3& kModelLoc =  kTerrainTransform.m_Rotate.Inverse() * (
        (kLocation - kTerrainTransform.m_Translate)
        / kTerrainTransform.m_fScale);

    NiTerrainPositionRandomAccessIterator kPositionIter;
    NiUInt8 ucMask = NiDataStream::LOCK_READ | 
        NiDataStream::LOCK_TOOL_READ;
    NiTerrainStreamLocks kLock;
    kLock.GetPositionIterator(pkLeaf, ucMask, 
        kPositionIter);

    // Get the width of the world
    NiPoint3 kTopRight;
    pkLeaf->GetVertexAt(kPositionIter, kTopRight, 
        NiIndex::UNIT * pkLeaf->GetCellSize());

    NiPoint3 kBottomLeft;
    pkLeaf->GetVertexAt(kPositionIter, kBottomLeft, NiIndex::ZERO);

    float fLeafWorldWidth = kTopRight.y - kBottomLeft.y;
    
    NiTextureRegion kTextureRegion = 
        pkLeaf->GetTextureRegion(NiTerrain::TextureType::BLEND_MASK);
    NiUInt32 uiWidth = kTextureRegion.GetTexture()->GetWidth();

    // Calculate the range of pixels in the mask to consider
    NiIndex kStartPoint = kTextureRegion.GetStartPixelIndex(); 
    NiIndex kEndPoint = kTextureRegion.GetEndPixelIndex(); 

    // Get the intersection location, in percent
    float fPercentX = NiAbs(1.0f - (kTopRight.x - kModelLoc.x) / fLeafWorldWidth);
    float fPercentY = NiAbs((kTopRight.y - kModelLoc.y) / fLeafWorldWidth);

    // We need to interpolate the values of our surrounding points
    NiUInt32 uiFloorX = (NiUInt32)(kStartPoint.x + fPercentX * 
        (kEndPoint.x - kStartPoint.x));
    NiUInt32 uiFloorY = (NiUInt32)(kStartPoint.y + fPercentY * 
        (kEndPoint.y - kStartPoint.y));

    if (uiFloorX > (NiUInt32)(uiWidth - 1))
        uiFloorX = (NiUInt32)(uiWidth - 1);
    if (uiFloorY > (NiUInt32)(uiWidth - 1))
        uiFloorY = (NiUInt32)(uiWidth - 1);

    NiUInt32 uiCeilX = uiFloorX + 1;
    NiUInt32 uiCeilY = uiFloorY + 1;
    if (uiCeilX > (uiWidth - 1))
        uiCeilX = (uiWidth - 1);
    if (uiCeilY > (uiWidth - 1))
        uiCeilY = (uiWidth - 1);

    NiUInt32 uiSum = 
        (NiUInt32)pkLeaf->GetPixelAt(NiTerrain::TextureType::BLEND_MASK, 
            NiIndex(uiCeilX, uiCeilY), uiComponent) +
        (NiUInt32)pkLeaf->GetPixelAt(NiTerrain::TextureType::BLEND_MASK, 
            NiIndex(uiCeilX, uiFloorY), uiComponent) +
        (NiUInt32)pkLeaf->GetPixelAt(NiTerrain::TextureType::BLEND_MASK, 
            NiIndex(uiFloorX, uiCeilY), uiComponent) +
        (NiUInt32)pkLeaf->GetPixelAt(NiTerrain::TextureType::BLEND_MASK, 
            NiIndex(uiFloorX, uiFloorY), uiComponent);

    ucValue = NiUInt8(uiSum / 4);
    return true;
}

//------------------------------------------------------------------------------------------------
void ResizeImage(NiUInt8* pucSrcPixels, 
                 NiUInt32 uiSrcWidth, NiUInt8* pucDstPixels, NiUInt32 uiDstWidth, 
                 NiUInt32 uiNumChannels)
{
    NiRect<float> kCropRegion;
    kCropRegion.m_top = 0.0f;
    kCropRegion.m_bottom = 1.0f;
    kCropRegion.m_left = 0.0f;
    kCropRegion.m_right = 1.0f;
    CropImage(kCropRegion, pucSrcPixels, uiSrcWidth, pucDstPixels, uiDstWidth,
        uiNumChannels);
}

//------------------------------------------------------------------------------------------------
void ResizeImage(NiUInt8* pucSrcPixels, 
    NiUInt32 uiSrcWidth, NiUInt32 uiSrcHeight, NiUInt8* pucDstPixels,
    NiUInt32 uiDstWidth, NiUInt32 uiDstHeight, NiUInt32 uiNumChannels)
{
    NiRect<float> kCropRegion;
    kCropRegion.m_top = 0.0f;
    kCropRegion.m_bottom = 1.0f;
    kCropRegion.m_left = 0.0f;
    kCropRegion.m_right = 1.0f;

    NiRect<NiInt32> kSampleRegion;
    kSampleRegion.m_left = 0;
    kSampleRegion.m_right = uiDstWidth - 1;
    kSampleRegion.m_top = 0;
    kSampleRegion.m_bottom = uiDstHeight - 1;

    ResampleImage(kCropRegion, pucSrcPixels, uiSrcWidth, uiSrcHeight, kSampleRegion, pucDstPixels,
        uiDstWidth, uiDstHeight, uiNumChannels);
}

//------------------------------------------------------------------------------------------------
void CropImage(const NiRect<float>& kCropRegion, NiUInt8* pucSrcPixels, 
     NiUInt32 uiSrcWidth, NiUInt8* pucDstPixels, NiUInt32 uiDstWidth, 
     NiUInt32 uiNumChannels)
{
    NiRect<NiInt32> kSampleRegion;
    kSampleRegion.m_left = 0;
    kSampleRegion.m_right = uiDstWidth - 1;
    kSampleRegion.m_top = 0;
    kSampleRegion.m_bottom = uiDstWidth - 1;

    ResampleImage(kCropRegion, pucSrcPixels, uiSrcWidth, kSampleRegion, pucDstPixels, uiDstWidth,
        uiNumChannels);
}

//------------------------------------------------------------------------------------------------
void ResampleImage(const NiRect<float>& kCropRegion, NiUInt8* pucSrcPixels, 
    NiUInt32 uiSrcWidth,  
    const NiRect<NiInt32>& kSampleRegion, NiUInt8* pucDstPixels, 
    NiUInt32 uiDstWidth,
    NiUInt32 uiNumChannels)
{
    ResampleImage(kCropRegion, pucSrcPixels, uiSrcWidth, uiSrcWidth, kSampleRegion, pucDstPixels, 
        uiDstWidth, uiDstWidth, uiNumChannels);
}

//------------------------------------------------------------------------------------------------
void ResampleImage(const NiRect<float>& kCropRegion, NiUInt8* pucSrcPixels, 
    NiUInt32 uiSrcWidth, NiUInt32 uiSrcHeight, 
    const NiRect<NiInt32>& kSampleRegion, NiUInt8* pucDstPixels, 
    NiUInt32 uiDstWidth, NiUInt32 uiDstHeight,
    NiUInt32 uiNumChannels)
{
    // Make sure the crop region is within bounds
    EE_ASSERT(kCropRegion.m_bottom > kCropRegion.m_top);
    EE_ASSERT(kCropRegion.m_right > kCropRegion.m_left);
    EE_ASSERT(kCropRegion.m_top < 1.0f);
    EE_ASSERT(kCropRegion.m_bottom > 0.0f);
    EE_ASSERT(kCropRegion.m_left < 1.0f);
    EE_ASSERT(kCropRegion.m_right > 0.0f);

    // Make sure the sample region is within bounds
    EE_ASSERT(kSampleRegion.m_left <= kSampleRegion.m_right);
    EE_ASSERT(kSampleRegion.m_top <= kSampleRegion.m_bottom);
    EE_ASSERT(kSampleRegion.m_left >= 0);
    EE_ASSERT(kSampleRegion.m_right < NiInt32(uiDstWidth));
    EE_ASSERT(kSampleRegion.m_top >= 0);
    EE_ASSERT(kSampleRegion.m_bottom <  NiInt32(uiDstWidth));

    // Use Bilinear filtering to generate the new image
    for (NiInt32 iY = kSampleRegion.m_top; iY <= kSampleRegion.m_bottom; ++iY)
    {
        for (NiInt32 iX = kSampleRegion.m_left; iX <= kSampleRegion.m_right; ++iX)
        {
            // Calculate UV point to sample on original image
            float fU = float(iX) / float(uiDstWidth);
            float fV = float(iY) / float(uiDstHeight);

            // Convert these coordinates so they lie within the crop region
            fU = fU * kCropRegion.GetWidth() + kCropRegion.m_left;
            fV = fV * kCropRegion.GetHeight() + kCropRegion.m_top;

            // Calculate the relevant pixels at this point
            fU *= float(uiSrcWidth);
            fV *= float(uiSrcHeight);
            int iOrigX = NiUInt32(NiFloor(fU));
            int iOrigY = NiUInt32(NiFloor(fV));
            float fURatio = fU - iOrigX;
            float fVRatio = fV - iOrigY;
            float fUOpposite = 1 - fURatio;
            float fVOpposite = 1 - fVRatio;

            // Sampling Indexes
            NiUInt32 uiTopLeft = 
                (iOrigY) * uiSrcWidth + (iOrigX);
            NiUInt32 uiTopRight = 
                (iOrigY) * uiSrcWidth + (iOrigX + 1);
            NiUInt32 uiBottomLeft = 
                (iOrigY + 1) * uiSrcWidth + (iOrigX);
            NiUInt32 uiBottomRight = 
                (iOrigY + 1) * uiSrcWidth + (iOrigX + 1);
            NiUInt32 uiNewIndex = iY * uiDstWidth + iX;

            // Adjust to account for the pixel stride
            uiTopLeft *= uiNumChannels;
            uiTopRight *= uiNumChannels;
            uiBottomLeft *= uiNumChannels;
            uiBottomRight *= uiNumChannels;
            uiNewIndex *= uiNumChannels;

            EE_ASSERT(uiTopLeft + uiNumChannels - 1 < uiSrcWidth * uiSrcHeight * uiNumChannels);
            EE_ASSERT(uiTopRight + uiNumChannels - 1  < uiSrcWidth * uiSrcHeight * uiNumChannels);
            EE_ASSERT(uiBottomLeft + uiNumChannels - 1  < uiSrcWidth * uiSrcHeight * uiNumChannels);
            EE_ASSERT(uiBottomRight + uiNumChannels - 1  < uiSrcWidth * uiSrcHeight * uiNumChannels);
            EE_ASSERT(uiNewIndex + uiNumChannels - 1  < uiDstWidth * uiDstHeight * uiNumChannels);

            // Loop through all the components:
            for (NiUInt32 uiComponent = 0; uiComponent < uiNumChannels; 
                ++uiComponent)
            {
                float fTopLeftValue = 
                    pucSrcPixels[uiTopLeft + uiComponent];
                float fTopRightValue = 
                    pucSrcPixels[uiTopRight + uiComponent];
                float fBottomLeftValue = 
                    pucSrcPixels[uiBottomLeft + uiComponent];
                float fBottomRightValue = 
                    pucSrcPixels[uiBottomRight + uiComponent];

                float fValue = 
                    (fTopLeftValue * fUOpposite + 
                    fTopRightValue * fURatio) * fVOpposite + 
                    (fBottomLeftValue * fUOpposite + 
                    fBottomRightValue * fURatio) * fVRatio;

                pucDstPixels[uiNewIndex + uiComponent] = NiUInt8(fValue);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
bool WriteTexture(const char* pcDestFilePath, NiPixelData* pkPixelData)
{
    EE_ASSERT(pkPixelData);

    NiFilename kFilename(pcDestFilePath);
    if (strcmp(kFilename.GetExt(), ".tga") != 0)
        return false;

    // We will first attempt to convert the pixel data into a known format we 
    // can use
    NiPixelDataPtr pkDstPixelData;
    NiDevImageConverter kConverter;
    const NiPixelFormat kSrcFormat = pkPixelData->GetPixelFormat();
    NiPixelFormat kDestFormat = NiPixelFormat::RGBA32;
    if (kSrcFormat.GetNumComponents() < 3)
        return false;
    if (kSrcFormat.GetFormat() == NiPixelFormat::FORMAT_RGB)
        kDestFormat = NiPixelFormat::RGB24;

    // Attempt conversion
    pkDstPixelData = kConverter.ConvertPixelData(*pkPixelData, 
        kDestFormat, NULL, false);

    // Now attempt to swap red and blue channels
    kDestFormat = NiPixelFormat(NiPixelFormat::FORMAT_RGBA, 
        NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_NORM_INT, 8,
        NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_NORM_INT, 8, 
        NiPixelFormat::COMP_RED, NiPixelFormat::REP_NORM_INT, 8,
        NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_NORM_INT, 8);
    if (kSrcFormat.GetFormat() == NiPixelFormat::FORMAT_RGB)
        kDestFormat = NiPixelFormat(NiPixelFormat::FORMAT_RGB, 
        NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_NORM_INT, 8,
        NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_NORM_INT, 8, 
        NiPixelFormat::COMP_RED, NiPixelFormat::REP_NORM_INT, 8);

    pkDstPixelData = kConverter.ConvertPixelData(*pkDstPixelData, 
        kDestFormat, NULL, false);

    if (!pkDstPixelData)
        return false;

    // Create the file:
    efd::File* pkFile = efd::File::GetFile(pcDestFilePath, efd::File::WRITE_ONLY);
    if (!pkFile)
        return false;

    // Setup cross platform endianess - presently only works for win32
    // This file should be in LITTLE_ENDIAN
    bool bIsLittleEndian = efd::SystemDesc::GetSystemDesc().IsLittleEndian();
    pkFile->SetEndianSwap(!bIsLittleEndian);

    // Gather information from the pixel data
    NiUInt32 uiStride = pkDstPixelData->GetPixelStride();

    NiUInt8 ucIDLength = 0; // No Image ID Data
    NiUInt8 ucColorMapType = 0; // No color map data
    NiUInt8 ucImageType = 2 | 0x08; // or 10 if colored AND RLE
    NiUInt8 aucColorMapSpec[5] = {0};
    NiUInt8 aucImageSpec[10] = {0};
    NiUInt16* pusWidth = (NiUInt16*)&aucImageSpec[4];
    NiUInt16* pusHeight = (NiUInt16*)&aucImageSpec[6];

    *pusWidth = (NiUInt16)pkDstPixelData->GetWidth();
    *pusHeight = (NiUInt16)pkDstPixelData->GetHeight();

    aucImageSpec[8] = (NiUInt8)(8 * uiStride); // 8 bits per pixel
    if (kDestFormat.GetNumComponents() == 4)
        aucImageSpec[9] = 8; // 8 bits for alpha, Bottom Left ordering
    else
        aucImageSpec[9] = 0; // 0 bits for alpha, Bottom Left ordering

    NiUInt64 ulExtAreaOffset = 0;
    NiUInt64 ulDeveloperDirOffset = 0;
    NiUInt8 aucSignature[] = {"TRUEVISION-XFILE."};

    // Write File Header
    pkFile->Seek(0);
    pkFile->Write(&ucIDLength, sizeof(ucIDLength));
    pkFile->Write(&ucColorMapType, sizeof(ucColorMapType));
    pkFile->Write(&ucImageType, sizeof(ucImageType));
    pkFile->Write(&aucColorMapSpec, sizeof(aucColorMapSpec));
    pkFile->Write(&aucImageSpec, sizeof(aucImageSpec));

    // Write image data (RLE)
    const unsigned char* pucCurPixel = pkDstPixelData->GetPixels();
    unsigned char aucNextColor[4] = {0};
    unsigned char aucCurrentColor[4] = {0};
    NiUInt32 uiWidth = *pusWidth;
    NiUInt32 uiHeight = *pusHeight;
    for (NiUInt32 uiY = 0; uiY < uiHeight; ++uiY)
    {
        for (NiUInt32 uiX = 0; uiX < uiWidth; ++uiX)
        {
            NiUInt8 ucPixCount = 0;
            NiUInt32 uiCurrentPixelID = 
                (((uiHeight - uiY - 1) * uiWidth) + uiX) * uiStride;                        

            //Parse the scan line (127 pixels max)
            bool bSameColor = true;
            bool bUseRLE = false;
            while (uiX < uiWidth && ucPixCount < 127)
            {                        
                bSameColor = true;
                // Compare two pixel and see if they are the same
                for (NiUInt32 uiC = 0; uiC < uiStride; ++uiC)
                {
                    aucCurrentColor[uiC] = pucCurPixel[uiCurrentPixelID + 
                        ((ucPixCount) * uiStride)  + uiC];
                    aucNextColor[uiC] = pucCurPixel[uiCurrentPixelID + 
                        ((ucPixCount + 1)* uiStride)  + uiC];                    
                    
                    if (aucCurrentColor[uiC] != aucNextColor[uiC] || 
                        uiX + 1 == uiWidth)
                    {
                        // we need to stop the packet regardless of compression
                        // if the next pixel is not on this line
                        bSameColor = (uiX + 1 == uiWidth && !bUseRLE);
                        break;
                    }
                }

                // We haven't got any pixel yet are there will be more than 
                // one, define whether we will be using RLE
                // compression for this packet or not
                if (ucPixCount == 0 && uiX + 1 != uiWidth)
                    bUseRLE = bSameColor;

                // Increase the count of pixels in this streak if we use 
                // RLE compression and the colors are the same or if we do
                // not use compression and the colors are different
                if ((bSameColor && bUseRLE) || (!bSameColor && !bUseRLE))
                {
                    ucPixCount++;
                    uiX++;
                }
                else
                    break;
            }

            // We should be at the end of the RLE Packet now
            NiUInt32 uiComponentSizes[] = {1, 1, 1, 1};
            EE_ASSERT(uiStride <= 4);
            if (!bUseRLE)
            {
                pkFile->BinaryWrite(&ucPixCount, sizeof(ucPixCount), uiComponentSizes, 1);
                pkFile->BinaryWrite(&pucCurPixel[uiCurrentPixelID], 
                    (ucPixCount + 1) * uiStride * sizeof(ucPixCount), uiComponentSizes, uiStride);
            }
            else
            {
                ucPixCount |= 0x80; // Mark as non-raw
                pkFile->BinaryWrite(&ucPixCount, sizeof(ucPixCount), uiComponentSizes, 1);
                pkFile->BinaryWrite(&pucCurPixel[uiCurrentPixelID], 
                    uiStride * sizeof(ucPixCount), uiComponentSizes, uiStride); 
            }
        }
    }

    // Write Image footer
    pkFile->Write(&ulExtAreaOffset, sizeof(ulExtAreaOffset));
    pkFile->Write(&ulDeveloperDirOffset, sizeof(ulDeveloperDirOffset));
    pkFile->Write(&aucSignature, sizeof(aucSignature));

    NiDelete pkFile;

    return true;
}

//------------------------------------------------------------------------------------------------
NiPixelData* ExtractPixelData(NiTexture* pkTexture)
{
    if (!pkTexture)
        return NULL;

    NiSourceTexture* pkSourceTexture = NiDynamicCast(NiSourceTexture, pkTexture);
    if (pkSourceTexture)
    {   
        return pkSourceTexture->GetSourcePixelData();
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
NiPixelData* ExtractPixelData(NiRenderClick* pkRenderClick)
{
    if (!pkRenderClick)
        return NULL;

    NiRenderTargetGroup* pkTarget = pkRenderClick->GetRenderTargetGroup();
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer && pkTarget);
    return pkRenderer->TakeScreenShot(0, pkTarget); 
}

//------------------------------------------------------------------------------------------------
void UpdateTextureRegion(NiSourceTexture* pkDstTexture, NiRect<efd::SInt32> kRegion)
{
    // If there is no renderer data on the texture then it hasn't been uploaded yet. 
    // so there is nothing for us to update.
    if (!pkDstTexture->GetRendererData())
        return;

    NiPixelData* pkSrcData = pkDstTexture->GetSourcePixelData();
    EE_ASSERT(pkSrcData);

    // Facilitate updating of the texture according to different renderers
    bool bUseFallback = false;
    switch(NiRenderer::GetRenderer()->GetRendererID())
    {
    case efd::SystemDesc::RENDERER_DX9:
        {
#ifdef WIN32
            NiDX9SourceTextureData* pkTextureData = 
                (NiDX9SourceTextureData*)pkDstTexture->GetRendererData();
            EE_ASSERT(pkTextureData);
            const NiPixelFormat* pkDstFormat = pkTextureData->GetPixelFormat();
            D3DTexturePtr pkD3DTexture = (D3DTexturePtr)pkTextureData->GetD3DTexture();
            EE_ASSERT(pkD3DTexture);

            // We only support one type of format at the moment
            if (pkSrcData->GetPixelFormat() != NiPixelFormat::RGB24 ||
                pkSrcData->GetPixelFormat().GetCompressed() || pkDstFormat->GetCompressed())
            {            
                bUseFallback = true;
                break;
            }
            
            // Lock the region we're interested in
            RECT kRect = {};
            kRect.top = kRegion.m_top;
            kRect.bottom = kRegion.m_bottom + 1;
            kRect.left = kRegion.m_left;
            kRect.right = kRegion.m_right + 1;
            D3DLOCKED_RECT kLockedRect;
            EE_VERIFYEQUALS(pkD3DTexture->LockRect(0, &kLockedRect, &kRect, 0), D3D_OK);
            
            // Calculate some values to help us copy the data to the right location
            const NiPixelFormat& kSrcFormat = pkSrcData->GetPixelFormat();
            efd::UInt32 uiNumScanlines = kRegion.GetHeight() + 1;
            efd::UInt32 uiScanlineWidth = kRegion.GetWidth() + 1;
            efd::UInt32 uiSrcStride = pkSrcData->GetPixelStride();
            efd::UInt32 uiDstStride = pkDstFormat->GetBitsPerPixel() / 8;
            efd::UInt8* pucPixels = pkSrcData->GetPixels();
            EE_ASSERT(pucPixels);

            // Copy the data across
            for (efd::UInt32 uiY = 0; uiY < uiNumScanlines; ++uiY)
            {
                // Calculate the source of the data
                efd::UInt32 uiSourceY = uiY + kRegion.m_top;
                efd::UInt32 uiSourceX = kRegion.m_left;
                efd::UInt32 uiSourcePixel = uiSourceY * pkSrcData->GetWidth() + uiSourceX;
                efd::UInt8* pucSrcLine = &pucPixels[uiSourcePixel * uiSrcStride];

                // Calculate the destination of the data
                efd::UInt8* pucDstLine = &((efd::UInt8*)kLockedRect.pBits)[uiY * kLockedRect.Pitch];

                for (efd::UInt32 uiX = 0; uiX < uiScanlineWidth; ++uiX)
                {
                    // Loop through all the valid components
                    for (efd::UInt32 uiC = 0; uiC < kSrcFormat.GetNumComponents(); ++uiC)
                    {
                        NiPixelFormat::Component kComponent;
                        NiPixelFormat::Representation kRepresentation;
                        efd::UInt8 ucBPC;
                        bool bSigned;
                        kSrcFormat.GetComponent(uiC, kComponent, kRepresentation, ucBPC, bSigned);
                        
                        EE_ASSERT(kSrcFormat.GetBits(kComponent) == 8);
                        EE_ASSERT(pkDstFormat->GetBits(kComponent) == 8);
                        efd::UInt32 uiSrcChannel = kSrcFormat.GetShift(kComponent) / 8;
                        efd::UInt32 uiDstChannel = pkDstFormat->GetShift(kComponent) / 8;

                        pucDstLine[uiX * uiDstStride + uiDstChannel] =
                            pucSrcLine[uiX * uiSrcStride + uiSrcChannel];
                    }

                    // Loop through all the leftover components set them to max values
                    for (efd::UInt32 uiC = kSrcFormat.GetNumComponents(); 
                        uiC < pkDstFormat->GetNumComponents(); ++uiC)
                    {
                        NiPixelFormat::Component kComponent;
                        NiPixelFormat::Representation kRepresentation;
                        efd::UInt8 ucBPC;
                        bool bSigned;
                        pkDstFormat->GetComponent(uiC, kComponent, kRepresentation, ucBPC, bSigned);
                        EE_ASSERT(pkDstFormat->GetBits(kComponent) == 8);
                        efd::UInt32 uiDstChannel = pkDstFormat->GetShift(kComponent) / 8;
                        pucDstLine[uiX * uiDstStride + uiDstChannel] = 255;
                    }
                }
            }

            // Unlock the surface
            pkD3DTexture->UnlockRect(0);
#endif // WIN32
            break;
        }
    default:
        // Renderer not recognized
        bUseFallback = true;
    }

    if (bUseFallback)
    {
        // We do not have any renderer specific code available to increase performance
        // use the standard gamebryo method
        pkSrcData->MarkAsChanged();
    }
}

//------------------------------------------------------------------------------------------------
NiSourceTexture* CombineImageChannelsToTexture(
    NiPixelData* pkRedImage, NiPixelFormat::Component eRedComponent, 
    NiPixelData* pkGreenImage, NiPixelFormat::Component eGreenComponent,
    NiPixelData* pkBlueImage, NiPixelFormat::Component eBlueComponent,
    NiPixelData* pkAlphaImage, NiPixelFormat::Component eAlphaComponent)
{
    // Do we need this texture?
    if (!pkRedImage && !pkGreenImage && !pkBlueImage && !pkAlphaImage)
        return NULL;

    // Create some helper variables
    NiPixelDataPtr apkChannelImage[4] = {pkRedImage, pkGreenImage, pkBlueImage, pkAlphaImage};
    NiPixelFormat::Component aeChannelComponent[4] = 
    {eRedComponent, eGreenComponent, eBlueComponent, eAlphaComponent};

    // Check the images for consistency
    efd::UInt32 uiWidth = 0;
    efd::UInt32 uiHeight = 0;
    efd::UInt32 uiNumChannels = 0;
    efd::UInt8* apucPixels[4] = {0};
    efd::UInt32 auiChannelOffset[4] = {0};
    efd::UInt32 auiChannelStride[4] = {0};
    {
        // Make sure all images are the same size
        for (efd::UInt32 uiChannel = 0; uiChannel < 4; ++uiChannel)
        {
            NiPixelData* pkChannelData = apkChannelImage[uiChannel];
            if (pkChannelData)
            {
                // Figure out the maximum valid channel
                uiNumChannels = uiChannel;

                // Figure out the size of the image
                if (uiWidth == 0 && uiHeight == 0)
                {
                    uiWidth = pkChannelData->GetWidth();
                    uiHeight = pkChannelData->GetHeight();
                }
                else
                {
                    EE_ASSERT(uiWidth == pkChannelData->GetWidth());
                    EE_ASSERT(uiHeight == pkChannelData->GetHeight());
                }

                // Check that the specified component exists in it
                EE_ASSERT(
                    pkChannelData->GetPixelFormat().GetBits(aeChannelComponent[uiChannel]) == 8);

                // Fetch the pixels
                apucPixels[uiChannel] = pkChannelData->GetPixels();

                // DT 40877
                // Override endian bit for correct GetShift results. This is necessary because the
                // terrain texture codepath currently assumes little endian formats. On PC it is a
                // no-op, on consoles we need to be sure GetShift does not perform an endian swap.
                // This will ultimately be replaced by tool-time compilation of terrain materials.
                const_cast<NiPixelFormat&>(pkChannelData->GetPixelFormat()).SetLittleEndian(
                    NiSystemDesc::GetSystemDesc().IsLittleEndian());

                // Fetch the offset of the channel in the image
                auiChannelOffset[uiChannel] = 
                    pkChannelData->GetPixelFormat().GetShift(aeChannelComponent[uiChannel]) / 8;

                // Fetch the stride in this channel
                auiChannelStride[uiChannel] = pkChannelData->GetPixelStride();
            }
        }
    }

    // Build the new texture
    NiPixelData* pkPixelData = NiNew NiPixelData(uiWidth, uiHeight, NiPixelFormat::RGBA32);
    {
        // Setup the variables
        efd::UInt32 uiNumPixels = uiWidth * uiHeight;
        efd::UInt8* pucDestinationPixels = pkPixelData->GetPixels();
        efd::UInt32 uiDstPixelStride = pkPixelData->GetPixelStride();

        // Figure out the channel offsets in the destination texture
        NiPixelFormat::Component aeDestinationComponent[4] = {
            NiPixelFormat::COMP_RED, 
            NiPixelFormat::COMP_GREEN, 
            NiPixelFormat::COMP_BLUE, 
            NiPixelFormat::COMP_ALPHA};

        // DT 40877
        // Override endian bit for correct GetShift results. This is necessary because the terrain
        // texture codepath currently assumes little endian formats. On PC it is a no-op, on
        // consoles we need to be sure GetShift does not perform an endian swap. This will
        // ultimately be replaced by tool-time compilation of terrain materials.
        bool isLittleEndian = pkPixelData->GetPixelFormat().GetLittleEndian();
        const_cast<NiPixelFormat&>(pkPixelData->GetPixelFormat()).SetLittleEndian(
            NiSystemDesc::GetSystemDesc().IsLittleEndian());

        efd::UInt32 auiDstChannelOffset[4] = {
            (efd::UInt32)(pkPixelData->GetPixelFormat().GetShift(aeDestinationComponent[0]) / 8),
            (efd::UInt32)(pkPixelData->GetPixelFormat().GetShift(aeDestinationComponent[1]) / 8),
            (efd::UInt32)(pkPixelData->GetPixelFormat().GetShift(aeDestinationComponent[2]) / 8),
            (efd::UInt32)(pkPixelData->GetPixelFormat().GetShift(aeDestinationComponent[3]) / 8)};

        // Restore endian bit
        const_cast<NiPixelFormat&>(pkPixelData->GetPixelFormat()).SetLittleEndian(isLittleEndian);

        // Copy the data
        for (efd::UInt32 uiIndex = 0; uiIndex < uiNumPixels; ++uiIndex)
        {
            for (efd::UInt32 uiChannel = 0; uiChannel < 4; ++uiChannel)
            {
                // Sample the source image
                efd::UInt8 uiSample = 0;
                if (apucPixels[uiChannel])
                {
                    efd::UInt32 uiSrcPixelStride = auiChannelStride[uiChannel];
                    efd::UInt32 uiChannelOffset = auiChannelOffset[uiChannel];
                    efd::UInt32 uiSrcByteIndex = uiIndex * uiSrcPixelStride + uiChannelOffset;
                    uiSample = (apucPixels[uiChannel])[uiSrcByteIndex];
                }

                // Store the value
                efd::UInt32 uiDstByteIndex = 
                    uiIndex * uiDstPixelStride + auiDstChannelOffset[uiChannel];
                pucDestinationPixels[uiDstByteIndex] = uiSample;
            }
        }
    }

    // Finalize the compiled texture and upload it
    NiDevImageConverter kImageConverter;
    NiTexture::FormatPrefs kFormatPrefs;
    kFormatPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::ALPHA_DEFAULT;
    kFormatPrefs.m_eMipMapped = NiTexture::FormatPrefs::YES;
    kFormatPrefs.m_ePixelLayout = NiTexture::FormatPrefs::PIX_DEFAULT;
    NiPixelData* pkMipmapPixelData = kImageConverter.GenerateMipmapLevels(pkPixelData, 0);
    EE_DELETE pkPixelData;

    // Generate the texture
    // If we are in tool mode, we need to keep the compiled data on the source texture so that it
    // can be streamed out to disk later.
    bool bKeepCompiledData = NiTerrain::InToolMode();
    NiSourceTexture* pkFinalTexture = NiSourceTexture::Create(pkMipmapPixelData, kFormatPrefs, 
        !bKeepCompiledData);

    if (!pkFinalTexture)
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
            ("Terrain surface compilation failed: Unable to create packed texture."));

        if (pkPixelData)
            NiDelete pkPixelData;
        return NULL;
    }
    return pkFinalTexture;
}

//------------------------------------------------------------------------------------------------

} // End namespace.
