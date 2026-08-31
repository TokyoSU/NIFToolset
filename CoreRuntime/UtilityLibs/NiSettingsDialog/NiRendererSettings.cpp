// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.

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
const char* NiRendererSettings::ms_pcRendererID = "RendererID";
const char* NiRendererSettings::ms_pcFullscreen = "Fullscreen";
const char* NiRendererSettings::ms_pcVSync = "VSync";
const char* NiRendererSettings::ms_pcMultiThread = "MultiThread";
const char* NiRendererSettings::ms_pcRendererDialog = "RendererDialog";
const char* NiRendererSettings::ms_pcSaveSettings = "SaveSettings";

//--------------------------------------------------------------------------------------------------
NiRendererSettings::NiRendererSettings() :
    m_uiScreenWidth(0),
    m_uiScreenHeight(0),
    m_uiMinScreenWidth(640),
    m_uiMinScreenHeight(480),
    m_eRendererID(efd::SystemDesc::RENDERER_BGFX),
    m_bFullscreen(false),
    m_bVSync(true),
    m_bMultiThread(false),
    // The retired dialog enumerated D3D adapters and formats. bgfx owns that
    // backend selection, so the legacy dialog is disabled by default.
    m_bRendererDialog(false),
    m_bSaveSettings(false),
    m_eVertexProcessing(VERTEX_HARDWARE)
{
}

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

    // Consume the key for backwards-compatible files, but bgfx is the only
    // renderer built by the current project.
    unsigned int uiRendererID = static_cast<unsigned int>(m_eRendererID);
    ReadUInt(pcFileName, ms_pcRendererID, uiRendererID);
    m_eRendererID = efd::SystemDesc::RENDERER_BGFX;

    ReadBool(pcFileName, ms_pcFullscreen, m_bFullscreen);
    ReadBool(pcFileName, ms_pcVSync, m_bVSync);
    ReadBool(pcFileName, ms_pcMultiThread, m_bMultiThread);
    ReadBool(pcFileName, ms_pcRendererDialog, m_bRendererDialog);
    m_bSaveSettings = false;
}

//--------------------------------------------------------------------------------------------------
void NiRendererSettings::SaveSettings(const char* pcFileName)
{
    WriteUInt(pcFileName, ms_pcScreenWidth, m_uiScreenWidth);
    WriteUInt(pcFileName, ms_pcScreenHeight, m_uiScreenHeight);
    WriteUInt(pcFileName, ms_pcMinScreenWidth, m_uiMinScreenWidth);
    WriteUInt(pcFileName, ms_pcMinScreenHeight, m_uiMinScreenHeight);
    WriteUInt(pcFileName, ms_pcRendererID,
        static_cast<unsigned int>(efd::SystemDesc::RENDERER_BGFX));
    WriteBool(pcFileName, ms_pcFullscreen, m_bFullscreen);
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

    unsigned int uiRendererID = static_cast<unsigned int>(m_eRendererID);
    ReadConfig(pkSection, ms_pcRendererID, uiRendererID);
    m_eRendererID = efd::SystemDesc::RENDERER_BGFX;

    ReadConfig(pkSection, ms_pcFullscreen, m_bFullscreen);
    ReadConfig(pkSection, ms_pcVSync, m_bVSync);
    ReadConfig(pkSection, ms_pcMultiThread, m_bMultiThread);
    ReadConfig(pkSection, ms_pcRendererDialog, m_bRendererDialog);
    m_bSaveSettings = false;
}

//--------------------------------------------------------------------------------------------------
