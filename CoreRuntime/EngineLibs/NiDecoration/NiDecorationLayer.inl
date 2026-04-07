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
inline NiUInt32 NiDecorationLayer::GetNumCellsX() const
{
    return m_auiNumCells[0];
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationLayer::GetNumCellsY() const
{
    return m_auiNumCells[1];
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationLayer::GetMaxRange() const
{
    return m_fMaxRange;
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationLayer::GetMinRange() const
{
    return m_fMinRange;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationLayer::SetFarFadeDistance(float fDistance)
{
    m_fFarFadeDistance = fDistance;
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationLayer::GetFarFadeDistance() const
{
    return m_fFarFadeDistance;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationLayer::SetNearFadeDistance(float fDistance)
{
    m_fNearFadeDistance = fDistance;
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationLayer::GetNearFadeDistance() const
{
    return m_fNearFadeDistance;
}

//------------------------------------------------------------------------------------------------
inline const NiPoint2& NiDecorationLayer::GetDimensions() const
{
    return m_kDimensions;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationLayer::GetBaseSeed() const
{
    return m_uiBaseSeed;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationLayer::SetBaseSeed(NiUInt32 uiBaseSeed)
{
    m_uiBaseSeed = uiBaseSeed;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationLayer::GetMaxCellsGeneratedPerFrame() const
{
    return m_uiMaxCellGenerationsPerCall;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationLayer::SetMaxCellsGeneratedPerFrame(NiUInt32 uiMaxCells)
{
    m_uiMaxCellGenerationsPerCall = uiMaxCells;
}

//------------------------------------------------------------------------------------------------
inline NiCamera* NiDecorationLayer::GetCamera() const
{
    return m_spCamera;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationLayer::SetCamera(NiCamera* pkCamera)
{
    m_spCamera = pkCamera;
    ResetCells();

    if (pkCamera)
        m_kLastCameraTransform = pkCamera->GetWorldTransform();
}

//------------------------------------------------------------------------------------------------
inline NiDecorationGenerator* NiDecorationLayer::GetGenerator() const
{
    return m_spGenerator;
}

//------------------------------------------------------------------------------------------------
inline NiDecorationMeshInfo* NiDecorationLayer::GetBaseMesh() const
{
    return m_spBaseMesh;
}
//
////------------------------------------------------------------------------------------------------
//inline void NiDecorationLayer::RequestCellUpload(NiDecorationCell* pkCell)
//{
//    NiUInt32 uiCellID;
//    EE_VERIFY(GetInstancePool()->FindCellID(pkCell, uiCellID));
//
//    // TODO: 
//    //EE_ASSERT(pkCell->m_uiPoolID == GetInstancePoolID());
//
//    RequestCellUpload(uiCellID);
//}
//
////------------------------------------------------------------------------------------------------
//inline void NiDecorationLayer::RequestCellUpload(NiUInt32 uiCellID)
//{
//    EE_ASSERT(uiCellID < GetInstancePool()->GetNumMaxCells());
//    m_kCellsToUpload.Add(uiCellID);
//}

//------------------------------------------------------------------------------------------------
inline NiTransform NiDecorationLayer::WorldToLocal(const NiTransform& kWorldTransform) const
{
    NiTransform kLocal;
    NiMatrix3 kInverseRotate = GetWorldRotate().Inverse();

    kLocal.m_Translate = kInverseRotate * ((kWorldTransform.m_Translate - GetWorldTranslate()) / 
        GetWorldScale());
    kLocal.m_Rotate = kInverseRotate * kWorldTransform.m_Rotate;
    kLocal.m_fScale = kWorldTransform.m_fScale / GetWorldScale();

    return kLocal;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationLayer::SetFunctorList(
    const FunctorList& kFunctorSet)
{
    RemoveFunctorsFromMesh();

    m_kFunctors = kFunctorSet;

    ApplyFunctorsToMesh();
}

//------------------------------------------------------------------------------------------------
inline const NiDecorationLayer::FunctorList& NiDecorationLayer::GetFunctorList() const
{
    return m_kFunctors;
}

//------------------------------------------------------------------------------------------------
