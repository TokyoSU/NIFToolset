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

#include "efdPCH.h"

#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

using namespace efd;

const efd::UInt8 efd::ILogger::kERR0 = 0;
const efd::UInt8 efd::ILogger::kERR1 = 1;
const efd::UInt8 efd::ILogger::kERR2 = 2;
const efd::UInt8 efd::ILogger::kERR3 = 3;
const efd::UInt8 efd::ILogger::kLVL0 = 4;
const efd::UInt8 efd::ILogger::kLVL1 = 5;
const efd::UInt8 efd::ILogger::kLVL2 = 6;
const efd::UInt8 efd::ILogger::kLVL3 = 7;
const efd::UInt8 efd::ILogger::kUknownLevel = EE_UINT8_MAX;
const efd::UInt8 efd::ILogger::kLogMask_Err0 = 1 << efd::ILogger::kERR0;
const efd::UInt8 efd::ILogger::kLogMask_Err1 = 1 << efd::ILogger::kERR1;
const efd::UInt8 efd::ILogger::kLogMask_Err2 = 1 << efd::ILogger::kERR2;
const efd::UInt8 efd::ILogger::kLogMask_Err3 = 1 << efd::ILogger::kERR3;
const efd::UInt8 efd::ILogger::kLogMask_Lvl0 = 1 << efd::ILogger::kLVL0;
const efd::UInt8 efd::ILogger::kLogMask_Lvl1 = 1 << efd::ILogger::kLVL1;
const efd::UInt8 efd::ILogger::kLogMask_Lvl2 = 1 << efd::ILogger::kLVL2;
const efd::UInt8 efd::ILogger::kLogMask_Lvl3 = 1 << efd::ILogger::kLVL3;

const efd::UInt8 efd::ILogger::kLogMask_UptoErr1 =
    efd::ILogger::kLogMask_Err0 | efd::ILogger::kLogMask_Err1;
const efd::UInt8 efd::ILogger::kLogMask_UptoErr2 =
    efd::ILogger::kLogMask_UptoErr1 | efd::ILogger::kLogMask_Err2;
const efd::UInt8 efd::ILogger::kLogMask_UptoErr3 =
    efd::ILogger::kLogMask_UptoErr2 | efd::ILogger::kLogMask_Err3;
const efd::UInt8 efd::ILogger::kLogMask_UptoLvl1 =
    efd::ILogger::kLogMask_Lvl0 | efd::ILogger::kLogMask_Lvl1;
const efd::UInt8 efd::ILogger::kLogMask_UptoLvl2 =
    efd::ILogger::kLogMask_UptoLvl1 | efd::ILogger::kLogMask_Lvl2;
const efd::UInt8 efd::ILogger::kLogMask_UptoLvl3 =
    efd::ILogger::kLogMask_UptoLvl2 | efd::ILogger::kLogMask_Lvl3;
const efd::UInt8 efd::ILogger::kLogMask_All =
    efd::ILogger::kLogMask_UptoErr3 | efd::ILogger::kLogMask_UptoLvl3;
const efd::UInt8 efd::ILogger::kLogMask_None = 0;
