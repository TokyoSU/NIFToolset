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
#include <efd/OS.h>
#include <efd/StackUtils.h>
#include <efd/DynamicModule.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

#if defined(EE_ENABLE_STACKTRACE)

#include <dbghelp.h>

//------------------------------------------------------------------------------------------------
namespace efd
{

typedef BOOL (__stdcall *SymInitialize_fn)(
    IN HANDLE   hProcess,
    IN PSTR     UserSearchPath,
    IN BOOL     fInvadeProcess);

typedef BOOL (__stdcall *SymCleanup_fn)(IN HANDLE hProcess);

typedef BOOL (__stdcall *StackWalk64_fn)(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress);

typedef PFUNCTION_TABLE_ACCESS_ROUTINE64 SymFunctionTableAccess64_fn;
typedef PGET_MODULE_BASE_ROUTINE64 SymGetModuleBase64_fn;

typedef BOOL (__stdcall *SymFromAddr_fn)(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    OUT PDWORD64            Displacement,
    IN OUT PSYMBOL_INFO     Symbol);


// All of our dynamically loaded functions from dbghelp.dll:
static SymInitialize_fn SymInitialize = NULL;
static SymCleanup_fn SymCleanup = NULL;
static StackWalk64_fn StackWalk64 = NULL;
static SymFunctionTableAccess64_fn SymFunctionTableAccess64 = NULL;
static SymGetModuleBase64_fn SymGetModuleBase64 = NULL;
static SymFromAddr_fn SymFromAddr = NULL;

// global initialization state:
static bool g_StackTraceReady = false;
static bool g_StackTraceFailed = false;

class SymbolCleaner
{
public:
    ~SymbolCleaner()
    {
        if (efd::SymInitialize && efd::SymCleanup)
        {
            efd::SymCleanup(GetCurrentProcess());
        }
    }
};

} // end namespace efd


//------------------------------------------------------------------------------------------------
inline bool PrepareForStackTrace()
{
    if (!efd::g_StackTraceReady && !efd::g_StackTraceFailed)
    {
        efd::StackUtils::EnableStackTracing();
    }
    return efd::g_StackTraceReady;
}


//------------------------------------------------------------------------------------------------
DWORD GetMachineType()
{
#ifdef _M_IX86
    return IMAGE_FILE_MACHINE_I386;
#elif _M_IA64
    return IMAGE_FILE_MACHINE_IA64;
#elif _M_X64
    return IMAGE_FILE_MACHINE_AMD64;
#else
    // Generic way is to check the image header for the current exe.
    pBase = MapViewOfFile(current exe, sizeof(IMAGE_NT_HEADERS), blah blah);

    PIMAGE_NT_HEADERS pHeader = ImageNTHeader(pBase);
    if (pHeader)
    {
        return pHeader->FileHeader.Machine;
    }
#endif
}

//------------------------------------------------------------------------------------------------
inline void SetInitialStackFrameFromContext(STACKFRAME64& frame, CONTEXT& context)
{
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Mode = AddrModeFlat;
  frame.AddrBStore.Mode = AddrModeFlat;

#ifdef _M_IX86
  frame.AddrPC.Offset = context.Eip;
  frame.AddrFrame.Offset = context.Ebp;
  frame.AddrStack.Offset = context.Esp;
#elif _M_IA64
  frame.AddrPC.Offset = context.StIIP;
  frame.AddrFrame.Offset = context.IntSp;
  frame.AddrStack.Offset = context.IntSp;
  frame.AddrBStore.Offset = context.RsBSP;
#elif _M_X64
  frame.AddrPC.Offset = context.Rip;
  frame.AddrFrame.Offset = context.Rsp;
  frame.AddrStack.Offset = context.Rsp;
#else
    #error Unknown architecture
#endif
}


//------------------------------------------------------------------------------------------------
bool efd::StackUtils::EnableStackTracing(bool enable)
{
    static efd::DynamicModule dbgHelp;
    if (enable)
    {
        // If we already loaded dbghelp then return the previous result
        if (dbgHelp.IsLoaded())
        {
            return g_StackTraceReady;
        }

        if (dbgHelp.LoadModule("DbgHelp.dll"))
        {
            bool required = true;
            bool optional = true;
#define STACKUTILS_REQUIREDFUNC(func)\
    required &= ((efd::func = (efd::func##_fn)dbgHelp.GetMethod(#func)) != NULL)
#define STACKUTILS_OPTIONALFUNC(func)\
    optional &= ((efd::func = (efd::func##_fn)dbgHelp.GetMethod(#func)) != NULL)

            // All the minimal walking functions are required:
            STACKUTILS_REQUIREDFUNC(SymInitialize);
            STACKUTILS_REQUIREDFUNC(StackWalk64);
            STACKUTILS_REQUIREDFUNC(SymFunctionTableAccess64);
            STACKUTILS_REQUIREDFUNC(SymGetModuleBase64);

            if (required)
            {
                // All the symbol lookup functions are optional, without them things are just
                // uglier.

                // Without this we'll leak some memory at process shutdown but otherwise function:
                STACKUTILS_OPTIONALFUNC(SymCleanup);
                // This is needed to make symbols into pretty names:
                STACKUTILS_OPTIONALFUNC(SymFromAddr);

                if (0 != efd::SymInitialize(GetCurrentProcess(), NULL, true))
                {
                    g_StackTraceReady = true;
                    g_StackTraceFailed = false;
                    return true;
                }
            }

#undef STACKUTILS_REQUIREDFUNC
#undef STACKUTILS_OPTIONALFUNC
        }

        g_StackTraceFailed = true;
    }
    else
    {
        g_StackTraceReady = false;
        g_StackTraceFailed = true;
        dbgHelp.UnloadModule();
    }

    return false;
}

//------------------------------------------------------------------------------------------------
void efd::StackUtils::TurnOffStackTracing()
{
    EnableStackTracing(false);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 efd::StackUtils::FastStackTrace(
    void** o_pResults,
    efd::UInt32 i_maxDepth,
    efd::UInt32 i_skipFrames)
{
    efd::UInt32 framesFound = 0;

    // API added for XP, we are still compiling VC8 targeted for Win2k.
#if (NTDDI_VERSION > NTDDI_WINXP)
    framesFound = RtlCaptureStackBackTrace(i_skipFrames, i_maxDepth, o_pResults, NULL);
#else
    if (!PrepareForStackTrace())
    {
        return 0;
    }

    // Always skip our own frame:
    ++i_skipFrames;

    CONTEXT context = {0};
    context.ContextFlags = CONTEXT_CONTROL;
    RtlCaptureContext(&context);

    STACKFRAME64 frame = {0};
    SetInitialStackFrameFromContext(frame, context);

    efd::UInt32 depth = 0;
    for (; depth < i_maxDepth+i_skipFrames; ++depth)
    {
        if (efd::StackWalk64(GetMachineType(),
                              GetCurrentProcess(),
                              GetCurrentThread(),
                              &frame,
                              &context,
                              NULL, // use default Read Memory Routine
                              efd::SymFunctionTableAccess64, // function table access routine
                              efd::SymGetModuleBase64, // get module base routine
                              NULL)) // 16-bit address translation routine
        {
            // successfully read a stack frame
            if (depth >= i_skipFrames)
            {
                o_pResults[framesFound] = (void*)frame.AddrPC.Offset;
                ++framesFound;
            }
        }
        else
        {
            // failed
            break;
        }
    }
#endif

    return framesFound;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 efd::StackUtils::ExceptionStackTrace(
    void** o_pResults,
    efd::UInt32 i_maxDepth,
    void* i_pPlatform1,
    void* i_pPlatform2)
{
    EE_UNUSED_ARG(i_pPlatform2);

    if (!PrepareForStackTrace())
    {
        return 0;
    }

    CONTEXT* pContext = (CONTEXT*)i_pPlatform1;

    STACKFRAME64 frame = {0};
    SetInitialStackFrameFromContext(frame, *pContext);

    efd::UInt32 framesFound = 0;
    for (efd::UInt32 depth = 0; depth < i_maxDepth; ++depth)
    {
        if (efd::StackWalk64(GetMachineType(),
            GetCurrentProcess(),
            GetCurrentThread(),
            &frame,
            pContext,
            NULL, // use default Read Memory Routine
            efd::SymFunctionTableAccess64, // function table access routine
            efd::SymGetModuleBase64, // get module base routine
            NULL)) // 16-bit address translation routine
        {
            // successfully read a stack frame
            o_pResults[framesFound] = (void*)frame.AddrPC.Offset;
            ++framesFound;
        }
        else
        {
            // failed
            break;
        }
    }

    return framesFound;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 efd::StackUtils::StackTrace(
    efd::UInt32 i_maxDepth,
    char* o_pszzResultBuffer,
    efd::UInt32 i_cchBufferSize,
    efd::UInt32 i_skipFrames,
    const char* i_pszPrefix)
{
    // Always skip our own frame:
    ++i_skipFrames;

    void* pResults[64];
    if (i_maxDepth > EE_ARRAYSIZEOF(pResults))
    {
        i_maxDepth = EE_ARRAYSIZEOF(pResults);
    }

    efd::UInt32 numframes = FastStackTrace(pResults, i_maxDepth, i_skipFrames);
    ResolveSymbolNames(pResults, numframes, o_pszzResultBuffer, i_cchBufferSize, i_pszPrefix);
    return numframes;
}

//------------------------------------------------------------------------------------------------
struct SymbolInfo : public SYMBOL_INFO
{
    TCHAR MoreName[128];

    SymbolInfo()
    {
        SizeOfStruct = sizeof(SYMBOL_INFO);
        MaxNameLen = sizeof(SymbolInfo) - sizeof(SYMBOL_INFO);
    }
};

//------------------------------------------------------------------------------------------------
bool efd::StackUtils::ResolveSymbolNames(
    const void* const * i_pSymbols,
    efd::UInt32 i_cSymbols,
    char* o_pszzResultBuffer,
    efd::UInt32 i_cchBufferSize,
    const char* i_pszPrefix)
{
    PrepareForStackTrace();

    // First truncate the output buffer to length zero
    EE_ASSERT(i_cchBufferSize > 0);
    o_pszzResultBuffer[0] = '\0';

    char buffer[32];
    SymbolInfo syminfo;
    for (efd::UInt32 i = 0; i < i_cSymbols; ++i)
    {
        efd::Strncat(o_pszzResultBuffer, i_cchBufferSize, i_pszPrefix, EE_TRUNCATE);
        DWORD64 cbOffset = 0;
        if (efd::SymFromAddr &&
             efd::SymFromAddr(GetCurrentProcess(), (DWORD64)(i_pSymbols[i]), &cbOffset, &syminfo))
        {
            syminfo.Name[syminfo.MaxNameLen] = '\0';
            efd::Strncat(o_pszzResultBuffer, i_cchBufferSize, syminfo.Name, EE_TRUNCATE);
            efd::Strncat(o_pszzResultBuffer, i_cchBufferSize, " + ", EE_TRUNCATE);
            _i64toa(cbOffset, buffer, 10);
            efd::Strncat(o_pszzResultBuffer, i_cchBufferSize, buffer, EE_TRUNCATE);
            efd::Strncat(o_pszzResultBuffer, i_cchBufferSize, "\n", EE_TRUNCATE);
        }
        else
        {
            efd::Strncat(o_pszzResultBuffer, i_cchBufferSize, "0x", EE_TRUNCATE);
            _ui64toa((DWORD64)(i_pSymbols[i]), buffer, 16);
            efd::Strncat(o_pszzResultBuffer, i_cchBufferSize, buffer, EE_TRUNCATE);
            efd::Strncat(o_pszzResultBuffer, i_cchBufferSize, "\n", EE_TRUNCATE);
        }
    }
    return true;
}

//------------------------------------------------------------------------------------------------
// These are global to reserve the memory in case our exception is something like a stack overflow
// in which case we might not be able to get a large buffer of memory on the stack.
char g_ExceptionInfoBuffer[1024];
void* g_ExceptionStackTrace[32];

//------------------------------------------------------------------------------------------------
LONG WINAPI EmergentUnhandledExceptionFilter(__in  struct _EXCEPTION_POINTERS* pExceptionInfo)
{
    g_ExceptionInfoBuffer[0] = '\0';
    int used = efd::Sprintf(g_ExceptionInfoBuffer, sizeof(g_ExceptionInfoBuffer),
        "Unhandled exception:\n"
        "  ExceptionCode: 0x%08X\n"
        "  ExceptionFlags: 0x%08X\n"
        "  ExceptionRecord: 0x%p\n"
        "  ExceptionAddress: 0x%p\n"
        "  NumberParameters: %d\n",
        pExceptionInfo->ExceptionRecord->ExceptionCode,
        pExceptionInfo->ExceptionRecord->ExceptionFlags,
        pExceptionInfo->ExceptionRecord->ExceptionRecord,
        pExceptionInfo->ExceptionRecord->ExceptionAddress,
        pExceptionInfo->ExceptionRecord->NumberParameters);
    for (DWORD i = 0; i < pExceptionInfo->ExceptionRecord->NumberParameters; ++i)
    {
        used += efd::Sprintf(g_ExceptionInfoBuffer + used, sizeof(g_ExceptionInfoBuffer) - used,
            "  ExceptionInformation[%d]: 0x%p\n",
            i,
            pExceptionInfo->ExceptionRecord->ExceptionInformation[i]);
    }

#if defined(EE_USE_EXCEPTION_STACKTRACE)
    used += efd::Sprintf(g_ExceptionInfoBuffer + used, sizeof(g_ExceptionInfoBuffer) - used,
        "  Stack Trace:\n");

    efd::UInt32 stackSize = efd::StackUtils::ExceptionStackTrace(
        g_ExceptionStackTrace,
        EE_ARRAYSIZEOF(g_ExceptionStackTrace),
        pExceptionInfo->ContextRecord,
        pExceptionInfo->ExceptionRecord);
    efd::StackUtils::ResolveSymbolNames(
        g_ExceptionStackTrace,
        stackSize,
        g_ExceptionInfoBuffer + used,
        sizeof(g_ExceptionInfoBuffer) - used,
        "    ");
#endif // defined(EE_USE_EXCEPTION_STACKTRACE)

    // todo: If the exception was an out of memory sort of exception we should go direct to
    // OutputDebugString as EE_LOG will try to allocate memory.
    if (efd::GetLogger())
    {
        EE_LOG(efd::kAssets, efd::ILogger::kERR0, ("%s", g_ExceptionInfoBuffer));

        efd::GetLogger()->Flush();
    }
    else
    {
        EE_OUTPUT_DEBUG_STRING(g_ExceptionInfoBuffer);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

//------------------------------------------------------------------------------------------------
void efd::StackUtils::LogOnUnhandledException(bool i_turnOn)
{
    if (i_turnOn)
    {
        SetUnhandledExceptionFilter(EmergentUnhandledExceptionFilter);
    }
    else
    {
        SetUnhandledExceptionFilter(NULL);
    }
}

//------------------------------------------------------------------------------------------------
#endif // defined(EE_ENABLE_STACKTRACE)

















