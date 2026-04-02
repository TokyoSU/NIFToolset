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

// Precompiled Header
#include "NiParticlePCH.h"

#include "NiPSCommonSemantics.h"

NiFixedString NiPSCommonSemantics::ms_akSemantics[];

//--------------------------------------------------------------------------------------------------
void NiPSCommonSemantics::_SDMInit()
{
    NiUInt16 usCount = 1;

    ms_akSemantics[PS_INVALID] = NULL;

#define DefSemantic(string) ms_akSemantics[PS_##string] = #string; usCount++;

    DefSemantic(PARTICLEPOSITION);
    DefSemantic(PARTICLEVELOCITY);
    DefSemantic(PARTICLEAGE);
    DefSemantic(PARTICLELIFESPAN);
    DefSemantic(PARTICLELASTUPDATE);
    DefSemantic(PARTICLEFLAGS);
    DefSemantic(PARTICLEINITSIZE);
    DefSemantic(PARTICLESIZE);
    DefSemantic(PARTICLEROTAXIS);
    DefSemantic(PARTICLEROTINITANGLE);
    DefSemantic(PARTICLEROTANGLE);
    DefSemantic(PARTICLEROTSPEED);
    DefSemantic(PARTICLECOLOR);
    DefSemantic(PARTICLEROTATION);
    DefSemantic(PARTICLESCALE);

#undef DefSemantic

    EE_ASSERT(usCount == PS_SEMANTICSCOUNT);
}

//--------------------------------------------------------------------------------------------------
void NiPSCommonSemantics::_SDMShutdown()
{
    NiUInt16 usCount = 0;

#define UndefSemantic(string) ms_akSemantics[PS_##string] = NULL; usCount++;

    UndefSemantic(INVALID);
    UndefSemantic(PARTICLEPOSITION);
    UndefSemantic(PARTICLEVELOCITY);
    UndefSemantic(PARTICLEAGE);
    UndefSemantic(PARTICLELIFESPAN);
    UndefSemantic(PARTICLELASTUPDATE);
    UndefSemantic(PARTICLEFLAGS);
    UndefSemantic(PARTICLEINITSIZE);
    UndefSemantic(PARTICLESIZE);
    UndefSemantic(PARTICLEROTAXIS);
    UndefSemantic(PARTICLEROTINITANGLE);
    UndefSemantic(PARTICLEROTANGLE);
    UndefSemantic(PARTICLEROTSPEED);
    UndefSemantic(PARTICLECOLOR);
    UndefSemantic(PARTICLEROTATION);
    UndefSemantic(PARTICLESCALE);

#undef UndefSemantic

    EE_ASSERT(usCount == PS_SEMANTICSCOUNT);
}

//--------------------------------------------------------------------------------------------------
NiPSCommonSemantics::CommonSemantics NiPSCommonSemantics::GetSemanticEnum(
    const NiFixedString& kSemantic)
{
    for (NiUInt16 us = 0; us < PS_SEMANTICSCOUNT; ++us)
    {
        if (kSemantic == ms_akSemantics[us])
        {
            return (CommonSemantics) us;
        }
    }

    return PS_INVALID;
}

//--------------------------------------------------------------------------------------------------
