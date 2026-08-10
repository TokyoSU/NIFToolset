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
#ifndef EE_D3D11HEADERS_H
#define EE_D3D11HEADERS_H

/** @file D3D11Headers.h
    This header file contains the various D3D11 header files needed by the ecrD3D11Renderer library.
*/

#include <efd/OS.h>

#include <wincodec.h>

// Include so _IID_ID3D11ShaderReflection is defined.
#include <initguid.h>

#include <D3D11.h>
#include <D3DCompiler.h>
#include <DXGI.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <ScreenGrab.h>
using namespace DirectX;


// Windows SDK intsafe.h defines aliases such as
//     #define Int32ToUInt16 IntToUShort
// These aliases also expand qualified identifiers, turning
// efd::Int32ToUInt16 into the nonexistent efd::IntToUShort. DirectXTK and
// DirectXTex can pull intsafe.h in indirectly, so remove only the aliases that
// collide with the long-standing EFD safe-cast helpers after all DirectX
// headers have been included.
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

#endif // EE_D3D11HEADERS_H
