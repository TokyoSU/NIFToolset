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

#include "D3D11Renderer.h"

#include "D3D11DataStreamFactory.h"
#include "D3D11Direct3DResource.h"
#include "D3D11Direct3DResourceData.h"
#include "D3D11DynamicTextureData.h"
#include "D3D11Error.h"
#include "D3D11ErrorShader.h"
#include "D3D11FragmentShader.h"
#include "D3D11GPUProgramCache.h"
#include "D3D11PersistentSrcTextureRendererData.h"
#include "D3D11PixelFormat.h"
#include "D3D11RenderedTextureData.h"
#include "D3D11RenderStateManager.h"
#include "D3D11ResourceManager.h"
#include "D3D11SourceTextureData.h"
#include "D3D11ShaderConstantManager.h"
#include "D3D11ShaderProgramFactory.h"
#include "D3D11ShadowWriteShader.h"
#include "D3D11TextureData.h"
#include "ecrD3D11RendererSDM.h"

#include <NiDirectionalShadowWriteMaterial.h>
#include <NiDynamicTexture.h>
#include <NiFilename.h>
#include <NiFragmentMaterial.h>
#include <NiPointShadowWriteMaterial.h>
#include <NiShadowManager.h>
#include <NiSingleShaderMaterial.h>
#include <NiSourceCubeMap.h>
#include <NiSpotShadowWriteMaterial.h>
#include <NiTPointerMap.h>
#include <NiVersion.h>

// for _chdir
#include <direct.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
// The following copyright notice may not be removed.
static efd::Char EmergentCopyright[] EE_UNUSED = "Copyright (c) 1996-2009 Emergent Game Technologies.";
//------------------------------------------------------------------------------------------------
static efd::Char acGamebryoVersion[] EE_UNUSED = GAMEBRYO_MODULE_VERSION_STRING(D3D11);
//------------------------------------------------------------------------------------------------

namespace
{
void LogRendererState(const char* pStage, const ecr::D3D11Renderer* pRenderer)
{
    if (pRenderer == NULL)
    {
        ecr::D3D11Error::ReportWarning("[D3D11Renderer] %s renderer=NULL", pStage);
        return;
    }

    ecr::D3D11Error::ReportWarning(
        "[D3D11Renderer] %s this=%p device=%p immediateContext=%p currentContext=%p deviceThreadId=%u",
        pStage,
        static_cast<const void*>(pRenderer),
        static_cast<const void*>(pRenderer->GetD3D11Device()),
        static_cast<const void*>(pRenderer->GetImmediateD3D11DeviceContext()),
        static_cast<const void*>(pRenderer->GetCurrentD3D11DeviceContext()),
        pRenderer->GetDeviceThreadID());
}
}

HINSTANCE D3D11Renderer::ms_hD3D11 = NULL;
HINSTANCE D3D11Renderer::ms_hD3D10 = NULL;
HINSTANCE D3D11Renderer::ms_hD3D9 = NULL;

PFN_D3D11_CREATE_DEVICE D3D11Renderer::ms_pD3D11CreateDeviceFunc = NULL;
PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN D3D11Renderer::ms_pD3D11CreateDeviceAndSwapChainFunc = NULL;

EE_LPD3D11PERF_BEGINEVENT D3D11Renderer::ms_pD3DPERF_BeginEventFunc = NULL;
EE_LPD3D11PERF_ENDEVENT D3D11Renderer::ms_pD3DPERF_EndEventFunc = NULL;
EE_LPD3D11PERF_GETSTATUS D3D11Renderer::ms_pD3DPERF_GetStatusFunc = NULL;

EE_LPD3D10CREATEBLOB D3D11Renderer::ms_pD3D10CreateBlob = NULL;

efd::CriticalSection D3D11Renderer::ms_libD3D11CriticalSection;

NiVisibleArray* D3D11Renderer::ms_pDefaultVisibleArray = NULL;

static ecrD3D11RendererSDM ecrD3D11RendererSDMObject;

NiImplementRTTI(D3D11Renderer, NiRenderer, NiTypeMask::D3D11Renderer);

//------------------------------------------------------------------------------------------------
D3D11Renderer::CreationParameters::CreationParameters() :
    m_adapterIndex(0),
    m_outputIndex(0),
    m_driverType(D3D11Renderer::DRIVER_TYPE_HARDWARE),
    m_pFeatureLevels(NULL),
    m_featureLevelCount(0),
#if defined(EE_CONFIG_DEBUG)
    m_createFlags(D3D11Renderer::CREATE_DEVICE_SINGLETHREADED |
        D3D11Renderer::CREATE_DEVICE_DEBUG),
#else //#if defined(EE_CONFIG_DEBUG)
    m_createFlags(D3D11Renderer::CREATE_DEVICE_SINGLETHREADED),
#endif //#if defined(EE_CONFIG_DEBUG)
    m_createSwapChain(false),
    m_createDepthStencilBuffer(true),
    m_associateWithWindow(false),
    m_windowAssociationFlags(0),
    m_depthStencilFormat(DXGI_FORMAT_UNKNOWN)
{
    m_swapChain.BufferDesc.Width = 640;
    m_swapChain.BufferDesc.Height= 480;
    m_swapChain.BufferDesc.RefreshRate.Numerator = 60;
    m_swapChain.BufferDesc.RefreshRate.Denominator = 1;
    m_swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_swapChain.BufferDesc.ScanlineOrdering =
        DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    m_swapChain.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    m_swapChain.SampleDesc.Count = 1;
    m_swapChain.SampleDesc.Quality = 0;
    m_swapChain.BufferUsage = DXGI_USAGE_BACK_BUFFER |
        DXGI_USAGE_RENDER_TARGET_OUTPUT;
    m_swapChain.BufferCount = 1;
    m_swapChain.OutputWindow = NULL;
    m_swapChain.Windowed = false;
    m_swapChain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    m_swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
}

//------------------------------------------------------------------------------------------------
D3D11Renderer::CreationParameters::CreationParameters(HWND hWnd) :
    m_adapterIndex(0),
    m_outputIndex(0),
    m_driverType(D3D11Renderer::DRIVER_TYPE_HARDWARE),
    m_pFeatureLevels(NULL),
    m_featureLevelCount(0),
#if defined(EE_CONFIG_DEBUG)
    m_createFlags(D3D11Renderer::CREATE_DEVICE_SINGLETHREADED |
        D3D11Renderer::CREATE_DEVICE_DEBUG),
#else //#if defined(EE_CONFIG_DEBUG)
    m_createFlags(D3D11Renderer::CREATE_DEVICE_SINGLETHREADED),
#endif //#if defined(EE_CONFIG_DEBUG)
    m_createSwapChain(true),
    m_createDepthStencilBuffer(true),
    m_associateWithWindow(true),
    m_windowAssociationFlags(0),
    m_depthStencilFormat(DXGI_FORMAT_UNKNOWN)
{
    m_swapChain.BufferDesc.Width = 640;
    m_swapChain.BufferDesc.Height = 480;
    m_swapChain.BufferDesc.RefreshRate.Numerator = 60;
    m_swapChain.BufferDesc.RefreshRate.Denominator = 1;
    m_swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_swapChain.BufferDesc.ScanlineOrdering =
        DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    m_swapChain.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    m_swapChain.SampleDesc.Count = 1;
    m_swapChain.SampleDesc.Quality = 0;
    m_swapChain.BufferUsage = DXGI_USAGE_BACK_BUFFER |
        DXGI_USAGE_RENDER_TARGET_OUTPUT;
    m_swapChain.BufferCount = 1;
    m_swapChain.OutputWindow = NULL;
    m_swapChain.Windowed = false;
    m_swapChain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    m_swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if (hWnd)
    {
        m_swapChain.OutputWindow = hWnd;
        m_swapChain.Windowed = true;
        RECT rect;
        ::GetClientRect(hWnd, &rect);

        m_swapChain.BufferDesc.Width = rect.right - rect.left;
        m_swapChain.BufferDesc.Height = rect.bottom - rect.top;
    }
}

//------------------------------------------------------------------------------------------------
D3D11Renderer::D3D11Renderer() :
    m_deviceThreadID(0),
    m_pD3D11Device(NULL),
	m_pImmediateD3D11DeviceContext(NULL),
	m_pCurrentD3D11DeviceContext(NULL),
    m_initialized(false),
    m_pCurrentRenderTargetGroup(NULL),
    m_depthClear(1.0f),
    m_stencilClear(0),
    m_leftRightSwap(false),
    m_currentFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f),
    m_kurrentViewPort(0.0f, 1.0f, 1.0f, 0.0f),
    m_syncInterval(1),
    m_maxFogFactor(0.0f),
    m_maxFogValue(1.0f),
    m_pDataStreamFactory(NULL),
    m_pDeviceState(NULL),
    m_pRenderStateManager(NULL),
    m_pResourceManager(NULL),
    m_pShaderConstantManager(NULL),
    m_supportedShaderProgramTypeCount(0),
    m_maxComputeShaderVersion(0),
    m_pVertexShaderVersionString(NULL),
    m_pHullShaderVersionString(NULL),
    m_pDomainShaderVersionString(NULL),
    m_pGeometryShaderVersionString(NULL),
    m_pPixelShaderVersionString(NULL),
    m_pComputeShaderVersionString(NULL),
    m_pTempVertices(NULL),
    m_pTempColors(NULL),
    m_pTempTexCoords(NULL),
    m_tempArraySize(0),
    m_isDeviceOccluded(false),
    m_isDeviceRemoved(false),
    m_pShaderMacroArray(NULL),
    m_shaderMacroCount(0),
    m_isPIXActive(false),
    m_supportsMMX(false),
    m_supportsSSE(false),
    m_supportsSSE2(false),
    m_backgroundRendering(false),
    m_backgroundResourceCreation(false)

{
    m_adapterIndex = (efd::UInt32)NULL_ADAPTER;
    m_usMaxAnisotropy = HW_MAX_ANISOTROPY;

    memset(m_supportedShaderProgramTypes, 0, sizeof(m_supportedShaderProgramTypes));
    memset(m_backgroundColor, 0, sizeof(m_backgroundColor));
    memset(m_formatSupportArray, 0, sizeof(m_formatSupportArray));

    m_d3dView = XMMatrixIdentity();
    m_invView = XMMatrixIdentity();
    m_d3dProj = XMMatrixIdentity();
    m_d3dModel = XMMatrixIdentity();
}

//------------------------------------------------------------------------------------------------
D3D11Renderer::~D3D11Renderer()
{
//CODEBLOCK(1) - DO NOT DELETE THIS LINE

    NiMaterial::UnloadShadersForAllMaterials();

    if (m_spInitialDefaultMaterial &&
        NiIsKindOf(NiSingleShaderMaterial, m_spInitialDefaultMaterial))
    {
        ((NiSingleShaderMaterial*)m_spInitialDefaultMaterial.data())->SetCachedShader(NULL);
    }
    if (m_spCurrentDefaultMaterial &&
        NiIsKindOf(NiSingleShaderMaterial, m_spCurrentDefaultMaterial))
    {
        ((NiSingleShaderMaterial*)m_spCurrentDefaultMaterial.data())->SetCachedShader(NULL);
    }

    m_spBatchMaterial = 0;

    EE_DELETE[] m_pTempVertices;
    EE_DELETE[] m_pTempColors;
    EE_DELETE[] m_pTempTexCoords;

    // Purge swap chains
    NiTListIterator iter = m_swapChainRenderTargetGroups.GetFirstPos();
    while (iter)
    {
        HWND hWnd;
        NiRenderTargetGroupPtr spRenderTargetGroup;

        m_swapChainRenderTargetGroups.GetNext(iter, hWnd, spRenderTargetGroup);
        if (spRenderTargetGroup)
        {
            for (efd::UInt32 i = 0; i < spRenderTargetGroup->GetBufferCount(); i++)
            {
                Ni2DBuffer* pBuffer = spRenderTargetGroup->GetBuffer(i);
                if (pBuffer)
                    pBuffer->SetRendererData(NULL);
            }

            Ni2DBuffer* pBuffer = spRenderTargetGroup->GetDepthStencilBuffer();
            if (pBuffer)
                pBuffer->SetRendererData(NULL);
        }
    }
    m_swapChainRenderTargetGroups.RemoveAll();

    // Purge textures
    PurgeAllTextures(true);

    // Purge the D3D11Shaders
    PurgeAllD3D11Shaders();

    // Release all D3D11Shaders loaded by the shader factory.
    D3D11ShaderFactory* pShaderFactory = D3D11ShaderFactory::GetD3D11ShaderFactory();
    if (pShaderFactory)
    {
        pShaderFactory->RemoveAllShaders();
        pShaderFactory->RemoveAllLibraries();
    }

    ReleaseManagers();
    ReleaseResources();
    ReleaseDevice();

    NiStream::UnregisterLoader("NiPersistentSrcTextureRendererData");
    NiPersistentSrcTextureRendererData::ResetStreamingFunctions();

    EE_FREE(m_pShaderMacroArray);
}

//------------------------------------------------------------------------------------------------
ID3D11Device* D3D11Renderer::GetD3D11Device() const
{
    return m_pD3D11Device;
}

//------------------------------------------------------------------------------------------------
ID3D11DeviceContext* D3D11Renderer::GetImmediateD3D11DeviceContext() const
{
    return m_pImmediateD3D11DeviceContext;
}

//------------------------------------------------------------------------------------------------
ID3D11DeviceContext* D3D11Renderer::GetCurrentD3D11DeviceContext() const
{
    return m_pCurrentD3D11DeviceContext;
}

//------------------------------------------------------------------------------------------------
D3D11DeviceState* D3D11Renderer::GetDeviceState() const
{
    return m_pDeviceState;
}

//------------------------------------------------------------------------------------------------
D3D11RenderStateManager* D3D11Renderer::GetRenderStateManager() const
{
    return m_pRenderStateManager;
}

//------------------------------------------------------------------------------------------------
D3D11ResourceManager* D3D11Renderer::GetResourceManager() const
{
    return m_pResourceManager;
}

//------------------------------------------------------------------------------------------------
D3D11ShaderConstantManager* D3D11Renderer::GetShaderConstantManager() const
{
    return m_pShaderConstantManager;
}

//------------------------------------------------------------------------------------------------
D3D11Renderer::FeatureLevel D3D11Renderer::GetFeatureLevel() const
{
    return (FeatureLevel)m_featureLevel;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::InvalidateDeviceState()
{
    if (m_pDeviceState)
        m_pDeviceState->InvalidateDeviceState();
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::_SDMInit()
{
    ms_pDefaultVisibleArray = EE_NEW NiVisibleArray();
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::_SDMShutdown()
{
    ReleaseD3D11();

    EE_DELETE ms_pDefaultVisibleArray;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::Create(CreationParameters& createParams,
    D3D11RendererPtr& spRenderer)
{
    spRenderer = NULL;

//CODEBLOCK(2) - DO NOT DELETE THIS LINE

    spRenderer = EE_NEW D3D11Renderer;
    EE_ASSERT(spRenderer);
    LogRendererState("Create.after-allocation", spRenderer);
    efd::Bool success = spRenderer->Initialize(createParams);

    if (success == false)
    {
        spRenderer = NULL;
        return false;
    }

    NiStream::UnregisterLoader("NiPersistentSrcTextureRendererData");
    NiStream::RegisterLoader("NiPersistentSrcTextureRendererData",
        D3D11PersistentSrcTextureRendererData::CreateObject);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::ResizeBuffers(efd::UInt32 width,
    efd::UInt32 height, HWND hOutputWnd)
{
    NiRenderTargetGroup* pRTG = m_spDefaultRenderTargetGroup;
    if (hOutputWnd)
        pRTG = GetSwapChainRenderTargetGroup(hOutputWnd);

    if (pRTG == NULL)
        return false;

    D3D11SwapChainBufferData* pSwapChainBufferData =
        (D3D11SwapChainBufferData*)pRTG->GetBufferRendererData(0);

    if (pSwapChainBufferData == NULL)
        return false;

    // Release depth/stencil buffer
    efd::Bool dsBuffer = pRTG->HasDepthStencil();
    const NiPixelFormat* pDSFormat = pRTG->GetDepthStencilPixelFormat();

    pRTG->AttachDepthStencilBuffer(NULL);

    efd::Bool success = pSwapChainBufferData->ResizeSwapChain(width, height);
    if (!success)
    {
        D3D11Renderer::Warning("D3D11SwapChainBufferData::ResizeSwapChain failed in " __FUNCTION__);

        // Don't return here - still need to recreate DS buffer.
    }

    // Recreate depth/stencil buffer
    if (dsBuffer)
    {
        NiDepthStencilBufferPtr spDepthBuffer = NiDepthStencilBuffer::Create(
            pSwapChainBufferData->GetWidth(),
            pSwapChainBufferData->GetHeight(),
            this,
            *pDSFormat,
            pSwapChainBufferData->GetMSAAPref());
        pRTG->AttachDepthStencilBuffer(spDepthBuffer);
    }

//CODEBLOCK(3) - DO NOT DELETE THIS LINE

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::Initialize(CreationParameters& createParams)
{
    LogRendererState("Initialize.entry", this);

    if (m_initialized)
    {
        D3D11Error::ReportWarning(
            "Attempting to initialize renderer that is already initialized.");
        return false;
    }

    if (CreateDevice(createParams))
    {
        LogRendererState("Initialize.after-CreateDevice", this);

        CreateManagers();
        LogRendererState("Initialize.after-CreateManagers", this);

        if (createParams.m_createSwapChain == false)
        {
            m_initialized = true;
        }
        else
        {
            LogRendererState("Initialize.before-CreateSwapChain", this);
            IDXGISwapChain* pSwapChain =
                CreateSwapChain(createParams.m_swapChain, createParams.m_outputIndex);
            if (pSwapChain != NULL)
            {
                LogRendererState("Initialize.after-CreateSwapChain", this);
                NiRenderTargetGroup* pRTGroup =
                    CreateRenderTargetGroupFromSwapChain(pSwapChain,
                    createParams.m_createDepthStencilBuffer,
                    createParams.m_depthStencilFormat);
                if (pRTGroup)
                {
                    m_spDefaultRenderTargetGroup = pRTGroup;

                    m_swapChainRenderTargetGroups.SetAt(
                        createParams.m_swapChain.OutputWindow, pRTGroup);

                    m_initialized = true;
                }
                else
                {
                    D3D11Error::ReportWarning(
                        "Initialization failed because NiRenderTargetGroup for default swap chain "
                        "could not be created; destroying D3D11 Device.");
                }
            }
            else
            {
                D3D11Error::ReportWarning(
                    "Initialization failed because default swap chain could not be created; "
                    "destroying D3D11 Device.");
            }
        }
    }

    if (m_initialized == false)
    {
        // Initialization failed
        ReleaseDevice();

        D3D11Renderer::Warning("Renderer failed initialization");
        return false;
    }

    if (createParams.m_associateWithWindow)
    {
        IDXGIFactory* pFactory = m_spSystemDesc->GetFactory();
        EE_ASSERT(pFactory);

        pFactory->MakeWindowAssociation(
            createParams.m_swapChain.OutputWindow,
            GetD3D11WindowAssociationFlags(
            createParams.m_windowAssociationFlags));
    }

    // Initialize shader system
    EE_ASSERT(D3D11ShaderFactory::GetD3D11ShaderFactory());
    EE_ASSERT(D3D11ShaderProgramFactory::GetInstance());

    D3D11ShaderFactory* pShaderFactory = D3D11ShaderFactory::GetD3D11ShaderFactory();

    pShaderFactory->SetAsActiveFactory();

    // Setup the default shader version information
    efd::UInt32 maxVertexShaderVersion = pShaderFactory->CreateVertexShaderVersion(0, 0);
    efd::UInt32 maxGeometryShaderVersion = pShaderFactory->CreateGeometryShaderVersion(0, 0);
    efd::UInt32 maxPixelShaderVersion = pShaderFactory->CreatePixelShaderVersion(0, 0);
    m_maxComputeShaderVersion = pShaderFactory->CreateComputeShaderVersion(0, 0);
    switch (m_featureLevel)
    {
    case D3D_FEATURE_LEVEL_11_0:
    {
        m_pVertexShaderVersionString = "vs_5_0";
        m_pHullShaderVersionString = "hs_5_0";
        m_pDomainShaderVersionString = "ds_5_0";
        m_pGeometryShaderVersionString = "gs_5_0";
        m_pPixelShaderVersionString = "ps_5_0";
        m_pComputeShaderVersionString = "cs_5_0";
        maxVertexShaderVersion = pShaderFactory->CreateVertexShaderVersion(5, 0);
        maxGeometryShaderVersion = pShaderFactory->CreateGeometryShaderVersion(5, 0);
        maxPixelShaderVersion = pShaderFactory->CreatePixelShaderVersion(5, 0);
        m_maxComputeShaderVersion = pShaderFactory->CreateComputeShaderVersion(5, 0);
        m_supportedShaderProgramTypeCount = 6;
        m_supportedShaderProgramTypes[0] = NiGPUProgram::PROGRAM_VERTEX;
        m_supportedShaderProgramTypes[1] = NiGPUProgram::PROGRAM_HULL;
        m_supportedShaderProgramTypes[2] = NiGPUProgram::PROGRAM_DOMAIN;
        m_supportedShaderProgramTypes[3] = NiGPUProgram::PROGRAM_GEOMETRY;
        m_supportedShaderProgramTypes[4] = NiGPUProgram::PROGRAM_PIXEL;
        m_supportedShaderProgramTypes[5] = NiGPUProgram::PROGRAM_COMPUTE;
        break;
    }
    case D3D_FEATURE_LEVEL_10_1:
    {
        m_pVertexShaderVersionString = "vs_4_1";
        m_pGeometryShaderVersionString = "gs_4_1";
        m_pPixelShaderVersionString = "ps_4_1";
        maxVertexShaderVersion = pShaderFactory->CreateVertexShaderVersion(4, 1);
        maxGeometryShaderVersion = pShaderFactory->CreateGeometryShaderVersion(4, 1);
        maxPixelShaderVersion = pShaderFactory->CreatePixelShaderVersion(4, 1);

        m_supportedShaderProgramTypeCount = 3;
        m_supportedShaderProgramTypes[0] = NiGPUProgram::PROGRAM_VERTEX;
        m_supportedShaderProgramTypes[1] = NiGPUProgram::PROGRAM_GEOMETRY;
        m_supportedShaderProgramTypes[2] = NiGPUProgram::PROGRAM_PIXEL;

        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwOptions;
        memset(&hwOptions, 0, sizeof(hwOptions));
        HRESULT hr = m_pD3D11Device->CheckFeatureSupport(
            D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS,
            &hwOptions,
            sizeof(hwOptions));
        EE_ASSERT(SUCCEEDED(hr));
        EE_UNUSED_ARG(hr);

        efd::Bool csSupport =
            (hwOptions.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x != 0);

        if (csSupport)
        {
            m_pComputeShaderVersionString = "cs_4_0";
            m_maxComputeShaderVersion = pShaderFactory->CreateComputeShaderVersion(4, 0);
            m_supportedShaderProgramTypeCount++;
            m_supportedShaderProgramTypes[3] = NiGPUProgram::PROGRAM_COMPUTE;
        }

        break;
    }
    case D3D_FEATURE_LEVEL_10_0:
    {
        m_pVertexShaderVersionString = "vs_4_0";
        m_pGeometryShaderVersionString = "gs_4_0";
        m_pPixelShaderVersionString = "ps_4_0";
        maxVertexShaderVersion = pShaderFactory->CreateVertexShaderVersion(4, 0);
        maxGeometryShaderVersion = pShaderFactory->CreateGeometryShaderVersion(4, 0);
        maxPixelShaderVersion = pShaderFactory->CreatePixelShaderVersion(4, 0);

        m_supportedShaderProgramTypeCount = 3;
        m_supportedShaderProgramTypes[0] = NiGPUProgram::PROGRAM_VERTEX;
        m_supportedShaderProgramTypes[1] = NiGPUProgram::PROGRAM_GEOMETRY;
        m_supportedShaderProgramTypes[2] = NiGPUProgram::PROGRAM_PIXEL;

        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwOptions;
        memset(&hwOptions, 0, sizeof(hwOptions));
        HRESULT hr = m_pD3D11Device->CheckFeatureSupport(
            D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS,
            &hwOptions,
            sizeof(hwOptions));
        EE_ASSERT(SUCCEEDED(hr));
        EE_UNUSED_ARG(hr);

        efd::Bool csSupport =
            (hwOptions.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x != 0);

        if (csSupport)
        {
            m_pComputeShaderVersionString = "cs_4_0";
            m_maxComputeShaderVersion = pShaderFactory->CreateComputeShaderVersion(4, 0);
            m_supportedShaderProgramTypeCount++;
            m_supportedShaderProgramTypes[3] = NiGPUProgram::PROGRAM_COMPUTE;
        }

        break;
    }
    case D3D_FEATURE_LEVEL_9_1:
        m_pVertexShaderVersionString = "vs_4_0_level_9_1";
        m_pPixelShaderVersionString = "ps_4_0_level_9_1";
        maxVertexShaderVersion = pShaderFactory->CreateVertexShaderVersion(2, 0);
        maxPixelShaderVersion = pShaderFactory->CreatePixelShaderVersion(2, 0);
        m_supportedShaderProgramTypeCount = 2;
        m_supportedShaderProgramTypes[0] = NiGPUProgram::PROGRAM_VERTEX;
        m_supportedShaderProgramTypes[1] = NiGPUProgram::PROGRAM_PIXEL;
        break;
    case D3D_FEATURE_LEVEL_9_2:
        m_pVertexShaderVersionString = "vs_4_0_level_9_1";
        m_pPixelShaderVersionString = "ps_4_0_level_9_1";
        maxVertexShaderVersion = pShaderFactory->CreateVertexShaderVersion(2, 0);
        maxPixelShaderVersion = pShaderFactory->CreatePixelShaderVersion(2, 0);
        m_supportedShaderProgramTypeCount = 2;
        m_supportedShaderProgramTypes[0] = NiGPUProgram::PROGRAM_VERTEX;
        m_supportedShaderProgramTypes[1] = NiGPUProgram::PROGRAM_PIXEL;
        break;
    case D3D_FEATURE_LEVEL_9_3:
        m_pVertexShaderVersionString = "vs_4_0_level_9_3";
        m_pPixelShaderVersionString = "ps_4_0_level_9_3";
        maxVertexShaderVersion = pShaderFactory->CreateVertexShaderVersion(3, 0);
        maxPixelShaderVersion = pShaderFactory->CreatePixelShaderVersion(3, 0);
        m_supportedShaderProgramTypeCount = 2;
        m_supportedShaderProgramTypes[0] = NiGPUProgram::PROGRAM_VERTEX;
        m_supportedShaderProgramTypes[1] = NiGPUProgram::PROGRAM_PIXEL;
        break;
    default:
        ReleaseManagers();
        ReleaseDevice();
        m_initialized = false;
        D3D11Error::ReportWarning(
            "Initialization failed unknown feature level 0x%08X returned.",
            m_featureLevel);
        return false;
    }

    // Inform the shadowing system of the shader model versions used by the
    // current hardware.
    if (NiShadowManager::GetShadowManager())
    {
        NiShadowManager::ValidateShaderVersions(
            (efd::UInt16)pShaderFactory->GetMajorVertexShaderVersion(
            maxVertexShaderVersion),
            (efd::UInt16)pShaderFactory->GetMinorVertexShaderVersion(
            maxVertexShaderVersion),
            (efd::UInt16)pShaderFactory->GetMajorGeometryShaderVersion(
            maxGeometryShaderVersion),
            (efd::UInt16)pShaderFactory->GetMinorGeometryShaderVersion(
            maxGeometryShaderVersion),
            (efd::UInt16)pShaderFactory->GetMajorPixelShaderVersion(
            maxPixelShaderVersion),
            (efd::UInt16)pShaderFactory->GetMinorPixelShaderVersion(
            maxPixelShaderVersion));
    }

    NiStandardMaterial* pStandardMaterial =
        NiDynamicCast(NiStandardMaterial, m_spInitialDefaultMaterial);
    if (pStandardMaterial)
    {
        NiRenderer::SetDefaultProgramCache(pStandardMaterial);
    }
    m_spCurrentDefaultMaterial = m_spInitialDefaultMaterial;

    // Shadow manager
    if (NiShadowManager::GetShadowManager())
    {
        NiDirectionalShadowWriteMaterial* pDirShadowWriteMaterial =
            (NiDirectionalShadowWriteMaterial*)NiShadowManager::GetShadowWriteMaterial(
            NiStandardMaterial::LIGHT_DIR);

        if (pDirShadowWriteMaterial)
        {
            NiRenderer::SetDefaultProgramCache(pDirShadowWriteMaterial);
        }

        NiPointShadowWriteMaterial* pPointShadowWriteMaterial =
            (NiPointShadowWriteMaterial*)NiShadowManager::GetShadowWriteMaterial(
            NiStandardMaterial::LIGHT_POINT);

        if (pPointShadowWriteMaterial)
        {
            NiRenderer::SetDefaultProgramCache(pPointShadowWriteMaterial);
        }

        NiSpotShadowWriteMaterial* pSpotShadowWriteMaterial =
            (NiSpotShadowWriteMaterial*)NiShadowManager::
            GetShadowWriteMaterial(NiStandardMaterial::LIGHT_SPOT);

        if (pSpotShadowWriteMaterial)
        {
            NiRenderer::SetDefaultProgramCache(pSpotShadowWriteMaterial);
        }
    }

    m_shaderLibraryVersion.SetSystemFeatureLevel(m_featureLevel);
    m_shaderLibraryVersion.SetMinFeatureLevel(D3D_FEATURE_LEVEL_9_1);
    m_shaderLibraryVersion.SetFeatureLevelRequest(m_featureLevel);
    m_shaderLibraryVersion.SetSystemComputeShaderVersion(m_maxComputeShaderVersion);
    m_shaderLibraryVersion.SetMinComputeShaderVersion(0, 0);
    m_shaderLibraryVersion.SetComputeShaderVersionRequest(m_maxComputeShaderVersion);
    m_shaderLibraryVersion.SetSystemUserVersion(0, 0);
    m_shaderLibraryVersion.SetMinUserVersion(0, 0);
    m_shaderLibraryVersion.SetUserVersionRequest(0, 0);
    m_shaderLibraryVersion.SetPlatform(NiShader::NISHADER_D3D11);

    D3D11ShaderFactory::GetD3D11ShaderFactory();

    NiShader* pErrorShader = EE_NEW D3D11ErrorShader;
    pErrorShader->Initialize();

    SetErrorShader(pErrorShader);

    // Initialize the safe zone to the inner 98% of the display
    m_kDisplaySafeZone.m_top =  0.01f;
    m_kDisplaySafeZone.m_left =  0.01f;
    m_kDisplaySafeZone.m_right =  0.99f;
    m_kDisplaySafeZone.m_bottom =  0.99f;

    m_isDeviceOccluded = false;
    m_isDeviceRemoved = false;

    return m_initialized;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateSwapChainRenderTargetGroup(
    DXGI_SWAP_CHAIN_DESC& swapChainDesc,
    efd::UInt32 outputIndex,
    efd::Bool createDepthStencilBuffer,
    DXGI_FORMAT depthStencilFormat)
{
    // Is there an output window?
    if (swapChainDesc.OutputWindow == NULL)
    {
        return false;
    }

    // Does a swap chain already exist for this window?
    NiRenderTargetGroupPtr spRenderTargetGroup;
    if (m_swapChainRenderTargetGroups.GetAt(swapChainDesc.OutputWindow, spRenderTargetGroup))
    {
        return false;
    }

    IDXGISwapChain* pSwapChain = CreateSwapChain(swapChainDesc, outputIndex);
    if (pSwapChain != NULL)
    {
        NiRenderTargetGroup* pRTGroup = CreateRenderTargetGroupFromSwapChain(
            pSwapChain,
            createDepthStencilBuffer,
            depthStencilFormat);

        m_swapChainRenderTargetGroups.SetAt(
            swapChainDesc.OutputWindow, pRTGroup);

        return true;
    }

    D3D11Error::ReportWarning("D3D11Renderer::CreateSwapChain failed in " __FUNCTION__);

    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::DestroySwapChainRenderTargetGroup(HWND hWnd)
{
    // Does the framebuffer exist?
    if (!hWnd)
    {
        return;
    }

    NiRenderTargetGroupPtr spRenderTargetGroup = NULL;

    // Does the framebuffer exist?
    if (!m_swapChainRenderTargetGroups.GetAt(hWnd, spRenderTargetGroup))
    {
        return;
    }

    // can only delete non-primary buffers that are not current targets
    if (spRenderTargetGroup != m_pCurrentRenderTargetGroup)
    {
        m_swapChainRenderTargetGroups.RemoveAt(hWnd);

        // Check to see if this is the default render target group
        if (spRenderTargetGroup == m_spDefaultRenderTargetGroup)
        {
            // Try to find another render target group to use as the default
            NiRenderTargetGroupPtr spNewDefaultRenderTargetGroup;
            if (!m_swapChainRenderTargetGroups.IsEmpty())
            {
                NiTMapIterator iter = m_swapChainRenderTargetGroups.GetFirstPos();
                HWND hNewWnd = NULL;
                m_swapChainRenderTargetGroups.GetNext(
                    iter,
                    hNewWnd,
                    spNewDefaultRenderTargetGroup);
            }

            m_spDefaultRenderTargetGroup = spNewDefaultRenderTargetGroup;
        }

        spRenderTargetGroup = NULL;
    }
    else
    {
        D3D11Error::ReportWarning(
            __FUNCTION__
            "destroying swap chain that is a current render target\n");
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::RecreateSwapChainRenderTargetGroup(
    DXGI_SWAP_CHAIN_DESC& swapChainDesc,
    efd::UInt32 outputIndex,
    efd::Bool createDepthStencilBuffer,
    DXGI_FORMAT depthStencilFormat)
{
    // Is there an output window?
    if (swapChainDesc.OutputWindow == NULL)
        return false;

    // Does a swap chain already exist for this window?
    NiRenderTargetGroupPtr spRenderTargetGroup;
    if (!m_swapChainRenderTargetGroups.GetAt(swapChainDesc.OutputWindow, spRenderTargetGroup))
        return false;

    IDXGISwapChain* pSwapChain = CreateSwapChain(swapChainDesc, outputIndex);
    if (pSwapChain == NULL)
        return false;

    if (!spRenderTargetGroup->AttachBuffer(NULL, 0))
        return false;

    if (!spRenderTargetGroup->AttachDepthStencilBuffer(NULL))
        return false;

    Ni2DBuffer* pBackBuffer;
    NiDepthStencilBuffer* pDepthBuffer;

    if (!CreateBuffersFromSwapChain(
        pSwapChain,
        createDepthStencilBuffer,
        depthStencilFormat,
        pBackBuffer,
        pDepthBuffer))
    {
        return false;
    }

    if (pBackBuffer)
    {
        spRenderTargetGroup->AttachBuffer(pBackBuffer, 0);

        if (pDepthBuffer)
            spRenderTargetGroup->AttachDepthStencilBuffer(pDepthBuffer);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
NiRenderTargetGroup* D3D11Renderer::GetSwapChainRenderTargetGroup(HWND hWnd) const
{
    if (!hWnd)
    {
        return NULL;
    }

    NiRenderTargetGroupPtr spRenderTargetGroup;

    // Does the framebuffer exist?
    if (m_swapChainRenderTargetGroups.GetAt(hWnd, spRenderTargetGroup))
        return spRenderTargetGroup;

    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::SetDefaultSwapChainRenderTargetGroup(HWND hWnd)
{
    NiRenderTargetGroup* pRTG = GetSwapChainRenderTargetGroup(hWnd);
    if (pRTG == NULL)
        return false;

    m_spDefaultRenderTargetGroup = pRTG;
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::EnsureD3D11Loaded()
{
    ms_libD3D11CriticalSection.Lock();

    efd::Bool success = true;
    if (ms_hD3D11 == NULL)
        success = LoadD3D11();

    ms_libD3D11CriticalSection.Unlock();

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::LoadD3D11()
{
    ms_libD3D11CriticalSection.Lock();

    if (ms_hD3D11 != NULL)
    {
        ms_libD3D11CriticalSection.Unlock();
        return true;
    }
    ms_hD3D11 = LoadLibrary("D3D11.dll");

    if (ms_hD3D11 == NULL)
    {
        D3D11Error::ReportError(D3D11Error::D3D11ERROR_D3D11_LIB_MISSING);
        ms_libD3D11CriticalSection.Unlock();
        return false;
    }

    ms_hD3D10 = LoadLibrary("D3D10.dll");
    if (ms_hD3D10 == NULL)
    {
        D3D11Error::ReportError(
            D3D11Error::D3D11ERROR_D3D11_LIB_MISSING,
            "Required D3D10 library not found.");

        ReleaseD3D11();
        ms_libD3D11CriticalSection.Unlock();

        return false;
    }

    ms_hD3D9 = LoadLibrary("D3D9.dll");

    if (ms_hD3D9 == NULL)
    {
        // No real problem; just that PIX markers won't pass through
    }

    ms_pD3D11CreateDeviceFunc = (PFN_D3D11_CREATE_DEVICE)
        GetProcAddress(ms_hD3D11, "D3D11CreateDevice");
    ms_pD3D11CreateDeviceAndSwapChainFunc = (PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN)
        GetProcAddress(ms_hD3D11, "D3D11CreateDeviceAndSwapChain");

    ms_pD3D10CreateBlob = (EE_LPD3D10CREATEBLOB)
        GetProcAddress(ms_hD3D10, "D3D10CreateBlob");

    if (ms_hD3D9)
    {
        ms_pD3DPERF_BeginEventFunc = (EE_LPD3D11PERF_BEGINEVENT)
            GetProcAddress(ms_hD3D9, "D3DPERF_BeginEvent");
        ms_pD3DPERF_EndEventFunc = (EE_LPD3D11PERF_ENDEVENT)
            GetProcAddress(ms_hD3D9, "D3DPERF_EndEvent");
        ms_pD3DPERF_GetStatusFunc = (EE_LPD3D11PERF_GETSTATUS)
            GetProcAddress(ms_hD3D9, "D3DPERF_GetStatus");
    }

    if (ms_pD3D11CreateDeviceFunc == NULL ||
        ms_pD3D11CreateDeviceAndSwapChainFunc == NULL)
    {
        D3D11Error::ReportError(
            D3D11Error::D3D11ERROR_D3D11_LIB_MISSING,
            "Library loaded but some procedure addresses not found; releasing library.");

        ReleaseD3D11();
        ms_libD3D11CriticalSection.Unlock();

        return false;
    }

    if (ms_pD3D10CreateBlob == NULL)
    {
        D3D11Error::ReportError(
            D3D11Error::D3D11ERROR_D3D11_LIB_MISSING,
            "Library loaded but some procedure addresses not found; releasing library.");

        ReleaseD3D11();
        ms_libD3D11CriticalSection.Unlock();

        return false;
    }

    ms_libD3D11CriticalSection.Unlock();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::ReleaseD3D11()
{
    ms_libD3D11CriticalSection.Lock();

    if (ms_hD3D11)
    {
        FreeLibrary(ms_hD3D11);
        ms_hD3D11 = NULL;
        ms_pD3D11CreateDeviceFunc = NULL;
        ms_pD3D11CreateDeviceAndSwapChainFunc = NULL;
    }

    if (ms_hD3D10)
    {
        FreeLibrary(ms_hD3D10);
        ms_hD3D10 = NULL;
        ms_pD3D10CreateBlob = NULL;
    }

    if (ms_hD3D9)
    {
        FreeLibrary(ms_hD3D9);
        ms_pD3DPERF_BeginEventFunc = NULL;
        ms_pD3DPERF_EndEventFunc = NULL;
        ms_pD3DPERF_GetStatusFunc = NULL;
    }

    ms_libD3D11CriticalSection.Unlock();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateDevice(CreationParameters& createParams)
{
    if ((createParams.m_swapChain.SampleDesc.Count != 1 ||
        createParams.m_swapChain.SampleDesc.Quality != 0) &&
        (createParams.m_createFlags & CREATE_DEVICE_SWITCH_TO_REF) != 0)
    {
        D3D11Error::ReportWarning(
            "SwitchToRef device not compatible with multisampled resources; "
            "disabling request for SwitchToRef device.");
        createParams.m_createFlags &= ~CREATE_DEVICE_SWITCH_TO_REF;
    }

    m_initialCreateParameters = createParams;

    D3D11SystemDesc::GetSystemDesc(m_spSystemDesc);
    if (m_spSystemDesc->IsEnumerationValid() == false)
    {
        if (m_spSystemDesc->Enumerate() == false)
        {
            D3D11Error::ReportWarning(
                "System enumeration failed; device cannot be created.");
            return false;
        }
    }

    //get instruction set support
    m_supportsMMX = efd::SystemDesc::GetSystemDesc().MMX_Supported();
    m_supportsSSE = efd::SystemDesc::GetSystemDesc().SSE_Supported();
    m_supportsSSE2 = efd::SystemDesc::GetSystemDesc().SSE2_Supported();

    // Determine if the application is being PIX captured or not
    if (ms_pD3DPERF_GetStatusFunc)
    {
        m_isPIXActive = (ms_pD3DPERF_GetStatusFunc() != 0);
    }

    efd::UInt32 adapterID = createParams.m_adapterIndex;
    if (adapterID == NULL_ADAPTER)
    {
        if (createParams.m_driverType != DRIVER_TYPE_REFERENCE ||
            createParams.m_driverType != DRIVER_TYPE_NULL)
        {
            createParams.m_driverType = DRIVER_TYPE_REFERENCE;
        }
    }
    else if (adapterID >= m_spSystemDesc->GetAdapterCount())
    {
        adapterID = 0;
    }

    const D3D11AdapterDesc* pAdapterDesc = m_spSystemDesc->GetAdapterDesc(adapterID);
    EE_ASSERT(pAdapterDesc);

    IDXGIAdapter* pAdapter = NULL;
    DriverType driverType = createParams.m_driverType;
    HMODULE hSoftware = NULL;

    ID3D11Device* pD3DDevice = NULL;
    ID3D11DeviceContext* pD3DDeviceContext = NULL;
    D3D_FEATURE_LEVEL currentLevel;
    HRESULT hr = S_OK;
    ms_libD3D11CriticalSection.Lock();

    if (EnsureD3D11Loaded())
    {
        if (createParams.m_driverType == DRIVER_TYPE_REFERENCE && !pAdapterDesc->IsPerfHUD())
        {
            adapterID = (efd::UInt32) NULL_ADAPTER;
        }

        if (adapterID != NULL_ADAPTER)
        {
            pAdapter = m_spSystemDesc->GetAdapter(adapterID);
            EE_ASSERT(pAdapter);
        }

        D3D_FEATURE_LEVEL proposedLevel;
        if (hSoftware)
        {
            // Software device
            driverType = DRIVER_TYPE_SOFTWARE;
            pAdapter = NULL;
        }
        else
        {
            // Hardware device
            if (pAdapter)
            {
                driverType = DRIVER_TYPE_UNKNOWN;
                proposedLevel = pAdapterDesc->GetHWDevice()->GetFeatureLevel();
                createParams.m_pFeatureLevels = (FeatureLevel*)&proposedLevel;
                createParams.m_featureLevelCount = 1;
            }
            else
            {
                EE_ASSERT(driverType != DRIVER_TYPE_UNKNOWN);
            }
        }


        hr = D3D11CreateDevice(
            pAdapter,
            D3D11Renderer::GetD3D11DriverType(driverType),
            NULL,
            createParams.m_createFlags,
            (D3D_FEATURE_LEVEL*)createParams.m_pFeatureLevels,
            createParams.m_featureLevelCount,
            D3D11_SDK_VERSION,
            &pD3DDevice,
            &currentLevel,
            &pD3DDeviceContext);

        if (pAdapter)
            pAdapter->Release();
    }
    else
    {
        D3D11Error::ReportWarning("D3D11 library failed loading; device cannot be created.");
        ms_libD3D11CriticalSection.Unlock();
        return false;
    }
    m_deviceThreadID = NiGetCurrentThreadId();

    ms_libD3D11CriticalSection.Unlock();

    if (FAILED(hr) || pD3DDevice == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_DEVICE_CREATION_FAILED,
                "Error HRESULT = 0x%08X.",
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_DEVICE_CREATION_FAILED,
                "No error message from D3D11, but device is NULL.");
        }

        if (pD3DDevice)
            pD3DDevice->Release();

        return false;
    }

    m_pD3D11Device = pD3DDevice;
    m_adapterIndex = adapterID;
    m_featureLevel = currentLevel;
    // DT33837 Support multiple device contexts.
    m_pImmediateD3D11DeviceContext = pD3DDeviceContext;
    m_pCurrentD3D11DeviceContext = pD3DDeviceContext;
    m_pCurrentD3D11DeviceContext->AddRef();

    StoreDeviceSettings();

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::StoreDeviceSettings()
{
    EE_ASSERT(m_pD3D11Device);
    // (DXGI_FORMAT)0 == DXGI_FORMAT_UNKNOWN
    efd::UInt32* pIterator = &(m_formatSupportArray[1]);
    for (efd::UInt32 i = 1; i < DXGI_FORMAT_COUNT; i++)
    {
        DXGI_FORMAT format = (DXGI_FORMAT)i;
        // These formats are deprecated.
        if (format == DXGI_FORMAT_B5G6R5_UNORM ||
            format == DXGI_FORMAT_B5G5R5A1_UNORM ||
            format == DXGI_FORMAT_B8G8R8A8_UNORM ||
            format == DXGI_FORMAT_B8G8R8X8_UNORM)
        {
            continue;
        }

        *pIterator = 0;
        HRESULT hr = m_pD3D11Device->CheckFormatSupport(format, pIterator);
        if (FAILED(hr))
        {
            // E_FAIL is the D3D11-specified return code when a format has no
            // hardware support at all (e.g. R1_UNORM).  It is not an error;
            // leave pIterator at 0 (no support flags) and move on silently.
            *pIterator = 0;
            if (hr != E_FAIL)
            {
                D3D11Error::ReportWarning(
                    "ID3D11Device::CheckFormatSupport failed on format %s; error HRESULT = 0x%08X.",
                    D3D11PixelFormat::GetFormatName(format, false),
                    (efd::UInt32)hr);
            }
        }
        pIterator++;
    }

    // Set replacement texture format
    // This is D3D11 - it's safe to assume that R8G8B8A8 is supported.
    EE_ASSERT((m_formatSupportArray[DXGI_FORMAT_R8G8B8A8_UNORM] &
        D3D11_FORMAT_SUPPORT_TEXTURE2D) != 0);
    NiTexture::RendererData::SetTextureReplacementFormat(
        D3D11PixelFormat::EE_FORMAT_R8G8B8A8_UNORM);

    D3D11_FEATURE_DATA_THREADING threadingSupport;
    HRESULT hr = m_pD3D11Device->CheckFeatureSupport(
        D3D11_FEATURE_THREADING,
        &threadingSupport,
        sizeof(threadingSupport));
    EE_ASSERT(SUCCEEDED(hr));
    EE_UNUSED_ARG(hr);

    m_backgroundRendering = threadingSupport.DriverCommandLists != 0;
    m_backgroundResourceCreation = threadingSupport.DriverConcurrentCreates != 0;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::CreateManagers()
{
    m_pDataStreamFactory = EE_NEW D3D11DataStreamFactory;

    efd::Bool hsdsSupport = false;
    efd::Bool gsSupport = false;
    efd::Bool csSupport = false;
    efd::Bool csDownLevel = false;
    if (m_featureLevel >= D3D_FEATURE_LEVEL_11_0)
    {
        hsdsSupport = true;
        gsSupport = true;
        csSupport = true;
    }
    else if (m_featureLevel >= D3D_FEATURE_LEVEL_10_0)
    {
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwOptions;
        memset(&hwOptions, 0, sizeof(hwOptions));
        HRESULT hr = m_pD3D11Device->CheckFeatureSupport(
            D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS,
            &hwOptions,
            sizeof(hwOptions));
        EE_ASSERT(SUCCEEDED(hr));
        EE_UNUSED_ARG(hr);

        gsSupport = true;
        csSupport =
            (hwOptions.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x != 0);
        csDownLevel = true;
    }

    m_pDeviceState = EE_NEW D3D11DeviceState(
        m_pCurrentD3D11DeviceContext,
        hsdsSupport,
        gsSupport,
        csSupport,
        csDownLevel);
    m_pRenderStateManager = EE_NEW D3D11RenderStateManager(m_pD3D11Device, m_pDeviceState);
    m_pResourceManager = EE_NEW D3D11ResourceManager(m_pD3D11Device);
    m_pShaderConstantManager = EE_NEW D3D11ShaderConstantManager(m_pDeviceState);
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::ReleaseManagers()
{
    EE_DELETE m_pDataStreamFactory;

    EE_DELETE m_pDeviceState;
    EE_DELETE m_pRenderStateManager;
    EE_DELETE m_pResourceManager;
    EE_DELETE m_pShaderConstantManager;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::AddOccludedNotificationFunc(
    OCCLUDEDNOTIFYFUNC pNotifyFunc,
    void* pData)
{
    efd::UInt32 slot = m_occludedNotifyFuncs.Add(pNotifyFunc);
    m_occludedNotifyFuncData.SetAtGrow(slot, pData);

    return slot;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::RemoveOccludedNotificationFunc(
    OCCLUDEDNOTIFYFUNC pNotifyFunc)
{
    efd::UInt32 index = FindOccludedNotificationFunc(pNotifyFunc);
    if (index == EE_UINT32_MAX)
        return false;

    m_occludedNotifyFuncs.RemoveAt(index);
    m_occludedNotifyFuncData.RemoveAt(index);
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::RemoveOccludedNotificationFunc(efd::UInt32 index)
{
    if (m_occludedNotifyFuncs.GetAt(index) == 0)
        return false;

    m_occludedNotifyFuncs.RemoveAt(index);
    m_occludedNotifyFuncData.RemoveAt(index);
    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::RemoveAllOccludedNotificationFuncs()
{
    m_occludedNotifyFuncs.RemoveAll();
    m_occludedNotifyFuncData.RemoveAll();
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::ChangeOccludedNotificationFuncData(
    OCCLUDEDNOTIFYFUNC pNotifyFunc,
    void* pData)
{
    efd::UInt32 index = FindOccludedNotificationFunc(pNotifyFunc);
    if (index == EE_UINT32_MAX)
        return false;

    EE_ASSERT(m_occludedNotifyFuncData.GetSize() > index);
    m_occludedNotifyFuncData.SetAt(index, pData);
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::ChangeOccludedNotificationFuncData(efd::UInt32 index, void* pData)
{
    if (m_occludedNotifyFuncs.GetAt(index) == 0)
        return false;

    EE_ASSERT(m_occludedNotifyFuncData.GetSize() > index);
    m_occludedNotifyFuncData.SetAt(index, pData);
    return true;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::GetOccludedNotificationFuncCount() const
{
    return m_occludedNotifyFuncs.GetEffectiveSize();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::GetOccludedNotificationFuncArrayCount() const
{
    return m_occludedNotifyFuncs.GetSize();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::FindOccludedNotificationFunc(OCCLUDEDNOTIFYFUNC pNotifyFunc) const
{
    for (efd::UInt32 i = 0; i < m_occludedNotifyFuncs.GetSize(); i++)
    {
        if (m_occludedNotifyFuncs.GetAt(i) == pNotifyFunc)
            return i;
    }
    return EE_UINT32_MAX;
}

//------------------------------------------------------------------------------------------------
D3D11Renderer::OCCLUDEDNOTIFYFUNC D3D11Renderer::GetOccludedNotificationFunc(
    efd::UInt32 index) const
{
    return m_occludedNotifyFuncs.GetAt(index);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::AddDeviceRemovedNotificationFunc(
    DEVICEREMOVEDNOTIFYFUNC pNotifyFunc,
    void* pData)
{
    efd::UInt32 slot = m_deviceRemovedNotifyFuncs.Add(pNotifyFunc);
    m_deviceRemovedNotifyFuncData.SetAtGrow(slot, pData);

    return slot;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::RemoveDeviceRemovedNotificationFunc(DEVICEREMOVEDNOTIFYFUNC pNotifyFunc)
{
    efd::UInt32 index = FindDeviceRemovedNotificationFunc(pNotifyFunc);
    if (index == EE_UINT32_MAX)
        return false;

    m_deviceRemovedNotifyFuncs.RemoveAt(index);
    m_deviceRemovedNotifyFuncData.RemoveAt(index);
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::RemoveDeviceRemovedNotificationFunc(efd::UInt32 index)
{
    if (m_deviceRemovedNotifyFuncs.GetAt(index) == 0)
        return false;

    m_deviceRemovedNotifyFuncs.RemoveAt(index);
    m_deviceRemovedNotifyFuncData.RemoveAt(index);
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::ChangeDeviceRemovedNotificationFuncData(
    DEVICEREMOVEDNOTIFYFUNC pNotifyFunc,
    void* pData)
{
    efd::UInt32 index = FindDeviceRemovedNotificationFunc(pNotifyFunc);
    if (index == EE_UINT32_MAX)
        return false;

    EE_ASSERT(m_deviceRemovedNotifyFuncData.GetSize() > index);
    m_deviceRemovedNotifyFuncData.SetAt(index, pData);
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::ChangeDeviceRemovedNotificationFuncData(efd::UInt32 index, void* pData)
{
    if (m_deviceRemovedNotifyFuncs.GetAt(index) == 0)
        return false;

    EE_ASSERT(m_deviceRemovedNotifyFuncData.GetSize() > index);
    m_deviceRemovedNotifyFuncData.SetAt(index, pData);
    return true;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::FindDeviceRemovedNotificationFunc(
    DEVICEREMOVEDNOTIFYFUNC pNotifyFunc) const
{
    for (efd::UInt32 i = 0; i < m_deviceRemovedNotifyFuncs.GetSize(); i++)
    {
        if (m_deviceRemovedNotifyFuncs.GetAt(i) == pNotifyFunc)
            return i;
    }
    return EE_UINT32_MAX;
}

//------------------------------------------------------------------------------------------------
IDXGISwapChain* D3D11Renderer::CreateSwapChain(
    DXGI_SWAP_CHAIN_DESC& swapChainDesc,
    efd::UInt32 outputIndex)
{
    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning(
            "Can't call CreateSwapChain without a valid device.");
        return NULL;
    }
    EE_ASSERT(m_spSystemDesc);

    EE_ASSERT(m_adapterIndex == NULL_ADAPTER ||
        m_adapterIndex < m_spSystemDesc->GetAdapterCount());

    DXGI_SWAP_CHAIN_DESC actualSwapChainDesc = swapChainDesc;
    if (m_adapterIndex != NULL_ADAPTER && swapChainDesc.Windowed == false)
    {
        const D3D11AdapterDesc* pAdapterDesc =
            m_spSystemDesc->GetAdapterDesc(m_adapterIndex);
        EE_ASSERT(pAdapterDesc);

        efd::UInt32 outputID = outputIndex;
        if (outputID >= pAdapterDesc->GetOutputCount())
            outputID = 0;
        const D3D11OutputDesc* pOutputDesc = pAdapterDesc->GetOutputDesc(outputID);
        if (pOutputDesc)
        {
            // FindClosestMatchingMode refines the buffer desc for the target
            // output; skip it gracefully when no outputs are available
            // (headless / Optimus adapter — windowed path already skips this).
            IDXGIOutput* pOutput = pOutputDesc->GetOutput();
            EE_ASSERT(pOutput);
            pOutput->FindClosestMatchingMode(
                &(swapChainDesc.BufferDesc),
                &(actualSwapChainDesc.BufferDesc),
                m_pD3D11Device);
        }
    }

    IDXGIFactory* pFactory = m_spSystemDesc->GetFactory();
    EE_ASSERT(pFactory);

    IDXGISwapChain* pSwapChain = NULL;
    HRESULT hr = pFactory->CreateSwapChain(m_pD3D11Device, &actualSwapChainDesc, &pSwapChain);

    if (FAILED(hr) || pSwapChain == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SWAP_CHAIN_CREATION_FAILED,
                "Error HRESULT = 0x%08X.",
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SWAP_CHAIN_CREATION_FAILED,
                "No error message from D3D11, but swap chain is NULL.");
        }

        if (pSwapChain)
            pSwapChain->Release();

        return NULL;
    }

    return pSwapChain;
}

//------------------------------------------------------------------------------------------------
NiRenderTargetGroup* D3D11Renderer::CreateRenderTargetGroupFromSwapChain(
    IDXGISwapChain* pSwapChain,
    efd::Bool createDepthStencilBuffer,
    DXGI_FORMAT depthStencilFormat)
{
    Ni2DBuffer* pBackBuffer;
    NiDepthStencilBuffer* pDepthBuffer;

    if (!CreateBuffersFromSwapChain(
        pSwapChain,
        createDepthStencilBuffer,
        depthStencilFormat,
        pBackBuffer,
        pDepthBuffer))
    {
        return NULL;
    }

    NiRenderTargetGroup* pRTGroup = NiRenderTargetGroup::Create(
        pBackBuffer,
        this,
        pDepthBuffer);

    return pRTGroup;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateBuffersFromSwapChain(
    IDXGISwapChain* pSwapChain,
    efd::Bool createDepthStencilBuffer,
    DXGI_FORMAT depthStencilFormat,
    Ni2DBuffer*& pBackBuffer,
    NiDepthStencilBuffer*& pDepthBuffer)
{
    LogRendererState("CreateBuffersFromSwapChain.entry", this);

    pBackBuffer = NULL;
    pDepthBuffer = NULL;

    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning("Can't call "
            "CreateRenderTargetGroupFromSwapChain without a valid device.");
        return false;
    }

    if (pSwapChain == NULL)
    {
        D3D11Error::ReportWarning("Can't call "
            "CreateRenderTargetGroupFromSwapChain without a valid "
            "swap chain.");
        return false;
    }

    D3D11SwapChainBufferData* pSwapChainBufferData =
        D3D11SwapChainBufferData::Create(pSwapChain, pBackBuffer, this);
    if (pSwapChainBufferData == NULL || pBackBuffer == NULL)
    {
        D3D11Error::ReportWarning(
            "CreateRenderTargetGroupFromSwapChain failed to create swap chain buffer data.");
        return false;
    }

    if (createDepthStencilBuffer)
    {
        if (DoesFormatSupportFlag(depthStencilFormat,
            D3D11_FORMAT_SUPPORT_DEPTH_STENCIL))
        {
            const NiPixelFormat* pFormat =
                D3D11PixelFormat::ObtainFromDXGIFormat(depthStencilFormat);
            EE_ASSERT(pFormat);
            pDepthBuffer = NiDepthStencilBuffer::Create(
                pBackBuffer->GetWidth(),
                pBackBuffer->GetHeight(),
                this,
                *pFormat,
                pBackBuffer->GetMSAAPref());
        }
        else
        {
            pDepthBuffer = NiDepthStencilBuffer::CreateCompatible(pBackBuffer, this);

            if (pDepthBuffer == NULL)
            {
                D3D11Error::ReportWarning(
                    "Compatible depth/stencil buffer could not be created; "
                    "render target group will have no depth/stencil buffer.");
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::ReleaseDevice()
{
    // DT33837 Support multiple device contexts.
    if (m_pImmediateD3D11DeviceContext)
    {
        m_pImmediateD3D11DeviceContext->Release();
        m_pImmediateD3D11DeviceContext = NULL;
    }

    if (m_pCurrentD3D11DeviceContext)
    {
        m_pCurrentD3D11DeviceContext->Release();
        m_pCurrentD3D11DeviceContext = NULL;
    }

    if (m_pD3D11Device)
    {
        m_pD3D11Device->Release();
        m_pD3D11Device = NULL;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateTempDevice(IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE driverType,
    efd::UInt32 flags,
    D3D_FEATURE_LEVEL& featureLevel,
    ID3D11Device*& pDevice,
    ID3D11DeviceContext*& pDeviceContext)
{
    pDevice = NULL;
    pDeviceContext = NULL;

    HRESULT hr = S_OK;
    ms_libD3D11CriticalSection.Lock();
    if (EnsureD3D11Loaded())
    {
        // Check for the NVPerfHUD device.
        efd::Bool isPerfHUD = false;
        if (pAdapter)
        {
            DXGI_ADAPTER_DESC adapterDesc;
            hr = pAdapter->GetDesc(&adapterDesc);
            if (SUCCEEDED(hr))
            {
                isPerfHUD = (wcsstr(adapterDesc.Description, L"PerfHUD") != NULL);
            }
            driverType = D3D_DRIVER_TYPE_UNKNOWN;
        }

        // If we have NVPerfHUD or a hardware
        // device type, create temp device.
        if (driverType == D3D_DRIVER_TYPE_REFERENCE && !isPerfHUD)
            pAdapter = NULL;

        hr = D3D11CreateDevice(
            pAdapter,
            driverType,
            NULL,
            flags,
            NULL,
            0,
            D3D11_SDK_VERSION,
            &pDevice,
            &featureLevel,
            &pDeviceContext);

        if (FAILED(hr) || pDevice == NULL)
        {
            if (FAILED(hr))
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_DEVICE_CREATION_FAILED,
                    "Error HRESULT = 0x%08X.",
                    (efd::UInt32)hr);
            }
            else
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_DEVICE_CREATION_FAILED,
                    "No error message from D3D11, but device is NULL.");
            }

            if (pDevice)
            {
                pDevice->Release();
                pDevice = NULL;
            }
            if (pDeviceContext)
            {
                pDeviceContext->Release();
                pDeviceContext = NULL;
            }
        }
    }
    else
    {
        D3D11Error::ReportWarning("D3D11 library failed to be loaded; device cannot be created.");
    }
    ms_libD3D11CriticalSection.Unlock();
    return (pDevice != NULL);
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11Renderer::GetDriverInfo() const
{
    const D3D11AdapterDesc* pAdapterDesc = m_spSystemDesc->GetAdapterDesc(m_adapterIndex);
    EE_ASSERT(pAdapterDesc);

    const DXGI_ADAPTER_DESC* pD3D11AdapterDesc = pAdapterDesc->GetDesc();
    EE_ASSERT(pD3D11AdapterDesc);

    efd::SInt32 result = WideCharToMultiByte(
        CP_ACP,
        0,
        pD3D11AdapterDesc->Description,
        -1,
        m_driverDesc,
        sizeof(m_driverDesc),
        NULL,
        NULL);

    EE_ASSERT(result < sizeof(m_driverDesc));

    if (result > 0)
    {
        const efd::Char* pDriverType = NULL;
        switch (m_initialCreateParameters.m_driverType)
        {
        case DRIVER_TYPE_HARDWARE:
            pDriverType = "Hardware driver";
            break;
        case DRIVER_TYPE_REFERENCE:
            pDriverType = "Reference driver";
            break;
        case DRIVER_TYPE_NULL:
            pDriverType = "Null driver";
            break;
        case DRIVER_TYPE_SOFTWARE:
            pDriverType = "Software driver";
            break;
        case DRIVER_TYPE_WARP:
            pDriverType = "Warp driver";
            break;
        case DRIVER_TYPE_UNKNOWN:
        default:
            pDriverType = "Unknown driver";
            break;
        }

        efd::Sprintf(
            m_driverDesc + result - 1,
            sizeof(m_driverDesc) - result,
            " (%s)",
            pDriverType);
    }

    return m_driverDesc;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::GetFlags() const
{
    switch (m_featureLevel)
    {
    case D3D_FEATURE_LEVEL_9_1:
    case D3D_FEATURE_LEVEL_9_2:
        return CAPS_HARDWARESKINNING |
            CAPS_NONPOW2_TEXT |
            CAPS_AA_RENDERED_TEXTURES |
            CAPS_ANISO_FILTERING;
    case D3D_FEATURE_LEVEL_9_3:
    case D3D_FEATURE_LEVEL_10_0:
    case D3D_FEATURE_LEVEL_10_1:
    case D3D_FEATURE_LEVEL_11_0:
        return CAPS_HARDWARESKINNING |
            CAPS_NONPOW2_TEXT |
            CAPS_AA_RENDERED_TEXTURES |
            CAPS_HARDWAREINSTANCING |
            CAPS_ANISO_FILTERING;
    default:
        EE_FAIL("Invalid feature level");
        return 0;
    }
}

//------------------------------------------------------------------------------------------------
efd::SystemDesc::RendererID D3D11Renderer::GetRendererID() const
{
    return efd::SystemDesc::RENDERER_D3D11;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::SetDepthClear(const efd::Float32 depthClear)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    m_depthClear = depthClear;
}

//------------------------------------------------------------------------------------------------
efd::Float32 D3D11Renderer::GetDepthClear() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    return m_depthClear;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::SetBackgroundColor(const NiColor& color)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    m_backgroundColor[0] = color.r;
    m_backgroundColor[1] = color.g;
    m_backgroundColor[2] = color.b;
    m_backgroundColor[3] = 1.0f;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::SetBackgroundColor(const NiColorA& color)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    m_backgroundColor[0] = color.r;
    m_backgroundColor[1] = color.g;
    m_backgroundColor[2] = color.b;
    m_backgroundColor[3] = color.a;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::GetBackgroundColor(NiColorA& color) const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    color.r = m_backgroundColor[0];
    color.g = m_backgroundColor[1];
    color.b = m_backgroundColor[2];
    color.a = m_backgroundColor[3];
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::SetStencilClear(efd::UInt32 stencilClear)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    m_stencilClear = (efd::UInt8)(stencilClear & 0x000000FF);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::GetStencilClear() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    return m_stencilClear;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::ValidateRenderTargetGroup(NiRenderTargetGroup* pTarget)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (pTarget == NULL)
        return false;

    D3D11_RTV_DIMENSION resourceType = D3D11_RTV_DIMENSION_UNKNOWN;
    efd::UInt32 elementOffset = EE_UINT32_MAX;
    efd::UInt32 elementWidth = EE_UINT32_MAX;
    efd::UInt32 mipSlice = EE_UINT32_MAX;
    efd::UInt32 arraySize = EE_UINT32_MAX;
    efd::UInt32 bufferWidth = EE_UINT32_MAX;
    efd::UInt32 bufferHeight = EE_UINT32_MAX;
    efd::Bool isCubeMap = false;

    Ni2DBuffer* pFirstBuffer = NULL;

    for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        // Note that finding *no* render target in the render target group
        // is still valid.
        D3D112DBufferData* p2DBufferData = (D3D112DBufferData*)pTarget->GetBufferRendererData(i);
        if (p2DBufferData == NULL)
            continue;

        if (pFirstBuffer == NULL)
            pFirstBuffer = pTarget->GetBuffer(i);

        D3D11RenderTargetBufferData* pRTBufferData =
            NiVerifyStaticCast(D3D11RenderTargetBufferData, p2DBufferData);

        ID3D11RenderTargetView* pRTView = pRTBufferData->GetRenderTargetView();

        if (pRTView == NULL)
            continue;

        D3D11_RENDER_TARGET_VIEW_DESC rtViewDesc;
        pRTView->GetDesc(&rtViewDesc);

        // Get 2D texture description
        ID3D11Texture2D* pRTTexture = pRTBufferData->GetRenderTargetBuffer();

        if (pRTTexture == NULL)
            continue;

        D3D11_TEXTURE2D_DESC rtTextureDesc;
        pRTTexture->GetDesc(&rtTextureDesc);

        // Check for un-set resource type
        if (resourceType == D3D11_RTV_DIMENSION_UNKNOWN)
        {
            resourceType = rtViewDesc.ViewDimension;
            isCubeMap = (0 != (rtTextureDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE));
        }

        // Check for mismatched cube map flag
        if (isCubeMap != ((rtTextureDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0))
        {
            return false;
        }

        EE_ASSERT(resourceType != D3D11_RTV_DIMENSION_UNKNOWN);

        // Check for mismatched resource data
        if (resourceType == D3D11_RTV_DIMENSION_BUFFER)
        {
            if (elementOffset == EE_UINT32_MAX)
            {
                EE_ASSERT(elementWidth == EE_UINT32_MAX);
                elementOffset = rtViewDesc.Buffer.ElementOffset;
                elementWidth = rtViewDesc.Buffer.ElementWidth;
            }
            else if (rtViewDesc.ViewDimension != D3D11_RTV_DIMENSION_BUFFER ||
                elementOffset != rtViewDesc.Buffer.ElementOffset ||
                elementWidth != rtViewDesc.Buffer.ElementWidth)
            {
                return false;
            }
        }
        else if (resourceType == D3D11_RTV_DIMENSION_TEXTURE1D)
        {
            efd::UInt32 tempMipSlice = EE_UINT32_MAX;
            if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1D)
            {
                // OK
                tempMipSlice = rtViewDesc.Texture1D.MipSlice;
            }
            else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1DARRAY)
            {
                if (mipSlice == EE_UINT32_MAX ||
                    rtViewDesc.Texture1DArray.ArraySize == 1)
                {
                    // OK
                }
                else
                {
                    return false;
                }
                tempMipSlice = rtViewDesc.Texture1DArray.MipSlice;
            }
            else
            {
                return false;
            }

            if (mipSlice == EE_UINT32_MAX)
                mipSlice = tempMipSlice;
            else if (mipSlice != tempMipSlice)
                return false;

            EE_ASSERT(rtViewDesc.Texture1D.MipSlice == 0);

            // For 1D resources, the height must be 1.
            EE_ASSERT(p2DBufferData->GetHeight() == 1);
            if (bufferWidth == EE_UINT32_MAX)
            {
                bufferWidth = p2DBufferData->GetWidth();
            }
            else if (p2DBufferData->GetWidth() != bufferWidth)
            {
                return false;
            }
        }
        else if (resourceType == D3D11_RTV_DIMENSION_TEXTURE1DARRAY)
        {
            efd::UInt32 tempMipSlice = EE_UINT32_MAX;
            if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1D)
            {
                if (arraySize == EE_UINT32_MAX || arraySize == 1)
                {
                    // OK
                }
                else
                {
                    return false;
                }
                tempMipSlice = rtViewDesc.Texture1D.MipSlice;
            }
            else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1DARRAY)
            {
                if (arraySize == EE_UINT32_MAX ||
                    rtViewDesc.Texture1DArray.ArraySize == arraySize)
                {
                    // OK
                }
                else
                {
                    return false;
                }
                tempMipSlice = rtViewDesc.Texture1DArray.MipSlice;
            }
            else
            {
                return false;
            }

            if (mipSlice == EE_UINT32_MAX)
            {
                EE_ASSERT(arraySize == EE_UINT32_MAX);
                mipSlice = tempMipSlice;
                arraySize = rtViewDesc.Texture1DArray.ArraySize;
            }
            else if (mipSlice != tempMipSlice)
            {
                return false;
            }

            EE_ASSERT(rtViewDesc.Texture1DArray.MipSlice == 0);

            // For 1D resources, the height must be 1.
            EE_ASSERT(p2DBufferData->GetHeight() == 1);
            if (bufferWidth == EE_UINT32_MAX)
            {
                bufferWidth = p2DBufferData->GetWidth();
            }
            else if (p2DBufferData->GetWidth() != bufferWidth)
            {
                return false;
            }
        }
        else if (resourceType == D3D11_RTV_DIMENSION_TEXTURE2D)
        {
            efd::UInt32 tempMipSlice = EE_UINT32_MAX;
            if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
            {
                // OK
                tempMipSlice = rtViewDesc.Texture2D.MipSlice;
            }
            else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY)
            {
                if (mipSlice == EE_UINT32_MAX ||
                    rtViewDesc.Texture2DArray.ArraySize == 1)
                {
                    // OK
                }
                else
                {
                    return false;
                }
                tempMipSlice = rtViewDesc.Texture2DArray.MipSlice;
            }
            else
            {
                return false;
            }

            if (mipSlice == EE_UINT32_MAX)
                mipSlice = tempMipSlice;
            else if (mipSlice != tempMipSlice)
                return false;

            EE_ASSERT(rtViewDesc.Texture2D.MipSlice == 0);

            if (bufferWidth == EE_UINT32_MAX)
            {
                EE_ASSERT(bufferHeight == EE_UINT32_MAX);
                bufferWidth = p2DBufferData->GetWidth();
                bufferHeight = p2DBufferData->GetHeight();
            }
            else if (p2DBufferData->GetWidth() != bufferWidth ||
                p2DBufferData->GetHeight() != bufferHeight)
            {
                return false;
            }
        }
        else if (resourceType == D3D11_RTV_DIMENSION_TEXTURE2DARRAY)
        {
            efd::UInt32 tempMipSlice = EE_UINT32_MAX;
            if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
            {
                if (arraySize == EE_UINT32_MAX || arraySize == 1)
                {
                    // OK
                }
                else
                {
                    return false;
                }
                tempMipSlice = rtViewDesc.Texture2D.MipSlice;
            }
            else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY)
            {
                if (arraySize == EE_UINT32_MAX ||
                    rtViewDesc.Texture2DArray.ArraySize == arraySize)
                {
                    // OK
                }
                else
                {
                    return false;
                }
                tempMipSlice = rtViewDesc.Texture2DArray.MipSlice;
            }
            else
            {
                return false;
            }

            if (mipSlice == EE_UINT32_MAX)
            {
                EE_ASSERT(arraySize == EE_UINT32_MAX);
                mipSlice = tempMipSlice;
                arraySize = rtViewDesc.Texture2DArray.ArraySize;
            }
            else if (mipSlice != tempMipSlice)
            {
                return false;
            }

            EE_ASSERT(rtViewDesc.Texture2DArray.MipSlice == 0);

            if (bufferWidth == EE_UINT32_MAX)
            {
                EE_ASSERT(bufferHeight == EE_UINT32_MAX);
                bufferWidth = p2DBufferData->GetWidth();
                bufferHeight = p2DBufferData->GetHeight();
            }
            else if (p2DBufferData->GetWidth() != bufferWidth ||
                p2DBufferData->GetHeight() != bufferHeight)
            {
                return false;
            }
        }
        else if (resourceType == D3D11_RTV_DIMENSION_TEXTURE2DMS)
        {
            if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
            {
                // OK
            }
            else if (rtViewDesc.ViewDimension ==
                D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY)
            {
                if (rtViewDesc.Texture2DMSArray.ArraySize == 1)
                {
                    // OK
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }

            // D3D11 doesn't support mipmapped multisampled textures.
            if (bufferWidth == EE_UINT32_MAX)
            {
                EE_ASSERT(bufferHeight == EE_UINT32_MAX);
                bufferWidth = p2DBufferData->GetWidth();
                bufferHeight = p2DBufferData->GetHeight();
            }
            else if (p2DBufferData->GetWidth() != bufferWidth ||
                p2DBufferData->GetHeight() != bufferHeight)
            {
                return false;
            }
        }
        else if (resourceType == D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY)
        {
            if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
            {
                if (arraySize == EE_UINT32_MAX || arraySize == 1)
                {
                    // OK
                }
                else
                {
                    return false;
                }
            }
            else if (rtViewDesc.ViewDimension ==
                D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY)
            {
                if (arraySize == EE_UINT32_MAX ||
                    rtViewDesc.Texture2DMSArray.ArraySize == arraySize)
                {
                    // OK
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }

            if (arraySize == EE_UINT32_MAX)
            {
                arraySize = rtViewDesc.Texture2DArray.ArraySize;
            }

            // D3D11 doesn't support mipmapped multisampled textures.
            if (bufferWidth == EE_UINT32_MAX)
            {
                EE_ASSERT(bufferHeight == EE_UINT32_MAX);
                bufferWidth = p2DBufferData->GetWidth();
                bufferHeight = p2DBufferData->GetHeight();
            }
            else if (p2DBufferData->GetWidth() != bufferWidth ||
                p2DBufferData->GetHeight() != bufferHeight)
            {
                return false;
            }
        }
        else if (resourceType == D3D11_RTV_DIMENSION_TEXTURE3D)
        {
            if (mipSlice == EE_UINT32_MAX)
            {
                EE_ASSERT(arraySize == EE_UINT32_MAX);
                mipSlice = rtViewDesc.Texture3D.MipSlice;
                arraySize = rtViewDesc.Texture3D.WSize;
            }
            else if (rtViewDesc.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE3D ||
                mipSlice != rtViewDesc.Texture3D.MipSlice ||
                arraySize != rtViewDesc.Texture3D.WSize)
            {
                return false;
            }
            EE_ASSERT(rtViewDesc.Texture3D.MipSlice == 0);

            if (bufferWidth == EE_UINT32_MAX)
            {
                // Third dimension is checked in WSize above.
                EE_ASSERT(bufferHeight == EE_UINT32_MAX);
                bufferWidth = p2DBufferData->GetWidth();
                bufferHeight = p2DBufferData->GetHeight();
            }
            else if (p2DBufferData->GetWidth() != bufferWidth ||
                p2DBufferData->GetHeight() != bufferHeight)
            {
                return false;
            }
        }

        // Check for duplicate views
        for (efd::UInt32 j = 0; j < i; j++)
        {
            D3D112DBufferData* pOther2DBufferData =
                (D3D112DBufferData*)pTarget->GetBufferRendererData(j);
            if (pOther2DBufferData == NULL)
                continue;

            EE_ASSERT(NiIsKindOf(D3D11RenderTargetBufferData,
                pOther2DBufferData));

            D3D11RenderTargetBufferData* pOtherRTBufferData =
                (D3D11RenderTargetBufferData*)pOther2DBufferData;

            ID3D11RenderTargetView* pOtherRTView =
                pOtherRTBufferData->GetRenderTargetView();

            if (pOtherRTView == pRTView)
                return false;
        }
    }

    // Check depth stencil buffer
    // Assume that the depth stencil buffer only needs to be tested against
    // one active color buffer.
    NiDepthStencilBuffer* pDSBuffer = pTarget->GetDepthStencilBuffer();
    if (pDSBuffer)
    {
        if (IsDepthBufferCompatible(pFirstBuffer, pDSBuffer) == false)
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::IsDepthBufferCompatible(
    Ni2DBuffer* pBuffer,
    NiDepthStencilBuffer* pDSBuffer)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    // Note that finding *no* render target in the render target group
    // is still valid.
    if (pBuffer == NULL || pDSBuffer == NULL)
        return true;

    // Get 2D buffer description
    D3D112DBufferData* p2DBufferData =
        (D3D112DBufferData*)pBuffer->GetRendererData();
    if (p2DBufferData == NULL)
        return false;

    EE_ASSERT(NiIsKindOf(D3D11RenderTargetBufferData, p2DBufferData));

    D3D11RenderTargetBufferData* pRTBufferData =
        (D3D11RenderTargetBufferData*)p2DBufferData;

    ID3D11RenderTargetView* pRTView = pRTBufferData->GetRenderTargetView();

    if (pRTView == NULL)
        return false;

    D3D11_RENDER_TARGET_VIEW_DESC rtViewDesc;
    pRTView->GetDesc(&rtViewDesc);

    // Get 2D texture description
    ID3D11Texture2D* pRTTexture = pRTBufferData->GetRenderTargetBuffer();

    if (pRTTexture == NULL)
        return false;

    D3D11_TEXTURE2D_DESC rtTextureDesc;
    pRTTexture->GetDesc(&rtTextureDesc);

    // Get depth/stencil buffer description
    p2DBufferData = (D3D112DBufferData*)pDSBuffer->GetRendererData();
    if (p2DBufferData == NULL)
        return false;

    D3D11DepthStencilBufferData* pDSBufferData =
        NiVerifyStaticCast(D3D11DepthStencilBufferData, p2DBufferData);

    ID3D11DepthStencilView* pDSView = pDSBufferData->GetDepthStencilView();

    if (pDSView == NULL)
        return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc;
    pDSView->GetDesc(&dsViewDesc);

    // Get depth/stencil texture description
    ID3D11Texture2D* pDSTexture = pDSBufferData->GetDepthStencilBuffer();

    if (pDSTexture == NULL)
        return false;

    D3D11_TEXTURE2D_DESC dsTextureDesc;
    pDSTexture->GetDesc(&dsTextureDesc);

    // DT33170 Must the render target and d/s buffer both be cube maps? Check downlevel hardware.

    if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1D)
    {
        efd::UInt32 dsMipSlice = EE_UINT32_MAX;
        if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE1D)
        {
            // OK
            dsMipSlice = dsViewDesc.Texture1D.MipSlice;
        }
        else if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE1DARRAY)
        {
            if (dsViewDesc.Texture1DArray.ArraySize == 1)
            {
                // OK
            }
            else
            {
                return false;
            }
            dsMipSlice = dsViewDesc.Texture1DArray.MipSlice;
        }
        else
        {
            return false;
        }
        EE_ASSERT(rtViewDesc.Texture1D.MipSlice == 0 && dsMipSlice == 0);

        // For 1D resources, the height must be 1.
        EE_ASSERT(pRTBufferData->GetHeight() == 1 &&
            pDSBufferData->GetHeight() == 1);
        if (pRTBufferData->GetWidth() != pDSBufferData->GetWidth())
            return false;
    }
    else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1DARRAY)
    {
        efd::UInt32 dsMipSlice = EE_UINT32_MAX;
        if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE1D)
        {
            if (rtViewDesc.Texture1DArray.ArraySize == 1)
            {
                // OK
            }
            else
            {
                return false;
            }
            dsMipSlice = dsViewDesc.Texture1D.MipSlice;
        }
        else if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE1DARRAY)
        {
            if (dsViewDesc.Texture1DArray.ArraySize ==
                rtViewDesc.Texture1DArray.ArraySize)
            {
                // OK
            }
            else
            {
                return false;
            }
            dsMipSlice = dsViewDesc.Texture1DArray.MipSlice;
        }
        else
        {
            return false;
        }
        EE_ASSERT(rtViewDesc.Texture1D.MipSlice == 0 && dsMipSlice == 0);

        // For 1D resources, the height must be 1.
        EE_ASSERT(pRTBufferData->GetHeight() == 1 &&
            pDSBufferData->GetHeight() == 1);
        if (pRTBufferData->GetWidth() != pDSBufferData->GetWidth())
            return false;
    }
    else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
    {
        efd::UInt32 dsMipSlice = EE_UINT32_MAX;
        if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D)
        {
            // OK
            dsMipSlice = dsViewDesc.Texture2D.MipSlice;
        }
        else if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY)
        {
            if (dsViewDesc.Texture2DArray.ArraySize == 1)
            {
                // OK
            }
            else
            {
                return false;
            }
            dsMipSlice = dsViewDesc.Texture2DArray.MipSlice;
        }
        else
        {
            return false;
        }
        EE_ASSERT(rtViewDesc.Texture2D.MipSlice == 0 && dsMipSlice == 0);

        if (pRTBufferData->GetWidth() != pDSBufferData->GetWidth() ||
            pRTBufferData->GetHeight() != pDSBufferData->GetHeight())
        {
            return false;
        }
    }
    else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY)
    {
        efd::UInt32 dsMipSlice = EE_UINT32_MAX;
        if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D)
        {
            if (rtViewDesc.Texture2DArray.ArraySize == 1)
            {
                // OK
            }
            else
            {
                return false;
            }
            dsMipSlice = dsViewDesc.Texture2D.MipSlice;
        }
        else if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY)
        {
            if (dsViewDesc.Texture2DArray.ArraySize ==
                rtViewDesc.Texture2DArray.ArraySize)
            {
                // OK
            }
            else
            {
                return false;
            }
            dsMipSlice = dsViewDesc.Texture2DArray.MipSlice;
        }
        else
        {
            return false;
        }
        EE_ASSERT(rtViewDesc.Texture2D.MipSlice == 0 && dsMipSlice == 0);

        if (pRTBufferData->GetWidth() != pDSBufferData->GetWidth() ||
            pRTBufferData->GetHeight() != pDSBufferData->GetHeight())
        {
            return false;
        }
    }
    else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
    {
        if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMS)
        {
            // OK
        }
        else if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY)
        {
            if (dsViewDesc.Texture2DMSArray.ArraySize == 1)
            {
                // OK
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        // D3D11 doesn't support mipmapped multisampled textures.
        if (pRTBufferData->GetWidth() != pDSBufferData->GetWidth() ||
            pRTBufferData->GetHeight() != pDSBufferData->GetHeight())
        {
            return false;
        }
    }
    else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY)
    {
        if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMS)
        {
            if (rtViewDesc.Texture2DMSArray.ArraySize == 1)
            {
                // OK
            }
            else
            {
                return false;
            }
        }
        else if (dsViewDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY)
        {
            if (dsViewDesc.Texture2DMSArray.ArraySize ==
                rtViewDesc.Texture2DMSArray.ArraySize)
            {
                // OK
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        // D3D11 doesn't support mipmapped multisampled textures.
        if (pRTBufferData->GetWidth() != pDSBufferData->GetWidth() ||
            pRTBufferData->GetHeight() != pDSBufferData->GetHeight())
        {
            return false;
        }
    }
    else if (rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_BUFFER ||
        rtViewDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE3D)
    {
        // Any other resource types other than those listed here are
        // not valid for the depth/stencil buffer, though
        // D3D11_RTV_DIMENSION_UNKNOWN indicates that no render target
        // exists, so the depth/stencil buffer can be whatever it
        // wants to be.
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
NiRenderTargetGroup* D3D11Renderer::GetDefaultRenderTargetGroup() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    return m_spDefaultRenderTargetGroup;
}

//------------------------------------------------------------------------------------------------
const NiRenderTargetGroup* D3D11Renderer::GetCurrentRenderTargetGroup() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    return m_pCurrentRenderTargetGroup;
}

//------------------------------------------------------------------------------------------------
NiDepthStencilBuffer* D3D11Renderer::GetDefaultDepthStencilBuffer() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (m_spDefaultRenderTargetGroup)
        return m_spDefaultRenderTargetGroup->GetDepthStencilBuffer();
    return NULL;
}

//------------------------------------------------------------------------------------------------
Ni2DBuffer* D3D11Renderer::GetDefaultBackBuffer() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (m_spDefaultRenderTargetGroup)
        return m_spDefaultRenderTargetGroup->GetBuffer(0);
    return NULL;
}

//------------------------------------------------------------------------------------------------
const NiPixelFormat* D3D11Renderer::FindClosestPixelFormat(
    NiTexture::FormatPrefs& formatPrefs) const
{
    NiTexture::FormatPrefs::PixelLayout layout = formatPrefs.m_ePixelLayout;
    NiTexture::FormatPrefs::AlphaFormat alphaFormat = formatPrefs.m_eAlphaFmt;

    if (layout == NiTexture::FormatPrefs::PIX_DEFAULT)
        layout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    if (alphaFormat == NiTexture::FormatPrefs::ALPHA_DEFAULT)
        alphaFormat = NiTexture::FormatPrefs::NONE;

    switch (layout)
    {
    case NiTexture::FormatPrefs::HIGH_COLOR_16: // 16-bit not supported
    case NiTexture::FormatPrefs::TRUE_COLOR_32:
        if (alphaFormat == NiTexture::FormatPrefs::BINARY)
            return &D3D11PixelFormat::EE_FORMAT_R10G10B10A2_UNORM;
        else
            return &D3D11PixelFormat::EE_FORMAT_R8G8B8A8_UNORM;
    case NiTexture::FormatPrefs::PALETTIZED_8: // Treat request for palettes
    case NiTexture::FormatPrefs::PALETTIZED_4: // as request for compression
    case NiTexture::FormatPrefs::COMPRESSED:
        if (alphaFormat == NiTexture::FormatPrefs::BINARY ||
            alphaFormat == NiTexture::FormatPrefs::NONE)
        {
            return &D3D11PixelFormat::EE_FORMAT_BC1_UNORM;
        }
        else
        {
            return &D3D11PixelFormat::EE_FORMAT_BC2_UNORM;
        }
    case NiTexture::FormatPrefs::BUMPMAP:
        return &D3D11PixelFormat::EE_FORMAT_R8G8B8A8_SNORM;
    case NiTexture::FormatPrefs::SINGLE_COLOR_8:
        return &D3D11PixelFormat::EE_FORMAT_R8_UNORM;
    case NiTexture::FormatPrefs::SINGLE_COLOR_16:
        return &D3D11PixelFormat::EE_FORMAT_R16_UNORM;
    case NiTexture::FormatPrefs::SINGLE_COLOR_32:
        return &D3D11PixelFormat::EE_FORMAT_R32_UINT;
    case NiTexture::FormatPrefs::DOUBLE_COLOR_32:
        return &D3D11PixelFormat::EE_FORMAT_R16G16_UNORM;
    case NiTexture::FormatPrefs::DOUBLE_COLOR_64:
        return &D3D11PixelFormat::EE_FORMAT_R32G32_UINT;
    case NiTexture::FormatPrefs::FLOAT_COLOR_32:
        return &D3D11PixelFormat::EE_FORMAT_R32_FLOAT;
    case NiTexture::FormatPrefs::FLOAT_COLOR_64:
        return &D3D11PixelFormat::EE_FORMAT_R32G32_FLOAT;
    case NiTexture::FormatPrefs::FLOAT_COLOR_128:
        return &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
    }

    D3D11Error::ReportWarning(__FUNCTION__ " failed to find a compatible format");

    return NULL;
}

//------------------------------------------------------------------------------------------------
const NiPixelFormat* D3D11Renderer::FindClosestDepthStencilFormat(
    const NiPixelFormat*,
    efd::UInt32 depthBPP,
    efd::UInt32 stencilBPP) const
{
    // Depth stencil format is independent of front buffer format;
    NiPixelFormat* formatArray[] =
    {
        &D3D11PixelFormat::EE_FORMAT_D32_FLOAT_S8X24_UINT,
        &D3D11PixelFormat::EE_FORMAT_D32_FLOAT,
        &D3D11PixelFormat::EE_FORMAT_D24_UNORM_S8_UINT,
        &D3D11PixelFormat::EE_FORMAT_D16_UNORM
    };
    const efd::UInt32 formatCount = sizeof(formatArray) / sizeof(formatArray[0]);

    // Array for order of attempting various formats
    efd::UInt32 formatPreferenceArray[formatCount];
    efd::UInt32 i = 0;
    for (; i < formatCount; i++)
        formatPreferenceArray[i] = i;

    if (stencilBPP > 0)
    {
        if (depthBPP <= 24)
        {
            formatPreferenceArray[0] = 2;
            formatPreferenceArray[1] = 0;
            formatPreferenceArray[2] = 3;
            formatPreferenceArray[3] = 1;
        }
        else
        {
            formatPreferenceArray[0] = 0;
            formatPreferenceArray[1] = 2;
            formatPreferenceArray[2] = 1;
            formatPreferenceArray[3] = 3;
        }
    }
    else
    {
        if (depthBPP <= 16)
        {
            formatPreferenceArray[0] = 3;
            formatPreferenceArray[1] = 2;
            formatPreferenceArray[2] = 1;
            formatPreferenceArray[3] = 0;
        }
        else if (depthBPP <= 24)
        {
            formatPreferenceArray[0] = 2;
            formatPreferenceArray[1] = 1;
            formatPreferenceArray[2] = 0;
            formatPreferenceArray[3] = 3;
        }
        else
        {
            formatPreferenceArray[0] = 1;
            formatPreferenceArray[1] = 0;
            formatPreferenceArray[2] = 2;
            formatPreferenceArray[3] = 3;
        }
    }

    for (i = 0; i < formatCount; i++)
    {
        NiPixelFormat* pFormat = formatArray[formatPreferenceArray[i]];
        if (DoesFormatSupportFlag(
            (DXGI_FORMAT)pFormat->GetRendererHint(),
            D3D11_FORMAT_SUPPORT_DEPTH_STENCIL))
        {
            return pFormat;
        }
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::GetMaxBuffersPerRenderTargetGroup() const
{
    switch (m_featureLevel)
    {
    case D3D_FEATURE_LEVEL_9_1:
    case D3D_FEATURE_LEVEL_9_2:
        return 1;
    case D3D_FEATURE_LEVEL_9_3:
        return 4;
    case D3D_FEATURE_LEVEL_10_0:
    case D3D_FEATURE_LEVEL_10_1:
    case D3D_FEATURE_LEVEL_11_0:
        return D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    default:
        EE_FAIL("Invalid feature level");
        return 1;
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::GetIndependentBufferBitDepths() const
{
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::PrecacheMesh(
    NiRenderObject* pRenderObject,
    efd::Bool releaseSystemMemory)
{
    if (pRenderObject == NULL || !NiIsKindOf(NiMesh, pRenderObject))
        return false;

    LockPrecacheCriticalSection();

    NiMesh* pMesh = (NiMesh*)pRenderObject;

    /// Gets the number of streamrefs attached to this mesh.
    const efd::UInt32 streamRefCount = pMesh->GetStreamRefCount();
    for (efd::UInt32 i = 0; i < streamRefCount; i++)
    {
        NiDataStreamRef* pDataStreamRef = pMesh->GetStreamRefAt(i);
        EE_ASSERT(pDataStreamRef);
        NiDataStream* pDataStream = pDataStreamRef->GetDataStream();
        EE_ASSERT(pDataStream && NiIsKindOf(D3D11DataStream, pDataStream));

        // Can only precache static, GPU-readable streams
        if ((pDataStream->GetAccessMask() & NiDataStream::ACCESS_CPU_WRITE_STATIC) == 0 ||
            (pDataStream->GetAccessMask() & NiDataStream::ACCESS_GPU_READ) == 0)
        {
            continue;
        }

        D3D11DataStream* pD3D11DataStream = (D3D11DataStream*)pDataStream;

        pD3D11DataStream->UpdateD3D11Buffers(releaseSystemMemory);
    }

    UnlockPrecacheCriticalSection();
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::PrecacheShader(NiRenderObject* pRenderObject)
{
    EE_ASSERT(pRenderObject != NULL);
    EE_ASSERT(NiIsKindOf(NiMesh, pRenderObject));

    NiMesh* pMesh = (NiMesh*)pRenderObject;

    D3D11MeshMaterialBindingPtr spDecl;
    D3D11ShaderInterface* pShader = GetShaderAndVertexDecl(pMesh, spDecl);

    // Make sure we have a valid shader and vertex decl for rendering.
    if (pShader == NULL || spDecl == NULL)
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::PrecacheTexture(NiTexture* pTexture)
{
    if (NiIsKindOf(NiSourceTexture, pTexture))
    {
        return CreateSourceTextureRendererData((NiSourceTexture*)pTexture);
    }
    else
    {
        return (pTexture->GetRendererData() != NULL);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::PerformPrecache()
{
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::SetMipmapSkipLevel(efd::UInt32 skipLevel)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    EE_ASSERT(skipLevel <= EE_UINT16_MAX);
    return D3D11SourceTextureData::SetMipmapSkipLevel(
        (efd::UInt16)(skipLevel & 0x0000FFFF));
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Renderer::GetMipmapSkipLevel() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    return D3D11SourceTextureData::GetMipmapSkipLevel();
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::PurgeMaterial(NiMaterialProperty*)
{
    // No renderer data stored on materials
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::PurgeEffect(NiDynamicEffect*)
{
    // No renderer data stored on dynamic effects
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::PurgeTexture(NiTexture* pTexture)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    D3D11TextureData* pData = (D3D11TextureData*)(pTexture->GetRendererData());

    if (pData)
    {
        efd::Bool isRenderedTexture = pData->IsRenderedTexture();

        if (isRenderedTexture)
        {
            if (NiIsKindOf(NiRenderedCubeMap, pTexture))
            {
                NiRenderedCubeMap* pRenderedCubeMap = (NiRenderedCubeMap*)pTexture;
                for (efd::UInt32 i = 0; i < NiRenderedCubeMap::FACE_NUM; i++)
                {
                    Ni2DBuffer* pBuffer = pRenderedCubeMap->GetFaceBuffer(
                        (NiRenderedCubeMap::FaceID)i);
                    pBuffer->SetRendererData(NULL);
                }
            }
            else
            {
                NiRenderedTexture* pRenderedTex = (NiRenderedTexture*)pTexture;
                Ni2DBuffer* pBuffer = pRenderedTex->GetBuffer();
                pBuffer->SetRendererData(NULL);
            }
        }

        pTexture->SetRendererData(NULL);
        EE_DELETE pData;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::PurgeAllTextures(efd::Bool)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    D3D11TextureData::ClearTextureData();
    return true;
}

//------------------------------------------------------------------------------------------------
NiPixelData* D3D11Renderer::TakeScreenShot(
    const NiRect<efd::UInt32>* pScreenRect,
    const NiRenderTargetGroup* pTarget)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (pTarget == NULL)
        pTarget = m_spDefaultRenderTargetGroup;

    ID3D11Resource* pScreenShot = GetBackBufferResource(pTarget, 0);

#if defined EE_ASSERTS_ARE_ENABLED
    D3D11_RESOURCE_DIMENSION resourceDim;
    pScreenShot->GetType(&resourceDim);
    EE_ASSERT(resourceDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D);
#endif // #if defined EE_ASSERTS_ARE_ENABLED

    ID3D11Texture2D* pScreenShot2D = (ID3D11Texture2D*)pScreenShot;

    D3D11_TEXTURE2D_DESC textureDesc;
    pScreenShot2D->GetDesc(&textureDesc);

    // Calculate dimensions to be copied
    D3D11_BOX box;
    efd::Bool copySubregion = false;
    efd::UInt32 ssWidth;
    efd::UInt32 ssHeight;
    if (pScreenRect)
    {
        if (pScreenRect->m_right > pScreenRect->m_left)
        {
            box.left = pScreenRect->m_left;
            box.right = pScreenRect->m_right;
        }
        else
        {
            box.right = pScreenRect->m_left;
            box.left = pScreenRect->m_right;
        }

        if (pScreenRect->m_bottom > pScreenRect->m_top)
        {
            box.top = pScreenRect->m_top;
            box.bottom = pScreenRect->m_bottom;
        }
        else
        {
            box.bottom = pScreenRect->m_top;
            box.top = pScreenRect->m_bottom;
        }

        box.front = 0;
        box.back = 1;

        if ((box.left >= textureDesc.Width) ||
            (box.bottom >= textureDesc.Height))
        {
            return NULL;
        }

        if (box.right > textureDesc.Width)
            box.right = textureDesc.Width;
        if (box.bottom > textureDesc.Height)
            box.bottom = textureDesc.Height;

        ssWidth = box.right - box.left;
        ssHeight = box.bottom - box.top;

        if (ssWidth < textureDesc.Width || ssHeight < textureDesc.Height)
            copySubregion = true;
    }
    else
    {
        ssWidth = textureDesc.Width;
        ssHeight = textureDesc.Height;
    }

    // If multisampled, must resolve to a single-sampled resource
    ID3D11Texture2D* pSingleSampled = NULL;
    if (textureDesc.SampleDesc.Count != 1 || textureDesc.SampleDesc.Quality != 0)
    {
        pSingleSampled = m_pResourceManager->CreateTexture2D(
            textureDesc.Width,
            textureDesc.Height,
            textureDesc.MipLevels,
            textureDesc.ArraySize,
            textureDesc.Format,
            1,
            0,
            D3D11_USAGE_DEFAULT,
            0,
            0,
            0);

        if (pSingleSampled == NULL)
            return NULL;

        m_pCurrentD3D11DeviceContext->ResolveSubresource(
            pSingleSampled,
            0,
            pScreenShot2D,
            0,
            textureDesc.Format);
    }
    else
    {
        pSingleSampled = pScreenShot2D;
        pSingleSampled->AddRef();
    };

    // Bring back to system RAM
    ID3D11Texture2D* pStagingTexture = m_pResourceManager->CreateTexture2D(
        ssWidth,
        ssHeight,
        1,
        textureDesc.ArraySize,
        textureDesc.Format,
        1,
        0,
        D3D11_USAGE_STAGING,
        0,
        D3D11_CPU_ACCESS_READ,
        0);

    if (pStagingTexture == NULL)
    {
        pSingleSampled->Release();
        return NULL;
    }

    if (copySubregion)
    {
        m_pCurrentD3D11DeviceContext->CopySubresourceRegion(
            pStagingTexture,
            0,
            0,
            0,
            0,
            pSingleSampled,
            0,
            &box);
    }
    else
    {
        m_pCurrentD3D11DeviceContext->CopyResource(pStagingTexture, pSingleSampled);
    }

    // Done with single-sampled resource
    pSingleSampled->Release();

    // Fill in NiPixelData
    // Create an NiPixelData object that matches the backbuffer in format and
    // size
    NiPixelData* pPixelData = EE_NEW NiPixelData(
        ssWidth,
        ssHeight,
        *(pTarget->GetPixelFormat(0)),
        1);

    if (pPixelData == NULL)
    {
        pStagingTexture->Release();
        return NULL;
    }

    efd::UInt32 rowSize = pPixelData->GetWidth(0) * pPixelData->GetPixelStride();
    efd::UInt8* pDest = pPixelData->GetPixels(0);

    D3D11_MAPPED_SUBRESOURCE mappedTexture;
    HRESULT hr = m_pCurrentD3D11DeviceContext->Map(
        pStagingTexture,
        0,
        D3D11_MAP_READ,

        0, &mappedTexture);
    if (FAILED(hr))
    {
        EE_DELETE pPixelData;
        pStagingTexture->Release();
        return NULL;
    }

    const efd::UInt8* pSrc = (efd::UInt8*)mappedTexture.pData;

    for (efd::UInt32 i = 0; i < ssHeight; i++)
    {
        NiMemcpy(pDest, pSrc, rowSize);

        pDest += rowSize;
        pSrc += mappedTexture.RowPitch;
    }

    m_pCurrentD3D11DeviceContext->Unmap(pStagingTexture, 0);

    pStagingTexture->Release();

    return pPixelData;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::SaveScreenShot(
    const efd::Char* pFilename,
    EScreenshotFormat format)
{
    D3DX11_IMAGE_FILE_FORMAT formatD3D11;
    switch (format)
    {
    case FORMAT_PNG:
        formatD3D11 = D3DX11_IFF_PNG;
        break;
    case FORMAT_JPEG:
        formatD3D11 = D3DX11_IFF_JPG;
        break;
    default:
        Error(
            "%s> Unsupported image format %d\n",
            __FUNCTION__,
            format);
        return false;
    }

    NiRenderTargetGroup* pTarget = m_pCurrentRenderTargetGroup;
    if (pTarget == NULL)
        pTarget = m_spDefaultRenderTargetGroup;

    ID3D11Resource* pScreenShot = GetBackBufferResource(pTarget, 0);

    // The rest of this code is to work around the fact that D3DX11SaveTextureToFile can only
    // save to the current directory

    // First get the full path to the destination file
    efd::Char absolutePath[efd::EE_MAX_PATH];

    EE_ASSERT(pFilename);
    if (efd::PathUtils::IsAbsolutePath(pFilename))
    {
        efd::Strcpy(absolutePath, efd::EE_MAX_PATH, pFilename);
    }
    else
    {
        efd::PathUtils::ConvertToAbsolute(absolutePath, efd::EE_MAX_PATH, pFilename, NULL);
    }

    // Split off the directory from the file
    NiFilename filename(absolutePath);
    efd::Char destPath[efd::EE_MAX_PATH];
    efd::Char destFile[efd::EE_MAX_PATH];
    efd::Sprintf(
        destPath,
        efd::EE_MAX_PATH,
        "%s%s",
        filename.GetDrive(),
        filename.GetDir());
    efd::Sprintf(
        destFile,
        efd::EE_MAX_PATH,
        "%s%s",
        filename.GetFilename(),
        filename.GetExt());

    // Save off the current working directory
    efd::Char currentPath[efd::EE_MAX_PATH];
    EE_VERIFY(efd::PathUtils::GetWorkingDirectory(currentPath, efd::EE_MAX_PATH));

    // Ensure the new current directory exists.
    efd::MakeDir(destPath);

    // Set the new current directory
    _chdir(destPath);

    ID3DBlob* pBlob = NULL;
    // Save the file to memory
    HRESULT hr = D3DX11SaveTextureToMemory(
        m_pCurrentD3D11DeviceContext,
        pScreenShot,
        formatD3D11,
        &pBlob,
        0);

    if (SUCCEEDED(hr) && pBlob != NULL)
    {

        SIZE_T size = pBlob->GetBufferSize();
        EE_ASSERT(size > 0);
        efd::Char fullPath[efd::EE_MAX_PATH];

        // Open file and save the memory
        if (filename.GetFullPath(fullPath, efd::EE_MAX_PATH))
        {
            EE_ASSERT(size > 0);
            if (size > 0)
            {
                void* pMem = pBlob->GetBufferPointer();
                EE_ASSERT(pMem != NULL);
                efd::File* pFile = NULL;
                if (pMem)
                    pFile = efd::File::GetFile(fullPath, efd::File::WRITE_ONLY);
                EE_ASSERT(pFile);
                if (pFile)
                {
                    pFile->Write(pMem, (efd::UInt32) size);
                    EE_DELETE pFile;
                }
            }
        }
    }
    if (pBlob)
        pBlob->Release();
    // Restore the original working directory
    _chdir(currentPath);

    return SUCCEEDED(hr);
}

//------------------------------------------------------------------------------------------------
ID3D11Texture2D* D3D11Renderer::GetBackBufferResource(
    const NiRenderTargetGroup* pTarget,
    efd::UInt32 renderTarget)
{
    EE_ASSERT(pTarget && pTarget->IsValid());

    D3D11RenderTargetBufferData* pBuffData = NiVerifyStaticCast(D3D11RenderTargetBufferData,
        (D3D112DBufferData*)pTarget->GetBufferRendererData(renderTarget));

    EE_ASSERT(pBuffData);

    ID3D11Texture2D* pTexture2D = pBuffData->GetRenderTargetBuffer();

    return pTexture2D;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::FastCopy(
    const Ni2DBuffer* pSrc,
    Ni2DBuffer* pDest,
    const NiRect<efd::UInt32>* pSrcRect,
    efd::UInt32 destX,
    efd::UInt32 destY)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    EE_ASSERT(pSrc != NULL);
    EE_ASSERT(pDest != NULL);

    D3D112DBufferData* pSrcRendData = (D3D112DBufferData*)pSrc->GetRendererData();
    D3D112DBufferData* pDestRendData = (D3D112DBufferData*)pDest->GetRendererData();

    if (pSrcRendData == NULL || pDestRendData == NULL)
    {
        D3D11Error::ReportWarning(__FUNCTION__ " failed - no RendererData found.");
        return false;
    }

    if (*(pSrcRendData->GetPixelFormat()) != *(pDestRendData->GetPixelFormat()))
    {
        D3D11Error::ReportWarning(__FUNCTION__ " failed - pixel formats do not match.");
        return false;
    }

    ID3D11Texture2D* pSourceSurface = pSrcRendData->GetTexture2D();
    ID3D11Texture2D* pDestSurface = pDestRendData->GetTexture2D();

    if (pSourceSurface == NULL || pDestSurface == NULL)
    {
        D3D11Error::ReportWarning(__FUNCTION__ " failed - NULL Surface found.");
        return false;
    }

    // Calculate dimensions to be copied
    if (pSrcRect)
    {
        D3D11_BOX box;
        box.left = pSrcRect->m_left;
        box.right = pSrcRect->m_right;
        box.top = pSrcRect->m_top;
        box.bottom = pSrcRect->m_bottom;
        box.front = 0;
        box.back = 1;

        m_pCurrentD3D11DeviceContext->CopySubresourceRegion(
            pDestSurface,
            0,
            destX,
            destY,
            0,
            pSourceSurface,
            0,
            &box);
    }
    else
    {
        m_pCurrentD3D11DeviceContext->CopyResource(pDestSurface, pSourceSurface);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::Copy(
    const Ni2DBuffer* pSrc,
    Ni2DBuffer* pDest,
    const NiRect<efd::UInt32>* pSrcRect,
    const NiRect<efd::UInt32>* pDestRect,
    Ni2DBuffer::CopyFilterPreference filterPref)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    EE_ASSERT(pSrc != NULL);
    EE_ASSERT(pDest != NULL);

    D3D112DBufferData* pSrcRendData = (D3D112DBufferData*)pSrc->GetRendererData();
    D3D112DBufferData* pDestRendData = (D3D112DBufferData*)pDest->GetRendererData();

    if (pSrcRendData == NULL || pDestRendData == NULL)
    {
        D3D11Error::ReportWarning(__FUNCTION__ " failed - no RendererData found.");
        return false;
    }

    if (*(pSrcRendData->GetPixelFormat()) != *(pDestRendData->GetPixelFormat()))
    {
        D3D11Error::ReportWarning(__FUNCTION__ " failed - pixel formats do not match.");
        return false;
    }

    ID3D11Texture2D* pSourceSurface = pSrcRendData->GetTexture2D();
    ID3D11Texture2D* pDestSurface = pDestRendData->GetTexture2D();

    if (pSourceSurface == NULL || pDestSurface == NULL)
    {
        D3D11Error::ReportWarning(__FUNCTION__ " failed - NULL Surface found.");
        return false;
    }

    D3DX11_FILTER_FLAG filterType;
    switch (filterPref)
    {
    default:
    case Ni2DBuffer::COPY_FILTER_NONE:
        filterType = D3DX11_FILTER_NONE;
        break;
    case Ni2DBuffer::COPY_FILTER_POINT:
        filterType = D3DX11_FILTER_POINT;
        break;
    case Ni2DBuffer::COPY_FILTER_LINEAR:
        filterType = D3DX11_FILTER_LINEAR;
        break;
    }

    D3DX11_TEXTURE_LOAD_INFO loadInfo;
    loadInfo.pSrcBox = NULL;
    loadInfo.pDstBox = NULL;
    loadInfo.SrcFirstMip = 0;
    loadInfo.DstFirstMip = 0;
    loadInfo.NumMips = 1; // Only doing one mip level
    loadInfo.SrcFirstElement = 0;
    loadInfo.DstFirstElement = 0;
    loadInfo.NumElements = 0;
    loadInfo.Filter = filterType;
    loadInfo.MipFilter = filterType;

    D3D11_BOX srcBox;
    if (pSrcRect)
    {
        srcBox.left = pSrcRect->m_left;
        srcBox.right = pSrcRect->m_right;
        srcBox.top = pSrcRect->m_top;
        srcBox.bottom = pSrcRect->m_bottom;
        srcBox.front = 0;
        srcBox.back = 1;

        loadInfo.pSrcBox = &srcBox;
    }

    D3D11_BOX destBox;
    if (pDestRect)
    {
        destBox.left = pDestRect->m_left;
        destBox.right = pDestRect->m_right;
        destBox.top = pDestRect->m_top;
        destBox.bottom = pDestRect->m_bottom;
        destBox.front = 0;
        destBox.back = 1;

        loadInfo.pDstBox = &destBox;
    }

    HRESULT hr = D3DX11LoadTextureFromTexture(
        m_pCurrentD3D11DeviceContext,
        pSourceSurface,
        &loadInfo,
        pDestSurface);

    if (FAILED(hr))
    {
        D3D11Error::ReportWarning(
            "D3DX11LoadTextureFromTexture failed in " __FUNCTION__ " - "
            "Error HRESULT = 0x%08X.",
            (efd::UInt32)hr);
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
NiDepthStencilBuffer* D3D11Renderer::CreateCompatibleDepthStencilBuffer(
    Ni2DBuffer* pBuffer) const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (!pBuffer)
        return NULL;

    const NiPixelFormat* pFormat =
        NiRenderer::FindClosestDepthStencilFormat(pBuffer->GetPixelFormat());

    if (pFormat)
    {
        // DT33170
        NiDepthStencilBuffer* pDSBuffer = NiDepthStencilBuffer::Create(
            pBuffer->GetWidth(),
            pBuffer->GetHeight(),
            NiRenderer::GetRenderer(), //"this" not used b/c of const
            *pFormat,
            pBuffer->GetMSAAPref());
        return pDSBuffer;
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::GetLeftRightSwap() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    return m_leftRightSwap;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::SetLeftRightSwap(efd::Bool swap)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    m_leftRightSwap = swap;
    EE_ASSERT(m_pRenderStateManager);
    m_pRenderStateManager->SetLeftRightSwap(swap);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Float32 D3D11Renderer::GetMaxFogValue() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    return m_maxFogValue;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::SetMaxFogValue(efd::Float32 fogVal)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (fogVal <= 0.0f)
        fogVal = 1e-5f;

    m_maxFogValue = fogVal;
    m_maxFogFactor = 1.0f / fogVal - 1.0f;
}

//------------------------------------------------------------------------------------------------
efd::Float32 D3D11Renderer::GetMaxFogFactor() const
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    return m_maxFogFactor;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::SetMaxAnisotropy(efd::UInt16 maxAnisotropy)
{
    m_usMaxAnisotropy = static_cast<efd::UInt16>(
        efd::Clamp(maxAnisotropy, 1, HW_MAX_ANISOTROPY));
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::UseLegacyPipelineAsDefaultMaterial()
{
    // Legacy pipeline not supported in D3D11
}

//------------------------------------------------------------------------------------------------
D3D11Renderer* D3D11Renderer::GetRenderer()
{
    if (ms_pkRenderer &&
        ms_pkRenderer->GetRendererID() == efd::SystemDesc::RENDERER_D3D11)
    {
        return (D3D11Renderer*)ms_pkRenderer;
    }
    else
    {
        return NULL;
    }
}

//------------------------------------------------------------------------------------------------
NiTexturePtr D3D11Renderer::CreateNiTextureFromD3D11Resource(
    ID3D11Resource* pD3D11Resource,
    ID3D11ShaderResourceView* pResourceView)
{
    if (pD3D11Resource == NULL)
        return NULL;

    D3D11Direct3DResourcePtr spTexture = D3D11Direct3DResource::Create(this);
    EE_ASSERT_MESSAGE(spTexture, ("Failed to create D3D11Direct3DResource!"));

    D3D11Direct3DResourceData::Create(spTexture, pD3D11Resource, pResourceView);

    return (NiTexture*)(spTexture.data());
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateSourceTextureRendererData(NiSourceTexture* pTexture)
{
    // Not a function that requires it be on the device thread

    if (pTexture == NULL)
        return false;

    if (m_deviceThreadID == GetCurrentThreadId())
    {
        // Device thread - load texture immediately
        D3D11TextureData* pTexData = (D3D11TextureData*)pTexture->GetRendererData();
        if (pTexData == NULL)
        {
            pTexData = (D3D11TextureData*)D3D11SourceTextureData::Create(pTexture);
        }

        // Check to see if the texture data can be released
        if (NiSourceTexture::GetDestroyAppDataFlag() &&
            pTexture->GetRendererData() &&
            pTexture->GetStatic())
        {
            pTexture->DestroyAppPixelData();
        }
    }
    else
    {
        // Non-device thread - delay texture loading

        // Load texture file so queries will work - this won't create a D3D
        // resource.
        if (!pTexture->GetLoadDirectToRendererHint())
            pTexture->LoadPixelDataFromFile();

        LockPrecacheCriticalSection();

        D3D11TextureData* pTexData = (D3D11TextureData*)pTexture->GetRendererData();
        if (pTexData == NULL)
        {
            NiSourceTexture* pSourceTex = NiDynamicCast(NiSourceTexture, pTexture);
            if (pSourceTex)
            {
                pTexData = D3D11SourceTextureData::Create(pSourceTex);

                // Check to see if the texture data can be released
                if (NiSourceTexture::GetDestroyAppDataFlag() &&
                    pSourceTex->GetRendererData() &&
                    pSourceTex->GetStatic())
                {
                    pSourceTex->DestroyAppPixelData();
                }
            }
            else
            {
                // All other texture types must have renderer data created
                // during texture construction. If we don't already have a
                // texture data object by now, we never will.
            }
        }

        UnlockPrecacheCriticalSection();
    }
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateRenderedTextureRendererData(
    NiRenderedTexture* pTexture,
    Ni2DBuffer::MultiSamplePreference multiSamplePref)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    D3D11TextureData* pData =
        (D3D11TextureData*)pTexture->GetRendererData();
    if (pData)
        return true;

    pData = D3D11RenderedTextureData::Create(pTexture, multiSamplePref);
    if (!pData)
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateSourceCubeMapRendererData(
    NiSourceCubeMap* pCubeMap)
{
    // Cube maps go through standard 2D texture path.
    return CreateSourceTextureRendererData(pCubeMap);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateRenderedCubeMapRendererData(
    NiRenderedCubeMap* pCubeMap)
{
    // Cube maps go through standard 2D texture path.
    return CreateRenderedTextureRendererData(pCubeMap);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateDynamicTextureRendererData(
    NiDynamicTexture* pTexture)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    D3D11DynamicTextureData* pDynamicTextureData = NULL;

    D3D11TextureData* pTextureData = (D3D11TextureData*)(pTexture->GetRendererData());
    if (pTextureData)
    {
        if (pTextureData->IsDynamicTexture())
            return true;
    }

    pDynamicTextureData = D3D11DynamicTextureData::Create(pTexture);
    if (!pDynamicTextureData)
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::CreatePaletteRendererData(NiPalette*)
{
    // Palettes not supported in D3D11.
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateDepthStencilRendererData(
    NiDepthStencilBuffer* pDSBuffer,
    const NiPixelFormat* pFormat,
    Ni2DBuffer::MultiSamplePreference multiSamplePref)
{
    return CreateDepthStencilRendererData(pDSBuffer, pFormat, multiSamplePref, false);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateDepthStencilRendererData(
    NiDepthStencilBuffer* pDSBuffer,
    const NiPixelFormat* pFormat,
    Ni2DBuffer::MultiSamplePreference multiSamplePref,
    efd::Bool isCubeMap)
{
    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning("Can't call "
            "CreateDepthStencilRendererData without a valid device.");
        return false;
    }

    if (pDSBuffer == NULL)
        return false;

    DXGI_FORMAT format;
    if (pFormat)
        format = D3D11PixelFormat::DetermineDXGIFormat(*pFormat);
    else
        format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    efd::UInt32 msaaCount;
    efd::UInt32 msaaQuality;

    Ni2DBuffer::GetMSAACountAndQualityFromPref(multiSamplePref, msaaCount, msaaQuality);

    D3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc;
    dsViewDesc.Format = format;
    dsViewDesc.Flags = 0;

    ID3D11Texture2D* pDSTexture = NULL;
    if (isCubeMap)
    {
        pDSTexture = m_pResourceManager->CreateTexture2D(
            pDSBuffer->GetWidth(),
            pDSBuffer->GetHeight(),
            1,
            6,
            format,
            msaaCount,
            msaaQuality,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_DEPTH_STENCIL,
            0,
            D3D11_RESOURCE_MISC_TEXTURECUBE);

        if (msaaCount == 1)
        {
            dsViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsViewDesc.Texture2DArray.MipSlice = 0;
            dsViewDesc.Texture2DArray.FirstArraySlice = 0;
            dsViewDesc.Texture2DArray.ArraySize = 1;
        }
        else
        {
            dsViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
            dsViewDesc.Texture2DMSArray.FirstArraySlice = 0;
            dsViewDesc.Texture2DMSArray.ArraySize = 1;
        }
    }
    else
    {
        pDSTexture = m_pResourceManager->CreateTexture2D(
            pDSBuffer->GetWidth(),
            pDSBuffer->GetHeight(),
            1,
            1,
            format,
            msaaCount,
            msaaQuality,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_DEPTH_STENCIL,
            0,
            0);

        if (msaaCount == 1)
        {
            dsViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsViewDesc.Texture2D.MipSlice = 0;
        }
        else
        {
            dsViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
    }

    if (pDSTexture == NULL)
    {
        // Resource creation would have thrown error message
        D3D11Error::ReportWarning("CreateDepthStencilRendererData "
            "failed because depth/stencil texture could not be created.");
        return false;
    }

    D3D11DepthStencilBufferData* pRendererData = D3D11DepthStencilBufferData::Create(
        pDSTexture,
        pDSBuffer,
        &dsViewDesc);

    // Reference to DS texture kept in D3D11DepthStencilBufferData
    pDSTexture->Release();

    if (pRendererData == NULL)
    {
        D3D11Error::ReportWarning(
            "CreateDepthStencilRendererData failed because D3D11DepthStencilBufferData could not "
            "be created; destroying depth/stencil texture.");
        return false;
    }
    else
    {
        return true;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::RemoveRenderedCubeMapData(NiRenderedCubeMap*)
{
    /* */
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::RemoveRenderedTextureData(NiRenderedTexture*)
{
    /* */
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::RemoveDynamicTextureData(NiDynamicTexture*)
{
    /* */
}

//------------------------------------------------------------------------------------------------
void* D3D11Renderer::LockDynamicTexture(
    const NiTexture::RendererData* pRData,
    efd::SInt32& pitch)
{
    void* pMem = NULL;
    pitch = 0; // Initialize in case there's an error return.

    D3D11TextureData* pTextureData = (D3D11TextureData*)(pRData);
    if (!pTextureData)
        return NULL;

    // Call GetAsDynamicTexture() so app won't crash in case this function
    // gets called using a non-dynamic texture, accidentally.
    D3D11DynamicTextureData* pDynamicTextureData =
        (pTextureData->IsDynamicTexture() ? (D3D11DynamicTextureData*)pTextureData : NULL);
    if (!pDynamicTextureData)
        return NULL;

    // Sanity check - error return if dynamic texture is already locked.
    if (pDynamicTextureData->IsLocked())
        return NULL;

    if (!pDynamicTextureData->GetResource())
    {
        NiTexture *pDynamicTexture = ((NiTexture::RendererData*)pDynamicTextureData)->GetTexture();
        if (!CreateDynamicTextureRendererData((NiDynamicTexture*)pDynamicTexture))
        {
            EE_ASSERT(false);
        }
    }
    if (pDynamicTextureData)
    {
        pMem = pDynamicTextureData->Lock(pitch);
    }

    pDynamicTextureData->SetLocked(true);

    return pMem;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::UnLockDynamicTexture(
    const NiTexture::RendererData* pRData)
{
    D3D11TextureData* pTextureData = (D3D11TextureData*)(pRData);
    if (!pTextureData)
        return false;

    D3D11DynamicTextureData* pTexture =
        (pTextureData->IsDynamicTexture() ? (D3D11DynamicTextureData*)pTextureData : NULL);

    if (pTexture && pTexture->IsLocked())
    {
        efd::Bool success = pTexture->UnLock();
        if (success)
            pTexture->SetLocked(false);
        return success;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
NiShader* D3D11Renderer::GetFragmentShader(NiMaterialDescriptor* pMaterialDescriptor)
{
    return EE_NEW D3D11FragmentShader(pMaterialDescriptor);
}

//------------------------------------------------------------------------------------------------
NiShader* D3D11Renderer::GetShadowWriteShader(NiMaterialDescriptor* pMaterialDescriptor)
{
    return EE_NEW D3D11ShadowWriteShader(pMaterialDescriptor);
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::SetRenderShadowCasterBackfaces(efd::Bool renderBackfaces)
{
    D3D11ShadowWriteShader::SetRenderBackfaces(renderBackfaces);
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::SetDefaultProgramCache(
    NiFragmentMaterial* pMaterial,
    efd::Bool autoWriteToDisk,
    efd::Bool writeDebugFile,
    efd::Bool load,
    efd::Bool noNewProgramCreation,
    const efd::Char* pWorkingDir)
{
    EE_ASSERT(m_pD3D11Device);

    const efd::Char* pProfile = GetVertexShaderProfile();
    NiGPUProgram::ProgramType shaderType = NiGPUProgram::PROGRAM_VERTEX;

    if (pWorkingDir == NULL)
        pWorkingDir = NiMaterial::GetDefaultWorkingDirectory();

    // No need to create caches for anything but PS and VS until a material requires them.

    D3D11GPUProgramCache* pCache = EE_NEW D3D11GPUProgramCache(
        pMaterial->GetProgramVersion(shaderType),
        pWorkingDir,
        shaderType,
        pProfile,
        pMaterial->GetName(),
        autoWriteToDisk,
        writeDebugFile,
        noNewProgramCreation,
        load);
    pMaterial->SetProgramCache(pCache, shaderType);

    pProfile = GetPixelShaderProfile();
    shaderType = NiGPUProgram::PROGRAM_PIXEL;
    pCache = EE_NEW D3D11GPUProgramCache(
        pMaterial->GetProgramVersion(shaderType),
        pWorkingDir,
        shaderType,
        pProfile,
        pMaterial->GetName(),
        autoWriteToDisk,
        writeDebugFile,
        noNewProgramCreation,
        load);
    pMaterial->SetProgramCache(pCache, shaderType);

    if (NiFragmentMaterial::GetDefaultCreateReplacementShaders())
        pMaterial->AddReplacementShaders();
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::Do_BeginFrame()
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (m_isDeviceRemoved)
    {
        // Renderer will not attempt to recover from this situation.
        return false;
    }
    else if (m_isDeviceOccluded)
    {
        // Check device state only when the device is already occluded.
        if (m_spDefaultRenderTargetGroup == NULL)
            return false;
        EE_ASSERT(m_spDefaultRenderTargetGroup->GetBuffer(0));
        Ni2DBuffer::RendererData* pBackBufferData =
            m_spDefaultRenderTargetGroup->GetBufferRendererData(0);
        HandleDisplayFrameResult(((D3D112DBufferData*)pBackBufferData)->DisplayFrame(0, true));
    }

    while (m_buffersToUseAtDisplayFrame.GetSize())
        m_buffersToUseAtDisplayFrame.RemoveHead();

    PerformPrecache();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::Do_EndFrame()
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

//CODEBLOCK(4) - DO NOT DELETE THIS LINE

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::Do_DisplayFrame()
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    while (m_buffersToUseAtDisplayFrame.GetSize())
    {
        D3D112DBufferDataPtr spBuffer = m_buffersToUseAtDisplayFrame.RemoveHead();

        HRESULT hr = spBuffer->DisplayFrame(m_syncInterval);
        if (HandleDisplayFrameResult(hr) == false)
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::HandleDisplayFrameResult(HRESULT hr)
{
    if (hr == DXGI_STATUS_OCCLUDED)
    {
        if (m_isDeviceOccluded == false)
        {
            // Signal application that it is now occluded
            const efd::UInt32 funcCount = m_occludedNotifyFuncs.GetSize();
            for (efd::UInt32 i = 0; i < funcCount; i++)
            {
                OCCLUDEDNOTIFYFUNC pNotifyFunc = m_occludedNotifyFuncs.GetAt(i);
                void* pData = m_occludedNotifyFuncData.GetAt(i);
                if (pNotifyFunc)
                {
                    efd::Bool success = (*pNotifyFunc)(true, pData);

                    if (success == false)
                    {
                        D3D11Error::ReportWarning(
                            "OCCLUDEDNOFIFYFUNC %d warning of an occluded window failed in "
                            __FUNCTION__
                            ".",
                            i);
                        return false;
                    }
                }
            }

            m_isDeviceOccluded = true;
        }
    }
    else if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            hr = m_pD3D11Device->GetDeviceRemovedReason();
            if (hr == DXGI_ERROR_DEVICE_REMOVED)
            {
                D3D11Error::ReportWarning(
                    "ID3D11Device::GetDeviceRemovedReason reports the "
                    "device has been removed. The D3D11 device must be "
                    "shut down and recreated, though the original "
                    "adapter may be missing.");
            }
            else if (hr == DXGI_ERROR_DEVICE_HUNG)
            {
                D3D11Error::ReportWarning(
                    "ID3D11Device::GetDeviceRemovedReason reports the "
                    "device has hung. The D3D11 device must be shut down "
                    "and recreated. The application can continue as "
                    "usual, but it is recommended that the graphics usage "
                    "be scaled back.");
            }
            else if (hr == DXGI_ERROR_DEVICE_RESET)
            {
                D3D11Error::ReportWarning(
                    "ID3D11Device::GetDeviceRemovedReason reports the "
                    "device has crashed and been reset by another "
                    "application. The D3D11 device must be shut down and "
                    "recreated.");
            }
            // Renderer will not attempt to recover from this situation.
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_DISPLAY_SWAP_CHAIN_FAILED,
                "Error HRESULT = 0x%08X.", (efd::UInt32)hr);
        }
        if (m_isDeviceRemoved == false)
        {
            // Signal application that the device is removed
            const efd::UInt32 funcCount = m_deviceRemovedNotifyFuncs.GetSize();
            for (efd::UInt32 i = 0; i < funcCount; i++)
            {
                DEVICEREMOVEDNOTIFYFUNC pNotifyFunc =
                    m_deviceRemovedNotifyFuncs.GetAt(i);
                void* pData = m_deviceRemovedNotifyFuncData.GetAt(i);
                if (pNotifyFunc)
                {
                    efd::Bool success = (*pNotifyFunc)(pData);

                    if (success == false)
                    {
                        D3D11Error::ReportWarning(
                            "DEVICEREMOVEDNOTIFYFUNC %d warning of a removed device failed in "
                            __FUNCTION__
                            ".",
                            i);
                        return false;
                    }
                }
            }
            m_isDeviceRemoved = true;
        }
        return false;
    }
    else if (m_isDeviceOccluded)
    {
        // Signal application that it is no longer occluded
        const efd::UInt32 funcCount =
            m_occludedNotifyFuncs.GetSize();
        for (efd::UInt32 i = 0; i < funcCount; i++)
        {
            OCCLUDEDNOTIFYFUNC pNotifyFunc = m_occludedNotifyFuncs.GetAt(i);
            void* pData = m_occludedNotifyFuncData.GetAt(i);
            if (pNotifyFunc)
            {
                efd::Bool success = (*pNotifyFunc)(false, pData);

                if (success == false)
                {
                    D3D11Error::ReportWarning(
                        "OCCLUDEDNOFIFYFUNC %d announcing an unoccluded window failed in "
                        __FUNCTION__
                        ".",
                        i);
                    return false;
                }
            }
        }
        m_isDeviceOccluded = false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::Do_ClearBuffer(
    const NiRect<efd::Float32>*,
    efd::UInt32 clearMode)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning("Can't call ClearBuffer without a valid device.");
        return;
    }

    // This should be verified in ClearBuffer
    EE_ASSERT(m_pCurrentRenderTargetGroup);

    if ((clearMode & NiRenderer::CLEAR_BACKBUFFER) != 0)
    {
        for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        {
            Ni2DBuffer* p2DBuffer = m_pCurrentRenderTargetGroup->GetBuffer(i);
            if (p2DBuffer == NULL)
                continue;

            D3D112DBufferData* p2DBufferData =
                (D3D112DBufferData*)p2DBuffer->GetRendererData();
            EE_ASSERT(NiIsKindOf(D3D11RenderTargetBufferData,
                p2DBufferData));

            D3D11RenderTargetBufferData* pRTBufferData =
                (D3D11RenderTargetBufferData*)p2DBufferData;

            ID3D11RenderTargetView* pRTView =
                pRTBufferData->GetRenderTargetView();
            if (pRTView)
            {
                m_pCurrentD3D11DeviceContext->ClearRenderTargetView(pRTView, m_backgroundColor);
            }
        }
    }


    efd::UInt32 dsClearModes = 0;
    if ((clearMode & NiRenderer::CLEAR_ZBUFFER) != 0)
        dsClearModes |= D3D11_CLEAR_DEPTH;
    if ((clearMode & NiRenderer::CLEAR_STENCIL) != 0)
        dsClearModes |= D3D11_CLEAR_STENCIL;

    if (dsClearModes != 0)
    {
        NiDepthStencilBuffer* pDSBuffer =
            m_pCurrentRenderTargetGroup->GetDepthStencilBuffer();
        if (pDSBuffer)
        {
            D3D112DBufferData* p2DBufferData =
                (D3D112DBufferData*)pDSBuffer->GetRendererData();
            EE_ASSERT(NiIsKindOf(D3D11DepthStencilBufferData,
                p2DBufferData));

            D3D11DepthStencilBufferData* pDSBufferData =
                (D3D11DepthStencilBufferData*)p2DBufferData;

            ID3D11DepthStencilView* pDSView =
                pDSBufferData->GetDepthStencilView();
            if (pDSView)
            {
                m_pCurrentD3D11DeviceContext->ClearDepthStencilView(
                    pDSView,
                    dsClearModes,
                    m_depthClear,
                    m_stencilClear);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::Do_SetCameraData(
    const NiPoint3& worldLoc,
    const NiPoint3& worldDir,
    const NiPoint3& worldUp,
    const NiPoint3& worldRight,
    const NiFrustum& frustum,
    const NiRect<efd::Float32>& port)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    // This should be verified in SetCameraData
    EE_ASSERT(m_pCurrentRenderTargetGroup);

    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning("Can't call SetCameraData without a valid device.");
        return;
    }

    // View matrix update
    m_d3dView.r[0] = XMVectorSet(worldRight.x, worldUp.x, worldDir.x, 0.0f);
    m_d3dView.r[1] = XMVectorSet(worldRight.y, worldUp.y, worldDir.y, 0.0f);
    m_d3dView.r[2] = XMVectorSet(worldRight.z, worldUp.z, worldDir.z, 0.0f);
    m_d3dView.r[3] = XMVectorSet(-(worldRight * worldLoc), -(worldUp * worldLoc), -(worldDir * worldLoc), 1.0f);

    m_invView.r[0] = XMVectorSet(worldRight.x, worldRight.y, worldRight.z, 0.0f);
    m_invView.r[1] = XMVectorSet(worldUp.x, worldUp.y, worldUp.z, 0.0f);
    m_invView.r[2] = XMVectorSet(worldDir.x, worldDir.y, worldDir.z, 0.0f);
    m_invView.r[3] = XMVectorSet(worldLoc.x, worldLoc.y, worldLoc.z, 1.0f);

    efd::Float32 fDepthRange = frustum.m_fFar - frustum.m_fNear;

    // Projection matrix update
    efd::Float32 fRmL = frustum.m_fRight - frustum.m_fLeft;
    efd::Float32 fRpL = frustum.m_fRight + frustum.m_fLeft;
    efd::Float32 fTmB = frustum.m_fTop - frustum.m_fBottom;
    efd::Float32 fTpB = frustum.m_fTop + frustum.m_fBottom;
    efd::Float32 fInvFmN = 1.0f / fDepthRange;

    if (frustum.m_bOrtho)
    {
        if (m_leftRightSwap)
        {
            m_d3dProj.r[0] = XMVectorSet(-2.0f / fRmL, 0.0f, 0.0f, 0.0f);
            m_d3dProj.r[3] = XMVectorSet(fRpL / fRmL, -fTpB / fTmB, -(frustum.m_fNear * fInvFmN), 1.0f);
        }
        else
        {
            m_d3dProj.r[0] = XMVectorSet(2.0f / fRmL, 0.0f, 0.0f, 0.0f);
            m_d3dProj.r[3] = XMVectorSet(-fRpL / fRmL, -fTpB / fTmB, -(frustum.m_fNear * fInvFmN), 1.0f);
        }
        m_d3dProj.r[1] = XMVectorSet(0.0f, 2.0f / fTmB, 0.0f, 0.0f);
        m_d3dProj.r[2] = XMVectorSet(0.0f, 0.0f, fInvFmN, 0.0f);
    }
    else
    {
        if (m_leftRightSwap)
        {
            m_d3dProj.r[0] = XMVectorSet(-2.0f / fRmL, 0.0f, 0.0f, 0.0f);
            m_d3dProj.r[2] = XMVectorSet(fRpL / fRmL, -fTpB / fTmB, frustum.m_fFar * fInvFmN, 1.0f);
        }
        else
        {
            m_d3dProj.r[0] = XMVectorSet(2.0f / fRmL, 0.0f, 0.0f, 0.0f);
            m_d3dProj.r[2] = XMVectorSet(-fRpL / fRmL, -fTpB / fTmB, frustum.m_fFar * fInvFmN, 1.0f);
        }
        m_d3dProj.r[1] = XMVectorSet(0.0f, 2.0f / fTmB, 0.0f, 0.0f);
        m_d3dProj.r[3] = XMVectorSet(0.0f, 0.0f, -(frustum.m_fNear * frustum.m_fFar * fInvFmN), 0.0f);
    }

    // Viewport update
    D3D11_VIEWPORT viewPort;
    efd::Float32 fWidth = (efd::Float32)m_pCurrentRenderTargetGroup->GetWidth(0);
    efd::Float32 fHeight = (efd::Float32)m_pCurrentRenderTargetGroup->GetHeight(0);

    viewPort.TopLeftX = port.m_left * fWidth;
    viewPort.TopLeftY = (1.0f - port.m_top) * fHeight;
    viewPort.Width = (port.m_right - port.m_left) * fWidth;
    viewPort.Height = (port.m_top - port.m_bottom) * fHeight;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;

    m_pCurrentD3D11DeviceContext->RSSetViewports(1, &viewPort);

    m_currentFrustum = frustum;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::Do_SetScreenSpaceCameraData(const NiRect<efd::Float32>* pPort)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    // This should be verified in SetScreenSpaceCameraData
    EE_ASSERT(m_pCurrentRenderTargetGroup);

    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning("Can't call SetScreenSpaceCameraData "
            "without a valid device.");
        return;
    }

    // Screen elements are in normalized display coordinates, (x,y), where
    // 0 <= x <= 1
    // 0 <= y <= 1
    // 0 <= z <= 9999
    // The screen buffer width is w and the screen height is h.
    // The camera parameters are:
    // Camera location: (1/2, 1/2, -1)
    // Camera right:    (1,  0, 0)
    // Camera up:       (0, -1, 0)
    // Camera dir:      (0,  0, 1)
    // Camera frustum:
    //      Left:   -1/2
    //      Right:  1/2
    //      Top:    1/2
    //      Bottom: -1/2
    //      Near:   1
    //      Far:    10000

    // View matrix update
    m_d3dView.r[0] = XMVectorSet(1.0f,  0.0f, 0.0f, 0.0f);
    m_d3dView.r[1] = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
    m_d3dView.r[2] = XMVectorSet(0.0f,  0.0f, 1.0f, 0.0f);
    m_d3dView.r[3] = XMVectorSet(-0.5f, 0.5f, 1.0f, 1.0f);

    m_invView.r[0] = XMVectorSet(1.0f,  0.0f,  0.0f, 0.0f);
    m_invView.r[1] = XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f);
    m_invView.r[2] = XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f);
    m_invView.r[3] = XMVectorSet(0.5f,  0.5f, -1.0f, 1.0f);

    const efd::Float32 fNearDepthDivDepthRange = 1.0f / 9999.0f;

    if (m_leftRightSwap)
        m_d3dProj.r[0] = XMVectorSet(-2.0f, 0.0f, 0.0f, 0.0f);
    else
        m_d3dProj.r[0] = XMVectorSet(2.0f, 0.0f, 0.0f, 0.0f);
    m_d3dProj.r[1] = XMVectorSet(0.0f, 2.0f, 0.0f, 0.0f);
    m_d3dProj.r[2] = XMVectorSet(0.0f, 0.0f, fNearDepthDivDepthRange, 0.0f);
    m_d3dProj.r[3] = XMVectorSet(0.0f, 0.0f, -fNearDepthDivDepthRange, 1.0f);

    // Viewport update
    D3D11_VIEWPORT viewPort;

    efd::UInt32 width = m_pCurrentRenderTargetGroup->GetWidth(0);
    efd::UInt32 height = m_pCurrentRenderTargetGroup->GetHeight(0);

    if (pPort)
    {
        efd::Float32 fWidth = (efd::Float32)width;
        efd::Float32 fHeight = (efd::Float32)height;
        viewPort.TopLeftX = pPort->m_left * fWidth;
        viewPort.TopLeftY = (1.0f - pPort->m_top) * fHeight;
        viewPort.Width = (pPort->m_right - pPort->m_left) * fWidth;
        viewPort.Height = (pPort->m_top - pPort->m_bottom) * fHeight;

        // Set cached port.
        m_kurrentViewPort = *pPort;
    }
    else
    {
        viewPort.TopLeftX = 0;
        viewPort.TopLeftY = 0;
        viewPort.Width = (efd::Float32)width;
        viewPort.Height = (efd::Float32)height;

        // Set cached port.
        m_kurrentViewPort = NiRect<efd::Float32>(0.0f, 1.0f, 1.0f, 0.0f);
    }

    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;

    m_pCurrentD3D11DeviceContext->RSSetViewports(1, &viewPort);

    // Set cached frustum.
    m_currentFrustum.m_fLeft = -0.5f;
    m_currentFrustum.m_fRight = 0.5f;
    m_currentFrustum.m_fTop = 0.5f;
    m_currentFrustum.m_fBottom = -0.5f;
    m_currentFrustum.m_fNear = 1.0f;
    m_currentFrustum.m_fFar = 10000.0f;
    m_currentFrustum.m_bOrtho = true;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::Do_GetCameraData(
    NiPoint3& worldLoc,
    NiPoint3& worldDir,
    NiPoint3& worldUp,
    NiPoint3& worldRight,
    NiFrustum& frustum,
    NiRect<efd::Float32>& port)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    worldRight.x = XMVectorGetX(m_invView.r[0]);
    worldRight.y = XMVectorGetY(m_invView.r[0]);
    worldRight.z = XMVectorGetZ(m_invView.r[0]);

    worldUp.x = XMVectorGetX(m_invView.r[1]);
    worldUp.y = XMVectorGetY(m_invView.r[1]);
    worldUp.z = XMVectorGetZ(m_invView.r[1]);

    worldDir.x = XMVectorGetX(m_invView.r[2]);
    worldDir.y = XMVectorGetY(m_invView.r[2]);
    worldDir.z = XMVectorGetZ(m_invView.r[2]);

    worldLoc.x = XMVectorGetX(m_invView.r[3]);
    worldLoc.y = XMVectorGetY(m_invView.r[3]);
    worldLoc.z = XMVectorGetZ(m_invView.r[3]);

    frustum = m_currentFrustum;
    port = m_kurrentViewPort;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::Do_BeginUsingRenderTargetGroup(
    NiRenderTargetGroup* pTarget,
    efd::UInt32 clearMode)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning("Can't call BeginUsingRenderTargetGroup "
            "without a valid device.");
        return false;
    }

    D3D11DeviceState* pDeviceState = GetDeviceState();

    EE_ASSERT(pDeviceState);

    pDeviceState->PSClearShaderResources();

    // This should have been checked in BeginUsingRenderTargetGroup
    EE_ASSERT(pTarget);

    EE_ASSERT(ValidateRenderTargetGroup(pTarget));

    ID3D11RenderTargetView* rtViewArray[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    memset(rtViewArray, 0, sizeof(rtViewArray));

    for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        Ni2DBuffer* p2DBuffer = pTarget->GetBuffer(i);
        if (p2DBuffer == NULL)
            continue;

        D3D112DBufferData* p2DBufferData =
            (D3D112DBufferData*)p2DBuffer->GetRendererData();
        EE_ASSERT(NiIsKindOf(D3D11RenderTargetBufferData, p2DBufferData));

        D3D11RenderTargetBufferData* pRTBufferData =
            (D3D11RenderTargetBufferData*)p2DBufferData;

        rtViewArray[i] = pRTBufferData->GetRenderTargetView();
    }

    ID3D11DepthStencilView* pDSView = NULL;
    NiDepthStencilBuffer* pDSBuffer = pTarget->GetDepthStencilBuffer();
    if (pDSBuffer)
    {
        D3D112DBufferData* p2DBufferData =
            (D3D112DBufferData*)pDSBuffer->GetRendererData();
        EE_ASSERT(NiIsKindOf(D3D11DepthStencilBufferData, p2DBufferData));

        D3D11DepthStencilBufferData* pDSBufferData =
            (D3D11DepthStencilBufferData*)p2DBufferData;

        pDSView = pDSBufferData->GetDepthStencilView();
    }

    m_pCurrentD3D11DeviceContext->OMSetRenderTargets(
        D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,
        rtViewArray,
        pDSView);

    m_pCurrentRenderTargetGroup = pTarget;

    Do_ClearBuffer(NULL, clearMode);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::Do_EndUsingRenderTargetGroup()
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning("Can't call EndUsingRenderTargetGroup "
            "without a valid device.");
        return false;
    }

    // This should have been checked in EndUsingRenderTargetGroup
    EE_ASSERT(m_pCurrentRenderTargetGroup);

    for (efd::UInt32 i = 0; i < m_pCurrentRenderTargetGroup->GetBufferCount(); i++)
    {
        D3D112DBufferData* pBuffData = (D3D112DBufferData*)
            m_pCurrentRenderTargetGroup->GetBufferRendererData(i);

        if (pBuffData == NULL)
            continue;

        // Store for DisplayFrame
        if (pBuffData->CanDisplayFrame())
        {
            // Only store once
            if (!m_buffersToUseAtDisplayFrame.FindPos(pBuffData))
                m_buffersToUseAtDisplayFrame.AddTail(pBuffData);
        }
    }

    m_pCurrentRenderTargetGroup = NULL;
    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::Do_RenderMesh(NiMesh* pMesh)
{
    EE_ASSERT_D3D11_DEVICE_THREAD;

    if (m_pD3D11Device == NULL)
    {
        D3D11Error::ReportWarning("Can't call RenderMesh without "
            "a valid device.");
        return;
    }

    EE_ASSERT(pMesh);


    // GetShaderAndVertexDecl will automatically increment the ref count
    // on the D3D11VertexDeclaration so it can be decremented at the end
    // of this function.
    D3D11MeshMaterialBindingPtr spMMB;
    D3D11ShaderInterface* pShader = GetShaderAndVertexDecl(pMesh, spMMB);
    EE_ASSERT(pShader);
    EE_ASSERT(m_pkCurrProp);

    if (spMMB == NULL)
    {
        // Can't render without a valid vertex declaration
        return;
    }

    // DT33837 Support multiple device contexts.
    ms_pDefaultVisibleArray->RemoveAll();
    ms_pDefaultVisibleArray->Add(*pMesh);

    pShader->RenderMeshes(ms_pDefaultVisibleArray);
}



//------------------------------------------------------------------------------------------------
D3D11ShaderInterface* D3D11Renderer::GetShaderAndVertexDecl(
    NiRenderObject* pRenderObject,
    D3D11MeshMaterialBindingPtr& spMMB)
{
    D3D11ShaderInterface* pShader = GetShaderAndVertexDecl_NoErrorShader(pRenderObject, spMMB);
    if (pShader && spMMB)
        return pShader;

    NiMesh* pMesh = NiVerifyStaticCast(NiMesh, pRenderObject);
    EE_ASSERT(pMesh);

    // If no shader is found, use error shader
    if (pShader == NULL)
    {
        Error(
            "No shader found for object \"%s\" pointer: %x\n"
            "Using Error Shader!\n", pMesh->GetName(), pMesh);
        EE_ASSERT(NiIsKindOf(D3D11ShaderInterface, GetErrorShader()));
        pShader = (D3D11ShaderInterface*)GetErrorShader();
        NiFixedString semantic("POSITION");

        NiDataStreamRef* pRef = NULL;
        NiDataStreamElement element;

        // Find a position stream. Otherwise, we'll have real problems with the
        // error shader.
        pMesh->FindStreamRefAndElementBySemantic(
            semantic,
            0,
            NiDataStreamElement::F_UNKNOWN,
            pRef,
            element);

        if (!pRef)
        {
            // POSITION stream was not found. Try for POSITION_BP.
            semantic = efd::FixedString("POSITION_BP");
            pMesh->FindStreamRefAndElementBySemantic(
                semantic,
                0,
                NiDataStreamElement::F_UNKNOWN,
                pRef,
                element);
        }

        // Ensure that either a POSITION or POSITION_BP stream was found and
        // that the stream supports GPU_READ. If not we'll return NULL.
        // If assertions are enabled, it will assert in Do_RenderMesh.
        // Otherwise, it will simply not render.
        if (pRef && (pRef->GetAccessMask() & NiDataStream::ACCESS_GPU_READ))
        {
            // Updated first entry in the semantic adapter table to use the
            // appropriate position semantic.
            pShader->GetSemanticAdapterTable().SetGenericSemantic(
                0,
                semantic,
                0);

            spMMB = D3D11MeshMaterialBinding::Create(
                NiVerifyStaticCast(NiMesh, pMesh),
                pShader->GetSemanticAdapterTable());
        }

        NiMaterialInstance* pMatInst = (NiMaterialInstance*)pMesh->GetActiveMaterialInstance();
        EE_ASSERT(pMatInst);

        // Update the material instance's cache with the error shader and
        // the associated vertex decl.
        pMatInst->ClearCachedShader();
        pMatInst->SetShaderCache(pShader);
        pMatInst->SetVertexDeclarationCache(spMMB);
    }

    EE_ASSERT(pShader);
    return pShader;
}

//------------------------------------------------------------------------------------------------
D3D11ShaderInterface* D3D11Renderer::GetShaderAndVertexDecl_NoErrorShader(
    NiRenderObject* pRenderObject,
    D3D11MeshMaterialBindingPtr& spMMB)
{
    spMMB = NULL;

    NiMesh* pMesh = NiVerifyStaticCast(NiMesh, pRenderObject);
    EE_ASSERT(pMesh);

    const efd::Bool noActiveMaterial_UseDefault = pMesh->GetActiveMaterial() == NULL;
    if (noActiveMaterial_UseDefault)
        pMesh->ApplyAndSetActiveMaterial(m_spCurrentDefaultMaterial);

    NiMaterialInstance* pMatInst = (NiMaterialInstance*)pMesh->GetActiveMaterialInstance();
    if (!pMatInst)
        return NULL;

    D3D11MeshMaterialBinding* pCachedMMB = (D3D11MeshMaterialBinding*)pMatInst->GetVertexDeclarationCache();

    // Fast path 1:
    // Cached shader + cached binding are both valid for this exact mesh.
    if (pMatInst->HasUsableCachedShaderAndVertexDecl(pMesh) &&
        pCachedMMB &&
        pCachedMMB->IsCompatibleWith(pMesh))
    {
        spMMB = pCachedMMB;
        return NiVerifyStaticCast(D3D11ShaderInterface, pMatInst->GetCachedShader());
    }

    // Fast path 2:
    // Shader is valid, but the vertex declaration cache is missing or belongs
    // to a different mesh. Rebuild geometry binding only; do not regenerate
    // the material descriptor.
    /*if (pMatInst->HasUsableCachedShader(pMesh))
    {
        D3D11ShaderInterface* pCachedShader = NiVerifyStaticCast(D3D11ShaderInterface, pMatInst->GetCachedShader());
        if (pCachedShader)
        {
            if (pCachedMMB && !pCachedMMB->IsCompatibleWith(pMesh))
            {
                // Important:
                // The cached binding belongs to another mesh/stream layout.
                // Do not let SetupGeometry reuse it.
                pMatInst->SetVertexDeclarationCache(NULL);
                pCachedMMB = NULL;
            }

            if (!pCachedMMB)
            {
                pCachedShader->SetupGeometry(pMesh, pMatInst);

                pCachedMMB =
                    (D3D11MeshMaterialBinding*)
                    pMatInst->GetVertexDeclarationCache();
            }

            if (pCachedMMB && pCachedMMB->IsCompatibleWith(pMesh))
            {
                spMMB = pCachedMMB;
                return pCachedShader;
            }
        }
    }*/

    // Slow path:
    // The material really needs shader resolution.
    //
    // If there is an incompatible binding still cached, clear it first.
    pCachedMMB = (D3D11MeshMaterialBinding*)pMatInst->GetVertexDeclarationCache();
    if (pCachedMMB && !pCachedMMB->IsCompatibleWith(pMesh))
        pMatInst->SetVertexDeclarationCache(NULL);

    D3D11ShaderInterface* pShader = NiVerifyStaticCast(D3D11ShaderInterface, pMesh->GetShaderFromMaterial());
    pCachedMMB = (D3D11MeshMaterialBinding*)pMatInst->GetVertexDeclarationCache();
    if (pCachedMMB && pCachedMMB->IsCompatibleWith(pMesh))
        spMMB = pCachedMMB;
    else
        spMMB = NULL;

    return pShader;
}

//------------------------------------------------------------------------------------------------
D3D11_PRIMITIVE_TOPOLOGY D3D11Renderer::GetD3D11TopologyFromPrimitiveType(
    NiPrimitiveType::Type primitiveType,
    efd::Bool adjacency)
{
    switch (primitiveType)
    {
    case NiPrimitiveType::PRIMITIVE_TRIANGLES:
        return (adjacency ?
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ :
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    case NiPrimitiveType::PRIMITIVE_TRISTRIPS:
        return (adjacency ?
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ :
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    case NiPrimitiveType::PRIMITIVE_LINES:
        return (adjacency ?
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ :
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    case NiPrimitiveType::PRIMITIVE_LINESTRIPS:
        return (adjacency ?
            D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ :
            D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    case NiPrimitiveType::PRIMITIVE_POINTS:
        return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    }
    return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

//------------------------------------------------------------------------------------------------
HRESULT D3D11Renderer::D3D11CreateDevice(
    __in_opt IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    __in_ecount_opt(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    __out_opt ID3D11Device** ppDevice,
    __out_opt D3D_FEATURE_LEVEL* pFeatureLevel,
    __out_opt ID3D11DeviceContext** ppImmediateContext)
{
    if (ms_hD3D11 == NULL)
        return E_INVALIDARG;

    EE_ASSERT(ms_pD3D11CreateDeviceFunc);

    return ms_pD3D11CreateDeviceFunc(
        pAdapter,
        DriverType,
        Software,
        Flags,
        pFeatureLevels,
        FeatureLevels,
        SDKVersion,
        ppDevice,
        pFeatureLevel,
        ppImmediateContext);
}

//------------------------------------------------------------------------------------------------
HRESULT D3D11Renderer::D3D11CreateDeviceAndSwapChain(
    __in_opt IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    __in_ecount_opt(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    __in_opt CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    __out_opt IDXGISwapChain** ppSwapChain,
    __out_opt ID3D11Device** ppDevice,
    __out_opt D3D_FEATURE_LEVEL* pFeatureLevel,
    __out_opt ID3D11DeviceContext** ppImmediateContext)
{
    if (ms_hD3D11 == NULL)
        return E_INVALIDARG;

    EE_ASSERT(ms_pD3D11CreateDeviceAndSwapChainFunc);

    return ms_pD3D11CreateDeviceAndSwapChainFunc(
        pAdapter,
        DriverType,
        Software,
        Flags,
        pFeatureLevels,
        FeatureLevels,
        SDKVersion,
        pSwapChainDesc,
        ppSwapChain,
        ppDevice,
        pFeatureLevel,
        ppImmediateContext);
}

//------------------------------------------------------------------------------------------------
const D3D_SHADER_MACRO* D3D11Renderer::GetD3D11MacroList(
    const efd::Char* pFileType,
    const D3D_SHADER_MACRO* pUserMacros)
{
    if (!GetGlobalMacroCount() && !GetMacroCount(pFileType))
    {
        // Exit if there are no global or file type-specific macros defined
        return pUserMacros;
    }

    ShaderData userMacros;
    NiTFixedStringMap<NiFixedString> allMacros;

    // Fill UserData array with user-supplied macro definitions
    // This will speed up searching for duplicate macro names,
    // because NiTFixedStringMap uses hash and only compares pointers.
    if (pUserMacros)
    {
        while (pUserMacros->Name && pUserMacros->Definition)
        {
            userMacros.AddMacro(
                NiFixedString(pUserMacros->Name),
                NiFixedString(pUserMacros->Definition));
            pUserMacros++;
        }
    }

    // Search for duplicate macro names and merge all macro containing structs
    BuildMacroList(pFileType, &userMacros, allMacros);

    // If buffer is not created or its size is too small, [re]allocate it
    if (!m_pShaderMacroArray || m_shaderMacroCount < (allMacros.GetCount() + 1))
    {
        if (m_pShaderMacroArray)
            EE_FREE(m_pShaderMacroArray);

        // Allocate buffer, that is 20% larger, than required to avoid
        // reallocations when few macros are added or changed
        m_shaderMacroCount = (efd::UInt32)((allMacros.GetCount() + 1) * 1.2f);
        m_pShaderMacroArray = EE_ALLOC(D3D_SHADER_MACRO, m_shaderMacroCount);
    }

    NiTMapIterator iter = allMacros.GetFirstPos();
    efd::UInt32 index = 0;
    while (iter)
    {
        NiFixedString name;
        NiFixedString value;
        allMacros.GetNext(iter, name, value);

        m_pShaderMacroArray[index].Name = name;
        m_pShaderMacroArray[index].Definition = value;
        index++;
    }
    // NULL-terminate a list
    m_pShaderMacroArray[index].Name = NULL;
    m_pShaderMacroArray[index].Definition = NULL;

    return m_pShaderMacroArray;
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::PushProfileMarker(const efd::Char* pName)
{
    if (m_isPIXActive == false || ms_pD3DPERF_BeginEventFunc == NULL)
        return;

    DWORD profileColor = (0xff000000U | (0xa0U << 16) | (0x80U << 8) | 0x80U);

    const efd::UInt32 bufferLen = 256;
    WCHAR wideString[bufferLen];
    efd::UInt32 stringLen = bufferLen;
    if (efd::Strlen(pName) + 1 > bufferLen)
    {
        //...
    }
    else
    {
        stringLen = efd::Strlen(pName) + 1;
    }

    efd::SInt32 result = MultiByteToWideChar(
        CP_ACP,
        0,
        pName,
        -1,
        wideString,
        stringLen);
    wideString[stringLen - 1] = 0;

    if (result != 0)
    {
        ms_pD3DPERF_BeginEventFunc(profileColor, wideString);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::PopProfileMarker()
{
    if (m_isPIXActive == false || ms_pD3DPERF_EndEventFunc == NULL)
        return;

    ms_pD3DPERF_EndEventFunc();
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::CreateWindowRenderTargetGroup(efd::WindowRef wndRef)
{
    CreationParameters createParams(wndRef);

    return CreateSwapChainRenderTargetGroup(createParams.m_swapChain);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Renderer::RecreateWindowRenderTargetGroup(efd::WindowRef wndRef)
{
    CreationParameters createParams(wndRef);

    return RecreateSwapChainRenderTargetGroup(createParams.m_swapChain);
}

//------------------------------------------------------------------------------------------------
void D3D11Renderer::ReleaseWindowRenderTargetGroup(efd::WindowRef wndRef)
{
    DestroySwapChainRenderTargetGroup(wndRef);
}

//------------------------------------------------------------------------------------------------
NiRenderTargetGroup* D3D11Renderer::GetWindowRenderTargetGroup(efd::WindowRef wndRef) const
{
    return GetSwapChainRenderTargetGroup(wndRef);
}

//------------------------------------------------------------------------------------------------
#if defined(EE_ASSERTS_ARE_ENABLED)
void D3D11Renderer::EnforceModifierPoliciy(NiVisibleArray* pArray)
{
    // Policy enforcement
    // Ensure that the NiShader completes all SYNC_RENDER modifers attached
    // to the render objects in the provided visible array.
    NiSyncArgs syncArgs;
    syncArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
    syncArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;

    const efd::UInt32 objectCount = pArray->GetCount();
    for (efd::UInt32 i = 0; i < objectCount; i++)
    {
        NiMesh* pMesh = NiDynamicCast(NiMesh, &pArray->GetAt(i));
        if (pMesh)
        {
            const efd::UInt32 modifierCount = pMesh->GetModifierCount();
            for (efd::UInt32 j = 0; j < modifierCount; j++)
            {
                NiMeshModifier* pModifier = pMesh->GetModifierAt(j);

                // Hitting this assert most likely indicates that the NiShader
                // used to render the mesh did _not_ call CompleteModifiers()
                // on the mesh. All NiShader derived classes that override the
                // Do_RenderMeshes() method must call CompleteModifiers() on
                // all the mesh objects passed in. This is required to ensure
                // that all outstanding SYNC_RENDER modifiers are completed
                // at render time. To address this issue add the following
                // code to the overriden Do_RenderMeshes() for each mesh:
                //
                // NiSyncArgs syncArgs;
                // syncArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
                // syncArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;
                // pMesh->CompleteModifiers(&syncArgs);
                //
                EE_ASSERT(pModifier->IsComplete(pMesh, &syncArgs));
            }
        }
    }
}
#endif

//------------------------------------------------------------------------------------------------
HRESULT D3D11Renderer::D3D10CreateBlob(SIZE_T NumBytes, LPD3D10BLOB* ppBuffer)
{
    EE_ASSERT(ms_pD3D10CreateBlob);
    return ms_pD3D10CreateBlob(NumBytes, ppBuffer);
}

//------------------------------------------------------------------------------------------------
