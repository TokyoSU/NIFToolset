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

#ifndef NIDECORATIONPIXELPROGRAMDESCRIPTOR_H
#define NIDECORATIONPIXELPROGRAMDESCRIPTOR_H

#include "NiDecorationLibType.h"

#include <NiMain.h>
#include <NiStandardPixelProgramDescriptor.h>

class NIDECORATION_ENTRY NiDecorationPixelProgramDescriptor : public 
    NiStandardPixelProgramDescriptor
{
public:
    NiDecorationPixelProgramDescriptor();

    // Offset 5, First Byte, Index 4
    NiDeclareDefaultIndexedBitfieldEntry(FADE_NOISE_ENABLED, 1, ALPHATEST, 4)

    NiString ToString();
};

#endif // NIDECORATIONPIXELPROGRAMDESCRIPTOR_H