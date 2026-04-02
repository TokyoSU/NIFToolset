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
#ifndef NITHASHSETBASE_H
#define NITHASHSETBASE_H

#include <NiMemObject.h>
#include <NiUniversalTypes.h>
#include <NiMemoryDefines.h>
#include <efd/Asserts.h>

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

typedef void* NiTHashSetIterator;

template <class TVAL> class NiTHashSetItem : public NiMemObject
{
public:
    NiTHashSetItem* m_pkNext;
    TVAL m_val;
};

// The hash function class
template <class TVAL> class NiTHashSetHashFunctor : public NiMemObject
{
public:
    static unsigned int KeyToHashIndex(TVAL val, NiUInt32 uiTableSize);
};

// The equality function class
template <class TVAL> class NiTHashSetEqualsFunctor : public NiMemObject
{
public:
    static bool IsKeysEqual(TVAL val1, TVAL val2);
};

template <class TheAllocator, class TVAL,
    class THASH = NiTHashSetHashFunctor<TVAL>,
    class TEQUALS = NiTHashSetEqualsFunctor<TVAL> > class NiTHashSetBase :
    public NiMemObject
{
public:
    // The NiTHashSetBase class expects its allocator to provide a memory chunk large enough to
    // store an entire NiTHashSetItem<TVAL>, rather than just the 'advertised' space for a TVAL.
    // Because the actual internal storage type is protected to each allocator, the interface for
    // NiTHashSet requires that the allocator provide an enumeration SizeOfAllocNode, set to the
    // maximum amount of the memory returned by Allocate() that may be modified by the caller.
    //
    // If you hit this assert, you need to add something like the following to the TheAllocator
    // class passed in to NiTHashSetBase:
    //     public: enum Constant { SizeOfAllocNode = sizeof(AllocNode) };
    EE_COMPILETIME_ASSERT(sizeof(NiTHashSetItem<TVAL>) <= TheAllocator::SizeOfAllocNode);

    // construction and destruction
    NiTHashSetBase(NiUInt32 uiHashSize = 257);
    virtual ~NiTHashSetBase();

    // counting elements in hash set
    inline NiUInt32 GetCount() const;
    inline bool IsEmpty() const;

    // add or remove elements
    inline void Add(TVAL val);
    inline bool Remove(TVAL val);
    inline void RemoveAll();

    // map traversal
    inline NiTHashSetIterator GetFirstPos() const;
    inline TVAL& GetNext(NiTHashSetIterator& pos) const;

    /**
        Resize the hash set.

        The hash table of the set is resized to the given size (which must be
        non-zero), and all of the entries in the set are re-mapped to new
        locations. Any interators will be invalid after this operation. This
        operation is expensive for large sets - the cost is at least linear
        in the number of entries.
    */
    inline void Resize(NiUInt32 uiNewHashSize);

protected:
    // hash table stored as array of singly-linked lists
    virtual void SetValue(NiTHashSetItem<TVAL>* pkItem, TVAL val);
    virtual void ClearValue(NiTHashSetItem<TVAL>* pkItem);

    virtual NiTHashSetItem<TVAL>* NewItem() = 0;
    virtual void DeleteItem(NiTHashSetItem<TVAL>* pkItem) = 0;

    NiUInt32 m_uiHashSize;                // maximum slots in hash table
    NiTHashSetItem<TVAL>** m_ppkHashTable;// hash table storage

    struct AntiBloatAllocator : public TheAllocator
    {
        // We reduce TheAllocator by 4 bytes by deriving
        // See http://www.cantrip.org/emptyopt.html
        NiUInt32 m_uiCount;      // number of elements in list
    };

    AntiBloatAllocator m_kAllocator;

private:
    // To prevent an application from inadvertently causing the compiler to
    // generate the default copy constructor or default assignment operator,
    // these methods are declared as private. They are not defined anywhere,
    // so code that attempts to use them will not link.
    NiTHashSetBase(const NiTHashSetBase&);
    NiTHashSetBase& operator=(const NiTHashSetBase&);
};

//--------------------------------------------------------------------------------------------------
// Inline include
#include "NiTHashSetBase.inl"

//--------------------------------------------------------------------------------------------------

#endif // NITHASHSETBASE_H
