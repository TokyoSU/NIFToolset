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

#include <direct.h>
#include <process.h>
#include <efd/Asserts.h>

namespace efd
{

//-------------------------------------------------------------------------------------------------
inline efd::SInt32 AtomicIncrement(efd::SInt32& value)
{
    return InterlockedIncrement((LONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::SInt32 AtomicDecrement(efd::SInt32& value)
{
    return InterlockedDecrement((LONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::SInt32 AtomicIncrement(volatile efd::SInt32& value)
{
    return InterlockedIncrement((volatile LONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::SInt32 AtomicDecrement(volatile efd::SInt32& value)
{
    return InterlockedDecrement((volatile LONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt32 AtomicIncrement(efd::UInt32& value)
{
    return InterlockedIncrement((LONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt32 AtomicDecrement(efd::UInt32& value)
{
    EE_ASSERT(value > 0);
    return InterlockedDecrement((LONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt32 AtomicIncrement(volatile efd::UInt32& value)
{
    return InterlockedIncrement((volatile LONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt32 AtomicDecrement(volatile efd::UInt32& value)
{
    EE_ASSERT(value > 0);
    return InterlockedDecrement((volatile LONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline void* AtomicCompareAndSwap(
    void* volatile* ppDestination,
    void* pComparand,
    void* pExchange)
{
    return InterlockedCompareExchangePointer(ppDestination, pExchange, pComparand);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt32 AtomicCompareAndSwap(
    efd::UInt32 volatile* pDestination,
    efd::UInt32 comparand,
    efd::UInt32 exchange)
{
    return InterlockedCompareExchange(
        reinterpret_cast<LONG volatile*>(pDestination),
        static_cast<LONG>(exchange),
        static_cast<LONG>(comparand));
}
//-------------------------------------------------------------------------------------------------
#if defined(EE_ARCH_64)
inline efd::UInt64 AtomicCompareAndSwap(
    efd::UInt64 volatile* pDestination,
    efd::UInt64 comparand,
    efd::UInt64 exchange)
{
    return InterlockedCompareExchange64(
        reinterpret_cast<LONGLONG volatile*>(pDestination),
        static_cast<LONGLONG>(exchange),
        static_cast<LONGLONG>(comparand));
}
#endif
//-------------------------------------------------------------------------------------------------
#if defined(EE_ARCH_64)
//-------------------------------------------------------------------------------------------------
inline efd::SInt64 AtomicIncrement(efd::SInt64& value)
{
    return InterlockedIncrement64((LONGLONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::SInt64 AtomicDecrement(efd::SInt64& value)
{
    return InterlockedDecrement64((LONGLONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::SInt64 AtomicIncrement(volatile efd::SInt64& value)
{
    return InterlockedIncrement64((volatile LONGLONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::SInt64 AtomicDecrement(volatile efd::SInt64& value)
{
    return InterlockedDecrement64((volatile LONGLONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt64 AtomicIncrement(efd::UInt64& value)
{
    return (efd::UInt64)InterlockedIncrement64((LONGLONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt64 AtomicDecrement(efd::UInt64& value)
{
    EE_ASSERT(value > 0);
    return (efd::UInt64)InterlockedDecrement64((LONGLONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt64 AtomicIncrement(volatile efd::UInt64& value)
{
    return (efd::UInt64)InterlockedIncrement64((volatile LONGLONG*)&value);
}
//-------------------------------------------------------------------------------------------------
inline efd::UInt64 AtomicDecrement(volatile efd::UInt64& value)
{
    EE_ASSERT(value > 0);
    return (efd::UInt64)InterlockedDecrement64((volatile LONGLONG*)&value);
}
//-------------------------------------------------------------------------------------------------
#endif // defined(EE_ARCH_64)

} // end namespace efd
