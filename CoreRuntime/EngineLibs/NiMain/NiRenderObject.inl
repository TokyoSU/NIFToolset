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

//--------------------------------------------------------------------------------------------------
inline NiPropertyState* NiRenderObject::GetPropertyState() const
{
    return m_spPropertyState;
}

//--------------------------------------------------------------------------------------------------
inline NiDynamicEffectState* NiRenderObject::GetEffectState() const
{
    return m_spEffectState;
}

//--------------------------------------------------------------------------------------------------
inline void NiRenderObject::SetPropertyState(NiPropertyState* pkState)
{
    m_spPropertyState = pkState;
    InvalidateRenderBucketCache();
}

//--------------------------------------------------------------------------------------------------
inline void NiRenderObject::SetEffectState(NiDynamicEffectState* pkState)
{
    m_spEffectState = pkState;
}

//--------------------------------------------------------------------------------------------------
inline void NiRenderObject::SetLightGroupMask(efd::UInt8 uiMask)
{
    m_uiLightGroupMask = uiMask;
}

//--------------------------------------------------------------------------------------------------
inline efd::UInt8 NiRenderObject::GetLightGroupMask() const
{
    return m_uiLightGroupMask;
}

//--------------------------------------------------------------------------------------------------
inline void NiRenderObject::SetSortObject(bool bSort)
{
    // Explicit scoping is used here, as a function of the same name is
    // declared as protected in NiAVObject, and it is the AVObject version
    // that we are wrapping with a public interface for NiRenderObject.  This
    // avoids bloating NiRenderObject with a flag vector
    NiAVObject::SetSortObject(bSort);
}

//--------------------------------------------------------------------------------------------------
inline bool NiRenderObject::GetSortObject() const
{
    // Explicit scoping is used here, as a function of the same name is
    // declared as protected in NiAVObject, and it is the AVObject version
    // that we are wrapping with a public interface for NiRenderObject.  This
    // avoids bloating NiRenderObject with a flag vector
    return NiAVObject::GetSortObject();
}

//--------------------------------------------------------------------------------------------------
inline NiUInt32 NiRenderObject::GetMaterialCount()
{
    return m_kMaterials.GetSize();
}

//--------------------------------------------------------------------------------------------------
inline const NiMaterialInstance* NiRenderObject::GetMaterialInstance(
    NiUInt32 uiInstance)
{
    return (m_kMaterials.GetAt(uiInstance));
}

//--------------------------------------------------------------------------------------------------
inline void NiRenderObject::SetMaterialNeedsUpdateDefault(bool bFlag)
{
    m_bMaterialNeedsUpdateDefault = bFlag;
}

//--------------------------------------------------------------------------------------------------
inline bool NiRenderObject::IsMaterialApplied(const NiFixedString& kName) const
{
    return IsMaterialApplied(NiMaterial::GetMaterial(kName));
}

//--------------------------------------------------------------------------------------------------
inline bool NiRenderObject::SetActiveMaterial(const NiFixedString& kName)
{
    return SetActiveMaterial(NiMaterial::GetMaterial(kName));
}

//--------------------------------------------------------------------------------------------------
inline const NiMaterial* NiRenderObject::GetActiveMaterial() const
{
    if (m_uiActiveMaterial >= m_kMaterials.GetSize())
        return NULL;

    const NiMaterialInstance* pkMatInst =
        m_kMaterials.GetAt(m_uiActiveMaterial);
    return pkMatInst->GetMaterial();
}

//--------------------------------------------------------------------------------------------------
inline const NiMaterialInstance* NiRenderObject::GetActiveMaterialInstance()
    const
{
    if (m_uiActiveMaterial >= m_kMaterials.GetSize())
        return NULL;
    return m_kMaterials.GetAt(m_uiActiveMaterial);
}

//--------------------------------------------------------------------------------------------------
inline bool NiRenderObject::GetStreamingAppliesDefaultMaterial()
{
    return ms_bStreamingAppliesDefaultMaterial;
}

//--------------------------------------------------------------------------------------------------
inline void NiRenderObject::SetStreamingAppliesDefaultMaterial(
    bool bApplyDefault)
{
    ms_bStreamingAppliesDefaultMaterial = bApplyDefault;
}

//--------------------------------------------------------------------------------------------------
inline bool NiRenderObject::GetMaterialNeedsUpdateDefault() const
{
    return m_bMaterialNeedsUpdateDefault;
}

//--------------------------------------------------------------------------------------------------
inline void NiRenderObject::AppendToCullingProcess(NiCullingProcess& kCuller)
{
    kCuller.Append(*this);
}

//--------------------------------------------------------------------------------------------------
inline bool NiRenderObject::RemoveMaterial(const NiFixedString& kName)
{
    return RemoveMaterial(NiMaterial::GetMaterial(kName));
}

//--------------------------------------------------------------------------------------------------
inline bool NiRenderObject::SwapMaterial(NiMaterial* pkMaterial, efd::UInt32 uiMaterialExtraData)
{
    EE_ASSERT(m_pkMaterialSwap == NULL);
    m_pkMaterialSwap = GetActiveMaterial();
    return ApplyAndSetActiveMaterial(pkMaterial, uiMaterialExtraData);
}

//--------------------------------------------------------------------------------------------------
inline void NiRenderObject::RestoreSwapMaterial()
{
    EE_ASSERT(m_pkMaterialSwap != NULL);
    SetActiveMaterial(m_pkMaterialSwap);
    m_pkMaterialSwap = NULL;
}

//--------------------------------------------------------------------------------------------------