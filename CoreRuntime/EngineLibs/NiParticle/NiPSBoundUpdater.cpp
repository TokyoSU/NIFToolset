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

// Precompiled Header
#include "NiParticlePCH.h"

#include "NiPSBoundUpdater.h"
#include "NiPSMeshParticleSystem.h"
#include "NiPSCommonSemantics.h"

NiImplementRTTI(NiPSBoundUpdater, NiObject);

//------------------------------------------------------------------------------------------------
NiPSBoundUpdater::NiPSBoundUpdater(NiInt16 sUpdateSkip) :
    m_sUpdateSkip(0),
    m_usUpdateCount(0),
    m_pkSkipBounds(NULL)
{
    SetUpdateSkip(sUpdateSkip);
}

//------------------------------------------------------------------------------------------------
NiPSBoundUpdater::~NiPSBoundUpdater()
{
    NiDelete[] m_pkSkipBounds;
}

//------------------------------------------------------------------------------------------------
void NiPSBoundUpdater::UpdateBound(NiPSParticleSystem* pkParticleSystem)
{
    // Get particle counts.
    NiUInt32 uiNumParticles = pkParticleSystem->GetNumParticles();
    NiUInt32 uiMaxNumParticles = pkParticleSystem->GetMaxNumParticles();

    // Check for mesh particle system.
    NiPSMeshParticleSystem* pkMeshParticleSystem = NiDynamicCast(
        NiPSMeshParticleSystem, pkParticleSystem);

    // Check for no particles.
    if (uiNumParticles == 0)
    {
        // Reset the update count.
        m_usUpdateCount = 0;

        return;
    }

    // Select the appropriate skip divisor.
    NiUInt8 ucSkipDivisor = pkMeshParticleSystem ? 20 : 50;

    // Check for auto setting the update skip.
    if (m_sUpdateSkip == AUTO_SKIP_UPDATE)
    {
        SetUpdateSkip((NiInt16) ((uiMaxNumParticles / ucSkipDivisor) + 1));
    }

    // Set the computed skip based on the number of particles.
    NiInt32 iComputedSkip = efd::Min<NiInt32>(m_sUpdateSkip, (uiNumParticles / ucSkipDivisor) + 1);

    // Ensure a minimum of 1.
    iComputedSkip = NiMax(iComputedSkip, 1);

    // Compute the skip bounds.
    NiBound kSkipBound;
    if (pkMeshParticleSystem)
    {
        // Get the particle container.
        NiNode* pkParticleContainer = pkMeshParticleSystem->GetParticleContainer();

        // Compute the bounding volume containing all mesh particles.
        kSkipBound = pkParticleContainer->GetAt(0)->GetWorldBound();
        for (NiUInt32 ui = m_usUpdateCount; ui < uiNumParticles; ui += iComputedSkip)
        {
            kSkipBound.Merge(&pkParticleContainer->GetAt(ui)->GetWorldBound());
        }
    }
    else
    {
        NiPoint3* pkPositions = pkParticleSystem->GetPositions();
        float* pfRadii = pkParticleSystem->GetInitialSizes();
        float* pfSizes = pkParticleSystem->GetSizes();

        // Compute the axis-aligned box containing the data. Start with the zero particle,
        // but not its size or radius, so that we know we have good data.
        NiPoint3 kPosition = pkPositions[0];
        float fMinX = kPosition.x;
        float fMaxX = fMinX;
        float fMinY = kPosition.y;
        float fMaxY = fMinY;
        float fMinZ = kPosition.z;
        float fMaxZ = fMinZ;

        // Now process all the particles we care about.
        for (NiUInt32 ui = m_usUpdateCount; ui < uiNumParticles; ui += iComputedSkip)
        {
            kPosition = pkPositions[ui];
            float fFinalSize = pfRadii[ui] * pfSizes[ui];

            fMinX = NiMin(fMinX, kPosition.x - fFinalSize);
            fMaxX = NiMax(fMaxX, kPosition.x + fFinalSize);

            fMinY = NiMin(fMinY, kPosition.y - fFinalSize);
            fMaxY = NiMax(fMaxY, kPosition.y + fFinalSize);

            fMinZ = NiMin(fMinZ, kPosition.z - fFinalSize);
            fMaxZ = NiMax(fMaxZ, kPosition.z + fFinalSize);
        }
        NiPoint3 kMin(fMinX, fMinY, fMinZ);
        NiPoint3 kMax(fMaxX, fMaxY, fMaxZ);

        kSkipBound.SetCenterAndRadius(0.5f * (kMin + kMax),
            (kMax - kMin).Length() / 2.0f);
    }

    // Update the skip bounds.
    m_pkSkipBounds[m_usUpdateCount] = kSkipBound;

    // Zero the unused bounds.
    for (NiInt32 i = iComputedSkip; i < m_sUpdateSkip; ++i)
    {
        m_pkSkipBounds[i].SetCenter(NiPoint3::ZERO);
        m_pkSkipBounds[i].SetRadius(0.0f);
    }

    // Merge the skip bounds.
    NiBound kTotalBound = m_pkSkipBounds[m_usUpdateCount];
    for (NiInt16 i = 0; i < m_usUpdateCount; ++i)
    {
        if (m_pkSkipBounds[i].GetRadius() != 0.0f)
        {
            kTotalBound.Merge(&m_pkSkipBounds[i]);
        }
    }
    for (NiInt16 i = m_usUpdateCount + 1; i < iComputedSkip; ++i)
    {
        if (m_pkSkipBounds[i].GetRadius() != 0.0f)
        {
            kTotalBound.Merge(&m_pkSkipBounds[i]);
        }
    }

    // Guarantee we have some type of bound.
    if (kTotalBound.GetRadius() != 0.0f)
    {
        // Set the bound on the particle system object.
        pkParticleSystem->SetModelBound(kTotalBound);
    }

    // Reset the update count.
    if (++m_usUpdateCount >= iComputedSkip)
    {
        m_usUpdateCount = 0;
    }
}

//------------------------------------------------------------------------------------------------
void NiPSBoundUpdater::SetUpdateSkip(NiInt16 sUpdateSkip)
{
    m_sUpdateSkip = sUpdateSkip;

    NiDelete[] m_pkSkipBounds;

    // Always have at least 1.
    sUpdateSkip = efd::Max<efd::SInt16>(sUpdateSkip, 1);

    m_pkSkipBounds = NiNew NiBound[sUpdateSkip];

    // Initialize the bounds.
    for (NiInt32 i = 0; i < sUpdateSkip; ++i)
    {
        m_pkSkipBounds[i].SetCenterAndRadius(NiPoint3::ZERO, 0.0f);
    }
}

//------------------------------------------------------------------------------------------------
void NiPSBoundUpdater::ResetUpdateSkipBounds()
{
    NiInt32 iUpdateSkip = efd::Max<efd::SInt32>(m_sUpdateSkip, 1);

    // Initialize the bounds
    for (NiInt32 i = 0; i < iUpdateSkip; ++i)
    {
        m_pkSkipBounds[i].SetCenterAndRadius(NiPoint3::ZERO, 0.0f);
    }
}

//------------------------------------------------------------------------------------------------
// Cloning
//------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSBoundUpdater);

//------------------------------------------------------------------------------------------------
void NiPSBoundUpdater::CopyMembers(
    NiPSBoundUpdater* pkDest,
    NiCloningProcess& kCloning)
{
    NiObject::CopyMembers(pkDest, kCloning);

    pkDest->SetUpdateSkip(m_sUpdateSkip);
}

//------------------------------------------------------------------------------------------------
// Streaming
//------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSBoundUpdater);

//------------------------------------------------------------------------------------------------
void NiPSBoundUpdater::LoadBinary(NiStream& kStream)
{
    NiObject::LoadBinary(kStream);

    NiInt16 sUpdateSkip;
    NiStreamLoadBinary(kStream, sUpdateSkip);
    SetUpdateSkip(sUpdateSkip);
}

//------------------------------------------------------------------------------------------------
void NiPSBoundUpdater::LinkObject(NiStream& kStream)
{
    NiObject::LinkObject(kStream);
}

//------------------------------------------------------------------------------------------------
bool NiPSBoundUpdater::RegisterStreamables(NiStream& kStream)
{
    return NiObject::RegisterStreamables(kStream);
}

//------------------------------------------------------------------------------------------------
void NiPSBoundUpdater::SaveBinary(NiStream& kStream)
{
    NiObject::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_sUpdateSkip);
}

//------------------------------------------------------------------------------------------------
bool NiPSBoundUpdater::IsEqual(NiObject* pkObject)
{
    if (!NiObject::IsEqual(pkObject))
    {
        return false;
    }

    NiPSBoundUpdater* pkDest = (NiPSBoundUpdater*) pkObject;

    if (pkDest->m_sUpdateSkip != m_sUpdateSkip)
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
// Viewer strings
//------------------------------------------------------------------------------------------------
void NiPSBoundUpdater::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSBoundUpdater::ms_RTTI.GetName()));

    pkStrings->Add(NiGetViewerString("UpdateSkip", m_sUpdateSkip));
}

//------------------------------------------------------------------------------------------------
