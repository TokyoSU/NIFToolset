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

#include <efd/utf8string.h>
#include <efd/StdContainers.h>
#include <efd/efdLogIDs.h>

#include <egf/ScriptContext.h>
#include <egf/EntityManager.h>
#include <egf/StandardModelLibraryPropertyIDs.h>

#include "bapiCoreRuntime.h"
#include "AttachNifData.h"
#include "SceneGraphService.h"
#include "PickService.h"


using namespace efd;
using namespace egf;
using namespace ecr;

// This collection of functions is used to expose some of the Gamebryo Renderer
// functionality to the behavior implementations that are written one of the
// supported scripting languages (e.g. Lua).

//--------------------------------------------------------------------------------------------------
// Attachment built-ins
//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::AttachEntity(
    egf::EntityID i_entityToAttachID,
    egf::EntityID i_entityTargetID,
    const efd::utf8string& i_nodeToAttachAt,
    bool i_inheritScale)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    SceneGraphService* pSceneGraphService = g_bapiContext.GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(pSceneGraphService);

    // FInd the Placeable to attach
    Entity* pEntityToAttach = pEntityManager->LookupEntity(i_entityToAttachID);
    if (!pEntityToAttach)
    {
        return false;
    }
    PlaceableModel* pPlaceable = (PlaceableModel*)pEntityToAttach->FindBuiltinModel(
        kFlatModelID_StandardModelLibrary_Placeable);
    if (!pPlaceable)
    {
        return false;
    }

    Entity* pEntityTarget = pEntityManager->LookupEntity(i_entityTargetID);
    if (!pEntityTarget)
    {
        return false;
    }

    // Find the entity and its scene graph
    NiAVObject* pAVObject = pSceneGraphService->GetSceneGraphFromEntity(pEntityTarget);
    if (!pAVObject)
        return false;

    NiAVObject* pNode = pAVObject->GetObjectByName(i_nodeToAttachAt.c_str());
    if (!pNode)
        return false;

    IPropertyCallback* pCallbackToIgnore = 0;
    if (pEntityToAttach->FindBuiltinModel(kFlatModelID_StandardModelLibrary_Actor))
    {
        pCallbackToIgnore = (IPropertyCallback*)g_bapiContext.GetServiceManager()->
            GetSystemServiceByName("AnimationService");
    }

    return pSceneGraphService->EnablePlaceableFeedback(
        pPlaceable,
        pEntityTarget,
        pNode,
        pCallbackToIgnore,
        !i_inheritScale);
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::DetachEntity(
    egf::EntityID i_entityToDetachID,
    egf::EntityID i_entityTargetID,
    const efd::utf8string& i_nodeToDetachFrom)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    SceneGraphService* pSceneGraphService = g_bapiContext.GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(pSceneGraphService);

    // FInd the Placeable to attach
    Entity* pEntityToDetach = pEntityManager->LookupEntity(i_entityToDetachID);
    if (!pEntityToDetach)
    {
        return false;
    }

    PlaceableModel* pPlaceable = (PlaceableModel*)pEntityToDetach->FindBuiltinModel(
        kFlatModelID_StandardModelLibrary_Placeable);
    if (!pPlaceable)
    {
        return false;
    }

    Entity* pEntityTarget = pEntityManager->LookupEntity(i_entityTargetID);
    if (!pEntityTarget)
    {
        return false;
    }

    // Find the entity and its scene graph
    NiAVObject* pAVObject = pSceneGraphService->GetSceneGraphFromEntity(pEntityTarget);
    if (!pAVObject)
        return false;

    NiAVObject* pNode = pAVObject->GetObjectByName(i_nodeToDetachFrom.c_str());
    if (!pNode)
        return false;

    return pSceneGraphService->DisablePlaceableFeedback(pPlaceable, pEntityTarget, pNode);
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::AttachSceneGraph(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey,
    const efd::utf8string& assetURN)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData) != PropertyResult_OK)
    {
        return false;
    }

    // Modify the property data
    AssetID assetID(assetURN);
    attachData.SetNifAsset(assetID);
    if (pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData) != PropertyResult_OK)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::DetachSceneGraph(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return false;
    }

    // Modify the property data
    AssetID assetID; // Empty asset ID.
    attachData.SetNifAsset(assetID);

    if (pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
efd::utf8string bapiCoreRuntime::GetAttachedSceneGraph(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    efd::utf8string result;

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return result;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return result;
    }

    // Modify the property data
    result = attachData.GetNifAsset().GetURN();

    return result;
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::SetAttachmentOffset(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey,
    const efd::Point3& offset)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return false;
    }

    // Modify the property data
    attachData.SetTranslation(offset);

    if (pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData) != PropertyResult_OK)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::SetAttachmentRotation(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey,
    const efd::Point3& rotation)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return false;
    }

    // Modify the property data
    attachData.SetRotation(rotation);

    if (pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData) != PropertyResult_OK)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::SetAttachmentScale(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey,
    efd::Float32 scale)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return false;
    }

    // Modify the property data
    attachData.SetScale(scale);

    if (pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData) != PropertyResult_OK)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::GetAttachmentOffset(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey,
    efd::Point3* o_offset)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return false;
    }

    // Modify the property data
    *o_offset = attachData.GetTranslation();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::GetAttachmentRotation(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey,
    efd::Point3* o_rotation)
{
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return false;
    }

    // Modify the property data
    *o_rotation = attachData.GetRotation();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::GetAttachmentScale(
    egf::EntityID entityID,
    const efd::utf8string& attachmentKey,
    efd::Float32* o_scale)
{
    // Start out parameters with known values
    *o_scale = 0.0f;

    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Get the property data
    AttachNifData attachData;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_AttachedObjects,
        attachmentKey, attachData)
        != PropertyResult_OK)
    {
        return false;
    }

    // Modify the property data
    *o_scale = attachData.GetScale();

    return true;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Scene graph node queries
//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::GetSceneGraphNodeTransform(
    egf::EntityID entityID,
    const char* nodeName,
    efd::Point3* o_position,
    efd::Point3* o_rotation,
    efd::Float32* o_scale)
{
    // Start out parameters with known values (Point3 has a constructor so it starts initialized)
    *o_scale = 0.0f;

    SceneGraphService* pSceneGraphService = g_bapiContext.GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(pSceneGraphService);

    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
    {
        return false;
    }

    // Find the entity and its scene graph
    NiAVObject* pAVObject = pSceneGraphService->GetSceneGraphFromEntity(pEntity);
    if (!pAVObject)
        return false;

    NiAVObject* pNode = pAVObject->GetObjectByName(nodeName);
    if (!pNode)
        return false;

    *o_position = pNode->GetWorldTranslate();
    NiMatrix3 rotateMatrix = pNode->GetWorldRotate();
    rotateMatrix.ToEulerAnglesXYZ(o_rotation->x, o_rotation->y, o_rotation->z);
    *o_scale = pNode->GetWorldScale();

    return true;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Picking functions
//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::ClosestObjectOnRay(
    const efd::Point3& origin,
    const efd::Point3& direction,
    egf::EntityID* o_entityID,
    efd::Float32* o_distance)
{
    // Start out parameters with known values
    *o_entityID = egf::kENTITY_INVALID;
    *o_distance = 0.0f;

    PickService* pPickService = g_bapiContext.GetSystemServiceAs<PickService>();
    EE_ASSERT(pPickService);

    SceneGraphService* pSceneGraphService = g_bapiContext.GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(pSceneGraphService);

    efd::Float32 lengthSq = direction.SqrLength();
    if (lengthSq < 1.0e-8f)
        return false;

    PickService::PickRecord* pPickRecord =
        pPickService->PerformPick(origin, direction, true, true, true);

    if (!pPickRecord)
        return false;

    const NiPick::Results* pRecords = pPickRecord->GetPickResult();
    EE_ASSERT(pRecords->GetSize() > 0);
    NiPick::Record* pClosestRecord = pRecords->GetAt(0);

    *o_distance = pClosestRecord->GetDistance();

    NiAVObject* pEntityObject = pClosestRecord->GetAVObject();
    // Get *root* of the scene graph so SceneGraphService can find it!
    while (pEntityObject->GetParent())
        pEntityObject = pEntityObject->GetParent();
    Entity* pEntity = pSceneGraphService->GetEntityFromSceneGraph(pEntityObject);
    if (pEntity)
        *o_entityID = pEntity->GetEntityID();
    else
        *o_entityID = egf::kENTITY_INVALID;

    EE_DELETE pPickRecord;

    return *o_entityID != egf::kENTITY_INVALID;
}

//--------------------------------------------------------------------------------------------------
bool bapiCoreRuntime::DistanceToEntityAlongRay(
    const efd::Point3& origin,
    const efd::Point3& direction,
    egf::EntityID entityID,
    efd::Float32* o_distance)
{
    // Start out parameters with known values
    *o_distance = 0.0f;

    efd::Float32 lengthSq = direction.SqrLength();
    if (lengthSq < 1.0e-8f)
        return false;

    SceneGraphService* pSceneGraphService = g_bapiContext.GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(pSceneGraphService);

    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pEntityManager);

    Entity* pEntity = pEntityManager->LookupEntity(entityID);
    if (!pEntity)
        return false;

    // Find the portion of the scene considered ground
    NiAVObject* pObject = pSceneGraphService->GetSceneGraphFromEntity(pEntity);
    if (!pObject)
        return false;

    PickService* pPickService = g_bapiContext.GetSystemServiceAs<PickService>();
    EE_ASSERT(pPickService);

    PickService::PickRecord* pPickRecord =
        pPickService->PerformPick(origin, direction, pObject, true, true, true);

    if (!pPickRecord)
        return false;

    const NiPick::Results* pRecords = pPickRecord->GetPickResult();
    EE_ASSERT(pRecords->GetSize() > 0);
    NiPick::Record* pClosestRecord = pRecords->GetAt(0);
    *o_distance = pClosestRecord->GetDistance();

    EE_DELETE pPickRecord;

    return true;
}

//--------------------------------------------------------------------------------------------------

