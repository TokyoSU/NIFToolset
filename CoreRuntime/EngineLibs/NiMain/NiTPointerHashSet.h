// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2010 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#pragma once
#ifndef NITPOINTERHASHSET_H
#define NITPOINTERHASHSET_H

#include "NiTHashSetBase.h"
#include "NiTPointerAllocator.h"

template <class TVAL,
    class THASH = NiTHashSetHashFunctor<TVAL>,
    class TEQUALS = NiTHashSetEqualsFunctor<TVAL> > class NiTPointerHashSet :
    public NiTHashSetBase<NiTPointerAllocator<size_t>, TVAL, THASH, TEQUALS>
{
public:
     inline NiTPointerHashSet(unsigned int uiHashSize = 257) : NiTHashSetBase<
         NiTPointerAllocator<size_t>, TVAL, THASH, TEQUALS>(uiHashSize)
         {}
    ~NiTPointerHashSet();
    virtual NiTHashSetItem<TVAL>* NewItem();
    virtual void DeleteItem(NiTHashSetItem<TVAL>* pkItem);
};

#include "NiTPointerHashSet.inl"

#endif // NITPOINTERHASHSET_H
