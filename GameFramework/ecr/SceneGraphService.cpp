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

#include "ecrPCH.h"

#include <efd/MessageService.h>
#include <efd/PathUtils.h>
#include <efd/IConfigManager.h>
#include <efd/ServiceManager.h>
#include <efd/AssetLocatorService.h>

#include <egf/EntityManager.h>
#include <egf/EntityChangeMessage.h>
#include <egf/egfLogIDs.h>
#include <egf/FlatModelManager.h>
#include <egf/egfClassIDs.h>
#include <egf/StandardModelLibraryPropertyIDs.h>
#include <egf/StandardModelLibraryFlatModelIDs.h>

#include <NiShadowManager.h>
#include <NiShadowGenerator.h>

#include <NiMemstream.h>
#include <NiStream.h>
#include <NiMesh.h>
#include <NiMeshUpdateProcess.h>
#include <NiDataStreamFactory.h>

#include <ecr/SceneGraphService.h>
#include <ecr/RenderService.h>
#include <ecr/CoreRuntimeMessages.h>
#include <ecr/CoreRuntimePropertyTypes.h>
#include <ecr/ShaderService.h>
#include <ecr/SnapshotGenerator.h>
#include <ecr/FogModel.h>

using namespace egf;
using namespace efd;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(SceneGraphService);

// Register for changes to locally owned entities:
EE_HANDLER_WRAP(SceneGraphService, HandleEntityEnterWorldMessage,
    egf::EntityChangeMessage, kMSGID_OwnedEntityEnterWorld);
EE_HANDLER_WRAP(SceneGraphService, HandleEntityExitWorldMessage,
    egf::EntityChangeMessage, kMSGID_OwnedEntityExitWorld);
EE_HANDLER_WRAP(SceneGraphService, HandleEntityUpdatedMessage,
    egf::EntityChangeMessage, kMSGID_OwnedEntityUpdated);

// Register for SceneGraphCache messages
EE_HANDLER(SceneGraphService, HandleCacheResponse, SceneGraphCacheResponse);

// Register for entity preload messages
EE_HANDLER(SceneGraphService, HandlePreloadRequest, EntityPreloadRequest);


const efd::UInt32 SceneGraphService::SceneGraphData::kInvalidUpdateIndex = (efd::UInt32)(-1);
const SceneGraphService::SceneGraphHandle SceneGraphService::kInvalidHandle = 0;


//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphService(
    NiSPWorkflowManager* pWorkflowManager,
    NiTexturePalette* pTexturePalette,
    const efd::Bool toolMode)
    : m_pMessageService(NULL)
    , m_pEntityManager(NULL)
    , m_spRenderService(NULL)
    , m_sceneGraphCacheCategory(kCAT_INVALID)
    , m_spWorkflowManager(pWorkflowManager)
    , m_nextUnusedHandle(1)
    , m_toolMode(toolMode)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 1500;

#if defined (EE_PLATFORM_XBOX360)
    m_strDataPath = "D:/Data";
#else
    m_strDataPath = "../../../Data";
#endif

    m_spSceneGraphCache = EE_NEW SceneGraphCache(pTexturePalette);

    if (!m_spWorkflowManager)
    {
        m_spWorkflowManager = NiNew NiSPWorkflowManager(64);
    }

    m_pUpdateProcess = NiNew NiMeshUpdateProcess(m_spWorkflowManager);
    m_pUpdateProcess->SetSubmitModifiers(true);
    m_pUpdateProcess->SetUpdateControllers(true);

    m_pBatchedUpdateProcess = NiNew NiBatchedUpdateProcess(m_spWorkflowManager);
    m_pBatchedUpdateProcess->SetSubmitModifiers(true);
    m_pBatchedUpdateProcess->SetUpdateControllers(true);
}

//------------------------------------------------------------------------------------------------
SceneGraphService::~SceneGraphService()
{

}

//------------------------------------------------------------------------------------------------
const char* SceneGraphService::GetDisplayName() const
{
    return "SceneGraphService";
}

//------------------------------------------------------------------------------------------------
SyncResult SceneGraphService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    // If the shader service is registered with the service manager then wait for it to complete
    // initializing before we initialize the SceneGraphService. This ensures that all shaders are
    // loaded before we attempt to load any NIFs that might require those shaders.
    pDependencyRegistrar->AddDependency<ShaderService>(sdf_Optional);
    // Because we register an IAssetFactory (via the SceneGraphCache that we manage) we must
    // depend on the AssetFactoryManager class.
    pDependencyRegistrar->AddDependency<AssetFactoryManager>();

    m_pMessageService = m_pServiceManager->GetSystemServiceAs<efd::MessageService>();
    EE_ASSERT(m_pMessageService);
    if (!m_pMessageService)
        return SyncResult_Failure;

    m_pEntityManager = m_pServiceManager->GetSystemServiceAs<egf::EntityManager>();
    EE_ASSERT(m_pEntityManager);
    if (!m_pEntityManager)
        return SyncResult_Failure;

    if (!SubscribeToMessages())
        return SyncResult_Failure;

    egf::FlatModelManager* pFlatModelManager =
        m_pServiceManager->GetSystemServiceAs<egf::FlatModelManager>();
    if (pFlatModelManager)
    {
        pFlatModelManager->RegisterPropertyFactory("AttachedNifAsset",
            AttachedNifAssetScalarProperty::CLASS_ID, AttachedNifAssetScalarPropertyFactory);
        pFlatModelManager->RegisterPropertyFactory("AssocAttachedNifAsset",
            AttachedNifAssetAssocProperty::CLASS_ID, AttachedNifAssetAssocPropertyFactory);
        pFlatModelManager->RegisterBuiltinModelFactory("Mesh",
            kFlatModelID_StandardModelLibrary_Mesh, MeshModel::MeshModelFactory);
        RenderableModel::Initialize();
        pFlatModelManager->RegisterBuiltinModelFactory("Renderable",
            kFlatModelID_StandardModelLibrary_Renderable, RenderableModel::RenderableModelFactory);
        
        pFlatModelManager->RegisterBuiltinModelFactory("Fog",
            kFlatModelID_StandardModelLibrary_Fog, 
            FogModel::FogModelFactory);
    }

    // Register for callbacks from various built-in models
    PlaceableModel::AddCallback(this);
    RenderableModel::AddCallback(this);

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult SceneGraphService::OnInit()
{
    // Register for preloading in OnInit when we know the EntityManager will have
    // been initialized ready to handle this method.
    m_pEntityManager->RegisterPreloadService(m_assetPreloadResponseCategory);

    m_spRenderService = m_pServiceManager->GetSystemServiceAs<RenderService>();

    // Setup the ReloadManager to invoke the SceneGraphCache's callback when .nif files are
    // changed
    ReloadManagerPtr pReloadManager = m_pServiceManager->GetSystemServiceAs<ReloadManager>();
    if (pReloadManager)
    {
        pReloadManager->RegisterAssetChangeHandler("gamebryo-scenegraph", m_spSceneGraphCache);
    }

    if (m_toolMode)
    {
        // Tool mode enables CPU read on all data.
        EE_ASSERT(NiDataStream::GetFactory());
        NiDataStream::GetFactory()->SetCallback(NiDataStreamFactory::ForceCPUReadAccessCallback);
    }

#ifndef EE_CONFIG_SHIPPING
    SnapshotGenerator::Create(m_pServiceManager);
#endif

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult SceneGraphService::OnTick()
{
    UpdateDynamicEntities();

    // Return pending to indicate the service should continue to be ticked.
    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UpdateDynamicEntities(
    bool advanceUpdateTime,
    bool performUpdate,
    bool performUpdateOnce)
{
    // Ticking the scene graph service causes all update-able scene graphs to be updated.
    EE_ASSERT(m_pUpdateProcess);

    if (advanceUpdateTime)
    {
        float currTime = (float)m_pServiceManager->GetTime(kCLASSID_GameTimeClock);
        m_pUpdateProcess->SetTime(currTime);
        m_pBatchedUpdateProcess->SetTime(currTime);
    }

    if (performUpdate)
    {
        efd::UInt32 numToUpdate = static_cast<efd::UInt32>(m_toUpdate.size());
        for (efd::UInt32 ui = 0; ui < numToUpdate; ++ui)
        {
            SceneGraphData* pData = m_toUpdate[ui];
            NiAVObject* pSceneGraph = pData->GetSceneGraph();
            EE_ASSERT(pSceneGraph);
            pSceneGraph->Update(*m_pBatchedUpdateProcess);

            UInt32 count = pData->m_placeableFeedback.size();
            for (UInt32 uj = 0; uj < count; ++uj)
            {
                PlaceableFeedback(pData->m_placeableFeedback[uj]);
            }
        }
        m_pBatchedUpdateProcess->Flush();
    }

    if (performUpdateOnce)
    {
        efd::UInt32 numToUpdate = static_cast<efd::UInt32>(m_toUpdateOnce.size());
        for (efd::UInt32 ui = 0; ui < numToUpdate; ++ui)
        {
            SceneGraphData* pData = m_toUpdateOnce[ui];
            NiAVObject* pSceneGraph = pData->GetSceneGraph();
            EE_ASSERT(pSceneGraph);

            // Do not use batched, because we want to submit and complete modifiers immediately.
            // There is no point in using a batched update in this case.
            pSceneGraph->Update(*m_pUpdateProcess);
            NiMesh::CompleteSceneModifiers(pSceneGraph);

            UInt32 count = pData->m_placeableFeedback.size();
            for (UInt32 uj = 0; uj < count; ++uj)
            {
                PlaceableFeedback(pData->m_placeableFeedback[uj]);
            }
        }
    }

    // Send the updated message, immediately
    OnSceneGraphsUpdated();

    // Clear it now so that services responding to the message can iterate on this list.
    m_toUpdateOnce.clear();
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::PlaceableFeedback(PlaceableFeedbackData& data)
{
    Point3 rotationXYZ;
    data.m_pNode->GetWorldRotate().ToEulerAnglesXYZ(
        rotationXYZ.x,
        rotationXYZ.y,
        rotationXYZ.z);
    rotationXYZ *= -EE_RADIANS_TO_DEGREES;

    data.m_pPlaceable->SetInternalPosition(
        data.m_pNode->GetWorldTranslate(),
        data.m_pIgnoreCallback);
    data.m_pPlaceable->SetInternalRotation(rotationXYZ, data.m_pIgnoreCallback);

    if (data.m_doScale)
        data.m_pPlaceable->SetInternalScale(data.m_pNode->GetWorldScale(), data.m_pIgnoreCallback);
}

//------------------------------------------------------------------------------------------------
AsyncResult SceneGraphService::OnShutdown()
{
    EE_ASSERT(m_pServiceManager);

    m_pMessageService = m_pServiceManager->GetSystemServiceAs<efd::MessageService>();

    if (m_pMessageService != NULL)
    {
        m_pMessageService->Unsubscribe(this, kCAT_LocalMessage);
        m_pMessageService->Unsubscribe(this, m_sceneGraphCacheCategory);
        m_pMessageService->Unsubscribe(this, m_assetPreloadRequestCategory);
        m_pMessageService->Unsubscribe(this, m_assetRefreshCategory);
    }

    m_toUpdate.clear();
    m_toUpdateOnce.clear();

    // Unregister for callbacks
    PlaceableModel::RemoveCallback(this);
    RenderableModel::RemoveCallback(this);

    efd::map<egf::Entity*, SceneGraphDataPtr>::iterator entityIter = m_entityData.begin();
    while (entityIter != m_entityData.end())
    {
        entityIter->second = 0;
        ++entityIter;
    }
    m_entityData.clear();

    efd::map<utf8string, AssetDataPtr>::iterator assetIDIter = m_assetsByPhysicalID.begin();
    while (assetIDIter != m_assetsByPhysicalID.end())
    {
        if (assetIDIter->second->m_cacheHandle)
        {
            m_spSceneGraphCache->UnCacheSceneGraph(assetIDIter->second->m_cacheHandle);
            assetIDIter->second->m_cacheHandle = 0;
        }
        assetIDIter->second = 0;
        ++assetIDIter;
    }
    m_assetsByPhysicalID.clear();

    efd::map<SceneGraphHandle, AttachedSceneGraphDataPtr>::iterator attachIter =
        m_attachedData.begin();
    while (attachIter != m_attachedData.end())
    {
        attachIter->second = 0;
        ++attachIter;
    }
    m_attachedData.clear();

    efd::map<SceneGraphHandle, SceneGraphDataPtr>::iterator handleIter = m_handleData.begin();
    while (handleIter != m_handleData.end())
    {
        handleIter->second = 0;
        ++handleIter;
    }
    m_handleData.clear();

    assetIDIter = m_pendingAssetLoads.begin();
    while (assetIDIter != m_pendingAssetLoads.end())
    {
        assetIDIter->second = 0;
        ++assetIDIter;
    }
    m_pendingAssetLoads.clear();

    NiDelete m_pBatchedUpdateProcess;
    NiDelete m_pUpdateProcess;

    m_spSceneGraphCache->Shutdown();
    m_spSceneGraphCache = 0;

    m_pUpdateProcess = 0;
    m_spWorkflowManager = 0;

    m_spRenderService = NULL;

    RenderableModel::Shutdown();

#ifndef EE_CONFIG_SHIPPING
    SnapshotGenerator::Destroy();
#endif

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SceneGraphService::GetObjectCountFromEntity(const egf::Entity* pEntity) const
{
    SceneGraphData* pData = GetEntityData(pEntity);
    if (!pData)
        return 0;

    return (efd::UInt32)pData->m_objects.size();
}

//------------------------------------------------------------------------------------------------
NiObject* SceneGraphService::GetObjectFromEntity(
    const egf::Entity* pEntity,
    efd::UInt32 objectIndex) const
{
    SceneGraphData* pData = GetEntityData(pEntity);
    if (!pData)
        return NULL;

    efd::UInt32 size = (efd::UInt32)pData->m_objects.size();
    if (size <= objectIndex)
        return NULL;

    return pData->m_objects[objectIndex];
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& SceneGraphService::GetSceneGraphPathFromEntity(
    const egf::Entity* pEntity) const
{
    SceneGraphData* pData = GetEntityData(pEntity);
    if (pData && pData->m_spAssetData)
    {
        return m_spSceneGraphCache->GetFilePath(pData->m_spAssetData->m_cacheHandle);
    }

    return efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
NiAVObject* SceneGraphService::GetSceneGraphFromEntity(const egf::Entity* pEntity) const
{
    SceneGraphData* pData = GetEntityData(pEntity);
    if (!pData)
        return NULL;

    return pData->GetSceneGraph();
}

//------------------------------------------------------------------------------------------------
egf::Entity* SceneGraphService::GetEntityFromSceneGraph(const NiAVObject* pObject) const
{
    efd::map<egf::Entity*, SceneGraphDataPtr>::const_iterator itor = m_entityData.begin();

    while (itor != m_entityData.end())
    {
        if (itor->second->GetSceneGraph() == pObject)
            return itor->first;

        ++itor;
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
egf::Entity* SceneGraphService::GetEntityFromSceneGraphNode(const NiAVObject* pObject) const
{
    const NiAVObject* pParent = pObject;
    while (pParent->GetParent())
        pParent = pParent->GetParent();

    return GetEntityFromSceneGraph(pParent);
}

//------------------------------------------------------------------------------------------------
egf::Entity* SceneGraphService::GetEntityFromObject(const NiObject* pObject) const
{
    efd::map<egf::Entity*, SceneGraphDataPtr>::const_iterator itor = m_entityData.begin();

    while (itor != m_entityData.end())
    {
        // Get the list of objects for this entity
        const efd::vector<NiObjectPtr>& objects = itor->second->m_objects;

        // Search the list for the passed in object
        efd::vector<NiObjectPtr>::const_iterator found_item =
            objects.find(const_cast<NiObject*>(pObject));

        // Check if we've found a match. If so, return the entity we're checking
        if (found_item != objects.end())
            return itor->first;

        ++itor;
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::ForEachEntitySceneGraph(EntitySceneGraphFunctor& functor)
{
    efd::map<egf::Entity*, SceneGraphDataPtr>::iterator itor;

    for (itor = m_entityData.begin(); itor != m_entityData.end(); ++itor)
    {
        SceneGraphData* pEntityData = itor->second;
        if (!pEntityData || (pEntityData->m_objects.size() == 0))
            continue;

        if (functor(itor->first, pEntityData->m_objects))
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& SceneGraphService::GetSceneGraphPathFromHandle(
    const SceneGraphHandle handle)
{
    efd::map<SceneGraphHandle, SceneGraphDataPtr>::iterator iter = m_handleData.find(handle);

    if (iter != m_handleData.end())
    {
        SceneGraphData* pData = (SceneGraphData*)iter->second;
        if (pData && pData->m_spAssetData)
        {
            return m_spSceneGraphCache->GetFilePath(pData->m_spAssetData->m_cacheHandle);
        }
    }

    return efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
NiAVObject* SceneGraphService::GetSceneGraphFromHandle(const SceneGraphHandle handle)
{
    efd::map<SceneGraphHandle, SceneGraphDataPtr>::iterator iter = m_handleData.find(handle);

    if (iter != m_handleData.end())
    {
        // Cached assets don't have a cloned scene graph.
        if (iter->second->m_objects.size() == 0)
            return NULL;

        return iter->second->GetSceneGraph();
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SceneGraphService::GetObjectCountFromHandle(const SceneGraphHandle handle)
{
    efd::map<SceneGraphHandle, SceneGraphDataPtr>::iterator iter = m_handleData.find(handle);

    if (iter != m_handleData.end())
    {
        return (efd::UInt32)iter->second->m_objects.size();
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
NiObject* SceneGraphService::GetObjectFromHandle(const SceneGraphHandle handle,
    efd::UInt32 objectIndex)
{
    efd::map<SceneGraphHandle, SceneGraphDataPtr>::iterator iter = m_handleData.find(handle);

    if (iter != m_handleData.end())
    {
        efd::UInt32 size = (efd::UInt32)iter->second->m_objects.size();
        if (size <= objectIndex)
            return (NiObject*)0;

        return iter->second->m_objects[objectIndex];
    }

    return (NiObject*)0;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::ForEachHandleSceneGraph(HandleSceneGraphFunctor& functor)
{
    efd::map<SceneGraphHandle, SceneGraphDataPtr>::iterator itor;

    for (itor = m_handleData.begin(); itor != m_handleData.end(); ++itor)
    {
        SceneGraphData* pData = itor->second;
        if (!pData || (pData->m_objects.size() == 0))
            continue;

        if (functor(itor->first, pData->m_objects))
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::ForEachUpdatedEntity(EntitySceneGraphFunctor& functor)
{
    efd::vector<SceneGraphData*>::iterator updateIterator = m_toUpdate.begin();
    while (updateIterator != m_toUpdate.end())
    {
        SceneGraphData* pData = *updateIterator;

        if (pData->IsEntity())
        {
            if (functor(pData->m_pEntity, pData->m_objects))
                return true;
        }

        ++updateIterator;
    }

    updateIterator = m_toUpdateOnce.begin();
    while (updateIterator != m_toUpdateOnce.end())
    {
        SceneGraphData* pData = *updateIterator;

        if (pData->IsEntity())
        {
            if (functor(pData->m_pEntity, pData->m_objects))
                return true;
        }

        ++updateIterator;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::ForEachUpdatedHandle(HandleSceneGraphFunctor& functor)
{
    efd::vector<SceneGraphData*>::iterator updateIterator = m_toUpdate.begin();
    while (updateIterator != m_toUpdate.end())
    {
        SceneGraphDataPtr spData = *updateIterator;

        if (!spData->IsEntity())
        {
            if (functor(spData->m_handle, spData->m_objects))
                return true;
        }

        ++updateIterator;
    }

    updateIterator = m_toUpdateOnce.begin();
    while (updateIterator != m_toUpdateOnce.end())
    {
        SceneGraphDataPtr spData = *updateIterator;

        if (!spData->IsEntity())
        {
            if (functor(spData->m_handle, spData->m_objects))
                return true;
        }

        ++updateIterator;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::HasNifAssetProperty(
    const egf::Entity* pEntity,
    efd::AssetID& sceneGraphAssetID)
{
    if (!pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Mesh))
    {
        return false;
    }

    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_NifAsset, sceneGraphAssetID) !=
        PropertyResult_OK)
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
            ("NifAsset property missing for entity model: %s", pEntity->GetModelName().c_str()));
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::AddEntityWithMeshModel(egf::Entity* pEntity)
{
    EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
        ("SceneGraphService::AddEntityWithMeshModel: entity ID: %s",
        pEntity->GetEntityID().ToString().c_str()));

    // Create EntityData to track this entity.
    SceneGraphData* pData = CreateEntityData(pEntity);

    if (!pData)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: entity added twice! ID: %s",
            __FUNCTION__,
            pEntity->GetEntityID().ToString().c_str()));
        return;
    }

    // All entities found this way start off with the same in/out world status as their entity
    pData->SetIsInWorld(pEntity->IsInWorld());

    // We do pose updates for all scene graphs added this way.
    pData->SetDoPoseUpdate(true);

    // Get the asset ID
    efd::AssetID assetID;
    EE_VERIFY(HasNifAssetProperty(pEntity, assetID));

    if (assetID.GetURN().empty())
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR2,
            ("NifAsset property value empty for entity ID: %s",
            pEntity->GetEntityID().ToString().c_str()));
        return;
    }

    AssetData* pAssetData = FindAssetData(assetID.GetURN());
    if (!pAssetData)
    {
        // This will launch an asset load request for the asset
        pAssetData = CreateAssetData(assetID.GetURN());
    }

    if (!pAssetData)
        return;

    // Look for attached nif assets and start to load them if not already available. We do this
    // before adding the entity to the asset data so that it correctly knows whether the
    // attachments are all ready.
    CreateAttachedSceneGraphs(pEntity);

    // This will add the entity to the asset data. If the asset is ready, it will set up
    // this entities objects, otherwise we just record that the entity needs the asset.
    // This function may generate messages when the asset is ready.
    AddEntityToAssetData(pAssetData, pData);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveEntityWithMeshModel(egf::Entity* pEntity)
{
    RemoveSceneGraph(pEntity);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::OnPropertyUpdate(
    const egf::FlatModelID& modelID,
    egf::Entity* pEntity,
    const egf::PropertyID& propertyID,
    const IProperty* pProperty,
    const efd::UInt32 tags)
{
    EE_UNUSED_ARG(tags);

    SceneGraphData* pData = GetEntityData(pEntity);
    if (!pData)
        return;

    NiAVObject* pNode = pData->GetSceneGraph();
    if (!pNode)
        return;

    if (modelID == kFlatModelID_StandardModelLibrary_Placeable &&
		pData->DoPoseUpdate() && 
		!pData->IsAssetExternal())
    {
        const PlaceableModel* pPlaceable = static_cast<const PlaceableModel*>(pProperty);
        EE_ASSERT(pPlaceable);

        switch (propertyID)
        {
        case kPropertyID_StandardModelLibrary_Position:
            {
                Point3 position = pPlaceable->GetPosition();
                pNode->SetTranslate(position.x, position.y, position.z);
            }
            break;

        case kPropertyID_StandardModelLibrary_Rotation:
            {
                Point3 rotation = pPlaceable->GetRotation();
                NiMatrix3 kXRot, kYRot, kZRot;
                kXRot.MakeXRotation(rotation.x * -EE_DEGREES_TO_RADIANS); // Roll  +x
                kYRot.MakeYRotation(rotation.y * -EE_DEGREES_TO_RADIANS); // Pitch +y
                kZRot.MakeZRotation(rotation.z * -EE_DEGREES_TO_RADIANS); // Yaw   +z
                pNode->SetRotate(kXRot * kYRot * kZRot);
            }
            break;

        case kPropertyID_StandardModelLibrary_Scale:
            {
                float scale = pPlaceable->GetScale();
                pNode->SetScale(scale);
            }
            break;

        default:
            break;
        }
    }
    else if (modelID == kFlatModelID_StandardModelLibrary_Renderable && pData->IsRenderable())
    {
        const RenderableModel* pRenderable = static_cast<const RenderableModel*>(pProperty);
        EE_ASSERT(pRenderable);

        switch (propertyID)
        {
        case kPropertyID_StandardModelLibrary_IsVisible:
            {
                bool isVisible = pRenderable->GetIsVisible();

                if (pNode->GetAppCulled() != (!isVisible))
                {
                    pNode->SetAppCulled(!isVisible);

                    // Check to see if the Entity casts shadows. If so we need to inform the 
                    // shadowing system that the entity's visibility has changed so the shadowing 
                    // system will recompute its shadow casters list.
                    bool bCastShadows;
                    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsCastingShadow,
                        bCastShadows) == PropertyResult_OK)
                    {
                        if (bCastShadows && NiShadowManager::GetShadowManager())
                        {
                            const NiTPointerList<NiShadowGeneratorPtr>& kGeneratorList =
                                NiShadowManager::GetShadowGenerators();
                            NiTListIterator kIter = kGeneratorList.GetHeadPos();

                            // Iterate through all the shadow generators and invalidate their
                            // cached render views.
                            while (kIter)
                            {
                                NiShadowGenerator* pkGenerator = kGeneratorList.GetNext(kIter);
                                pkGenerator->SetRenderViewsDirty(true);
                            }
                        }
                    }
                }
            }
            break;
		case kPropertyID_StandardModelLibrary_Queue:
			if (pData->IsInWorld())
			{
				efd::UInt32 queueID;
				if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Queue, queueID) ==
					PropertyResult_OK)
				{
					m_spRenderService->UpdateRenderedEntity(pEntity, pNode);
				}
			}
			break;
        default:
            break;
        }
    }

    if (!pData->IsDynamic() && pData->IsInWorld())
        AddToUpdateOnce(pData);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::HandleEntityEnterWorldMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);

    Entity* pEntity = pMessage->GetEntity();
    EE_ASSERT(pEntity);

    SceneGraphData* pSceneData = GetEntityData(pEntity);
    if (!pSceneData)
    {
        EE_ASSERT(!pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Mesh));
        return;
    }

    UpdateDiscoveredEntity(pEntity);

    EntityEnterWorld(pSceneData);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::CreateAttachedSceneGraphs(egf::Entity* pEntity)
{
    efd::list<efd::utf8string> keyList;
    if (pEntity->GetPropertyKeys(kPropertyID_StandardModelLibrary_AttachedObjects, keyList) ==
        PropertyResult_OK)
    {
        for (efd::list<efd::utf8string>::iterator iter = keyList.begin();
            iter != keyList.end();
            ++iter)
        {
            AttachNifData attachData;
            pEntity->GetPropertyValue(
                kPropertyID_StandardModelLibrary_AttachedObjects,
                *iter,
                attachData);

            if (!AttachSceneGraph(pEntity, *iter))
            {
                EE_LOG(efd::kGamebryoGeneral0,
                    efd::ILogger::kERR1,
                    ("%s: Unable to attach scene graph '%s' to entity '%s'.",
                    __FUNCTION__,
                    attachData.GetNifAsset().GetURN().c_str(),
                    pEntity->GetEntityID().ToString().c_str()));
                continue;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::HandleEntityExitWorldMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_UNUSED_ARG(targetChannel);

    Entity* pEntity = pMessage->GetEntity();
    EE_ASSERT(pEntity);

    SceneGraphData* pSceneData = GetEntityData(pEntity);
    if (!pSceneData)
        return;

    EntityExitWorld(pSceneData);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::HandleEntityUpdatedMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);

    egf::Entity* pEntity = pMessage->GetEntity();
    EE_ASSERT(pEntity);

    UpdateDirtyProperties(pEntity);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::ProcessSceneGraphCacheReload(const SceneGraphCacheResponse* pMessage)
{
    // We only get these for reload requests
    EE_ASSERT(pMessage->GetHandleCount() == 1);

    const utf8string& physicalID = m_spSceneGraphCache->GetPhysicalID(pMessage->GetHandle(0));
    const utf8string& fileName = m_spSceneGraphCache->GetFilePath(pMessage->GetHandle(0));
    AssetDataPtr spAssetData = FindAssetData(physicalID);
    if (!spAssetData)
    {
        spAssetData = FindAssetData(fileName);
    }
    if (!spAssetData)
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kLVL3,
            ("%s: Asset Refresh requested for an asset that we do not know about. "
            "Asset ID: %s, file name: %s",
            __FUNCTION__,
            physicalID.c_str(),
            fileName.c_str()));
        return;
    }

    spAssetData->m_state = AssetData::ADS_Reloading;
    SetAssetDataFromCache(spAssetData, pMessage->GetHandle(0), false);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::HandleCacheResponse(
    const ecr::SceneGraphCacheResponse* pResponse,
    efd::Category targetChannel)
{
    if (targetChannel == m_assetRefreshCategory)
    {
        EE_ASSERT(pResponse->GetRequestData() == 0);
        ProcessSceneGraphCacheReload(pResponse);
        return;
    }

    SceneGraphCacheRequestDataPtr spCRData =
        (SceneGraphCacheRequestData*)pResponse->GetRequestData();
    EE_ASSERT(spCRData);

    if (pResponse->GetHandleCount() == 0)
    {
        // This situation only arises if the desired file cannot be located at all, in which case
        // it is not in the cache and we should remove all references to it, as it no longer
        // exists.
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR1,
            ("%s: Found no assets when looking for identifier '%s'. "
            "Reloads or other change notifications for this identifier will be ignored and "
            "existing assets using this identifer will have their scene graphs removed. ",
            __FUNCTION__,
            pResponse->GetIdentifier().c_str()));

        m_pendingAssetLoads.erase(pResponse->GetIdentifier());

        // Remove any existing asset data for this asset. The m_pAssetData pointer is not set by
        // Cache or CacheSceneGraphFileName so only do this when we have m_pAssetData set.
        if (spCRData->m_pAssetData)
        {
            OnAssetLoadFailure(spCRData->m_pAssetData);
        }
        else
        {
            EE_ASSERT(spCRData->m_action == SGCRA_CacheSceneGraph);
        }

        // Some operations still need to send responses, so don't always exit.
        if (spCRData->m_action != SGCRA_CacheSceneGraph)
        {
            return;
        }
    }

    if (spCRData->m_action == SGCRA_AssetLoadRequest)
    {
        if (pResponse->GetHandleCount() > 1)
        {
            EE_LOG(efd::kGamebryoGeneral0,
                efd::ILogger::kERR1,
                ("%s: Found %d assets when looking for tag '%s'. "
                "Ignoring all but the first.",
                __FUNCTION__,
                pResponse->GetHandleCount(),
                pResponse->GetIdentifier().c_str()));
        }

        spCRData->m_pAssetData =
            SetAssetDataFromCache(spCRData->m_pAssetData, pResponse->GetHandle(0), false);
    }
    else if (spCRData->m_action == SGCRA_CacheSceneGraph)
    {
        SceneGraphCacheResponse* pNewResponse = EE_NEW SceneGraphCacheResponse(
            pResponse->GetIdentifier(),
            spCRData->m_responseCategory,
            spCRData->m_pRequestData,
            pResponse->GetResult(),
            pResponse->GetType());

        if (pResponse->GetHandleCount() == 1)
        {
            m_requestIDToPhysicalID[pResponse->GetIdentifier()] =
                m_spSceneGraphCache->GetPhysicalID(pResponse->GetHandle(0));
        }

        vector<SceneGraphCache::SceneGraphCacheHandle> handles;
        for (UInt32 ui = 0; ui < pResponse->GetHandleCount(); ++ui)
        {
            handles.push_back(pResponse->GetHandle(ui));
            pNewResponse->AddHandle(pResponse->GetHandle(ui));
        }
        CacheHandles(handles);

        if (spCRData->m_responseCategory.IsValid())
        {
            // Forward the message to the given response category
            m_pMessageService->SendLocal(pNewResponse, spCRData->m_responseCategory);
        }
        else
        {
            EE_DELETE pNewResponse;
        }
    }

    return;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::HandlePreloadRequest(
    const egf::EntityPreloadRequest* pRequest,
    efd::Category targetChannel)
{
    EE_UNUSED_ARG(targetChannel);

    // Examine the entity for assets we need to preload
    Entity* pEntity = pRequest->GetEntity();
    EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
        ("Scene graph handling preload of entity %s",
         pEntity->GetModel()->GetName().c_str()));

    // We should have been informed that the entity exists from the Mesh built-in model.
    SceneGraphData* pEntityData = GetEntityData(pEntity);
    if (!pEntityData || pEntityData->IsAssetExternal())
    {
        EntityPreloadResponse* pResponse = EE_NEW EntityPreloadResponse;
        pResponse->m_pEntity = pEntity;
        m_pMessageService->SendLocal(pResponse, m_assetPreloadResponseCategory);
        return;
    }

    EntityPreloadDataPtr spData = EE_NEW EntityPreloadData;
    spData->m_pEntityData = pEntityData;
    spData->m_assetsNeeded = 0;
    spData->m_assetsFound = 0;
    spData->m_responseCategory = m_assetPreloadResponseCategory;

    efd::AssetID assetID;
    if (HasNifAssetProperty(pEntity, assetID) && !assetID.GetURN().empty())
    {
        Preload(spData, assetID.GetURN());
    }

    // Look for attached nif assets and attempt to load them
    PreloadAttachedSceneGraphs(spData);

    // We might already have all the needed assets
    if (spData->m_assetsNeeded == spData->m_assetsFound)
    {
        EntityPreloadResponse* pResponse = EE_NEW EntityPreloadResponse;
        pResponse->m_pEntity = pEntity;
        m_pMessageService->SendLocal(pResponse, m_assetPreloadResponseCategory);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::Preload(
    egf::Entity* pEntity,
    const efd::utf8string& filePath,
    const efd::Category& responseCategory)
{
    // Examine the entity for assets we need to preload
    EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
        ("Scene graph handling preload of entity ID %s",
         pEntity->GetEntityID().ToString().c_str()));

    // We must have been informed that the entity exists, presumably through
    // CreateSceneGraphFilename.
    SceneGraphData* pEntityData = GetEntityData(pEntity);
    if (!pEntityData || pEntityData->IsAssetExternal())
    {
        // Examine the entity for assets we need to preload
        if (!pEntityData)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                ("%s: Preload requested for unknown entity ID %s. You must create the "
                "scene graph for the entity with CreateSceneGraphFilename before preloading.",
                __FUNCTION__,
                pEntity->GetEntityID().ToString().c_str()));
        }

        EntityPreloadResponse* pResponse = EE_NEW EntityPreloadResponse;
        pResponse->m_pEntity = pEntity;
        m_pMessageService->SendLocal(pResponse, responseCategory);
        return;
    }

    EntityPreloadDataPtr spData = EE_NEW EntityPreloadData;
    spData->m_pEntityData = pEntityData;
    spData->m_assetsNeeded = 0;
    spData->m_assetsFound = 0;
    spData->m_responseCategory = responseCategory;

    if (!filePath.empty())
    {
        Preload(spData, filePath);
    }

    // Look for attached nif assets and attempt to load them
    PreloadAttachedSceneGraphs(spData);

    // We might already have all the needed assets
    if (spData->m_assetsNeeded == spData->m_assetsFound)
    {
        if (responseCategory.IsValid())
        {
            EntityPreloadResponse* pResponse = EE_NEW EntityPreloadResponse;
            pResponse->m_pEntity = spData->m_pEntityData->m_pEntity;
            m_pMessageService->SendLocal(pResponse, responseCategory);
        }
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::PreloadAttachedSceneGraphs(EntityPreloadData* pData)
{
    efd::list<efd::utf8string> keyList;
    if (pData->m_pEntityData->m_pEntity->GetPropertyKeys(
        kPropertyID_StandardModelLibrary_AttachedObjects,
        keyList) == PropertyResult_OK)
    {
        for (efd::list<efd::utf8string>::iterator iter = keyList.begin();
            iter != keyList.end();
            ++iter)
        {
            AttachNifData attachData;
            pData->m_pEntityData->m_pEntity->GetPropertyValue(
                kPropertyID_StandardModelLibrary_AttachedObjects,
                *iter,
                attachData);

            if (attachData.GetNifAsset().GetURN().empty())
                continue;

            if (attachData.GetNifAsset().IsValidURN())
            {
                Preload(pData, attachData.GetNifAsset());
            }
        }
    }

}

//------------------------------------------------------------------------------------------------
void SceneGraphService::Preload(EntityPreloadData* pData, const efd::utf8string& assetURN)
{
    ++(pData->m_assetsNeeded);

    // Make asset data if it doesn't already exist. It should, but maybe the asset was changed
    // between Mesh built-in model initialization and preload.
    AssetData* pAssetData = FindAssetData(assetURN);
    if (!pAssetData)
    {
        pAssetData = CreateAssetData(assetURN);
    }

    if (pAssetData->m_state == AssetData::ADS_Ready)
    {
        // Increment the cached count
        ++(pData->m_assetsFound);
    }
    else
    {
        pAssetData->m_preloads.push_back(pData);
    }
}

//------------------------------------------------------------------------------------------------
// Update functions
//------------------------------------------------------------------------------------------------
void SceneGraphService::EntityEnterWorld(SceneGraphData* pSceneGraphData)
{
    EE_ASSERT(pSceneGraphData->IsEntity());

    if (pSceneGraphData->IsInWorld())
        return;

    NiAVObject* pAVObject = pSceneGraphData->GetSceneGraph();
    if (pAVObject)
    {
        if (pSceneGraphData->IsRenderable() && m_spRenderService)
        {
            // Tell the renderer, if rendered
            m_spRenderService->AddRenderedEntity(
                pSceneGraphData->m_pEntity,
                pAVObject);
            pSceneGraphData->SetIsRendered(true);
        }
        else
        {
            pSceneGraphData->SetIsRendered(false);
        }

        if (pSceneGraphData->IsDynamic())
        {
            AddToUpdate(pSceneGraphData);
        }
        else
        {
            AddToUpdateOnce(pSceneGraphData);
        }
    }

    pSceneGraphData->SetIsInWorld(true);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::EntityExitWorld(SceneGraphData* pSceneGraphData)
{
    EE_ASSERT(pSceneGraphData->IsEntity());

    if (!pSceneGraphData->IsInWorld())
        return;

    // Take the entity out of the rendered world
    if (pSceneGraphData->IsRendered())
    {
        EE_ASSERT(m_spRenderService);
        m_spRenderService->RemoveRenderedEntity(
            pSceneGraphData->m_pEntity,
            pSceneGraphData->GetSceneGraph());
        pSceneGraphData->SetIsRendered(false);
    }

    NiAVObject* pAVObject = pSceneGraphData->GetSceneGraph();
    if (pAVObject)
    {
        RemoveFromUpdate(pSceneGraphData);
        RemoveFromUpdateOnce(pSceneGraphData);
    }

    pSceneGraphData->SetIsInWorld(false);
}

//------------------------------------------------------------------------------------------------
bool SceneGraphService::EnablePlaceableFeedback(
    egf::PlaceableModel* pPlaceable,
    egf::Entity* pSourceEntity,
    NiAVObject* pSourceNode,
    egf::IPropertyCallback* pIgnoreCallback,
    bool ignoreScale)
{
    EE_ASSERT(pSourceEntity);
    EE_ASSERT(pSourceNode);

    SceneGraphDataPtr spSGData = 0;
    if (!m_entityData.find(pSourceEntity, spSGData))
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR1,
            ("%s: Scene Graph Service does not know of feedback source entity (ID=%s).",
            __FUNCTION__,
            pSourceEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    if (!SceneGraphContains(spSGData->GetSceneGraph(), pSourceNode))
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR1,
            ("%s: Feedback source entity (ID=%s) does not contain node (name %s).",
            __FUNCTION__,
            pSourceEntity->GetEntityID().ToString().c_str(),
            (const char*)pSourceNode->GetName()));
        return false;
    }

    if (spSGData->DoPoseUpdate() && pSourceEntity == pPlaceable->GetOwningEntity())
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR1,
            ("%s: Unable to enable placeable feedback of entity (ID=%s) on itself when that "
            "entity has the PoseUpdate setting enabled, as it would result in a feedback loop.",
            __FUNCTION__,
            pSourceEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    PlaceableFeedbackData feedbackData;
    feedbackData.m_pPlaceable = pPlaceable;
    feedbackData.m_pNode = pSourceNode;
    feedbackData.m_pIgnoreCallback = pIgnoreCallback;
    feedbackData.m_doScale = !ignoreScale;
    spSGData->m_placeableFeedback.push_back(feedbackData);

    // Look for an entity owning the placeable to modify update order
    SceneGraphDataPtr spPlaceableSGData = 0;
    if (m_entityData.find(pPlaceable->GetOwningEntity(), spPlaceableSGData))
    {
        if (spPlaceableSGData->m_updateIndex < spSGData->m_updateIndex)
        {
            SwapUpdateOrder(spPlaceableSGData, spSGData);
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool SceneGraphService::DisablePlaceableFeedback(
    egf::PlaceableModel* pPlaceable,
    egf::Entity* pSourceEntity,
    NiAVObject* pSourceNode)
{
    EE_ASSERT(pSourceEntity);
    EE_ASSERT(pSourceNode);

    SceneGraphDataPtr spSGData = 0;
    if (!m_entityData.find(pSourceEntity, spSGData))
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR1,
            ("%s: Scene Graph Service does not know of feedback source entity (ID=%s).",
            __FUNCTION__,
            pSourceEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    vector<PlaceableFeedbackData>::iterator iter = spSGData->m_placeableFeedback.begin();
    while (iter != spSGData->m_placeableFeedback.end())
    {
        if ((*iter).m_pPlaceable == pPlaceable && (*iter).m_pNode == pSourceNode)
        {
            spSGData->m_placeableFeedback.erase(iter);
            return true;
        }
        ++iter;
    }

    // Didn't find the data.
    EE_LOG(efd::kGamebryoGeneral0,
        efd::ILogger::kERR1,
        ("%s: Unable to disable placeable feedback because placeable is not known to service.",
        __FUNCTION__));
    return false;
}

//------------------------------------------------------------------------------------------------
bool SceneGraphService::SceneGraphContains(NiAVObject* pSceneGraph, NiAVObject* pTarget)
{
    if (pTarget == pSceneGraph)
        return true;

    NiNode* pNode = NiDynamicCast(NiNode, pSceneGraph);
    if (pNode)
    {
        UInt32 childCount = pNode->GetArrayCount();
        for (UInt32 ui = 0; ui < childCount; ++ui)
        {
            if (SceneGraphContains(pNode->GetAt(ui), pTarget))
            {
                return true;
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveAttachmentPlaceableFeedback(AttachedSceneGraphData* pSceneData)
{
    SceneGraphData* pEntityData = pSceneData->m_pEntityData;

    vector<PlaceableFeedbackData>::iterator iter = pEntityData->m_placeableFeedback.begin();
    while (iter != pEntityData->m_placeableFeedback.end())
    {
        NiAVObject* pTarget = (*iter).m_pNode;
        NiAVObject* pSceneGraph = pSceneData->GetSceneGraph();
        if (SceneGraphContains(pSceneGraph, pTarget))
        {
            iter = pEntityData->m_placeableFeedback.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UpdateDiscoveredEntity(egf::Entity* pEntity)
{
    // Only do work if we know about the scene graph
    SceneGraphData* pSceneGraphData = GetEntityData(pEntity);

    // Ignore entities we don't know about
    if (!pSceneGraphData)
        return;

    // We only update the properties of entities we are managing.
    if (pSceneGraphData->IsAssetExternal())
        return;

    // Get the asset data
    AssetData* pAssetData = pSceneGraphData->m_spAssetData;

    // Look for a change in the scene graph, if it even has such a property. Scene graphs added
    // for non Mesh model entities will not have this property.
    efd::AssetID sceneGraphName;
    if (HasNifAssetProperty(pEntity, sceneGraphName))
    {
        // If the asset ID is invalid, remove the actor data
        if (!sceneGraphName.IsValidURN())
        {
            if (pAssetData)
            {
                RemoveEntityFromAssetData(pSceneGraphData);
            }
        }
        else
        {
            // Look for asset data for the current name
            AssetDataPtr spNewAssetData = FindAssetData(sceneGraphName);

            if (!spNewAssetData || (spNewAssetData != pAssetData))
            {
                ReplaceAsset(pSceneGraphData, spNewAssetData, sceneGraphName);
            }
        }
    }

    UpdateAttachedProperty(pEntity);

    UpdateNonVolatileProperties(pEntity, pSceneGraphData);

    NiAVObject* pObject = pSceneGraphData->GetSceneGraph();
    if (pObject)
    {
        UpdateVolatileProperties(pEntity, pObject);

        // Make sure we do an update even for static objects
        if (!pSceneGraphData->IsDynamic() && pSceneGraphData->IsInWorld())
            AddToUpdateOnce(pSceneGraphData);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UpdateDirtyProperties(egf::Entity* pEntity)
{
    // Only do work if we know about the scene graph
    SceneGraphData* pSceneGraphData = GetEntityData(pEntity);

    // Ignore entities we don't know about
    if (!pSceneGraphData)
        return;

    // We only update the properties of entities we are managing.
    if (pSceneGraphData->IsAssetExternal())
        return;

    // Get the asset data
    AssetData* pAssetData = pSceneGraphData->m_spAssetData;

    // Look for a change in the scene graph, if it even has such a property. Scene graphs added
    // for non Mesh model entities will not have this property.
    efd::AssetID sceneGraphName;
    if (HasNifAssetProperty(pEntity, sceneGraphName) &&
        pEntity->IsDirty(kPropertyID_StandardModelLibrary_NifAsset))
    {
        // If the asset ID is invalid, remove the actor data
        if (!sceneGraphName.IsValidURN())
        {
            if (pAssetData)
            {
                RemoveEntityFromAssetData(pSceneGraphData);
            }
        }
        else
        {
            // Look for asset data for the current name
            AssetDataPtr spNewAssetData = FindAssetData(sceneGraphName);
            if (!spNewAssetData || (spNewAssetData != pAssetData))
            {
                ReplaceAsset(pSceneGraphData, spNewAssetData, sceneGraphName);
            }
        }
    }

    // Dirty attachments?
    if (pEntity->IsDirty(kPropertyID_StandardModelLibrary_AttachedObjects))
    {
        UpdateAttachedProperty(pEntity);
    }

    // Update infrequently changed properties.
    if (pEntity->IsDirty(kPropertyID_StandardModelLibrary_IsStatic) ||
        pEntity->IsDirty(kPropertyID_StandardModelLibrary_IsNifAssetShared))
    {
        UpdateNonVolatileProperties(pEntity, pSceneGraphData);
    }

    // Frequently updated properties from built-in models are handled via the callbacks.
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UpdateAttachedProperty(egf::Entity* pEntity)
{
    // Get the data for the scene, if any
    SceneGraphData* pSceneGraphData = GetEntityData(pEntity);

    if (!pSceneGraphData)
        return;

    // Need to keep track of which existing attachments we have new data for
    efd::vector<utf8string> seenAttachments;

    // Walk through all the attached objects in the data and see what has changed from
    // our current information
    efd::list<efd::utf8string> keyList;
    if (pEntity->GetPropertyKeys(kPropertyID_StandardModelLibrary_AttachedObjects, keyList) ==
        PropertyResult_OK)
    {
        for (efd::list<efd::utf8string>::iterator iter = keyList.begin();
            iter != keyList.end();
            ++iter)
        {
            AttachNifData attachData;
            if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects, *iter,
                attachData) != PropertyResult_OK)
            {
                continue;
            }

            // Look for this attachment in our known list
            efd::map<utf8string, AttachedSceneGraphDataPtr>::iterator iterAttached =
                pSceneGraphData->m_attached.find(*iter);
            if (iterAttached == pSceneGraphData->m_attached.end())
            {
                // Create new attachment data ...
                if (!AttachSceneGraph(pEntity, *iter))
                {
                    EE_LOG(efd::kGamebryoGeneral0,
                        efd::ILogger::kERR1,
                        ("%s: Unable to attach scene graph '%s' to entity '%s'.",
                        __FUNCTION__,
                        attachData.GetNifAsset().GetURN().c_str(),
                        pEntity->GetEntityID().ToString().c_str()));
                }
            }
            else
            {
                // We have some data for this attachment
                UpdateAttachmentPropertyData(pEntity, *iter, iterAttached->second, attachData);
            }
            seenAttachments.push_back(*iter);
        }
    }

    // Walk through the data we already have and remove any that no longer appear in the
    // properties.
    efd::map<utf8string, AttachedSceneGraphDataPtr>::iterator attachedIter =
        pSceneGraphData->m_attached.begin();
    while (attachedIter != pSceneGraphData->m_attached.end())
    {
        vector<utf8string>::iterator seenIter = seenAttachments.find(attachedIter->first);
        if (seenIter != seenAttachments.end())
        {
            ++attachedIter;
            continue;
        }

        // We face a problem removing the attachment, in that it will reset the
        // iterator we are holding. The way around this, slightly inefficient, is
        // to reset the iterator back to the start and re-iterate over things we
        // have already processed.
        RemoveAttachmentData(pSceneGraphData, attachedIter->second->m_attachmentKey);
        attachedIter = pSceneGraphData->m_attached.begin();
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UpdateAttachmentPropertyData(
    egf::Entity* pEntity,
    const utf8string& slotName,
    AttachedSceneGraphData* pAttachData,
    const AttachNifData& attachData)
{
    EE_ASSERT(pAttachData->m_attachmentKey == slotName);

    // Check for an invalid or empty URN. If so, we are removing the existing attachment
    if (!attachData.GetNifAsset().IsValidURN())
    {
        RemoveAttachmentFromAssetData(pAttachData);
        return;
    }

    // Get the existing asset data
    AssetData* pAssetData = pAttachData->m_spAssetData;

    // Look for the new asset data
    AssetData* pNewAssetData = FindAssetData(attachData.GetNifAsset().GetURN());

    // See if the asset has changed
    if (pNewAssetData && (pNewAssetData == pAssetData))
    {
        // Same asset. Update other properties.
        NiAVObject* pAttachSceneGraph = pAttachData->GetSceneGraph();
        if (!pAttachSceneGraph)
        {
            EE_LOG(efd::kGamebryoGeneral0,
                efd::ILogger::kERR1,
                ("%s: Node to attach at attachment point '%s' is not an NiAVObject.",
                __FUNCTION__,
                attachData.GetAttachPoint().c_str()));

            // Detach any existing scene graph
            DetachSceneGraph(pEntity, slotName);
            return;
        }

        NiAVObject* pEntitySceneGraph = GetSceneGraphFromEntity(pEntity);

        // Check if we have to attach it someplace else
        const char* pAttachPoint = attachData.GetAttachPoint().c_str();
        NiAVObject* pTargetAVParent = NULL;
        if (pAttachPoint != NULL && NiStricmp(pAttachPoint, "") != 0)
            pTargetAVParent = pEntitySceneGraph->GetObjectByName(pAttachPoint);
        else
            pTargetAVParent = pEntitySceneGraph;

        NiNode* pTargetParent = NiDynamicCast(NiNode, pTargetAVParent);;
        if (!pTargetParent)
        {
            EE_LOG(efd::kGamebryoGeneral0,
                efd::ILogger::kERR1,
                ("%s: Cannot find attachment point '%s', or node is not an NiNode.",
                __FUNCTION__,
                attachData.GetAttachPoint().c_str()));

            DetachSceneGraph(pEntity, slotName);

            return;
        }

        NiNode* pCurrentParent = pAttachSceneGraph->GetParent();
        EE_ASSERT(pCurrentParent);

        SceneGraphData* pSceneGraphData = GetEntityData(pEntity);
        EE_ASSERT(pSceneGraphData);

        if (pCurrentParent != pTargetParent)
        {
            // We need to attach it someplace else, which will deal with scene graph update.

            DetachSceneGraph(pEntitySceneGraph, pAttachSceneGraph);
            AttachSceneGraph(pEntitySceneGraph, pAttachSceneGraph, attachData);
        }
        else
        {
            // We need to make sure the transform is updated.
            pAttachSceneGraph->SetTranslate(
                attachData.GetTranslation().x,
                attachData.GetTranslation().y,
                attachData.GetTranslation().z);

            NiMatrix3 kXRot, kYRot, kZRot;
            kXRot.MakeXRotation(attachData.GetRotation().x * -EE_DEGREES_TO_RADIANS); // Roll  +x
            kYRot.MakeYRotation(attachData.GetRotation().y * -EE_DEGREES_TO_RADIANS); // Pitch +y
            kZRot.MakeZRotation(attachData.GetRotation().z * -EE_DEGREES_TO_RADIANS); // Yaw   +z
            pAttachSceneGraph->SetRotate(kXRot * kYRot * kZRot);

            pAttachSceneGraph->SetScale(attachData.GetScale());

            if (!pSceneGraphData->IsDynamic() && pSceneGraphData->IsInWorld())
                AddToUpdateOnce(pSceneGraphData);
        }

        return;
    }

    // Remove the attachment from its existing asset data
    RemoveAttachmentFromAssetData(pAttachData);

    // Set up a new attachment.
    AttachSceneGraph(pEntity, slotName);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UpdateNonVolatileProperties(
    egf::Entity* pEntity,
    SceneGraphData* pSceneGraphData)
{
    EE_ASSERT(pEntity);
    EE_ASSERT(pSceneGraphData);

    efd::Bool isStatic = false;
    if (pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Bakeable) &&
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsStatic, isStatic) ==
        PropertyResult_OK &&
        isStatic == pSceneGraphData->IsDynamic())
    {
        // Change dynamic status
        NiAVObject* pAVObject = pSceneGraphData->GetSceneGraph();
        if (pSceneGraphData->IsDynamic())
        {
            if (pAVObject && pSceneGraphData->m_updateIndex != SceneGraphData::kInvalidUpdateIndex)
            {
                // Complete any modifiers, as we may not update this scene graph again.
                NiMesh::CompleteSceneModifiers(pAVObject);

                // Remove it
                RemoveFromUpdate(pSceneGraphData);
            }

            pSceneGraphData->SetIsDynamic(false);
            pSceneGraphData->m_updateIndex = SceneGraphData::kInvalidUpdateIndex;
        }
        else
        {
            pSceneGraphData->SetIsDynamic(true);

            if (pAVObject && pSceneGraphData->IsInWorld())
            {
                AddToUpdate(pSceneGraphData);
            }
        }
    }

    efd::Bool isAssetShared = false;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsNifAssetShared, isAssetShared)
        == PropertyResult_OK &&
        isAssetShared != pSceneGraphData->IsAssetShared())
    {
        // We do not support dynamically changing this property
        EE_LOG(
            efd::kGamebryoGeneral0,
            efd::ILogger::kERR2,
            ("%s: Dynamic changes to the IsNifAssetShared property are not supported. The "
            "internal value remains %s",
            __FUNCTION__,
            pSceneGraphData->IsAssetShared() ? "true" : "false"));
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UpdateVolatileProperties(
    egf::Entity* pEntity,
    NiAVObject* pNode,
    bool initialUpdate)
{
    EE_ASSERT(pEntity);
    EE_ASSERT(pNode);

    SceneGraphData* pData = GetEntityData(pEntity);

    if (pData->DoPoseUpdate() || initialUpdate)
    {
        // Apply the current entity position to the scenegraph node
        efd::Point3 position;
        if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Position, position) ==
            PropertyResult_OK)
        {
            pNode->SetTranslate(position.x, position.y, position.z);
        }

        // Apply the current entity rotation to the scenegraph node
        efd::Point3 rotation;
        if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Rotation, rotation) ==
            PropertyResult_OK)
        {
            NiMatrix3 kXRot, kYRot, kZRot;
            kXRot.MakeXRotation(rotation.x * -EE_DEGREES_TO_RADIANS); // Roll  +x
            kYRot.MakeYRotation(rotation.y * -EE_DEGREES_TO_RADIANS); // Pitch +y
            kZRot.MakeZRotation(rotation.z * -EE_DEGREES_TO_RADIANS); // Yaw   +z
            pNode->SetRotate(kXRot * kYRot * kZRot);
        }

        // Apply the current entity scale to the scenegraph node
        float scale;
        if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Scale, scale) ==
            PropertyResult_OK)
        {
            pNode->SetScale(scale);
        }
    }

    // Apply the current entity visibility to the scenegraph node
    bool isVisible;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsVisible, isVisible) ==
        PropertyResult_OK)
    {
        if (pNode->GetAppCulled() != (!isVisible))
        {
            pNode->SetAppCulled(!isVisible);

            // Check to see if the Entity casts shadows. If so we need to inform the shadowing
            // system that the entity's visibility has changed so the shadowing system will
            // recompute its shadow casters list.
            bool bCastShadows;
            if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsCastingShadow,
                bCastShadows) == PropertyResult_OK)
            {
                if (bCastShadows && NiShadowManager::GetShadowManager())
                {
                    const NiTPointerList<NiShadowGeneratorPtr>& kGeneratorList =
                        NiShadowManager::GetShadowGenerators();
                    NiTListIterator kIter = kGeneratorList.GetHeadPos();

                    // Iterate through all the shadow generators and invalidate their
                    // cached render views.
                    while (kIter)
                    {
                        NiShadowGenerator* pkGenerator = kGeneratorList.GetNext(kIter);
                        pkGenerator->SetRenderViewsDirty(true);
                    }
                }
            }
        }
    }


}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::CreateSceneGraphAssetID(
    egf::Entity* pEntity,
    const efd::AssetID& assetID,
    const efd::Bool delayInWorld,
    const efd::Bool forceSharing,
    const efd::Bool updatePose)
{
    EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
        ("%s: entity ID: %s", __FUNCTION__, pEntity->GetEntityID().ToString().c_str()));

    // Create EntityData to track this entity.
    SceneGraphData* pData = CreateEntityData(pEntity);

    if (!pData)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: entity added twice! ID: %s",
            __FUNCTION__,
            pEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    if (forceSharing)
        pData->SetIsAssetShared(true);
    pData->SetIsInWorld(!delayInWorld);
    pData->SetDoPoseUpdate(updatePose);

    if (assetID.GetURN().empty())
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR2,
            ("%s: assetID empty for entity ID: %s",
            __FUNCTION__, pEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    AssetData* pAssetData = FindAssetData(assetID.GetURN());
    if (!pAssetData)
    {
        // This will launch an asset load request for the asset
        pAssetData = CreateAssetData(assetID.GetURN());
    }

    if (!pAssetData)
        return false;

    // Create any attachments for this entity. Do it before adding the entity to the asset
    // data so that we correctly track whether or not a message should be sent.
    CreateAttachedSceneGraphs(pEntity);

    // This will add the entity to the asset data. If the asset is ready, it will set up
    // this entities objects, otherwise we just record that the entity needs the asset.
    // This function may generate messages when the asset is ready.
    AddEntityToAssetData(pAssetData, pData);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::CreateSceneGraphFileName(
    egf::Entity* pEntity,
    const efd::utf8string& fileName,
    const efd::Bool prependDataPath,
    const efd::Bool delayInWorld,
    const efd::Bool forceSharing,
    const efd::Bool updatePose)
{
    EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
        ("%s: entity ID: %s", __FUNCTION__, pEntity->GetEntityID().ToString().c_str()));

    // Create EntityData to track this entity.
    SceneGraphData* pData = CreateEntityData(pEntity);

    if (!pData)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: entity added twice! ID: %s",
            __FUNCTION__,
            pEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    if (forceSharing)
        pData->SetIsAssetShared(true);
    pData->SetIsInWorld(!delayInWorld);
    pData->SetDoPoseUpdate(updatePose);

    efd::utf8string pathToScene;
    if (prependDataPath)
    {
        pathToScene = m_strDataPath;
        pathToScene += efd::PathUtils::GetNativePathSeperator() + fileName;
    }
    else
    {
        pathToScene = fileName;
    }

    AssetData* pAssetData = FindAssetData(pathToScene);
    if (!pAssetData)
    {
        // This will launch an asset load request for the asset
        pAssetData = CreateAssetData(pathToScene);
    }

    if (!pAssetData)
        return false;

    // Create any attachments for this entity. Do it before adding the entity to the asset
    // data so that we correctly track whether or not a message should be sent.
    CreateAttachedSceneGraphs(pEntity);

    // This will add the entity to the asset data. If the asset is ready, it will set up
    // this entities objects, otherwise we just record that the entity needs the asset.
    // This function may generate messages when the asset is ready.
    AddEntityToAssetData(pAssetData, pData);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::CreateExternalSceneGraph(
    egf::Entity* pEntity,
    efd::vector<NiObjectPtr>& objects,
    const efd::Bool callUpdate,
    const efd::Bool delayInWorld)
{
    EE_ASSERT(pEntity);

    EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
        ("%s: entity ID: %s", __FUNCTION__, pEntity->GetEntityID().ToString().c_str()));

    if (objects.empty())
        return false;

    // Create EntityData to track this entity.
    SceneGraphData* pData = CreateEntityData(pEntity);

    if (!pData)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: entity added twice! ID: %s",
            __FUNCTION__,
            pEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    pData->SetIsRenderable(
        pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Renderable));
    pData->SetIsAssetExternal(true);
    pData->SetIsDynamic(callUpdate);
    pData->SetIsAssetShared(false);
    pData->SetIsInWorld(!delayInWorld);

    // No asset data on this object.
    pData->m_spAssetData = 0;

    // Copy the objects directly
    efd::UInt32 objectCount = (efd::UInt32)objects.size();
    for (efd::UInt32 ui = 0; ui < objectCount; ++ui)
        pData->m_objects.push_back(objects[ui]);

    // Do the initial update, send messages, add to render service, etc.
    ProcessFreshSceneGraph(pData);

    return true;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::ReplaceAsset(
    SceneGraphData* pSceneGraphData,
    AssetDataPtr& spNewAssetData,
    const utf8string& newSceneGraphName)
{
    EE_ASSERT(!newSceneGraphName.empty());

    if (pSceneGraphData->m_spAssetData)
    {
        RemoveEntityFromAssetData(pSceneGraphData);
    }

    if (!spNewAssetData)
    {
        // This will launch an asset load request for the asset
        spNewAssetData = CreateAssetData(newSceneGraphName);
    }

    // This will add the entity to the asset data. If the asset is ready, it will set up
    // this entities objects, otherwise we just record that the entity needs the asset.
    // This function may generate messages when the asset is ready.
    AddEntityToAssetData(spNewAssetData, pSceneGraphData);
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::RecreateSceneGraph(
    egf::Entity* pEntity,
    const efd::AssetID& sceneGraphAssetID)
{
    EE_ASSERT(pEntity);

    SceneGraphData* pEntityData = GetEntityData(pEntity);
    if (!pEntityData)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("%s: Request to reload asset for an entity we do not manage. Entity %s",
             __FUNCTION__, pEntity->GetModel()->GetName().c_str()));
        return false;
    }

    if (!sceneGraphAssetID.IsValidURN())
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("%s: AssetID has an invalid URN, unable to look up "
             "geometry for entity %s so removing it",
             __FUNCTION__, pEntity->GetModel()->GetName().c_str()));

        // Recreating an invalid urn always removes the scene graph
        RemoveEntityFromAssetData(pEntityData);

        // This is a good result if the urn was empty
        return sceneGraphAssetID.GetURN().empty();
    }

    EE_ASSERT(sceneGraphAssetID.GetURN().find("urn:") != utf8string::npos);

    AssetDataPtr spNewAssetData = FindAssetData(sceneGraphAssetID.GetURN());

    ReplaceAsset(pEntityData, spNewAssetData, sceneGraphAssetID.GetURN());

    return spNewAssetData->m_state == AssetData::ADS_Ready;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::AddEntityToAssetData(AssetData* pAssetData, SceneGraphData* pEntityData)
{
    EE_ASSERT(pAssetData->m_entities.find(pEntityData) == pAssetData->m_entities.end());
    EE_ASSERT(pEntityData->m_spAssetData == 0);

    pAssetData->m_entities.push_back(pEntityData);
    pEntityData->m_spAssetData = pAssetData;

    if (pAssetData->m_state == AssetData::ADS_Ready)
    {
        SetSceneGraph(pAssetData, pEntityData);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveEntityFromAssetData(SceneGraphData* pEntityData)
{
    AssetData* pAssetData = pEntityData->m_spAssetData;
    if (!pAssetData)
        return;

    UnsetSceneGraph(pEntityData);

    efd::vector<SceneGraphData*>::iterator iter = pAssetData->m_entities.find(pEntityData);
    pAssetData->m_entities.erase(iter);

    pAssetData->RemoveEntityFromPreload(pEntityData->m_pEntity);

    RemoveUnusedAssetData(pAssetData);

    pEntityData->m_spAssetData = 0;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::AddHandleToAssetData(AssetData* pAssetData, SceneGraphData* pHandleData)
{
    EE_ASSERT(pAssetData->m_handles.find(pHandleData) == pAssetData->m_handles.end());
    EE_ASSERT(pHandleData->m_spAssetData == 0);

    pAssetData->m_handles.push_back(pHandleData);
    pHandleData->m_spAssetData = pAssetData;

    if (pAssetData->m_state == AssetData::ADS_Ready)
    {
        SetSceneGraph(pAssetData, pHandleData);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveHandleFromAssetData(SceneGraphData* pHandleData)
{
    AssetData* pAssetData = pHandleData->m_spAssetData;
    if (!pAssetData)
        return;

    if (pAssetData->m_state == AssetData::ADS_Ready)
    {
        UnsetSceneGraph(pHandleData);
    }

    efd::vector<SceneGraphData*>::iterator iter = pAssetData->m_handles.find(pHandleData);
    pAssetData->m_handles.erase(iter);

    RemoveUnusedAssetData(pAssetData);

    pHandleData->m_spAssetData = 0;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::AddAttachmentToAssetData(
    AssetData* pAssetData,
    AttachedSceneGraphData* pAttachData)
{
    EE_ASSERT(pAssetData->m_attachments.find(pAttachData) == pAssetData->m_attachments.end());
    EE_ASSERT(!pAttachData->m_isAttached);
    EE_ASSERT(pAttachData->m_spAssetData == 0);

    pAssetData->m_attachments.push_back(pAttachData);
    pAttachData->m_spAssetData = pAssetData;

    if (pAssetData->m_state == AssetData::ADS_Ready)
    {
        SetAttachedSceneGraph(pAssetData, pAttachData);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveAttachmentFromAssetData(AttachedSceneGraphData* pAttachData)
{
    AssetData* pAssetData = pAttachData->m_spAssetData;
    if (!pAssetData)
        return;

    if (pAssetData->m_state == AssetData::ADS_Ready)
    {
        UnsetAttachedSceneGraph(pAttachData);
    }

    efd::vector<AttachedSceneGraphData*>::iterator iter =
        pAssetData->m_attachments.find(pAttachData);
    pAssetData->m_attachments.erase(iter);

    pAssetData->RemoveEntityFromPreload(pAttachData->m_pEntityData->m_pEntity);

    RemoveUnusedAssetData(pAssetData);

    pAttachData->m_spAssetData = 0;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::ReloadAsset(
    const efd::utf8string& physicalID,
    const efd::utf8string& nifPath)
{
    AssetData* pAssetData = FindAssetData(physicalID);
    if (!pAssetData)
        pAssetData = FindAssetData(nifPath);
    if (!pAssetData)
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kLVL1,
            ("%s: ReloadAsset requested for an asset that is not known about. "
            "The refresh will be ignored. Asset ID: %s",
            __FUNCTION__,
            physicalID.c_str()));
        return false;
    }

    pAssetData->m_spLoadData = EE_NEW SceneGraphCacheRequestData();
    pAssetData->m_spLoadData->m_action = SGCRA_AssetLoadRequest;
    pAssetData->m_spLoadData->m_pAssetData = pAssetData;
    pAssetData->m_spLoadData->m_responseCategory = kCAT_INVALID;

    EE_VERIFY(m_spSceneGraphCache->ReloadSceneGraph(
        pAssetData->m_cacheHandle,
        m_sceneGraphCacheCategory,
        pAssetData->m_spLoadData));

    pAssetData->m_state = AssetData::ADS_Reloading;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::CacheSceneGraphFileName(
    const efd::utf8string& fileName,
    SceneGraphCache::ISceneGraphCacheRequestData* pRequestData,
    const efd::Category responseCategory,
    vector<SceneGraphCache::SceneGraphCacheHandle>& handles)
{
    // Make a cache request even if we already have the asset so that we send messages.
    // All caching calls delay creating their asset data until the asset has been loaded.
    SceneGraphCacheRequestData* pCacheData = EE_NEW SceneGraphCacheRequestData;
    pCacheData->m_action = SGCRA_CacheSceneGraph;
    pCacheData->m_pRequestData = pRequestData;
    pCacheData->m_responseCategory = responseCategory;
    if (m_spSceneGraphCache->CacheSceneGraph(
        fileName,
        m_sceneGraphCacheCategory,
        pCacheData,
        handles))
    {
        EE_DELETE pCacheData;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphHandle SceneGraphService::CreateSceneGraphFileName(
    const efd::utf8string& fileName,
    efd::Bool isDynamic,
    efd::Bool isRendered,
    efd::Bool prependDataPath)
{
    // Create EntityData to track this entity.
    SceneGraphData* pData = CreateHandleData();
    EE_ASSERT(pData);

    pData->SetIsDynamic(isDynamic);
    pData->SetIsRenderable(isRendered);
    pData->SetIsInWorld(true);

    efd::utf8string pathToScene;
    if (prependDataPath)
    {
        pathToScene = m_strDataPath;
        pathToScene += efd::PathUtils::GetNativePathSeperator() + fileName;
    }
    else
    {
        pathToScene = fileName;
    }

    AssetData* pAssetData = FindAssetData(pathToScene);
    if (!pAssetData)
    {
        // This will launch an asset load request for the asset
        pAssetData = CreateAssetData(pathToScene);
    }

    if (!pAssetData)
        return false;

    // This will add the entity to the asset data. If the asset is ready, it will set up
    // this entities objects, otherwise we just record that the entity needs the asset.
    // This function may generate messages when the asset is ready.
    AddHandleToAssetData(pAssetData, pData);

    return pData->m_handle;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphHandle SceneGraphService::AddSceneGraph(
    const efd::vector<NiObjectPtr>& objects,
    const efd::Bool isDynamic,
    const efd::Bool isRendered)
{
    if (objects.empty())
        return kInvalidHandle;

    // Create EntityData to track this entity.
    SceneGraphData* pData = CreateHandleData();

    pData->SetIsAssetExternal(true);
    pData->SetIsDynamic(isDynamic);
    pData->SetIsRenderable(isRendered);
    pData->SetIsAssetShared(false);
    pData->SetIsInWorld(true);

    // No asset data on this object.
    pData->m_spAssetData = 0;

    // Copy the objects directly
    efd::UInt32 objectCount = (efd::UInt32)objects.size();
    for (efd::UInt32 ui = 0; ui < objectCount; ++ui)
        pData->m_objects.push_back(objects[ui]);

    // Do the initial update, send messages, add to render service, etc.
    ProcessFreshSceneGraph(pData);

    return pData->m_handle;
}

//------------------------------------------------------------------------------------------------
// Methods that manage SceneGraphData
//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphData* SceneGraphService::CreateEntityData(egf::Entity* pEntity)
{
    if (GetEntityData(pEntity))
        return NULL;

    // Create the data
    SceneGraphData* pNewEntityData = EE_NEW SceneGraphData();
    pNewEntityData->m_pEntity = pEntity;
    pNewEntityData->SetIsEntity(true);

    // Get some rarely-changed properties from the mesh model or other models.
    bool isShared = true;
    if (pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Mesh))
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsNifAssetShared, isShared);
    pNewEntityData->SetIsAssetShared(isShared);

    bool isStatic = false;
    if (pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Bakeable))
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsStatic, isStatic);
    pNewEntityData->SetIsDynamic(!isStatic);

    pNewEntityData->SetIsRenderable(
        pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Renderable));

    // Add to the map of known entities
    m_entityData[pEntity] = pNewEntityData;

    return pNewEntityData;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphData* SceneGraphService::CreateHandleData()
{
    // Create the data
    SceneGraphData* pNewHandleData = EE_NEW SceneGraphData();
    pNewHandleData->m_handle = AllocateHandle();;
    pNewHandleData->SetIsEntity(false);
    pNewHandleData->SetIsAssetShared(true);

    // Add to the map of known entities
    m_handleData[pNewHandleData->m_handle] = pNewHandleData;

    return pNewHandleData;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::SetSceneGraph(AssetData* pAssetData, SceneGraphData* pSceneData)
{
    if (pSceneData->IsAssetShared())
    {
        GetSharedObjects(pAssetData->m_cacheHandle, pSceneData->m_objects);
    }
    else
    {
        GetNonSharedObjects(pAssetData->m_cacheHandle, pSceneData->m_objects);
    }

    ProcessFreshSceneGraph(pSceneData);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::ProcessFreshSceneGraph(SceneGraphData* pSceneData)
{
    NiAVObject* pAVObject = pSceneData->GetSceneGraph();
    if (pAVObject)
    {
        // Re-attach any attachments and update volatile properties
        if (pSceneData->IsEntity() && !pSceneData->IsAssetExternal())
        {
            AttachAllSceneGraphs(pSceneData, pAVObject);
            UpdateVolatileProperties(pSceneData->m_pEntity, pAVObject, true);
        }

        pAVObject->UpdateProperties();
        pAVObject->UpdateEffects();
        EE_ASSERT(m_pUpdateProcess);
        m_pUpdateProcess->SetTime(0.0f);
        pAVObject->Update(*m_pUpdateProcess);
        NiMesh::CompleteSceneModifiers(pAVObject);

        if (pSceneData->IsDynamic() && pSceneData->IsInWorld())
        {
            AddToUpdate(pSceneData);
        }

        if (pSceneData->IsRenderable() && pSceneData->IsInWorld() && m_spRenderService)
        {
            if (pSceneData->IsEntity())
            {
                m_spRenderService->AddRenderedEntity(
                    pSceneData->m_pEntity,
                    pAVObject);
            }
            else
            {
                m_spRenderService->AddRenderedObject(
                    pSceneData->m_handle,
                    pAVObject);
            }
            pSceneData->SetIsRendered(true);
        }

        if (!pSceneData->IsWaitingOnAttachments())
        {
            if (pSceneData->IsEntity())
            {
                OnSceneGraphAdded(pSceneData->m_pEntity, pAVObject);
            }
            else
            {
                OnSceneGraphAdded(pSceneData->m_handle, pAVObject);
            }
            pSceneData->SetIsWaitingToSendEvent(false);
        }
        else
        {
            pSceneData->SetIsWaitingToSendEvent(true);
        }
    }
    else
    {
        // Remove from the update list if it's on it
        if (pSceneData->m_updateIndex != SceneGraphData::kInvalidUpdateIndex)
        {
            RemoveFromUpdate(pSceneData);
        }

        RemoveFromUpdateOnce(pSceneData);

        pSceneData->SetIsWaitingToSendEvent(false);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UnsetSceneGraph(SceneGraphData* pSceneData)
{
    NiAVObject* pAVObject = pSceneData->GetSceneGraph();
    if (pAVObject)
    {
        if (pSceneData->IsRendered() && m_spRenderService)
        {
            EE_ASSERT(pAVObject);
            if (pSceneData->IsEntity())
            {
                m_spRenderService->RemoveRenderedEntity(
                    pSceneData->m_pEntity,
                    pAVObject);
            }
            else
            {
                m_spRenderService->RemoveRenderedObject(
                    pSceneData->m_handle,
                    pAVObject);
            }
            pSceneData->SetIsRendered(false);
        }

        // Remove all Placeable Feedback
        pSceneData->m_placeableFeedback.clear();

        // Detach all attachments.
        DetachAllSceneGraphs(pSceneData);

        // Inform the rest of the system that we are removing the entities' scene graphs.
        // NOTE: Other services (AnimationService) depend on OnSceneGraphAdded always
        // being called after OnSceneGraphRemoved during a rapid iteration circumstance like
        // this. If this behavior is changed, please re-test those services.
        if (pSceneData->IsEntity())
        {
            OnSceneGraphRemoved(pSceneData->m_pEntity, pAVObject);
        }
        else
        {
            OnSceneGraphRemoved(pSceneData->m_handle, pAVObject);
        }

        // Remove from updated entity list, if present
        if (pSceneData->IsDynamic())
        {
            RemoveFromUpdate(pSceneData);
        }

        RemoveFromUpdateOnce(pSceneData);
    }

    // Clear the objects
    efd::UInt32 numEntityObjects = static_cast<efd::UInt32>(pSceneData->m_objects.size());
    for (efd::UInt32 ui = 0; ui < numEntityObjects; ++ui)
        pSceneData->m_objects[ui] = 0;
    pSceneData->m_objects.clear();
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveSceneGraph(egf::Entity* pEntity, efd::Bool forceRemoval)
{
    EE_ASSERT(pEntity);

    SceneGraphDataPtr spEntityData = GetEntityData(pEntity);
    if (!spEntityData)
        return;

    if (spEntityData->IsAssetExternal())
    {
        if (!forceRemoval)
            return;

        // RemoveEntityFromAssetData would normally call UnsetSceneGraph, but we have no
        // asset data so we won't do anything. Do it explicitly here.
        UnsetSceneGraph(spEntityData);
    }

    // Remove all attachment info. Do this before removing the scene graph itself so that we
    // can cleanly remove the attachments.
    RemoveAllAttachmentData(spEntityData);

    // Remove it from the AssetData. This will remove it from the render list, the update list,
    // and send the removed message.
    RemoveEntityFromAssetData(spEntityData);

    // Remove from the map of entities to data
    m_entityData.erase(pEntity);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveSceneGraph(const SceneGraphHandle handle)
{
    if (handle == kInvalidHandle)
        return;

    efd::map<SceneGraphHandle, SceneGraphDataPtr>::iterator iter = m_handleData.find(handle);

    if (iter == m_handleData.end())
        return;

    SceneGraphDataPtr spData = iter->second;

    if (spData->IsAssetExternal())
    {
        // RemoveEntityFromAssetData would normally call UnsetSceneGraph, but we have no
        // asset data so we won't do anything. Do it explicitly here.
        UnsetSceneGraph(spData);
    }

    // Remove it from the AssetData. This will remove it from the render list, the update list,
    // and send the removed message.
    RemoveEntityFromAssetData(spData);

    // Remove from the map of handles to data
    m_handleData.erase(handle);

    ReleaseHandle(handle);
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Attachment functions
//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::AttachSceneGraph(
    egf::Entity* pEntity,
    const efd::utf8string& slotName)
{
    // Get the property data
    AttachNifData attachData;
    EE_VERIFYEQUALS(
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        slotName, attachData), PropertyResult_OK);

    // Get the scene data for the entity we are attached to
    SceneGraphData* pEntityData = GetEntityData(pEntity);
    if (!pEntityData)
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR2,
            ("%s: Asked to attach to an entity we know nothing of. Entity: '%s'.",
            __FUNCTION__,
            pEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    // Check if we already have an asset for this attachment. If not, make it.
    AttachedSceneGraphDataPtr spData = 0;
    if (!pEntityData->m_attached.find(slotName, spData))
    {
        // Create attachment data, even if it will be empty
        spData = EE_NEW AttachedSceneGraphData();
        spData->m_pEntityData = pEntityData;
        spData->m_attachmentKey = slotName;
        spData->m_isAttached = false;
        spData->m_spAssetData = NULL;
        pEntityData->m_attached[slotName] = spData;
    }
    else
    {
        // Make sure anything currently attached is removed
        DetachSceneGraph(pEntity, slotName);
    }

    if (attachData.GetNifAsset().GetURN().empty() ||
        !attachData.GetNifAsset().IsValidURN())
    {
        // There is no asset, so we go no further. At this point there is attachment data for
        // the slot on the entity, but no scene graph is attached an no asset is associated
        // with the attachment.
        return true;
    }

    // Look for existing asset data. If we have succeeded in loading we'll find it.
    AssetData* pAssetData = FindAssetData(attachData.GetNifAsset().GetURN());
    if (!pAssetData)
    {
        pAssetData = CreateAssetData(attachData.GetNifAsset().GetURN());
    }

    // This will add the attachment to the asset data. If the asset is ready, it will set up
    // the attachment objects and go as far as it can in attaching the scene graph.
    AddAttachmentToAssetData(pAssetData, spData);

    return true;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveAllAttachmentData(SceneGraphData* pEntityData)
{
    // Iterate over all the possible attachments
    efd::map<efd::utf8string, AttachedSceneGraphDataPtr>::const_iterator iter =
        pEntityData->m_attached.begin();
    while (iter != pEntityData->m_attached.end())
    {
        RemoveAttachmentData(pEntityData, iter->first);

        iter = pEntityData->m_attached.begin();
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveAttachmentData(
    SceneGraphData* pEntityData,
    const efd::utf8string& slotName)
{
    AttachedSceneGraphDataPtr spAttachData = 0;
    if (!pEntityData->m_attached.find(slotName, spAttachData))
        return;

    RemoveAttachmentFromAssetData(spAttachData);

    pEntityData->m_attached[slotName] = 0;
    pEntityData->m_attached.erase(slotName);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::SetAttachedSceneGraph(
    AssetData* pAssetData,
    AttachedSceneGraphData* pAttachData)
{
    EE_ASSERT(pAssetData->m_attachments.find(pAttachData) != pAssetData->m_attachments.end());
    EE_ASSERT(!pAttachData->m_isAttached);
    EE_ASSERT(pAttachData->m_spAssetData == pAssetData);

    GetSharedObjects(pAssetData->m_cacheHandle, pAttachData->m_objects);

    Entity* pEntity = pAttachData->m_pEntityData->m_pEntity;
    NiAVObject* pEntitySceneGraph = GetSceneGraphFromEntity(pEntity);
    if (!pEntitySceneGraph)
    {
        return;
    }

    AttachNifData attachData;
    EE_VERIFYEQUALS(
        pEntity->GetPropertyValue(
        kPropertyID_StandardModelLibrary_AttachedObjects,
        pAttachData->m_attachmentKey, attachData), PropertyResult_OK);

    // Attach the new scene graph
    if (AttachSceneGraph(pEntitySceneGraph, pAttachData->GetSceneGraph(), attachData))
    {
        pAttachData->m_isAttached = true;

        OnAttachmentMade(pEntity, pAttachData->m_attachmentKey);

        if (!pAttachData->m_pEntityData->IsWaitingOnAttachments()
            && pAttachData->m_pEntityData->IsWaitingToSendEvent())
        {
            OnSceneGraphAdded(pEntity, pEntitySceneGraph);
            pAttachData->m_pEntityData->SetIsWaitingToSendEvent(false);
        }
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UnsetAttachedSceneGraph(AttachedSceneGraphData* pSceneData)
{
    NiAVObject* pEntitySceneGraph = pSceneData->m_pEntityData->GetSceneGraph();
    EE_ASSERT(pEntitySceneGraph);

    RemoveAttachmentPlaceableFeedback(pSceneData);
    DetachSceneGraph(pEntitySceneGraph, pSceneData->GetSceneGraph());
    pSceneData->m_isAttached = false;

    OnAttachmentBroken(pSceneData->m_pEntityData->m_pEntity, pSceneData->m_attachmentKey);

    efd::UInt32 numAttachedObjects = static_cast<efd::UInt32>(pSceneData->m_objects.size());
    for (efd::UInt32 ui = 0; ui < numAttachedObjects; ++ui)
        pSceneData->m_objects[ui] = 0;
    pSceneData->m_objects.clear();
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::AttachSceneGraph(
    NiAVObject* pEntitySceneGraph,
    NiAVObject* pAttachSceneGraph,
    const AttachNifData& attachData)
{
    const char* pAttachPoint = attachData.GetAttachPoint().c_str();
    NiAVObject* pTargetAVObject = NULL;

    if (pAttachPoint != NULL && NiStricmp(pAttachPoint, "") != 0)
        pTargetAVObject = pEntitySceneGraph->GetObjectByName(pAttachPoint);
    else
        pTargetAVObject = pEntitySceneGraph;

    NiNode* pTargetNode = NiDynamicCast(NiNode, pTargetAVObject);;
    if (!pTargetNode)
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR1,
            ("%s: Cannot find attachment point '%s', or node is not an NiNode.",
            __FUNCTION__,
            attachData.GetAttachPoint().c_str()));

        return false;
    }

    // We allow attachment of a NULL scene graph
    if (!pAttachSceneGraph)
        return true;

    // Do the actual attachment.
    pAttachSceneGraph->SetTranslate(
        attachData.GetTranslation().x,
        attachData.GetTranslation().y,
        attachData.GetTranslation().z);

    NiMatrix3 kXRot, kYRot, kZRot;
    kXRot.MakeXRotation(attachData.GetRotation().x * -EE_DEGREES_TO_RADIANS); // Roll  +x
    kYRot.MakeYRotation(attachData.GetRotation().y * -EE_DEGREES_TO_RADIANS); // Pitch +y
    kZRot.MakeZRotation(attachData.GetRotation().z * -EE_DEGREES_TO_RADIANS); // Yaw   +z
    pAttachSceneGraph->SetRotate(kXRot * kYRot * kZRot);

    pAttachSceneGraph->SetScale(attachData.GetScale());

    pTargetNode->AttachChild(pAttachSceneGraph);

    pEntitySceneGraph->UpdateProperties();
    pEntitySceneGraph->UpdateEffects();
    EE_ASSERT(m_pUpdateProcess);
    m_pUpdateProcess->SetTime((float)m_pServiceManager->GetTime(kCLASSID_GameTimeClock));
    pEntitySceneGraph->Update(*m_pUpdateProcess);
    NiMesh::CompleteSceneModifiers(pEntitySceneGraph);

    return true;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::AttachAllSceneGraphs(SceneGraphData* pEntityData, NiAVObject* pAVObject)
{
    // Iterate over all the attachments
    efd::map<utf8string, AttachedSceneGraphDataPtr>::iterator iter =
        pEntityData->m_attached.begin();
    while (iter != pEntityData->m_attached.end())
    {
        AttachNifData attachData;
        EE_VERIFYEQUALS(pEntityData->m_pEntity->GetPropertyValue(
            kPropertyID_StandardModelLibrary_AttachedObjects,
            iter->first, attachData),
            PropertyResult_OK);

        // Only attach if there is something to rattach.
        AttachedSceneGraphData* pAttachData = iter->second;
        if (!pAttachData->m_spAssetData ||
            pAttachData->m_spAssetData->m_state != AssetData::ADS_Ready)
        {
            ++iter;
            continue;
        }

        EE_ASSERT(!pAttachData->m_isAttached);

        NiAVObject* pAttachSceneGraph = pAttachData->GetSceneGraph();

        if (AttachSceneGraph(pAVObject, pAttachSceneGraph, attachData))
            pAttachData->m_isAttached = true;

        ++iter;
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::DetachAllSceneGraphs(SceneGraphData* pEntityData)
{
    // Iterate over all the possible attachments
    efd::map<efd::utf8string, AttachedSceneGraphDataPtr>::const_iterator iter =
        pEntityData->m_attached.begin();
    while (iter != pEntityData->m_attached.end())
    {
        if (iter->second->m_isAttached)
        {
            DetachSceneGraph(pEntityData->m_pEntity, iter->first);
        }
        ++iter;
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::DetachSceneGraph(
    egf::Entity* pEntity,
    const efd::utf8string& slotName)
{
    // Get the entity and attachment data.
    SceneGraphData* pEntityData = GetEntityData(pEntity);
    AttachedSceneGraphDataPtr spAttachmentData = 0;
    if (!pEntityData ||
        !pEntityData->m_attached.find(slotName, spAttachmentData))
    {
        // No entity data, or no attachment data for this slot. Return.
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR2,
            ("%s: Asked to detach from an entity we know nothing of or which has no "
            "attachment data. Entity: '%s' Slot '%s'.",
            __FUNCTION__,
            pEntity->GetEntityID().ToString().c_str(),
            slotName.c_str()));
        return false;
    }

    // Do nothing if no asset data. This means there was no valid asset named in the attachment.
    if (!spAttachmentData->m_spAssetData)
        return true;

    // If actually attached, detach.
    if (spAttachmentData->m_isAttached)
    {
        RemoveAttachmentPlaceableFeedback(spAttachmentData);

        NiAVObject* pEntitySceneGraph = GetSceneGraphFromEntity(pEntity);
        DetachSceneGraph(pEntitySceneGraph, spAttachmentData->GetSceneGraph());
        spAttachmentData->m_isAttached = false;
        OnAttachmentBroken(pEntity, slotName);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::DetachSceneGraph(
    NiAVObject* pEntitySceneGraph,
    NiAVObject* pAttachSceneGraph)
{
    if (!pAttachSceneGraph)
        return;

    NiNode* pTargetNode = pAttachSceneGraph->GetParent();
    if (!pTargetNode)
        return;

    pTargetNode->DetachChild(pAttachSceneGraph);

    pEntitySceneGraph->UpdateProperties();
    pEntitySceneGraph->UpdateEffects();

    EE_ASSERT(m_pUpdateProcess);
    m_pUpdateProcess->SetTime((float)m_pServiceManager->GetTime(kCLASSID_GameTimeClock));

    pEntitySceneGraph->Update(*m_pUpdateProcess);
    NiMesh::CompleteSceneModifiers(pEntitySceneGraph);
}

//------------------------------------------------------------------------------------------------
NiAVObject* SceneGraphService::GetAttachedSceneGraph(
    egf::Entity* pEntity,
    const efd::utf8string& slotName) const
{
    EE_ASSERT(pEntity);

    // Does this entity have an existing scene graph?
    SceneGraphData* pEntityData = GetEntityData(pEntity);
    if (!pEntityData || pEntityData->m_objects.size() <= 0)
        return NULL;

    // Is there attachment data for this slot?
    AttachedSceneGraphDataPtr spAttachmentData;
    if (!pEntity || !pEntityData->m_attached.find(slotName, spAttachmentData))
        return NULL;

    return spAttachmentData->GetSceneGraph();
}

//------------------------------------------------------------------------------------------------
// AssetData management methods
//------------------------------------------------------------------------------------------------
SceneGraphService::AssetData* SceneGraphService::FindAssetData(
    const efd::utf8string& identifier) const
{
    // Look for asset data by any name it might be found.

    AssetDataPtr spData = 0;
    if (m_assetsByPhysicalID.find(identifier, spData))
    {
        return spData;
    }

    if (m_pendingAssetLoads.find(identifier, spData))
    {
        return spData;
    }

    utf8string physicalID;
    if (m_requestIDToPhysicalID.find(identifier, physicalID))
    {
        if (m_assetsByPhysicalID.find(physicalID, spData))
            return spData;
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::AssetData* SceneGraphService::CreateAssetData(const utf8string& identifier)
{
    EE_ASSERT(!identifier.empty());
    EE_ASSERT(FindAssetData(identifier) == 0);

    AssetDataPtr spAssetData = EE_NEW AssetData;
    spAssetData->m_identifier = identifier;

    spAssetData->m_spLoadData = EE_NEW SceneGraphCacheRequestData;
    spAssetData->m_spLoadData->m_action = SGCRA_AssetLoadRequest;
    spAssetData->m_spLoadData->m_pAssetData = spAssetData;
    vector<SceneGraphCache::SceneGraphCacheHandle> handles;
    if (m_spSceneGraphCache->CacheSceneGraph(
        identifier,
        m_sceneGraphCacheCategory,
        spAssetData->m_spLoadData,
        handles))
    {
        if (handles.size() == 0)
        {
            EE_LOG(efd::kGamebryoGeneral0,
                efd::ILogger::kERR1,
                ("%s: Found no assets when looking for tag '%s', or load error. ",
                __FUNCTION__,
                identifier.c_str()));

            spAssetData = 0;

            return 0;
        }

        if (handles.size() > 1)
        {
            EE_LOG(efd::kGamebryoGeneral0,
                efd::ILogger::kERR1,
                ("%s: Found %d assets when looking for tag '%s'. "
                "Ignoring all but the first.",
                __FUNCTION__,
                handles.size(),
                identifier.c_str()));
        }

        spAssetData = SetAssetDataFromCache(spAssetData, handles[0], true);

        return spAssetData;
    }

    m_pendingAssetLoads[identifier] = spAssetData;

    return spAssetData;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::AssetData* SceneGraphService::CreateAssetDataWithHandle(
    SceneGraphCache::SceneGraphCacheHandle handle)
{
    EE_ASSERT(handle);
    const utf8string& physicalID = m_spSceneGraphCache->GetPhysicalID(handle);
    EE_ASSERT(FindAssetData(physicalID) == 0);

    AssetData* pAssetData = EE_NEW AssetData;
    pAssetData->m_isCached = true;

    pAssetData->m_physicalID = physicalID;
    pAssetData->m_cacheHandle = handle;
    pAssetData->m_state = AssetData::ADS_Ready;
    m_assetsByPhysicalID[physicalID] = pAssetData;

    return pAssetData;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::AssetData* SceneGraphService::SetAssetDataFromCache(
    AssetDataPtr spAssetData,
    SceneGraphCache::SceneGraphCacheHandle cacheHandle,
    bool usagePending)
{
    EE_ASSERT(cacheHandle);

    // Clear any load data on the asset data
    spAssetData->m_spLoadData = 0;

    // Get the physical ID that we are setting from.
    const utf8string& physicalID = m_spSceneGraphCache->GetPhysicalID(cacheHandle);

    if (spAssetData->m_state == AssetData::ADS_Reloading)
    {
        EE_ASSERT(spAssetData->m_physicalID == physicalID);
        EE_ASSERT(spAssetData->m_cacheHandle == cacheHandle);

        // Clear the scene graphs from all entities, attachments and handles
        efd::UInt32 numEntities = static_cast<efd::UInt32>(spAssetData->m_entities.size());
        for (efd::UInt32 ui = 0; ui < numEntities; ++ui)
        {
            SceneGraphData* pEntityData = spAssetData->m_entities[ui];
            EE_ASSERT(pEntityData);

            UnsetSceneGraph(pEntityData);
        }
        efd::UInt32 numHandles = static_cast<efd::UInt32>(spAssetData->m_handles.size());
        for (efd::UInt32 ui = 0; ui < numHandles; ++ui)
        {
            SceneGraphData* pHandleData = spAssetData->m_handles[ui];
            EE_ASSERT(pHandleData);

            UnsetSceneGraph(pHandleData);
        }
        efd::UInt32 numAttach = static_cast<efd::UInt32>(spAssetData->m_attachments.size());
        for (efd::UInt32 ui = 0; ui < numAttach; ++ui)
        {
            AttachedSceneGraphData* pAttachData = spAssetData->m_attachments[ui];
            EE_ASSERT(pAttachData);

            UnsetAttachedSceneGraph(pAttachData);
        }
    }
    else
    {
        EE_ASSERT(spAssetData->m_state == AssetData::ADS_Loading);

        // Add it to the set of known identifier->physical IDs and remove it from pending loads.
        m_pendingAssetLoads.erase(spAssetData->m_identifier);
        m_requestIDToPhysicalID[spAssetData->m_identifier] = physicalID;
        spAssetData->m_identifier.clear();

        // Now that we have a physical ID, we might discover we already have the NIF for
        // this request; we previously loaded it under a different request
        AssetData* pExistingData = FindAssetData(physicalID);
        if (pExistingData && pExistingData != spAssetData)
        {
            // We do indeed have an existing asset for this physical ID. Merge this one
            // with the existing one, which will do much of the same work as this function.
            MergeAssetData(pExistingData, spAssetData);
            return pExistingData;
        }

        // Set up data we now have from the cache handle.
        spAssetData->m_physicalID = physicalID;
        spAssetData->m_cacheHandle = cacheHandle;
        m_assetsByPhysicalID[physicalID] = spAssetData;
    }

    // We have new data and are ready for use.
    spAssetData->m_state = AssetData::ADS_Ready;

    // We are also assuming that at this point no entity, attachment or handle using this
    // asset has a scene graph in use.

    // Set the scene graphs on all entities, attachments and handles
    efd::UInt32 numEntities = static_cast<efd::UInt32>(spAssetData->m_entities.size());
    for (efd::UInt32 ui = 0; ui < numEntities; ++ui)
    {
        SceneGraphData* pEntityData = spAssetData->m_entities[ui];
        EE_ASSERT(pEntityData);

        SetSceneGraph(spAssetData, pEntityData);
    }
    efd::UInt32 numHandles = static_cast<efd::UInt32>(spAssetData->m_handles.size());
    for (efd::UInt32 ui = 0; ui < numHandles; ++ui)
    {
        SceneGraphData* pHandleData = spAssetData->m_handles[ui];
        EE_ASSERT(pHandleData);

        SetSceneGraph(spAssetData, pHandleData);
    }
    efd::UInt32 numAttach = static_cast<efd::UInt32>(spAssetData->m_attachments.size());
    for (efd::UInt32 ui = 0; ui < numAttach; ++ui)
    {
        AttachedSceneGraphData* pAttachData = spAssetData->m_attachments[ui];
        EE_ASSERT(pAttachData);

        SetAttachedSceneGraph(spAssetData, pAttachData);
    }

    // Mark all the preloads for this asset as completed.
    MarkPreloadsAsFound(spAssetData);

    // In theory this asset data is used, but remove it if it isn't and it isn't expected to be.
    if (!usagePending)
        RemoveUnusedAssetData(spAssetData);

    return spAssetData;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::MergeAssetData(AssetData *pExistingData, AssetData *pRedundantData)
{
    // Go through all the objects on the redundant data and shift them to the new data.
    while (pRedundantData->m_entities.size() > 0)
    {
        SceneGraphData* pEntityData = pRedundantData->m_entities.back();
        EE_ASSERT(pEntityData);

        EE_ASSERT(pEntityData->IsWaitingOnAsset());

        pEntityData->m_spAssetData = pExistingData;
        pExistingData->m_entities.push_back(pEntityData);
        pRedundantData->m_entities.pop_back();

        if (pExistingData->m_state == AssetData::ADS_Ready)
        {
            SetSceneGraph(pExistingData, pEntityData);
        }
    }

    while (pRedundantData->m_handles.size() > 0)
    {
        SceneGraphData* pHandleData = pRedundantData->m_handles.back();
        EE_ASSERT(pHandleData);

        EE_ASSERT(pHandleData->IsWaitingOnAsset());

        pHandleData->m_spAssetData = pExistingData;
        pExistingData->m_handles.push_back(pHandleData);
        pRedundantData->m_handles.pop_back();

        if (pExistingData->m_state == AssetData::ADS_Ready)
        {
            SetSceneGraph(pExistingData, pHandleData);
        }
    }

    while (pRedundantData->m_attachments.size() > 0)
    {
        AttachedSceneGraphData* pAttachData = pRedundantData->m_attachments.back();
        EE_ASSERT(pAttachData);

        EE_ASSERT(!pAttachData->m_isAttached);

        pAttachData->m_spAssetData = pExistingData;
        pExistingData->m_attachments.push_back(pAttachData);
        pRedundantData->m_attachments.pop_back();

        if (pExistingData->m_state == AssetData::ADS_Ready)
        {
            SetAttachedSceneGraph(pExistingData, pAttachData);
        }
    }

    // Transfer any preload info. If the existing data has been loaded, then the preloads are
    // all found. If not, we move them to the list for the existing data.
    if (pExistingData->m_state == AssetData::ADS_Ready)
    {
        MarkPreloadsAsFound(pRedundantData);
    }
    else
    {
        while (pRedundantData->m_preloads.size() > 0)
        {
            pExistingData->m_preloads.push_back(pRedundantData->m_preloads.back());
            pRedundantData->m_preloads.pop_back();
        }
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveUnusedAssetData(AssetDataPtr spAssetData)
{
    if (!spAssetData->InUse())
    {
        if (spAssetData->m_cacheHandle && spAssetData->m_state == AssetData::ADS_Ready)
        {
            m_spSceneGraphCache->UnCacheSceneGraph(spAssetData->m_cacheHandle);
            m_assetsByPhysicalID.erase(spAssetData->m_physicalID);

            // Mark as loading in case smart pointers cause this data to hang around
            spAssetData->m_state = AssetData::ADS_Unloaded;
        }
        // else we should be still loading or reloading and we will remove once loaded
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::MarkPreloadsAsFound(AssetData* pAssetData)
{
    while (pAssetData->m_preloads.size() > 0)
    {
        EntityPreloadDataPtr spPreloadData = pAssetData->m_preloads.back();

        ++(spPreloadData->m_assetsFound);

        if (spPreloadData->m_assetsNeeded == spPreloadData->m_assetsFound &&
            spPreloadData->m_responseCategory.IsValid())
        {
            EntityPreloadResponse* pPreloadResponse = EE_NEW EntityPreloadResponse;
            pPreloadResponse->m_pEntity = spPreloadData->m_pEntityData->m_pEntity;
            m_pMessageService->SendLocal(pPreloadResponse, spPreloadData->m_responseCategory);
        }

        pAssetData->m_preloads.pop_back();
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::OnAssetLoadFailure(AssetDataPtr spAssetData)
{
    EE_ASSERT(spAssetData);

    // Mark all preloads as done, because we will never get their asset
    MarkPreloadsAsFound(spAssetData);

    // Uncache it now so that it is removed when the last dependent is removed.
    spAssetData->m_isCached = false;

    // Clear the scene graph data from all entities, attachments and handles
    while (spAssetData->m_entities.size() > 0)
    {
        RemoveEntityFromAssetData(spAssetData->m_entities.back());
    }
    while (spAssetData->m_handles.size() > 0)
    {
        RemoveHandleFromAssetData(spAssetData->m_handles.back());
    }
    while (spAssetData->m_attachments.size() > 0)
    {
        RemoveAttachmentFromAssetData(spAssetData->m_attachments.back());
    }

    EE_ASSERT(!spAssetData->InUse());
    EE_ASSERT(!FindAssetData(spAssetData->m_physicalID));
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::GetSharedObjects(
    SceneGraphCache::SceneGraphCacheHandle handle,
    efd::vector<NiObjectPtr>& objects)
{
    NiCloningProcess cloningProcess;
    cloningProcess.m_eCopyType = NiObjectNET::COPY_EXACT;
    cloningProcess.m_eAffectedNodeRelationBehavior = NiCloningProcess::CLONE_RELATION_CLONEDONLY;
    efd::UInt32 objectCount = m_spSceneGraphCache->GetObjectCount(handle);
    for (efd::UInt32 ui = 0; ui < objectCount; ++ui)
    {
        NiObject* pClone =
            m_spSceneGraphCache->GetObjectAt(handle, ui)->Clone(cloningProcess);
        EE_ASSERT(pClone);
        objects.push_back(pClone);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::GetNonSharedObjects(
    SceneGraphCache::SceneGraphCacheHandle handle,
    efd::vector<NiObjectPtr>& objects)
{
    // Deep copy the data for the entity
    efd::UInt32 size = m_spSceneGraphCache->GetObjectCount(handle);
    NiStream stream;
    stream.SetTexturePalette(m_spSceneGraphCache->GetTexturePalette());
    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        stream.InsertObject(m_spSceneGraphCache->GetObjectAt(handle, ui));
    }

    /* stream object to memory block */
    char* pBuffer = 0;
    int iBufferSize = 0;
    EE_VERIFY(stream.Save(pBuffer, iBufferSize));

    /* stream memory block back to object */
    stream.Load(pBuffer, iBufferSize);

    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        objects.push_back(stream.GetObjectAt(ui));
    }

    EE_FREE(pBuffer);
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Message sending functions
//------------------------------------------------------------------------------------------------
void SceneGraphService::OnSceneGraphsUpdated()
{
    SceneGraphsUpdatedMessagePtr spMessage = EE_NEW SceneGraphsUpdatedMessage();
    m_pMessageService->SendImmediate(spMessage);
}

//------------------------------------------------------------------------------------------------

// DT32964
// The following methods use SendImmediate to send messages. However, some of these methods can
// be invoked in the context of another message handler. This ordering violates best practices
// and should be investigated for refactoring as it can cause problems with the order that
// handlers are invoked in other services.
void SceneGraphService::OnSceneGraphAdded(egf::Entity* pEntity, NiAVObject* pSceneGraph)
{
    EE_UNUSED_ARG(pSceneGraph);

    SceneGraphAddedMessagePtr spMessage = EE_NEW SceneGraphAddedMessage(pEntity);
    m_pMessageService->SendImmediate(spMessage);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::OnSceneGraphRemoved(egf::Entity* pEntity, NiAVObject* pSceneGraph)
{
    EE_UNUSED_ARG(pSceneGraph);

    SceneGraphRemovedMessagePtr spMessage = EE_NEW SceneGraphRemovedMessage(pEntity);
    m_pMessageService->SendImmediate(spMessage);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::OnSceneGraphAdded(SceneGraphHandle handle, NiAVObject* pSceneGraph)
{
    EE_UNUSED_ARG(pSceneGraph);

    SceneGraphAddedMessagePtr spMessage = EE_NEW SceneGraphAddedMessage(handle);
    m_pMessageService->SendImmediate(spMessage);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::OnSceneGraphRemoved(SceneGraphHandle handle, NiAVObject* pSceneGraph)
{
    EE_UNUSED_ARG(pSceneGraph);

    SceneGraphRemovedMessagePtr spMessage = EE_NEW SceneGraphRemovedMessage(handle);
    m_pMessageService->SendImmediate(spMessage);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::OnAttachmentMade(egf::Entity* pEntity, const utf8string& slotName)
{
    AttachmentMadeMessagePtr spMessage = EE_NEW AttachmentMadeMessage(pEntity, slotName);
    m_pMessageService->SendImmediate(spMessage);
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::OnAttachmentBroken(egf::Entity* pEntity, const utf8string& slotName)
{
    AttachmentBrokenMessagePtr spMessage = EE_NEW AttachmentBrokenMessage(pEntity, slotName);
    m_pMessageService->SendImmediate(spMessage);
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphHandle SceneGraphService::AllocateHandle()
{
    if (m_unusedHandles.size() > 0)
    {
        SceneGraphHandle result = m_unusedHandles.back();
        m_unusedHandles.pop_back();
        return result;
    }

    return m_nextUnusedHandle++;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::ReleaseHandle(const SceneGraphHandle handle)
{
    m_unusedHandles.push_back(handle);
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::Cache(
    const efd::AssetID& assetID,
    SceneGraphCache::ISceneGraphCacheRequestData* pRequestData,
    const efd::Category responseCategory,
    efd::vector<SceneGraphCache::SceneGraphCacheHandle>& handles)
{
    EE_ASSERT(assetID.IsValidURN());

    // See if we already know of this ID
    utf8string physicalID;
    AssetDataPtr spAssetData = FindAssetData(assetID.GetURN());
    if (spAssetData && spAssetData->m_state == AssetData::ADS_Ready)
    {
        handles.push_back(spAssetData->m_cacheHandle);
        CacheHandles(handles);
        return true;
    }

    AssetID filtered = assetID;
    if (assetID.GetURN().find(":gamebryo-scenegraph") == efd::utf8string::npos)
    {
        // Append scene graph tag.  If this is not here, then asset locator overrides may cause
        // some assets to not be returned, i.e. if "reaper.kfm" and "reaper.nif" both exist
        // and the scene graph tag is not there, only one will be returned by the AssetLocate.
        utf8string urn = filtered.GetURN();
        urn += ":gamebryo-scenegraph";
        filtered.SetURN(urn);
    }

    // Send the request on to the cache. Note we may have already requested the asset for
    // another purpose, but requesting an as-yet-unready asset again ensures that messages
    // are correctly sent.
    SceneGraphCacheRequestData* pCacheData = EE_NEW SceneGraphCacheRequestData;
    pCacheData->m_action = SGCRA_CacheSceneGraph;
    pCacheData->m_pRequestData = pRequestData;
    pCacheData->m_responseCategory = responseCategory;
    if (m_spSceneGraphCache->CacheSceneGraph(
        filtered,
        m_sceneGraphCacheCategory,
        pCacheData,
        handles))
    {
        EE_DELETE pCacheData;
        CacheHandles(handles);

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::CacheHandles(
    efd::vector<SceneGraphCache::SceneGraphCacheHandle>& handles)
{
    AssetDataPtr spAssetData = 0;

    for (UInt32 ui = 0; ui < handles.size(); ++ui)
    {
        spAssetData = FindAssetData(m_spSceneGraphCache->GetPhysicalID(handles[ui]));
        if (!spAssetData)
        {
            spAssetData = CreateAssetDataWithHandle(handles[ui]);
        }

        spAssetData->m_isCached = true;
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::UnCacheHandles(vector<SceneGraphCache::SceneGraphCacheHandle>& handles)
{
    for (UInt32 ui = 0; ui < handles.size(); ++ui)
    {
        AssetData* pAssetData = FindAssetData(m_spSceneGraphCache->GetPhysicalID(handles[ui]));
        if (pAssetData)
        {
            pAssetData->m_isCached = false;
            RemoveUnusedAssetData(pAssetData);
        }
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphService::RemoveAllFromCache()
{
    efd::map<efd::utf8string, AssetDataPtr>::iterator iter = m_assetsByPhysicalID.begin();
    while (iter != m_assetsByPhysicalID.end())
    {
        efd::map<efd::utf8string, AssetDataPtr>::iterator thisElem = iter++;

        AssetDataPtr spAssetData = thisElem->second;
        spAssetData->m_isCached = false;
        RemoveUnusedAssetData(spAssetData);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::SubscribeToMessages()
{
    m_pMessageService->Subscribe(this, kCAT_LocalMessage);

    EntityManager* pEntityManager = m_pServiceManager->GetSystemServiceAs<egf::EntityManager>();
    EE_ASSERT(pEntityManager);
    if (!pEntityManager)
        return false;
    m_assetPreloadRequestCategory = pEntityManager->GetEntityPreloadCategory();
    m_pMessageService->Subscribe(this, m_assetPreloadRequestCategory);

    m_assetPreloadResponseCategory = m_pMessageService->GetGloballyUniqueCategory();

    m_sceneGraphCacheCategory = m_pMessageService->GetGloballyUniqueCategory();
    m_pMessageService->Subscribe(this, m_sceneGraphCacheCategory);

    AssetFactoryManager* pAFM = m_pServiceManager->GetSystemServiceAs<efd::AssetFactoryManager>();
    AssetLocatorService* pALS = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    m_spSceneGraphCache->Init(m_pMessageService, pALS, pAFM);

    m_assetRefreshCategory = m_spSceneGraphCache->GetReloadCategory();
    m_pMessageService->Subscribe(this, m_assetRefreshCategory);

    return true;
}

//------------------------------------------------------------------------------------------------
class SetMaterialNeedsUpdateFunctor : public ecr::SceneGraphService::EntitySceneGraphFunctor
{
public:
    virtual ~SetMaterialNeedsUpdateFunctor() { }

    efd::Bool operator()(const egf::Entity*, const efd::vector<NiObjectPtr>& objects)
    {
        efd::vector<NiObjectPtr>::const_iterator objIter;
        for (objIter = objects.begin(); objIter != objects.end(); objIter++)
        {
            NiAVObjectPtr spScene = NiDynamicCast(NiAVObject, *objIter);
            if (!spScene)
                continue;

            NiRenderObject::RecursiveSetMaterialNeedsUpdate(spScene, true);
        }

        return false;
    }
};

//------------------------------------------------------------------------------------------------
void SceneGraphService::ForceMaterialUpdate()
{
    // Iterate through all the registered scene graphs calling RecursiveSetMaterialNeedsUpdate
    // on each object.
    SetMaterialNeedsUpdateFunctor kFunctor;
    ForEachEntitySceneGraph(kFunctor);
}

//------------------------------------------------------------------------------------------------
SceneGraphService::AssetData::AssetData()
    : m_state(ADS_Loading)
    , m_cacheHandle(0)
    , m_spLoadData(0)
    , m_isCached(false)
{
}

//------------------------------------------------------------------------------------------------
SceneGraphService::AssetData::~AssetData()
{
    m_cacheHandle = 0; // Clear the smart pointer
    m_spLoadData = 0;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::AssetData::InUse() const
{
    return !m_entities.empty() || !m_attachments.empty() || !m_handles.empty() || m_isCached;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::AssetData::RemoveEntityFromPreload(Entity* pEntity)
{
    efd::vector<EntityPreloadDataPtr>::iterator iter = m_preloads.begin();
    while (iter != m_preloads.end())
    {
        if ((*iter)->m_pEntityData->m_pEntity == pEntity)
        {
            (*iter)->m_assetsFound++;
            m_preloads.erase(iter);
            return true;
        }

        ++iter;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::AttachedSceneGraphData::AttachedSceneGraphData() :
    m_pEntityData(NULL),
    m_spAssetData(0),
    m_isAttached(false)
{
}

//------------------------------------------------------------------------------------------------
SceneGraphService::AttachedSceneGraphData::~AttachedSceneGraphData()
{
    m_objects.clear();
    m_spAssetData = 0;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphData::SceneGraphData() :
    m_spAssetData(0),
    m_updateIndex(kInvalidUpdateIndex),
    m_pEntity(NULL),
    m_flags(0)
{
}

//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphData::~SceneGraphData()
{
    efd::UInt32 count = m_objects.size();
    for (efd::UInt32 ui = 0; ui < count; ++ui)
        m_objects[ui] = 0;
    m_objects.clear();

    efd::map<utf8string, AttachedSceneGraphDataPtr>::iterator iter = m_attached.begin();
    while (iter != m_attached.end())
    {
        iter->second = 0;
        ++iter;
    }
    m_attached.clear();

    m_placeableFeedback.clear();

    m_spAssetData = 0;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::SceneGraphData::IsWaitingOnAsset() const
{
    return !m_spAssetData || m_spAssetData->m_state != AssetData::ADS_Ready;
}

//------------------------------------------------------------------------------------------------
efd::Bool SceneGraphService::SceneGraphData::IsWaitingOnAttachments() const
{
    if (!IsEntity())
        return false;

    efd::map<efd::utf8string, AttachedSceneGraphDataPtr>::const_iterator iter = m_attached.begin();
    while (iter != m_attached.end())
    {
        if (iter->second->m_spAssetData && !iter->second->m_isAttached)
            return true;
        ++iter;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphCacheRequestData::SceneGraphCacheRequestData()
    : m_action(SGCRA_Invalid)
    , m_pAssetData(0)
    , m_pRequestData(0)
    , m_responseCategory(kCAT_INVALID)
{
}

//------------------------------------------------------------------------------------------------
SceneGraphService::SceneGraphCacheRequestData::~SceneGraphCacheRequestData()
{
}

//------------------------------------------------------------------------------------------------
