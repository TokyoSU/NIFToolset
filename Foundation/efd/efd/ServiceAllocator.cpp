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
#include "efdPCH.h"

#include <efd/ConfigManager.h>
#include <efd/efdLogIDs.h>
#include <efd/EnumManager.h>
#include <efd/MessageService.h>
#include <efd/AssetLocatorService.h>
#include <efd/AssetFactoryManager.h>
#include <efd/Logger.h>
#include <efd/ServiceAllocator.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
bool efd::CreateFoundationServices(
    efd::ServiceManager* pServiceManager,
    int argcInit,
    char** argvInit,
    efd::UInt32 flags)
{
    // The ConfigManager parses options from both the command line (on platforms that have a
    // command line) and from the specified initialization file.  Unlike normal services, the
    // ConfigManager does all of its initialization work during its constructor instead of waiting
    // until OnPreInit.  This ensures that all configuration settings are ready for use by other
    // services during their OnPreInit and/or OnInit methods.
    IConfigManagerPtr spConfigManager = EE_NEW ConfigManager(
        "Config.ini",
        argcInit,
        argvInit,
        false);
    pServiceManager->RegisterSystemService(spConfigManager);

    if (!(flags & fsaf_NO_LOGGING))
    {
        // The EnumManager is used to load data files that can map common integers into a human
        // readable string name.  While the use of this service is optional, we use it in numerous
        // places such as making log file output more easily read.  It is recommended that this
        // service be used at least when log files are enabled.
        EnumManagerPtr spEnumManager = EE_NEW EnumManager();
        pServiceManager->RegisterSystemService(spEnumManager);

        // The Logger::ReadConfig method can use enumerations to support string-based names for
        // configuring logging.  To support this we must configure the EnumManager prior to
        // configuring the logger.
        spEnumManager->LoadConfigOptions(spConfigManager);

        ILogger* pLogger = efd::LoggerSingleton::Instance()->GetLogger();
        if (pLogger)
        {
            // To support reading log destinations from the config file we need to call ReadConfig.
            pLogger->ReadConfig(spConfigManager, spEnumManager);
        }
    }

    // The MessageService is required in order to send and receive messages.  Its uses a publish-
    // subscribe model based on Category.  Other services like the Scheduler rely heavily on
    // message passing in order to allow flexibility while avoiding dependencies.
    MessageServicePtr spMessageService = EE_NEW MessageService();
    pServiceManager->RegisterSystemService(spMessageService);

    // The AssetLocatorService provides a way to load assets from an arbitrary directory hierarchy
    // without having to know and hard code the path to the asset.  An asset is any sort of data
    // such as script files, flat models, block files, textures, animations, and 3d models.
    // We use a tagging system in order to allow efficient loading, for example you might tag
    // all the assets used in a given level with "level-1" and then you can pre-fetch all the
    // required data with a single query for the "level-1" tag.  Some data that is tagged with
    // "level-1" might also be tagged with another level too if it is used in multiple places.
    AssetLocatorServicePtr spAssetLocatorService = EE_NEW AssetLocatorService(pServiceManager);
    pServiceManager->RegisterSystemService(spAssetLocatorService);

    // The AssetFactoryManager handles asset load requests from various system services. These
    // requests are processed asynchronously using a background thread. Once an asset is loaded,
    // a response is forwarded to the requesting service.
    efd::AssetFactoryManagerPtr spAFM = EE_NEW efd::AssetFactoryManager();
    pServiceManager->RegisterSystemService(spAFM);

    return true;
}
