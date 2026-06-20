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
#include "NiBool.h"
#include "NiStencilProperty.h"
#include <NiBinaryStream.h>
#include <NiStream.h>

NiImplementRTTI(NiStencilProperty, NiProperty, NiTypeMask::NiStencilProperty);

NiStencilPropertyPtr NiStencilProperty::ms_spDefault;

//--------------------------------------------------------------------------------------------------
// cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiStencilProperty);
//--------------------------------------------------------------------------------------------------
void NiStencilProperty::CopyMembers(NiStencilProperty* pkDest,
    NiCloningProcess& kCloning)
{
    NiProperty::CopyMembers(pkDest, kCloning);

    pkDest->m_uFlags = m_uFlags;

    pkDest->m_uiRef = m_uiRef;
    pkDest->m_uiMask = m_uiMask;
    pkDest->m_uiWriteMask = m_uiWriteMask;
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiStencilProperty);
//--------------------------------------------------------------------------------------------------
void NiStencilProperty::LoadBinary(NiStream& kStream)
{
    NiProperty::LoadBinary(kStream);

    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 2))
    {
        // In 20.1.0.2 and later, these values are located in the flags.
        NiBool bEnable;
        NiStreamLoadBinary(kStream, bEnable);
        SetStencilOn(bEnable != 0);

        TestFunc eTest;
        NiStreamLoadEnum(kStream, eTest);
        SetStencilFunction(eTest);

        NiStreamLoadBinary(kStream, m_uiRef);
        NiStreamLoadBinary(kStream, m_uiMask);

        Action eAct;
        NiStreamLoadEnum(kStream, eAct);
        SetStencilFailAction(eAct);

        NiStreamLoadEnum(kStream, eAct);
        SetStencilPassZFailAction(eAct);

        NiStreamLoadEnum(kStream, eAct);
        SetStencilPassAction(eAct);

        DrawMode eDrawMode;
        NiStreamLoadEnum(kStream, eDrawMode);
        SetDrawMode(eDrawMode);
        m_uiWriteMask = m_uiMask;
    }
    else if (kStream.GetFileVersion() < NiStream::GetVersion(30, 3, 0, 5))
    {
        NiStreamLoadBinary(kStream, m_uFlags);

        NiStreamLoadBinary(kStream, m_uiRef);
        NiStreamLoadBinary(kStream, m_uiMask);
        m_uiWriteMask = m_uiMask;
    }
    else
    {
        NiStreamLoadBinary(kStream, m_uFlags);
        NiStreamLoadBinary(kStream, m_uiRef);
        NiStreamLoadBinary(kStream, m_uiMask);
        NiStreamLoadBinary(kStream, m_uiWriteMask);
    }
}
//--------------------------------------------------------------------------------------------------
void NiStencilProperty::LinkObject(NiStream& kStream)
{
    NiProperty::LinkObject(kStream);
}
//--------------------------------------------------------------------------------------------------
bool NiStencilProperty::RegisterStreamables(NiStream& kStream)
{
    return NiProperty::RegisterStreamables(kStream);
}
//--------------------------------------------------------------------------------------------------
void NiStencilProperty::SaveBinary(NiStream& kStream)
{
    NiProperty::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uFlags);
    NiStreamSaveBinary(kStream, m_uiRef);
    NiStreamSaveBinary(kStream, m_uiMask);
    NiStreamSaveBinary(kStream, m_uiWriteMask);
}
//--------------------------------------------------------------------------------------------------
bool NiStencilProperty::IsEqual(NiObject* pkObject)
{
    if (!NiProperty::IsEqual(pkObject))
        return false;

    NiStencilProperty* pkStencil = (NiStencilProperty*) pkObject;

    if ((pkStencil->m_uFlags != m_uFlags) ||
        (pkStencil->m_uiRef != m_uiRef) ||
        (pkStencil->m_uiMask != m_uiMask) ||
        (pkStencil->m_uiWriteMask != m_uiWriteMask))
    {
        return false;
    }

    return true;
}
//--------------------------------------------------------------------------------------------------
char* NiStencilProperty::GetViewerString(const char* pcPrefix,
    TestFunc eFunc)
{
    size_t stLen = strlen(pcPrefix) + 22;
    char* pcString = NiAlloc(char, stLen);

    switch (eFunc)
    {
    case TEST_NEVER:
        NiSprintf(pcString, stLen, "%s = TEST_NEVER", pcPrefix);
        break;
    case TEST_LESS:
        NiSprintf(pcString, stLen, "%s = TEST_LESS", pcPrefix);
        break;
    case TEST_EQUAL:
        NiSprintf(pcString, stLen, "%s = TEST_EQUAL", pcPrefix);
        break;
    case TEST_LESSEQUAL:
        NiSprintf(pcString, stLen, "%s = TEST_LESSEQUAL", pcPrefix);
        break;
    case TEST_GREATER:
       NiSprintf(pcString, stLen, "%s = TEST_GREATER", pcPrefix);
        break;
    case TEST_NOTEQUAL:
        NiSprintf(pcString, stLen, "%s = TEST_NOTEQUAL", pcPrefix);
        break;
    case TEST_GREATEREQUAL:
        NiSprintf(pcString, stLen, "%s = TEST_GREATEREQUAL", pcPrefix);
        break;
    case TEST_ALWAYS:
        NiSprintf(pcString, stLen, "%s = TEST_ALWAYS", pcPrefix);
        break;
    default:
        NiSprintf(pcString, stLen, "%s = UNKNOWN!!!", pcPrefix);
        break;
    }

    return pcString;
}
//--------------------------------------------------------------------------------------------------
char* NiStencilProperty::GetViewerString(const char* pcPrefix,
    Action eAct)
{
    size_t stLen = strlen(pcPrefix) + 22;
    char* pcString = NiAlloc(char, stLen);

    switch (eAct)
    {
    case ACTION_KEEP:
        NiSprintf(pcString, stLen, "%s = ACTION_KEEP", pcPrefix);
        break;
    case ACTION_ZERO:
        NiSprintf(pcString, stLen, "%s = ACTION_ZERO", pcPrefix);
        break;
    case ACTION_REPLACE:
        NiSprintf(pcString, stLen, "%s = ACTION_REPLACE", pcPrefix);
        break;
    case ACTION_INCREMENT:
        NiSprintf(pcString, stLen, "%s = ACTION_INCREMENT", pcPrefix);
        break;
    case ACTION_DECREMENT:
        NiSprintf(pcString, stLen, "%s = ACTION_DECREMENT", pcPrefix);
        break;
    case ACTION_INVERT:
        NiSprintf(pcString, stLen, "%s = ACTION_INVERT", pcPrefix);
        break;
    default:
        NiSprintf(pcString, stLen, "%s = UNKNOWN!!!", pcPrefix);
        break;
    }

    return pcString;
}
//--------------------------------------------------------------------------------------------------
char* NiStencilProperty::GetViewerString(const char* pcPrefix,
    DrawMode eMode)
{
    size_t stLen = strlen(pcPrefix) + 22;
    char* pcString = NiAlloc(char, stLen);

    switch (eMode)
    {
    case DRAW_CCW_OR_BOTH:
        NiSprintf(pcString, stLen, "%s = DRAW_CCW_OR_BOTH", pcPrefix);
        break;
    case DRAW_CCW:
        NiSprintf(pcString, stLen, "%s = DRAW_CCW", pcPrefix);
        break;
    case DRAW_CW:
        NiSprintf(pcString, stLen, "%s = DRAW_CW", pcPrefix);
        break;
    case DRAW_BOTH:
        NiSprintf(pcString, stLen, "%s = DRAW_BOTH", pcPrefix);
        break;
    default:
        NiSprintf(pcString, stLen, "%s = UNKNOWN!!!", pcPrefix);
        break;
    }

    return pcString;
}
//--------------------------------------------------------------------------------------------------
void NiStencilProperty::GetViewerStrings(NiViewerStringsArray* pcStrings)
{
    NiProperty::GetViewerStrings(pcStrings);

    pcStrings->Add(NiGetViewerString(NiStencilProperty::ms_RTTI.GetName()));

    pcStrings->Add(NiGetViewerString("Enabled", GetStencilOn()));
    pcStrings->Add(NiGetViewerString("m_uiRef", GetStencilReference()));
    pcStrings->Add(NiGetViewerString("m_uiMask", GetStencilMask()));
    pcStrings->Add(NiGetViewerString("m_uiWriteMask", GetStencilWriteMask()));
    pcStrings->Add(GetViewerString("Test Funtion", GetStencilFunction()));
    pcStrings->Add(GetViewerString("Test Fail Action",
        GetStencilFailAction()));
    pcStrings->Add(GetViewerString("Depth Fail Action",
        GetStencilPassZFailAction()));
    pcStrings->Add(GetViewerString("Test Pass Action",
        GetStencilPassAction()));
    pcStrings->Add(GetViewerString("Draw Mode", GetDrawMode()));
}
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// NiStaticDataManager
//--------------------------------------------------------------------------------------------------
void NiStencilProperty::_SDMInit()
{
    ms_spDefault = NiNew NiStencilProperty;
}
//--------------------------------------------------------------------------------------------------
void NiStencilProperty::_SDMShutdown()
{
#if defined(ENABLE_SDM_SHUTDOWN_ASSERT)
    EE_ASSERT(ms_spDefault->GetRefCount() == 1);
#endif
    ms_spDefault = NULL;
}
//--------------------------------------------------------------------------------------------------
