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
inline const NiFixedString& NiMaterial::GetName() const
{
    return m_kMaterialName;
}

//--------------------------------------------------------------------------------------------------
inline void NiMaterial::SetSwapMaterial(SwapMaterialType type, NiMaterial* pkMaterial)
{
    m_spSwapMaterials[type] = pkMaterial; 
}

//--------------------------------------------------------------------------------------------------
inline NiMaterial* NiMaterial::GetSwapMaterial(SwapMaterialType type) const
{
    return m_spSwapMaterials[type];
}

//--------------------------------------------------------------------------------------------------