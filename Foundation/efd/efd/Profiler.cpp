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
#include "Profiler.h"



//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
// Demo Use 

//#define EE_PROFILER_DISABLE

#define EE_PROFILER_MODULE_FORCE_ENABLE
//#define EE_PROFILER_MODULE_FORCE_DISABLE

#include <efd/Profiler.h>

void A()
{

}

void B()
{

}

unsigned int C(efd::Profiler::FlagType, const char* )
{
    return 3;
}

unsigned int D(unsigned int )
{
    return 1;
}

EE_PROFILER_CONTEXT(Physics);

void ProfileThisBiach()
{
    efd::Profiler profiler;
    profiler._Initialize = &A;
    profiler._Shutdown = &B;
    profiler.Enter = &C;
    profiler.Leave = &D;
    efd::Profiler::Initialize(&profiler);

    EE_PROFILER_CONTEXT_ENABLE(Physics, true);
    EE_PROFILER_CONTEXT_ENABLE_ALL(true);

    {
        EE_PROFILER_ZONE(Physics, 0, "Do Something");
        //Something to be done
        efd::Sleep(1000);
    }

    {
        EE_PROFILER_ZONE(Physics, 0, "Do SomethingA");
        //Something to be done
        efd::Sleep(3);
    }

    efd::Profiler::Shutdown();
}
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------


using namespace efd;
//------------------------------------------------------------------------------------------------
Profiler Profiler::ms_profiler;
bool Profiler::ms_bInitialized = false;
ProfilerContext* ProfilerContext::ms_pHeadContext = NULL;
//------------------------------------------------------------------------------------------------
Profiler::Profiler()
{
    AssignDefaults();
}

//------------------------------------------------------------------------------------------------
void Profiler::AssignDefaults()
{
    m_pConfig = NULL;
    EE_PROFILER_API_ASSIGN_DEFAULT(*this, _Initialize);
    EE_PROFILER_API_ASSIGN_DEFAULT(*this, _Shutdown);
    EE_PROFILER_API_ASSIGN_DEFAULT(*this, _Tick);
    EE_PROFILER_API_ASSIGN_DEFAULT(*this, Enter);
    EE_PROFILER_API_ASSIGN_DEFAULT(*this, Leave);
    EE_PROFILER_API_ASSIGN_DEFAULT(*this, MemAlloc);
    EE_PROFILER_API_ASSIGN_DEFAULT(*this, MemFree);
}

//------------------------------------------------------------------------------------------------
void Profiler::Initialize(Profiler* pProfilerTable)
{
    if (ms_bInitialized)
        Shutdown();

    if (pProfilerTable)
    {
        ms_profiler = *pProfilerTable;
    }

    ms_profiler._Initialize();
    ms_bInitialized = true;
}

//------------------------------------------------------------------------------------------------
void Profiler::Shutdown()
{
    ms_profiler._Shutdown();
    ms_bInitialized = false;
}

//------------------------------------------------------------------------------------------------
void Profiler::Tick()
{
    if (ms_bInitialized)
        ms_profiler._Tick();
}

//------------------------------------------------------------------------------------------------
Profiler* Profiler::GetInstance()
{
    return &ms_profiler;
}

//------------------------------------------------------------------------------------------------
void* Profiler::GetConfig()
{
    return ms_profiler.m_pConfig;
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
ProfilerContext::ProfilerContext(const char* pName)
: m_pName(pName)
, m_pProfiler(NULL)
, m_pNextContext(NULL)
{
    RegisterContext(this);
}

//------------------------------------------------------------------------------------------------
ProfilerContext::~ProfilerContext()
{
    // Break the chain
    ms_pHeadContext = NULL;
}

//------------------------------------------------------------------------------------------------
ProfilerContext* ProfilerContext::FindContext(const char* pName)
{
    ProfilerContext* pCurrent = ms_pHeadContext;
    while (pCurrent)
    {
        if (strcmp(pName, pCurrent->m_pName) == 0)
            return pCurrent;

        pCurrent = pCurrent->m_pNextContext;
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
void ProfilerContext::SetEnabledAll(bool bEnable)
{
    ProfilerContext* pCurrent = ms_pHeadContext;
    while (pCurrent)
    {
        pCurrent->SetEnabled(bEnable);

        pCurrent = pCurrent->m_pNextContext;
    }
}

//------------------------------------------------------------------------------------------------
void ProfilerContext::SetEnabled(bool bEnable)
{
    if (bEnable)
        m_pProfiler = Profiler::GetInstance();
    else
        m_pProfiler = NULL;
}

//------------------------------------------------------------------------------------------------
const char* ProfilerContext::GetName() const
{
    return m_pName;
}

//------------------------------------------------------------------------------------------------
void ProfilerContext::RegisterContext(ProfilerContext* pContext)
{
    if (!ms_pHeadContext)
    {
        ms_pHeadContext = pContext;
    }
    else
    {
        ProfilerContext* pCurrent = ms_pHeadContext;
        while (pCurrent->m_pNextContext != NULL)
        {
            pCurrent = pCurrent->m_pNextContext;
        }

        pCurrent->m_pNextContext = pContext;
    }
}

//------------------------------------------------------------------------------------------------