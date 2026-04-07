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

#include <efd/Metrics.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

#if defined(EE_ENABLE_METRICS_LOGGING) && !defined(EE_DISABLE_LOGGING)

//------------------------------------------------------------------------------------------------
#define EE_ModToStr(module) (efd::GetLogger()->GetModuleName(module).toupper().c_str())

//------------------------------------------------------------------------------------------------
void efd::LogMetric(efd::UInt16 module, const char* metric, efd::SInt32 data)
{
    EE_LOG(efd::kMetrics, efd::ILogger::kLVL0, ("%s.%s = %d", EE_ModToStr(module), metric, data));
}
//------------------------------------------------------------------------------------------------
void efd::LogMetric(efd::UInt16 module, const char* metric, efd::UInt32 data)
{
    EE_LOG(efd::kMetrics, efd::ILogger::kLVL0, ("%s.%s = %u", EE_ModToStr(module), metric, data));
}
//------------------------------------------------------------------------------------------------
void efd::LogMetric(efd::UInt16 module, const char* metric, efd::SInt64 data)
{
    EE_LOG(efd::kMetrics, efd::ILogger::kLVL0, ("%s.%s = %ll", EE_ModToStr(module), metric, data));
}
//------------------------------------------------------------------------------------------------
void efd::LogMetric(efd::UInt16 module, const char* metric, efd::UInt64 data)
{
    EE_LOG(efd::kMetrics, efd::ILogger::kLVL0, ("%s.%s = %ull", EE_ModToStr(module), metric, data));
}
//------------------------------------------------------------------------------------------------
void efd::LogMetric(efd::UInt16 module, const char* metric, efd::Float64 data)
{
    EE_LOG(efd::kMetrics, efd::ILogger::kLVL0, ("%s.%s = %g", EE_ModToStr(module), metric, data));
}

#undef EE_ModToStr

#endif // defined(EE_LOG_METRICS) && !defined(EE_DISABLE_LOGGING)

