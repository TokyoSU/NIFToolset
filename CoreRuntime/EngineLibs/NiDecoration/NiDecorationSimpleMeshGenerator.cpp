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
#include "NiDecorationSimpleMeshGenerator.h"
#include "NiDecorationMaterial.h"

#include <efd/efdLogIDs.h>

#include <NiMesh.h>
#include <NiSkinningMeshModifier.h>

NiImplementRTTI(NiDecorationSimpleMeshGenerator, NiDecorationGenerator, NiTypeMask::NiDecorationSimpleMeshGenerator);

NiFixedString NiDecorationSimpleMeshGenerator::GENERATOR_NAME = NULL;

NiFixedString NiDecorationSimpleMeshGenerator::ms_kErrorSubmeshCount;
NiFixedString NiDecorationSimpleMeshGenerator::ms_kErrorSkinnedMesh;

//------------------------------------------------------------------------------------------------
NiDecorationSimpleMeshGenerator::NiDecorationSimpleMeshGenerator() :
    NiDecorationGenerator(true),
    m_spTransDataStream(NULL)
{
}

//------------------------------------------------------------------------------------------------
NiDecorationSimpleMeshGenerator::~NiDecorationSimpleMeshGenerator()
{
}

//------------------------------------------------------------------------------------------------
const NiFixedString& NiDecorationSimpleMeshGenerator::GetGeneratorType()
{
    return GENERATOR_NAME;
}

//------------------------------------------------------------------------------------------------
void NiDecorationSimpleMeshGenerator::_SDMInit()
{
    GENERATOR_NAME = "Simple Mesh Generator";

    ms_kErrorSubmeshCount = "Mesh must contain exactly 1 submesh.";
    ms_kErrorSkinnedMesh = "Cannot apply hardware instancing to a mesh with skinned animation.";
}

//------------------------------------------------------------------------------------------------
void NiDecorationSimpleMeshGenerator::_SDMShutdown()
{
    GENERATOR_NAME = NULL;

    ms_kErrorSubmeshCount = NULL;
    ms_kErrorSkinnedMesh = NULL;
}

//------------------------------------------------------------------------------------------------
void NiDecorationSimpleMeshGenerator::Initialize()
{
}

//------------------------------------------------------------------------------------------------
void NiDecorationSimpleMeshGenerator::InitializeTransformStream(NiUInt32 uiInstancesPerCell, 
    NiUInt32 uiRequiredCells) 
{
    NiUInt32 uiMaxInstances = uiInstancesPerCell * uiRequiredCells;

    if (m_spTransformStreamManager == NULL)
        m_spTransformStreamManager = EE_NEW NiDecorationTransformManager();

    if (m_spTransformStreamManager->GetInstancesPerCell() != uiInstancesPerCell ||
        m_spTransformStreamManager->GetNumMaxCells() != uiRequiredCells)
    {
        if (m_bUseInstancing)
        {
            // Create the element set to be used the be transform data stream(s).
            NiDataStreamElementSet kElementSet;
            kElementSet.AddElement(NiDataStreamElement::F_FLOAT32_4);
            kElementSet.AddElement(NiDataStreamElement::F_FLOAT32_4);
            kElementSet.AddElement(NiDataStreamElement::F_FLOAT32_4);

            NiFixedString kSemantic;
            NiUInt8 uiAccessMask = NiDataStream::ACCESS_CPU_WRITE_MUTABLE |
                NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_CPU_READ;
            NiDataStream::Usage eUsage = NiDataStream::USAGE_VERTEX;

            // Create the primary transform data stream.
            m_spTransDataStream =
                NiDataStream::CreateDataStream(kElementSet, uiMaxInstances,
                uiAccessMask, eUsage, false);
            EE_ASSERT(m_spTransDataStream);
        }
        else
        {
            m_spTransDataStream = NULL;
        }

        // Need to initialize the transform manager
        m_spTransformStreamManager->SetInstanceStream(m_spTransDataStream, 
            uiRequiredCells, uiInstancesPerCell);
    }
}

//------------------------------------------------------------------------------------------------
NiDecorationMeshInfo* NiDecorationSimpleMeshGenerator::CreateBaseMesh(NiUInt32 uiNumCells, 
    NiUInt32 uiInstancesPerCell)
{
    EE_UNUSED_ARG(uiNumCells);
    EE_UNUSED_ARG(uiInstancesPerCell);

    NiDecorationMeshInfo* pkMeshInfo = NULL;
    NiFixedString kErrorMessage = "No NiMesh objects contained in stream.";
    NiFixedString kObjectName = m_kObjectName.c_str();

    // Attempt to load the resource file
    NiStream kStream;
    if (!kStream.Load(GetMeshPath().c_str()))
    {
        EE_LOG(efd::kApp, efd::ILogger::kERR0, 
            ("Failed to load stream at '%s'", GetMeshPath().c_str()));
        return NULL;
    }

    NiUInt32 uiObjectCount = kStream.GetObjectCount();
    if (uiObjectCount == 0)
        return NULL;

    // Find the first NiMesh in the scene
    NiObject* pkObject = NULL;
    for (NiUInt32 ui = 0; ui < uiObjectCount; ++ui)
    {
        pkObject = kStream.GetObjectAt(ui);

        NiMesh* pkMesh = NULL;

        if (NiIsKindOf(NiAVObject, pkObject))
        {
            NiAVObject* pkAVObject = (NiAVObject*)pkObject;

            // Find the mesh by name, or hunt for best?
            if (kObjectName.GetLength() != 0)
            {
                pkMesh = NiDynamicCast(NiMesh, pkAVObject->GetObjectByName(kObjectName));
            }
            else
            {
                pkMesh = FindNiMesh((NiAVObject*)pkObject);
            }
        }

        // Is the mesh compatible?
        if (pkMesh != NULL && IsMeshCompatible(pkMesh, kErrorMessage))
        {
            // Clone the mesh
            pkMesh = NiVerifyStaticCast(NiMesh, pkMesh->Clone());

            // Is there a time controller?
            NiTimeController* pkControllers = pkMesh->GetControllers();
            if (pkControllers)
            {
                // Force animation to loop.
                pkControllers->SetCycleType(NiTimeController::LOOP);
            }

            pkMeshInfo = EE_NEW NiDecorationMeshInfo(pkMesh);
            break;
        }
    }

    if (!pkMeshInfo)
    {
        EE_LOG(efd::kApp, efd::ILogger::kERR0, ("Failed to load mesh from steam at '%s'. "
            "Last error: (%s)", GetMeshPath().c_str(), (const char*)kErrorMessage));

        return NULL;
    }

    /*
     * Decoration Material
     */
    NiMesh* pkMesh = pkMeshInfo->GetMesh();
    NiDecorationMaterial* pkMaterial = NiDecorationMaterial::Create();
    pkMesh->ApplyAndSetActiveMaterial(pkMaterial);
    NiFloatExtraData* pkExtraData;
    pkExtraData = NiNew NiFloatExtraData(NiSqr(0.0f));
    pkMesh->AddExtraData("g_FadeMinDistSqr", pkExtraData);
    pkExtraData = NiNew NiFloatExtraData(NiSqr(1.0f));
    pkMesh->AddExtraData("g_FadeMaxDistSqr", pkExtraData); 

    /*
     * Enable instancing
     */
    // TODO: Experiment with disabling CPU READ

    // Set up instancing using a fake instance stream which we will later replace
    bool bRes = NiInstancingUtilities::EnableMeshInstancing(pkMesh, 2, NULL, 0, 
        65536, false, true);
    if (!bRes)
    {
        EE_LOG(efd::kApp, efd::ILogger::kERR0, 
            ("Failed to enabled instancing on mesh '%s' in '%s'.", 
                (const char*)pkMesh->GetName(), GetMeshPath().c_str())
             );

        EE_DELETE(pkMeshInfo);

        return NULL;
    }

    return pkMeshInfo;
}

//------------------------------------------------------------------------------------------------
void NiDecorationSimpleMeshGenerator::UpdatePropertyData(NiDecorationMeshInfo* pkBase)
{
    NiMesh* pkBaseMesh = pkBase->GetMesh();
    EE_ASSERT(pkBaseMesh);

    // Name a texturing property accordingly    
    NiTexturingProperty* pkTexturingProperty = 
        (NiTexturingProperty*)pkBaseMesh->GetProperty(NiProperty::TEXTURING);

    // No texturing property created?
    if (pkTexturingProperty)
        pkTexturingProperty->SetName(DEFAULT_TEXTURING_PROPERTY_NAME);

    // Call the super function after we add our properties
    NiDecorationGenerator::UpdatePropertyData(pkBase);
}

//------------------------------------------------------------------------------------------------
bool NiDecorationSimpleMeshGenerator::IsMeshCompatible(const NiMesh* pkMesh,
    NiFixedString& kErrorMessage)
{
    // Must have exactly 1 submesh
    if (pkMesh->GetSubmeshCount() != 1)
    {
        kErrorMessage = ms_kErrorSubmeshCount;
        return false;
    }

    // Don't support skinned meshes (because gamebryo doesn't!)
    NiSkinningMeshModifier* pkSkinningMeshModifier = NiDynamicCast(NiSkinningMeshModifier, 
        pkMesh->GetModifierByType(&NiSkinningMeshModifier::ms_RTTI));
    if (pkSkinningMeshModifier != NULL)
    {
        kErrorMessage = ms_kErrorSkinnedMesh;
        return false;
    }
    
    kErrorMessage = NULL;
    return true;
}

//------------------------------------------------------------------------------------------------
NiMesh* NiDecorationSimpleMeshGenerator::FindNiMesh(NiAVObject* pkObject)
{
    NiMesh* pkMesh = NULL;
    if (NiIsKindOf(NiNode, pkObject))
    {
        NiNode* pkNode = ((NiNode*)pkObject);
        for (NiUInt32 ui = 0; ui < pkNode->GetArrayCount(); ++ui)
        {
            pkMesh = FindNiMesh(pkNode->GetAt(ui));
            if (pkMesh != NULL)
                break;
        }
    }
    else if (NiIsKindOf(NiMesh, pkObject))
    {
        // Only use the mesh if it is visible
        if (!pkObject->GetAppCulled())
            pkMesh = (NiMesh*)pkObject;
    }
    
    return pkMesh;
}

//------------------------------------------------------------------------------------------------
void NiDecorationSimpleMeshGenerator::UpdateAnimation(NiDecorationMeshInfo* pkBase, 
    NiUpdateProcess& kUpdateProcess)
{
    NiMesh* pkMesh = pkBase->GetMesh();
    if (pkMesh)
        pkMesh->UpdateControllers(kUpdateProcess.GetTime());
}

//------------------------------------------------------------------------------------------------
void NiDecorationSimpleMeshGenerator::SetTransforms(NiDecorationMeshInfo* pkBase, 
    NiTransform* pkTransforms, NiUInt32 uiNumCells, NiUInt32 uiStartCell)
{
    NiMesh* pkBaseMesh = pkBase->GetMesh();
    NiUInt32 uiInstancesPerCell = m_spTransformStreamManager->GetInstancesPerCell();

    EE_ASSERT (GetUseInstancing(pkBase) == true);

    bool bRes = NiInstancingUtilities::SetInstanceTransformations(
        pkBaseMesh, 
        pkTransforms, 
        uiNumCells * uiInstancesPerCell, 
        uiStartCell * uiInstancesPerCell);

    // Force upload of new instances
    NiInstancingUtilities::SetActiveInstanceCount(pkBaseMesh,
        NiInstancingUtilities::GetActiveInstanceCount(pkBaseMesh));

    EE_UNUSED_ARG(bRes);
    EE_ASSERT(bRes);
}

//------------------------------------------------------------------------------------------------
void NiDecorationSimpleMeshGenerator::SetActiveInstanceCount(NiDecorationMeshInfo* pkBase, 
    NiUInt32 uiInstanceCount)
{
    EE_ASSERT (GetUseInstancing(pkBase) == true);

    // Work out the offset from the beginning of the transform stream
    NiUInt32 uiOffset;
    if (!m_spTransformStreamManager->GetFirstCellIndex(pkBase, uiOffset))
        uiOffset = 0;
    uiOffset *= m_spTransformStreamManager->GetInstancesPerCell();

    // Our transform manager has already taken care of the region sizes.
    EE_UNUSED_ARG(uiInstanceCount);
    EE_UNUSED_ARG(uiOffset);
    EE_ASSERT(uiOffset + uiInstanceCount <= m_spTransformStreamManager->GetTransformCount());
}

//------------------------------------------------------------------------------------------------
bool NiDecorationSimpleMeshGenerator::GetUseInstancing(NiDecorationMeshInfo* pkBase) const
{
    // We need to override this function to force a user to recreate the
    // base mesh after toggling instancing for changes to take effect.
    if (pkBase && pkBase->GetMesh())
        return pkBase->GetMesh()->GetInstanced();
    else
        return true;
}

//------------------------------------------------------------------------------------------------
