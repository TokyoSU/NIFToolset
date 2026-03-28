// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

// Precompiled Header
#include "NiAudioPCH.h"
#include "NiAudio.h"
#include "NiAudioSDM.h"

NiImplementSDMConstructor(NiAudio, "NiMain");

#ifdef NIAUDIO_EXPORT
NiImplementDllMain(NiAudio);
#endif

//---------------------------------------------------------------------------
void NiAudioSDM::Init()
{
    NiImplementSDMInitCheck();

    NiRegisterStream(NiAudioListener);
    NiRegisterStream(NiAudioSource);
    NiRegisterStream(NiAudioSystem);

    //NiAudioSystem::ms_pAudioSystem = (NiAudioSystem*)NiNew NiAudioSystem;
    //NIASSERT(NiAudioSystem::ms_pAudioSystem);
}
//---------------------------------------------------------------------------
void NiAudioSDM::Shutdown()
{
    NiImplementSDMShutdownCheck();

    NiUnregisterStream(NiAudioListener);
    NiUnregisterStream(NiAudioSource);
    NiUnregisterStream(NiAudioSystem);

    //NiDelete NiAudioSystem::ms_pAudioSystem;
    //NiAudioSystem::ms_pAudioSystem = NULL;
}
//---------------------------------------------------------------------------
