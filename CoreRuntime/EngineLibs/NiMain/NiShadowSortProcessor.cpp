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

#include "NiShadowSortProcessor.h"
#include "NiRenderer.h"
#include "NiRenderObject.h"

NiImplementRTTI(NiShadowSortProcessor, NiMaterialSwapProcessor);

//------------------------------------------------------------------------------------------------
void NiShadowSortProcessor::PreRenderProcessList(
    const NiVisibleArray* pkInput, NiVisibleArray&, void*)
{
    // If the input array pointer is null, do nothing.
    if (!pkInput)
    {
        return;
    }

    // Get renderer pointer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    const unsigned int uiInputCount = pkInput->GetCount();

    // Clear the buckets
    m_kBatchKeys.RemoveAll();

    for (NiUInt32 uiBucketID = 0;
        uiBucketID < m_kMeshBuckets.GetSize();
        uiBucketID++)
    {
        NiVisibleArray* pkVisibleArray = m_kMeshBuckets.GetAt(uiBucketID);
        EE_ASSERT(pkVisibleArray);
        pkVisibleArray->RemoveAll();
    }

    // Iterate over input array, compute bucket key for each mesh and
    // assign each mesh to the appropriate bucket.
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);

        // Backup current active material.
        m_kOldMaterials.Add(kMesh.GetActiveMaterial());

        // Apply and set active material to new material.
        kMesh.ApplyAndSetActiveMaterial(m_spMaterial,
            m_uiMaterialExtraData);

        if (!pkRenderer->PrecacheShader(&kMesh))
            continue;

        NiMaterialInstance* pkSrcMatInst =
            (NiMaterialInstance*)kMesh.GetActiveMaterialInstance();

        NiUInt32 uiKeyID = 0;
        for (uiKeyID = 0; uiKeyID < m_kBatchKeys.GetSize(); uiKeyID++)
        {
            NiMaterialInstance* pkCmpMatInst = m_kBatchKeys.GetAt(uiKeyID);

            if (pkSrcMatInst->ShaderEquals(pkCmpMatInst))
                break;
        }

        // First time encountering this type material instance
        // add it to the bucket
        if (uiKeyID == m_kBatchKeys.GetSize())
        {
            m_kBatchKeys.Add(pkSrcMatInst);
        }

        if (uiKeyID >= m_kMeshBuckets.GetSize())
        {
            NiVisibleArray* pkVisibleArray = NiNew NiVisibleArray();
            m_kMeshBuckets.Add(pkVisibleArray);
            pkVisibleArray->Add(kMesh);
        }
        else
        {
            NiVisibleArray* pkVisibleArray = m_kMeshBuckets.GetAt(uiKeyID);
            EE_ASSERT(pkVisibleArray);
            pkVisibleArray->Add(kMesh);
        }
    }

    // Iterate over buckets and render them
    for (NiUInt32 uiBucketID = 0;
        uiBucketID < m_kMeshBuckets.GetSize();
        uiBucketID++)
    {
        NiVisibleArray* pkVisibleArray = m_kMeshBuckets.GetAt(uiBucketID);
        EE_ASSERT(pkVisibleArray);

        if (pkVisibleArray->GetCount() > 0)
        {
            NiRenderObject& kMesh = pkVisibleArray->GetAt(0);

            const NiMaterialInstance* pkMatInst = kMesh.GetActiveMaterialInstance();
            if (!pkMatInst)
            {
                // Failed to produce a shader for this mesh group.
                continue;
            }

            NiShader* pkShader = pkMatInst->GetCachedShader();
            EE_ASSERT(pkShader);
            pkShader->RenderMeshes(pkVisibleArray);
        }
    }

    // Restore the original material
    for (unsigned int ui = 0; ui < uiInputCount; ui++)
    {
        NiRenderObject& kMesh = pkInput->GetAt(ui);

        kMesh.SetActiveMaterial(m_kOldMaterials.GetAt(ui));
    }

    m_kOldMaterials.RemoveAll();
}

//------------------------------------------------------------------------------------------------
void NiShadowSortProcessor::ReleaseCaches()
{
    for (NiUInt32 uiBucketID = 0;
        uiBucketID < m_kMeshBuckets.GetSize();
        uiBucketID++)
    {
        NiVisibleArray* pkVisibleArray = m_kMeshBuckets.GetAt(uiBucketID);
        EE_ASSERT(pkVisibleArray);
        NiDelete pkVisibleArray;
    }

    m_kBatchKeys.RemoveAll();
    m_kBatchKeys.Realloc();

    m_kMeshBuckets.RemoveAll();
    m_kMeshBuckets.Realloc();

    m_kOldMaterials.RemoveAll();
    m_kOldMaterials.Realloc();
}

//------------------------------------------------------------------------------------------------
NiShadowSortProcessor::~NiShadowSortProcessor()
{
    ReleaseCaches();
}

//------------------------------------------------------------------------------------------------
