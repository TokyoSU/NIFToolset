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
#include "ecrD3D11RendererPCH.h"

#include "D3D11MeshMaterialBinding.h"
#include "D3D11Error.h"
#include "D3D11PixelFormat.h"
#include "D3D11Renderer.h"

#include <NiMeshMaterialBinding.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11MeshMaterialBinding::InputLayoutEntry::InputLayoutEntry() :
    m_pInputLayout(NULL),
    m_pInputSignature(NULL),
    m_pNext(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11MeshMaterialBinding::InputLayoutEntry::~InputLayoutEntry()
{
    if (m_pInputLayout)
        m_pInputLayout->Release();
    EE_DELETE m_pNext;
}

//------------------------------------------------------------------------------------------------
D3D11MeshMaterialBinding::D3D11MeshMaterialBinding() :
    m_elementDescArray(NULL),
    m_elementCount(0),
    m_inputLayoutArray(NULL),
    m_pCurrentInputLayout(NULL),
    m_pIndexStreamRef(NULL)
{
    memset(m_streamsToSetArray, 0, sizeof(m_streamsToSetArray));
}

//------------------------------------------------------------------------------------------------
D3D11MeshMaterialBinding::~D3D11MeshMaterialBinding()
{
    ReleaseCachedInputLayouts();
    ReleaseElementArray();
}

//------------------------------------------------------------------------------------------------
D3D11MeshMaterialBindingPtr D3D11MeshMaterialBinding::Create(
    NiRenderObject* pMesh, 
    const NiSemanticAdapterTable& adapterTable)
{
    D3D11MeshMaterialBindingPtr spMMB = EE_NEW D3D11MeshMaterialBinding;

    if (!spMMB->FillElementDescArray(pMesh, adapterTable))
    {
        spMMB = NULL;
    }
    return spMMB;
}

//------------------------------------------------------------------------------------------------
void D3D11MeshMaterialBinding::UpdateInputLayout(
    void* pInputSignature,
    efd::UInt32 inputSignatureSize)
{
    InputLayoutEntry* pEntry = m_inputLayoutArray;
    while (pEntry)
    {
        if (pEntry->m_pInputSignature == pInputSignature)
        {
            m_pCurrentInputLayout = pEntry->m_pInputLayout;
            return;
        }
        pEntry = pEntry->m_pNext;
    }

    EE_ASSERT(D3D11Renderer::GetRenderer() != NULL);
    ID3D11Device* pDevice = D3D11Renderer::GetRenderer()->GetD3D11Device();
    EE_ASSERT(pDevice != NULL);

    ID3D11InputLayout* pInputLayout = NULL;
    HRESULT hr = pDevice->CreateInputLayout(m_elementDescArray,
        m_elementCount, 
        pInputSignature, 
        inputSignatureSize,
        &pInputLayout);

    if (FAILED(hr) || pInputLayout == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_INPUT_LAYOUT_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_INPUT_LAYOUT_CREATION_FAILED,
                "No error message from D3D11, but input layout is NULL.");
        }

        if (pInputLayout)
        {
            pInputLayout->Release();
            pInputLayout = NULL;
        }
        return;
    }

    pEntry = EE_NEW InputLayoutEntry;
    pEntry->m_pInputLayout = pInputLayout;
    pEntry->m_pInputSignature = pInputSignature;
    pEntry->m_pNext = m_inputLayoutArray;
    m_inputLayoutArray = pEntry;

    m_pCurrentInputLayout = pEntry->m_pInputLayout;
}

//------------------------------------------------------------------------------------------------
struct D3D11CreateBindingContext : public NiMeshMaterialBinding::CreateBindingBaseContext
{
    // +1 for end element
    D3D11_INPUT_ELEMENT_DESC m_akVertexElements[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT + 1];

    efd::UInt16* m_pStreamsToSet;
    efd::UInt16* m_pLastValidStream;

    virtual bool CallBack_EndOfElementLoop(
        efd::UInt32 stream,
        const NiDataStreamRef* pStreamRef,
        const NiDataStreamElement element,
        NiDataStreamElement::Format packedDataFormat,
        const NiFixedString& rendererSemantic,
        const efd::UInt8 rendererSemanticIndex,
        efd::UInt32 packedDataFormatComponentCount,
        efd::UInt32 packedOffset);
};

//------------------------------------------------------------------------------------------------
bool D3D11CreateBindingContext::CallBack_EndOfElementLoop(
    efd::UInt32 stream,
    const NiDataStreamRef* pStreamRef,
    const NiDataStreamElement,
    NiDataStreamElement::Format packedDataFormat,
    const NiFixedString& rendererSemantic,
    const efd::UInt8 rendererSemanticIndex,
    efd::UInt32,
    efd::UInt32 packedOffset)
{
    EE_ASSERT(m_uiCurrentStream < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

    // Add new entry
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();

    D3D11_INPUT_CLASSIFICATION instanceType;
    efd::UInt32 instanceStep;
    if (!pStreamRef->IsPerInstance())
    {
        instanceType = D3D11_INPUT_PER_VERTEX_DATA;
        instanceStep = 0;
    }
    else
    {
        instanceType = D3D11_INPUT_PER_INSTANCE_DATA;
        instanceStep = 1;
    }

    DXGI_FORMAT dxgiFormat = D3D11PixelFormat::DetermineDXGIFormat(packedDataFormat);

    if (dxgiFormat != DXGI_FORMAT_UNKNOWN &&
        pRenderer->DoesFormatSupportFlag(dxgiFormat, D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER))
    {
        if (m_uiCurrentElement >= D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
        {
            NiRenderer::Warning(
                __FUNCTION__ "> "
                "Malformed semantic adapter table for mesh %s: "
                "too many vertex elements in stream %d.\n"
                "    Vertex declaration cannot be created.",
                m_pkMesh->GetName(),
                m_uiCurrentStream);
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC& currentElement =
            m_akVertexElements[m_uiCurrentElement];
        currentElement.SemanticName = rendererSemantic;
        currentElement.SemanticIndex = rendererSemanticIndex;
        currentElement.Format = dxgiFormat;
        currentElement.InputSlot = m_uiCurrentStream;
        currentElement.AlignedByteOffset = packedOffset;
        currentElement.InputSlotClass = instanceType;
        currentElement.InstanceDataStepRate = instanceStep;

        // Store the streams that will need to be set to device.
        // For each device stream that will be set,
        // associate our stream index from the NiMesh
        m_pStreamsToSet[m_uiCurrentStream] = (efd::UInt16)stream;
        *m_pLastValidStream = (efd::UInt16)m_uiCurrentStream;
    }
    else
    {
        NiRenderer::Warning(
            __FUNCTION__ "> "
            "Malformed semantic adapter table for mesh %s: Data type "
            "and/or renderer semantic name %s aren't supported by "
            "D3D11.\n"
            "    Vertex declaration cannot be created.",
            m_pkMesh->GetName(),
            rendererSemantic);
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11MeshMaterialBinding::FillElementDescArray(
    NiRenderObject* pMesh_beforecast,
    const NiSemanticAdapterTable& adapterTable)
{
    NiMesh* pMesh = NiVerifyStaticCast(NiMesh, pMesh_beforecast);

    // This should never be called if an element desc array already exists
    EE_ASSERT(m_elementDescArray == NULL && m_elementCount == 0);

    NiDataStreamRef* pIndexRef = pMesh->FindStreamRef(NiCommonSemantics::INDEX());
    NiDataStreamRef* pDLRef = pMesh->FindStreamRef(NiCommonSemantics::DISPLAYLIST());
    if (pDLRef && !pIndexRef)
    {
        NiRenderer::Warning(
            "%s> Failed to create a vertex declaration for "
            "mesh (pointer: 0x%08X, name: %s) because the INDEX stream "
            "was removed during a platform-specific export.",
            __FUNCTION__,
            pMesh,
            (const char*) pMesh->GetName());
        return false;
    }

    D3D11CreateBindingContext kContext;
    kContext.m_pkMesh = pMesh;
    kContext.m_pStreamsToSet = m_streamsToSetArray;
    kContext.m_pLastValidStream = &m_lastValidStream;

    m_lastValidStream = 0;

    if (!NiMeshMaterialBinding::CreateBinding<D3D11CreateBindingContext>
        (kContext, adapterTable))
    {
        return false;
    }

    // Copy to D3D11MeshMaterialBinding object
    m_elementCount = (efd::UInt16)kContext.m_uiCurrentElement;
    m_elementDescArray = EE_ALLOC(D3D11_INPUT_ELEMENT_DESC, m_elementCount);
    efd::Memcpy(m_elementDescArray, 
        kContext.m_akVertexElements,
        m_elementCount * sizeof(*m_elementDescArray));

    // Set the index stream
    m_pIndexStreamRef = pIndexRef;

    return true;
}

//------------------------------------------------------------------------------------------------
