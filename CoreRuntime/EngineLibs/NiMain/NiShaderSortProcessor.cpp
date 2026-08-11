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
#include "NiMainPCH.h"

#include "NiShaderSortProcessor.h"
#include "NiRenderer.h"
#include "NiRenderObject.h"
#include "efd/Profiler.h"

//--------------------------------------------------------------------------------------------------
#define USE_GPU_MARKERS 0

#if USE_GPU_MARKERS
#define MARKER_PUSH(x) EE_PUSH_GPU_MARKER(x)
#define MARKER_POP() EE_POP_GPU_MARKER()
#else
#define MARKER_PUSH(x) {}
#define MARKER_POP() {}
#endif

//--------------------------------------------------------------------------------------------------
EE_PROFILER_CONTEXT_EXTERN(Renderer);
//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiShaderSortProcessor, NiAlphaSortProcessor, NiTypeMask::NiShaderSortProcessor);
//--------------------------------------------------------------------------------------------------
NiShaderSortProcessor::NiShaderSortProcessor(
    NiUInt32 uiBucketSize,
    NiUInt32 uiPrimeMapSize,
    NiUInt32 uiNodePoolSize,
    NiUInt32 uiEffectPoolSize) :
    m_kMeshBuckets(uiBucketSize, uiBucketSize*2),
    m_kNodePool(uiNodePoolSize, uiNodePoolSize*2),
    m_kEffectHeadPool(uiEffectPoolSize, uiEffectPoolSize*2),
    m_bTransparentPass(true)
{
    m_uiInitPrimeMapSize = uiPrimeMapSize;

    for (NiUInt32 uiBucketID = 0; uiBucketID < uiBucketSize; uiBucketID++)
    {
        RenderObjectBucket* pkBucket = NiNew RenderObjectBucket();
        pkBucket->m_pkEffectStateMap =
            NiNew NiTPointerMap<NiDynamicEffectState*, PerEffectBucket*>(uiPrimeMapSize);

        m_kMeshBuckets.SetAt(uiBucketID, pkBucket);
    }

    for (NiUInt32 uiNodeID = 0; uiNodeID < uiNodePoolSize; uiNodeID++)
    {
        m_kNodePool.SetAt(uiNodeID, NiNew RenderObjectNode());
    }

    for (NiUInt32 ui = 0; ui < uiEffectPoolSize; ++ui)
    {
        m_kEffectHeadPool.SetAt(ui, NiNew PerEffectBucket());
    }
}

//--------------------------------------------------------------------------------------------------
NiShaderSortProcessor::~NiShaderSortProcessor()
{
    ReleaseCaches();
}

//--------------------------------------------------------------------------------------------------
void NiShaderSortProcessor::PreRenderProcessList(
    const NiVisibleArray* pkInput, NiVisibleArray& kOutput, void*)
{
    // If the input array pointer is null, do nothing.
    if (!pkInput)
    {
        return;
    }

    // Get renderer pointer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Get camera data from renderer.
    NiPoint3 kWorldLoc, kWorldDir, kWorldUp, kWorldRight;
    NiFrustum kFrustum;
    NiRect<float> kViewport;
    pkRenderer->GetCameraData(kWorldLoc, kWorldDir, kWorldUp, kWorldRight,
        kFrustum, kViewport);

    const unsigned int uiInputCount = pkInput->GetCount();

    // Initialize size of object depths array.
    if (m_uiAllocatedDepths < uiInputCount)
    {
        NiFree(m_pfDepths);
        m_pfDepths = NiAlloc(float, uiInputCount);
        m_uiAllocatedDepths = uiInputCount;
    }

    // Clear the buckets
    ClearBuckets();
    ClearNodePool();
    ClearPerEffectBucketPool();
    m_kNoSortVisibleArray.RemoveAll();

    // Iterate over input array, compute bucket key for each mesh and
    // assign each mesh to the appropriate bucket.
    unsigned int uiDepthIndex = 0;
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);

        if (!AcceptRenderObject(kMesh))
            continue;

        if (!pkRenderer->PrecacheShader(&kMesh))
            continue;

        if (m_bTransparentPass && IsTransparent(kMesh))
        {
            // Render objects with active alpha properties must be
            // sorted separately.
            kOutput.Add(kMesh);

            // Compute and store depth from camera.
            m_pfDepths[uiDepthIndex++] =
                ComputeDepth(kMesh, kWorldLoc, kWorldDir);
            continue;
        }
        else if (!kMesh.GetSortObject())
        {
            // Render objects with active alpha properties must be
            // sorted separately.
            m_kNoSortVisibleArray.Add(kMesh);

            continue;
        }

        NiMaterialInstance* pkSrcMatInst =
            (NiMaterialInstance*)kMesh.GetActiveMaterialInstance();
        if (!pkSrcMatInst || !pkSrcMatInst->GetCachedShader())
        {
            // PrecacheShader() is required to leave a valid material/shader
            // cache entry. Treat a failed renderer/material contract as a
            // non-renderable object instead of inserting a null-shader bucket.
            continue;
        }

        // Find a bucket with a matching NiShader.
        // TODO: mbailey 3/10/09 If this method starts showing up in performance
        // profiles the following linear lookup should be replaced with a hashmap.
        NiUInt32 uiKeyID;
        for (uiKeyID = 0; uiKeyID < m_uiActiveBuckets; uiKeyID++)
        {
            RenderObjectBucket& kBucket = *m_kMeshBuckets.GetAt(uiKeyID);

            if (!kBucket.m_pkMatInstanceKey)
                continue;
            EE_ASSERT(kBucket.m_pkRenderObjectList);
            EE_ASSERT(kBucket.m_pkRenderObjectList->m_pkRenderObject);

            NiMaterialInstance* pkCmpMatInst = kBucket.m_pkMatInstanceKey;
            const NiRenderObject* pkFirst = kBucket.m_pkRenderObjectList->m_pkRenderObject;

            if (pkSrcMatInst->ShaderEquals(pkCmpMatInst) &&
               ((!pkFirst->GetEffectState()) == (!kMesh.GetEffectState())))
            {
                break;
            }
        }

        // Create a new node for the mesh
        RenderObjectNode* pkNode = GetFreeRONode();
        pkNode->m_pkRenderObject = &kMesh;

        if (uiKeyID == m_uiActiveBuckets)
        {
            // First time encountering this type of material instance, create a new
            // bucket+sub-bucket and add the mesh
            pkNode->m_pkNext = NULL;

            // Create a new per-effect sub-bucket for this effect state
            PerEffectBucket* pkSubBucket = GetFreePerEffectBucket();
            pkSubBucket->m_pkHead = pkNode;
            pkSubBucket->m_pkTail = pkNode;

            // Add the effect sub-bucket to a new bucket
            RenderObjectBucket* pkBucket = GetFreeBucket();
            pkBucket->m_pkMatInstanceKey = pkSrcMatInst;
            pkBucket->m_pkRenderObjectList = pkNode;
            pkBucket->m_pkEffectStateMap->SetAt(kMesh.GetEffectState(), pkSubBucket);
        }
        else
        {
            // This material instance has already been seen before, grab it's bucket
            RenderObjectBucket& kBucket = *m_kMeshBuckets.GetAt(uiKeyID);

            // Check if a matching effect state has already been hit.
            PerEffectBucket* pkSubBucket = NULL;
            kBucket.m_pkEffectStateMap->GetAt(kMesh.GetEffectState(), pkSubBucket);

            if (pkSubBucket != NULL)
            {
                // Insert the new node at the end of the sub-bucket list
                RenderObjectNode* pkOldTail = pkSubBucket->m_pkTail;
                pkNode->m_pkNext = pkOldTail->m_pkNext;
                pkOldTail->m_pkNext = pkNode;
                pkSubBucket->m_pkTail = pkNode;
            }
            else
            {
                // It's a new effect state, so we need a new sub-bucket

                // Create a new per-effect sub-bucket for this effect state
                pkSubBucket = GetFreePerEffectBucket();
                pkSubBucket->m_pkHead = pkNode;
                pkSubBucket->m_pkTail = pkNode;
                kBucket.m_pkEffectStateMap->SetAt(kMesh.GetEffectState(), pkSubBucket);

                // Insert this sub-bucket at the front of the linked list for the whole bucket
                pkNode->m_pkNext = kBucket.m_pkRenderObjectList;
                kBucket.m_pkRenderObjectList = pkNode;
            }

        }
    }

    MARKER_PUSH("Sorted");
    // Iterate over buckets
    {
        EE_PROFILER_ZONE(Renderer, 0, "ShaderSortProcessor::RenderSorted");
        for (NiUInt32 uiBucketID = 0; uiBucketID < m_uiActiveBuckets; uiBucketID++)
        {
            RenderObjectBucket& kBucket = *m_kMeshBuckets.GetAt(uiBucketID);
            EE_ASSERT(kBucket.m_pkRenderObjectList);

            NiRenderObject* pkMesh = kBucket.m_pkRenderObjectList->m_pkRenderObject;
            const NiMaterialInstance* pkMatInst = pkMesh->GetActiveMaterialInstance();
            if (!pkMatInst)
            {
                // Failed to produce a shader for this mesh group.
                continue;
            }

            // Fill the visible array
            m_kTempVisibleArray.RemoveAll();
            RenderObjectNode* pkRONode = kBucket.m_pkRenderObjectList;
            while (pkRONode)
            {
                m_kTempVisibleArray.Add(*pkRONode->m_pkRenderObject);
                pkRONode = pkRONode->m_pkNext;
            }

            {
                const NiMaterial* pkMaterial = pkMatInst->GetMaterial();
                NiShader* pkShader = pkMatInst->GetCachedShader();
                if (!pkMaterial || !pkShader)
                {
                    // The material may have been invalidated after precaching.
                    // Never dereference a stale/null shader cache entry.
                    continue;
                }

                EE_PROFILER_ZONE(Renderer, 0, pkMaterial->GetName());
                pkShader->RenderMeshes(&m_kTempVisibleArray);
            }
        }
    }
    MARKER_POP();

    MARKER_PUSH("Unsorted");
    {
        EE_PROFILER_ZONE(Renderer, 0, "ShaderSortProcessor::RenderUnsorted");

        // Render non-sorted objects
        const unsigned int uiNoSortCount = m_kNoSortVisibleArray.GetCount();
        for (unsigned int ui = 0; ui < uiNoSortCount; ui++)
        {
            NiAVObject* pkAVObj = &m_kNoSortVisibleArray.GetAt(ui);
            EE_ASSERT(NiIsKindOf(NiRenderObject, pkAVObj));
            reinterpret_cast<NiRenderObject*>(pkAVObj)->RenderImmediate(pkRenderer);
        }
    }
    MARKER_POP();

    // Sort output array by depth. The output array should only contain
    // transparent objects.
    SortObjectsByDepth(kOutput, 0, kOutput.GetCount() - 1);

    if (m_bTransparentPass)
    {
        EE_PROFILER_ZONE(Renderer, 0, "ShaderSortProcessor::RenderTransparent");

        MARKER_PUSH("Transparent");
        // Immediately render transparent objects now that they've been sorted.  Note that the array
        // is organized such that the "back-most" object appears at the end of the array.
        const unsigned int uiTransparentCount = kOutput.GetCount();
        for (unsigned int ui = uiTransparentCount; ui > 0; ui--)
        {
            NiAVObject* pkAVObj = &kOutput.GetAt(ui - 1);
            EE_ASSERT(NiIsKindOf(NiRenderObject, pkAVObj));
            reinterpret_cast<NiRenderObject*>(pkAVObj)->RenderImmediate(pkRenderer);
        }
        MARKER_POP();
    }

    // Ensure array is clear for next render view.
    m_kNoSortVisibleArray.RemoveAll();
    kOutput.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
void NiShaderSortProcessor::ReleaseCaches()
{
    // Clear out the mesh buckets
    RenderObjectBucket** pkRenderObjectBuckets = m_kMeshBuckets.GetBase();
    for (NiUInt32 ui = 0; ui < m_kMeshBuckets.GetAllocatedSize(); ui++)
    {
        EE_ASSERT(pkRenderObjectBuckets[ui]);

        if (pkRenderObjectBuckets[ui]->m_pkEffectStateMap)
        {
            pkRenderObjectBuckets[ui]->m_pkEffectStateMap->RemoveAll();
            NiDelete pkRenderObjectBuckets[ui]->m_pkEffectStateMap;
        }

        NiDelete pkRenderObjectBuckets[ui];
    }
    m_kMeshBuckets.RemoveAll();
    m_kMeshBuckets.SetSize(0);

    // Clear out the node pool
    RenderObjectNode** pkRenderObjectNodes = m_kNodePool.GetBase();
    for (NiUInt32 uiNodeID = 0; uiNodeID < m_kNodePool.GetAllocatedSize(); uiNodeID++)
    {
        EE_ASSERT(pkRenderObjectNodes[uiNodeID]);
        NiDelete pkRenderObjectNodes[uiNodeID];

    }
    m_kNodePool.RemoveAll();
    m_kNodePool.SetSize(0);

    // Clear out the per-effect head pool
    PerEffectBucket** pkEffectHeads = m_kEffectHeadPool.GetBase();
    for (NiUInt32 ui = 0; ui < m_kEffectHeadPool.GetAllocatedSize(); ++ui)
    {
        EE_ASSERT(pkEffectHeads[ui]);
        NiDelete pkEffectHeads[ui];
    }
    m_kEffectHeadPool.RemoveAll();
    m_kEffectHeadPool.SetSize(0);


    // Empty the temp visible array
    m_kNoSortVisibleArray.RemoveAll();
    m_kNoSortVisibleArray.SetAllocatedSize(0);
    m_kTempVisibleArray.RemoveAll();
    m_kTempVisibleArray.SetAllocatedSize(0);

    NiAlphaSortProcessor::ReleaseCaches();
}

//--------------------------------------------------------------------------------------------------
bool NiShaderSortProcessor::AcceptRenderObject(const NiRenderObject& kMesh) const
{
    return true;
}
