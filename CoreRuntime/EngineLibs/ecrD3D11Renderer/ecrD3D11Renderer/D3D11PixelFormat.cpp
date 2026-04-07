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
#include "ecrD3D11RendererPCH.h"

#include "D3D11PixelFormat.h"

using namespace ecr;

NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32B32A32_TYPELESS(
    NiPixelFormat::FORMAT_RGBA, 128, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32B32A32_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_UNKNOWN, 32, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT(
    NiPixelFormat::FORMAT_RGBA, 128, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32B32A32_FLOAT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_FLOAT,   32, true);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32B32A32_UINT(
    NiPixelFormat::FORMAT_RGBA, 128, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32B32A32_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_INT,     32, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32B32A32_SINT(
    NiPixelFormat::FORMAT_RGBA, 128, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32B32A32_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_INT,     32, true);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32B32_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 96, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32B32_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32B32_FLOAT(
    NiPixelFormat::FORMAT_RGB, 96, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32B32_FLOAT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32B32_UINT(
    NiPixelFormat::FORMAT_RGB, 96, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32B32_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32B32_SINT(
    NiPixelFormat::FORMAT_RGB, 96, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32B32_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16B16A16_TYPELESS(
    NiPixelFormat::FORMAT_RGBA, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16B16A16_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 16, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN, 16, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_UNKNOWN, 16, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_UNKNOWN, 16, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT(
    NiPixelFormat::FORMAT_RGBA, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16B16A16_FLOAT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_HALF,    16, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_HALF,    16, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_HALF,    16, true,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_HALF,    16, true);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16B16A16_UNORM(
    NiPixelFormat::FORMAT_RGBA, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16B16A16_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,   16, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,   16, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,   16, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,   16, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16B16A16_UINT(
    NiPixelFormat::FORMAT_RGBA, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16B16A16_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     16, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     16, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,     16, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_INT,     16, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16B16A16_SNORM(
    NiPixelFormat::FORMAT_BUMPLUMA, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16B16A16_SNORM, false,
    NiPixelFormat::COMP_OFFSET_U,   NiPixelFormat::REP_NORM_INT,   16, true,
    NiPixelFormat::COMP_OFFSET_V,   NiPixelFormat::REP_NORM_INT,   16, true,
    NiPixelFormat::COMP_LUMA,       NiPixelFormat::REP_NORM_INT,   16, true,
    NiPixelFormat::COMP_HEIGHT,     NiPixelFormat::REP_NORM_INT,   16, true);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16B16A16_SINT(
    NiPixelFormat::FORMAT_RGBA, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16B16A16_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     16, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     16, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,     16, true,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_INT,     16, true);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32_FLOAT(
    NiPixelFormat::FORMAT_RGB, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32_FLOAT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32_UINT(
    NiPixelFormat::FORMAT_RGB, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G32_SINT(
    NiPixelFormat::FORMAT_RGB, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G32_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32G8X24_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32G8X24_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN, 24, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_D32_FLOAT_S8X24_UINT(
    NiPixelFormat::FORMAT_DEPTH_STENCIL, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT, false,
    NiPixelFormat::COMP_DEPTH,  NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_STENCIL,NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN, 24, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32_FLOAT_X8X24_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN, 24, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_X32_TYPELESS_G8X24_UINT(
    NiPixelFormat::FORMAT_RGB, 64, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN, 24, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R10G10B10A2_TYPELESS(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R10G10B10A2_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 10, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN, 10, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_UNKNOWN, 10, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_UNKNOWN,  2, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R10G10B10A2_UNORM(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R10G10B10A2_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,   10, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,   10, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,   10, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,    2, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R10G10B10A2_UINT(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R10G10B10A2_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     10, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     10, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,     10, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_INT,      2, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R11G11B10_FLOAT(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R11G11B10_FLOAT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_FLOAT,   11, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_FLOAT,   11, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_FLOAT,   10, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8B8A8_TYPELESS(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8B8A8_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_UNKNOWN,  8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8B8A8_UNORM(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8B8A8_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,    8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8B8A8_UNORM_SRGB(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, true,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,    8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8B8A8_UINT(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8B8A8_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_INT,      8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8B8A8_SNORM(
    NiPixelFormat::FORMAT_BUMPLUMA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8B8A8_SNORM, false,
    NiPixelFormat::COMP_OFFSET_U,   NiPixelFormat::REP_NORM_INT,    8, true,
    NiPixelFormat::COMP_OFFSET_V,   NiPixelFormat::REP_NORM_INT,    8, true,
    NiPixelFormat::COMP_LUMA,       NiPixelFormat::REP_NORM_INT,    8, true,
    NiPixelFormat::COMP_HEIGHT,     NiPixelFormat::REP_NORM_INT,    8, true);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8B8A8_SINT(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8B8A8_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,      8, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,      8, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_INT,      8, true,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_INT,      8, true);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 16, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN, 16, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16_FLOAT(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16_FLOAT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_HALF,    16, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_HALF,    16, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16_UNORM(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,   16, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,   16, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16_UINT(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     16, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     16, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16_SNORM(
    NiPixelFormat::FORMAT_BUMP, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16_SNORM, false,
    NiPixelFormat::COMP_OFFSET_U,   NiPixelFormat::REP_NORM_INT,   16, true,
    NiPixelFormat::COMP_OFFSET_V,   NiPixelFormat::REP_NORM_INT,   16, true,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16G16_SINT(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16G16_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     16, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,     16, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 32, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_D32_FLOAT(
    NiPixelFormat::FORMAT_DEPTH_STENCIL, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_D32_FLOAT, false,
    NiPixelFormat::COMP_DEPTH,  NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32_FLOAT(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32_FLOAT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_FLOAT,   32, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32_UINT(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     32, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R32_SINT(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R32_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     32, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R24G8_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R24G8_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 24, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN,  8, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_D24_UNORM_S8_UINT(
    NiPixelFormat::FORMAT_DEPTH_STENCIL, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_D24_UNORM_S8_UINT, false,
    NiPixelFormat::COMP_DEPTH,  NiPixelFormat::REP_NORM_INT,   24, false,
    NiPixelFormat::COMP_STENCIL,NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R24_UNORM_X8_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,   24, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_X24_TYPELESS_G8_UINT(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN, 24, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8_UNORM(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8_UINT(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8_SNORM(
    NiPixelFormat::FORMAT_BUMP, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8_SNORM, false,
    NiPixelFormat::COMP_OFFSET_U,   NiPixelFormat::REP_NORM_INT,    8, true,
    NiPixelFormat::COMP_OFFSET_V,   NiPixelFormat::REP_NORM_INT,    8, true,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8_SINT(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,      8, true,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_INT,      8, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 16, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16_FLOAT(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16_FLOAT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_HALF,    16, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_D16_UNORM(
    NiPixelFormat::FORMAT_DEPTH_STENCIL, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_D16_UNORM, false,
    NiPixelFormat::COMP_DEPTH,  NiPixelFormat::REP_NORM_INT,   16, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16_UNORM(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,   16, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16_UINT(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     16, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16_SNORM(
    NiPixelFormat::FORMAT_BUMP, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16_SNORM, false,
    NiPixelFormat::COMP_OFFSET_U,   NiPixelFormat::REP_NORM_INT,   16, true,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R16_SINT(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R16_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,     16, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8_TYPELESS(
    NiPixelFormat::FORMAT_RGB, 8, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8_TYPELESS, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN,  8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8_UNORM(
    NiPixelFormat::FORMAT_RGB, 8, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8_UINT(
    NiPixelFormat::FORMAT_RGB, 8, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8_UINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,      8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8_SNORM(
    NiPixelFormat::FORMAT_BUMP, 8, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8_SNORM, false,
    NiPixelFormat::COMP_OFFSET_U,   NiPixelFormat::REP_NORM_INT,    8, true,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,      NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8_SINT(
    NiPixelFormat::FORMAT_RGB, 8, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8_SINT, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_INT,      8, true,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_A8_UNORM(
    NiPixelFormat::FORMAT_RGBA, 8, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_A8_UNORM, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R1_UNORM(
    NiPixelFormat::FORMAT_RGB, 1, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R1_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    1, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R9G9B9E5_SHAREDEXP(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R8G8_B8G8_UNORM(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R8G8_B8G8_UNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_G8R8_G8B8_UNORM(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_G8R8_G8B8_UNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC1_TYPELESS(
    NiPixelFormat::FORMAT_DXT1, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC1_TYPELESS, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC1_UNORM(
    NiPixelFormat::FORMAT_DXT1, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC1_UNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC1_UNORM_SRGB(
    NiPixelFormat::FORMAT_DXT1, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC1_UNORM_SRGB, true,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC2_TYPELESS(
    NiPixelFormat::FORMAT_DXT3, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC2_TYPELESS, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC2_UNORM(
    NiPixelFormat::FORMAT_DXT3, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC2_UNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC2_UNORM_SRGB(
    NiPixelFormat::FORMAT_DXT3, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC2_UNORM_SRGB, true,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC3_TYPELESS(
    NiPixelFormat::FORMAT_DXT5, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC3_TYPELESS, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC3_UNORM(
    NiPixelFormat::FORMAT_DXT5, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC3_UNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC3_UNORM_SRGB(
    NiPixelFormat::FORMAT_DXT5, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC3_UNORM_SRGB, true,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC4_TYPELESS(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC4_TYPELESS, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC4_UNORM(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC4_UNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC4_SNORM(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC4_SNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC5_TYPELESS(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC5_TYPELESS, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC5_UNORM(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC5_UNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC5_SNORM(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC5_SNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_B5G6R5_UNORM(
    NiPixelFormat::FORMAT_RGB, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_B5G6R5_UNORM, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,    5, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    6, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    5, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_B5G5R5A1_UNORM(
    NiPixelFormat::FORMAT_RGBA, 16, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_B5G5R5A1_UNORM, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,    5, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    5, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    5, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,    1, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_B8G8R8A8_UNORM(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_B8G8R8A8_UNORM, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,    8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_B8G8R8X8_UNORM(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_B8G8R8X8_UNORM, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN,     8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_R10G10B10_XR_BIAS_A2_UNORM(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN,   10, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN,   10, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_UNKNOWN,   10, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,    2, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_B8G8R8A8_TYPELESS(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_B8G8R8A8_TYPELESS, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_UNKNOWN, 8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN, 8, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 8, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_UNKNOWN, 8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_B8G8R8A8_UNORM_SRGB(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_ALPHA,  NiPixelFormat::REP_NORM_INT,    8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_B8G8R8X8_TYPELESS(
    NiPixelFormat::FORMAT_RGBA, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_B8G8R8X8_TYPELESS, false,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_UNKNOWN, 8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_UNKNOWN, 8, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_UNKNOWN, 8, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN, 8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_B8G8R8X8_UNORM_SRGB(
    NiPixelFormat::FORMAT_RGB, 32, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, true,
    NiPixelFormat::COMP_BLUE,   NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_GREEN,  NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_RED,    NiPixelFormat::REP_NORM_INT,    8, false,
    NiPixelFormat::COMP_PADDING,NiPixelFormat::REP_UNKNOWN,     8, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC6H_TYPELESS(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC6H_TYPELESS, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC6H_UF16(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC6H_UF16, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC6H_SF16(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC6H_SF16, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC7_TYPELESS(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC7_TYPELESS, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC7_UNORM(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC7_UNORM, false,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);
NiPixelFormat D3D11PixelFormat::EE_FORMAT_BC7_UNORM_SRGB(
    NiPixelFormat::FORMAT_RENDERERSPECIFIC, 0, NiPixelFormat::TILE_NONE, true,
    DXGI_FORMAT_BC7_UNORM_SRGB, true,
    NiPixelFormat::COMP_COMPRESSED,NiPixelFormat::REP_COMPRESSED, 0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false,
    NiPixelFormat::COMP_EMPTY,  NiPixelFormat::REP_UNKNOWN,  0, false);

//------------------------------------------------------------------------------------------------
DXGI_FORMAT D3D11PixelFormat::DetermineDXGIFormat(const NiPixelFormat& pixelFormat)
{
    DXGI_FORMAT dxgiFormat = (DXGI_FORMAT)pixelFormat.GetRendererHint();
    if (dxgiFormat != NiPixelFormat::INVALID_RENDERER_HINT)
        return dxgiFormat;

    NiPixelFormat::Format format = pixelFormat.GetFormat();
    efd::UInt8 bpp = pixelFormat.GetBitsPerPixel();
    efd::UInt32 componentCount = pixelFormat.GetNumComponents();

    NiPixelFormat::Component component = NiPixelFormat::COMP_EMPTY;
    NiPixelFormat::Representation rep = NiPixelFormat::REP_UNKNOWN;
    efd::UInt8 bitCount = 0;
    efd::Bool isSigned = false;
    if (!pixelFormat.GetComponent(0, component, rep, bitCount, isSigned))
        return DXGI_FORMAT_UNKNOWN;

    dxgiFormat = DXGI_FORMAT_UNKNOWN;
    switch (format)
    {
    case NiPixelFormat::FORMAT_ONE_CHANNEL:
    case NiPixelFormat::FORMAT_TWO_CHANNEL:
    case NiPixelFormat::FORMAT_THREE_CHANNEL:
    case NiPixelFormat::FORMAT_FOUR_CHANNEL:
    case NiPixelFormat::FORMAT_RGB:
    case NiPixelFormat::FORMAT_RGBA:
    case NiPixelFormat::FORMAT_BUMP:
    case NiPixelFormat::FORMAT_BUMPLUMA:
        switch (bpp)
        {
        case 1:
            dxgiFormat = DXGI_FORMAT_R1_UNORM;
            break;
        case 8:
            if (componentCount == 1)
            {
                if (rep == NiPixelFormat::REP_NORM_INT)
                {
                    if (isSigned)
                    {
                        dxgiFormat = DXGI_FORMAT_R8_SNORM;
                    }
                    else
                    {
                        if (component == NiPixelFormat::COMP_RED)
                            dxgiFormat = DXGI_FORMAT_R8_UNORM;
                        else
                            dxgiFormat = DXGI_FORMAT_A8_UNORM;
                    }
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R8_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R8_UINT;
                }
                else
                {
                    dxgiFormat = DXGI_FORMAT_R8_TYPELESS;
                }
            }
            break;
        case 16:
            if (componentCount == 1)
            {
                if (rep == NiPixelFormat::REP_FLOAT ||
                    rep == NiPixelFormat::REP_HALF)
                {
                    dxgiFormat = DXGI_FORMAT_R16_FLOAT;
                }
                else if (rep == NiPixelFormat::REP_NORM_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R16_SNORM;
                    else
                        dxgiFormat = DXGI_FORMAT_R16_UNORM;
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R16_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R16_UINT;
                }
                else
                {
                    dxgiFormat = DXGI_FORMAT_R16_TYPELESS;
                }
            }
            else if (componentCount == 2)
            {
                if (rep == NiPixelFormat::REP_NORM_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R8G8_SNORM;
                    else
                        dxgiFormat = DXGI_FORMAT_R8G8_UNORM;
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R8G8_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R8G8_UINT;
                }
                else
                {
                    dxgiFormat = DXGI_FORMAT_R8G8_TYPELESS;
                }
                break;
            }
            else if (componentCount == 3)
            {
                dxgiFormat = DXGI_FORMAT_B5G6R5_UNORM;
            }
            else if (componentCount == 4)
            {
                dxgiFormat = DXGI_FORMAT_B5G5R5A1_UNORM;
            }
            break;
        case 32:
            if (bitCount == 11)
            {
                dxgiFormat = DXGI_FORMAT_R11G11B10_FLOAT;
            }
            else if (bitCount == 10)
            {
                if (rep == NiPixelFormat::REP_NORM_INT)
                    dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
                else if (rep == NiPixelFormat::REP_INT)
                    dxgiFormat = DXGI_FORMAT_R10G10B10A2_UINT;
                else if (rep == NiPixelFormat::REP_UNKNOWN)
                {
                    NiPixelFormat::Component tempComponent = NiPixelFormat::COMP_EMPTY;
                    NiPixelFormat::Representation tempRep = NiPixelFormat::REP_UNKNOWN;
                    efd::UInt8 tempBitCount = 0;
                    efd::Bool tempSigned = false;
                    pixelFormat.GetComponent(3, tempComponent, tempRep, tempBitCount, tempSigned);
                    if (tempRep == NiPixelFormat::REP_NORM_INT)
                        dxgiFormat = DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
                    else
                        dxgiFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
                }
            }
            else if (bitCount == 8)
            {
                if (rep == NiPixelFormat::REP_NORM_INT)
                {
                    if (component == NiPixelFormat::COMP_RED)
                    {
                        if (pixelFormat.GetSRGBSpace())
                            dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        else
                            dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                    }
                    else if (component == NiPixelFormat::COMP_OFFSET_U)
                    {
                        dxgiFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
                    }
                    else
                    {
                        if (pixelFormat.GetBits(NiPixelFormat::COMP_ALPHA) != 0)
                        {
                            if (pixelFormat.GetSRGBSpace())
                                dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
                            else
                                dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
                        }
                        else
                        {
                            if (pixelFormat.GetSRGBSpace())
                                dxgiFormat = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
                            else
                                dxgiFormat = DXGI_FORMAT_B8G8R8X8_UNORM;
                        }
                    }
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R8G8B8A8_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UINT;
                }
                else if (rep == NiPixelFormat::REP_UNKNOWN)
                {
                    if (component == NiPixelFormat::COMP_RED)
                    {
                        dxgiFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
                    }
                    else
                    {
                        if (pixelFormat.GetBits(NiPixelFormat::COMP_ALPHA) != 0)
                            dxgiFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
                        else
                            dxgiFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
                    }
                }
            }
            else if (bitCount == 16)
            {
                if (rep == NiPixelFormat::REP_FLOAT ||
                    rep == NiPixelFormat::REP_HALF)
                {
                    dxgiFormat = DXGI_FORMAT_R16G16_FLOAT;
                }
                else if (rep == NiPixelFormat::REP_NORM_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R16G16_SNORM;
                    else
                        dxgiFormat = DXGI_FORMAT_R16G16_UNORM;
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R16G16_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R16G16_UINT;
                }
                else if (rep == NiPixelFormat::REP_UNKNOWN)
                {
                    dxgiFormat = DXGI_FORMAT_R16G16_TYPELESS;
                }
            }
            else if (bitCount == 24)
            {
                if (rep == NiPixelFormat::REP_NORM_INT)
                {
                    dxgiFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                }
                else if (rep == NiPixelFormat::REP_UNKNOWN)
                {
                    if (component == NiPixelFormat::COMP_RED)
                        dxgiFormat = DXGI_FORMAT_R24G8_TYPELESS;
                    else
                        dxgiFormat = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
                }
            }
            else if (bitCount == 32)
            {
                if (rep == NiPixelFormat::REP_FLOAT)
                {
                    dxgiFormat = DXGI_FORMAT_R32_FLOAT;
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R32_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R32_UINT;
                }
                else if (rep == NiPixelFormat::REP_UNKNOWN)
                {
                    dxgiFormat = DXGI_FORMAT_R32_TYPELESS;
                }
            }
            break;
        case 64:
            if (componentCount == 2)
            {
                if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R32G32_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R32G32_UINT;
                }
                else
                {
                    if (rep == NiPixelFormat::REP_FLOAT)
                    {
                        if (pixelFormat.GetBits(NiPixelFormat::COMP_GREEN) != 0)
                            dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
                        else
                            dxgiFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                    }
                    else if (rep == NiPixelFormat::REP_UNKNOWN)
                    {
                        if (component == NiPixelFormat::COMP_RED)
                        {
                            dxgiFormat = DXGI_FORMAT_R32G32_TYPELESS;
                        }
                        else if (component == NiPixelFormat::COMP_PADDING)
                        {
                            dxgiFormat = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
                        }
                    }

                }
            }
            else if (componentCount == 3)
            {
                if (rep == NiPixelFormat::REP_FLOAT)
                {
                    dxgiFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                }
                else if (rep == NiPixelFormat::REP_UNKNOWN)
                {
                    if (pixelFormat.GetBits(NiPixelFormat::COMP_RED) == 32)
                        dxgiFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
                    else
                        dxgiFormat = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
                }
            }
            else if (componentCount == 4)
            {
                if (rep == NiPixelFormat::REP_FLOAT ||
                    rep == NiPixelFormat::REP_HALF)
                {
                    dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
                }
                else if (rep == NiPixelFormat::REP_NORM_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R16G16B16A16_SNORM;
                    else
                        dxgiFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R16G16B16A16_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R16G16B16A16_UINT;
                }
                else if (rep == NiPixelFormat::REP_UNKNOWN)
                {
                    dxgiFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
                }
            }
            break;
        case 96:
            if (componentCount == 3)
            {
                if (rep == NiPixelFormat::REP_FLOAT)
                {
                    dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R32G32B32_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R32G32B32_UINT;
                }
                else
                {
                    dxgiFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
                }
            }
            break;
        case 128:
            if (componentCount == 4)
            {
                if (rep == NiPixelFormat::REP_FLOAT)
                {
                    dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
                }
                else if (rep == NiPixelFormat::REP_INT)
                {
                    if (isSigned)
                        dxgiFormat = DXGI_FORMAT_R32G32B32A32_SINT;
                    else
                        dxgiFormat = DXGI_FORMAT_R32G32B32A32_UINT;
                }
                else
                {
                    dxgiFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
                }
            }
            break;
        }
        break;
    case NiPixelFormat::FORMAT_DXT1:
        if (pixelFormat.GetSRGBSpace())
            dxgiFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
        else
            dxgiFormat = DXGI_FORMAT_BC1_UNORM;
        break;
    case NiPixelFormat::FORMAT_DXT3:
        if (pixelFormat.GetSRGBSpace())
            dxgiFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
        else
            dxgiFormat = DXGI_FORMAT_BC2_UNORM;
        break;
    case NiPixelFormat::FORMAT_DXT5:
        if (pixelFormat.GetSRGBSpace())
            dxgiFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
        else
            dxgiFormat = DXGI_FORMAT_BC3_UNORM;
        break;
    case NiPixelFormat::FORMAT_DEPTH_STENCIL:
        if (bpp == 64)
        {
            dxgiFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        }
        else if (bpp == 32)
        {
            if (pixelFormat.GetBits(NiPixelFormat::COMP_STENCIL) == 0)
                dxgiFormat = DXGI_FORMAT_D32_FLOAT;
            else
                dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        }
        else if (bpp == 16)
        {
            dxgiFormat = DXGI_FORMAT_D16_UNORM;
        }
        break;
    case NiPixelFormat::FORMAT_PAL:
    case NiPixelFormat::FORMAT_PALALPHA:
    case NiPixelFormat::FORMAT_RENDERERSPECIFIC:
    default:
        dxgiFormat = DXGI_FORMAT_UNKNOWN;
        break;
    }

    return dxgiFormat;
}

//------------------------------------------------------------------------------------------------
DXGI_FORMAT D3D11PixelFormat::DetermineDXGIFormat(NiDataStreamElement::Format format)
{
    switch (format)
    {
    case NiDataStreamElement::F_INT8_1:
        return DXGI_FORMAT_R8_SINT;
    case NiDataStreamElement::F_INT8_2:
        return DXGI_FORMAT_R8G8_SINT;
    case NiDataStreamElement::F_INT8_4:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case NiDataStreamElement::F_UINT8_1:
        return DXGI_FORMAT_R8_UINT;
    case NiDataStreamElement::F_UINT8_2:
        return DXGI_FORMAT_R8G8_UINT;
    case NiDataStreamElement::F_UINT8_4:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case NiDataStreamElement::F_NORMINT8_1:
        return DXGI_FORMAT_R8_SNORM;
    case NiDataStreamElement::F_NORMINT8_2:
        return DXGI_FORMAT_R8G8_SNORM;
    case NiDataStreamElement::F_NORMINT8_4:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case NiDataStreamElement::F_NORMUINT8_1:
        return DXGI_FORMAT_R8_UNORM;
    case NiDataStreamElement::F_NORMUINT8_2:
        return DXGI_FORMAT_R8G8_UNORM;
    case NiDataStreamElement::F_NORMUINT8_4:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case NiDataStreamElement::F_INT16_1:
        return DXGI_FORMAT_R16_SINT;
    case NiDataStreamElement::F_INT16_2:
        return DXGI_FORMAT_R16G16_SINT;
    case NiDataStreamElement::F_INT16_4:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case NiDataStreamElement::F_UINT16_1:
        return DXGI_FORMAT_R16_UINT;
    case NiDataStreamElement::F_UINT16_2:
        return DXGI_FORMAT_R16G16_UINT;
    case NiDataStreamElement::F_UINT16_4:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case NiDataStreamElement::F_NORMINT16_1:
        return DXGI_FORMAT_R16_SNORM;
    case NiDataStreamElement::F_NORMINT16_2:
        return DXGI_FORMAT_R16G16_SNORM;
    case NiDataStreamElement::F_NORMINT16_4:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
    case NiDataStreamElement::F_NORMUINT16_1:
        return DXGI_FORMAT_R16_UNORM;
    case NiDataStreamElement::F_NORMUINT16_2:
        return DXGI_FORMAT_R16G16_UNORM;
    case NiDataStreamElement::F_NORMUINT16_4:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case NiDataStreamElement::F_INT32_1:
        return DXGI_FORMAT_R32_SINT;
    case NiDataStreamElement::F_INT32_2:
        return DXGI_FORMAT_R32G32_SINT;
    case NiDataStreamElement::F_INT32_4:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case NiDataStreamElement::F_UINT32_1:
        return DXGI_FORMAT_R32_UINT;
    case NiDataStreamElement::F_UINT32_2:
        return DXGI_FORMAT_R32G32_UINT;
    case NiDataStreamElement::F_UINT32_4:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case NiDataStreamElement::F_FLOAT16_1:
        return DXGI_FORMAT_R16_FLOAT;
    case NiDataStreamElement::F_FLOAT16_2:
        return DXGI_FORMAT_R16G16_FLOAT;
    case NiDataStreamElement::F_FLOAT16_4:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case NiDataStreamElement::F_FLOAT32_1:
        return DXGI_FORMAT_R32_FLOAT;
    case NiDataStreamElement::F_FLOAT32_2:
        return DXGI_FORMAT_R32G32_FLOAT;
    case NiDataStreamElement::F_FLOAT32_3:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case NiDataStreamElement::F_FLOAT32_4:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case NiDataStreamElement::F_UINT_10_10_10_2:
        return DXGI_FORMAT_R10G10B10A2_UINT;
    case NiDataStreamElement::F_UINT_10_10_10_L1:
    case NiDataStreamElement::F_NORMINT_10_10_10_L1:
    case NiDataStreamElement::F_NORMINT_10_10_10_2:
    case NiDataStreamElement::F_INT8_3:
    case NiDataStreamElement::F_UINT8_3:
    case NiDataStreamElement::F_NORMINT8_3:
    case NiDataStreamElement::F_NORMUINT8_3:
    case NiDataStreamElement::F_INT16_3:
    case NiDataStreamElement::F_UINT16_3:
    case NiDataStreamElement::F_NORMINT16_3:
    case NiDataStreamElement::F_NORMUINT16_3:
    case NiDataStreamElement::F_INT32_3:
    case NiDataStreamElement::F_UINT32_3:
    case NiDataStreamElement::F_NORMINT32_1:
    case NiDataStreamElement::F_NORMINT32_2:
    case NiDataStreamElement::F_NORMINT32_3:
    case NiDataStreamElement::F_NORMINT32_4:
    case NiDataStreamElement::F_NORMUINT32_1:
    case NiDataStreamElement::F_NORMUINT32_2:
    case NiDataStreamElement::F_NORMUINT32_3:
    case NiDataStreamElement::F_NORMUINT32_4:
    case NiDataStreamElement::F_FLOAT16_3:
    case NiDataStreamElement::F_NORMINT_11_11_10:
    case NiDataStreamElement::F_NORMUINT8_4_BGRA:
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

//------------------------------------------------------------------------------------------------
 efd::Bool D3D11PixelFormat::IsDXGIFormatSupported(DXGI_FORMAT dxgiFormat)
 {
     // DT33844 Update to refelct downlevel support.
     switch (dxgiFormat)
     {
     case DXGI_FORMAT_B8G8R8X8_UNORM:
         return false;
     }
     return true;
}
//------------------------------------------------------------------------------------------------
 efd::Bool D3D11PixelFormat::DXGIFormatSupportsMipmaps(DXGI_FORMAT dxgiFormat)
 {
     // DT33844 Update to refelct downlevel support.
     switch (dxgiFormat)
     {
     case DXGI_FORMAT_B8G8R8X8_UNORM:
         return false;
     }
     return true;
}
//------------------------------------------------------------------------------------------------
void D3D11PixelFormat::InitFromDXGIFormat(DXGI_FORMAT dxgiFormat, NiPixelFormat& pixelFormat)
{
    // CreateFromDXGIFormat only ever returns an existing NiPixelFormat,
    // and will not create a new one.
    const NiPixelFormat* pPixelFormat = ObtainFromDXGIFormat(dxgiFormat);
    pixelFormat = *pPixelFormat;
}

//------------------------------------------------------------------------------------------------
const NiPixelFormat* D3D11PixelFormat::ObtainFromDXGIFormat(DXGI_FORMAT dxgiFormat)
{
    switch (dxgiFormat)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        return &EE_FORMAT_R32G32B32A32_TYPELESS;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return &EE_FORMAT_R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        return &EE_FORMAT_R32G32B32A32_UINT;
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return &EE_FORMAT_R32G32B32A32_SINT;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
        return &EE_FORMAT_R32G32B32_TYPELESS;
    case DXGI_FORMAT_R32G32B32_FLOAT:
        return &EE_FORMAT_R32G32B32_FLOAT;
    case DXGI_FORMAT_R32G32B32_UINT:
        return &EE_FORMAT_R32G32B32_UINT;
    case DXGI_FORMAT_R32G32B32_SINT:
        return &EE_FORMAT_R32G32B32_SINT;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        return &EE_FORMAT_R16G16B16A16_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return &EE_FORMAT_R16G16B16A16_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return &EE_FORMAT_R16G16B16A16_UNORM;
    case DXGI_FORMAT_R16G16B16A16_UINT:
        return &EE_FORMAT_R16G16B16A16_UINT;
    case DXGI_FORMAT_R16G16B16A16_SNORM:
        return &EE_FORMAT_R16G16B16A16_SNORM;
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return &EE_FORMAT_R16G16B16A16_SINT;
    case DXGI_FORMAT_R32G32_TYPELESS:
        return &EE_FORMAT_R32G32_TYPELESS;
    case DXGI_FORMAT_R32G32_FLOAT:
        return &EE_FORMAT_R32G32_FLOAT;
    case DXGI_FORMAT_R32G32_UINT:
        return &EE_FORMAT_R32G32_UINT;
    case DXGI_FORMAT_R32G32_SINT:
        return &EE_FORMAT_R32G32_SINT;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return &EE_FORMAT_R32G8X24_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return &EE_FORMAT_D32_FLOAT_S8X24_UINT;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        return &EE_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return &EE_FORMAT_X32_TYPELESS_G8X24_UINT;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        return &EE_FORMAT_R10G10B10A2_TYPELESS;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return &EE_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_R10G10B10A2_UINT:
        return &EE_FORMAT_R10G10B10A2_UINT;
    case DXGI_FORMAT_R11G11B10_FLOAT:
        return &EE_FORMAT_R11G11B10_FLOAT;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        return &EE_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return &EE_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return &EE_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_R8G8B8A8_UINT:
        return &EE_FORMAT_R8G8B8A8_UINT;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        return &EE_FORMAT_R8G8B8A8_SNORM;
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return &EE_FORMAT_R8G8B8A8_SINT;
    case DXGI_FORMAT_R16G16_TYPELESS:
        return &EE_FORMAT_R16G16_TYPELESS;
    case DXGI_FORMAT_R16G16_FLOAT:
        return &EE_FORMAT_R16G16_FLOAT;
    case DXGI_FORMAT_R16G16_UNORM:
        return &EE_FORMAT_R16G16_UNORM;
    case DXGI_FORMAT_R16G16_UINT:
        return &EE_FORMAT_R16G16_UINT;
    case DXGI_FORMAT_R16G16_SNORM:
        return &EE_FORMAT_R16G16_SNORM;
    case DXGI_FORMAT_R16G16_SINT:
        return &EE_FORMAT_R16G16_SINT;
    case DXGI_FORMAT_R32_TYPELESS:
        return &EE_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT:
        return &EE_FORMAT_D32_FLOAT;
    case DXGI_FORMAT_R32_FLOAT:
        return &EE_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R32_UINT:
        return &EE_FORMAT_R32_UINT;
    case DXGI_FORMAT_R32_SINT:
        return &EE_FORMAT_R32_SINT;
    case DXGI_FORMAT_R24G8_TYPELESS:
        return &EE_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return &EE_FORMAT_D24_UNORM_S8_UINT;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        return &EE_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return &EE_FORMAT_X24_TYPELESS_G8_UINT;
    case DXGI_FORMAT_R8G8_TYPELESS:
        return &EE_FORMAT_R8G8_TYPELESS;
    case DXGI_FORMAT_R8G8_UNORM:
        return &EE_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_R8G8_UINT:
        return &EE_FORMAT_R8G8_UINT;
    case DXGI_FORMAT_R8G8_SNORM:
        return &EE_FORMAT_R8G8_SNORM;
    case DXGI_FORMAT_R8G8_SINT:
        return &EE_FORMAT_R8G8_SINT;
    case DXGI_FORMAT_R16_TYPELESS:
        return &EE_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R16_FLOAT:
        return &EE_FORMAT_R16_FLOAT;
    case DXGI_FORMAT_D16_UNORM:
        return &EE_FORMAT_D16_UNORM;
    case DXGI_FORMAT_R16_UNORM:
        return &EE_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R16_UINT:
        return &EE_FORMAT_R16_UINT;
    case DXGI_FORMAT_R16_SNORM:
        return &EE_FORMAT_R16_SNORM;
    case DXGI_FORMAT_R16_SINT:
        return &EE_FORMAT_R16_SINT;
    case DXGI_FORMAT_R8_TYPELESS:
        return &EE_FORMAT_R8_TYPELESS;
    case DXGI_FORMAT_R8_UNORM:
        return &EE_FORMAT_R8_UNORM;
    case DXGI_FORMAT_R8_UINT:
        return &EE_FORMAT_R8_UINT;
    case DXGI_FORMAT_R8_SNORM:
        return &EE_FORMAT_R8_SNORM;
    case DXGI_FORMAT_R8_SINT:
        return &EE_FORMAT_R8_SINT;
    case DXGI_FORMAT_A8_UNORM:
        return &EE_FORMAT_A8_UNORM;
    case DXGI_FORMAT_R1_UNORM:
        return &EE_FORMAT_R1_UNORM;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return &EE_FORMAT_R9G9B9E5_SHAREDEXP;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
        return &EE_FORMAT_R8G8_B8G8_UNORM;
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
        return &EE_FORMAT_G8R8_G8B8_UNORM;
    case DXGI_FORMAT_BC1_TYPELESS:
        return &EE_FORMAT_BC1_TYPELESS;
    case DXGI_FORMAT_BC1_UNORM:
        return &EE_FORMAT_BC1_UNORM;
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return &EE_FORMAT_BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_TYPELESS:
        return &EE_FORMAT_BC2_TYPELESS;
    case DXGI_FORMAT_BC2_UNORM:
        return &EE_FORMAT_BC2_UNORM;
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return &EE_FORMAT_BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_TYPELESS:
        return &EE_FORMAT_BC3_TYPELESS;
    case DXGI_FORMAT_BC3_UNORM:
        return &EE_FORMAT_BC3_UNORM;
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return &EE_FORMAT_BC3_UNORM_SRGB;
    case DXGI_FORMAT_BC4_TYPELESS:
        return &EE_FORMAT_BC4_TYPELESS;
    case DXGI_FORMAT_BC4_UNORM:
        return &EE_FORMAT_BC4_UNORM;
    case DXGI_FORMAT_BC4_SNORM:
        return &EE_FORMAT_BC4_SNORM;
    case DXGI_FORMAT_BC5_TYPELESS:
        return &EE_FORMAT_BC5_TYPELESS;
    case DXGI_FORMAT_BC5_UNORM:
        return &EE_FORMAT_BC5_UNORM;
    case DXGI_FORMAT_BC5_SNORM:
        return &EE_FORMAT_BC5_SNORM;
    case DXGI_FORMAT_B5G6R5_UNORM:
        return &EE_FORMAT_B5G6R5_UNORM;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return &EE_FORMAT_B5G5R5A1_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return &EE_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return &EE_FORMAT_B8G8R8X8_UNORM;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        return &EE_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        return &EE_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return &EE_FORMAT_B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        return &EE_FORMAT_B8G8R8X8_TYPELESS;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return &EE_FORMAT_B8G8R8X8_UNORM_SRGB;
    case DXGI_FORMAT_BC6H_TYPELESS:
        return &EE_FORMAT_BC6H_TYPELESS;
    case DXGI_FORMAT_BC6H_UF16:
        return &EE_FORMAT_BC6H_UF16;
    case DXGI_FORMAT_BC6H_SF16:
        return &EE_FORMAT_BC6H_SF16;
    case DXGI_FORMAT_BC7_TYPELESS:
        return &EE_FORMAT_BC7_TYPELESS;
    case DXGI_FORMAT_BC7_UNORM:
        return &EE_FORMAT_BC7_UNORM;
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return &EE_FORMAT_BC7_UNORM_SRGB;
    case DXGI_FORMAT_UNKNOWN:
    default:
        return NULL;
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11PixelFormat::GetBitsPerPixel(DXGI_FORMAT dxgiFormat)
{
    switch (dxgiFormat)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return 64;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return 32;
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return 16;
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
        return 8;
    case DXGI_FORMAT_R1_UNORM:
        return 1;
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 0;
    case DXGI_FORMAT_UNKNOWN:
    default:
        return UINT_MAX;
    }
}

//------------------------------------------------------------------------------------------------
const efd::Char* const D3D11PixelFormat::GetFormatName(DXGI_FORMAT dxgiFormat, bool withPrefix)
{
    const efd::Char* pStr = NULL;
    switch (dxgiFormat)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            pStr = "DXGI_FORMAT_R32G32B32A32_TYPELESS";
            break;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            pStr = "DXGI_FORMAT_R32G32B32A32_FLOAT";
            break;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            pStr = "DXGI_FORMAT_R32G32B32A32_UINT";
            break;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            pStr = "DXGI_FORMAT_R32G32B32A32_SINT";
            break;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            pStr = "DXGI_FORMAT_R32G32B32_TYPELESS";
            break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            pStr = "DXGI_FORMAT_R32G32B32_FLOAT";
            break;
        case DXGI_FORMAT_R32G32B32_UINT:
            pStr = "DXGI_FORMAT_R32G32B32_UINT";
            break;
        case DXGI_FORMAT_R32G32B32_SINT:
            pStr = "DXGI_FORMAT_R32G32B32_SINT";
            break;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            pStr = "DXGI_FORMAT_R16G16B16A16_TYPELESS";
            break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            pStr = "DXGI_FORMAT_R16G16B16A16_FLOAT";
            break;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            pStr = "DXGI_FORMAT_R16G16B16A16_UNORM";
            break;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            pStr = "DXGI_FORMAT_R16G16B16A16_UINT";
            break;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            pStr = "DXGI_FORMAT_R16G16B16A16_SNORM";
            break;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            pStr = "DXGI_FORMAT_R16G16B16A16_SINT";
            break;
        case DXGI_FORMAT_R32G32_TYPELESS:
            pStr = "DXGI_FORMAT_R32G32_TYPELESS";
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            pStr = "DXGI_FORMAT_R32G32_FLOAT";
            break;
        case DXGI_FORMAT_R32G32_UINT:
            pStr = "DXGI_FORMAT_R32G32_UINT";
            break;
        case DXGI_FORMAT_R32G32_SINT:
            pStr = "DXGI_FORMAT_R32G32_SINT";
            break;
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            pStr = "DXGI_FORMAT_R32G8X24_TYPELESS";
            break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            pStr = "DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
            break;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            pStr = "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
            break;
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            pStr = "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";
            break;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            pStr = "DXGI_FORMAT_R10G10B10A2_TYPELESS";
            break;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            pStr = "DXGI_FORMAT_R10G10B10A2_UNORM";
            break;
        case DXGI_FORMAT_R10G10B10A2_UINT:
            pStr = "DXGI_FORMAT_R10G10B10A2_UINT";
            break;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            pStr = "DXGI_FORMAT_R11G11B10_FLOAT";
            break;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            pStr = "DXGI_FORMAT_R8G8B8A8_TYPELESS";
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            pStr = "DXGI_FORMAT_R8G8B8A8_UNORM";
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            pStr = "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
            break;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            pStr = "DXGI_FORMAT_R8G8B8A8_UINT";
            break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            pStr = "DXGI_FORMAT_R8G8B8A8_SNORM";
            break;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            pStr = "DXGI_FORMAT_R8G8B8A8_SINT";
            break;
        case DXGI_FORMAT_R16G16_TYPELESS:
            pStr = "DXGI_FORMAT_R16G16_TYPELESS";
            break;
        case DXGI_FORMAT_R16G16_FLOAT:
            pStr = "DXGI_FORMAT_R16G16_FLOAT";
            break;
        case DXGI_FORMAT_R16G16_UNORM:
            pStr = "DXGI_FORMAT_R16G16_UNORM";
            break;
        case DXGI_FORMAT_R16G16_UINT:
            pStr = "DXGI_FORMAT_R16G16_UINT";
            break;
        case DXGI_FORMAT_R16G16_SNORM:
            pStr = "DXGI_FORMAT_R16G16_SNORM";
            break;
        case DXGI_FORMAT_R16G16_SINT:
            pStr = "DXGI_FORMAT_R16G16_SINT";
            break;
        case DXGI_FORMAT_R32_TYPELESS:
            pStr = "DXGI_FORMAT_R32_TYPELESS";
            break;
        case DXGI_FORMAT_D32_FLOAT:
            pStr = "DXGI_FORMAT_D32_FLOAT";
            break;
        case DXGI_FORMAT_R32_FLOAT:
            pStr = "DXGI_FORMAT_R32_FLOAT";
            break;
        case DXGI_FORMAT_R32_UINT:
            pStr = "DXGI_FORMAT_R32_UINT";
            break;
        case DXGI_FORMAT_R32_SINT:
            pStr = "DXGI_FORMAT_R32_SINT";
            break;
        case DXGI_FORMAT_R24G8_TYPELESS:
            pStr = "DXGI_FORMAT_R24G8_TYPELESS";
            break;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            pStr = "DXGI_FORMAT_D24_UNORM_S8_UINT";
            break;
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            pStr = "DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
            break;
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            pStr = "DXGI_FORMAT_X24_TYPELESS_G8_UINT";
            break;
        case DXGI_FORMAT_R8G8_TYPELESS:
            pStr = "DXGI_FORMAT_R8G8_TYPELESS";
            break;
        case DXGI_FORMAT_R8G8_UNORM:
            pStr = "DXGI_FORMAT_R8G8_UNORM";
            break;
        case DXGI_FORMAT_R8G8_UINT:
            pStr = "DXGI_FORMAT_R8G8_UINT";
            break;
        case DXGI_FORMAT_R8G8_SNORM:
            pStr = "DXGI_FORMAT_R8G8_SNORM";
            break;
        case DXGI_FORMAT_R8G8_SINT:
            pStr = "DXGI_FORMAT_R8G8_SINT";
            break;
        case DXGI_FORMAT_R16_TYPELESS:
            pStr = "DXGI_FORMAT_R16_TYPELESS";
            break;
        case DXGI_FORMAT_R16_FLOAT:
            pStr = "DXGI_FORMAT_R16_FLOAT";
            break;
        case DXGI_FORMAT_D16_UNORM:
            pStr = "DXGI_FORMAT_D16_UNORM";
            break;
        case DXGI_FORMAT_R16_UNORM:
            pStr = "DXGI_FORMAT_R16_UNORM";
            break;
        case DXGI_FORMAT_R16_UINT:
            pStr = "DXGI_FORMAT_R16_UINT";
            break;
        case DXGI_FORMAT_R16_SNORM:
            pStr = "DXGI_FORMAT_R16_SNORM";
            break;
        case DXGI_FORMAT_R16_SINT:
            pStr = "DXGI_FORMAT_R16_SINT";
            break;
        case DXGI_FORMAT_R8_TYPELESS:
            pStr = "DXGI_FORMAT_R8_TYPELESS";
            break;
        case DXGI_FORMAT_R8_UNORM:
            pStr = "DXGI_FORMAT_R8_UNORM";
            break;
        case DXGI_FORMAT_R8_UINT:
            pStr = "DXGI_FORMAT_R8_UINT";
            break;
        case DXGI_FORMAT_R8_SNORM:
            pStr = "DXGI_FORMAT_R8_SNORM";
            break;
        case DXGI_FORMAT_R8_SINT:
            pStr = "DXGI_FORMAT_R8_SINT";
            break;
        case DXGI_FORMAT_A8_UNORM:
            pStr = "DXGI_FORMAT_A8_UNORM";
            break;
        case DXGI_FORMAT_R1_UNORM:
            pStr = "DXGI_FORMAT_R1_UNORM";
            break;
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            pStr = "DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
            break;
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
            pStr = "DXGI_FORMAT_R8G8_B8G8_UNORM";
            break;
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
            pStr = "DXGI_FORMAT_G8R8_G8B8_UNORM";
            break;
        case DXGI_FORMAT_BC1_TYPELESS:
            pStr = "DXGI_FORMAT_BC1_TYPELESS";
            break;
        case DXGI_FORMAT_BC1_UNORM:
            pStr = "DXGI_FORMAT_BC1_UNORM";
            break;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            pStr = "DXGI_FORMAT_BC1_UNORM_SRGB";
            break;
        case DXGI_FORMAT_BC2_TYPELESS:
            pStr = "DXGI_FORMAT_BC2_TYPELESS";
            break;
        case DXGI_FORMAT_BC2_UNORM:
            pStr = "DXGI_FORMAT_BC2_UNORM";
            break;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            pStr = "DXGI_FORMAT_BC2_UNORM_SRGB";
            break;
        case DXGI_FORMAT_BC3_TYPELESS:
            pStr = "DXGI_FORMAT_BC3_TYPELESS";
            break;
        case DXGI_FORMAT_BC3_UNORM:
            pStr = "DXGI_FORMAT_BC3_UNORM";
            break;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            pStr = "DXGI_FORMAT_BC3_UNORM_SRGB";
            break;
        case DXGI_FORMAT_BC4_TYPELESS:
            pStr = "DXGI_FORMAT_BC4_TYPELESS";
            break;
        case DXGI_FORMAT_BC4_UNORM:
            pStr = "DXGI_FORMAT_BC4_UNORM";
            break;
        case DXGI_FORMAT_BC4_SNORM:
            pStr = "DXGI_FORMAT_BC4_SNORM";
            break;
        case DXGI_FORMAT_BC5_TYPELESS:
            pStr = "DXGI_FORMAT_BC5_TYPELESS";
            break;
        case DXGI_FORMAT_BC5_UNORM:
            pStr = "DXGI_FORMAT_BC5_UNORM";
            break;
        case DXGI_FORMAT_BC5_SNORM:
            pStr = "DXGI_FORMAT_BC5_SNORM";
            break;
        case DXGI_FORMAT_B5G6R5_UNORM:
            pStr = "DXGI_FORMAT_B5G6R5_UNORM";
            break;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            pStr = "DXGI_FORMAT_B5G5R5A1_UNORM";
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            pStr = "DXGI_FORMAT_B8G8R8A8_UNORM";
            break;
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            pStr = "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
            break;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            pStr = "DXGI_FORMAT_B8G8R8A8_TYPELESS";
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            pStr = "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
            break;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            pStr = "DXGI_FORMAT_B8G8R8X8_TYPELESS";
            break;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            pStr = "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";
            break;
        case DXGI_FORMAT_BC6H_TYPELESS:
            pStr = "DXGI_FORMAT_BC6H_TYPELESS";
            break;
        case DXGI_FORMAT_BC6H_UF16:
            pStr = "DXGI_FORMAT_BC6H_UF16";
            break;
        case DXGI_FORMAT_BC6H_SF16:
            pStr = "DXGI_FORMAT_BC6H_SF16";
            break;
        case DXGI_FORMAT_BC7_TYPELESS:
            pStr = "DXGI_FORMAT_BC7_TYPELESS";
            break;
        case DXGI_FORMAT_BC7_UNORM:
            pStr = "DXGI_FORMAT_BC7_UNORM";
            break;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            pStr = "DXGI_FORMAT_BC7_UNORM_SRGB";
            break;
        default:
            pStr = "Unknown format";
            break;
    }

    if (withPrefix || strstr(pStr, "DXGI_FORMAT_") == NULL)
        return pStr;
    else
        return pStr + efd::Strlen("DXGI_FORMAT_");
}

//------------------------------------------------------------------------------------------------
