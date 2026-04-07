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
#include "NiTerrainDecal.h"
#include "NiTerrainDecalManager.h"

//---------------------------------------------------------------------------
NiTerrainDecalManager::NiTerrainDecalManager()
{
}

//---------------------------------------------------------------------------
NiTerrainDecalManager::~NiTerrainDecalManager()
{
   RemoveAll();
}

//---------------------------------------------------------------------------
void NiTerrainDecalManager::AddDecal(NiTerrainDecal* pkDecal)
{
    m_kDecalList.push_back(pkDecal);
    AttachChild(pkDecal);
}

//---------------------------------------------------------------------------
void NiTerrainDecalManager::RemoveDecal(NiTerrainDecal* pkDecal)
{
    m_kDecalList.remove(pkDecal);
    DetachChild(pkDecal);
}

//---------------------------------------------------------------------------
void NiTerrainDecalManager::RemoveAll()
{
    // Remove each decal 
    while (m_kDecalList.size())
    {
        NiTerrainDecal* pkHead = *m_kDecalList.begin();
        RemoveDecal(pkHead);
    }

    // Making sure all items have been removed
    m_kDecalList.clear();
}

//---------------------------------------------------------------------------
void NiTerrainDecalManager::UpdateDecals(NiTerrain* pkTerrain, 
    float fDeltaTime)
{
    DecalList::iterator kIter;
    for (kIter = m_kDecalList.begin(); kIter != m_kDecalList.end(); ++kIter)
    {
        NiTerrainDecal* pkDecal = *kIter;

        // Update the decal
        NiUInt8 ucRet = pkDecal->UpdateDecal(pkTerrain, fDeltaTime);

        // The timer has come to an end, we do not want to display this decal 
        // again it should therefore be destroyed
        if (ucRet == 1)
            RemoveDecal(pkDecal);
    }
}

//---------------------------------------------------------------------------
void NiTerrainDecalManager::NotifyTerrainChanged(NiRect<efd::SInt32>& kTerrainRegion)
{
    // Iterate over all the decals
    DecalList::iterator kIter;
    for (kIter = m_kDecalList.begin(); kIter != m_kDecalList.end(); ++kIter)
    {
        NiTerrainDecal* pkDecal = *kIter;
        pkDecal->NotifyTerrainChanged(kTerrainRegion);
    }
}

//---------------------------------------------------------------------------
void NiTerrainDecalManager::Cull(NiCullingProcess &kCuller)
{
    // Get the camera
    const NiCamera *pkCamera = kCuller.GetCamera();

    // Iterate over all the decals
    DecalList::iterator kIter;
    for (kIter = m_kDecalList.begin(); kIter != m_kDecalList.end(); ++kIter)
    {
        NiTerrainDecal* pkDecal = *kIter;

        // Get the decal to calculate their adjustments based on the camera
        pkDecal->AdjustToCamera(pkCamera);

        // Check the decal bounds against the frustum
        const NiBound kBound = pkDecal->GetMesh()->GetWorldBound();
        int iWhichSide = 0;

        NiFrustumPlanes kFrustumPlanes;
        kFrustumPlanes.Set(*pkCamera);
        NiPlane kPlane;

        bool bIsVisible = true;
        for (int i = 0; i < NiFrustumPlanes::MAX_PLANES; ++i)
        {
            // find the distance to this plane 
            kPlane = kFrustumPlanes.GetPlane(i);
            iWhichSide = kPlane.WhichSide(
                kBound.GetCenter() + (kPlane.GetNormal() * 
                kBound.GetRadius())
                );

            if (iWhichSide == NiPlane::NEGATIVE_SIDE) 
            {
                bIsVisible = false;
                break;
            }
        }

        if (bIsVisible)
        {
            kCuller.GetVisibleSet()->Add(*(pkDecal->GetMesh()));
        }
    }
}

//---------------------------------------------------------------------------
