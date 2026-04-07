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

#include "NiSettingsDialogPCH.h"
#include "NiRendererSettings.h"

#include <efd/IConfigManager.h>
#include <efd/IConfigSection.h>
#include <efd/ParseHelper.h>

using namespace efd;

const char* NiRendererSettings::ms_pcSectionName = "Renderer.Win32";
const char* NiRendererSettings::ms_pcScreenWidth = "ScreenWidth";
const char* NiRendererSettings::ms_pcScreenHeight = "ScreenHeight";
const char* NiRendererSettings::ms_pcMinScreenWidth = "MinScreenWidth";
const char* NiRendererSettings::ms_pcMinScreenHeight = "MinScreenHeight";
const char* NiRendererSettings::ms_pcVertexProcessing = "VertexProcessing";
const char* NiRendererSettings::ms_pcDX9RenderTargetMode = "DX9RenderTargetMode";
const char* NiRendererSettings::ms_pcDX9DepthSurfaceMode = "DX9DepthSurfaceMode";
const char* NiRendererSettings::ms_pcDX9FrameBufferMode = "DX9FrameBufferMode";
const char* NiRendererSettings::ms_pcD3D10OutputIdx = "D3D10OutputIdx";
const char* NiRendererSettings::ms_pcD3D10MultisampleCount = "D3D10MultisampleCount";
const char* NiRendererSettings::ms_pcD3D10MultisampleQuality = "D3D10MultisampleQuality";
const char* NiRendererSettings::ms_pcD3D10DSFormat = "D3D10DSFormat";
const char* NiRendererSettings::ms_pcD3D10RTFormat = "D3D10RTFormat";
const char* NiRendererSettings::ms_pcD3D10Renderer = "UseD3D10Renderer";
const char* NiRendererSettings::ms_pcD3D11OutputIdx = "D3D11OutputIdx";
const char* NiRendererSettings::ms_pcD3D11MultisampleCount = "D3D11MultisampleCount";
const char* NiRendererSettings::ms_pcD3D11MultisampleQuality = "D3D11MultisampleQuality";
const char* NiRendererSettings::ms_pcD3D11DSFormat = "D3D11DSFormat";
const char* NiRendererSettings::ms_pcD3D11RTFormat = "D3D11RTFormat";
const char* NiRendererSettings::ms_pcD3D11FeatureLevel9_1 = "D3D11FeatureLevel9_1";
const char* NiRendererSettings::ms_pcD3D11FeatureLevel9_2 = "D3D11FeatureLevel9_2";
const char* NiRendererSettings::ms_pcD3D11FeatureLevel9_3 = "D3D11FeatureLevel9_3";
const char* NiRendererSettings::ms_pcD3D11FeatureLevel10_0 = "D3D11FeatureLevel10_0";
const char* NiRendererSettings::ms_pcD3D11FeatureLevel10_1 = "D3D11FeatureLevel10_1";
const char* NiRendererSettings::ms_pcD3D11FeatureLevel11_0 = "D3D11FeatureLevel11_0";
const char* NiRendererSettings::ms_pcRendererID = "RendererID";
const char* NiRendererSettings::ms_pcFullscreen = "Fullscreen";
const char* NiRendererSettings::ms_pcNVPerfHUD = "NVPerfHUD";
const char* NiRendererSettings::ms_pcRefRast = "RefRast";
const char* NiRendererSettings::ms_pcUse16Bit = "Use16Bit";
const char* NiRendererSettings::ms_pcPureDevice = "PureDevice";
const char* NiRendererSettings::ms_pcVSync = "VSync";
const char* NiRendererSettings::ms_pcMultiThread = "MultiThread";
const char* NiRendererSettings::ms_pcRendererDialog = "RendererDialog";
const char* NiRendererSettings::ms_pcSaveSettings = "SaveSettings";

//--------------------------------------------------------------------------------------------------
// Constructor - here all settings are initialized to default values
//--------------------------------------------------------------------------------------------------
NiRendererSettings::NiRendererSettings() :
    // Default settings. Saved settings and command line parameters
    // will override them.
    m_uiScreenWidth(0),
    m_uiScreenHeight(0),
    m_uiMinScreenWidth(640),
    m_uiMinScreenHeight(480),
    m_eRendererID(efd::SystemDesc::RENDERER_DX9),
    m_bFullscreen(false),
    m_bNVPerfHUD(false),
    m_bRefRast(false),
    m_bUse16Bit(false),
    m_bPureDevice(true),
    m_bVSync(false),
    m_bStencil(false),
    m_bMultiThread(false),
    m_bRendererDialog(true),
    m_bSaveSettings(false),
    m_uiAdapterIdx(0),
    m_uiNVPerfHUDAdapterIdx(0),
    m_eVertexProcessing(VERTEX_HARDWARE),
    m_eDX9RTFormat(NiDX9Renderer::FBFMT_X8R8G8B8),
    m_eDX9DSFormat(NiDX9Renderer::DSFMT_D24S8),
    m_eDX9FBFormat(NiDX9Renderer::FBMODE_DEFAULT),
    m_uiD3D10OutputIdx(0),
    m_uiD3D10MultisampleCount(1),
    m_uiD3D10MultisampleQuality(0),
    m_eD3D10DSFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),
    m_eD3D10RTFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
    m_uiD3D11OutputIdx(0),
    m_uiD3D11MultisampleCount(1),
    m_uiD3D11MultisampleQuality(0),
    m_eD3D11DSFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),
    m_eD3D11RTFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
    m_bD3D11FeatureLevel9_1(true),
    m_bD3D11FeatureLevel9_2(true),
    m_bD3D11FeatureLevel9_3(true),
    m_bD3D11FeatureLevel10_0(true),
    m_bD3D11FeatureLevel10_1(true),
    m_bD3D11FeatureLevel11_0(true)
{

}

//--------------------------------------------------------------------------------------------------
// Settings loading / saving functions
//--------------------------------------------------------------------------------------------------
bool NiRendererSettings::ReadConfig(
    const ISection* pkSection,
    const char* pcValueName,
    unsigned int& uiVal)
{
    EE_ASSERT(pkSection && pcValueName);

    const efd::utf8string& kValue = pkSection->FindValue(pcValueName);

    if (kValue.empty())
        return false;

    UInt32 uiTemp = 0;
    if (!efd::ParseHelper<efd::UInt32>::FromString(kValue, uiTemp))
        return false;

    uiVal = uiTemp;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiRendererSettings::ReadConfig(
    const ISection* pkSection,
    const char* pcValueName,
    bool& bVal)
{
    EE_ASSERT(pkSection && pcValueName);

    const efd::utf8string& kValue = pkSection->FindValue(pcValueName);

    if (kValue.empty())
        return false;

    bool bTemp = false;

    if (!efd::ParseHelper<efd::Bool>::FromString(kValue, bTemp))
        return false;

    bVal = bTemp;
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiRendererSettings::LoadSettings(const char* pcFileName)
{
    ReadUInt(pcFileName, ms_pcScreenWidth, m_uiScreenWidth);
    ReadUInt(pcFileName, ms_pcScreenHeight, m_uiScreenHeight);
    ReadUInt(pcFileName, ms_pcMinScreenWidth, m_uiMinScreenWidth);
    ReadUInt(pcFileName, ms_pcMinScreenHeight, m_uiMinScreenHeight);
    ReadUInt(pcFileName, ms_pcVertexProcessing, (unsigned int&)(m_eVertexProcessing));

    ReadUInt(pcFileName, ms_pcDX9RenderTargetMode, (unsigned int&)(m_eDX9RTFormat));
    ReadUInt(pcFileName, ms_pcDX9DepthSurfaceMode, (unsigned int&)(m_eDX9DSFormat));
    ReadUInt(pcFileName, ms_pcDX9FrameBufferMode, (unsigned int&)(m_eDX9FBFormat));
    
    ReadUInt(pcFileName, ms_pcD3D10OutputIdx, m_uiD3D10OutputIdx);
    ReadUInt(pcFileName, ms_pcD3D10MultisampleCount, m_uiD3D10MultisampleCount);
    ReadUInt(pcFileName, ms_pcD3D10MultisampleQuality, m_uiD3D10MultisampleQuality);
    ReadUInt(pcFileName, ms_pcD3D10DSFormat, (unsigned int&)(m_eD3D10DSFormat));
    ReadUInt(pcFileName, ms_pcD3D10RTFormat, (unsigned int&)(m_eD3D10RTFormat));
    
    ReadUInt(pcFileName, ms_pcD3D11OutputIdx, m_uiD3D11OutputIdx);
    ReadUInt(pcFileName, ms_pcD3D11MultisampleCount, m_uiD3D11MultisampleCount);
    ReadUInt(pcFileName, ms_pcD3D11MultisampleQuality, m_uiD3D11MultisampleQuality);
    ReadUInt(pcFileName, ms_pcD3D11DSFormat, (unsigned int&)(m_eD3D11DSFormat));
    ReadUInt(pcFileName, ms_pcD3D11RTFormat, (unsigned int&)(m_eD3D11RTFormat));
    ReadBool(pcFileName, ms_pcD3D11FeatureLevel9_1, m_bD3D11FeatureLevel9_1);
    ReadBool(pcFileName, ms_pcD3D11FeatureLevel9_2, m_bD3D11FeatureLevel9_2);
    ReadBool(pcFileName, ms_pcD3D11FeatureLevel9_3, m_bD3D11FeatureLevel9_3);
    ReadBool(pcFileName, ms_pcD3D11FeatureLevel10_0, m_bD3D11FeatureLevel10_0);
    ReadBool(pcFileName, ms_pcD3D11FeatureLevel10_1, m_bD3D11FeatureLevel10_1);
    ReadBool(pcFileName, ms_pcD3D11FeatureLevel11_0, m_bD3D11FeatureLevel11_0);

    // Support deprecated key in old settings files
    // Value from old key will only be used if new key not found
    bool bD3D10Renderer = false;
    ReadBool(pcFileName, ms_pcD3D10Renderer, bD3D10Renderer);
    
    efd::SystemDesc::RendererID eInitialValue = m_eRendererID;
    m_eRendererID = efd::SystemDesc::RENDERER_COUNT;
    ReadUInt(pcFileName, ms_pcRendererID, (unsigned int&)(m_eRendererID));
    if (m_eRendererID == efd::SystemDesc::RENDERER_COUNT)
    {
        // ms_pcRendererID key not in settings file
        // If D3D10Renderer key has been set, use that; otherwise, use
        //   initial default value for renderer ID
        if (bD3D10Renderer)
            m_eRendererID = efd::SystemDesc::RENDERER_D3D10;
        else
            m_eRendererID = eInitialValue;
    }

    ReadBool(pcFileName, ms_pcFullscreen, m_bFullscreen);
    ReadBool(pcFileName, ms_pcNVPerfHUD, m_bNVPerfHUD);
    ReadBool(pcFileName, ms_pcRefRast, m_bRefRast);
    ReadBool(pcFileName, ms_pcUse16Bit, m_bUse16Bit);
    ReadBool(pcFileName, ms_pcPureDevice, m_bPureDevice);
    ReadBool(pcFileName, ms_pcVSync, m_bVSync);
    ReadBool(pcFileName, ms_pcMultiThread, m_bMultiThread);
    ReadBool(pcFileName, ms_pcRendererDialog, m_bRendererDialog);
    // Don't read SaveSettings from file - it always defaults to false.
    m_bSaveSettings = false;
}

//--------------------------------------------------------------------------------------------------
void NiRendererSettings::SaveSettings(const char* pcFileName)
{
    WriteUInt(pcFileName, ms_pcScreenWidth, m_uiScreenWidth);
    WriteUInt(pcFileName, ms_pcScreenHeight, m_uiScreenHeight);
    WriteUInt(pcFileName, ms_pcMinScreenWidth, m_uiMinScreenWidth);
    WriteUInt(pcFileName, ms_pcMinScreenHeight, m_uiMinScreenHeight);
    WriteUInt(pcFileName, ms_pcVertexProcessing, m_eVertexProcessing);
    
    WriteUInt(pcFileName, ms_pcDX9RenderTargetMode, m_eDX9RTFormat);
    WriteUInt(pcFileName, ms_pcDX9DepthSurfaceMode, m_eDX9DSFormat);
    WriteUInt(pcFileName, ms_pcDX9FrameBufferMode, m_eDX9FBFormat);
    
    WriteUInt(pcFileName, ms_pcD3D10OutputIdx, m_uiD3D10OutputIdx);
    WriteUInt(pcFileName, ms_pcD3D10MultisampleCount, m_uiD3D10MultisampleCount);
    WriteUInt(pcFileName, ms_pcD3D10MultisampleQuality, m_uiD3D10MultisampleQuality);
    WriteUInt(pcFileName, ms_pcD3D10DSFormat, m_eD3D10DSFormat);
    WriteUInt(pcFileName, ms_pcD3D10RTFormat, m_eD3D10RTFormat);
    
    WriteUInt(pcFileName, ms_pcD3D11OutputIdx, m_uiD3D11OutputIdx);
    WriteUInt(pcFileName, ms_pcD3D11MultisampleCount, m_uiD3D11MultisampleCount);
    WriteUInt(pcFileName, ms_pcD3D11MultisampleQuality, m_uiD3D11MultisampleQuality);
    WriteUInt(pcFileName, ms_pcD3D11DSFormat, m_eD3D11DSFormat);
    WriteUInt(pcFileName, ms_pcD3D11RTFormat, m_eD3D11RTFormat);
    WriteBool(pcFileName, ms_pcD3D11FeatureLevel9_1, m_bD3D11FeatureLevel9_1);
    WriteBool(pcFileName, ms_pcD3D11FeatureLevel9_2, m_bD3D11FeatureLevel9_2);
    WriteBool(pcFileName, ms_pcD3D11FeatureLevel9_3, m_bD3D11FeatureLevel9_3);
    WriteBool(pcFileName, ms_pcD3D11FeatureLevel10_0, m_bD3D11FeatureLevel10_0);
    WriteBool(pcFileName, ms_pcD3D11FeatureLevel10_1, m_bD3D11FeatureLevel10_1);
    WriteBool(pcFileName, ms_pcD3D11FeatureLevel11_0, m_bD3D11FeatureLevel11_0);

    WriteUInt(pcFileName, ms_pcRendererID, m_eRendererID);
    WriteBool(pcFileName, ms_pcFullscreen, m_bFullscreen);
    WriteBool(pcFileName, ms_pcNVPerfHUD, m_bNVPerfHUD);
    WriteBool(pcFileName, ms_pcRefRast, m_bRefRast);
    WriteBool(pcFileName, ms_pcUse16Bit, m_bUse16Bit);
    WriteBool(pcFileName, ms_pcPureDevice, m_bPureDevice);
    WriteBool(pcFileName, ms_pcVSync, m_bVSync);
    WriteBool(pcFileName, ms_pcMultiThread, m_bMultiThread);
    WriteBool(pcFileName, ms_pcRendererDialog, m_bRendererDialog);
}

//--------------------------------------------------------------------------------------------------
void NiRendererSettings::LoadFromConfigManager(IConfigManager* pkConfigManager)
{
    EE_ASSERT(pkConfigManager);
    if (!pkConfigManager)
        return;

    const ISection* pkSection =
        pkConfigManager->GetConfiguration()->FindSection(ms_pcSectionName);
    if (!pkSection)
        return;

    ReadConfig(pkSection, ms_pcScreenWidth, m_uiScreenWidth);
    ReadConfig(pkSection, ms_pcScreenHeight, m_uiScreenHeight);
    ReadConfig(pkSection, ms_pcMinScreenWidth, m_uiMinScreenWidth);
    ReadConfig(pkSection, ms_pcMinScreenHeight, m_uiMinScreenHeight);
    ReadConfig(pkSection, ms_pcVertexProcessing, (unsigned int&)(m_eVertexProcessing));
    
    ReadConfig(pkSection, ms_pcDX9RenderTargetMode, (unsigned int&)(m_eDX9RTFormat));
    ReadConfig(pkSection, ms_pcDX9DepthSurfaceMode, (unsigned int&)(m_eDX9DSFormat));
    ReadConfig(pkSection, ms_pcDX9FrameBufferMode, (unsigned int&)(m_eDX9FBFormat));
    
    ReadConfig(pkSection, ms_pcD3D10OutputIdx, m_uiD3D10OutputIdx);
    ReadConfig(pkSection, ms_pcD3D10MultisampleCount, m_uiD3D10MultisampleCount);
    ReadConfig(pkSection, ms_pcD3D10MultisampleQuality, m_uiD3D10MultisampleQuality);
    ReadConfig(pkSection, ms_pcD3D10DSFormat, (unsigned int&)(m_eD3D10DSFormat));
    ReadConfig(pkSection, ms_pcD3D10RTFormat, (unsigned int&)(m_eD3D10RTFormat));
    
    ReadConfig(pkSection, ms_pcD3D11OutputIdx, m_uiD3D11OutputIdx);
    ReadConfig(pkSection, ms_pcD3D11MultisampleCount, m_uiD3D11MultisampleCount);
    ReadConfig(pkSection, ms_pcD3D11MultisampleQuality, m_uiD3D11MultisampleQuality);
    ReadConfig(pkSection, ms_pcD3D11DSFormat, (unsigned int&)(m_eD3D11DSFormat));
    ReadConfig(pkSection, ms_pcD3D11RTFormat, (unsigned int&)(m_eD3D11RTFormat));
    ReadConfig(pkSection, ms_pcD3D11FeatureLevel9_1, m_bD3D11FeatureLevel9_1);
    ReadConfig(pkSection, ms_pcD3D11FeatureLevel9_2, m_bD3D11FeatureLevel9_2);
    ReadConfig(pkSection, ms_pcD3D11FeatureLevel9_3, m_bD3D11FeatureLevel9_3);
    ReadConfig(pkSection, ms_pcD3D11FeatureLevel10_0, m_bD3D11FeatureLevel10_0);
    ReadConfig(pkSection, ms_pcD3D11FeatureLevel10_1, m_bD3D11FeatureLevel10_1);
    ReadConfig(pkSection, ms_pcD3D11FeatureLevel11_0, m_bD3D11FeatureLevel11_0);

    // Support deprecated key in old settings files
    // Value from old key will only be used if new key not found
    bool bD3D10Renderer = false;
    ReadConfig(pkSection, ms_pcD3D10Renderer, bD3D10Renderer);

    if (!ReadConfig(pkSection, ms_pcRendererID, (unsigned int&)(m_eRendererID)))
    {
        // ms_pcRendererID key not in config section
        // If D3D10Renderer key has been set, use that; otherwise, use
        //   initial default value for renderer ID
        if (bD3D10Renderer)
        {
            m_eRendererID = efd::SystemDesc::RENDERER_D3D10;
        }
    }

    ReadConfig(pkSection, ms_pcFullscreen, m_bFullscreen);
    ReadConfig(pkSection, ms_pcNVPerfHUD, m_bNVPerfHUD);
    ReadConfig(pkSection, ms_pcRefRast, m_bRefRast);
    ReadConfig(pkSection, ms_pcUse16Bit, m_bUse16Bit);
    ReadConfig(pkSection, ms_pcPureDevice, m_bPureDevice);
    ReadConfig(pkSection, ms_pcVSync, m_bVSync);
    ReadConfig(pkSection, ms_pcMultiThread, m_bMultiThread);
    ReadConfig(pkSection, ms_pcRendererDialog, m_bRendererDialog);
    // Don't read SaveSettings from file - it always defaults to false.
    m_bSaveSettings = false;
}

//--------------------------------------------------------------------------------------------------
