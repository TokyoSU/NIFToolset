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
#include "NiSystemPCH.h"

#include "NiInitOptions.h"
#include "NiMemTracker.h"
#include "NiStandardAllocator.h"
#include "NiDebug.h"
#include "NiSystemSDM.h"

// Note: This declaration must be in a file other than NiSystemSDM.cpp, as
// it is repsonsible for using the code from NiSystemSDM.  Otherwise, the
// closed usage that doesn't touch the rest of the code base can get removed
// by the linker.
static NiSystemSDM NiSystemSDMObject;

//--------------------------------------------------------------------------------------------------
NiInitOptions::NiInitOptions()
{
#if defined (EE_PLATFORM_PS3)
    m_uiJobWorkloadCount = 14;
    m_uiTaskWorkloadCount = 2;
#else
    m_bParallelExecution = true;
    m_pfnDefineWorkerThreadCountFunc = NULL;
    m_pfnAssignDispatcherThreadAffinityFunc = NULL;
    m_pfnAssignWorkerThreadAffinityFunc = NULL;
#endif

}

//--------------------------------------------------------------------------------------------------
NiInitOptions::~NiInitOptions()
{
}

//--------------------------------------------------------------------------------------------------
#if !defined(EE_PLATFORM_PS3)
void NiInitOptions::SetFloodgateDefineWorkerThreadCountFunc(void* pfnCallback)
{
    EE_ASSERT(pfnCallback);
    m_pfnDefineWorkerThreadCountFunc = pfnCallback;
}

//--------------------------------------------------------------------------------------------------
const void* NiInitOptions::GetFloodgateDefineWorkerThreadCountFunc()
{
    return m_pfnDefineWorkerThreadCountFunc;
}

//--------------------------------------------------------------------------------------------------
void NiInitOptions::SetDispatchThreadAffinityFunc(void* pfnCallback)
{
    EE_ASSERT(pfnCallback);
    m_pfnAssignDispatcherThreadAffinityFunc = pfnCallback;
}

//--------------------------------------------------------------------------------------------------
const void* NiInitOptions::GetDispatchThreadAffinityFunc()
{
    return m_pfnAssignDispatcherThreadAffinityFunc;
}

//--------------------------------------------------------------------------------------------------
void NiInitOptions::SetWorkerThreadAffinityFunc(void* pfnCallback)
{
    EE_ASSERT(pfnCallback);
    m_pfnAssignWorkerThreadAffinityFunc = pfnCallback;
}

//--------------------------------------------------------------------------------------------------
const void* NiInitOptions::GetWorkerThreadAffinityFunc()
{
    return m_pfnAssignWorkerThreadAffinityFunc;
}

//--------------------------------------------------------------------------------------------------
void NiInitOptions::SetFloodgateParallelExecution(bool bParallelExecution)
{
    m_bParallelExecution = bParallelExecution;
}

//--------------------------------------------------------------------------------------------------
bool NiInitOptions::GetParallelExecution()
{
    return m_bParallelExecution;
}
#endif

//--------------------------------------------------------------------------------------------------
#if defined (EE_PLATFORM_PS3)

//--------------------------------------------------------------------------------------------------
NiUInt32 NiInitOptions::GetJobWorkloadCount() const
{
    return m_uiJobWorkloadCount;
}

//--------------------------------------------------------------------------------------------------
void NiInitOptions::SetJobWorkloadCount(NiUInt32 uiJobWorkloadCount)
{
    m_uiJobWorkloadCount = uiJobWorkloadCount;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiInitOptions::GetTaskWorkloadCount() const
{
    return m_uiTaskWorkloadCount;
}

//--------------------------------------------------------------------------------------------------
void NiInitOptions::SetTaskWorkloadCount(NiUInt32 uiTaskWorkloadCount)
{
    m_uiTaskWorkloadCount = uiTaskWorkloadCount;
}
#endif

//--------------------------------------------------------------------------------------------------

