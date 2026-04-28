#include <efd/DefaultInitializeMemoryLogHandler.h>
#include <efd/DefaultInitializeMemoryManager.h>

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
