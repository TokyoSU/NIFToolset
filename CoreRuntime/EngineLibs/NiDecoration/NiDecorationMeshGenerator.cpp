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
#include "NiDecorationMeshGenerator.h"
#include "NiDecorationMaterial.h"

NiImplementRTTI(NiDecorationMeshGenerator, NiDecorationGenerator);

//------------------------------------------------------------------------------------------------
NiDecorationMeshGenerator::NiDecorationMeshGenerator(bool bUseInstancing) :
    NiDecorationGenerator(bUseInstancing),
    m_uiInstancesPerCell(0),
    m_uiIndicesPerMesh(0),
    m_uiVerticesPerMesh(0)
{
}

//------------------------------------------------------------------------------------------------
NiDecorationMeshGenerator::~NiDecorationMeshGenerator()
{
}

//------------------------------------------------------------------------------------------------
void NiDecorationMeshGenerator::InitializeTransformStream(NiUInt32 uiInstancesPerCell, 
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
bool NiDecorationMeshGenerator::GetUseInstancing(NiAVObject* pkBase) const
{
    NiMesh* pkBaseMesh = NiDynamicCast(NiMesh, pkBase);

    // We need to override this function to force a user to recreate the
    // base mesh after toggling instancing for changes to take effect.
    if (pkBaseMesh)
        return pkBaseMesh->GetInstanced();
    else
        return m_bUseInstancing;
}

//------------------------------------------------------------------------------------------------
NiDecorationMeshInfo* NiDecorationMeshGenerator::CreateBaseMesh(NiUInt32 uiNumCells, 
    NiUInt32 uiInstancesPerCell)
{
    NiUInt32 uiMaxInstances = uiNumCells * uiInstancesPerCell;
    m_uiInstancesPerCell = uiInstancesPerCell;

    NiDataStreamElementSet kVertexElements;
    NiDataStreamElement::Format eIndexFormat;

    NiDataStreamRef* pkVertexStreamRef = NULL;
    NiDataStreamRef* pkIndexStreamRef = NULL;
    NiDataStreamRef* pkVertexStaticStreamRef = NULL;
    NiDataStream* pkVertexStream = NULL;
    NiDataStream* pkIndexStream = NULL;
    NiDataStream* pkVertexStaticStream = NULL;
    NiDataStreamElementLock kLock;

    // Begin creation of the base mesh
    NiMesh* pkBaseMesh = NiNew NiMesh();
    pkBaseMesh->SetSubmeshCount(1);
    pkBaseMesh->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);
    pkBaseMesh->SetName("Decoration Layer Mesh");

    // It is important that the ordering of this remains consistent with the
    // STREAMREF_ID enum.
    pkVertexStreamRef = pkBaseMesh->AddStreamRef();
    pkIndexStreamRef = pkBaseMesh->AddStreamRef();
    if (!m_bUseInstancing)
        pkVertexStaticStreamRef = pkBaseMesh->AddStreamRef();

    InitializeVertexElements(kVertexElements);

    /*
     * Create vertex stream(s)
     */
    {
        NiDataStreamElementSet kTransformableVertexElements;
        NiDataStreamElementSet kStaticVertexElements;

        NiUInt32 uiNumVertices = GetVerticesPerMesh();

        if (m_bUseInstancing)
        {
            kTransformableVertexElements = kVertexElements;
        }
        else
        {
            // Sort the elements
            NiFixedString kSemantic;
            NiUInt32 uiSemanticIndex;
            bool bRequiresTransform;
            for (NiUInt32 ui = 0; ui < kVertexElements.GetSize(); ++ui)
            {
                NiDataStreamElement& kElement = kVertexElements.GetAt(ui);

                GetVertexElementSemantic(ui, kSemantic, uiSemanticIndex);
                GetVertexElementRequiresTransform(kSemantic, uiSemanticIndex, bRequiresTransform);

                // Are we static or transformable?
                if (bRequiresTransform)
                    kTransformableVertexElements.AddElement(kElement.GetFormat());
                else
                    kStaticVertexElements.AddElement(kElement.GetFormat());
            }

            // If we have some static elements, we will need to create an 
            // additional stream
            if (kStaticVertexElements.GetSize())
            {
                pkVertexStaticStream = NiDataStream::CreateDataStream(
                    kStaticVertexElements, uiNumVertices * uiInstancesPerCell,
                    NiDataStream::ACCESS_CPU_WRITE_MUTABLE |
                    NiDataStream::ACCESS_GPU_READ,
                    NiDataStream::USAGE_VERTEX);

                pkVertexStaticStreamRef->SetDataStream(pkVertexStaticStream);

                if (!pkVertexStaticStream)
                {
                    NiDelete (pkBaseMesh);
                    return NULL;
                }
            }
            else
            {
                pkBaseMesh->RemoveStreamRef(pkVertexStaticStreamRef);
                pkVertexStaticStreamRef = NULL;
            }

            uiNumVertices *= uiMaxInstances;
        }

        pkVertexStream = NiDataStream::CreateDataStream(
            kTransformableVertexElements, uiNumVertices,
            NiDataStream::ACCESS_CPU_WRITE_MUTABLE |
            NiDataStream::ACCESS_GPU_READ,
            NiDataStream::USAGE_VERTEX);

        if (!pkVertexStream)
        {
            NiDelete (pkBaseMesh);
            return NULL;
        }

        pkVertexStreamRef->SetDataStream(pkVertexStream);
    }

    /*
     * Create index stream
     */
    {
        // How many bits will we need per index?
        NiUInt32 uiVerticesPerCell = GetVerticesPerMesh();
        if (!m_bUseInstancing)
            uiVerticesPerCell *= uiInstancesPerCell;

        if (uiVerticesPerCell <= UCHAR_MAX)
            // 8 bit index buffers are not supported by DirectX9.
            eIndexFormat = NiDataStreamElement::F_UINT16_1;
        else if (uiVerticesPerCell <= USHRT_MAX)
            eIndexFormat = NiDataStreamElement::F_UINT16_1;
        else if (uiVerticesPerCell <= UINT_MAX)
            eIndexFormat = NiDataStreamElement::F_UINT32_1;
        else
        {
            EE_FAIL("Cannot create that many instances.");
            NiDelete (pkBaseMesh);
            return NULL;
        }

        NiDataStreamElementSet kIndexElements;
        kIndexElements.AddElement(eIndexFormat);

        NiUInt32 uiNumIndices = GetIndicesPerMesh();
        if (!m_bUseInstancing)
            uiNumIndices *= uiInstancesPerCell;

        pkIndexStream = NiDataStream::CreateDataStream(
            kIndexElements, uiNumIndices,
            NiDataStream::ACCESS_CPU_WRITE_STATIC |
            NiDataStream::ACCESS_CPU_READ | /* required for instancing */
            NiDataStream::ACCESS_GPU_READ,
            NiDataStream::USAGE_VERTEX_INDEX);

        if (!pkIndexStream)
        {
            NiDelete (pkBaseMesh);
            return NULL;
        }

        pkIndexStreamRef->SetDataStream(pkIndexStream);
        pkIndexStreamRef->BindSemanticToElementDescAt(0, 
            NiCommonSemantics::INDEX(), 0);
    }


    // Create the mesh. (Directly access variable here, since mesh has not been
    // initialized yet)
    if (m_bUseInstancing)
    {
        /*
         * Vertex Stream
         */
        // Create the region
        pkVertexStream->RemoveAllRegions();
        pkVertexStream->AddRegion(NiDataStream::Region(0, GetVerticesPerMesh()));
        pkVertexStreamRef->BindRegionToSubmesh(0, 0);

        {
            NiFixedString kSemantic;
            NiUInt32 uiSemanticIndex;
            NiUInt32 uiNumVertices = GetVerticesPerMesh();
            for (NiUInt32 ui = 0; ui < kVertexElements.GetSize(); ++ui)
            {
                NiDataStreamElement& kElement = kVertexElements.GetAt(ui);

                // Bind the semantic
                GetVertexElementSemantic(ui, kSemantic, uiSemanticIndex);
                pkVertexStreamRef->BindSemanticToElementDescAt(ui, kSemantic, uiSemanticIndex);

                // Get a generic 8 bit iterator, so we don't need to worry about
                // the type of data we are copying
                kLock = NiDataStreamElementLock(pkBaseMesh, kSemantic, uiSemanticIndex, 
                    kElement.GetFormat());

                NiTStridedRandomAccessIterator<NiUInt8> kData = kLock.begin<NiUInt8>();

                // Populate with default data
                size_t stElementSize = kElement.GetComponentSize() * kElement.GetComponentCount();
                size_t stExpectedTemplateSize = stElementSize * uiNumVertices;
                
                size_t stTemplateSize;
                NiUInt8* pucTemplate = (NiUInt8*)GetVertexElementTemplateData(stTemplateSize,
                    kElement.GetFormat(), kSemantic, uiSemanticIndex);
                NIASSERT(stTemplateSize == stExpectedTemplateSize && pucTemplate != NULL);

                for (NiUInt32 uiVert = 0; uiVert < uiNumVertices; ++uiVert)
                {
                    NiMemcpy(&kData[uiVert], stElementSize, 
                        &pucTemplate[uiVert*stElementSize], stElementSize);
                }

                // Release the iterator
                kLock.Unlock();
            }
        }

        /*
         * Index stream
         */
        // Create the region
        pkIndexStream->RemoveAllRegions();
        pkIndexStream->AddRegion(NiDataStream::Region(0, GetIndicesPerMesh()));

        // Bind the compulsory region
        pkIndexStreamRef->BindRegionToSubmesh(0, 0);

        // Populate data
        {
            kLock = NiDataStreamElementLock(pkBaseMesh, 
                NiCommonSemantics::INDEX(), 0, eIndexFormat);

            // Generic Iterator
            NiTStridedRandomAccessIterator<NiUInt8> kIndices = kLock.begin<NiUInt8>();

            NiUInt32 uiNumIndices = GetIndicesPerMesh();

            // We always get the template data in NiUInt32 format, so we may 
            // need to cast it to a different format.
            NiUInt32 uiTemplateNumIndices;
            const NiUInt32* puiIndexBuffer = GetIndexTemplateData(uiTemplateNumIndices);
            NIASSERT(uiNumIndices == uiTemplateNumIndices && puiIndexBuffer != NULL);

            if (eIndexFormat == NiDataStreamElement::F_UINT32_1)
            {
                // Simply copy from one stream to the other
                CastStream<NiUInt32, NiUInt32>(puiIndexBuffer, &kIndices[0], uiNumIndices);
            }
            else if (eIndexFormat == NiDataStreamElement::F_UINT16_1)
            {
                // Cast stream contents from UInt32 to UInt16
                CastStream<NiUInt32, NiUInt16>(puiIndexBuffer, &kIndices[0], uiNumIndices);
            }
            else if (eIndexFormat == NiDataStreamElement::F_UINT8_1)
            {
                // Cast stream contents from UInt32 to UInt8
                CastStream<NiUInt32, NiUInt8>(puiIndexBuffer, &kIndices[0], uiNumIndices);
            }

            kLock.Unlock();
        }
    }
    else
    {
        /*
        * Vertex Stream
        */
        // Create all the vertex stream regions
        pkVertexStream->RemoveAllRegions();
        NiDataStream::Region kRegion(0, GetVerticesPerMesh() * uiInstancesPerCell);
        for (NiUInt32 ui = 0; ui < uiNumCells; ++ui)
        {
            pkVertexStream->AddRegion(kRegion);
            kRegion.SetStartIndex(kRegion.GetStartIndex() + kRegion.GetRange());
        }
        // Bind the compulsory region
        pkVertexStreamRef->BindRegionToSubmesh(0, 0);

        if (pkVertexStaticStreamRef != NULL)
        {
            pkVertexStaticStream->RemoveAllRegions();
            pkVertexStaticStream->AddRegion(NiDataStream::Region(0, 
                GetVerticesPerMesh() * uiInstancesPerCell));
            pkVertexStaticStreamRef->BindRegionToSubmesh(0, 0);
        }

        // Populate data
        {
            /*
                Since we are not using GPU mesh instancing, we need to sort
                the elements into one of two streams: transformed and static.
             */
            NiFixedString kSemantic;
            NiUInt32 uiSemanticIndex;
            bool bRequiresTransform;
            NiUInt32 uiNextStaticDescIndex = 0;

            // Initialize the elements
            for (NiUInt32 ui = 0; ui < kVertexElements.GetSize(); ++ui)
            {
                NiDataStreamElement& kElement = kVertexElements.GetAt(ui);

                GetVertexElementSemantic(ui, kSemantic, uiSemanticIndex);
                GetVertexElementRequiresTransform(kSemantic, uiSemanticIndex, bRequiresTransform);

                // Transformable element?
                if (bRequiresTransform)
                {
                    pkVertexStreamRef->BindSemanticToElementDescAt(
                        ui - uiNextStaticDescIndex, kSemantic, uiSemanticIndex);

                    // We don't need to initialize the data
                    continue;
                }

                // If we got this far, this element doesn't require transforms, so we must have a
                // static element stream.
                EE_ASSERT(pkVertexStaticStream);

                // Bind the semantic
                pkVertexStaticStreamRef->BindSemanticToElementDescAt(
                    uiNextStaticDescIndex, kSemantic, uiSemanticIndex);
                ++uiNextStaticDescIndex;

                // Populate element with default data

                // Get a generic 8 bit iterator, so we don't need to know the
                // type of the data we are copying
                kLock = NiDataStreamElementLock(pkBaseMesh,
                    kSemantic, uiSemanticIndex, kElement.GetFormat());

                NiTStridedRandomAccessIterator<NiUInt8> kData = kLock.begin<NiUInt8>();

                size_t stElementSize = kElement.GetComponentSize() * kElement.GetComponentCount();
                
                size_t stExpectedStreamSize = stElementSize * GetVerticesPerMesh();
                size_t stStreamSize;
                NiUInt8* pucTemplate = (NiUInt8*)GetVertexElementTemplateData(stStreamSize, 
                    kElement.GetFormat(), kSemantic, uiSemanticIndex);
                NIASSERT(stStreamSize == stExpectedStreamSize && pucTemplate != NULL);

                // Copy the template data for each vertex in the first instance
                for (NiUInt32 uiVert = 0; uiVert < GetVerticesPerMesh(); ++uiVert)
                {
                    NiUInt8* pucSource = &pucTemplate[uiVert * stElementSize / sizeof(NiUInt8)];

                    NiMemcpy(&(kData[uiVert]), stElementSize, pucSource, stElementSize);
                }

                // Release the iterator
                kLock.Unlock();
            }

            // Duplicate the static data across all instances
            if (pkVertexStaticStream && pkVertexStaticStream->GetElementDescCount() > 0)
            {
                EE_ASSERT(uiInstancesPerCell * GetVerticesPerMesh() == 
                    pkVertexStaticStream->GetCount(0));

                // Acquire a stream lock
                NiUInt8* pucBuffer = (NiUInt8*)pkVertexStaticStream->LockWrite();

                size_t stStride = pkVertexStaticStream->GetStride() * GetVerticesPerMesh();
                size_t stRemaining = pkVertexStaticStream->GetSize();

                // Skip the first entry, since it is the source!
                NiUInt8* pucStart = pucBuffer;
                pucBuffer += stStride;
                stRemaining -= stStride;

                for (NiUInt32 uiInstance = 1; uiInstance < uiInstancesPerCell; ++uiInstance)
                {
                    NiMemcpy(pucBuffer, stRemaining, pucStart, stStride);
                    pucBuffer += stStride;
                    stRemaining -= stStride;
                }

                pkVertexStaticStream->UnlockWrite();
            }
        }

        /*
         * Index stream, shared across regions
         */
        // Create the regions
        pkIndexStream->RemoveAllRegions();
        pkIndexStream->AddRegion(NiDataStream::Region(0, uiInstancesPerCell * GetIndicesPerMesh()));

        // Bind the compulsory region
        pkIndexStreamRef->BindRegionToSubmesh(0, 0);

        // Populate data
        {
            kLock = NiDataStreamElementLock(pkBaseMesh, 
                NiCommonSemantics::INDEX(), 0, eIndexFormat);

            // Generic Iterator
            const NiDataStreamElement& kElement = kLock.GetDataStreamElement();
            NiTStridedRandomAccessIterator<NiUInt8> kIndices = kLock.begin<NiUInt8>();

            size_t stElementSize = kElement.GetComponentSize() * kElement.GetComponentCount();
            NiUInt32 uiNumIndices = GetIndicesPerMesh();

            const NiUInt32* puiIndexTemplate = GetIndexTemplateData(uiNumIndices);

            NiUInt32* puiIndexBuffer = NiAlloc(NiUInt32, uiNumIndices);

            for (NiUInt32 uiInstance = 0; uiInstance < uiInstancesPerCell; ++uiInstance)
            {
                for (NiUInt32 uiIndex = 0; uiIndex < uiNumIndices; ++uiIndex)
                {
                    puiIndexBuffer[uiIndex] = puiIndexTemplate[uiIndex] + 
                        uiInstance * GetVerticesPerMesh();                    
                }

                NiUInt32 uiOffset = uiInstance * uiNumIndices;

                if (eIndexFormat == NiDataStreamElement::F_UINT16_1)
                {
                    // Cast stream contents from UInt32 to UInt16
                    CastStream<NiUInt32, NiUInt16>(puiIndexBuffer, 
                        &kIndices[uiOffset], uiNumIndices);
                }
                else if (eIndexFormat == NiDataStreamElement::F_UINT32_1)
                {
                    // No casting required
                    NiMemcpy(&kIndices[uiOffset], uiNumIndices * stElementSize,
                        puiIndexBuffer, uiNumIndices * stElementSize);
                }
                else if (eIndexFormat == NiDataStreamElement::F_UINT8_1)
                {
                    // Cast stream contents from UInt32 to UInt8
                    CastStream<NiUInt32, NiUInt8>(puiIndexBuffer, 
                        &kIndices[uiOffset], uiNumIndices);
                }
            }

            NiFree(puiIndexBuffer);

            kLock.Unlock();
        }
    }

    /*
     * Decoration Material
     */
    NiDecorationMaterial* pkMaterial = NiDecorationMaterial::Create();
    pkBaseMesh->ApplyAndSetActiveMaterial(pkMaterial);
    NiFloatExtraData* pkExtraData;
    pkExtraData = NiNew NiFloatExtraData(NiSqr(0.0f));
    pkBaseMesh->AddExtraData("g_FadeMinDistSqr", pkExtraData);
    pkExtraData = NiNew NiFloatExtraData(NiSqr(1.0f));
    pkBaseMesh->AddExtraData("g_FadeMaxDistSqr", pkExtraData); 

    /*
     * Enable instancing?
     */
    if (m_bUseInstancing)
    {
        // Set up instancing using a fake instance stream which we will replace
        bool bRes = NiInstancingUtilities::EnableMeshInstancing(pkBaseMesh, 2, NULL, 0, 
            GetIndicesPerMesh(), false, true, true);
        NIASSERT(bRes);
    }

    return NiNew NiDecorationMeshInfo(pkBaseMesh);
}

//------------------------------------------------------------------------------------------------
void NiDecorationMeshGenerator::SetActiveInstanceCount(NiDecorationMeshInfo* pkBase, 
    NiUInt32 uiInstanceCount)
{
    // Work out the offset from the beginning of the transform stream
    NiUInt32 uiOffset;
    if (!m_spTransformStreamManager->GetFirstCellIndex(pkBase, uiOffset))
        uiOffset = 0;
    uiOffset *= m_spTransformStreamManager->GetInstancesPerCell();

    NiMesh* pkBaseMesh = pkBase->GetMesh();

    if (GetUseInstancing(pkBaseMesh))
    {
        // Our transform manager has already taken care of the region sizes.
        EE_UNUSED_ARG(uiInstanceCount);
        EE_ASSERT(uiOffset + uiInstanceCount <= m_spTransformStreamManager->GetTransformCount());

        return;
    }
    else
    {
        EE_ASSERT(m_uiInstancesPerCell > 0);
        NiUInt32 uiCellCount = uiInstanceCount / m_uiInstancesPerCell;

        // TODO: Test
        NiUInt32 uiLastCellCount = pkBaseMesh->GetSubmeshCount();
        NiUInt32 uiLastInstanceCount = uiLastCellCount * m_uiInstancesPerCell;

        if (uiLastInstanceCount < uiInstanceCount)
        {
            pkBaseMesh->SetSubmeshCount(uiCellCount);

            // Bind the streams to the regions
            NiDataStreamRef* pkVertexRef = pkBaseMesh->GetStreamRefAt(SRI_VERTEX);
            NiDataStreamRef* pkIndexRef = pkBaseMesh->GetStreamRefAt(SRI_INDEX);

            NiDataStreamRef* pkVertexStaticRef = NULL;
            if (pkBaseMesh->GetStreamRefCount() == 3)
                pkVertexStaticRef = pkBaseMesh->GetStreamRefAt(SRI_VERTEX_STATIC);

            for (NiUInt32 uiCell = uiLastCellCount; uiCell < uiCellCount; ++uiCell)
            {
                pkVertexRef->BindRegionToSubmesh(uiCell, uiCell);
                pkIndexRef->BindRegionToSubmesh(uiCell, 0);

                if (pkVertexStaticRef)
                    pkVertexStaticRef->BindRegionToSubmesh(uiCell, 0);
            }
        }
        else
        {
            // Removing cells, just set submesh count. Note that we must
            // ALWAYS have at least 1 submesh; even if the cell count is zero.
            pkBaseMesh->SetSubmeshCount(NiMax(uiCellCount, (NiUInt32)1));
        }
    }
}

//------------------------------------------------------------------------------------------------
void NiDecorationMeshGenerator::SetTransforms(NiDecorationMeshInfo* pkBase, 
    NiTransform* pkTransforms, NiUInt32 uiNumCells, NiUInt32 uiStartCell)
{
    NiMesh* pkBaseMesh = pkBase->GetMesh();

    if (GetUseInstancing(pkBaseMesh))
    {
        bool bRes = NiInstancingUtilities::SetInstanceTransformations(
            pkBaseMesh, pkTransforms, 
            uiNumCells * m_uiInstancesPerCell, 
            uiStartCell * m_uiInstancesPerCell);

        NiInstancingUtilities::SetActiveInstanceCount(pkBaseMesh,
            NiInstancingUtilities::GetActiveInstanceCount(pkBaseMesh));

        EE_UNUSED_ARG(bRes);
        EE_ASSERT(bRes);
    }
    else
    {
        NiDataStreamRef* pkStreamRef = pkBaseMesh->GetStreamRefAt(SRI_VERTEX);

        NiUInt32 uiElements = pkStreamRef->GetElementDescCount();
        NiFixedString kSemantic;
        NiUInt32 uiSemanticIndex;
        bool bRequiresTransform, bIsNormalized;

        for (NiUInt32 uiElement = 0; uiElement < uiElements; ++uiElement)
        {
            const NiDataStreamElement& kElement = pkStreamRef->GetElementDescAt(uiElement);

            // We can only transform NiPoint3's
            EE_ASSERT(kElement.GetFormat() == NiDataStreamElement::F_FLOAT32_3);

            // Does this element require transformation?
            kSemantic = pkStreamRef->GetSemanticNameAt(uiElement);
            uiSemanticIndex = pkStreamRef->GetSemanticIndexAt(uiElement);

            EE_VERIFY(GetVertexElementRequiresTransform(
                kSemantic, uiSemanticIndex, bRequiresTransform));
            EE_ASSERT(bRequiresTransform);

            EE_VERIFY(GetVertexElementIsNormalized(kSemantic, uiSemanticIndex, bIsNormalized));

            NiDataStreamElementLock kLock = NiDataStreamElementLock(
                pkBaseMesh, kSemantic, uiSemanticIndex, kElement.GetFormat());

            EE_ASSERT(uiStartCell + uiNumCells <= pkBaseMesh->GetSubmeshCount());

            EE_ASSERT(kLock.IsLocked());
            if (!kLock.IsLocked())
                continue;

            NiUInt32 uiVertsPerMesh = GetVerticesPerMesh();
            size_t stElementSize = kElement.GetComponentSize() * kElement.GetComponentCount();
            
            size_t stExpectedTemplateSize = stElementSize * uiVertsPerMesh;
            size_t stTemplateSize;
            NiPoint3* pkTemplate = (NiPoint3*)GetVertexElementTemplateData(stTemplateSize, 
                kElement.GetFormat(), kSemantic, uiSemanticIndex);
            NIASSERT(stExpectedTemplateSize == stTemplateSize && pkTemplate != NULL);

            NiTransform* pkTransform;
            for (NiUInt32 uiCell = 0; uiCell < uiNumCells; ++uiCell)
            {
                NiTStridedRandomAccessIterator<NiPoint3> kIter = 
                    kLock.begin<NiPoint3>(uiStartCell + uiCell);

                for (NiUInt32 uiInst = 0; uiInst < m_uiInstancesPerCell; ++uiInst)
                {
                    NiUInt32 uiVert = 0;
                    pkTransform = &pkTransforms[uiInst];
                    if (bIsNormalized)
                    {
                        for (; uiVert < uiVertsPerMesh; ++uiVert)
                        {
                            // Don't need to consider translation or scale,
                            // just rotation
                            kIter[0] = pkTransform->m_Rotate * pkTemplate[uiVert];
                            kIter++;
                        }
                    }
                    else
                    {
                        for (; uiVert < uiVertsPerMesh; ++uiVert)
                        {
                            // Full transformation
                            kIter[0] = pkTransform->m_Translate + pkTransform->m_fScale * 
                                (pkTransform->m_Rotate * pkTemplate[uiVert]);
                            kIter++;
                        }
                    }
                }
            }

            kLock.Unlock();
        }
    }
}

//------------------------------------------------------------------------------------------------
