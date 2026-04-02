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

//-------------------------------------------------------------------------------------------------

#include "efdPCH.h"

#include <efd/SystemDesc.h>
#include <efd/Asserts.h>
#include <efd/StringUtilities.h>

using namespace efd;
//-------------------------------------------------------------------------------------------------
SystemDesc* SystemDesc::ms_SystemDesc = NULL;
//-------------------------------------------------------------------------------------------------
const char* SystemDesc::GetRendererString(const SystemDesc::RendererID eRenderer)
{
    switch (eRenderer)
    {
    case SystemDesc::RENDERER_PS3:
        return "PS3";
    case SystemDesc::RENDERER_XBOX360:
        return "XBOX360";
    case SystemDesc::RENDERER_DX9:
        return "DX9";
    case SystemDesc::RENDERER_D3D10:
        return "D3D10";
    case SystemDesc::RENDERER_D3D11:
        return "D3D11";
    case SystemDesc::RENDERER_GENERIC:
        return "Generic";
    default:
        break;
    }

    return "Unknown";
}
//---------------------------------------------------------------------------
SystemDesc::RendererID SystemDesc::GetRendererID(const char* pcRendererName)
{
    for (unsigned int ui = 0; ui < RENDERER_COUNT; ui++)
    {
        if (efd::Stricmp(pcRendererName, GetRendererString((SystemDesc::RendererID)ui)) == 0)
        {
            return (SystemDesc::RendererID)ui;
        }

        // This condition attempts to handle the disparity between old Gamebryo naming ("XENON")
        // and new Emergent naming ("XBOX360") for the Xbox 360 console until Gamebryo has fully
        // adopted the new Emergent naming.  Once "XENON" is no longer used throughout Gamebryo
        // (e.g., for mesh profile renderer specification), the condition below may be removed.
        if ((efd::Stricmp(pcRendererName, "XENON") == 0) &&
            (efd::Stricmp(GetRendererString((SystemDesc::RendererID)ui), "XBOX360") == 0))
        {
            return (SystemDesc::RendererID)ui;
        }
    }

    return SystemDesc::RENDERER_GENERIC;
}
//---------------------------------------------------------------------------
const char* SystemDesc::GetPlatformString(efd::SystemDesc::PlatformID e)
{
    switch (e)
    {
    case PLATFORM_WIN32:
        return "WIN32";
    case PLATFORM_XBOX360:
        return "XBOX360";
    case PLATFORM_PS3:
        return "PS3";
    case PLATFORM_LINUX:
        return "LINUX";
    default:
        return "Unknown";
    }
}
//---------------------------------------------------------------------------
SystemDesc::PlatformID SystemDesc::GetPlatformID(const char* pString)
{
    for (unsigned int ui = 0; ui < PLATFORM_COUNT; ui++)
    {
        if (efd::Stricmp(pString, GetPlatformString((SystemDesc::PlatformID)ui)) == 0)
        {
            return (SystemDesc::PlatformID)ui;
        }
    }
    return (SystemDesc::PlatformID)-1;
}
//---------------------------------------------------------------------------
bool SystemDesc::IsPlatformLittleEndian(SystemDesc::PlatformID ePlatform)
{
    switch (ePlatform)
    {
    case PLATFORM_WIN32:
    case PLATFORM_LINUX:
        return true;
    case PLATFORM_XBOX360:
    case PLATFORM_PS3:
        return false;
    default:
        EE_FAIL("Unknown platform");
        return true;
    }
}
//---------------------------------------------------------------------------
bool SystemDesc::IsRendererLittleEndian(SystemDesc::RendererID eRenderer)
{
    switch (eRenderer)
    {
    case RENDERER_XBOX360:
    case RENDERER_PS3:
        return false;
    case RENDERER_DX9:
    case RENDERER_D3D10:
    case RENDERER_D3D11:
    case RENDERER_GENERIC:
        return true;
    default:
        EE_FAIL("Unknown renderer");
        return true;
    }
}
//---------------------------------------------------------------------------
bool SystemDesc::GetToolModeRendererIsLittleEndian() const
{
    return IsRendererLittleEndian(m_ToolModeRendererID);
}
//---------------------------------------------------------------------------
SystemDesc::SystemDesc(const SystemDesc&)
{
    EE_FAIL("This method should never be called");
}
//---------------------------------------------------------------------------
