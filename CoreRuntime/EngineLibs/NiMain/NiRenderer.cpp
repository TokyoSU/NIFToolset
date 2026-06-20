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

#include "NiRenderer.h"
#include "NiAccumulator.h"
#include "NiNode.h"
#include "NiCamera.h"
#include "NiRenderObject.h"
#include "NiMaterialNodeLibrary.h"
#include "NiMaterialProperty.h"
#include "NiRenderTargetGroup.h"
#include "NiStandardMaterial.h"
#include "NiStandardMaterialNodeLibrary.h"
#include "NiShadowTechnique.h"
#include "NiSourceTexture.h"
#include "NiTextureValidator.h"
#include <efd/Profiler.h>

NiImplementRTTI(NiRenderer, NiObject, NiTypeMask::NiRenderer);

char NiRenderer::ms_acErrorString[256] = "";
NiRenderer* NiRenderer::ms_pkRenderer = NULL;
NiRenderer::CallbackObject* NiRenderer::ms_pkCallbackObject = NULL;

//--------------------------------------------------------------------------------------------------
EE_PROFILER_CONTEXT(Renderer);
//------------------------------------------------------------------------------------------------
NiRenderer::NiRenderer() :
//    m_kRendererLock("RenderL"),
//    m_kPrecacheCriticalSection("RPCache"),
//    m_kSourceDataCriticalSection("RSrcDat"),
    m_bRenderTargetGroupActive(false),
    m_eFrameState(FRAMESTATE_OUTSIDE_FRAME),
    m_uiFrameID(0),
    m_iMarkerDepth(0),
    m_usMaxAnisotropy(1)
{
    EE_ASSERT(ms_pkRenderer == NULL);
    ms_pkRenderer = this;

    NiMaterialNodeLibraryPtr spMaterialNodeLibrary =
        NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary();
    NiStandardMaterial* pkStandardMaterial =
        NiNew NiStandardMaterial(spMaterialNodeLibrary, false);
    pkStandardMaterial->AddDefaultFallbacks();
    m_spInitialDefaultMaterial = pkStandardMaterial;

// Use the default version of the error/warning/message callback object
// in debug builds.
#ifdef NIDEBUG
    if (!ms_pkCallbackObject)
    {
        ms_pkCallbackObject = &ms_kDefaultMessageProcessor;
    }
#endif
}

//------------------------------------------------------------------------------------------------
NiRenderer::~NiRenderer()
{
    //The default callback object is about to expire, so ensure that no
    //renderer created in the future attemps to invoke it
#ifdef NIDEBUG
    if (ms_pkCallbackObject == &ms_kDefaultMessageProcessor)
    {
        ms_pkCallbackObject = NULL;
    }
#endif

    ms_pkRenderer = NULL;

    DeleteAllMacros();
}

//------------------------------------------------------------------------------------------------
void NiRenderer::RemoveEffect(NiDynamicEffect* pkEffect)
{
    if (ms_pkRenderer)
        ms_pkRenderer->PurgeEffect(pkEffect);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::RemoveMaterial(NiMaterialProperty* pkMaterial)
{
    if (ms_pkRenderer)
        ms_pkRenderer->PurgeMaterial(pkMaterial);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::PurgeAllRendererData(NiAVObject* pkScene)
{
    PurgeAllTextures(true);
    if (pkScene)
        pkScene->PurgeRendererData(this);
};

//------------------------------------------------------------------------------------------------
bool NiRenderer::MapWindowPointToBufferPoint(unsigned int uiX,
    unsigned int uiY, float& fX, float& fY,
    const NiRenderTargetGroup* pkTarget)
{
    //const NiRenderTargetGroup* pkCurrentRTGroup = pkTarget;

    if (pkTarget == NULL)
        pkTarget = GetDefaultRenderTargetGroup();

    if (!pkTarget)
        return false;

    unsigned int uiXSize = pkTarget->GetWidth(0);
    unsigned int uiYSize = pkTarget->GetHeight(0);

    if (uiXSize && uiYSize)
    {
        fX = (float)(uiX) / (float)uiXSize;
        fY = 1.0f - (float)(uiY) / (float)uiYSize;

        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------------------------
const NiPixelFormat* NiRenderer::FindClosestDepthStencilFormat(
    const NiPixelFormat* pkFrontBufferFormat) const
{
    return FindClosestDepthStencilFormat(pkFrontBufferFormat, 24, 8);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiRenderer::ms_RTTI.GetName()));
}

//------------------------------------------------------------------------------------------------
void NiRenderer::SetDefaultProgramCache(NiFragmentMaterial* pkMaterial)
{
    SetDefaultProgramCache(pkMaterial,
        NiFragmentMaterial::GetDefaultAutoSaveProgramCache(),
        NiFragmentMaterial::GetDefaultWriteDebugProgramData(),
        NiFragmentMaterial::GetDefaultLoadProgramCacheOnCreation(),
        NiFragmentMaterial::GetDefaultLockProgramCache(),
        NiMaterial::GetDefaultWorkingDirectory());
}

//------------------------------------------------------------------------------------------------
void NiRenderer::Message(const char* pcFormat, ...)
{
    EE_UNUSED_ARG(pcFormat);

    //Return immediately if there is no callback object
    if (!ms_pkCallbackObject)
        return;

    //Format the message string
    char acOutput[1024] = "NiRenderer MESSAGE: ";
    size_t stIndex = strlen(acOutput);

    va_list kArgs;
    va_start(kArgs, pcFormat);
    stIndex += NiVsprintf(&acOutput[stIndex], 1024 - stIndex, pcFormat, kArgs);
    va_end(kArgs);

    // Newline and null terminate
    stIndex = efd::Min(stIndex, sizeof(acOutput) - 2);
    if (acOutput[stIndex-1] != '\n')
    {
        acOutput[stIndex  ] = '\n';
        acOutput[stIndex+1] = NULL;
    }
    else
    {
        acOutput[stIndex] = NULL;
    }

    //Invoke the callback object
    ms_pkCallbackObject->Message(acOutput);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::Warning(const char* pcFormat, ...)
{
    EE_UNUSED_ARG(pcFormat);

    //Return immediately if there is no callback object
    if (!ms_pkCallbackObject)
        return;

    //Format the message string
    char acOutput[1024] = "NiRenderer WARNING: ";
    size_t stIndex = strlen(acOutput);

    va_list kArgs;
    va_start(kArgs, pcFormat);
    stIndex += NiVsprintf(&acOutput[stIndex], 1024 - stIndex, pcFormat, kArgs);
    va_end(kArgs);

    // Newline and null terminate
    stIndex = efd::Min(stIndex, sizeof(acOutput) - 2);
    if (acOutput[stIndex-1] != '\n')
    {
        acOutput[stIndex  ] = '\n';
        acOutput[stIndex+1] = NULL;
    }
    else
    {
        acOutput[stIndex] = NULL;
    }

    //Invoke the callback object
    ms_pkCallbackObject->Warning(acOutput);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::Error(const char* pcFormat, ...)
{
    EE_UNUSED_ARG(pcFormat);

    //Return immediately if there is no callback object
    if (!ms_pkCallbackObject)
        return;

    //Format the message string
    char acOutput[1024] = "NiRenderer ERROR: ";
    size_t stIndex = strlen(acOutput);

    va_list kArgs;
    va_start(kArgs, pcFormat);
    stIndex += NiVsprintf(&acOutput[stIndex], 1024 - stIndex, pcFormat, kArgs);
    va_end(kArgs);

    // Newline and null terminate
    stIndex = efd::Min(stIndex, sizeof(acOutput) - 2);
    if (acOutput[stIndex-1] != '\n')
    {
        acOutput[stIndex  ] = '\n';
        acOutput[stIndex+1] = NULL;
    }
    else
    {
        acOutput[stIndex] = NULL;
    }

    //Invoke the callback object
    ms_pkCallbackObject->Error(acOutput);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::SetCallbackObject(CallbackObject* pkCallbackObject)
{
    ms_pkCallbackObject = pkCallbackObject;
}

//------------------------------------------------------------------------------------------------
NiRenderer::CallbackObject* NiRenderer::GetCallbackObject()
{
    return ms_pkCallbackObject;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::_SDMShutdown()
{
    ms_pkRenderer = NULL;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::SetCameraData(const NiCamera* pkCamera)
{
    SetCameraData(pkCamera->GetWorldLocation(),
        pkCamera->GetWorldDirection(), pkCamera->GetWorldUpVector(),
        pkCamera->GetWorldRightVector(), pkCamera->GetViewFrustum(),
        pkCamera->GetViewPort());
}

//------------------------------------------------------------------------------------------------
void NiRenderer::GetCameraData(NiCamera& kCamera)
{
    NiPoint3 kWorldLoc, kWorldDir, kWorldUp, kWorldRight;
    NiFrustum kFrustum;
    NiRect<float> kPort;
    GetCameraData(kWorldLoc, kWorldDir, kWorldUp, kWorldRight, kFrustum,
        kPort);

    // Build world-space camera transform.
    NiTransform kWorldTransform;
    kWorldTransform.m_Translate = kWorldLoc;
    kWorldTransform.m_Rotate = NiMatrix3(kWorldDir, kWorldUp, kWorldRight);
    kWorldTransform.m_fScale = 1.0f;

    // Compute local camera transform.
    NiTransform kLocalTransform;
    NiNode* pkParent = kCamera.GetParent();
    if (pkParent)
    {
        // If camera has a parent, un-apply parent transform.
        NiTransform kInverseParent;
        pkParent->GetWorldTransform().Invert(kInverseParent);
        kLocalTransform = kInverseParent * kWorldTransform;
    }
    else
    {
        kLocalTransform = kWorldTransform;
    }

    // Set both local and world transform on camera so that it does not need
    // to be updated here.
    kCamera.SetTranslate(kLocalTransform.m_Translate);
    kCamera.SetRotate(kLocalTransform.m_Rotate);
    kCamera.SetScale(kLocalTransform.m_fScale);
    kCamera.SetWorldTranslate(kWorldTransform.m_Translate);
    kCamera.SetWorldRotate(kWorldTransform.m_Rotate);
    kCamera.SetWorldScale(kWorldTransform.m_fScale);

    // Set frustum and viewport on camera.
    kCamera.SetViewFrustum(kFrustum);
    kCamera.SetViewPort(kPort);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::SetDefaultMaterial(NiMaterial* pkMaterial)
{
    if (pkMaterial == NULL)
        pkMaterial = m_spInitialDefaultMaterial;
    m_spCurrentDefaultMaterial = pkMaterial;
    EE_ASSERT(m_spCurrentDefaultMaterial);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::ApplyDefaultMaterial(NiRenderObject* pkGeometry)
{
    pkGeometry->ApplyAndSetActiveMaterial(GetDefaultMaterial());
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::PrecacheMesh(NiRenderObject*, bool)
{
    // Default behavior: do nothing.
    return true;
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::PrecacheShader(NiRenderObject*)
{
    // Default behavior: do nothing.
    return true;
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::PerformPrecache()
{
    // Default behavior: do nothing.
    return true;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::GetOnScreenCoord(
    const float fXOffset, const float fYOffset,
    const unsigned int uiWidth, const unsigned int uiHeight,
    unsigned int& uiOutX, unsigned int& uiOutY,
    const DisplayCorner eCorner,
    const bool bForceSafeZone)
{
    unsigned int uiX = 0;
    unsigned int uiY = 0;
    unsigned int uiXOffset, uiYOffset;

    unsigned int uiScreenWidth = GetDefaultBackBuffer()->GetWidth();
    unsigned int uiScreenHeight = GetDefaultBackBuffer()->GetHeight();

    ConvertFromNDCToPixels(fXOffset, fYOffset, uiXOffset, uiYOffset);

    // Use given ScreenCorner to determine which direction the
    // given offset will be moved.
    switch (eCorner)
    {
    case CORNER_TOP_LEFT:
        uiX = uiXOffset;
        uiY = uiYOffset;
        break;
    case CORNER_TOP_RIGHT:
        uiX = uiScreenWidth - uiXOffset;
        uiY = uiYOffset;
        break;
    case CORNER_BOTTOM_LEFT:
        uiX = uiXOffset;
        uiY = uiScreenHeight - uiYOffset;
        break;
    case CORNER_BOTTOM_RIGHT:
        uiX = uiScreenWidth - uiXOffset;
        uiY = uiScreenHeight - uiYOffset;
        break;
    }

    unsigned int uiRight = uiScreenWidth;
    unsigned int uiBottom = uiScreenHeight;

    if (bForceSafeZone)
    {
        ForceInSafeZone(uiRight, uiBottom);
        ForceInSafeZone(uiX, uiY);
    }

    // Ensure an object of uiWidth and uiHeight will be able to fully
    // fit onto the display.
    int iXDiff = uiRight - (uiX + uiWidth);
    int iYDiff = uiBottom - (uiY + uiHeight);
    iXDiff = efd::Min(0, iXDiff);
    iYDiff = efd::Min(0, iYDiff);

    uiOutX = efd::Max((int)uiX + iXDiff, 0);
    uiOutY = efd::Max((int)uiY + iYDiff, 0);
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::EndUsingRenderTargetGroup()
{
    if (!Check_RenderTargetGroupActive("EndUsingRenderTargetGroup", true))
        return false;

    if (!Do_EndUsingRenderTargetGroup())
        return false;

    UnlockRenderer(); // Finish lock for duration of render target

    m_pkCurrentRenderTargetGroup = NULL;
    m_bRenderTargetGroupActive = false;
    return true;
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::BeginInternalFrame()
{
    if (m_eFrameState == FRAMESTATE_INTERNAL_FRAME)
        return false;

    m_eSavedFrameState = m_eFrameState;
    m_eFrameState = FRAMESTATE_INTERNAL_FRAME;
    return true;
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::EndInternalFrame()
{
    if (!Check_FrameState("EndInternalFrame", FRAMESTATE_INTERNAL_FRAME))
        return false;
    m_eFrameState = m_eSavedFrameState;
    return true;
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::EndFrame()
{
    if (!Check_FrameState("EndFrame", FRAMESTATE_INSIDE_FRAME))
        return false;

    if (!Do_EndFrame())
    {
        m_eFrameState = FRAMESTATE_OUTSIDE_FRAME;
#ifdef NIDEBUG
        Warning("NiRenderer::EndFrame> Failed - "
            "Returning to frame to 'not set' state.\n");
#endif
        return false;
    }

    m_eFrameState = FRAMESTATE_WAITING_FOR_DISPLAY;
    return true;
}

//------------------------------------------------------------------------------------------------
NiDepthStencilBuffer* NiRenderer::CreateCompatibleDepthStencilBuffer(
    Ni2DBuffer* pkBuffer) const
{
    if (!pkBuffer)
        return NULL;

    const NiPixelFormat* pkFormat =
        FindClosestDepthStencilFormat(pkBuffer->GetPixelFormat());

    if (pkFormat)
    {
        NiDepthStencilBuffer* pkDSBuffer = NiDepthStencilBuffer::Create(
            pkBuffer->GetWidth(),
            pkBuffer->GetHeight(),
            NiRenderer::GetRenderer(), //"this" not used b/c of const
            *pkFormat,
            pkBuffer->GetMSAAPref());
        return pkDSBuffer;
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::SaveScreenShot(const char*, EScreenshotFormat)
{
    return false;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::SetRenderShadowTechnique(
    NiShadowTechnique*)
{
    // Base class version does nothing.
    return;
}
// Shader macros cleanup methods
//------------------------------------------------------------------------------------------------
void NiRenderer::ShaderData::DeleteAllMacros()
{
    m_kStringMap.RemoveAll();
}

//------------------------------------------------------------------------------------------------
void NiRenderer::DeleteAllMacros()
{
    // Remove all global macros
    m_kGlobalShaderData.DeleteAllMacros();

    // Delete allocated tables
    for (NiUInt32 i = 0; i < m_kShaderData.GetSize(); i++)
    {
        ShaderData* pkData = m_kShaderData.GetAt(i);
        if (pkData)
            NiDelete pkData;
    }

    // Clear arrays of tables and file type maps
    m_kShaderData.RemoveAll();
    m_kFiletypeMap.RemoveAll();
}

//------------------------------------------------------------------------------------------------
// Returns pointer to ShaderData structure for specified file type and creates
// new structure if requested, associating it with file type pcFileType
//------------------------------------------------------------------------------------------------
NiRenderer::ShaderData* NiRenderer::GetShaderData(
    const char* pcFileType,
    bool bCreate)
{
    if (!pcFileType)
        return NULL;

    NiMacrosIterator kIter = m_kFiletypeMap.GetFirstPos();
    while (kIter)
    {
        NiFixedString kStr;
        ShaderData* pkShaderData;
        m_kFiletypeMap.GetNext(kIter, kStr, pkShaderData);
        if (kStr.EqualsNoCase(pcFileType))
            return pkShaderData;
    }

    if (!bCreate)
        return NULL;

    ShaderData* pkData = NiNew ShaderData();
    m_kShaderData.Add(pkData);
    m_kFiletypeMap.SetAt(NiFixedString(pcFileType), pkData);
    return pkData;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::BuildMacroList(
    const char* pcFileType,
    ShaderData* pkUserMacros,
    NiTFixedStringMap<NiFixedString>& kFinalList)
{
    ShaderData* pkGData = GetGlobalShaderData();
    EE_ASSERT(pkGData);
    ShaderData* pkTData = GetShaderData(pcFileType);
    bool bPerTypeMacroExist = pkTData && pkTData->GetMacroCount() > 0;
    bool bUserMacroExist = pkUserMacros && pkUserMacros->GetMacroCount() > 0;
    NiFixedString kName;
    NiFixedString kValue;

    // Add global macros. Global macro/flags structure always exists.
    NiMacrosIterator kIter = pkGData->GetFirstMacro();
    while (kIter)
    {
        pkGData->GetNextMacro(kIter, kName, kValue);

        // Skip if same named macro exists in per-file or user definitions
        if (bPerTypeMacroExist && pkTData->IsMacro(kName))
            continue;

        if (bUserMacroExist && pkUserMacros->IsMacro(kName))
            continue;

        kFinalList.SetAt(kName, kValue);
    }

    if (bPerTypeMacroExist)
    {
        // Add file type-specific macros
        kIter = pkTData->GetFirstMacro();
        while (kIter)
        {
            pkTData->GetNextMacro(kIter, kName, kValue);

            // Skip if same named macro exists in user specified definitions
            if (bUserMacroExist && pkUserMacros->IsMacro(kName))
                continue;

            kFinalList.SetAt(kName, kValue);
        }
    }

    // And, finally, append user-specified macros
    if (!bUserMacroExist)
        return;

    kIter = pkUserMacros->GetFirstMacro();
    while (kIter)
    {
        pkUserMacros->GetNextMacro(kIter, kName, kValue);
        kFinalList.SetAt(kName, kValue);
    }
}

//------------------------------------------------------------------------------------------------
void NiRenderer::PushProfileMarker(const char*)
{
}

//------------------------------------------------------------------------------------------------
void NiRenderer::PopProfileMarker()
{
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::CreateWindowRenderTargetGroup(NiWindowRef)
{
    return false;
}

//------------------------------------------------------------------------------------------------
bool NiRenderer::RecreateWindowRenderTargetGroup(NiWindowRef)
{
    return false;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::ReleaseWindowRenderTargetGroup(NiWindowRef)
{
    /* */
}

//------------------------------------------------------------------------------------------------
NiRenderTargetGroup* NiRenderer::GetWindowRenderTargetGroup(NiWindowRef) const
{
    return NULL;
}

//------------------------------------------------------------------------------------------------
NiDepthStencilBuffer* NiRenderer::CreateResolvableDepthStencilBuffer(
    unsigned int,
    unsigned int,
    Ni2DBuffer::RendererData*) const
{
    return NULL;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::PushProfileMarkerVA(const char* pcFormat, ...)
{
    char acBuffer[2048];

    va_list kArgs;
    va_start(kArgs, pcFormat);
    efd::Vsnprintf(acBuffer, 2048, EE_TRUNCATE, pcFormat, kArgs);
    va_end(kArgs);

    PushProfileMarker(acBuffer);
}

//------------------------------------------------------------------------------------------------
NiTextureValidator* NiRenderer::GetTextureValidator()
{
    return m_spTextureValidator;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::SetTextureValidator(NiTextureValidator* pValidator)
{
    m_spTextureValidator = pValidator;
}

//------------------------------------------------------------------------------------------------
void NiRenderer::ValidateTexture(NiTexture* pTexture, efd::SmartPointer<NiPixelData>& spPixelData)
{
    if (!m_spTextureValidator)
        return;

    // Source texture validation
    NiSourceTexture* pSource = NiDynamicCast(NiSourceTexture, pTexture);
    if (pSource)
    {
        m_spTextureValidator->ValidateTexture(pSource, spPixelData);
    }
}

//------------------------------------------------------------------------------------------------
// CallbackObject functions
//------------------------------------------------------------------------------------------------

void NiRenderer::CallbackObject::Message(const char* pcText)
{
    EE_UNUSED_ARG(pcText);
    NiOutputDebugString(pcText);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::CallbackObject::Warning(const char* pcText)
{
    EE_UNUSED_ARG(pcText);
    NiOutputDebugString(pcText);
}

//------------------------------------------------------------------------------------------------
void NiRenderer::CallbackObject::Error(const char* pcText)
{
    EE_UNUSED_ARG(pcText);
    NiOutputDebugString(pcText);
}

//------------------------------------------------------------------------------------------------
