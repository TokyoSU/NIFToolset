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
inline NiDecorationFunctorBase::NiDecorationFunctorBase()
{
}

//------------------------------------------------------------------------------------------------
inline NiDecorationFunctorBase::~NiDecorationFunctorBase()
{
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline NiTDecorationFunctor<T>::NiTDecorationFunctor() : m_pkTarget(NULL)
{
    T::InitializeExtraData(m_kExtraData);
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline NiTDecorationFunctor<T>::NiTDecorationFunctor(
    typename T::target_type* pkTarget) : m_pkTarget(pkTarget)
{
    EE_ASSERT(NiTDecorationFunctor<T>::IsValidTargetType(pkTarget));
    T::InitializeExtraData(m_kExtraData);
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline NiTDecorationFunctor<T>::~NiTDecorationFunctor()
{
    NiUInt32 uiNumExtraData = m_kExtraData.GetSize();
    for (NiUInt32 ui = 0; ui < uiNumExtraData; ++ui)
        NiDelete m_kExtraData.GetAt(ui);

    m_kExtraData.RemoveAll();
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline void NiTDecorationFunctor<T>::SetTarget(NiObject* pkTarget)
{
    EE_ASSERT(NiTDecorationFunctor<T>::IsValidTargetType(pkTarget));
    m_pkTarget = (typename T::target_type*)pkTarget;
}

//---------------------------------------------------------------------------
template <class T> 
inline NiObject* NiTDecorationFunctor<T>::GetTarget() const
{
    return m_pkTarget;
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline bool NiTDecorationFunctor<T>::IsValidTargetType(NiObject* pkTarget)
{
    return T::IsValidTargetType(pkTarget);
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline bool NiTDecorationFunctor<T>::Validate(
    const NiUInt32 auiCellCount[2],
    const NiPoint2& kCellRange,
    const NiTransform& kLayerWorldTransform)
{
    return m_pkTarget && T::Validate(m_pkTarget, m_kExtraData,
        auiCellCount, kCellRange,
        kLayerWorldTransform);
}

//------------------------------------------------------------------------------------------------
template <class T>
inline void NiTDecorationFunctor<T>::ConfigureMesh(const NiUInt32 auiCellCount[2], 
    const NiPoint2& kCellRange,
    NiUInt32 uiFieldIndex, 
    const NiTransform& kLayerWorldTransform, 
    NiAVObject* pkBase) const
{
    T::ConfigureMesh(m_kExtraData, auiCellCount, kCellRange, uiFieldIndex, kLayerWorldTransform, 
        m_pkTarget, pkBase);
}

//------------------------------------------------------------------------------------------------
template <class T> inline NiTPrimitiveArray<NiExtraData*>& NiTDecorationFunctor<T>::GetExtraData()
{
    return m_kExtraData;
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline bool NiTDecorationFunctor<T>::GenerateTransforms(const NiUInt32 auiCellCount[2], 
    NiDecorationCell* pkCell, 
    NiTransform* pkTransforms, 
    NiUInt32 uiTransformCount, 
    const NiPoint2& kRange, 
    NiUInt32 uiFieldIndex,
    const NiTransform& kWorldTransform, 
    NiRandomLCG* pkRandom)
{
    const NiUInt32 auiIndex[] = {pkCell->m_uiIndexX, pkCell->m_uiIndexY};
    return T::GenerateTransforms(m_pkTarget, m_kExtraData,
        pkTransforms, uiTransformCount,
        auiCellCount, auiIndex,
        pkCell->m_kLocalTransform.m_Translate, kRange, uiFieldIndex,
        kWorldTransform, pkRandom);
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline NiObject* NiTDecorationFunctor<T>::CreateClone(NiCloningProcess& kCloning)
{
    NiTDecorationFunctor<T>* pkObject;
    typename T::target_type* pTarget = (typename T::target_type*)GetTarget();

    pkObject = NiNew NiTDecorationFunctor<T>(pTarget);
    EE_ASSERT(pkObject != NULL);

    CopyMembers(pkObject, kCloning);

    return pkObject;
}

//------------------------------------------------------------------------------------------------
template <class T> 
inline void NiTDecorationFunctor<T>::CopyMembers(NiTDecorationFunctor<T>* pDest,
    NiCloningProcess& kCloning)
{
    // Copy the values of the extra data
    NiUInt32 uiExtraDataSize = pDest->m_kExtraData.GetSize();
    for (NiUInt32 ui = 0; ui < uiExtraDataSize; ++ui)
    {        
        // Delete the default extra data
        NiDelete pDest->m_kExtraData.GetAt(ui);

        // Create a clone of the source extra data
        NiExtraData* pkSourceExtraData = (NiExtraData*)m_kExtraData.GetAt(ui);
        EE_ASSERT(pkSourceExtraData);
        NiExtraData* pkClone = (NiExtraData*)pkSourceExtraData->CreateClone(kCloning);

        // Add the new extra data with the copied value
        pDest->m_kExtraData.SetAt(ui, pkClone);
    }
}

//------------------------------------------------------------------------------------------------
