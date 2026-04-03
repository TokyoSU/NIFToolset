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

// Pre-compiled header
#include "ecrPCH.h"

#include "FogModel.h"

#include <egf/StandardModelLibraryBehaviorIDs.h>
#include <ecr/SceneGraphService.h>

using namespace ecr;
using namespace egf;

//--------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(FogModel);
EE_IMPLEMENT_BUILTINMODEL_PROPERTIES(FogModel);
//--------------------------------------------------------------------------------------------------
EE_HANDLER(FogModel, HandleSceneGraphAddedMessage, ecr::SceneGraphAddedMessage);
EE_HANDLER(FogModel, HandleSceneGraphRemovedMessage, ecr::SceneGraphRemovedMessage);
//--------------------------------------------------------------------------------------------------
IBuiltinModel* FogModel::FogModelFactory()
{
    return EE_NEW FogModel();
}

//--------------------------------------------------------------------------------------------------
FogModel::FogModel()
{
    m_spFogProperty = NiNew NiFogProperty();
}
    
//--------------------------------------------------------------------------------------------------
FogModel::~FogModel()
{
}

//--------------------------------------------------------------------------------------------------
void FogModel::OnAdded()
{
    IBuiltinModel::OnAdded();
    
    RenderableModel::AddCallback(this);
    
    // Begin listening to messages
    efd::MessageService* pMessageService = 
        GetServiceManager()->GetSystemServiceAs<efd::MessageService>();
    EE_ASSERT(pMessageService);
    pMessageService->Subscribe(this, efd::kCAT_LocalMessage);

    if (GetEnabled())
        AttachAllSceneGraphs();
}

//--------------------------------------------------------------------------------------------------
void FogModel::OnRemoved()
{
    // Remove this fog property from all scene graphs
    DetachAllSceneGraphs();

    // Stop listening to messages
    efd::MessageService* pMessageService = 
        GetServiceManager()->GetSystemServiceAs<efd::MessageService>();
    if (pMessageService)
        pMessageService->Unsubscribe(this, efd::kCAT_LocalMessage);

    RenderableModel::RemoveCallback(this);

    IBuiltinModel::OnRemoved();
}

//--------------------------------------------------------------------------------------------------
bool FogModel::operator==(const IProperty& other) const
{
    if (!IBuiltinModel::operator ==(other))
        return false;
        
    const FogModel* otherClass = static_cast<const FogModel*>(&other);

    if (GetEnabled() != otherClass->GetEnabled() ||
        GetFunction() != otherClass->GetFunction() ||
        GetDepth() != otherClass->GetDepth() ||
        GetColor() != otherClass->GetColor())
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool FogModel::GetEnabled() const
{
    return m_spFogProperty->GetFog();
}

//--------------------------------------------------------------------------------------------------
void FogModel::SetEnabled(const bool& bEnabled)
{
    if (bEnabled == GetEnabled())
        return;

    m_spFogProperty->SetFog(bEnabled);

    if (GetServiceManager())
    {
        if (bEnabled)
        {
            AttachAllSceneGraphs();
        }
        else
        {
            DetachAllSceneGraphs();
        }
    }
}

//--------------------------------------------------------------------------------------------------
efd::utf8string FogModel::GetFunction() const
{
    switch (m_spFogProperty->GetFogFunction())
    {
    case NiFogProperty::FOG_RANGE_SQ:
        return "Square";
    case NiFogProperty::FOG_Z_LINEAR:
    default:
        return "Linear";
    }
}

//--------------------------------------------------------------------------------------------------
void FogModel::SetFunction(const efd::utf8string& fogFunction)
{
    if (fogFunction == "Square")
    {
        m_spFogProperty->SetFogFunction(NiFogProperty::FOG_RANGE_SQ);
    }
    else
    {
        m_spFogProperty->SetFogFunction(NiFogProperty::FOG_Z_LINEAR);
    }
}

//--------------------------------------------------------------------------------------------------
efd::Float32 FogModel::GetDepth() const
{
    return m_spFogProperty->GetDepth();
}

//--------------------------------------------------------------------------------------------------
void FogModel::SetDepth(const efd::Float32& depth)
{
    m_spFogProperty->SetDepth(depth);
}

//--------------------------------------------------------------------------------------------------
efd::Color FogModel::GetColor() const
{
    return m_spFogProperty->GetFogColor();
}

//--------------------------------------------------------------------------------------------------
void FogModel::SetColor(const efd::Color& color)
{
    m_spFogProperty->SetFogColor(color);
}

//--------------------------------------------------------------------------------------------------
NiFogProperty* FogModel::GetFogProperty()
{
    return m_spFogProperty;
}

//--------------------------------------------------------------------------------------------------
bool FogModel::IsAffectedByFog(const egf::Entity* pEntity)
{
    // It's possible that the entity associated with the scene graph has already
    // been removed from the entity manager (if the entity is removed before
    // pending messages associated with it are complete).
    if (!pEntity)
        return false;

    // Default affected by fog for an entity is true
    efd::Bool bAffectedByFog = false;

    // Only consider the entity for fogging if it contains the 'FogAffected' model.
    const egf::FlatModel* pModel = pEntity->GetModel();
    EE_ASSERT(pModel);
    if (!pModel->ContainsModel(egf::kFlatModelID_StandardModelLibrary_Renderable))
        return bAffectedByFog;

    // Check the flag on the entity to see if it is affected by fog
    if (pEntity->GetPropertyValue(
        egf::kPropertyID_StandardModelLibrary_IsFogAffected, 
        bAffectedByFog) != PropertyResult_OK)
    {
        return bAffectedByFog;
    }

    return bAffectedByFog;
}

//--------------------------------------------------------------------------------------------------
void FogModel::AttachSceneGraph(const egf::Entity* pEntity, NiAVObject* pNode)
{
    EE_UNUSED_ARG(pEntity);

    // Attach the property
    pNode->AttachProperty(m_spFogProperty);
    pNode->UpdateProperties();
}

//--------------------------------------------------------------------------------------------------
void FogModel::DetachSceneGraph(const egf::Entity* pEntity, NiAVObject* pNode)
{
    EE_UNUSED_ARG(pEntity);
    
    // Detach the property
    if (pNode->GetProperty(NiFogProperty::GetType()) == m_spFogProperty)
    {
        pNode->RemoveProperty(NiFogProperty::GetType());
        pNode->UpdateProperties();
    }
}

//------------------------------------------------------------------------------------------------
void FogModel::HandleSceneGraphAddedMessage(const ecr::SceneGraphAddedMessage* pMessage,
    efd::Category targetChannel)
{
    EE_UNUSED_ARG(targetChannel);

    Entity* pEntity = pMessage->GetEntity();
    if (!pEntity)
        return;

    // Check if it is supposed to be affected by fog
    if (!IsAffectedByFog(pEntity))
        return;

    // Fetch the scene graph for the entity
    SceneGraphService* pSceneGraphService = 
        GetServiceManager()->GetSystemServiceAs<SceneGraphService>();
    NiAVObjectPtr spObject = NULL;
    spObject = pSceneGraphService->GetSceneGraphFromEntity(pEntity);
    if (!spObject)
        return;

    AttachSceneGraph(pEntity, spObject);
}

//------------------------------------------------------------------------------------------------
void FogModel::HandleSceneGraphRemovedMessage(const ecr::SceneGraphRemovedMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);
    EE_UNUSED_ARG(targetChannel);

    Entity* pEntity = pMessage->GetEntity();

    SceneGraphService* pSceneGraphService = 
        GetServiceManager()->GetSystemServiceAs<SceneGraphService>();
    NiAVObject* pObject = NULL;
    if (pEntity)
        pObject = pSceneGraphService->GetSceneGraphFromEntity(pEntity);

    NiAVObject* pNode = NiDynamicCast(NiAVObject, pObject);
    if (!pNode)
        return;

    DetachSceneGraph(pEntity, pNode);
}

//------------------------------------------------------------------------------------------------
void FogModel::OnPropertyUpdate(
    const egf::FlatModelID& modelID,
    egf::Entity* pEntity,
    const egf::PropertyID& propertyID,
    const egf::IProperty* pProperty,
    const efd::UInt32 tags)
{
    EE_UNUSED_ARG(tags);

    if (modelID != egf::kFlatModelID_StandardModelLibrary_Renderable)
        return;

    if (propertyID != egf::kPropertyID_StandardModelLibrary_IsFogAffected)
        return;

    SceneGraphService* pSceneGraphService = 
        GetServiceManager()->GetSystemServiceAs<SceneGraphService>();
    NiAVObject* pObject = NULL;
    if (pEntity)
        pObject = pSceneGraphService->GetSceneGraphFromEntity(pEntity);

    NiAVObject* pNode = NiDynamicCast(NiAVObject, pObject);
    if (!pNode)
        return;

    const RenderableModel* pRenderable = static_cast<const RenderableModel*>(pProperty);
    EE_ASSERT(pRenderable);

    if (pRenderable->GetIsFogAffected())
        AttachSceneGraph(pEntity, pNode);
    else
        DetachSceneGraph(pEntity, pNode);
}

//------------------------------------------------------------------------------------------------
FogModel::AttachFogFunctor::AttachFogFunctor(FogModel* pFogModel, bool bDetach)
    : m_pFogModel(pFogModel)
    , m_detach(bDetach)
{
}

//------------------------------------------------------------------------------------------------
efd::Bool FogModel::AttachFogFunctor::operator()(const egf::Entity* pEntity,
    const efd::vector<NiObjectPtr>& objects)
{
    NiAVObjectPtr spNode = NiDynamicCast(NiAVObject, objects[0]);
    if (!spNode)
        return false;
        
    if (!m_detach)
    {
        if (m_pFogModel->IsAffectedByFog(pEntity))
        {
             m_pFogModel->AttachSceneGraph(pEntity, spNode);
        }
    }
    else
    {
        m_pFogModel->DetachSceneGraph(pEntity, spNode);
    }

    return false;
}

//------------------------------------------------------------------------------------------------
void FogModel::AttachAllSceneGraphs()
{
    SceneGraphService* pSceneGraphService = 
        GetServiceManager()->GetSystemServiceAs<SceneGraphService>();
    if (!pSceneGraphService)
        return;

    AttachFogFunctor functor(this, false);
    pSceneGraphService->ForEachEntitySceneGraph(functor);
}

//------------------------------------------------------------------------------------------------
void FogModel::DetachAllSceneGraphs()
{
    SceneGraphService* pSceneGraphService = 
        GetServiceManager()->GetSystemServiceAs<SceneGraphService>();
    if (!pSceneGraphService)
        return;

    AttachFogFunctor functor(this, true);
    pSceneGraphService->ForEachEntitySceneGraph(functor);
}

//------------------------------------------------------------------------------------------------