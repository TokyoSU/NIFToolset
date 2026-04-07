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
#include "NiTerrainDecal.h"
#include "NiTerrainStreamLocks.h"
#include "NiTerrain.h"

#include <NiMath.h>

//--------------------------------------------------------------------------------------------------
NiTerrainDecal::NiTerrainDecal() :
    m_spMesh(NULL),
    m_spTexture(NULL),
    m_kPosition(0.0f, 0.0f, 0.0f),
    m_fTimer(0.0f),
    m_fDecayTime(-1.0f),
    m_fTimedAlpha(1.0f),
    m_fDistanceAlpha(1.0f),
    m_fFadingDistance(-1.0f),
    m_fDepthBiasOffset(0.01f),
    m_uiSize(0),
    m_fRatio(1.0f),
    m_bRequiresUpdate(false),
    m_bUseTimer(false)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainDecal::~NiTerrainDecal()
{
    this->DetachChild(m_spMesh);
    m_spMesh = NULL;
    m_spTexture = NULL;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::Initialize(NiTexture* pkTexture, NiPoint3 kPosition,
    NiTerrain* pkTerrain, NiUInt32 uiSize, float fRatio,
    float fTimeOfDeath, float fDecayTime, float fDepthBiasOffset)
{
    // Configure the fade out and the texture
    m_fTimer = fTimeOfDeath;
    m_fRatio = fRatio;
    m_uiSize = uiSize;
    m_kPosition = kPosition;
    m_spTexture = pkTexture;
    m_fDecayTime = fDecayTime;
    m_fDepthBiasOffset = fDepthBiasOffset;

    if (m_fTimer > 0.0f)
        m_bUseTimer = true;

    // Initialize the mesh and the shape
    InitializeRegion(pkTerrain, kPosition, uiSize);
    BuildMesh(pkTerrain);

    this->Update(0.0f);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::InitializeRegion(NiTerrain* pkTerrain, NiPoint3 kPosition, NiUInt32 uiSize)
{
    // Work out the terrain space location of the position
    NiTransform kTransform = pkTerrain->GetWorldTransform();
    NiTransform kInverseTransform;
    kTransform.Invert(kInverseTransform);
    NiPoint3 kHmLocation = kInverseTransform * kPosition;

    // Origin is the center of the height-map.
    efd::UInt32 uiHeightMapSize = pkTerrain->GetCalcSectorSize() - 1;
    efd::Float32 fHeightMapCenter = uiHeightMapSize / 2.0f;
    efd::Float32 fHalfSize = float(uiSize) * 0.5f;

    m_kTerrainSpaceRegion.m_left = (efd::SInt32)(fHeightMapCenter + kHmLocation.x - fHalfSize);
    m_kTerrainSpaceRegion.m_right = m_kTerrainSpaceRegion.m_left + uiSize - 1;
    m_kTerrainSpaceRegion.m_bottom = (efd::SInt32)(fHeightMapCenter + kHmLocation.y - fHalfSize);
    m_kTerrainSpaceRegion.m_top = m_kTerrainSpaceRegion.m_bottom + uiSize - 1;
}

//--------------------------------------------------------------------------------------------------
NiUInt8 NiTerrainDecal::UpdateDecal(NiTerrain* pkTerrain, float fAccumTime)
{
    // Calculate the decal's decay according to time
    if (m_bUseTimer)
    {
        if (m_fTimer <= fAccumTime)
            return UpdateFlags::DECAL_EXPIRED;

        if (m_fDecayTime <= fAccumTime)
        {
            float fTimeTillDeath = m_fTimer - fAccumTime;
            if (fTimeTillDeath > 0)
            {
                m_fTimedAlpha = fTimeTillDeath / (m_fTimer - m_fDecayTime);
            }
        }
    }

    // Update the mesh (even it's shape if required)
    if (!m_bRequiresUpdate)
    {
        m_spMesh->Update(fAccumTime);
        return UpdateFlags::DECAL_UPDATED;
    }
    else
    {
        m_bRequiresUpdate = false;
        
        // Regenerate the decal's shape
        BuildMesh(pkTerrain);

        return UpdateFlags::DECAL_REBUILT;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::RequestUpdate()
{
    m_bRequiresUpdate = true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::SetDecayStartTime(float fDecayTime)
{
    m_fDecayTime = fDecayTime;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::SetFadingDistance(float fMaxDistance)
{
    m_fFadingDistance = fMaxDistance;
}

//--------------------------------------------------------------------------------------------------
NiBool NiTerrainDecal::GetRequiresUpdate()
{
    return m_bRequiresUpdate;
}

//--------------------------------------------------------------------------------------------------
NiMeshPtr NiTerrainDecal::GetMesh()
{
    return m_spMesh;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::AdjustToCamera(const NiCamera* pkCamera)
{
    CalculateAlphaDegeneration(pkCamera);
    UpdateZFightingOffset(pkCamera);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::CalculateAlphaDegeneration(const NiCamera* pkCamera)
{
    NiPoint3 kCamPos = pkCamera->GetWorldLocation();

    if (m_fFadingDistance < 0)
        return;

    float fDistanceFromDecal = (kCamPos - m_kPosition).Length();
    float fDifference = fDistanceFromDecal - m_fFadingDistance * 0.5f;

    if (fDifference <= m_fFadingDistance * 0.5f && fDistanceFromDecal >
        m_fFadingDistance/2)
    {
        m_fDistanceAlpha = 1.0f - fDifference / (m_fFadingDistance * 0.5f);
    }
    else if (fDifference > m_fFadingDistance * 0.5f)
    {
        m_fDistanceAlpha = 0.0f;
    }

    NiMaterialProperty* pkMaterial =
        NiDynamicCast(NiMaterialProperty,
        m_spMesh->GetProperty(NiMaterialProperty::GetType()));
    EE_ASSERT(pkMaterial);

    pkMaterial->SetAlpha(m_fDistanceAlpha * m_fTimedAlpha);
    m_spMesh->UpdateProperties();

}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::UpdateZFightingOffset(const NiCamera* pkCamera)
{
    NiPoint3 kDirection = pkCamera->GetWorldLocation() - this->GetTranslate();

    float fLength = kDirection.Length();
    float fRatio = (fLength - this->GetWorldBound().GetRadius());

    float fScale = NiClamp(this->GetScale(), 1.0f, 100.0f);
    if (fRatio <= 0)
    {
        kDirection = NiPoint3::UNIT_Z;
    }
    else
    {
        // We divide the ratio by the scale to give sensible step in the
        // position offset
        kDirection.z = NiClamp(fRatio / fScale, 1.0f, 10.0f);
        kDirection.x = 0.0f;
        kDirection.y = 0.0f;
    }

    SetTranslate(kDirection * m_fDepthBiasOffset);
    this->Update(0.0f);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::NotifyTerrainChanged(NiRect<efd::SInt32>& kTerrainRegion)
{
    // Work out the intersection of the two regions. 
    NiRect<efd::SInt32> kIntersection = kTerrainRegion;
    kIntersection.m_left = efd::Max(m_kTerrainSpaceRegion.m_left, kTerrainRegion.m_left);
    kIntersection.m_right = efd::Min(m_kTerrainSpaceRegion.m_right, kTerrainRegion.m_right);
    kIntersection.m_bottom = efd::Max(m_kTerrainSpaceRegion.m_bottom, kTerrainRegion.m_bottom);
    kIntersection.m_top = efd::Min(m_kTerrainSpaceRegion.m_top, kTerrainRegion.m_top);

    // If the intersection has a non zero area, then we should rebuild the mesh
    if ((kIntersection.m_right - kIntersection.m_left) >= 0 && 
        (kIntersection.m_top - kIntersection.m_bottom) >= 0)
    {
        RequestUpdate();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::BuildMesh(NiTerrain* pkTerrain)
{
    // Reset the mesh
    if (m_spMesh)
    {
        DetachChild(m_spMesh);
        m_spMesh = 0;
    }

    m_spMesh = NiNew NiMesh();
    m_spMesh->SetName("NiTerrainDecal");
    m_spMesh->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);
    m_spMesh->SetSubmeshCount(1);
    AttachChild(m_spMesh);

    // Work out the size of the streams:
    efd::SInt32 uiWidth = m_kTerrainSpaceRegion.GetWidth();
    efd::SInt32 uiHeight = m_kTerrainSpaceRegion.GetHeight();
    efd::UInt32 uiNumberOfIndices = (uiWidth * uiHeight) * 2 * 3;
    
    efd::SInt32 uiWidthInVerts = uiWidth + 1;
    efd::SInt32 uiHeightInVerts = uiHeight + 1;
    efd::UInt32 uiNumberOfVertices = (uiWidthInVerts) * (uiHeightInVerts);

    // Create the streams
    EE_ASSERT(uiNumberOfVertices < USHRT_MAX);

    // position stream
    NiDataStreamElementLock kPositionLock = m_spMesh->AddStreamGetLock(
        NiCommonSemantics::POSITION(), 0,
        NiDataStreamElement::F_FLOAT32_3,
        uiNumberOfVertices,
        NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_CPU_WRITE_STATIC | 
        NiDataStream::ACCESS_CPU_READ,
        NiDataStream::USAGE_VERTEX, false, true);

    // Texture stream
    NiDataStreamElementLock kUVLock = m_spMesh->AddStreamGetLock(
        NiCommonSemantics::TEXCOORD(), 0,
        NiDataStreamElement::F_FLOAT32_2,
        uiNumberOfVertices,
        NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_CPU_WRITE_STATIC,
        NiDataStream::USAGE_VERTEX, false, true);

    // Index stream
    NiDataStreamElementLock kIndexLock = m_spMesh->AddStreamGetLock(
        NiCommonSemantics::INDEX(), 0,
        NiDataStreamElement::F_UINT16_1,
        uiNumberOfIndices,
        NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_CPU_WRITE_STATIC,
        NiDataStream::USAGE_VERTEX_INDEX, false, true);

    // Create the iterators
    NiTStridedRandomAccessIterator<NiPoint3> kPositionIter = kPositionLock.begin<NiPoint3>(); 
    NiTStridedRandomAccessIterator<NiPoint2> kUVIter = kUVLock.begin<NiPoint2>();
    NiTStridedRandomAccessIterator<NiUInt16> kIndexIter = kIndexLock.begin<NiUInt16>();

    // Fetch the relevant heightmap from the terrain
    HeightMapBuffer kHeightmap;
    pkTerrain->GetHeightMap(m_kTerrainSpaceRegion, &kHeightmap);
    float fMinElevation = pkTerrain->GetMinHeight();
    float fMaxElevation = pkTerrain->GetMaxHeight();
    
    EE_ASSERT(kHeightmap.GetWidth() == efd::UInt32(uiWidthInVerts));
    EE_ASSERT(kHeightmap.GetHeight() == efd::UInt32(uiHeightInVerts));
    efd::UInt16* pusHeights = kHeightmap.GetBuffer();

    // Populate the position and texture streams
    efd::UInt32 uiHeightMapSize = pkTerrain->GetCalcSectorSize() - 1;
    NiPoint3 kHeightMapCenter(uiHeightMapSize / 2.0f, uiHeightMapSize / 2.0f, 0.0f);
    for (efd::UInt32 uiIndex = 0; uiIndex < uiNumberOfVertices; ++uiIndex)
    {
        efd::UInt32 uiXOffset = uiIndex % uiWidthInVerts;
        efd::UInt32 uiYOffset = uiIndex / uiWidthInVerts;

        // Calculate the position
        NiPoint3 kPos(
            float(m_kTerrainSpaceRegion.m_left + uiXOffset),
            float(m_kTerrainSpaceRegion.m_bottom + uiYOffset), 0.0f);
        
        // Convert the heightmap value into a valid height
        kPos.z = float(pusHeights[uiIndex]) / float(NiUInt16(-1));
        kPos.z = kPos.z * (fMaxElevation - fMinElevation) + fMinElevation;

        // Calculate the texture coordinates
        NiPoint2 kUV(
            float(uiXOffset) / float(uiWidthInVerts - 1),
            float(uiYOffset) / float(uiHeightInVerts - 1));

        // Store the values in the streams
        kPositionIter[uiIndex] = kPos - kHeightMapCenter;
        kUVIter[uiIndex] = kUV;
    }

    // Populate the index streams
    efd::UInt32 uiNumIndicesCreated = 0;
    for (efd::SInt32 iCurX = m_kTerrainSpaceRegion.m_left; 
        iCurX != m_kTerrainSpaceRegion.m_right; ++iCurX)
    {
        for (efd::SInt32 iCurY = m_kTerrainSpaceRegion.m_bottom; 
            iCurY != m_kTerrainSpaceRegion.m_top; ++iCurY)
        {
            // Work out the indices of the verts for this quad
            efd::UInt16 usVertIndexBottomLeft = efd::UInt16(
                (iCurY - m_kTerrainSpaceRegion.m_bottom) * uiWidthInVerts + 
                (iCurX - m_kTerrainSpaceRegion.m_left));
            efd::UInt16 usVertIndexBottomRight = usVertIndexBottomLeft + 1;
            efd::UInt16 usVertIndexTopLeft = usVertIndexBottomLeft + efd::UInt16(uiWidthInVerts);
            efd::UInt16 usVertIndexTopRight = usVertIndexTopLeft + 1;

            // Work out the tesselation direction for this quad
            // True = split diagonally from bottomLeft to topRight
            bool bTesselationDirection = ((iCurX % 2) ^ (iCurY % 2)) == 0;

            if (bTesselationDirection)
            {
                kIndexIter[uiNumIndicesCreated++] = usVertIndexTopRight;
                kIndexIter[uiNumIndicesCreated++] = usVertIndexTopLeft;
                kIndexIter[uiNumIndicesCreated++] = usVertIndexBottomLeft;

                kIndexIter[uiNumIndicesCreated++] = usVertIndexBottomLeft;
                kIndexIter[uiNumIndicesCreated++] = usVertIndexBottomRight;
                kIndexIter[uiNumIndicesCreated++] = usVertIndexTopRight;
            }
            else
            {
                kIndexIter[uiNumIndicesCreated++] = usVertIndexBottomRight;
                kIndexIter[uiNumIndicesCreated++] = usVertIndexTopLeft;
                kIndexIter[uiNumIndicesCreated++] = usVertIndexBottomLeft;

                kIndexIter[uiNumIndicesCreated++] = usVertIndexTopLeft;
                kIndexIter[uiNumIndicesCreated++] = usVertIndexBottomRight;
                kIndexIter[uiNumIndicesCreated++] = usVertIndexTopRight;
            }
        }
    }
    EE_ASSERT(uiNumIndicesCreated == uiNumberOfIndices);

    m_spMesh->RecomputeBounds();
    AttachProperties();
    m_spMesh->Update(0.0f);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDecal::AttachProperties()
{
    EE_ASSERT(m_spMesh);
    EE_ASSERT(m_spTexture);

    // Create the texturing property
    NiTexturingPropertyPtr spTextProp = NiNew NiTexturingProperty();
    spTextProp->SetBaseTexture(m_spTexture);
    spTextProp->SetBaseFilterMode(NiTexturingProperty::FILTER_BILERP);
    spTextProp->SetApplyMode(NiTexturingProperty::APPLY_DECAL);
    spTextProp->SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);

    // We then add an alpha property
    // create the alpha property
    NiAlphaPropertyPtr spAlpha = NiNew NiAlphaProperty();
    spAlpha->SetAlphaBlending(true);
    spAlpha->SetNoSorter(false);
    spAlpha->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
    spAlpha->SetDestBlendMode(NiAlphaProperty::ALPHA_INVSRCALPHA);

    NiMaterialPropertyPtr spMatProp = NiNew NiMaterialProperty();
    // Alpha is ignored, will need to make a modifier
    spMatProp->SetAlpha(m_fTimedAlpha * m_fDistanceAlpha);
    spMatProp->SetAmbientColor(NiColor(1.0f, 1.0f, 1.0f));
    spMatProp->SetDiffuseColor(NiColor(1.0f, 1.0f, 1.0f));
    spMatProp->SetSpecularColor(NiColor(1.0f, 1.0f, 1.0f));
    spMatProp->SetEmittance(NiColor(1.0f, 1.0f, 1.0f));
    spMatProp->SetShineness(10);

    // A Z buffer property to determine the order of drawing
    // create z buffer property
    NiZBufferPropertyPtr spZBuffer = NiNew NiZBufferProperty();
    spZBuffer->SetZBufferTest(true);
    spZBuffer->SetZBufferWrite(false);
    spZBuffer->SetTestFunction(NiZBufferProperty::TEST_LESSEQUAL);

    // Finally a color property
    NiVertexColorProperty *pkVertexColorProperty =
        NiNew NiVertexColorProperty();
    pkVertexColorProperty->SetSourceMode
        (NiVertexColorProperty::SOURCE_IGNORE);
    pkVertexColorProperty->SetLightingMode
        (NiVertexColorProperty::LIGHTING_E);

    // Attach the properties
    m_spMesh->AttachProperty(spTextProp);
    m_spMesh->AttachProperty(pkVertexColorProperty);
    m_spMesh->AttachProperty(spZBuffer);
    m_spMesh->AttachProperty(spAlpha);
    m_spMesh->AttachProperty(spMatProp);
}

//--------------------------------------------------------------------------------------------------
