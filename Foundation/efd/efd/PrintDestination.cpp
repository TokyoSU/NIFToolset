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

#include "efdPCH.h"

#include <stdio.h>
#include <stdarg.h>
#include <efd/PrintDestination.h>


using namespace efd;
//--------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(PrintDestination);

//--------------------------------------------------------------------------------------------------
PrintDestination::PrintDestination(const efd::utf8string& name,
                                    efd::Bool decorateMessage /*= false*/,
                                    efd::Bool fileInfoWithMsg /*= false*/,
                                    efd::Bool fileInfoWithAssert /*= true*/)
    : ILogDestination(name)
    , m_decorateMessage(decorateMessage)
    , m_fileWithMsg(fileInfoWithMsg)
    , m_fileWithAssert(fileInfoWithAssert)
{
}

//--------------------------------------------------------------------------------------------------
PrintDestination::~PrintDestination()
{
}

//--------------------------------------------------------------------------------------------------
efd::Bool PrintDestination::OnInit()
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void PrintDestination::BeginLog(
    efd::Bool assert,
    const char* timeStampMachine,
    efd::TimeType timeStampGame,
    const char* module,
    const char* level,
    const char* file,
    efd::SInt32 line)
{
    if (m_decorateMessage)
    {
        // |time|level|module|||msg|
        if ((!assert && !m_fileWithMsg) || (assert && !m_fileWithAssert))
        {
            printf(
                "%s|%s|%s|||",
                timeStampMachine,
                level,
                module);
        }
        else // |time|level|module|file|line|msg|
        {
            printf(
                "%s|%s|%s|%s|%d|",
                timeStampMachine,
                level,
                module,
                file,
                line);
        }
    }
    else
    {
        printf("%s\t", timeStampMachine);
    }

    EE_UNUSED_ARG(timeStampGame);
}

//--------------------------------------------------------------------------------------------------
void PrintDestination::ContinueLog(const char* pMsg)
{
    printf("%s", pMsg);
}

//--------------------------------------------------------------------------------------------------
void PrintDestination::EndLog()
{
    if (m_decorateMessage)
        printf("|\n");
    else
        printf("\n");
}


//--------------------------------------------------------------------------------------------------
void PrintDestination::Flush()
{
    fflush(stdout);
}
