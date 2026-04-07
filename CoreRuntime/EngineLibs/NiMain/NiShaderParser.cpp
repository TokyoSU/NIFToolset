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

#include "NiShaderParser.h"

NiTPrimitiveSet<NiShaderParser::ParserCallbackInfo>*
    NiShaderParser::ms_pkParserCallbacks = NULL;

//--------------------------------------------------------------------------------------------------
void NiShaderParser::_SDMInit()
{
    ms_pkParserCallbacks = NiNew NiTPrimitiveSet<ParserCallbackInfo>;
}

//--------------------------------------------------------------------------------------------------
void NiShaderParser::_SDMShutdown()
{
    NiDelete ms_pkParserCallbacks;
}

//--------------------------------------------------------------------------------------------------
void NiShaderParser::AddParserCallback(const char* pcName,
    NISHADERPARSER_CLASSCREATIONCALLBACK pfnCallback)
{
    unsigned int uiEmpty = ms_pkParserCallbacks->GetSize();

    for (unsigned int i = 0; i < ms_pkParserCallbacks->GetSize(); i++)
    {
        if (!ms_pkParserCallbacks->GetAt(i).pcName &&
            !ms_pkParserCallbacks->GetAt(i).pfnCallback)
        {
            uiEmpty = i;
        }

        if (strcmp(ms_pkParserCallbacks->GetAt(i).pcName, pcName))
            continue;

        ms_pkParserCallbacks->GetAt(i).pfnCallback = pfnCallback;
        return;
    }

    if (uiEmpty >= ms_pkParserCallbacks->GetSize())
        uiEmpty = ms_pkParserCallbacks->Add(ParserCallbackInfo());

    ms_pkParserCallbacks->GetAt(uiEmpty).pcName = pcName;
    ms_pkParserCallbacks->GetAt(uiEmpty).pfnCallback = pfnCallback;
}

//--------------------------------------------------------------------------------------------------
void NiShaderParser::RemoveParserCallback(
    NISHADERPARSER_CLASSCREATIONCALLBACK pfnCallback)
{
    unsigned int ui = 0;
    bool bFound = false;
    for (ui = 0; ui < ms_pkParserCallbacks->GetSize(); ui++)
    {
        if (ms_pkParserCallbacks->GetAt(ui).pfnCallback == pfnCallback)
        {
            bFound = true;
            break;
        }
    }

    if (bFound)
        ms_pkParserCallbacks->RemoveAt(ui);
}

//--------------------------------------------------------------------------------------------------
unsigned int NiShaderParser::GetNumParserCallbacks()
{
    return ms_pkParserCallbacks->GetSize();
}

//--------------------------------------------------------------------------------------------------
const char* NiShaderParser::GetParserName(unsigned int uiIdx)
{
    return ms_pkParserCallbacks->GetAt(uiIdx).pcName;
}

//--------------------------------------------------------------------------------------------------
NiShaderParser::NISHADERPARSER_CLASSCREATIONCALLBACK
    NiShaderParser::GetParserCallback(unsigned int uiIdx)
{
    return ms_pkParserCallbacks->GetAt(uiIdx).pfnCallback;
}

//--------------------------------------------------------------------------------------------------