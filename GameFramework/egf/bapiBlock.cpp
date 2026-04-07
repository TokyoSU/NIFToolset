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

#include "egfPCH.h"

#include <egf/bapiBlock.h>
#include <egf/EntityLoaderService.h>
#include <egf/ScriptContext.h>

using namespace efd;
using namespace egf;



//------------------------------------------------------------------------------------------------
bool BehaviorAPI::LoadBlockFile(
    char* blockURN,
    char* callback,
    efd::UInt32 context,
    bool autoEnterWorld)
{
    EntityLoaderService* pELS = g_bapiContext.GetSystemServiceAs<EntityLoaderService>();

    BlockLoadParameters blp;
    blp.SetAutoEnterWorld(autoEnterWorld);

    if (callback && *callback)
    {
        Entity* pEntity = g_bapiContext.GetScriptEntity();
        const egf::BehaviorDescriptor* pbd = pEntity->GetModel()->GetBehaviorDescriptor(callback);
        if (!pbd)
        {
            return false;
        }
        blp.SetBehaviorCallback(pEntity->GetEntityID(), pbd->GetID(), context);
    }

    AsyncResult ar = pELS->RequestEntitySetLoad(blockURN, &blp);
    return (ar != AsyncResult_Failure);
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::LoadBlockFile(char* blockURN, ParameterList* params)
{
    EntityLoaderService* pELS = g_bapiContext.GetSystemServiceAs<EntityLoaderService>();

    BlockIdentification bid(blockURN);
    BlockLoadParameters blp;
    if (params)
    {
        efd::UInt32 instance;
        if (pr_OK == params->GetParameter("Instance", instance))
        {
            bid.m_instance = instance;
        }

        bool autoEnter;
        if (pr_OK == params->GetParameter("AutoEnter", autoEnter))
        {
            blp.SetAutoEnterWorld(autoEnter);
        }

        utf8string callback;
        if (pr_OK == params->GetParameter("Callback", callback))
        {
            Entity* pEntity = g_bapiContext.GetScriptEntity();
            const egf::BehaviorDescriptor* pbd =
                pEntity->GetModel()->GetBehaviorDescriptor(callback);
            if (!pbd)
            {
                return false;
            }
            UInt32 context = 0;
            params->GetParameter("Context", context);

            blp.SetBehaviorCallback(pEntity->GetEntityID(), pbd->GetID(), context);
        }

        UInt32 activeCallbacks;
        if (pr_OK == params->GetParameter("ActiveCallbacks", activeCallbacks))
        {
            blp.SetActiveCallbacks(activeCallbacks);
        }

        Point3 rotation;
        if (pr_OK == params->GetParameter("Rotation", rotation))
        {
            blp.SetBlockRotation(rotation);
        }

        Point3 offset;
        if (pr_OK == params->GetParameter("Offset", offset))
        {
            blp.SetBlockOffset(offset);
        }

        UInt32 loadThreshold;
        if (pr_OK == params->GetParameter("LoadThreashold", loadThreshold))
        {
            blp.SetLoadThresholdOverride(loadThreshold);
        }

        UInt32 unloadThreshold;
        if (pr_OK == params->GetParameter("UnloadThreashold", unloadThreshold))
        {
            blp.SetUnloadThresholdOverride(unloadThreshold);
        }
    }

    AsyncResult ar = pELS->RequestEntitySetLoad(bid, &blp);
    return (ar != AsyncResult_Failure);
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::UnloadBlockFile(char* blockURN, char* callback, efd::UInt32 context)
{
    EntityLoaderService* pELS = g_bapiContext.GetSystemServiceAs<EntityLoaderService>();
    BehaviorID bid = 0;
    Category cat(0ULL);

    if (callback && *callback)
    {
        Entity* pEntity = g_bapiContext.GetScriptEntity();
        const egf::BehaviorDescriptor* pbd = pEntity->GetModel()->GetBehaviorDescriptor(callback);
        if (!pbd)
        {
            return false;
        }
        bid = pbd->GetID();
        cat = pEntity->GetEntityID();
    }

    AsyncResult ar = pELS->RequestEntitySetUnload(blockURN, cat, context, bid);
    return (ar != AsyncResult_Failure);
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::UnloadBlockFile(char* blockURN, ParameterList* params)
{
    EntityLoaderService* pELS = g_bapiContext.GetSystemServiceAs<EntityLoaderService>();

    BlockIdentification bid(blockURN);
    Category cat(0ULL);
    BehaviorID behavior = 0;
    UInt32 context = 0;

    if (params)
    {
        efd::UInt32 instance;
        if (pr_OK == params->GetParameter("Instance", instance))
        {
            bid.m_instance = instance;
        }

        utf8string callback;
        if (pr_OK == params->GetParameter("Callback", callback))
        {
            Entity* pEntity = g_bapiContext.GetScriptEntity();
            const egf::BehaviorDescriptor* pbd =
                pEntity->GetModel()->GetBehaviorDescriptor(callback);
            if (!pbd)
            {
                return false;
            }
            params->GetParameter("Context", context);

            cat = pEntity->GetEntityID();
            behavior = pbd->GetID();
        }
    }

    AsyncResult ar = pELS->RequestEntitySetUnload(bid, cat, context, behavior);
    return (ar != AsyncResult_Failure);
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::EntitySetEnterWorld(char* blockURN, efd::UInt32 instance)
{
    EntityLoaderService* pELS = g_bapiContext.GetSystemServiceAs<EntityLoaderService>();
    if (!pELS)
    {
        return false;
    }
    BlockIdentification bid(blockURN, instance);
    pELS->RequestEntitySetEnterWorld(bid);
    return true;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::EntitySetExitWorld(char* blockURN, efd::UInt32 instance)
{
    EntityLoaderService* pELS = g_bapiContext.GetSystemServiceAs<EntityLoaderService>();
    if (!pELS)
    {
        return false;
    }
    BlockIdentification bid(blockURN, instance);
    pELS->RequestEntitySetExitWorld(bid);
    return true;
}

