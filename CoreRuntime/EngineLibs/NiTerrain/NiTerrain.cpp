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

#include "NiTerrainPCH.h"

#include "NiTerrain.h"
#include "NiTerrainCellShaderData.h"
#include "NiTerrainSector.h"
#include "NiTerrainConfiguration.h"
#include "NiTerrainStreamLocks.h"
#include "NiTerrainXMLHelpers.h"
#include "NiTerrainCullingProcess.h"
#include "NiTerrainFile.h"
#include "NiTerrainSectorFile.h"
#include "NiTerrainSectorFileDeveloper.h"
#include "NiTerrainResourceManager.h"
#include "NiTerrainStreamingManager.h"
#include "NiTerrainSectorSelectorDefault.h"
#include "NiTerrainDataSnapshot.h"
#include "NiTerrainDecal.h"
#include "NiTerrainSurfacePackageFile.h"

#include "NiTerrainFileVersion0.h"
#include "NiTerrainFileVersion1.h"
#include "NiTerrainFileVersion2.h"
#include "NiTerrainFileVersion3.h"
#include "NiTerrainFileVersion4.h"

#include "NiTerrainSectorFileVersion1.h"
#include "NiTerrainSectorFileVersion2.h"
#include "NiTerrainSectorFileVersion3.h"
#include "NiTerrainSectorFileVersion4.h"
#include "NiTerrainSectorFileVersion5.h"
#include "NiTerrainSectorFileVersion6.h"
#include "NiTerrainSectorFileVersion7.h"

#include <NiFixedString.h>

// Include DX9 Renderer for device reset callbacks
#if defined(WIN32) && defined(NI_RENDERER_DX9)
#include <NiDX9Renderer.h>
#endif

using namespace efd;

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrain, NiNode, NiTypeMask::NiTerrain);
NiImplementRTTI(NiTerrain::StoragePolicy, NiTerrainStoragePolicy, NiTypeMask::NiTerrainStoragePolicy);
//--------------------------------------------------------------------------------------------------
NiTerrainConfiguration NiTerrain::ms_kDefaultConfiguration;
bool NiTerrain::ms_bInToolMode = false;
//--------------------------------------------------------------------------------------------------
NiTerrain::NiTerrain() :
    m_bHasShapeChangedLastUpdate(false),
    m_bObjDestroyed(false),
    m_uiBlockSize(32),
    m_uiNumLOD(5), 
    m_uiMaskSize(512),
    m_uiLowDetailTextureSize(512),
    m_fLODScale(NiSqrt(2.0f) * 2.0f),
    m_fLODShift(0.0f),
    m_fMinHeight(0.0f),
    m_fMaxHeight(100.0f),
    m_fVertexSpacing(1.0f),
    m_uiLODMode(NiTerrainSectorData::LOD_MODE_2D | NiTerrainSectorData::LOD_MORPH_ENABLE),
    m_kSectors(31),
    m_pkPaintingData(NULL)
{
    // Initial sector map size of 31, leaning toward optimal situation with 3^2
    // loaded sector. ie, 2 sectors in all directions around the active sector

    Initialize();
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::Initialize()
{
    m_spSectorSelector = 0;
    m_spDecalManager = 0;

    // Add default terrain-wide shader extra data
    float afDummyValues[2] = {1.0f, 1.0f};
    AddExtraData(NiTerrainCellShaderData::RENDERMODE_SHADER_CONSTANT, 
        NiNew NiIntegerExtraData(NiTerrainCellShaderData::RENDER_DISPLAY));
    AddExtraData(NiTerrainCellShaderData::DEBUGMODE_SHADER_CONSTANT, 
        NiNew NiIntegerExtraData(NiTerrainCellShaderData::DEBUG_OFF));
    AddExtraData(NiTerrainCellShaderData::LOWDETAIL_SPECULAR_SHADER_CONSTANT, 
        NiNew NiFloatsExtraData(2, afDummyValues));

    // Create the terrain material
    m_spTerrainMaterial = NiTerrainMaterial::Create();

    // Hook into the DX9 renderer for context events
    SubscribeToDXDeviceResetNotification();
}

//--------------------------------------------------------------------------------------------------
NiTerrain::~NiTerrain()
{
    DestroyData();
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::DestroyData()
{
    // Make sure we aren't destroying the data twice
    if (m_bObjDestroyed)
        return;

    UnsubscribeToDXDeviceResetNotification();

    // Wait for all loading/unloading tasks to complete
    m_kStreamingManager.WaitForAllTasksCompleted();

    // Request unload and remove all sectors on this terrain
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();
    while (kIterator)
    {
        NiTerrainSector* pkSector = NULL;
        NiUInt32 ulIndex;
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);
        EE_VERIFY(m_kStreamingManager.RequestPageSector(pkSector, -1));
    }

    // Wait for the above tasks to complete
    m_kStreamingManager.WaitForAllTasksCompleted();

    // Destroy the loading manager
    m_kStreamingManager.Abort();

    // Destroy surface index
    RemoveAllSurfaces();

    // Destroy remaining objects
    NiDelete m_pkPaintingData;
    m_spSurfaceLibrary = 0;
    m_spSectorSelector = 0;
    m_spTerrainMaterial = 0;
    m_spTerrainPropertyState = 0;
    m_spTerrainEffectState = 0;
    m_spCustomDataPolicy = 0;
    m_spDecalManager = 0;
    m_spResourceManager = 0;
    
    m_bObjDestroyed = true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::_SDMInit()
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::_SDMShutdown()
{
}

//--------------------------------------------------------------------------------------------------
const NiFixedString NiTerrain::CreateSurfacePath(
    const NiFixedString& kIndexPath)
{
    NiString kTerrainPath = kIndexPath;
    NiFixedString kSurfaceRoot;

#ifndef _PS3
    kSurfaceRoot = kTerrainPath + "\\..\\";
#else
    kSurfaceRoot = kTerrainPath + "/../";
#endif

    if (NiPath::IsRelative(kSurfaceRoot))
    {
        char* pcAbsolutePath = NiAlloc(char, 512);
        NiPath::ConvertToAbsolute(
            pcAbsolutePath, 512,
            kSurfaceRoot, 0);

        kSurfaceRoot = NiFixedString(pcAbsolutePath);
        NiFree(pcAbsolutePath);
    }

    return kSurfaceRoot;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::Save(const char* pcArchivePath, NiUInt32* puiErrorCode)
{
    bool bRes = true;
    
    // Assign a default error code register
    NiUInt32 uiDummyErrorCode;
    if (!puiErrorCode)
        puiErrorCode = &uiDummyErrorCode;

    // Save the terrain configuration
    if (!SaveTerrainConfig(pcArchivePath, puiErrorCode))
        return false;

    // Save all the sectors
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();
    while (kIterator)
    {
        NiTerrainSector* pkSector = NULL;
        NiUInt32 ulIndex;
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);

        // Attempt to save all sectors, regardless of the other ones failures
        NiUInt32 uiSectorErrors = 0;
        bool bSectorResult = pkSector->Save(pcArchivePath, &uiSectorErrors);
        bRes &= bSectorResult;
    }

    return bRes;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::Load(const char* pcFileName, NiUInt32* puiErrorCode)
{
    // Assign a default archive if one isn't specified
    if (!pcFileName)
        pcFileName = GetArchivePath();

    // Assign a default error code register
    NiUInt32 uiDummyErrorCode;
    if (!puiErrorCode)
        puiErrorCode = &uiDummyErrorCode;

    // Load the terrain configuration
    if (!LoadTerrainConfig(pcFileName, puiErrorCode))
        return false;
    
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::PageSector(NiInt16 sIndexX, NiInt16 sIndexY, NiInt32 iLOD)
{
    // Fetch the sector to begin paging data for.
    NiTerrainSector* pkSector = GetSector(sIndexX, sIndexY);
    if (!pkSector)
    {
        // Create the sector if necessary
        pkSector = CreateSector(sIndexX, sIndexY); 
    }
    
    // Decide upon what sort of operation to perform on this sector.
    NiInt32 uiLoadedLoD = pkSector->GetSectorData()->GetHighestLoadedLOD();
    if (uiLoadedLoD != iLOD)
    {
        // Perform paging on this sector
        EE_ASSERT(iLOD == NiClamp(iLOD, -1, GetNumLOD()));

        return m_kStreamingManager.RequestPageSector(pkSector, iLOD);
    }
    
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CreateBlankSector(NiInt16 sIndexX, NiInt16 sIndexY, bool bWaitForCompletion)
{
    // Create the sector if necessary
    NiTerrainSector* pkSector = GetSector(sIndexX, sIndexY);
    if (!pkSector)
    {
        pkSector = CreateSector(sIndexX, sIndexY); 
    }

    return m_kStreamingManager.RequestCreateBlankSector(pkSector, bWaitForCompletion);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::RebuildGeometry()
{
    // Wait for the streaming manager to complete it's current tasks
    m_kStreamingManager.WaitForAllTasksCompleted();

    // Setup a generic build region covering an entire sector
    NiRect<NiInt32> m_kBuildRegion;
    m_kBuildRegion.m_left = 0;
    m_kBuildRegion.m_right = GetCalcSectorSize() - 1;
    m_kBuildRegion.m_top = GetCalcSectorSize() - 1;
    m_kBuildRegion.m_bottom = 0;

    // Issue tasks for all loaded sectors to be rebuilt
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();
    bool bRes = true;
    while (kIterator)
    {
        NiTerrainSector* pkSector = NULL;
        NiUInt32 ulIndex;
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);

        // Submit a task
        m_kStreamingManager.RequestRebuildSector(pkSector, m_kBuildRegion);
    }
    return bRes;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::OnVisible(NiCullingProcess& kCuller)
{
    // If a terrain specific culler is in use, we can limit the LOD
    NiTerrainCullingProcess* pkTerrainCuller = NiDynamicCast(
        NiTerrainCullingProcess, &kCuller);

    bool bProcessBorders = true;
    NiUInt32 uiMaxLOD = NiTerrainUtils::ms_uiMAX_LOD;
    NiUInt32 uiMinLOD = 0;
    if (pkTerrainCuller)
    {
        uiMaxLOD = pkTerrainCuller->GetMaxTerrainLOD();
        uiMinLOD = pkTerrainCuller->GetMinTerrainLOD();
        bProcessBorders = pkTerrainCuller->GetUpdateGeometry();
    }

    // Perform LOD calculations
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();
    NiTerrainSector* pkSector = NULL;
    NiUInt32 ulIndex;
    while (kIterator)
    {
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);

        if (pkSector)
        {
            NiUInt32 uiOldMaxLOD =
                pkSector->GetSectorData()->GetHighestVisibleLOD();
            NiUInt32 uiOldMinLOD =
                pkSector->GetSectorData()->GetLowestVisibleLOD();

            pkSector->SetMinMaxVisibleLOD(uiMinLOD, uiMaxLOD);
            pkSector->OnVisible(kCuller);
            pkSector->SetMinMaxVisibleLOD(uiOldMinLOD, uiOldMaxLOD);
        }
    }

    if (bProcessBorders)
    {
        // Perform stitching of sectors
        kIterator = m_kSectors.GetFirstPos();
        while (kIterator)
        {
            m_kSectors.GetNext(kIterator, ulIndex, pkSector);
            pkSector->ProcessBorders();
        }
    }

    // Perform Decal culling
    // If a terrain specific culler is in use, we may need to disable rendering decals
    if (m_spDecalManager && 
        (!pkTerrainCuller || (pkTerrainCuller && pkTerrainCuller->GetRenderDecals())))
    {
        m_spDecalManager->Cull(kCuller);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{
    DoUpdate(kUpdate);
    NiNode::UpdateDownwardPass(kUpdate);

    EnforceValidBound();
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    }
    NiNode::UpdateSelectedDownwardPass(kUpdate);

    EnforceValidBound();
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    }
    NiNode::UpdateRigidDownwardPass(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::UpdateEffectsDownward(NiDynamicEffectState* pkParentState)
{
    NiDynamicEffectStatePtr spState 
        = PushLocalEffects(pkParentState, true);

    if (m_spTerrainEffectState)
        m_spTerrainEffectState = 0;

    if (spState)
        m_spTerrainEffectState = spState->Copy();
    
    for (unsigned int i = 0; i < m_kChildren.GetSize(); i++)
    {
        NiAVObject* pkChild = m_kChildren.GetAt(i);
        if (pkChild)
            pkChild->UpdateEffectsDownward (spState);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::UpdatePropertiesDownward(NiPropertyState* pkParentState)
{
    NiPropertyStatePtr spState 
        = PushLocalProperties(pkParentState, true);

    if (m_spTerrainPropertyState)
        m_spTerrainPropertyState = 0;

    if (spState)
        m_spTerrainPropertyState = NiNew NiPropertyState(*spState);
    
    for (unsigned int i = 0; i < m_kChildren.GetSize(); i++)
    {
        NiAVObject* pkChild = m_kChildren.GetAt(i);
        if (pkChild)
            pkChild->UpdatePropertiesDownward (spState);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::DoUpdate(NiUpdateProcess& kUpdate) 
{
    // Set shape changed to false now (If a sector has changed it will
    // set it to true after this when we propagate the update)
    SetShapeChangedLastUpdate(false);

    // Update any sectors that may require updating
    if (HasSettingChanged())
    {
        NiTMapIterator kIterator = m_kSectors.GetFirstPos();
        while (kIterator)
        {
            NiTerrainSector* pkSector = NULL;
            NiUInt32 ulIndex;
            m_kSectors.GetNext(kIterator, ulIndex, pkSector);

            UpdateSector(pkSector);
            
            // Sector Update is called by the NiNode::Update code
            // DO NOT propagate the Update to children here! This
            // would create multiple update calls and break the update
            // order when using the NiEntity/Component system.
        }
        SetBit(false, SETTING_CHANGED);
    }

    // Update the world location data (info that the decals and/or streaming may rely upon)
    NiAVObject::UpdateWorldData();

    // Update streaming tasks
    UpdateStreaming();

    // Update any decals
    if (m_spDecalManager)
    {
        m_spDecalManager->UpdateDecals(this, kUpdate.GetTime());
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::UpdateStreaming()
{
    // Update the set of loaded sectors
    NiTerrainSectorSelector* pkSelector = GetSectorSelector();
    if (pkSelector->UpdateSectorSelection())
    {
        typedef NiTerrainSectorSelector::LoadingInfoListType listtype;

        // Make a copy of the selected sector list, so that we can re-add any failed page requests
        listtype kListCopy;
        {
            const listtype& kList = pkSelector->GetSelectedSectors();
            kListCopy.assign(kList.begin(), kList.end());
        }

        // Clear the selection
        pkSelector->ClearSelection();

        for (listtype::const_iterator kIter = kListCopy.begin(); kIter != kListCopy.end(); kIter++)
        {
            const NiTerrainSectorSelector::LoadingInfo& kItem = *kIter;

            bool bRes = PageSector(
                            kItem.m_sIndexX, 
                            kItem.m_sIndexY, 
                            NiClamp(kItem.m_iTargetLoD, -1, m_uiNumLOD));

            if (!bRes)
            {
                pkSelector->Resubmit(kItem);
            }
        }
    }

    // Process the sector removal tasks, which are waiting for the terrain to be in a safe state
    m_kStreamingManager.PerformRendererThreadTasks(4);
}

//--------------------------------------------------------------------------------------------------
const NiTerrainConfiguration NiTerrain::GetDefaultConfiguration()
{
    return ms_kDefaultConfiguration;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::SetDefaultConfiguration(NiTerrainConfiguration kConfig)
{
    bool bValid = true;

    // Make sure that the configuration is valid
    if (!kConfig.ValidateConfiguration())
        bValid = false;

    if (bValid)
        ms_kDefaultConfiguration = kConfig;
}

//--------------------------------------------------------------------------------------------------
const NiTerrainConfiguration NiTerrain::GetConfiguration() const
{
    return ms_kDefaultConfiguration;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::SetResourceManager(NiTerrainResourceManager* pkResourceManager)
{
    if (m_spResourceManager && m_spResourceManager->GetNumActiveObjects() != 0)
    {
        EE_FAIL("NiTerrain::SetAllocator failed because current allocator is still active.");
        return;
    }

    m_spResourceManager = pkResourceManager;
}

//--------------------------------------------------------------------------------------------------
NiTerrainResourceManager* NiTerrain::GetResourceManager()
{
    if (!m_spResourceManager)
    {
        // Create a default allocator to use:
        m_spResourceManager = EE_NEW NiTerrainStandardResourceManager(this);
    }
    
    return m_spResourceManager;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::SetRenderedCellsPerFrame(NiUInt32 uiNumCells)
{
    if (m_pkPaintingData)
        m_pkPaintingData->m_uiMaxLowDetailRenderedCellsPerFrame = uiNumCells;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiTerrain::GetRenderedCellsPerFrame()
{
    if (m_pkPaintingData)
        return m_pkPaintingData->m_uiMaxLowDetailRenderedCellsPerFrame;
    return 0;
}

//--------------------------------------------------------------------------------------------------
NiNode* NiTerrain::GetLowDetailScene()
{
    if (m_pkPaintingData)
        return m_pkPaintingData->m_spLowDetailScene;
    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiRenderStep* NiTerrain::GetLowDetailRenderStep()
{
    if (m_pkPaintingData)
        return m_pkPaintingData->m_spLowDetailRenderStep;
    return NULL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CreatePaintingData()
{
    if (m_pkPaintingData)
        return true;

    // Initialise the painting data
    m_pkPaintingData = NiNew RuntimePaintingData();
    m_pkPaintingData->m_spLowDetailScene = NiNew NiNode();
    m_pkPaintingData->m_spLowDetailRenderStep = 
        NiNew NiDefaultClickRenderStep();
    m_pkPaintingData->m_spLowDetailRenderStep->SetPreProcessingCallbackFunc(
        &Callback_UpdateLowDetailRenderStep, this);
    m_pkPaintingData->m_spLowDetailRenderStep->SetPostProcessingCallbackFunc(
        &Callback_EndUsingRenderTargetGroup, this);
    m_pkPaintingData->m_uiMaxLowDetailRenderedCellsPerFrame = 0;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::InvalidateLowDetailTexture()
{
    // Loop through all the sectors
    NiTerrainSector* pkSector = NULL;
    NiUInt32 ulIndex;
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();
    while (kIterator)
    {
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);
        EE_ASSERT(pkSector);
     
        // Invalidate low detail texture for that sector
        pkSector->QueueAllCellsForLowDetailUpdate();
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::RenderLowDetailTextures(NiUInt32* puiErrorCode)
{
    NiUInt32 uiDummyErrorCode;
    if (!puiErrorCode)
        puiErrorCode = &uiDummyErrorCode;

    // Make sure painting data has been created
    if (!m_pkPaintingData)
        EE_VERIFY(CreatePaintingData());

    // Fetch the render steps
    NiRenderStep* pkStep = GetLowDetailRenderStep();
    EE_ASSERT(pkStep);
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    m_pkPaintingData->uiErrorCode = 0;

    if (pkRenderer->BeginOffScreenFrame())
    {
        pkStep->Render();
        *puiErrorCode = m_pkPaintingData->uiErrorCode;
        if(!pkRenderer->EndOffScreenFrame() || *puiErrorCode & EC_LRT_RENDERING_FAILED)
        {
            return false;
        }
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::Callback_UpdateLowDetailRenderStep(NiRenderStep* pkCurrentStep, 
        void* pvCallbackData)
{
    NiTerrain* pkTerrain = NiStaticCast(NiTerrain, pvCallbackData);
    NiDefaultClickRenderStep* pkStep = NiDynamicCast(NiDefaultClickRenderStep,
        pkCurrentStep);
    EE_ASSERT(pkStep);

    NiInt32 iMaxNumCells = pkTerrain->GetRenderedCellsPerFrame();
    if (iMaxNumCells == 0)
        iMaxNumCells = EE_SINT32_MAX;

    // Clear this render step
    pkStep->RemoveAllRenderClicks();

    // Iterate over all the sectors and request the update of the low res 
    // texture
    NiTerrainSector* pkSector = NULL;
    NiUInt32 ulIndex;
    NiTMapIterator kIterator = pkTerrain->m_kSectors.GetFirstPos();
    while (kIterator)
    {
        if (iMaxNumCells < 0) 
           break;
        pkTerrain->m_kSectors.GetNext(kIterator, ulIndex, pkSector);
        EE_ASSERT(pkSector);

        // Append the sectors render clicks
        NiUInt32 uiSectorErrors = 0;
        if (!pkSector->AppendRenderClicks(pkStep, iMaxNumCells, &uiSectorErrors))
        {
            pkTerrain->m_pkPaintingData->uiErrorCode |= EC_LRT_RENDERING_FAILED;
        }
    }

    // Update the low detail scene
    pkTerrain->m_pkPaintingData->m_spLowDetailScene->Update(0);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::Callback_EndUsingRenderTargetGroup(NiRenderStep* pkCurrentStep, 
        void* pvCallbackData)
{
    EE_UNUSED_ARG(pvCallbackData);
    EE_UNUSED_ARG(pkCurrentStep);

    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    if (pkRenderer->IsRenderTargetGroupActive())
        return pkRenderer->EndUsingRenderTargetGroup();
    else
        return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::SubscribeToDXDeviceResetNotification()
{
    m_uiDXDeviceResetCallbackIndex = 0;
    m_bRegisteredDXDeviceResetCallback = false;

#if defined(WIN32) && defined(NI_RENDERER_DX9)
    NiDX9Renderer* pkRenderer = 
        NiDynamicCast(NiDX9Renderer, NiRenderer::GetRenderer());
    if (pkRenderer)
    {
        m_uiDXDeviceResetCallbackIndex = pkRenderer->AddResetNotificationFunc(
            (NiDX9Renderer::RESETNOTIFYFUNC)&HandleDXDeviceReset, this);
        m_bRegisteredDXDeviceResetCallback = true;
    }
#endif
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::UnsubscribeToDXDeviceResetNotification()
{
#if defined(WIN32) && defined(NI_RENDERER_DX9)
    NiDX9Renderer* pkRenderer = 
        NiDynamicCast(NiDX9Renderer, NiRenderer::GetRenderer());
    if (pkRenderer && m_bRegisteredDXDeviceResetCallback)
    {
        pkRenderer->RemoveResetNotificationFunc(m_uiDXDeviceResetCallbackIndex);
    }
#endif
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::HandleDXDeviceReset(bool bBeforeReset, void* pvVoid)
{
    if (!bBeforeReset) // Wait until after device reset
    {
        NiTerrain* pkTerrain = (NiTerrain*)pvVoid;
        EE_ASSERT(pkTerrain);

        // Reset all the referenced surfaces
        SurfaceReferenceMap::iterator kIter;
        for (kIter = pkTerrain->m_kSurfaces.begin(); kIter != pkTerrain->m_kSurfaces.end(); ++kIter)
        {
            SurfaceEntry* pkEntry = kIter->second;
            if (!pkEntry)
                continue;

            NiSurface* pkSuface = const_cast<NiSurface*>(pkEntry->m_pkSurface);
            if (!pkSuface)
                continue;

            pkSuface->SetTexturingProperty(0);
            pkSuface->PrepareForUse();
        }

        // Loop through each sector and force regeneration of blend textures
        NiTerrainSector* pkSector = NULL;
        NiUInt32 ulIndex;
        NiTMapIterator kIterator = pkTerrain->m_kSectors.GetFirstPos();
        while (kIterator)
        {
            pkTerrain->m_kSectors.GetNext(kIterator, ulIndex, pkSector);
            if (pkSector)
            {
                pkSector->HandleDXDeviceReset();
            }
        }
        
        // If low detail rendering has been initialised, re-render that too
        if (pkTerrain->m_pkPaintingData)
            pkTerrain->RenderLowDetailTextures();
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::SetShaderMode(NiTerrainCellShaderData::RenderMode eRenderMode, 
    NiTerrainCellShaderData::DebugMode eDebugMode, bool bUpdateLowDetailNow)
{
    bool bUpdateLowDetail = false;

    // Update the render mode
    {
        NiIntegerExtraData* pkRenderData = NiDynamicCast(NiIntegerExtraData, 
            GetExtraData(NiTerrainCellShaderData::RENDERMODE_SHADER_CONSTANT));
        NiTerrainCellShaderData::RenderMode eOriginalMode = 
            (NiTerrainCellShaderData::RenderMode)pkRenderData->GetValue();

        pkRenderData->SetValue(eRenderMode);
        bUpdateLowDetail |= eRenderMode != eOriginalMode;
    }

    // Update the debug mode
    {
        NiIntegerExtraData* pkDebugData = NiDynamicCast(NiIntegerExtraData, 
            GetExtraData(NiTerrainCellShaderData::DEBUGMODE_SHADER_CONSTANT));
        NiTerrainCellShaderData::DebugMode eOriginalMode = 
            (NiTerrainCellShaderData::DebugMode)pkDebugData->GetValue();

        pkDebugData->SetValue(eDebugMode);
        bUpdateLowDetail |= eDebugMode != eOriginalMode;
    }

    // Update all material instances and re-render the low detail textures
    if (bUpdateLowDetail)
    {
        // Flag all objects on this terrain as requiring a material update
        NiRenderObject::RecursiveSetMaterialNeedsUpdate(this, true);

        // Flag all high detail cells on the terrain to be re-rendered to the low detail texture
        InvalidateLowDetailTexture();
    }

    if (bUpdateLowDetailNow)
        RenderLowDetailTextures(NULL);
}

//--------------------------------------------------------------------------------------------------
NiTerrainCellShaderData::DebugMode NiTerrain::GetDebugMode() const
{
    NiIntegerExtraData* pkDebugData = NiDynamicCast(NiIntegerExtraData, 
        GetExtraData(NiTerrainCellShaderData::DEBUGMODE_SHADER_CONSTANT));

    return (NiTerrainCellShaderData::DebugMode)pkDebugData->GetValue();
}

//--------------------------------------------------------------------------------------------------
NiPropertyState* NiTerrain::GetCachedPropertyState()
{
    return m_spTerrainPropertyState;
}

//--------------------------------------------------------------------------------------------------
NiDynamicEffectState* NiTerrain::GetCachedEffectState()
{
    return m_spTerrainEffectState;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::SetSectorSelector(NiTerrainSectorSelector* pkSelector)
{
    m_spSectorSelector = pkSelector;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorSelector* NiTerrain::GetSectorSelector()
{
    if (!m_spSectorSelector)
    {
        // We need to use a default selector. This will only allow to load the sector 0_0.
        // In order to load other sectors or use paging, a NiTerrainSectorSelector must be 
        // assigned to the terrain when creating the scene
        m_spSectorSelector = 
            EE_NEW NiTerrainSectorSelectorDefault(this, NiIndex(0,0), NiIndex(1,1));
    }

    return m_spSectorSelector;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::ModifySurfaceEntry(efd::UInt32 uiSurfaceIndex, SurfaceEntry* pkEntry)
{
    NiTerrainSurfaceLibrary* pkLibrary = GetSurfaceLibrary();
    
    SurfaceReferenceMap::iterator kIter = m_kSurfaces.find(uiSurfaceIndex);
    if (kIter != m_kSurfaces.end())
    {
        // Deregister this reference from the library
        NiTerrainAssetReference& kReference = kIter->second->m_kPackageReference;
        pkLibrary->DeregisterReference(kReference, this, &NiTerrain::OnSurfacePackageUpdated);

        // Check if we should be deleting this surface entry object
        if (kIter->second != pkEntry)
            NiDelete kIter->second;

        // Remove the entry from the map
        m_kSurfaces.erase(kIter);
    }

    // Update the references
    if (pkEntry)
    {
        // Update the map
        m_kSurfaces[uiSurfaceIndex] = pkEntry;

        // Register this reference with the library
        NiTerrainAssetReference& kReference = pkEntry->m_kPackageReference;
        pkLibrary->RegisterReference(kReference, this, &NiTerrain::OnSurfacePackageUpdated);
    }
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 NiTerrain::FindFirstEmptySurfaceReference()
{
    SurfaceReferenceMap::iterator kIter;
    efd::SInt32 iFirstEntry = 0;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        if (kIter->first != iFirstEntry)
            break;
        iFirstEntry++;
    }

    return iFirstEntry;
}

//--------------------------------------------------------------------------------------------------
NiInt32 NiTerrain::AddSurface(const NiSurface* pkSurface)
{
    NiUInt32 uiIndex = GetSurfaceIndex(pkSurface);
    if (uiIndex != UINT_MAX)
        return uiIndex;

    // Create an entry that matches this surface
    SurfaceEntry* pkEntry = EE_NEW SurfaceEntry();
    pkEntry->m_pkSurface = pkSurface;
    pkEntry->m_kSurfaceName = pkSurface->GetName();
    pkEntry->m_kPackageReference.SetAssetID(pkSurface->GetPackage()->GetAssetID());
    pkEntry->m_kPackageReference.SetResolvedLocation(pkSurface->GetPackage()->GetFilename());
    pkEntry->m_uiIteration = pkSurface->GetPackage()->GetIteration();
    
    efd::SInt32 iSurfaceIndex = FindFirstEmptySurfaceReference();
    ModifySurfaceEntry(iSurfaceIndex, pkEntry);
    return iSurfaceIndex;
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 NiTerrain::AddSurface(NiTerrainAssetReference* pkReference, 
    efd::UInt32 uiPackageIteration, efd::utf8string kSurfaceName)
{
    // Attempt to find the surface reference on the terrain already
    efd::SInt32 iSurfaceIndex = 
        GetSurfaceIndex(pkReference->GetAssetID().c_str(), kSurfaceName.c_str());
    if (iSurfaceIndex != -1)
    {
        // Check that the new reference being added's iteration value matches the one that is 
        // already there.
        SurfaceEntry* pkEntry = m_kSurfaces[iSurfaceIndex];
        if (pkEntry->m_uiIteration != uiPackageIteration)
        {
            EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                ("NiTerrain: A referenced material has changed since some sectors were saved. "
                "(Package URN: %s, Surface: %s), please rebuild the Terrain from Toolbench.", 
                pkReference->GetAssetID().c_str(), kSurfaceName.c_str()));
        }

        return iSurfaceIndex;
    }

    // Not there, so add an entry and return the index
    SurfaceEntry* pkEntry = EE_NEW SurfaceEntry();
    pkEntry->m_pkSurface = NULL;
    pkEntry->m_kSurfaceName = kSurfaceName.c_str();
    pkEntry->m_kPackageReference = *pkReference;
    pkEntry->m_uiIteration = uiPackageIteration;
    
    iSurfaceIndex = FindFirstEmptySurfaceReference();
    ModifySurfaceEntry(iSurfaceIndex, pkEntry);
    return iSurfaceIndex;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::InsertSurface(const NiSurface* pkSurface, NiInt32 uiSurfaceIndex)
{
    SurfaceReferenceMap::iterator kIter = m_kSurfaces.find(uiSurfaceIndex);
    if (kIter != m_kSurfaces.end() && kIter->second != NULL)
        return false;

    // Check that the index is in range
    if (uiSurfaceIndex < 0)
        return false;
    efd::SInt32 uiIndex = uiSurfaceIndex;

    // Create an entry that matches this surface
    SurfaceEntry* pkEntry = EE_NEW SurfaceEntry();
    pkEntry->m_pkSurface = pkSurface;
    pkEntry->m_kSurfaceName = pkSurface->GetName();
    pkEntry->m_kPackageReference.SetAssetID(pkSurface->GetPackage()->GetAssetID());
    pkEntry->m_kPackageReference.SetResolvedLocation(pkSurface->GetPackage()->GetFilename());
    pkEntry->m_uiIteration = pkSurface->GetPackage()->GetIteration();

    ModifySurfaceEntry(uiIndex, pkEntry);
    return true;
}

//--------------------------------------------------------------------------------------------------
NiInt32 NiTerrain::RemoveSurface(const NiSurface* pkSurface)
{
    if (!pkSurface)
        return -1;

    NiUInt32 uiSurfaceIndex = GetSurfaceIndex(pkSurface);
    if (uiSurfaceIndex == UINT_MAX)
        return uiSurfaceIndex;

    // Force the removal of the surface from on all sectors
    NiTerrainSector* pkSector = NULL;
    NiUInt32 ulIndex;
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();
    while (kIterator)
    {
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);    
        
        if (pkSector)
            pkSector->RemoveSurface(uiSurfaceIndex);
    }

    // Remove the surface from our surface index
    ModifySurfaceEntry(uiSurfaceIndex, NULL);
    return uiSurfaceIndex;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::RemoveAllSurfaces()
{
    // Force the removal of all surfaces on all sectors
    NiTerrainSector* pkSector = NULL;
    NiUInt32 ulIndex;
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();
    while (kIterator)
    {
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);    
        
        if (pkSector)
            pkSector->RemoveAllSurfaces();
    }

    // Remove all the surfaces from the surface index
    SurfaceReferenceMap::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        ModifySurfaceEntry(kIter->first, NULL);
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::SaveTerrainConfig(const char* pcArchive, NiUInt32* puiErrorCode)
{
    NiUInt32 uiDummyErrorCode;
    if (!puiErrorCode)
        puiErrorCode = &uiDummyErrorCode;

    // Set the default archive path if required
    if (!pcArchive)
        pcArchive = GetArchivePath();

    // Make sure that the archive path exists
    NiFixedString kArchivePath(pcArchive);
    if (!efd::File::DirectoryExists(kArchivePath))
    {
        if (!efd::File::CreateDirectoryRecursive(kArchivePath))
        {
            // Failed to even create the directory
            *puiErrorCode |= EC_INVALID_ARCHIVE_PATH;
            return false;
        }
    }

    // Create a file to write with
    NiTerrainFile* pkFile = OpenTerrainFile(true, pcArchive);
    if (!pkFile)
    {
        *puiErrorCode |= EC_TERRAIN_FILE_INVALID;
        NiDelete pkFile;
        return false;
    }

    // Save the configuration to the file:
    efd::UInt32 uiSectorSize = GetCalcSectorSize();
    efd::UInt32 uiNumLOD = GetNumLOD();
    efd::UInt32 uiMaskSize = GetMaskSize();
    efd::UInt32 uiLowDetailSize = GetLowDetailTextureSize();
    float fMinElevation = GetMinHeight();
    float fMaxElevation = GetMaxHeight();
    float fVertexSpacing = GetVertexSpacing();
    float fLowDetailSpecularPower = GetLowDetailSpecularPower();
    float fLowDetailSpecularIntensity = GetLowDetailSpecularIntensity();
    pkFile->WriteConfiguration(uiSectorSize, uiNumLOD, uiMaskSize, uiLowDetailSize, 
        fMinElevation, fMaxElevation, fVertexSpacing, fLowDetailSpecularPower, 
        fLowDetailSpecularIntensity);

    pkFile->Close();
    NiDelete pkFile;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::LoadTerrainConfig(const char* pcArchive, NiUInt32* puiErrorCode)
{
    NiUInt32 uiDummyErrorCode;
    if (!puiErrorCode)
        puiErrorCode = &uiDummyErrorCode;

    // Open the file for reading
    NiTerrainFile* pkFile = OpenTerrainFile(false, pcArchive);
    if(!pkFile)
    {
        // Failed to open the file
        *puiErrorCode |= EC_TERRAIN_FILE_INVALID;
        return false;
    }

    // Check the version 
    if(pkFile->GetFileVersion() != pkFile->GetInterfaceVersion())
    {
        // File is out of date
        *puiErrorCode |= EC_TERRAIN_FILE_OUTOFDATE;
    }

    // Load the configuration from the file:
    efd::UInt32 uiSectorSize, uiNumLOD, uiMaskSize, uiLowDetailSize;
    float fLowDetailSpecularPower;
    float fLowDetailSpecularIntensity;
    float fMinElevation, fMaxElevation, fVertexSpacing;
    if (!pkFile->ReadConfiguration(uiSectorSize, uiNumLOD, uiMaskSize, uiLowDetailSize, 
        fMinElevation, fMaxElevation, fVertexSpacing, fLowDetailSpecularPower, 
        fLowDetailSpecularIntensity))
    {
        *puiErrorCode |= EC_TERRAIN_MISSING_DATA;
        return false;
    }

    // Calculate the cell size
    efd::UInt32 uiCellSize = (uiSectorSize - 1) >> uiNumLOD;

    // Apply the configuration to terrain
    SetNumLOD(uiNumLOD);
    SetCellSize(uiCellSize);
    SetMaskSize(uiMaskSize);
    SetLowDetailTextureSize(uiLowDetailSize);
    SetMinHeight(fMinElevation);
    SetMaxHeight(fMaxElevation);
    SetVertexSpacing(fVertexSpacing);
    SetLowDetailSpecularPower(fLowDetailSpecularPower);
    SetLowDetailSpecularIntensity(fLowDetailSpecularIntensity);
    EE_ASSERT(GetCalcSectorSize() == uiSectorSize);

    pkFile->Close();
    NiDelete pkFile;
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::OnSurfacePackageUpdated(NiSurfacePackage* pkPackage, 
    const efd::utf8string& kAssetID)
{
    NotifySurfacePackageLoaded(kAssetID, pkPackage);
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::NotifySurfacePackageLoaded(const efd::utf8string& kAssetID, 
    NiSurfacePackage* pkPackage)
{   
    // Loop through all the surface entries:
    SurfaceReferenceMap::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        SurfaceEntry* pkEntry = kIter->second;
        if (!pkEntry)
            continue;

        // Extract the relevant information from the entry
        efd::UInt32 uiSurfaceIndex = kIter->first;
        NiFixedString kSurfaceID = pkEntry->m_kSurfaceName;
        efd::utf8string kPackageID = pkEntry->m_kPackageReference.GetAssetID();
        
        if (kPackageID == kAssetID)
        {
            // This package is relevant to us!
            NiSurface* pkSurface = NULL;
            if (pkPackage && pkPackage->GetSurface(kSurfaceID, pkSurface))
            {
                EE_ASSERT(pkSurface);
                EE_ASSERT(pkSurface->IsResolved());

                // Check that the package iteration is correct 
                // (only check on the initial resolution)
                if (pkEntry->m_pkSurface == NULL &&
                    pkPackage->GetIteration() != pkEntry->m_uiIteration)
                {
                    EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                        ("NiTerrain: Referenced material has changed since Terrain was last saved "
                        "(Package: %s, Surface: %s), please resave the Terrain from Toolbench.", 
                        (const char*)pkPackage->GetName(), (const char*)pkSurface->GetName()));
                }

                ResolveSurface(uiSurfaceIndex, pkSurface);
            }
            else
            {
                ResolveSurface(uiSurfaceIndex, NiSurface::GetErrorSurface());
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CheckPackageIteration(NiSurfacePackage* pkPackage) const
{
    // Loop through all the surface entries:
    SurfaceReferenceMap::const_iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        SurfaceEntry* pkEntry = kIter->second;
        if (!pkEntry)
            continue;

        // Extract the relevant information from the entry
        efd::utf8string kPackageID = pkEntry->m_kPackageReference.GetAssetID();
        if (kPackageID == pkPackage->GetAssetID())
        {
            // Check that the package iteration is correct
            if (pkPackage->GetIteration() != pkEntry->m_uiIteration)
            {
                return false;
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::NotifySurfaceChanged(const NiSurface* pkSurface)
{
    // Find the surface index:
    NiUInt32 uiSurfaceIndex = GetSurfaceIndex(pkSurface);
    if (uiSurfaceIndex == NiUInt32(-1))
        return;

    // Iterate over all the sectors and tell them a surface has changed
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();
    while (kIterator)
    {
        NiTerrainSector* pkSector = NULL;
        NiUInt32 ulIndex;
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);

        pkSector->NotifySurfaceChanged(uiSurfaceIndex);
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::IsSectorOnDisk(NiInt16 iSectorX, NiInt16 iSectorY)
{
    // Attempt to open the sector's file
    NiTerrainSectorFile* pkFile = OpenSectorFile(iSectorX, iSectorY, false);
    if (!pkFile)
        return false;

    // Sector does exist on disk (and is readable)
    pkFile->Close();
    EE_DELETE pkFile;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::InitializeDeformationBuffer(const NiRect<efd::SInt32>& kWorldSpaceRegion, 
    SurfaceMaskBuffer* pkBuffer)
{
    // Populate the deformation buffer with relevant information
    // Work out the sectors that this region relates to:
    efd::SInt32 iSectorWidth = GetCalcSectorSize() - 1;
    NiRect<efd::SInt32> kAffectedSectorRange;
    kAffectedSectorRange.m_left = 
        efd::SInt32(NiFloor(float(kWorldSpaceRegion.m_left) / float(iSectorWidth)));
    kAffectedSectorRange.m_right = 
        efd::SInt32(NiFloor(float(kWorldSpaceRegion.m_right) / float(iSectorWidth)));
    kAffectedSectorRange.m_top = 
        efd::SInt32(NiFloor(float(kWorldSpaceRegion.m_top) / float(iSectorWidth)));
    kAffectedSectorRange.m_bottom = 
        efd::SInt32(NiFloor(float(kWorldSpaceRegion.m_bottom) / float(iSectorWidth)));

    // Work out the height map region on a per sector basis
    NiRect<efd::SInt32> kHeightmapRegion;
    kHeightmapRegion.m_left = 
        kWorldSpaceRegion.m_left - kAffectedSectorRange.m_left * iSectorWidth;
    kHeightmapRegion.m_right = 
        kWorldSpaceRegion.m_right - kAffectedSectorRange.m_right * iSectorWidth;
    kHeightmapRegion.m_top = 
        kWorldSpaceRegion.m_top - kAffectedSectorRange.m_top * iSectorWidth;
    kHeightmapRegion.m_bottom = 
        kWorldSpaceRegion.m_bottom - kAffectedSectorRange.m_bottom * iSectorWidth;

    // Work out the actual region in pixels that this data will represent
    // The region may need to expand because there is not an exact correlation between pixels and 
    // vertices. 
    float fMaskDensity = GetSurfaceMaskDensity();
    float fPixelOffset = (0.5f / fMaskDensity);
    NiRect<efd::SInt32> kMaskRegion;
    kMaskRegion.m_left = 
        efd::SInt32(NiFloor(float(kHeightmapRegion.m_left) * fMaskDensity - fPixelOffset));
    kMaskRegion.m_right = 
        efd::SInt32(NiFloor(float(kHeightmapRegion.m_right) * fMaskDensity + fPixelOffset));
    kMaskRegion.m_top = 
        efd::SInt32(NiFloor(float(kHeightmapRegion.m_top) * fMaskDensity + fPixelOffset));
    kMaskRegion.m_bottom = 
        efd::SInt32(NiFloor(float(kHeightmapRegion.m_bottom) * fMaskDensity - fPixelOffset));

    // Check for out of bound region and adjust
    efd::SInt32 iSectorPixels = m_uiMaskSize - (2 << m_uiNumLOD);
    if (kMaskRegion.m_left < 0)
    {
        kMaskRegion.m_left += iSectorPixels;
        kAffectedSectorRange.m_left -= 1;
    }
    if (kMaskRegion.m_bottom < 0)
    {
        kMaskRegion.m_bottom += iSectorPixels;
        kAffectedSectorRange.m_bottom -= 1;
    }
    if (kMaskRegion.m_right >= iSectorPixels)
    {
        kMaskRegion.m_right -= iSectorPixels;
        kAffectedSectorRange.m_right += 1;
    }
    if (kMaskRegion.m_top >= iSectorPixels)
    {
        kMaskRegion.m_top -= iSectorPixels;
        kAffectedSectorRange.m_top += 1;
    }

    // Work out the width and height of this buffer (add one because these ranges are inclusive)
    efd::SInt32 iWidth = kMaskRegion.m_right - kMaskRegion.m_left + 1;
    efd::SInt32 iHeight = kMaskRegion.m_top - kMaskRegion.m_bottom + 1;
    // Factor in the width of a sector in pixels to the size of the buffer as well
    for (efd::SInt32 iSectX = kAffectedSectorRange.m_left; 
        iSectX < kAffectedSectorRange.m_right; ++iSectX)
    {
        iWidth += iSectorPixels;
    }
    for (efd::SInt32 iSectY = kAffectedSectorRange.m_bottom; 
        iSectY < kAffectedSectorRange.m_top; ++iSectY)
    {
        iHeight += iSectorPixels;
    }
    EE_ASSERT(iWidth > 0);
    EE_ASSERT(iHeight > 0);
 
    // Calculate the terrain space region
    efd::Float32 maskPixelSize = efd::Float32(iSectorWidth) / efd::Float32(iSectorPixels);
    efd::Float32 halfMaskPixelSize = maskPixelSize / 2.0f;
    NiRect<efd::Float32> kTerrainSpaceRegion;
    kTerrainSpaceRegion.m_left = (kAffectedSectorRange.m_left * efd::Float32(iSectorWidth)) + 
        kMaskRegion.m_left * maskPixelSize + halfMaskPixelSize;
    kTerrainSpaceRegion.m_bottom = (kAffectedSectorRange.m_bottom * efd::Float32(iSectorWidth)) +
        kMaskRegion.m_bottom * maskPixelSize + halfMaskPixelSize;
    kTerrainSpaceRegion.m_right = kTerrainSpaceRegion.m_left + 
        (efd::Float32(iWidth - 1) * maskPixelSize);
    kTerrainSpaceRegion.m_top = kTerrainSpaceRegion.m_bottom + 
        (efd::Float32(iHeight - 1) * maskPixelSize);

    // Assert that the terrainspace region returned is larger than the requested region
    EE_ASSERT(kTerrainSpaceRegion.m_left <= 
        efd::Float32(kWorldSpaceRegion.m_left) + halfMaskPixelSize);
    EE_ASSERT(kTerrainSpaceRegion.m_right >= 
        efd::Float32(kWorldSpaceRegion.m_right) - halfMaskPixelSize);
    EE_ASSERT(kTerrainSpaceRegion.m_top >= 
        efd::Float32(kWorldSpaceRegion.m_top) - halfMaskPixelSize);
    EE_ASSERT(kTerrainSpaceRegion.m_bottom <= 
        efd::Float32(kWorldSpaceRegion.m_bottom) + halfMaskPixelSize);

    // Prepare this buffer for the region
    if (!pkBuffer->SetDeformationRegion(kAffectedSectorRange, kMaskRegion, kTerrainSpaceRegion,
        iWidth, iHeight))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::GetSurfaceMask(const NiSurface* pkSurface, NiRect<efd::SInt32> kWorldSpaceRegion, 
    SurfaceMaskBuffer* pkMaskBuffer, SurfaceMaskBuffer* pkMaskSumBuffer,
    NiTerrainDataSnapshot* pkSnapshot)
{
    // Fetch the surface index of this surface
    efd::SInt32 iSurfaceIndex = GetSurfaceIndex(pkSurface);
    return GetSurfaceMask(iSurfaceIndex, kWorldSpaceRegion, pkMaskBuffer, pkMaskSumBuffer, 
        pkSnapshot);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::GetSurfaceMask(efd::SInt32 iSurfaceIndex, NiRect<efd::SInt32> kWorldSpaceRegion, 
    SurfaceMaskBuffer* pkMaskBuffer, SurfaceMaskBuffer* pkMaskSumBuffer,
    NiTerrainDataSnapshot* pkSnapshot)
{
    EE_ASSERT(pkMaskBuffer);

    // Zero the buffers (incase sectors don't exist, or the surface does not exist in an area)
    if (!InitializeDeformationBuffer(kWorldSpaceRegion, pkMaskBuffer))
        return false;
    if (pkMaskSumBuffer)
    {
        pkMaskSumBuffer->SetDeformationRegion(pkMaskBuffer->GetAffectedSectorRange(), 
            pkMaskBuffer->GetDataRegion(), pkMaskBuffer->GetTerrainSpaceRegion(),
            pkMaskBuffer->GetWidth(), pkMaskBuffer->GetHeight());
    }

    // Fetch the surface index of this surface
    if (iSurfaceIndex == -1 && !pkMaskSumBuffer)
    {
        // The surface mask is obviously empty, and we aren't looking for the current sum
        return true;
    }

    // Loop across all the relevant sectors and fetch the data
    NiRect<efd::SInt32> kAffectedSectorRange = pkMaskBuffer->GetAffectedSectorRange();
    for (efd::SInt32 iSectX = kAffectedSectorRange.m_left; 
        iSectX <= kAffectedSectorRange.m_right; ++iSectX)
    {
        for (efd::SInt32 iSectY = kAffectedSectorRange.m_bottom; 
            iSectY <= kAffectedSectorRange.m_top; ++iSectY)
        {
            NiTerrainSector* pkSector = 
                GetSector(efd::Int32ToInt16(iSectX), efd::Int32ToInt16(iSectY));
            if (pkSector)
                pkSector->GetSurfaceMask(iSurfaceIndex, pkMaskBuffer, pkMaskSumBuffer, pkSnapshot);
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::SetSurfaceMask(const NiSurface* pkSurface, SurfaceMaskBuffer* pkMaskBuffer, 
                               SurfaceMaskBuffer* pkMaskSumBuffer)
{
    // Fetch the surface index of this surface
    efd::SInt32 iSurfaceIndex = GetSurfaceIndex(pkSurface);
    if (iSurfaceIndex == -1)
    {
        // This surface does not yet exist on this terrain.
        // Make sure we add it
        iSurfaceIndex = AddSurface(pkSurface);
    }

    return SetSurfaceMask(iSurfaceIndex, pkMaskBuffer, pkMaskSumBuffer);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::SetSurfaceMask(efd::SInt32 iSurfaceIndex, SurfaceMaskBuffer* pkMaskBuffer, 
    SurfaceMaskBuffer* pkMaskSumBuffer)
{
    EE_ASSERT(pkMaskBuffer);

    // Detect if we are modifying any sector border pixels
    // Increase the affected sector range and adjust the pixel coordinates
    // Pixel (-1) is shared with Pixel (iSectorPixels - 1) of the adjacent sector and
    // Pixel (iSectorPixels) is shared with Pixel(0) of the adjacent sector. 
    // Lower Bounds:
    efd::SInt32 iSectorPixels = m_uiMaskSize - (2 << m_uiNumLOD);
    NiRect<efd::SInt32> kMaskRegion = pkMaskBuffer->GetDataRegion();
    NiRect<efd::SInt32> kAffectedSectorRange = pkMaskBuffer->GetAffectedSectorRange();
    if (kMaskRegion.m_left <= 0)
    {
        EE_ASSERT(kMaskRegion.m_left >= -1);
        kMaskRegion.m_left += iSectorPixels;
        kAffectedSectorRange.m_left -= 1;
    }
    if (kMaskRegion.m_bottom <= 0)
    {
        EE_ASSERT(kMaskRegion.m_bottom >= -1);
        kMaskRegion.m_bottom += iSectorPixels;
        kAffectedSectorRange.m_bottom -= 1;
    }
    // Upper Bounds: 
    if (kMaskRegion.m_right >= iSectorPixels - 1)
    {
        EE_ASSERT(kMaskRegion.m_right <= iSectorPixels);
        kMaskRegion.m_right -= iSectorPixels;
        kAffectedSectorRange.m_right += 1;
    }
    if (kMaskRegion.m_top >= iSectorPixels - 1)
    {
        EE_ASSERT(kMaskRegion.m_top <= iSectorPixels);
        kMaskRegion.m_top -= iSectorPixels;
        kAffectedSectorRange.m_top += 1;
    }

    // Add the surface to this region if possible
    EE_ASSERT(iSurfaceIndex > -1);
    AddSurface(iSurfaceIndex, kAffectedSectorRange, kMaskRegion);

    // Loop across all the relevant sectors and push the data
    for (efd::SInt32 iSectX = kAffectedSectorRange.m_left; 
        iSectX <= kAffectedSectorRange.m_right; ++iSectX)
    {
        for (efd::SInt32 iSectY = kAffectedSectorRange.m_bottom;
            iSectY <= kAffectedSectorRange.m_top; ++iSectY)
        {
            NiTerrainSector* pkSector = 
                GetSector(efd::Int32ToInt16(iSectX), efd::Int32ToInt16(iSectY));
            if (pkSector)
                pkSector->SetSurfaceMask(iSurfaceIndex, pkMaskBuffer, pkMaskSumBuffer);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::AddSurface(efd::UInt32 uiSurfaceIndex, NiRect<efd::SInt32> kAffectedSectorRange, 
                           NiRect<efd::SInt32> kMaskRegion)
{
    // Loop across all the relevant sectors and push the data
    for (efd::SInt32 iSectX = kAffectedSectorRange.m_left; 
        iSectX <= kAffectedSectorRange.m_right; ++iSectX)
    {
        for (efd::SInt32 iSectY = kAffectedSectorRange.m_bottom; 
            iSectY <= kAffectedSectorRange.m_top; ++iSectY)
        {
            NiTerrainSector* pkSector = 
                GetSector(efd::Int32ToInt16(iSectX), efd::Int32ToInt16(iSectY));
            if (pkSector)
                pkSector->AddSurface(uiSurfaceIndex, kAffectedSectorRange, kMaskRegion);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::InitializeDeformationBuffer(const NiRect<efd::SInt32>& kWorldSpaceRegion, 
                                            HeightMapBuffer* pkBuffer)
{
    // Calculate the terrain space region
    NiRect<efd::Float32> kTerrainSpaceRegion;
    kTerrainSpaceRegion.m_left = (efd::Float32)kWorldSpaceRegion.m_left;
    kTerrainSpaceRegion.m_right = (efd::Float32)kWorldSpaceRegion.m_right;
    kTerrainSpaceRegion.m_top = (efd::Float32)kWorldSpaceRegion.m_top;
    kTerrainSpaceRegion.m_bottom = (efd::Float32)kWorldSpaceRegion.m_bottom;

    // Populate the deformation buffer with relevant information
    // Work out the sectors that this region relates to:
    // NOTE: Must convert to floats to use correct division results - non integer math
    efd::SInt32 iSectorWidth = GetCalcSectorSize() - 1;
    NiRect<efd::SInt32> kAffectedSectorRange;
    NiRect<efd::SInt32> kHeightmapRegion = kWorldSpaceRegion;
    kAffectedSectorRange.m_left = 
        efd::SInt32(NiFloor(float(kHeightmapRegion.m_left) / float(iSectorWidth)));
    kAffectedSectorRange.m_right = 
        efd::SInt32(NiFloor(float(kHeightmapRegion.m_right) / float(iSectorWidth)));
    kAffectedSectorRange.m_top = 
        efd::SInt32(NiFloor(float(kHeightmapRegion.m_top) / float(iSectorWidth)));
    kAffectedSectorRange.m_bottom = 
        efd::SInt32(NiFloor(float(kHeightmapRegion.m_bottom) / float(iSectorWidth)));

    // Work out the height map region on a per sector basis
    kHeightmapRegion.m_left = 
        kHeightmapRegion.m_left - kAffectedSectorRange.m_left * iSectorWidth;
    kHeightmapRegion.m_right = 
        kHeightmapRegion.m_right - kAffectedSectorRange.m_right * iSectorWidth;
    kHeightmapRegion.m_top = 
        kHeightmapRegion.m_top - kAffectedSectorRange.m_top * iSectorWidth;
    kHeightmapRegion.m_bottom = 
        kHeightmapRegion.m_bottom - kAffectedSectorRange.m_bottom * iSectorWidth;

    // Detect if we are modifying any sector border verts
    // We want the smallest set of sectors that will give us our data
    // Decrease the affected sector range and adjust the vert coordinates
    // Vert (0) is shared with Vert (iSectorWidth) of the adjacent sector
    // Upper Bounds:
     if (kHeightmapRegion.m_right == iSectorWidth)
    {
        kHeightmapRegion.m_right -= iSectorWidth;
        kAffectedSectorRange.m_right += 1;
    }
    if (kHeightmapRegion.m_top == iSectorWidth)
    {
        kHeightmapRegion.m_top -= iSectorWidth;
        kAffectedSectorRange.m_top += 1;
    }
    // Lower Bounds: 
    if (kHeightmapRegion.m_left == 0)
    {
        kHeightmapRegion.m_left += iSectorWidth;
        kAffectedSectorRange.m_left -= 1;
    }
    if (kHeightmapRegion.m_bottom == 0)
    {
        kHeightmapRegion.m_bottom += iSectorWidth;
        kAffectedSectorRange.m_bottom -= 1;
    }

    // Work out the width and height of this buffer (add one because these ranges are inclusive)
    efd::SInt32 iWidth = kHeightmapRegion.m_right - kHeightmapRegion.m_left + 1;
    efd::SInt32 iHeight = kHeightmapRegion.m_top - kHeightmapRegion.m_bottom + 1;
    // Factor in the width of a sector in pixels to the size of the buffer as well
    for (efd::SInt32 iSectX = kAffectedSectorRange.m_left; 
        iSectX < kAffectedSectorRange.m_right; ++iSectX)
    {
        iWidth += iSectorWidth;
    }
    for (efd::SInt32 iSectY = kAffectedSectorRange.m_bottom; 
        iSectY < kAffectedSectorRange.m_top; ++iSectY)
    {
        iHeight += iSectorWidth;
    }
    EE_ASSERT(iWidth > 0);
    EE_ASSERT(iHeight > 0);

    // Prepare this buffer for the region
    if (!pkBuffer->SetDeformationRegion(kAffectedSectorRange, kHeightmapRegion,
        kTerrainSpaceRegion, iWidth, iHeight))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::GetHeightMap(NiRect<efd::SInt32> kWorldSpaceRegion, HeightMapBuffer* pkBuffer,
    NiTerrainDataSnapshot* pkSnapshot)
{
    // Zero the buffer (incase sectors don't exist, or the surface does not exist in an area)
    if (!InitializeDeformationBuffer(kWorldSpaceRegion, pkBuffer))
        return false;

    // Detect if we are modifying any sector border verts
    // Increase the affected sector range and adjust the vert coordinates
    // Vert (0) is shared with Vert (iSectorWidth) of the adjacent sector
    efd::SInt32 iSectorWidth = GetCalcSectorSize() - 1;
    NiRect<efd::SInt32> kMapRegion = pkBuffer->GetDataRegion();
    NiRect<efd::SInt32> kAffectedSectorRange = pkBuffer->GetAffectedSectorRange();
    // Lower Bounds:
    if (kMapRegion.m_left <= 0)
    {
        EE_ASSERT(kMapRegion.m_left >= 0);
        kAffectedSectorRange.m_left -= 1;
    }
    if (kMapRegion.m_bottom <= 0)
    {
        EE_ASSERT(kMapRegion.m_bottom >= 0);
        kAffectedSectorRange.m_bottom -= 1;
    }
    // Upper Bounds: 
    if (kMapRegion.m_right >= iSectorWidth)
    {
        EE_ASSERT(kMapRegion.m_right <= iSectorWidth);
        kAffectedSectorRange.m_right += 1;
    }
    if (kMapRegion.m_top >= iSectorWidth)
    {
        EE_ASSERT(kMapRegion.m_top <= iSectorWidth);
        kAffectedSectorRange.m_top += 1;
    }

    // Loop across all the relevant sectors and fetch the data
    for (efd::SInt32 iSectX = kAffectedSectorRange.m_left; 
        iSectX <= kAffectedSectorRange.m_right; ++iSectX)
    {
        for (efd::SInt32 iSectY = kAffectedSectorRange.m_bottom; 
            iSectY <= kAffectedSectorRange.m_top; ++iSectY)
        {
            NiTerrainSector* pkSector = 
                GetSector(efd::Int32ToInt16(iSectX), efd::Int32ToInt16(iSectY));
            if (pkSector)
                pkSector->GetHeightMap(pkBuffer, pkSnapshot);
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::SetHeightMap(HeightMapBuffer* pkBuffer)
{
    // Can't set the heightmap if we aren't in tool mode (streams are GPU_READ only)
    if (!InToolMode())
        return false;

    // Detect if we are modifying any sector border verts
    // Increase the affected sector range and adjust the vert coordinates
    // Vert (0) is shared with Vert (iSectorWidth) of the adjacent sector
    efd::SInt32 iSectorWidthInVerts = GetCalcSectorSize();
    NiRect<efd::SInt32> kMapRegion = pkBuffer->GetDataRegion();
    NiRect<efd::SInt32> kAffectedSectorRange = pkBuffer->GetAffectedSectorRange();
    // Lower Bounds:
    if (kMapRegion.m_left <= 0)
    {
        EE_ASSERT(kMapRegion.m_left >= 0);
        kMapRegion.m_left += iSectorWidthInVerts - 1;
        kAffectedSectorRange.m_left -= 1;
    }
    if (kMapRegion.m_bottom <= 0)
    {
        EE_ASSERT(kMapRegion.m_bottom >= 0);
        kMapRegion.m_bottom += iSectorWidthInVerts - 1;
        kAffectedSectorRange.m_bottom -= 1;
    }
    // Upper Bounds: 
    if (kMapRegion.m_right >= iSectorWidthInVerts - 1)
    {
        EE_ASSERT(kMapRegion.m_right <= iSectorWidthInVerts - 1);
        kMapRegion.m_right -= iSectorWidthInVerts - 1;
        kAffectedSectorRange.m_right += 1;
    }
    if (kMapRegion.m_top >= iSectorWidthInVerts - 1)
    {
        EE_ASSERT(kMapRegion.m_top <= iSectorWidthInVerts - 1);
        kMapRegion.m_top -= iSectorWidthInVerts - 1;
        kAffectedSectorRange.m_top += 1;
    }

    // Loop across all the relevant sectors and push the data
    efd::queue< NiRect<efd::SInt32> > kRegionRecord;
    for (efd::SInt32 iSectX = kAffectedSectorRange.m_left; 
        iSectX <= kAffectedSectorRange.m_right; ++iSectX)
    {
        for (efd::SInt32 iSectY = kAffectedSectorRange.m_bottom; 
            iSectY <= kAffectedSectorRange.m_top; ++iSectY)
        {
            NiTerrainSector* pkSector = 
                GetSector(efd::Int32ToInt16(iSectX), efd::Int32ToInt16(iSectY));
            if (pkSector)
                kRegionRecord.push(pkSector->SetHeightMap(pkBuffer));
        }
    }

    // Loop across all the relevant sectors and notify the heightmap region has changed
    for (efd::SInt32 iSectX = kAffectedSectorRange.m_left; 
        iSectX <= kAffectedSectorRange.m_right; ++iSectX)
    {
        for (efd::SInt32 iSectY = kAffectedSectorRange.m_bottom; 
            iSectY <= kAffectedSectorRange.m_top; ++iSectY)
        {
            NiTerrainSector* pkSector = 
                GetSector(efd::Int32ToInt16(iSectX), efd::Int32ToInt16(iSectY));
            if (pkSector)
            {
                pkSector->NotifyHeightMapRegionChanged(kRegionRecord.front());
                kRegionRecord.pop();
            }
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::RestoreDataSnapshot(NiTerrainDataSnapshot* pkSnapshot)
{
    if (!pkSnapshot)
        return;

    // Restore the data stored in the snapshot to this terrain
    NiTerrainDataSnapshot::CellMap kCells = pkSnapshot->GetCellSnapshots();
    NiTerrainDataSnapshot::CellStack kCellStack = pkSnapshot->GetCellStack();
    NiTerrainDataSnapshot::CellStack::reverse_iterator kIter;
    for (kIter = kCellStack.rbegin(); kIter != kCellStack.rend(); ++kIter)
    {
        // Look up the snapshot:
        NiTerrainSector::CellID kCellID = *kIter;
        NiTerrainDataSnapshot::CellSnapshot* kCell = kCells[kCellID];
        EE_ASSERT(kCell);

        // Fetch the heightmap and set it
        if (kCell->ContainsData(NiTerrainDataSnapshot::BufferType::HEIGHTMAP))
        {
            HeightMapBuffer* pkHeightmap = kCell->GetHeightmap();
            if (pkHeightmap)
                SetHeightMap(pkHeightmap);
        }

        // Fetch the surface masks and set those
        if (kCell->ContainsData(NiTerrainDataSnapshot::BufferType::SURFACE_MASK))
        {
            const NiTerrainDataSnapshot::CellSnapshot::SurfaceMaskMap& kSurfaces = 
                kCell->GetSurfaceMasks();

            // Find the cell
            NiTerrainSector* pkSector = NULL;
            m_kSectors.GetAt(kCellID.m_kSectorId, pkSector);
            if (!pkSector)
                continue;
            NiTerrainCellLeaf* pkLeaf = 
                NiDynamicCast(NiTerrainCellLeaf, pkSector->GetCell(kCellID.m_uiCellId));
            EE_ASSERT(pkLeaf);

            // First, remove any surfaces from the cell that arent' in the snapshot
            NiTerrainDataSnapshot::CellSnapshot::SurfaceMaskMap::const_iterator kSurfIter;
            for (efd::SInt32 uiSlot = 0; uiSlot < efd::SInt32(pkLeaf->GetSurfaceCount()); ++uiSlot)
            {
                efd::SInt32 iSurfaceIndex = pkLeaf->GetSurfaceIndex(uiSlot);
                
                // Find this surface in the list
                kSurfIter = kSurfaces.find(iSurfaceIndex);
                if (kSurfIter == kSurfaces.end())
                {
                    // This surface isn't in the snapshot, so remove it
                    pkLeaf->RemoveSurface(iSurfaceIndex);

                    // go backwards on the uiSlot value because if the surface was removed then we 
                    // should stay checking the current slot index (as it will now contain
                    // a different surface's data (or won't be used anymore))
                    uiSlot--;
                }
            }
            
            // Now set the masks of the correct surfaces
            for (kSurfIter = kSurfaces.begin(); kSurfIter != kSurfaces.end(); ++kSurfIter)
            {
                efd::UInt32 uiSurfaceIndex = kSurfIter->first;
                SurfaceMaskBuffer* pkMask = kSurfIter->second;
                if (!pkMask)
                    continue;
                
                // Set the surface mask
                SetSurfaceMask(uiSurfaceIndex, pkMask, NULL);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFile* NiTerrain::OpenSectorFile(NiInt32 iSectorX, NiInt32 iSectorY, bool bWrite,
    const char* pucArchive)
{
    // Default archive path (can be overridden for autosave)
    if (!pucArchive)
        pucArchive = GetArchivePath();

    // Figure out which callback to execute
    StoragePolicy* pkStoragePolicy = GetStoragePolicy();
    return pkStoragePolicy->OpenSectorFile(pucArchive, iSectorX, iSectorY, bWrite);
}

//--------------------------------------------------------------------------------------------------
NiTerrainFile* NiTerrain::OpenTerrainFile(bool bWrite, const char* pucArchive)
{
    // Default archive path (can be overridden for autosave)
    if (!pucArchive)
        pucArchive = GetArchivePath();

    // Figure out which callback to execute
    StoragePolicy* pkStoragePolicy = GetStoragePolicy();
    return pkStoragePolicy->OpenTerrainFile(pucArchive, bWrite);
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::LockSurface(bool bLockRead)
{
    if (bLockRead)
        m_kSurfaceRWLock.LockRead();
    else
        m_kSurfaceRWLock.LockWrite();
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::UnlockSurface(bool bLockRead)
{
    if (bLockRead)
        m_kSurfaceRWLock.UnlockRead();
    else
        m_kSurfaceRWLock.UnlockWrite();
}

//--------------------------------------------------------------------------------------------------
NiTerrain::StoragePolicy::StoragePolicy()
{
    // Make the read/write formats default to the standard file format.
    m_spTerrainReadFormat = EE_NEW NiTerrainFileFormatRegistry();
    m_spTerrainWriteFormat = m_spTerrainReadFormat;

    // Enable the ability to read only the latest file format
    m_spTerrainReadFormat->AddFileFormat<NiTerrainFileVersion0>(0);
    m_spTerrainReadFormat->AddFileFormat<NiTerrainFileVersion1>(1);
    m_spTerrainReadFormat->AddFileFormat<NiTerrainFileVersion2>(2);
    m_spTerrainReadFormat->AddFileFormat<NiTerrainFileVersion3>(3);
    m_spTerrainReadFormat->AddFileFormat<NiTerrainFileVersion4>(4);

    // Make the read/write formats default to the standard file format.
    m_spSectorReadFormat = EE_NEW NiTerrainSectorFileFormatRegistry();
    m_spSectorWriteFormat = m_spSectorReadFormat;

    // Enable the ability to read only the latest file format
    m_spSectorReadFormat->AddFileFormat<NiTerrainSectorFileVersion1>(1);
    m_spSectorReadFormat->AddFileFormat<NiTerrainSectorFileVersion2>(2);
    m_spSectorReadFormat->AddFileFormat<NiTerrainSectorFileVersion3>(3);
    m_spSectorReadFormat->AddFileFormat<NiTerrainSectorFileVersion4>(4);
    m_spSectorReadFormat->AddFileFormat<NiTerrainSectorFileVersion5>(5);
    m_spSectorReadFormat->AddFileFormat<NiTerrainSectorFileVersion6>(6);
    m_spSectorReadFormat->AddFileFormat<NiTerrainSectorFileVersion7>(7);
}

//--------------------------------------------------------------------------------------------------
NiTerrainFile* NiTerrain::StoragePolicy::OpenTerrainFile(const char* kTerrainArchive, 
    bool bWriteAccess)
{
    NiTerrainFileFormatRegistry* pkFormat = 
        (bWriteAccess) ? (m_spTerrainWriteFormat) : (m_spTerrainReadFormat);
    EE_ASSERT(pkFormat);

    if (!kTerrainArchive)
        return NULL;

    NiTerrainFile::FileIdentifier kFileID;
    kFileID.m_kArchivePath = kTerrainArchive;
    kFileID.m_pkStoragePolicy = this;
    return pkFormat->OpenFile(kFileID, bWriteAccess);
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFile* NiTerrain::StoragePolicy::OpenSectorFile(const char* kTerrainArchive, 
    NiInt32 iSectorX, NiInt32 iSectorY, bool bWriteAccess)
{
    NiTerrainSectorFileFormatRegistry* pkFormat = 
        (bWriteAccess) ? (m_spSectorWriteFormat) : (m_spSectorReadFormat);
    EE_ASSERT(pkFormat);

    if (!kTerrainArchive)
        return NULL;

    NiTerrainSectorFile::FileIdentifier kFileID;
    kFileID.m_kArchivePath = kTerrainArchive;
    kFileID.m_iSectorX = iSectorX;
    kFileID.m_iSectorY = iSectorY;
    kFileID.m_pkStoragePolicy = this;
    return pkFormat->OpenFile(kFileID, bWriteAccess);
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 NiTerrain::CustomDataPolicy::GetLoadPrecacheFields()
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CustomDataPolicy::BeginLoadCustomData(NiTerrainSector* pkSector, 
    efd::SInt32 iTargetLOD, NiTerrainSectorFile* pkFile)
{
    EE_UNUSED_ARG(pkSector);
    EE_UNUSED_ARG(pkFile);
    EE_UNUSED_ARG(iTargetLOD);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CustomDataPolicy::LoadCustomData(NiTerrainSector* pkSector, efd::SInt32 iTargetLOD,  
    NiTerrainSectorFile* pkFile)
{
    EE_UNUSED_ARG(pkSector);
    EE_UNUSED_ARG(pkFile);
    EE_UNUSED_ARG(iTargetLOD);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CustomDataPolicy::EndLoadCustomData(NiTerrainSector* pkSector, 
    efd::SInt32 iTargetLOD, NiTerrainSectorFile* pkFile)
{
    EE_UNUSED_ARG(pkSector);
    EE_UNUSED_ARG(pkFile);
    EE_UNUSED_ARG(iTargetLOD);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CustomDataPolicy::BeginUnloadCustomData(NiTerrainSector* pkSector, 
    efd::SInt32 iTargetLOD)
{
    EE_UNUSED_ARG(pkSector);
    EE_UNUSED_ARG(iTargetLOD);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CustomDataPolicy::UnloadCustomData(NiTerrainSector* pkSector, 
    efd::SInt32 iTargetLOD)
{
    EE_UNUSED_ARG(pkSector);
    EE_UNUSED_ARG(iTargetLOD);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CustomDataPolicy::EndUnloadCustomData(NiTerrainSector* pkSector, 
    efd::SInt32 iTargetLOD)
{
    EE_UNUSED_ARG(pkSector);
    EE_UNUSED_ARG(iTargetLOD);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::CustomDataPolicy::SaveCustomData(NiTerrainSector* pkSector, 
    NiTerrainSectorFile* pkFile)
{
    EE_UNUSED_ARG(pkSector);
    EE_UNUSED_ARG(pkFile);

    return true;
}

//--------------------------------------------------------------------------------------------------
NiTerrain::SurfaceEntry::SurfaceEntry()
    : m_pkSurface(NULL)
    , m_uiIteration(0)
{
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::Collide(NiRay& kRay, NiUInt32 uiDetailLOD) const
{
    bool bFound = false;

    if (kRay.GetDirection().SqrLength() == 0.0f)
        return bFound;

    // Get Ray origin and direction in Terrain space
    NiTransform kTerrainTransform = GetWorldTransform();
    NiTransform kInverseTransform;
    kTerrainTransform.Invert(kInverseTransform);
    NiPoint3 kOrigin = kInverseTransform * kRay.GetOrigin();

    NiPoint3 kDirection = kRay.GetDirection() * kTerrainTransform.m_Rotate;
    kDirection.Unitize();

    // Check whether we need to parse through all sectors or if we can use a 2D ray    
    if (NiAbs(kDirection.z) > (1.0f - FLT_EPSILON) &&
        NiAbs(kDirection.z) < (1.0f + FLT_EPSILON))
    {
        // The ray is normal to the X-Y Plane: Find the sector we will collide with
        float fSectorWidth = float(GetCalcSectorSize() - 1);
        kOrigin /= fSectorWidth;
        kOrigin = kOrigin + NiPoint3(0.5f, 0.5f, 0.5f);

        NiInt16 sSectorX = NiInt16(efd::Floor(kOrigin.x));
        NiInt16 sSectorY = NiInt16(efd::Floor(kOrigin.y));
        const NiTerrainSector* pkSector = GetSector(sSectorX, sSectorY);
        
        if (pkSector)
        {            
            bFound = pkSector->CollideWithRay2D(kRay, uiDetailLOD);
        }
    }
    else
    {
        NiTMapIterator kIterator = m_kSectors.GetFirstPos();
        //Loop through all sectors
        while (kIterator)
        {
            NiTerrainSector* pkSector = NULL;
            NiUInt32 ulIndex;
            m_kSectors.GetNext(kIterator, ulIndex, pkSector);

            if (pkSector && pkSector->CollideWithRay(kRay, uiDetailLOD))
            {
                bFound = true;
            }
        }
    }

    return bFound;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::Test(NiRay& kRay, NiUInt32 uiDetailLOD) const
{
    NiTMapIterator kIterator = m_kSectors.GetFirstPos();

    // Loop through all sectors
    while (kIterator)
    {
        NiTerrainSector* pkSector = NULL;
        NiUInt32 ulIndex;
        m_kSectors.GetNext(kIterator, ulIndex, pkSector);

        if (pkSector && pkSector->CollideWithRay(kRay, uiDetailLOD))
            return true;
    }
    return false;
}

//---------------------------------------------------------------------------
NiTerrainDecal* NiTerrain::CreateDecal(NiTexture* pkDecalTexture, NiPoint3 kPosition, 
    NiUInt32 uiSize, NiUInt32 uiRatio, float fTimeOfDeath, float fDecayLength,
    float fDepthBiasOffset)
{
    // Create a new decal
    NiTerrainDecal* pkDecal = NiNew NiTerrainDecal();

    // Configure it with the given parameters
    pkDecal->Initialize(pkDecalTexture, kPosition, this, uiSize,
        (float)uiRatio, fTimeOfDeath, fTimeOfDeath -fDecayLength,
        fDepthBiasOffset);

    // Configure the fading distance - Should always be set to the morph distance
    // in order to avoid z fighting issues
    NiUInt32 uiBlockWidth = this->GetCellSize();

    static const float fSqrt2 = NiSqrt(2.0f);
    float fMaxDist = NiSqr(float(uiBlockWidth) * GetLODScale()* fSqrt2);
    float fMorphDistance = fSqrt2 * (uiBlockWidth / 2);

    float fLODThres = NiSqrt(fMaxDist) - fMorphDistance;
    pkDecal->SetFadingDistance(fLODThres);
    
    // Add it to the manager
    AddDecal(pkDecal);

    // Return the decal
    return pkDecal;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::AddDecal(NiTerrainDecal* pkDecal)
{
    GetDecalManager()->AddDecal(pkDecal);
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::RemoveDecal(NiTerrainDecal* pkDecal)
{
    GetDecalManager()->RemoveDecal(pkDecal);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::GetSurfaceOpacity(NiRay& kRay, const NiSurface* pkSurface, NiUInt8& ucOpacity, 
    NiUInt32 uiDetailLevel, bool bIncludeLowerDetail) const
{
    EE_ASSERT(pkSurface);

    bool bFound = Collide(kRay, uiDetailLevel);

    // Loop through all sectors
    if (bFound)
    {
        NiPoint3 kLocation, kNormal;
        kRay.GetIntersection(kLocation, kNormal);

        NiTerrainCellLeaf *pkLeaf = 
            NiDynamicCast(NiTerrainCellLeaf, kRay.GetCollidedCell());

        NiUInt32 uiNumSurfaces = pkLeaf->GetSurfaceCount();
        while (uiNumSurfaces == 0 && bIncludeLowerDetail)
        {
            pkLeaf = NiDynamicCast(NiTerrainCellLeaf, pkLeaf->GetParent());

            if (!pkLeaf)
                break;

            uiNumSurfaces = pkLeaf->GetSurfaceCount();
            uiDetailLevel = pkLeaf->GetLevel();
        }

        return NiTerrainUtils::GetSurfaceOpacity(pkSurface, kLocation,
            ucOpacity, uiDetailLevel, bIncludeLowerDetail, pkLeaf);
    }

    return false;
}

//---------------------------------------------------------------------------
bool NiTerrain::GetMetaData(NiRay& kRay, efd::map<efd::utf8string, NiMetaData>& kMetaData) const
{
    bool bFound = Collide(kRay);

    if (bFound)
    {
        NiPoint3 kLocation, kNormal;
        kRay.GetIntersection(kLocation, kNormal);
        GetMetaData(kLocation, kRay.GetCollidedCell(), kMetaData);
    }

    return bFound;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::GetMetaData(const NiPoint3& kIntersectionPt, const NiTerrainCell* pkCell, 
    efd::map<efd::utf8string, NiMetaData>& kMetaData) const
{
    NiTerrainCellLeaf* pkCellLeaf = NiDynamicCast(NiTerrainCellLeaf, pkCell);

    if (!pkCellLeaf)
        return;
    kMetaData.clear();

    // Climb up to the first leaf with a surface associated with it
    NiUInt32 uiNumSurfaces = pkCellLeaf->GetSurfaceCount();
    NiUInt32 uiDetailLevel = 0;
    while (uiNumSurfaces == 0)
    {
        pkCellLeaf = NiDynamicCast(NiTerrainCellLeaf, pkCellLeaf->GetParent());

        if (!pkCellLeaf)
            return;

        uiNumSurfaces = pkCellLeaf->GetSurfaceCount();
        uiDetailLevel = pkCellLeaf->GetLevel();
    }

    // Iterate over the surfaces
    for (NiUInt32 ui = 0; ui < uiNumSurfaces; ui++)
    {
        const NiSurface *pkSurface = pkCellLeaf->GetSurface(ui);

        NiUInt8 ucOpacity;
        NiTerrainUtils::GetSurfaceOpacity(
            pkSurface, kIntersectionPt, ucOpacity, uiDetailLevel, true,
            pkCellLeaf);

        if (ucOpacity)
        {
            float fWeight = ucOpacity / 255.0f;

            efd::utf8string kSurfaceName = (const char*)pkSurface->GetName();
            NiMetaData kSurfaceMeta = pkSurface->GetMetaData();
            kSurfaceMeta.UpdateWeights(fWeight);
            kMetaData[kSurfaceName] = kSurfaceMeta;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::QueryMetaData(const NiPoint3& kIntersectionPt, const NiTerrainCellLeaf *pkLeaf,
    MetaDataVisitor* pkVisitor) const
{
    EE_ASSERT(pkLeaf);

    NiUInt32 uiNumSurfaces = pkLeaf->GetSurfaceCount();
    NiUInt32 uiDetailLevel = 0;

    // Iterate over the surfaces
    for (NiUInt32 ui = 0; ui < uiNumSurfaces; ui++)
    {
        const NiSurface *pkSurface = pkLeaf->GetSurface(ui);

        NiUInt8 ucOpacity = 0;
        if (!NiTerrainUtils::GetSurfaceOpacity(pkSurface, kIntersectionPt, ucOpacity, uiDetailLevel, 
            true, pkLeaf))
        {
            // This can happen when preloading surfaces
            continue;
        }

        if (ucOpacity)
        {
            float fWeight = ucOpacity / 255.0f;
            pkVisitor->Visit(pkSurface, pkSurface->GetMetaData(), fWeight);
        }
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrain::GetBlendedMetaData(NiRay& kRay, efd::map<efd::utf8string, NiMetaData>& kMetaData,
    NiMetaData& kBlendedMetaData) const
{
    bool bFound = Collide(kRay);

    if (bFound)
    {
        NiPoint3 kLocation, kNormal;
        kRay.GetIntersection(kLocation, kNormal);
        GetBlendedMetaData(kLocation, kRay.GetCollidedCell(), 
            kMetaData,
            kBlendedMetaData);
    }

    return bFound;
}

//--------------------------------------------------------------------------------------------------
void NiTerrain::GetBlendedMetaData(const NiPoint3& kIntersectionPt, const NiTerrainCell* pkCell, 
    efd::map<efd::utf8string, NiMetaData>& kMetaData, NiMetaData& kBlendedMetaData) const
{
    GetMetaData(kIntersectionPt, pkCell, kMetaData);

    efd::map<efd::utf8string, NiMetaData>::iterator kIter = kMetaData.begin();
    while (kIter != kMetaData.end())
    {
        kBlendedMetaData.Blend(&kIter->second, kIter->second.GetWeight());
        ++kIter;
    }
}

//--------------------------------------------------------------------------------------------------
NiTerrain::MetaDataVisitor::~MetaDataVisitor()
{
    // Do nothing in the default destructor
}

//--------------------------------------------------------------------------------------------------
