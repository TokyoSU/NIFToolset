// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#ifndef NISEMAPHORE_H
#define NISEMAPHORE_H

#include "NiSystemLibType.h"
#include "NiRTLib.h"

class NiSemaphore {
public:
    NiSemaphore();
    NiSemaphore(int iCount);
    NiSemaphore(int iCount, int iMaxCount);
    ~NiSemaphore();

    inline int GetCount();
    inline int GetMaxCount();

    inline int Signal();
    inline int Wait();

protected:
#if !defined(_PS3) 
    volatile int m_iCount;
#endif
    int m_iMaxCount;

#if defined(WIN32) || defined(_XENON)
    HANDLE m_hSemaphore;
#elif defined(_PS3) 
    sys_semaphore_t m_hSemaphore;
#elif defined(_WII)
    OSSemaphore m_hSemaphore;
#endif

};

//---------------------------------------------------------------------------
inline NiSemaphore::NiSemaphore() :
    m_iCount(0),
    m_iMaxCount(1)
{
    m_hSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
    NIASSERT(m_hSemaphore != NULL);
}
//---------------------------------------------------------------------------
inline NiSemaphore::NiSemaphore(int iCount) :
    m_iCount(iCount),
    m_iMaxCount(iCount + 1)
{
    m_hSemaphore = CreateSemaphore(NULL, m_iCount, m_iMaxCount, NULL);
    NIASSERT(m_hSemaphore != NULL);
}
//---------------------------------------------------------------------------
inline NiSemaphore::NiSemaphore(int iCount, int iMaxCount) :
    m_iCount(iCount),
    m_iMaxCount(iMaxCount)
{
    m_hSemaphore = CreateSemaphore(NULL, m_iCount, m_iMaxCount, NULL);
    NIASSERT(m_hSemaphore != NULL);
}
//---------------------------------------------------------------------------
inline NiSemaphore::~NiSemaphore()
{
    CloseHandle(m_hSemaphore);
}
//---------------------------------------------------------------------------
inline int NiSemaphore::GetCount()
{
    return m_iCount;
}
//---------------------------------------------------------------------------
inline int NiSemaphore::GetMaxCount()
{
    return m_iMaxCount;
}
//---------------------------------------------------------------------------
inline int NiSemaphore::Signal()
{
    ReleaseSemaphore(m_hSemaphore, 1, NULL);
    return NiAtomicIncrement(const_cast<int&>(m_iCount));
}
//---------------------------------------------------------------------------
inline int NiSemaphore::Wait()
{
    WaitForSingleObject(m_hSemaphore, INFINITE);
    return NiAtomicDecrement(const_cast<NiInt32&>(m_iCount));
}
//---------------------------------------------------------------------------

#endif
