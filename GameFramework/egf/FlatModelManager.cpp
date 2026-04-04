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

#include <egf/FlatModelManager.h>
#include <efd/ConfigManager.h>
#include <efd/ServiceManager.h>
#include <efd/ILogger.h>
#include <egf/PrimitiveProperties.h>
#include <egf/UtilityProperties.h>
#include <egf/SAXModelParser.h>
#include <efd/PathUtils.h>
#include <egf/Scheduler.h>
#include <egf/EntityManager.h>
#include <efd/AssetLocatorService.h>
#include <efd/Metrics.h>
#include <egf/EntityChangeMessage.h>
#include <egf/egfLogIDs.h>
#include <efd/SmartCriticalSection.h>
#include <egf/FlatModelFactoryResponse.h>
#include <egf/ScriptFactoryResponse.h>
#include <egf/PlaceableModel.h>

using namespace egf;
using namespace efd;


EE_IMPLEMENT_CONCRETE_CLASS_INFO(FlatModelManager);

EE_HANDLER_WRAP(FlatModelManager, HandleAssetLookupMsg,
                AssetLocatorResponse, kMSGID_AssetLocatorResponse);

EE_HANDLER(FlatModelManager, HandleAssetLoadMsg, AssetLoadResponse);
EE_HANDLER_SUBCLASS(
    FlatModelManager,
    HandleAssetLoadMsg,
    AssetLoadResponse,
    FlatModelFactoryResponse);

//------------------------------------------------------------------------------------------------
FlatModelManager::FlatModelManager()
: m_metadata()
#ifdef EE_PLATFORM_XBOX360
, m_modelPath("D:")
#else
, m_modelPath(".")
#endif
, m_modelFileExt(".flat")
, m_asset_lookup_in_progress(false)
, m_asset_lookup_complete(false)
, m_callback(kCAT_INVALID)
, m_preloadLookupCategory(kCAT_INVALID)
, m_pendingPreloads()
, m_pfnEntityFactory(NULL)
, m_entityFactoryPriority(-1)
, m_pFlatModelFactory(0)
, m_loadedPrimTypes(false)
, m_loadedUtilTypes(false)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2300;

    // add each of the primitive properties to the property factory
    RegisterPrimitivePropFactories();
    RegisterUtilityPropFactories();
}

//------------------------------------------------------------------------------------------------
FlatModelManager::~FlatModelManager()
{
    // maps all contain flat model smart pointers, they'll dec the ref count
    // automatically, nothing to do here
}

//------------------------------------------------------------------------------------------------
SyncResult FlatModelManager::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_ASSERT(m_pServiceManager);

    // Because we register an IAssetFactory whenever there in an AssetFactoryManager we must depend
    // on the AssetFactoryManager service if it exists.
    pDependencyRegistrar->AddDependency<AssetFactoryManager>(sdf_Optional);

    // If there is a Scheduler, we want it to initialize the SchedulerLua script engine before we
    // are initialized so that we can connect the flat model factory to the script factory.
    pDependencyRegistrar->AddDependency<Scheduler>(sdf_Optional);

    // Best practice for message subscribers is to depend on the message service.
    pDependencyRegistrar->AddDependency<MessageService>();

    // @todo: remove the legacy method of caching all flat model locations
    //@{
    AssetFactoryManager* pAFM = m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>();
    AssetLocatorService* pALS = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    if (!pAFM && pALS)
    {
        pDependencyRegistrar->AddDependency<AssetLocatorService>();
    }
    //@}

    ReadConfigOptions();

    MessageService* pMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    RegisterMessageWrapperFactory<AssetLocatorResponse, kMSGID_AssetLocatorResponse>(
        pMessageService);

    // Register the Placeable Model factory
    RegisterBuiltinModelFactory(
        "PlaceableModel",
        kFlatModelID_StandardModelLibrary_Placeable,
        &(egf::PlaceableModel::PlaceableModelFactory));

    m_callback = GenerateAssetResponseCategory();
    m_preloadLookupCategory = GenerateAssetResponseCategory();

    pMessageService->Subscribe(this, kCAT_LocalMessage);
    pMessageService->Subscribe(this, m_callback);
    pMessageService->Subscribe(this, m_preloadLookupCategory);

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult FlatModelManager::OnInit()
{
    EE_ASSERT(m_pServiceManager);

    Scheduler* pSim = m_pServiceManager->GetSystemServiceAs<Scheduler>();

    // IF we have an AssetFactoryManager, wait for it to initialize.
    AssetFactoryManager* pAFM = m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>();
    if (pAFM)
    {
        IScriptFactory* scriptFactory = 0;

        if (pSim)
        {
            ISchedulerScripting* pLua = pSim->GetScriptingRuntime(BehaviorType_Lua);
            if (pLua)
            {
                scriptFactory = EE_DYNAMIC_CAST(
                    IScriptFactory,
                    pAFM->GetAssetFactory(pLua->GetAssetLoadCategory()));
            }
        }
        m_pFlatModelFactory = EE_NEW FlatModelFactory(
            m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>(),
            this,
            scriptFactory);

        pAFM->RegisterAssetFactory(m_callback, m_pFlatModelFactory);

        m_asset_lookup_complete = true;

        // Don't precache flat models in this configuration.
        return efd::AsyncResult_Complete;
    }

    // If the asset locator service is available, use it to populate a "directory" of flat model
    // locations for later (JIT) loading.
    AssetLocatorService* pALS = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    if (pALS)
    {
        // Send one-time request to locate all the flat model assets
        if (!m_asset_lookup_in_progress)
        {
            // Send request to fetch the location for all flat model assets
            if (pSim)
                pSim->PauseScheduler(true);

            if (pALS->AssetLocate("urn:emergent-flat-model", m_callback))
            {
                m_asset_lookup_in_progress = true;
            }
            else
            {
                m_asset_lookup_complete = true;
            }
        }

        if (!m_asset_lookup_complete)
        {
            return efd::AsyncResult_Pending;
        }
    }
    else
    {
        m_asset_lookup_complete = true;
    }

    // no asset service used
    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::ResetFlatModelRequest()
{
    AssetLocatorService* pALS = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    if (!pALS)
        return;

    // Reset the flat indicating the lookup is not complete until we get the new paths to
    // the flat files.
    m_asset_lookup_complete = false;

    // Simulation needs to be paused while the flat model location cache is updated
    Scheduler* pSim = m_pServiceManager->GetSystemServiceAs<Scheduler>();
    if (pSim)
        pSim->PauseScheduler(true);

    m_metadataLock.Lock();
    m_metadata.m_flatModelLocations.clear();
    m_metadataLock.Unlock();

    // Send request to fetch the location for all flat model assets
    pALS->AssetLocate("urn:emergent-flat-model", m_callback);
}

//------------------------------------------------------------------------------------------------
AsyncResult FlatModelManager::OnTick()
{
    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult FlatModelManager::OnShutdown()
{
    AssetFactoryManager* pAFM = m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>();
    if (pAFM)
    {
        pAFM->UnregisterAssetFactory(m_callback);
    }

    MessageService* pMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    pMessageService->Unsubscribe(this, kCAT_LocalMessage);
    pMessageService->Unsubscribe(this, m_callback);
    pMessageService->Unsubscribe(this, m_preloadLookupCategory);

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::RegisterPrimitivePropFactories()
{
    EE_ASSERT(!m_loadedPrimTypes);

    if (m_loadedPrimTypes)
        return;

    m_loadedPrimTypes = true;

#define RegisterScalarHelper(Name) this->RegisterPropertyFactory(\
    #Name, Name##ScalarProperty::CLASS_ID, Name##ScalarPropertyFactory)

    RegisterScalarHelper(Boolean);
    RegisterScalarHelper(SInt16);
    RegisterScalarHelper(UInt16);
    RegisterScalarHelper(SInt32);
    RegisterScalarHelper(UInt32);
    RegisterScalarHelper(SInt64);
    RegisterScalarHelper(UInt64);
    RegisterScalarHelper(Float32);
    RegisterScalarHelper(Float64);
    RegisterScalarHelper(Char);
    RegisterScalarHelper(String);
    RegisterScalarHelper(EntityRef);

#undef  RegisterScalarHelper


#define RegisterAssocHelper(Name) this->RegisterPropertyFactory(\
    "Assoc" #Name, Name##AssocProperty::CLASS_ID, Name##AssocPropertyFactory)

    RegisterAssocHelper(Boolean);
    RegisterAssocHelper(SInt16);
    RegisterAssocHelper(UInt16);
    RegisterAssocHelper(SInt32);
    RegisterAssocHelper(UInt32);
    RegisterAssocHelper(SInt64);
    RegisterAssocHelper(UInt64);
    RegisterAssocHelper(Float32);
    RegisterAssocHelper(Float64);
    RegisterAssocHelper(Char);
    RegisterAssocHelper(String);
    RegisterAssocHelper(EntityRef);

#undef  RegisterAssocHelper
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::RegisterUtilityPropFactories()
{
    EE_ASSERT(!m_loadedUtilTypes);

    if (m_loadedUtilTypes)
        return;

    m_loadedUtilTypes = true;

#define RegisterScalarHelper(Name) \
    RegisterPropertyFactory(#Name, Name##ScalarProperty::CLASS_ID, Name##ScalarPropertyFactory)

    RegisterScalarHelper(Color);
    RegisterScalarHelper(ColorA);
    RegisterScalarHelper(Point2);
    RegisterScalarHelper(Point3);
    RegisterScalarHelper(Matrix3);
    RegisterScalarHelper(AssetID);

#undef  RegisterScalarHelper


#define RegisterAssocHelper(Name) \
    RegisterPropertyFactory("Assoc"#Name, Name##AssocProperty::CLASS_ID, Name##AssocPropertyFactory)

    RegisterAssocHelper(Color);
    RegisterAssocHelper(ColorA);
    RegisterAssocHelper(Point2);
    RegisterAssocHelper(Point3);
    RegisterAssocHelper(Matrix3);
    RegisterAssocHelper(AssetID);

#undef  RegisterAssocHelper
}

//------------------------------------------------------------------------------------------------
FlatModelManager::LoadResult FlatModelManager::ParseModelFile(
    const efd::utf8string& modelFile,
    FlatModelPtr& o_pModel)
{
    SAXModelParser saxParser(*this);
    o_pModel = saxParser.Parse(modelFile);
    if (!o_pModel)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Error parsing the flat model file \"%s\".", modelFile.c_str()));

        return ParseError;
    }
    return OK;
}

//------------------------------------------------------------------------------------------------
efd::utf8string FlatModelManager::GetPathForModel(const efd::utf8string& modelName)
{
    // Check if the flat file location is already known (from Asset Locator)
    utf8string rval("");
    m_metadataLock.Lock();
    if (m_metadata.m_flatModelLocations.find(modelName) != m_metadata.m_flatModelLocations.end())
    {
        rval = m_metadata.m_flatModelLocations[modelName];
    }
    else
    {
        // Try a case-insensitive search, just in case the file name differs only in case.
        for (FlatModelLocationMap::const_iterator i = m_metadata.m_flatModelLocations.begin();
             i != m_metadata.m_flatModelLocations.end();
             ++i)
        {
            if (0 == modelName.icompare(i->first))
            {
                rval = i->second;
            }
        }
    }
    m_metadataLock.Unlock();

    if (rval.length())
    {
        return rval;
    }

    // Right now the Linux code sometimes needs to use the Model path even
    // when an AssetLocatorService is defined.  This is because the Linux version
    // of the AssetController code can not generate metadata.  When it can
    // this define can be removed.
#if !defined(EE_PLATFORM_LINUX)
    // If we have an Asset Locator service then we should never need to use the default
    // path to find an asset.  Log the attempt to do so and return an empty string.
    AssetLocatorServicePtr m_pAssetLocator =
        m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    if (m_pAssetLocator)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Flat model file path unknown for model: %s", modelName.c_str()));
        return "";
    }
#endif

    // Use setting loaded from config manager to determine file location
    utf8string strFullPath;
    strFullPath = PathUtils::PathCombine(GetModelPath(), modelName);
    strFullPath = PathUtils::PathAddExtension(strFullPath, GetModelFileExt());
    return strFullPath;
}

//------------------------------------------------------------------------------------------------
FlatModelManager::LoadResult FlatModelManager::ParseModel(
    const efd::utf8string& modelName,
    FlatModelPtr& o_pModel)
{
    efd::utf8string modelPath = GetPathForModel(modelName);
    if (modelPath.length() == 0)
    {
        return ModelFileNotFound;
    }
    return ParseModelFile(modelPath, o_pModel);
}

//------------------------------------------------------------------------------------------------
FlatModelManager::LoadResult FlatModelManager::LoadModelFile(const efd::utf8string& modelFile)
{
    FlatModelPtr newModel;
    FlatModelManager::LoadResult result = ParseModelFile(modelFile, newModel);

    if (OK == result)
    {
        AddModel(newModel);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
FlatModelManager::LoadResult FlatModelManager::LoadModel(const efd::utf8string& modelName)
{
    if (FindModel(modelName))
    {
        return AlreadyLoaded;
    }

    // Note: It is assumed at this point that if there is an AssetLocatorService, that all
    // flatmodel locations have been retrieved which would occur on full initiation of the
    // flatmodelmanager.
    FlatModelPtr pNewModel;
    FlatModelManager::LoadResult result = ParseModel(modelName, pNewModel);

    if (OK == result)
    {
        AddModel(pNewModel);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::AddModel(FlatModelPtr model)
{
    bool result = true;

    EE_ASSERT(model);

    m_metadataLock.Lock();

    // If the model has been added by name, post error and return.
    if (m_metadata.m_flatModelsByName.find(model->GetName()) != m_metadata.m_flatModelsByName.end())
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Error adding model '%s' (%d): A model named '%s' already exists",
            model->GetName().c_str(), model->GetID(), model->GetName().c_str()));

        m_metadataLock.Unlock();
        return false;
    }

    // If the model has been added by ID, post error and return.
    if (m_metadata.m_flatModelsByID.find(model->GetID()) != m_metadata.m_flatModelsByID.end())
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Error adding model '%s' (%d): A model with id '%d' already exists",
            model->GetName().c_str(), model->GetID(), model->GetID()));

        m_metadataLock.Unlock();
        return false;
    }

    // enumerate the properties in this model and keep a mapping of property name to ID.
    for (FlatModel::PropertyDescriptorMapByID::iterator iter =
        model->m_propertyDescriptorsByID.begin();
        iter != model->m_propertyDescriptorsByID.end();
        ++iter)
    {
        PropertyDescriptor* pDesc = iter->second;

        PropNameToIdMap::iterator iterN2I = m_metadata.m_mapPropNameToId.find(pDesc->GetName());
        if (iterN2I == m_metadata.m_mapPropNameToId.end())
        {
            m_metadata.m_mapPropNameToId[ pDesc->GetName() ] = pDesc->GetPropertyID();
        }
        else if (iterN2I->second != pDesc->GetPropertyID())
        {
            // property with same name but different value added
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("Error adding model '%s' (%d): "
                "Property '%s' (%d) conflicts with property '%s' (%d)",
                model->GetName().c_str(), model->GetID(),
                pDesc->GetName().c_str(), pDesc->GetPropertyID(),
                iterN2I->first.c_str(), iterN2I->second));
            result = false;
        }

        PropIdToNameMap::iterator iterI2N =
            m_metadata.m_mapPropIdToName.find(pDesc->GetPropertyID());
        if (iterI2N == m_metadata.m_mapPropIdToName.end())
        {
            m_metadata.m_mapPropIdToName[ pDesc->GetPropertyID() ] = pDesc->GetName();
        }
        else if (iterI2N->second != pDesc->GetName())
        {
            // property with same ID but different name added
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("Error adding model '%s' (%d): "
                "Property '%s' (%d) conflicts with property '%s' (%d)",
                model->GetName().c_str(), model->GetID(),
                pDesc->GetName().c_str(), pDesc->GetPropertyID(),
                iterI2N->second.c_str(), iterI2N->first));
            result = false;
        }
    }

    // enumerate the behaviors in this model and keep a mapping of behavior name to ID.
    for (FlatModel::BehaviorDescriptorMapByID::iterator iter =
        model->m_BehaviorDescriptorsByID.begin();
        iter != model->m_BehaviorDescriptorsByID.end();
        ++iter)
    {
        BehaviorDescriptor* pDesc = iter->second;

        RegisterBehaviorName(pDesc->GetName(), pDesc->GetID());
    }

    if (result)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("Adding model '%s' (0x%08X)",
            model->GetName().c_str(), model->GetID()));

        m_metadata.m_flatModelsByID[model->GetID()] = model;
        m_metadata.m_flatModelsByName[model->GetName()] = model;
    }

    m_metadataLock.Unlock();
    return result;
}

//------------------------------------------------------------------------------------------------
const FlatModel* FlatModelManager::FindModel(FlatModelID i_id) const
{
    m_metadataLock.Lock();
    FlatModel* rval = 0;
    FlatModelMapByID::const_iterator iter = m_metadata.m_flatModelsByID.find(i_id);
    if (iter != m_metadata.m_flatModelsByID.end())
    {
        rval = iter->second;
    }
    m_metadataLock.Unlock();
    return rval;
}

//------------------------------------------------------------------------------------------------
const FlatModel* FlatModelManager::FindModel(const efd::utf8string& i_name) const
{
    m_metadataLock.Lock();
    FlatModel* rval = 0;
    FlatModelMapByName::const_iterator iter = m_metadata.m_flatModelsByName.find(i_name);
    if (iter != m_metadata.m_flatModelsByName.end())
    {
        rval = iter->second;
    }
    m_metadataLock.Unlock();
    return rval;
}

//------------------------------------------------------------------------------------------------
const FlatModel* FlatModelManager::FindOrLoadModel(const efd::utf8string& i_name)
{
    LoadModel(i_name);
    return FindModel(i_name);
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& FlatModelManager::GetModelNameByID(
    FlatModelID i_id,
    bool i_bCheckMixins /*= true*/) const
{
    const FlatModel* pModel = FindModel(i_id);
    if (pModel)
    {
        return pModel->GetName();
    }

    if (i_bCheckMixins)
    {
        SmartCriticalSection cs(m_metadataLock);
        cs.Lock();
        MixinNameByIdMap::const_iterator iter = m_metadata.m_mixinNameById.find(i_id);
        if (iter != m_metadata.m_mixinNameById.end())
        {
            return iter->second;
        }
    }

    return utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
FlatModelID FlatModelManager::GetModelIDByName(
    const efd::utf8string& i_name,
    bool i_bCheckMixins /*= true*/) const
{
    FlatModelID rval = 0;;
    const FlatModel* pModel = FindModel(i_name);
    if (pModel)
    {
        rval = pModel->GetID();
    }

    if (!rval && i_bCheckMixins)
    {
        m_metadataLock.Lock();
        MixinIDByNameMap::const_iterator iter = m_metadata.m_mixinIdByName.find(i_name);
        if (iter != m_metadata.m_mixinIdByName.end())
        {
            rval = iter->second;
        }
        m_metadataLock.Unlock();
    }

    return rval;
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::RegisterMixin(const efd::utf8string& i_name, FlatModelID i_id)
{
    bool result = true;

    m_metadataLock.Lock();

    MixinNameByIdMap::iterator iterId = m_metadata.m_mixinNameById.find(i_id);
    if (iterId != m_metadata.m_mixinNameById.end())
    {
        if (iterId->second != i_name)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("Error adding mixin data '%s' (%d): "
                "A mixin with id '%d' is already assigned to model '%s'!",
                i_name.c_str(), i_id, iterId->first, iterId->second.c_str()));
            result = false;
        }
    }
    MixinIDByNameMap::iterator iterName = m_metadata.m_mixinIdByName.find(i_name);
    if (iterName != m_metadata.m_mixinIdByName.end())
    {
        if (iterName->second != i_id)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("Error adding mixin data '%s' (%d): "
                "A mixin with model '%s' is already assigned to id '%d'",
                i_name.c_str(), i_id, iterName->first.c_str(), iterName->second));
            result = false;
        }
    }

    if (result)
    {
        m_metadata.m_mixinNameById[i_id] = i_name;
        m_metadata.m_mixinIdByName[i_name] = i_id;
    }

    m_metadataLock.Unlock();

    return result;
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::ReadConfigOptions()
{
    const efd::utf8string kConfigSectionName = "ModelData";
    const efd::utf8string kModelPath = "ModelDir";
    const efd::utf8string kModelFileExt = "ModelFileExtension";

    // get the config manager from the framework.
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    if (!pConfigManager)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Could not find config manager while attempting to read model options. "
            "Default values will be used by FlatModelManager."));
        return;
    }

    // find the root section in the config manager
    const ISection* pRoot = pConfigManager->GetConfiguration();
    if (!pRoot)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Could not find root config section while attempting to read model options. "
            "Default values will be used by FlatModelManager."));
        return;
    }

    // look for the model section
    const ISection* pModelSection = pRoot->FindSection(kConfigSectionName);
    if (!pModelSection)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("Could not find model section, '%s', in the config manager while attempting to load "
            "model options.  Default values will be used by FlatModelManager.",
            kConfigSectionName.c_str()));
        return;
    }

    if (!pModelSection->FindValue(kModelPath, m_modelPath))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("Could not find value for key '%s' in section '%s' of the config manager while "
            "attempting to load model options.  Default value of '%s' will be used",
            kModelPath.c_str(), kConfigSectionName.c_str(), m_modelPath.c_str()));
    }

    // It is not expected that FileExt will normally be set, so don't log if not found
    pModelSection->FindValue(kModelFileExt, m_modelFileExt);
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& FlatModelManager::GetModelPath() const
{
    return m_modelPath;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& FlatModelManager::GetModelFileExt() const
{
    return m_modelFileExt;
}

//------------------------------------------------------------------------------------------------
IProperty* FlatModelManager::FactoryProperty(efd::ClassID classID)
{
    return m_propertyFactory.CreateObject(classID);
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::RegisterPropertyFactory(
    const efd::utf8string& name,
    efd::ClassID classID,
    PropertyFactory::FactoryMethod pfn)
{
    m_metadataLock.Lock();
    m_metadata.m_mapPropTypeNameToId[name] = classID;
    m_metadata.m_mapPropTypeIdToName[classID] = name;
    m_metadataLock.Unlock();

    m_propertyFactory.RegisterFactoryMethod(classID, pfn);
    return true;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& FlatModelManager::GetTypeNameByID(efd::ClassID i_id) const
{
    SmartCriticalSection cs(m_metadataLock);
    cs.Lock();
    PropTypeIdToNameMap::const_iterator iter = m_metadata.m_mapPropTypeIdToName.find(i_id);
    if (iter != m_metadata.m_mapPropTypeIdToName.end())
    {
        return iter->second;
    }
    return utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
efd::ClassID FlatModelManager::GetTypeIDByName(const efd::utf8string& i_name) const
{
    ClassID rval = 0;
    m_metadataLock.Lock();
    PropTypeNameToIdMap::const_iterator iter = m_metadata.m_mapPropTypeNameToId.find(i_name);
    if (iter != m_metadata.m_mapPropTypeNameToId.end())
    {
        rval = iter->second;
    }
    m_metadataLock.Unlock();
    return rval;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& FlatModelManager::GetPropertyNameByID(PropertyID i_id) const
{
    SmartCriticalSection cs(m_metadataLock);
    cs.Lock();
    PropIdToNameMap::const_iterator iter = m_metadata.m_mapPropIdToName.find(i_id);
    if (iter != m_metadata.m_mapPropIdToName.end())
    {
        return iter->second;
    }
    return utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::GetPropertyNameByID(PropertyID i_id, efd::utf8string& o_name) const
{
    SmartCriticalSection cs(m_metadataLock);
    cs.Lock();
    PropIdToNameMap::const_iterator iter = m_metadata.m_mapPropIdToName.find(i_id);
    if (iter != m_metadata.m_mapPropIdToName.end())
    {
        o_name = iter->second;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
PropertyID FlatModelManager::GetPropertyIDByName(const efd::utf8string& i_name) const
{
    PropertyID rval = 0;

    m_metadataLock.Lock();
    PropNameToIdMap::const_iterator iter = m_metadata.m_mapPropNameToId.find(i_name);
    if (iter != m_metadata.m_mapPropNameToId.end())
    {
        rval = iter->second;
    }
    m_metadataLock.Unlock();
    return rval;
}

//------------------------------------------------------------------------------------------------
IBuiltinModel* FlatModelManager::FactoryBuiltinModel(FlatModelID classID)
{
    return m_builtinModelFactory.CreateObject(classID);
}


//------------------------------------------------------------------------------------------------
IBuiltinModel* FlatModelManager::FactoryBuiltinModel(const efd::utf8string& i_strComponentName)
{
    FlatModelID comp = GetBuiltinModelIDByName(i_strComponentName);
    return FactoryBuiltinModel(comp);
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::RegisterBuiltinModelFactory(
    const efd::utf8string& i_strName,
    FlatModelID i_componentID,
    BuiltinModelFactory::FactoryMethod pfn,
    efd::SInt32 i_priority /*=0*/)
{
    m_metadataLock.Lock();
    m_metadata.m_mapCompIdToName[i_componentID] = i_strName;
    m_metadata.m_mapCompNameToId[i_strName] = i_componentID;
    m_metadataLock.Unlock();

    m_builtinModelFactory.RegisterFactoryMethod(i_componentID, pfn);

    EE_UNUSED_ARG(i_priority);

    return true;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& FlatModelManager::GetBuiltinModelNameByID(FlatModelID i_id) const
{
    SmartCriticalSection cs(m_metadataLock);
    cs.Lock();
    CompIdToNameMap::const_iterator iter = m_metadata.m_mapCompIdToName.find(i_id);
    if (iter != m_metadata.m_mapCompIdToName.end())
    {
        return iter->second;
    }
    return utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
FlatModelID FlatModelManager::GetBuiltinModelIDByName(const efd::utf8string& i_name) const
{
    FlatModelID rval = 0;

    m_metadataLock.Lock();
    CompNameToIdMap::const_iterator iter = m_metadata.m_mapCompNameToId.find(i_name);
    if (iter != m_metadata.m_mapCompNameToId.end())
    {
        rval = iter->second;
    }
    m_metadataLock.Unlock();

    return rval;
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::RegisterBehaviorName(
    const efd::utf8string& i_strName,
    BehaviorID i_behaviorID)
{
    bool rval = true;
    m_metadataLock.Lock();

    BehIdToNameMap::const_iterator iter = m_metadata.m_mapBehIdToName.find(i_behaviorID);
    if (iter != m_metadata.m_mapBehIdToName.end())
    {
        EE_ASSERT_MESSAGE(i_strName == iter->second,
            ("Error: Attempt to register behavior name with a different ID."));
        if (i_strName != iter->second)
            rval = false;
    }
    else
    {
        m_metadata.m_mapBehIdToName[i_behaviorID] = i_strName;
        m_metadata.m_mapBehNameToId[i_strName] = i_behaviorID;
    }

    m_metadataLock.Unlock();
    return rval;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& FlatModelManager::GetBehaviorNameByID(BehaviorID i_id) const
{
    SmartCriticalSection cs(m_metadataLock);
    cs.Lock();
    BehIdToNameMap::const_iterator iter = m_metadata.m_mapBehIdToName.find(i_id);
    if (iter != m_metadata.m_mapBehIdToName.end())
    {
        return iter->second;
    }
    return utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
BehaviorID FlatModelManager::GetBehaviorIDByName(const efd::utf8string& i_name) const
{
    BehaviorID rval = 0;

    m_metadataLock.Lock();
    BehNameToIdMap::const_iterator iter = m_metadata.m_mapBehNameToId.find(i_name);
    if (iter != m_metadata.m_mapBehNameToId.end())
    {
        rval = iter->second;
    }
    m_metadataLock.Unlock();
    return rval;
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::CleanModelMaps()
{
    m_metadataLock.Lock();
    m_metadata.m_flatModelsByID.clear();
    m_metadata.m_flatModelsByName.clear();
    m_metadata.m_mixinNameById.clear();
    m_metadata.m_mixinIdByName.clear();
    m_metadata.m_mapPropNameToId.clear();
    m_metadata.m_mapPropIdToName.clear();
    m_metadata.m_mapCompNameToId.clear();
    m_metadata.m_mapCompIdToName.clear();
    m_metadata.m_mapBehNameToId.clear();
    m_metadata.m_mapBehIdToName.clear();
    m_metadataLock.Unlock();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 FindEntitiesByModel(
    ServiceManager* pServiceManager,
    const efd::utf8string& i_modelName,
    efd::list<EntityPtr>& o_results)
{
    o_results.clear();

    EntityManager* pEntityManager = pServiceManager->GetSystemServiceAs<EntityManager>();
    EntityManager::EntityMap::const_iterator iter = pEntityManager->GetFirstEntityPos();
    Entity* pEntity;
    while (pEntityManager->GetNextEntity(iter, pEntity))
    {
        if (pEntity->GetModelName() == i_modelName)
        {
            o_results.push_back(pEntity);
        }
    }

    return (UInt32)o_results.size();
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::ReloadAllModels(bool fromToolbench)
{
    bool bResult = true;

    m_metadataLock.Lock();

    UInt32 flatModelCount = m_metadata.m_flatModelsByID.size();
    FlatModel** originalModels = EE_EXTERNAL_NEW FlatModel*[flatModelCount];

    efd::UInt32 i = 0;
    for (FlatModelMapByID::const_iterator iter = m_metadata.m_flatModelsByID.begin();
        iter != m_metadata.m_flatModelsByID.end();
        iter++)
    {
        originalModels[i++] =
            const_cast<FlatModel*>(static_cast<const FlatModel*>(iter->second));
    }
    m_metadataLock.Unlock();

    for (efd::UInt32 i2 = 0; i2 < flatModelCount; i2++)
    {
        bResult &= ReloadModel(originalModels[i2], fromToolbench);
    }

    EE_EXTERNAL_DELETE originalModels;

    return bResult;
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::ReloadModel(const efd::utf8string& modelName)
{
    // First find the existing model.  We do nothing if this model isn't already loaded.
    FlatModel* pOriginalModel = const_cast<FlatModel*>(FindModel(modelName));
    if (!pOriginalModel)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("Model isn't loaded '%s'; Canceling reload",
            modelName.c_str()));

        // Model isn't loaded, nothing to do!
        return true;
    }

    // assumption: Calls to ReloadModel come from RapidIteration, not Toolbench.
    return FlatModelManager::ReloadModel(pOriginalModel, false);
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::ReloadModel(FlatModel* pOriginalModel, bool fromToolbench)
{
    EE_ASSERT(pOriginalModel);

    EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
        ("Reloading model '%s' (%d)",
        pOriginalModel->GetName().c_str(), pOriginalModel->GetID()));

    bool bResult = true;

    // If we have an AssetFactoryManager, use it to reload the flat model file in the background.
    if (!fromToolbench &&
        m_pServiceManager->CheckSystemServiceState(AssetFactoryManager::CLASS_ID) !=
            ServiceManager::kSysServState_Invalid)
    {
        MessageService* pMsgSvc = m_pServiceManager->GetSystemServiceAs<MessageService>();
        EE_ASSERT(pMsgSvc);

        utf8string urn(Formatted, "urn:emergent-flat-model:%s", pOriginalModel->GetName().c_str());

        FlatModelFactoryRequest* pRequest = EE_NEW FlatModelFactoryRequest(
            urn,
            m_callback);
        pRequest->SetIsReload(true);

        pMsgSvc->SendLocal(pRequest, AssetFactoryManager::MessageCategory());
    }

    // Otherwise load the file directly.
    else
    {
        // Next we reload the new model
        FlatModelPtr spNewModel;
        ParseModel(pOriginalModel->GetName(), spNewModel);

        bResult = ProcessReload(pOriginalModel, spNewModel);
    }


    return bResult;
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::RemoveModel(const FlatModel* pModel)
{
    EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
        ("Removing model '%s' (%d)",
        pModel->GetName().c_str(), pModel->GetID()));

    m_metadataLock.Lock();
    size_t IdRemoved = m_metadata.m_flatModelsByID.erase(pModel->GetID());
    size_t NameRemoved = m_metadata.m_flatModelsByName.erase(pModel->GetName());
    m_metadataLock.Unlock();

    return IdRemoved && NameRemoved;
}


//------------------------------------------------------------------------------------------------
void FlatModelManager::DeleteEntities(efd::list<egf::EntityPtr>& i_entities)
{
    // This is called when something bad happened, so these entities must be flushed from
    // the entity manager.
    EntityManager* pRef = m_pServiceManager->GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pRef);

    for (efd::list<egf::EntityPtr>::iterator iter = i_entities.begin();
          iter != i_entities.end();
          ++iter)
    {
        egf::EntityPtr& spEntity = *iter;

        // We already removed from the scheduler all of those entities, so now we need to repeat
        // that process for the replicated entities.
        Entity* pLocalEntity = pRef->LookupEntity(spEntity->GetEntityID());
        if (pLocalEntity)
        {
            EE_ASSERT(pLocalEntity == spEntity);

            // This is called under error conditions, so we need to simply remove this entity
            // rather than calling Scheduler::DestroyEntity.  This means the OnDestroy behaviors
            // for these entities will not be called, but since they are in an error state
            // that is required.
            pRef->RemoveEntity(pLocalEntity);
        }
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::AddEntities(efd::list<egf::EntityPtr>& i_entities)
{
    // This is called when something bad happened, so these entities must be flushed from
    // the entity manager.
    EntityManager* pRef = m_pServiceManager->GetSystemServiceAs<EntityManager>();
    EE_ASSERT(pRef);

    for (efd::list<egf::EntityPtr>::iterator iter = i_entities.begin();
        iter != i_entities.end();
        ++iter)
    {
        egf::EntityPtr& spEntity = *iter;

        pRef->AddEntity(spEntity);
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::ReinitializeEntities(efd::list<egf::EntityPtr>& i_entities)
{
    for (efd::list<egf::EntityPtr>::iterator iter = i_entities.begin();
        iter != i_entities.end();
        ++iter)
    {
        egf::EntityPtr& spEntity = *iter;
        spEntity->OnReinitialized();
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::UpdateEntities(
    const efd::list<egf::EntityPtr>& i_entities,
    const FlatModel* pOldModel,
    const FlatModel* pNewModel)
{
    // no entities to update
    if (!i_entities.size())
    {
        return;
    }

    efd::set<PropertyDescriptorPtr> unchangedProperties;
    efd::set<PropertyDescriptorPtr> newProperties;
    efd::set<PropertyDescriptorPtr> updatedProperties;
    efd::set<PropertyDescriptorPtr> deletedProperties;

    pOldModel->DiffProperties(
        pNewModel,
        unchangedProperties,
        newProperties,
        updatedProperties,
        deletedProperties);

    efd::set<BuiltinModelDescriptorPtr> addedBuiltins;
    efd::set<BuiltinModelDescriptorPtr> removedBuiltins;

    pOldModel->DiffBuiltinModels(pNewModel, addedBuiltins, removedBuiltins);

    for (efd::list<egf::EntityPtr>::const_iterator iter = i_entities.begin();
        iter != i_entities.end();
        ++iter)
    {
        const egf::EntityPtr& spEntity = *iter;

        // Reset the properties on entities that are in the properties to remove list.  This will
        // remove them from the override map and free their memory.
        for (efd::set<PropertyDescriptorPtr>::const_iterator iterProp = deletedProperties.begin();
            iterProp != deletedProperties.end();
            ++iterProp)
        {
            // We cannot simply call ResetProperty because that will not remove built-in model
            // properties from the override map
            spEntity->SetDirty((*iterProp)->GetPropertyID(), NULL);
            spEntity->RemoveProperty((*iterProp)->GetPropertyID());
        }

        for (efd::set<PropertyDescriptorPtr>::const_iterator iterProp = updatedProperties.begin();
            iterProp != updatedProperties.end();
            ++iterProp)
        {
            const PropertyDescriptor* newDesc = *iterProp;
            PropertyID changedPropID = newDesc->GetPropertyID();
            spEntity->RapidIterateDefaultPropertyValue(changedPropID, newDesc);
        }

        // Set any new properties
        for (efd::set<PropertyDescriptorPtr>::const_iterator iterProp = newProperties.begin();
            iterProp != newProperties.end(); ++iterProp)
        {
            FlatModel* m = const_cast<FlatModel *>(spEntity->GetModel());
            m->AddPropertyDescriptor((*iterProp));
            spEntity->SetDirty((*iterProp)->GetPropertyID(), (*iterProp)->GetDefaultProperty());
        }

        // DT32396 improve rapid iteration of built-in models
        EE_UNUSED_ARG(removedBuiltins);
        EE_UNUSED_ARG(addedBuiltins);
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::HandleAssetLookupMsg(
    const efd::AssetLocatorResponse* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(m_pServiceManager->GetSystemServiceAs<AssetLocatorService>());

    AssetLocatorService::AssetLocationMapPtr pLocations =
        AssetLocatorService::AssetLocationsFromResponseMsg(pMessage);
    EE_ASSERT(pLocations);

    if (targetChannel == m_preloadLookupCategory)
    {
        utf8string uri = pMessage->GetResponseURI();

        // find the pending preload for this response
        PendingPreloadMap::iterator pendingIt = m_pendingPreloads.find(uri);

        UInt32 uiResponse = pLocations->Count();
        if (uiResponse == 0)
        {
            EE_LOG(efd::kAssets, efd::ILogger::kERR1,
                ("FlatModelManager received no assets that matched URI '%s'", uri.c_str()));
        }

        // Background load each of the located assets
        for (UInt32 x=0; x<uiResponse; x++)
        {
            utf8string modelName = pLocations->GetName(x);
            utf8string url = pLocations->GetURL(x);
            utf8string native_url = PathUtils::PathMakeNative (url);

            utf8string urn(Formatted, "urn:emergent-flat-model:%s", modelName.c_str());

            // Forward AssetLoadResponse messages when we get a load response to the callback.
            if (pendingIt != m_pendingPreloads.end())
            {
                m_pendingPreloads[urn] = pendingIt->second;
            }

            FlatModelFactoryRequestPtr pRequest = EE_NEW FlatModelFactoryRequest(
                urn,
                m_callback,
                native_url);

            MessageService* pMsgSrv = m_pServiceManager->GetSystemServiceAs<MessageService>();
            EE_ASSERT(pMsgSrv);

            pMsgSrv->SendLocal(pRequest, AssetFactoryManager::MessageCategory());
        }
        m_pendingPreloads.erase(uri);
    }
    else
    {
        UInt32 uiResponse = pLocations->Count();
        if (uiResponse == 0)
        {
            utf8string uri = pMessage->GetResponseURI();
            EE_LOG(efd::kAssets, efd::ILogger::kERR1,
                ("FlatModelManager received no assets that matched URI '%s'", uri.c_str()));
        }

        // Store all the locations in a map so they can be used to JIT load the model files
        for (UInt32 x=0; x<uiResponse; x++)
        {
            utf8string modelName = pLocations->GetName(x);
            utf8string url = pLocations->GetURL(x);
            utf8string native_url = PathUtils::PathMakeNative (url);

            m_metadataLock.Lock();
            if (m_metadata.m_flatModelLocations.find(modelName) !=
                    m_metadata.m_flatModelLocations.end())
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                    ("Located multiple flat models called '%s'. Only one will be used.",
                    modelName.c_str()));
            }
            m_metadata.m_flatModelLocations[modelName] = native_url;
            m_metadataLock.Unlock();
        }

        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("FlatModelManager: %u flat models cached",
            pLocations->Count()));

        Scheduler* pSim = m_pServiceManager->GetSystemServiceAs<Scheduler>();
        if (pSim)
            pSim->PauseScheduler(false);
        m_asset_lookup_complete = true;

    }
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::UpdateModelLocation (const utf8string& model, utf8string& path)
{
    m_metadataLock.Lock();
    m_metadata.m_flatModelLocations[model] = path;
    m_metadataLock.Unlock();
}

//------------------------------------------------------------------------------------------------
EntityPtr FlatModelManager::FactoryEntity(
    const efd::utf8string& i_strModelName,
    EntityID i_eid,
    bool i_master)
{
    const FlatModel* pModel = FindOrLoadModel(i_strModelName);
    if (pModel)
    {
        return FactoryEntity(pModel, i_eid, i_master);
    }

    EE_LOG(efd::kEntity, efd::ILogger::kERR1,
        ("Error: Tried to create an entity with an invalid Model name: '%s'",
        i_strModelName.c_str()));
    return NULL;
}

//------------------------------------------------------------------------------------------------
EntityPtr FlatModelManager::FactoryEntity(FlatModelID i_modelID, EntityID i_eid, bool i_master)
{
    const FlatModel* pModel = FindModel(i_modelID);
    if (pModel)
    {
        return FactoryEntity(pModel, i_eid, i_master);
    }

    EE_LOG(efd::kEntity, efd::ILogger::kERR1,
        ("Error: Tried to create an entity with an invalid Model ID: %d", i_modelID));
    return NULL;
}

//------------------------------------------------------------------------------------------------
EntityPtr FlatModelManager::FactoryEntity(
    const FlatModel* i_pModel,
    EntityID i_eid,
    bool i_master)
{
    SmartCriticalSection cs(m_factoryLock);
    cs.Lock();

    EE_ASSERT(m_pfnEntityFactory);
    if (m_pfnEntityFactory)
    {
        EntityPtr pResult = (*m_pfnEntityFactory)(i_pModel, i_eid, i_master);
        if (pResult->CreateBuiltinModels(this))
        {
            EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
                ("Entity %s of type '%s' created successfully",
                pResult->GetEntityID().ToString().c_str(), i_pModel->GetName().c_str()));

            return pResult;
        }
        else
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("Error: Could not create built-in models for new '%s' entity (%s).",
                i_pModel->GetName().c_str(), pResult->GetEntityID().ToString().c_str()));
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::RegisterEntityFactory(EntityFactoryMethod i_pfn, efd::SInt32 i_priority)
{
    SmartCriticalSection cs(m_factoryLock);
    cs.Lock();

    if (i_priority > m_entityFactoryPriority)
    {
        m_entityFactoryPriority = i_priority;
        m_pfnEntityFactory = i_pfn;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
FlatModelFactory* FlatModelManager::GetFlatModelFactory() const
{
    return m_pFlatModelFactory;
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::PreloadModel(
    const efd::utf8string& modelName,
    const efd::Category& callback)
{
    EE_ASSERT(modelName.size());

    utf8string urn(Formatted, "urn:emergent-flat-model:%s", modelName.c_str());

    if (callback.IsValid())
    {
        m_pendingPreloads[urn].insert(callback);
    }

    MessageService* pMsgSvc = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(pMsgSvc);

    FlatModelFactoryRequest* pRequest = EE_NEW FlatModelFactoryRequest(urn, m_callback);
    pMsgSvc->SendLocal(pRequest, AssetFactoryManager::MessageCategory());
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::PreloadModels(
    const efd::utf8string& urn,
    const efd::Category& callback)
{
    EE_ASSERT(urn.size());

    if (callback.IsValid())
    {
        m_pendingPreloads[urn].insert(callback);
    }

    // locate the requested files.
    AssetLocatorService* pALS = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    EE_ASSERT(pALS);

    pALS->AssetLocate(urn, m_preloadLookupCategory);
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::HandleAssetLoadMsg(
    const efd::AssetLoadResponse* pMsg,
    efd::Category)
{
    const FlatModelFactoryResponse* pResponse = EE_DYNAMIC_CAST(FlatModelFactoryResponse, pMsg);

    // If the cast failed, this is a 'asset not found' response from the AssetFactoryManager.
    if (!pResponse)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("Unable to load flat model file. Asset not found. URN: %s", pMsg->GetURN().c_str()));

        if (m_pendingPreloads.find(pMsg->GetURN()) != m_pendingPreloads.end())
        {
            ForwardPreloadResponse(pMsg);
        }
        return;
    }

    ISchedulerScripting* pLua = 0;
    Scheduler *pSim = m_pServiceManager->GetSystemServiceAs<Scheduler>();
    if (pSim)
    {
        pLua = pSim->GetScriptingRuntime(BehaviorType_Lua);
    }

    typedef FlatModelFactoryResponse::DependentScriptSet ScriptSet;

    if (pMsg->GetIsReload())
    {
        const FlatModelFactoryResponse::FlatModelMap& flatModels = pResponse->GetFlatModelMap();
        for (FlatModelFactoryResponse::FlatModelMap::const_iterator it = flatModels.begin();
            it != flatModels.end();
            ++it)
        {
            FlatModel* pNewModel = it->second;
            FlatModel* pOriginalModel = const_cast<FlatModel*>(FindModel(pNewModel->GetName()));

            if (pOriginalModel)
            {
                ProcessReload(pOriginalModel, pNewModel);
            }
            else
            {
                AddModel(pNewModel);
            }

            if (pLua)
            {
                // forward script reloads to SchedulerLua.
                const ScriptSet& scripts = pResponse->GetDependentScripts();
                for (ScriptSet::const_iterator sIt = scripts.begin(); sIt != scripts.end(); ++sIt)
                {
                    pLua->HandleAssetLoadResponse(*sIt, kCAT_INVALID);
                }
            }
        }
    }
    else
    {
        if (m_pendingPreloads.find(pMsg->GetURN()) != m_pendingPreloads.end())
        {
            ForwardPreloadResponse(pMsg);
        }
    }

    const FlatModelFactoryResponse::FlatModelMap& models = pResponse->GetFlatModelMap();
    for (FlatModelFactoryResponse::FlatModelMap::const_iterator fmIt = models.begin();
        fmIt != models.end();
        ++fmIt)
    {
        if (!FindModel(fmIt->first))
        {
            AddModel(fmIt->second);

            if (pLua)
            {
                const ScriptSet& scripts = pResponse->GetDependentScripts();
                for (ScriptSet::const_iterator sIt = scripts.begin(); sIt != scripts.end(); ++sIt)
                {
                    pLua->HandleAssetLoadResponse(*sIt, kCAT_INVALID);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelManager::ForwardPreloadResponse(const efd::AssetLoadResponse* pMsg)
{
    set<Category> callbacks;
    if (m_pendingPreloads.find(pMsg->GetURN(), callbacks))
    {
        MessageService* pMsgSvc = m_pServiceManager->GetSystemServiceAs<MessageService>();
        EE_ASSERT(pMsgSvc);

        for (set<Category>::const_iterator it = callbacks.begin(); it != callbacks.end(); ++it)
        {
            pMsgSvc->SendLocal(pMsg, *it);
        }
    }
    m_pendingPreloads.erase(pMsg->GetURN());
}

//------------------------------------------------------------------------------------------------
bool FlatModelManager::ProcessReload(FlatModel* pOriginalModel, FlatModel* pNewModel)
{
    EE_ASSERT(pOriginalModel);

    bool bResult = true;

    // pause the scheduler while we reload.
    Scheduler* pSim = m_pServiceManager->GetSystemServiceAs<Scheduler>();
    if (pSim)
    {
        pSim->PauseScheduler(true);
    }

    // We need to find all entities that are using the model that's about to change
    efd::list<egf::EntityPtr> spEntityList;
    FindEntitiesByModel(m_pServiceManager, pOriginalModel->GetName(), spEntityList);

    if (!pNewModel)
    {
        if (!spEntityList.empty())
        {
            // We failed to reload the new model or the model was deleted.  We'd better get
            // rid of all entities for which we no longer have a valid flat model.
            DeleteEntities(spEntityList);
        }

        // Remove the old flat model so no one can use it, without forcing a new reload.
        RemoveModel(pOriginalModel);
    }
    else
    {
        // We need to compare the old and the new models.  There are six cases we care about:
        // 1.) the models mixin composition (i.e. base model types) have changed
        //      - in this case, if the simulator exists we DO NOT update the entities because this
        //        has too many side effects. Instead, the entities using model are deleted from the
        //        simulation.  However, if the scheduler does not exist we remove the entities,
        //        update them, and re-add them so that they are properly added back to the services
        //        that should be watching them.
        // 2.) properties from the old model that are removed in the new model
        //      - in this case we must remove this property from the entities
        // 3.) Properties that exist in both models but have changed type
        //      - in this case we try to convert the old value into a string and then into
        //        the new value.  This allows things like changing a UInt into an SInt to
        //        mostly work.
        // 4.) Properties that have changed default values.
        //      - for each entity with this property that is using the default, we update to the
        //        new value.
        // 5.) built-in models from the old model that are removed in the new model
        // 6.) built-in models in the new model that are not in the old model
        // These are all calculated and handled in UpdateEntities.

        // Determine whether mix-in composition is changed
        set<utf8string> addedMix, deletedMix;
        pOriginalModel->DiffMixins(pNewModel, addedMix, deletedMix);

        FlatModelPtr spOriginalModel = pOriginalModel;
        if (!addedMix.empty() || !deletedMix.empty())
        {
            // For these type changes we need to delete the entities, update them, then re-insert
            // them into the simulator.
            DeleteEntities(spEntityList);

            // If no scheduler is running it is ok to update and reinsert the entities into the
            // manager.
            if (pSim == NULL)
            {
                // Update any existing entities to reflect changes to their model.
                UpdateEntities(spEntityList, pOriginalModel, pNewModel);

                // Remove the original model, adapt it to the new model and add the adapted version.
                RemoveModel(spOriginalModel);
                spOriginalModel->Mutate(pNewModel);
                AddModel(spOriginalModel);

                ReinitializeEntities(spEntityList);

                // Add the entities back to the entity manager using the newly adapted model.
                AddEntities(spEntityList);

                EE_LOG(efd::kEntity, efd::ILogger::kERR3,
                    ("Entities of model %s have been removed and readded to the entity manager "
                    "because there was a mixin change in their model.",
                    pOriginalModel->GetName().c_str()));
            }
            else
            {
                // Remove the original model, add the new model.  Since we deleted all the entities
                // that use the original model there is no reason to in-place mutate it.
                RemoveModel(spOriginalModel);
                AddModel(pNewModel);

                EE_LOG(efd::kEntity, efd::ILogger::kERR3,
                    ("Deleting all entities of model %s due to model mixin changes. "
                    "Reset sim to restore.",
                    pOriginalModel->GetName().c_str()));
            }
        }
        else
        {
            // Update any existing entities to reflect changes to their model.
            UpdateEntities(spEntityList, spOriginalModel, pNewModel);
            // We need to call AddModel to update the FlatModelManager state as to what property
            // names and behavior names exist, but we can't add something that's already been
            // added so we remove it first and then re-add it.
            RemoveModel(spOriginalModel);
            spOriginalModel->Mutate(pNewModel);
            AddModel(spOriginalModel);
        }
    }

    spEntityList.clear();

    // When we're done, restart the scheduler.
    if (pSim)
    {
        pSim->PauseScheduler(false);
    }

    return bResult;
}
