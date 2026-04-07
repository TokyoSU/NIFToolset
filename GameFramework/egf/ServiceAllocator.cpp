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
#include "egfPCH.h"

#include <efdLogServer/LogService.h>
#include <egf/EntityManager.h>
#include <egf/EntityLoaderService.h>
#include <egf/FlatModelManager.h>
#include <egf/Scheduler.h>
#include <egf/NotificationService.h>
#include <egf/RapidIterationService.h>
#include <egf/ServiceAllocator.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
Bool egf::CreateGameServices(ServiceManager* pServiceManager, UInt32 flags)
{
    if (!(flags & gsaf_USE_CUSTOM_ENTITY_MANAGER))
    {
        // The EntityManager stores entities created by other services. It provides lookup
        // capabilities other services can use to retrieve entities by entity ID, and forwards
        // messages to entities as needed.
        EntityManagerPtr spEntityManager = EE_NEW EntityManager();
        pServiceManager->RegisterSystemService(spEntityManager);
    }

    // The Scheduler is responsible for running behaviors on entities.  It can schedule behaviors
    // to run in the future and ensures that behaviors run in the proper order.
    SchedulerPtr spScheduler = EE_NEW Scheduler();
    pServiceManager->RegisterSystemService(spScheduler);

    // The FlatModelManager knows how to load our data driven entity definitions, which are
    // called FlatModels.  It is also responsible for the creation of Entity instances.  All
    // Entity instances are run-time instantiations of a single specific flat model.
    FlatModelManagerPtr spFlatModelManager = EE_NEW FlatModelManager();
    pServiceManager->RegisterSystemService(spFlatModelManager);

    // The notification service is responsible for monitoring changes to entities and notifying
    // other entities and services about those changes.  It can send notifications by message, or
    // entity behavior invocation.
    NotificationServicePtr spNotify = EE_NEW NotificationService();
    EE_ASSERT(spNotify);
    pServiceManager->RegisterSystemService(spNotify);

    // The EntityLoaderService is the asset loader for World files.  A World file is simply
    // a collection of Entity declarations which can optionally have some of their properties
    // overridden from the default value.  You can also unload a World file in oder to remove
    // all the entities that were created as a result of loading the block file.
    EntityLoaderServicePtr spEntityLoaderService = EE_NEW EntityLoaderService();
    pServiceManager->RegisterSystemService(spEntityLoaderService);

#if !defined(EE_CONFIG_SHIPPING)
    // The RapidIterationSystemService knows how to connect to ToolBench which enables various
    // rapid iteration changes to be communicated in real time.  Things like changes to flat
    // model files and block files can be pushed into a running game.  It also allows tools
    // like the development debugger to connect to this game.  Obviously this service is only
    // needed for development versions of your project and should be removed from shipping
    // versions.
    if (!(flags & gsaf_NO_RAPID_ITERATION))
    {
        RapidIterationSystemServicePtr spRapidIterationService =
            EE_NEW RapidIterationSystemService();
        pServiceManager->RegisterSystemService(spRapidIterationService);
    }
#endif

    return true;
}
