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

#ifndef NIDECORATIONSDM_H
#define NIDECORATIONSDM_H

#include <NiStaticDataManager.h>
#include "NiDecorationLibType.h"

// NOTE: SDM is only called for non-DLL builds.  This is because the static
// data initialization for a plug-in needs to be done in the constructor of
// the plug-in, because that is the only way ordering of the SDMInit() can
// be ensured.  If DLL builds are being used for non-plug-ins, then they
// will need to do the SDM initialization themselves via
// NiDecorationSDM::Init() and NiDecorationSDM::Shutdown()
// respectively. Static library builds will SDMInit and SDMShutdown
// as per normal.
NiDeclareSDM(NiDecoration, NIDECORATION_ENTRY);

#endif // NIDECORATIONSDM_H
