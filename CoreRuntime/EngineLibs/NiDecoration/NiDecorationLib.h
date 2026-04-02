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

#ifndef NIDECORATIONLIB_H
#define NIDECORATIONLIB_H

// sets up for DLL import (if desired)
#include "NiDecorationLibType.h"

// Scene graph
#include "NiDecorationPlane.h"

// Functors are used to attach instances to a surface (like a terrain)
#include "NiDecorationFunctor.h"

// Generators dictate what mesh instances look like
#include "NiDecorationGenerator.h"
#include "NiDecorationBillBoardGenerator.h"
#include "NiDecorationCrossBillBoardGenerator.h"
#include "NiDecorationSimpleMeshGenerator.h"

// Custom material that supports screen-door alpha testing
#include "NiDecorationMaterial.h"

// Supporting classes
#include "NiDecorationFactories.h"

// Finally, static data
#include "NiDecorationSDM.h"
static NiDecorationSDM NiDecorationSDMObject;

#endif //#ifndef NIDECORATIONLIB_H
