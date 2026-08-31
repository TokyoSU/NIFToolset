// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.

#pragma once
#ifndef NIRENDERERSETTINGS_H
#define NIRENDERERSETTINGS_H

#include "NiSettingsDialogLibType.h"
#include <NiMemObject.h>
#include <NiSystem.h>
#include <efd/SystemDesc.h>

namespace efd
{
class IConfigManager;
class ISection;
};

/// Portable renderer settings shared by the settings UI and GameFramework.
///
/// The original structure exposed DX9/DX10/DX11 device-specific fields. bgfx
/// owns API/device selection internally, so only backend-independent settings
/// remain here.
class NISETTINGSDIALOG_ENTRY NiRendererSettings : public NiMemObject
{
public:
    NiRendererSettings();

    // Kept for NiBaseRendererDesc compatibility. bgfx does not expose a
    // hardware/software vertex-processing choice.
    enum VertexProcessing
    {
        VERTEX_UNSUPPORTED,
        VERTEX_HARDWARE,
        VERTEX_MIXED,
        VERTEX_SOFTWARE
    };

    unsigned int m_uiScreenWidth;
    unsigned int m_uiScreenHeight;
    unsigned int m_uiMinScreenWidth;
    unsigned int m_uiMinScreenHeight;

    efd::SystemDesc::RendererID m_eRendererID;
    bool m_bFullscreen;
    bool m_bVSync;
    bool m_bMultiThread;
    bool m_bRendererDialog;
    bool m_bSaveSettings;

    VertexProcessing m_eVertexProcessing;

    void LoadSettings(const char* pcFileName);
    void LoadFromConfigManager(efd::IConfigManager* pkConfigManager);
    void SaveSettings(const char* pcFileName);

protected:
    static bool ReadConfig(
        const efd::ISection* pkSection,
        const char* pcValueName,
        unsigned int& uiVal);
    static bool ReadConfig(
        const efd::ISection* pkSection,
        const char* pcValueName,
        bool& bVal);

    static void ReadUInt(const char* pcFileName, const char* pcName,
        unsigned int& uiVal);
    static void ReadBool(const char* pcFileName, const char* pcName, bool& bVal);
    static void WriteUInt(const char* pcFileName, const char* pcName,
        unsigned int uiVal);
    static void WriteBool(const char* pcFileName, const char* pcName, bool bVal);

    static const char* ms_pcSectionName;
    static const char* ms_pcScreenWidth;
    static const char* ms_pcScreenHeight;
    static const char* ms_pcMinScreenWidth;
    static const char* ms_pcMinScreenHeight;
    static const char* ms_pcRendererID;
    static const char* ms_pcFullscreen;
    static const char* ms_pcVSync;
    static const char* ms_pcMultiThread;
    static const char* ms_pcRendererDialog;
    static const char* ms_pcSaveSettings;
};

#include "NiRendererSettings.inl"

#endif // NIRENDERERSETTINGS_H
