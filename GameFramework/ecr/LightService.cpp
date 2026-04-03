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

#include "LightService.h"
#include "RenderService.h"
#include "SceneGraphService.h"

#include <NiLightManager.h>
#include <NiSpotLight.h>
#include <NiDirectionalLight.h>
#include <NiAmbientLight.h>

#include <efd/ILogger.h>
#include <efd/ecrLogIDs.h>
#include <efd/MessageService.h>
#include <efd/PathUtils.h>
#include <egf/EntityManager.h>
#include <egf/egfClassIDs.h>
#include <efd/ServiceManager.h>
#include <egf/StandardModelLibraryPropertyIDs.h>
#include <egf/StandardModelLibraryFlatModelIDs.h>
#include <NiLightManager.h>
#include <NiPSSMShadowClickGenerator.h>
#include <NiMeshCullingProcess.h>

using namespace efd;
using namespace egf;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(LightService);

EE_HANDLER_WRAP(LightService, HandleEntityAddedMessage, EntityChangeMessage,
                kMSGID_OwnedEntityEnterWorld);

EE_HANDLER_WRAP(LightService, HandleEntityUpdatedMessage, EntityChangeMessage,
                kMSGID_OwnedEntityUpdated);

EE_HANDLER_WRAP(LightService, HandleEntityRemovedMessage, EntityChangeMessage,
                kMSGID_OwnedEntityExitWorld);

EE_HANDLER(LightService, HandleSceneGraphsUpdatedMessage, SceneGraphsUpdatedMessage);

EE_HANDLER(LightService, HandleSceneGraphAddedMessage, SceneGraphAddedMessage);

EE_HANDLER(LightService, HandleSceneGraphRemovedMessage, SceneGraphRemovedMessage);


//------------------------------------------------------------------------------------------------
LightService::LightService(bool enableAutomaticRelighting) :
    m_uiMaxAmbientLightsPerObject(EE_UINT8_MAX),
    m_uiMaxDirectionalLightsPerObject(EE_UINT8_MAX),
    m_uiMaxPointLightsPerObject(EE_UINT8_MAX),
    m_uiMaxSpotLightsPerObject(EE_UINT8_MAX),
    m_RecieverListDirty (true),
    m_enableAutomaticRelighting(enableAutomaticRelighting),
    m_bUsePrecomputedLighting(false)
{
    // If this default priority is changed, also update the service quick reference documentation.
    m_defaultPriority = 1480;
}

//------------------------------------------------------------------------------------------------
LightService::~LightService()
{
    // This method intentionally left blank (all shutdown occurs in OnShutdown).
}

//------------------------------------------------------------------------------------------------
const char* LightService::GetDisplayName() const
{
    return "LightService";
}

//------------------------------------------------------------------------------------------------
SyncResult LightService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<egf::EntityManager>();
    pDependencyRegistrar->AddDependency<SceneGraphService>();

    m_pMessageService = m_pServiceManager->GetSystemServiceAs<efd::MessageService>();
    EE_ASSERT(m_pMessageService);

    m_spRenderService = m_pServiceManager->GetSystemServiceAs<RenderService>();
    // the render service is optional
    if (m_spRenderService)
        m_spRenderService->AddDelegate(this);

    NiLightManager::Initialize();

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult LightService::OnInit()
{
    // Grab pointers to other important services.
    m_pEntityManager = m_pServiceManager->GetSystemServiceAs<egf::EntityManager>();
    EE_ASSERT(m_pEntityManager);

    m_spSceneGraphService = m_pServiceManager->GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(m_spSceneGraphService);

    // Register with the render service.
    if (m_spRenderService)
    {
        if (!m_spRenderService->GetRenderer())
            return AsyncResult_Pending;

        NiPSSMConfiguration kConfig;

        kConfig.SetNumSlices(4);
        kConfig.SetCustomSceneCameraFarPlaneEnabled(false);
        kConfig.SetBorderTestingEnabled(true);
        kConfig.SetSliceTransitionEnabled(false);
        kConfig.SetSliceTransitionSize(300.0f);
        kConfig.SetSliceTransitionNoiseScale(0.0015f);
        kConfig.SetSubTexelOffsetEnabled(true);
        kConfig.SetSceneDependentFrustumsEnabled(false);

        NiPSSMShadowClickGenerator* pkClickGenerator =
            NiNew NiPSSMShadowClickGenerator(kConfig);

        NiShadowManager::RegisterShadowClickGenerator(pkClickGenerator);

        // Initialize PSSM shadow click generator and activate it.
        NiShadowManager::SetActiveShadowClickGenerator("NiPSSMShadowClickGenerator");
    }

    SubscribeToMessages();

    return AsyncResult_Complete;
}
//------------------------------------------------------------------------------------------------
class UpdateAffectsFunctor :
    public SceneGraphService::EntitySceneGraphFunctor,
    public SceneGraphService::HandleSceneGraphFunctor
{
public:
    UpdateAffectsFunctor()
    {
    }

    ~UpdateAffectsFunctor()
    {
    }

    efd::Bool operator()(const egf::EntityID entityID, const efd::vector<NiObjectPtr>& objects)
    {
        EE_UNUSED_ARG(entityID);
        EE_ASSERT(objects.size() > 0);
        NiNode* pNode = NiDynamicCast(NiNode, objects[0]);

        if (pNode != NULL)
            pNode->UpdateEffects();

        return false;
    }

    efd::Bool operator()(
        const SceneGraphService::SceneGraphHandle handle,
        const efd::vector<NiObjectPtr>& objects)
    {
        EE_UNUSED_ARG(handle);
        EE_ASSERT(objects.size() > 0);
        NiNode* pNode = NiDynamicCast(NiNode, objects[0]);

        if (pNode != NULL)
            pNode->UpdateEffects();

        return false;
    }
};

//------------------------------------------------------------------------------------------------
void LightService::RelightScene(bool onlyChangedObjects)
{
    bool sortAxisChanged = DetermineSortAxis();
    SetAffectedNodeLists(sortAxisChanged || !onlyChangedObjects);
}

//------------------------------------------------------------------------------------------------
AsyncResult LightService::OnTick()
{
    // If nothing was added or changed, return.
    if (m_changedLightList.empty() && m_changedNodeList.empty())
        return AsyncResult_Pending;

    if (m_enableAutomaticRelighting)
    {
        RelightScene(true);
    }

    m_changedLightList.clear();
    m_changedNodeList.clear();

    // Return pending to indicate the service should continue to be ticked.
    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
AsyncResult LightService::OnShutdown()
{
    EE_ASSERT(m_pServiceManager);

    if (m_pMessageService != NULL)
    {
        // UnSubscribe to all messages sent on kCAT_LocalMessage.
        m_pMessageService->Unsubscribe(this, kCAT_LocalMessage);
    }

    efd::map<egf::EntityID, LightData*>::iterator itor;

    for (itor = m_entityLightMap.begin(); itor != m_entityLightMap.end(); itor++)
        itor->second->m_spLight->DetachAllAffectedNodes();

    m_entityLightMap.clear();
    m_entityReceiverMap.clear();

    // Empty change lists.
    m_changedNodeList.clear();
    m_changedLightList.clear();

    // Release Light & Receiver Lists.
    efd::vector<LightData*>::iterator itCurrLight;

    for (itCurrLight = m_lightList.begin(); itCurrLight != m_lightList.end(); itCurrLight++)
        EE_DELETE(*itCurrLight);

    m_lightList.clear();

    efd::vector<ReceiverData*>::iterator itCurrReceiver;
    for (itCurrReceiver = m_receiverList.begin(); itCurrReceiver != m_receiverList.end();
        itCurrReceiver++)
    {
        EE_DELETE(*itCurrReceiver);
    }
    m_receiverList.clear();

    // Unregister from the render service and release service pointers.
    if (m_spRenderService)
        m_spRenderService->RemoveDelegate(this);

    m_spSceneGraphService = NULL;
    m_spRenderService = NULL;
    m_pEntityManager = NULL;

    NiLightManager::Shutdown();

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void LightService::SubscribeToMessages()
{
    // Register for local entity creation so we are notified when non-reflected entities are
    // created (e.g., entities that are basically client only).
    m_pMessageService->Subscribe(this, kCAT_LocalMessage);
}

//------------------------------------------------------------------------------------------------
class AddAffectedObjectFunctor :
    public SceneGraphService::EntitySceneGraphFunctor,
    public SceneGraphService::HandleSceneGraphFunctor
{
public:
    AddAffectedObjectFunctor(NiLightPtr spLight)
    {
        m_spLight = spLight;
    }

    ~AddAffectedObjectFunctor()
    {
        m_spLight = 0;
    }

    efd::Bool operator()(const egf::Entity* pEntity, const efd::vector<NiObjectPtr>& objects)
    {
        EE_UNUSED_ARG(pEntity);
        EE_ASSERT(objects.size() > 0);

        NiNode* pNode = NiDynamicCast(NiNode, objects[0]);
        if (pNode)
        {
            if (pNode->AttachEffect(m_spLight))
                pNode->UpdateEffects();
        }

        return false;
    }

    efd::Bool operator()(const SceneGraphService::SceneGraphHandle handle,
        const efd::vector<NiObjectPtr>& objects)
    {
        EE_UNUSED_ARG(handle);
        EE_ASSERT(objects.size() > 0);

        NiNode* pNode = NiDynamicCast(NiNode, objects[0]);
        if (pNode)
        {
            if (pNode->AttachEffect(m_spLight))
                pNode->UpdateEffects();
        }

        return false;
    }

    NiLightPtr m_spLight;
};

//------------------------------------------------------------------------------------------------
class AddUnaffectedCasterReceiverFunctor
    : public SceneGraphService::EntitySceneGraphFunctor
    , public SceneGraphService::HandleSceneGraphFunctor
{
public:
    AddUnaffectedCasterReceiverFunctor(EntityManager* pEntityManager,
        NiShadowGeneratorPtr spShadowGenerator)
    {
        m_pEntityManager = pEntityManager;
        m_spShadowGenerator = spShadowGenerator;
    }

    ~AddUnaffectedCasterReceiverFunctor()
    {
        m_spShadowGenerator = 0;
    }

    efd::Bool operator()(const egf::Entity* pEntity, const efd::vector<NiObjectPtr>& objects)
    {
        EE_ASSERT(objects.size() > 0);

        NiNode* pNode = NiDynamicCast(NiNode, objects[0]);

        if (pNode != NULL && pEntity != NULL)
        {
            // For this shadow generator, ensure that, depending on the entity property settings,
            // the NiNode casts shadows or not, and receives shadows or not.
            bool bUpdateNeeded = false;
            bool bIsCastingShadow;
            if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsCastingShadow,
                bIsCastingShadow) == PropertyResult_OK)
            {
                if (!bIsCastingShadow)
                    bUpdateNeeded = m_spShadowGenerator->AttachUnaffectedCasterNode(pNode);
            }

            bool bIsReceivingShadow;
            if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsReceivingShadow,
                bIsReceivingShadow) == PropertyResult_OK)
            {
                if (!bIsReceivingShadow)
                    bUpdateNeeded |= m_spShadowGenerator->AttachUnaffectedReceiverNode(pNode);
            }

            if (bUpdateNeeded)
                pNode->UpdateEffects();
        }

        return false;
    }

    efd::Bool operator()(const SceneGraphService::SceneGraphHandle handle,
        const efd::vector<NiObjectPtr>& objects)
    {
        EE_UNUSED_ARG(handle);
        EE_UNUSED_ARG(objects);

        return true;
    }

    EntityManager* m_pEntityManager;
    NiShadowGeneratorPtr m_spShadowGenerator;
};

//------------------------------------------------------------------------------------------------
// Handle entity added message (for lights only).
void LightService::HandleEntityAddedMessage(const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);
    EE_UNUSED_ARG(targetChannel);

    EE_ASSERT(m_spSceneGraphService);

    Entity* pEntity = pMessage->GetEntity();
    EE_ASSERT(pEntity);
    const egf::EntityID& entityID = pEntity->GetEntityID();

    const egf::FlatModel* pModel = pEntity->GetModel();
    EE_ASSERT(pModel);

    // Just handle lights.
    if (!pModel->ContainsModel(kFlatModelID_StandardModelLibrary_Light))
        return;

    // Create a light and update it's properties.
    NiLight* pLight = CreateLight(pModel);
    UpdateLightProperties(pLight, pEntity);

    // Add the light to Light Manager
    NiLightManager::GetLightManager()->Add(pLight);

    // Add to light list (position, radius, id).
    NiPoint3 center = pLight->GetTranslate();

    efd::Float32 range = 100.0f;
    pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MaxRange, range);

    LightData* pLightData = EE_NEW LightData();
    if (pLightData)
    {
        // Store light's EntityID, range, and center.
        pLightData->m_entityID = entityID;
        pLightData->m_radius = range;
        pLightData->m_position = center;
        pLightData->m_spLight = pLight;

        m_lightList.push_back(pLightData);
        m_entityLightMap[entityID] = pLightData;
        m_changedLightList.push_back(pLightData);

        // Populate the LightData's priority override map.
        efd::list<efd::utf8string> keyList;
        if (pEntity->GetPropertyKeys(kPropertyID_StandardModelLibrary_AlwaysAffectedByLight,
            keyList) == PropertyResult_OK)
        {
            for (efd::list<efd::utf8string>::iterator iter = keyList.begin();
                iter != keyList.end();  ++iter)
            {
                egf::EntityID kAffectedEntityID;
                pEntity->GetPropertyValue(
                    kPropertyID_StandardModelLibrary_AlwaysAffectedByLight, *iter,
                    kAffectedEntityID);
                pLightData->m_entityPriorityOverride[kAffectedEntityID] = MAX_LIGHT_PRIORITY;
            }
        }

        if (pEntity->GetPropertyKeys(kPropertyID_StandardModelLibrary_NeverAffectedByLight,
            keyList) == PropertyResult_OK)
        {
            for (efd::list<efd::utf8string>::iterator iter = keyList.begin();
                iter != keyList.end(); ++iter)
            {
                egf::EntityID kAffectedEntityID;
                pEntity->GetPropertyValue(
                    kPropertyID_StandardModelLibrary_NeverAffectedByLight, *iter,
                    kAffectedEntityID);
                pLightData->m_entityPriorityOverride[kAffectedEntityID] = 0;
            }
        }
    }

    if (!m_enableAutomaticRelighting)
    {
        // Add all nodes to the light's effect.
        AddAffectedObjectFunctor kFunctor(pLight);
        m_spSceneGraphService->ForEachEntitySceneGraph(kFunctor);
    }

    UpdateShadowCaster(pLight, pEntity);
}

//------------------------------------------------------------------------------------------------
NiLight* LightService::CreateLight(const egf::FlatModel* pModel)
{
    if (pModel->ContainsModel(kFlatModelID_StandardModelLibrary_SpotLight))
        return EE_NEW NiSpotLight();
    else if (pModel->ContainsModel(kFlatModelID_StandardModelLibrary_PointLight))
        return EE_NEW NiPointLight();
    else if (pModel->ContainsModel(kFlatModelID_StandardModelLibrary_DirectionalLight))
        return EE_NEW NiDirectionalLight();
    else if (pModel->ContainsModel(kFlatModelID_StandardModelLibrary_AmbientLight))
        return EE_NEW NiAmbientLight();

    EE_FAIL_MESSAGE(("Unknown Light Type for model '%s'", pModel->GetName().c_str()));
    return NULL;
}

//------------------------------------------------------------------------------------------------
// Handle entity removed message (for lights only).
void LightService::HandleEntityRemovedMessage(const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);
    EE_UNUSED_ARG(targetChannel);

    Entity* pEntity = pMessage->GetEntity();
    EE_ASSERT(pEntity);
    egf::EntityID entityID = pEntity->GetEntityID();

    // Look for a node that represents the entity.
    efd::map<egf::EntityID, LightData*>::iterator itLight = m_entityLightMap.find(entityID);

    // If the entity being removed isn't a light, return.
    if (itLight == m_entityLightMap.end())
        return;

    NiLight* pLight = itLight->second->m_spLight;

    NiShadowGenerator* pShadowGenerator = pLight->GetShadowGenerator();

    NiPSSMShadowClickGenerator* pPSSMClickGenerator = NiDynamicCast(NiPSSMShadowClickGenerator,
        NiShadowManager::GetActiveShadowClickGenerator());

    if (pPSSMClickGenerator && pShadowGenerator)
    {
        if (pPSSMClickGenerator->GetActiveGenerator() == pShadowGenerator)
            pPSSMClickGenerator->SetActiveGenerator(NULL);

        // Force the PSSM to rebuild the shadowing frustums.
        NiPSSMConfiguration* pConfiguration =
            pPSSMClickGenerator->GetConfiguration(pShadowGenerator);
        pConfiguration->SetRebuildFrustums(true);
    }

    NiNodeList& affectedNodeList = const_cast<NiNodeList&>(pLight->GetAffectedNodeList());
    while (!affectedNodeList.IsEmpty())
    {
        NiNode *pNode = affectedNodeList.GetHead();
        affectedNodeList.RemoveHead();
        if (pNode)
        {
            pNode->DetachEffect(pLight);
            pNode->UpdateEffects();
        }
    }

    pLight->DetachAllAffectedNodes();
    pLight->DetachAllUnaffectedNodes();
    pLight->Update(0.0f);

    // Remove the light from Light Manager
    NiLightManager::GetLightManager()->Remove(pLight);

    // Remove light from light entity map and light range map.
    m_entityLightMap.erase(entityID);

    // Remove light from the light list.
    efd::vector<LightData*>::iterator itCurrLight;
    for (itCurrLight = m_lightList.begin(); itCurrLight != m_lightList.end(); itCurrLight++)
    {
        if ((*itCurrLight)->m_entityID == entityID)
        {
            EE_DELETE(*itCurrLight);
            m_lightList.erase(itCurrLight);
            break;
        }
    }

    // Remove light from the list of updates.
    for (itCurrLight = m_changedLightList.begin(); itCurrLight != m_changedLightList.end();
        itCurrLight++)
    {
        if ((*itCurrLight)->m_entityID == entityID)
        {
            m_changedLightList.erase(itCurrLight);
            break;
        }
    }
}

//------------------------------------------------------------------------------------------------
void LightService::HandleEntityUpdatedMessage(const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);
    EE_UNUSED_ARG(targetChannel);

    Entity* pEntity = pMessage->GetEntity();
    EE_ASSERT(pEntity);

    egf::EntityID kEntityID = pEntity->GetEntityID();

    // Look for a node that represents the entity.
    efd::map<egf::EntityID, LightData*>::iterator itor = m_entityLightMap.find(kEntityID);

    // If the entity being updated isn't a light.
    if (itor == m_entityLightMap.end())
    {
		// Check embedded lights
		const egf::FlatModel* pModel = pEntity->GetModel();
		EE_ASSERT(pModel);
		if (pModel->ContainsProperty(kPropertyID_StandardModelLibrary_UseEmbeddedLights))
		{
			if (pEntity->IsDirty(kPropertyID_StandardModelLibrary_UseEmbeddedLights))
			{
				NiAVObject* pObject = m_spSceneGraphService->GetSceneGraphFromEntity(pEntity);
				if (pObject)
				{
					// Is the entity using embedded lights?
					bool bUseEmbeddedLights = false;
					pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_UseEmbeddedLights,
						bUseEmbeddedLights);

					// Scrape the lights from the Entity.
					efd::vector<NiLight*> lights;
					ScrapeLights(pObject, lights);

					NiLightManager* pLightManager = NiLightManager::GetLightManager();

					// Add/Remove lights depending on settings
					for (efd::vector<NiLight*>::iterator iter = lights.begin();
						iter != lights.end();
						++iter)
					{
						if (bUseEmbeddedLights)
							pLightManager->Add(*iter);
						else
							pLightManager->Remove(*iter);
					}
				}
			}
		}

		// Check shadow cast/receive property settings.
        bool bCastDirty = false;
        bool bIsCastingShadow = false;
        if (pEntity->IsDirty(kPropertyID_StandardModelLibrary_IsCastingShadow))
        {
            EE_VERIFYEQUALS(
                pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsCastingShadow,
                bIsCastingShadow), PropertyResult_OK);
            bCastDirty = true;
        }

        bool bReceiveDirty = false;
        bool bIsReceivingShadow = false;
        if (pEntity->IsDirty(kPropertyID_StandardModelLibrary_IsReceivingShadow))
        {
            EE_VERIFYEQUALS(
                pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsReceivingShadow,
                bIsReceivingShadow), PropertyResult_OK);
            bReceiveDirty = true;
        }

        if (!bCastDirty && !bReceiveDirty)
            return;

        NiNode* pNode =
            NiDynamicCast(NiNode, m_spSceneGraphService->GetSceneGraphFromEntity(pEntity));

        if (!pNode)
            return;

        // Iterate through all the shadow generators and ensure that, depending on the entity
        // property settings, the NiNode casts shadows or not, and receives shadows or not.
        bool bUpdateNeeded = false;
        const NiTPointerList<NiShadowGeneratorPtr>& kShadowGenerators =
            NiShadowManager::GetShadowGenerators();
        NiTListIterator kIter = kShadowGenerators.GetHeadPos();
        while (kIter)
        {
            NiShadowGeneratorPtr spGenerator = kShadowGenerators.GetNext(kIter);
            if (spGenerator)
            {
                if (bCastDirty)
                {
                    if (!bIsCastingShadow)
                        bUpdateNeeded = spGenerator->AttachUnaffectedCasterNode(pNode);
                    else
                        bUpdateNeeded = spGenerator->DetachUnaffectedCasterNode(pNode);
                }

                if (bReceiveDirty)
                {
                    if (!bIsReceivingShadow)
                        bUpdateNeeded |= spGenerator->AttachUnaffectedReceiverNode(pNode);
                    else
                        bUpdateNeeded |= spGenerator->DetachUnaffectedReceiverNode(pNode);
                }
            }
        }

        if (bUpdateNeeded)
            pNode->UpdateEffects();

        return;
    }
    else
    {
        LightEntityChanged(itor->second, pEntity);
    }

	
	

	

}

//------------------------------------------------------------------------------------------------
// Functor for managing scene graph updates
class UpdateEntitiesFunctor : public SceneGraphService::EntitySceneGraphFunctor
{
public:
    UpdateEntitiesFunctor(LightService* pLightService, EntityManager* pEntityManager)
        : m_pLightService(pLightService)
        , m_pEntityManager(pEntityManager)
    {
    }

    efd::Bool operator()(const egf::Entity* pEntity, const efd::vector<NiObjectPtr>& objects)
    {
        // If the entity being updated isn't a light, mark it as changed.
        if (!m_pLightService->IsEntityLight(pEntity->GetEntityID()))
        {
            if (objects.size() == 0)
                return false;

            NiNodePtr spNode = NiDynamicCast(NiNode, objects[0]);
            if (!spNode)
                return false;

            m_pLightService->EntityChanged(spNode, pEntity->GetEntityID());
        }

        return false;
    }

    LightService* m_pLightService;
    EntityManager* m_pEntityManager;
};

//------------------------------------------------------------------------------------------------
// Handle scene graph updated message for entities (not lights) that have changed.
void LightService::HandleSceneGraphsUpdatedMessage(const SceneGraphsUpdatedMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);
    EE_UNUSED_ARG(targetChannel);
    EE_UNUSED_ARG(pMessage);

    UpdateEntitiesFunctor functor(this, m_pEntityManager);
    m_spSceneGraphService->ForEachUpdatedEntity(functor);
}

//------------------------------------------------------------------------------------------------
void LightService::HandleSceneGraphAddedMessage(const SceneGraphAddedMessage* pMessage, 
	efd::Category targetChannel)
{
	// Get the entity and model
	EE_ASSERT(pMessage);
	EE_UNUSED_ARG(targetChannel);
	
	Entity* pEntity = pMessage->GetEntity();
	if (!pEntity)
		return;

	const egf::FlatModel* pModel = pEntity->GetModel();
	EE_ASSERT(pModel);

	// Can this entity have embedded lights
	if (pModel->ContainsProperty(kPropertyID_StandardModelLibrary_UseEmbeddedLights))
	{
		// Is the entity using embedded lights
		bool useEmbeddedLights = false;
		pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_UseEmbeddedLights,
			useEmbeddedLights);

		if (!useEmbeddedLights)
			return;

		// Scrape for the lights
		NiAVObject* pObject = m_spSceneGraphService->GetSceneGraphFromEntity(pEntity);
		if (pObject)
		{
			typedef efd::vector<NiLight*> LightCollection;
			LightCollection kLights;
			ScrapeLights(pObject, kLights);

			// Iterate and add the lights
			for (LightCollection::iterator iter = kLights.begin();
				iter != kLights.end();
				++iter)
			{
				NiLightManager::GetLightManager()->Add(*iter);
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
void LightService::HandleSceneGraphRemovedMessage(const SceneGraphRemovedMessage* pMessage, 
	efd::Category targetChannel)
{
	// Get the entity and model
	EE_ASSERT(pMessage);
	EE_UNUSED_ARG(targetChannel);

	Entity* pEntity = pMessage->GetEntity();
	if (!pEntity)
		return;

	const egf::FlatModel* pModel = pEntity->GetModel();
	EE_ASSERT(pModel);

	// Can this entity have embedded lights
	if (pModel->ContainsProperty(kPropertyID_StandardModelLibrary_UseEmbeddedLights))
	{
		bool useEmbeddedLights = false;
		pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_UseEmbeddedLights,
			useEmbeddedLights);

		// Is the entity using embedded lights
		if (!useEmbeddedLights)
			return;

		// Scrape for the lights
		NiAVObject* pObject = m_spSceneGraphService->GetSceneGraphFromEntity(pEntity);
		if (pObject)
		{
			typedef efd::vector<NiLight*> LightCollection;
			LightCollection kLights;
			ScrapeLights(pObject, kLights);

			// Iterate and remove the lights
			for (LightCollection::iterator iter = kLights.begin();
				iter != kLights.end();
				++iter)
			{
				NiLightManager::GetLightManager()->Remove(*iter);
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
bool LightService::ShadowRenderStepPre(NiRenderStep* pCurrentStep, void* pvCallbackData)
{
    EE_UNUSED_ARG(pvCallbackData);

    // Get the list of render clicks from the shadow manager.
    const NiTPointerList<NiRenderClick*>& kShadowClicks = NiShadowManager::GenerateRenderClicks();

    // Replace the render clicks in the shadow render step with those provided by the shadow
    // manager.
    EE_ASSERT(NiIsKindOf(NiDefaultClickRenderStep, pCurrentStep));
    NiDefaultClickRenderStep* pClickRenderStep = (NiDefaultClickRenderStep*)pCurrentStep;
    pClickRenderStep->GetRenderClickList().RemoveAll();
    NiTListIterator kIter = kShadowClicks.GetHeadPos();

    while (kIter)
        pClickRenderStep->AppendRenderClick(kShadowClicks.GetNext(kIter));

    return true;
}

//------------------------------------------------------------------------------------------------
void LightService::EntityChanged(NiNodePtr spNode, const egf::EntityID entityID)
{
    // Update receiver's information (position, radius, id).
    EE_ASSERT(spNode);

    ReceiverData* pReceiverData = FindReceiverData(entityID);
    if (pReceiverData)
    {
        // Store object's bounding sphere radius, and center.
        NiBound kBound = spNode->GetWorldBound();

        if (pReceiverData->m_position != kBound.GetCenter() ||
            pReceiverData->m_radius != kBound.GetRadius())
        {
            pReceiverData->m_radius = kBound.GetRadius();
            pReceiverData->m_position = kBound.GetCenter();
            pReceiverData->m_spNode = spNode;

            // Add this receiver to the changed list.
            m_changedNodeList.push_back(pReceiverData);
            m_RecieverListDirty = true;
        }
    }
}

//------------------------------------------------------------------------------------------------
void LightService::LightEntityChanged(LightData* pLightData, egf::Entity* pEntity)
{
    // Found the light.
    NiLight* pkLight = pLightData->m_spLight;

    // Store properties on NiLight and add Entity to changed light list.
    egf::EntityID kEntityID = pEntity->GetEntityID();

    UpdateLightProperties(pkLight, pEntity);
    UpdateLightData(pEntity, kEntityID);
    UpdateShadowCaster(pkLight, pEntity);

    // Check to see if we need to recompute which objects the light affects.
    bool bUpdateOnMove = true;
    pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_UpdateLightingOnMove,
        bUpdateOnMove);

    if (bUpdateOnMove)
        m_changedLightList.push_back(pLightData);
}

//------------------------------------------------------------------------------------------------
void LightService::UpdateLightProperties(NiLight* pLight, Entity* pEntity)
{
    // Position.
    efd::Point3 position;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Position,
        position) == PropertyResult_OK)
    {
        pLight->SetTranslate(position.x, position.y, position.z);
    }

    // Rotation.
    efd::Point3 rotation;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Rotation,
        rotation) == PropertyResult_OK)
    {
        NiMatrix3 kXRot, kYRot, kZRot;
        kXRot.MakeXRotation(rotation.x * -EE_DEGREES_TO_RADIANS);   // Roll  +x
        kYRot.MakeYRotation(rotation.y * -EE_DEGREES_TO_RADIANS);   // Pitch +y
        kZRot.MakeZRotation(rotation.z * -EE_DEGREES_TO_RADIANS);   // Yaw   +z
        pLight->SetRotate(kXRot * kYRot * kZRot);
    }

    // Scale.
    float scale;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Scale, scale)
        == PropertyResult_OK)
    {
        pLight->SetScale(scale);
    }

    // General Light Properties.
    efd::Color ambientColor;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AmbientColor, ambientColor) ==
        PropertyResult_OK)
    {
        pLight->SetAmbientColor(NiColor(ambientColor.m_r, ambientColor.m_g, ambientColor.m_b));
    }
    efd::Color diffuseColor;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_DiffuseColor, diffuseColor) ==
        PropertyResult_OK)
    {
        pLight->SetDiffuseColor(NiColor(diffuseColor.m_r, diffuseColor.m_g, diffuseColor.m_b));
    }
    efd::Color specularColor;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_SpecularColor, specularColor) ==
        PropertyResult_OK)
    {
        pLight->SetSpecularColor(
            NiColor(specularColor.m_r, specularColor.m_g, specularColor.m_b));
    }
    efd::Float32 dimmer;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Dimmer, dimmer) ==
        PropertyResult_OK)
    {
        pLight->SetDimmer(dimmer);
    }

	efd::Float32 falloff;
	if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Falloff, falloff) == 
		PropertyResult_OK)
	{
		pLight->SetFalloff(falloff);
	}

	efd::Float32 range;
	if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Range, range) == 
		PropertyResult_OK)
	{
		pLight->SetRange(range);
	}

    const egf::FlatModel* pModel = pEntity->GetModel();
    bool isSpotLight = pModel->ContainsModel(kFlatModelID_StandardModelLibrary_SpotLight);
    bool isPointLight =
        isSpotLight || pModel->ContainsModel(kFlatModelID_StandardModelLibrary_PointLight);

    if (isSpotLight)    // Spot Light properties.
    {
        NiSpotLight* spotLight = NiVerifyStaticCast(NiSpotLight, pLight);

        efd::Float32 outerSpotAngle;
        if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_OuterSpotAngle,
            outerSpotAngle) == PropertyResult_OK)
        {
            spotLight->SetSpotAngle(outerSpotAngle);
        }

        efd::Float32 innerSpotAngle;
        if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_InnerSpotAngle,
            innerSpotAngle) == PropertyResult_OK)
        {
            spotLight->SetInnerSpotAngle(innerSpotAngle);
        }

        efd::Float32 spotExp;
        if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_SpotExponent, spotExp) ==
            PropertyResult_OK)
        {
            spotLight->SetSpotExponent(spotExp);
        }
    }

    if (isPointLight)   // Spot or Point Light properties.
    {

    }

    // Update changed lights.
    float currTime = (float)m_pServiceManager->GetTime(kCLASSID_GameTimeClock);
    pLight->Update(currTime);
}

//------------------------------------------------------------------------------------------------
void LightService::UpdateShadowCaster(NiLight* pLight, Entity* pEntity)
{
    bool isShadowCaster;
    NiShadowGenerator* pShadowGenerator = NULL;

    NiPSSMShadowClickGenerator* pkPSSMClickGenerator =
        NiDynamicCast(NiPSSMShadowClickGenerator,
        NiShadowManager::GetActiveShadowClickGenerator());

    if (pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_ShadowGenerator) &&
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_CastShadows, isShadowCaster) ==
        PropertyResult_OK)
    {
        if (isShadowCaster)
        {
            // Get NiShadowGenerator or add one to the light, as appropriate.
            pShadowGenerator = pLight->GetShadowGenerator();
            if (!pShadowGenerator)
            {
                pShadowGenerator = EE_NEW NiShadowGenerator(pLight);
                EE_ASSERT(pShadowGenerator);

                if (!pShadowGenerator)
                    return;

                NiShadowManager::AddShadowGenerator(pShadowGenerator);

                // Iterate through all the scene graphs and update the Unaffected Caster and
                // Unaffected Receiver lists accordingly.
                AddUnaffectedCasterReceiverFunctor kFunctor(m_pEntityManager, pShadowGenerator);
                m_spSceneGraphService->ForEachEntitySceneGraph(kFunctor);
            }
            pShadowGenerator->SetActive(true, true, true);

            if (pkPSSMClickGenerator)
            {
                bool bPSSMEnabled = false;
                if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_PSSMEnabled,
                    bPSSMEnabled) == PropertyResult_OK)
                {

                   if (bPSSMEnabled && pkPSSMClickGenerator->GetActiveGenerator() != NULL &&
                        pkPSSMClickGenerator->GetActiveGenerator() != pShadowGenerator)
                    {
                        // A different directional light's shadow generator is currently set as
                        // the active PSSM shadow generator.  Currently, only one directional
                        // light is allowed to use PSSM.

                        // Disable PSSM support for this light.
                        pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_PSSMEnabled,
                            false);

                        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR3,
                            ("Attempted to enable PSSM, but PSSM is already enabled."
                            " PSSM can only be enabled on one directional light."));

                        bPSSMEnabled = false;
                    }

                    if (bPSSMEnabled)
                    {
                        // Set the directional light's shadow generator as the active PSSM
                        // generator.
                        pkPSSMClickGenerator->SetActiveGenerator(pShadowGenerator);

                        // Force the PSSM to rebuild the shadowing frustums.
                        NiPSSMConfiguration* pkConfiguration =
                            pkPSSMClickGenerator->GetConfiguration(pShadowGenerator);
                        pkConfiguration->SetRebuildFrustums(true);

                        // Set PSSM configurations.
                        efd::UInt32 bInt32;
                        efd::Bool bBool;
                        efd::Float32 fFloat;

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_SceneDependentFrustums, bBool))
                        {
                            pkConfiguration->SetSceneDependentFrustumsEnabled(bBool);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_SliceCount, bInt32))
                        {
                            pkConfiguration->SetNumSlices((efd::UInt8)bInt32);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_SliceDistanceExponentFactor, fFloat))
                        {
                            pkConfiguration->SetSliceLambda(fFloat);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_SliceTransitionLength, fFloat))
                        {
                            pkConfiguration->SetSliceTransitionSize(fFloat);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_SliceTransitionNoiseGranularity,
                            fFloat))
                        {
                            pkConfiguration->SetSliceTransitionNoiseScale(fFloat);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_SliceTransitions, bBool))
                        {
                            pkConfiguration->SetSliceTransitionEnabled(bBool);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_SuppressShimmer, bBool))
                        {
                            pkConfiguration->SetSubTexelOffsetEnabled(bBool);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_UseCustomFarPlane, bBool))
                        {
                            pkConfiguration->SetCustomSceneCameraFarPlaneEnabled(bBool);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_CustomFarPlane, fFloat))
                        {
                            pkConfiguration->SetCustomSceneCameraFarPlane(fFloat);
                        }

                        if (PropertyResult_OK == pEntity->GetPropertyValue(
                            kPropertyID_StandardModelLibrary_CameraDistanceScaleFactor, fFloat))
                        {
                            pkConfiguration->SetCameraDistanceScaleFactor(fFloat);
                        }
                    }
                }

                if (!bPSSMEnabled &&
                    pkPSSMClickGenerator->GetActiveGenerator() == pShadowGenerator)
                {
                    // pShadowGenerator is still set as the active PSSM directional light, even
                    // though PSSM is not enabled for this light.  So set the active PSSM shadow
                    // generator to null.
                    pkPSSMClickGenerator->SetActiveGenerator(NULL);

                    // Force the PSSM to rebuild the shadowing frustums.
                    NiPSSMConfiguration* pkConfiguration =
                        pkPSSMClickGenerator->GetConfiguration(pShadowGenerator);
                    pkConfiguration->SetRebuildFrustums(true);
                }
            }
        }
        else
        {
            // Get and delete the NiShadowGenerator, if it exists.
            pShadowGenerator = pLight->GetShadowGenerator();
            if (pShadowGenerator)
            {
                if (pkPSSMClickGenerator->GetActiveGenerator() == pShadowGenerator)
                {
                    // pShadowGenerator is still set as the active PSSM directional light, even
                    // though PSSM is not enabled for this light.  So set the active PSSM shadow
                    // generator to null.
                    pkPSSMClickGenerator->SetActiveGenerator(NULL);

                    // Force the PSSM to rebuild the shadowing frustums.
                    NiPSSMConfiguration* pkConfiguration =
                        pkPSSMClickGenerator->GetConfiguration(pShadowGenerator);
                    pkConfiguration->SetRebuildFrustums(true);
                }

                NiShadowManager::DeleteShadowGenerator(pShadowGenerator);
                pShadowGenerator = NULL;
            }
        }
    }

    if (!pShadowGenerator)
        return;

    bool bFlag;

    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_RenderBackfaces, bFlag) ==
        PropertyResult_OK)
    {
        pShadowGenerator->SetRenderBackfaces(bFlag);
    }
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_StaticShadows, bFlag) ==
        PropertyResult_OK)
    {
        pShadowGenerator->SetStatic(bFlag);
    }
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_StrictlyObserveSizeHint, bFlag)
        == PropertyResult_OK)
    {
        pShadowGenerator->SetStrictlyObserveSizeHint(bFlag);
    }

    bool bUseDefaultDepthBias;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_UseDefaultDepthBias,
        bUseDefaultDepthBias) == PropertyResult_OK)
    {
        if (bUseDefaultDepthBias)
        {
            pShadowGenerator->SetDepthBiasToDefault();
        }
        else
        {
            float fDepthBias;
            if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_DepthBias, fDepthBias)
                == PropertyResult_OK)
            {
                pShadowGenerator->SetDepthBias(fDepthBias);
            }
        }
    }

    efd::UInt16 usSizeHint;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_SizeHint, usSizeHint) ==
        PropertyResult_OK)
    {
        pShadowGenerator->SetSizeHint(usSizeHint);
    }

    efd::utf8string kTechniqueName;
    if (pEntity->GetPropertyValue(
        kPropertyID_StandardModelLibrary_ShadowTechnique, kTechniqueName) == PropertyResult_OK)
    {
        pShadowGenerator->SetShadowTechnique(NiFixedString(kTechniqueName.c_str()),
            bUseDefaultDepthBias);
    }
}

//------------------------------------------------------------------------------------------------
float LightService::GetLightPriorityForReceiver(LightData* pLightData, ReceiverData* pReceiverData)
{
    EE_ASSERT(pLightData && pReceiverData);
    NiLight* pkLight = pLightData->m_spLight;

    // This function works by first computing the light's influence on the receiver and then
    // adding in the light's priority.
    // Influence is computed as a sum of the following 3 components:
    //   -  The 'rough' percentage of the receiver that will be affected by the light.
    //   -  The receiver's distance from the light, as a percentage of the light's range.
    //   -  The light's intensity on the receiver
    // Each component can range from 0.0 to 1.0f.

    NiPoint3 kDelta = pLightData->m_position - pReceiverData->m_position;
    const float fDistance = kDelta.Length();

    // Add FLT_EPSILON to avoid divide by zero.
    float fReceiverRadius = pReceiverData->m_radius + FLT_EPSILON;
    float fLightRadius = pLightData->m_radius + FLT_EPSILON;

    // Compute the 'rough' 1D percent of the receiver that is affected.
    float fAffectedDist = fDistance - fLightRadius - fReceiverRadius;
    fAffectedDist = -fAffectedDist;
    fAffectedDist = efd::Min(fAffectedDist, fReceiverRadius * 2);
    float fPercentAffected = fAffectedDist / (fReceiverRadius * 2);
    fPercentAffected = efd::Clamp(fPercentAffected, 0.0f, 1.0f);

    // Compute the 'in range' percentage of the reciever.
    float fDistMinRadius = fDistance - fReceiverRadius;
    fDistMinRadius = efd::Max(fDistMinRadius, 0.0f);
    float fPercentInRange = efd::Min(fDistMinRadius, fLightRadius);
    fPercentInRange = 1.0f - (fPercentInRange / fLightRadius);
    fPercentInRange = efd::Clamp(fPercentInRange, 0.0f, 1.0f);

    // Compute light intensity.  Note: Light color is explicitly _not_ considered when computing
    // the light's intensity.
    float fIntensity = efd::Abs(pkLight->GetDimmer());
    
    // Compute the attenuation equation.
    float fAttenuation = efd::Clamp(fDistMinRadius / pkLight->GetRange(), 0.0f, 1.0f);
    fAttenuation = efd::Pow(fAttenuation, pkLight->GetFalloff());

    fIntensity *= fAttenuation;
        
    // Clamp intensity range
    fIntensity = efd::Min(fIntensity, 1.0f);
    fIntensity = efd::Clamp(fIntensity, 0.0f, 1.0f);

    // Obtain light's priority.
    float fPriority = 0;

    // First, we check to see if the receiver is in the light's priority override map.
    efd::hash_map<egf::EntityID, float, egf::EntityIDHashFunctor>::iterator itReciever =
        pLightData->m_entityPriorityOverride.find(pReceiverData->m_entityID);

    if (itReciever != pLightData->m_entityPriorityOverride.end())
    {
        // Use the priority override.  The priority override used to allow certain entities to
        // either be explicitly lit or not light by a light.
        fPriority = itReciever->second;
    }
    else
    {
        // If the receiver was not in the priority override, look up the light's priority.
        egf::Entity* pLightEntity = m_pEntityManager->LookupEntity(pLightData->m_entityID);
        EE_ASSERT(pLightEntity);
        if (pLightEntity)
        {
            pLightEntity->GetPropertyValue(
                kPropertyID_StandardModelLibrary_LightPriority,
                fPriority);
        }
        // @todo: what to do with fPriority if pLightEntity is not found? I'm assuming leaving 0
        // is a good choice, but I don't know this code path.
    }

    // Since Intensity + PercentInRange + PercentAffected can sum to a maximum of 3.0f, we scale
    // the priority to range from -4.0f to 4.0f.  This approach enables a light with zero
    // influence, but a priority of MAX_LIGHT_PRIORITY, to be chosen over a light with full
    // influence but default priority.
    fPriority /= MAX_LIGHT_PRIORITY;
    fPriority = (fPriority * 8.0f) - 4.0f;

    // Compute the light's influence on the receiver and add it to the priority.
    fPriority += fIntensity + fPercentInRange + fPercentAffected;
    return fPriority;
}

//------------------------------------------------------------------------------------------------
bool LightService::AddReceiverToLightQueue(LightData* pLightData, ReceiverData* pReceiverData)
{
    bool bUpdateNeeded = false;
    LightPriority kLightPriority;

    // Add the light to the appopriate light type queue, except for ambient lights.  There is no
    // limit to the number of ambient lights that can affect an entity.
    switch (pLightData->m_spLight->GetEffectType())
    {
        case NiDynamicEffect::AMBIENT_LIGHT:
            bUpdateNeeded |= pReceiverData->m_spNode->AttachEffect(pLightData->m_spLight);
        break;

        case NiDynamicEffect::POINT_LIGHT:
        case NiDynamicEffect::SHADOWPOINT_LIGHT:
            kLightPriority.m_pLight = pLightData->m_spLight;
            kLightPriority.m_fPriority = GetLightPriorityForReceiver(pLightData, pReceiverData);
            m_PointPriorityQueue.push(kLightPriority);
        break;

        case NiDynamicEffect::DIR_LIGHT:
        case NiDynamicEffect::SHADOWDIR_LIGHT:
            kLightPriority.m_pLight = pLightData->m_spLight;
            kLightPriority.m_fPriority = GetLightPriorityForReceiver(pLightData, pReceiverData);
            m_DirPriorityQueue.push(kLightPriority);
        break;

        case NiDynamicEffect::SPOT_LIGHT:
        case NiDynamicEffect::SHADOWSPOT_LIGHT:
            kLightPriority.m_pLight = pLightData->m_spLight;
            kLightPriority.m_fPriority = GetLightPriorityForReceiver(pLightData, pReceiverData);
            m_SpotPriorityQueue.push(kLightPriority);
        break;

        default:
            EE_FAIL("Unsupported NiDynamicEffect Type");
    }

    return bUpdateNeeded;
}

//------------------------------------------------------------------------------------------------
void LightService::AddReceiverData(NiNodePtr spNode, const egf::EntityID& entityID)
{
    EE_ASSERT(spNode != NULL);

    ReceiverData* pReceiverData = EE_NEW ReceiverData();
    if (pReceiverData)
    {
        // Store object's EntityID, bounding sphere radius, and center.
        NiBound kBound = spNode->GetWorldBound();
        pReceiverData->m_entityID = entityID;
        pReceiverData->m_radius = kBound.GetRadius();
        pReceiverData->m_position = kBound.GetCenter();
        pReceiverData->m_spNode = spNode;
        m_receiverList.push_back(pReceiverData);
        m_entityReceiverMap[entityID] = pReceiverData;

        // If you hit this assert, the receiver node has an invalid bounding volume.  Most likely,
        // it has never been updated before.
        EE_ASSERT(kBound.GetRadius() > 0.0f);

        // Add this new node to the changed node list.
        m_changedNodeList.push_back(pReceiverData);
    }
}

//------------------------------------------------------------------------------------------------
void LightService::RemoveReceiverData(const egf::EntityID& entityID)
{
    // Remove light from light entity map and light range map.
    m_entityReceiverMap.erase(entityID);

    efd::vector<ReceiverData*>::iterator itCurrReceiver;
    for (itCurrReceiver = m_receiverList.begin(); itCurrReceiver != m_receiverList.end();
        itCurrReceiver++)
    {
        if ((*itCurrReceiver)->m_entityID == entityID)
        {
            EE_DELETE(*itCurrReceiver);
            m_receiverList.erase(itCurrReceiver);
            break;
        }
    }

    // Remove any pending changes - allowing for the possibility of more than one per entity.
    itCurrReceiver = m_changedNodeList.begin();
    while (itCurrReceiver != m_changedNodeList.end())
    {
        ReceiverData* pReceiverData = *itCurrReceiver;
        if (pReceiverData->m_entityID == entityID)
        {
            // Remove change.
            efd::vector<ReceiverData*>::iterator itErase = itCurrReceiver;
            ++itCurrReceiver;

            // break if erasing the last item.
            if (itCurrReceiver == m_changedNodeList.end())
            {
                m_changedNodeList.erase(itErase);
                break;
            }
            else
            {
                m_changedNodeList.erase(itErase);
            }
        }
        else
        {
            ++itCurrReceiver;
        }
    }
}

//------------------------------------------------------------------------------------------------
void LightService::UpdateLightData(egf::Entity* pEntity, const egf::EntityID& entityID)
{
    // Update lights's information (position, range).
    EE_ASSERT(pEntity);

    // Find light data and update it.
    LightData* pLightData = NULL;
    efd::vector<LightData*>::iterator itCurrLight;
    for (itCurrLight = m_lightList.begin(); itCurrLight != m_lightList.end(); itCurrLight++)
    {
        if ((*itCurrLight)->m_entityID == entityID)
        {
            pLightData = (*itCurrLight);
            break;
        }
    }

    if (pLightData)
    {
        // Position.
        efd::Point3 position;
        if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Position, position) ==
            PropertyResult_OK)
        {
            pLightData->m_position = position;
        }

        // Max Range.
        efd::Float32 maxRange;
        if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MaxRange, maxRange) ==
            PropertyResult_OK)
        {
            pLightData->m_radius = maxRange;
        }

        // Keep the LightData's priority override map up-to-date.
        if (pEntity->IsDirty(kPropertyID_StandardModelLibrary_AlwaysAffectedByLight) ||
            pEntity->IsDirty(kPropertyID_StandardModelLibrary_NeverAffectedByLight))
        {
            // Clear the map and the re-populate it with the new data.
            pLightData->m_entityPriorityOverride.clear();

            efd::list<efd::utf8string> keyList;
            if (pEntity->GetPropertyKeys(kPropertyID_StandardModelLibrary_AlwaysAffectedByLight,
                keyList) == PropertyResult_OK)
            {
                for (efd::list<efd::utf8string>::iterator iter = keyList.begin();
                    iter != keyList.end(); ++iter)
                {
                    egf::EntityID kEntityID;
                    pEntity->GetPropertyValue(
                        kPropertyID_StandardModelLibrary_AlwaysAffectedByLight, *iter, kEntityID);
                    pLightData->m_entityPriorityOverride[kEntityID] = MAX_LIGHT_PRIORITY;
                }
            }

            if (pEntity->GetPropertyKeys(kPropertyID_StandardModelLibrary_NeverAffectedByLight,
                keyList) == PropertyResult_OK)
            {
                for (efd::list<efd::utf8string>::iterator iter = keyList.begin();
                    iter != keyList.end(); ++iter)
                {
                    egf::EntityID kEntityID;
                    pEntity->GetPropertyValue(
                        kPropertyID_StandardModelLibrary_NeverAffectedByLight, *iter, kEntityID);
                    pLightData->m_entityPriorityOverride[kEntityID] = 0;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void LightService::SortList(efd::vector<BaseObjectData*> * pObjList)
{
    // Insertion sort used for list will be kept mostly sorted
    int n = (int)pObjList->size();

    for (int i = 1; i < n; i++)
    {
        BaseObjectData* pTemp = (*pObjList)[i];

        int j = i-1;
        while ((j >= 0) && (BaseObjectData::ObjectFurther((*pObjList)[j], pTemp)))
        {
            (*pObjList)[j+1] = (*pObjList)[j];
            --j;
        }
        (*pObjList)[j+1] = pTemp;
    }
}

//------------------------------------------------------------------------------------------------
bool LightService::LightAffectsReceiver(const LightData* pkLight, const ReceiverData* pkReceiver)
{
    // Handle precomputed objects
    if (m_bUsePrecomputedLighting)
    {
        Entity* pLightEntity = m_pEntityManager->LookupEntity(pkLight->m_entityID);
        Entity* pRecieverEntity = m_pEntityManager->LookupEntity(pkReceiver->m_entityID);
        EE_ASSERT(pLightEntity);
        EE_ASSERT(pRecieverEntity);

        bool bRecieverUsePCL = false;
        pRecieverEntity->GetPropertyValue(
            kPropertyID_StandardModelLibrary_UseForPrecomputedLighting,
            bRecieverUsePCL);

        if (bRecieverUsePCL)
        {
            // If this object is already lit by precomputed lighting, and the light is not set
            // to add dynamic lighting to precomputed objects, do not light.
            bool bLightPCLObjectsAtRuntime = false;
            pLightEntity->GetPropertyValue(
                kPropertyID_StandardModelLibrary_LightPCLObjectsAtRuntime,
                bLightPCLObjectsAtRuntime);

            if (!bLightPCLObjectsAtRuntime)
                return false;
        }
        else
        {
            // If the receiver doesn't use precomputed lighting, and the light is not set to light
            // non-precomputed lighting objects, also do not light.
            bool bLightNonPCLObjectsAtRuntime = true;
            pLightEntity->GetPropertyValue(
                kPropertyID_StandardModelLibrary_LightNonPCLObjectsAtRuntime,
                bLightNonPCLObjectsAtRuntime);

            if (!bLightNonPCLObjectsAtRuntime)
                return false;
        }
    }

    // Light affects object if it is in range.
    efd::Point3 lightToObject = pkLight->m_position - pkReceiver->m_position;
    efd::Float32 rangePlusRadius = pkLight->m_radius + pkReceiver->m_radius;

    // Comparing squared distance to avoid slow square root calculation.
    return(lightToObject.SqrLength() < (rangePlusRadius * rangePlusRadius));
}

//------------------------------------------------------------------------------------------------
void LightService::SetAffectedNodeLists(bool sortAxisChanged)
{
    // Determine which receivers to consider.
    efd::vector<ReceiverData*> * pReceiversToConsider = NULL;

    if (sortAxisChanged)
    {
        SortList((efd::vector<BaseObjectData*>*)&m_lightList);
        SortList((efd::vector<BaseObjectData*>*)&m_receiverList);
        m_RecieverListDirty = false;

        pReceiversToConsider = &m_receiverList;
    }
    else if (!m_changedLightList.empty())
    {
        // Sort lights if necessary.
        SortList((efd::vector<BaseObjectData*>*)&m_lightList);

        // Lights moved, need to re-evaluate lighting for all receivers.
        pReceiversToConsider = &m_receiverList;

        // Sort receivers if necessary.
        if (!m_changedNodeList.empty())
        {
            // The entire receiver list needs to be sorted.  This sort is required since we have
            // to recompute lighting for all receivers when a light changes.
            SortList((efd::vector<BaseObjectData*>*)pReceiversToConsider);
            m_RecieverListDirty = false;
        }
    }
    else
    {
        // Only objects moved, just need to re-evaluate the moved receivers.
        pReceiversToConsider = &m_changedNodeList;

        SortList((efd::vector<BaseObjectData*>*)pReceiversToConsider);
    }

    // Only consider sorting the entire receiver list if we are about to operate on the entire
    // receiver list.  This step prevents possibly sorting both the full receiver list and the
    // change receiver list in the same tick.
    if (m_RecieverListDirty && pReceiversToConsider == &m_receiverList)
    {
        SortList((efd::vector<BaseObjectData*>*)pReceiversToConsider);
        m_RecieverListDirty = false;
    }

    // As we walk the receiver list in order, consider all lights in order.
    unsigned int uiLowerBoundLight = 0;
    unsigned int uiLightToConsider = 0;

    efd::vector<ReceiverData*>::iterator itCurReceiver = pReceiversToConsider->begin();
    efd::vector<ReceiverData*>::iterator itEndReceiver = pReceiversToConsider->end();

    // For each Receiver in the list:
    for (; itCurReceiver != itEndReceiver; ++itCurReceiver)
    {
        // Get NiNode represented by the receiver entity.
        ReceiverData* pReceiverData = *itCurReceiver;

        if (!pReceiverData)
            continue;

        egf::Entity* pReceiverEntity = m_pEntityManager->LookupEntity(pReceiverData->m_entityID);

        // Note: If you hit this assert it most likely indicates a different service
        // has issued an incorrect number of SceneGraphAddedMessage or SceneGraphRemovedMessage
        // messages. The m_receiverList should contain an entry for every entity with a scene
        // graph. If size of m_receiverList does not match the number of entities with scene
        // graphs (which should never happen) then this indicates a service incorrectly fired
        // a SceneGraphAddedMessage or SceneGraphRemovedMessage.
        if (!EE_VERIFY(pReceiverEntity))
            continue;

        // Clear the light priority queues.
        m_PointPriorityQueue.empty();
        m_SpotPriorityQueue.empty();
        m_DirPriorityQueue.empty();

        NiNode* pNode = pReceiverData->m_spNode;

        if (!pNode)
            continue;

        bool bUpdateNeeded = false;

        // Ensure all lights that are known to be out of range do not affect the receiver.
        for (efd::UInt32 ui = 0; ui < uiLowerBoundLight; ui++)
        {
            LightData* pLightData = m_lightList[ui];

            efd::hash_map<egf::EntityID, float, egf::EntityIDHashFunctor>::iterator itReciever =
                pLightData->m_entityPriorityOverride.find(pReceiverData->m_entityID);

            if (itReciever != pLightData->m_entityPriorityOverride.end() &&
                itReciever->second == MAX_LIGHT_PRIORITY)
            {
                bUpdateNeeded |= AddReceiverToLightQueue(pLightData, pReceiverData);
            }
            else if (pLightData->m_spLight)
            {
                bUpdateNeeded |= pNode->DetachEffect(pLightData->m_spLight);
            }
        }

        // Until we've reached the first receiver in range, detach light.
        while (uiLowerBoundLight < m_lightList.size() &&
            m_lightList[uiLowerBoundLight]->m_projectedCenter +
            m_lightList[uiLowerBoundLight]->m_radius <
            pReceiverData->m_projectedCenter - pReceiverData->m_radius)
        {
            LightData* pLightData = m_lightList[uiLowerBoundLight];

            efd::hash_map<egf::EntityID, float, egf::EntityIDHashFunctor>::iterator itReciever =
                pLightData->m_entityPriorityOverride.find(pReceiverData->m_entityID);

            if (itReciever != pLightData->m_entityPriorityOverride.end() &&
                itReciever->second == MAX_LIGHT_PRIORITY)
            {
                bUpdateNeeded |= AddReceiverToLightQueue(pLightData, pReceiverData);
            }
            else if (pLightData->m_spLight)
            {
                bUpdateNeeded |= pNode->DetachEffect(pLightData->m_spLight);
            }
            uiLowerBoundLight++;
        }

        // Consider lights in range.
        uiLightToConsider = uiLowerBoundLight;

        // While lights are in range of receiver, detach or attach lights that should affect the
        // receiver.
        while (uiLightToConsider < m_lightList.size() &&
            m_lightList[uiLightToConsider]->m_projectedCenter -
            m_lightList[uiLightToConsider]->m_radius <
            pReceiverData->m_projectedCenter + pReceiverData->m_radius)
        {
            LightData* pLightData = m_lightList[uiLightToConsider];
            EE_ASSERT(pLightData && pLightData->m_spLight);

            efd::hash_map<egf::EntityID, float, egf::EntityIDHashFunctor>::iterator itReciever =
                pLightData->m_entityPriorityOverride.find(pReceiverData->m_entityID);

            // Check if receiver should receive light.  If so, add to affected node list;
            // otherwise, remove.
            if ((itReciever != pLightData->m_entityPriorityOverride.end() &&
                itReciever->second == MAX_LIGHT_PRIORITY) ||
                LightAffectsReceiver(m_lightList[uiLightToConsider], pReceiverData))
            {
                bUpdateNeeded |= AddReceiverToLightQueue(pLightData, pReceiverData);
            }
            else
            {
                bUpdateNeeded |= pNode->DetachEffect(pLightData->m_spLight);
            }

            uiLightToConsider++;
        }

        // Detach from the rest of the lights.
        while (uiLightToConsider < m_lightList.size())
        {
            LightData* pLightData = m_lightList[uiLightToConsider];

            efd::hash_map<egf::EntityID, float, egf::EntityIDHashFunctor>::iterator itReciever =
                pLightData->m_entityPriorityOverride.find(pReceiverData->m_entityID);

            if (itReciever != pLightData->m_entityPriorityOverride.end() &&
                itReciever->second == MAX_LIGHT_PRIORITY)
            {
                bUpdateNeeded |= AddReceiverToLightQueue(pLightData, pReceiverData);
            }
            else if (pLightData->m_spLight)
            {
                bUpdateNeeded |= pNode->DetachEffect(pLightData->m_spLight);
            }

            uiLightToConsider++;
        }

        // Handle Point lights.
        NiUInt32 uiMaxPointLights = 0;
        pReceiverEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MaxPointLights,
            uiMaxPointLights);

        NiUInt32 uiCounter = 0;
        while (!m_PointPriorityQueue.empty())
        {
            LightPriority kLightPriority = m_PointPriorityQueue.top();

            if (uiCounter < uiMaxPointLights && kLightPriority.m_fPriority > 0.0f)
                bUpdateNeeded |= pNode->AttachEffect(kLightPriority.m_pLight);
            else
                bUpdateNeeded |= pNode->DetachEffect(kLightPriority.m_pLight);

            uiCounter++;
            m_PointPriorityQueue.pop();
        }

        // Handle Spot lights.
        NiUInt32 uiMaxSpotLights = 0;
        pReceiverEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MaxSpotLights,
            uiMaxSpotLights);

        uiCounter = 0;
        while (!m_SpotPriorityQueue.empty())
        {
            LightPriority kLightPriority = m_SpotPriorityQueue.top();

            if (uiCounter < uiMaxSpotLights && kLightPriority.m_fPriority > 0.0f)
                bUpdateNeeded |= pNode->AttachEffect(kLightPriority.m_pLight);
            else
                bUpdateNeeded |= pNode->DetachEffect(kLightPriority.m_pLight);

            uiCounter++;
            m_SpotPriorityQueue.pop();
        }

        // Handle Dir lights.
        NiUInt32 uiMaxDirLights = 0;
        pReceiverEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MaxDirectionalLights,
            uiMaxDirLights);

        uiCounter = 0;
        while (!m_DirPriorityQueue.empty())
        {
            LightPriority kLightPriority = m_DirPriorityQueue.top();

            if (uiCounter < uiMaxDirLights && kLightPriority.m_fPriority > 0.0f)
                bUpdateNeeded |= pNode->AttachEffect(kLightPriority.m_pLight);
            else
                bUpdateNeeded |= pNode->DetachEffect(kLightPriority.m_pLight);

            uiCounter++;
            m_DirPriorityQueue.pop();
        }

        if (bUpdateNeeded)
            pNode->UpdateEffects();
    }
}

//------------------------------------------------------------------------------------------------
void LightService::DumpLights()
{
    efd::utf8string msg;
    msg.sprintf("%d Lights\n", m_lightList.size());
    EE_OUTPUT_DEBUG_STRING(msg.c_str());

    for (efd::vector<LightData*>::iterator itCurrLight = m_lightList.begin();
        itCurrLight != m_lightList.end(); itCurrLight++)
    {
        (*itCurrLight)->DumpData();
    }
}

//------------------------------------------------------------------------------------------------
void LightService::DumpReceivers()
{
    efd::utf8string msg;
    msg.sprintf("%d Receivers\n", m_receiverList.size());
    EE_OUTPUT_DEBUG_STRING(msg.c_str());

    for (efd::vector<ReceiverData*>::iterator itCurrReceiver = m_receiverList.begin();
        itCurrReceiver != m_receiverList.end(); itCurrReceiver++)
    {
        (*itCurrReceiver)->DumpData();
    }
}

//------------------------------------------------------------------------------------------------
bool LightService::DetermineSortAxis()
{
    // The sort axis is the axis that objects and lights will be sorted on to determine relative
    // distances used in determining what objects lights should affect.  For simplicity, we will
    // find the cardinal axis that most of the receiving objects tend to stretch along and use it.
    // We do this by looking at the largest variance among the 3 cardinal axes.

    // For each position
    //   sum += position
    //   SumSquared += pos*pos
    // mean = sum/N objects
    // variance = (SumSquared - sum*mean)/N
    // set axis to the greatest of X Y Z

    int nObjects = (int)(m_lightList.size() + m_receiverList.size());
    efd::Point3 sortAxis = efd::Point3::UNIT_X;

    if (nObjects < 2)
        return false;

    efd::Point3 sum = efd::Point3(0,0,0);
    efd::Point3 sumSquared = efd::Point3(0,0,0);
    efd::Point3 mean = efd::Point3(0,0,0);
    efd::Point3 variance = efd::Point3(0,0,0);

    for (efd::vector<LightData*>::iterator itCurrLight = m_lightList.begin();
        itCurrLight != m_lightList.end(); itCurrLight++)
    {
        efd::Point3 pos = (*itCurrLight)->m_position;
        sum += pos;
        efd::Point3 posSquared = efd::Point3::ComponentProduct(pos, pos);
        sumSquared += posSquared;
    }

    for (efd::vector<ReceiverData*>::iterator itCurrReceiver = m_receiverList.begin();
        itCurrReceiver != m_receiverList.end(); itCurrReceiver++)
    {
        efd::Point3 pos = (*itCurrReceiver)->m_position;
        sum += pos;
        efd::Point3 posSquared = efd::Point3::ComponentProduct(pos, pos);
        sumSquared += posSquared;
    }

    const float inverseNumObjects = 1.0f / nObjects;
    mean = sum * inverseNumObjects;
    variance = (sumSquared - efd::Point3::ComponentProduct(sum, mean)) * inverseNumObjects;

    // Find which component has the largest variance.
    if (variance.y > variance.x)
    {
        if (variance.z > variance.y)
            sortAxis = efd::Point3::UNIT_Z;
        else
            sortAxis = efd::Point3::UNIT_Y;
    }
    else if (variance.z > variance.x)
    {
        sortAxis = efd::Point3::UNIT_Z;
    }

    if (sortAxis != m_sortAxis)
    {
        // Update *all* projections, since the axis changed.
        m_sortAxis = sortAxis;
        for (efd::vector<LightData*>::iterator itCurrLight = m_lightList.begin();
            itCurrLight != m_lightList.end(); itCurrLight++)
        {
            LightData* pData = *itCurrLight;
            EE_ASSERT(pData);

            if (pData)
            {
                const efd::Point3& pos = pData->m_position;
                pData->m_projectedCenter = pos * sortAxis;
            }
        }

        for (efd::vector<ReceiverData*>::iterator itCurrReceiver = m_receiverList.begin();
            itCurrReceiver != m_receiverList.end(); itCurrReceiver++)
        {
            ReceiverData* pData = *itCurrReceiver;
            EE_ASSERT(pData);

            if (pData)
            {
                const efd::Point3& pos = pData->m_position;
                pData->m_projectedCenter = pos * sortAxis;
            }
        }

        return true;
    }
    else
    {
        // The sort axis didn't change, so the projections can be selectively updated.

        // Update the projections for the moved lights.
        for (efd::vector<LightData*>::iterator itCurrLight = m_changedLightList.begin();
            itCurrLight != m_changedLightList.end(); ++itCurrLight)
        {
            LightData* pData = *itCurrLight;
            EE_ASSERT(pData);

            if (pData)
            {
                const efd::Point3& pos = pData->m_position;
                pData->m_projectedCenter = pos * sortAxis;
            }
        }

        // Update the projections for the moved nodes.
        for (efd::vector<ReceiverData*>::iterator itCurrReceiver = m_changedNodeList.begin();
            itCurrReceiver != m_changedNodeList.end(); ++itCurrReceiver)
        {
            ReceiverData* pData = *itCurrReceiver;
            EE_ASSERT(pData);

            if (pData)
            {
                const efd::Point3& pos = pData->m_position;
                pData->m_projectedCenter = pos * sortAxis;
            }
        }

        return false;
    }
}
//------------------------------------------------------------------------------------------------
void LightService::OnSurfaceAdded(RenderService* pService, RenderSurface* pSurface)
{
    NiCamera* pCamera = pSurface->GetCamera();
    EE_ASSERT(pCamera);

    EE_UNUSED_ARG(pService);
    EE_UNUSED_ARG(pCamera);

    // Create shadow render step.
    NiDefaultClickRenderStep* pShadowRenderStep = EE_NEW NiDefaultClickRenderStep;
    pShadowRenderStep->SetName("ShadowStep");
    pShadowRenderStep->SetPreProcessingCallbackFunc(ShadowRenderStepPre);

    pSurface->GetRenderFrame()->PrependRenderStep(pShadowRenderStep);
}
//------------------------------------------------------------------------------------------------
void LightService::OnRenderedEntityAdded(
    RenderService* pService, egf::Entity* pEntity, NiAVObject* pAVObject)
{
    EE_UNUSED_ARG(pService);

    if (!pEntity)
        return;

    // It's possible that the entity associated with the scene graph has already
    // been removed from the entity manager (if the entity is removed before
    // pending messages associated with it are complete).
    if (!m_pEntityManager->LookupEntity(pEntity->GetEntityID()))
        return;

    const egf::FlatModel* pModel = pEntity->GetModel();
    EE_ASSERT(pModel);

    // Only consider the entity for lighting if it contains the 'lightable' model.
    if (!pModel->ContainsModel(kFlatModelID_StandardModelLibrary_Lightable))
        return;

    egf::EntityID entityID = pEntity->GetEntityID();

    if (!pAVObject)
        return;

    NiNodePtr spNode = NiDynamicCast(NiNode, pAVObject);

    if (!spNode)
        return;

    AddReceiverData(spNode, entityID);

    // Add every light to the new entity (which is not a light).
    for (efd::map<egf::EntityID, LightData*>::iterator iter = m_entityLightMap.begin();
        iter != m_entityLightMap.end(); ++iter)
    {
        spNode->AttachEffect(iter->second->m_spLight);
        iter->second->m_spLight->UpdateEffects();
    }

    // Iterate through all the shadow generators and ensure that, depending on the entity property
    // settings, the NiNode casts shadows or not, and receives shadows or not.
    const NiTPointerList<NiShadowGeneratorPtr>& shadowGenerators =
        NiShadowManager::GetShadowGenerators();
    NiTListIterator kIter = shadowGenerators.GetHeadPos();
    while (kIter)
    {
        NiShadowGeneratorPtr spGenerator = shadowGenerators.GetNext(kIter);
        if (spGenerator)
        {
            bool isCastingShadow;
            if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsCastingShadow,
                isCastingShadow) == PropertyResult_OK)
            {
                if (!isCastingShadow)
                    spGenerator->AttachUnaffectedCasterNode(spNode);
            }

            bool isReceivingShadow;
            if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsReceivingShadow,
                isReceivingShadow) == PropertyResult_OK)
            {
                if (!isReceivingShadow)
                    spGenerator->AttachUnaffectedReceiverNode(spNode);
            }
        }
    }

    spNode->UpdateEffects();
}
//------------------------------------------------------------------------------------------------
void LightService::OnRenderedEntityRemoved(
    RenderService* pService, egf::Entity* pEntity, NiAVObject* pAVObject)
{
    EE_UNUSED_ARG(pService);

    NiNode* pNode = NiDynamicCast(NiNode, pAVObject);

    if (!pNode)
        return;

    // Detach entity from every light.
    for (efd::map<egf::EntityID, LightData*>::iterator iter = m_entityLightMap.begin();
        iter != m_entityLightMap.end(); ++iter)
    {
        iter->second->m_spLight->DetachAffectedNode(pNode);
        iter->second->m_spLight->UpdateEffects();
    }

    // Iterate through all the shadow generators and ensure that the NiNode is no longer a part of
    // the unaffected caster or unaffected receiver lists.
    const NiTPointerList<NiShadowGeneratorPtr>& shadowGenerators =
        NiShadowManager::GetShadowGenerators();
    NiTListIterator kIter = shadowGenerators.GetHeadPos();
    while (kIter)
    {
        NiShadowGeneratorPtr spGenerator = shadowGenerators.GetNext(kIter);
        if (spGenerator)
        {
            spGenerator->DetachUnaffectedCasterNode(pNode);
            spGenerator->DetachUnaffectedReceiverNode(pNode);
        }
    }

    pNode->UpdateEffects();

    RemoveReceiverData(pEntity->GetEntityID());
}

//------------------------------------------------------------------------------------------------
void LightService::ScrapeLights(NiAVObject* pkObject, efd::vector<NiLight*> &kLights)
{
	if (pkObject)
	{
		NiNode* pkNode = NiDynamicCast(NiNode, pkObject);
		NiLight* pkLight = NiDynamicCast(NiLight, pkObject);

		if (pkNode)
		{
			for (unsigned int ui = 0;
				ui < pkNode->GetArrayCount();
				++ui )
			{

				NiAVObject* pkChild = pkNode->GetAt(ui);
				ScrapeLights(pkChild, kLights); // recurse
			}
		}

		if (pkLight)
		{ 
			kLights.push_back(pkLight); // Push into the lights
		}
		
	}
}

//------------------------------------------------------------------------------------------------
