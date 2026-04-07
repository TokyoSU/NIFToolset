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
#include "NiBASSAudioSystem.h"

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

    // NiBASSAudioSystem::Create() must be called manually after this
    // with the application HWND, then Startup() to initialize BASS.

    NiOutputDebugString("NiAudio Initialized\n");
}
//---------------------------------------------------------------------------
void NiAudioSDM::Shutdown()
{
    NiImplementSDMShutdownCheck();

    NiBASSAudioSystem::Destroy();

    NiUnregisterStream(NiAudioListener);
    NiUnregisterStream(NiAudioSource);
    NiUnregisterStream(NiAudioSystem);
}
//---------------------------------------------------------------------------
