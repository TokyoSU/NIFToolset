#pragma once
#ifndef NIF_NATIVE_SYSTEM_H
#define NIF_NATIVE_SYSTEM_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

NIFTOOLSET_NATIVE_ENTRY void NIF_System_Init(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_System_Shutdown(void);
NIFTOOLSET_NATIVE_ENTRY int NIF_System_IsInitialized(void);
NIFTOOLSET_NATIVE_ENTRY float NIF_System_GetCurrentTimeInSec(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_System_ResetBaseTime(void);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_SYSTEM_H
