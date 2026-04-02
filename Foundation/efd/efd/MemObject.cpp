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
#include "efdPCH.h"

#include <efd/MemObject.h>

#include <efd/MemManager.h>

#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
#include <exception> // for std::bad_alloc
#include <new>
#endif

using namespace efd;

//------------------------------------------------------------------------------------------------
#if defined(EE_MEMORY_DEBUGGER)
    void* MemObject::operator new(size_t stSize, MemHint)
#if !defined(EE_MEMORY_ENABLE_EXCEPTIONS)
        EE_EMPTY_THROW
#endif
    {
#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
        throw std::bad_alloc();
#else
        return NULL;
#endif
        EE_UNUSED_ARG(stSize);
    }

    void* MemObject::operator new[](size_t stSize, MemHint)
#if !defined(EE_MEMORY_ENABLE_EXCEPTIONS)
        EE_EMPTY_THROW
#endif
    {
        EE_UNUSED_ARG(stSize);
#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
        throw std::bad_alloc();
#else
        return NULL;
#endif
    }

    void* MemObject::operator new(
        size_t stSizeInBytes,
        MemHint kHint,
        const char* pcSourceFile,
        int iSourceLine,
        const char* pcFunction)
    {
        EE_ASSERT(MemManager::IsInitialized());
        if (stSizeInBytes == 0)
            stSizeInBytes = 1;

        kHint |= MemHint::COMPILER_PROVIDES_SIZE_ON_DEALLOCATE;
        void* p = MemManager::Get().Allocate(stSizeInBytes,
            EE_MEM_ALIGNMENT_DEFAULT, kHint, EE_MET_NEW,
            pcSourceFile, iSourceLine, pcFunction);

#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
        if (p == 0)
            throw std::bad_alloc();
#endif

        return p;
    }

    void* MemObject::operator new[](
        size_t stSizeInBytes,
        MemHint kHint,
        const char* pcSourceFile,
        int iSourceLine,
        const char* pcFunction)
    {
        EE_ASSERT(MemManager::IsInitialized());

        if (stSizeInBytes == 0)
            stSizeInBytes = 1;

#if defined(EE_PLATFORM_PS3) || defined(EE_PLATFORM_LINUX)
        // On GCC the allocated size is passed into operator delete[]
        // so there is no need for the allocator to save the size of
        // the allocation.
        kHint |= MemHint::COMPILER_PROVIDES_SIZE_ON_DEALLOCATE;
#endif

        void* p = MemManager::Get().Allocate(stSizeInBytes,
            EE_MEM_ALIGNMENT_DEFAULT, kHint, EE_MET_NEW_ARRAY,
            pcSourceFile, iSourceLine, pcFunction);

#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
        if (p == 0)
            throw std::bad_alloc();
#endif

        return p;
    }
#else
    void* MemObject::operator new(size_t stSizeInBytes,
        MemHint kHint)
    {
        EE_ASSERT(MemManager::IsInitialized());

        if (stSizeInBytes == 0)
            stSizeInBytes = 1;

        kHint |= MemHint::COMPILER_PROVIDES_SIZE_ON_DEALLOCATE;
        void* p = MemManager::Get().Allocate(stSizeInBytes,
            EE_MEM_ALIGNMENT_DEFAULT, kHint, EE_MET_NEW);

#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
        if (p == 0)
            throw std::bad_alloc();
#endif

        return p;
    }

    void* MemObject::operator new[](size_t stSizeInBytes,
        MemHint kHint)
    {
        EE_ASSERT(MemManager::IsInitialized());

        if (stSizeInBytes == 0)
            stSizeInBytes = 1;

#if defined(EE_PLATFORM_PS3) || defined(EE_PLATFORM_LINUX)
        // On GCC the allocated size is passed into operator delete[]
        // so there is no need for the allocator to save the size of
        // the allocation.
        kHint |= MemHint::COMPILER_PROVIDES_SIZE_ON_DEALLOCATE;
#endif

        void* p =  MemManager::Get().Allocate(stSizeInBytes,
            EE_MEM_ALIGNMENT_DEFAULT, kHint, EE_MET_NEW_ARRAY);

#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
        if (p == 0)
            throw std::bad_alloc();
#endif

        return p;
    }
#endif

//------------------------------------------------------------------------------------------------
void MemObject::operator delete(void* pvMem, size_t stElementSize)
{
    if (pvMem)
    {
        EE_ASSERT(MemManager::IsInitialized());
        MemManager::Get().Deallocate(pvMem, EE_MET_DELETE,
            stElementSize);
    }
}

//------------------------------------------------------------------------------------------------
#if defined(EE_PLATFORM_PS3) || defined(EE_PLATFORM_LINUX)
// On GCC the allocated size is passed into operator delete[] so there is no
// need for the allocator to save the size of the allocation.
void MemObject::operator delete[](void* pvMem, size_t stElementSize)
{
    if (pvMem)
    {
        EE_ASSERT(MemManager::IsInitialized());

        MemManager::Get().Deallocate(pvMem, EE_MET_DELETE_ARRAY,
            stElementSize);
    }
}
#else
void MemObject::operator delete[](void* pvMem, size_t)
{
    if (pvMem)
    {
        EE_ASSERT(MemManager::IsInitialized());

        MemManager::Get().Deallocate(pvMem, EE_MET_DELETE_ARRAY);
    }
}
#endif

//------------------------------------------------------------------------------------------------
