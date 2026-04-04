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

#include "NiSettingsDialogPCH.h"

#include "NiBaseRendererSetup.h"

NiTPointerList<NiBaseRendererSetup*> NiBaseRendererSetup::ms_kRendererSetupList;

//--------------------------------------------------------------------------------------------------
NiRendererPtr NiBaseRendererSetup::CreateRenderer(
    NiRendererSettings* pkRendererSettings,
    NiWindowRef pkWndDevice,
    NiWindowRef pkWndFocus)
{
    NiTListIterator kIter = 0;
    NiBaseRendererSetup* pkRendererSetup = GetFirstRendererSetup(kIter);
    while (pkRendererSetup)
    {
        if (pkRendererSetup->GetRendererID() == pkRendererSettings->m_eRendererID)
        {
            return pkRendererSetup->CreatePlatformRenderer(
                pkRendererSettings, 
                pkWndDevice, 
                pkWndFocus);
        }
        pkRendererSetup = GetNextRendererSetup(kIter);
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
