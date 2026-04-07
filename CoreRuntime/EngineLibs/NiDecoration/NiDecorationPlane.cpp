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
#include "NiDecorationPlane.h"
#include "NiDecorationFactories.h"
#include "NiDecorationBillBoardGenerator.h"

#include <efd/ecrLogIDs.h>
#include <efd/ParseHelper.h>

NiImplementRTTI(NiDecorationPlane, NiNode);

NiFixedString NiDecorationPlane::ms_kChildFieldName;

//------------------------------------------------------------------------------------------------
NiDecorationPlane::NiDecorationPlane() :
    m_kLastLocalTransform(),
    m_kDimension(NiPoint2(10.0f, 10.0f)),
    m_pFunctorTarget(NULL),
    m_pCamera(NULL),
    m_bRequiresRecreation(false)
{
    m_kLastLocalTransform.MakeIdentity();
}

//------------------------------------------------------------------------------------------------
NiDecorationPlane::~NiDecorationPlane()
{
    // Make sure all the arrays are cleared
    m_kLayers.clear();
    m_pCamera = NULL;
    m_kFieldMap.RemoveAll();
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::_SDMInit()
{
    ms_kChildFieldName = "DecorationField";
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::_SDMShutdown()
{
    ms_kChildFieldName = NULL;
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{
    NiNode::UpdateDownwardPass(kUpdate);

    DoUpdate(kUpdate.GetTime());
    EnforceValidBound();
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{
    NiNode::UpdateSelectedDownwardPass(kUpdate);

    DoUpdate(kUpdate.GetTime());
    EnforceValidBound();
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    NiNode::UpdateRigidDownwardPass(kUpdate);

    DoUpdate(kUpdate.GetTime());
    EnforceValidBound();
}

//------------------------------------------------------------------------------------------------
NiDecorationField* NiDecorationPlane::CreateDecorationFieldAt(NiUInt32 uiKey)
{
    NiDecorationField* pkField = CreateField();
    EE_ASSERT(pkField != NULL);
    if (pkField == NULL)
        return NULL;

    // Set the position of the field
    NiInt16 sIndexX, sIndexY;
    GenerateFieldIndex(uiKey, sIndexX, sIndexY);
    NiPoint3 kTranslate;
    kTranslate.x = (float)sIndexX * GetDimension().x;
    kTranslate.y = (float)sIndexY * GetDimension().y;
    kTranslate.z = 0.0f;
    pkField->SetTranslate(kTranslate);
    pkField->SetFieldIndex(uiKey);

    // Add the field to the field map, making sure no field already exists
    // in this spot
    EE_ASSERT(!GetDecorationFieldAt(uiKey));
    m_kFieldMap.SetAt(uiKey, pkField);

    return pkField;
}

//------------------------------------------------------------------------------------------------
NiDecorationField* NiDecorationPlane::GetDecorationFieldAt(NiUInt32 uiKey)
{
    NiDecorationField* pkField = NULL;
    if (m_kFieldMap.GetAt(uiKey, pkField))
        return pkField;

    return NULL;
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::GetDecorationFieldKeys(NiTPrimitiveSet<FieldID>& kKeys)
{
    NiTMapIterator kFieldIter = m_kFieldMap.GetFirstPos();
    while (kFieldIter)
    {
        FieldID uiKey;
        NiDecorationField* pkValue;
        m_kFieldMap.GetNext(kFieldIter, uiKey, pkValue);

        kKeys.Add(uiKey);
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::RemoveDecorationFieldAt(NiUInt32 uiKey)
{
    NiDecorationField* pkField = GetDecorationFieldAt(uiKey);

    EE_ASSERT(pkField);
    DetachChild(pkField);
    m_kFieldMap.RemoveAt(uiKey);
}

//------------------------------------------------------------------------------------------------
NiDecorationField* NiDecorationPlane::CreateField()
{
    NiDecorationField* pkField = NiNew NiDecorationField(GetDimension());
    if (!pkField)
        return NULL;

    pkField->SetName(ms_kChildFieldName);

    bool bFieldRequiresInitialization  = false;

    // Add create and add appropriate layers
    for (LayerInfoMap::iterator iter = m_kLayers.begin();
        iter != m_kLayers.end();
        iter++)
    {
        NiDecorationLayerInfo& kLayerInfo = iter->second;
        bFieldRequiresInitialization |= kLayerInfo.m_bRequiresInitialization;

        // Create the layer object
        const efd::utf8string& kLayerName = iter->first;
        CreateLayerForField(kLayerName, pkField);
    }

    AttachChild(pkField);

    return pkField;
}

//------------------------------------------------------------------------------------------------
NiDecorationLayer* NiDecorationPlane::CreateLayerForField(const efd::utf8string& kLayerName, 
    NiDecorationField* pkField)
{
    NiDecorationLayerInfo& kLayerInfo = GetLayerInfo(kLayerName);

    // Make sure a generator has been created for this layer
    NiDecorationGenerator* pkGenerator = kLayerInfo.m_spGenerator;

    // Create the decoration layer
    NiDecorationLayer* pkLayer = NiNew NiDecorationLayer(pkGenerator);
    pkLayer->SetCamera(GetCamera());
    pkLayer->SetMaxCellsGeneratedPerFrame(kLayerInfo.m_uiMaxCellsGenerated);

    kLayerInfo.m_bRequiresInitialization = true;

    pkField->AddLayer(kLayerName, pkLayer);

    AttachFunctorsToLayer(kLayerName, pkLayer);

    pkLayer->Update(0.0f);

    return pkLayer;
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::AttachFunctorsToLayer(const efd::utf8string& kLayerName,
    NiDecorationLayer* pkLayer)
{
    // layer info object
    NiDecorationLayerInfo& kInfo = m_kLayers[kLayerName];

    if (pkLayer)
    {
        // We have been given a specific layer instance
        pkLayer->SetFunctorList(kInfo.m_kFunctors);
    }
    else
    {
        // Give the list to all instances of this layer
        NiTMapIterator kFieldIter = m_kFieldMap.GetFirstPos();
        while (kFieldIter)
        {
            NiUInt32 uiKey;
            NiDecorationField* pkField;
            m_kFieldMap.GetNext(kFieldIter, uiKey, pkField);

            pkLayer = pkField->GetLayerAt(kLayerName);
            if (pkLayer)
                pkLayer->SetFunctorList(kInfo.m_kFunctors);
        }
    }

    // Layers should be reset after their functor list changes
    kInfo.m_bRequiresReset = true;
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::ReconfigureFunctors(const efd::utf8string& kLayerName)
{
    // Set the settings
    NiDecorationLayerInfo& kInfo = m_kLayers[kLayerName];

    for (NiDecorationLayerInfo::FunctorList::iterator iter = kInfo.m_kFunctors.begin();
        iter != kInfo.m_kFunctors.end();
        iter++)
    {
        NiDecorationFunctorBasePtr spFunctor = *iter;
        EE_ASSERT(spFunctor);

        // Set the functor parameters
        NiTPrimitiveArray<NiExtraData*>& kExtraData = spFunctor->GetExtraData();
        for (NiUInt32 ui = 0; ui < kExtraData.GetSize(); ++ui)
        {
            NiExtraData* pkData = kExtraData.GetAt(ui);

            // Key the setting key and value
            SettingsMap::iterator pos = kInfo.m_functorSettings.find(
                efd::utf8string(pkData->GetName()));

            if (pos == kInfo.m_functorSettings.end())
                continue;
            const efd::utf8string& kData = pos->second;

            // Convert and set the data value
            if (NiIsKindOf(NiIntegerExtraData, pkData))
            {
                int parsedData;
                if (efd::ParseHelper<int>::FromString(kData, parsedData))
                {
                    ((NiIntegerExtraData*)pkData)->SetValue(parsedData);
                }
                else
                {
                    EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1, ("NiDecoration: "
                        "Functor Setting (%s) - Failed to convert %s to an int.",
                        (const char*)pkData->GetName(), kData.c_str()));
                }
            }
            else if (NiIsKindOf(NiFloatExtraData, pkData))
            {
                float parsedData;
                if (efd::ParseHelper<float>::FromString(kData, parsedData))
                {
                    ((NiFloatExtraData*)pkData)->SetValue(parsedData);
                }
                else
                {
                    EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1, ("NiDecoration: "
                        "Functor Setting (%s) - Failed to convert %s to a float.",
                        (const char*)pkData->GetName(), kData.c_str()));
                }
            }
            else if (NiIsKindOf(NiBooleanExtraData, pkData))
            {
                bool parsedData;
                if (efd::ParseHelper<bool>::FromString(kData, parsedData))
                {
                    ((NiBooleanExtraData*)pkData)->SetValue(parsedData);
                }
                else
                {
                    EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1, ("NiDecoration: "
                        "Functor Setting (%s) - Failed to convert %s to a bool.",
                        (const char*)pkData->GetName(), kData.c_str()));
                }
            }
            else if (NiIsKindOf(NiStringExtraData, pkData))
            {
                ((NiStringExtraData*)pkData)->SetValue(kData.c_str());
            }
        }
    }


    // Give the list to all instances of this layer
    NiTMapIterator kFieldIter = m_kFieldMap.GetFirstPos();
    while (kFieldIter)
    {
        NiUInt32 uiKey;
        NiDecorationField* pkField;
        m_kFieldMap.GetNext(kFieldIter, uiKey, pkField);

        NiDecorationLayer* pkLayer = pkField->GetLayerAt(kLayerName);
        if (pkLayer)
        {
            pkLayer->ApplyFunctorsToMesh();
        }
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::AddLayer(const efd::utf8string& kLayerName)
{
    NIASSERT(m_kLayers.find(kLayerName) == m_kLayers.end());
    
    // Create the layer info
    m_kLayers[kLayerName];

    // We need to re-init this layer
    m_kLayers[kLayerName].m_bRequiresInitialization = true;
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::RemoveLayer(const efd::utf8string& kLayerName)
{
    // Create the layer
    m_kLayers.erase(kLayerName);

    // Create layer objects for all the fields
    NiTMapIterator kFieldIter = m_kFieldMap.GetFirstPos();
    while (kFieldIter)
    {
        FieldID uiKey;
        NiDecorationField* pkValue;
        m_kFieldMap.GetNext(kFieldIter, uiKey, pkValue);

        pkValue->RemoveLayer(kLayerName);
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::DoUpdate(float fTime)
{    
    EE_UNUSED_ARG(fTime);

    // Has our transformation changed?
    if (GetLocalTransform() != m_kLastLocalTransform)
    {
        // Yes; we need to reset all the cells.
        for (LayerInfoMap::iterator iter = m_kLayers.begin();
            iter != m_kLayers.end();
            iter++)
        {
            NiDecorationLayerInfo& kLayerInfo = iter->second;
            kLayerInfo.m_bRequiresReset = true;
        }

        m_kLastLocalTransform = GetLocalTransform();
    }

    InitializeFunctors();

    InitializeFields();

    // TODO: Perhaps also store a global flag for these two to check if -any- layers or cells need 
    // resetting
    InitializeLayers();

    InitializeCells();

    UpdateLayers();
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::InitializeFunctors()
{
    // See which functors are able to deal with our scene graph
    NiTFunctorFactory<NiDecorationFunctorBase*>* pkFactory = 
        NiDecorationFactories::GetFunctorFactory();

    NiTObjectArray<NiFixedString> kKeys;
    pkFactory->GetKeys(kKeys);

    // Attempt to add functors to the layers
    for (LayerInfoMap::iterator layerIter = m_kLayers.begin(); 
        layerIter != m_kLayers.end(); 
        layerIter++)
    {
        NiDecorationLayerInfo& kInfo = layerIter->second;
        if (kInfo.m_bRequiresFunctorCreation)
        {
            // Remove all old functors
            kInfo.m_kFunctors.clear();

            for (NiUInt32 ui = 0; ui < kKeys.GetSize(); ui++)
            {
                NiFixedString& kKey = kKeys.GetAt(ui);
                if (pkFactory->IsValidTargetType(kKey, m_pFunctorTarget))
                {
                    // Create functor instance
                    NiDecorationFunctorBasePtr spFunctor = pkFactory->Create(kKey);
                    EE_ASSERT(spFunctor);
                    spFunctor->SetTarget(m_pFunctorTarget);

                    // Can the layer accept the functor?
                    NiPoint2 kCellRange = NiPoint2(m_kDimension.x / float(kInfo.m_uiNumCellsX),
                        m_kDimension.y / float(kInfo.m_uiNumCellsY));
                    NiUInt32 auiNumCells[] = {kInfo.m_uiNumCellsX, kInfo.m_uiNumCellsY};

                    if (!spFunctor->Validate(auiNumCells, kCellRange, GetWorldTransform()))
                        continue;

                    // The layer is compatible with this functor
                    kInfo.m_kFunctors.push_back(spFunctor);

                    // Force a reset of this layer
                    kInfo.m_bRequiresReset = true;
                }
            }

            // Attach the functors to any existing layers
            AttachFunctorsToLayer(layerIter->first);

            // Update the settings
            kInfo.m_bRequiresFunctorCreation = false;

            // After attaching the functors, we need to configure them
            kInfo.m_bRequiresFunctorReinitialization = true;
        }

        if (kInfo.m_bRequiresFunctorReinitialization)
        {
            ReconfigureFunctors(layerIter->first);
            kInfo.m_bRequiresFunctorReinitialization = false;
        }
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::InitializeFields()
{
    if (!m_bRequiresRecreation)
        return;

    NiTPrimitiveSet<FieldID> kFields;
    GetDecorationFieldKeys(kFields);

    // Recreate all fields
    for (NiUInt32 ui = 0; ui < kFields.GetSize(); ++ui)
    {
        FieldID kFieldID = kFields.GetAt(ui);
        RemoveDecorationFieldAt(kFieldID);
        CreateDecorationFieldAt(kFieldID);
    }

    // We will require initialization after this step.
    for (LayerInfoMap::iterator iter = m_kLayers.begin();
        iter != m_kLayers.end();
        iter++)
    {
        NiDecorationLayerInfo& kLayerInfo = iter->second;
        kLayerInfo.m_bRequiresInitialization = true;
    }

    m_bRequiresRecreation = false;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationPlane::InitializeLayers()
{
    // Re-create the instance pools for all the generators
    for (LayerInfoMap::iterator kLayerIter = m_kLayers.begin();
        kLayerIter != m_kLayers.end();
        kLayerIter++)
    {
        const efd::utf8string& layerName = kLayerIter->first;
        NiDecorationLayerInfo& kLayerInfo = kLayerIter->second;

        // Do we actually need to reset this layer?
        if (!kLayerInfo.m_bRequiresInitialization)
            continue;

        // Create the layer in all fields, if it doesn't exist.
        // Re-create the layer if the generator has changed.
        // Otherwise, reset the layer so that it releases all cells from the instance pool.
        NiTMapIterator kFieldIter = m_kFieldMap.GetFirstPos();
        while (kFieldIter)
        {
            NiUInt32 uiKey;
            NiDecorationField* pkValue;
            m_kFieldMap.GetNext(kFieldIter, uiKey, pkValue);

            NiDecorationLayer* pkLayer = pkValue->GetLayerAt(layerName);
            if (!pkLayer)
            {
                // Create a newly added layer
                CreateLayerForField(layerName, pkValue);
            }
            else if (pkLayer->GetGenerator() != kLayerInfo.m_spGenerator)
            {
                // Recreate a layer with a changed generator
                pkValue->RemoveLayer(layerName);
                CreateLayerForField(layerName, pkValue);
            }
            else
            {
                pkLayer->ResetCells();
            }
        }

        // Generator settings
        NiDecorationGenerator* pkGenerator = kLayerInfo.m_spGenerator;
        if (!pkGenerator)
            continue;

        const NiPoint2& kDimensions = GetDimension();
        NiPoint2 kCellSpacing(
            kDimensions.x / (float)kLayerInfo.m_uiNumCellsX, 
            kDimensions.y / (float)kLayerInfo.m_uiNumCellsY);

        float fCellRadius = NiMax(kCellSpacing.x, kCellSpacing.y) * 0.5f * NiSqrt(2.0f);
        float fMaxRange = NiMax(kLayerInfo.m_fMaxRange, kLayerInfo.m_fMaxRange + fCellRadius);

        // How many cells will we ever need at any one time?
        NiUInt32 uiRequiredCellsX = (NiUInt32)NiFloor(
            2.0f + 2.0f * (fMaxRange / kCellSpacing.x));
        NiUInt32 uiRequiredCellsY = (NiUInt32)NiFloor(
            2.0f + 2.0f * (fMaxRange  / kCellSpacing.y));
        NiUInt32 uiRequiredCells = uiRequiredCellsX * uiRequiredCellsY;

        // Work out instances per cell
        NiUInt32 uiInstancesPerCell;
        NiUInt32 uiCellCount = kLayerInfo.m_uiNumCellsX * kLayerInfo.m_uiNumCellsY;
        if (uiCellCount != 0)
        {
            NiUInt32 uiInstancesPerField = kLayerInfo.m_uiInstancesPerField;
            uiInstancesPerCell = uiInstancesPerField / uiCellCount ;
            if (uiInstancesPerField % uiCellCount != 0)
                uiInstancesPerCell++;

            // Not perfect, but prevents any more than one fields worth of cells being 
            // created at a time. This may break if the person has really tiny fields but
            // a huge decoration draw distance; this should be considered a system 
            // limitation
            if (uiRequiredCells > uiCellCount)
                uiRequiredCells = uiCellCount;
        }
        else
        {
            uiInstancesPerCell = 0;
            uiRequiredCells = 0;
        }

        pkGenerator->InitializeTransformStream(uiInstancesPerCell, uiRequiredCells);

        // Initialize the layer instances to use the new instance pools
        kFieldIter = m_kFieldMap.GetFirstPos();
        while (kFieldIter)
        {
            NiUInt32 uiKey;
            NiDecorationField* pkValue;
            m_kFieldMap.GetNext(kFieldIter, uiKey, pkValue);

            // Get the decoration layer
            NiDecorationLayer* pkLayer = pkValue->GetLayerAt(kLayerIter->first);
            EE_ASSERT(pkLayer);

            pkLayer->Initialize(kLayerInfo.m_uiNumCellsX, kLayerInfo.m_uiNumCellsY, 
                GetDimension(), kLayerInfo.m_fMaxRange, kLayerInfo.m_fMinRange, 
                kLayerInfo.m_fFarFadeDistance, kLayerInfo.m_fNearFadeDistance);
        }

        // Finished, set the appropriate flags
        kLayerInfo.m_bRequiresInitialization = false;
        kLayerInfo.m_bRequiresReset = true;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationPlane::InitializeCells()
{
    for (LayerInfoMap::iterator kLayerIter = m_kLayers.begin();
        kLayerIter != m_kLayers.end();
        kLayerIter++)
    {
        NiDecorationLayerInfo& kLayerInfo = kLayerIter->second;
        if (!kLayerInfo.m_bRequiresReset)
            continue;

        kLayerInfo.m_bRequiresReset = false;

        NiTMapIterator kFieldIter = m_kFieldMap.GetFirstPos();
        while (kFieldIter)
        {
            NiUInt32 uiKey;
            NiDecorationField* pkValue;
            m_kFieldMap.GetNext(kFieldIter, uiKey, pkValue);

            // Set the seed
            NiDecorationLayer* pkLayer = pkValue->GetLayerAt(kLayerIter->first);
            pkLayer->SetBaseSeed(kLayerInfo.m_uiRandomSeed);

            // Set non-critical shader constants
            pkLayer->SetNearFadeDistance(kLayerInfo.m_fNearFadeDistance);
            pkLayer->SetFarFadeDistance(kLayerInfo.m_fFarFadeDistance);

            // Set the active camera (which will normally trigger all cells to be reset)
            pkLayer->SetCamera(GetCamera());

            // Update shader
            pkValue->UpdateShaderConstants();
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void NiDecorationPlane::UpdateLayers()
{
    NiTMapIterator kFieldIter;

    // Re-create the instance pools for all the generators
    for (LayerInfoMap::iterator kLayerIter = m_kLayers.begin();
        kLayerIter != m_kLayers.end();
        kLayerIter++)
    {
        const efd::utf8string& layerName = kLayerIter->first;
        NiDecorationLayerInfo& kLayerInfo = kLayerIter->second;

        if (kLayerInfo.m_spGenerator == NULL)
            continue;

        NiDecorationTransformManager* pkManager = kLayerInfo.m_spGenerator->GetTransformManager();
        if (!pkManager)
            continue;

        // Tell each layer instance up update its visibility info.
        kFieldIter = m_kFieldMap.GetFirstPos();
        while (kFieldIter)
        {
            NiUInt32 uiKey;
            NiDecorationField* pkValue;
            m_kFieldMap.GetNext(kFieldIter, uiKey, pkValue);

            NiDecorationLayer* pkLayer = pkValue->GetLayerAt(layerName);
            if (!pkLayer)
                continue;

            pkLayer->ReleaseInvisibleCells();

            pkLayer->CreateVisibleCells();
        }

        // Process then upload changed cells
        kLayerInfo.m_spGenerator->ProcessChangedCells();
    }
}

//---------------------------------------------------------------------------
void NiDecorationPlane::GenerateFieldID(NiInt16 sIndexX, NiInt16 sIndexY, FieldID& kFieldID)
{
    kFieldID = 0;
    kFieldID |= (NiUInt16)sIndexX;
    kFieldID <<= 16;
    kFieldID |= (NiUInt16)sIndexY;
}

//---------------------------------------------------------------------------
void NiDecorationPlane::GenerateFieldIndex(const FieldID& kFieldID, NiInt16& sIndexX, 
    NiInt16& sIndexY)
{
    sIndexX = (NiInt16)(kFieldID >> 16);
    sIndexY = (NiInt16)(kFieldID & (NiUInt32)USHRT_MAX);
}

//------------------------------------------------------------------------------------------------
NiDecorationPlane::NiDecorationLayerInfo::NiDecorationLayerInfo(NiDecorationGenerator* pkGenerator,
    NiUInt32 uiNumCellsX, NiUInt32 uiNumCellsY, NiUInt32 uiInstancesPerField,
    float fMaxRange, float fMinRange, float fFarFadeDistance, float fNearFadeDistance,
    NiUInt32 uiMaxCellsGenerated, NiUInt32 uiRandomSeed)
    : m_spGenerator(pkGenerator)
    , m_uiNumCellsX(uiNumCellsX)
    , m_uiNumCellsY(uiNumCellsY)
    , m_uiInstancesPerField(uiInstancesPerField)
    , m_uiMaxCellsGenerated(uiMaxCellsGenerated)
    , m_uiRandomSeed(uiRandomSeed)
    , m_fMaxRange(fMaxRange)
    , m_fMinRange(fMinRange)
    , m_fNearFadeDistance(fNearFadeDistance)
    , m_fFarFadeDistance(fFarFadeDistance)
    , m_bRequiresInitialization(false)
    , m_bRequiresReset(false)
{
}

//------------------------------------------------------------------------------------------------