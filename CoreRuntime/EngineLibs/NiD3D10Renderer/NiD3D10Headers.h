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
#include <D3DX10.h>

// Rather than using NiGPUProgram::PROGRAM_MAX, which extends beyond the shaders supported
// by D3D10, define a max shader program count here.
const unsigned int g_uiMaxSupportedProgramTypes = 3;

#endif // #ifndef NID3D10HEADERS_H
