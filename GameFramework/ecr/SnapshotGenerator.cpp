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

#include "ecrPCH.h"

// SnapshotGenerator is compiled out of Shipping configurations
#ifndef EE_CONFIG_SHIPPING

#include <ecr/RenderService.h>
#include <ecr/SnapshotGenerator.h>
#include <efd/Endian.h>
#include <egf/egfBaseIDs.h>
#include <egf/SimDebugger.h>
#include <NiDrawSceneUtility.h>
#include <NiMeshCullingProcess.h>

#if defined(EE_PLATFORM_WIN32) && defined(NI_RENDERER_DX11)
    #include <ecrD3D11Renderer/D3D11Renderer.h>
    #include <ecrD3D11Renderer/D3D112DBufferData.h>
    #include <ecrD3D11Renderer/D3D11RenderedTextureData.h>
    #include <ecrD3D11Renderer/D3D11ResourceManager.h>
#elif defined(EE_PLATFORM_WIN32) && defined(NI_RENDERER_DX10)
    #include <NiD3D10RenderedTextureData.h>
    #include <NiD3D10ResourceManager.h>
#elif defined(EE_PLATFORM_WIN32) && defined(NI_RENDERER_DX9)
    #include <NiDX9RenderedTextureData.h>
#elif defined EE_PLATFORM_XBOX360
    #include <NiD3DRendererHeaders.h>
    #include <NiXenonRenderedTextureData.h>
    #include <NiXenonRenderTargetGroupData.h>
#endif

using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(SnapshotGenerator);

/// The Category for messages sent from the Toolbench Sim Debugger.
static const efd::Category kCAT_FromToolbenchSimDebugger = efd::Category(
    efd::UniversalID::ECU_Any, efd::kNetID_Any, efd::kBASEID_FromToolbenchSimDebugger);
/// The Category for messages sent to the Toolbench Sim Debugger.
static const efd::Category kCAT_ToToolbenchSimDebugger = efd::Category(
    efd::UniversalID::ECU_Any, efd::kNetID_Any, efd::kBASEID_ToToolbenchSimDebugger);

SnapshotGeneratorPtr SnapshotGenerator::ms_spInstance;

EE_HANDLER_WRAP(
    SnapshotGenerator,
    HandleSimDebuggerCommand,
    efd::StreamMessage,
    efd::kMSGID_SimDebuggerCommand);

#if defined(EE_PLATFORM_PS3) || defined(NI_RENDERER_BGFX)
// PS3 and bgfx use the bmp format to deliver image data to Toolbench.
struct BmpHeader
{
    // Leading UInt16 magic number is handled separately to avoid undesired padding
    efd::UInt32 m_fileSize;
    efd::UInt16 m_creator1;
    efd::UInt16 m_creator2;
    efd::UInt32 m_bmpOffset;
    efd::UInt32 m_headerSize;
    efd::UInt32 m_width;
    efd::UInt32 m_height;
    efd::UInt16 m_numPlanes;
    efd::UInt16 m_bitsPerPixel;
    efd::UInt32 m_compressType;
    efd::UInt32 m_bmpByteSize;
    efd::UInt32 m_horizontalRes;
    efd::UInt32 m_verticalRes;
    efd::UInt32 m_numColors;
    efd::UInt32 m_importantColors;

    BmpHeader()
    {
        memset(this, 0, sizeof(BmpHeader));

        m_bmpOffset = 54; // sizeof(magic number) + sizeof(BmpHeader)
        m_headerSize = 40;
        m_numPlanes = 1;
        m_bitsPerPixel = 24;
        m_horizontalRes = 2835;
        m_verticalRes = 2835;
    }

    void Serialize(efd::Archive& ar)
    {
        efd::Serializer::SerializeObject(m_fileSize, ar);
        efd::Serializer::SerializeObject(m_creator1, ar);
        efd::Serializer::SerializeObject(m_creator2, ar);
        efd::Serializer::SerializeObject(m_bmpOffset, ar);
        efd::Serializer::SerializeObject(m_headerSize, ar);
        efd::Serializer::SerializeObject(m_width, ar);
        efd::Serializer::SerializeObject(m_height, ar);
        efd::Serializer::SerializeObject(m_numPlanes, ar);
        efd::Serializer::SerializeObject(m_bitsPerPixel, ar);
        efd::Serializer::SerializeObject(m_compressType, ar);
        efd::Serializer::SerializeObject(m_bmpByteSize, ar);
        efd::Serializer::SerializeObject(m_horizontalRes, ar);
        efd::Serializer::SerializeObject(m_verticalRes, ar);
        efd::Serializer::SerializeObject(m_numColors, ar);
        efd::Serializer::SerializeObject(m_importantColors, ar);
    }
};
#endif


//-------------------------------------------------------------------------------------------------
SnapshotGenerator::SnapshotGenerator()
    : m_initialized(false)
    , m_pServiceManager(NULL)
    , m_pMessageService(NULL)
{
}

//-------------------------------------------------------------------------------------------------
void SnapshotGenerator::Create(efd::ServiceManager* pServiceMgr)
{
    if (ms_spInstance == NULL)
    {
        ms_spInstance = EE_NEW SnapshotGenerator();
        ms_spInstance->Initialize(pServiceMgr);
    }
}

//-------------------------------------------------------------------------------------------------
void SnapshotGenerator::Destroy()
{
    if (ms_spInstance)
    {
        ms_spInstance->Shutdown();
        ms_spInstance = NULL;
    }
}

//-------------------------------------------------------------------------------------------------
void SnapshotGenerator::Initialize(efd::ServiceManager* pServiceMgr)
{
    if (m_initialized || egf::SimDebugger::Instance() == NULL)
        return;

    m_pServiceManager = pServiceMgr;

    // Prepare to receive command messages from Toolbench
    m_pMessageService = pServiceMgr->GetSystemServiceAs<efd::MessageService>(
        efd::kCLASSID_ToolsMessageService);
    m_pMessageService->Subscribe(this, kCAT_FromToolbenchSimDebugger);

    m_initialized = true;
}

//-------------------------------------------------------------------------------------------------
void SnapshotGenerator::Shutdown()
{
    if (m_initialized)
    {
        // Cleanup messaging
        m_pMessageService->Unsubscribe(this, kCAT_FromToolbenchSimDebugger);
    }
}

//-------------------------------------------------------------------------------------------------
void SnapshotGenerator::HandleSimDebuggerCommand(
    const efd::StreamMessage* pMessage,
    efd::Category targetChannel)
{
    pMessage->ResetForUnpacking();

    // Find command
    efd::utf8string command;
    *pMessage >> command;

    efd::UInt32 imageWidth, imageHeight;
    efd::Point3 cameraPosition;

    if (command == "GENERATE_BACKDROP")
    {
        *pMessage >> imageWidth;
        *pMessage >> imageHeight;

        cameraPosition = efd::Point3(0.0f, 0.0f, 10000.0f);

        GenerateImage(imageWidth, imageHeight, cameraPosition, true);
    }
    else if (command == "GENERATE_SNAPSHOT")
    {
        *pMessage >> imageWidth;
        *pMessage >> imageHeight;

        *pMessage >> cameraPosition;

        GenerateImage(imageWidth, imageHeight, cameraPosition, false);
    }
}

//-------------------------------------------------------------------------------------------------
bool SnapshotGenerator::GenerateImage(
    efd::UInt32 width,
    efd::UInt32 height,
    const efd::Point3& cameraPos,
    efd::Bool isBackdrop)
{
    // Prepare service access
    ecr::RenderService* pRenderService =
        m_pServiceManager->GetSystemServiceAs<ecr::RenderService>();
    if (!pRenderService)
        return false;
    ecr::SceneGraphService* pSceneGraphService =
        m_pServiceManager->GetSystemServiceAs<ecr::SceneGraphService>();
    if (!pSceneGraphService)
        return false;
    egf::EntityManager* pEntityManager =
        m_pServiceManager->GetSystemServiceAs<egf::EntityManager>();
    if (!pEntityManager)
        return false;

    // Create a target for rendering
    NiRenderer* pRenderer = pRenderService->GetRenderer();
    if (!pRenderer)
        return false;
    NiRenderedTexturePtr spRenderedTexture = NiRenderedTexture::Create(width, height, pRenderer);
    if (!spRenderedTexture)
        return false;
    NiRenderTargetGroupPtr spRenderTargetGroup = NiRenderTargetGroup::Create(
        spRenderedTexture->GetBuffer(),
        pRenderer,
        true,
        true);
    if (!spRenderTargetGroup)
        return false;

    NiTPointerList<NiAVObjectPtr> staticScenes;
    NiBound bounds;
    bounds.SetCenter(0.0f, 0.0f, 0.0f);
    bounds.SetRadius(0.0f);

    // Accumulate objects to be rendered and compute world bounds
    egf::EntityManager::EntityMap::const_iterator it = pEntityManager->GetFirstEntityPos();
    egf::Entity* pEntity;
    while (pEntityManager->GetNextEntity(it, pEntity))
    {
        // Filter out dynamic entities for backdrop image
        if (isBackdrop && pEntity->GetModel()->ContainsModel("Actor"))
            continue;

        NiAVObject* pScene = pSceneGraphService->GetSceneGraphFromEntity(pEntity);
        if (pScene)
        {
            staticScenes.AddTail(pScene);
            bounds.Merge(&pScene->GetWorldBound());
        }
    }
    efd::Float32 worldExtents = efd::Max(efd::Abs(bounds.GetCenter().x),
        efd::Abs(bounds.GetCenter().y)) + bounds.GetRadius();

    // Prepare camera
    NiCameraPtr spCamera = NiNew NiCamera;
    if (isBackdrop)
    {
        // Use orthographic camera for backdrop image
        NiFrustum frustum(
            -worldExtents,
            worldExtents,
            worldExtents,
            -worldExtents,
            1.0f,
            20000.0f,
            true);
        spCamera->SetViewFrustum(frustum);
    }
    else
    {
        // Use perspective camera for snapshot image
        float dim = 1.0f / efd::Sqrt(3.0f);
        NiFrustum frustum(-dim, dim, dim, -dim, 1.0f, 100000.0f);
        spCamera->SetViewFrustum(frustum);
    }
    NiPoint3 lookDir(0.0f, 0.0001f, -1.0f);
    lookDir.Unitize();
    NiPoint3 lookTangent = lookDir.Cross(NiPoint3::UNIT_Z);
    lookTangent.Unitize();
    NiPoint3 lookBiTangent = lookTangent.Cross(lookDir);
    NiMatrix3 rotation = NiMatrix3(lookDir, lookBiTangent, lookTangent);
    spCamera->SetRotate(rotation);
    spCamera->SetTranslate(cameraPos);
    spCamera->Update(0.0f);

    // Begin image rendering
    pRenderer->BeginOffScreenFrame();
    pRenderer->BeginUsingRenderTargetGroup(spRenderTargetGroup, NiRenderer::CLEAR_ALL);

    // Render all collected objects to the target
    NiVisibleArray visibleSet(1024, 1024);
    NiMeshCullingProcess culler(&visibleSet, NULL);
    NiTListIterator pos = staticScenes.GetHeadPos();
    while (pos)
    {
        NiAVObject* pObject = staticScenes.GetNext(pos);
        NiCullScene(spCamera, pObject, culler, visibleSet);
        NiDrawVisibleArray(spCamera, visibleSet);
    }

    // End image rendering
    pRenderer->EndUsingRenderTargetGroup();
    pRenderer->EndOffScreenFrame();

    // Prepare results message for Toolbench
    efd::StreamMessagePtr spResultsMessage;
    if (isBackdrop)
    {
        spResultsMessage = EE_NEW efd::MessageWrapper<efd::StreamMessage,
            efd::kMSGID_BackdropGenerated>;
        *spResultsMessage << worldExtents;
    }
    else
    {
        spResultsMessage = EE_NEW efd::MessageWrapper<efd::StreamMessage,
            efd::kMSGID_SnapshotGenerated>;
    }
    efd::Archive& ar = spResultsMessage->GetArchive();

#ifdef EE_PLATFORM_WIN32
    #if defined(NI_RENDERER_DX9)
    if (pRenderer->GetRendererID() == efd::SystemDesc::RENDERER_DX9)
    {
        // Get pointer to D3D device
        NiDX9Renderer* pDX9Renderer = NiDynamicCast(NiDX9Renderer, pRenderer);
        if (!pDX9Renderer)
            return false;
        D3DDevicePtr pD3DDevice = pDX9Renderer->GetD3DDevice();

        // Get D3D surface pointer to source pixel data in GPU memory
        NiDX9RenderedTextureData* pRenderedTextureData =
            NiDynamicCast(NiDX9RenderedTextureData,
                spRenderedTexture->GetRendererData());
        if (!pRenderedTextureData)
            return false;
        D3DSurfacePtr pSrc;
        ((LPDIRECT3DTEXTURE9)pRenderedTextureData->GetD3DTexture())->GetSurfaceLevel(0, &pSrc);
        if (!pSrc)
            return false;

        // Create a destination texture in system memory to copy source pixel data into
        D3DSURFACE_DESC surfDesc;
        pSrc->GetDesc(&surfDesc);
        LPDIRECT3DTEXTURE9 pD3DReadTexture = NULL;
        pD3DDevice->CreateTexture(surfDesc.Width, surfDesc.Height, 1, 0, surfDesc.Format,
            D3DPOOL_SYSTEMMEM, &pD3DReadTexture, 0);
        if (!pD3DReadTexture)
        {
            pSrc->Release();
            return false;
        }

        // Copy pixel data to an accessible texture
        D3DSurfacePtr pDst;
        pD3DReadTexture->GetSurfaceLevel(0, &pDst);
        pD3DDevice->GetRenderTargetData(pSrc, pDst);
        pSrc->Release();

        // Save the texture data to a buffer in PNG format
        ID3DXBuffer* pBuffer = NULL;
        D3DXSaveSurfaceToFileInMemory(&pBuffer, D3DXIFF_PNG, pDst, NULL, NULL);
        pDst->Release();
        D3D_POINTER_RELEASE(pD3DReadTexture);
        if (!pBuffer)
            return false;

        // Write the image data to the Toolbench message
        efd::Serializer::SerializeRawBytes(
            (efd::UInt8*)pBuffer->GetBufferPointer(),
            pBuffer->GetBufferSize(),
            ar);
        pBuffer->Release();
    }
    else
    {
        return false;
    }
#elif defined(NI_RENDERER_DX10)
    if (pRenderer->GetRendererID() == efd::SystemDesc::RENDERER_D3D10)
    {
        // Get pointer to D3D resource manager
        NiD3D10Renderer* pD3D10Renderer = NiDynamicCast(NiD3D10Renderer, pRenderer);
        if (!pD3D10Renderer)
            return false;
        NiD3D10ResourceManager* pResourceManager = pD3D10Renderer->GetResourceManager();

        // Get texture pointer to data in GPU memory
        NiD3D10RenderTargetBufferData* pBuffData = NiVerifyStaticCast(NiD3D10RenderTargetBufferData,
            (NiD3D102DBufferData*)spRenderTargetGroup->GetBufferRendererData(0));
        EE_ASSERT(pBuffData);
        ID3D10Resource* pRenderedTexture = pBuffData->GetRenderTargetBuffer();
#ifdef EE_ASSERTS_ARE_ENABLED
        D3D10_RESOURCE_DIMENSION resourceDim;
        pRenderedTexture->GetType(&resourceDim);
        EE_ASSERT(resourceDim == D3D10_RESOURCE_DIMENSION_TEXTURE2D);
#endif
        ID3D10Texture2D* pRenderedTexture2D = (ID3D10Texture2D*)pRenderedTexture;

        // If multisampled, must resolve to a single-sampled resource
        D3D10_TEXTURE2D_DESC textureDesc;
        pRenderedTexture2D->GetDesc(&textureDesc);
        ID3D10Texture2D* pSingleSampled = NULL;
        if (textureDesc.SampleDesc.Count != 1 || textureDesc.SampleDesc.Quality != 0)
        {
            pSingleSampled = pResourceManager->CreateTexture2D(textureDesc.Width,
                textureDesc.Height, textureDesc.MipLevels, textureDesc.ArraySize,
                textureDesc.Format, 1, 0, D3D10_USAGE_DEFAULT, 0, 0, 0);
            if (pSingleSampled == NULL)
                return false;

            pD3D10Renderer->GetD3D10Device()->ResolveSubresource(pSingleSampled, 0,
                pRenderedTexture2D, 0, textureDesc.Format);
        }
        else
        {
            pSingleSampled = pRenderedTexture2D;
            pSingleSampled->AddRef();
        }

        // Bring back to system RAM
        ID3D10Texture2D* pStagingTexture = pResourceManager->CreateTexture2D(textureDesc.Width,
            textureDesc.Height, 1, textureDesc.ArraySize, textureDesc.Format, 1, 0,
            D3D10_USAGE_STAGING, 0, D3D10_CPU_ACCESS_READ, 0);
        if (!pStagingTexture)
        {
            pSingleSampled->Release();
            return false;
        }
        pD3D10Renderer->GetD3D10Device()->CopyResource(pStagingTexture, pSingleSampled);
        pSingleSampled->Release();

        // Save the texture data to a buffer in PNG format
        ID3D10Blob* pBuffer = NULL;
        D3DX10SaveTextureToMemory(pStagingTexture, D3DX10_IFF_PNG, &pBuffer, 0);
        pStagingTexture->Release();

        // Write the image data to the Toolbench message
        efd::Serializer::SerializeRawBytes(
            (efd::UInt8*)pBuffer->GetBufferPointer(),
            pBuffer->GetBufferSize(),
            ar);
        pBuffer->Release();
    }
    else
    {
        return false;
    }
#elif defined(NI_RENDERER_DX11)
    if (pRenderer->GetRendererID() == efd::SystemDesc::RENDERER_D3D11)
    {
        // Get pointer to D3D resource manager
        ecr::D3D11Renderer* pD3D11Renderer = NiDynamicCast(ecr::D3D11Renderer, pRenderer);
        if (!pD3D11Renderer)
            return false;
        ecr::D3D11ResourceManager* pResourceManager = pD3D11Renderer->GetResourceManager();

        // Get texture pointer to data in GPU memory
        D3D11RenderTargetBufferData* pBuffData = NiVerifyStaticCast(D3D11RenderTargetBufferData,
            (D3D112DBufferData*)spRenderTargetGroup->GetBufferRendererData(0));
        EE_ASSERT(pBuffData);
        ID3D11Resource* pRenderedTexture = pBuffData->GetRenderTargetBuffer();
#ifdef EE_ASSERTS_ARE_ENABLED
        D3D11_RESOURCE_DIMENSION resourceDim;
        pRenderedTexture->GetType(&resourceDim);
        EE_ASSERT(resourceDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D);
#endif
        ID3D11Texture2D* pRenderedTexture2D = (ID3D11Texture2D*)pRenderedTexture;

        // If multisampled, must resolve to a single-sampled resource
        D3D11_TEXTURE2D_DESC textureDesc;
        pRenderedTexture2D->GetDesc(&textureDesc);
        ID3D11Texture2D* pSingleSampled = NULL;
        if (textureDesc.SampleDesc.Count != 1 || textureDesc.SampleDesc.Quality != 0)
        {
            pSingleSampled = pResourceManager->CreateTexture2D(textureDesc.Width,
                textureDesc.Height, textureDesc.MipLevels, textureDesc.ArraySize,
                textureDesc.Format, 1, 0, D3D11_USAGE_DEFAULT, 0, 0, 0);

            if (pSingleSampled == NULL)
                return false;

            pD3D11Renderer->GetCurrentD3D11DeviceContext()->ResolveSubresource(pSingleSampled, 0,
                pRenderedTexture2D, 0, textureDesc.Format);
        }
        else
        {
            pSingleSampled = pRenderedTexture2D;
            pSingleSampled->AddRef();
        };

        // Bring back to system RAM
        ID3D11Texture2D* pStagingTexture = pResourceManager->CreateTexture2D(textureDesc.Width,
            textureDesc.Height, 1, textureDesc.ArraySize, textureDesc.Format, 1, 0,
            D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0);
        if (pStagingTexture == NULL)
        {
            pSingleSampled->Release();
            return false;
        }
        pD3D11Renderer->GetCurrentD3D11DeviceContext()->CopyResource(pStagingTexture,
            pSingleSampled);
        pSingleSampled->Release();

        // Save the texture data to a buffer in PNG format
        ID3D10Blob* pBuffer = NULL;
        D3DX11SaveTextureToMemory(pD3D11Renderer->GetCurrentD3D11DeviceContext(), pStagingTexture,
            D3DX11_IFF_PNG, &pBuffer, 0);
        pStagingTexture->Release();

        // Write the image data to the Toolbench message
        efd::Serializer::SerializeRawBytes(
            (efd::UInt8*)pBuffer->GetBufferPointer(),
            pBuffer->GetBufferSize(),
            ar);
        pBuffer->Release();
    }
    else
    {
        return false;
    }
#elif defined(NI_RENDERER_BGFX)
    if (pRenderer->GetRendererID() == efd::SystemDesc::RENDERER_BGFX)
    {
        NiPixelDataPtr spPixelData = pRenderer->TakeScreenShot(NULL,
            spRenderTargetGroup);
        if (!spPixelData ||
            spPixelData->GetPixelFormat() != NiPixelFormat::RGBA32)
        {
            return false;
        }

        // BMP scanlines are padded to a 4-byte boundary and stored bottom-up.
        // TakeScreenShot returns RGBA32, so emit the required BGR byte order.
        const efd::UInt32 rowBytes = width * 3;
        const efd::UInt32 rowPadding = (4 - (rowBytes & 3)) & 3;
        const efd::UInt32 rowStride = rowBytes + rowPadding;

        efd::UInt16 magicNumber = 0x4d42;
        BmpHeader bmpHeader;
        bmpHeader.m_width = width;
        bmpHeader.m_height = height;
        bmpHeader.m_bmpByteSize = rowStride * height;
        bmpHeader.m_fileSize = bmpHeader.m_bmpOffset +
            bmpHeader.m_bmpByteSize;

        const efd::UInt32 streamSize = sizeof(efd::UInt16) +
            sizeof(BmpHeader) + bmpHeader.m_bmpByteSize;
        ar.CheckBytes(streamSize);
        ar.SetEndianness(efd::Endian_Little);
        efd::Serializer::SerializeObject(magicNumber, ar);
        efd::Serializer::SerializeObject(bmpHeader, ar);

        const efd::UInt8 zero = 0;
        for (efd::SInt32 row = static_cast<efd::SInt32>(height) - 1;
            row >= 0; --row)
        {
            const efd::UInt8* pixels = spPixelData->GetPixels() +
                static_cast<size_t>(row) * width * 4;
            for (efd::UInt32 col = 0; col < width; ++col)
            {
                // GetPixels() is const in this path, so the dereferenced bytes
                // are const UInt8 values. SerializeObject<const UInt8> would
                // select the generic object serializer and try to call
                // UInt8::Serialize(). This path only packs the screenshot, so
                // use the serializer's const-aware primitive entry point.
                efd::Serializer::SerializeConstObject(*(pixels + col * 4 + 2), ar);
                efd::Serializer::SerializeConstObject(*(pixels + col * 4 + 1), ar);
                efd::Serializer::SerializeConstObject(*(pixels + col * 4 + 0), ar);
            }
            for (efd::UInt32 pad = 0; pad < rowPadding; ++pad)
                efd::Serializer::SerializeConstObject(zero, ar);
        }
        ar.SetEndianness(efd::Endian_NetworkOrder);
    }
    else
    {
        return false;
    }
#else
    return false;
#endif
#elif defined EE_PLATFORM_XBOX360
    // Get pointer to D3D device
    NiXenonRenderer* pXenonRenderer = NiDynamicCast(NiXenonRenderer, pRenderer);
    if (!pXenonRenderer)
        return false;
    D3DDevicePtr pD3DDevice = pXenonRenderer->GetD3DDevice();

    // Block until GPU is done rendering to buffer
    DWORD fence = pD3DDevice->InsertFence();
    pD3DDevice->BlockOnFence(fence);

    // Get a pointer to the rendered texture
    unsigned int uiFace = 0;
    D3DBaseTexturePtr pTex = NULL;
    NiXenon2DBufferData* pBuffData = (NiXenon2DBufferData*)
        spRenderTargetGroup->GetBufferRendererData(0);
    if (!pBuffData)
        return false;
    pBuffData->GetResolveDestination(pTex, uiFace);

    // Lock the surface so pixel data can be accessed
    XGTEXTURE_DESC desc;
    XGGetTextureDesc(pTex, 0, &desc);
    D3DSurfacePtr pSurf = NULL;
    ((D3DTexturePtr)pTex)->GetSurfaceLevel(0, &pSurf);
    D3DLOCKED_RECT lockedRect;
    pSurf->LockRect(&lockedRect, NULL, 0);

    // Save the texture data to a buffer in PNG format
    XGEndianSwapMemory(lockedRect.pBits, lockedRect.pBits, XGENDIAN_8IN32, 4, desc.SlicePitch / 4);
    ID3DXBuffer* pBuffer = NULL;
    D3DXSaveTextureToFileInMemory(&pBuffer, D3DXIFF_PNG, (D3DTexturePtr)pTex, NULL);
    pSurf->UnlockRect();
    if (!pBuffer)
        return false;

    // Write the image data to the Toolbench message
    efd::Serializer::SerializeRawBytes(
        (efd::UInt8*)pBuffer->GetBufferPointer(),
        pBuffer->GetBufferSize(),
        ar);
    pBuffer->Release();

    NiXenonRenderTargetGroupData::ClearRenderTargets();
#elif defined EE_PLATFORM_PS3
    NiPixelData* pPixelData = pRenderer->TakeScreenShot(NULL, spRenderTargetGroup);
    if (!pPixelData)
        return false;

    // Create a bmp from the rendered pixels
    efd::UInt16 magicNumber = 0x4d42;

    BmpHeader bmpHeader;
    bmpHeader.m_width = width;
    bmpHeader.m_height = height;
    bmpHeader.m_bmpByteSize = width * height * 3;
    bmpHeader.m_fileSize = bmpHeader.m_bmpOffset + bmpHeader.m_bmpByteSize;

    // Pre-allocate the large stream buffer for performance
    efd::UInt32 streamSize = sizeof(efd::UInt16) + sizeof(BmpHeader) + width * height * 3;
    ar.CheckBytes(streamSize);

    // Header must be written as Little Endian (which is currently our default, but this is just
    // in case that ever changes).
    ar.SetEndianness(efd::Endian_Little);
    efd::Serializer::SerializeObject(magicNumber, ar);
    efd::Serializer::SerializeObject(bmpHeader, ar);

    // Convert pixel format
    efd::UInt8* pixels;
    for (efd::SInt32 row = height - 1; row >= 0; --row)
    {
        pixels = pPixelData->GetPixels() + row * width * 4;
        for (efd::UInt32 col = 0; col < width; ++col)
        {
            efd::Serializer::SerializeObject(*(pixels + col * 4 + 3), ar);
            efd::Serializer::SerializeObject(*(pixels + col * 4 + 2), ar);
            efd::Serializer::SerializeObject(*(pixels + col * 4 + 1), ar);
        }
    }
    ar.SetEndianness(efd::Endian_NetworkOrder);
#endif

    // Send the results
    m_pMessageService->Send(spResultsMessage, kCAT_ToToolbenchSimDebugger);
    return true;
}

#endif // EE_CONFIG_SHIPPING
