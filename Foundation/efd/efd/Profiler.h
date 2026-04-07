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

#pragma once
#ifndef EE_PROFILER_H
#define EE_PROFILER_H

#include "Asserts.h"

namespace efd
{
//------------------------------------------------------------------------------------------------
// Disable warnings for this particular file
//
// Disable the warning "unreferenced formal parameter"
#pragma warning( push )
#pragma warning( disable : 4100 )

//------------------------------------------------------------------------------------------------
// Detect what to do with the profiler based on the build type
//
// Vaporize profiler code during shipping by default.

#ifndef EE_PROFILER_DISABLE
    #ifdef EE_CONFIG_SHIPPING
        #define EE_PROFILER_DISABLE
    #endif
#endif

//------------------------------------------------------------------------------------------------
// Detect what to do with the profiler based on this module (the file including this file)
//
// Individual modules can forcibly include their profiler code by manually
// declaring one of these macros before including this file.

#ifdef EE_PROFILER_DISABLE
    #ifdef EE_PROFILER_MODULE_FORCE_ENABLE
        #undef EE_PROFILER_DISABLE
    #endif
#else
    #ifdef EE_PROFILER_MODULE_FORCE_DISENABLE
        #define EE_PROFILER_DISABLE
    #endif
#endif

//------------------------------------------------------------------------------------------------
// Setup the vaporize macro
//
// Vaporized macros will vaporize into a default value when profiling is disabled.

#ifdef EE_PROFILER_DISABLED
    #define EE_PROFILER_VAPOR(command, vapor) vapor
#else
    #define EE_PROFILER_VAPOR(command, vapor) command
#endif

//------------------------------------------------------------------------------------------------
// Define the API definition macros
//
// Use the API macro to define a function type, a variable to store the function pointer, 
// and a default implementation of the function.
// 

#define EE_PROFILER_API_TYPE(name) name##Type
#define EE_PROFILER_API_NULL_IMPL(name) _##name##NullImpl
#define EE_PROFILER_API(retType, default, name, ...) \
    typedef retType (*EE_PROFILER_API_TYPE(name))(__VA_ARGS__);\
    static retType EE_PROFILER_API_NULL_IMPL(name)(__VA_ARGS__) { return default;}\
    EE_PROFILER_API_TYPE(name) name;
#define EE_PROFILER_API_ASSIGN_DEFAULT(profiler, name) \
    (profiler).name = Profiler::EE_PROFILER_API_NULL_IMPL(name);

//------------------------------------------------------------------------------------------------
// Define the profiler
//
// All builds of the engine have an available profiler. However the hooks for individual modules
// or all modules may have been vaporized. This allows all aspects of code to be profiled
// if required.

// Global profiler class to map profiler functions to a set of profiling functions
// or a visualizer
class EE_EFD_ENTRY Profiler
{
public:
    Profiler();

    void AssignDefaults();

    static void Initialize(Profiler* pProfilerTable = NULL);
    static void Tick();
    static void Shutdown();
    static Profiler* GetInstance();
    static void* GetConfig();

    typedef efd::UInt32 FlagType;

public:
    EE_PROFILER_API(void, , _Initialize);
    EE_PROFILER_API(void, , _Shutdown);
    EE_PROFILER_API(efd::UInt32, 0, _Tick);

    EE_PROFILER_API(efd::UInt32, 0, Enter, FlagType flags, const char* name);
    EE_PROFILER_API(efd::UInt32, 0, Leave, efd::UInt32 id);

    EE_PROFILER_API(efd::UInt32, 0, MemAlloc, const void* pAddress, efd::UInt32 size, 
        const char* pDesc, const char* pFileName, int line);
    EE_PROFILER_API(efd::UInt32, 0, MemFree, const void* pAddress);

protected:

    void* m_pConfig;

    static bool ms_bInitialized;
    static Profiler ms_profiler;
};

// Profiler Context definition. Each module should have an appropriate
// context that may be enabled/disabled from generating profiler data.
class EE_EFD_ENTRY ProfilerContext
{
public:

    ProfilerContext(const char* pName);
    ~ProfilerContext();

    static ProfilerContext* FindContext(const char* pName);
    static void SetEnabledAll(bool bEnable);
    void SetEnabled(bool bEnable);
    const char* GetName() const;
    Profiler* m_pProfiler;

protected:

    static void RegisterContext(ProfilerContext* pContext);

    const char* m_pName;
    static ProfilerContext* ms_pHeadContext;
    ProfilerContext* m_pNextContext;
};

// Profiler zone definition. 
class EE_EFD_ENTRY ProfilerZone
{
public:

    ProfilerZone(efd::ProfilerContext& context, unsigned int enterID);
    ~ProfilerZone();

protected:
    efd::ProfilerContext& m_context;
    unsigned int m_enterID;

private:
    // Not Implemented
    ProfilerZone& operator =(const ProfilerZone& other);
};

//------------------------------------------------------------------------------------------------
// Profiler HOOK configuration
//

#define EE_PROFILER_TYPE efd::Profiler

#define EE_PROFILER_CONTEXT_TYPE efd::ProfilerContext
#define EE_PROFILER_CONTEXT_NAME(name) g_profilerContext_##name

#define EE_PROFILER_ZONE_TYPE efd::ProfilerZone

//------------------------------------------------------------------------------------------------
// Profiler HOOK helpers
//
#define EE_PROFILER_CONTEXT_IMPL(name) EE_PROFILER_VAPOR(EE_PROFILER_CONTEXT_TYPE EE_PROFILER_CONTEXT_NAME(name)(#name), )
#define EE_PROFILER_CONTEXT_QUALIFIER(qualifier, name) EE_PROFILER_VAPOR(qualifier EE_PROFILER_CONTEXT_TYPE EE_PROFILER_CONTEXT_NAME(name), )
#define EE_PROFILER_CONTEXT_EXTERN(name) EE_PROFILER_CONTEXT_QUALIFIER(extern, name)
#define EE_PROFILER_CONTEXT(name) \
    EE_PROFILER_CONTEXT_EXTERN(name);\
    EE_PROFILER_CONTEXT_IMPL(name)

#define EE_PROFILER_CONTEXT_ENABLE(name, enable) EE_PROFILER_VAPOR(EE_PROFILER_CONTEXT_NAME(name).SetEnabled(enable), )
#define EE_PROFILER_CONTEXT_ENABLE_ALL(enable) EE_PROFILER_VAPOR(EE_PROFILER_CONTEXT_TYPE::SetEnabledAll(enable), )

#define EE_PROFILER_USE_CONTEXT(context) (context.m_pProfiler)
#define EE_PROFILER_GUARD(context, command, vapor) ((EE_PROFILER_USE_CONTEXT(context)) ? (command) : (vapor))

#define EE_PROFILER_CALL(context, function, vapor, ...) EE_PROFILER_VAPOR(EE_PROFILER_GUARD(context, EE_PROFILER_USE_CONTEXT(context)->function(__VA_ARGS__), vapor), vapor)
#define EE_PROFILER_CALL_CONTEXT(contextName, function, vapor, ...) EE_PROFILER_CALL(EE_PROFILER_CONTEXT_NAME(contextName), function, vapor, __VA_ARGS__)

#define EE_PROFILER_ZONE_IMPL(context, flags, name) \
    EE_PROFILER_ZONE_TYPE EE_MAKE_UNIQUE_NAME(profilerZone)(EE_PROFILER_CONTEXT_NAME(context), EE_PROFILER_ENTER(context, flags, name))

//------------------------------------------------------------------------------------------------
// Profiler HOOKS 
//

// Zones:
#define EE_PROFILER_ENTER(context, flags, name) EE_PROFILER_CALL_CONTEXT(context, Enter, 0, flags, name)
#define EE_PROFILER_LEAVE(context, id) EE_PROFILER_CALL_CONTEXT(context, Leave, 0, id)
#define EE_PROFILER_ZONE(context, flags, name) EE_PROFILER_VAPOR(EE_PROFILER_ZONE_IMPL(context, flags, name), )

// Memory allocation:
#define EE_PROFILER_MEM_ALLOC(context, address, size, description, file, line) \
    EE_PROFILER_CALL_CONTEXT(context, MemAlloc, 0, address, size, description, file, line)
#define EE_PROFILER_MEM_FREE(context, address) EE_PROFILER_CALL_CONTEXT(context, MemFree, 0, address);

//------------------------------------------------------------------------------------------------
#include "Profiler.inl"

} // End namespace efd

#pragma warning( pop )

#endif // EE_PROFILER_H