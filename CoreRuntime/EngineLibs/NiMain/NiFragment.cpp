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
#include "NiMainPCH.h"

#include "NiFragment.h"
#include "NiCriticalSection.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiFragment, NiObject);
//--------------------------------------------------------------------------------------------------
NiFragment::NiFragment(NiUInt16 usVertexVersion,
        NiUInt16 usGeometryVersion,
        NiUInt16 usPixelVersion):
    m_usVertexVersion(usVertexVersion),
    m_usGeometryVersion(usGeometryVersion),
    m_usPixelVersion(usPixelVersion),
    m_pkMaterial(0)
{
}

//--------------------------------------------------------------------------------------------------
NiFragment::~NiFragment()
{
}

//--------------------------------------------------------------------------------------------------
void NiFragment::SetOwner(NiFragmentMaterial* pkMaterial)
{
    // Only allow this function to be called once
    EE_ASSERT(m_pkMaterial == NULL);
    {
        m_pkMaterial = pkMaterial;
        m_pkMaterial->m_kFragments.Add(this);
    }

    // Update the owning material:
    {
        // Increase the appropriate version variables:
        m_pkMaterial->m_usVertexVersion =
            m_pkMaterial->m_usVertexVersion + m_usVertexVersion;
        m_pkMaterial->m_usGeometryVersion =
            m_pkMaterial->m_usGeometryVersion + m_usGeometryVersion;
        m_pkMaterial->m_usPixelVersion =
            m_pkMaterial->m_usPixelVersion + m_usPixelVersion;

        // Make sure the material has the required libraries
        // (purely for versioning purposes, material does not require them)
        for (unsigned int ui = 0; ui < m_kLibraries.GetSize(); ui++)
        {
            NiMaterialNodeLibrary* pkLib = m_kLibraries.GetAt(ui);
            if (pkLib)
            {
                m_pkMaterial->m_kLibraries.Add(pkLib);
            }
        }
    }

    // Gather the dependancies for this fragment
    FetchDependencies();
}

//--------------------------------------------------------------------------------------------------
void NiFragment::FetchDependencies()
{
    NIASSERT(m_pkMaterial);
}

//--------------------------------------------------------------------------------------------------
NiMaterialNode* NiFragment::GetAttachableNodeFromLibrary(
        const NiFixedString& kNodeName)
{
    for (unsigned int ui = 0; ui < m_kLibraries.GetSize(); ui++)
    {
        NiMaterialNodeLibrary* pkLib = m_kLibraries.GetAt(ui);
        if (pkLib)
        {
            NiMaterialNode* pkNode = pkLib->GetAttachableNodeByName(kNodeName);
            if (pkNode)
                return pkNode;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragment::InsertTextureSampler(
        Context& kContext,
        const NiFixedString& kMapName,
        TextureMapSamplerType eSamplerType,
        unsigned int uiOccurance,
        NiShaderAttributeDesc::ObjectType eObjectType)
{
    return m_pkMaterial->InsertTextureSampler(kContext, kMapName, eSamplerType,
        uiOccurance, eObjectType);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragment::AddOutputPredefined(
    NiMaterialNode* pkNode,
    NiShaderConstantMap::DefinedMappings eMapping,
    unsigned int uiNumRegisters, unsigned int uiCount,
    unsigned int uiExtraData)
{
    return m_pkMaterial->AddOutputPredefined(pkNode, eMapping, uiNumRegisters,
        uiCount, uiExtraData);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragment::AddOutputObject(
    NiMaterialNode* pkNode,
    NiShaderConstantMap::ObjectMappings eMapping,
    NiShaderAttributeDesc::ObjectType eObjectType,
    unsigned int uiObjectCount, const char* pcVariableModifier,
    unsigned int uiCount)
{
    return m_pkMaterial->AddOutputObject(pkNode, eMapping, eObjectType,
        uiObjectCount, pcVariableModifier, uiCount);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragment::AddOutputAttribute(
    NiMaterialNode* pkNode,
    const NiFixedString& kVariableName,
    NiShaderAttributeDesc::AttributeType eType,
    unsigned int uiCount)
{
    return m_pkMaterial->AddOutputAttribute(pkNode, kVariableName, eType,
        uiCount);
}

//--------------------------------------------------------------------------------------------------
NiMaterialResource* NiFragment::AddOutputGlobal(
    NiMaterialNode* pkNode,
    const NiFixedString& kVariableName,
    NiShaderAttributeDesc::AttributeType eType,
    unsigned int uiCount)
{
    return m_pkMaterial->AddOutputGlobal(pkNode, kVariableName, eType,
        uiCount);
}

//--------------------------------------------------------------------------------------------------
unsigned int NiFragment::GetHighestObjectOffset(
    NiShaderAttributeDesc::ObjectType eObjectType,
    RenderPassDescriptor* pkRenderPasses, unsigned int uiCount)
{
    return NiFragmentMaterial::GetHighestObjectOffset(eObjectType,
        pkRenderPasses, uiCount);
}

//--------------------------------------------------------------------------------------------------
