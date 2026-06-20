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
#include "NiParticlePCH.h"

#include "NiPoint2.h"
#include "NiPSFacingQuadGenerator.h"
#include "NiPSParticleSystem.h"
#include <NiCommonSemantics.h>
#include "NiPSCommonSemantics.h"
#include <NiSPWorkflow.h>
#include <NiCullingProcess.h>

NiImplementRTTI(NiPSFacingQuadGenerator, NiMeshModifier, NiTypeMask::NiPSFacingQuadGenerator);

const NiUInt32 NiPSFacingQuadGenerator::MAX_SUPPORTED_PARTICLES = UINT_MAX / 6;
const NiUInt16 NiPSFacingQuadGenerator::MAX_PARTICLES_FOR_16BIT_INDICES =
    USHRT_MAX / 4;

//--------------------------------------------------------------------------------------------------
NiPSFacingQuadGenerator::NiPSFacingQuadGenerator() :
    m_kKernelStructIS(&m_kKernelStruct, 1),
    m_sortStructIS(&m_sortStruct, 1),
    m_b32BitIndices(false),
    m_bColors(false),
    m_bAttached(false),
    m_spSortTask(0),
    m_previousParticleCount(0)
{
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::Attach(NiMesh* pkMesh)
{
    AddSubmitSyncPoint(NiSyncArgs::SYNC_VISIBLE);
    AddCompleteSyncPoint(NiSyncArgs::SYNC_RENDER);

    // Get pointer to NiPSParticleSystem.
    EE_ASSERT(NiIsKindOf(NiPSParticleSystem, pkMesh));
    NiPSParticleSystem* pkParticleSystem = (NiPSParticleSystem*) pkMesh;
    EE_ASSERT(pkParticleSystem);

    // Get maximum number of particles.
    const NiUInt32 uiMaxNumParticles = pkParticleSystem->GetMaxNumParticles();

    if (uiMaxNumParticles > MAX_SUPPORTED_PARTICLES)
    {
        NILOG("NiPSFacingQuadGenerator Error: MaxNumParticles is %d, which is "
            "too large to support 6 indices per particle. The mesh modifier "
            "cannot be attached.\n", uiMaxNumParticles);
        return false;
    }
    else if (uiMaxNumParticles == 0)
    {
        NILOG("NiPSFacingQuadGenerator Error: MaxNumParticles is 0. The mesh "
            "modifier will not be attached to a particle system that can "
            "contain no particles.\n");
        return false;
    }

    // Set the primitive type for the particle system.
    pkParticleSystem->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);

    const NiUInt32 uiNumVertices = uiMaxNumParticles * 4;
    const NiUInt32 uiNumIndices = uiMaxNumParticles * 6;

    // Add data stream for vertices.
    NiDataStream* pkDataStream = AddDataStream(
        pkParticleSystem,
        NiCommonSemantics::POSITION(),
        0,
        NiDataStreamElement::F_FLOAT32_3,
        uiNumVertices,
        NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_VOLATILE,
        NiDataStream::USAGE_VERTEX,
        NiObject::CLONE_BLANK_COPY,
        sizeof(NiPoint3));
    m_kVertexOS.SetDataSource(pkDataStream, false);
    m_kVertexOS.SetStride(sizeof(NiPoint3) * 4);

    // Add data stream for indices.
    if (uiMaxNumParticles < MAX_PARTICLES_FOR_16BIT_INDICES)
    {
        // If the number of vertices is less than 65536, 16-bit indices can
        // be used.
        NiUInt16* pusIndices = NiAlloc(NiUInt16, uiNumIndices);
        NiUInt16 usStartingVertex = 0;
        for (NiUInt32 ui = 0; ui + 5 < uiNumIndices; ui += 6)
        {
            pusIndices[ui] = usStartingVertex;
            pusIndices[ui + 1] = usStartingVertex + 1;
            pusIndices[ui + 2] = usStartingVertex + 2;
            pusIndices[ui + 3] = usStartingVertex + 2;
            pusIndices[ui + 4] = usStartingVertex + 3;
            pusIndices[ui + 5] = usStartingVertex;
            usStartingVertex += 4;
        }
        AddDataStream(
            pkParticleSystem,
            NiCommonSemantics::INDEX(),
            0,
            NiDataStreamElement::F_UINT16_1,
            uiNumIndices,
            NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX_INDEX,
            NiObject::CLONE_SHARE,
            sizeof(NiUInt16),
            pusIndices);
        NiFree(pusIndices);
    }
    else
    {
        m_b32BitIndices = true;

        NiUInt32* puiIndices = NiAlloc(NiUInt32, uiNumIndices);
        NiUInt32 uiStartingVertex = 0;
        for (NiUInt32 ui = 0; ui + 5 < uiNumIndices; ui += 6)
        {
            puiIndices[ui] = uiStartingVertex;
            puiIndices[ui + 1] = uiStartingVertex + 1;
            puiIndices[ui + 2] = uiStartingVertex + 2;
            puiIndices[ui + 3] = uiStartingVertex + 2;
            puiIndices[ui + 4] = uiStartingVertex + 3;
            puiIndices[ui + 5] = uiStartingVertex;
            uiStartingVertex += 4;
        }
        AddDataStream(
            pkParticleSystem,
            NiCommonSemantics::INDEX(),
            0,
            NiDataStreamElement::F_UINT32_1,
            uiNumIndices,
            NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX_INDEX,
            NiObject::CLONE_SHARE,
            sizeof(NiUInt32),
            puiIndices);
        NiFree(puiIndices);
    }

    // Add data stream for normals.
    NiDataStream* pkNormals = AddDataStream(
        pkParticleSystem,
        NiCommonSemantics::NORMAL(),
        0,
        NiDataStreamElement::F_FLOAT32_3,
        1,
        NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_VOLATILE,
        NiDataStream::USAGE_VERTEX,
        NiObject::CLONE_BLANK_COPY,
        sizeof(NiPoint3));
    pkNormals->SetGPUConstantSingleEntry(true);

    // Add data stream for texture coordinates.
    NiDataStreamElement::Format eUVFormat = NiDataStreamElement::F_FLOAT32_2;
    NiUInt32 uiUVSize = sizeof(NiPoint2);
    NiPoint2* pkUVs = NiAlloc(NiPoint2, uiNumVertices);
    for (NiUInt32 ui = 0; ui + 3 < uiNumVertices; ui += 4)
    {
        pkUVs[ui] = NiPoint2::ZERO;
        pkUVs[ui + 1] = NiPoint2::UNIT_Y;
        pkUVs[ui + 2] = NiPoint2(1.0f, 1.0f);
        pkUVs[ui + 3] = NiPoint2::UNIT_X;
    }
    AddDataStream(
        pkParticleSystem,
        NiCommonSemantics::TEXCOORD(),
        0,
        eUVFormat,
        uiNumVertices,
        NiDataStream::ACCESS_GPU_READ |
        NiDataStream::ACCESS_CPU_WRITE_STATIC,
        NiDataStream::USAGE_VERTEX,
        NiObject::CLONE_SHARE,
        uiUVSize,
        pkUVs);
    NiFree(pkUVs);

    if (pkParticleSystem->HasColors())
    {
        m_bColors = true;

        // Add data stream for colors.
        pkDataStream = AddDataStream(
            pkParticleSystem,
            NiCommonSemantics::COLOR(),
            0,
            NiDataStreamElement::F_NORMUINT8_4,
            uiNumVertices,
            NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_WRITE_VOLATILE,
            NiDataStream::USAGE_VERTEX,
            NiObject::CLONE_BLANK_COPY,
            sizeof(NiRGBA));
        m_kColorOS.SetDataSource(pkDataStream, false);
        m_kColorOS.SetStride(sizeof(NiRGBA) * 4);
    }

    // create default remap data if we are sorting
    if (pkParticleSystem->IsSorted() && (uiMaxNumParticles <= MAX_SORTED_PARTICLES))
    {
        NiUInt32* pRemapData = NiAlloc(NiUInt32, uiMaxNumParticles);
        for (NiUInt32 i = 0; i < uiMaxNumParticles; i++)
            pRemapData[i] = i;
        // the remap array is input to the optional sort kernel as well as its output, so we have
        // two streams that point to the same data and floodgate can still sort the tasks
        m_remapIS.SetData(pRemapData);
        m_remapOS.SetData(pRemapData);
    }
    else
    {
        m_remapIS.SetData(NULL);
        m_remapOS.SetData(NULL);
    }

    // if sorting is enabled, create a task to update the remap data
    if (pkParticleSystem->IsSorted())
    {
        // warn the user if they are trying to sort more than the maximum sort size
        if (uiMaxNumParticles > MAX_SORTED_PARTICLES)
        {
            NILOG("NiPSFacingQuadGenerator Warning: Particle sorting is enabled on a mesh for "
                "which MaxNumParticles exceeds the recommended count of sorted particles. Sorting "
                "will be disabled.\n");
        }
        else
        {
            // create the sorting floodgate task if it is enabled
            m_spSortTask = NiSPTask::GetNewTask(3, 1);
            m_spSortTask->SetKernel(&m_sortKernel);
            m_spSortTask->AddInput(&m_sortStructIS);
            m_spSortTask->AddInput(&m_kPositionIS);
            m_spSortTask->AddInput(&m_remapIS);
            m_spSortTask->AddOutput(&m_remapOS);
        }
    }

    // Set the number of submeshes on the particle system.
    pkParticleSystem->SetSubmeshCount(1);

    // Create Floodgate task for quad generation
    m_spTask = NiSPTask::GetNewTask(7, 2);
    m_spTask->SetKernel(&m_kKernel);
    m_spTask->AddInput(&m_kKernelStructIS);
    m_spTask->AddInput(&m_kPositionIS);
    m_spTask->AddInput(&m_kRadiusIS);
    m_spTask->AddInput(&m_kSizeIS);
    m_spTask->AddInput(&m_kColorIS);
    m_spTask->AddInput(&m_kRotAngleIS);
    m_spTask->AddInput(&m_remapOS);
    m_spTask->AddOutput(&m_kVertexOS);
    m_spTask->AddOutput(&m_kColorOS);

#ifdef _XENON
    // Set optimal block count for task.
    m_spTask->SetOptimalBlockCount((NiUInt32) ceil(1536.0f /
        m_kColorIS.GetStride()));
#endif  // #ifdef _XENON

    // Associate particle data with Floodgate streams.
    m_kPositionIS.SetData(pkParticleSystem->GetPositions());
    m_kRadiusIS.SetData(pkParticleSystem->GetInitialSizes());
    m_kSizeIS.SetData(pkParticleSystem->GetSizes());
    m_kColorIS.SetData(pkParticleSystem->GetColors());
    m_kRotAngleIS.SetData(pkParticleSystem->GetRotationAngles());

    m_bAttached = true;

    // Update the cached primitive count since POSITION & INDEX data streams
    // were added to the mesh.
    pkMesh->UpdateCachedPrimitiveCount();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::Detach(NiMesh*)
{
    m_spTask = 0;

    m_b32BitIndices = false;
    m_bColors = false;
    m_bAttached = false;

    // we need to free the remapping arrays that we have created
    NiUInt32* pRemapData = (NiUInt32*)m_remapIS.GetData();
    NiFree(pRemapData);
    m_remapIS.SetData(NULL);
    m_remapOS.SetData(NULL);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::SubmitTasks(NiMesh* pkMesh,
    NiSyncArgs* pkArgs, NiSPWorkflowManager* pkWFManager)
{
    EE_ASSERT(pkArgs->m_uiSubmitPoint == NiSyncArgs::SYNC_ANY ||
        pkArgs->m_uiSubmitPoint == NiSyncArgs::SYNC_VISIBLE);

    // If a previous workflow is in flight from another culling operation,
    // finish that workflow before restarting.
    if (m_spWorkflow != NULL)
    {
        CompleteTasks(pkMesh, pkArgs);
    }

    // Get pointer to NiPSParticleSystem.
    EE_ASSERT(NiIsKindOf(NiPSParticleSystem, pkMesh));
    NiPSParticleSystem* pkParticleSystem = (NiPSParticleSystem*) pkMesh;
    EE_ASSERT(pkParticleSystem);

    // Get number of active particles.
    const NiUInt32 uiNumParticles = pkParticleSystem->GetNumParticles();

    // Do not add task if there are no particles.
    if (uiNumParticles == 0)
    {
        return false;
    }
    EE_ASSERT(uiNumParticles < MAX_SUPPORTED_PARTICLES);

    const NiUInt32 uiVertexCount = uiNumParticles * 4;
    const NiUInt32 uiIndexCount = uiNumParticles * 6;

    // Update block count for Floodgate streams to the number of active
    // particles.
    m_kPositionIS.SetBlockCount(uiNumParticles);
    m_kRadiusIS.SetBlockCount(uiNumParticles);
    m_kSizeIS.SetBlockCount(uiNumParticles);
    m_kColorIS.SetBlockCount(uiNumParticles);
    m_kRotAngleIS.SetBlockCount(uiNumParticles);
    m_remapIS.SetBlockCount(uiNumParticles);
    m_remapOS.SetBlockCount(uiNumParticles);

    // Set region range for vertices.
    NiDataStreamRef* pkRef = NULL;
    pkRef = pkMesh->FindStreamRef(NiCommonSemantics::POSITION());
    EE_ASSERT(pkRef);
    pkRef->SetActiveCount(0, uiVertexCount);
    m_kVertexOS.SetBlockCount(uiNumParticles);

    // Set region range for indices.
    pkRef = pkMesh->FindStreamRef(NiCommonSemantics::INDEX());
    EE_ASSERT(pkRef);
    pkRef->SetActiveCount(0, uiIndexCount);

    // Set region range for texture coordinates.
    pkRef = pkMesh->FindStreamRef(NiCommonSemantics::TEXCOORD());
    EE_ASSERT(pkRef);
    pkRef->SetActiveCount(0, uiVertexCount);

    // Set region range for colors.
    pkRef = pkMesh->FindStreamRef(NiCommonSemantics::COLOR());
    if (pkRef)
    {
        pkRef->SetActiveCount(0, uiVertexCount);
        m_kColorOS.SetBlockCount(uiNumParticles);
    }

    // Tell the particle system to update its cached primitive count. This is
    // necessary because the primitive count is changing each frame.
    pkMesh->UpdateCachedPrimitiveCount();

    // Get current camera.
    EE_ASSERT(pkArgs->m_uiSubmitPoint == NiSyncArgs::SYNC_VISIBLE);
    NiCullingSyncArgs* pkCullingArgs = (NiCullingSyncArgs*) pkArgs;
    const NiCamera* pkCamera = pkCullingArgs->m_kCullingProcess.GetCamera();
    EE_ASSERT(pkCamera);

    // Compute model-space camera parameters.
    const NiMatrix3& kWorldRotate = pkParticleSystem->GetWorldRotate();
    NiPoint3 kModelCamUp = pkCamera->GetWorldUpVector() * kWorldRotate;
    NiPoint3 kModelCamRight = pkCamera->GetWorldRightVector() * kWorldRotate;

    // Compute and set normal vector for each quad.
    m_kKernelStruct.m_kNormal = kModelCamRight.Cross(kModelCamUp);

    // Set camera parameters.
    m_kKernelStruct.m_kCamUp = kModelCamUp;
    m_kKernelStruct.m_kCamRight = kModelCamRight;

    // If we are also sorting, prepare the struct for that
    if (m_spSortTask)
    {
        NiTransform invWorld;
        pkParticleSystem->GetWorldTransform().Invert(invWorld);
        m_sortStruct.m_camPos = invWorld * pkCamera->GetWorldTranslate();
        m_sortStruct.m_normal = m_kKernelStruct.m_kNormal;

        // we have to do some maintenance on the remapping array if our particle count changed
        NiUInt32* pRemapData = (NiUInt32*)m_remapIS.GetData();
        // if particles were added, make sure every referenced index in the remap array is valid
        for (NiUInt32 ui = m_previousParticleCount; ui < uiNumParticles; ui++)
            pRemapData[ui] = ui;
        // if particles were removed, we have to find entries referencing invalid indices and 
        // replace them with the valid indices that otherwise wouldn't have been referenced due
        // to the smaller uiNumParticles
        if (m_previousParticleCount > uiNumParticles)
        {
            // point to the end of our previous remap array (for finding valid particles that got
            // bumped out) and use this index to count back until we are pointing to the valid
            // portion of the array, at which point we will know we've copied all of the valid
            // out of bounds indices back in bounds
            NiUInt32 cutoffIndex = m_previousParticleCount - 1;
            NiUInt32 ui = 0;
            // we only need to iterate until we've found the number of bogus indices we know exist
            while (cutoffIndex != uiNumParticles - 1)
            {
                // if we've found a bogus index, swap it for the last entry in the previous remap
                if (pRemapData[ui] >= uiNumParticles)
                {
                    pRemapData[ui] = pRemapData[cutoffIndex];
                    // if we swapped in an out of bounds index, don't swap that index in again
                    cutoffIndex--;
                }
                else
                {
                    // only increment our counter if we didn't perform a swap
                    ui++;
                }
            }
        }
        // our remap array should represent every index between 0 and uiNumParticles now
        // remember the size so we can check for adds/removes next time
        m_previousParticleCount = uiNumParticles;

        // sorting must operate on the entire stream, not be divided into chunks
        m_spSortTask->SetOptimalBlockCount(uiNumParticles);
        // this goes for quad generation as well
        m_spTask->SetOptimalBlockCount(uiNumParticles);
    }

    NiUInt32 uiTaskGroup = NiSyncArgs::GetTaskGroupID(
        NiSyncArgs::SYNC_VISIBLE, NiSyncArgs::SYNC_RENDER);
    m_spWorkflow = pkWFManager->AddRelatedTask(m_spTask, uiTaskGroup, false);
    if (m_spSortTask)
        m_spWorkflow->Add(m_spSortTask);

    // Set the normal explicitly, since there's only one piece of data and
    // there's no need to send it through NiFloodgate.
    {
        NiDataStreamRef* pkNormRef =
            pkMesh->FindStreamRef(NiCommonSemantics::NORMAL());
        EE_ASSERT(pkNormRef);

        NiDataStream* pkNorm = pkNormRef->GetDataStream();
        EE_ASSERT(pkNorm);

        NiPoint3* pkData = (NiPoint3*)pkNorm->Lock(NiDataStream::LOCK_WRITE);
        EE_ASSERT(pkData);

        pkData[0] = m_kKernelStruct.m_kNormal;

        pkNorm->Unlock(NiDataStream::LOCK_WRITE);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::CompleteTasks(NiMesh* pkMesh, NiSyncArgs* pkArgs)
{
    if (m_spWorkflow)
    {
        NiStreamProcessor::Get()->Wait(m_spWorkflow);
        m_spWorkflow = 0;
    }

    // Note: The INDEX and TEXCOORD streams were created as CLONE_SHARE
    // Any particle systems cloned from an original will share these streams,
    // but the active count of these streams needs to be different per
    // clone.  Therefore, the active count is reset here, just before the
    // system is rendered.
    if (pkArgs->m_uiCompletePoint == NiSyncArgs::SYNC_RENDER)
    {
        // Get the count of active particles
        NiPSParticleSystem* pkParticles =
            NiVerifyStaticCast(NiPSParticleSystem, pkMesh);
        const NiUInt32 uiNumParticles = pkParticles->GetNumParticles();

        // Set the region range for indices
        const NiUInt32 uiIndexCount = uiNumParticles * 6;
        NiDataStreamRef* pkIndexRef =
            pkMesh->FindStreamRef(NiCommonSemantics::INDEX());
        EE_ASSERT(pkIndexRef);
        pkIndexRef->SetActiveCount(0, uiIndexCount);

        // Set the region range for texure coordinates
        const NiUInt32 uiVertexCount = uiNumParticles * 4;
        NiDataStreamRef* pkTCRef =
            pkMesh->FindStreamRef(NiCommonSemantics::TEXCOORD());
        EE_ASSERT(pkTCRef);
        pkTCRef->SetActiveCount(0, uiVertexCount);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::IsComplete(NiMesh* /*pkMesh*/, NiSyncArgs* /*pkArgs*/)
{
    if (m_spWorkflow != NULL)
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::AreRequirementsMet(
    NiMesh* pkMesh, NiSystemDesc::RendererID) const
{
    if (!NiIsKindOf(NiPSParticleSystem, pkMesh))
    {
        return false;
    }
    NiPSParticleSystem* pkParticleSystem = (NiPSParticleSystem*) pkMesh;

    const bool b16BitIndices = (pkParticleSystem->GetMaxNumParticles() <
        MAX_PARTICLES_FOR_16BIT_INDICES);

    const NiDataStreamRef* pkRef = pkMesh->FindStreamRef(
        NiCommonSemantics::INDEX());
    if (pkRef && !ValidateStream(pkRef,
        NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_CPU_WRITE_STATIC,
        NiDataStream::USAGE_VERTEX_INDEX,
        NiObject::CLONE_SHARE,
        b16BitIndices ? NiDataStreamElement::F_UINT16_1 :
            NiDataStreamElement::F_UINT32_1))
    {
        NILOG("NiPSFacingQuadGenerator> Indices are wrong format.\n");
        return false;
    }

    pkRef = pkMesh->FindStreamRef(NiCommonSemantics::POSITION());
    if (pkRef && !ValidateStream(pkRef,
        NiDataStream::ACCESS_GPU_READ |
        NiDataStream::ACCESS_CPU_WRITE_VOLATILE,
        NiDataStream::USAGE_VERTEX,
        NiObject::CLONE_BLANK_COPY,
        NiDataStreamElement::F_FLOAT32_3))
    {
        NILOG("NiPSFacingQuadGenerator> Positions are wrong format.\n");
        return false;
    }

    pkRef = pkMesh->FindStreamRef(NiCommonSemantics::NORMAL());
    if (pkRef && !ValidateStream(pkRef,
        NiDataStream::ACCESS_GPU_READ |
        NiDataStream::ACCESS_CPU_WRITE_VOLATILE,
        NiDataStream::USAGE_VERTEX,
        NiObject::CLONE_BLANK_COPY,
        NiDataStreamElement::F_FLOAT32_3))
    {
        NILOG("NiPSFacingQuadGenerator> Normals are wrong format.\n");
        return false;
    }

    pkRef = pkMesh->FindStreamRef(NiCommonSemantics::TEXCOORD());
    if (pkRef && !ValidateStream(pkRef,
        NiDataStream::ACCESS_GPU_READ |
        NiDataStream::ACCESS_CPU_WRITE_STATIC,
        NiDataStream::USAGE_VERTEX,
        NiObject::CLONE_SHARE,
        NiDataStreamElement::F_FLOAT32_2))
    {
        NILOG("NiPSFacingQuadGenerator> TexCoords are wrong format.\n");
        return false;
    }


    pkRef = pkMesh->FindStreamRef(NiCommonSemantics::COLOR());
    if (pkRef &&
        !(ValidateStream(pkRef,
        NiDataStream::ACCESS_GPU_READ |
        NiDataStream::ACCESS_CPU_WRITE_VOLATILE,
        NiDataStream::USAGE_VERTEX,
        NiObject::CLONE_BLANK_COPY,
        NiDataStreamElement::F_NORMUINT8_4) ||

        ValidateStream(pkRef,
        NiDataStream::ACCESS_GPU_READ |
        NiDataStream::ACCESS_CPU_WRITE_VOLATILE,
        NiDataStream::USAGE_VERTEX,
        NiObject::CLONE_BLANK_COPY,
        NiDataStreamElement::F_NORMUINT8_4_BGRA)))
    {
        NILOG("NiPSFacingQuadGenerator> Colors are wrong format.\n");
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::ValidateStream(const NiDataStreamRef* pkRef,
    NiUInt8 uiAccessMask, NiDataStream::Usage eUsage,
    NiObject::CloningBehavior eCloningBehavior,
    NiDataStreamElement::Format eFormat)
{
    EE_ASSERT(pkRef);
    return pkRef->GetUsage() == eUsage &&
        pkRef->GetAccessMask() & uiAccessMask &&
        pkRef->GetElementDescCount() == 1 &&
        pkRef->GetElementDescAt(0).GetFormat() == eFormat &&
        pkRef->GetDataStream() != NULL &&
        pkRef->GetDataStream()->GetCloningBehavior() == eCloningBehavior;
}

//--------------------------------------------------------------------------------------------------
void NiPSFacingQuadGenerator::RetrieveRequirements(NiMeshRequirements&
    kRequirements) const
{
    for (NiUInt32 ui = 0; ui < 2; ui++)
    {
        NiUInt32 uiSet = kRequirements.CreateNewRequirementSet();

        // Add INDEX requirement.
        kRequirements.AddRequirement(uiSet, NiCommonSemantics::INDEX(), 0,
            NiMeshRequirements::STRICT_INTERLEAVE, 0, NiDataStream::
            USAGE_VERTEX_INDEX, NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC, m_b32BitIndices ?
            NiDataStreamElement::F_UINT32_1 : NiDataStreamElement::F_UINT16_1);

        // Add POSITION requirement.
        kRequirements.AddRequirement(uiSet, NiCommonSemantics::POSITION(), 0,
            NiMeshRequirements::STRICT_INTERLEAVE, 1, NiDataStream::
            USAGE_VERTEX, NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_VOLATILE, NiDataStreamElement::
            F_FLOAT32_3);

        // Add NORMAL requirement.
        kRequirements.AddRequirement(uiSet, NiCommonSemantics::NORMAL(), 0,
            NiMeshRequirements::STRICT_INTERLEAVE, 2, NiDataStream::
            USAGE_VERTEX, NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_VOLATILE, NiDataStreamElement::
            F_FLOAT32_3);

        // Add TEXCOORD requirement.
        kRequirements.AddRequirement(uiSet, NiCommonSemantics::TEXCOORD(), 0,
            NiMeshRequirements::STRICT_INTERLEAVE, 3, NiDataStream::
            USAGE_VERTEX, NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStreamElement::F_FLOAT32_2);

        // Add COLOR requirement.
        if (ui == 0)
        {
            NiUInt32 uiReqIndex = kRequirements.AddRequirement(uiSet, NiCommonSemantics::COLOR(), 0,
                NiMeshRequirements::STRICT_INTERLEAVE, 4, NiDataStream::
                USAGE_VERTEX, NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_WRITE_VOLATILE, NiDataStreamElement::F_UNKNOWN);

            NiMeshRequirements::SemanticRequirement* pkColorReq = kRequirements.GetRequirement(uiSet, uiReqIndex);
            pkColorReq->m_kFormats.Add(NiDataStreamElement::F_NORMUINT8_4);
            pkColorReq->m_kFormats.Add(NiDataStreamElement::F_NORMUINT8_4_BGRA);
        }
    }
}

//--------------------------------------------------------------------------------------------------
NiDataStream* NiPSFacingQuadGenerator::AddDataStream(
    NiMesh* pkMesh,
    const NiFixedString& kSemantic,
    NiUInt32 uiSemanticIndex,
    NiDataStreamElement::Format eFormat,
    NiUInt32 uiCount,
    NiUInt8 uiAccessMask,
    NiDataStream::Usage eUsage,
    NiObject::CloningBehavior eCloningBehavior,
    size_t stElementSize,
    const void* pvData)
{
    EE_UNUSED_ARG(stElementSize);
    EE_ASSERT(pkMesh);

    NiDataStream* pkDataStream = NULL;

    NiDataStreamRef* pkStreamRef = pkMesh->FindStreamRef(kSemantic,
        uiSemanticIndex, eFormat);
    if (pkStreamRef)
    {
        pkDataStream = pkStreamRef->GetDataStream();
        EE_ASSERT(pkDataStream);

        // Assert that the stream found matches the one being asked for.
        EE_ASSERT(pkDataStream->GetAccessMask() & uiAccessMask);
        EE_ASSERT(pkDataStream->GetUsage() == eUsage);
        EE_ASSERT(pkDataStream->GetSize() == uiCount * stElementSize);
        EE_ASSERT(pkDataStream->GetCloningBehavior() == eCloningBehavior);
    }
    else
    {
        // Create data stream.
        pkDataStream = NiDataStream::CreateSingleElementDataStream(eFormat,
            uiCount, uiAccessMask, eUsage, pvData);
        EE_ASSERT(pkDataStream);
        pkDataStream->SetCloningBehavior(eCloningBehavior);

        // Create stream ref.
        pkStreamRef = pkMesh->AddStreamRef(pkDataStream, kSemantic,
            uiSemanticIndex);
        EE_ASSERT(pkStreamRef);
        pkStreamRef->BindRegionToSubmesh(0, 0);
        pkStreamRef->SetActiveCount(0, 0);
    }

    EE_ASSERT(pkDataStream);

    pkDataStream->SetStreamable(false);

    return pkDataStream;
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSFacingQuadGenerator);

//--------------------------------------------------------------------------------------------------
void NiPSFacingQuadGenerator::CopyMembers(
    NiPSFacingQuadGenerator* pkDest,
    NiCloningProcess& kCloning)
{
    NiMeshModifier::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSFacingQuadGenerator);

//--------------------------------------------------------------------------------------------------
void NiPSFacingQuadGenerator::LoadBinary(NiStream& kStream)
{
    NiMeshModifier::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSFacingQuadGenerator::LinkObject(NiStream& kStream)
{
    NiMeshModifier::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::RegisterStreamables(NiStream& kStream)
{
    return NiMeshModifier::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSFacingQuadGenerator::SaveBinary(NiStream& kStream)
{
    NiMeshModifier::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSFacingQuadGenerator::IsEqual(NiObject* pkObject)
{
    return NiMeshModifier::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSFacingQuadGenerator::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiMeshModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSFacingQuadGenerator::ms_RTTI
        .GetName()));
}

//--------------------------------------------------------------------------------------------------
