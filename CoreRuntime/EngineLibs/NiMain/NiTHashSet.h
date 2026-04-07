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
#ifndef NITHASHSET_H
#define NIHASHSET_H

#include <NiTHashSetBase.h>
#include <NiTDefaultAllocator.h>

// The hash set class implements a hash table of TVAL to store values of TVAL.
// It uses modular arithmetic for building the hash keys with a default
// table size of 257.  If you want a larger table size, the best bet is to
// use a large prime number.  Consult a standard text on hashing for the
// basic theory.
//
// The template class assumes that type TVAL has the following:
//   1.  Default constructor, TVAL::TVAL();
//   2.  Copy constructor, TVAL::TVAL(const TVAL&);
//   3.  Assignment, TVAL& operator=(const TVAL&);
//   4.  Comparison, bool TVAL::operator==(const TKEY&), or supply a
//       specialized equality testing class in your template.
//   5.  Implicit conversion, TVAL::operator long(), for building hash key,
//       or you must pass in your own hash function class in your template.
//
// In both cases, the compiler-generated default constructor, copy
// constructor, and assignment operator are acceptable.
//
// Example of iteration over hash set
//
//     NiTHashSet<TVAL> kHashSet;
//     NiTHashSetIterator pos = kHashSet.GetFirstPos();
//     while (pos)
//     {
//         TVAL val;
//         kMap.GetNext(pos,val);
//         <process val here>;
//     }

template <class TVAL,
    class THASH = NiTHashSetHashFunctor<TVAL>,
    class TEQUALS = NiTHashSetEqualsFunctor<TVAL> > class NiTHashSet :
    public NiTHashSetBase<NiTDefaultAllocator<TVAL>, TVAL, THASH, TEQUALS>
{
public:
    inline NiTHashSet(unsigned int uiHashSize = 257) :
        NiTHashSetBase<NiTDefaultAllocator<TVAL>, TVAL, THASH, TEQUALS >
        (uiHashSize) {};
    ~NiTHashSet();

    virtual NiTHashSetItem<TVAL>* NewItem();
    virtual void DeleteItem(NiTHashSetItem<TVAL>* pkItem);

    // Prime numbers
    static NiUInt32 NextPrime(const NiUInt32 uiTarget);

    static const NiUInt32 NUM_PRIMES;
    static const NiUInt32 PRIMES[];
};

#include "NiTHashSet.inl"

#endif // NITHAHSET_H
