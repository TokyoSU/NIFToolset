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
inline void NiDecorationPlane::SetCamera(NiCamera* pkCamera)
{
    m_pCamera = pkCamera;

    // Make sure all layers know about this camera change
    InvalidateAllTransforms();
}

//------------------------------------------------------------------------------------------------
inline NiCamera* NiDecorationPlane::GetCamera() const
{
    return m_pCamera;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationPlane::GetNumLayers() const
{
    return m_kLayers.size();
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetDimension(const NiPoint2& kDimensions)
{
    if (kDimensions != m_kDimension)
    {
        m_kDimension = kDimensions;
        m_bRequiresRecreation = true;

        // All functors will need recreating
        for (LayerInfoMap::iterator iter = m_kLayers.begin();
            iter != m_kLayers.end();
            iter++)
        {
            iter->second.m_bRequiresFunctorCreation = true;
        }
    }
}

//------------------------------------------------------------------------------------------------
inline const NiPoint2& NiDecorationPlane::GetDimension() const
{
    return m_kDimension;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetNumCellsX(const efd::utf8string& kLayerName, NiUInt32 uiNumCells)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (uiNumCells != kLayerInfo.m_uiNumCellsX)
    {
        kLayerInfo.m_uiNumCellsX = uiNumCells;
        kLayerInfo.m_bRequiresInitialization = true;
        kLayerInfo.m_bRequiresFunctorCreation = true;
    }
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationPlane::GetNumCellsX(const efd::utf8string& kLayerName) const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_uiNumCellsX;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetNumCellsY(const efd::utf8string& kLayerName, NiUInt32 uiNumCells)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (uiNumCells != kLayerInfo.m_uiNumCellsY)
    {
        kLayerInfo.m_uiNumCellsY = uiNumCells;
        kLayerInfo.m_bRequiresInitialization = true;
        kLayerInfo.m_bRequiresFunctorCreation = true;
    }
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationPlane::GetNumCellsY(const efd::utf8string& kLayerName) const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_uiNumCellsY;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetInstancesPerField(const efd::utf8string& kLayerName, 
    NiUInt32 uiNumInstances)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (uiNumInstances != kLayerInfo.m_uiInstancesPerField)
    {
        kLayerInfo.m_uiInstancesPerField = uiNumInstances;
        kLayerInfo.m_bRequiresInitialization = true;
    }
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationPlane::GetInstancesPerField(const efd::utf8string& kLayerName) const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_uiInstancesPerField;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetMaxRange(const efd::utf8string& kLayerName, float fMaxRange)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (fMaxRange != kLayerInfo.m_fMaxRange)
    {
        kLayerInfo.m_fMaxRange = fMaxRange;
        kLayerInfo.m_bRequiresInitialization = true;
        kLayerInfo.m_bRequiresFunctorCreation = true;
    }
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationPlane::GetMaxRange(const efd::utf8string& kLayerName) const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_fMaxRange;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetMinRange(const efd::utf8string& kLayerName, float fMinRange)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (fMinRange != kLayerInfo.m_fMinRange)
    {
        kLayerInfo.m_fMinRange = fMinRange;
        kLayerInfo.m_bRequiresInitialization = true;
        kLayerInfo.m_bRequiresFunctorCreation = true;
    }
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationPlane::GetMinRange(const efd::utf8string& kLayerName) const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_fMinRange;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetNearFade(const efd::utf8string& kLayerName, float fNearFade)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (fNearFade != kLayerInfo.m_fNearFadeDistance)
    {
        kLayerInfo.m_fNearFadeDistance = fNearFade;
        kLayerInfo.m_bRequiresReset = true;
    }
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationPlane::GetNearFade(const efd::utf8string& kLayerName) const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_fNearFadeDistance;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetFarFade(const efd::utf8string& kLayerName, float fFarFade)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (fFarFade != kLayerInfo.m_fFarFadeDistance)
    {
        kLayerInfo.m_fFarFadeDistance = fFarFade;
        kLayerInfo.m_bRequiresReset = true;
    }
}

//------------------------------------------------------------------------------------------------
inline float NiDecorationPlane::GetFarFade(const efd::utf8string& kLayerName) const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_fFarFadeDistance;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetGenerator(const efd::utf8string& kLayerName, 
    NiDecorationGenerator* pkGenerator)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (pkGenerator != kLayerInfo.m_spGenerator)
    {
        kLayerInfo.m_spGenerator = pkGenerator;
        kLayerInfo.m_bRequiresInitialization = true;
    }
}

//------------------------------------------------------------------------------------------------
inline NiDecorationGenerator* NiDecorationPlane::GetGenerator(const efd::utf8string& kLayerName) 
    const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_spGenerator;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetRandomSeed(const efd::utf8string& kLayerName, NiUInt32 uiSeed)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    if (uiSeed != kLayerInfo.m_uiRandomSeed)
    {
        kLayerInfo.m_uiRandomSeed = uiSeed;
        kLayerInfo.m_bRequiresReset = true;
    }
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationPlane::GetRandomSeed(const efd::utf8string& kLayerName) const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_uiRandomSeed;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetFunctorTarget(NiAVObject* pkTarget)
{
    m_pFunctorTarget = pkTarget;

    for (LayerInfoMap::iterator iter = m_kLayers.begin();
        iter != m_kLayers.end();
        iter++)
    {
        iter->second.m_bRequiresFunctorCreation = true;
    }
}

//------------------------------------------------------------------------------------------------
inline NiAVObject* NiDecorationPlane::GetFunctorTarget() const
{
    return m_pFunctorTarget;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetFunctorTargetSettings(const efd::utf8string& kLayerName, 
    const SettingsMap& kSettings)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    kLayerInfo.m_functorSettings = kSettings;
    kLayerInfo.m_bRequiresFunctorReinitialization = true;
}

//------------------------------------------------------------------------------------------------
inline const NiDecorationPlane::SettingsMap& NiDecorationPlane::GetFunctorTargetSettings(
    const efd::utf8string& kLayerName)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_functorSettings;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::ReapplyFunctorConfiguration()
{
    for (LayerInfoMap::iterator iter = m_kLayers.begin();
        iter != m_kLayers.end();
        iter++)
    {
        iter->second.m_bRequiresFunctorReinitialization = true;
    }
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::InvalidateAllTransforms()
{
    for (LayerInfoMap::iterator iter = m_kLayers.begin();
        iter != m_kLayers.end();
        iter++)
    {
        iter->second.m_bRequiresReset = true;
    }
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationPlane::GetMaxCellsGeneratedPerFrame(const efd::utf8string& kLayerName) 
    const
{
    const NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    return kLayerInfo.m_uiMaxCellsGenerated;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::SetMaxCellsGeneratedPerFrame(const efd::utf8string& kLayerName, 
    NiUInt32 uiMaxCells)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);
    kLayerInfo.m_uiMaxCellsGenerated = uiMaxCells;

    // Technically, we shouldn't need a full reset here, we could get away with finding all layers
    // and just setting it. However, for the sake of cleanliness, just perform the reset.
    kLayerInfo.m_bRequiresReset = true;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationPlane::EnforceValidBound()
{
    if (m_kWorldBound.GetRadius() == 0.0f)
        m_kWorldBound.SetRadius(1.0f);
}

//------------------------------------------------------------------------------------------------
inline const NiDecorationPlane::NiDecorationLayerInfo& NiDecorationPlane::GetLayerInfo(
    const efd::utf8string& kLayerName) const
{
    LayerInfoMap::const_iterator pos = m_kLayers.find(kLayerName);
    NIASSERT(pos != m_kLayers.end());
    return pos->second;
}

//------------------------------------------------------------------------------------------------
inline NiDecorationPlane::NiDecorationLayerInfo& NiDecorationPlane::GetLayerInfo(
    const efd::utf8string& kLayer)
{
    LayerInfoMap::iterator pos = m_kLayers.find(kLayer);
    NIASSERT(pos != m_kLayers.end());
    return pos->second;
}

//------------------------------------------------------------------------------------------------