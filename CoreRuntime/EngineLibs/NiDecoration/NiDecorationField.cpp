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

#include "NiDecorationPCH.h"
#include "NiDecorationField.h"

#include <NiTransform.h>

NiImplementRTTI(NiDecorationField, NiNode, NiTypeMask::NiDecorationField);

//------------------------------------------------------------------------------------------------
NiDecorationField::NiDecorationField(NiPoint2 kWidth) 
    : m_kWidth(kWidth)
    , m_uiFieldIndex(0)
{
}

//------------------------------------------------------------------------------------------------
NiDecorationField::~NiDecorationField()
{
    m_kLayers.clear();
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::AddLayer(const efd::utf8string& kLayerName, NiDecorationLayer* pkLayer)
{
    EE_ASSERT(pkLayer);
    if (!pkLayer)
        return;

    NIASSERT(m_kLayers.find(kLayerName) == m_kLayers.end());
    m_kLayers[kLayerName] = pkLayer;

    AttachChild(pkLayer, true);

    UpdateProperties();
    UpdateEffects();
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::ResetCells()
{
    for (LayerMap::iterator kIter = m_kLayers.begin();
        kIter != m_kLayers.end();
        kIter++)
    {
        kIter->second->ResetCells();
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::RemoveLayer(NiDecorationLayer* pkLayer)
{
    DetachChild(pkLayer);

    for (LayerMap::iterator kIter = m_kLayers.begin();
        kIter != m_kLayers.end();
        kIter++)
    {
        if (kIter->second == pkLayer)
        {
            m_kLayers.erase(kIter);
            break;
        }
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::RemoveLayer(const efd::utf8string& kLayer)
{
    DetachChild(GetLayerAt(kLayer));

    m_kLayers.erase(kLayer);
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::GetLayers(NiTPrimitiveSet<NiDecorationLayer*>& kLayers) const
{
    for (LayerMap::const_iterator kIter = m_kLayers.begin();
        kIter != m_kLayers.end();
        kIter++)
    {
        kLayers.Add(kIter->second);
    }
}

//------------------------------------------------------------------------------------------------
NiUInt32 NiDecorationField::GetNumLayers() const
{
    return m_kLayers.size();
}

//------------------------------------------------------------------------------------------------
NiDecorationLayer* NiDecorationField::GetLayerAt(const efd::utf8string& kLayerName) const
{
    LayerMap::const_iterator kPos = m_kLayers.find(kLayerName);
    if (kPos != m_kLayers.end())
        return kPos->second;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{
    NiTransform kOldTransform = GetWorldTransform();

    UpdateAnimation(kUpdate);

    NiNode::UpdateDownwardPass(kUpdate);


    if (kOldTransform != GetWorldTransform())
        RebuildLayerTransforms();
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{
    NiTransform kOldTransform = GetWorldTransform();

    NiNode::UpdateSelectedDownwardPass(kUpdate);

    if (kOldTransform != GetWorldTransform())
        RebuildLayerTransforms();
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    NiTransform kOldTransform = GetWorldTransform();

    NiNode::UpdateRigidDownwardPass(kUpdate);

    if (kOldTransform != GetWorldTransform())
        RebuildLayerTransforms();
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::RebuildLayerTransforms()
{
    for (LayerMap::const_iterator kIter = m_kLayers.begin();
        kIter != m_kLayers.end();
        kIter++)
    {
        kIter->second->ResetCells();
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::UpdateAnimation(NiUpdateProcess& kUpdate)
{
    NiDecorationLayer* pkLayer;
    NiDecorationMeshInfo* pkBase;
    for (LayerMap::const_iterator kIter = m_kLayers.begin();
        kIter != m_kLayers.end();
        kIter++)
    {
        pkLayer = kIter->second;

        pkBase = pkLayer->GetBaseMesh();
        if (!pkBase)
            continue;

        EE_ASSERT(pkLayer->GetGenerator());
        pkLayer->GetGenerator()->UpdateAnimation(pkBase, kUpdate);
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::UpdateShaderConstants()
{
    NiDecorationLayer* pkLayer;
    for (LayerMap::const_iterator kIter = m_kLayers.begin();
        kIter != m_kLayers.end();
        kIter++)
    {
        pkLayer = kIter->second;

        NiDecorationGenerator* pkGenerator = pkLayer->GetGenerator();
        if (!pkGenerator)
            continue;

        pkLayer->UpdateShaderConstants();
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationField::SetFieldIndex(NiUInt32 uiFieldIndex)
{
    m_uiFieldIndex = uiFieldIndex;
}

//------------------------------------------------------------------------------------------------
NiUInt32 NiDecorationField::GetFieldIndex() const
{
    return m_uiFieldIndex;
}

//------------------------------------------------------------------------------------------------
