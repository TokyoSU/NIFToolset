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

#ifndef NIDECORATIONMATERIALDESCRIPTOR_H
#define NIDECORATIONMATERIALDESCRIPTOR_H

#include <NiStandardMaterialDescriptor.h>
#include "NiDecorationLibType.h"

class NIDECORATION_ENTRY NiDecorationMaterialDescriptor : public NiStandardMaterialDescriptor
{
public:

    // Offset 7, First Byte, Index 4
    NiDeclareDefaultIndexedBitfieldEntry(FADE_METHOD, 1, PSSMWHICHLIGHT, 4)

    // Offset 0, Second Byte, Index 4
};

#endif // NIDECORATIONMATERIALDESCRIPTOR_H