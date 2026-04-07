// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiTerrainPCH.h"

#include "NiTerrainSector.h"
#include "NiTerrainDecal.h"
#include "NiTerrainCell.h"
#include "NiSurface.h"
#include "NiTerrain.h" 
#include "NiTerrainStreamLocks.h"
#include "NiTerrainCellShaderData.h"

#ifdef _WII
#include "NiWiiTerrainShader.h"
#endif

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainCell, NiNode);
//--------------------------------------------------------------------------------------------------
NiTerrainCell::NiTerrainCell(NiTerrainSector* pkSector, NiUInt32 uiLevel) : NiNode(NULL),
    m_kShaderData(pkSector->GetTerrain()),
    m_uiUpdateFlags(0),
    m_uiLevel(uiLevel),
    m_uiRegionID(0),
    m_uiCellID(0),
    m_uiDrawnPoolIndex(0),
    m_fMaxDistanceSqr(0),
    m_ucStitchingIndex(0),
    m_pkContainingSector(pkSector),
    m_pkSectorData(pkSector->GetSectorData()),
    m_pkDrawnPool(NULL),
    m_pkCustomPositionStream(0),
    m_pkCustomNormalTangentStream(0),
    m_pkCustomIndexStream(0),
    m_pkCustomUVStream(0),
    m_pkParentCell(NULL)
{
    RequestUpdate();

    // Set a zero bound, so that the default application doesn't try to render
    // all existing blocks
    m_kBound.SetCenterAndRadius(NiPoint3::ZERO, 0.0f);
}

//--------------------------------------------------------------------------------------------------
NiTerrainCell::~NiTerrainCell()
{ 
    // Remove all references to the mesh
    DestroyMesh();

    // If we are at the top of the tree, we are responsible for destroying
    // some cache objects.
    m_pkDrawnPool->ReleaseValue();
    if (m_pkParentCell == 0)
        NiDelete m_pkDrawnPool;
    else
        m_pkDrawnPool = 0;
}

//--------------------------------------------------------------------------------------------------
NiTerrainResourceManager* NiTerrainCell::GetResourceManager()
{
    return GetContainingSector()->GetResourceManager();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::SetBoundData(NiPoint3 kCenter, float fRadius)
{
    m_kBound.SetCenterAndRadius(kCenter, fRadius);
    m_kWorldBound = m_kBound;

    // Make sure the bounds on the mesh are up to date
    m_spMesh->SetModelBound(m_kBound);
    NiBound kWorldBound;
    kWorldBound.Update(m_kBound, GetWorldTransform());
    m_spMesh->SetWorldBound(kWorldBound);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::SectorIndexToLocal(const NiIndex& kSector, NiIndex& kLocal) const
{
    NiUInt32 uiSize = GetCellSize();
    
    const NiIndex& kIndexMin = m_kDataBottomLeftIndex;

    // Are we inside the correct range?
    if (kSector.x < kIndexMin.x ||
        kSector.y < kIndexMin.y)
    {
        return false;
    }

    NiIndex kIndexMax(
        kIndexMin.x + (uiSize << GetNumSubDivisions()),
        kIndexMin.y + (uiSize << GetNumSubDivisions()));
    if (kSector.x > kIndexMax.x || 
        kSector.y > kIndexMax.y)
    {
        return false;
    }

    // Spacing is always a power of two.
    NiUInt32 uiSpacing = 1 << GetNumSubDivisions();

    // Bitwise OR operation. If spacing is 32, and vertex is a multiple of
    // 32... then vertex & (31) == 0
    if ((kSector.x & (uiSpacing - 1)) != 0 || 
        (kSector.y & (uiSpacing - 1)) != 0)
    {
        return false;
    }
    
    // Ok, we are a valid index... but what IS our index?
    SectorIndexToLocalFast(kSector, kLocal);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::RequestUpdate()
{
    SetUpdateFlag(true, UPDATE_CELL_REQUIRED);
    if (!m_pkParentCell || m_pkParentCell->GetUpdateFlag(UPDATE_CELL_REQUIRED)) 
        return;
    else if (!GetUpdateFlag(UPDATE_CELL_JUST_LOADED))
        m_pkParentCell->RequestUpdate();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::RequiresUpdate() const
{
    return GetUpdateFlag(UPDATE_CELL_REQUIRED);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::RequestBoundsUpdate()
{
    SetUpdateFlag(true, UPDATE_CELL_BOUNDS);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::IsJustLoaded() const
{
    return GetUpdateFlag(UPDATE_CELL_JUST_LOADED);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::Update()
{
    // Update the cell with anything it needs from its parents
    if (GetUpdateFlag(UPDATE_CELL_JUST_LOADED))
        UpdateJustLoaded();

    // Update the bounding data for this cell
    if (GetUpdateFlag(UPDATE_CELL_BOUNDS))
        UpdateBounds();    

    // Update the shader data for this cell
    UpdateShaderData();
    
    SetUpdateFlag(false, UPDATE_CELL_REQUIRED);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::UpdateEffectsDownward(NiDynamicEffectState* pkParentState)
{
    NiDynamicEffectStatePtr spState 
        = PushLocalEffects(pkParentState, true);
    
    for (unsigned int i = 0; i < m_kChildren.GetSize(); i++)
    {
        NiAVObject* pkChild = m_kChildren.GetAt(i);

        if (pkChild)
        {
            if ((NiIsKindOf(NiTerrainCell, pkChild) && 
                m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel) + 1)) ||
                (NiIsKindOf(NiMesh, pkChild)))
            {
                pkChild->UpdateEffectsDownward(spState);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::UpdatePropertiesDownward(NiPropertyState* pkParentState)
{
    NiPropertyStatePtr spState 
        = PushLocalProperties(pkParentState, true);
    
    for (unsigned int i = 0; i < m_kChildren.GetSize(); i++)
    {
        NiAVObject* pkChild = m_kChildren.GetAt(i);

        if (pkChild)
        {
            if ((NiIsKindOf(NiTerrainCell, pkChild) && 
                m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel + 1))) ||
                (NiIsKindOf(NiMesh, pkChild)))
            {
                pkChild->UpdatePropertiesDownward(spState);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::UpdateBounds()
{
    // Calculate a bounding box to encapsulate this cell. 
    NiPoint3 kBoxMaxPoint;
    NiPoint3 kBoxMinPoint;
    NiTerrainStreamLocks kLock;
    NiPoint3 kPoint;
    NiTerrainPositionRandomAccessIterator kIter;
    NiUInt8 ucMask = NiDataStream::LOCK_READ | 
        NiDataStream::LOCK_TOOL_READ;
    NiIndex kMaxIndex(GetWidthInVerts() - 1, GetWidthInVerts() - 1);
    kLock.GetPositionIterator(this, ucMask, kIter);
    for (NiUInt32 y = 0; y <= kMaxIndex.y; ++y)
    {
        for (NiUInt32 x = 0; x <= kMaxIndex.x; ++x)
        {
            NiIndex kIndex(x, y);
            GetVertexAt(kIter, kPoint, kIndex);
        
            if (kIndex == NiIndex::ZERO)
            {
                // Prime the min/max positions with the first value. 
                // Set the Min XY values to this vertex
                kBoxMinPoint = kPoint;
                kBoxMaxPoint.z = kPoint.z;
            }
            else if (kIndex == kMaxIndex)
            {
                // Set the Max XY values to this vertex
                kBoxMaxPoint.x = kPoint.x;
                kBoxMaxPoint.y = kPoint.y;
            }
            
            // Calculate the extreme Z extents of this cell
            kBoxMaxPoint.z = NiMax(kBoxMaxPoint.z, kPoint.z);
            kBoxMinPoint.z = NiMin(kBoxMinPoint.z, kPoint.z);
        }
    }

    // Assign the bounding box
    NiPoint3 kCenter = (kBoxMaxPoint + kBoxMinPoint) / 2.0f;
    NiPoint3 kExtent = kBoxMaxPoint - kCenter;
    m_kBoxBound.SetCenter(kCenter);
    m_kBoxBound.SetExtent(0, kExtent.x);
    m_kBoxBound.SetExtent(1, kExtent.y);
    m_kBoxBound.SetExtent(2, kExtent.z);

    // Assign the bounding sphere
    m_kBound.SetCenter(kCenter);
    m_kBound.SetRadius(kExtent.Length());

    m_spMesh->SetModelBound(m_kBound);
    NiBound kWorldBound;
    kWorldBound.Update(m_kBound, GetWorldTransform());
    m_spMesh->SetWorldBound(kWorldBound);

    SetUpdateFlag(false, UPDATE_CELL_BOUNDS);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::UpdateShaderData()
{
    // Update the base texture if necessary
    NiTexturingProperty* pkTexProp = GetTexturingProperty();
    if (pkTexProp)
    {            
        // Apply the low detail diffuse texture
        NiTexture* pkTexture = m_kLowDetailTextureRegion.GetTexture();
        if (pkTexture)
        {
            NiTexturingProperty::Map* pkBaseMap = 
                NiNew NiTexturingProperty::Map(
                pkTexture, NiTexturingProperty::BASE_INDEX,
                NiTexturingProperty::CLAMP_S_CLAMP_T, 
                NiTexturingProperty::FILTER_BILERP);

            pkTexProp->SetBaseMap(pkBaseMap);
        }

        // Apply the low detail normal map
        pkTexture = m_spLowDetailNormalTexture;
        if (pkTexture)
        {
            NiTexturingProperty::Map* pkNormalMap = 
                NiNew NiTexturingProperty::Map(
                pkTexture, NiTexturingProperty::NORMAL_INDEX,
                NiTexturingProperty::CLAMP_S_CLAMP_T, 
                NiTexturingProperty::FILTER_BILERP);

            pkTexProp->SetNormalMap(pkNormalMap);
        }
        else
        {
            pkTexProp->SetNormalMap(NULL);
        }
        m_spMesh->UpdateProperties();
    }

    if (GetUpdateFlag(UPDATE_CELL_TEXTURE_REGIONS))
    {
        // update the texture extra data
        NiPoint2 kBlendOffset = m_kLowDetailTextureRegion.GetTextureOffset();
        float fLeafScale = m_kLowDetailTextureRegion.GetTextureScale();

        // Ensure that our shader has actually been created - for cases where
        // we are inheriting a mask
        bool bShaderCreated = CreateShaderData();
        if (!bShaderCreated)
            return;

        m_kShaderData.m_kLowDetailTextureOffset = kBlendOffset;
        m_kShaderData.m_kLowDetailTextureScale = NiPoint2(1.0f / fLeafScale, 1.0f / fLeafScale);

        // Set the size of these textures so the shader can offset accordingly
        NiUInt32 uiLDDiffuseTextureSize = 1;
        NiUInt32 uiLDNormalTextureSize = 1;
        
        if (m_kLowDetailTextureRegion.GetTexture())
            uiLDDiffuseTextureSize =  m_kLowDetailTextureRegion.GetTexture()->GetWidth();
        if (m_spLowDetailNormalTexture)
            uiLDNormalTextureSize = m_spLowDetailNormalTexture->GetWidth();

        m_kShaderData.m_kLowDetailTextureSize.x = (float)uiLDDiffuseTextureSize;
        m_kShaderData.m_kLowDetailTextureSize.y = (float)uiLDNormalTextureSize;
        m_kShaderData.UpdateShaderData(m_spMesh);
    }
    
    SetUpdateFlag(false, UPDATE_CELL_TEXTURE_REGIONS);
    m_kShaderData.UpdateShaderData(m_spMesh);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::UpdateJustLoaded()
{
    // Update the properties on this cell with those currently on the terrain
    NiPropertyStatePtr spPropertyState = 
        m_pkContainingSector->GetTerrain()->GetCachedPropertyState();
    if (!spPropertyState)
        spPropertyState = NiNew NiPropertyState;
    UpdatePropertiesDownward(spPropertyState);

    // Update the effects on this cell with those currently on the terrain
    NiDynamicEffectStatePtr spEffectState = 
        m_pkContainingSector->GetTerrain()->GetCachedEffectState();
    UpdateEffectsDownward(spEffectState);
    
    // Fetch the texture region data from the parent
    if (m_pkParentCell)
        UpdateTextureRegions();

    // Flag this cell as having had its initial update
    SetUpdateFlag(false, UPDATE_CELL_JUST_LOADED);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::UpdateTextureRegions()
{
    NiTerrainCellNode* pkNode = 
        NiDynamicCast(NiTerrainCellNode, m_pkParentCell);

    NiTextureRegion kParentDiffuseRegion = 
        pkNode->GetTextureRegion(TextureType::LOWDETAIL_DIFFUSE);   

    m_kLowDetailTextureRegion.SetTexture(
        pkNode->GetTexture(TextureType::LOWDETAIL_DIFFUSE));
    m_spLowDetailNormalTexture = 
        pkNode->GetTexture(TextureType::LOWDETAIL_NORMAL);
    
    // Adjust the scale and offset for the children
    float fScale = 2.0f * kParentDiffuseRegion.GetTextureScale();
    NiPoint2 kOffset = kParentDiffuseRegion.GetTextureOffset();
    const NiPoint2 kOffsetMultipliers[4] = {
        NiPoint2(0.0f, 1.0f),
        NiPoint2(1.0f, 1.0f),
        NiPoint2(1.0f, 0.0f),
        NiPoint2(0.0f, 0.0f)};
    float fOffsetScale  = 1.0f / (fScale);

    // Iterate over the children
    for (NiUInt32 ui = 0; ui < 4; ui++)
    {        
        // Fetch the child
        NiTerrainCell* pkChild = pkNode->GetChildAt(ui);
        if (pkChild != this)
        {
            continue;
        }

        // Calculate this child's offset values
        kOffset += kOffsetMultipliers[ui] * fOffsetScale;
        
        m_kLowDetailTextureRegion.SetRegion(kOffset, fScale);   
        break;
    }

    SetUpdateFlag(true, UPDATE_CELL_TEXTURE_REGIONS);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::ProcessLOD()
{
    if (!m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel)))
    {
        return false;
    }
    
    // Are we within the frustum? if not, do not even check our children.
    // Test for bounding sphere intersection with the view frustum.

    // Calculate our distances to each of the planes
    const NiBound* pkBound = &GetLocalBound();
    NiInt32 iWhichSide = 0;
    for (NiInt32 i = 0; i < NiFrustumPlanes::MAX_PLANES; ++i)
    {
        // find the distance to this plane - Note that the camera has already
        // been transformed into terrain coordinate space
        iWhichSide = pkBound->WhichSide(
            m_pkSectorData->GetFrustumPlanes().GetPlane(i));

        if (iWhichSide == NiPlane::NEGATIVE_SIDE)
        {
            SetCullingResult(FRUSTUM_CULLED);
            return false;
        }
    }
   
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::ProcessBorders()
{
    if (!m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel)))
    {
        return;
    }

    NiUInt8 ucMask = 0;
    const NiTerrainCell* pkCell;

    // We only need to process our borders if we were added to the visible set
    if (GetCullingResult() == SELF_VISIBLE)
    {
        // For each side:
        NiUInt8 ucBorder;
        for (NiUInt8 uc = 0; uc < 4; ++uc)
        {
            ucBorder = 1 << uc;

            // Look at the sibling block on this border. If that sibling was 
            // not drawn and none of its children were drawn, we need to
            // stitch
            pkCell = GetAdjacent(ucBorder);

            // Check if we need to stitch in this direction.
            bool bRequiresStitching = false;


            // Check if the sibling cell would have been included by LOD.
            if (pkCell && pkCell->GetCullingResult() == LOD_CULLED)
                bRequiresStitching = true;

            // Check if the sibling cell has not yet been loaded on the 
            // adjacent sector
            if (!pkCell && m_pkContainingSector->GetAdjacentSector(ucBorder))
                bRequiresStitching = true;

            // Apply stitching in this direction if required
            if(bRequiresStitching)
                ucMask |= ucBorder;
        }

        SetStitchingIndex(ucMask);
    }
    
    return;
}

//--------------------------------------------------------------------------------------------------
NiDataStream* NiTerrainCell::GetStream(StreamType::Value eStreamType)
{
    efd::UInt32 uiStreamIndex;
    switch(eStreamType)
    {
    case StreamType::INDEX:
        uiStreamIndex = NiTerrainCell::INDEX;
        break;
    case StreamType::TEXTURE_COORD:
        uiStreamIndex = NiTerrainCell::UV;
        break;
    case StreamType::POSITION:
        uiStreamIndex = NiTerrainCell::POSITION;
        break;
    case StreamType::NORMAL_TANGENT:
        uiStreamIndex = NiTerrainCell::NORMAL_TANGENT;
        break;
    default:
        return NULL;
    }

    if (!m_spMesh)
        return NULL;
    NiDataStreamRef* pkStreamRef = m_spMesh->GetStreamRefAt(uiStreamIndex);
    if (!pkStreamRef)
        return NULL;

    return pkStreamRef->GetDataStream();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::SetDynamicVertexStreams(NiDataStream* pkPositionStream, 
    NiDataStream* pkNormalTangentStream)
{
    // Fetch basic variables
    NiUInt32 uiBlockWidthInVerts = m_pkSectorData->GetCellWidthInVerts();
    NiUInt32 uiVertsPerBlock = uiBlockWidthInVerts * uiBlockWidthInVerts;

    // Position Stream:
    if (pkPositionStream)
    {
        // Create a region
        NiDataStream::Region kRegion;
        kRegion.SetStartIndex(0);
        kRegion.SetRange(uiVertsPerBlock);
        pkPositionStream->RemoveAllRegions();
        pkPositionStream->AddRegion(kRegion);

        // Verify that this new stream is suitable
        if (pkPositionStream->GetCount(0) != uiVertsPerBlock)
        {
            return false;
        }

        // Swap the streams around
        m_pkCustomPositionStream = pkPositionStream;

        // Attach the stream to the mesh?
        if (m_spMesh)
        {
            NiDataStreamRef* pkStreamRef = m_spMesh->GetStreamRefAt(POSITION);
            EE_ASSERT(pkStreamRef->GetSemanticNameAt(0).Equals(NiCommonSemantics::POSITION()));
            pkStreamRef->Reset();
            pkStreamRef->SetDataStream(pkPositionStream);
            pkStreamRef->BindSemanticToElementDescAt(0, NiCommonSemantics::POSITION(), 0);
            pkStreamRef->BindRegionToSubmesh(0, 0);
        }
    }
    
    // Normal/Tangent Stream:
    if (pkNormalTangentStream)
    {
        // Create a region
        NiDataStream::Region kRegion;
        kRegion.SetStartIndex(0);
        kRegion.SetRange(uiVertsPerBlock);
        pkNormalTangentStream->RemoveAllRegions();
        pkNormalTangentStream->AddRegion(kRegion);

        // Verify that this new stream is suitable
        if (pkNormalTangentStream->GetCount(0) != uiVertsPerBlock)
        {
            return false;
        }
    
        // Swap the streams around
        m_pkCustomNormalTangentStream = pkNormalTangentStream;

        // Attach the stream to the mesh
        if (m_spMesh)
        {
            NiDataStreamRef* pkStreamRef = m_spMesh->GetStreamRefAt(NORMAL_TANGENT);
            EE_ASSERT(pkStreamRef->GetSemanticNameAt(0).Equals(NiCommonSemantics::NORMAL()));
            pkStreamRef->Reset();
            pkStreamRef->SetDataStream(pkNormalTangentStream);
            pkStreamRef->BindSemanticToElementDescAt(0, NiCommonSemantics::NORMAL(), 0);
            
            if (GetConfiguration().IsTangentDataEnabled())
            {
                pkStreamRef->BindSemanticToElementDescAt(1, NiCommonSemantics::TANGENT(), 0);
            }

            pkStreamRef->BindRegionToSubmesh(0, 0);
        }
    }
    
    RequestUpdate();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::SetDynamicIndexStream(NiDataStream* pkIndexStream)
{
    EE_UNUSED_ARG(pkIndexStream);
    // This will create a clone (copy, not reference) of the static index
    // stream, so that triangles can be flipped?

    // When a terrain is saved out, maybe save this information somehow so that
    // it can be put in the standard index stream when loaded again
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::SetDynamicUVStream(NiDataStream* pkUVStream)
{
    EE_UNUSED_ARG(pkUVStream);
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::RemoveDynamicVertexStreams()
{
    EE_ASSERT(m_pkSectorData->GetStaticPositionStream(m_uiLevel));        
    EE_ASSERT(m_pkSectorData->GetStaticNormalTangentStream(m_uiLevel));

    m_pkCustomPositionStream = 0;
    m_pkCustomNormalTangentStream = 0;

    if (!m_spMesh)
        return;
    
    // Now we need to re-bind our mesh to the streams
    NiDataStreamRef* pkStreamRef;

    // Position StreamRef
    pkStreamRef = m_spMesh->GetStreamRefAt(POSITION);
    EE_ASSERT(pkStreamRef->GetSemanticNameAt(0).Equals(NiCommonSemantics::POSITION()));
    pkStreamRef->Reset();
    pkStreamRef->SetDataStream(m_pkSectorData->GetStaticPositionStream(m_uiLevel));
    pkStreamRef->BindSemanticToElementDescAt(0, NiCommonSemantics::POSITION(), 0);
    pkStreamRef->BindRegionToSubmesh(0, m_uiRegionID);

    // Tangent and normal streamrefs
    pkStreamRef = m_spMesh->GetStreamRefAt(NORMAL_TANGENT);
    EE_ASSERT(pkStreamRef->GetSemanticNameAt(0).Equals(NiCommonSemantics::NORMAL()));
    pkStreamRef->Reset();
    pkStreamRef->SetDataStream(m_pkSectorData->GetStaticNormalTangentStream(m_uiLevel));
    pkStreamRef->BindSemanticToElementDescAt(0, NiCommonSemantics::NORMAL(), 0);
    if (GetConfiguration().IsTangentDataEnabled())
    {
        pkStreamRef->BindSemanticToElementDescAt(1, NiCommonSemantics::TANGENT(), 0);
    }
    pkStreamRef->BindRegionToSubmesh(0, m_uiRegionID);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::RemoveDynamicIndexStream()
{
    EE_ASSERT(m_pkSectorData->GetStaticIndexStream());

    m_pkCustomIndexStream = 0;

    if (!m_spMesh)
        return;
    
    // Now we need to re-bind our mesh to the static stream
    NiDataStreamRef* pkStreamRef;

    // Index StreamRef
    pkStreamRef = m_spMesh->GetStreamRefAt(2);
    EE_ASSERT(pkStreamRef->GetSemanticNameAt(0).Equals(
        NiCommonSemantics::INDEX()));
    pkStreamRef->Reset();
    pkStreamRef->SetDataStream(m_pkSectorData->GetStaticIndexStream());
    pkStreamRef->BindSemanticToElementDescAt(
        0, NiCommonSemantics::INDEX(), 0);
    pkStreamRef->BindRegionToSubmesh(0, 0);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::RemoveDynamicUVStream()
{
    EE_ASSERT(m_pkSectorData->GetStaticUVStream());

    m_pkCustomUVStream = 0;

    if (!m_spMesh)
        return;
    
    // Now we need to re-bind our mesh to the static stream
    NiDataStreamRef* pkStreamRef;

    // UV StreamRef
    pkStreamRef = m_spMesh->GetStreamRefAt(3);
    EE_ASSERT(pkStreamRef->GetSemanticNameAt(0).Equals(
        NiCommonSemantics::TEXCOORD()));
    pkStreamRef->Reset();
    pkStreamRef->SetDataStream(m_pkSectorData->GetStaticUVStream());
    pkStreamRef->BindSemanticToElementDescAt(
        0, NiCommonSemantics::TEXCOORD(), 0);
    pkStreamRef->BindRegionToSubmesh(0, 0);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::CreateMesh()
{
    // Create the dynamic streams before creating the mesh for the tool mode
    if (NiTerrain::InToolMode())
    {
        NiDataStream* pkPositionStream;
        NiDataStream* pkNormalTangentStream;
        
        pkPositionStream = m_pkSectorData->GetDynamicStreamCache()->
            RequestStream(NiTerrain::StreamType::POSITION);
        pkNormalTangentStream = m_pkSectorData->GetDynamicStreamCache()->
            RequestStream(NiTerrain::StreamType::NORMAL_TANGENT);

        EE_VERIFY(
            SetDynamicVertexStreams(pkPositionStream, pkNormalTangentStream));
    }

    EE_ASSERT(!m_spMesh);

    m_spMesh = NiNew NiMesh();

    // Attach only to the node children list
    AttachChild(m_spMesh);

    // Assign ourselves a draw pool unique index 
    // (Allocate here so that the indicies are in Level order)
    m_uiDrawnPoolIndex = m_pkDrawnPool->GetNew();
    
#ifdef NIDEBUG    
    // Give the mesh a distinguishable name to assist in debugging:
    NiString kName("");

    NiIndex kIndex;
    GetBottomLeftIndex(kIndex);

    kName.Format("Block {ID %d, X %d, Y %d, L %d}", 
        m_uiCellID, 
        kIndex.x /(m_pkSectorData->GetCellSize() << GetNumSubDivisions()),
        kIndex.y /(m_pkSectorData->GetCellSize() << GetNumSubDivisions()),
        GetNumSubDivisions());

    NiFixedString kFixedName(kName);

    SetName(kFixedName);
    m_spMesh->SetName(kFixedName);
#endif

    m_spMesh->SetSubmeshCount(1);
    m_spMesh->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);

    NiDataStreamRef* pkStreamRef;
    // Index StreamRef
    EE_ASSERT(m_spMesh->GetStreamRefCount() == INDEX);
    pkStreamRef = m_spMesh->AddStreamRef();
    pkStreamRef->SetDataStream(m_pkSectorData->GetStaticIndexStream());
    pkStreamRef->BindSemanticToElementDescAt(
        0, NiCommonSemantics::INDEX(), 0);
    pkStreamRef->BindRegionToSubmesh(0, 0);

    // UV StreamRef
    EE_ASSERT(m_spMesh->GetStreamRefCount() == UV);
    pkStreamRef = m_spMesh->AddStreamRef();
    pkStreamRef->SetDataStream(m_pkSectorData->GetStaticUVStream());
    pkStreamRef->BindSemanticToElementDescAt(
        0, NiCommonSemantics::TEXCOORD(), 0);
    pkStreamRef->BindRegionToSubmesh(0, 0);

    // Geometry StreamRefs
    if (HasDynamicVertexStreams())
    {
        EE_ASSERT(m_spMesh->GetStreamRefCount() == POSITION);
        pkStreamRef = m_spMesh->AddStreamRef();
        pkStreamRef->SetDataStream(m_pkCustomPositionStream);
        pkStreamRef->BindSemanticToElementDescAt(0, NiCommonSemantics::POSITION(), 0);
        pkStreamRef->BindRegionToSubmesh(0, 0);
        
        EE_ASSERT(m_spMesh->GetStreamRefCount() == NORMAL_TANGENT);
        pkStreamRef = m_spMesh->AddStreamRef();
        pkStreamRef->SetDataStream(m_pkCustomNormalTangentStream);
        pkStreamRef->BindSemanticToElementDescAt(0, NiCommonSemantics::NORMAL(), 0);
        if (GetConfiguration().IsTangentDataEnabled())
        {
            pkStreamRef->BindSemanticToElementDescAt(1, 
                NiCommonSemantics::TANGENT(), 0);
        }
        pkStreamRef->BindRegionToSubmesh(0, 0);
    }
    else
    {
        NiUInt32 uiDetailLevel = m_uiLevel;
        EE_ASSERT(m_pkSectorData->GetStaticPositionStream(uiDetailLevel));        

        // Position StreamRef
        EE_ASSERT(m_spMesh->GetStreamRefCount() == POSITION);
        pkStreamRef = m_spMesh->AddStreamRef();
        pkStreamRef->SetDataStream(m_pkSectorData->GetStaticPositionStream(uiDetailLevel));
        pkStreamRef->BindSemanticToElementDescAt(0, NiCommonSemantics::POSITION(), 0);
        pkStreamRef->BindRegionToSubmesh(0, m_uiRegionID);

        // Tangent and normal streamrefs
        NiDataStream* pkNormalTangentStream = 
            m_pkSectorData->GetStaticNormalTangentStream(uiDetailLevel);
        if (pkNormalTangentStream)
        {
            EE_ASSERT(m_spMesh->GetStreamRefCount() == NORMAL_TANGENT);
            pkStreamRef = m_spMesh->AddStreamRef();
            pkStreamRef->SetDataStream(pkNormalTangentStream);
            pkStreamRef->BindSemanticToElementDescAt(0, NiCommonSemantics::NORMAL(), 0);
            if (GetConfiguration().IsTangentDataEnabled() && 
                uiDetailLevel == m_pkSectorData->GetNumLOD())
            {
                pkStreamRef->BindSemanticToElementDescAt(1, NiCommonSemantics::TANGENT(), 0);
            }
            pkStreamRef->BindRegionToSubmesh(0, m_uiRegionID);
        }
    }

    CreateShaderData();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::DestroyMesh()
{
    EE_ASSERT(m_spMesh);
    DetachChild(m_spMesh);
    m_spMesh = NULL; 
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::SetTextureRegion(NiPoint2 kOffset, float fScale, 
    NiTerrainCell::TextureType::Value eTexType)
{
    if (eTexType == TextureType::LOWDETAIL_DIFFUSE ||
        eTexType == TextureType::LOWDETAIL_NORMAL)
    {
        m_kLowDetailTextureRegion.SetRegion(kOffset, fScale);
    }

    SetUpdateFlag(true, UPDATE_CELL_TEXTURE_REGIONS);
    RequestUpdate();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::SetTexture(NiTexture* pkTexture, 
    NiTerrainCell::TextureType::Value eTexType)
{
    if (eTexType == TextureType::LOWDETAIL_DIFFUSE)
    {
        m_kLowDetailTextureRegion.SetTexture(pkTexture);
    }
    if (eTexType == TextureType::LOWDETAIL_NORMAL)
    {
        m_spLowDetailNormalTexture = pkTexture;
    }

    SetUpdateFlag(true, UPDATE_CELL_TEXTURE_REGIONS);
    RequestUpdate();
}

//--------------------------------------------------------------------------------------------------
NiTexture* NiTerrainCell::GetTexture(NiTerrainCell::TextureType::Value eTexType)
{
    if (eTexType == TextureType::LOWDETAIL_DIFFUSE)
    {
        return m_kLowDetailTextureRegion.GetTexture();
    }
    if (eTexType == TextureType::LOWDETAIL_NORMAL)
    {
        return m_spLowDetailNormalTexture;
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiTexturingProperty* NiTerrainCell::GetTexturingProperty()
{
    NiTexturingProperty* pkProperty = 
        (NiTexturingProperty*)m_spMesh->GetProperty(
            NiTexturingProperty::GetType());

    if (!pkProperty)
    {
        EE_VERIFY(CreateShaderData());
        pkProperty = (NiTexturingProperty*)m_spMesh->GetProperty(
            NiTexturingProperty::GetType());

        EE_ASSERT(pkProperty);
    }

    return pkProperty;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::UpdateMorphConstants()
{
    // Calculate the distance for this cell to be culled
    m_fMaxDistanceSqr = GetContainingSector()->GetTerrain()->GetCellRange(GetLevel());

    // Reset the morphing data in the shader.
    const float fSquareRoot2 = NiSqrt(2.0f);
    NiUInt32 uiNumDivisions = GetNumSubDivisions();
    float fCellWorldWidth = float(m_pkSectorData->GetCellSize() << uiNumDivisions);
    float fMorphDistance = fSquareRoot2 * (fCellWorldWidth * 0.5f);
    float fThresholdDistance = NiSqrt(m_fMaxDistanceSqr) - fMorphDistance;

    m_kShaderData.m_fMorphThreshold = fThresholdDistance;
    m_kShaderData.m_fMorphDistance = fMorphDistance;
    m_kShaderData.m_uiMorphMode = m_pkSectorData->GetLODMode();
    m_kShaderData.UpdateShaderData(m_spMesh);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::IsInRange()
{
    // Check if this LOD has even been loaded yet
    if (!m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel)))
    {
        return false;
    }

    // Fetch the relevant values
    const NiPoint3& kCameraLocation = m_pkSectorData->GetLODCamera()->GetWorldLocation();
    const NiPoint3& kCellCenter = GetLocalBound().GetCenter();

    // Perform the range test
    return GetContainingSector()->GetTerrain()->IsCellInRange(m_fMaxDistanceSqr, GetLevel(), 
        kCameraLocation, kCellCenter);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::AddCellToVisible(NiTerrainCell* pkCell)
{
    pkCell->AddToVisible();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::IsCellVisible(NiTerrainCell* pkCell)
{
    return pkCell->IsVisible();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCell::CreateShaderData()
{
    EE_ASSERT(m_spMesh);

    if (m_spMesh->GetProperty(NiTexturingProperty::TEXTURING) == NULL)
    {
        // Create a new texturing property 
        NiTexturingProperty* pkTextureProp = NiNew NiTexturingProperty();

        // Finally, attach the property to the mesh
        m_spMesh->AttachProperty(pkTextureProp);
    }
    
    // Select and assign the appropriate material
    NiMaterial* pkMaterial = m_pkContainingSector->GetTerrain()->GetMaterial();
    EE_ASSERT(pkMaterial);  
    m_spMesh->ApplyAndSetActiveMaterial(pkMaterial);

    if (!m_spMesh->GetExtraDataSize())
        m_kShaderData.InitializeShaderData(m_spMesh);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::SetStitchingIndex(NiUInt8 ucStitchingIndex)
{
    m_ucStitchingIndex = ucStitchingIndex;
    NiDataStreamRef* pkIndexStream = m_spMesh->GetStreamRefAt(INDEX);
    EE_ASSERT(pkIndexStream->GetSemanticNameAt(0).Equals(
        NiCommonSemantics::INDEX()));

    pkIndexStream->BindRegionToSubmesh(
        0, *m_pkSectorData->GetIndexRegionByStitchIndex(ucStitchingIndex));

    // Update the stitching information for the shader:
    if (!(m_pkSectorData->GetLODMode() & NiTerrainSectorData::LOD_MORPH_ENABLE))
    {
        NiIndex kBottomLeftIndex;
        GetBottomLeftIndex(kBottomLeftIndex);

        NiPoint2 kMinXY((float)kBottomLeftIndex.x, (float)kBottomLeftIndex.y);
        kMinXY.x -= m_pkSectorData->GetSectorSize() >> 1;
        kMinXY.y -= m_pkSectorData->GetSectorSize() >> 1;
        NiPoint2 kMaxXY = kMinXY;
        NiUInt32 uiSubDivisions = GetNumSubDivisions();
        kMaxXY.x += GetCellSize() << uiSubDivisions;           
        kMaxXY.y += GetCellSize() << uiSubDivisions;

        // Shift according to the borders sets
        if (ucStitchingIndex & NiTerrainCell::BORDER_LEFT)
            kMinXY.x ++;
        else if (ucStitchingIndex & NiTerrainCell::BORDER_RIGHT)
            kMaxXY.x --;
        if (ucStitchingIndex & NiTerrainCell::BORDER_BOTTOM)
            kMinXY.y ++;
        else if (ucStitchingIndex & NiTerrainCell::BORDER_TOP)
            kMaxXY.y --;

        EE_ASSERT(kMinXY.x < kMaxXY.x && kMinXY.y < kMaxXY.y);
        m_kShaderData.m_kStitchingInfo = NiPoint4(kMinXY.x, kMinXY.y, kMaxXY.x, kMaxXY.y);
    }

    m_kShaderData.UpdateShaderData(m_spMesh);
}

//--------------------------------------------------------------------------------------------------
const NiTerrainConfiguration& NiTerrainCell::GetConfiguration() const
{
    return m_pkContainingSector->GetConfiguration();
}

//--------------------------------------------------------------------------------------------------
NiTerrainCell* NiTerrainCell::DoGetAdjacent(NiUInt32 uiBorder) const
{
    NiIndex kIndex;
    GetBottomLeftIndex(kIndex);
    NiUInt32 uiIncrement = m_pkSectorData->GetCellSize() << GetNumSubDivisions();
    NiUInt32 uiMaxIndex = m_pkSectorData->GetSectorSize() - uiIncrement;
    NiUInt32 uiWidthInBlocks = 1 << GetLevel();
    
    const NiTerrainSector *pkSector = GetContainingSector();
    
    NiUInt32 uiSectorBorder = 0;
    if (uiBorder & BORDER_LEFT && kIndex.x == 0)
    {
        uiSectorBorder |= BORDER_LEFT;
        uiBorder ^= BORDER_LEFT;
        kIndex.x += uiMaxIndex;
    }
    if (uiBorder & BORDER_RIGHT && kIndex.x >= uiMaxIndex)
    {
        uiSectorBorder |= BORDER_RIGHT;
        uiBorder ^= BORDER_RIGHT;
        kIndex.x -= uiMaxIndex;
    }
    if (uiBorder & BORDER_TOP && kIndex.y >= uiMaxIndex)
    {
        uiSectorBorder |= BORDER_TOP;
        uiBorder ^= BORDER_TOP;
        kIndex.y -= uiMaxIndex;
    }
    if (uiBorder & BORDER_BOTTOM && kIndex.y == 0)
    {
        uiSectorBorder |= BORDER_BOTTOM;
        uiBorder ^= BORDER_BOTTOM;
        kIndex.y += uiMaxIndex;
    }

    if (uiSectorBorder)
    {
        pkSector = pkSector->GetAdjacentSector(uiSectorBorder);
        if (!pkSector)
        {
            return NULL;
        }
    }   

    NiInt32 iLevel = GetLevel();
    if (pkSector->GetSectorData()->GetHighestLoadedLOD() < iLevel)
    {
        return NULL;
    }

    kIndex /= uiIncrement;
    if (uiBorder & BORDER_LEFT)
        kIndex.x--;
    else if (uiBorder & BORDER_RIGHT)
        kIndex.x++;   
    if (uiBorder & BORDER_TOP)
        kIndex.y++;
    else if (uiBorder & BORDER_BOTTOM)
        kIndex.y--;

    NiUInt32 uiBlockID = pkSector->GetCellOffset(iLevel) +
        kIndex.x + kIndex.y * uiWidthInBlocks;

    return pkSector->GetCell(uiBlockID);
}

//--------------------------------------------------------------------------------------------------
NiTerrainCell* NiTerrainCell::DoGetAdjacentFast(NiUInt32 uiBorder) 
    const
{
    NiIndex kIndex;
    GetBottomLeftIndex(kIndex);
    NiUInt32 uiIncrement = m_pkSectorData->GetCellSize() << GetNumSubDivisions();
    NiUInt32 uiMaxIndex = m_pkSectorData->GetSectorSize() - uiIncrement;
    NiUInt32 usWidthInBlocks = 1 << GetLevel();

    const NiTerrainSector *pkSector = GetContainingSector();

    NiUInt32 uiSectorBorder = 0;
    if (uiBorder & BORDER_LEFT && kIndex.x == 0)
    {
        uiSectorBorder |= BORDER_LEFT;
        uiBorder ^= BORDER_LEFT;
        kIndex.x += uiMaxIndex;
    }
    if (uiBorder & BORDER_RIGHT && kIndex.x >= uiMaxIndex)
    {
        uiSectorBorder |= BORDER_RIGHT;
        uiBorder ^= BORDER_RIGHT;
        kIndex.x -= uiMaxIndex;
    }
    if (uiBorder & BORDER_TOP && kIndex.y >= uiMaxIndex)
    {
        uiSectorBorder |= BORDER_TOP;
        uiBorder ^= BORDER_TOP;
        kIndex.y -= uiMaxIndex;
    }
    if (uiBorder & BORDER_BOTTOM && kIndex.y == 0)
    {
        uiSectorBorder |= BORDER_BOTTOM;
        uiBorder ^= BORDER_BOTTOM;
        kIndex.y += uiMaxIndex;
    }

    if (uiSectorBorder)
    {
        pkSector = pkSector->GetAdjacentSector(uiSectorBorder);
        if (pkSector == 0)
        {
            return 0;
        }
    }

    kIndex /= uiIncrement;
    if (uiBorder & BORDER_LEFT)
        kIndex.x--;
    else if (uiBorder & BORDER_RIGHT)
        kIndex.x++;   
    if (uiBorder & BORDER_TOP)
        kIndex.y++;
    else if (uiBorder & BORDER_BOTTOM)
        kIndex.y--;

    NiUInt32 uiBlockID = pkSector->GetCellOffset(GetLevel()) +
        kIndex.x + kIndex.y * usWidthInBlocks;

    return pkSector->GetCell(uiBlockID);
}

//--------------------------------------------------------------------------------------------------
const NiTextureRegion& NiTerrainCell::GetTextureRegion(
    NiTerrainCell::TextureType::Value eType) const
{
    EE_UNUSED_ARG(eType);
    return m_kLowDetailTextureRegion;
}

//--------------------------------------------------------------------------------------------------
NiTextureRegion& NiTerrainCell::GetTextureRegion(
    NiTerrainCell::TextureType::Value eType)
{
    EE_UNUSED_ARG(eType);
    return m_kLowDetailTextureRegion;
}

//--------------------------------------------------------------------------------------------------
NiUInt8 NiTerrainCell::GetPixelAt(NiTerrainCell::TextureType::Value eType, 
    NiIndex kCoordinates, NiUInt32 uiComponent) const
{
    EE_UNUSED_ARG(eType);
    EE_UNUSED_ARG(kCoordinates);
    EE_UNUSED_ARG(uiComponent);

    return 0;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCell::SetPixelAt(NiTerrainCell::TextureType::Value eType, 
    NiIndex kCoordinates, NiUInt32 uiComponent, NiUInt8 ucNewValue)
{
    EE_UNUSED_ARG(eType);
    EE_UNUSED_ARG(kCoordinates);
    EE_UNUSED_ARG(uiComponent);
    EE_UNUSED_ARG(ucNewValue);
}

//--------------------------------------------------------------------------------------------------