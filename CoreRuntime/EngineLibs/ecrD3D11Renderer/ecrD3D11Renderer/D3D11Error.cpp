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

#include "D3D11Error.h"

#include <NiSystem.h>
#include "NiRenderer.h"

using namespace ecr;

D3D11Error* D3D11Error::ms_pD3D11Error = NULL;

//------------------------------------------------------------------------------------------------
void D3D11Error::_SDMInit()
{
    // Don't need critical section because this is only called during startup
    ms_pD3D11Error = EE_NEW D3D11Error;
}

//------------------------------------------------------------------------------------------------
void D3D11Error::_SDMShutdown()
{
    // Don't need critical section because this is only called during startup
    EE_DELETE ms_pD3D11Error;
    ms_pD3D11Error = NULL;
}

//------------------------------------------------------------------------------------------------
D3D11Error::D3D11Error() :
    m_lastErrorMessage(D3D11ERROR_NONE)
{
    memset(m_lastAdditionalInfo, 0, sizeof(m_lastAdditionalInfo));
}

//------------------------------------------------------------------------------------------------
D3D11Error::~D3D11Error()
{
    /* */
}

//------------------------------------------------------------------------------------------------
void D3D11Error::ReportMessage(const efd::Char* pMessage, ...)
{
    if (pMessage == NULL)
        pMessage = "";

    const efd::UInt32 bufferLength = MAX_ADDITIONAL_INFO_STRING_LENGTH;
    efd::Char errorString[bufferLength] = "(D3D11Renderer) ";
    efd::UInt32 index = efd::Strlen(errorString);

    va_list args;
    va_start(args, pMessage);
    index += efd::Vsprintf(
        &errorString[index], 
        bufferLength - index,
        pMessage, 
        args);
    va_end(args);

    // Rely on NiRenderer::Message to all newline and null termination
    return NiRenderer::Message(errorString);
}

//------------------------------------------------------------------------------------------------
void D3D11Error::ReportWarning(const efd::Char* pWarningMessage, ...)
{
    if (pWarningMessage == NULL)
        pWarningMessage = "";

    const efd::UInt32 bufferLength = MAX_ADDITIONAL_INFO_STRING_LENGTH;
    efd::Char errorString[bufferLength] = "(D3D11Renderer) ";
    efd::UInt32 index = efd::Strlen(errorString);

    va_list args;
    va_start(args, pWarningMessage);
    index += efd::Vsprintf(
        &errorString[index], 
        bufferLength - index,
        pWarningMessage, 
        args);
    va_end(args);

    // Rely on NiRenderer::Warning to all newline and null termination
    return NiRenderer::Warning(errorString);
}

//------------------------------------------------------------------------------------------------
void D3D11Error::ReportError(
    ErrorMessage error,
    const efd::Char* pAdditionalMessage, 
    ...)
{
    if (ms_pD3D11Error == NULL)
        return;

    ms_pD3D11Error->m_lastErrorMessage = error;
    if (pAdditionalMessage == NULL)
        pAdditionalMessage = "";

    const efd::UInt32 bufferLength = MAX_ADDITIONAL_INFO_STRING_LENGTH;
    efd::Char errorString[bufferLength];
    efd::Sprintf(
        errorString, 
        bufferLength, 
        "(D3D11Renderer) %s ",
        GetErrorText(error));
    efd::UInt32 index = efd::Strlen(errorString);
    va_list args;
    va_start(args, pAdditionalMessage);
    index += efd::Vsprintf(
        &errorString[index], 
        bufferLength - index,
        pAdditionalMessage, 
        args);
    va_end(args);

    // Rely on NiRenderer::Error to all newline and null termination
    return NiRenderer::Error(errorString);
}

//------------------------------------------------------------------------------------------------
D3D11Error::ErrorMessage D3D11Error::GetLastErrorMessage()
{
    if (ms_pD3D11Error == NULL)
        return D3D11ERROR_ERROR_SYSTEM_ERROR;
    return ms_pD3D11Error->m_lastErrorMessage;
}

//------------------------------------------------------------------------------------------------
const efd::Char* const D3D11Error::GetErrorText(ErrorMessage eMessage)
{
    switch (eMessage)
    {
    case D3D11ERROR_NONE:
        return "No error.";
    case D3D11ERROR_ERROR_SYSTEM_ERROR:
        return "Error with the D3D11Error system.";
    case D3D11ERROR_D3D11_LIB_MISSING:
        return "Error loading D3D11 library.";
    case D3D11ERROR_DXGI_LIB_MISSING:
        return "Error loading DXGI library.";
    case D3D11ERROR_DEVICE_CREATION_FAILED:
        return "Error creating D3D11 device.";
    case D3D11ERROR_FACTORY_CREATION_FAILED:
        return "Error creating DXGI factory.";
    case D3D11ERROR_SWAP_CHAIN_CREATION_FAILED:
        return "Error creating swap chain resource.";
    case D3D11ERROR_TEXTURE1D_CREATION_FAILED:
        return "Error creating 1D texture resource.";
    case D3D11ERROR_TEXTURE2D_CREATION_FAILED:
        return "Error creating 2D texture resource.";
    case D3D11ERROR_TEXTURE3D_CREATION_FAILED:
        return "Error creating 3D texture resource.";
    case D3D11ERROR_BUFFER_CREATION_FAILED:
        return "Error creating buffer resource.";
    case D3D11ERROR_RENDER_TARGET_VIEW_CREATION_FAILED:
        return "Error creating render target view.";
    case D3D11ERROR_DEPTH_STENCIL_VIEW_CREATION_FAILED:
        return "Error creating depth stencil view.";
    case D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED:
        return "Error creating shader resource view.";
    case D3D11ERROR_UNORDERED_ACCESS_VIEW_CREATION_FAILED:
        return "Error creating unordered access view.";
    case D3D11ERROR_BLEND_STATE_CREATION_FAILED:
        return "Error creating blend state object.";
    case D3D11ERROR_DEPTH_STENCIL_STATE_CREATION_FAILED:
        return "Error creating depth stencil state object.";
    case D3D11ERROR_RASTERIZER_STATE_CREATION_FAILED:
        return "Error creating rasterizer state object.";
    case D3D11ERROR_SAMPLER_CREATION_FAILED:
        return "Error creating sampler object.";
    case D3D11ERROR_INPUT_LAYOUT_CREATION_FAILED:
        return "Error creating input layout object.";
    case D3D11ERROR_DISPLAY_SWAP_CHAIN_FAILED:
        return "Error displaying swap chain.";
    case D3D11ERROR_GET_BUFFER_FROM_SWAP_CHAIN_FAILED:
        return "Error obtaining buffer from swap chain.";
    case D3D11ERROR_TEXTURE1D_LOCK_FAILED:
        return "Error locking 1D texture.";
    case D3D11ERROR_TEXTURE2D_LOCK_FAILED:
        return "Error locking 2D texture.";
    case D3D11ERROR_TEXTURE3D_LOCK_FAILED:
        return "Error locking 3D texture.";
    case D3D11ERROR_UNSUPPORTED_RESOURCE_LOCK_FAILED:
        return "Error locking unsupported resource.";
    case D3D11ERROR_SHADER_CONSTANT_MAPPING_FAILED:
        return "Error mapping shader constants.";
    case D3D11ERROR_SHADER_MISSING:
        return "Error due to missing shader.";
    default:
        return "Missing error text.";
    }
}

//------------------------------------------------------------------------------------------------
