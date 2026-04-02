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
#include "NiSystemPCH.h"

#include "NiMessageBox.h"
#include "NiOS.h"

MessageBoxFunction NiMessageBoxUtilities::ms_pfnMessageBox =
    NiMessageBoxUtilities::DefaultMessageBox;

//--------------------------------------------------------------------------------------------------
unsigned int NiMessageBoxUtilities::DefaultMessageBox(const char* pcText,
    const char* pcCaption, void*)
{
    MessageBox(NULL, pcText, pcCaption, MB_OK | MB_TOPMOST);
    return 0;
}

//--------------------------------------------------------------------------------------------------
