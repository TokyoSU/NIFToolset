#include "NIF.Native.System.h"

#include <efd/DefaultInitializeMemoryLogHandler.h>
#include <efd/DefaultInitializeMemoryManager.h>
#include <NiSystem.h>

#if defined(EE_EFD_NO_IMPORT)
efd::IAllocator* efd::CreateGlobalMemoryAllocator()
{
	return efd::CreateDefaultGlobalMemoryAllocator(false);
}

efd::IMemLogHandler* efd::InitializeMemoryLogHandler()
{
	return efd::CreateDefaultMemoryLogHandler();
}
#endif

extern "C"
{

void NIF_System_Init(void)
{
	NiInit();
}

void NIF_System_Shutdown(void)
{
	NiShutdown();
}

int NIF_System_IsInitialized(void)
{
	return NiStaticDataManager::IsInitialized() ? 1 : 0;
}

float NIF_System_GetCurrentTimeInSec(void)
{
	return NiGetCurrentTimeInSec();
}

void NIF_System_ResetBaseTime(void)
{
	NiResetBaseTime();
}

}
