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

#ifndef NIDECORATIONCELL_H 
#define NIDECORATIONCELL_H

#include <NiAVObject.h>
#include <NiTransform.h>
#include <NiUniversalTypes.h>

#include "NiDecorationMeshInfo.h"

/**
    Simple class that represents a chunk of a specific decoration layer. It contains sufficient
    data for a generator to generate transforms for instances within this cell.

    This class is used internally by the decorations system and should only be
    used when creating a custom NiDecorationGenerator derived class.

    @internal
 */
class NIDECORATION_ENTRY NiDecorationCell : public NiMemObject
{
public:

    /// @cond EMERGENT_INTERNAL

    /// World space transform of this cell.
    NiTransform m_kWorldTransform;

    /// Layer space transform of this cell.
    NiTransform m_kLocalTransform;

    /// Indices in the 2D seed array.
    //@{
    NiUInt32 m_uiIndexX;
    NiUInt32 m_uiIndexY;
    //@}

    /// In cases where an allocation region is used over multiple fields, this
    /// is used to keep track of which field the cell belongs too.
    NiDecorationMeshInfo* m_pkRegionID;

    /// Index of this cell within the owning instance region
    NiUInt32 m_uiIndexInRegion;

    /// @endcond

};

#endif // NIDECORATIONCELL_H
