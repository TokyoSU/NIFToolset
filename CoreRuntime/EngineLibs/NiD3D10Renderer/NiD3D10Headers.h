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

#pragma once
#ifndef NID3D10HEADERS_H
#define NID3D10HEADERS_H

#include <D3D10_1.h>
#include <D3D10.h>
#include <DXGI.h>
#include <wincodec.h>
#include <D3DCompiler.h>
#include <DirectXTex.h>

#include "../NiBgfxRenderer/NiBgfxMath.h"

// Rather than using NiGPUProgram::PROGRAM_MAX, which extends beyond the shaders supported
// by D3D10, define a max shader program count here.
const unsigned int g_uiMaxSupportedProgramTypes = 3;

// Windows SDK intsafe.h defines aliases such as
//     #define Int32ToUInt16 IntToUShort
// These aliases also expand qualified identifiers, turning
// efd::Int32ToUInt16 into the nonexistent efd::IntToUShort. DirectXTex can
// pull intsafe.h in indirectly, so remove only the aliases that collide with
// the long-standing EFD safe-cast helpers after all DirectX headers have been
// included.
#ifdef Int32ToUInt8
#undef Int32ToUInt8
#endif
#ifdef Int32ToUInt16
#undef Int32ToUInt16
#endif
#ifdef Int32ToUInt32
#undef Int32ToUInt32
#endif
#ifdef Int32ToInt8
#undef Int32ToInt8
#endif
#ifdef Int32ToInt16
#undef Int32ToInt16
#endif

#endif // #ifndef NID3D10HEADERS_H
