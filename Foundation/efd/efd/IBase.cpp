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

#include <efd/IBase.h>
#include <efd/MemTracker.h>

using namespace efd;


#if defined(EE_MEMTRACKER_DETAILEDREPORTING)

//------------------------------------------------------------------------------------------------
IBase::IBase()
{
    // This will set a report but only if our "this" pointer matches the start of a MemTracker
    // allocation unit. This means that in cases of multiple inheritance the 'first' leaf class in
    // the inheritance tree needs to be an IBase in order for this to work. If you are multiplely
    // derived from IBase this will only succeed for one of the super classes but will work just
    // fine so long as that first leaf is an IBase. There will be no harmful side effects if the
    // first leaf super class isn't an IBase, you simply won't get a detailed report for that
    // class unless it registers its own reporting method. Also, derived classes can provide a more
    // detailed reporting function simply by placing a EE_MEM_SETDETAILEDREPORT call in their
    // constructor to register another reporting function. Doing so will completely replace this
    // method with the new method, it will not chain together the methods or anything like that.
    EE_MEM_SETDETAILEDREPORT(this, IBase::LeakDump);
}

//------------------------------------------------------------------------------------------------
void IBase::LeakDump(void* pMem, char* o_buffer, unsigned int i_cchBuffer)
{
    IBase* pBase = reinterpret_cast<IBase*>(pMem);

    efd::Snprintf(o_buffer, i_cchBuffer, EE_TRUNCATE,
        "IBase<%s 0x%08X>",
        pBase->GetClassDesc()->GetClassName(),
        pBase->GetClassID());
}
#endif

//------------------------------------------------------------------------------------------------
void IBase::DeleteThis() const
{
    EE_DELETE this;
}

//------------------------------------------------------------------------------------------------
