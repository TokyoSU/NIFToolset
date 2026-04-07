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
#include <efd/SystemLogger.h>
#include <efd/ILogger.h>

using namespace efd;

//-------------------------------------------------------------------------------------------------
efd::ILoggerPtr SystemLogger::CreateSystemLogger()
{
#if !defined(EE_DISABLE_LOGGING)
    efd::LoggerPtr spLogger = EE_NEW Logger();
    efd::LoggerSingleton::Initialize(spLogger);
    return (Logger*)spLogger;
#else
    return NULL;
#endif //!defined(EE_DISABLE_LOGGING)
}
